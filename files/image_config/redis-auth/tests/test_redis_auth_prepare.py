"""Boot-only Redis credential checks. Run as root in an isolated container."""

import base64
import contextlib
import hashlib
import io
import multiprocessing
import os
from pathlib import Path
import stat
import subprocess
import sys
import tempfile
import unittest
from unittest import mock

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import redis_auth_prepare as helper  # noqa: E402


def _prepare_worker(runtime, barrier, queue):
    barrier.wait(timeout=10)
    try:
        queue.put(("ok", helper.prepare_credential(runtime_root=runtime)))
    except Exception as error:  # pragma: no cover - report to parent
        queue.put(("error", type(error).__name__))


class RedisAuthPrepareTest(unittest.TestCase):
    PASSWORD = "A" * 43

    def setUp(self):
        if os.geteuid() != 0:
            self.skipTest("protected credential fixtures require root")
        temporary = tempfile.TemporaryDirectory(prefix="redis-auth-prepare-test-")
        self.addCleanup(temporary.cleanup)
        self.root = Path(temporary.name)
        self.root.chmod(0o755)
        self.runtime = self.root / "run"
        self.target = self.runtime / helper.INSTANCE
        self.secret = self.target / "writer.secret"
        self.digest = self.target / "writer.sha256"

    def prepare(self):
        return helper.prepare_credential(runtime_root=self.runtime)

    def create(self):
        with mock.patch.object(helper.secrets, "token_urlsafe", return_value=self.PASSWORD):
            return self.prepare()

    def rewrite(self, path, content):
        path.chmod(0o600)
        path.write_bytes(content)
        path.chmod(0o400)

    def test_fresh_runtime_creates_one_complete_pair(self):
        with mock.patch.object(helper.secrets, "token_urlsafe", wraps=helper.secrets.token_urlsafe) as generate:
            self.assertTrue(self.prepare())
        generate.assert_called_once_with(32)
        password = self.secret.read_bytes()
        self.assertEqual(len(password), 43)
        self.assertEqual(len(base64.b64decode(password + b"=", altchars=b"-_", validate=True)), 32)
        self.assertEqual(self.digest.read_bytes(), hashlib.sha256(password).hexdigest().encode() + b"\n")
        self.assertEqual(set(self.runtime.iterdir()), {self.target})
        for path in (self.runtime, self.target, self.secret, self.digest):
            info = path.lstat()
            self.assertEqual(stat.S_IMODE(info.st_mode), 0o755 if path.is_dir() else 0o400)
            self.assertEqual((info.st_uid, info.st_gid), (0, 0))
            if path.is_file():
                self.assertEqual(info.st_nlink, 1)

    def test_same_boot_reuses_pair_and_file_inodes(self):
        self.assertTrue(self.create())
        paths = (self.target, self.secret, self.digest)
        before = [(path.stat().st_ino, path.stat().st_mtime_ns) for path in paths]
        with mock.patch.object(helper.secrets, "token_urlsafe") as generate:
            self.assertFalse(self.prepare())
            self.assertFalse(self.prepare())
        generate.assert_not_called()
        self.assertEqual([(path.stat().st_ino, path.stat().st_mtime_ns) for path in paths], before)
        self.assertEqual(self.secret.read_text(), self.PASSWORD)

    def test_new_host_boot_uses_fresh_runtime_and_password(self):
        self.create()
        new_runtime = self.root / "next-boot-run"
        with mock.patch.object(helper.secrets, "token_urlsafe", return_value="B" * 43):
            self.assertTrue(helper.prepare_credential(runtime_root=new_runtime))
        next_secret = new_runtime / helper.INSTANCE / "writer.secret"
        self.assertNotEqual(next_secret.read_bytes(), self.secret.read_bytes())
        self.assertEqual(self.secret.read_text(), self.PASSWORD)

    def test_concurrent_preparation_creates_once(self):
        if "fork" not in multiprocessing.get_all_start_methods():
            self.skipTest("requires fork")
        context = multiprocessing.get_context("fork")
        barrier, queue = context.Barrier(2), context.Queue()
        processes = [context.Process(target=_prepare_worker, args=(str(self.runtime), barrier, queue))
                     for _ in range(2)]
        try:
            for process in processes:
                process.start()
            for process in processes:
                process.join(timeout=15)
            self.assertEqual([process.exitcode for process in processes], [0, 0])
            results = [queue.get(timeout=5) for _ in processes]
        finally:
            for process in processes:
                if process.is_alive():
                    process.terminate()
                    process.join(timeout=5)
            queue.close()
        self.assertEqual(sorted(results), [("ok", False), ("ok", True)])
        self.assertFalse(self.prepare())

    def test_pair_publishes_in_one_directory_rename(self):
        rename = helper.os.rename
        def publish(source, target):
            source = Path(source)
            self.assertFalse(Path(target).exists())
            self.assertEqual(set(path.name for path in source.iterdir()), {"writer.secret", "writer.sha256"})
            self.assertEqual((source / "writer.secret").read_text(), self.PASSWORD)
            return rename(source, target)
        with mock.patch.object(helper.os, "rename", side_effect=publish) as publish_pair:
            self.assertTrue(self.create())
        self.assertEqual(publish_pair.call_count, 1)

    def test_interruption_before_publication_leaves_no_final_state(self):
        write = helper._write
        def stop_at_hash(path, content):
            if Path(path).name == "writer.sha256":
                raise OSError("injected interruption")
            return write(path, content)
        with mock.patch.object(helper, "_write", side_effect=stop_at_hash):
            with self.assertRaises(OSError):
                self.create()
        self.assertFalse(self.target.exists())
        self.assertEqual(list(self.runtime.iterdir()), [])
        self.assertTrue(self.create())

    def test_failed_rename_leaves_no_final_state(self):
        with mock.patch.object(helper.os, "rename", side_effect=OSError("injected interruption")):
            with self.assertRaises(OSError):
                self.create()
        self.assertFalse(self.target.exists())
        self.assertEqual(list(self.runtime.iterdir()), [])
        self.assertTrue(self.create())

    def test_abandoned_private_stage_does_not_become_final_state(self):
        self.runtime.mkdir(mode=0o755)
        stage = self.runtime / ".local-system-abandoned"
        stage.mkdir(mode=0o700)
        (stage / "writer.secret").write_text("unused")
        self.assertTrue(self.create())
        self.assertEqual(self.secret.read_text(), self.PASSWORD)
        self.assertTrue(stage.exists())

    def test_missing_file_in_published_pair_never_generates_replacement(self):
        for name in ("writer.secret", "writer.sha256"):
            with self.subTest(name=name):
                runtime = self.root / name
                helper.prepare_credential(runtime_root=runtime)
                (runtime / helper.INSTANCE / name).unlink()
                with mock.patch.object(helper.secrets, "token_urlsafe") as generate:
                    with self.assertRaises(helper.CredentialStateError):
                        helper.prepare_credential(runtime_root=runtime)
                generate.assert_not_called()

    def test_empty_published_directory_never_generates_replacement(self):
        self.runtime.mkdir(mode=0o755)
        self.target.mkdir(mode=0o755)
        with mock.patch.object(helper.secrets, "token_urlsafe") as generate:
            with self.assertRaises(helper.CredentialStateError):
                self.prepare()
        generate.assert_not_called()
        self.assertEqual(list(self.target.iterdir()), [])

    def test_changed_password_is_rejected_without_replacement(self):
        self.create()
        self.rewrite(self.secret, b"B" * 43)
        with mock.patch.object(helper.secrets, "token_urlsafe") as generate:
            with self.assertRaisesRegex(helper.CredentialStateError, "does not match"):
                self.prepare()
        generate.assert_not_called()
        self.assertEqual(self.secret.read_bytes(), b"B" * 43)

    def test_invalid_credential_size_is_rejected(self):
        self.create()
        self.rewrite(self.secret, b"short")
        with self.assertRaisesRegex(helper.CredentialStateError, "size"):
            self.prepare()

    def test_changed_hash_is_rejected_without_replacement(self):
        self.create()
        self.rewrite(self.digest, b"0" * 64 + b"\n")
        with self.assertRaisesRegex(helper.CredentialStateError, "does not match"):
            self.prepare()
        self.assertEqual(self.digest.read_bytes(), b"0" * 64 + b"\n")

    def test_unsafe_file_mode_is_rejected(self):
        self.create()
        self.secret.chmod(0o644)
        with self.assertRaises(helper.CredentialStateError):
            self.prepare()

    def test_unsafe_file_owner_is_rejected(self):
        self.create()
        os.chown(self.secret, 65534, 65534)
        with self.assertRaises(helper.CredentialStateError):
            self.prepare()

    def test_hard_linked_file_is_rejected(self):
        self.create()
        os.link(self.secret, self.root / "linked-secret")
        with self.assertRaises(helper.CredentialStateError):
            self.prepare()

    def test_symlinked_file_is_rejected(self):
        self.create()
        self.digest.unlink()
        self.digest.symlink_to(self.secret)
        with self.assertRaises(helper.CredentialStateError):
            self.prepare()

    def test_fifo_is_rejected_without_blocking(self):
        self.create()
        self.secret.unlink()
        os.mkfifo(self.secret, 0o400)
        with self.assertRaises(helper.CredentialStateError):
            self.prepare()

    def test_symlinked_final_directory_is_rejected(self):
        self.runtime.mkdir(mode=0o755)
        other = self.root / "other"
        other.mkdir(mode=0o755)
        self.target.symlink_to(other, target_is_directory=True)
        with self.assertRaises(helper.CredentialStateError):
            self.prepare()
        self.assertEqual(list(other.iterdir()), [])

    def test_symlinked_runtime_root_is_rejected(self):
        other = self.root / "other"
        other.mkdir(mode=0o755)
        self.runtime.symlink_to(other, target_is_directory=True)
        with self.assertRaises(helper.CredentialStateError):
            self.prepare()
        self.assertEqual(list(other.iterdir()), [])

    def test_writable_runtime_ancestor_is_rejected(self):
        self.root.chmod(0o777)
        with self.assertRaises(helper.CredentialStateError):
            self.prepare()
        self.assertFalse(self.runtime.exists())

    def test_invalid_runtime_root_is_rejected(self):
        for root in ("relative", "/", "//run/redis-auth", str(self.runtime) + "/../run"):
            with self.subTest(root=root), self.assertRaises(helper.CredentialStateError):
                helper.prepare_credential(runtime_root=root)

    def test_non_root_can_read_public_policy_but_not_password_or_hash(self):
        self.create()
        policy = self.runtime / "local.json"
        policy.write_text("{}", encoding="ascii")
        policy.chmod(0o644)
        check = """
from pathlib import Path
import sys
assert Path(sys.argv[1]).read_text() == "{}"
for path in sys.argv[2:]:
    try:
        Path(path).read_bytes()
    except PermissionError:
        continue
    raise AssertionError("credential is readable")
"""
        result = subprocess.run([sys.executable, "-c", check, str(policy),
                                 str(self.secret), str(self.digest)],
                                user=65534, group=65534, extra_groups=(),
                                capture_output=True, text=True, check=False)
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)

    def test_core_emits_no_password_or_hash(self):
        stdout, stderr = io.StringIO(), io.StringIO()
        with contextlib.redirect_stdout(stdout), contextlib.redirect_stderr(stderr):
            self.create()
        self.assertEqual(stdout.getvalue() + stderr.getvalue(), "")

    def test_cli_status_contains_only_fixed_name_and_action(self):
        for created, action in ((True, "created"), (False, "reused")):
            stdout, stderr = io.StringIO(), io.StringIO()
            with mock.patch.object(helper, "prepare_credential", return_value=created), \
                 contextlib.redirect_stdout(stdout), contextlib.redirect_stderr(stderr):
                self.assertEqual(helper.main([]), 0)
            self.assertEqual(stdout.getvalue(), f"redis-auth-prepare: instance=local-system action={action}\n")
            self.assertEqual(stderr.getvalue(), "")

    def test_non_root_stops_before_preparing_runtime(self):
        stderr = io.StringIO()
        with mock.patch.object(helper.os, "geteuid", return_value=1000), \
             mock.patch.object(helper, "_runtime_directory") as directory, \
             contextlib.redirect_stderr(stderr):
            self.assertEqual(helper.main([]), 1)
        directory.assert_not_called()
        self.assertIn("must run as root", stderr.getvalue())

    def test_cli_contains_operating_system_error_details(self):
        stderr = io.StringIO()
        with mock.patch.object(helper, "prepare_credential", side_effect=OSError("/secret/path")), \
             contextlib.redirect_stderr(stderr):
            self.assertEqual(helper.main([]), 1)
        self.assertIn("operating-system failure", stderr.getvalue())
        self.assertNotIn("/secret/path", stderr.getvalue())

    def test_cli_has_no_persistent_state_or_initialization_options(self):
        for option in ("--instance", "--initialize", "--state-root", "--runtime-root"):
            stderr = io.StringIO()
            with self.subTest(option=option), contextlib.redirect_stderr(stderr):
                with self.assertRaises(SystemExit) as error:
                    helper.main([option])
            self.assertEqual(error.exception.code, 2)


if __name__ == "__main__":
    unittest.main()
