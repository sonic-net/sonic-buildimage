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
Ephemeral Linux bridge that absorbs every ``tmfifo_net*`` interface on the
host for the duration of a single ``sonic-bfb-installer`` run.

Why this exists
---------------
Each DPU exposes itself to the host through a ``tmfifo_net<N>`` netdev that
appears when its rshim daemon starts. During a BFB install we want a single,
stable host-side IP that every DPU can reach over tmfifo so the DPU-side
installer can POST diagnostic dumps to us on failure.

Lifecycle (driven by ``sonic-bfb-installer`` main):
    1. Before any rshim is started, ``setup()`` creates the bridge, gives it
       a static IP, and starts a daemon thread that continuously enslaves
       any ``tmfifo_net*`` it sees. Existing tmfifo_net interfaces (if any)
       are enslaved immediately.
    2. ``sonic-bfb-installer`` runs its per-DPU installs in parallel. As
       each DPU's rshim daemon comes up, the corresponding ``tmfifo_net<N>``
       appears and is auto-enslaved by the watcher.
    3. ``teardown()`` (always called from ``finally``) stops the watcher,
       releases any slaves, and deletes the bridge.

Nothing here persists across the process; if ``sonic-bfb-installer`` is
killed without running ``teardown()``, a stale ``bridge-tmfifo`` may remain
but ``setup()`` on the next run replaces it idempotently.
"""

from contextlib import AbstractContextManager
import logging
import os
import re
import subprocess
import threading
import time
from typing import Iterable, List, Optional, Set


DEFAULT_BRIDGE_NAME = "bridge-tmfifo"
DEFAULT_BRIDGE_CIDR = "192.168.100.254/24"
TMFIFO_IFACE_RE = re.compile(r"^tmfifo_net\d+$")

# How often to scan /sys/class/net for new tmfifo_net interfaces. Short enough
# that rshim_daemon.wait_for_rshim_boot's 10s budget always covers it, but not
# so short that we burn CPU during a multi-DPU install.
_WATCHER_POLL_INTERVAL_SEC = 0.5

logger = logging.getLogger(__name__)


def _run_ip(args: List[str], *, check: bool, log_prefix: str = "ip") -> int:
    """Run an ``ip``/``bridge`` command, logging any non-zero result.

    Returns the exit code. When ``check`` is True, raises CalledProcessError
    on failure; the caller uses this for the bridge create/delete operations
    that we want to surface, and ``check=False`` for per-iface enslavement
    that is allowed to fail (e.g. interface vanished mid-loop).
    """
    cmd = ["ip"] + args if not (args and args[0] == "bridge") else args
    logger.debug("%s: running %s", log_prefix, " ".join(cmd))
    try:
        result = subprocess.run(
            cmd,
            stdin=subprocess.DEVNULL,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            check=check,
        )
    except subprocess.CalledProcessError as e:
        logger.warning(
            "%s: command failed (rc=%s): %s | stderr=%s",
            log_prefix,
            e.returncode,
            " ".join(cmd),
            (e.stderr or "").strip(),
        )
        raise
    if result.returncode != 0:
        logger.debug(
            "%s: command non-zero (rc=%s): %s | stderr=%s",
            log_prefix,
            result.returncode,
            " ".join(cmd),
            (result.stderr or "").strip(),
        )
    return result.returncode


def _list_tmfifo_ifaces() -> List[str]:
    """Return current ``tmfifo_net*`` netdev names from ``/sys/class/net``."""
    try:
        names = os.listdir("/sys/class/net")
    except OSError as e:
        logger.warning("could not list /sys/class/net: %s", e)
        return []
    return sorted(n for n in names if TMFIFO_IFACE_RE.match(n))


def _iface_master(iface: str) -> Optional[str]:
    """Return the current master bridge of ``iface``, or None if unenslaved."""
    master_link = f"/sys/class/net/{iface}/master"
    try:
        return os.path.basename(os.readlink(master_link))
    except OSError:
        return None


class TmfifoBridge(AbstractContextManager):
    """Manages an ephemeral host bridge that absorbs all ``tmfifo_net*`` ifaces.

    Use as a context manager for proper teardown::

        with TmfifoBridge() as bridge:
            ... run installs ...

    Or call ``setup()`` / ``teardown()`` directly.
    """

    def __init__(
        self,
        bridge_name: str = DEFAULT_BRIDGE_NAME,
        bridge_cidr: str = DEFAULT_BRIDGE_CIDR,
    ) -> None:
        self.bridge_name = bridge_name
        self.bridge_cidr = bridge_cidr
        self._stop_evt: Optional[threading.Event] = None
        self._watcher: Optional[threading.Thread] = None
        # Track which interfaces *we* enslaved so teardown only releases ours.
        self._enslaved_by_us: Set[str] = set()
        self._enslaved_lock = threading.Lock()
        self._setup_done = False

    # ---- Public lifecycle -------------------------------------------------

    def setup(self) -> None:
        """Create the bridge, assign it the static IP, and start the watcher.

        Safe to call once per instance. If the bridge already exists (stale
        from a prior aborted run) it is deleted and recreated so the IP and
        forwarding state are known-good.
        """
        if self._setup_done:
            logger.debug("setup() already called; skipping")
            return

        # If there's a stale leftover bridge from a previous aborted run,
        # remove it so we start from a clean state. ``ip link del`` of an
        # already-absent device fails, which we ignore.
        _run_ip(["link", "del", self.bridge_name], check=False, log_prefix="bridge-del-pre")

        _run_ip(["link", "add", "name", self.bridge_name, "type", "bridge"], check=True)
        # Bridge defaults are fine for a single broadcast domain between
        # the host and its DPUs over tmfifo.
        _run_ip(["addr", "add", self.bridge_cidr, "dev", self.bridge_name], check=True)
        _run_ip(["link", "set", self.bridge_name, "up"], check=True)
        logger.info(
            "tmfifo bridge %s up with %s", self.bridge_name, self.bridge_cidr
        )

        # Enslave anything already present, then start the watcher for
        # devices that show up later (i.e. when each rshim daemon starts).
        self._enslave_existing()

        self._stop_evt = threading.Event()
        self._watcher = threading.Thread(
            target=self._watcher_loop,
            name=f"tmfifo-bridge-watcher-{self.bridge_name}",
            daemon=True,
        )
        self._watcher.start()
        self._setup_done = True

    def teardown(self) -> None:
        """Stop the watcher, release slaves we attached, and delete the bridge.

        Safe to call multiple times and safe to call without a prior
        ``setup()`` (no-op in that case). Always best-effort: it logs but
        does not raise on individual command failures so an installer
        finally-block can always reach completion.
        """
        if self._stop_evt is not None:
            self._stop_evt.set()
        if self._watcher is not None:
            self._watcher.join(timeout=5.0)
            if self._watcher.is_alive():
                logger.warning(
                    "tmfifo bridge watcher %s did not exit within 5s",
                    self.bridge_name,
                )
            self._watcher = None

        # Release the slaves we attached. We deliberately leave alone any
        # tmfifo_net that some external actor enslaved elsewhere.
        with self._enslaved_lock:
            to_release = list(self._enslaved_by_us)
            self._enslaved_by_us.clear()
        for iface in to_release:
            # Only unset master if we are still the master. If the iface
            # vanished or moved, ignore.
            if _iface_master(iface) == self.bridge_name:
                _run_ip(
                    ["link", "set", iface, "nomaster"],
                    check=False,
                    log_prefix=f"release-{iface}",
                )

        _run_ip(
            ["link", "del", self.bridge_name],
            check=False,
            log_prefix="bridge-del",
        )
        self._setup_done = False
        logger.info("tmfifo bridge %s torn down", self.bridge_name)

    # ---- Context manager --------------------------------------------------

    def __enter__(self) -> "TmfifoBridge":
        self.setup()
        return self

    def __exit__(self, exc_type, exc, tb) -> None:
        self.teardown()

    # ---- Internals --------------------------------------------------------

    def _enslave_existing(self) -> None:
        self._enslave_iter(_list_tmfifo_ifaces())

    def _enslave_iter(self, ifaces: Iterable[str]) -> None:
        for iface in ifaces:
            self._enslave_one(iface)

    def _enslave_one(self, iface: str) -> None:
        """Enslave a single ``tmfifo_net*`` to the bridge and bring it up.

        Idempotent: if already enslaved to our bridge, just bring it up; if
        enslaved to some other bridge, log and skip (don't tug-of-war).
        """
        current_master = _iface_master(iface)
        if current_master == self.bridge_name:
            # Already ours: just make sure it's up.
            _run_ip(
                ["link", "set", iface, "up"],
                check=False,
                log_prefix=f"reup-{iface}",
            )
            return
        if current_master is not None:
            # Someone else owns it.
            logger.warning(
                "%s is already enslaved to %s; not moving to %s",
                iface,
                current_master,
                self.bridge_name,
            )
            return

        rc = _run_ip(
            ["link", "set", iface, "master", self.bridge_name],
            check=False,
            log_prefix=f"enslave-{iface}",
        )
        if rc != 0:
            return
        _run_ip(
            ["link", "set", iface, "up"],
            check=False,
            log_prefix=f"up-{iface}",
        )
        with self._enslaved_lock:
            self._enslaved_by_us.add(iface)
        logger.info("enslaved %s to %s", iface, self.bridge_name)

    def _watcher_loop(self) -> None:
        """Background poller: enslave any tmfifo_net* that shows up.

        Polls /sys/class/net at a coarse interval. Polling (vs. listening on
        netlink) keeps this dependency-free and easy to reason about; the
        tmfifo_net devices on a smart switch number in the single digits and
        only appear/disappear at rshim daemon start/stop, so 0.5s latency is
        fine.
        """
        assert self._stop_evt is not None
        already_seen: Set[str] = set()
        while not self._stop_evt.is_set():
            current = set(_list_tmfifo_ifaces())
            new_ifaces = current - already_seen
            if new_ifaces:
                logger.debug(
                    "watcher discovered new tmfifo ifaces: %s",
                    sorted(new_ifaces),
                )
                self._enslave_iter(sorted(new_ifaces))
            # Forget interfaces that have gone away so we'll re-enslave them
            # if they come back (e.g. rshim restart).
            already_seen = current
            self._stop_evt.wait(_WATCHER_POLL_INTERVAL_SEC)
