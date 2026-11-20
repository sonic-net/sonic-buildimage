use std::collections::HashMap;

use indexmap::IndexMap;
use swss_common::{
    sonic_db_config_initialize_global, ConfigDBConnector, ConfigDBPipeConnector, CxxString,
};

use crate::{
    cli::Cli,
    error::{AppError, Result},
    input::{merge_source, LoadedData},
    selector::{Selector, SelectorPath},
    value::{DataKey, DataValue},
};

#[derive(Clone, Copy)]
pub enum DbRequest<'a> {
    Var(&'a SelectorPath),
    VarJson(&'a str),
    PrintData,
}

#[derive(Clone, Debug, PartialEq)]
pub struct RawRow {
    pub key: String,
    pub fields: Vec<(String, String)>,
}

#[derive(Clone, Debug, PartialEq)]
pub enum RawTable {
    Absent,
    Present(Vec<RawRow>),
}

#[derive(Clone, Debug, PartialEq)]
pub enum RawTargetedRow {
    TableAbsent,
    RowPresent(Vec<(String, String)>),
}

pub trait ConfigDbReader {
    fn connect(&mut self, wait_for_init: bool, retry_on: bool) -> Result<()>;
    fn targeted_row(&mut self, table: &str, row: &str) -> Result<RawTargetedRow>;
    fn probe_table(&mut self, table: &str) -> Result<bool>;
    fn ordered_table(&mut self, table: &str) -> Result<RawTable>;
    fn ordered_config(&mut self) -> Result<Vec<(String, Vec<RawRow>)>>;
}

pub trait ConfigDbFactory {
    type Reader: ConfigDbReader;

    fn create(
        &mut self,
        use_unix_socket_path: bool,
        namespace: Option<&str>,
        full_config: bool,
    ) -> Result<Self::Reader>;
}

pub fn augment(cli: &Cli, request: DbRequest<'_>, loaded: &mut LoadedData) -> Result<()> {
    let mut factory = SwssFactory;
    augment_with_factory(
        cli,
        request,
        loaded,
        &mut factory,
        rustix::process::getuid().is_root(),
    )
}

pub fn augment_with_factory<F: ConfigDbFactory>(
    cli: &Cli,
    request: DbRequest<'_>,
    loaded: &mut LoadedData,
    factory: &mut F,
    use_unix_socket_path: bool,
) -> Result<()> {
    if !cli.from_db {
        return Ok(());
    }

    let plan = plan(request, loaded.non_db_source_participates)?;
    let full_config = matches!(&plan, ReadPlan::Full);
    let mut reader = factory.create(use_unix_socket_path, cli.namespace.as_deref(), full_config)?;
    reader.connect(true, false)?;
    let incoming = execute_plan(&mut reader, plan)?;
    merge_source(&mut loaded.root, incoming, "CONFIG_DB")
}

#[derive(Clone, Debug, PartialEq)]
enum ReadPlan {
    Full,
    Table {
        table: String,
    },
    Row {
        table: String,
        row_key: DataKey,
        raw_row: String,
    },
    Probe {
        table: String,
    },
}

fn plan(request: DbRequest<'_>, mixed: bool) -> Result<ReadPlan> {
    if mixed {
        return Ok(ReadPlan::Full);
    }
    match request {
        DbRequest::PrintData => Ok(ReadPlan::Full),
        DbRequest::VarJson(table) => {
            validate_table(table)?;
            Ok(ReadPlan::Table {
                table: table.to_owned(),
            })
        }
        DbRequest::Var(path) => {
            let selectors = path.selectors();
            let Selector::Member(table) = &selectors[0] else {
                return Err(AppError::operational(
                    "selector root is not a CONFIG_DB table name",
                ));
            };
            validate_table(table)?;
            match selectors.get(1) {
                None => Ok(ReadPlan::Table {
                    table: table.clone(),
                }),
                Some(Selector::Index(_)) => Ok(ReadPlan::Probe {
                    table: table.clone(),
                }),
                Some(Selector::Member(row)) => {
                    validate_row_component(row)?;
                    validate_raw_row(row)?;
                    Ok(ReadPlan::Row {
                        table: table.clone(),
                        row_key: DataKey::Text(row.clone()),
                        raw_row: row.clone(),
                    })
                }
                Some(Selector::CompositeKey(components)) => {
                    for component in components {
                        validate_row_component(component)?;
                    }
                    let raw_row = components.join("|");
                    validate_raw_row(&raw_row)?;
                    Ok(ReadPlan::Row {
                        table: table.clone(),
                        row_key: DataKey::Composite(components.clone()),
                        raw_row,
                    })
                }
            }
        }
    }
}

fn validate_table(table: &str) -> Result<()> {
    if table.contains('\0') {
        Err(AppError::operational(
            "CONFIG_DB table name contains U+0000",
        ))
    } else {
        Ok(())
    }
}

fn validate_row_component(component: &str) -> Result<()> {
    if component.contains('\0') {
        Err(AppError::operational("CONFIG_DB row key contains U+0000"))
    } else {
        Ok(())
    }
}

fn validate_raw_row(row: &str) -> Result<()> {
    validate_row_component(row)
}

fn execute_plan<R: ConfigDbReader>(
    reader: &mut R,
    plan: ReadPlan,
) -> Result<IndexMap<DataKey, DataValue>> {
    let mut root = IndexMap::new();
    match plan {
        ReadPlan::Full => {
            for (table, rows) in reader.ordered_config()? {
                root.insert(DataKey::Text(table), decode_rows(rows)?);
            }
        }
        ReadPlan::Table { table } => match reader.ordered_table(&table)? {
            RawTable::Absent => {}
            RawTable::Present(rows) => {
                root.insert(DataKey::Text(table), decode_rows(rows)?);
            }
        },
        ReadPlan::Probe { table } => {
            if reader.probe_table(&table)? {
                root.insert(DataKey::Text(table), DataValue::Object(IndexMap::new()));
            }
        }
        ReadPlan::Row {
            table,
            row_key,
            raw_row,
        } => match reader.targeted_row(&table, &raw_row)? {
            RawTargetedRow::TableAbsent => {}
            RawTargetedRow::RowPresent(fields) => {
                let mut rows = IndexMap::new();
                rows.insert(row_key, decode_fields(fields)?);
                root.insert(DataKey::Text(table), DataValue::Object(rows));
            }
        },
    }
    Ok(root)
}

fn decode_rows(rows: Vec<RawRow>) -> Result<DataValue> {
    let mut output = IndexMap::new();
    for row in rows {
        let key = if row.key.contains('|') {
            DataKey::Composite(row.key.split('|').map(str::to_owned).collect())
        } else {
            DataKey::Text(row.key)
        };
        output.insert(key, decode_fields(row.fields)?);
    }
    Ok(DataValue::Object(output))
}

fn decode_fields(fields: Vec<(String, String)>) -> Result<DataValue> {
    let mut output = IndexMap::new();
    for (field, value) in fields {
        if field == "NULL" {
            continue;
        }
        if let Some(field) = field.strip_suffix('@') {
            output.insert(
                DataKey::Text(field.to_owned()),
                DataValue::Array(
                    value
                        .split(',')
                        .map(|item| DataValue::String(item.to_owned()))
                        .collect(),
                ),
            );
        } else {
            output.insert(DataKey::Text(field), DataValue::String(value));
        }
    }
    Ok(DataValue::Object(output))
}

struct SwssFactory;

enum SwssReader {
    Targeted(ConfigDBConnector),
    Full(ConfigDBPipeConnector),
}

impl ConfigDbFactory for SwssFactory {
    type Reader = SwssReader;

    fn create(
        &mut self,
        use_unix_socket_path: bool,
        namespace: Option<&str>,
        full_config: bool,
    ) -> Result<Self::Reader> {
        if namespace.is_some() {
            sonic_db_config_initialize_global(
                "/var/run/redis/sonic-db/database_global.json",
                false,
            )
            .map_err(map_swss_error)?;
        }
        if full_config {
            ConfigDBPipeConnector::new(use_unix_socket_path, namespace.map(str::to_owned))
                .map(SwssReader::Full)
                .map_err(map_swss_error)
        } else {
            ConfigDBConnector::new(use_unix_socket_path, namespace.map(str::to_owned))
                .map(SwssReader::Targeted)
                .map_err(map_swss_error)
        }
    }
}

impl ConfigDbReader for SwssReader {
    fn connect(&mut self, wait_for_init: bool, retry_on: bool) -> Result<()> {
        match self {
            Self::Targeted(reader) => reader.connect(wait_for_init, retry_on),
            Self::Full(reader) => reader.connect(wait_for_init, retry_on),
        }
        .map_err(map_swss_error)
    }

    fn targeted_row(&mut self, table: &str, row: &str) -> Result<RawTargetedRow> {
        let Self::Targeted(reader) = self else {
            unreachable!("full CONFIG_DB reader cannot perform targeted row reads")
        };
        let fields = reader.get_entry(table, row).map_err(map_swss_error)?;
        if fields.is_empty() {
            // The existing connector intentionally reports both a missing
            // table and a missing row as an empty map. The fast path accepts
            // that legacy connector limitation for schema-valid callers.
            Ok(RawTargetedRow::TableAbsent)
        } else {
            convert_fields(fields).map(RawTargetedRow::RowPresent)
        }
    }

    fn probe_table(&mut self, table: &str) -> Result<bool> {
        let Self::Targeted(reader) = self else {
            unreachable!("full CONFIG_DB reader cannot probe a table")
        };
        reader
            .get_keys(table, false)
            .map(|keys| !keys.is_empty())
            .map_err(map_swss_error)
    }

    fn ordered_table(&mut self, table: &str) -> Result<RawTable> {
        let Self::Targeted(reader) = self else {
            unreachable!("full CONFIG_DB reader cannot read a targeted table")
        };
        let rows = reader.get_table(table).map_err(map_swss_error)?;
        if rows.is_empty() {
            Ok(RawTable::Absent)
        } else {
            convert_rows(rows).map(RawTable::Present)
        }
    }

    fn ordered_config(&mut self) -> Result<Vec<(String, Vec<RawRow>)>> {
        let Self::Full(reader) = self else {
            unreachable!("targeted CONFIG_DB reader cannot perform a full read")
        };
        let config = reader.get_config().map_err(map_swss_error)?;
        let mut tables = config.into_iter().collect::<Vec<_>>();
        tables.sort_unstable_by(|left, right| left.0.cmp(&right.0));
        tables
            .into_iter()
            .map(|(table, rows)| Ok((table, convert_rows(rows)?)))
            .collect()
    }
}

fn convert_rows(rows: HashMap<String, HashMap<String, CxxString>>) -> Result<Vec<RawRow>> {
    let mut rows = rows.into_iter().collect::<Vec<_>>();
    rows.sort_unstable_by(|left, right| left.0.cmp(&right.0));
    rows.into_iter()
        .map(|(key, fields)| {
            Ok(RawRow {
                key,
                fields: convert_fields(fields)?,
            })
        })
        .collect()
}

fn convert_fields(fields: HashMap<String, CxxString>) -> Result<Vec<(String, String)>> {
    let mut fields = fields.into_iter().collect::<Vec<_>>();
    fields.sort_unstable_by(|left, right| left.0.cmp(&right.0));
    fields
        .into_iter()
        .map(|(field, value)| {
            let value = value
                .to_str()
                .map_err(|_| AppError::operational("CONFIG_DB value contains invalid UTF-8"))?
                .to_owned();
            Ok((field, value))
        })
        .collect()
}

fn map_swss_error(error: swss_common::Exception) -> AppError {
    match error.message() {
        "CONFIG_DB table name contains U+0000"
        | "CONFIG_DB row key contains U+0000"
        | "CONFIG_DB field name contains U+0000" => {
            AppError::operational(error.message().to_owned())
        }
        _ => AppError::operational("CONFIG_DB operation failed"),
    }
}

#[cfg(test)]
mod tests {
    use std::{cell::RefCell, rc::Rc};

    use super::*;
    use crate::{cli::Cli, selector::Evaluation};

    #[derive(Clone, Debug, PartialEq)]
    enum Call {
        Create(bool, Option<String>, bool),
        Connect(bool, bool),
        Row(String, String),
        Probe(String),
        Table(String),
        Full,
    }

    struct FakeFactory {
        calls: Rc<RefCell<Vec<Call>>>,
        row: RawTargetedRow,
        probe: bool,
        table: RawTable,
        config: Vec<(String, Vec<RawRow>)>,
    }

    struct FakeReader {
        calls: Rc<RefCell<Vec<Call>>>,
        row: RawTargetedRow,
        probe: bool,
        table: RawTable,
        config: Vec<(String, Vec<RawRow>)>,
    }

    impl Default for FakeFactory {
        fn default() -> Self {
            Self {
                calls: Rc::new(RefCell::new(Vec::new())),
                row: RawTargetedRow::TableAbsent,
                probe: false,
                table: RawTable::Absent,
                config: Vec::new(),
            }
        }
    }

    impl ConfigDbFactory for FakeFactory {
        type Reader = FakeReader;

        fn create(
            &mut self,
            use_unix_socket_path: bool,
            namespace: Option<&str>,
            full_config: bool,
        ) -> Result<Self::Reader> {
            self.calls.borrow_mut().push(Call::Create(
                use_unix_socket_path,
                namespace.map(str::to_owned),
                full_config,
            ));
            Ok(FakeReader {
                calls: self.calls.clone(),
                row: self.row.clone(),
                probe: self.probe,
                table: self.table.clone(),
                config: self.config.clone(),
            })
        }
    }

    impl ConfigDbReader for FakeReader {
        fn connect(&mut self, wait_for_init: bool, retry_on: bool) -> Result<()> {
            self.calls
                .borrow_mut()
                .push(Call::Connect(wait_for_init, retry_on));
            Ok(())
        }

        fn targeted_row(&mut self, table: &str, row: &str) -> Result<RawTargetedRow> {
            self.calls
                .borrow_mut()
                .push(Call::Row(table.into(), row.into()));
            Ok(self.row.clone())
        }

        fn probe_table(&mut self, table: &str) -> Result<bool> {
            self.calls.borrow_mut().push(Call::Probe(table.into()));
            Ok(self.probe)
        }

        fn ordered_table(&mut self, table: &str) -> Result<RawTable> {
            self.calls.borrow_mut().push(Call::Table(table.into()));
            Ok(self.table.clone())
        }

        fn ordered_config(&mut self) -> Result<Vec<(String, Vec<RawRow>)>> {
            self.calls.borrow_mut().push(Call::Full);
            Ok(self.config.clone())
        }
    }

    fn cli(arguments: &[&str]) -> Cli {
        Cli::parse_checked_from(
            std::iter::once("sonic-cfggen-lite-rs").chain(arguments.iter().copied()),
        )
        .unwrap()
    }

    fn loaded(participates: bool) -> LoadedData {
        LoadedData {
            root: IndexMap::new(),
            non_db_source_participates: participates,
        }
    }

    #[test]
    fn row_target_states_build_connector_compatible_envelopes() {
        let args = cli(&["-d", "-v", "TABLE.row.field"]);
        let path = SelectorPath::parse("TABLE.row.field").unwrap();
        for (state, expected) in [
            (
                RawTargetedRow::TableAbsent,
                Evaluation::MissingIntermediate(0),
            ),
            (
                RawTargetedRow::RowPresent(vec![("NULL".into(), "NULL".into())]),
                Evaluation::MissingFinal,
            ),
        ] {
            let mut factory = FakeFactory {
                row: state,
                ..FakeFactory::default()
            };
            let mut data = loaded(false);
            augment_with_factory(&args, DbRequest::Var(&path), &mut data, &mut factory, true)
                .unwrap();
            let root = DataValue::Object(data.root);
            assert_eq!(path.evaluate(&root), expected);
            assert_eq!(
                &*factory.calls.borrow(),
                &[
                    Call::Create(true, None, false),
                    Call::Connect(true, false),
                    Call::Row("TABLE".into(), "row".into()),
                ]
            );
        }
    }

    #[test]
    fn composite_rows_decode_lists_and_preserve_null_only_presence() {
        let args = cli(&["-d", "-v", "T[('a','b')]"]);
        let path = SelectorPath::parse("T[('a','b')]").unwrap();
        let mut factory = FakeFactory {
            row: RawTargetedRow::RowPresent(vec![
                ("NULL".into(), "NULL".into()),
                ("members@".into(), "one,two".into()),
                ("value".into(), "x\0y".into()),
            ]),
            ..FakeFactory::default()
        };
        let mut data = loaded(false);
        augment_with_factory(&args, DbRequest::Var(&path), &mut data, &mut factory, false).unwrap();
        let root = DataValue::Object(data.root);
        assert!(matches!(
            path.evaluate(&root),
            Evaluation::Value(DataValue::Object(_))
        ));
        assert!(factory
            .calls
            .borrow()
            .contains(&Call::Row("T".into(), "a|b".into())));
    }

    #[test]
    fn root_index_uses_only_probe_for_large_indices() {
        let args = cli(&["-d", "-v", "mixedCase[999999999999999999999999]"]);
        let path = SelectorPath::parse("mixedCase[999999999999999999999999]").unwrap();
        let mut factory = FakeFactory {
            probe: true,
            ..FakeFactory::default()
        };
        let mut data = loaded(false);
        augment_with_factory(&args, DbRequest::Var(&path), &mut data, &mut factory, false).unwrap();
        assert_eq!(
            &*factory.calls.borrow(),
            &[
                Call::Create(false, None, false),
                Call::Connect(true, false),
                Call::Probe("mixedCase".into()),
            ]
        );
        assert_eq!(
            path.evaluate(&DataValue::Object(data.root)),
            Evaluation::MissingFinal
        );
    }

    #[test]
    fn mixed_sources_force_full_read_and_merge_db_last() {
        let args = cli(&["-d", "-a", r#"{"A":{"value":"file"}}"#, "-v", "A.value"]);
        let path = SelectorPath::parse("A.value").unwrap();
        let mut factory = FakeFactory {
            config: vec![(
                "A".into(),
                vec![RawRow {
                    key: "value".into(),
                    fields: vec![("field".into(), "db".into())],
                }],
            )],
            ..FakeFactory::default()
        };
        let mut data = loaded(true);
        data.root.insert(
            DataKey::text("A"),
            DataValue::Object(IndexMap::from([(DataKey::text("old"), DataValue::Null)])),
        );
        augment_with_factory(&args, DbRequest::Var(&path), &mut data, &mut factory, true).unwrap();
        assert_eq!(
            &*factory.calls.borrow(),
            &[
                Call::Create(true, None, true),
                Call::Connect(true, false),
                Call::Full,
            ]
        );
        assert!(!factory
            .calls
            .borrow()
            .iter()
            .any(|call| matches!(call, Call::Row(_, _) | Call::Table(_) | Call::Probe(_))));
    }

    #[test]
    fn targeted_nul_is_rejected_before_connector_construction() {
        for (request, expected) in [
            (
                DbRequest::VarJson("bad\0table"),
                "CONFIG_DB table name contains U+0000",
            ),
            (
                DbRequest::Var(Box::leak(Box::new(
                    SelectorPath::parse("T['bad\\u0000row']").unwrap(),
                ))),
                "CONFIG_DB row key contains U+0000",
            ),
        ] {
            let args = cli(&["-d", "--var-json", "T"]);
            let mut factory = FakeFactory::default();
            let error =
                augment_with_factory(&args, request, &mut loaded(false), &mut factory, true)
                    .unwrap_err();
            assert_eq!(error.to_string(), expected);
            assert!(factory.calls.borrow().is_empty());
        }
    }

    #[test]
    fn named_namespace_and_var_json_table_reach_connector() {
        let args = cli(&["-d", "-n", "asic0", "--var-json", "A*?[x]\\B"]);
        let mut factory = FakeFactory::default();
        augment_with_factory(
            &args,
            DbRequest::VarJson("A*?[x]\\B"),
            &mut loaded(false),
            &mut factory,
            false,
        )
        .unwrap();
        assert_eq!(
            &*factory.calls.borrow(),
            &[
                Call::Create(false, Some("asic0".into()), false),
                Call::Connect(true, false),
                Call::Table("A*?[x]\\B".into()),
            ]
        );
    }
}
