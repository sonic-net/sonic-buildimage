use std::{
    ffi::OsStr,
    fs,
    path::{Path, PathBuf},
    process::{Command, Output},
};

use tempfile::TempDir;

fn python_cfggen() -> PathBuf {
    Path::new(env!("CARGO_MANIFEST_DIR")).join("../sonic-config-engine/sonic-cfggen")
}

fn python_path() -> String {
    let source = Path::new(env!("CARGO_MANIFEST_DIR")).join("..");
    [
        source.join("sonic-py-common"),
        source.join("sonic-config-engine"),
        source.join("sonic-py-swsssdk/src"),
    ]
    .into_iter()
    .map(|path| path.to_string_lossy().into_owned())
    .collect::<Vec<_>>()
    .join(":")
}

fn python_cfggen_available() -> bool {
    Command::new("/usr/bin/python3")
        .arg(python_cfggen())
        .arg("--help")
        .env("PYTHONPATH", python_path())
        .output()
        .is_ok_and(|output| output.status.success())
}

fn invoke<I, S>(rust: bool, arguments: I, namespace: Option<&str>) -> Output
where
    I: IntoIterator<Item = S>,
    S: AsRef<OsStr>,
{
    let mut command = if rust {
        Command::new(env!("CARGO_BIN_EXE_sonic-cfggen-lite-rs"))
    } else {
        let mut command = Command::new("/usr/bin/python3");
        command
            .arg(python_cfggen())
            .env("PYTHONPATH", python_path());
        command
    };
    command.args(arguments);
    match namespace {
        Some(namespace) => {
            command.env("NAMESPACE_ID", namespace);
        }
        None => {
            command.env_remove("NAMESPACE_ID");
        }
    }
    command.output().expect("running differential CLI")
}

fn assert_exact(arguments: &[&str]) {
    let rust = invoke(true, arguments, None);
    let python = invoke(false, arguments, None);
    assert_eq!(rust.status.code(), python.status.code(), "{arguments:?}");
    assert_eq!(rust.stdout, python.stdout, "{arguments:?}");
    if rust.status.success() {
        assert_eq!(rust.stderr, python.stderr, "{arguments:?}");
    }
}

#[test]
fn supported_file_and_selector_shapes_match_python() {
    if !python_cfggen_available() {
        eprintln!("legacy sonic-cfggen dependencies are unavailable; skipping differential");
        return;
    }

    let temp = TempDir::new().unwrap();
    let json = temp.path().join("base.json");
    let later = temp.path().join("later.json");
    let yaml = temp.path().join("base.yml");
    fs::write(
        &json,
        r#"{"Z":{"row|two":{"value":"tuple"}},"A":{"array":[1],"text":"line\n😀"}}"#,
    )
    .unwrap();
    fs::write(
        &later,
        r#"{"Z":{"row|two":{"value":"later"}},"A":{"extra":true}}"#,
    )
    .unwrap();
    fs::write(&yaml, "A:\n  scalar: yaml\n").unwrap();
    let json = json.to_str().unwrap();
    let later = later.to_str().unwrap();
    let yaml = yaml.to_str().unwrap();

    for arguments in [
        vec!["-y", yaml, "-v", "A.scalar"],
        vec!["-a", r#"{"A":{"value":"inline"}}"#, "-v", "A.value"],
        vec!["-j", json, "-v", "Z[('row','two')].value"],
        vec!["-j", json, "-j", later, "-v", "Z[('row','two')].value"],
        vec!["-a", r#"{"A":1}"#, "-v", "A.field"],
        vec!["-a", r#"{"A":1}"#, "-v", "A.field.more"],
        vec![
            "-a",
            r#"{"A":[1]}"#,
            "-v",
            "A[184467440737095516160000000000]",
        ],
        vec!["-a", r#"{"A":{"value":"x\u0000y"}}"#, "-v", "A.value"],
    ] {
        assert_exact(&arguments);
    }

    let rust = invoke(true, ["-j", json, "--print-data"], None);
    let python = invoke(false, ["-j", json, "--print-data"], None);
    assert!(rust.status.success() && python.status.success());
    let rust: serde_json::Value = serde_json::from_slice(&rust.stdout).unwrap();
    let python: serde_json::Value = serde_json::from_slice(&python.stdout).unwrap();
    assert_eq!(rust, python);
}

#[test]
fn namespace_seed_matches_python_direct_environment() {
    if !python_cfggen_available() {
        eprintln!("legacy sonic-cfggen dependencies are unavailable; skipping differential");
        return;
    }

    let arguments = [
        "-a",
        r#"{"DEVICE_METADATA":{"localhost":{"extra":"x"}}}"#,
        "-v",
        "DEVICE_METADATA.localhost.namespace_id",
    ];
    let rust = invoke(true, arguments, Some("asic9"));
    let python = invoke(false, arguments, Some("asic9"));
    assert_eq!(rust.status.code(), python.status.code());
    assert_eq!(rust.stdout, python.stdout);
    assert_eq!(rust.stderr, python.stderr);
}

#[test]
fn unsupported_template_operation_remains_available_in_python() {
    if !python_cfggen_available() {
        eprintln!("legacy sonic-cfggen dependencies are unavailable; skipping differential");
        return;
    }

    let temp = TempDir::new().unwrap();
    let template = temp.path().join("template.j2");
    fs::write(&template, "{{ A }}\n").unwrap();
    let output = invoke(
        false,
        [
            "-a",
            r#"{"A":"python-template"}"#,
            "-t",
            template.to_str().unwrap(),
        ],
        None,
    );
    assert!(output.status.success());
    assert_eq!(output.stdout, b"python-template\n");
    assert!(output.stderr.is_empty());
}
