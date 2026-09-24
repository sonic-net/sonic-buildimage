import importlib.util
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
import unittest


MODULE_PATH = Path(__file__).parents[1] / "efi_sbatlevel.py"
SPEC = importlib.util.spec_from_file_location(
    "efi_sbatlevel", MODULE_PATH
)
efi_sbatlevel = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(efi_sbatlevel)


def create_pe(timestamp, sbatlevel_version=0):
    pe_offset = 0x40
    optional_header_size = 0
    section_header_offset = (
        pe_offset
        + len(efi_sbatlevel.PE_SIGNATURE)
        + efi_sbatlevel.COFF_HEADER_SIZE
    )
    raw_offset = section_header_offset + efi_sbatlevel.SECTION_HEADER_SIZE
    section = (
        struct.pack("<III", sbatlevel_version, 8, 55)
        + f"sbat,1,{timestamp}\nshim,4\n".encode()
        + b"\0sbat,1,2099010100\nshim,99\n\0"
    )
    string_table = b".sbatlevel\0"
    symbol_table_offset = raw_offset + len(section)
    data = bytearray(
        symbol_table_offset + 4 + len(string_table)
    )
    data[:2] = efi_sbatlevel.DOS_SIGNATURE
    struct.pack_into(
        "<I", data, efi_sbatlevel.DOS_PE_HEADER_OFFSET, pe_offset
    )
    data[pe_offset:pe_offset + 4] = efi_sbatlevel.PE_SIGNATURE
    coff_offset = pe_offset + len(efi_sbatlevel.PE_SIGNATURE)
    struct.pack_into(
        "<H",
        data,
        coff_offset + efi_sbatlevel.COFF_SECTION_COUNT_OFFSET,
        1,
    )
    struct.pack_into(
        "<H",
        data,
        coff_offset + efi_sbatlevel.COFF_OPTIONAL_HEADER_SIZE_OFFSET,
        optional_header_size,
    )
    struct.pack_into(
        "<II",
        data,
        coff_offset + efi_sbatlevel.COFF_SYMBOL_TABLE_OFFSET,
        symbol_table_offset,
        0,
    )
    data[
        section_header_offset:section_header_offset + 8
    ] = b"/4\0\0\0\0\0\0"
    struct.pack_into(
        "<II",
        data,
        section_header_offset
        + efi_sbatlevel.SECTION_RAW_DATA_FIELDS_OFFSET,
        len(section),
        raw_offset,
    )
    data[raw_offset:raw_offset + len(section)] = section
    struct.pack_into(
        "<I", data, symbol_table_offset, 4 + len(string_table)
    )
    data[symbol_table_offset + 4:] = string_table
    return data


class EfiSbatLevelTest(unittest.TestCase):
    def write_shim(self, directory, name, timestamp):
        path = Path(directory) / name
        path.write_bytes(create_pe(timestamp))
        return path

    def test_uses_first_automatic_timestamp(self):
        with tempfile.TemporaryDirectory() as directory:
            path = self.write_shim(
                directory, "shimx64.efi", "2024010900"
            )

            _, value = efi_sbatlevel.get_automatic_timestamp(path)

        self.assertEqual(value, "2024010900")

    def test_rejects_unsupported_sbatlevel_version(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "shimx64.efi"
            path.write_bytes(create_pe("2024010900", sbatlevel_version=1))

            with self.assertRaisesRegex(
                ValueError, "unsupported .sbatlevel version: 1"
            ):
                efi_sbatlevel.get_automatic_timestamp(path)

    def test_upgrade(self):
        with tempfile.TemporaryDirectory() as directory:
            installed = self.write_shim(
                directory, "installed.efi", "2024010900"
            )
            incoming = self.write_shim(
                directory, "incoming.efi", "2024040900"
            )

            result = efi_sbatlevel.compare_shims(
                installed, incoming
            )

        self.assertEqual(result, efi_sbatlevel.EXIT_SUCCESS)

    def test_sequence_suffix_is_not_treated_as_time(self):
        with tempfile.TemporaryDirectory() as directory:
            installed = self.write_shim(
                directory, "installed.efi", "2023012900"
            )
            incoming = self.write_shim(
                directory, "incoming.efi", "2023012950"
            )

            result = efi_sbatlevel.compare_shims(
                installed, incoming
            )

        self.assertEqual(result, efi_sbatlevel.EXIT_SUCCESS)

    def test_downgrade(self):
        with tempfile.TemporaryDirectory() as directory:
            installed = self.write_shim(
                directory, "installed.efi", "2024040900"
            )
            incoming = self.write_shim(
                directory, "incoming.efi", "2024010900"
            )

            result = efi_sbatlevel.compare_shims(
                installed, incoming
            )

        self.assertEqual(result, efi_sbatlevel.EXIT_DOWNGRADE)

    def test_rejects_invalid_pe(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "invalid.efi"
            path.write_bytes(b"not a PE image")

            with self.assertRaisesRegex(
                ValueError, "missing DOS header"
            ):
                efi_sbatlevel.get_automatic_timestamp(path)

    def test_invalid_installed_shim_has_distinct_exit_status(self):
        with tempfile.TemporaryDirectory() as directory:
            installed = Path(directory) / "installed.efi"
            installed.write_bytes(b"not a PE image")
            incoming = self.write_shim(
                directory, "incoming.efi", "2024040900"
            )

            result = subprocess.run(
                [
                    sys.executable,
                    str(MODULE_PATH),
                    str(installed),
                    str(incoming),
                ],
                check=False,
                capture_output=True,
                text=True,
            )

        self.assertEqual(
            result.returncode,
            efi_sbatlevel.EXIT_INSTALLED_INVALID,
        )

    def test_invalid_incoming_shim_fails_comparison(self):
        with tempfile.TemporaryDirectory() as directory:
            installed = self.write_shim(
                directory, "installed.efi", "2024040900"
            )
            incoming = Path(directory) / "incoming.efi"
            incoming.write_bytes(b"not a PE image")

            result = subprocess.run(
                [
                    sys.executable,
                    str(MODULE_PATH),
                    str(installed),
                    str(incoming),
                ],
                check=False,
                capture_output=True,
                text=True,
            )

        self.assertEqual(result.returncode, efi_sbatlevel.EXIT_ERROR)


if __name__ == "__main__":
    unittest.main()
