# SPDX-License-Identifier: Apache-2.0
import importlib.machinery
import importlib.util
import subprocess
import tempfile
import unittest
from pathlib import Path
from unittest import mock

SCRIPT = Path(__file__).resolve().parents[1] / "hotpatches-auto-install"
loader = importlib.machinery.SourceFileLoader("hotpatches_auto_install", str(SCRIPT))
spec = importlib.util.spec_from_loader(loader.name, loader)
hotpatch = importlib.util.module_from_spec(spec)
loader.exec_module(hotpatch)


class FakeConnector:
    APPL_DB = 0

    def __init__(self, results):
        self.results = iter(results)
        self.connected = []
        self.closed = []
        self.keys = []

    def connect(self, db, wait_for_init):
        self.connected.append((db, wait_for_init))

    def exists(self, db, key):
        self.keys.append((db, key))
        return next(self.results)

    def close(self, db):
        self.closed.append(db)


class HotpatchAutoInstallTest(unittest.TestCase):
    def test_list_patches_sorts_hotfix_number_and_ignores_other_files(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            for name in ("sub-Hotfix10.tar.gz", "sub-Hotfix2.tar.gz", "Hotfix1.tar.gz"):
                (root / name).touch()
            (root / "README.txt").touch()
            (root / "nested").mkdir()

            self.assertEqual(
                [p.name for p in hotpatch.list_patches(root)],
                ["Hotfix1.tar.gz", "sub-Hotfix2.tar.gz", "sub-Hotfix10.tar.gz"],
            )

    def test_list_patches_returns_empty_for_missing_directory(self):
        self.assertEqual(hotpatch.list_patches(Path("/does/not/exist")), [])

    def test_wait_for_port_init_done_uses_appl_db_and_closes_connection(self):
        connector = FakeConnector([False, False, True])

        ready = hotpatch.wait_for_port_init_done(
            connector=connector,
            timeout=3,
            interval=1,
            sleep=lambda _seconds: None,
        )

        self.assertTrue(ready)
        self.assertEqual(connector.connected, [(connector.APPL_DB, False)])
        self.assertEqual(
            connector.keys,
            [(connector.APPL_DB, "PORT_TABLE:PortInitDone")] * 3,
        )
        self.assertEqual(connector.closed, [connector.APPL_DB])

    def test_wait_for_port_init_done_times_out(self):
        connector = FakeConnector([False, False])
        self.assertFalse(
            hotpatch.wait_for_port_init_done(
                connector=connector,
                timeout=2,
                interval=1,
                sleep=lambda _seconds: None,
            )
        )

    def test_main_exits_successfully_when_no_patches_are_persisted(self):
        with mock.patch.object(hotpatch, "list_patches", return_value=[]), \
             mock.patch.object(hotpatch.shutil, "which") as which:
            self.assertEqual(hotpatch.main(), 0)
            which.assert_not_called()

    def test_main_reports_database_errors(self):
        patch = Path("/patches/Hotfix1.tar.gz")
        with mock.patch.object(hotpatch, "list_patches", return_value=[patch]), \
             mock.patch.object(hotpatch.shutil, "which", return_value="/usr/bin/sonic-installer"), \
             mock.patch.object(hotpatch, "wait_for_port_init_done", side_effect=OSError("redis unavailable")):
            self.assertEqual(hotpatch.main(), 1)

    def test_main_fails_clearly_when_cli_is_unavailable(self):
        patch = Path("/patches/Hotfix1.tar.gz")
        with mock.patch.object(hotpatch, "list_patches", return_value=[patch]), \
             mock.patch.object(hotpatch.shutil, "which", return_value=None):
            self.assertEqual(hotpatch.main(), 1)

    def test_apply_patches_uses_argv_and_reports_only_failures(self):
        patches = [Path("/patches/Hotfix1.tar.gz"), Path("/patches/Hotfix2.tar.gz")]
        runner = mock.Mock(
            side_effect=[
                subprocess.CompletedProcess([], 0, stdout="ok", stderr=""),
                subprocess.CompletedProcess([], 1, stdout="", stderr="failed"),
            ]
        )

        failed = hotpatch.apply_patches(patches, runner=runner)

        self.assertEqual(failed, [patches[1]])
        self.assertEqual(
            runner.call_args_list[0].args[0],
            ["sonic-installer", "hotpatch-install-single", str(patches[0]), "-y"],
        )
        self.assertEqual(runner.call_args_list[0].kwargs["shell"], False)


if __name__ == "__main__":
    unittest.main()
