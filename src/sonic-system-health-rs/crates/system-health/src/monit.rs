//! `monit summary -B` parsing.

use crate::pyfmt::{char_find, char_slice, splitlines};

pub const MIN_CHECK_CMD_LINES: usize = 3;

#[derive(Debug, PartialEq, Eq)]
pub struct MonitService {
    pub name: String,
    pub status: String,
    pub service_type: String,
}

#[derive(Debug, PartialEq, Eq)]
pub enum MonitError {
    NotReady,
    Invalid,
}

/// Splits each service line at the columns of the "Status" and "Type" headers, as the Python checker.
pub fn parse_summary(output: &str) -> Result<Vec<MonitService>, MonitError> {
    let lines = splitlines(output);
    if lines.len() < MIN_CHECK_CMD_LINES {
        return Err(MonitError::NotReady);
    }
    let (Some(status_begin), Some(type_begin)) = (char_find(lines[1], "Status"), char_find(lines[1], "Type")) else {
        return Err(MonitError::Invalid);
    };
    Ok(lines[2..]
        .iter()
        .map(|line| MonitService {
            name: char_slice(line, 0, Some(status_begin)).trim().to_string(),
            status: char_slice(line, status_begin, Some(type_begin)).trim().to_string(),
            service_type: char_slice(line, type_begin, None).trim().to_string(),
        })
        .collect())
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn parses_columns() {
        let output = "Monit 5.34.3 uptime: 23h 11m
 Service Name                     Status                      Type
 vlab-01                          OK                          System
 vlab-02                          Resource limit matched      System
 var-log                          Does not exist              Filesystem
 routeCheck                       Status failed               Program
";
        let services = parse_summary(output).unwrap();
        assert_eq!(services.len(), 4);
        assert_eq!(
            services[1],
            MonitService {
                name: "vlab-02".into(),
                status: "Resource limit matched".into(),
                service_type: "System".into()
            }
        );
        assert_eq!(services[3].status, "Status failed");
        assert_eq!(services[3].service_type, "Program");
    }

    #[test]
    fn short_or_invalid_output() {
        assert_eq!(parse_summary("Monit\n"), Err(MonitError::NotReady));
        assert_eq!(parse_summary(""), Err(MonitError::NotReady));
        assert_eq!(parse_summary("a\nb\nc\n"), Err(MonitError::Invalid));
    }
}
