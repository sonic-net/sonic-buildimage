#!/usr/bin/env python3

# Copyright 2026 Nexthop Systems Inc. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

import json
import tempfile
import textwrap
from unittest.mock import patch

import pytest


@pytest.fixture
def gen_lib_module():
    """Loads the module before each test. This is to let conftest.py inject deps first."""
    from nexthop import gen_lib

    yield gen_lib


def _tmp_file(content):
    f = tempfile.NamedTemporaryFile(mode="w+t", delete=False)
    f.write(textwrap.dedent(content))
    f.close()
    return f.name


class TestRender:
    def test_substitutes_variables(self, gen_lib_module):
        template = _tmp_file("bus is {{asic_bus}}\n")
        assert gen_lib_module.render(template, {"asic_bus": "e5"}) == "bus is e5\n"

class TestWriteAtomically:
    def test_writes_the_text(self, gen_lib_module, tmp_path):
        target = tmp_path / "pcie.yaml"
        gen_lib_module.write_atomically("rendered\n", str(target))
        assert target.read_text() == "rendered\n"

    def test_leaves_no_temp_file_behind(self, gen_lib_module, tmp_path):
        target = tmp_path / "pcie.yaml"
        gen_lib_module.write_atomically("rendered\n", str(target))
        assert [p.name for p in tmp_path.iterdir()] == ["pcie.yaml"]

    def test_failure_cleans_up_and_leaves_the_target_alone(
        self, gen_lib_module, tmp_path
    ):
        target = tmp_path / "pcie.yaml"
        target.write_text("stale\n")
        with patch.object(gen_lib_module.os, "replace", side_effect=OSError("boom")):
            with pytest.raises(OSError):
                gen_lib_module.write_atomically("fresh\n", str(target))
        assert target.read_text() == "stale\n"
        assert [p.name for p in tmp_path.iterdir()] == ["pcie.yaml"]
