use swss_common::DbConnector;


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

pub fn is_fast_reboot_enabled() -> Result<bool> {
    let state_db = StateDBConnector::new()
        .map_err(|e| DeviceInfoError::SwSS(e))?;
    is_fast_reboot_enabled_with_db(&state_db)
}