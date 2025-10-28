use docker_wait_any_rs::{run_main, DockerApi};
use futures_util::stream::{self, Stream, StreamExt};
use mockall::mock;
use mockall::predicate::*;
use serial_test::serial;
use std::pin::Pin;
use std::sync::Arc;
use std::time::Duration;
use tokio::time::timeout;
use bollard::container::WaitContainerOptions;
use std::sync::atomic::{AtomicBool, Ordering};
use injectorpp::{interface::injector::{InjectorPP, FuncPtr, CallCountVerifier}, func, fake};


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

    let mock_docker = MockDockerApi::new();

    let result = run_main(Arc::new(mock_docker), None, None).await;

    assert!(result.is_ok(), "Main should exit successfully");
    let exit_code = result.unwrap();
    assert_eq!(exit_code, 0, "Main should exit with code 0 when no containers specified");
}

#[tokio::test]
#[serial]
async fn test_teamd_exits_warm_restart_main_will_not_exit() {

    let services = vec!["swss".to_string()];
    let dependents = vec!["syncd".to_string(), "teamd".to_string()];

    let mut mock_docker = MockDockerApi::new();

    // teamd exits (simulates container exit, warm restart will cause it to be called again)
    mock_docker
        .expect_wait_container()
        .with(eq("teamd".to_string()), always())
        .returning(|_, _| {
            Box::pin(futures_util::stream::iter(vec![Ok(
                bollard::models::ContainerWaitResponse {
                    status_code: 0,
                    error: None,
                }
            )]).then(|item| async move {
                // Add context switch to allow timeout to work
                tokio::task::yield_now().await;
                item
            }))
        });

    mock_docker
        .expect_wait_container()
        .with(eq("swss".to_string()), always())
        .returning(|_, _| Box::pin(stream::pending()));

    mock_docker
        .expect_wait_container()
        .with(eq("syncd".to_string()), always())
        .returning(|_, _| Box::pin(stream::pending()));

    // Mock warm restart enabled using injectorpp - keep injector alive for entire test
    let mut injector = InjectorPP::new();
    injector
        .when_called(func!(sonic_rs_common::device_info::is_warm_restart_enabled, fn(&str) -> Result<bool, sonic_rs_common::device_info::DeviceInfoError>))
        .will_execute(fake!(
            func_type: fn(container_name: &str) -> Result<bool, sonic_rs_common::device_info::DeviceInfoError>,
            returns: Ok(true)
        ));
    injector
        .when_called(func!(sonic_rs_common::device_info::is_fast_reboot_enabled, fn() -> Result<bool, sonic_rs_common::device_info::DeviceInfoError>))
        .will_execute(fake!(
            func_type: fn() -> Result<bool, sonic_rs_common::device_info::DeviceInfoError>,
            returns: Ok(false)
        ));

    // Test run_main - should timeout because warm restart is enabled and main will not exit
    let result = timeout(
        Duration::from_secs(3),
        run_main(Arc::new(mock_docker), Some(services), Some(dependents))
    )
    .await;

    // Should timeout because warm restart is enabled and dependent service teamd exits but main continues
    assert!(result.is_err(), "run_main should timeout when warm restart is enabled and teamd exits");
}
