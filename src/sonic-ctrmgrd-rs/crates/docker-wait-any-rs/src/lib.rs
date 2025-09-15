use bollard::container::WaitContainerOptions;
use bollard::Docker;
use futures_util::stream::{Stream, TryStreamExt};
use sonic_rs_common::device_info::{self, StateDBTrait, StateDBConnector};
use std::pin::Pin;
use std::sync::atomic::{AtomicBool, Ordering};
use std::sync::Arc;
use tokio::task::JoinSet;
use log::{error, info};

pub static G_SERVICE: std::sync::Mutex<Vec<String>> = std::sync::Mutex::new(Vec::new());
pub static G_DEP_SERVICES: std::sync::Mutex<Vec<String>> = std::sync::Mutex::new(Vec::new());

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
}

pub trait DockerApi: Send + Sync {
    fn wait_container(
        &self,
        container_name: String,
        options: Option<WaitContainerOptions<String>>,
    ) -> Pin<Box<dyn Stream<Item = Result<bollard::models::ContainerWaitResponse, bollard::errors::Error>> + Send>>;
}

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
    g_thread_exit_event: Arc<AtomicBool>,
) -> Result<(), Error> {
    let state_db = StateDBConnector::new()
        .map_err(|e| Error::DeviceInfo(sonic_rs_common::device_info::DeviceInfoError::SwSS(e)))?;
    wait_for_container_with_db(docker_client, container_name, g_thread_exit_event, &state_db).await
}

pub async fn wait_for_container_with_db<T: StateDBTrait>(
    docker_client: &dyn DockerApi,
    container_name: String,
    g_thread_exit_event: Arc<AtomicBool>,
    state_db: &T,
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
            if g_thread_exit_event.load(Ordering::Acquire) {
                info!("Container {} wait thread get exception: {}", container_name, e);
                return Ok(());
            }
            // If a container is killed, `wait_container` may return an error.
            // Treat this as the container having exited.
            info!("Container {} exited with a status that resulted in an error from the Docker API: {}", container_name, e);
        }

        info!("No longer waiting on container '{}'", container_name);

        let g_dep_services = G_DEP_SERVICES.lock().unwrap();
        if g_dep_services.contains(&container_name) {
            let warm_restart = device_info::is_warm_restart_enabled_with_db(&container_name, state_db)?;
            let fast_reboot = device_info::is_fast_reboot_enabled_with_db(state_db)?;

            if warm_restart || fast_reboot {
                continue;
            }
        }

        g_thread_exit_event.store(true, Ordering::Release);
        return Ok(());
    }
}

pub async fn run_main(
    docker_client: Arc<dyn DockerApi>,
    service: Option<Vec<String>>,
    dependent: Option<Vec<String>>,
) -> Result<i32, Error> {
    {
        let mut g_service = G_SERVICE.lock().unwrap();
        let mut g_dep_services = G_DEP_SERVICES.lock().unwrap();

        if let Some(service) = service {
            *g_service = service;
        }
        if let Some(dependent) = dependent {
            *g_dep_services = dependent;
        }
    }

    let mut container_names = Vec::new();
    {
        let g_service = G_SERVICE.lock().unwrap();
        let g_dep_services = G_DEP_SERVICES.lock().unwrap();
        container_names.extend(g_service.clone());
        container_names.extend(g_dep_services.clone());
    }

    if container_names.is_empty() {
        return Ok(0);
    }

    let g_thread_exit_event = Arc::new(AtomicBool::new(false));
    let mut tasks = JoinSet::new();

    for container_name in container_names {
        let docker_clone = docker_client.clone();
        let event_clone = g_thread_exit_event.clone();

        tasks.spawn(async move {
            wait_for_container(docker_clone.as_ref(), container_name, event_clone).await
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