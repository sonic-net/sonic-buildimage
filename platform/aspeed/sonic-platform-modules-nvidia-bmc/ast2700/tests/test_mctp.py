#
# SPDX-FileCopyrightText: NVIDIA CORPORATION & AFFILIATES
# Copyright (c) 2026 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
# SPDX-License-Identifier: Apache-2.0
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

import json
import subprocess
from unittest import mock

import pytest

from sonic_platform import mctp
from sonic_platform.mctp import MctpError


def _completed(stdout):
    return subprocess.CompletedProcess(args=["busctl"], returncode=0, stdout=stdout, stderr="")


# busctl --json=short renders a method reply as a JSON array of its return
# values; SetupEndpoint returns (y eid, i net, s path, b new).
SETUP_REPLY = json.dumps([8, 1, "/au/com/codeconstruct/mctp1/networks/1/endpoints/8", False])

# GetManagedObjects returns a single a{oa{sa{sv}}} value: path -> iface -> prop.
def _managed_objects(*endpoints, irot_net=None):
    objs = {}
    for net, eid, types in endpoints:
        path = f"/au/com/codeconstruct/mctp1/networks/{net}/endpoints/{eid}"
        objs[path] = {
            mctp.ENDPOINT_IFACE: {
                "SupportedMessageTypes": list(types),
                "NetworkId": net,
                "EID": eid,
            }
        }
    if irot_net is None:
        irot_net = endpoints[0][0] if endpoints else 1
    objs[f"{mctp.MCTP_BASE_PATH}/interfaces/{mctp.IROT_INTERFACE}"] = {
        mctp.INTERFACE_IFACE: {"NetworkId": irot_net}
    }
    return json.dumps([objs])


class TestSetupEndpoint:

    def test_returns_net_eid(self):
        with mock.patch("sonic_platform.mctp.subprocess.run", return_value=_completed(SETUP_REPLY)) as run:
            net, eid = mctp.setup_endpoint("mctpirot0")
        assert (net, eid) == (1, 8)
        # Invoked the bus-owner SetupEndpoint with an empty hwaddr (ay 0).
        called = run.call_args[0][0]
        assert "SetupEndpoint" in called and "ay" in called and "0" in called
        assert "/au/com/codeconstruct/mctp1/interfaces/mctpirot0" in called

    def test_short_reply_raises(self):
        with mock.patch("sonic_platform.mctp.subprocess.run", return_value=_completed(json.dumps([8, 1]))):
            with pytest.raises(MctpError):
                mctp.setup_endpoint()

    def test_busctl_failure_raises(self):
        err = subprocess.CalledProcessError(1, ["busctl"], stderr="boom")
        with mock.patch("sonic_platform.mctp.subprocess.run", side_effect=err):
            with pytest.raises(MctpError):
                mctp.setup_endpoint()

    def test_busctl_missing_raises(self):
        with mock.patch("sonic_platform.mctp.subprocess.run", side_effect=FileNotFoundError()):
            with pytest.raises(MctpError):
                mctp.setup_endpoint()

    def test_busctl_permission_error_raises_mctp_error(self):
        with mock.patch(
            "sonic_platform.mctp.subprocess.run",
            side_effect=PermissionError("Permission denied"),
        ):
            with pytest.raises(MctpError, match="busctl execution failed"):
                mctp.setup_endpoint()

    def test_bad_json_raises(self):
        with mock.patch("sonic_platform.mctp.subprocess.run", return_value=_completed("not json")):
            with pytest.raises(MctpError):
                mctp.setup_endpoint()

    def test_non_integer_reply_raises_mctp_error(self):
        bad_reply = json.dumps([8, "not-a-net", "/path", False])
        with mock.patch("sonic_platform.mctp.subprocess.run", return_value=_completed(bad_reply)):
            with pytest.raises(MctpError, match="invalid SetupEndpoint reply"):
                mctp.setup_endpoint()


class TestFindBmcEndpoint:

    def test_filters_to_pldm_capable_endpoint(self):
        objs = json.loads(_managed_objects((1, 9, [0]), (1, 8, [0, 1])))[0]
        assert mctp.find_bmc_endpoint(objs) == (1, 8)

    def test_network_filter(self):
        objs = json.loads(_managed_objects((1, 8, [0, 1]), (2, 20, [0, 1])))[0]
        assert mctp.find_bmc_endpoint(objs, network=2) == (2, 20)

    def test_no_match_raises(self):
        objs = json.loads(_managed_objects((1, 9, [0])))[0]
        with pytest.raises(MctpError):
            mctp.find_bmc_endpoint(objs)

    def test_ignores_non_endpoint_objects(self):
        objs = {
            "/au/com/codeconstruct/mctp1/interfaces/mctpirot0": {
                "au.com.codeconstruct.MCTP.Interface1": {"NetworkId": 1}
            }
        }
        with pytest.raises(MctpError):
            mctp.find_bmc_endpoint(objs)

    def test_malformed_props_skipped_then_no_match_raises(self):
        # A malformed endpoint must be skipped rather than abort the scan;
        # with no other endpoints, the final "no match" error is raised.
        objs = {
            "/au/com/codeconstruct/mctp1/endpoints/legacy": {
                mctp.ENDPOINT_IFACE: {
                    "SupportedMessageTypes": [0, 1],
                    "NetworkId": "not-a-net",
                    "EID": 8,
                }
            }
        }
        with pytest.raises(MctpError, match="no PLDM-capable BMC endpoint"):
            mctp.find_bmc_endpoint(objs)

    def test_malformed_endpoint_skipped_then_valid_returned(self):
        # A malformed endpoint is skipped and scanning continues to a valid one.
        objs = {
            "/au/com/codeconstruct/mctp1/endpoints/legacy": {
                mctp.ENDPOINT_IFACE: {
                    "SupportedMessageTypes": [0, 1],
                    "NetworkId": "not-a-net",
                    "EID": 8,
                }
            }
        }
        objs.update(json.loads(_managed_objects((1, 8, [0, 1])))[0])
        assert mctp.find_bmc_endpoint(objs) == (1, 8)

    def test_missing_props_fallback_returns_none_and_skips(self):
        objs = {
            "/au/com/codeconstruct/mctp1/endpoints/legacy": {
                mctp.ENDPOINT_IFACE: {
                    "SupportedMessageTypes": [0, 1],
                    "NetworkId": 1,
                }
            }
        }
        with pytest.raises(MctpError):
            mctp.find_bmc_endpoint(objs)


class TestGetBmcEid:

    def test_queries_object_manager(self):
        reply = _managed_objects((1, 8, [0, 1]))
        with mock.patch("sonic_platform.mctp.subprocess.run", return_value=_completed(reply)) as run:
            assert mctp.get_bmc_eid() == (1, 8)
        called = run.call_args[0][0]
        assert "GetManagedObjects" in called

    def test_mctp_address_string(self):
        reply = _managed_objects((1, 8, [0, 1]))
        with mock.patch("sonic_platform.mctp.subprocess.run", return_value=_completed(reply)):
            assert mctp.mctp_address() == "1,8"

    def test_defaults_to_irot_network(self):
        # Prefer the IRoT network even when another PLDM endpoint exists first.
        reply = _managed_objects((2, 20, [0, 1]), (1, 8, [0, 1]), irot_net=1)
        with mock.patch("sonic_platform.mctp.subprocess.run", return_value=_completed(reply)):
            assert mctp.get_bmc_eid() == (1, 8)
            assert mctp.mctp_address() == "1,8"

    def test_get_interface_network(self):
        reply = _managed_objects((1, 8, [0, 1]), irot_net=1)
        with mock.patch("sonic_platform.mctp.subprocess.run", return_value=_completed(reply)):
            assert mctp.get_interface_network() == 1


# Newer busctl/systemd wrap the whole reply, and every D-Bus VARIANT inside an
# a{sv} map, in a {"type": <sig>, "data": <value>} envelope.
def _enveloped_managed_objects(*endpoints, irot_net=None):
    objs = {}
    for net, eid, types in endpoints:
        path = f"/au/com/codeconstruct/mctp1/networks/{net}/endpoints/{eid}"
        objs[path] = {
            mctp.ENDPOINT_IFACE: {
                "SupportedMessageTypes": {"type": "ay", "data": list(types)},
                "NetworkId": {"type": "u", "data": net},
                "EID": {"type": "y", "data": eid},
            }
        }
    if irot_net is None:
        irot_net = endpoints[0][0] if endpoints else 1
    objs[f"{mctp.MCTP_BASE_PATH}/interfaces/{mctp.IROT_INTERFACE}"] = {
        mctp.INTERFACE_IFACE: {
            "NetworkId": {"type": "u", "data": irot_net},
        }
    }
    return json.dumps({"type": "a{oa{sa{sv}}}", "data": [objs]})


class TestBusctlEnvelopes:

    def test_unwrap_strips_variant_envelopes(self):
        assert mctp._unwrap({"type": "ay", "data": [1, 5, 126]}) == [1, 5, 126]
        assert mctp._unwrap({"type": "u", "data": 1}) == 1
        nested = {"role": {"type": "s", "data": "v"}, "eids": [{"type": "y", "data": 8}]}
        assert mctp._unwrap(nested) == {"role": "v", "eids": [8]}

    def test_unwrap_preserves_non_envelope_dicts(self):
        # A two-key dict that is not a variant envelope (no "data"/non-str type)
        # is left intact.
        assert mctp._unwrap({"NetworkId": 1, "EID": 8}) == {"NetworkId": 1, "EID": 8}

    def test_mctp_address_with_enveloped_reply(self):
        reply = _enveloped_managed_objects((1, 230, [0]), (1, 8, [1, 5, 126]))
        with mock.patch("sonic_platform.mctp.subprocess.run", return_value=_completed(reply)):
            assert mctp.mctp_address() == "1,8"


class TestInterfaceUp:

    def test_interface_is_up_when_operstate_up(self):
        with mock.patch.object(mctp, "interface_operstate", return_value="up"):
            assert mctp.interface_is_up("mctpirot0") is True

    def test_interface_is_up_when_operstate_down(self):
        with mock.patch.object(mctp, "interface_operstate", return_value="down"):
            assert mctp.interface_is_up("mctpirot0") is False

    def test_interface_is_up_when_operstate_unknown_with_carrier(self):
        with mock.patch.object(mctp, "interface_operstate", return_value="unknown"), \
             mock.patch.object(mctp, "_interface_carrier", return_value="1"):
            assert mctp.interface_is_up("mctpirot0") is True

    def test_interface_is_up_when_operstate_unknown_without_carrier(self):
        with mock.patch.object(mctp, "interface_operstate", return_value="unknown"), \
             mock.patch.object(mctp, "_interface_carrier", return_value="0"):
            assert mctp.interface_is_up("mctpirot0") is False

    def test_wait_for_interface_up_succeeds(self):
        states = iter(["down", "down", "up"])

        def operstate(_iface):
            return next(states)

        with mock.patch.object(mctp, "interface_operstate", side_effect=operstate):
            mctp.wait_for_interface_up("mctpirot0", timeout=10, poll_interval=0, sleep=lambda _s: None)

    def test_wait_for_interface_up_times_out(self):
        with mock.patch.object(mctp, "interface_operstate", return_value="down"):
            with pytest.raises(MctpError, match="did not come up"):
                mctp.wait_for_interface_up(
                    "mctpirot0", timeout=0, poll_interval=0, sleep=lambda _s: None
                )

    def test_wait_for_interface_up_succeeds_when_operstate_unknown_with_carrier(self):
        with mock.patch.object(mctp, "interface_operstate", return_value="unknown"), \
             mock.patch.object(mctp, "_interface_carrier", return_value="1"):
            mctp.wait_for_interface_up(
                "mctpirot0", timeout=1, poll_interval=0, sleep=lambda _s: None
            )


class TestMain:

    def test_success_returns_zero(self):
        with mock.patch("sonic_platform.mctp.wait_for_interface_up"), \
             mock.patch("sonic_platform.mctp.subprocess.run", return_value=_completed(SETUP_REPLY)):
            assert mctp.main(["setup", "mctpirot0"], sleep=lambda _s: None) == 0

    def test_recovers_after_transient_failures_and_link_wait_timeout(self):
        unknown_operstate_timeout = MctpError(
            "MCTP interface mctpirot0 did not come up within 120s (operstate=unknown)"
        )
        with mock.patch(
            "sonic_platform.mctp.wait_for_interface_up",
            side_effect=unknown_operstate_timeout,
        ), mock.patch(
            "sonic_platform.mctp.subprocess.run",
            side_effect=[FileNotFoundError(), FileNotFoundError(), _completed(SETUP_REPLY)],
        ) as run:
            assert mctp.main(["setup"], attempts=3, delay=0, sleep=lambda _s: None) == 0
        assert run.call_count == 3

    def test_proceeds_with_setup_when_link_wait_times_out(self):
        with mock.patch(
            "sonic_platform.mctp.wait_for_interface_up",
            side_effect=MctpError("operstate=unknown"),
        ), mock.patch(
            "sonic_platform.mctp.subprocess.run",
            return_value=_completed(SETUP_REPLY),
        ) as run:
            assert mctp.main(["setup"], sleep=lambda _s: None) == 0
        assert run.call_count == 1

    def test_failure_returns_one_after_retries(self):
        with mock.patch("sonic_platform.mctp.wait_for_interface_up"), \
             mock.patch("sonic_platform.mctp.subprocess.run", side_effect=FileNotFoundError()) as run:
            assert mctp.main(["setup"], attempts=3, delay=0, sleep=lambda _s: None) == 1
        assert run.call_count == 3

    def test_retries_then_succeeds(self):
        results = [FileNotFoundError(), _completed(SETUP_REPLY)]

        def side_effect(*_a, **_k):
            item = results.pop(0)
            if isinstance(item, Exception):
                raise item
            return item

        with mock.patch("sonic_platform.mctp.wait_for_interface_up"), \
             mock.patch("sonic_platform.mctp.subprocess.run", side_effect=side_effect):
            assert mctp.main(["setup"], attempts=3, delay=0, sleep=lambda _s: None) == 0

    def test_retries_after_permission_error_then_succeeds(self):
        results = [PermissionError("Permission denied"), _completed(SETUP_REPLY)]

        def side_effect(*_a, **_k):
            item = results.pop(0)
            if isinstance(item, Exception):
                raise item
            return item

        with mock.patch("sonic_platform.mctp.wait_for_interface_up"), \
             mock.patch("sonic_platform.mctp.subprocess.run", side_effect=side_effect) as run:
            assert mctp.main(["setup"], attempts=3, delay=0, sleep=lambda _s: None) == 0
        assert run.call_count == 2
