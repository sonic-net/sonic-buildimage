use std::collections::HashMap;
use std::fs;
use std::path::{Path, PathBuf};

use swss_common::DbConnector;
use yaml_rust2::{Yaml, YamlLoader};


#[derive(thiserror::Error, Debug)]
pub enum DeviceInfoError {
    #[error("SwSS common error")]
    SwSS(#[from] swss_common::Exception),
}

pub type Result<T> = std::result::Result<T, DeviceInfoError>;

// Trait for database operations to enable dependency injection
pub trait StateDBTrait {
    fn hget(&self, key: &str, field: &str) -> std::result::Result<Option<swss_common::CxxString>, swss_common::Exception>;
}

// Production implementation
pub struct StateDBConnector {
    db: DbConnector,
}

impl StateDBConnector {
    pub fn new() -> std::result::Result<Self, swss_common::Exception> {
        let db = DbConnector::new_named("STATE_DB", true, 0)?;
        Ok(StateDBConnector { db })
    }

    pub fn new_with_namespace(namespace: &str) -> std::result::Result<Self, swss_common::Exception> {
        let db = DbConnector::new_keyed("STATE_DB", false, 0, "", namespace)?;
        Ok(StateDBConnector { db })
    }
}

impl StateDBTrait for StateDBConnector {
    fn hget(&self, key: &str, field: &str) -> std::result::Result<Option<swss_common::CxxString>, swss_common::Exception> {
        self.db.hget(key, field)
    }
}

// Internal function that accepts a database connector for testing
pub fn is_warm_restart_enabled_with_db<T: StateDBTrait>(
    container_name: &str,
    state_db: &T,
) -> Result<bool> {
    let table_name_separator = "|";
    let prefix = format!("WARM_RESTART_ENABLE_TABLE{}", table_name_separator);

    let system_key = format!("{}system", prefix);
    let wr_system_state = state_db.hget(&system_key, "enable")?;
    let mut wr_enable_state = wr_system_state.as_deref().map_or(false, |s| s == "true");

    let container_key = format!("{}{}", prefix, container_name);
    let wr_container_state = state_db.hget(&container_key, "enable")?;
    wr_enable_state |= wr_container_state.as_deref().map_or(false, |s| s == "true");

    Ok(wr_enable_state)
}

pub fn is_fast_reboot_enabled_with_db<T: StateDBTrait>(state_db: &T) -> Result<bool> {
    let table_name_separator = "|";
    let prefix = format!("FAST_RESTART_ENABLE_TABLE{}", table_name_separator);

    let system_key = format!("{}system", prefix);
    let fb_system_state = state_db.hget(&system_key, "enable")?;
    let fb_enable_state = fb_system_state.as_deref().map_or(false, |s| s == "true");

    Ok(fb_enable_state)
}

// Public API functions using production database
pub fn is_warm_restart_enabled(container_name: &str) -> Result<bool> {
    let state_db = StateDBConnector::new()
        .map_err(|e| DeviceInfoError::SwSS(e))?;
    is_warm_restart_enabled_with_db(container_name, &state_db)
}

pub fn is_warm_restart_enabled_in_namespace(container_name: &str, namespace: &str) -> Result<bool> {
    let state_db = StateDBConnector::new_with_namespace(namespace)
        .map_err(|e| DeviceInfoError::SwSS(e))?;
    is_warm_restart_enabled_with_db(container_name, &state_db)
}

pub fn is_fast_reboot_enabled() -> Result<bool> {
    let state_db = StateDBConnector::new()
        .map_err(|e| DeviceInfoError::SwSS(e))?;
    is_fast_reboot_enabled_with_db(&state_db)
}

pub fn is_fast_reboot_enabled_in_namespace(namespace: &str) -> Result<bool> {
    let state_db = StateDBConnector::new_with_namespace(namespace)
        .map_err(|e| DeviceInfoError::SwSS(e))?;
    is_fast_reboot_enabled_with_db(&state_db)
}

const HOST_DEVICE_PATH: &str = "usr/share/sonic/device";
const CONTAINER_PLATFORM_PATH: &str = "usr/share/sonic/platform";
const MACHINE_CONF_PATH: &str = "host/machine.conf";
const SONIC_VERSION_YAML_PATH: &str = "etc/sonic/sonic_version.yml";
const ASIC_CONF_FILENAME: &str = "asic.conf";
const PLATFORM_ENV_CONF_FILENAME: &str = "platform_env.conf";
const CHASSIS_DB_CONF_FILENAME: &str = "chassisdb.conf";
const EXPECTED_ASIC_LIST_FILENAME: &str = "platform_expected_asic_list.conf";
const PLATFORM_JSON_FILE: &str = "platform.json";
const HWSKU_JSON_FILE: &str = "hwsku.json";
const PORT_CONFIG_FILE: &str = "port_config.ini";

/// File-based device information, as read by sonic_py_common.device_info.
///
/// All paths are resolved under `root` ("/" on a device), so tests can use a temporary directory.
#[derive(Clone, Debug)]
pub struct DeviceInfo {
    root: PathBuf,
    platform_override: Option<String>,
}

impl Default for DeviceInfo {
    fn default() -> Self {
        Self::new()
    }
}

impl DeviceInfo {
    pub fn new() -> Self {
        Self {
            root: PathBuf::from("/"),
            platform_override: std::env::var("PLATFORM").ok().filter(|p| !p.is_empty()),
        }
    }

    pub fn with_root(root: impl Into<PathBuf>) -> Self {
        Self {
            root: root.into(),
            platform_override: None,
        }
    }

    pub fn root(&self) -> &Path {
        &self.root
    }

    fn path(&self, relative: &str) -> PathBuf {
        self.root.join(relative)
    }

    /// Key/value pairs of /host/machine.conf.
    pub fn machine_info(&self) -> Option<HashMap<String, String>> {
        let content = fs::read_to_string(self.path(MACHINE_CONF_PATH)).ok()?;
        Some(parse_conf(&content).into_iter().collect())
    }

    /// Platform identifier from the PLATFORM environment variable or machine.conf.
    pub fn platform(&self) -> Option<String> {
        if let Some(platform) = &self.platform_override {
            return Some(platform.clone());
        }
        let machine_info = self.machine_info()?;
        machine_info
            .get("onie_platform")
            .or_else(|| machine_info.get("aboot_platform"))
            .cloned()
    }

    /// /usr/share/sonic/device/<platform>
    pub fn host_platform_dir(&self) -> Option<PathBuf> {
        self.platform().map(|p| self.path(HOST_DEVICE_PATH).join(p))
    }

    /// The platform directory: /usr/share/sonic/platform if present, else the host device directory.
    pub fn platform_dir(&self) -> Option<PathBuf> {
        let container_path = self.path(CONTAINER_PLATFORM_PATH);
        if container_path.is_dir() {
            return Some(container_path);
        }
        self.host_platform_dir().filter(|p| p.is_dir())
    }

    fn platform_file(&self, file_name: &str) -> Option<PathBuf> {
        let candidates = [
            Some(self.path(CONTAINER_PLATFORM_PATH).join(file_name)),
            self.host_platform_dir().map(|p| p.join(file_name)),
        ];
        candidates.into_iter().flatten().find(|p| p.is_file())
    }

    /// Every value of `key` (matched case-insensitively) in a platform file, in file order.
    fn platform_conf_values(&self, file_name: &str, key: &str) -> Vec<String> {
        let Some(content) = self
            .platform_file(file_name)
            .and_then(|p| fs::read_to_string(p).ok())
        else {
            return Vec::new();
        };
        parse_conf(&content)
            .into_iter()
            .filter(|(k, _)| k.to_lowercase() == key)
            .map(|(_, v)| v)
            .collect()
    }

    fn platform_conf_value(&self, file_name: &str, key: &str) -> Option<String> {
        self.platform_conf_values(file_name, key).into_iter().next()
    }

    /// NUM_ASIC from asic.conf, its last entry as Python reads it; 1 when the file or the entry is missing.
    pub fn num_asics(&self) -> u32 {
        self.platform_conf_values(ASIC_CONF_FILENAME, "num_asic")
            .pop()
            .and_then(|v| v.parse().ok())
            .unwrap_or(1)
    }

    pub fn is_multi_asic(&self) -> bool {
        self.num_asics() > 1
    }

    pub fn is_supervisor(&self) -> bool {
        self.platform_conf_value(PLATFORM_ENV_CONF_FILENAME, "supervisor")
            .as_deref()
            == Some("1")
    }

    pub fn is_disaggregated_chassis(&self) -> bool {
        // Python matches this key case-sensitively, unlike "supervisor".
        let Some(path) = self.platform_file(PLATFORM_ENV_CONF_FILENAME) else {
            return false;
        };
        let content = fs::read_to_string(path).unwrap_or_default();
        parse_conf(&content)
            .into_iter()
            .any(|(k, v)| k == "disaggregated_chassis" && v == "1")
    }

    pub fn is_macsec_supported(&self) -> bool {
        self.platform_conf_value(PLATFORM_ENV_CONF_FILENAME, "macsec_enabled")
            .and_then(|v| v.parse::<i64>().ok())
            .is_some_and(|v| v != 0)
    }

    pub fn is_chassis_config_absent(&self) -> bool {
        self.platform_file(CHASSIS_DB_CONF_FILENAME).is_none()
    }

    /// ASIC ids listed in platform_expected_asic_list.conf (supervisor cards only); quoted ids count, as Python's
    /// int() takes them.
    pub fn expected_asic_list(&self) -> Vec<u32> {
        let Some(path) = self.platform_file(EXPECTED_ASIC_LIST_FILENAME) else {
            return Vec::new();
        };
        load_yaml(&path)
            .and_then(|list| {
                list.as_vec()?
                    .iter()
                    .map(|id| {
                        id.as_i64()
                            .or_else(|| id.as_str()?.trim().parse().ok())
                            .and_then(|id| u32::try_from(id).ok())
                    })
                    .collect()
            })
            .unwrap_or_default()
    }

    /// `asic_type` from /etc/sonic/sonic_version.yml.
    pub fn asic_type(&self) -> Option<String> {
        load_yaml(&self.path(SONIC_VERSION_YAML_PATH))?["asic_type"]
            .as_str()
            .map(str::to_string)
    }

    /// Port configuration file of the hwsku, as sonic_py_common.device_info.get_path_to_port_config_file.
    pub fn port_config_file(&self, hwsku: &str, asic: Option<&str>) -> Option<PathBuf> {
        self.platform()?;
        let platform_path = self.platform_dir()?;
        let hwsku_path = platform_path.join(hwsku);
        let mut candidates = Vec::new();
        if hwsku_path.join(HWSKU_JSON_FILE).is_file() {
            let platform_json = platform_path.join(PLATFORM_JSON_FILE);
            let has_interfaces = fs::read_to_string(&platform_json)
                .ok()
                .and_then(|s| serde_json::from_str::<serde_json::Value>(&s).ok())
                .and_then(|v| v.get("interfaces").cloned())
                .is_some_and(|i| match i {
                    serde_json::Value::Object(m) => !m.is_empty(),
                    serde_json::Value::Array(a) => !a.is_empty(),
                    serde_json::Value::String(s) => !s.is_empty(),
                    _ => false,
                });
            if has_interfaces {
                candidates.push(platform_json);
            }
        }
        match asic {
            Some(asic) => candidates.push(hwsku_path.join(asic).join(PORT_CONFIG_FILE)),
            None => candidates.push(hwsku_path.join(PORT_CONFIG_FILE)),
        }
        candidates.into_iter().find(|p| p.is_file())
    }
}

/// `key=value` lines, split on '=' as Python's `line.split('=')`: the key as is, the value stripped.
fn parse_conf(content: &str) -> Vec<(String, String)> {
    content
        .lines()
        .filter_map(|line| {
            let mut tokens = line.split('=');
            let key = tokens.next()?;
            let value = tokens.next()?;
            Some((key.to_string(), value.trim().to_string()))
        })
        .collect()
}

/// The first document of a YAML file.
fn load_yaml(path: &Path) -> Option<Yaml> {
    let content = fs::read_to_string(path).ok()?;
    YamlLoader::load_from_str(&content).ok()?.into_iter().next()
}
