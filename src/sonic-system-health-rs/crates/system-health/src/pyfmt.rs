//! Python-compatible number parsing and formatting, so messages match the Python daemon.

/// Parses a string as Python's `float()`.
pub fn py_float(s: &str) -> Option<f64> {
    let s = s.trim();
    if s.is_empty() {
        return None;
    }
    // PEP 515: float() takes an underscore between two digits, and no other.
    let bytes = s.as_bytes();
    let between_digits =
        |i: usize| i > 0 && bytes[i - 1].is_ascii_digit() && bytes.get(i + 1).is_some_and(u8::is_ascii_digit);
    if (0..bytes.len()).any(|i| bytes[i] == b'_' && !between_digits(i)) {
        return None;
    }
    s.replace('_', "").parse::<f64>().ok()
}

/// Formats a float as Python's `repr()`/`str()`.
pub fn py_repr(value: f64) -> String {
    if value.is_nan() {
        return "nan".to_string();
    }
    if value.is_infinite() {
        return if value > 0.0 { "inf" } else { "-inf" }.to_string();
    }
    // `{:e}` gives the shortest round-trip digits, as Python's repr.
    let formatted = format!("{value:e}");
    let (mantissa, exponent) = formatted.split_once('e').expect("exponent format");
    let exponent: i32 = exponent.parse().expect("exponent");
    let (sign, mantissa) = match mantissa.strip_prefix('-') {
        Some(m) => ("-", m),
        None => ("", mantissa),
    };
    let digits: String = mantissa.chars().filter(|c| *c != '.').collect();

    if (-4..16).contains(&exponent) {
        let point = exponent + 1;
        let body = if point <= 0 {
            format!("0.{}{}", "0".repeat((-point) as usize), digits)
        } else if point as usize >= digits.len() {
            format!("{}{}.0", digits, "0".repeat(point as usize - digits.len()))
        } else {
            format!("{}.{}", &digits[..point as usize], &digits[point as usize..])
        };
        format!("{sign}{body}")
    } else {
        let mantissa = if digits.len() > 1 {
            format!("{}.{}", &digits[..1], &digits[1..])
        } else {
            digits
        };
        let exp_sign = if exponent < 0 { '-' } else { '+' };
        format!("{sign}{mantissa}e{exp_sign}{:02}", exponent.abs())
    }
}

/// Splits lines as Python's `str.splitlines()` for \n, \r\n and \r.
pub fn splitlines(s: &str) -> Vec<&str> {
    let mut lines = Vec::new();
    let mut start = 0;
    let bytes = s.as_bytes();
    let mut i = 0;
    while i < bytes.len() {
        match bytes[i] {
            b'\n' => {
                lines.push(&s[start..i]);
                start = i + 1;
            }
            b'\r' => {
                lines.push(&s[start..i]);
                if bytes.get(i + 1) == Some(&b'\n') {
                    i += 1;
                }
                start = i + 1;
            }
            _ => {}
        }
        i += 1;
    }
    if start < s.len() {
        lines.push(&s[start..]);
    }
    lines
}

/// Slices by character positions with Python's clamping (`s[start:end]`).
pub fn char_slice(s: &str, start: usize, end: Option<usize>) -> String {
    let end = end.unwrap_or(usize::MAX);
    if start >= end {
        return String::new();
    }
    s.chars().skip(start).take(end - start).collect()
}

/// Character position of `needle`, as Python's `str.find()`.
pub fn char_find(s: &str, needle: &str) -> Option<usize> {
    s.find(needle).map(|byte_pos| s[..byte_pos].chars().count())
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn repr_matches_python() {
        let cases = [
            (85.0, "85.0"),
            (85.5, "85.5"),
            (-3.0, "-3.0"),
            (0.0, "0.0"),
            (-0.0, "-0.0"),
            (100.0, "100.0"),
            (0.1, "0.1"),
            (0.0001, "0.0001"),
            (0.00001, "1e-05"),
            (1.5e-7, "1.5e-07"),
            (1e16, "1e+16"),
            (123456789012345678.0, "1.2345678901234568e+17"),
            (1234567890123456.0, "1234567890123456.0"),
            (12.345, "12.345"),
            (f64::INFINITY, "inf"),
            (f64::NEG_INFINITY, "-inf"),
            (f64::NAN, "nan"),
        ];
        for (value, expected) in cases {
            assert_eq!(py_repr(value), expected, "{value}");
        }
    }

    #[test]
    fn float_parses_as_python() {
        assert_eq!(py_float(" 85 "), Some(85.0));
        assert_eq!(py_float("1e3"), Some(1000.0));
        assert_eq!(py_float("inf"), Some(f64::INFINITY));
        assert!(py_float("nan").unwrap().is_nan());
        assert_eq!(py_float("N/A"), None);
        assert_eq!(py_float(""), None);
        // PEP 515: an underscore between two digits, as float() takes it.
        assert_eq!(py_float("1_0"), Some(10.0));
        assert_eq!(py_float("1_000.2_5"), Some(1000.25));
        assert_eq!(py_float("1e1_0"), Some(1e10));
        for rejected in ["_1", "1_", "1__0", "1_.5", "1._5", "1_e5", "1e_5"] {
            assert_eq!(py_float(rejected), None, "{rejected}");
        }
    }

    #[test]
    fn splitlines_matches_python() {
        assert_eq!(splitlines("a\nb\r\nc\rd"), vec!["a", "b", "c", "d"]);
        assert_eq!(splitlines("a\n"), vec!["a"]);
        assert_eq!(splitlines("a\n "), vec!["a", " "]);
        assert!(splitlines("").is_empty());
    }

    #[test]
    fn char_slice_clamps() {
        assert_eq!(char_slice("abc", 1, Some(10)), "bc");
        assert_eq!(char_slice("abc", 5, None), "");
        assert_eq!(char_slice("abc", 2, Some(1)), "");
    }
}
