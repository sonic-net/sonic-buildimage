//! Mocks for every external dependency of a check cycle.

#![allow(dead_code)]

use std::cell::RefCell;
use std::collections::{HashMap, HashSet};
use std::fs;
use std::path::PathBuf;
use std::time::Duration;

use anyhow::{anyhow, Result};
use async_trait::async_trait;
use sonic_rs_common::DeviceInfo;
use system_health::command::{CommandError, CommandRunner};
use system_health::context::CheckContext;
use system_health::db::memory::{MemoryConfigDb, MemoryStateDb};
use system_health::docker::{ContainerRuntime, ContainerSummary};
use system_health::events::RecordedEvents;
use system_health::systemd::{Systemd, UnitProperties};

pub const PLATFORM: &str = "x86_64-mlnx_msn2700-r0";

#[derive(Default)]
pub struct MockDocker {
    pub containers: RefCell<Vec<ContainerSummary>>,
    pub images: RefCell<HashSet<String>>,
    pub list_fails: RefCell<bool>,
}

impl MockDocker {
    pub fn run(&self, name: &str) {
        self.containers.borrow_mut().push(ContainerSummary {
            name: name.to_string(),
            labels: HashMap::new(),
        });
    }

    pub fn run_with_label(&self, name: &str, label: (&str, &str)) {
        self.containers.borrow_mut().push(ContainerSummary {
            name: name.to_string(),
            labels: HashMap::from([(label.0.to_string(), label.1.to_string())]),
        });
    }

    pub fn stop(&self, name: &str) {
        self.containers.borrow_mut().retain(|c| c.name != name);
    }
}

#[async_trait(?Send)]
impl ContainerRuntime for MockDocker {
    async fn running_containers(&self) -> Result<Vec<ContainerSummary>> {
        if *self.list_fails.borrow() {
            return Err(anyhow!("docker API error"));
        }
        Ok(self.containers.borrow().clone())
    }

    async fn image_exists(&self, image: &str) -> bool {
        self.images.borrow().contains(image)
    }
}

/// Unit properties by unit name; a missing unit fails as a D-Bus timeout would.
#[derive(Default)]
pub struct MockSystemd {
    pub units: RefCell<HashMap<String, UnitProperties>>,
}

impl MockSystemd {
    pub fn set(&self, unit: &str, props: UnitProperties) {
        self.units.borrow_mut().insert(unit.to_string(), props);
    }

    pub fn set_active(&self, unit: &str, active_state: &str) {
        self.set(
            unit,
            UnitProperties {
                active_state: active_state.to_string(),
                ..Default::default()
            },
        );
    }
}

#[async_trait(?Send)]
impl Systemd for MockSystemd {
    async fn unit_properties(&self, unit: &str) -> Result<UnitProperties> {
        self.units
            .borrow()
            .get(unit)
            .cloned()
            .ok_or_else(|| anyhow!("timed out reading {unit} from systemd"))
    }
}

pub fn unit(load: &str, file_state: &str, unit_type: &str, active: &str, sub: &str, result: &str) -> UnitProperties {
    UnitProperties {
        load_state: load.to_string(),
        unit_file_state: file_state.to_string(),
        unit_type: unit_type.to_string(),
        active_state: active.to_string(),
        sub_state: sub.to_string(),
        result: result.to_string(),
        condition_result: "yes".to_string(),
        condition_timestamp_monotonic: "1000".to_string(),
    }
}

/// Command output by the joined argv; a missing command times out.
#[derive(Default)]
pub struct MockCommands {
    pub outputs: RefCell<HashMap<String, Result<String, CommandError>>>,
    pub calls: RefCell<Vec<(String, Duration)>>,
}

impl MockCommands {
    pub fn set(&self, cmd: &str, output: &str) {
        self.outputs
            .borrow_mut()
            .insert(cmd.to_string(), Ok(output.to_string()));
    }

    pub fn set_error(&self, cmd: &str, error: CommandError) {
        self.outputs.borrow_mut().insert(cmd.to_string(), Err(error));
    }

    pub fn was_run(&self, cmd: &str) -> bool {
        self.calls.borrow().iter().any(|(c, _)| c == cmd)
    }
}

/// The command lines of the Python daemon, as MockCommands keys them.
pub fn supervisorctl_status(container: &str) -> String {
    format!("docker exec {container} bash -c supervisorctl status")
}

pub fn inspect_merged_dir(container: &str) -> String {
    format!("docker inspect {container} --format {{{{.GraphDriver.Data.MergedDir}}}}")
}

pub const MONIT_ACTIVE: &str = "systemctl is-active monit.service";

#[async_trait(?Send)]
impl CommandRunner for MockCommands {
    async fn run(&self, argv: &[&str], timeout: Duration) -> Result<String, CommandError> {
        let cmd = match argv {
            ["/bin/sh", "-c", cmd] => cmd.to_string(),
            _ => argv.join(" "),
        };
        self.calls.borrow_mut().push((cmd.clone(), timeout));
        match self.outputs.borrow().get(&cmd) {
            Some(Ok(out)) => Ok(out.clone()),
            Some(Err(CommandError::Failed(e))) => Err(CommandError::Failed(e.clone())),
            _ => Err(CommandError::Timeout),
        }
    }
}

pub const MONIT_OK: &str = "Monit 5.34.3 uptime: 23h 11m
 Service Name                     Status                      Type
 sonic                            OK                          System
 rsyslog                          OK                          Process
 root-overlay                     OK                          Filesystem
";

/// A device root, a /proc tree and container merged directories in a temporary directory, and every mock.
pub struct Harness {
    pub dir: tempfile::TempDir,
    pub device: DeviceInfo,
    pub state_db: MemoryStateDb,
    pub config_db: MemoryConfigDb,
    pub docker: MockDocker,
    pub systemd: MockSystemd,
    pub commands: MockCommands,
    pub events: RecordedEvents,
}

impl Harness {
    pub fn new() -> Self {
        let dir = tempfile::tempdir().unwrap();
        let root = dir.path().join("root");
        fs::create_dir_all(root.join("host")).unwrap();
        fs::write(root.join("host/machine.conf"), format!("onie_platform={PLATFORM}\n")).unwrap();
        fs::create_dir_all(root.join("usr/share/sonic/device").join(PLATFORM)).unwrap();
        fs::create_dir_all(dir.path().join("proc")).unwrap();
        fs::write(dir.path().join("proc/uptime"), "1000.50 2000.00\n").unwrap();
        let harness = Self {
            device: DeviceInfo::with_root(&root),
            dir,
            state_db: MemoryStateDb::default(),
            config_db: MemoryConfigDb::default(),
            docker: MockDocker::default(),
            systemd: MockSystemd::default(),
            commands: MockCommands::default(),
            events: RecordedEvents::default(),
        };
        harness.commands.set(MONIT_ACTIVE, "active\n");
        harness.commands.set("monit summary -B", MONIT_OK);
        harness
    }

    pub fn ctx(&self) -> CheckContext<'_> {
        CheckContext {
            state_db: &self.state_db,
            config_db: &self.config_db,
            docker: &self.docker,
            commands: &self.commands,
            events: &self.events,
            device: &self.device,
            proc_root: self.dir.path().join("proc"),
        }
    }

    pub fn platform_dir(&self) -> PathBuf {
        self.dir.path().join("root/usr/share/sonic/device").join(PLATFORM)
    }

    pub fn set_platform_file(&self, name: &str, content: &str) {
        fs::write(self.platform_dir().join(name), content).unwrap();
    }

    pub fn set_uptime(&self, seconds: f64) {
        fs::write(self.dir.path().join("proc/uptime"), format!("{seconds} 0\n")).unwrap();
    }

    /// The container's merged directory, as `docker inspect` reports it.
    pub fn merged_dir(&self, name: &str) -> PathBuf {
        self.dir.path().join("docker").join(name).join("merged")
    }

    /// A running container with an enabled FEATURE entry and a merged directory, without critical_processes.
    pub fn add_container_without_file(&self, name: &str) {
        self.config_db.set("FEATURE", name, &[("state", "enabled")]);
        self.docker.run(name);
        let merged = self.merged_dir(name);
        fs::create_dir_all(merged.join("etc/supervisor")).unwrap();
        self.commands
            .set(&inspect_merged_dir(name), &format!("{}\n", merged.display()));
    }

    /// A running container with an enabled FEATURE entry, its critical_processes file and supervisord states.
    pub fn add_container(&self, name: &str, critical: &str, states: &[(&str, &str)]) {
        self.add_container_without_file(name);
        fs::write(
            self.merged_dir(name).join("etc/supervisor/critical_processes"),
            critical,
        )
        .unwrap();
        self.set_supervisor(name, states);
    }

    /// What `supervisorctl status` prints in the container.
    pub fn set_supervisor(&self, name: &str, states: &[(&str, &str)]) {
        let output: String = states
            .iter()
            .map(|(process, state)| format!("{process:<32} {state:<9} pid 1, uptime 0:01:00\n"))
            .collect();
        self.commands.set(&supervisorctl_status(name), &output);
    }

    pub fn set_supervisor_output(&self, name: &str, output: &str) {
        self.commands.set(&supervisorctl_status(name), output);
    }

    pub fn set_supervisor_timeout(&self, name: &str) {
        self.commands
            .set_error(&supervisorctl_status(name), CommandError::Timeout);
    }
}
