use std::{collections::HashSet, env, fs, path::Path};

use indexmap::IndexMap;
use serde::de::{Deserialize, Deserializer, Error as _, MapAccess, SeqAccess, Visitor};

use crate::{
    cli::Cli,
    error::{AppError, Result},
    value::{from_json_value, DataKey, DataValue},
    yaml,
};

#[derive(Clone, Debug, PartialEq)]
pub struct LoadedData {
    pub root: IndexMap<DataKey, DataValue>,
    pub non_db_source_participates: bool,
}

pub fn load(cli: &Cli) -> Result<LoadedData> {
    load_with_namespace(cli, env::var_os("NAMESPACE_ID"))
}

fn load_with_namespace(cli: &Cli, namespace: Option<std::ffi::OsString>) -> Result<LoadedData> {
    let mut root = IndexMap::new();
    let mut namespace_participates = false;
    if let Some(namespace) = namespace {
        let namespace = namespace
            .into_string()
            .map_err(|_| AppError::operational("NAMESPACE_ID contains non-Unicode data"))?;
        if !namespace.is_empty() {
            namespace_participates = true;
            merge_source(&mut root, namespace_seed(namespace), "NAMESPACE_ID")?;
        }
    }

    for (index, path) in cli.json.iter().enumerate() {
        let source = format!("JSON[{}] {}", index + 1, path.display());
        let mut value = parse_json_file(path, &source)?;
        convert_file_row_keys(&mut value);
        merge_source(&mut root, value.into_object(&source)?, &source)?;
    }
    for (index, path) in cli.yaml.iter().enumerate() {
        let source = format!("YAML[{}] {}", index + 1, path.display());
        let mut value = parse_yaml_file(path, &source)?;
        convert_file_row_keys(&mut value);
        merge_source(&mut root, value.into_object(&source)?, &source)?;
    }
    if let Some(additional) = cli.additional_data.as_deref() {
        let value = parse_json(additional, "--additional-data")?;
        merge_source(
            &mut root,
            value.into_object("--additional-data")?,
            "--additional-data",
        )?;
    }
    Ok(LoadedData {
        root,
        non_db_source_participates: namespace_participates
            || !cli.json.is_empty()
            || !cli.yaml.is_empty()
            || cli.additional_data.is_some(),
    })
}

fn parse_json_file(path: &Path, source: &str) -> Result<DataValue> {
    let contents = read_utf8(path, source)?;
    parse_json(&contents, source)
}

fn parse_yaml_file(path: &Path, source: &str) -> Result<DataValue> {
    let contents = read_utf8(path, source)?;
    yaml::parse(&contents).map_err(|error| AppError::operational(format!("{source}: {error}")))
}

fn read_utf8(path: &Path, source: &str) -> Result<String> {
    fs::read_to_string(path).map_err(|error| {
        AppError::operational(format!("{source}: unable to read UTF-8 input: {error}"))
    })
}

pub fn parse_json(input: &str, source: &str) -> Result<DataValue> {
    let mut duplicate_check = serde_json::Deserializer::from_str(input);
    DuplicateChecked::deserialize(&mut duplicate_check)
        .and_then(|_| duplicate_check.end())
        .map_err(|error| AppError::operational(format!("{source}: invalid JSON: {error}")))?;

    let value = serde_json::from_str(input)
        .map_err(|error| AppError::operational(format!("{source}: invalid JSON: {error}")))?;
    from_json_value(value).map_err(|error| AppError::operational(format!("{source}: {error}")))
}

pub fn convert_file_row_keys(value: &mut DataValue) {
    let DataValue::Object(root) = value else {
        return;
    };
    for child in root.values_mut() {
        let DataValue::Object(rows) = child else {
            continue;
        };
        let old_rows = std::mem::take(rows);
        rows.extend(old_rows.into_iter().map(|(key, value)| {
            let key = match key {
                DataKey::Text(text) if text.contains('|') => {
                    DataKey::Composite(text.split('|').map(str::to_owned).collect())
                }
                other => other,
            };
            (key, value)
        }));
    }
}

pub fn merge_source(
    destination: &mut IndexMap<DataKey, DataValue>,
    incoming: IndexMap<DataKey, DataValue>,
    source: &str,
) -> Result<()> {
    let mut candidate = destination.clone();
    merge_object(&mut candidate, incoming, source)?;
    *destination = candidate;
    Ok(())
}

fn merge_object(
    destination: &mut IndexMap<DataKey, DataValue>,
    incoming: IndexMap<DataKey, DataValue>,
    path: &str,
) -> Result<()> {
    for (key, incoming_value) in incoming {
        let component = key_display(&key);
        let child_path = format!("{path}.{component}");
        match incoming_value {
            DataValue::Object(incoming_object) => match destination.get_mut(&key) {
                None => {
                    destination.insert(key, DataValue::Object(incoming_object));
                }
                Some(DataValue::Object(destination_object)) => {
                    merge_object(destination_object, incoming_object, &child_path)?;
                }
                Some(_) => {
                    return Err(AppError::operational(format!(
                        "merge conflict at {child_path}: object cannot replace a non-object"
                    )))
                }
            },
            replacement => {
                if let Some(existing) = destination.get_mut(&key) {
                    *existing = replacement;
                } else {
                    destination.insert(key, replacement);
                }
            }
        }
    }
    Ok(())
}

fn key_display(key: &DataKey) -> String {
    match key {
        DataKey::Text(text) => text.clone(),
        DataKey::Composite(components) => format!("({})", components.join(",")),
    }
}

fn namespace_seed(namespace: String) -> IndexMap<DataKey, DataValue> {
    let mut localhost = IndexMap::new();
    localhost.insert(DataKey::text("namespace_id"), DataValue::String(namespace));
    let mut metadata = IndexMap::new();
    metadata.insert(DataKey::text("localhost"), DataValue::Object(localhost));
    let mut root = IndexMap::new();
    root.insert(
        DataKey::text("DEVICE_METADATA"),
        DataValue::Object(metadata),
    );
    root
}

struct DuplicateChecked;

impl<'de> Deserialize<'de> for DuplicateChecked {
    fn deserialize<D>(deserializer: D) -> std::result::Result<Self, D::Error>
    where
        D: Deserializer<'de>,
    {
        deserializer.deserialize_any(DuplicateVisitor)
    }
}

struct DuplicateVisitor;

impl<'de> Visitor<'de> for DuplicateVisitor {
    type Value = DuplicateChecked;

    fn expecting(&self, formatter: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        formatter.write_str("a JSON value")
    }

    fn visit_bool<E>(self, _: bool) -> std::result::Result<Self::Value, E> {
        Ok(DuplicateChecked)
    }

    fn visit_i64<E>(self, _: i64) -> std::result::Result<Self::Value, E> {
        Ok(DuplicateChecked)
    }

    fn visit_u64<E>(self, _: u64) -> std::result::Result<Self::Value, E> {
        Ok(DuplicateChecked)
    }

    fn visit_f64<E>(self, _: f64) -> std::result::Result<Self::Value, E> {
        Ok(DuplicateChecked)
    }

    fn visit_str<E>(self, _: &str) -> std::result::Result<Self::Value, E> {
        Ok(DuplicateChecked)
    }

    fn visit_none<E>(self) -> std::result::Result<Self::Value, E> {
        Ok(DuplicateChecked)
    }

    fn visit_unit<E>(self) -> std::result::Result<Self::Value, E> {
        Ok(DuplicateChecked)
    }

    fn visit_seq<A>(self, mut sequence: A) -> std::result::Result<Self::Value, A::Error>
    where
        A: SeqAccess<'de>,
    {
        while sequence.next_element::<DuplicateChecked>()?.is_some() {}
        Ok(DuplicateChecked)
    }

    fn visit_map<A>(self, mut map: A) -> std::result::Result<Self::Value, A::Error>
    where
        A: MapAccess<'de>,
    {
        let mut keys = HashSet::new();
        while let Some(key) = map.next_key::<String>()? {
            if !keys.insert(key) {
                return Err(A::Error::custom("duplicate object key"));
            }
            map.next_value::<DuplicateChecked>()?;
        }
        Ok(DuplicateChecked)
    }
}

#[cfg(test)]
mod tests {
    use std::{ffi::OsString, path::PathBuf};

    use num_bigint::BigInt;
    use tempfile::TempDir;

    use super::*;
    use crate::cli::Cli;

    fn cli(arguments: &[&str]) -> Cli {
        Cli::parse_checked_from(
            std::iter::once("sonic-cfggen-lite-rs").chain(arguments.iter().copied()),
        )
        .unwrap()
    }

    fn write(temp: &TempDir, name: &str, contents: &[u8]) -> PathBuf {
        let path = temp.path().join(name);
        fs::write(&path, contents).unwrap();
        path
    }

    #[test]
    fn strict_json_preserves_large_integers_and_rejects_duplicates_and_overflow() {
        let value = parse_json(
            r#"{"integer":123456789012345678901234567890,"float":1.5}"#,
            "fixture",
        )
        .unwrap();
        let object = value.into_object("fixture").unwrap();
        assert_eq!(
            object[&DataKey::text("integer")],
            DataValue::Integer(BigInt::parse_bytes(b"123456789012345678901234567890", 10).unwrap())
        );
        assert_eq!(object[&DataKey::text("float")], DataValue::Float(1.5));
        assert!(parse_json(r#"{"a":1,"a":2}"#, "fixture").is_err());
        assert!(parse_json(r#"{"a":1e9999}"#, "fixture").is_err());
        assert!(parse_json(r#"{"a":NaN}"#, "fixture").is_err());
        assert!(parse_json(r#"{"a":"\uD800"}"#, "fixture").is_err());
        assert!(parse_json("[]", "fixture")
            .unwrap()
            .into_object("fixture")
            .is_err());
    }

    #[test]
    fn strict_json_accepts_every_supported_nested_value_kind() {
        let value = parse_json(
            r#"{"array":[null,true,false,"text",-2,2.25,{"nested":1}],"empty":{}}"#,
            "fixture",
        )
        .unwrap();
        let object = value.into_object("fixture").unwrap();
        let DataValue::Array(array) = &object[&DataKey::text("array")] else {
            panic!("array is not an array")
        };
        assert_eq!(array[0], DataValue::Null);
        assert_eq!(array[1], DataValue::Bool(true));
        assert_eq!(array[2], DataValue::Bool(false));
        assert_eq!(array[3], DataValue::String("text".into()));
        assert_eq!(array[4], DataValue::Integer(BigInt::from(-2)));
        assert_eq!(array[5], DataValue::Float(2.25));
        assert!(matches!(array[6], DataValue::Object(_)));
        assert!(parse_json(r#"{"outer":{"same":1,"same":2}}"#, "fixture").is_err());
    }

    #[test]
    fn converts_only_file_second_level_separator_keys() {
        let mut value = parse_json(
            r#"{"TABLE":{"a|b":{"deep|field":1},"plain":2},"top|name":{"row|key":3}}"#,
            "fixture",
        )
        .unwrap();
        convert_file_row_keys(&mut value);
        let root = value.into_object("fixture").unwrap();
        assert!(root.contains_key(&DataKey::text("top|name")));
        let DataValue::Object(rows) = &root[&DataKey::text("TABLE")] else {
            panic!("TABLE is not an object")
        };
        assert!(rows.contains_key(&DataKey::Composite(vec!["a".into(), "b".into()])));
        let DataValue::Object(fields) = &rows[&DataKey::Composite(vec!["a".into(), "b".into()])]
        else {
            panic!("row is not an object")
        };
        assert!(fields.contains_key(&DataKey::text("deep|field")));
    }

    #[test]
    fn source_order_is_namespace_then_json_then_yaml_then_inline() {
        let temp = TempDir::new().unwrap();
        let json1 = write(&temp, "one.json", br#"{"A":{"value":"json1","first":1}}"#);
        let json2 = write(&temp, "two.json", br#"{"A":{"value":"json2","second":2}}"#);
        let yaml = write(&temp, "three.yml", b"A:\n  value: yaml\n  third: 3\n");
        let args = cli(&[
            "-y",
            yaml.to_str().unwrap(),
            "-j",
            json1.to_str().unwrap(),
            "-a",
            r#"{"A":{"value":"inline","fourth":4}}"#,
            "-j",
            json2.to_str().unwrap(),
            "--print-data",
        ]);
        let loaded = load_with_namespace(&args, Some(OsString::from("asic0"))).unwrap();
        let DataValue::Object(a) = &loaded.root[&DataKey::text("A")] else {
            panic!("A is not an object")
        };
        assert_eq!(
            a[&DataKey::text("value")],
            DataValue::String("inline".into())
        );
        assert_eq!(
            a.keys().cloned().collect::<Vec<_>>(),
            ["value", "first", "second", "third", "fourth"]
                .map(DataKey::text)
                .to_vec()
        );
        assert!(loaded.root.contains_key(&DataKey::text("DEVICE_METADATA")));
    }

    #[test]
    fn merge_conflict_is_atomic_and_exact_key_typed() {
        let mut destination = parse_json(r#"{"A":1}"#, "base")
            .unwrap()
            .into_object("base")
            .unwrap();
        let before = destination.clone();
        let incoming = parse_json(r#"{"A":{"child":2}}"#, "next")
            .unwrap()
            .into_object("next")
            .unwrap();
        assert!(merge_source(&mut destination, incoming, "next").is_err());
        assert_eq!(destination, before);

        let mut destination = IndexMap::new();
        destination.insert(DataKey::text("a|b"), DataValue::Integer(BigInt::from(1)));
        let mut incoming = IndexMap::new();
        incoming.insert(
            DataKey::Composite(vec!["a".into(), "b".into()]),
            DataValue::Integer(BigInt::from(2)),
        );
        merge_source(&mut destination, incoming, "next").unwrap();
        assert_eq!(destination.len(), 2);
    }

    #[test]
    fn empty_namespace_and_non_object_file_cases_are_handled_before_actions() {
        let temp = TempDir::new().unwrap();
        let scalar_yaml = write(&temp, "scalar.yml", b"- item\n");
        let args = cli(&["-y", scalar_yaml.to_str().unwrap(), "--print-data"]);
        assert!(load_with_namespace(&args, Some(OsString::new())).is_err());

        let missing = temp.path().join("missing.json");
        let args = cli(&["-j", missing.to_str().unwrap(), "--print-data"]);
        assert!(load_with_namespace(&args, None)
            .unwrap_err()
            .to_string()
            .contains("unable to read"));

        let args = cli(&["--print-data"]);
        assert!(load_with_namespace(&args, None).unwrap().root.is_empty());
        assert!(load_with_namespace(&args, Some(OsString::new()))
            .unwrap()
            .root
            .is_empty());

        let mut scalar = DataValue::String("unchanged".into());
        convert_file_row_keys(&mut scalar);
        assert_eq!(scalar, DataValue::String("unchanged".into()));
    }

    #[test]
    fn reports_file_utf8_and_non_unicode_namespace_without_payload_disclosure() {
        let temp = TempDir::new().unwrap();
        let invalid = write(&temp, "invalid.json", &[0xff]);
        let args = cli(&["-j", invalid.to_str().unwrap(), "--print-data"]);
        assert!(load_with_namespace(&args, None)
            .unwrap_err()
            .to_string()
            .contains("UTF-8"));

        #[cfg(unix)]
        {
            use std::os::unix::ffi::OsStringExt;
            let error =
                load_with_namespace(&args, Some(OsString::from_vec(vec![0xff]))).unwrap_err();
            assert!(error.to_string().contains("NAMESPACE_ID"));
        }

        let secret = "top-secret-value";
        let args = cli(&[
            "-a",
            &format!(r#"{{"secret":"{secret}",}}"#),
            "--print-data",
        ]);
        assert!(!load_with_namespace(&args, None)
            .unwrap_err()
            .to_string()
            .contains(secret));
    }
}
