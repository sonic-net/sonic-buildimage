mod common;

use std::cell::RefCell;
use std::fs;
use std::path::PathBuf;

use common::{unit, Harness};
use system_health::config::{Config, CONFIG_FILE};
use system_health::sysmonitor::{Console, SysmonContext, Sysmonitor, SystemState, UnitStatus};

#[derive(Default)]
struct RecordingConsole {
    messages: RefCell<Vec<String>>,
}

impl Console for RecordingConsole {
    fn write(&self, message: &str) {
        self.messages.borrow_mut().push(message.to_string());
    }
}

struct Sysmon {
    h: Harness,
    console: RecordingConsole,
    systemd_dir: PathBuf,
}

impl Sysmon {
    fn new() -> Self {
        let h = Harness::new();
        let systemd_dir = h.dir.path().join("systemd");
        for target in ["multi-user.target.wants", "sonic.target.wants"] {
            fs::create_dir_all(systemd_dir.join(target)).unwrap();
        }
        Self {
            h,
            console: RecordingConsole::default(),
            systemd_dir,
        }
    }

    fn ctx(&self) -> SysmonContext<'_> {
        SysmonContext {
            state_db: &self.h.state_db,
            config_db: &self.h.config_db,
            systemd: &self.h.systemd,
            device: &self.h.device,
            console: &self.console,
            systemd_dir: self.systemd_dir.clone(),
        }
    }

    fn want(&self, target: &str, service: &str) {
        fs::write(self.systemd_dir.join(target).join(service), "").unwrap();
    }

    fn sysmonitor(&self, config_json: Option<&str>) -> Sysmonitor {
        let path = self.h.platform_dir().join(CONFIG_FILE);
        if let Some(json) = config_json {
            fs::write(&path, json).unwrap();
        }
        Sysmonitor::new(Config::new(path, self.h.dir.path().join("monitrc")))
    }

    fn service_status(&self, name: &str) -> Option<(String, String, String, String)> {
        let entry = self.h.state_db.entry(&format!("ALL_SERVICE_STATUS|{name}"))?;
        Some((
            entry["service_status"].clone(),
            entry["app_ready_status"].clone(),
            entry["fail_reason"].clone(),
            entry["update_time"].clone(),
        ))
    }

    fn system_ready(&self) -> Option<String> {
        self.h
            .state_db
            .entry("SYSTEM_READY|SYSTEM_STATE")
            .map(|e| e["Status"].clone())
    }
}

fn s(a: &str, b: &str, c: &str, d: &str) -> Option<(String, String, String, String)> {
    Some((a.into(), b.into(), c.into(), d.into()))
}

#[tokio::test]
async fn service_list_from_targets_and_features() {
    let t = Sysmon::new();
    t.want("multi-user.target.wants", "rsyslog.service");
    t.want("multi-user.target.wants", "ztp.service");
    t.want("multi-user.target.wants", "some.timer");
    t.want("sonic.target.wants", "rsyslog.service");
    t.want("sonic.target.wants", "ignored.service");
    t.h.config_db.set("FEATURE", "radv", &[("state", "enabled")]);
    t.h.config_db.set("FEATURE", "bgp", &[("state", "always_enabled")]);
    t.h.config_db.set("FEATURE", "pmon", &[("state", "disabled")]);
    t.h.config_db.set(
        "FEATURE",
        "mux",
        &[("state", "{% if 'subtype' in DEVICE_METADATA['localhost'] and DEVICE_METADATA['localhost']['subtype'] == 'DualToR' %}enabled{% else %}always_disabled{% endif %}")],
    );
    t.h.config_db.set("DEVICE_METADATA", "localhost", &[("hwsku", "x")]);

    let mut sysmon = t.sysmonitor(Some(r#"{"services_to_ignore": ["ignored.service", "radv"]}"#));
    let list = sysmon.get_all_service_list(&t.ctx()).await;
    assert_eq!(list, ["bgp.service", "radv.service", "rsyslog.service"]);

    t.h.config_db
        .set("DEVICE_METADATA", "localhost", &[("subtype", "DualToR")]);
    let list = sysmon.get_all_service_list(&t.ctx()).await;
    assert!(list.contains(&"mux.service".to_string()));
}

#[tokio::test(start_paused = true)]
async fn feature_table_not_ready_is_retried() {
    let t = Sysmon::new();
    t.want("multi-user.target.wants", "rsyslog.service");
    t.h.config_db.set("FEATURE", "radv", &[("auto_restart", "enabled")]);
    let mut sysmon = t.sysmonitor(None);
    let list = sysmon.get_all_service_list(&t.ctx()).await;
    assert_eq!(list, ["rsyslog.service"]);
}

#[tokio::test]
async fn multi_asic_prunes_per_asic_only_services() {
    let t = Sysmon::new();
    t.h.set_platform_file("asic.conf", "NUM_ASIC=2\n");
    t.want("multi-user.target.wants", "swss.service");
    t.want("multi-user.target.wants", "swss@0.service");
    t.want("multi-user.target.wants", "lldp.service");
    t.h.config_db.set(
        "FEATURE",
        "swss",
        &[("state", "enabled"), ("has_global_scope", "False")],
    );
    t.h.config_db
        .set("FEATURE", "lldp", &[("state", "enabled"), ("has_global_scope", "True")]);
    let mut sysmon = t.sysmonitor(None);
    let list = sysmon.get_all_service_list(&t.ctx()).await;
    assert_eq!(list, ["lldp.service", "swss@0.service"]);
}

#[tokio::test]
async fn unit_status_cases() {
    let t = Sysmon::new();
    let systemd = &t.h.systemd;
    systemd.set(
        "exited.service",
        unit("loaded", "enabled", "oneshot", "active", "exited", "success"),
    );
    systemd.set(
        "running.service",
        unit("loaded", "enabled", "simple", "active", "running", "success"),
    );
    systemd.set(
        "generated.service",
        unit("loaded", "generated", "simple", "active", "running", "success"),
    );
    systemd.set(
        "generated-down.service",
        unit("loaded", "generated", "simple", "inactive", "dead", "success"),
    );
    systemd.set(
        "starting.service",
        unit("loaded", "enabled", "simple", "activating", "start", "success"),
    );
    systemd.set(
        "stopping.service",
        unit("loaded", "enabled", "simple", "deactivating", "stop", "success"),
    );
    systemd.set(
        "failed.service",
        unit("loaded", "enabled", "simple", "failed", "failed", "exit-code"),
    );
    systemd.set(
        "oneshot.service",
        unit("loaded", "static", "oneshot", "inactive", "dead", "success"),
    );
    systemd.set(
        "database-chassis.service",
        unit("loaded", "enabled", "simple", "inactive", "dead", "success"),
    );
    systemd.set(
        "skipped.service",
        unit("loaded", "enabled", "simple", "inactive", "dead", "exec-condition"),
    );
    let mut unmet = unit("loaded", "enabled", "simple", "inactive", "dead", "success");
    unmet.condition_result = "no".into();
    systemd.set("unmet.service", unmet.clone());
    unmet.condition_timestamp_monotonic = "0".into();
    systemd.set("gc.service", unmet);
    systemd.set("masked.service", unit("masked", "masked", "", "inactive", "dead", ""));
    systemd.set(
        "disabled.service",
        unit("loaded", "disabled", "simple", "inactive", "dead", "success"),
    );

    let mut sysmon = t.sysmonitor(None);
    let ctx = t.ctx();
    let cases = [
        ("exited", UnitStatus::Ok, s("OK", "OK", "-", "-")),
        ("running", UnitStatus::Ok, s("OK", "OK", "-", "-")),
        ("generated", UnitStatus::Ok, s("OK", "OK", "-", "-")),
        ("generated-down", UnitStatus::NotOk, s("Down", "Down", "Inactive", "-")),
        ("starting", UnitStatus::NotOk, s("Starting", "Starting", "-", "-")),
        ("stopping", UnitStatus::NotOk, s("Stopping", "Stopping", "-", "-")),
        ("failed", UnitStatus::NotOk, s("Down", "Down", "exit-code", "-")),
        ("oneshot", UnitStatus::Ok, s("OK", "OK", "-", "-")),
        ("database-chassis", UnitStatus::Ok, s("OK", "OK", "-", "-")),
        ("skipped", UnitStatus::Ok, s("OK", "OK", "exec-condition", "-")),
        ("unmet", UnitStatus::Ok, s("OK", "OK", "condition-unmet", "-")),
        ("gc", UnitStatus::NotOk, s("Down", "Down", "Inactive", "-")),
        ("masked", UnitStatus::Unmanaged, None),
        ("disabled", UnitStatus::Unmanaged, None),
        ("no-answer", UnitStatus::Unknown, None),
    ];
    for (name, expected, status) in cases {
        assert_eq!(
            sysmon.get_unit_status(&ctx, &format!("{name}.service")).await,
            expected,
            "{name}"
        );
        assert_eq!(t.service_status(name), status, "{name}");
    }
}

#[tokio::test]
async fn app_ready_status() {
    let t = Sysmon::new();
    for name in ["swss", "bgp", "snmp", "lldp"] {
        t.h.systemd.set(
            &format!("{name}.service"),
            unit("loaded", "enabled", "simple", "active", "running", "success"),
        );
    }
    t.h.config_db
        .set("FEATURE", "swss", &[("state", "enabled"), ("check_up_status", "True")]);
    t.h.config_db
        .set("FEATURE", "bgp", &[("state", "enabled"), ("check_up_status", "true")]);
    t.h.config_db
        .set("FEATURE", "snmp", &[("state", "enabled"), ("check_up_status", "true")]);
    t.h.config_db
        .set("FEATURE", "lldp", &[("state", "enabled"), ("check_up_status", "false")]);
    t.h.state_db.set(
        "FEATURE|swss",
        &[("up_status", "true"), ("update_time", "2026-10-16 10:00:00")],
    );
    t.h.state_db.set(
        "FEATURE|bgp",
        &[("up_status", "false"), ("fail_reason", "docker start")],
    );

    let mut sysmon = t.sysmonitor(None);
    let ctx = t.ctx();
    assert_eq!(sysmon.get_unit_status(&ctx, "swss.service").await, UnitStatus::Ok);
    assert_eq!(t.service_status("swss"), s("OK", "OK", "-", "2026-10-16 10:00:00"));
    assert_eq!(sysmon.get_unit_status(&ctx, "bgp.service").await, UnitStatus::NotOk);
    assert_eq!(t.service_status("bgp"), s("OK", "Starting", "-", "-"));
    assert_eq!(sysmon.get_unit_status(&ctx, "snmp.service").await, UnitStatus::NotOk);
    assert_eq!(t.service_status("snmp"), s("OK", "Down", "NA", "-"));
    assert_eq!(sysmon.get_unit_status(&ctx, "lldp.service").await, UnitStatus::Ok);
}

/// As in Python, where the read raised in get_unit_status: the unit's status is not known, nothing is posted for it,
/// and a service that was down stays down rather than counting as up.
#[tokio::test]
async fn a_feature_table_that_cannot_be_read_leaves_the_unit_unknown() {
    let t = Sysmon::new();
    t.h.systemd.set("swss.service", ok_unit());
    t.h.config_db
        .set("FEATURE", "swss", &[("state", "enabled"), ("check_up_status", "true")]);
    t.h.config_db.failing.borrow_mut().push("FEATURE".to_string());
    let mut sysmon = t.sysmonitor(None);
    let ctx = t.ctx();
    assert_eq!(sysmon.get_unit_status(&ctx, "swss.service").await, UnitStatus::Unknown);
    assert_eq!(t.service_status("swss"), None);
}

fn ok_unit() -> system_health::systemd::UnitProperties {
    unit("loaded", "enabled", "simple", "active", "running", "success")
}

fn failed_unit() -> system_health::systemd::UnitProperties {
    unit("loaded", "enabled", "simple", "failed", "failed", "exit-code")
}

#[tokio::test]
async fn system_ready_is_published_on_change_only() {
    let t = Sysmon::new();
    t.want("multi-user.target.wants", "a.service");
    t.want("multi-user.target.wants", "b.service");
    t.h.systemd.set("a.service", ok_unit());
    t.h.systemd.set("b.service", failed_unit());
    let mut sysmon = t.sysmonitor(None);
    let ctx = t.ctx();

    sysmon.update_system_status(&ctx).await;
    assert_eq!(sysmon.state(), SystemState::Down);
    assert_eq!(t.system_ready(), None);
    assert!(t.console.messages.borrow().is_empty());

    t.h.systemd.set("b.service", ok_unit());
    sysmon.check_unit_status(&ctx, "b.service").await;
    assert_eq!(t.system_ready().as_deref(), Some("UP"));
    assert_eq!(*t.console.messages.borrow(), ["System is ready"]);

    t.h.systemd.set("a.service", failed_unit());
    sysmon.check_unit_status(&ctx, "a.service").await;
    assert_eq!(t.system_ready().as_deref(), Some("DOWN"));
    assert_eq!(
        t.console.messages.borrow().last().map(String::as_str),
        Some("System is not ready - one or more services are not up")
    );
    sysmon.update_system_status(&ctx).await;
    assert_eq!(t.console.messages.borrow().len(), 2);

    sysmon.cleanup(&ctx);
    assert_eq!(t.system_ready(), None);
    assert_eq!(t.service_status("a"), None);
}

#[tokio::test]
async fn timed_out_unit_keeps_its_last_status() {
    let t = Sysmon::new();
    t.want("multi-user.target.wants", "a.service");
    t.want("multi-user.target.wants", "b.service");
    t.h.systemd.set("a.service", ok_unit());
    t.h.systemd.set("b.service", failed_unit());
    let mut sysmon = t.sysmonitor(None);
    let ctx = t.ctx();
    sysmon.update_system_status(&ctx).await;
    assert!(sysmon.down_services().contains("b.service"));

    t.h.systemd.units.borrow_mut().remove("b.service");
    sysmon.check_unit_status(&ctx, "b.service").await;
    sysmon.update_system_status(&ctx).await;
    assert!(sysmon.down_services().contains("b.service"));
    assert_eq!(t.service_status("b"), s("Down", "Down", "exit-code", "-"));

    t.h.systemd.units.borrow_mut().remove("a.service");
    sysmon.update_system_status(&ctx).await;
    assert!(!sysmon.down_services().contains("a.service"));
}

#[tokio::test]
async fn masked_or_removed_service_is_cleaned_up() {
    let t = Sysmon::new();
    t.want("multi-user.target.wants", "a.service");
    t.want("multi-user.target.wants", "b.service");
    t.h.systemd.set("a.service", ok_unit());
    t.h.systemd.set("b.service", failed_unit());
    let mut sysmon = t.sysmonitor(None);
    let ctx = t.ctx();
    sysmon.update_system_status(&ctx).await;

    t.h.systemd
        .set("b.service", unit("masked", "masked", "", "inactive", "dead", ""));
    sysmon.check_unit_status(&ctx, "b.service").await;
    assert_eq!(t.service_status("b"), None);
    assert_eq!(t.system_ready().as_deref(), Some("UP"));

    t.h.systemd.set("b.service", failed_unit());
    sysmon.update_system_status(&ctx).await;
    assert_eq!(t.system_ready().as_deref(), Some("DOWN"));
    fs::remove_file(t.systemd_dir.join("multi-user.target.wants/b.service")).unwrap();
    sysmon.check_unit_status(&ctx, "b.service").await;
    assert_eq!(t.service_status("b"), None);
    assert_eq!(t.system_ready().as_deref(), Some("UP"));
}

#[tokio::test]
async fn timer_event_keeps_service_entry() {
    let t = Sysmon::new();
    t.want("multi-user.target.wants", "a.service");
    t.h.systemd.set("a.service", ok_unit());
    let mut sysmon = t.sysmonitor(None);
    let ctx = t.ctx();
    sysmon.update_system_status(&ctx).await;
    sysmon.check_unit_status(&ctx, "a.timer").await;
    assert!(t.service_status("a").is_some());
}

#[tokio::test]
async fn unit_name_with_several_dots() {
    let t = Sysmon::new();
    t.want("multi-user.target.wants", "a.b.service");
    t.h.systemd.set("a.b.service", failed_unit());
    let mut sysmon = t.sysmonitor(None);
    let ctx = t.ctx();
    sysmon.check_unit_status(&ctx, "a.b.service").await;
    assert_eq!(t.service_status("a.b"), s("Down", "Down", "exit-code", "-"));
}
