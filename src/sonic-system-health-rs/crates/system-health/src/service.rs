//! Service checks: monit, expected containers and the critical processes inside them.

use std::collections::{BTreeSet, HashMap};
use std::path::Path;

use anyhow::{anyhow, Result};
use indexmap::IndexMap;

use crate::config::Config;
use crate::context::CheckContext;
use crate::db::TableData;
use crate::events::Params;
use crate::log;
use crate::monit::{parse_summary, MonitError};
use crate::status::CheckerInfo;
use crate::supervisor::process_states;

/// Inside the container's merged directory.
const CRITICAL_PROCESSES_PATH: &str = "etc/supervisor/critical_processes";
const EXPECTED_STATUS: &str = "OK";
const PROCESS_EVENT_TAG: &str = "process-not-running";
/// Containers managed by K8s, never checked.
const CONTAINER_K8S_WHITELIST: [&str; 3] = ["telemetry", "acms", "restapi"];
/// Containers present only in some images: container name to docker image name.
const OPTIONAL_CONTAINERS: [(&str, &str); 2] = [("otel", "docker-sonic-otel"), ("hft", "docker-hft")];
/// Services that run every instance whatever the ASIC presence.
const RUN_ALL_INSTANCE_LIST: [&str; 2] = ["database", "bgp"];

fn is_disabled(state: &str) -> bool {
    state == "disabled" || state == "always_disabled"
}

#[derive(Default)]
pub struct ServiceChecker {
    container_critical_processes: IndexMap<String, Vec<String>>,
    bad_containers: Vec<String>,
    container_feature: HashMap<String, String>,
}

impl ServiceChecker {
    pub const CATEGORY: &'static str = "Services";
    pub const NAME: &'static str = "ServiceChecker";

    pub async fn check(&mut self, config: &Config, ctx: &CheckContext<'_>) -> Result<CheckerInfo> {
        let mut info = CheckerInfo::default();
        check_by_monit(config, ctx, &mut info).await;
        self.check_services(config, ctx, &mut info).await?;
        Ok(info)
    }

    async fn check_services(&mut self, config: &Config, ctx: &CheckContext<'_>, info: &mut CheckerInfo) -> Result<()> {
        let feature_table = ctx.config_db.get_table("FEATURE")?;
        let (expected, container_feature) = expected_running_containers(&feature_table, config, ctx).await?;
        self.container_feature = container_feature;
        let running = self.current_running_containers(ctx).await?;

        self.container_critical_processes.retain(|c, _| expected.contains(c));

        for container in expected.difference(&running) {
            info.set_object_not_ok("Service", container, format!("Container '{container}' is not running"));
        }

        if self.container_critical_processes.is_empty() {
            info.set_object_not_ok("Service", "system", "no critical process found");
            return Ok(());
        }

        // A stopped container is reported above; Python's docker exec on it printed nothing, so judged nothing.
        for (container, processes) in &self.container_critical_processes {
            if running.contains(container) {
                self.check_process_existence(container, processes, config, &feature_table, ctx, info)
                    .await;
            }
        }

        for container in &self.bad_containers {
            info.set_object_not_ok("Service", container, "Syntax of critical_processes file is incorrect");
        }
        Ok(())
    }

    async fn current_running_containers(&mut self, ctx: &CheckContext<'_>) -> Result<BTreeSet<String>> {
        let mut running = BTreeSet::new();
        let containers = match ctx.docker.running_containers().await {
            Ok(containers) => containers,
            Err(e) => {
                log::error(
                    log::SERVICE_CHECKER,
                    &format!("Failed to retrieve the running container list. Error: '{e}'"),
                );
                return Ok(running);
            }
        };
        for container in containers {
            if container.labels.get("io.kubernetes.pod.namespace").map(String::as_str) == Some("sonic") {
                continue;
            }
            if CONTAINER_K8S_WHITELIST.contains(&container.name.as_str()) {
                continue;
            }
            running.insert(container.name.clone());
            if !self.container_critical_processes.contains_key(&container.name) {
                self.fill_critical_processes(&container.name, ctx).await?;
            }
        }
        Ok(running)
    }

    /// Reads the container's critical_processes file through its merged directory, as Python did.
    async fn fill_critical_processes(&mut self, container: &str, ctx: &CheckContext<'_>) -> Result<()> {
        let folder = ctx
            .commands
            .run(
                &[
                    "docker",
                    "inspect",
                    container,
                    "--format",
                    "{{.GraphDriver.Data.MergedDir}}",
                ],
                crate::CALL_TIMEOUT,
            )
            .await
            .unwrap_or_default();
        let folder = Path::new(folder.trim());
        if folder.as_os_str().is_empty() {
            log::warning(
                log::SERVICE_CHECKER,
                &format!("Could not find MergedDir of container {container}, was container stopped?"),
            );
            return Ok(());
        }
        if !folder.exists() {
            log::warning(
                log::SERVICE_CHECKER,
                &format!(
                    "MergedDir {} of container {container} not found in filesystem, was container stopped?",
                    folder.display()
                ),
            );
            return Ok(());
        }
        let file = folder.join(CRITICAL_PROCESSES_PATH);
        if !file.is_file() {
            log::debug(
                log::SERVICE_CHECKER,
                &format!(
                    "Failed to get critical process file for {container}, {} does not exist",
                    file.display()
                ),
            );
            self.container_critical_processes
                .insert(container.to_string(), Vec::new());
            return Ok(());
        }
        let content = String::from_utf8_lossy(&std::fs::read(&file)?).into_owned();
        let (mut processes, groups) = self.parse_critical_processes(container, &content);

        if !groups.is_empty() {
            let count_before = processes.len();
            match process_states(ctx.commands, container).await {
                Some(states) if !states.is_empty() => {
                    for name in states.keys() {
                        if let Some((group, _)) = name.split_once(':') {
                            if groups.iter().any(|g| g == group) {
                                processes.push(name.clone());
                            }
                        }
                    }
                }
                _ => log::warning(
                    log::SERVICE_CHECKER,
                    &format!("Failed to get supervisorctl status for container {container}, was container stopped?"),
                ),
            }
            // Not cached, so the next cycle retries while supervisord is still starting.
            if processes.len() == count_before {
                log::warning(
                    log::SERVICE_CHECKER,
                    &format!(
                        "Group expansion produced no processes for container {container}; will retry on next check"
                    ),
                );
                return Ok(());
            }
        }
        self.container_critical_processes
            .insert(container.to_string(), processes);
        Ok(())
    }

    /// Program and group names of a critical_processes file, as the Python regex `^\s*((.+):(.*))*\s*$`.
    fn parse_critical_processes(&mut self, container: &str, content: &str) -> (Vec<String>, Vec<String>) {
        let mut processes = Vec::new();
        let mut groups = Vec::new();
        for line in content.split('\n') {
            if line.trim().is_empty() {
                continue;
            }
            let Some(pos) = line.rfind(':').filter(|p| *p > 0) else {
                if !self.bad_containers.iter().any(|c| c == container) {
                    self.bad_containers.push(container.to_string());
                    log::error(
                        log::SERVICE_CHECKER,
                        &format!("Invalid syntax in critical_processes file of {container}"),
                    );
                }
                continue;
            };
            let key = line[..pos].trim();
            let value = line[pos + 1..].trim();
            if value.is_empty() {
                continue;
            }
            match key {
                "program" => processes.push(value.to_string()),
                "group" => groups.push(value.to_string()),
                _ => {}
            }
        }
        (processes, groups)
    }

    async fn check_process_existence(
        &self,
        container: &str,
        processes: &[String],
        config: &Config,
        feature_table: &TableData,
        ctx: &CheckContext<'_>,
        info: &mut CheckerInfo,
    ) {
        let Some(feature) = self.container_feature.get(container) else {
            return;
        };
        let Some(state) = feature_table.get(feature).and_then(|entry| entry.get("state")) else {
            return;
        };
        if is_disabled(state) {
            return;
        }

        let states = match process_states(ctx.commands, container).await {
            Some(states) => states,
            // Timed out: every critical process is not running, each with an event, as in Python.
            None => {
                for process in processes {
                    info.set_object_not_ok(
                        "Process",
                        &format!("{container}:{process}"),
                        format!("Process '{process}' in container '{container}' is not running"),
                    );
                }
                let events: Vec<[(&str, &str); 2]> = processes
                    .iter()
                    .map(|process| [("ctr_name", container), ("process_name", process.as_str())])
                    .collect();
                let batch: Vec<Params> = events.iter().map(|params| &params[..]).collect();
                ctx.events.publish(PROCESS_EVENT_TAG, &batch);
                return;
            }
        };

        for process in processes {
            if config.ignores_service(process) {
                continue;
            }
            // A critical process missing from supervisord's configuration never runs; it is not checked.
            if let Some(state) = states.get(process) {
                let name = format!("{container}:{process}");
                if state != "RUNNING" {
                    info.set_object_not_ok(
                        "Process",
                        &name,
                        format!("Process '{process}' in container '{container}' is not running"),
                    );
                } else {
                    info.set_object_ok("Process", &name);
                }
            }
        }
    }
}

async fn check_by_monit(config: &Config, ctx: &CheckContext<'_>, info: &mut CheckerInfo) {
    let active = ctx
        .commands
        .run(&["systemctl", "is-active", "monit.service"], crate::CALL_TIMEOUT)
        .await
        .unwrap_or_default();
    if active.trim() != "active" {
        info.set_object_not_ok("Service", "monit", "monit service is not running");
        return;
    }
    let services = match ctx.commands.run(&["monit", "summary", "-B"], crate::CALL_TIMEOUT).await {
        Ok(output) => parse_summary(&output),
        Err(_) => Err(MonitError::NotReady),
    };
    match services {
        Err(MonitError::NotReady) => info.set_object_not_ok("Service", "monit", "monit service is not ready"),
        Err(MonitError::Invalid) => info.set_object_not_ok(
            "Service",
            "monit",
            "output of \"monit summary -B\" is invalid or incompatible",
        ),
        Ok(services) => {
            for service in services {
                if config.ignores_service(&service.name) {
                    continue;
                }
                if service.status != EXPECTED_STATUS {
                    info.set_object_not_ok(
                        &service.service_type,
                        &service.name,
                        format!(
                            "{} status is {}, expected {EXPECTED_STATUS}",
                            service.name, service.status
                        ),
                    );
                } else {
                    info.set_object_ok(&service.service_type, &service.name);
                }
            }
        }
    }
}

/// Containers expected to run, and the FEATURE entry of each.
async fn expected_running_containers(
    feature_table: &TableData,
    config: &Config,
    ctx: &CheckContext<'_>,
) -> Result<(BTreeSet<String>, HashMap<String, String>)> {
    let mut optional: IndexMap<String, String> = OPTIONAL_CONTAINERS
        .iter()
        .map(|(c, i)| (c.to_string(), i.to_string()))
        .collect();
    optional.extend(config.optional_containers.clone());

    let mut containers = Vec::new();
    for name in feature_table.keys() {
        if CONTAINER_K8S_WHITELIST.contains(&name.as_str()) {
            log::debug(
                log::SERVICE_CHECKER,
                &format!("Skipping K8s managed container '{name}' from expected running check"),
            );
            continue;
        }
        // frr_bmp is a bgpd option, not a container.
        if name == "frr_bmp" {
            continue;
        }
        if let Some(image) = optional.get(name) {
            if !ctx.docker.image_exists(image).await {
                log::debug(
                    log::SERVICE_CHECKER,
                    &format!("Ignoring {name} container check on image which has no corresponding docker image"),
                );
                continue;
            }
        }
        containers.push(name.clone());
    }

    let multi_asic = ctx.device.is_multi_asic();
    let (num_asics, presence) = if multi_asic {
        (
            ctx.device.num_asics(),
            sonic_rs_common::multi_asic::asic_presence_list(ctx.device),
        )
    } else {
        (1, Vec::new())
    };

    let mut expected = BTreeSet::new();
    let mut container_feature = HashMap::new();
    for name in containers {
        let entry = &feature_table[&name];
        let state = entry.get("state").ok_or_else(|| anyhow!("KeyError('state')"))?;
        if is_disabled(state) {
            continue;
        }
        if multi_asic {
            if entry.get("has_global_scope").map_or("True", String::as_str) == "True" {
                expected.insert(name.clone());
                container_feature.insert(name.clone(), name.clone());
            }
            if entry.get("has_per_asic_scope").map_or("False", String::as_str) == "True" {
                for asic in 0..num_asics {
                    if presence.contains(&asic) || RUN_ALL_INSTANCE_LIST.contains(&name.as_str()) {
                        let instance = format!("{name}{asic}");
                        expected.insert(instance.clone());
                        container_feature.insert(instance, name.clone());
                    }
                }
            }
        } else {
            expected.insert(name.clone());
            container_feature.insert(name.clone(), name.clone());
        }
    }

    if ctx.device.is_supervisor() || ctx.device.is_disaggregated_chassis() {
        expected.insert("database-chassis".to_string());
        container_feature.insert("database-chassis".to_string(), "database".to_string());
    }
    Ok((expected, container_feature))
}
