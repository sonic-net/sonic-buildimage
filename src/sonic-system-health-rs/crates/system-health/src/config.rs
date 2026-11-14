//! system_health_monitoring_config.json, reloaded when it changes.

use std::fs;
use std::os::unix::fs::MetadataExt;
use std::path::{Path, PathBuf};
use std::time::Duration;

use indexmap::IndexMap;
use serde_json::Value;
use sonic_rs_common::DeviceInfo;

use crate::db::TableData;

pub const CONFIG_FILE: &str = "system_health_monitoring_config.json";
pub const MONIT_CONFIG_FILE: &str = "/etc/monit/monitrc";
pub const DEFAULT_INTERVAL: f64 = 60.0;
pub const DEFAULT_BOOTUP_TIMEOUT: f64 = 300.0;
const MONIT_START_DELAY_CONFIG: &str = "with start delay";

/// Seconds from the configuration as a Duration: below zero is zero, too many for a Duration is the longest one.
pub fn seconds(secs: f64) -> Duration {
    Duration::try_from_secs_f64(secs.max(0.0)).unwrap_or(Duration::MAX)
}

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
struct FileStamp {
    dev: u64,
    ino: u64,
    size: u64,
    mtime: (i64, i64),
    ctime: (i64, i64),
}

impl FileStamp {
    fn of(path: &Path) -> Option<Self> {
        let m = fs::metadata(path).ok()?;
        Some(Self {
            dev: m.dev(),
            ino: m.ino(),
            size: m.size(),
            mtime: (m.mtime(), m.mtime_nsec()),
            ctime: (m.ctime(), m.ctime_nsec()),
        })
    }
}

#[derive(Clone, Debug)]
pub struct Config {
    path: PathBuf,
    monit_config: PathBuf,
    stamp: Option<FileStamp>,
    data: Option<Value>,
    pub interval: f64,
    pub ignore_services: Option<Vec<String>>,
    pub ignore_devices: Option<Vec<String>>,
    pub user_defined_checkers: Option<Vec<String>>,
    pub include_devices: Option<Vec<String>>,
    pub optional_containers: IndexMap<String, String>,
}

/// The platform, as sonic_py_common.device_info.get_platform: PLATFORM or machine.conf, else
/// DEVICE_METADATA|localhost in CONFIG_DB, read only when those have none.
pub fn platform_name(device: &DeviceInfo, device_metadata: impl FnOnce() -> Option<TableData>) -> String {
    device
        .platform()
        .or_else(|| device_metadata()?.get("localhost")?.get("platform").cloned())
        .unwrap_or_default()
}

impl Config {
    /// Config of the platform: /usr/share/sonic/device/<platform>/system_health_monitoring_config.json.
    pub fn for_platform(device_dir: &Path, platform: &str) -> Self {
        Self::new(
            device_dir.join(platform).join(CONFIG_FILE),
            PathBuf::from(MONIT_CONFIG_FILE),
        )
    }

    pub fn new(path: PathBuf, monit_config: PathBuf) -> Self {
        Self {
            path,
            monit_config,
            stamp: None,
            data: None,
            interval: DEFAULT_INTERVAL,
            ignore_services: None,
            ignore_devices: None,
            user_defined_checkers: None,
            include_devices: None,
            optional_containers: IndexMap::new(),
        }
    }

    pub fn path(&self) -> &Path {
        &self.path
    }

    pub fn config_file_exists(&self) -> bool {
        self.path.exists()
    }

    /// Reloads the file when it changed. A missing or invalid file resets every entry to its default.
    pub fn load_config(&mut self) {
        if !self.config_file_exists() {
            if self.stamp.is_some() {
                self.reset();
            }
            return;
        }
        let stamp = FileStamp::of(&self.path);
        if stamp == self.stamp && stamp.is_some() {
            return;
        }
        let parsed = fs::read_to_string(&self.path)
            .ok()
            .and_then(|s| serde_json::from_str::<Value>(&s).ok())
            .filter(Value::is_object);
        match parsed {
            Some(data) => {
                self.interval = data
                    .get("polling_interval")
                    .and_then(Value::as_f64)
                    .unwrap_or(DEFAULT_INTERVAL);
                self.ignore_services = list(&data, "services_to_ignore");
                self.ignore_devices = list(&data, "devices_to_ignore");
                self.user_defined_checkers = list(&data, "user_defined_checkers");
                self.include_devices = list(&data, "include_devices");
                self.optional_containers = optional_containers(data.get("optional_containers"));
                self.data = Some(data);
                self.stamp = stamp;
            }
            None => self.reset(),
        }
    }

    fn reset(&mut self) {
        let (path, monit_config) = (self.path.clone(), self.monit_config.clone());
        *self = Self::new(path, monit_config);
    }

    pub fn ignores_service(&self, name: &str) -> bool {
        self.ignore_services
            .as_ref()
            .is_some_and(|s| s.iter().any(|x| x == name))
    }

    pub fn ignores_device(&self, name: &str) -> bool {
        self.ignore_devices
            .as_ref()
            .is_some_and(|s| s.iter().any(|x| x == name))
    }

    pub fn includes_device(&self, name: &str) -> bool {
        self.include_devices
            .as_ref()
            .is_some_and(|s| s.iter().any(|x| x == name))
    }

    /// LED color for "normal", "fault" or "booting".
    pub fn get_led_color(&self, status: &str) -> String {
        if let Some(color) = self
            .data
            .as_ref()
            .and_then(|d| d.get("led_color"))
            .and_then(|c| c.get(status))
        {
            return match color {
                Value::String(s) => s.clone(),
                other => other.to_string(),
            };
        }
        match status {
            "normal" => "green",
            _ => "red",
        }
        .to_string()
    }

    /// Monit's start delay in seconds, 300 when monitrc has none.
    pub fn get_bootup_timeout(&self) -> f64 {
        let Ok(content) = fs::read_to_string(&self.monit_config) else {
            return DEFAULT_BOOTUP_TIMEOUT;
        };
        for line in content.lines() {
            let line = line.trim();
            if line.is_empty() || line.starts_with('#') {
                continue;
            }
            let line = line.split('#').next().unwrap_or("");
            if let Some(pos) = line.find(MONIT_START_DELAY_CONFIG) {
                return line[pos + MONIT_START_DELAY_CONFIG.len()..]
                    .trim()
                    .parse::<i64>()
                    .map(|v| v as f64)
                    .unwrap_or(DEFAULT_BOOTUP_TIMEOUT);
            }
        }
        DEFAULT_BOOTUP_TIMEOUT
    }
}

/// A list entry, deduplicated in file order; strings only.
fn list(data: &Value, key: &str) -> Option<Vec<String>> {
    let items = data.get(key)?.as_array()?;
    let mut result: Vec<String> = Vec::new();
    for item in items.iter().filter_map(Value::as_str) {
        if !result.iter().any(|x| x == item) {
            result.push(item.to_string());
        }
    }
    Some(result)
}

/// Container name to docker image name, non-empty strings only.
pub fn optional_containers(value: Option<&Value>) -> IndexMap<String, String> {
    let Some(Value::Object(map)) = value else {
        return IndexMap::new();
    };
    map.iter()
        .filter_map(|(k, v)| match v {
            Value::String(image) if !k.is_empty() && !image.is_empty() => Some((k.clone(), image.clone())),
            _ => None,
        })
        .collect()
}

#[cfg(test)]
mod tests {
    use super::*;
    use serde_json::json;

    #[test]
    fn seconds_never_panic() {
        assert_eq!(seconds(1.5), Duration::from_millis(1500));
        assert_eq!(seconds(-3.0), Duration::ZERO);
        assert_eq!(seconds(1e300), Duration::MAX);
        assert_eq!(seconds(f64::INFINITY), Duration::MAX);
    }

    fn write(path: &Path, content: &str) {
        fs::write(path, content).unwrap();
    }

    #[test]
    fn the_platform_comes_from_config_db_only_without_machine_conf() {
        let dir = tempfile::tempdir().unwrap();
        let metadata = || {
            let localhost = [("platform".to_string(), "x86_64-from-config-db".to_string())];
            Some(TableData::from([(
                "localhost".to_string(),
                localhost.into_iter().collect(),
            )]))
        };
        let device = DeviceInfo::with_root(dir.path());
        assert_eq!(platform_name(&device, metadata), "x86_64-from-config-db");
        assert_eq!(platform_name(&device, || None), "");

        fs::create_dir_all(dir.path().join("host")).unwrap();
        write(
            &dir.path().join("host/machine.conf"),
            "onie_platform=x86_64-from-machine-conf\n",
        );
        assert_eq!(
            platform_name(&device, || panic!("CONFIG_DB is read only without machine.conf")),
            "x86_64-from-machine-conf"
        );
    }

    #[test]
    fn invalid_file_resets_to_defaults() {
        let dir = tempfile::tempdir().unwrap();
        let path = dir.path().join(CONFIG_FILE);
        let mut config = Config::new(path.clone(), dir.path().join("monitrc"));
        write(&path, r#"{"polling_interval": 10}"#);
        config.load_config();
        assert_eq!(config.interval, 10.0);
        write(&path, r#"{"polling_interval": 10"#);
        config.load_config();
        assert_eq!(config.interval, DEFAULT_INTERVAL);
        write(&path, r#"{"polling_interval": 20}"#);
        config.load_config();
        assert_eq!(config.interval, 20.0);
        // Empty, as some vendors ship it.
        write(&path, "");
        config.load_config();
        assert_eq!(config.interval, DEFAULT_INTERVAL);
        write(&path, r#"{"polling_interval": 20}"#);
        config.load_config();
        assert_eq!(config.interval, 20.0);
        write(&path, "[1, 2]");
        config.load_config();
        assert_eq!(config.interval, DEFAULT_INTERVAL);
    }

    #[test]
    fn defaults_without_file() {
        let dir = tempfile::tempdir().unwrap();
        let mut config = Config::new(dir.path().join(CONFIG_FILE), dir.path().join("monitrc"));
        config.load_config();
        assert!(!config.config_file_exists());
        assert_eq!(config.interval, DEFAULT_INTERVAL);
        assert_eq!(config.ignore_services, None);
        assert_eq!(config.get_led_color("normal"), "green");
        assert_eq!(config.get_led_color("fault"), "red");
        assert_eq!(config.get_led_color("booting"), "red");
        assert_eq!(config.get_bootup_timeout(), DEFAULT_BOOTUP_TIMEOUT);
    }

    #[test]
    fn loads_and_reloads_on_change() {
        let dir = tempfile::tempdir().unwrap();
        let path = dir.path().join(CONFIG_FILE);
        write(
            &path,
            r#"{"services_to_ignore": ["a", "b", "a"], "devices_to_ignore": ["psu"], "user_defined_checkers": ["x"],
                "include_devices": ["liquid_cooling"], "polling_interval": 30,
                "led_color": {"fault": "amber", "normal": "green", "booting": "orange_blink"},
                "optional_containers": {"foo": "docker-foo", "": "x", "bar": ""}}"#,
        );
        let mut config = Config::new(path.clone(), dir.path().join("monitrc"));
        config.load_config();
        assert_eq!(config.interval, 30.0);
        assert_eq!(config.ignore_services, Some(vec!["a".to_string(), "b".to_string()]));
        assert!(config.ignores_device("psu"));
        assert!(config.includes_device("liquid_cooling"));
        assert_eq!(config.user_defined_checkers, Some(vec!["x".to_string()]));
        assert_eq!(config.get_led_color("fault"), "amber");
        assert_eq!(config.get_led_color("booting"), "orange_blink");
        assert_eq!(config.optional_containers.len(), 1);

        write(&path, r#"{"polling_interval": 10, "led_color": {"fault": "red"}}"#);
        config.load_config();
        assert_eq!(config.interval, 10.0);
        assert_eq!(config.ignore_services, None);
        assert_eq!(config.get_led_color("normal"), "green");

        write(&path, "not json");
        config.load_config();
        assert_eq!(config.interval, DEFAULT_INTERVAL);

        fs::remove_file(&path).unwrap();
        write(&path, r#"{"polling_interval": 5}"#);
        config.load_config();
        assert_eq!(config.interval, 5.0);
        fs::remove_file(&path).unwrap();
        config.load_config();
        assert_eq!(config.interval, DEFAULT_INTERVAL);
    }

    #[test]
    fn bootup_timeout_from_monitrc() {
        let dir = tempfile::tempdir().unwrap();
        let monitrc = dir.path().join("monitrc");
        write(
            &monitrc,
            "# with start delay 1\nset daemon 60    # check services at 1-minute intervals\n    with start delay 300    # we delay Monit\n",
        );
        let config = Config::new(dir.path().join(CONFIG_FILE), monitrc.clone());
        assert_eq!(config.get_bootup_timeout(), 300.0);
        write(&monitrc, "set daemon 60\n  with start delay 120\n");
        assert_eq!(config.get_bootup_timeout(), 120.0);
        write(&monitrc, "set daemon 60\n");
        assert_eq!(config.get_bootup_timeout(), DEFAULT_BOOTUP_TIMEOUT);
    }

    #[test]
    fn optional_containers_keep_valid_entries() {
        let value = json!({"otel": "docker-sonic-otel", "a": 1, "b": "", "": "img"});
        let result = optional_containers(Some(&value));
        assert_eq!(result.get("otel").map(String::as_str), Some("docker-sonic-otel"));
        assert_eq!(result.len(), 1);
        assert!(optional_containers(Some(&json!(["x"]))).is_empty());
        assert!(optional_containers(None).is_empty());
    }
}
