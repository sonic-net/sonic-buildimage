use std::collections::BTreeMap;

use indexmap::IndexMap;

use crate::{
    error::{AppError, Result},
    value::{DataKey, DataValue},
};

pub fn render_legacy(value: &DataValue) -> Result<Vec<u8>> {
    let mut output = String::new();
    if let DataValue::String(value) = value {
        output.push_str(value);
    } else {
        write_python_value(&mut output, value)?;
    }
    output.push('\n');
    Ok(output.into_bytes())
}

pub fn render_structured(value: &DataValue) -> Result<Vec<u8>> {
    let mut output = String::new();
    write_json_value(&mut output, value, 0)?;
    output.push('\n');
    Ok(output.into_bytes())
}

pub fn render_var_json(
    root: &IndexMap<DataKey, DataValue>,
    table: &str,
    key: Option<&str>,
) -> Result<Vec<u8>> {
    let Some(table_value) = root.get(&DataKey::Text(table.to_owned())) else {
        return Ok(Vec::new());
    };

    let Some(filter) = key else {
        return render_structured(table_value);
    };
    let DataValue::Object(rows) = table_value else {
        return Err(AppError::operational(
            "--key requires the selected --var-json value to be an object",
        ));
    };

    let mut selected = IndexMap::new();
    if let Some((row_key, row_value)) = rows.iter().find(|(row_key, _)| match row_key {
        DataKey::Text(text) => text == filter,
        DataKey::Composite(components) => components.iter().any(|component| component == filter),
    }) {
        selected.insert(row_key.clone(), row_value.clone());
    }
    render_structured(&DataValue::Object(selected))
}

fn write_python_value(output: &mut String, value: &DataValue) -> Result<()> {
    match value {
        DataValue::Object(object) => {
            output.push('{');
            for (index, (key, value)) in object.iter().enumerate() {
                if index != 0 {
                    output.push_str(", ");
                }
                write_python_key(output, key);
                output.push_str(": ");
                write_python_value(output, value)?;
            }
            output.push('}');
        }
        DataValue::Array(array) => {
            output.push('[');
            for (index, value) in array.iter().enumerate() {
                if index != 0 {
                    output.push_str(", ");
                }
                write_python_value(output, value)?;
            }
            output.push(']');
        }
        DataValue::String(value) => write_python_string(output, value),
        DataValue::Integer(value) => output.push_str(&value.to_string()),
        DataValue::Float(value) => output.push_str(&python_float(*value)?),
        DataValue::Bool(value) => output.push_str(if *value { "True" } else { "False" }),
        DataValue::Null => output.push_str("None"),
    }
    Ok(())
}

fn write_python_key(output: &mut String, key: &DataKey) {
    match key {
        DataKey::Text(text) => write_python_string(output, text),
        DataKey::Composite(components) => {
            output.push('(');
            for (index, component) in components.iter().enumerate() {
                if index != 0 {
                    output.push_str(", ");
                }
                write_python_string(output, component);
            }
            if components.len() == 1 {
                output.push(',');
            }
            output.push(')');
        }
    }
}

fn write_python_string(output: &mut String, value: &str) {
    let quote = if value.contains('\'') && !value.contains('"') {
        '"'
    } else {
        '\''
    };
    output.push(quote);
    for character in value.chars() {
        match character {
            '\\' => output.push_str("\\\\"),
            '\t' => output.push_str("\\t"),
            '\n' => output.push_str("\\n"),
            '\r' => output.push_str("\\r"),
            character if character == quote => {
                output.push('\\');
                output.push(character);
            }
            character if python_printable(character) => output.push(character),
            character => write_python_codepoint_escape(output, character),
        }
    }
    output.push(quote);
}

fn python_printable(character: char) -> bool {
    if character.is_ascii() {
        return (' '..='~').contains(&character);
    }
    character.escape_debug().to_string() == character.to_string()
}

fn write_python_codepoint_escape(output: &mut String, character: char) {
    let codepoint = character as u32;
    if codepoint <= 0xff {
        output.push_str(&format!("\\x{codepoint:02x}"));
    } else if codepoint <= 0xffff {
        output.push_str(&format!("\\u{codepoint:04x}"));
    } else {
        output.push_str(&format!("\\U{codepoint:08x}"));
    }
}

pub fn python_float(value: f64) -> Result<String> {
    if !value.is_finite() {
        return Err(AppError::operational(
            "cannot render a non-finite binary64 value",
        ));
    }
    if value == 0.0 {
        return Ok(if value.is_sign_negative() {
            "-0.0".to_owned()
        } else {
            "0.0".to_owned()
        });
    }

    // serde_json's vendored Schubfach formatter supplies correctly rounded
    // shortest digits. Reapply Python's fixed/scientific thresholds and
    // exponent spelling to those digits rather than Rust's different tie
    // choice for a small set of binary64 values.
    let shortest = serde_json::Number::from_f64(value)
        .ok_or_else(|| AppError::operational("cannot render a non-finite binary64 value"))?
        .to_string();
    python_float_from_shortest(&shortest)
}

fn python_float_from_shortest(shortest: &str) -> Result<String> {
    let (sign, unsigned) = shortest
        .strip_prefix('-')
        .map_or(("", shortest), |value| ("-", value));
    let (mantissa, explicit_exponent) = match unsigned.split_once(['e', 'E']) {
        Some((mantissa, exponent)) => {
            let exponent = exponent
                .parse::<i32>()
                .map_err(|_| AppError::operational("invalid binary64 exponent"))?;
            (mantissa, exponent)
        }
        None => (unsigned, 0),
    };
    let (integer, fraction) = mantissa.split_once('.').unwrap_or((mantissa, ""));
    let combined = format!("{integer}{fraction}");
    let first_nonzero = combined
        .bytes()
        .position(|byte| byte != b'0')
        .ok_or_else(|| AppError::operational("invalid zero binary64 spelling"))?;
    let integer_digits = integer.len();
    let scientific_exponent = integer_digits as i32 - first_nonzero as i32 - 1 + explicit_exponent;
    let significant = combined[first_nonzero..].trim_end_matches('0');

    if !(-4..16).contains(&scientific_exponent) {
        let mut output = format!("{sign}{}", &significant[..1]);
        if significant.len() > 1 {
            output.push('.');
            output.push_str(&significant[1..]);
        }
        output.push_str(&format!("e{scientific_exponent:+03}"));
        return Ok(output);
    }

    let decimal_position = scientific_exponent + 1;
    let mut output = sign.to_owned();
    if decimal_position <= 0 {
        output.push_str("0.");
        output.extend(std::iter::repeat_n('0', (-decimal_position) as usize));
        output.push_str(significant);
    } else if decimal_position as usize >= significant.len() {
        output.push_str(significant);
        output.extend(std::iter::repeat_n(
            '0',
            decimal_position as usize - significant.len(),
        ));
        output.push_str(".0");
    } else {
        let position = decimal_position as usize;
        output.push_str(&significant[..position]);
        output.push('.');
        output.push_str(&significant[position..]);
    }
    Ok(output)
}

fn write_json_value(output: &mut String, value: &DataValue, depth: usize) -> Result<()> {
    match value {
        DataValue::Object(object) => write_json_object(output, object, depth)?,
        DataValue::Array(array) => write_json_array(output, array, depth)?,
        DataValue::String(value) => write_json_string(output, value),
        DataValue::Integer(value) => output.push_str(&value.to_string()),
        DataValue::Float(value) => output.push_str(&python_float(*value)?),
        DataValue::Bool(value) => output.push_str(if *value { "true" } else { "false" }),
        DataValue::Null => output.push_str("null"),
    }
    Ok(())
}

fn write_json_object(
    output: &mut String,
    object: &IndexMap<DataKey, DataValue>,
    depth: usize,
) -> Result<()> {
    let mut serialized = BTreeMap::<String, &DataValue>::new();
    for (key, value) in object {
        if let DataKey::Text(text) = key {
            serialized.insert(text.clone(), value);
        }
    }
    for (key, value) in object {
        if let DataKey::Composite(components) = key {
            serialized.insert(components.join("|"), value);
        }
    }

    if serialized.is_empty() {
        output.push_str("{}");
        return Ok(());
    }
    output.push_str("{\n");
    let length = serialized.len();
    for (index, (key, value)) in serialized.into_iter().enumerate() {
        indent(output, depth + 1);
        write_json_string(output, &key);
        output.push_str(": ");
        write_json_value(output, value, depth + 1)?;
        if index + 1 != length {
            output.push(',');
        }
        output.push('\n');
    }
    indent(output, depth);
    output.push('}');
    Ok(())
}

fn write_json_array(output: &mut String, array: &[DataValue], depth: usize) -> Result<()> {
    if array.is_empty() {
        output.push_str("[]");
        return Ok(());
    }
    output.push_str("[\n");
    for (index, value) in array.iter().enumerate() {
        indent(output, depth + 1);
        write_json_value(output, value, depth + 1)?;
        if index + 1 != array.len() {
            output.push(',');
        }
        output.push('\n');
    }
    indent(output, depth);
    output.push(']');
    Ok(())
}

fn write_json_string(output: &mut String, value: &str) {
    output.push('"');
    for character in value.chars() {
        match character {
            '"' => output.push_str("\\\""),
            '\\' => output.push_str("\\\\"),
            '\u{08}' => output.push_str("\\b"),
            '\u{0c}' => output.push_str("\\f"),
            '\n' => output.push_str("\\n"),
            '\r' => output.push_str("\\r"),
            '\t' => output.push_str("\\t"),
            ' '..='~' => output.push(character),
            character if (character as u32) <= 0xffff => {
                output.push_str(&format!("\\u{:04x}", character as u32));
            }
            character => {
                let codepoint = character as u32 - 0x10000;
                let high = 0xd800 + (codepoint >> 10);
                let low = 0xdc00 + (codepoint & 0x3ff);
                output.push_str(&format!("\\u{high:04x}\\u{low:04x}"));
            }
        }
    }
    output.push('"');
}

fn indent(output: &mut String, depth: usize) {
    for _ in 0..depth * 4 {
        output.push(' ');
    }
}

#[cfg(test)]
mod tests {
    use indexmap::indexmap;
    use num_bigint::BigInt;

    use super::*;

    #[test]
    fn renders_legacy_python_repr_for_all_supported_types() {
        let value = DataValue::Object(indexmap! {
            DataKey::text("a|b") => DataValue::String("joined's".into()),
            DataKey::Composite(vec!["a".into(), "b".into()]) => DataValue::Array(vec![
                DataValue::Integer(BigInt::parse_bytes(b"123456789012345678901234567890", 10).unwrap()),
                DataValue::Float(-0.0),
                DataValue::Float(1e-7),
                DataValue::Bool(true),
                DataValue::Null,
                DataValue::String("line\n😀\u{7f}".into()),
            ]),
        });
        assert_eq!(
            String::from_utf8(render_legacy(&value).unwrap()).unwrap(),
            r#"{'a|b': "joined's", ('a', 'b'): [123456789012345678901234567890, -0.0, 1e-07, True, None, 'line\n😀\x7f']}"#.to_owned() + "\n"
        );
        assert_eq!(
            render_legacy(&DataValue::String("raw\0😀".into())).unwrap(),
            b"raw\0\xF0\x9F\x98\x80\n"
        );

        let escapes = DataValue::Object(indexmap! {
            DataKey::Composite(vec!["single".into()]) => DataValue::String(
                "slash\\ tab\t return\r 'and\"' \u{2028}\u{e0001}".into()
            ),
        });
        assert_eq!(
            String::from_utf8(render_legacy(&escapes).unwrap()).unwrap(),
            "{('single',): 'slash\\\\ tab\\t return\\r \\'and\"\\' \\u2028\\U000e0001'}\n"
        );
    }

    #[test]
    fn renders_python_float_boundary_spellings() {
        for (value, expected) in [
            (1.0, "1.0"),
            (0.0, "0.0"),
            (-0.0, "-0.0"),
            (1e-7, "1e-07"),
            (1e-4, "0.0001"),
            (1e-5, "1e-05"),
            (1e15, "1000000000000000.0"),
            (1e16, "1e+16"),
            (5e-324, "5e-324"),
            (2.2250738585072014e-308, "2.2250738585072014e-308"),
            (1.7976931348623157e308, "1.7976931348623157e+308"),
        ] {
            assert_eq!(python_float(value).unwrap(), expected);
        }
        assert!(python_float(f64::INFINITY).is_err());
        assert_eq!(
            python_float(-1250201187231379.2_f64).unwrap(),
            "-1250201187231379.2"
        );
    }

    #[test]
    fn structured_json_is_sorted_ascii_indented_and_composite_wins() {
        let value = DataValue::Object(indexmap! {
            DataKey::Composite(vec!["z".into(), "row".into()]) => DataValue::Integer(BigInt::from(1)),
            DataKey::text("a") => DataValue::String("😀\0".into()),
            DataKey::Composite(vec!["same".into(), "key".into()]) => DataValue::String("composite".into()),
            DataKey::text("same|key") => DataValue::String("text".into()),
        });
        assert_eq!(
            String::from_utf8(render_structured(&value).unwrap()).unwrap(),
            "{\n    \"a\": \"\\ud83d\\ude00\\u0000\",\n    \"same|key\": \"composite\",\n    \"z|row\": 1\n}\n"
        );

        let array = DataValue::Array(vec![
            DataValue::Array(Vec::new()),
            DataValue::Float(1e-5),
            DataValue::Bool(false),
            DataValue::Null,
            DataValue::String("\"\\\u{8}\u{c}\n\r\t".into()),
        ]);
        assert_eq!(
            String::from_utf8(render_structured(&array).unwrap()).unwrap(),
            "[\n    [],\n    1e-05,\n    false,\n    null,\n    \"\\\"\\\\\\b\\f\\n\\r\\t\"\n]\n"
        );
    }

    #[test]
    fn var_json_filters_original_order_before_sorting() {
        let table = DataValue::Object(indexmap! {
            DataKey::Composite(vec!["z".into(), "needle".into()]) => DataValue::String("first".into()),
            DataKey::text("needle") => DataValue::String("second".into()),
            DataKey::Composite(vec!["a".into(), "needle".into()]) => DataValue::String("third".into()),
        });
        let root = indexmap! { DataKey::text("TABLE") => table };
        assert_eq!(
            String::from_utf8(render_var_json(&root, "TABLE", Some("needle")).unwrap()).unwrap(),
            "{\n    \"z|needle\": \"first\"\n}\n"
        );
        assert_eq!(render_var_json(&root, "ABSENT", None).unwrap(), b"");
        assert_eq!(
            render_var_json(&root, "TABLE", Some("missing")).unwrap(),
            b"{}\n"
        );

        let scalar = indexmap! { DataKey::text("TABLE") => DataValue::Null };
        let error = render_var_json(&scalar, "TABLE", Some("x")).unwrap_err();
        assert_eq!(error.exit_code(), 1);
    }
}
