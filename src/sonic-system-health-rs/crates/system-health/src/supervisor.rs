//! Process states from `docker exec <container> bash -c "supervisorctl status"`, as the Python daemon read them.

use indexmap::IndexMap;

use crate::command::CommandRunner;

/// Process name ("name", or "group:name") to state name ("RUNNING", ...).
pub type ProcessStates = IndexMap<String, String>;

/// The process states supervisorctl reports in a container; None when the command times out.
/// A container whose supervisord is not up yet prints an error line, which names no critical process.
pub async fn process_states(commands: &dyn CommandRunner, container: &str) -> Option<ProcessStates> {
    let output = commands
        .run(
            &["docker", "exec", container, "bash", "-c", "supervisorctl status"],
            crate::CALL_TIMEOUT,
        )
        .await
        .ok()?;
    Some(parse_status(&output))
}

/// The first two words of each line, as Python's `_parse_supervisorctl_status`.
pub fn parse_status(output: &str) -> ProcessStates {
    output
        .lines()
        .filter_map(|line| {
            let mut items = line.split_whitespace();
            Some((items.next()?.to_string(), items.next()?.to_string()))
        })
        .collect()
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn parses_supervisorctl_status() {
        let output = "arp_update                       RUNNING   pid 67, uptime 1:03:56\n\
                      dhcp-relay:dhcp6relay            FATAL     Exited too quickly\n\
                      \n\
                      lonely\n\
                      buffermgrd                       EXITED    Oct 20 09:00 PM\n";
        let states = parse_status(output);
        assert_eq!(
            states.into_iter().collect::<Vec<_>>(),
            [
                ("arp_update".to_string(), "RUNNING".to_string()),
                ("dhcp-relay:dhcp6relay".to_string(), "FATAL".to_string()),
                ("buffermgrd".to_string(), "EXITED".to_string()),
            ]
        );
    }
}
