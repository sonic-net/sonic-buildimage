use std::{fs, process::Command};

use tempfile::TempDir;

fn cfggen(arguments: &[&str]) -> std::process::Output {
    Command::new(env!("CARGO_BIN_EXE_sonic-cfggen-lite-rs"))
        .args(arguments)
        .output()
        .expect("running sonic-cfggen-lite-rs")
}

#[test]
fn help_and_version_are_standalone_successes() {
    let help = cfggen(&["--help"]);
    assert!(help.status.success());
    assert!(String::from_utf8(help.stdout).unwrap().contains("Usage:"));
    assert!(help.stderr.is_empty());

    let version = cfggen(&["--version"]);
    assert!(version.status.success());
    assert_eq!(
        String::from_utf8(version.stdout).unwrap(),
        "sonic-cfggen-lite-rs 1.0.0\n"
    );
    assert!(version.stderr.is_empty());
}

#[test]
fn process_boundary_classifies_usage_and_redacts_inline_payloads() {
    let usage = cfggen(&["--unsupported", "--print-data"]);
    assert_eq!(usage.status.code(), Some(2));
    assert!(usage.stdout.is_empty());
    assert!(String::from_utf8(usage.stderr)
        .unwrap()
        .contains("unexpected argument '--unsupported'"));

    let secret = "do-not-disclose-this-secret";
    let invalid = format!(r#"{{"password":"{secret}",}}"#);
    let operational = cfggen(&["--additional-data", &invalid, "--print-data"]);
    assert_eq!(operational.status.code(), Some(1));
    assert!(operational.stdout.is_empty());
    let stderr = String::from_utf8(operational.stderr).unwrap();
    assert!(stderr.starts_with("sonic-cfggen-lite-rs: --additional-data: invalid JSON:"));
    assert!(!stderr.contains(secret));
}

#[test]
fn structured_actions_write_complete_buffers() {
    let output = cfggen(&[
        "--additional-data",
        r#"{"A":{"row|text":{"value":1}}}"#,
        "--print-data",
    ]);
    assert!(output.status.success());
    assert_eq!(
        String::from_utf8(output.stdout).unwrap(),
        "{\n    \"A\": {\n        \"row|text\": {\n            \"value\": 1\n        }\n    }\n}\n"
    );
    assert!(output.stderr.is_empty());

    let relationship = cfggen(&["--namespace", "asic0", "--print-data"]);
    assert_eq!(relationship.status.code(), Some(2));
    assert!(relationship.stdout.is_empty());
    assert_eq!(
        String::from_utf8(relationship.stderr).unwrap(),
        "sonic-cfggen-lite-rs: -n/--namespace requires --from-db or --write-to-db\n"
    );
}

#[test]
fn selector_process_outcomes_cover_typed_keys_and_arbitrary_indices() {
    let temp = TempDir::new().unwrap();
    let file = temp.path().join("tuple.json");
    fs::write(&file, r#"{"T":{"a|b":{"number":1.0,"value":"tuple"}}}"#).unwrap();
    let file = file.to_str().unwrap();

    let tuple = cfggen(&["-j", file, "-v", "T[('a','b')].value"]);
    assert!(tuple.status.success());
    assert_eq!(tuple.stdout, b"tuple\n");
    assert!(tuple.stderr.is_empty());

    let joined_is_distinct = cfggen(&["-j", file, "-v", "T['a|b']"]);
    assert!(joined_is_distinct.status.success());
    assert_eq!(joined_is_distinct.stdout, b"\n");

    let inline = cfggen(&[
        "-a",
        r#"{"T":{"a|b":{"value":"text"}}}"#,
        "-v",
        "T['a|b'].value",
    ]);
    assert!(inline.status.success());
    assert_eq!(inline.stdout, b"text\n");

    let huge = "184467440737095516160000000000000000000000000000";
    let final_missing = cfggen(&["-a", r#"{"A":[0]}"#, "-v", &format!("A[{huge}")]);
    assert_eq!(final_missing.status.code(), Some(2));
    assert!(final_missing.stdout.is_empty());

    let final_missing = cfggen(&["-a", r#"{"A":[0]}"#, "-v", &format!("A[{huge}]")]);
    assert!(final_missing.status.success());
    assert_eq!(final_missing.stdout, b"\n");
    assert!(final_missing.stderr.is_empty());

    let intermediate = cfggen(&["-a", r#"{"A":[0]}"#, "-v", &format!("A[{huge}].field")]);
    assert_eq!(intermediate.status.code(), Some(1));
    assert!(intermediate.stdout.is_empty());
    assert!(String::from_utf8(intermediate.stderr)
        .unwrap()
        .contains("missing at component 2"));

    let rejected = cfggen(&["-a", "{}", "-v", "A[0:1]"]);
    assert_eq!(rejected.status.code(), Some(2));
    assert!(rejected.stdout.is_empty());
}

#[test]
fn var_json_uses_pre_sort_insertion_order_and_composite_wins_collisions() {
    let temp = TempDir::new().unwrap();
    let file = temp.path().join("ordered.json");
    fs::write(
        &file,
        r#"{"TABLE":{"z|needle":{"chosen":"first"},"a|needle":{"chosen":"third"}}}"#,
    )
    .unwrap();
    let file = file.to_str().unwrap();

    let selected = cfggen(&[
        "-j",
        file,
        "-a",
        r#"{"TABLE":{"needle":{"chosen":"second"},"z|needle":{"chosen":"text"}}}"#,
        "--var-json",
        "TABLE",
        "-K",
        "needle",
    ]);
    assert!(selected.status.success());
    assert_eq!(
        String::from_utf8(selected.stdout).unwrap(),
        "{\n    \"z|needle\": {\n        \"chosen\": \"first\"\n    }\n}\n"
    );

    let collision = cfggen(&[
        "-j",
        file,
        "-a",
        r#"{"TABLE":{"z|needle":{"chosen":"text"}}}"#,
        "--var-json",
        "TABLE",
    ]);
    assert!(collision.status.success());
    let parsed: serde_json::Value = serde_json::from_slice(&collision.stdout).unwrap();
    assert_eq!(parsed["z|needle"]["chosen"], "first");

    let absent = cfggen(&["-a", "{}", "--var-json", "ABSENT"]);
    assert!(absent.status.success());
    assert!(absent.stdout.is_empty());

    let shape = cfggen(&["-a", r#"{"TABLE":null}"#, "--var-json", "TABLE", "-K", "x"]);
    assert_eq!(shape.status.code(), Some(1));
    assert!(shape.stdout.is_empty());
}
