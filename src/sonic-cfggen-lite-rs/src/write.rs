use std::collections::HashMap;

use indexmap::IndexMap;
use swss_common::{
    sonic_db_config_initialize_global, ConfigDBPipeConnector, ConfigDbRawMap, CxxString,
};

use crate::{
    cli::Cli,
    error::{AppError, Result},
    output::python_float,
    value::{DataKey, DataValue},
};

pub trait ConfigDbWriter {
    fn connect(&mut self, wait_for_init: bool, retry_on: bool) -> Result<()>;
    fn mod_config_raw(&mut self, config: &ConfigDbRawMap) -> Result<()>;
}

pub trait ConfigDbWriterFactory {
    type Writer: ConfigDbWriter;

    fn create(
        &mut self,
        use_unix_socket_path: bool,
        namespace: Option<&str>,
    ) -> Result<Self::Writer>;
}

pub fn write(cli: &Cli, root: &IndexMap<DataKey, DataValue>) -> Result<()> {
    write_with_factory(cli, root, &mut SwssWriterFactory)
}

pub fn write_with_factory<F: ConfigDbWriterFactory>(
    cli: &Cli,
    root: &IndexMap<DataKey, DataValue>,
    factory: &mut F,
) -> Result<()> {
    let plan = normalize(root)?;
    let mut writer = factory.create(true, cli.namespace.as_deref())?;
    writer.connect(false, false)?;
    writer.mod_config_raw(&plan)
}

fn normalize(root: &IndexMap<DataKey, DataValue>) -> Result<ConfigDbRawMap> {
    // Python indexes every table name before it considers eligibility. Keep
    // this scan separate so no sibling can be normalized before an empty name
    // is rejected.
    for table_key in root.keys() {
        let table_name = table_name(table_key)?;
        if table_name.is_empty() {
            return Err(AppError::operational("CONFIG_DB table name is empty"));
        }
    }

    let mut plan = ConfigDbRawMap::new();
    for (table_key, table_value) in root {
        let table_name = table_name(table_key)?;
        if !is_schema_table(table_name) {
            continue;
        }
        validate_identifier(table_name, "table name")?;

        match table_value {
            DataValue::Null => {
                plan.insert(table_name.to_owned(), HashMap::new());
            }
            DataValue::Object(rows) if rows.is_empty() => {}
            DataValue::Object(rows) => {
                let rows = normalize_rows(rows)?;
                plan.insert(table_name.to_owned(), rows);
            }
            _ => {
                return Err(AppError::operational(format!(
                    "CONFIG_DB table {table_name} must be an object or null"
                )))
            }
        }
    }
    Ok(plan)
}

fn is_schema_table(name: &str) -> bool {
    name.as_bytes().first().is_some_and(u8::is_ascii_uppercase)
}

fn table_name(key: &DataKey) -> Result<&str> {
    match key {
        DataKey::Text(name) => Ok(name),
        DataKey::Composite(_) => Err(AppError::operational("CONFIG_DB table name must be text")),
    }
}

fn normalize_rows(
    rows: &IndexMap<DataKey, DataValue>,
) -> Result<HashMap<String, HashMap<String, CxxString>>> {
    // IndexMap makes the typed-key collision rule explicit: replacement keeps
    // the raw key's position while the later typed row supplies its value.
    let mut normalized = IndexMap::<String, HashMap<String, CxxString>>::new();
    for (row_key, row_value) in rows {
        let raw_key = match row_key {
            DataKey::Text(key) => key.clone(),
            DataKey::Composite(components) => components.join("|"),
        };
        validate_identifier(&raw_key, "row key")?;
        let fields = normalize_row(row_value)?;
        normalized.insert(raw_key, fields);
    }
    Ok(normalized.into_iter().collect())
}

fn normalize_row(value: &DataValue) -> Result<HashMap<String, CxxString>> {
    match value {
        DataValue::Null => Ok(HashMap::new()),
        DataValue::Object(fields) if fields.is_empty() => {
            Ok(HashMap::from([("NULL".to_owned(), CxxString::new("NULL"))]))
        }
        DataValue::Object(fields) => {
            let mut normalized = HashMap::new();
            for (field_key, field_value) in fields {
                let DataKey::Text(field_name) = field_key else {
                    return Err(AppError::operational("CONFIG_DB field name must be text"));
                };
                validate_identifier(field_name, "field name")?;
                let (field_name, value) = normalize_field(field_name, field_value)?;
                // The list suffix is part of the raw field identifier and is
                // validated independently in case its construction changes.
                validate_identifier(&field_name, "field name")?;
                normalized.insert(field_name, CxxString::new(value));
            }
            Ok(normalized)
        }
        _ => Err(AppError::operational(
            "CONFIG_DB row must be an object or null",
        )),
    }
}

fn normalize_field(name: &str, value: &DataValue) -> Result<(String, String)> {
    let value = match value {
        DataValue::String(value) => value.clone(),
        DataValue::Integer(value) => value.to_string(),
        DataValue::Float(value) => python_float(*value)?,
        DataValue::Bool(value) => {
            if *value {
                "True".to_owned()
            } else {
                "False".to_owned()
            }
        }
        DataValue::Null => "None".to_owned(),
        DataValue::Array(values) => {
            let mut members = Vec::with_capacity(values.len());
            for value in values {
                let DataValue::String(value) = value else {
                    return Err(AppError::operational(
                        "CONFIG_DB list fields may contain only strings",
                    ));
                };
                members.push(value.as_str());
            }
            return Ok((format!("{name}@"), members.join(",")));
        }
        DataValue::Object(_) => {
            return Err(AppError::operational(
                "CONFIG_DB field value has an unsupported object shape",
            ))
        }
    };
    Ok((name.to_owned(), value))
}

fn validate_identifier(value: &str, kind: &str) -> Result<()> {
    if value.contains('\0') {
        Err(AppError::operational(format!(
            "CONFIG_DB {kind} contains U+0000"
        )))
    } else {
        Ok(())
    }
}

struct SwssWriterFactory;

struct SwssWriter(ConfigDBPipeConnector);

impl ConfigDbWriterFactory for SwssWriterFactory {
    type Writer = SwssWriter;

    fn create(
        &mut self,
        use_unix_socket_path: bool,
        namespace: Option<&str>,
    ) -> Result<Self::Writer> {
        if namespace.is_some() {
            sonic_db_config_initialize_global(
                "/var/run/redis/sonic-db/database_global.json",
                false,
            )
            .map_err(map_swss_error)?;
        }
        ConfigDBPipeConnector::new(use_unix_socket_path, namespace.map(str::to_owned))
            .map(SwssWriter)
            .map_err(map_swss_error)
    }
}

impl ConfigDbWriter for SwssWriter {
    fn connect(&mut self, wait_for_init: bool, retry_on: bool) -> Result<()> {
        self.0
            .connect(wait_for_init, retry_on)
            .map_err(map_swss_error)
    }

    fn mod_config_raw(&mut self, config: &ConfigDbRawMap) -> Result<()> {
        self.0.mod_config_raw(config).map_err(map_swss_error)
    }
}

fn map_swss_error(error: swss_common::Exception) -> AppError {
    let message = error.message();
    if message.contains("transaction acknowledgement")
        || message.contains("transaction connection lost")
        || message.contains("transaction interrupted")
        || message.contains("transaction reply count")
        || message.contains("transaction reply type")
    {
        AppError::operational("CONFIG_DB transaction outcome is ambiguous")
    } else {
        AppError::operational("CONFIG_DB operation failed")
    }
}

#[cfg(test)]
mod tests {
    use std::{cell::RefCell, rc::Rc};

    use indexmap::indexmap;

    use super::*;

    #[derive(Clone, Debug, PartialEq, Eq)]
    enum Call {
        Create(bool, Option<String>),
        Connect(bool, bool),
        Mod,
    }

    #[derive(Clone, Debug, PartialEq, Eq)]
    struct CapturedPlan(HashMap<String, HashMap<String, HashMap<String, Vec<u8>>>>);

    struct FakeFactory {
        calls: Rc<RefCell<Vec<Call>>>,
        captured: Rc<RefCell<Option<CapturedPlan>>>,
        connect_error: bool,
        mod_error: Option<&'static str>,
    }

    struct FakeWriter {
        calls: Rc<RefCell<Vec<Call>>>,
        captured: Rc<RefCell<Option<CapturedPlan>>>,
        connect_error: bool,
        mod_error: Option<&'static str>,
    }

    impl Default for FakeFactory {
        fn default() -> Self {
            Self {
                calls: Rc::new(RefCell::new(Vec::new())),
                captured: Rc::new(RefCell::new(None)),
                connect_error: false,
                mod_error: None,
            }
        }
    }

    impl ConfigDbWriterFactory for FakeFactory {
        type Writer = FakeWriter;

        fn create(
            &mut self,
            use_unix_socket_path: bool,
            namespace: Option<&str>,
        ) -> Result<Self::Writer> {
            self.calls.borrow_mut().push(Call::Create(
                use_unix_socket_path,
                namespace.map(str::to_owned),
            ));
            Ok(FakeWriter {
                calls: self.calls.clone(),
                captured: self.captured.clone(),
                connect_error: self.connect_error,
                mod_error: self.mod_error,
            })
        }
    }

    impl ConfigDbWriter for FakeWriter {
        fn connect(&mut self, wait_for_init: bool, retry_on: bool) -> Result<()> {
            self.calls
                .borrow_mut()
                .push(Call::Connect(wait_for_init, retry_on));
            if self.connect_error {
                Err(AppError::operational("confirmed connection failure"))
            } else {
                Ok(())
            }
        }

        fn mod_config_raw(&mut self, config: &ConfigDbRawMap) -> Result<()> {
            self.calls.borrow_mut().push(Call::Mod);
            let captured = config
                .iter()
                .map(|(table, rows)| {
                    let rows = rows
                        .iter()
                        .map(|(row, fields)| {
                            let fields = fields
                                .iter()
                                .map(|(field, value)| (field.clone(), value.as_bytes().to_vec()))
                                .collect();
                            (row.clone(), fields)
                        })
                        .collect();
                    (table.clone(), rows)
                })
                .collect();
            *self.captured.borrow_mut() = Some(CapturedPlan(captured));
            self.mod_error
                .map_or(Ok(()), |message| Err(AppError::operational(message)))
        }
    }

    fn cli(arguments: &[&str]) -> Cli {
        Cli::parse_checked_from(
            std::iter::once("sonic-cfggen-lite-rs").chain(arguments.iter().copied()),
        )
        .unwrap()
    }

    #[test]
    fn normalizes_all_write_value_and_delete_semantics_once() {
        let root = indexmap! {
            DataKey::text("TABLE") => DataValue::Object(indexmap! {
                DataKey::text("row") => DataValue::Object(indexmap! {
                    DataKey::text("string") => DataValue::String("a\0b".into()),
                    DataKey::text("integer") => DataValue::Integer(123.into()),
                    DataKey::text("float") => DataValue::Float(1e-7),
                    DataKey::text("true") => DataValue::Bool(true),
                    DataKey::text("false") => DataValue::Bool(false),
                    DataKey::text("none") => DataValue::Null,
                    DataKey::text("members") => DataValue::Array(vec![
                        DataValue::String("a\0b".into()),
                        DataValue::String("c".into()),
                    ]),
                }),
                DataKey::text("empty") => DataValue::Object(IndexMap::new()),
                DataKey::text("delete") => DataValue::Null,
            }),
            DataKey::text("DELETE_TABLE") => DataValue::Null,
            DataKey::text("EMPTY_TABLE") => DataValue::Object(IndexMap::new()),
        };
        let mut factory = FakeFactory::default();
        write_with_factory(
            &cli(&["-j", "unused", "-w", "-n", "asic0"]),
            &root,
            &mut factory,
        )
        .unwrap();
        assert_eq!(
            &*factory.calls.borrow(),
            &[
                Call::Create(true, Some("asic0".into())),
                Call::Connect(false, false),
                Call::Mod,
            ]
        );
        let captured = factory.captured.borrow();
        let plan = &captured.as_ref().unwrap().0;
        assert!(!plan.contains_key("EMPTY_TABLE"));
        assert!(plan["DELETE_TABLE"].is_empty());
        assert!(plan["TABLE"]["delete"].is_empty());
        assert_eq!(plan["TABLE"]["empty"]["NULL"], b"NULL");
        let fields = &plan["TABLE"]["row"];
        assert_eq!(fields["string"], b"a\0b");
        assert_eq!(fields["integer"], b"123");
        assert_eq!(fields["float"], b"1e-07");
        assert_eq!(fields["true"], b"True");
        assert_eq!(fields["false"], b"False");
        assert_eq!(fields["none"], b"None");
        assert_eq!(fields["members@"], b"a\0b,c");
    }

    #[test]
    fn applies_ascii_schema_table_filter_without_traversing_ignored_values() {
        let root = indexmap! {
            DataKey::text("ASCII") => DataValue::Null,
            DataKey::text("Ä_TABLE") => DataValue::Null,
            DataKey::text("a_lower") => DataValue::Array(vec![DataValue::Object(IndexMap::new())]),
            DataKey::text("1_uncased") => DataValue::String("ignored".into()),
            DataKey::text("ǅ_title_case") => DataValue::Object(indexmap! {
                DataKey::text("invalid") => DataValue::Array(vec![DataValue::Integer(1.into())]),
            }),
        };
        let plan = normalize(&root).unwrap();
        assert_eq!(
            plan.keys()
                .cloned()
                .collect::<std::collections::BTreeSet<_>>(),
            ["ASCII".to_owned()].into_iter().collect()
        );
    }

    #[test]
    fn later_text_row_wins_after_composite_raw_key_collision() {
        let root = indexmap! {
            DataKey::text("TABLE") => DataValue::Object(indexmap! {
                DataKey::Composite(vec!["a".into(), "b".into()]) => DataValue::Object(indexmap! {
                    DataKey::text("value") => DataValue::String("composite".into()),
                }),
                DataKey::text("a|b") => DataValue::Object(indexmap! {
                    DataKey::text("value") => DataValue::String("text".into()),
                }),
            }),
        };
        let plan = normalize(&root).unwrap();
        assert_eq!(plan["TABLE"].len(), 1);
        assert_eq!(plan["TABLE"]["a|b"]["value"].as_bytes(), b"text");
    }

    #[test]
    fn empty_and_nul_names_fail_before_connector_construction() {
        for (root, expected) in [
            (
                indexmap! {
                    DataKey::text("VALID") => DataValue::Null,
                    DataKey::text("") => DataValue::Null,
                },
                "CONFIG_DB table name is empty",
            ),
            (
                indexmap! { DataKey::text("T\0PREFIX") => DataValue::Null },
                "CONFIG_DB table name contains U+0000",
            ),
            (
                indexmap! {
                    DataKey::text("TABLE") => DataValue::Object(indexmap! {
                        DataKey::text("row\0prefix") => DataValue::Null,
                    }),
                },
                "CONFIG_DB row key contains U+0000",
            ),
            (
                indexmap! {
                    DataKey::text("TABLE") => DataValue::Object(indexmap! {
                        DataKey::text("row") => DataValue::Object(indexmap! {
                            DataKey::text("field\0prefix") => DataValue::String("value".into()),
                        }),
                    }),
                },
                "CONFIG_DB field name contains U+0000",
            ),
        ] {
            let mut factory = FakeFactory::default();
            let error =
                write_with_factory(&cli(&["-j", "unused", "-w"]), &root, &mut factory).unwrap_err();
            assert_eq!(error.to_string(), expected);
            assert!(factory.calls.borrow().is_empty());
        }
    }

    #[test]
    fn rejects_only_eligible_invalid_shapes_before_connector_construction() {
        let invalid_values = [
            DataValue::String("table".into()),
            DataValue::Array(Vec::new()),
            DataValue::Object(indexmap! {
                DataKey::text("row") => DataValue::String("row".into()),
            }),
            DataValue::Object(indexmap! {
                DataKey::text("row") => DataValue::Object(indexmap! {
                    DataKey::text("field") => DataValue::Object(IndexMap::new()),
                }),
            }),
            DataValue::Object(indexmap! {
                DataKey::text("row") => DataValue::Object(indexmap! {
                    DataKey::text("field") => DataValue::Array(vec![DataValue::Integer(1.into())]),
                }),
            }),
        ];
        for invalid in invalid_values {
            let root = indexmap! { DataKey::text("TABLE") => invalid };
            let mut factory = FakeFactory::default();
            assert!(
                write_with_factory(&cli(&["-j", "unused", "-w"]), &root, &mut factory,).is_err()
            );
            assert!(factory.calls.borrow().is_empty());
        }
    }

    #[test]
    fn propagates_confirmed_and_ambiguous_writer_failures_without_success() {
        for (connect_error, mod_error, expected_calls) in [
            (true, None, 2),
            (false, Some("CONFIG_DB transaction outcome is ambiguous"), 3),
            (false, Some("confirmed transaction failure"), 3),
        ] {
            let root = indexmap! { DataKey::text("TABLE") => DataValue::Null };
            let mut factory = FakeFactory {
                connect_error,
                mod_error,
                ..FakeFactory::default()
            };
            assert!(
                write_with_factory(&cli(&["-j", "unused", "-w"]), &root, &mut factory,).is_err()
            );
            assert_eq!(factory.calls.borrow().len(), expected_calls);
        }
    }
}
