use docker_wait_any_rs::{run_main, wait_for_container_with_db, G_DEP_SERVICES, G_SERVICE, DockerApi};
use futures_util::stream::{self, Stream};
use mockall::mock;
use mockall::predicate::*;
use serial_test::serial;
use std::pin::Pin;
use std::sync::Arc;
use std::time::Duration;
use tokio::time::timeout;
use bollard::container::WaitContainerOptions;
use sonic_rs_common::device_info::StateDBTrait;
use std::collections::HashMap;
use std::sync::atomic::{AtomicBool, Ordering};

fn setup_method() {
    G_SERVICE.lock().unwrap().clear();
    G_DEP_SERVICES.lock().unwrap().clear();
}

mock! {
    pub DockerApi {}

    impl DockerApi for DockerApi {
        fn wait_container(
            &self,
            container_name: String,
            options: Option<WaitContainerOptions<String>>,
        ) -> Pin<Box<dyn Stream<Item = Result<bollard::models::ContainerWaitResponse, bollard::errors::Error>> + Send>>;
    }
}

#[tokio::test]
#[serial]
async fn test_all_containers_running_timeout() {
    setup_method();

    let services = vec!["swss".to_string()];
    let dependents = vec!["syncd".to_string(), "teamd".to_string()];

    let mut mock_docker = MockDockerApi::new();

    mock_docker
        .expect_wait_container()
        .with(eq("swss".to_string()), always())
        .returning(|_, _| Box::pin(stream::pending()));

    mock_docker
        .expect_wait_container()
        .with(eq("syncd".to_string()), always())
        .returning(|_, _| Box::pin(stream::pending()));

    mock_docker
        .expect_wait_container()
        .with(eq("teamd".to_string()), always())
        .returning(|_, _| Box::pin(stream::pending()));

    let result = timeout(
        Duration::from_secs(2),
        run_main(Arc::new(mock_docker), Some(services), Some(dependents)),
    )
    .await;

    assert!(result.is_err(), "Should timeout when containers keep running");
}

#[tokio::test]
#[serial]
async fn test_swss_exits_main_will_exit() {
    setup_method();

    let services = vec!["swss".to_string()];
    let dependents = vec!["syncd".to_string(), "teamd".to_string()];

    let mut mock_docker = MockDockerApi::new();

    mock_docker
        .expect_wait_container()
        .with(eq("swss".to_string()), always())
        .returning(|_, _| {
            Box::pin(futures_util::stream::iter(vec![Ok(
                bollard::models::ContainerWaitResponse {
                    status_code: 0,
                    error: None,
                },
            )]))
        });

    mock_docker
        .expect_wait_container()
        .with(eq("syncd".to_string()), always())
        .returning(|_, _| Box::pin(stream::pending()));

    mock_docker
        .expect_wait_container()
        .with(eq("teamd".to_string()), always())
        .returning(|_, _| Box::pin(stream::pending()));

    let result = timeout(
        Duration::from_secs(2),
        run_main(Arc::new(mock_docker), Some(services), Some(dependents)),
    )
    .await;

    assert!(result.is_ok(), "Should not timeout");
    let exit_code = result.unwrap().unwrap();
    assert_eq!(exit_code, 0, "Should exit with code 0");
}

#[tokio::test]
#[serial]
async fn test_empty_args_main_will_exit() {
    setup_method();

    let mock_docker = MockDockerApi::new();

    let result = run_main(Arc::new(mock_docker), None, None).await;

    assert!(result.is_ok(), "Main should exit successfully");
    let exit_code = result.unwrap();
    assert_eq!(exit_code, 0, "Main should exit with code 0 when no containers specified");
}

// Mock implementation for testing
#[derive(Default)]
struct MockStateDbConnector {
    data: HashMap<String, HashMap<String, String>>,
}

impl MockStateDbConnector {
    fn new() -> Self {
        Self {
            data: HashMap::new(),
        }
    }

    fn set_value(&mut self, key: &str, field: &str, value: &str) {
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

#[tokio::test]
#[serial]
async fn test_teamd_exits_warm_restart_main_will_not_exit() {
    setup_method();

    let services = vec!["swss".to_string()];
    let dependents = vec!["syncd".to_string(), "teamd".to_string()];

    // Set up warm restart enabled
    let mut mock_db = MockStateDbConnector::new();
    mock_db.set_value("WARM_RESTART_ENABLE_TABLE|system", "enable", "true");

    let mut mock_docker = MockDockerApi::new();

    // teamd exits after returning one wait response, others wait indefinitely
    mock_docker
        .expect_wait_container()
        .with(eq("teamd".to_string()), always())
        .returning(|_, _| {
            Box::pin(futures_util::stream::iter(vec![Ok(
                bollard::models::ContainerWaitResponse {
                    status_code: 0,
                    error: None,
                },
            )]))
        });

    mock_docker
        .expect_wait_container()
        .with(eq("swss".to_string()), always())
        .returning(|_, _| Box::pin(stream::pending()));

    mock_docker
        .expect_wait_container()
        .with(eq("syncd".to_string()), always())
        .returning(|_, _| Box::pin(stream::pending()));

    // Set up global variables
    {
        let mut g_service = G_SERVICE.lock().unwrap();
        let mut g_dep_services = G_DEP_SERVICES.lock().unwrap();
        *g_service = services;
        *g_dep_services = dependents;
    }

    let g_thread_exit_event = Arc::new(AtomicBool::new(false));

    // Test teamd container specifically - it should continue waiting due to warm restart
    let docker_clone = Arc::new(mock_docker);
    let event_clone = g_thread_exit_event.clone();

    let result = timeout(
        Duration::from_secs(2),
        wait_for_container_with_db(
            docker_clone.as_ref(),
            "teamd".to_string(),
            event_clone,
            &mock_db,
        ),
    )
    .await;

    // Should timeout because warm restart is enabled and teamd is a dependent service
    assert!(result.is_err(), "Should timeout when warm restart is enabled and teamd exits");

    // Verify exit event was not set (container should continue waiting)
    assert!(!g_thread_exit_event.load(Ordering::Acquire), "Exit event should not be set when warm restart is enabled");
}
