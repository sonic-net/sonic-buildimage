#!/usr/bin/env python

# Copyright 2025 Nexthop Systems Inc. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

import textwrap
import tempfile
from unittest.mock import MagicMock, patch

import pytest


@pytest.fixture(scope="function", autouse=True)
def pcie_lib_module():
    """Loads the module before each test. This is to let conftest.py inject deps first."""
    from nexthop import pcie_lib

    yield pcie_lib


def _tmp_file(content):
    f = tempfile.NamedTemporaryFile(mode="w+t", delete=False)
    f.write(textwrap.dedent(content))
    f.close()
    return f.name


class TestPcieLib:
    INPUT_PCIE_VARIABLES = textwrap.dedent(
        """
        - name: "cpu_card_fpga_bdf"
          lookup_command: "echo 0000:03:00.0"

        - name: "switchcard_fpga_bdf"
          lookup_command: "echo 0000:04:00.0"

        - name: "foo_var"
          lookup_command: "echo 'foo'"
        """
    )

    @pytest.fixture
    def tmp_pcie_variables_yml_file(self):
        with tempfile.NamedTemporaryFile(mode="w+t") as f:
            f.write(self.INPUT_PCIE_VARIABLES)
            f.flush()
            yield f.name

    def test_get_var_name_to_cmd_map(self, pcie_lib_module, tmp_pcie_variables_yml_file):
        result = pcie_lib_module.get_var_name_to_cmd_map(tmp_pcie_variables_yml_file)
        assert result == {
            "cpu_card_fpga_bdf": "echo 0000:03:00.0",
            "switchcard_fpga_bdf": "echo 0000:04:00.0",
            "foo_var": "echo 'foo'",
        }

    def test_get_pcie_variables(self, pcie_lib_module, tmp_pcie_variables_yml_file):
        result = pcie_lib_module.get_pcie_variables(tmp_pcie_variables_yml_file)
        assert result == {
            "cpu_card_fpga_bdf": "0000:03:00.0",
            "switchcard_fpga_bdf": "0000:04:00.0",
            "foo_var": "foo",
        }

    def test_get_pcie_variables_with_vars_to_get(
        self, pcie_lib_module, tmp_pcie_variables_yml_file
    ):
        result = pcie_lib_module.get_pcie_variables(
            tmp_pcie_variables_yml_file, vars_to_get={"cpu_card_fpga_bdf", "foo_var"}
        )
        assert result == {
            "cpu_card_fpga_bdf": "0000:03:00.0",
            "foo_var": "foo",
        }

    def test_get_pcie_variables_with_empty_vars_to_get(
        self, pcie_lib_module, tmp_pcie_variables_yml_file
    ):
        result = pcie_lib_module.get_pcie_variables(tmp_pcie_variables_yml_file, vars_to_get={})
        assert result == {}

    def test_get_cpu_card_fpga_bdf(self, pcie_lib_module, tmp_pcie_variables_yml_file):
        result = pcie_lib_module.get_cpu_card_fpga_bdf(tmp_pcie_variables_yml_file)
        assert result == "0000:03:00.0"

    def test_get_switchcard_fpga_bdf(self, pcie_lib_module, tmp_pcie_variables_yml_file):
        result = pcie_lib_module.get_switchcard_fpga_bdf(tmp_pcie_variables_yml_file)
        assert result == "0000:04:00.0"


@pytest.mark.parametrize(
    "input_yaml",
    [
        pytest.param(
            """
            - name: "cpu_card_fpga_bdf"
              lookup_command: "echo 0000:03:00.0"
            - name: "switchcard_fpga_bdf"
            """,
            id="missing_lookup_command",
        ),
        pytest.param(
            """
            - name: "cpu_card_fpga_bdf"
              lookup_command: "echo 0000:03:00.0"
            - lookup_command: "echo 0000:04:00.0"
            """,
            id="missing_name",
        ),
        pytest.param(
            """
            - name: "cpu_card_fpga_bdf"
              lookup_command: "echo 0000:03:00.0"
            - name: "cpu_card_fpga_bdf"
              lookup_command: "echo 0000:04:00.0"
            """,
            id="duplicate_name",
        ),
    ],
)
def test_get_var_name_to_cmd_map_raises_on_invalid_yaml(pcie_lib_module, input_yaml):
    """Verifies that improper YAML structures trigger a SystemExit."""
    yaml_content = textwrap.dedent(input_yaml)
    with tempfile.NamedTemporaryFile(mode="w+t") as f:
        f.write(yaml_content)
        f.flush()
        with pytest.raises(Exception):
            pcie_lib_module.get_var_name_to_cmd_map(f.name)


class TestDeviceTypeForVarName:
    @pytest.mark.parametrize(
        "name",
        [
            "asic_bus",
            "asic_0_bus",
            "asic_1_bus",
            "asic_10_bus",
        ],
    )
    def test_matches_asic_bus(self, pcie_lib_module, name):
        assert (
            pcie_lib_module.device_type_for_var_name(name)
            is pcie_lib_module.PcieDeviceType.ASIC
        )

    @pytest.mark.parametrize(
        "name",
        [
            "asic_0_device_id",
            "asic_1_device_id",
            "switchcard_fpga_0_bus",
            "cpu_card_fpga_bus",
            "nvme_bus",
            "amd_soc_group_0_bus",
            "asic",
            "",
        ],
    )
    def test_does_not_match_unrelated_vars(self, pcie_lib_module, name):
        assert pcie_lib_module.device_type_for_var_name(name) is None


class TestGetDeviceBuses:
    def _buses(self, pcie_lib_module, content):
        return pcie_lib_module._get_device_buses(
            _tmp_file(content), pcie_lib_module.PcieDeviceType.ASIC
        )

    def test_returns_only_asic_var_outputs(self, pcie_lib_module):
        buses = self._buses(
            pcie_lib_module,
            """
            - name: "asic_0_bus"
              lookup_command: "echo 'e5'"

            - name: "asic_1_bus"
              lookup_command: "echo 'c1'"

            - name: "nvme_bus"
              lookup_command: "echo '07'"

            - name: "cpu_card_fpga_bdf"
              lookup_command: "echo '0000:03:00.0'"
            """,
        )
        assert sorted(buses) == ["c1", "e5"]

    def test_missing_file_warns_and_returns_empty(self, pcie_lib_module):
        with patch.object(pcie_lib_module, "_warn") as warn:
            assert (
                pcie_lib_module._get_device_buses(
                    "/nonexistent/pcie-variables.yaml",
                    pcie_lib_module.PcieDeviceType.ASIC,
                )
                == []
            )
        assert warn.call_count == 1

    def test_failed_lookup_is_skipped_and_reported(self, pcie_lib_module):
        # An unpopulated slot can fail; the ASICs that resolve still come back.
        with patch.object(pcie_lib_module, "_warn") as warn:
            assert self._buses(
                pcie_lib_module,
                """
                - name: "asic_0_bus"
                  lookup_command: "exit 1"

                - name: "asic_1_bus"
                  lookup_command: "echo 'c2'"
                """,
            ) == ["c2"]
        assert warn.call_count == 1

    def test_unparseable_bus_is_skipped(self, pcie_lib_module):
        with patch.object(pcie_lib_module, "_warn") as warn:
            assert self._buses(
                pcie_lib_module,
                """
                - name: "asic_0_bus"
                  lookup_command: "echo 'zz'"

                - name: "asic_1_bus"
                  lookup_command: "echo 'c3'"
                """,
            ) == ["c3"]
        assert warn.call_count == 1

    def test_bus_zero_is_rejected(self, pcie_lib_module):
        with patch.object(pcie_lib_module, "_warn") as warn:
            assert self._buses(
                pcie_lib_module,
                """
                - name: "asic_0_bus"
                  lookup_command: "echo '00'"

                - name: "asic_1_bus"
                  lookup_command: "echo 'c4'"
                """,
            ) == ["c4"]
        assert warn.call_count == 1

class TestGetPcieDeviceBdfs:
    ASIC_VARIABLES = textwrap.dedent(
        """
        - name: "asic_0_bus"
          lookup_command: "echo 'e5'"

        - name: "asic_1_bus"
          lookup_command: "echo 'c1'"

        - name: "nvme_bus"
          lookup_command: "echo '07'"
        """
    )

    PCIE_YAML = textwrap.dedent(
        """
        - bus: '00'
          dev: '01'
          fn: '2'
          id: '14db'
          name: 'PCI bridge: upstream of the ASIC'
        - bus: 'e5'
          dev: '00'
          fn: '0'
          id: f914
          name: 'Ethernet controller: BCM78914 Switch ASIC'
        - bus: '07'
          dev: '00'
          fn: '0'
          id: 110b
          name: 'Non-Volatile memory controller: the NVMe'
        - bus: 'c1'
          dev: '00'
          fn: '0'
          id: f914
          name: 'Ethernet controller: BCM78914 Switch ASIC, second die'
        """
    )

    PCIE_YAML_TEMPLATE = textwrap.dedent(
        """
        - bus: '{{asic_0_bus}}'
          dev: '00'
          fn: '0'
          id: f914
          name: 'Ethernet controller: BCM78914 Switch ASIC'
        - bus: '{{nvme_bus}}'
          dev: '00'
          fn: '0'
          id: 110b
          name: 'Non-Volatile memory controller: the NVMe'
        - bus: '{{asic_1_bus}}'
          dev: '00'
          fn: '0'
          id: f914
          name: 'Ethernet controller: BCM78914 Switch ASIC, second die'
        """
    )
    PLACEHOLDER = '- description: "Generated at runtime by pcie.yaml.j2"\n'

    @pytest.fixture
    def platform_dir(self, pcie_lib_module, tmp_path):
        """Stands in for the device's platform directory."""
        (tmp_path / "pcie-variables.yaml").write_text(self.ASIC_VARIABLES)
        (tmp_path / "pcie.yaml.j2").write_text(self.PCIE_YAML_TEMPLATE)
        (tmp_path / "pcie.yaml").write_text(self.PCIE_YAML)
        with patch.object(
            pcie_lib_module.device_info,
            "get_path_to_platform_dir",
            return_value=str(tmp_path),
        ):
            yield tmp_path

    def test_returns_only_the_asic_rows(self, pcie_lib_module, platform_dir):
        # The NVMe resolves a bus too, and the ASIC sits behind a bridge row.
        assert pcie_lib_module.get_pcie_device_bdfs() == ["e5:00.0", "c1:00.0"]

    def test_device_and_function_come_from_pcie_yaml(
        self, pcie_lib_module, platform_dir
    ):
        # Not assumed to be 00.0: a BIOS update could move them.
        (platform_dir / "pcie.yaml").write_text(
            textwrap.dedent(
                """
                - bus: 'e5'
                  dev: '03'
                  fn: '2'
                  id: f914
                  name: 'Ethernet controller: ASIC somewhere else on its bus'
                - bus: '07'
                  dev: '00'
                  fn: '0'
                  id: 110b
                  name: 'Non-Volatile memory controller: the NVMe'
                """
            )
        )
        assert pcie_lib_module.get_pcie_device_bdfs() == ["e5:03.2"]

    def test_bdf_is_normalised_the_way_pcied_renders_it(
        self, pcie_lib_module, platform_dir
    ):
        (platform_dir / "pcie.yaml").write_text(
            textwrap.dedent(
                """
                - bus: 'e5'
                  dev: '0'
                  fn: '0'
                  id: f914
                  name: 'Ethernet controller: single-digit dev'
                - bus: '07'
                  dev: '00'
                  fn: '0'
                  id: 110b
                  name: 'Non-Volatile memory controller: the NVMe'
                """
            )
        )
        assert pcie_lib_module.get_pcie_device_bdfs() == ["e5:00.0"]

    def test_unresolvable_bus_returns_empty_without_warning(
        self, pcie_lib_module, platform_dir
    ):
        # A platform with no ASIC variable is not a missing ASIC.
        (platform_dir / "pcie-variables.yaml").write_text(
            textwrap.dedent(
                """
                - name: "nvme_bus"
                  lookup_command: "echo '07'"
                """
            )
        )
        with patch.object(pcie_lib_module, "_warn") as warn:
            assert pcie_lib_module.get_pcie_device_bdfs() == []
        warn.assert_not_called()

    def test_malformed_row_is_skipped(self, pcie_lib_module, platform_dir):
        (platform_dir / "pcie.yaml").write_text(
            textwrap.dedent(
                """
                - bus: 'e5'
                  dev: 'zz'
                  fn: '0'
                - bus: 'c1'
                  dev: '00'
                  fn: '0'
                """
            )
        )
        with patch.object(pcie_lib_module, "_warn"):
            assert pcie_lib_module.get_pcie_device_bdfs() == ["c1:00.0"]

class TestGeneratesPcieYamlWhenUngenerated:
    @pytest.fixture
    def platform_dir(self, pcie_lib_module, tmp_path):
        (tmp_path / "pcie-variables.yaml").write_text(
            TestGetPcieDeviceBdfs.ASIC_VARIABLES
        )
        (tmp_path / "pcie.yaml.j2").write_text(TestGetPcieDeviceBdfs.PCIE_YAML_TEMPLATE)
        with patch.object(
            pcie_lib_module.device_info,
            "get_path_to_platform_dir",
            return_value=str(tmp_path),
        ):
            yield tmp_path

    def test_generates_from_the_placeholder_a_fresh_image_ships(
        self, pcie_lib_module, platform_dir
    ):
        (platform_dir / "pcie.yaml").write_text(TestGetPcieDeviceBdfs.PLACEHOLDER)
        assert pcie_lib_module.get_pcie_device_bdfs() == ["e5:00.0", "c1:00.0"]
        assert "bus" in (platform_dir / "pcie.yaml").read_text()

    def test_leaves_a_generated_file_alone(self, pcie_lib_module, platform_dir):
        # A warm boot wants last boot's enumeration, not a fresh read.
        generated = TestGetPcieDeviceBdfs.PCIE_YAML
        (platform_dir / "pcie.yaml").write_text(generated)
        assert pcie_lib_module.get_pcie_device_bdfs() == ["e5:00.0", "c1:00.0"]
        assert (platform_dir / "pcie.yaml").read_text() == generated

    def test_an_unrenderable_template_warns_and_returns_empty(
        self, pcie_lib_module, platform_dir
    ):
        (platform_dir / "pcie.yaml").write_text(TestGetPcieDeviceBdfs.PLACEHOLDER)
        (platform_dir / "pcie.yaml.j2").unlink()
        with patch.object(pcie_lib_module, "_warn") as warn:
            assert pcie_lib_module.get_pcie_device_bdfs() == []
        assert warn.called


def _completed(returncode=0, stdout="", stderr=""):
    result = MagicMock()
    result.returncode = returncode
    result.stdout = stdout
    result.stderr = stderr
    return result


class TestSetpciRead:
    def test_returns_stripped_hex(self, pcie_lib_module):
        with patch.object(pcie_lib_module.subprocess, "run", autospec=True, return_value=_completed(stdout="0146\n")) as run:
            assert pcie_lib_module.setpci_read("01:00.0", "0x04.w") == "0146"
        run.assert_called_once_with(
            ["setpci", "-s", "01:00.0", "0x04.w"], capture_output=True, text=True, check=False
        )

    def test_nonzero_returncode_raises(self, pcie_lib_module):
        with patch.object(pcie_lib_module.subprocess, "run", autospec=True, return_value=_completed(returncode=1, stderr="boom")):
            with pytest.raises(RuntimeError):
                pcie_lib_module.setpci_read("01:00.0", "0x04.w")

    def test_empty_output_raises(self, pcie_lib_module):
        with patch.object(pcie_lib_module.subprocess, "run", autospec=True, return_value=_completed(stdout="  \n")):
            with pytest.raises(RuntimeError):
                pcie_lib_module.setpci_read("01:00.0", "0x04.w")

    def test_missing_binary_propagates(self, pcie_lib_module):
        with patch.object(pcie_lib_module.subprocess, "run", autospec=True, side_effect=FileNotFoundError):
            with pytest.raises(FileNotFoundError):
                pcie_lib_module.setpci_read("01:00.0", "0x04.w")


class TestSetpciWrite:
    def test_success_does_not_raise(self, pcie_lib_module):
        with patch.object(pcie_lib_module.subprocess, "run", autospec=True, return_value=_completed()) as run:
            pcie_lib_module.setpci_write("01:00.0", "0x04.w", "0546")
        run.assert_called_once_with(
            ["setpci", "-s", "01:00.0", "0x04.w=0546"], capture_output=True, text=True, check=False
        )

    def test_failure_raises(self, pcie_lib_module):
        with patch.object(pcie_lib_module.subprocess, "run", autospec=True, return_value=_completed(returncode=1, stderr="nope")):
            with pytest.raises(RuntimeError):
                pcie_lib_module.setpci_write("01:00.0", "0x04.w", "0546")


class TestApplyWordMask:
    def test_set_mask(self, pcie_lib_module):
        assert pcie_lib_module.apply_word_mask(0x0146, set_mask=0x0400) == 0x0546

    def test_clear_mask(self, pcie_lib_module):
        assert pcie_lib_module.apply_word_mask(0x8001, clear_mask=0x8000) == 0x0001

    def test_set_and_clear(self, pcie_lib_module):
        assert pcie_lib_module.apply_word_mask(0x8000, set_mask=0x0001, clear_mask=0x8000) == 0x0001

    def test_noop(self, pcie_lib_module):
        assert pcie_lib_module.apply_word_mask(0x1234) == 0x1234


class TestPciDevicePresent:
    def test_present(self, pcie_lib_module):
        out = "01:00.0 0200: 14e4:b900\n02:00.0 0c03: 1022:149c\n"
        with patch.object(pcie_lib_module.subprocess, "run", autospec=True, return_value=_completed(stdout=out)):
            assert pcie_lib_module.pci_device_present("01:00.0") is True

    def test_absent(self, pcie_lib_module):
        with patch.object(pcie_lib_module.subprocess, "run", autospec=True, return_value=_completed(stdout="02:00.0 ...\n")):
            assert pcie_lib_module.pci_device_present("01:00.0") is False

    def test_lspci_failure_raises(self, pcie_lib_module):
        with patch.object(pcie_lib_module.subprocess, "run", autospec=True, return_value=_completed(returncode=1, stderr="err")):
            with pytest.raises(RuntimeError):
                pcie_lib_module.pci_device_present("01:00.0")


def _config_reader(config):
    """Build a setpci_read side_effect from an {offset_with_width: hex} map."""
    def _read(bdf, offset_with_width):
        return config[offset_with_width]

    return _read


class TestFindPciCapability:
    # Capability list: 0x34 -> 0x40 (MSI, id 0x05) -> 0x50 (MSI-X, id 0x11) -> end.
    CAP_LIST = {
        "0x34.b": "40",
        "0x40.b": "05", "0x41.b": "50",
        "0x50.b": "11", "0x51.b": "00",
    }

    def test_finds_each_capability(self, pcie_lib_module):
        with patch.object(pcie_lib_module, "setpci_read", autospec=True, side_effect=_config_reader(self.CAP_LIST)):
            assert pcie_lib_module.find_pci_capability("01:00.0", pcie_lib_module.PCI_CAP_ID_MSI) == 0x40
            assert pcie_lib_module.find_pci_capability("01:00.0", pcie_lib_module.PCI_CAP_ID_MSIX) == 0x50

    def test_absent_capability_returns_none(self, pcie_lib_module):
        config = {"0x34.b": "40", "0x40.b": "05", "0x41.b": "00"}
        with patch.object(pcie_lib_module, "setpci_read", autospec=True, side_effect=_config_reader(config)):
            assert pcie_lib_module.find_pci_capability("01:00.0", pcie_lib_module.PCI_CAP_ID_MSIX) is None

    def test_no_capabilities_list_returns_none(self, pcie_lib_module):
        with patch.object(pcie_lib_module, "setpci_read", autospec=True, side_effect=_config_reader({"0x34.b": "00"})):
            assert pcie_lib_module.find_pci_capability("01:00.0", pcie_lib_module.PCI_CAP_ID_MSI) is None
