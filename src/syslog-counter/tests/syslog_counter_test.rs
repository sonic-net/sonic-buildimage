#[path = "../src/lib.rs"]
mod lib;

#[cfg(test)]
mod tests {
    use lib::*;
    use swss_common::*;
    use super::*;
    use std::sync::{Arc, Mutex};
    use std::thread;
    use std::time::Duration;

    pub struct MockDbConnector {
    }

    impl MockDbConnector {
        pub fn new() -> Self {
            MockDbConnector {
            }
        }
    }

    impl DbConnectorTrait for MockDbConnector {
        fn new_named(
            _db_name: impl Into<String>,
            _is_tcp_conn: bool,
            _timeout_ms: u32,
        ) -> Result<Box<dyn DbConnectorTrait + Send + 'static>> {
            Ok(Box::new(MockDbConnector::new()))
        }

        fn hget(&self, key: &str, field: &str) -> Result<Option<CxxString>> {
            Ok(Some(convert_to_CxxString(42)))
        }

        fn hset(&self, key: &str, field: &str, value: &CxxStr) -> Result<()> {
            Ok(())
        }
    }


    #[test]
    fn test_convert_to_CxxString() {
        let result = convert_to_CxxString(123);
        assert_eq!(result.to_str().unwrap(), "123");
    }

    #[test]
    fn test_convert_to_u64() {
        let cxx = CxxString::new("456".to_string());
        let result = convert_to_u64(cxx);
        assert_eq!(result, Ok(456));
    }

    #[test]
    fn test_counters_db_get_count() {
        let connector = MockDbConnector::new();
        let db = CountersDb { connector: Box::new(connector) };
        let count = db.get_count();
        assert_eq!(count.unwrap(), 42);
    }

    #[test]
    fn test_counters_db_set_count() {
        let connector = MockDbConnector::new();
        let db = CountersDb { connector: Box::new(connector) };
        let result = db.set_count(100);
        assert!(result.is_ok());
    }

    #[test]
    fn test_update_counter_db() {
        let counter = Arc::new(Mutex::new(0u64));
        let counter_clone = Arc::clone(&counter);

        let handle = thread::spawn(move || {
            let connector = MockDbConnector::new();
            let db = CountersDb { connector: Box::new(connector) };
            let initial_value = db.get_count().unwrap();
            let mut count = counter_clone.lock().unwrap();
            *count = initial_value;
        });

        handle.join().unwrap();
        let count = counter.lock().unwrap();
        assert_eq!(*count, 42);
    }

    #[test]
    fn test_plugin_main_initialization() {
        let counter = Arc::new(Mutex::new(0u64));
        let counter_clone = Arc::clone(&counter);

        let handle = thread::spawn(move || {
            let connector = MockDbConnector::new();
            let db = CountersDb { connector: Box::new(connector) };
            let initial_value = db.get_count().unwrap();
            let mut count = counter_clone.lock().unwrap();
            *count = initial_value;
        });

        handle.join().unwrap();
        let count = counter.lock().unwrap();
        assert_eq!(*count, 42);
    }
}
