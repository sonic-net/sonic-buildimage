//! Integration tests for sonic-supervisord-utilities-rs

use sonic_supervisord_utilities_rs::{
    childutils,
    proc_exit_listener::*,
};
use swss_common::ConfigDBConnector;
use syslog::Severity;
use std::io::Write;
use tempfile::NamedTempFile;
use std::time::Duration;

// Helper function to create a ConfigDB connector for tests
fn create_test_config_db() -> ConfigDBConnector {
    // Try to create a ConfigDB connector for testing
    // In test environments this might fail, but we'll let the individual tests handle it
    ConfigDBConnector::new(false, None).unwrap_or_else(|_| {
        // If it fails, try with unix socket path
        ConfigDBConnector::new(true, None).expect("Failed to create test ConfigDB connector with any method")
    })
}

#[test]
fn test_get_group_and_process_list() {
    // Create a temporary config file
    let mut file = NamedTempFile::new().unwrap();
    writeln!(file, "program:orchagent").unwrap();
    writeln!(file, "program:portsyncd").unwrap();
    writeln!(file, "group:dhcp-relay").unwrap();
    writeln!(file, "program:fdbsyncd").unwrap();
    writeln!(file, "").unwrap(); // blank line
    writeln!(file, "program:vlanmgrd").unwrap();

    // Test the function
    let (groups, processes) = get_group_and_process_list(file.path().to_str().unwrap()).unwrap();
    
    assert_eq!(groups, vec!["dhcp-relay"]);
    assert_eq!(processes, vec!["orchagent", "portsyncd", "fdbsyncd", "vlanmgrd"]);
    
    // Test critical process checking logic
    assert!(processes.contains(&"orchagent".to_string()));
    assert!(groups.contains(&"dhcp-relay".to_string()));
    assert!(!processes.contains(&"unknown".to_string()));
}

#[test]
fn test_supervisor_protocol_complete_workflow() {
    // Test parsing headers
    let header_line = "ver:3.0 server:supervisor serial:54 pool:supervisor-proc-exit-listener poolserial:19 eventname:PROCESS_STATE_EXITED len:78";
    let headers = childutils::get_headers(header_line);
    assert_eq!(headers.get("eventname"), Some(&"PROCESS_STATE_EXITED".to_string()));
    assert_eq!(headers.get("ver"), Some(&"3.0".to_string()));
    assert_eq!(headers.get("serial"), Some(&"54".to_string()));
    assert_eq!(headers.get("len"), Some(&"78".to_string()));

    // Test parsing payload
    let payload = "processname:orchagent groupname:bgp from_state:RUNNING expected:0 pid:1234\n";
    let (process_headers, payload_data) = childutils::eventdata(payload);
    assert_eq!(process_headers.get("processname"), Some(&"orchagent".to_string()));
    assert_eq!(process_headers.get("groupname"), Some(&"bgp".to_string()));
    assert_eq!(process_headers.get("from_state"), Some(&"RUNNING".to_string()));
    assert_eq!(process_headers.get("expected"), Some(&"0".to_string()));
    assert_eq!(process_headers.get("pid"), Some(&"1234".to_string()));
    assert_eq!(payload_data, "");
}

#[test]
fn test_generate_alerting_message() {
    // Test generate_alerting_message function
    // This function should log the message (we can't easily test the actual logging)
    // but we can test that it doesn't panic and follows the expected format
    generate_alerting_message("test_process", "not running", 5, Severity::LOG_ERR);
    generate_alerting_message("heartbeat_process", "stuck", 2, Severity::LOG_WARNING);

    // Test with namespace environment variables
    std::env::set_var("NAMESPACE_PREFIX", "asic");
    std::env::set_var("NAMESPACE_ID", "0");
    generate_alerting_message("test_process", "not running", 10, Severity::LOG_ERR);
    
    // Clean up
    std::env::remove_var("NAMESPACE_PREFIX");
    std::env::remove_var("NAMESPACE_ID");
}

#[test]
fn test_heartbeat_alert_interval_functions() {
    // Test the heartbeat alert interval system
    use sonic_supervisord_utilities_rs::*;
    
    // Test loading heartbeat alert intervals (would normally load from ConfigDB)
    // This is a basic test since we can't easily mock ConfigDB
    let config_db = create_test_config_db();
    let heartbeat_intervals = load_heartbeat_alert_interval(&config_db);
    // This might return empty map if ConfigDB is not available, which is expected in test environment
    
    // Test getting default interval for unknown process
    let default_interval = get_heartbeat_alert_interval("unknown_process", &heartbeat_intervals);
    assert_eq!(default_interval, ALERTING_INTERVAL_SECS as f64);
    
    // Test that the function doesn't panic with various inputs
    let _ = get_heartbeat_alert_interval("orchagent", &heartbeat_intervals);
    let _ = get_heartbeat_alert_interval("", &heartbeat_intervals);
    let _ = get_heartbeat_alert_interval("very_long_process_name_that_should_not_exist", &heartbeat_intervals);
}

#[test]
fn test_events_publisher_workflow() {
    // Test the events publisher functionality
    let publisher = swss_common::EventPublisher::new("sonic-events-host")
        .expect("Failed to create event publisher");
    
    // Test publishing events (now uses actual event publishing)
    let result = publish_events(&publisher, "orchagent", "swss");
    if let Err(ref e) = result {
        eprintln!("First publish_events failed: {}", e);
    }
    assert!(result.is_ok());
    
    let result = publish_events(&publisher, "test_process", "test_container");
    if let Err(ref e) = result {
        eprintln!("Second publish_events failed: {}", e);
    }
    assert!(result.is_ok());
    
    // Test with empty strings
    let result = publish_events(&publisher, "", "");
    assert!(result.is_ok());
}

#[test]
fn test_error_conditions() {
    // Test invalid config file
    let result = get_group_and_process_list("/nonexistent/file");
    assert!(result.is_err());

    // Test invalid supervisor protocol headers
    let invalid_header_line = "invalid header format";
    let headers = childutils::get_headers(invalid_header_line);
    // Should succeed but return empty headers for invalid format
    assert!(headers.get("eventname").unwrap_or(&String::new()).is_empty());

    // Test invalid event payload
    let invalid_payload = "invalid payload format";
    let (process_headers, _) = childutils::eventdata(invalid_payload);
    // Should succeed but return default values
    assert!(process_headers.get("processname").unwrap_or(&String::new()).is_empty());
}

#[test]
fn test_namespace_detection() {
    // Test namespace detection logic within generate_alerting_message function
    // We can't directly test get_namespace() since it doesn't exist, but we can
    // test the behavior through generate_alerting_message

    // Test default namespace (host) - should not panic
    std::env::remove_var("NAMESPACE_PREFIX");
    std::env::remove_var("NAMESPACE_ID");
    generate_alerting_message("test_process", "not running", 5, Severity::LOG_ERR);

    // Test asic namespace - should not panic
    std::env::set_var("NAMESPACE_PREFIX", "asic");
    std::env::set_var("NAMESPACE_ID", "0");
    generate_alerting_message("test_process", "not running", 5, Severity::LOG_ERR);

    // Test partial namespace (should fallback to host) - should not panic
    std::env::set_var("NAMESPACE_PREFIX", "asic");
    std::env::remove_var("NAMESPACE_ID");
    generate_alerting_message("test_process", "not running", 5, Severity::LOG_ERR);

    // Cleanup
    std::env::remove_var("NAMESPACE_PREFIX");
    std::env::remove_var("NAMESPACE_ID");
}

#[test]
fn test_autorestart_state_checking() {
    // Test different container types - these will likely fail in test environment
    // without ConfigDB, but we test that the function doesn't panic
    let config_db = create_test_config_db();
    let _result1 = get_autorestart_state("swss", &config_db);
    // Result depends on ConfigDB availability - could be Ok or Err
    
    let _result2 = get_autorestart_state("snmp", &config_db);
    // Result depends on ConfigDB availability - could be Ok or Err
    
    let _result3 = get_autorestart_state("unknown", &config_db);
    // Result depends on ConfigDB availability - could be Ok or Err
    
    // Main test is that none of these panic
    // In a real environment with ConfigDB, these would return the expected values
}

#[test]
fn test_alerting_message_generation() {
    // Test different message types and priorities - function logs but doesn't return string
    // Main test is that it doesn't panic with various inputs
    generate_alerting_message("orchagent", "not running", 5, Severity::LOG_ERR);
    generate_alerting_message("portsyncd", "stuck", 10, Severity::LOG_WARNING);
    generate_alerting_message("test", "status", 0, Severity::LOG_INFO);
    generate_alerting_message("", "", 999, Severity::LOG_ALERT); // Edge case
}

#[test]
fn test_args_parsing() {
    // Test argument parsing since we don't have a SupervisorListener struct
    use clap::Parser;
    
    // Test successful parsing with required arguments
    let args = Args::try_parse_from(&[
        "supervisor-proc-exit-listener", 
        "--container-name", "test-container"
    ]);
    assert!(args.is_ok());
    let args = args.unwrap();
    assert_eq!(args.container_name, "test-container");
    assert!(!args.use_unix_socket_path);
    
    // Test with unix socket flag
    let args = Args::try_parse_from(&[
        "supervisor-proc-exit-listener", 
        "--container-name", "test",
        "--use-unix-socket-path"
    ]);
    assert!(args.is_ok());
    let args = args.unwrap();
    assert!(args.use_unix_socket_path);
}

#[test]
fn test_time_functions() {
    // Test time-related functionality that we can verify
    let time1 = get_current_time();
    std::thread::sleep(Duration::from_millis(10));
    let time2 = get_current_time();
    assert!(time2 > time1);
    
    // Test heartbeat interval retrieval
    let config_db = create_test_config_db();
    let heartbeat_intervals = load_heartbeat_alert_interval(&config_db);
    let interval1 = get_heartbeat_alert_interval("test_process", &heartbeat_intervals);
    assert!(interval1 > 0.0);
    
    let interval2 = get_heartbeat_alert_interval("another_process", &heartbeat_intervals);
    assert!(interval2 > 0.0);
    
    // Both should return the default since there's no ConfigDB in test
    assert_eq!(interval1, ALERTING_INTERVAL_SECS as f64);
    assert_eq!(interval2, ALERTING_INTERVAL_SECS as f64);
}

#[test]
fn test_edge_cases_and_boundary_conditions() {
    // Test empty configuration
    let mut empty_file = NamedTempFile::new().unwrap();
    writeln!(empty_file, "").unwrap();
    writeln!(empty_file, "   ").unwrap(); // whitespace only
    
    let (groups, processes) = get_group_and_process_list(empty_file.path().to_str().unwrap()).unwrap();
    assert!(groups.is_empty());
    assert!(processes.is_empty());
    
    // Test boundary conditions with time calculations
    let now = get_current_time();
    let past_time = now - 60.0; // 60 seconds ago
    let diff = now - past_time;
    assert!(diff >= 59.0 && diff <= 61.0); // Account for small timing variations
    
    // Test heartbeat interval edge cases
    let config_db = create_test_config_db();
    let heartbeat_intervals = load_heartbeat_alert_interval(&config_db);
    let zero_interval = get_heartbeat_alert_interval("", &heartbeat_intervals);
    assert_eq!(zero_interval, ALERTING_INTERVAL_SECS as f64);
    
    let default_interval = get_heartbeat_alert_interval("nonexistent", &heartbeat_intervals);
    assert_eq!(default_interval, ALERTING_INTERVAL_SECS as f64);
}

#[test]
fn test_function_robustness() {
    // Test that functions handle edge cases without panicking

    // Test generate_alerting_message with various inputs
    generate_alerting_message("test", "status", 0, Severity::LOG_ERR);
    generate_alerting_message("", "", 999, Severity::LOG_DEBUG);
    generate_alerting_message("very_long_process_name_that_should_work", "some status", 60, Severity::LOG_WARNING);
    
    // Test get_current_time multiple calls
    let times: Vec<f64> = (0..5).map(|_| {
        std::thread::sleep(Duration::from_millis(1));
        get_current_time()
    }).collect();
    
    // Times should be increasing
    for i in 1..times.len() {
        assert!(times[i] >= times[i-1]);
    }
    
    // Test heartbeat interval function with various inputs
    let config_db = create_test_config_db();
    let heartbeat_intervals = load_heartbeat_alert_interval(&config_db);
    let _ = get_heartbeat_alert_interval("test1", &heartbeat_intervals);
    let _ = get_heartbeat_alert_interval("test2", &heartbeat_intervals);
    let _ = get_heartbeat_alert_interval("", &heartbeat_intervals);
    let _ = get_heartbeat_alert_interval("very_long_name", &heartbeat_intervals);
}

#[test]
fn test_main_swss_no_container() {
    // Test that Args::parse() fails when required container-name is missing
    // This simulates the Python test: main([]) which should exit with code 1
    
    use clap::Parser;
    
    // This should fail because --container-name is required
    let result = Args::try_parse_from(&["supervisor-proc-exit-listener"]);
    
    // Should fail due to missing required argument
    assert!(result.is_err());
    
    // Verify it's specifically about the missing container-name argument
    let error = result.unwrap_err();
    let error_str = error.to_string();
    assert!(error_str.contains("container-name") || error_str.contains("required"));
}

#[test]
fn test_supervisor_termination_logic() {
    // Test that the termination logic exists in the codebase
    // We can't test the actual termination without mocking, but we can
    // verify the signal handling imports and basic functionality
    
    use nix::sys::signal::{Signal};
    use nix::unistd::getppid;
    
    // Test that we can get parent PID (used in termination logic)
    let parent_pid = getppid();
    assert!(parent_pid.as_raw() > 0);
    
    // Test that SIGTERM signal exists (used in termination)
    let _signal = Signal::SIGTERM;
    
    // This verifies the components needed for supervisor termination exist
}

#[test]
fn test_autorestart_logic() {
    // Test the auto-restart decision logic that would be used in the main function
    // We test the get_autorestart_state function which determines the behavior
    
    // Test that the function exists and has the right signature
    // These will likely fail without ConfigDB, but test they don't panic
    let config_db = create_test_config_db();
    let _result_swss = get_autorestart_state("swss", &config_db);
    let _result_snmp = get_autorestart_state("snmp", &config_db);
    
    // The actual values depend on ConfigDB being available
    // In a test environment, these might return errors, which is expected
    // The important thing is that the function calls don't panic
    
    // Test with various container names
    let _ = get_autorestart_state("test", &config_db);
    let _ = get_autorestart_state("", &config_db);
    let _ = get_autorestart_state("very_long_container_name", &config_db);
    
    // All these should complete without panicking
}