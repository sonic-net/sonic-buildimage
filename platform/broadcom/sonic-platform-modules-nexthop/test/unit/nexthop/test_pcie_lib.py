#!/usr/bin/env python

# Copyright 2025 Nexthop Systems Inc. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

import textwrap
import tempfile
import pytest


@pytest.fixture(scope="function", autouse=True)
def pcie_lib_module():
    """Loads the module before each test. This is to let conftest.py inject deps first."""
    from nexthop import pcie_lib

    yield pcie_lib


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
