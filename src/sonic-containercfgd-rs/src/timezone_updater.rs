use std::{
    collections::HashMap,
    ffi::OsStr,
    os::unix::fs::symlink,
    path::{Path, PathBuf},
    process::Command,
};

#[cfg(not(test))]
use std::fs;

#[cfg(test)]
use tests::fs;

use anyhow::Context;
use log::{debug, error, info};
use swss_common::{CxxString, KeyOperation};

use crate::db_listener::SonicDatabaseChanges;

const TIMEZONE_FILE: &str = "/etc/localtime";

#[derive(Debug)]
pub struct TimezoneUpdater {
    current_timezone: String,
}

impl TimezoneUpdater {
    pub fn new() -> anyhow::Result<Self> {
        let timezone_path = fs::read_link(TIMEZONE_FILE)
            .context("Timezone file is either missing or not a symlink")?;
        let mut timezone_path_components = timezone_path
            .components()
            .skip_while(|x| x.as_os_str() != OsStr::new("zoneinfo"));
        timezone_path_components.next();
        let current_timezone = PathBuf::from_iter(timezone_path_components)
            .to_str()
            .context("Invalid characters present in timezone name")?
            .to_owned();

        info!("Currently-configured timezone is {current_timezone}");

        Ok(TimezoneUpdater { current_timezone })
    }
}

impl SonicDatabaseChanges for TimezoneUpdater {
    /// Process changes to the timezone field of CONFIG_DB. If the timezone has changed, then update
    /// the /etc/localtime symlink file to point to the new timezone.
    fn handle_change(
        &mut self,
        operation: KeyOperation,
        _entry: &str,
        field_values: &HashMap<String, CxxString>,
    ) -> anyhow::Result<()> {
        match operation {
            KeyOperation::Set => {
                let mut values_changed = false;

                for (field, value) in field_values {
                    if field.as_str() == "timezone" {
                        let value = value
                            .to_str()
                            .context("Unable to parse timezone as a string")?;
                        if self.current_timezone != value {
                            info!("Setting timezone to {}", value);
                            self.current_timezone = value.to_owned();
                            values_changed = true;
                        }
                    }
                }

                if values_changed {
                    let timezone_name = Path::new(&self.current_timezone);
                    anyhow::ensure!(
                        !timezone_name.is_absolute()
                            && !timezone_name
                                .components()
                                .any(|component| component == std::path::Component::ParentDir),
                        "Invalid timezone name: {}",
                        &self.current_timezone
                    );
                    let timezone_path = Path::new("/usr/share/zoneinfo").join(timezone_name);
                    if !timezone_path.exists() {
                        error!(
                            "Timezone file at {timezone_path:?} doesn't exist, not updating timezone"
                        );
                    }
                    let timezone_symlink_path = Path::new("/etc/localtime");
                    if timezone_symlink_path.exists() {
                        fs::remove_file("/etc/localtime")
                            .context("Unable to remove existing /etc/localtime file")?;
                    }
                    symlink(timezone_path, "/etc/localtime").context(
                        "Unable to update /etc/timezone symlink to current timezone file",
                    )?;
                    let exit_code = Command::new("supervisorctl")
                        .args(["restart", "rsyslogd"])
                        .status()?;
                    anyhow::ensure!(
                        exit_code.success(),
                        "Failed to restart rsyslog after timezone update: {exit_code}"
                    );
                }
            }
            KeyOperation::Del => debug!("Not doing anything for key deletion"),
        }
        Ok(())
    }
}

#[cfg(test)]
mod tests {
    use std::cell::RefCell;

    use super::*;

    thread_local! {
        static TIMEZONE_SYMLINK_TARGET: RefCell<Option<PathBuf>> = const { RefCell::new(None) };
    }

    #[test]
    fn one_level_timezone() {
        TIMEZONE_SYMLINK_TARGET.replace(Some(PathBuf::from("/usr/share/zoneinfo/UTC")));
        let updater = TimezoneUpdater::new().unwrap();
        assert_eq!(updater.current_timezone, "UTC");
    }

    #[test]
    fn two_level_timezone() {
        TIMEZONE_SYMLINK_TARGET.replace(Some(PathBuf::from(
            "/usr/share/zoneinfo/America/Los_Angeles",
        )));
        let updater = TimezoneUpdater::new().unwrap();
        assert_eq!(updater.current_timezone, "America/Los_Angeles");
    }

    #[test]
    fn three_level_timezone() {
        TIMEZONE_SYMLINK_TARGET.replace(Some(PathBuf::from(
            "/usr/share/zoneinfo/America/Indiana/Vevay",
        )));
        let updater = TimezoneUpdater::new().unwrap();
        assert_eq!(updater.current_timezone, "America/Indiana/Vevay");
    }

    #[test]
    fn relative_timezone_symlink() {
        TIMEZONE_SYMLINK_TARGET.replace(Some(PathBuf::from(
            "../usr/share/zoneinfo/Australia/Darwin",
        )));
        let updater = TimezoneUpdater::new().unwrap();
        assert_eq!(updater.current_timezone, "Australia/Darwin");
    }

    #[test]
    fn shorter_relative_timezone_symlink() {
        TIMEZONE_SYMLINK_TARGET.replace(Some(PathBuf::from("../zoneinfo/Europe/Helsinki")));
        let updater = TimezoneUpdater::new().unwrap();
        assert_eq!(updater.current_timezone, "Europe/Helsinki");
    }

    pub mod fs {
        use std::{
            io::{self, Error, ErrorKind},
            path::Path,
        };

        use super::*;

        pub fn read_link<P: AsRef<Path>>(path: P) -> io::Result<PathBuf> {
            if path
                .as_ref()
                .to_str()
                .ok_or(Error::from(ErrorKind::InvalidInput))?
                == TIMEZONE_FILE
            {
                if let Some(path) = TIMEZONE_SYMLINK_TARGET.take() {
                    Ok(path)
                } else {
                    Err(Error::from(ErrorKind::NotFound))
                }
            } else {
                Err(Error::from(ErrorKind::NotFound))
            }
        }

        pub fn remove_file<P: AsRef<Path>>(path: P) -> io::Result<()> {
            if path
                .as_ref()
                .to_str()
                .ok_or(Error::from(ErrorKind::InvalidInput))?
                == TIMEZONE_FILE
            {
                Ok(())
            } else {
                Err(Error::from(ErrorKind::NotFound))
            }
        }
    }
}
