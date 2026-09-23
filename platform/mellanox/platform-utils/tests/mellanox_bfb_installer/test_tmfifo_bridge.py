#!/usr/bin/env python3
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
Unit tests for mellanox_bfb_installer.tmfifo_bridge.

These are pure unit tests: ``subprocess.run`` (used to invoke ``ip``/``bridge``)
and the relevant ``os`` filesystem APIs are mocked, so no real bridge or
network namespace change happens. The watcher thread is exercised through
``setup()``/``teardown()`` and via direct calls to its helpers; we never
sleep for the real poll interval to keep tests fast.
"""

import os
import subprocess
import sys
import threading
import unittest
from unittest import mock

sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "..")))


def _ok_run(returncode: int = 0, stderr: str = ""):
    """Build a MagicMock subprocess.run return value."""
    return mock.MagicMock(returncode=returncode, stdout="", stderr=stderr)


# ---------------------------------------------------------------------------
# _run_ip
# ---------------------------------------------------------------------------


class TestRunIp(unittest.TestCase):
    """The thin wrapper around subprocess.run for ip/bridge commands."""

    def test_prepends_ip_when_first_arg_is_not_bridge(self):
        from mellanox_bfb_installer import tmfifo_bridge

        with mock.patch.object(tmfifo_bridge.subprocess, "run", return_value=_ok_run()) as m:
            rc = tmfifo_bridge._run_ip(["link", "add", "br0", "type", "bridge"], check=True)
            self.assertEqual(rc, 0)
            argv = m.call_args[0][0]
            self.assertEqual(argv[0], "ip")
            self.assertEqual(argv[1:], ["link", "add", "br0", "type", "bridge"])
            # subprocess.run kwargs we care about for safety:
            kwargs = m.call_args[1]
            self.assertIs(kwargs.get("stdin"), tmfifo_bridge.subprocess.DEVNULL)
            self.assertTrue(kwargs.get("text"))
            self.assertTrue(kwargs.get("check"))

    def test_does_not_prepend_ip_when_first_arg_is_bridge(self):
        """If a caller ever passes `bridge ...` we trust them and don't add 'ip'."""
        from mellanox_bfb_installer import tmfifo_bridge

        with mock.patch.object(tmfifo_bridge.subprocess, "run", return_value=_ok_run()) as m:
            tmfifo_bridge._run_ip(["bridge", "link", "show"], check=False)
            argv = m.call_args[0][0]
            self.assertEqual(argv[0], "bridge")

    def test_returns_nonzero_when_check_false(self):
        from mellanox_bfb_installer import tmfifo_bridge

        with mock.patch.object(tmfifo_bridge.subprocess, "run", return_value=_ok_run(returncode=2)):
            rc = tmfifo_bridge._run_ip(["link", "del", "br0"], check=False)
            self.assertEqual(rc, 2)

    def test_called_process_error_propagates_when_check_true(self):
        """check=True must let the CalledProcessError escape so setup() can fail."""
        from mellanox_bfb_installer import tmfifo_bridge

        err = subprocess.CalledProcessError(returncode=1, cmd=["ip"], stderr="boom")
        with mock.patch.object(tmfifo_bridge.subprocess, "run", side_effect=err):
            with self.assertRaises(subprocess.CalledProcessError):
                tmfifo_bridge._run_ip(["link", "add", "br0", "type", "bridge"], check=True)


# ---------------------------------------------------------------------------
# _list_tmfifo_ifaces / _iface_master
# ---------------------------------------------------------------------------


class TestListTmfifoIfaces(unittest.TestCase):
    def test_filters_to_tmfifo_net_pattern(self):
        from mellanox_bfb_installer import tmfifo_bridge

        with mock.patch.object(
            tmfifo_bridge.os,
            "listdir",
            return_value=[
                "lo",
                "eth0",
                "tmfifo_net0",
                "tmfifo_net12",
                "tmfifo_netX",  # must NOT match: not all digits
                "tmfifo_net",  # must NOT match: missing digits
                "bridge-midplane",
            ],
        ):
            result = tmfifo_bridge._list_tmfifo_ifaces()
            self.assertEqual(result, ["tmfifo_net0", "tmfifo_net12"])

    def test_returns_empty_list_when_sysfs_unavailable(self):
        from mellanox_bfb_installer import tmfifo_bridge

        with mock.patch.object(
            tmfifo_bridge.os, "listdir", side_effect=OSError("not found")
        ):
            self.assertEqual(tmfifo_bridge._list_tmfifo_ifaces(), [])

    def test_results_are_sorted(self):
        from mellanox_bfb_installer import tmfifo_bridge

        with mock.patch.object(
            tmfifo_bridge.os,
            "listdir",
            return_value=["tmfifo_net2", "tmfifo_net0", "tmfifo_net10", "tmfifo_net1"],
        ):
            self.assertEqual(
                tmfifo_bridge._list_tmfifo_ifaces(),
                # Lexicographic, matching the production code's `sorted(...)`.
                ["tmfifo_net0", "tmfifo_net1", "tmfifo_net10", "tmfifo_net2"],
            )


class TestIfaceMaster(unittest.TestCase):
    def test_returns_basename_of_master_symlink(self):
        from mellanox_bfb_installer import tmfifo_bridge

        with mock.patch.object(
            tmfifo_bridge.os, "readlink", return_value="/sys/class/net/bridge-tmfifo"
        ):
            self.assertEqual(tmfifo_bridge._iface_master("tmfifo_net0"), "bridge-tmfifo")

    def test_returns_none_when_no_master_link(self):
        """Interfaces without /sys/class/net/<iface>/master raise OSError on readlink."""
        from mellanox_bfb_installer import tmfifo_bridge

        with mock.patch.object(
            tmfifo_bridge.os, "readlink", side_effect=OSError("no such link")
        ):
            self.assertIsNone(tmfifo_bridge._iface_master("tmfifo_net0"))


# ---------------------------------------------------------------------------
# TmfifoBridge.setup() / teardown()
# ---------------------------------------------------------------------------


class _BridgeBaseTest(unittest.TestCase):
    """Common helpers for TmfifoBridge tests.

    Patches subprocess.run to a counter+returncode-controlled fake and
    captures every command argv for downstream assertions.
    """

    def _install_subprocess_patch(self, returncodes_by_cmd=None, raise_for_argv=None):
        """Patch tmfifo_bridge.subprocess.run.

        returncodes_by_cmd: optional dict { argv_tuple: returncode } that
            overrides the default rc=0. Tuples are tried with the leading 'ip'
            stripped because _run_ip prepends it.
        raise_for_argv: optional set of argv tuples for which CalledProcessError
            should be raised (with check=True semantics).
        """
        from mellanox_bfb_installer import tmfifo_bridge

        self._calls = []

        def fake_run(argv, **kwargs):
            argv_tuple = tuple(argv)
            self._calls.append(argv_tuple)
            if raise_for_argv and argv_tuple in raise_for_argv:
                raise subprocess.CalledProcessError(
                    returncode=1, cmd=list(argv_tuple), stderr="forced"
                )
            rc = 0
            if returncodes_by_cmd and argv_tuple in returncodes_by_cmd:
                rc = returncodes_by_cmd[argv_tuple]
            if kwargs.get("check") and rc != 0:
                raise subprocess.CalledProcessError(
                    returncode=rc, cmd=list(argv_tuple), stderr=""
                )
            return _ok_run(returncode=rc)

        patcher = mock.patch.object(tmfifo_bridge.subprocess, "run", side_effect=fake_run)
        patcher.start()
        self.addCleanup(patcher.stop)

    def _commands(self):
        """Return the list of argv tuples in call order."""
        return list(self._calls)


class TestTmfifoBridgeSetup(_BridgeBaseTest):
    def test_setup_runs_expected_ip_commands_in_order(self):
        """setup() must: pre-delete (best-effort), add bridge, add IP, set up."""
        from mellanox_bfb_installer import tmfifo_bridge

        self._install_subprocess_patch()
        with mock.patch.object(tmfifo_bridge, "_list_tmfifo_ifaces", return_value=[]):
            br = tmfifo_bridge.TmfifoBridge(bridge_name="bridge-test", bridge_cidr="10.0.0.1/24")
            try:
                br.setup()
            finally:
                # Stop the watcher thread so the test doesn't leave one running.
                br.teardown()

        # Find the four bridge-management commands by their leading tokens.
        bridge_cmds = [c for c in self._commands() if "bridge-test" in c]
        # The pre-delete is first, then add/addr/up.
        self.assertEqual(bridge_cmds[0], ("ip", "link", "del", "bridge-test"))
        self.assertEqual(
            bridge_cmds[1], ("ip", "link", "add", "name", "bridge-test", "type", "bridge")
        )
        self.assertEqual(
            bridge_cmds[2], ("ip", "addr", "add", "10.0.0.1/24", "dev", "bridge-test")
        )
        self.assertEqual(bridge_cmds[3], ("ip", "link", "set", "bridge-test", "up"))

    def test_setup_pre_delete_is_best_effort(self):
        """Pre-delete failure must NOT abort setup (no leftover bridge case)."""
        from mellanox_bfb_installer import tmfifo_bridge

        # First call (`ip link del`) returns non-zero, all others succeed.
        self._install_subprocess_patch(
            returncodes_by_cmd={("ip", "link", "del", "bridge-test"): 1}
        )
        with mock.patch.object(tmfifo_bridge, "_list_tmfifo_ifaces", return_value=[]):
            br = tmfifo_bridge.TmfifoBridge(bridge_name="bridge-test", bridge_cidr="10.0.0.1/24")
            try:
                br.setup()  # must not raise
                self.assertTrue(br._setup_done)
            finally:
                br.teardown()

    def test_setup_raises_when_bridge_add_fails(self):
        """`ip link add ... type bridge` is check=True; failure must surface
        so the caller in main.py can degrade gracefully."""
        from mellanox_bfb_installer import tmfifo_bridge

        bad = ("ip", "link", "add", "name", "bridge-test", "type", "bridge")
        self._install_subprocess_patch(raise_for_argv={bad})

        with mock.patch.object(tmfifo_bridge, "_list_tmfifo_ifaces", return_value=[]):
            br = tmfifo_bridge.TmfifoBridge(bridge_name="bridge-test", bridge_cidr="10.0.0.1/24")
            with self.assertRaises(subprocess.CalledProcessError):
                br.setup()
            self.assertFalse(br._setup_done)
            # No watcher should have been started since setup didn't complete.
            self.assertIsNone(br._watcher)

    def test_setup_is_idempotent_when_called_twice(self):
        """A second setup() call on the same instance must be a no-op."""
        from mellanox_bfb_installer import tmfifo_bridge

        self._install_subprocess_patch()
        with mock.patch.object(tmfifo_bridge, "_list_tmfifo_ifaces", return_value=[]):
            br = tmfifo_bridge.TmfifoBridge(bridge_name="bridge-test", bridge_cidr="10.0.0.1/24")
            try:
                br.setup()
                first_calls = len(self._commands())
                br.setup()
                self.assertEqual(
                    len(self._commands()),
                    first_calls,
                    "second setup() must not run any new ip commands",
                )
            finally:
                br.teardown()

    def test_setup_starts_watcher_thread(self):
        """A daemon watcher thread is spawned and stops cleanly via teardown."""
        from mellanox_bfb_installer import tmfifo_bridge

        self._install_subprocess_patch()
        with mock.patch.object(tmfifo_bridge, "_list_tmfifo_ifaces", return_value=[]):
            br = tmfifo_bridge.TmfifoBridge(bridge_name="bridge-test", bridge_cidr="10.0.0.1/24")
            try:
                br.setup()
                self.assertIsNotNone(br._watcher)
                self.assertTrue(br._watcher.is_alive())
                self.assertTrue(br._watcher.daemon)
            finally:
                br.teardown()
            self.assertIsNone(br._watcher)

    def test_setup_enslaves_already_present_tmfifo_ifaces(self):
        """If tmfifo_net interfaces exist before setup(), they must be enslaved."""
        from mellanox_bfb_installer import tmfifo_bridge

        self._install_subprocess_patch()
        # Two pre-existing tmfifo_net ifaces, neither has a master yet.
        with mock.patch.object(
            tmfifo_bridge, "_list_tmfifo_ifaces", return_value=["tmfifo_net0", "tmfifo_net1"]
        ), mock.patch.object(tmfifo_bridge, "_iface_master", return_value=None):
            br = tmfifo_bridge.TmfifoBridge(bridge_name="bridge-test", bridge_cidr="10.0.0.1/24")
            try:
                br.setup()
            finally:
                br.teardown()

        # The enslave + set-up commands must have been issued for both.
        cmds = self._commands()
        self.assertIn(
            ("ip", "link", "set", "tmfifo_net0", "master", "bridge-test"), cmds
        )
        self.assertIn(("ip", "link", "set", "tmfifo_net0", "up"), cmds)
        self.assertIn(
            ("ip", "link", "set", "tmfifo_net1", "master", "bridge-test"), cmds
        )
        self.assertIn(("ip", "link", "set", "tmfifo_net1", "up"), cmds)


class TestTmfifoBridgeTeardown(_BridgeBaseTest):
    def test_teardown_without_setup_is_noop_safe(self):
        """teardown() before setup() must NOT crash and must NOT issue ip commands."""
        from mellanox_bfb_installer import tmfifo_bridge

        self._install_subprocess_patch()
        br = tmfifo_bridge.TmfifoBridge(bridge_name="bridge-test", bridge_cidr="10.0.0.1/24")
        # Even with no setup, teardown attempts the `link del` (idempotent
        # cleanup); that's fine. What must NOT happen is releasing any slaves.
        br.teardown()
        cmds = self._commands()
        # No `nomaster` releases occurred.
        self.assertFalse(
            any("nomaster" in c for c in cmds),
            "teardown without setup must not release any slaves",
        )

    def test_teardown_deletes_bridge(self):
        from mellanox_bfb_installer import tmfifo_bridge

        self._install_subprocess_patch()
        with mock.patch.object(tmfifo_bridge, "_list_tmfifo_ifaces", return_value=[]):
            br = tmfifo_bridge.TmfifoBridge(bridge_name="bridge-test", bridge_cidr="10.0.0.1/24")
            br.setup()
            br.teardown()

        cmds = self._commands()
        # Two link dels happen: pre-delete during setup, and final delete in teardown.
        # The teardown one must be after addr add.
        del_indices = [i for i, c in enumerate(cmds) if c == ("ip", "link", "del", "bridge-test")]
        self.assertGreaterEqual(len(del_indices), 2)
        addr_index = cmds.index(("ip", "addr", "add", "10.0.0.1/24", "dev", "bridge-test"))
        self.assertGreater(del_indices[-1], addr_index)

    def test_teardown_releases_only_slaves_we_attached(self):
        """teardown() must call `nomaster` for ifaces we attached, and only if
        they're still our slaves (a moved/vanished iface must be skipped).

        We stub out the watcher loop to avoid racing the test's
        ``_iface_master`` mock from the background thread; the watcher itself
        is covered by ``TestTmfifoBridgeWatcherDirect``.
        """
        from mellanox_bfb_installer import tmfifo_bridge

        self._install_subprocess_patch()

        # Stateful fake so concurrent calls (if any) all see consistent values.
        # tmfifo_net0: unenslaved at setup, owned by us during teardown.
        # tmfifo_net1: owned by another bridge from the start.
        master_state = {"tmfifo_net0": None, "tmfifo_net1": "some-other-bridge"}
        teardown_phase = threading.Event()

        def fake_master(iface):
            if teardown_phase.is_set() and iface == "tmfifo_net0":
                return "bridge-test"
            return master_state.get(iface)

        with mock.patch.object(
            tmfifo_bridge, "_list_tmfifo_ifaces", return_value=["tmfifo_net0", "tmfifo_net1"]
        ), mock.patch.object(
            tmfifo_bridge, "_iface_master", side_effect=fake_master
        ), mock.patch.object(
            tmfifo_bridge.TmfifoBridge, "_watcher_loop", lambda self: None
        ):
            br = tmfifo_bridge.TmfifoBridge(bridge_name="bridge-test", bridge_cidr="10.0.0.1/24")
            br.setup()
            # Sanity: only tmfifo_net0 should be tracked.
            self.assertEqual(br._enslaved_by_us, {"tmfifo_net0"})

            teardown_phase.set()
            br.teardown()

        cmds = self._commands()
        # tmfifo_net0 must have been released; tmfifo_net1 must not have been
        # touched by us at any point.
        self.assertIn(
            ("ip", "link", "set", "tmfifo_net0", "nomaster"), cmds
        )
        self.assertNotIn(
            ("ip", "link", "set", "tmfifo_net1", "nomaster"), cmds
        )
        # And we never tried to enslave the one already owned elsewhere.
        self.assertNotIn(
            ("ip", "link", "set", "tmfifo_net1", "master", "bridge-test"), cmds
        )

    def test_teardown_skips_release_when_iface_moved_to_different_master(self):
        """If an iface we enslaved was moved away by an external actor before
        teardown, we must NOT call nomaster on it (would yank an unrelated
        bridge's slave)."""
        from mellanox_bfb_installer import tmfifo_bridge

        self._install_subprocess_patch()

        teardown_phase = threading.Event()

        def fake_master(iface):
            # During setup: unenslaved; during teardown: hijacked by another.
            return "someone-else" if teardown_phase.is_set() else None

        with mock.patch.object(
            tmfifo_bridge, "_list_tmfifo_ifaces", return_value=["tmfifo_net0"]
        ), mock.patch.object(
            tmfifo_bridge, "_iface_master", side_effect=fake_master
        ), mock.patch.object(
            tmfifo_bridge.TmfifoBridge, "_watcher_loop", lambda self: None
        ):
            br = tmfifo_bridge.TmfifoBridge(bridge_name="bridge-test", bridge_cidr="10.0.0.1/24")
            br.setup()
            teardown_phase.set()
            br.teardown()

        cmds = self._commands()
        # Sanity: setup actually enslaved the iface first, so the teardown-skip
        # below is exercising the real "moved away by an external actor" path.
        self.assertIn(("ip", "link", "set", "tmfifo_net0", "master", "bridge-test"), cmds)
        self.assertNotIn(("ip", "link", "set", "tmfifo_net0", "nomaster"), cmds)


# ---------------------------------------------------------------------------
# TmfifoBridge._enslave_one  branch coverage
# ---------------------------------------------------------------------------


class TestTmfifoBridgeEnslaveOne(_BridgeBaseTest):
    """Directly drive _enslave_one through each of its three branches."""

    def _make_bridge(self):
        from mellanox_bfb_installer import tmfifo_bridge

        return tmfifo_bridge.TmfifoBridge(bridge_name="bridge-test", bridge_cidr="10.0.0.1/24")

    def test_already_ours_only_brings_up(self):
        """If master already == us, we only re-up (no second master attach,
        no addition to tracking set since we don't own it via setup)."""
        from mellanox_bfb_installer import tmfifo_bridge

        self._install_subprocess_patch()
        with mock.patch.object(tmfifo_bridge, "_iface_master", return_value="bridge-test"):
            br = self._make_bridge()
            br._enslave_one("tmfifo_net0")

        cmds = self._commands()
        self.assertIn(("ip", "link", "set", "tmfifo_net0", "up"), cmds)
        self.assertNotIn(
            ("ip", "link", "set", "tmfifo_net0", "master", "bridge-test"), cmds
        )
        # We didn't attach it, so we don't claim it for teardown.
        self.assertNotIn("tmfifo_net0", br._enslaved_by_us)

    def test_owned_by_someone_else_is_skipped(self):
        """Don't tug-of-war: leave the iface alone, log only."""
        from mellanox_bfb_installer import tmfifo_bridge

        self._install_subprocess_patch()
        with mock.patch.object(tmfifo_bridge, "_iface_master", return_value="other-bridge"):
            br = self._make_bridge()
            br._enslave_one("tmfifo_net0")

        cmds = self._commands()
        self.assertNotIn(
            ("ip", "link", "set", "tmfifo_net0", "master", "bridge-test"), cmds
        )
        self.assertNotIn(("ip", "link", "set", "tmfifo_net0", "up"), cmds)
        self.assertNotIn("tmfifo_net0", br._enslaved_by_us)

    def test_unenslaved_attaches_and_brings_up_and_tracks(self):
        from mellanox_bfb_installer import tmfifo_bridge

        self._install_subprocess_patch()
        with mock.patch.object(tmfifo_bridge, "_iface_master", return_value=None):
            br = self._make_bridge()
            br._enslave_one("tmfifo_net0")

        cmds = self._commands()
        self.assertIn(
            ("ip", "link", "set", "tmfifo_net0", "master", "bridge-test"), cmds
        )
        self.assertIn(("ip", "link", "set", "tmfifo_net0", "up"), cmds)
        self.assertIn("tmfifo_net0", br._enslaved_by_us)

    def test_enslave_failure_does_not_track(self):
        """If `ip link set ... master <br>` fails, we must NOT bring it up
        and must NOT track it (since teardown would then try nomaster on
        an iface we never owned)."""
        from mellanox_bfb_installer import tmfifo_bridge

        bad = ("ip", "link", "set", "tmfifo_net0", "master", "bridge-test")
        self._install_subprocess_patch(returncodes_by_cmd={bad: 1})
        with mock.patch.object(tmfifo_bridge, "_iface_master", return_value=None):
            br = self._make_bridge()
            br._enslave_one("tmfifo_net0")

        cmds = self._commands()
        # The attach was attempted but the follow-up `link set ... up` was NOT.
        self.assertIn(bad, cmds)
        self.assertNotIn(("ip", "link", "set", "tmfifo_net0", "up"), cmds)
        self.assertNotIn("tmfifo_net0", br._enslaved_by_us)


# ---------------------------------------------------------------------------
# TmfifoBridge context-manager protocol
# ---------------------------------------------------------------------------


class TestTmfifoBridgeContextManager(_BridgeBaseTest):
    def test_enter_calls_setup_exit_calls_teardown(self):
        from mellanox_bfb_installer import tmfifo_bridge

        self._install_subprocess_patch()
        with mock.patch.object(tmfifo_bridge, "_list_tmfifo_ifaces", return_value=[]):
            with tmfifo_bridge.TmfifoBridge(
                bridge_name="bridge-test", bridge_cidr="10.0.0.1/24"
            ) as br:
                self.assertTrue(br._setup_done)
                self.assertIsNotNone(br._watcher)
            self.assertFalse(br._setup_done)
            self.assertIsNone(br._watcher)

    def test_exit_runs_teardown_even_when_body_raises(self):
        from mellanox_bfb_installer import tmfifo_bridge

        self._install_subprocess_patch()
        with mock.patch.object(tmfifo_bridge, "_list_tmfifo_ifaces", return_value=[]):
            with self.assertRaises(RuntimeError):
                with tmfifo_bridge.TmfifoBridge(
                    bridge_name="bridge-test", bridge_cidr="10.0.0.1/24"
                ) as br:
                    raise RuntimeError("boom")
            self.assertFalse(br._setup_done)


# ---------------------------------------------------------------------------
# Watcher loop: ensure new ifaces appearing post-setup get enslaved
# ---------------------------------------------------------------------------


class TestTmfifoBridgeWatcherDirect(_BridgeBaseTest):
    """Exercise the watcher loop directly with mocked dependencies.

    We don't rely on the real thread for this test: we instantiate a bridge,
    set up the stop event, drive one iteration manually, then signal stop.
    """

    def test_watcher_picks_up_new_iface_and_enslaves_it(self):
        from mellanox_bfb_installer import tmfifo_bridge

        self._install_subprocess_patch()

        # Sequence of /sys/class/net snapshots:
        #   1st poll: nothing
        #   2nd poll: tmfifo_net0 appears
        # After 2 polls the stop event is set.
        snapshots = [[], ["tmfifo_net0"]]
        snapshot_iter = iter(snapshots)

        def fake_list():
            try:
                return next(snapshot_iter)
            except StopIteration:
                return ["tmfifo_net0"]

        with mock.patch.object(
            tmfifo_bridge, "_list_tmfifo_ifaces", side_effect=fake_list
        ), mock.patch.object(tmfifo_bridge, "_iface_master", return_value=None):
            br = tmfifo_bridge.TmfifoBridge(
                bridge_name="bridge-test", bridge_cidr="10.0.0.1/24"
            )
            # Bypass setup() to avoid the bridge-management commands;
            # we're isolating the watcher behaviour here.
            br._stop_evt = threading.Event()

            # Run the watcher in this thread for a bounded number of polls.
            done = threading.Event()

            def stop_after_polls():
                # Allow a couple of poll cycles, then stop.
                for _ in range(60):  # up to ~3s at 0.05s poll
                    if len(self._commands()) >= 3:  # attach + up at least
                        break
                    if br._enslaved_by_us:
                        break
                    if done.is_set():
                        break
                    # Speed up the wait by using a tiny poll interval.
                    br._stop_evt.wait(0.05)
                br._stop_evt.set()

            # Tighten the poll interval so the test runs fast.
            with mock.patch.object(tmfifo_bridge, "_WATCHER_POLL_INTERVAL_SEC", 0.01):
                stopper = threading.Thread(target=stop_after_polls, daemon=True)
                stopper.start()
                try:
                    br._watcher_loop()
                finally:
                    done.set()
                    stopper.join(timeout=2)

        # After running, the watcher should have enslaved tmfifo_net0.
        self.assertIn("tmfifo_net0", br._enslaved_by_us)
        cmds = self._commands()
        self.assertIn(
            ("ip", "link", "set", "tmfifo_net0", "master", "bridge-test"), cmds
        )


if __name__ == "__main__":
    unittest.main()
