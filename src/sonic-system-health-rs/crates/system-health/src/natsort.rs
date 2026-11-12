//! Natural sort order of STATE_DB keys, as in the Python checker.

pub fn natsorted(mut items: Vec<String>) -> Vec<String> {
    items.sort_by(|a, b| natord::compare(a, b));
    items
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn sorts_numbers_by_value() {
        let sorted = natsorted(
            [
                "FAN_INFO|fan10",
                "FAN_INFO|fan2",
                "FAN_INFO|fan1",
                "PSU_INFO|PSU 11",
                "PSU_INFO|PSU 3",
                "PSU_INFO|PDB 1",
            ]
            .map(String::from)
            .to_vec(),
        );
        assert_eq!(
            sorted,
            [
                "FAN_INFO|fan1",
                "FAN_INFO|fan2",
                "FAN_INFO|fan10",
                "PSU_INFO|PDB 1",
                "PSU_INFO|PSU 3",
                "PSU_INFO|PSU 11"
            ]
        );
        let sorted = natsorted(["ASIC1", "ASIC", "ASIC10", "ASIC0"].map(String::from).to_vec());
        assert_eq!(sorted, ["ASIC", "ASIC0", "ASIC1", "ASIC10"]);
    }
}
