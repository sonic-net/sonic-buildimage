use num_bigint::BigUint;
use num_traits::ToPrimitive;

use crate::{
    error::{AppError, Result},
    value::{DataKey, DataValue},
};

#[derive(Clone, Debug, Eq, PartialEq)]
pub enum Selector {
    Member(String),
    Index(BigUint),
    CompositeKey(Vec<String>),
}

#[derive(Clone, Debug, Eq, PartialEq)]
pub struct SelectorPath {
    selectors: Vec<Selector>,
}

#[derive(Clone, Copy, Debug, PartialEq)]
pub enum Evaluation<'a> {
    Value(&'a DataValue),
    MissingFinal,
    MissingIntermediate(usize),
}

impl SelectorPath {
    pub fn parse(input: &str) -> Result<Self> {
        Parser::new(input).parse()
    }

    pub fn selectors(&self) -> &[Selector] {
        &self.selectors
    }

    pub fn evaluate<'a>(&self, root: &'a DataValue) -> Evaluation<'a> {
        let mut current = root;
        for (position, selector) in self.selectors.iter().enumerate() {
            let selected = match (current, selector) {
                (DataValue::Object(object), Selector::Member(member)) => {
                    object.get(&DataKey::Text(member.clone()))
                }
                (DataValue::Object(object), Selector::CompositeKey(components)) => {
                    object.get(&DataKey::Composite(components.clone()))
                }
                (DataValue::Array(array), Selector::Index(index)) => {
                    let length = BigUint::from(array.len());
                    if index >= &length {
                        None
                    } else {
                        index.to_usize().and_then(|index| array.get(index))
                    }
                }
                _ => None,
            };
            match selected {
                Some(value) => current = value,
                None if position + 1 == self.selectors.len() => {
                    return Evaluation::MissingFinal;
                }
                None => return Evaluation::MissingIntermediate(position),
            }
        }
        Evaluation::Value(current)
    }
}

struct Parser<'a> {
    input: &'a str,
    position: usize,
}

impl<'a> Parser<'a> {
    fn new(input: &'a str) -> Self {
        Self { input, position: 0 }
    }

    fn parse(mut self) -> Result<SelectorPath> {
        let mut selectors = vec![Selector::Member(self.identifier()?)];
        while let Some(character) = self.peek() {
            match character {
                '.' => {
                    self.advance();
                    selectors.push(Selector::Member(self.identifier()?));
                }
                '[' => selectors.push(self.bracket_selector()?),
                _ => return self.error("expected '.' or '['"),
            }
        }
        Ok(SelectorPath { selectors })
    }

    fn identifier(&mut self) -> Result<String> {
        let start = self.position;
        let Some(first) = self.peek() else {
            return self.error("expected identifier");
        };
        if first != '_' && !first.is_ascii_alphabetic() {
            return self.error("expected identifier");
        }
        self.advance();
        while self
            .peek()
            .is_some_and(|character| character == '_' || character.is_ascii_alphanumeric())
        {
            self.advance();
        }
        Ok(self.input[start..self.position].to_owned())
    }

    fn bracket_selector(&mut self) -> Result<Selector> {
        self.expect('[')?;
        self.whitespace();
        let parenthesized = self.consume('(');
        if parenthesized {
            self.whitespace();
        }

        let selector = match self.peek() {
            Some('\'' | '"') => {
                let first = self.quoted()?;
                self.whitespace();
                if self.consume(',') {
                    let mut components = vec![first];
                    loop {
                        self.whitespace();
                        if !matches!(self.peek(), Some('\'' | '"')) {
                            return self.error("expected quoted tuple component");
                        }
                        components.push(self.quoted()?);
                        self.whitespace();
                        if !self.consume(',') {
                            break;
                        }
                    }
                    Selector::CompositeKey(components)
                } else if parenthesized {
                    return self.error("tuple selector requires at least two components");
                } else {
                    Selector::Member(first)
                }
            }
            Some(character) if character.is_ascii_digit() && !parenthesized => {
                let start = self.position;
                while self.peek().is_some_and(|value| value.is_ascii_digit()) {
                    self.advance();
                }
                let index = BigUint::parse_bytes(&self.input.as_bytes()[start..self.position], 10)
                    .ok_or_else(|| self.usage("invalid unsigned index"))?;
                Selector::Index(index)
            }
            _ => return self.error("expected quoted key, tuple key, or unsigned index"),
        };

        self.whitespace();
        if parenthesized {
            self.expect(')')?;
            self.whitespace();
        }
        self.expect(']')?;
        Ok(selector)
    }

    fn quoted(&mut self) -> Result<String> {
        let quote = self
            .advance()
            .filter(|value| matches!(value, '\'' | '"'))
            .ok_or_else(|| self.usage("expected quoted string"))?;
        let mut output = String::new();
        loop {
            let Some(character) = self.advance() else {
                return self.error("unterminated quoted string");
            };
            if character == quote {
                return Ok(output);
            }
            if character != '\\' {
                output.push(character);
                continue;
            }

            let Some(escape) = self.advance() else {
                return self.error("unterminated escape");
            };
            match escape {
                '\\' => output.push('\\'),
                '\'' if quote == '\'' => output.push('\''),
                '"' if quote == '"' => output.push('"'),
                'n' => output.push('\n'),
                'r' => output.push('\r'),
                't' => output.push('\t'),
                'u' => output.push(self.unicode_escape()?),
                _ => return self.error("unsupported escape"),
            }
        }
    }

    fn unicode_escape(&mut self) -> Result<char> {
        let first = self.hex_quad()?;
        if (0xD800..=0xDBFF).contains(&first) {
            if !self.consume('\\') || !self.consume('u') {
                return self.error("high surrogate requires a low surrogate");
            }
            let second = self.hex_quad()?;
            if !(0xDC00..=0xDFFF).contains(&second) {
                return self.error("invalid low surrogate");
            }
            let codepoint = 0x10000 + ((first - 0xD800) << 10) + (second - 0xDC00);
            return char::from_u32(codepoint).ok_or_else(|| self.usage("invalid Unicode escape"));
        }
        if (0xDC00..=0xDFFF).contains(&first) {
            return self.error("unexpected low surrogate");
        }
        char::from_u32(first).ok_or_else(|| self.usage("invalid Unicode escape"))
    }

    fn hex_quad(&mut self) -> Result<u32> {
        let start = self.position;
        let mut value = 0_u32;
        for _ in 0..4 {
            let Some(character) = self.advance() else {
                self.position = self.input.len();
                return self.error("incomplete Unicode escape");
            };
            let Some(digit) = character.to_digit(16) else {
                self.position = start;
                return self.error("invalid Unicode escape");
            };
            value = value * 16 + digit;
        }
        Ok(value)
    }

    fn whitespace(&mut self) {
        while matches!(self.peek(), Some(' ' | '\t' | '\r' | '\n')) {
            self.advance();
        }
    }

    fn expect(&mut self, expected: char) -> Result<()> {
        if self.consume(expected) {
            Ok(())
        } else {
            self.error(&format!("expected '{expected}'"))
        }
    }

    fn consume(&mut self, expected: char) -> bool {
        if self.peek() == Some(expected) {
            self.advance();
            true
        } else {
            false
        }
    }

    fn peek(&self) -> Option<char> {
        self.input[self.position..].chars().next()
    }

    fn advance(&mut self) -> Option<char> {
        let character = self.peek()?;
        self.position += character.len_utf8();
        Some(character)
    }

    fn usage(&self, message: &str) -> AppError {
        AppError::usage(format!(
            "invalid selector at byte {}: {message}",
            self.position
        ))
    }

    fn error<T>(&self, message: &str) -> Result<T> {
        Err(self.usage(message))
    }
}

#[cfg(test)]
mod tests {
    use indexmap::indexmap;
    use num_bigint::BigInt;

    use super::*;

    fn parse(input: &str) -> SelectorPath {
        SelectorPath::parse(input).unwrap()
    }

    #[test]
    fn parses_every_supported_selector_form_and_escape() {
        assert_eq!(
            parse("VLAN_MEMBER[ ( 'Vlan\\u0031', \"Ethernet\\t0\" ) ].tagging_mode").selectors(),
            [
                Selector::Member("VLAN_MEMBER".into()),
                Selector::CompositeKey(vec!["Vlan1".into(), "Ethernet\t0".into()]),
                Selector::Member("tagging_mode".into()),
            ]
        );
        assert_eq!(
            parse("A['a','b']").selectors(),
            [
                Selector::Member("A".into()),
                Selector::CompositeKey(vec!["a".into(), "b".into()]),
            ]
        );
        assert_eq!(
            parse("A[18446744073709551616000000000000000000]").selectors()[1],
            Selector::Index(
                BigUint::parse_bytes(b"18446744073709551616000000000000000000", 10).unwrap()
            )
        );
        assert_eq!(
            parse("A['\\uD83D\\uDE00']").selectors()[1],
            Selector::Member("😀".into())
        );
    }

    #[test]
    fn rejects_jinja_and_malformed_grammar_with_usage_status() {
        for invalid in [
            "",
            "A()",
            "A|int",
            "A is none",
            "A+1",
            "A[1:2]",
            "A[*]",
            "A[-1]",
            "A['a',]",
            "A[('a')]",
            "A['a'] trailing",
            "A..b",
            "A[\"\\q\"]",
            "A['\\uD800']",
            "A['\\uDC00']",
            "A['a' 'b']",
        ] {
            let error = SelectorPath::parse(invalid).unwrap_err();
            assert_eq!(error.exit_code(), 2, "{invalid}");
            assert!(error.to_string().contains("byte"), "{invalid}: {error}");
        }
    }

    #[test]
    fn evaluates_typed_keys_indices_and_all_missing_classes() {
        let root = DataValue::Object(indexmap! {
            DataKey::text("A") => DataValue::Object(indexmap! {
                DataKey::text("a|b") => DataValue::String("text".into()),
                DataKey::Composite(vec!["a".into(), "b".into()]) => DataValue::Array(vec![
                    DataValue::Integer(BigInt::from(7)),
                ]),
            }),
            DataKey::text("scalar") => DataValue::Bool(true),
        });

        assert_eq!(
            parse("A['a|b']").evaluate(&root),
            Evaluation::Value(&DataValue::String("text".into()))
        );
        assert_eq!(
            parse("A[('a','b')][0]").evaluate(&root),
            Evaluation::Value(&DataValue::Integer(BigInt::from(7)))
        );
        assert_eq!(
            parse("A[('a','b')][184467440737095516160000000000]").evaluate(&root),
            Evaluation::MissingFinal
        );
        assert_eq!(
            parse("A[('a','b')][184467440737095516160000000000].x").evaluate(&root),
            Evaluation::MissingIntermediate(2)
        );
        assert_eq!(parse("missing").evaluate(&root), Evaluation::MissingFinal);
        assert_eq!(
            parse("missing.x").evaluate(&root),
            Evaluation::MissingIntermediate(0)
        );
        assert_eq!(parse("scalar.x").evaluate(&root), Evaluation::MissingFinal);
        assert_eq!(
            parse("scalar.x.y").evaluate(&root),
            Evaluation::MissingIntermediate(1)
        );
    }
}
