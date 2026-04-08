use bollard::container::*;
use bollard::Docker;
use futures_util::stream::TryStreamExt;

pub struct Container<'a> {
    feature: &'a str,
}

#[derive(thiserror::Error, Debug)]
pub enum Error {
    #[error("Unable to parse JSON file")]
    JSONParse(#[from] serde_json::Error),
    #[error("Docker error")]
    Docker(#[from] bollard::errors::Error),
    #[error("Unable to open file")]
    IO(#[from] std::io::Error),
    #[error("UTF-8 parsing error")]
    UTF8(#[from] std::str::Utf8Error),
    #[error("Syslog initialization error: {0}")]
    Syslog(String),
}

impl<'a> Container<'a> {
    pub fn new(feature: &str) -> Container {
        Container {
            feature,
        }
    }

    pub async fn start(&self) -> Result<(), Error> {
        let docker = Docker::connect_with_local_defaults()?;
        docker
            .start_container(self.feature, None::<StartContainerOptions<String>>)
            .await?;
        Ok(())
    }

    pub async fn stop(&self, timeout: Option<i64>) -> Result<(), Error> {
        let docker = Docker::connect_with_local_defaults()?;
        let stop_options = timeout.map(|timeout| StopContainerOptions { t: timeout });
        docker.stop_container(self.feature, stop_options).await?;
        Ok(())
    }

    pub async fn kill(&self) -> Result<(), Error> {
        let docker = Docker::connect_with_local_defaults()?;
        docker
            .kill_container(self.feature, Some(KillContainerOptions { signal: "SIGINT" }))
            .await?;
        Ok(())
    }

    pub async fn wait(&self) -> Result<(), Error> {
        let docker = Docker::connect_with_local_defaults()?;
        docker
            .wait_container(
                self.feature,
                Some(WaitContainerOptions {
                    condition: "not-running",
                }),
            )
            .try_collect::<Vec<_>>()
            .await?;
        Ok(())
    }
}
