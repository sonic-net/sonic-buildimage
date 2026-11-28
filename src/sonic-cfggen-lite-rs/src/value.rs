use std::str::FromStr;

use crate::error::{AppError, Result};
use indexmap::IndexMap;
use num_bigint::BigInt;

#[derive(Clone, Debug, Eq, Hash, PartialEq)]
pub enum DataKey {
    Text(String),
    Composite(Vec<String>),
}

impl DataKey {
    pub fn text(value: impl Into<String>) -> Self {
        Self::Text(value.into())
    }
}

#[derive(Clone, Debug, PartialEq)]
pub enum DataValue {
    Object(IndexMap<DataKey, DataValue>),
    Array(Vec<DataValue>),
    String(String),
    Integer(BigInt),
    Float(f64),
    Bool(bool),
    Null,
}

impl DataValue {
    pub fn into_object(self, source: &str) -> Result<IndexMap<DataKey, DataValue>> {
        match self {
            Self::Object(object) => Ok(object),
            _ => Err(AppError::operational(format!(
                "{source} must contain an object at its root"
            ))),
        }
    }
}

pub fn from_json_value(value: serde_json::Value) -> Result<DataValue> {
    match value {
        serde_json::Value::Null => Ok(DataValue::Null),
        serde_json::Value::Bool(value) => Ok(DataValue::Bool(value)),
        serde_json::Value::String(value) => Ok(DataValue::String(value)),
        serde_json::Value::Array(values) => values
            .into_iter()
            .map(from_json_value)
            .collect::<Result<Vec<_>>>()
            .map(DataValue::Array),
        serde_json::Value::Object(values) => values
            .into_iter()
            .map(|(key, value)| Ok((DataKey::Text(key), from_json_value(value)?)))
            .collect::<Result<IndexMap<_, _>>>()
            .map(DataValue::Object),
        serde_json::Value::Number(number) => {
            let spelling = number.as_str();
            if spelling.contains(['.', 'e', 'E']) {
                let value = f64::from_str(spelling).map_err(|_| {
                    AppError::operational("JSON float is outside finite binary64 range")
                })?;
                if !value.is_finite() {
                    return Err(AppError::operational(
                        "JSON float is outside finite binary64 range",
                    ));
                }
                Ok(DataValue::Float(value))
            } else {
                BigInt::from_str(spelling)
                    .map(DataValue::Integer)
                    .map_err(|_| AppError::operational("invalid JSON integer"))
            }
        }
    }
}
