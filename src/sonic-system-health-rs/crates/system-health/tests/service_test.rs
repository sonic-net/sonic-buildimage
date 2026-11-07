mod common;

use common::{inspect_merged_dir, supervisorctl_status, Harness, MONIT_ACTIVE};
use system_health::config::Config;
use system_health::service::ServiceChecker;
use system_health::status::{CheckerInfo, STATUS_NOT_OK, STATUS_OK};

fn config(ignore_services: &[&str]) -> Config {
    let dir = std::env::temp_dir();
    let mut config = Config::new(dir.join("no-such-config.json"), dir.join("no-such-monitrc"));
    if !ignore_services.is_empty() {
        config.ignore_services = Some(ignore_services.iter().map(|s| s.to_string()).collect());
    }
    config
}

fn status(info: &CheckerInfo, name: &str) -> String {
    info.get(name)
        .map(|o| o.status.clone())
        .unwrap_or_else(|| panic!("{name} not checked"))
}

fn message(info: &CheckerInfo, name: &str) -> String {
    info.get(name)
        .map(|o| o.message.clone())
        .unwrap_or_else(|| panic!("{name} not checked"))
}

#[tokio::test]
async fn single_asic_processes_and_containers() {
    let h = Harness::new();
    h.add_container(
        "swss",
        "program:orchagent\nprogram:portsyncd\nprogram:radv\n",
        &[
            ("orchagent", "RUNNING"),
            ("portsyncd", "EXITED"),
            ("supervisor-proc-exit-listener", "RUNNING"),
        ],
    );
    h.add_container("lldp", "program:lldpd\n", &[("lldpd", "RUNNING")]);
    h.config_db.set("FEATURE", "snmp", &[("state", "enabled")]);
    h.config_db.set("FEATURE", "pmon", &[("state", "disabled")]);

    let mut checker = ServiceChecker::default();
    let info = checker.check(&config(&[]), &h.ctx()).await.unwrap();

    assert_eq!(status(&info, "swss:orchagent"), STATUS_OK);
    assert_eq!(info.get("swss:orchagent").unwrap().object_type, "Process");
    assert_eq!(
        message(&info, "swss:portsyncd"),
        "Process 'portsyncd' in container 'swss' is not running"
    );
    // In critical_processes but not in supervisord's configuration: never runs, not checked.
    assert!(info.get("swss:radv").is_none());
    assert_eq!(status(&info, "lldp:lldpd"), STATUS_OK);
    assert_eq!(message(&info, "snmp"), "Container 'snmp' is not running");
    assert!(info.get("pmon").is_none());
    assert_eq!(status(&info, "sonic"), STATUS_OK);
    assert_eq!(info.get("sonic").unwrap().object_type, "System");
}

#[tokio::test]
async fn ignored_process_is_not_checked() {
    let h = Harness::new();
    h.add_container("swss", "program:orchagent\n", &[("orchagent", "STOPPED")]);
    let info = ServiceChecker::default()
        .check(&config(&["orchagent"]), &h.ctx())
        .await
        .unwrap();
    assert!(info.get("swss:orchagent").is_none());
}

#[tokio::test]
async fn group_expansion() {
    let h = Harness::new();
    h.add_container(
        "dhcp_relay",
        "group:dhcp-relay\nprogram:dhcpmon\n",
        &[
            ("dhcp-relay:isc-dhcpv4-relay-Vlan1000", "RUNNING"),
            ("dhcp-relay:dhcp6relay", "FATAL"),
            ("dhcpmon", "RUNNING"),
        ],
    );
    let info = ServiceChecker::default().check(&config(&[]), &h.ctx()).await.unwrap();
    assert_eq!(status(&info, "dhcp_relay:dhcpmon"), STATUS_OK);
    assert_eq!(
        status(&info, "dhcp_relay:dhcp-relay:isc-dhcpv4-relay-Vlan1000"),
        STATUS_OK
    );
    assert_eq!(
        message(&info, "dhcp_relay:dhcp-relay:dhcp6relay"),
        "Process 'dhcp-relay:dhcp6relay' in container 'dhcp_relay' is not running"
    );
}

#[tokio::test]
async fn group_expansion_retries_while_supervisord_is_starting() {
    let h = Harness::new();
    h.add_container("dhcp_relay", "group:dhcp-relay\n", &[]);
    let mut checker = ServiceChecker::default();
    let info = checker.check(&config(&[]), &h.ctx()).await.unwrap();
    assert_eq!(message(&info, "system"), "no critical process found");

    h.set_supervisor("dhcp_relay", &[("dhcp-relay:dhcp6relay", "RUNNING")]);
    let info = checker.check(&config(&[]), &h.ctx()).await.unwrap();
    assert_eq!(status(&info, "dhcp_relay:dhcp-relay:dhcp6relay"), STATUS_OK);
}

#[tokio::test]
async fn container_without_critical_processes_file() {
    let h = Harness::new();
    h.add_container("swss", "program:orchagent\n", &[("orchagent", "RUNNING")]);
    h.add_container_without_file("mgmt-framework");
    let info = ServiceChecker::default().check(&config(&[]), &h.ctx()).await.unwrap();
    assert_eq!(status(&info, "swss:orchagent"), STATUS_OK);
    assert!(!info.info().keys().any(|k| k.starts_with("mgmt-framework")));
    assert!(!info.has_not_ok());
}

#[tokio::test]
async fn no_critical_process_found() {
    let h = Harness::new();
    let info = ServiceChecker::default().check(&config(&[]), &h.ctx()).await.unwrap();
    assert_eq!(message(&info, "system"), "no critical process found");
    assert_eq!(info.get("system").unwrap().object_type, "Service");
}

#[tokio::test]
async fn bad_critical_processes_syntax() {
    let h = Harness::new();
    h.add_container(
        "swss",
        "program:orchagent\njunk line\n\n:x\n",
        &[("orchagent", "RUNNING")],
    );
    let info = ServiceChecker::default().check(&config(&[]), &h.ctx()).await.unwrap();
    assert_eq!(status(&info, "swss:orchagent"), STATUS_OK);
    assert_eq!(message(&info, "swss"), "Syntax of critical_processes file is incorrect");
}

#[tokio::test]
async fn supervisorctl_timeout_fails_every_critical_process_and_sends_events() {
    // As in Python: on a timeout every critical process, an ignored one included, is not running and gets an
    // event, every cycle, the container's events in one batch; a process found stopped sends none.
    let h = Harness::new();
    h.add_container(
        "swss",
        "program:orchagent\nprogram:portsyncd\nprogram:radv\n",
        &[("orchagent", "STOPPED"), ("portsyncd", "RUNNING"), ("radv", "RUNNING")],
    );
    let mut checker = ServiceChecker::default();
    let info = checker.check(&config(&["radv"]), &h.ctx()).await.unwrap();
    assert_eq!(status(&info, "swss:orchagent"), STATUS_NOT_OK);
    assert!(take_process_events(&h).is_empty());

    h.set_supervisor_timeout("swss");
    for _ in 0..2 {
        let info = checker.check(&config(&["radv"]), &h.ctx()).await.unwrap();
        for process in ["orchagent", "portsyncd", "radv"] {
            assert_eq!(
                message(&info, &format!("swss:{process}")),
                format!("Process '{process}' in container 'swss' is not running")
            );
        }
        assert_eq!(
            take_process_events(&h),
            [
                process_event("swss", "orchagent"),
                process_event("swss", "portsyncd"),
                process_event("swss", "radv")
            ]
        );
        assert_eq!(h.events.batches.borrow_mut().drain(..).collect::<Vec<_>>(), [3]);
    }
}

#[tokio::test]
async fn stopped_container_keeps_its_processes_unjudged() {
    let h = Harness::new();
    h.add_container("swss", "program:orchagent\n", &[("orchagent", "RUNNING")]);
    let mut checker = ServiceChecker::default();
    checker.check(&config(&[]), &h.ctx()).await.unwrap();

    h.docker.stop("swss");
    let info = checker.check(&config(&[]), &h.ctx()).await.unwrap();
    assert_eq!(message(&info, "swss"), "Container 'swss' is not running");
    assert!(info.get("swss:orchagent").is_none());
    assert!(h.events.events.borrow().is_empty());
}

#[tokio::test]
async fn disabled_feature_is_dropped() {
    let h = Harness::new();
    h.add_container("swss", "program:orchagent\n", &[("orchagent", "RUNNING")]);
    h.add_container("sflow", "program:sflowmgrd\n", &[("sflowmgrd", "STOPPED")]);
    let mut checker = ServiceChecker::default();
    let info = checker.check(&config(&[]), &h.ctx()).await.unwrap();
    assert_eq!(status(&info, "sflow:sflowmgrd"), STATUS_NOT_OK);

    h.config_db.set("FEATURE", "sflow", &[("state", "disabled")]);
    let info = checker.check(&config(&[]), &h.ctx()).await.unwrap();
    assert!(info.get("sflow:sflowmgrd").is_none());
    assert!(info.get("sflow").is_none());
}

#[tokio::test]
async fn kubernetes_and_whitelisted_containers_are_skipped() {
    let h = Harness::new();
    h.add_container("swss", "program:orchagent\n", &[("orchagent", "RUNNING")]);
    h.config_db.set("FEATURE", "telemetry", &[("state", "enabled")]);
    h.config_db.set("FEATURE", "restapi", &[("state", "enabled")]);
    h.docker
        .run_with_label("k8s_snmp", ("io.kubernetes.pod.namespace", "sonic"));
    let info = ServiceChecker::default().check(&config(&[]), &h.ctx()).await.unwrap();
    assert!(info.get("telemetry").is_none());
    assert!(info.get("restapi").is_none());
    assert!(!info.info().keys().any(|k| k.starts_with("k8s_snmp")));
    assert!(!h.commands.was_run(&inspect_merged_dir("k8s_snmp")));
    assert!(!h.commands.was_run(&supervisorctl_status("k8s_snmp")));
}

#[tokio::test]
async fn optional_containers_need_their_image() {
    let h = Harness::new();
    h.add_container("swss", "program:orchagent\n", &[("orchagent", "RUNNING")]);
    h.config_db.set("FEATURE", "otel", &[("state", "enabled")]);
    h.config_db.set("FEATURE", "hft", &[("state", "enabled")]);
    h.config_db.set("FEATURE", "extra", &[("state", "enabled")]);
    let mut config = config(&[]);
    config
        .optional_containers
        .insert("extra".to_string(), "docker-extra".to_string());
    let info = ServiceChecker::default().check(&config, &h.ctx()).await.unwrap();
    assert!(info.get("otel").is_none());
    assert!(info.get("hft").is_none());
    assert!(info.get("extra").is_none());

    h.docker.images.borrow_mut().insert("docker-sonic-otel".to_string());
    h.docker.images.borrow_mut().insert("docker-hft".to_string());
    let info = ServiceChecker::default().check(&config, &h.ctx()).await.unwrap();
    assert_eq!(message(&info, "otel"), "Container 'otel' is not running");
    assert_eq!(message(&info, "hft"), "Container 'hft' is not running");
}

#[tokio::test]
async fn frr_bmp_is_not_a_container() {
    let h = Harness::new();
    h.add_container("swss", "program:orchagent\n", &[("orchagent", "RUNNING")]);
    h.config_db.set("FEATURE", "frr_bmp", &[("state", "enabled")]);
    let info = ServiceChecker::default().check(&config(&[]), &h.ctx()).await.unwrap();
    assert!(info.get("frr_bmp").is_none());
}

#[tokio::test]
async fn multi_asic_instances() {
    let h = Harness::new();
    h.set_platform_file("asic.conf", "NUM_ASIC=3\n");
    let per_asic = [
        ("state", "enabled"),
        ("has_global_scope", "False"),
        ("has_per_asic_scope", "True"),
    ];
    h.config_db.set("FEATURE", "swss", &per_asic);
    h.config_db.set(
        "FEATURE",
        "database",
        &[
            ("state", "enabled"),
            ("has_global_scope", "True"),
            ("has_per_asic_scope", "True"),
        ],
    );
    h.config_db.set("FEATURE", "snmp", &[("state", "enabled")]);
    h.add_container("snmp", "program:snmpd\n", &[("snmpd", "RUNNING")]);
    for name in ["swss0", "swss1", "database", "database0", "database1", "database2"] {
        h.docker.run(name);
    }
    let info = ServiceChecker::default().check(&config(&[]), &h.ctx()).await.unwrap();
    assert_eq!(message(&info, "swss2"), "Container 'swss2' is not running");
    assert!(info.get("swss").is_none());
    assert!(info.get("database").is_none());
    assert_eq!(status(&info, "snmp:snmpd"), STATUS_OK);
}

#[tokio::test]
async fn supervisor_expects_chassis_database() {
    let h = Harness::new();
    h.set_platform_file("platform_env.conf", "supervisor=1\n");
    h.add_container("swss", "program:orchagent\n", &[("orchagent", "RUNNING")]);
    let info = ServiceChecker::default().check(&config(&[]), &h.ctx()).await.unwrap();
    assert_eq!(
        message(&info, "database-chassis"),
        "Container 'database-chassis' is not running"
    );
}

#[tokio::test]
async fn feature_without_state_is_an_error() {
    let h = Harness::new();
    h.config_db.set("FEATURE", "swss", &[("auto_restart", "enabled")]);
    let err = ServiceChecker::default()
        .check(&config(&[]), &h.ctx())
        .await
        .unwrap_err();
    assert_eq!(err.to_string(), "KeyError('state')");
}

#[tokio::test]
async fn docker_api_failure_means_nothing_runs() {
    let h = Harness::new();
    h.add_container("swss", "program:orchagent\n", &[("orchagent", "RUNNING")]);
    *h.docker.list_fails.borrow_mut() = true;
    let info = ServiceChecker::default().check(&config(&[]), &h.ctx()).await.unwrap();
    assert_eq!(message(&info, "swss"), "Container 'swss' is not running");
    assert_eq!(message(&info, "system"), "no critical process found");
}

#[tokio::test]
async fn monit_summary() {
    let h = Harness::new();
    h.commands.set(
        "monit summary -B",
        "Monit 5.34.3 uptime: 23h 11m
 Service Name                     Status                      Type
 vlab-01                          OK                          System
 vlab-02                          Resource limit matched      System
 rsyslog                          OK                          Process
 root-overlay                     OK                          Filesystem
 var-log                          Does not exist              Filesystem
 routeCheck                       Status failed               Program
 diskCheck                        OK                          Program
",
    );
    let info = ServiceChecker::default()
        .check(&config(&["diskCheck"]), &h.ctx())
        .await
        .unwrap();
    assert_eq!(status(&info, "vlab-01"), STATUS_OK);
    assert_eq!(
        message(&info, "vlab-02"),
        "vlab-02 status is Resource limit matched, expected OK"
    );
    assert_eq!(status(&info, "rsyslog"), STATUS_OK);
    assert_eq!(status(&info, "root-overlay"), STATUS_OK);
    assert_eq!(
        message(&info, "var-log"),
        "var-log status is Does not exist, expected OK"
    );
    assert_eq!(info.get("var-log").unwrap().object_type, "Filesystem");
    assert_eq!(
        message(&info, "routeCheck"),
        "routeCheck status is Status failed, expected OK"
    );
    assert!(info.get("diskCheck").is_none());
}

#[tokio::test]
async fn monit_not_running_or_not_ready() {
    let h = Harness::new();
    h.commands.set(MONIT_ACTIVE, "inactive\n");
    let info = ServiceChecker::default().check(&config(&[]), &h.ctx()).await.unwrap();
    assert_eq!(message(&info, "monit"), "monit service is not running");

    let h = Harness::new();
    h.commands.outputs.borrow_mut().remove(MONIT_ACTIVE);
    let info = ServiceChecker::default().check(&config(&[]), &h.ctx()).await.unwrap();
    assert_eq!(message(&info, "monit"), "monit service is not running");

    let h = Harness::new();
    h.commands.set("monit summary -B", "Monit 5.34.3 uptime: 0m\n");
    let info = ServiceChecker::default().check(&config(&[]), &h.ctx()).await.unwrap();
    assert_eq!(message(&info, "monit"), "monit service is not ready");

    let h = Harness::new();
    h.commands.outputs.borrow_mut().remove("monit summary -B");
    let info = ServiceChecker::default().check(&config(&[]), &h.ctx()).await.unwrap();
    assert_eq!(message(&info, "monit"), "monit service is not ready");
    assert!(h
        .commands
        .calls
        .borrow()
        .iter()
        .all(|(_, timeout)| *timeout == system_health::CALL_TIMEOUT));

    let h = Harness::new();
    h.commands.set("monit summary -B", "Monit\nName State Kind\nx OK y\n");
    let info = ServiceChecker::default().check(&config(&[]), &h.ctx()).await.unwrap();
    assert_eq!(
        message(&info, "monit"),
        "output of \"monit summary -B\" is invalid or incompatible"
    );
}

#[tokio::test]
async fn critical_processes_are_read_through_the_merged_dir() {
    let h = Harness::new();
    h.add_container("swss", "program:orchagent\n", &[("orchagent", "RUNNING")]);
    let info = ServiceChecker::default().check(&config(&[]), &h.ctx()).await.unwrap();
    assert_eq!(status(&info, "swss:orchagent"), STATUS_OK);

    // No merged directory reported, or one that is gone: not cached, read again next cycle.
    let h = Harness::new();
    h.add_container("swss", "program:orchagent\n", &[("orchagent", "RUNNING")]);
    h.commands.set(&inspect_merged_dir("swss"), "\n");
    let mut checker = ServiceChecker::default();
    let info = checker.check(&config(&[]), &h.ctx()).await.unwrap();
    assert!(info.get("swss:orchagent").is_none());
    h.commands.set(&inspect_merged_dir("swss"), "/no/such/merged\n");
    let info = checker.check(&config(&[]), &h.ctx()).await.unwrap();
    assert!(info.get("swss:orchagent").is_none());
    h.commands.set(
        &inspect_merged_dir("swss"),
        &format!("{}\n", h.merged_dir("swss").display()),
    );
    let info = checker.check(&config(&[]), &h.ctx()).await.unwrap();
    assert_eq!(status(&info, "swss:orchagent"), STATUS_OK);
}

#[tokio::test]
async fn ignored_container_is_still_reported_as_not_running() {
    // As in Python: services_to_ignore applies to processes and monit services, not to expected containers.
    let h = Harness::new();
    h.add_container("swss", "program:orchagent\n", &[("orchagent", "RUNNING")]);
    h.config_db.set("FEATURE", "snmp", &[("state", "enabled")]);
    let info = ServiceChecker::default()
        .check(&config(&["snmp"]), &h.ctx())
        .await
        .unwrap();
    assert_eq!(message(&info, "snmp"), "Container 'snmp' is not running");
}

/// The (container, process) of every process-not-running event published since the last call.
fn take_process_events(h: &Harness) -> Vec<(String, String)> {
    h.events
        .events
        .borrow_mut()
        .drain(..)
        .map(|(tag, params)| {
            assert_eq!(tag, "process-not-running");
            let get = |key: &str| params.iter().find(|(k, _)| k == key).unwrap().1.clone();
            (get("ctr_name"), get("process_name"))
        })
        .collect()
}

fn process_event(container: &str, process: &str) -> (String, String) {
    (container.to_string(), process.to_string())
}

#[tokio::test]
async fn supervisorctl_without_supervisord_judges_nothing() {
    // As in Python: supervisord not up yet (container starting) prints an error line naming no process.
    let h = Harness::new();
    h.add_container("swss", "program:orchagent\n", &[]);
    h.set_supervisor_output("swss", "unix:///var/run/supervisor.sock no such file\n");
    let info = ServiceChecker::default().check(&config(&[]), &h.ctx()).await.unwrap();
    assert!(info.get("swss:orchagent").is_none());
    assert!(h.events.events.borrow().is_empty());
}
