//! External commands with a timeout that kills the whole process group, as Python's `utils.run_command`.

use std::process::Stdio;
use std::time::Duration;

use async_trait::async_trait;

use crate::log;

#[derive(Debug, PartialEq, Eq)]
pub enum CommandError {
    Timeout,
    Failed(String),
}

impl std::fmt::Display for CommandError {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        match self {
            CommandError::Timeout => write!(f, "timed out"),
            CommandError::Failed(e) => write!(f, "{e}"),
        }
    }
}

/// Runs a command and returns its stdout, whatever its exit code.
#[async_trait(?Send)]
pub trait CommandRunner {
    async fn run(&self, argv: &[&str], timeout: Duration) -> Result<String, CommandError>;

    /// Runs `cmd` through `/bin/sh -c`.
    async fn run_shell(&self, cmd: &str, timeout: Duration) -> Result<String, CommandError> {
        self.run(&["/bin/sh", "-c", cmd], timeout).await
    }
}

pub struct TokioCommandRunner;

#[async_trait(?Send)]
impl CommandRunner for TokioCommandRunner {
    async fn run(&self, argv: &[&str], timeout: Duration) -> Result<String, CommandError> {
        let (program, args) = argv
            .split_first()
            .ok_or_else(|| CommandError::Failed("empty command".into()))?;
        let mut command = tokio::process::Command::new(program);
        command
            .args(args)
            .stdin(Stdio::null())
            .stdout(Stdio::piped())
            .stderr(Stdio::null())
            .kill_on_drop(true);
        // A session of its own, as Python's start_new_session: no controlling terminal, and a process group of its
        // own for the timeout to kill.
        // SAFETY: the child only calls setsid, which is async-signal-safe, before exec.
        unsafe {
            command.pre_exec(|| nix::unistd::setsid().map(drop).map_err(std::io::Error::from));
        }
        let child = command.spawn().map_err(|e| CommandError::Failed(e.to_string()))?;
        let pid = child.id();
        match tokio::time::timeout(timeout, child.wait_with_output()).await {
            Ok(Ok(output)) => Ok(String::from_utf8_lossy(&output.stdout).into_owned()),
            Ok(Err(e)) => Err(CommandError::Failed(e.to_string())),
            Err(_) => {
                log::warning(
                    log::UTILS,
                    &format!(
                        "Failed to run command: Command '{}' timed out after {} seconds",
                        argv.join(" "),
                        timeout.as_secs()
                    ),
                );
                if let Some(pid) = pid {
                    // The group holds the shell's children too.
                    let _ = nix::sys::signal::killpg(
                        nix::unistd::Pid::from_raw(pid as i32),
                        nix::sys::signal::Signal::SIGKILL,
                    );
                    log::notice(log::UTILS, &format!("Initiate stuck process cleanup: pid={pid}"));
                }
                Err(CommandError::Timeout)
            }
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[tokio::test]
    async fn returns_stdout_whatever_the_exit_code() {
        let runner = TokioCommandRunner;
        let out = runner.run_shell("echo hello; exit 3", Duration::from_secs(5)).await;
        assert_eq!(out, Ok("hello\n".to_string()));
    }

    #[tokio::test]
    async fn argv_is_not_interpreted_by_a_shell() {
        let runner = TokioCommandRunner;
        let out = runner.run(&["echo", "$HOME;", "x"], Duration::from_secs(5)).await;
        assert_eq!(out, Ok("$HOME; x\n".to_string()));
    }

    #[tokio::test]
    async fn timeout_kills_the_process_group() {
        let dir = tempfile::tempdir().unwrap();
        let marker = dir.path().join("marker");
        let cmd = format!("(sleep 2; touch {}) & sleep 30", marker.display());
        let runner = TokioCommandRunner;
        let out = runner.run_shell(&cmd, Duration::from_millis(300)).await;
        assert_eq!(out, Err(CommandError::Timeout));
        tokio::time::sleep(Duration::from_secs(3)).await;
        assert!(!marker.exists(), "background child survived the timeout");
    }

    #[tokio::test]
    async fn a_command_leads_a_session_of_its_own() {
        let runner = TokioCommandRunner;
        let stat = runner
            .run_shell("cat /proc/$$/stat", Duration::from_secs(5))
            .await
            .unwrap();
        let pid = stat.split_whitespace().next().unwrap();
        // After "pid (comm)": the state, the parent, the process group and the session.
        let fields: Vec<&str> = stat.rsplit_once(')').unwrap().1.split_whitespace().collect();
        assert_eq!((fields[2], fields[3]), (pid, pid), "{stat}");
    }

    #[tokio::test]
    async fn missing_program_fails() {
        let runner = TokioCommandRunner;
        assert!(matches!(
            runner.run(&["/nonexistent/program"], Duration::from_secs(1)).await,
            Err(CommandError::Failed(_))
        ));
    }
}
