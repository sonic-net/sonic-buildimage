use docker_wait_any_rs::{G_DEP_SERVICES, G_SERVICE};
use serial_test::serial;
use std::time::Duration;
use tokio::time::timeout;

fn setup_method() {
    G_SERVICE.lock().unwrap().clear();
    G_DEP_SERVICES.lock().unwrap().clear();
}

#[serial]
#[tokio::test]
async fn test_empty_args_main_will_exit() {
    setup_method();

    let result = docker_wait_any_rs::run_main(None, None).await;

    assert!(result.is_ok(), "Main should exit successfully");
    let exit_code = result.unwrap();
    assert_eq!(exit_code, 0, "Main should exit with code 0 when no containers specified");
}

#[serial]
#[tokio::test]
async fn test_service_and_dependent_args_setup() {
    setup_method();

    // Test that services and dependencies are properly set up
    let services = vec!["swss".to_string()];
    let dependents = vec!["syncd".to_string(), "teamd".to_string()];

    // This should fail because Docker connection will fail, but we can verify setup
    let result = timeout(Duration::from_millis(100), async {
        docker_wait_any_rs::run_main(Some(services.clone()), Some(dependents.clone())).await
    }).await;

    // Verify the global variables were set correctly
    let g_service = G_SERVICE.lock().unwrap();
    let g_dep_services = G_DEP_SERVICES.lock().unwrap();
    
    assert_eq!(*g_service, services, "Service should be set correctly");
    assert_eq!(*g_dep_services, dependents, "Dependents should be set correctly");
    
    // Should timeout or error trying to connect to Docker, which is expected
    assert!(result.is_err() || result.unwrap().is_err(), "Should timeout or error with no Docker");
}

#[serial]
#[tokio::test]
async fn test_only_service_args_setup() {
    setup_method();

    let services = vec!["redis".to_string(), "database".to_string()];

    // This should fail because Docker connection will fail, but we can verify setup
    let result = timeout(Duration::from_millis(100), async {
        docker_wait_any_rs::run_main(Some(services.clone()), None).await
    }).await;

    // Verify the global variables were set correctly
    let g_service = G_SERVICE.lock().unwrap();
    let g_dep_services = G_DEP_SERVICES.lock().unwrap();
    
    assert_eq!(*g_service, services, "Service should be set correctly");
    assert!(g_dep_services.is_empty(), "Dependents should be empty");
    
    // Should timeout or error trying to connect to Docker, which is expected
    assert!(result.is_err() || result.unwrap().is_err(), "Should timeout or error with no Docker");
}

#[serial]
#[tokio::test]
async fn test_only_dependent_args_setup() {
    setup_method();

    let dependents = vec!["helper1".to_string(), "helper2".to_string()];

    // This should fail because Docker connection will fail, but we can verify setup
    let result = timeout(Duration::from_millis(100), async {
        docker_wait_any_rs::run_main(None, Some(dependents.clone())).await
    }).await;

    // Verify the global variables were set correctly
    let g_service = G_SERVICE.lock().unwrap();
    let g_dep_services = G_DEP_SERVICES.lock().unwrap();
    
    assert!(g_service.is_empty(), "Service should be empty");
    assert_eq!(*g_dep_services, dependents, "Dependents should be set correctly");
    
    // Should timeout or error trying to connect to Docker, which is expected
    assert!(result.is_err() || result.unwrap().is_err(), "Should timeout or error with no Docker");
}