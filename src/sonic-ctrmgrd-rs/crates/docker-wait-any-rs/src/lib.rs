use bollard::container::WaitContainerOptions;
use bollard::Docker;
use futures_util::stream::{Stream, TryStreamExt};
use sonic_rs_common::device_info;
use std::pin::Pin;
use std::sync::atomic::{AtomicBool, Ordering};
use std::sync::Arc;
use tokio::task::JoinSet;
use tracing::{error, info};


#[derive(thiserror::Error, Debug)]
pub enum Error {
    #[error("Docker error")]
    Docker(#[from] bollard::errors::Error),
    #[error("IO error")]
    IO(#[from] std::io::Error),
    #[error("Task join error")]
    Join(#[from] tokio::task::JoinError),
    #[error("Device info error")]
    DeviceInfo(#[from] sonic_rs_common::device_info::DeviceInfoError),
    #[error("Syslog initialization error: {0}")]
    Syslog(String),
}

pub trait DockerApi: Send + Sync {
    fn wait_container(
        &self,
        container_name: String,
        options: Option<WaitContainerOptions<String>>,
    ) -> Pin<Box<dyn Stream<Item = Result<bollard::models::ContainerWaitResponse, bollard::errors::Error>> + Send>>;
}

#[derive(Debug)]
pub struct BollardDockerApi {
    docker: Docker,
}

impl BollardDockerApi {
    pub fn new() -> Result<Self, bollard::errors::Error> {
        Ok(BollardDockerApi {
            docker: Docker::connect_with_local_defaults()?,
        })
    }
}

impl DockerApi for BollardDockerApi {
    fn wait_container(
        &self,
        container_name: String,
        options: Option<WaitContainerOptions<String>>,
    ) -> Pin<Box<dyn Stream<Item = Result<bollard::models::ContainerWaitResponse, bollard::errors::Error>> + Send>> {
        Box::pin(self.docker.wait_container(&container_name, options))
    }
}

pub async fn wait_for_container(
    docker_client: &dyn DockerApi,
    container_name: String,
    dependent_services: Arc<Vec<String>>,
    exit_event: Arc<AtomicBool>,
) -> Result<(), Error> {
    info!("Waiting on container '{}'", container_name);

    loop {
        let wait_result = docker_client
            .wait_container(
                container_name.clone(),
                Some(WaitContainerOptions {
                    condition: "not-running".to_string(),
                }),
            )
            .try_collect::<Vec<_>>()
            .await;

        if let Err(e) = wait_result {
            if exit_event.load(Ordering::Acquire) {
                info!("Container {} wait thread get exception: {}", container_name, e);
                return Ok(());
            }
            // If a container is killed, `wait_container` may return an error.
            // Treat this as the container having exited.
            info!("Container {} exited with a status that resulted in an error from the Docker API: {}", container_name, e);
        }

        info!("No longer waiting on container '{}'", container_name);

        if dependent_services.contains(&container_name) {
            let warm_restart = device_info::is_warm_restart_enabled(&container_name)?;
            let fast_reboot = device_info::is_fast_reboot_enabled()?;

            if warm_restart || fast_reboot {
                continue;
            }
        }

        exit_event.store(true, Ordering::Release);
        return Ok(());
    }
}

pub async fn run_main(
    docker_client: Arc<dyn DockerApi>,
    service: Option<Vec<String>>,
    dependent: Option<Vec<String>>,
) -> Result<i32, Error> {
    let service_containers = service.unwrap_or_default();
    let dependent_containers = dependent.unwrap_or_default();

    let mut container_names = Vec::new();
    container_names.extend(service_containers);
    container_names.extend(dependent_containers.clone());

    if container_names.is_empty() {
        return Ok(0);
    }

    let exit_event = Arc::new(AtomicBool::new(false));
    let dependent_containers_arc = Arc::new(dependent_containers);
    let mut tasks = JoinSet::new();

    for container_name in container_names {
        let docker_clone = docker_client.clone();
        let event_clone = exit_event.clone();
        let deps_clone = dependent_containers_arc.clone();

        tasks.spawn(async move {
            wait_for_container(docker_clone.as_ref(), container_name, deps_clone, event_clone).await
        });
    }

    while let Some(result) = tasks.join_next().await {
        match result {
            Ok(Ok(())) => {
                break;
            }
            Ok(Err(e)) => {
                error!("Container watcher error: {}", e);
                return Err(e);
            }
            Err(e) => {
                error!("Task join error: {}", e);
                return Err(Error::Join(e));
            }
        }
    }

    Ok(0)
}