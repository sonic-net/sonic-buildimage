#!/usr/bin/env python

# Copyright 2025 Nexthop Systems Inc. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

"""
Sanity tests for pddf/pd-plugin.json.j2.

The template is the only pd-plugin source for platforms that ship one; it is
rendered at boot by `nh_gen pd_plugin_json`, with its Jinja booleans supplied by
pddf/feature-flags.json. These tests catch the failure mode that render silently
swallows: a gate naming a flag that is not declared renders as undefined (falsy),
so the else-branch ships without any error.
"""

import json
import os

import jinja2
import jinja2.meta
import pytest

CWD = os.path.dirname(os.path.realpath(__file__))
BASE_PLATFORM_PDDF_PATH = "../../../../../device/nexthop/{}/pddf"

PLATFORMS_WITH_PD_PLUGIN_TEMPLATE = [
    "x86_64-nexthop_4210-r0021",
    "x86_64-nexthop_4210-r1032",
]


def platform_pddf_path(platform_variant):
    """Path to the pddf directory of the given platform variant."""
    return os.path.join(CWD, BASE_PLATFORM_PDDF_PATH.format(platform_variant))


def _declared_flag_names(pddf_path):
    feature_flags_path = os.path.join(pddf_path, "feature-flags.json")
    if not os.path.isfile(feature_flags_path):
        return set()
    with open(feature_flags_path) as f:
        return {flag["name"] for flag in json.load(f)}


def _render(template_path, variables):
    loader = jinja2.FileSystemLoader(os.path.dirname(os.path.abspath(template_path)))
    # nosemgrep: python.flask.security.xss.audit.direct-use-of-jinja2.direct-use-of-jinja2
    j2_env = jinja2.Environment(loader=loader, keep_trailing_newline=True)
    template = j2_env.get_template(os.path.basename(template_path))
    # nosemgrep: python.flask.security.xss.audit.direct-use-of-jinja2.direct-use-of-jinja2
    return template.render(variables)


@pytest.mark.parametrize("platform_variant", PLATFORMS_WITH_PD_PLUGIN_TEMPLATE)
class TestPdPluginTemplate:
    def test_variables_are_declared_feature_flags(self, platform_variant):
        pddf_path = platform_pddf_path(platform_variant)
        template_path = os.path.join(pddf_path, "pd-plugin.json.j2")
        with open(template_path) as f:
            source = f.read()
        referenced = jinja2.meta.find_undeclared_variables(
            jinja2.Environment().parse(source)
        )
        undeclared = referenced - _declared_flag_names(pddf_path)
        assert not undeclared, (
            f"{platform_variant}: pd-plugin.json.j2 references {sorted(undeclared)}, "
            f"which feature-flags.json does not declare; such a gate always renders "
            f"its else-branch"
        )

    def test_renders_valid_json_for_every_flag_value(self, platform_variant):
        pddf_path = platform_pddf_path(platform_variant)
        template_path = os.path.join(pddf_path, "pd-plugin.json.j2")
        for value in (False, True):
            variables = {name: value for name in _declared_flag_names(pddf_path)}
            try:
                json.loads(_render(template_path, variables))
            except (jinja2.TemplateError, json.JSONDecodeError) as e:
                pytest.fail(
                    f"{platform_variant}: pd-plugin.json.j2 with all flags {value}: {e}"
                )
