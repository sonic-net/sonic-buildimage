mod common;

use std::fs;

use common::Harness;
use serde_json::{json, Value};
use system_health::db::TableData;
use system_health::template::{device_runtime_metadata, render, render_value};

/// FEATURE templates of init_cfg.json.j2 rendered by jinja2 in five device contexts.
#[test]
fn feature_templates_render_as_jinja2() {
    let golden: Value = serde_json::from_str(include_str!("feature_templates.json")).unwrap();
    let cases = golden["cases"].as_array().unwrap();
    assert!(cases.len() >= 60);
    for case in cases {
        let template = case["template"].as_str().unwrap();
        let context = &golden["contexts"][case["context"].as_str().unwrap()];
        let expected = case["expected"].as_str().unwrap();
        assert_eq!(
            render(template, context).unwrap(),
            expected,
            "{template} in {}",
            case["context"]
        );
    }
}

#[test]
fn unexpected_value_is_an_error() {
    let context = json!({});
    assert_eq!(render_value("enabled", &context, &["enabled"]).unwrap(), "enabled");
    assert!(render_value("{{ 'other' }}", &context, &["enabled"]).is_err());
}

fn metadata(fields: &[(&str, &str)]) -> TableData {
    let mut table = TableData::new();
    table.insert(
        "localhost".to_string(),
        fields.iter().map(|(k, v)| (k.to_string(), v.to_string())).collect(),
    );
    table
}

#[test]
fn runtime_metadata_of_a_switch() {
    let h = Harness::new();
    let hwsku_dir = h.platform_dir().join("SKU1");
    fs::create_dir_all(&hwsku_dir).unwrap();
    fs::write(hwsku_dir.join("port_config.ini"), "# name lanes\n").unwrap();
    let runtime = device_runtime_metadata(&h.device, &metadata(&[("hwsku", "SKU1"), ("type", "ToRRouter")]));
    assert_eq!(
        runtime,
        json!({"DEVICE_RUNTIME_METADATA": {"ETHERNET_PORTS_PRESENT": true, "MACSEC_SUPPORTED": false}})
    );

    let runtime = device_runtime_metadata(&h.device, &metadata(&[("hwsku", "OTHER")]));
    assert_eq!(
        runtime["DEVICE_RUNTIME_METADATA"]["ETHERNET_PORTS_PRESENT"],
        json!(false)
    );
}

#[test]
fn runtime_metadata_of_a_chassis() {
    let h = Harness::new();
    h.set_platform_file("platform_env.conf", "supervisor=1\nmacsec_enabled=1\n");
    h.set_platform_file("chassisdb.conf", "start_chassis_db=1\n");
    let runtime = device_runtime_metadata(&h.device, &metadata(&[("switch_type", "voq")]));
    assert_eq!(
        runtime["DEVICE_RUNTIME_METADATA"]["CHASSIS_METADATA"],
        json!({"module_type": "supervisor", "chassis_type": "voq"})
    );
    assert_eq!(runtime["DEVICE_RUNTIME_METADATA"]["MACSEC_SUPPORTED"], json!(true));

    let runtime = device_runtime_metadata(&h.device, &metadata(&[("switch_type", "chassis-packet")]));
    assert_eq!(
        runtime["DEVICE_RUNTIME_METADATA"]["CHASSIS_METADATA"]["chassis_type"],
        json!("packet")
    );

    let h = Harness::new();
    let runtime = device_runtime_metadata(&h.device, &metadata(&[("switch_type", "voq")]));
    assert!(runtime["DEVICE_RUNTIME_METADATA"].get("CHASSIS_METADATA").is_none());
}

/// A vs image is a virtual chassis only with a chassis database: a single-ASIC VOQ vlab has none.
#[test]
fn runtime_metadata_of_a_virtual_chassis() {
    let h = Harness::new();
    let etc = h.dir.path().join("root/etc/sonic");
    std::fs::create_dir_all(&etc).unwrap();
    std::fs::write(etc.join("sonic_version.yml"), "asic_type: vs\n").unwrap();
    let chassis = |h: &Harness| {
        device_runtime_metadata(&h.device, &metadata(&[("switch_type", "dummy-sup")]))["DEVICE_RUNTIME_METADATA"]
            .get("CHASSIS_METADATA")
            .cloned()
    };
    assert_eq!(chassis(&h), None);
    h.set_platform_file("chassisdb.conf", "start_chassis_db=1\n");
    assert_eq!(
        chassis(&h),
        Some(json!({"module_type": "linecard", "chassis_type": "packet"}))
    );
}
