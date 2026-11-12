//! FEATURE templates, rendered as sonic_py_common renders them with jinja2.

use anyhow::{anyhow, Result};
use serde_json::{json, Map, Value};
use sonic_rs_common::DeviceInfo;

use crate::db::TableData;

/// The values a FEATURE state renders to.
pub const FEATURE_STATES: [&str; 4] = ["enabled", "disabled", "always_enabled", "always_disabled"];

pub fn render(template: &str, context: &Value) -> Result<String> {
    Ok(minijinja::Environment::new().render_str(template, context)?)
}

/// Renders `template` and checks the result is one of `expected`.
pub fn render_value(template: &str, context: &Value, expected: &[&str]) -> Result<String> {
    let value = render(template, context)?;
    if !expected.contains(&value.as_str()) {
        return Err(anyhow!("Invalid value rendered for configuration {template}: {value}"));
    }
    Ok(value)
}

pub fn table_to_json(table: &TableData) -> Value {
    let map: Map<String, Value> = table
        .iter()
        .map(|(key, fields)| {
            let fields: Map<String, Value> = fields
                .iter()
                .map(|(f, v)| (f.clone(), Value::String(v.clone())))
                .collect();
            (key.clone(), Value::Object(fields))
        })
        .collect();
    Value::Object(map)
}

/// What FEATURE templates are rendered with: DEVICE_METADATA and DEVICE_RUNTIME_METADATA.
pub fn feature_context(device: &DeviceInfo, device_metadata: &TableData) -> Value {
    let mut context = device_runtime_metadata(device, device_metadata);
    if let Value::Object(map) = &mut context {
        map.insert("DEVICE_METADATA".to_string(), table_to_json(device_metadata));
    }
    context
}

/// DEVICE_RUNTIME_METADATA, as sonic_py_common.device_info.get_device_runtime_metadata.
pub fn device_runtime_metadata(device: &DeviceInfo, device_metadata: &TableData) -> Value {
    let localhost = device_metadata.get("localhost");
    let field = |name: &str| localhost.and_then(|l| l.get(name)).map(String::as_str);
    let switch_type = field("switch_type");

    let is_voq_chassis = matches!(switch_type, Some("voq") | Some("fabric")) && !device.is_chassis_config_absent();
    let is_packet_chassis = switch_type == Some("chassis-packet");
    let is_virtual_chassis = device.asic_type().as_deref() == Some("vs")
        && matches!(switch_type, Some("dummy-sup") | Some("voq") | Some("chassis-packet"))
        && !device.is_chassis_config_absent();
    let is_chassis = field("type") == Some("SpineRouter")
        || (is_voq_chassis && !device.is_disaggregated_chassis())
        || is_packet_chassis
        || is_virtual_chassis;

    let mut runtime = Map::new();
    if is_chassis {
        runtime.insert(
            "CHASSIS_METADATA".to_string(),
            json!({
                "module_type": if device.is_supervisor() { "supervisor" } else { "linecard" },
                "chassis_type": if is_voq_chassis { "voq" } else { "packet" },
            }),
        );
    }
    let asic = if device.num_asics() > 1 { Some("0") } else { None };
    let ports_present = field("hwsku").is_some_and(|hwsku| device.port_config_file(hwsku, asic).is_some());
    runtime.insert("ETHERNET_PORTS_PRESENT".to_string(), Value::Bool(ports_present));
    runtime.insert(
        "MACSEC_SUPPORTED".to_string(),
        Value::Bool(device.is_macsec_supported()),
    );
    json!({ "DEVICE_RUNTIME_METADATA": Value::Object(runtime) })
}
