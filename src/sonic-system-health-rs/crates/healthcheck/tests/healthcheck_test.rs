//! The CLI binary itself.

use std::os::unix::fs::PermissionsExt;
use std::os::unix::process::CommandExt;
use std::process::{Command, Stdio};
use std::time::{Duration, Instant};

/// The user and group id of nobody on Debian.
const NOBODY: u32 = 65534;

/// As another user the vendor's LED writes fail and log errors, so the binary stops before it reads anything, as
/// `show system-health` does.
#[test]
fn cli_refuses_to_run_without_root() {
    // A copy anyone may run, whatever the permissions of the target directory.
    let dir = tempfile::tempdir().unwrap();
    std::fs::set_permissions(dir.path(), std::fs::Permissions::from_mode(0o755)).unwrap();
    let exe = dir.path().join(env!("CARGO_PKG_NAME"));
    std::fs::copy(env!("CARGO_BIN_EXE_healthcheck"), &exe).unwrap();
    let mut command = Command::new(&exe);
    command.arg("--json").stdout(Stdio::piped()).stderr(Stdio::piped());
    let as_nobody = nix::unistd::geteuid().is_root();
    if as_nobody {
        command.uid(NOBODY).gid(NOBODY);
    }
    let mut child = match command.spawn() {
        Ok(child) => child,
        // Root that may not change its user, as in a container without CAP_SETUID: no other user to run as.
        Err(e) if as_nobody && e.raw_os_error() == Some(nix::errno::Errno::EPERM as i32) => {
            eprintln!("skipped: cannot run the binary as nobody here: {e}");
            return;
        }
        Err(e) => panic!("{e}"),
    };
    let deadline = Instant::now() + Duration::from_secs(10);
    while child.try_wait().unwrap().is_none() {
        if Instant::now() > deadline {
            child.kill().unwrap();
            panic!("the binary still runs after 10 s");
        }
        std::thread::sleep(Duration::from_millis(50));
    }
    let output = child.wait_with_output().unwrap();

    assert_eq!(
        String::from_utf8_lossy(&output.stderr),
        "Root privileges are required for this operation\n"
    );
    assert!(output.stdout.is_empty());
    assert_eq!(output.status.code(), Some(2));
}
