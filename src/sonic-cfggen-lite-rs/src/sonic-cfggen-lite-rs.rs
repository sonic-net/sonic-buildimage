mod cli;
mod db;
mod error;
mod input;
mod output;
mod selector;
mod value;
mod write;
mod yaml;

use std::io::Write as _;
use std::process::ExitCode;

use cli::{Action, Cli};
use error::{AppError, Result};
use selector::{Evaluation, SelectorPath};
use value::DataValue;

enum PreparedAction<'a> {
    Var {
        original: &'a str,
        path: SelectorPath,
    },
    VarJson(&'a str),
    PrintData,
    WriteToDb,
}

impl<'a> PreparedAction<'a> {
    fn prepare(action: Action<'a>) -> Result<Self> {
        match action {
            Action::Var(original) => Ok(Self::Var {
                original,
                path: SelectorPath::parse(original)?,
            }),
            Action::VarJson(table) => Ok(Self::VarJson(table)),
            Action::PrintData => Ok(Self::PrintData),
            Action::WriteToDb => Ok(Self::WriteToDb),
        }
    }
}

fn run() -> Result<Vec<u8>> {
    let cli = Cli::parse_process()?;
    let action = PreparedAction::prepare(cli.action())?;
    let mut loaded = input::load(&cli)?;
    if cli.from_db {
        let request = match &action {
            PreparedAction::Var { path, .. } => db::DbRequest::Var(path),
            PreparedAction::VarJson(table) => db::DbRequest::VarJson(table),
            PreparedAction::PrintData => db::DbRequest::PrintData,
            PreparedAction::WriteToDb => {
                return Err(AppError::usage("--write-to-db does not support --from-db"));
            }
        };
        db::augment(&cli, request, &mut loaded)?;
    }

    match action {
        PreparedAction::Var { original, path } => {
            let root = DataValue::Object(loaded.root);
            match path.evaluate(&root) {
                Evaluation::Value(value) => output::render_legacy(value),
                Evaluation::MissingFinal => Ok(b"\n".to_vec()),
                Evaluation::MissingIntermediate(component) => Err(AppError::operational(format!(
                    "selector path is missing at component {} of {original}",
                    component + 1
                ))),
            }
        }
        PreparedAction::VarJson(table) => {
            output::render_var_json(&loaded.root, table, cli.key.as_deref())
        }
        PreparedAction::PrintData => output::render_structured(&DataValue::Object(loaded.root)),
        PreparedAction::WriteToDb => {
            write::write(&cli, &loaded.root)?;
            Ok(Vec::new())
        }
    }
}

fn main() -> ExitCode {
    match run() {
        Ok(output) => {
            if let Err(error) = std::io::stdout().write_all(&output) {
                eprintln!("sonic-cfggen-lite-rs: unable to write stdout: {error}");
                return ExitCode::FAILURE;
            }
            ExitCode::SUCCESS
        }
        Err(error) => {
            eprintln!("sonic-cfggen-lite-rs: {error}");
            ExitCode::from(error.exit_code())
        }
    }
}
