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

"""
MCTP helpers for the NVIDIA AST2700 BMC.

The BMC firmware device (FD) is reachable over the Aspeed IRoT MCTP link
(``mctpirot0``) once ``mctpd`` (the CodeConstruct MCTP control daemon) is
running. ``mctpd`` does not enumerate or assign an EID to the IRoT peer on its
own; a bus-owner method must be invoked once. This module provides:

* :func:`setup_endpoint` - run *once* at boot (via the ``mctp-bmc-setup``
  systemd oneshot) to assign/learn the BMC's EID. ``SetupEndpoint`` is
  idempotent: it queries for an existing EID first and only assigns when none
  is present, so re-running it (e.g. after an ``mctpd`` restart) does not churn
  the EID.
* :func:`get_bmc_eid` - query the EID later (e.g. from the firmware
  ``Component``) by walking ``mctpd``'s D-Bus object tree, without re-running a
  bus-owner assignment.

All D-Bus access goes through ``busctl --json=short`` so we avoid a hard
dependency on a python D-Bus binding.
"""

import json
import re
import subprocess
import sys
import time
from pathlib import Path

from sonic_py_common.logger import Logger

logger = Logger()

# CodeConstruct mctpd D-Bus service and object tree.
MCTP_SERVICE = "au.com.codeconstruct.MCTP1"
MCTP_BASE_PATH = "/au/com/codeconstruct/mctp1"
BUSOWNER_IFACE = "au.com.codeconstruct.MCTP.BusOwner1"
OBJECT_MANAGER_IFACE = "org.freedesktop.DBus.ObjectManager"
# Endpoint objects expose the OpenBMC MCTP endpoint interface.
ENDPOINT_IFACE = "xyz.openbmc_project.MCTP.Endpoint"
# Per-netdev objects under .../interfaces/<name> expose NetworkId.
INTERFACE_IFACE = "au.com.codeconstruct.MCTP.Interface1"

# The Aspeed IRoT MCTP netdev created by the nvidia-ast27xx-irot kernel driver.
IROT_INTERFACE = "mctpirot0"

# PLDM message type (DSP0239); the BMC FD speaks PLDM for firmware update.
MCTP_MSG_TYPE_PLDM = 1

# Endpoint object paths look like
# /au/com/codeconstruct/mctp1/networks/<net>/endpoints/<eid>
_ENDPOINT_PATH_RE = re.compile(r"/networks/(\d+)/endpoints/(\d+)$")

_BUSCTL = "busctl"

# Default boot wait for mctpirot0: mctpd registers the netdev while operstate is
# still "down"; SetupEndpoint fails with EHOSTUNREACH until the IRoT link is up.
_DEFAULT_LINK_TIMEOUT = 120.0
_DEFAULT_LINK_POLL = 2.0


class MctpError(Exception):
    """Raised when an mctpd D-Bus operation fails or returns no BMC endpoint."""


def _unwrap(value):
    """
    Recursively strip busctl's ``{"type": <sig>, "data": <value>}`` envelopes.

    ``busctl`` wraps the overall method reply, and every D-Bus VARIANT inside it
    (e.g. the values of an ``a{sv}`` property map), in a two-key ``{"type",
    "data"}`` object; container and basic types are emitted as plain JSON. Strip
    those envelopes so callers see plain Python values, independent of the
    busctl/systemd version (older builds emitted bare values, so this is a no-op
    for them).
    """
    if isinstance(value, dict):
        if len(value) == 2 and "data" in value and isinstance(value.get("type"), str):
            return _unwrap(value["data"])
        return {key: _unwrap(val) for key, val in value.items()}
    if isinstance(value, list):
        return [_unwrap(item) for item in value]
    return value


def _busctl_json(args, timeout=10):
    """
    Run ``busctl --json=short`` with `args` and return the decoded reply.

    The reply is the JSON array of the method's return values, with busctl's
    ``{"type", "data"}`` variant envelopes stripped (see :func:`_unwrap`).

    Returns:
        list: the decoded array of return values.

    Raises:
        MctpError: if busctl fails or its output cannot be parsed.
    """
    cmd = [_BUSCTL, "--json=short", *args]
    try:
        proc = subprocess.run(
            cmd,
            check=True,
            capture_output=True,
            text=True,
            timeout=timeout,
        )
    except FileNotFoundError as exc:
        raise MctpError(f"busctl not found: {exc}") from exc
    except OSError as exc:
        # Covers PermissionError and other exec failures so callers (e.g.
        # main()'s SetupEndpoint retry loop) see a uniform MctpError.
        raise MctpError(f"busctl execution failed: {exc}") from exc
    except subprocess.TimeoutExpired as exc:
        raise MctpError(f"busctl timed out: {' '.join(cmd)}") from exc
    except subprocess.CalledProcessError as exc:
        raise MctpError(
            f"busctl failed ({exc.returncode}): {' '.join(cmd)}: "
            f"{(exc.stderr or '').strip()}"
        ) from exc

    try:
        return _unwrap(json.loads(proc.stdout))
    except (ValueError, TypeError) as exc:
        raise MctpError(f"unable to parse busctl output: {proc.stdout!r}") from exc


def interface_operstate(interface):
    """Return the kernel ``operstate`` for `interface` (e.g. ``up`` or ``down``)."""
    path = Path("/sys/class/net") / interface / "operstate"
    try:
        return path.read_text().strip()
    except OSError as exc:
        raise MctpError(f"cannot read operstate for {interface}: {exc}") from exc


def _interface_carrier(interface):
    """Return the kernel ``carrier`` for `interface`, or ``None`` if unavailable."""
    path = Path("/sys/class/net") / interface / "carrier"
    try:
        return path.read_text().strip()
    except OSError:
        return None


def interface_is_up(interface):
    """
    True when the MCTP netdev is ready for SetupEndpoint.

    The IRoT driver often leaves operstate at ``unknown`` even when carrier is
    present; treat ``unknown`` with carrier ``1`` as ready.
    """
    try:
        state = interface_operstate(interface)
    except MctpError:
        return False
    if state == "up":
        return True
    if state == "unknown":
        return _interface_carrier(interface) == "1"
    return False


def wait_for_interface_up(
    interface,
    timeout=_DEFAULT_LINK_TIMEOUT,
    poll_interval=_DEFAULT_LINK_POLL,
    sleep=time.sleep,
):
    """
    Block until `interface` is ready for SetupEndpoint.

    mctpd may expose the D-Bus BusOwner object while the netdev is still down;
    calling SetupEndpoint then fails with "No route to host". The IRoT netdev
    may report operstate ``unknown`` indefinitely even when carrier is up.
    """
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        if interface_is_up(interface):
            logger.log_info(f"MCTP interface {interface} is up")
            return
        sleep(poll_interval)

    state = "missing"
    try:
        state = interface_operstate(interface)
    except MctpError:
        pass
    raise MctpError(
        f"MCTP interface {interface} did not come up within {timeout:.0f}s "
        f"(operstate={state})"
    )


def setup_endpoint(interface=IROT_INTERFACE):
    """
    Ensure the BMC endpoint on `interface` has an EID (idempotent).

    Invokes the bus-owner ``SetupEndpoint`` method, which queries the peer for
    an existing EID and only assigns a new one when needed. The IRoT link is a
    point-to-point mailbox, so no physical address is supplied (empty ``ay``).

    Returns:
        tuple(int, int): ``(net, eid)`` of the BMC endpoint.

    Raises:
        MctpError: if the D-Bus call fails or returns an unexpected shape.
    """
    iface_path = f"{MCTP_BASE_PATH}/interfaces/{interface}"
    reply = _busctl_json([
        "call", MCTP_SERVICE, iface_path, BUSOWNER_IFACE,
        "SetupEndpoint", "ay", "0",
    ])

    # SetupEndpoint returns (y eid, i net, s path, b new).
    if not isinstance(reply, list) or len(reply) < 4:
        raise MctpError(f"unexpected SetupEndpoint reply: {reply!r}")
    eid, net, path, new = reply[0], reply[1], reply[2], reply[3]
    try:
        net_id = int(net)
        eid_id = int(eid)
    except (TypeError, ValueError) as exc:
        raise MctpError(
            f"invalid SetupEndpoint reply for {interface}: "
            f"net={net!r} eid={eid!r}"
        ) from exc
    logger.log_info(
        f"MCTP SetupEndpoint on {interface}: net={net_id} eid={eid_id} "
        f"path={path} new={bool(new)}"
    )
    return net_id, eid_id


def _get_managed_objects():
    """Return the ``GetManagedObjects`` mapping from mctpd's object tree."""
    reply = _busctl_json([
        "call", MCTP_SERVICE, MCTP_BASE_PATH,
        OBJECT_MANAGER_IFACE, "GetManagedObjects",
    ])
    # The single return value is a{oa{sa{sv}}} -> a path->iface->prop mapping.
    if not isinstance(reply, list) or not reply:
        raise MctpError(f"unexpected GetManagedObjects reply: {reply!r}")
    objects = reply[0]
    if not isinstance(objects, dict):
        raise MctpError(f"unexpected GetManagedObjects payload: {objects!r}")
    return objects


def _endpoint_net_eid(path, props):
    """
    Resolve ``(net, eid)`` for an endpoint object.

    Prefers the object path (authoritative), falling back to the endpoint
    interface properties.
    """
    match = _ENDPOINT_PATH_RE.search(path)
    if match:
        return int(match.group(1)), int(match.group(2))
    net = props.get("NetworkId")
    eid = props.get("EID")
    if net is None or eid is None:
        return None
    try:
        return int(net), int(eid)
    except (TypeError, ValueError) as exc:
        raise MctpError(
            f"invalid endpoint properties for {path}: "
            f"net={net!r} eid={eid!r}"
        ) from exc


def _network_id_for_interface(objects, interface=IROT_INTERFACE):
    """
    Return the MCTP network id for `interface` from a GetManagedObjects map.

    Raises:
        MctpError: if the interface object is missing or NetworkId is invalid.
    """
    path = f"{MCTP_BASE_PATH}/interfaces/{interface}"
    ifaces = objects.get(path)
    if not isinstance(ifaces, dict):
        raise MctpError(f"MCTP interface {interface} not found in mctpd")
    props = ifaces.get(INTERFACE_IFACE) or {}
    net = props.get("NetworkId")
    try:
        return int(net)
    except (TypeError, ValueError) as exc:
        raise MctpError(
            f"invalid NetworkId for {interface}: {net!r}"
        ) from exc


def get_interface_network(interface=IROT_INTERFACE):
    """Query mctpd for the MCTP network id bound to `interface`."""
    return _network_id_for_interface(_get_managed_objects(), interface)


def find_bmc_endpoint(objects, network=None, msg_type=MCTP_MSG_TYPE_PLDM):
    """
    Find the PLDM-capable BMC endpoint in a ``GetManagedObjects`` mapping.

    Args:
        objects: mapping returned by :func:`_get_managed_objects`.
        network: if set, restrict the search to this MCTP network id.
        msg_type: required entry in the endpoint's ``SupportedMessageTypes``.

    Returns:
        tuple(int, int): ``(net, eid)`` of the matching endpoint.

    Raises:
        MctpError: if no matching endpoint is found.
    """
    for path, ifaces in objects.items():
        endpoint = ifaces.get(ENDPOINT_IFACE)
        if not isinstance(endpoint, dict):
            continue
        supported = endpoint.get("SupportedMessageTypes") or []
        if msg_type not in supported:
            continue
        try:
            resolved = _endpoint_net_eid(path, endpoint)
        except MctpError as exc:
            logger.log_warning(f"skipping endpoint with invalid MCTP data: {exc}")
            continue
        if resolved is None:
            continue
        net, eid = resolved
        if network is not None and net != network:
            continue
        return net, eid
    raise MctpError(
        f"no PLDM-capable BMC endpoint found (network={network})"
    )


def get_bmc_eid(network=None):
    """
    Query the BMC's ``(net, eid)`` from mctpd without re-assigning an EID.

    Args:
        network: MCTP network id to restrict the search to. When ``None``,
            the IRoT interface (``mctpirot0``) network is used so pldm-fw
            always targets the BMC over IRoT.

    Returns:
        tuple(int, int): ``(net, eid)`` of the BMC endpoint.

    Raises:
        MctpError: if mctpd cannot be queried or no endpoint matches.
    """
    objects = _get_managed_objects()
    if network is None:
        network = _network_id_for_interface(objects, IROT_INTERFACE)
    return find_bmc_endpoint(objects, network=network)


def mctp_address(network=None):
    """Return the BMC endpoint as the ``"<net>,<eid>"`` string pldm-fw expects."""
    net, eid = get_bmc_eid(network=network)
    return f"{net},{eid}"


def main(
    argv=None,
    attempts=10,
    delay=2.0,
    link_timeout=_DEFAULT_LINK_TIMEOUT,
    sleep=time.sleep,
):
    """
    Entry point for the ``mctp-bmc-setup`` systemd oneshot.

    Performs the one-time ``SetupEndpoint`` so the BMC has an EID for the rest
    of the boot. Waits for the IRoT netdev to reach operstate ``up`` before
    calling into mctpd when possible, then retries SetupEndpoint to cover any
    remaining startup races (including when operstate stays ``unknown``). 
    ``SetupEndpoint`` is idempotent so retries are safe.
    Returns a process exit code.
    """
    argv = list(sys.argv[1:] if argv is None else argv)
    interface = argv[1] if len(argv) > 1 and argv[0] == "setup" else IROT_INTERFACE
    try:
        wait_for_interface_up(
            interface, timeout=link_timeout, sleep=sleep
        )
    except MctpError as exc:
        logger.log_warning(
            f"{exc}; proceeding with SetupEndpoint retries"
        )

    last_error = None
    for attempt in range(1, attempts + 1):
        try:
            net, eid = setup_endpoint(interface)
        except MctpError as exc:
            last_error = exc
            logger.log_warning(
                f"MCTP BMC endpoint setup attempt {attempt}/{attempts} failed: {exc}"
            )
            if attempt < attempts:
                sleep(delay)
            continue
        logger.log_info(f"MCTP BMC endpoint ready at {net},{eid}")
        return 0
    logger.log_error(f"MCTP BMC endpoint setup failed: {last_error}")
    return 1


if __name__ == "__main__":
    sys.exit(main())
