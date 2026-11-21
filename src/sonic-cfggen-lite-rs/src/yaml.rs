use std::collections::HashMap;

use indexmap::IndexMap;
use num_bigint::BigInt;
use num_traits::Num;
use yaml_rust2::{
    parser::{Event, EventReceiver, Parser, Tag},
    scanner::TScalarStyle,
};

use crate::{
    error::{AppError, Result},
    value::{DataKey, DataValue},
};

const MAX_NESTING: usize = 256;

struct EventSink {
    events: Vec<Event>,
}

impl EventReceiver for EventSink {
    fn on_event(&mut self, event: Event) {
        self.events.push(event);
    }
}

pub fn parse(input: &str) -> Result<DataValue> {
    let mut sink = EventSink { events: Vec::new() };
    Parser::new_from_str(input)
        .load(&mut sink, true)
        .map_err(|error| AppError::operational(format!("invalid YAML: {error}")))?;
    EventTree::new(&sink.events).document()
}

struct EventTree<'a> {
    events: &'a [Event],
    position: usize,
    anchors: HashMap<usize, DataValue>,
}

impl<'a> EventTree<'a> {
    fn new(events: &'a [Event]) -> Self {
        Self {
            events,
            position: 0,
            anchors: HashMap::new(),
        }
    }

    fn document(mut self) -> Result<DataValue> {
        self.expect_control(Event::StreamStart)?;
        self.expect_control(Event::DocumentStart)?;
        let value = self.node(0)?;
        self.expect_control(Event::DocumentEnd)?;
        match self.peek() {
            Some(Event::StreamEnd) => {
                self.position += 1;
                if self.position == self.events.len() {
                    Ok(value)
                } else {
                    Err(AppError::operational("invalid YAML event stream"))
                }
            }
            Some(Event::DocumentStart) => Err(AppError::operational(
                "YAML input must contain exactly one document",
            )),
            _ => Err(AppError::operational("invalid YAML document boundary")),
        }
    }

    fn node(&mut self, depth: usize) -> Result<DataValue> {
        if depth > MAX_NESTING {
            return Err(AppError::operational(format!(
                "YAML nesting exceeds {MAX_NESTING} levels"
            )));
        }
        let event = self
            .next()
            .cloned()
            .ok_or_else(|| AppError::operational("unexpected end of YAML input"))?;
        match event {
            Event::Scalar(value, style, anchor, tag) => {
                let node = scalar(value, style, tag.as_ref())?;
                self.remember(anchor, &node);
                Ok(node)
            }
            Event::SequenceStart(anchor, tag) => {
                require_container_tag(tag.as_ref(), "seq")?;
                let mut values = Vec::new();
                while !matches!(self.peek(), Some(Event::SequenceEnd)) {
                    values.push(self.node(depth + 1)?);
                }
                self.position += 1;
                let node = DataValue::Array(values);
                self.remember(anchor, &node);
                Ok(node)
            }
            Event::MappingStart(anchor, tag) => {
                require_container_tag(tag.as_ref(), "map")?;
                let mut values = IndexMap::new();
                while !matches!(self.peek(), Some(Event::MappingEnd)) {
                    let key = match self.node(depth + 1)? {
                        DataValue::String(key) => key,
                        _ => {
                            return Err(AppError::operational("YAML mapping keys must be strings"))
                        }
                    };
                    if key == "<<" {
                        return Err(AppError::operational("YAML merge keys are not supported"));
                    }
                    let key = DataKey::Text(key);
                    if values.contains_key(&key) {
                        return Err(AppError::operational("duplicate YAML mapping key"));
                    }
                    values.insert(key, self.node(depth + 1)?);
                }
                self.position += 1;
                let node = DataValue::Object(values);
                self.remember(anchor, &node);
                Ok(node)
            }
            Event::Alias(anchor) => self.anchors.get(&anchor).cloned().ok_or_else(|| {
                AppError::operational("cyclic or forward YAML alias is not supported")
            }),
            _ => Err(AppError::operational("unexpected YAML parser event")),
        }
    }

    fn peek(&self) -> Option<&Event> {
        self.events.get(self.position)
    }

    fn next(&mut self) -> Option<&Event> {
        let event = self.events.get(self.position);
        self.position += usize::from(event.is_some());
        event
    }

    fn expect_control(&mut self, expected: Event) -> Result<()> {
        if self.next() == Some(&expected) {
            Ok(())
        } else {
            Err(AppError::operational("invalid YAML event stream"))
        }
    }

    fn remember(&mut self, anchor: usize, value: &DataValue) {
        if anchor != 0 {
            self.anchors.insert(anchor, value.clone());
        }
    }
}

fn scalar(value: String, style: TScalarStyle, tag: Option<&Tag>) -> Result<DataValue> {
    if let Some(kind) = scalar_tag(tag)? {
        return explicit_scalar(value, kind);
    }
    if style != TScalarStyle::Plain {
        return Ok(DataValue::String(value));
    }
    if is_null(&value) {
        return Ok(DataValue::Null);
    }
    if let Some(value) = parse_bool(&value) {
        return Ok(DataValue::Bool(value));
    }
    if let Some(value) = parse_integer(&value) {
        return value.map(DataValue::Integer);
    }
    if let Some(value) = parse_float(&value) {
        return value.map(DataValue::Float);
    }
    if resembles_timestamp(&value) {
        return Err(AppError::operational("YAML timestamps are not supported"));
    }
    Ok(DataValue::String(value))
}

fn scalar_tag(tag: Option<&Tag>) -> Result<Option<&str>> {
    let Some(tag) = tag else {
        return Ok(None);
    };
    if tag.handle == "!" && tag.suffix.is_empty() {
        return Ok(Some("str"));
    }
    if tag.handle == "!!" || tag.handle == "tag:yaml.org,2002:" {
        return Ok(Some(tag.suffix.as_str()));
    }
    Err(AppError::operational("custom YAML tags are not supported"))
}

fn require_container_tag(tag: Option<&Tag>, expected: &str) -> Result<()> {
    match scalar_tag(tag)? {
        None => Ok(()),
        Some(actual) if actual == expected => Ok(()),
        Some(_) => Err(AppError::operational("unsupported YAML container tag")),
    }
}

fn explicit_scalar(value: String, kind: &str) -> Result<DataValue> {
    match kind {
        "str" => Ok(DataValue::String(value)),
        "null" if is_null(&value) => Ok(DataValue::Null),
        "bool" => parse_bool(&value)
            .map(DataValue::Bool)
            .ok_or_else(|| AppError::operational("invalid explicitly tagged YAML boolean")),
        "int" => parse_integer(&value)
            .ok_or_else(|| AppError::operational("invalid explicitly tagged YAML integer"))?
            .map(DataValue::Integer),
        "float" => parse_float(&value)
            .ok_or_else(|| AppError::operational("invalid explicitly tagged YAML float"))?
            .map(DataValue::Float),
        "null" => Err(AppError::operational("invalid explicitly tagged YAML null")),
        _ => Err(AppError::operational("unsupported YAML scalar tag")),
    }
}

fn is_null(value: &str) -> bool {
    value.is_empty() || matches!(value, "~" | "null" | "Null" | "NULL")
}

fn parse_bool(value: &str) -> Option<bool> {
    match value {
        "yes" | "Yes" | "YES" | "true" | "True" | "TRUE" | "on" | "On" | "ON" => Some(true),
        "no" | "No" | "NO" | "false" | "False" | "FALSE" | "off" | "Off" | "OFF" => Some(false),
        _ => None,
    }
}

fn parse_integer(value: &str) -> Option<Result<BigInt>> {
    let normalized = value.replace('_', "");
    if normalized.contains(':') {
        if normalized.contains('.') {
            return None;
        }
        if !has_numeric_sexagesimal_head(&normalized) {
            return None;
        }
        return Some(parse_sexagesimal_integer(&normalized));
    }
    let (negative, unsigned) = strip_sign(&normalized);
    let parsed = if let Some(digits) = unsigned.strip_prefix("0b").filter(|digits| {
        !digits.is_empty() && digits.chars().all(|digit| matches!(digit, '0' | '1'))
    }) {
        BigInt::from_str_radix(digits, 2)
    } else if let Some(digits) = unsigned.strip_prefix("0x").filter(|digits| {
        !digits.is_empty() && digits.chars().all(|digit| digit.is_ascii_hexdigit())
    }) {
        BigInt::from_str_radix(digits, 16)
    } else if unsigned.len() > 1
        && unsigned.starts_with('0')
        && unsigned[1..]
            .chars()
            .all(|digit| matches!(digit, '0'..='7'))
    {
        BigInt::from_str_radix(&unsigned[1..], 8)
    } else if unsigned.len() > 1 && unsigned.starts_with('0') && !unsigned.contains('.') {
        return None;
    } else if unsigned.chars().all(|character| character.is_ascii_digit()) && !unsigned.is_empty() {
        BigInt::from_str_radix(unsigned, 10)
    } else {
        return None;
    };
    Some(
        parsed
            .map(|integer| if negative { -integer } else { integer })
            .map_err(|_| AppError::operational("invalid YAML integer")),
    )
}

fn parse_sexagesimal_integer(value: &str) -> Result<BigInt> {
    let (negative, unsigned) = strip_sign(value);
    let mut components = unsigned.split(':');
    let first = components.next().unwrap_or_default();
    if first.is_empty() {
        return Err(AppError::operational("invalid YAML base-60 integer"));
    }
    let mut result = BigInt::from_str_radix(first, 10)
        .map_err(|_| AppError::operational("invalid YAML base-60 integer"))?;
    for component in components {
        if component.is_empty() {
            return Err(AppError::operational("invalid YAML base-60 integer"));
        }
        let number = component
            .parse::<u8>()
            .map_err(|_| AppError::operational("invalid YAML base-60 integer"))?;
        if number >= 60 {
            return Err(AppError::operational("invalid YAML base-60 integer"));
        }
        result = result * 60u8 + number;
    }
    Ok(if negative { -result } else { result })
}

fn parse_float(value: &str) -> Option<Result<f64>> {
    let normalized = value.replace('_', "");
    let lowercase = normalized.to_ascii_lowercase();
    if matches!(lowercase.as_str(), ".inf" | "+.inf" | "-.inf" | ".nan") {
        return Some(Err(AppError::operational(
            "non-finite YAML floats are not supported",
        )));
    }
    let decimal_float = is_decimal_float(&normalized);
    let looks_float = decimal_float || normalized.contains(':');
    if !looks_float {
        return None;
    }
    if normalized.contains(':') {
        if !has_numeric_sexagesimal_head(&normalized) {
            return None;
        }
        return Some(parse_sexagesimal_float(&normalized));
    }
    Some(
        normalized
            .parse::<f64>()
            .map_err(|_| AppError::operational("invalid YAML float"))
            .and_then(|number| {
                if number.is_finite() {
                    Ok(number)
                } else {
                    Err(AppError::operational(
                        "YAML float is outside finite binary64 range",
                    ))
                }
            }),
    )
}

fn parse_sexagesimal_float(value: &str) -> Result<f64> {
    let (negative, unsigned) = strip_sign(value);
    let mut result = 0.0;
    for (index, component) in unsigned.split(':').enumerate() {
        let number = component
            .parse::<f64>()
            .map_err(|_| AppError::operational("invalid YAML base-60 float"))?;
        if (index > 0 && !(0.0..60.0).contains(&number)) || !number.is_finite() {
            return Err(AppError::operational("invalid YAML base-60 float"));
        }
        result = result * 60.0 + number;
    }
    Ok(if negative { -result } else { result })
}

fn is_decimal_float(value: &str) -> bool {
    let (_, unsigned) = strip_sign(value);
    let exponent_index = unsigned
        .char_indices()
        .find_map(|(index, character)| matches!(character, 'e' | 'E').then_some(index));
    let (mantissa, has_exponent) = if let Some(index) = exponent_index {
        let (mantissa, exponent_with_marker) = unsigned.split_at(index);
        let exponent = &exponent_with_marker[1..];
        if exponent
            .chars()
            .any(|character| matches!(character, 'e' | 'E'))
            || !exponent.strip_prefix(['+', '-']).is_some_and(|digits| {
                !digits.is_empty() && digits.bytes().all(|byte| byte.is_ascii_digit())
            })
        {
            return false;
        }
        (mantissa, true)
    } else {
        (unsigned, false)
    };

    if let Some((integer, fraction)) = mantissa.split_once('.') {
        !mantissa[integer.len() + 1..].contains('.')
            && (!integer.is_empty() || !fraction.is_empty())
            && integer.bytes().all(|byte| byte.is_ascii_digit())
            && fraction.bytes().all(|byte| byte.is_ascii_digit())
    } else {
        has_exponent && !mantissa.is_empty() && mantissa.bytes().all(|byte| byte.is_ascii_digit())
    }
}

fn strip_sign(value: &str) -> (bool, &str) {
    if let Some(unsigned) = value.strip_prefix('-') {
        (true, unsigned)
    } else if let Some(unsigned) = value.strip_prefix('+') {
        (false, unsigned)
    } else {
        (false, value)
    }
}

fn has_numeric_sexagesimal_head(value: &str) -> bool {
    let (_, unsigned) = strip_sign(value);
    unsigned
        .split_once(':')
        .is_some_and(|(head, _)| !head.is_empty() && head.bytes().all(|byte| byte.is_ascii_digit()))
}

fn resembles_timestamp(value: &str) -> bool {
    let bytes = value.as_bytes();
    bytes.len() >= 10
        && bytes[..4].iter().all(u8::is_ascii_digit)
        && bytes[4] == b'-'
        && bytes[5..7].iter().all(u8::is_ascii_digit)
        && bytes[7] == b'-'
        && bytes[8..10].iter().all(u8::is_ascii_digit)
        && (bytes.len() == 10 || matches!(bytes[10], b'T' | b't' | b' '))
}

#[cfg(test)]
mod tests {
    use super::*;

    fn object(input: &str) -> IndexMap<DataKey, DataValue> {
        parse(input).unwrap().into_object("YAML").unwrap()
    }

    #[test]
    fn resolves_full_loader_scalar_domain() {
        let parsed = object(
            "null_value: ~\nbool_value: YES\nint_value: -0x10\nbig_value: 123456789012345678901234567890\nfloat_value: -0.0\ntext_value: 'yes'\n",
        );
        assert_eq!(parsed[&DataKey::text("null_value")], DataValue::Null);
        assert_eq!(parsed[&DataKey::text("bool_value")], DataValue::Bool(true));
        assert_eq!(
            parsed[&DataKey::text("int_value")],
            DataValue::Integer(BigInt::from(-16))
        );
        assert!(matches!(
            parsed[&DataKey::text("big_value")],
            DataValue::Integer(_)
        ));
        assert_eq!(
            parsed[&DataKey::text("float_value")],
            DataValue::Float(-0.0)
        );
        assert_eq!(
            parsed[&DataKey::text("text_value")],
            DataValue::String("yes".into())
        );
    }

    #[test]
    fn resolves_explicit_standard_tags_and_sexagesimal_numbers() {
        let parsed = object(
            "string_value: !!str yes\n\
             bool_value: !!bool FALSE\n\
             null_value: !!null null\n\
             int_value: !!int 0b101\n\
             octal_value: 0755\n\
             base_60_int: 1:02\n\
             float_value: !!float 1.5\n\
             base_60_float: 1:02.5\n\
             sequence: !!seq [one, two]\n\
             mapping: !!map {field: value}\n",
        );
        assert_eq!(
            parsed[&DataKey::text("string_value")],
            DataValue::String("yes".into())
        );
        assert_eq!(parsed[&DataKey::text("bool_value")], DataValue::Bool(false));
        assert_eq!(parsed[&DataKey::text("null_value")], DataValue::Null);
        assert_eq!(
            parsed[&DataKey::text("int_value")],
            DataValue::Integer(BigInt::from(5))
        );
        assert_eq!(
            parsed[&DataKey::text("octal_value")],
            DataValue::Integer(BigInt::from(493))
        );
        assert_eq!(
            parsed[&DataKey::text("base_60_int")],
            DataValue::Integer(BigInt::from(62))
        );
        assert_eq!(parsed[&DataKey::text("float_value")], DataValue::Float(1.5));
        assert_eq!(
            parsed[&DataKey::text("base_60_float")],
            DataValue::Float(62.5)
        );
        assert!(matches!(
            parsed[&DataKey::text("sequence")],
            DataValue::Array(_)
        ));
        assert!(matches!(
            parsed[&DataKey::text("mapping")],
            DataValue::Object(_)
        ));
    }

    #[test]
    fn resolves_arbitrary_precision_sexagesimal_integer_head() {
        let parsed = object(
            "real_world_shape: 20260820:19\n\
             u8_boundary: 255:59\n\
             above_u8_boundary: 256:59\n\
             arbitrary_head: 123456789012345678901234567890:59\n",
        );

        for (key, head, tail) in [
            ("real_world_shape", "20260820", 19u8),
            ("u8_boundary", "255", 59u8),
            ("above_u8_boundary", "256", 59u8),
            ("arbitrary_head", "123456789012345678901234567890", 59u8),
        ] {
            let expected = BigInt::from_str_radix(head, 10).unwrap() * 60u8 + tail;
            assert_eq!(
                parsed[&DataKey::text(key)],
                DataValue::Integer(expected),
                "unexpected value for {key}"
            );
        }
    }

    #[test]
    fn preserves_plain_colon_scalars_with_non_numeric_heads_as_strings() {
        let parsed = object(
            "real_world_shape: build:marker\n\
             decimal_tail: build:1.5\n",
        );

        assert_eq!(
            parsed[&DataKey::text("real_world_shape")],
            DataValue::String("build:marker".into())
        );
        assert_eq!(
            parsed[&DataKey::text("decimal_tail")],
            DataValue::String("build:1.5".into())
        );
    }

    #[test]
    fn matches_full_loader_decimal_float_lexical_recognition() {
        let parsed = object(
            "version_shape: 13.6.0\n\
             multiple_dots: 1.2.3\n\
             unsigned_exponent: 1e2\n\
             decimal_unsigned_exponent: 1.0e2\n\
             exponent: 1.0e+2\n\
             trailing_dot: 1.\n\
             leading_dot: .5\n",
        );

        for (key, expected) in [
            ("version_shape", "13.6.0"),
            ("multiple_dots", "1.2.3"),
            ("unsigned_exponent", "1e2"),
            ("decimal_unsigned_exponent", "1.0e2"),
        ] {
            assert_eq!(
                parsed[&DataKey::text(key)],
                DataValue::String(expected.into())
            );
        }
        assert_eq!(parsed[&DataKey::text("exponent")], DataValue::Float(100.0));
        assert_eq!(
            parsed[&DataKey::text("trailing_dot")],
            DataValue::Float(1.0)
        );
        assert_eq!(parsed[&DataKey::text("leading_dot")], DataValue::Float(0.5));
    }

    #[test]
    fn rejects_invalid_sexagesimal_integer_tail_components() {
        for input in [
            "value: 256:60\n",
            "value: 256:256\n",
            "value: 256:not-a-number\n",
            "value: 256::01\n",
        ] {
            let error = parse(input).unwrap_err();
            assert_eq!(error.to_string(), "invalid YAML base-60 integer");
        }
    }

    #[test]
    fn expands_acyclic_aliases_as_value_copies() {
        let parsed = object("base: &base {field: value}\ncopy: *base\n");
        assert_eq!(
            parsed[&DataKey::text("base")],
            parsed[&DataKey::text("copy")]
        );
    }

    #[test]
    fn rejects_unsupported_yaml_constructs() {
        for input in [
            "a: 2024-01-02\n",
            "a: !!binary SGVsbG8=\n",
            "a: !custom value\n",
            "a: .inf\n",
            "a: &a [*a]\n",
            "a: 1\na: 2\n",
            "? [a, b]\n: value\n",
            "base: &base {a: 1}\nresult: {<<: *base}\n",
            "--- {}\n--- {}\n",
            "a: !!bool maybe\n",
            "a: !!int nope\n",
            "a: !!float nope\n",
            "a: !!null nope\n",
            "a: !!seq scalar\n",
            "a: !!map []\n",
            "a: !!seq {}\n",
            "a: 1:99\n",
            "a: 1:99.0\n",
            "a: 1.0e+9999\n",
            "a: !!float 1.2.3\n",
        ] {
            assert!(parse(input).is_err(), "accepted {input:?}");
        }
    }

    #[test]
    fn rejects_excessive_nesting_and_malformed_event_boundaries() {
        let mut nested_events = vec![Event::StreamStart, Event::DocumentStart];
        nested_events
            .extend(std::iter::repeat_with(|| Event::SequenceStart(0, None)).take(MAX_NESTING + 1));
        nested_events.push(Event::Scalar("null".into(), TScalarStyle::Plain, 0, None));
        nested_events.extend(std::iter::repeat_with(|| Event::SequenceEnd).take(MAX_NESTING + 1));
        nested_events.extend([Event::DocumentEnd, Event::StreamEnd]);
        assert!(EventTree::new(&nested_events)
            .document()
            .unwrap_err()
            .to_string()
            .contains("nesting exceeds"));

        assert!(EventTree::new(&[]).document().is_err());
        assert!(EventTree::new(&[Event::StreamStart, Event::DocumentStart])
            .document()
            .is_err());
        assert!(EventTree::new(&[
            Event::StreamStart,
            Event::DocumentStart,
            Event::Alias(99),
            Event::DocumentEnd,
            Event::StreamEnd,
        ])
        .document()
        .is_err());
        assert!(EventTree::new(&[
            Event::StreamStart,
            Event::DocumentStart,
            Event::Nothing,
            Event::DocumentEnd,
            Event::StreamEnd,
        ])
        .document()
        .is_err());
    }
}
