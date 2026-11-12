//! Running containers and images through the Docker API, as the Python daemon's docker client.

use std::collections::HashMap;

use anyhow::Result;
use async_trait::async_trait;
use bollard::container::ListContainersOptions;
use bollard::Docker;

#[derive(Clone, Debug, Default, PartialEq, Eq)]
pub struct ContainerSummary {
    pub name: String,
    pub labels: HashMap<String, String>,
}

#[async_trait(?Send)]
pub trait ContainerRuntime {
    async fn running_containers(&self) -> Result<Vec<ContainerSummary>>;
    async fn image_exists(&self, image: &str) -> bool;
}

/// Connects to the Docker socket on first use, and again on the next call after a failure.
#[derive(Default)]
pub struct BollardRuntime {
    docker: std::sync::OnceLock<Docker>,
}

impl BollardRuntime {
    fn docker(&self) -> Result<&Docker> {
        if let Some(docker) = self.docker.get() {
            return Ok(docker);
        }
        let docker = Docker::connect_with_unix(
            "unix:///var/run/docker.sock",
            crate::CALL_TIMEOUT.as_secs(),
            bollard::API_DEFAULT_VERSION,
        )?;
        Ok(self.docker.get_or_init(|| docker))
    }
}

#[async_trait(?Send)]
impl ContainerRuntime for BollardRuntime {
    async fn running_containers(&self) -> Result<Vec<ContainerSummary>> {
        let options = ListContainersOptions::<String> {
            filters: HashMap::from([("status".to_string(), vec!["running".to_string()])]),
            ..Default::default()
        };
        let containers = self.docker()?.list_containers(Some(options)).await?;
        Ok(containers
            .into_iter()
            .filter_map(|c| {
                let name = c.names?.into_iter().next()?.trim_start_matches('/').to_string();
                Some(ContainerSummary {
                    name,
                    labels: c.labels.unwrap_or_default(),
                })
            })
            .collect())
    }

    async fn image_exists(&self, image: &str) -> bool {
        match self.docker() {
            Ok(docker) => docker.inspect_image(image).await.is_ok(),
            Err(_) => false,
        }
    }
}
