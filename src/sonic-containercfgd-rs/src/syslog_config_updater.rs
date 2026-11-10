use std::{
    collections::HashMap,
    fs::{self, File},
    io::{BufRead, BufReader},
    process::Command,
};

use anyhow::Context;
use log::{debug, info, warn};
use regex::regex;
use swss_common::{CxxString, KeyOperation};

use crate::db_listener::SonicDatabaseChanges;

#[derive(Debug)]
pub struct SyslogConfigUpdater {
    service_name: String,
    rate_limit_interval: u32,
    rate_limit_burst: u32,
}

impl SyslogConfigUpdater {
    pub fn new(service_name: &str) -> anyhow::Result<Self> {
        anyhow::ensure!(
            !service_name.contains("\""),
            "Service name {service_name} contains \", which is not allowed"
        );

        let file = File::open("/etc/rsyslog.conf")
            .context("Unable to open rsyslogd config file at /etc/rsyslog.conf")?;
        let buf_reader = BufReader::new(file);
        let mut rate_limit_interval = 0;
        let mut rate_limit_burst = 0;
        for line in buf_reader.lines() {
            let line_content = line.context("Unable to read rsyslogd config file")?;
            if let Some(interval_regex_capture) =
                regex!(r#".*SysSock.RateLimit.Interval\s*=\s*"(\d+)".*"#).captures(&line_content)
            {
                let current_interval_str = interval_regex_capture
                    .get(1)
                    .context("Missing capture group for getting interval regex")?
                    .as_str();
                rate_limit_interval = current_interval_str
                    .parse()
                    .context("Unable to parse interval")
                    .unwrap();
            }
            if let Some(burst_regex_capture) =
                regex!(r#".*SysSock.RateLimit.Burst\s*=\s*"(\d+)".*"#).captures(&line_content)
            {
                let current_burst_str = burst_regex_capture
                    .get(1)
                    .context("Missing capture group for getting interval regex")?
                    .as_str();
                rate_limit_burst = current_burst_str
                    .parse()
                    .context("Unable to parse burst")
                    .unwrap();
            }
        }

        Ok(SyslogConfigUpdater {
            service_name: service_name.to_owned(),
            rate_limit_interval,
            rate_limit_burst,
        })
    }
}

impl SonicDatabaseChanges for SyslogConfigUpdater {
    /// Process changes to the syslog table in CONFIG_DB. Update the rate limit interval and burst
    /// if they have changed, regenerate rsyslog.conf, and restart rsyslogd.
    fn handle_change(
        &mut self,
        _operation: KeyOperation,
        entry: &str,
        field_values: &HashMap<String, CxxString>,
    ) -> anyhow::Result<()> {
        if entry != self.service_name {
            debug!("Ignoring event that came in for {entry}");
            return Ok(());
        }

        let mut values_changed = false;

        let new_rate_limit_interval = match field_values.get("rate_limit_interval") {
            Some(value) => value
                .to_str()?
                .parse()
                .context("Unable to parse rate_limit_interval as integer")?,
            None => 0,
        };
        if self.rate_limit_interval != new_rate_limit_interval {
            info!("Setting rate limit interval to {}", new_rate_limit_interval);
            self.rate_limit_interval = new_rate_limit_interval;
            values_changed = true;
        }

        let new_rate_limit_burst = match field_values.get("rate_limit_burst") {
            Some(value) => value
                .to_str()?
                .parse()
                .context("Unable to parse rate_limit_burst as integer")?,
            None => 0,
        };
        if self.rate_limit_burst != new_rate_limit_burst {
            info!("Setting rate limit burst to {}", new_rate_limit_burst);
            self.rate_limit_burst = new_rate_limit_burst;
            values_changed = true;
        }

        if values_changed {
            let file = File::options()
                .create(true)
                .write(true)
                .truncate(true)
                .open("/tmp/rsyslog.conf")?;

            let json_args = format!(r#"{{"container_name": "{}"}}"#, self.service_name);
            let exit_code = Command::new("sonic-cfggen")
                .args([
                    "-d",
                    "-t",
                    "/usr/share/sonic/templates/rsyslog-container.conf.j2",
                    "-a",
                    &json_args,
                ])
                .stdout(file)
                .status()?;
            anyhow::ensure!(
                exit_code.success(),
                "Failed to generate the updated rsyslog.conf: {exit_code}"
            );
            fs::copy("/tmp/rsyslog.conf", "/etc/rsyslog.conf")
                .context("Unable to replace /etc/rsyslog.conf with the updated configuration")?;
            if let Err(err) = fs::remove_file("/tmp/rsyslog.conf") {
                warn!("Unable to remove temporary file: {}", err);
            }
            let exit_code = Command::new("supervisorctl")
                .args(["restart", "rsyslogd"])
                .status()?;
            anyhow::ensure!(
                exit_code.success(),
                "Failed to restart rsyslog: {exit_code}"
            );
        }

        Ok(())
    }
}
