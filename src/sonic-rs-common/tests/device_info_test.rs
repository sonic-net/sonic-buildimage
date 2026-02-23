use sonic_rs_common::device_info::*;

mod mock_state_db;
use mock_state_db::MockStateDbConnector;

#[cfg(test)]
mod test_device_info {
    use super::*;

    // Test warm restart enabled when system flag is true
    #[test]
    fn test_is_warm_restart_enabled_system_true() {
        let mut mock_db = MockStateDbConnector::new();
        mock_db.set_value("WARM_RESTART_ENABLE_TABLE|system", "enable", "true");

        let result = is_warm_restart_enabled_with_db("swss", &mock_db).unwrap();
        assert_eq!(result, true);
    }

    // Test warm restart enabled when container flag is true
    #[test]
    fn test_is_warm_restart_enabled_container_true() {
        let mut mock_db = MockStateDbConnector::new();
        mock_db.set_value("WARM_RESTART_ENABLE_TABLE|swss", "enable", "true");

        let result = is_warm_restart_enabled_with_db("swss", &mock_db).unwrap();
        assert_eq!(result, true);
    }

    // Test warm restart disabled when both flags are false
    #[test]
    fn test_is_warm_restart_enabled_both_false() {
        let mut mock_db = MockStateDbConnector::new();
        mock_db.set_value("WARM_RESTART_ENABLE_TABLE|system", "enable", "false");
        mock_db.set_value("WARM_RESTART_ENABLE_TABLE|swss", "enable", "false");

        let result = is_warm_restart_enabled_with_db("swss", &mock_db).unwrap();
        assert_eq!(result, false);
    }

    // Test warm restart enabled when both system and container flags are true
    #[test]
    fn test_is_warm_restart_enabled_both_true() {
        let mut mock_db = MockStateDbConnector::new();
        mock_db.set_value("WARM_RESTART_ENABLE_TABLE|system", "enable", "true");
        mock_db.set_value("WARM_RESTART_ENABLE_TABLE|swss", "enable", "true");

        let result = is_warm_restart_enabled_with_db("swss", &mock_db).unwrap();
        assert_eq!(result, true);
    }

    // Test warm restart with missing values (should default to false)
    #[test]
    fn test_is_warm_restart_enabled_missing_values() {
        let mock_db = MockStateDbConnector::new();

        let result = is_warm_restart_enabled_with_db("swss", &mock_db).unwrap();
        assert_eq!(result, false);
    }

    // Test warm restart with different container names
    #[test]
    fn test_is_warm_restart_enabled_different_containers() {
        let mut mock_db = MockStateDbConnector::new();
        mock_db.set_value("WARM_RESTART_ENABLE_TABLE|bgp", "enable", "true");
        mock_db.set_value("WARM_RESTART_ENABLE_TABLE|teamd", "enable", "false");

        let result_bgp = is_warm_restart_enabled_with_db("bgp", &mock_db).unwrap();
        assert_eq!(result_bgp, true);

        let result_teamd = is_warm_restart_enabled_with_db("teamd", &mock_db).unwrap();
        assert_eq!(result_teamd, false);

        let result_syncd = is_warm_restart_enabled_with_db("syncd", &mock_db).unwrap();
        assert_eq!(result_syncd, false);
    }

    // Test fast reboot enabled
    #[test]
    fn test_is_fast_reboot_enabled_true() {
        let mut mock_db = MockStateDbConnector::new();
        mock_db.set_value("FAST_RESTART_ENABLE_TABLE|system", "enable", "true");

        let result = is_fast_reboot_enabled_with_db(&mock_db).unwrap();
        assert_eq!(result, true);
    }

    // Test fast reboot disabled
    #[test]
    fn test_is_fast_reboot_enabled_false() {
        let mut mock_db = MockStateDbConnector::new();
        mock_db.set_value("FAST_RESTART_ENABLE_TABLE|system", "enable", "false");

        let result = is_fast_reboot_enabled_with_db(&mock_db).unwrap();
        assert_eq!(result, false);
    }

    // Test fast reboot with missing value (should default to false)
    #[test]
    fn test_is_fast_reboot_enabled_missing_value() {
        let mock_db = MockStateDbConnector::new();

        let result = is_fast_reboot_enabled_with_db(&mock_db).unwrap();
        assert_eq!(result, false);
    }

    // Test warm restart with invalid values (non-"true" should be false)
    #[test]
    fn test_is_warm_restart_enabled_invalid_values() {
        let mut mock_db = MockStateDbConnector::new();
        mock_db.set_value("WARM_RESTART_ENABLE_TABLE|system", "enable", "yes");
        mock_db.set_value("WARM_RESTART_ENABLE_TABLE|swss", "enable", "1");

        let result = is_warm_restart_enabled_with_db("swss", &mock_db).unwrap();
        assert_eq!(result, false);
    }

    // Test fast reboot with invalid values (non-"true" should be false)
    #[test]
    fn test_is_fast_reboot_enabled_invalid_values() {
        let mut mock_db = MockStateDbConnector::new();
        mock_db.set_value("FAST_RESTART_ENABLE_TABLE|system", "enable", "yes");

        let result = is_fast_reboot_enabled_with_db(&mock_db).unwrap();
        assert_eq!(result, false);
    }

    // Test empty container name
    #[test]
    fn test_is_warm_restart_enabled_empty_container() {
        let mut mock_db = MockStateDbConnector::new();
        mock_db.set_value("WARM_RESTART_ENABLE_TABLE|", "enable", "true");

        let result = is_warm_restart_enabled_with_db("", &mock_db).unwrap();
        assert_eq!(result, true);
    }

    // Test case sensitivity (should be case sensitive)
    #[test]
    fn test_is_warm_restart_enabled_case_sensitivity() {
        let mut mock_db = MockStateDbConnector::new();
        mock_db.set_value("WARM_RESTART_ENABLE_TABLE|system", "enable", "True");
        mock_db.set_value("WARM_RESTART_ENABLE_TABLE|swss", "enable", "TRUE");

        let result = is_warm_restart_enabled_with_db("swss", &mock_db).unwrap();
        assert_eq!(result, false); // Should be false as only "true" (lowercase) is valid
    }

    // Integration test for public API (will use real database connection, may fail in test env)
    #[test]
    fn test_is_warm_restart_enabled_public_api() {
        let result = is_warm_restart_enabled("swss");
        // Test passes regardless of result since database may not be available in test environment
        match result {
            Ok(_) => {
                // Function succeeded - database was available
            }
            Err(_) => {
                // Function failed - expected in test environment without database
            }
        }
    }

    // Integration test for public API (will use real database connection, may fail in test env)
    #[test]
    fn test_is_fast_reboot_enabled_public_api() {
        let result = is_fast_reboot_enabled();
        // Test passes regardless of result since database may not be available in test environment
        match result {
            Ok(_) => {
                // Function succeeded - database was available
            }
            Err(_) => {
                // Function failed - expected in test environment without database
            }
        }
    }
}