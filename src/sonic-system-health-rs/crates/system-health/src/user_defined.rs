//! Vendor checks configured in `user_defined_checkers`, run through the shell each cycle.
//!
//! The command prints a category line, then `object:message` lines; "OK" means OK, anything else is Not OK.

use std::time::Duration;

use crate::command::CommandRunner;
use crate::pyfmt::splitlines;
use crate::status::CheckerInfo;

pub const DEFAULT_CATEGORY: &str = "UserDefine";

pub struct UserDefinedChecker {
    cmd: String,
}

impl UserDefinedChecker {
    pub fn new(cmd: &str) -> Self {
        Self { cmd: cmd.to_string() }
    }

    pub fn name(&self) -> String {
        format!("UserDefinedChecker - {}", self.cmd)
    }

    /// Returns the category the command reported and its objects.
    pub async fn check(&self, commands: &dyn CommandRunner, timeout: Duration) -> (String, CheckerInfo) {
        let mut info = CheckerInfo::default();
        let output = commands.run_shell(&self.cmd, timeout).await.unwrap_or_default();
        let lines: Vec<&str> = splitlines(output.trim())
            .into_iter()
            .map(str::trim)
            .filter(|l| !l.is_empty())
            .collect();
        let Some((category, objects)) = lines.split_first() else {
            info.set_object_not_ok(
                DEFAULT_CATEGORY,
                &self.name(),
                format!("Failed to get output of command \"{}\"", self.cmd),
            );
            return (DEFAULT_CATEGORY.to_string(), info);
        };
        for line in objects {
            let Some((name, msg)) = line.split_once(':') else {
                continue;
            };
            let (name, msg) = (name.trim(), msg.trim());
            if msg != "OK" {
                info.set_object_not_ok(DEFAULT_CATEGORY, name, msg);
            } else {
                info.set_object_ok(DEFAULT_CATEGORY, name);
            }
        }
        (category.to_string(), info)
    }
}
