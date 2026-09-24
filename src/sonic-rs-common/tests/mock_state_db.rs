use sonic_rs_common::device_info::StateDBTrait;
use std::collections::HashMap;

// Mock implementation for testing
#[derive(Default)]
pub struct MockStateDbConnector {
    data: HashMap<String, HashMap<String, String>>,
}

impl MockStateDbConnector {
    pub fn new() -> Self {
        Self {
            data: HashMap::new(),
        }
    }

    pub fn set_value(&mut self, key: &str, field: &str, value: &str) {
        self.data
            .entry(key.to_string())
            .or_insert_with(HashMap::new)
            .insert(field.to_string(), value.to_string());
    }
}

impl StateDBTrait for MockStateDbConnector {
    fn hget(&self, key: &str, field: &str) -> std::result::Result<Option<swss_common::CxxString>, swss_common::Exception> {
        Ok(self.data
            .get(key)
            .and_then(|fields| fields.get(field))
            .map(|s| swss_common::CxxString::from(s.as_str())))
    }
}