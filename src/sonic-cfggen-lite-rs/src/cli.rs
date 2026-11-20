use std::path::PathBuf;

#[cfg(test)]
use std::ffi::OsString;

use clap::{ArgAction, Parser};

#[cfg(test)]
use clap::error::ErrorKind;

use crate::error::{AppError, Result};

#[derive(Clone, Debug, Parser, PartialEq, Eq)]
#[command(
    name = "sonic-cfggen-lite-rs",
    version,
    about = "Rust SONiC configuration data reader and writer",
    disable_help_flag = true,
    disable_version_flag = true
)]
pub struct Cli {
    #[arg(short = 'd', long = "from-db")]
    pub from_db: bool,

    #[arg(short = 'j', long = "json", value_name = "FILE")]
    pub json: Vec<PathBuf>,

    #[arg(short = 'y', long = "yaml", value_name = "FILE")]
    pub yaml: Vec<PathBuf>,

    #[arg(short = 'a', long = "additional-data", value_name = "JSON")]
    pub additional_data: Option<String>,

    #[arg(short = 'n', long = "namespace", value_name = "NAME")]
    pub namespace: Option<String>,

    #[arg(short = 'v', long = "var", value_name = "PATH")]
    pub var: Option<String>,

    #[arg(long = "var-json", value_name = "TABLE")]
    pub var_json: Option<String>,

    #[arg(long = "print-data")]
    pub print_data: bool,

    #[arg(short = 'w', long = "write-to-db")]
    pub write_to_db: bool,

    #[arg(short = 'K', long = "key", value_name = "KEY")]
    pub key: Option<String>,

    #[arg(short = 'h', long = "help", action = ArgAction::Help, exclusive = true)]
    help: Option<bool>,

    #[arg(long = "version", action = ArgAction::Version, exclusive = true)]
    version: Option<bool>,
}

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum Action<'a> {
    Var(&'a str),
    VarJson(&'a str),
    PrintData,
    WriteToDb,
}

impl Cli {
    pub fn parse_process() -> Result<Self> {
        let cli = Self::try_parse().unwrap_or_else(|error| error.exit());
        cli.validate()?;
        Ok(cli)
    }

    #[cfg(test)]
    pub fn parse_checked_from<I, T>(arguments: I) -> Result<Self>
    where
        I: IntoIterator<Item = T>,
        T: Into<OsString> + Clone,
    {
        let cli = Self::try_parse_from(arguments).map_err(clap_error)?;
        cli.validate()?;
        Ok(cli)
    }

    pub fn action(&self) -> Action<'_> {
        if let Some(path) = self.var.as_deref() {
            Action::Var(path)
        } else if let Some(table) = self.var_json.as_deref() {
            Action::VarJson(table)
        } else if self.print_data {
            Action::PrintData
        } else {
            Action::WriteToDb
        }
    }

    fn validate(&self) -> Result<()> {
        let action_count = usize::from(self.var.is_some())
            + usize::from(self.var_json.is_some())
            + usize::from(self.print_data)
            + usize::from(self.write_to_db);
        if action_count != 1 {
            return Err(AppError::usage(
                "exactly one of -v, --var-json, --print-data, and --write-to-db is required",
            ));
        }
        if self.key.is_some() && self.var_json.is_none() {
            return Err(AppError::usage("-K/--key requires --var-json"));
        }
        if self.namespace.is_some() && !self.from_db && !self.write_to_db {
            return Err(AppError::usage(
                "-n/--namespace requires --from-db or --write-to-db",
            ));
        }
        if self.write_to_db {
            if self.json.is_empty() && self.additional_data.is_none() {
                return Err(AppError::usage(
                    "--write-to-db requires at least one --json or --additional-data source",
                ));
            }
            if self.from_db || !self.yaml.is_empty() {
                return Err(AppError::usage(
                    "--write-to-db does not support --from-db or --yaml",
                ));
            }
        }
        Ok(())
    }
}

#[cfg(test)]
fn clap_error(error: clap::Error) -> AppError {
    debug_assert!(!matches!(
        error.kind(),
        ErrorKind::DisplayHelp | ErrorKind::DisplayVersion
    ));
    AppError::usage(error.to_string().trim_end().to_owned())
}

#[cfg(test)]
mod tests {
    use super::*;

    fn parse(arguments: &[&str]) -> Result<Cli> {
        Cli::parse_checked_from(
            std::iter::once("sonic-cfggen-lite-rs").chain(arguments.iter().copied()),
        )
    }

    #[test]
    fn accepts_supported_actions_and_spellings() {
        assert!(parse(&["-d", "-v", "A.B"]).is_ok());
        assert!(parse(&["--json", "a.json", "--print-data"]).is_ok());
        assert!(parse(&["-y", "a.yml", "--var-json", "T", "-K", "R"]).is_ok());
        assert!(parse(&["-j", "a.json", "-a", "{}", "-w", "-n", "asic0"]).is_ok());
    }

    #[test]
    fn rejects_action_and_relationship_errors() {
        for arguments in [
            vec![],
            vec!["-v", "A", "--print-data"],
            vec!["-K", "R", "--print-data"],
            vec!["-n", "asic0", "-v", "A"],
            vec!["-w"],
            vec!["-j", "a", "-y", "b", "-w"],
            vec!["-j", "a", "-d", "-w"],
            vec!["--unsupported", "-v", "A"],
        ] {
            assert_eq!(
                parse(&arguments).unwrap_err().exit_code(),
                2,
                "{arguments:?}"
            );
        }
    }

    #[test]
    fn preserves_repeated_file_order() {
        let cli = parse(&[
            "-j",
            "one",
            "--json",
            "two",
            "-y",
            "three",
            "--yaml",
            "four",
            "--print-data",
        ])
        .unwrap();
        assert_eq!(cli.json, [PathBuf::from("one"), PathBuf::from("two")]);
        assert_eq!(cli.yaml, [PathBuf::from("three"), PathBuf::from("four")]);
    }
}
