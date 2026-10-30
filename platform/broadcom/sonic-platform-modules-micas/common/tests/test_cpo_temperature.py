"""CPO temperature polling through independent OE and ELS endpoints."""
import importlib.util
import os
from pathlib import Path
import sys
import types
from unittest.mock import Mock

import pytest


ROOT = Path(os.environ.get("CPO_DRAFT_REPO", Path(__file__).resolve().parents[5]))
PLUGIN = ROOT / "device/micas/x86_64-micas_m2-w6940-128x1-fr4-r0/plugins/sfputil.py"
METHODS = [("get_highest_temperature_cpo_oe", "oe"),
           ("get_highest_temperature_cpo_rlm", "elsfp")]


@pytest.fixture
def plugin(monkeypatch):
    chassis = Mock(spec=["get_all_cpos"])
    chassis.get_all_cpos.return_value = []
    get_chassis = Mock(return_value=chassis)
    platform = types.ModuleType("sonic_platform")
    platform.platform = types.SimpleNamespace(
        Platform=Mock(return_value=types.SimpleNamespace(get_chassis=get_chassis)))
    sfp_package = types.ModuleType("sonic_sfp")
    sfp_base = types.ModuleType("sonic_sfp.sfputilbase")
    sfp_base.SfpUtilBase = object
    for name, module in [("sonic_platform", platform), ("sonic_sfp", sfp_package),
                         ("sonic_sfp.sfputilbase", sfp_base)]:
        monkeypatch.setitem(sys.modules, name, module)
    spec = importlib.util.spec_from_file_location("cpo_temperature_under_test", PLUGIN)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    monkeypatch.setattr(module.syslog, "openlog", Mock())
    monkeypatch.setattr(module, "write_syslog", Mock())
    return module.SfpUtil(), chassis, get_chassis


def make_endpoint(name, temperature):
    api = Mock(spec=["get_module_temperature"])
    api.get_module_temperature.return_value = temperature
    endpoint = Mock(spec=["get_name", "get_api", "get_presence"])
    endpoint.get_name.return_value = name
    endpoint.get_api.return_value = api
    endpoint.get_presence.return_value = True
    return endpoint


@pytest.mark.parametrize("method,target", METHODS)
def test_reads_correct_endpoint_and_deduplicates_nonadjacent_resources(plugin, method, target):
    utility, chassis, _ = plugin
    first = make_endpoint("{}0".format(target), 35.25)
    second = make_endpoint("{}1".format(target), 48.5)
    chassis.get_all_cpos.return_value = [
        types.SimpleNamespace(**{target: endpoint}) for endpoint in (first, second, first)]
    assert getattr(utility, method)() == 48.5
    chassis.get_all_cpos.assert_called_once_with()
    for endpoint in (first, second):
        endpoint.get_api.assert_called_once_with()
        endpoint.get_api.return_value.get_module_temperature.assert_called_once_with()


@pytest.mark.parametrize("method,target", METHODS)
@pytest.mark.parametrize("failure", [
    "api-error", "read-error", "no-api", "none", "text", "bool", "nan", "infinite", "too-hot", "too-cold",
])
def test_bad_resource_does_not_hide_later_valid_temperature(plugin, method, target, failure):
    utility, chassis, _ = plugin
    first = make_endpoint("{}0".format(target), 35.0)
    second = make_endpoint("{}1".format(target), 46.75)
    if failure == "api-error":
        first.get_api.side_effect = OSError("test API failure")
    elif failure == "read-error":
        first.get_api.return_value.get_module_temperature.side_effect = OSError("test read failure")
    elif failure == "no-api":
        first.get_api.return_value = None
    else:
        values = {"none": None, "text": "N/A", "bool": True, "nan": float("nan"),
                  "infinite": float("inf"), "too-hot": 201, "too-cold": -51}
        first.get_api.return_value.get_module_temperature.return_value = values[failure]
    chassis.get_all_cpos.return_value = [types.SimpleNamespace(**{target: first}),
                                         types.SimpleNamespace(**{target: second})]
    assert getattr(utility, method)() == 46.75


@pytest.mark.parametrize("method,target", METHODS)
def test_empty_topology_and_missing_chassis_keep_distinct_sentinels(plugin, method, target):
    utility, _, get_chassis = plugin
    assert getattr(utility, method)() == utility.TEMP_INVALID
    get_chassis.return_value = None
    assert getattr(utility, method)() == utility.TEMP_ERROR


@pytest.mark.parametrize("method,target", METHODS)
def test_all_reads_failing_return_error(plugin, method, target):
    utility, chassis, _ = plugin
    endpoint = make_endpoint("{}0".format(target), None)
    chassis.get_all_cpos.return_value = [types.SimpleNamespace(**{target: endpoint})]
    assert getattr(utility, method)() == utility.TEMP_ERROR


@pytest.mark.parametrize("method,target", METHODS)
@pytest.mark.parametrize("temperature", [-50, -12.25, 200])
def test_bounds_and_negative_temperatures_are_valid_celsius(plugin, method, target, temperature):
    utility, chassis, _ = plugin
    endpoint = make_endpoint("{}0".format(target), temperature)
    chassis.get_all_cpos.return_value = [types.SimpleNamespace(**{target: endpoint})]
    assert getattr(utility, method)() == temperature


def test_absent_els_does_not_access_its_api(plugin):
    utility, chassis, _ = plugin
    els = make_endpoint("ELS0", 25.0)
    els.get_presence.return_value = False
    chassis.get_all_cpos.return_value = [types.SimpleNamespace(elsfp=els)]
    assert utility.get_highest_temperature_cpo_rlm() == utility.TEMP_INVALID
    els.get_api.assert_not_called()
