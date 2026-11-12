//! The daemon binary itself.

use std::fs;
use std::path::Path;
use std::process::{Command, Stdio};
use std::time::{Duration, Instant};

/// Without its configuration file healthd has nothing to do: it exits before it opens the vendor's platform, whose
/// package it does not even import.
#[test]
fn without_its_configuration_healthd_exits_before_opening_the_platform() {
    let dir = Path::new(env!("CARGO_TARGET_TMPDIR")).join("healthd-without-configuration");
    let _ = fs::remove_dir_all(&dir);
    // A vendor package that shows whether it was imported.
    let package = dir.join("sonic_platform");
    fs::create_dir_all(&package).unwrap();
    let imported = dir.join("imported");
    fs::write(
        package.join("__init__.py"),
        format!("open('{}', 'w').close()\n", imported.display()),
    )
    .unwrap();
    // The platform API's facade, as the binary would find it installed.
    let common = concat!(env!("CARGO_MANIFEST_DIR"), "/../../../sonic-platform-common");
    let mut child = Command::new(env!("CARGO_BIN_EXE_healthd"))
        .env("PLATFORM", "x86_64-no_such_platform-r0")
        .env("PYTHONPATH", format!("{}:{common}", dir.display()))
        .stdout(Stdio::null())
        .stderr(Stdio::piped())
        .spawn()
        .unwrap();
    let deadline = Instant::now() + Duration::from_secs(30);
    while child.try_wait().unwrap().is_none() {
        if Instant::now() > deadline {
            child.kill().unwrap();
            panic!("healthd still runs after 30 s");
        }
        std::thread::sleep(Duration::from_millis(50));
    }
    let output = child.wait_with_output().unwrap();
    let stderr = String::from_utf8_lossy(&output.stderr);

    assert!(!imported.exists(), "the vendor's package was imported: {stderr}");
    assert_eq!(output.status.code(), Some(0), "{stderr}");
}
