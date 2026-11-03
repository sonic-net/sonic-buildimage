use indexmap::IndexMap;
use serde::{Deserialize, Serialize};

pub const STATUS_OK: &str = "OK";
pub const STATUS_NOT_OK: &str = "Not OK";

/// Status of one checked object, as the Python `HealthChecker._info` entries.
#[derive(Clone, Debug, PartialEq, Eq, Serialize, Deserialize)]
pub struct ObjectInfo {
    #[serde(rename = "type")]
    pub object_type: String,
    pub message: String,
    pub status: String,
}

impl ObjectInfo {
    pub fn is_ok(&self) -> bool {
        self.status == STATUS_OK
    }
}

/// Objects of one category, in check order.
pub type CategoryInfo = IndexMap<String, ObjectInfo>;

/// Categories in check order: Services, Hardware, user-defined categories, Internal.
pub type Stats = IndexMap<String, CategoryInfo>;

/// Results collected by one checker run.
#[derive(Clone, Debug, Default)]
pub struct CheckerInfo {
    info: CategoryInfo,
    not_ok: bool,
}

impl CheckerInfo {
    pub fn set_object_not_ok(&mut self, object_type: &str, name: &str, message: impl Into<String>) {
        self.info.insert(
            name.to_string(),
            ObjectInfo {
                object_type: object_type.to_string(),
                message: message.into(),
                status: STATUS_NOT_OK.to_string(),
            },
        );
        self.not_ok = true;
    }

    pub fn set_object_ok(&mut self, object_type: &str, name: &str) {
        self.info.insert(
            name.to_string(),
            ObjectInfo {
                object_type: object_type.to_string(),
                message: String::new(),
                status: STATUS_OK.to_string(),
            },
        );
    }

    /// True once any object was set Not OK, even if it was set OK later, as the Python summary.
    pub fn has_not_ok(&self) -> bool {
        self.not_ok
    }

    pub fn get(&self, name: &str) -> Option<&ObjectInfo> {
        self.info.get(name)
    }

    pub fn info(&self) -> &CategoryInfo {
        &self.info
    }

    pub fn into_info(self) -> CategoryInfo {
        self.info
    }
}
