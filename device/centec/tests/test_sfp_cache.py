import errno
import importlib.util
import os
from pathlib import Path
import stat
import tempfile
import types
import unittest
from unittest.mock import Mock, patch


PLATFORMS = (
    "x86_64-centec_e582_48x2q4z-r0",
    "x86_64-centec_e582_48x6q-r0",
)


@unittest.skipUnless(hasattr(os, "O_NOFOLLOW"), "Requires POSIX directory APIs")
class TestSfpCache(unittest.TestCase):
    def setUp(self):
        self.temp_dir = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp_dir.cleanup)
        self.root = Path(self.temp_dir.name)

    def load_plugin(self, platform):
        base_module = types.ModuleType("sonic_sfp.sfputilbase")
        base_module.SfpUtilBase = type("SfpUtilBase", (), {})
        path = (Path(__file__).resolve().parents[1] /
                platform / "plugins" / "sfputil.py")
        spec = importlib.util.spec_from_file_location("sfputil_under_test", path)
        module = importlib.util.module_from_spec(spec)
        with patch.dict("sys.modules", {
            "sonic_sfp": types.ModuleType("sonic_sfp"),
            "sonic_sfp.sfputilbase": base_module,
        }):
            spec.loader.exec_module(module)
        return module

    def initialize(self, module, cache, before_open=None, chmod_error=None):
        def open_cache(path, flags):
            if before_open:
                before_open()
            return os.open(cache, flags)

        chmod = Mock(side_effect=chmod_error or os.fchmod)
        close = Mock(wraps=os.close)
        cache_os = types.SimpleNamespace(
            makedirs=lambda path, **kwargs: os.makedirs(cache, **kwargs),
            open=open_cache,
            fchmod=chmod,
            close=close,
            O_RDONLY=os.O_RDONLY,
            O_DIRECTORY=os.O_DIRECTORY,
            O_NOFOLLOW=os.O_NOFOLLOW,
        )
        with patch.object(module, "os", cache_os), \
                patch.object(module, "socket"), \
                patch.object(module.SfpUtil, "PORT_END", 0), \
                patch.object(module, "DBG_PRINT") as report:
            module.SfpUtil()
        return chmod, close, report

    def test_creates_cache_and_repairs_existing_permissions(self):
        for platform in PLATFORMS:
            module = self.load_plugin(platform)
            for existing in (False, True):
                with self.subTest(platform=platform, existing=existing):
                    cache = self.root / (platform + str(existing))
                    if existing:
                        cache.mkdir()
                        cache.chmod(0o777)
                    _, close, report = self.initialize(module, cache)
                    self.assertEqual(
                        stat.S_IMODE(cache.stat().st_mode), 0o755)
                    close.assert_called_once()
                    report.assert_not_called()

    def test_rejects_symlinks_without_changing_targets(self):
        for platform in PLATFORMS:
            module = self.load_plugin(platform)
            for target_type in ("directory", "file", "missing"):
                with self.subTest(platform=platform, target_type=target_type):
                    target = self.root / (platform + target_type)
                    cache = self.root / (platform + target_type + "-cache")
                    if target_type == "directory":
                        target.mkdir()
                    elif target_type == "file":
                        target.touch()
                    if target.exists():
                        target.chmod(0o700)
                    cache.symlink_to(target)
                    chmod, close, report = self.initialize(module, cache)
                    chmod.assert_not_called()
                    close.assert_not_called()
                    report.assert_called_once()
                    if target.exists():
                        self.assertEqual(
                            stat.S_IMODE(target.stat().st_mode), 0o700)
                    else:
                        self.assertFalse(target.exists())

    def test_rejects_regular_file(self):
        for platform in PLATFORMS:
            with self.subTest(platform=platform):
                cache = self.root / platform
                cache.touch()
                cache.chmod(0o600)
                chmod, close, report = self.initialize(
                    self.load_plugin(platform), cache)
                chmod.assert_not_called()
                close.assert_not_called()
                report.assert_called_once()
                self.assertEqual(
                    stat.S_IMODE(cache.stat().st_mode), 0o600)

    def test_rejects_symlink_substituted_after_makedirs(self):
        for platform in PLATFORMS:
            with self.subTest(platform=platform):
                target = self.root / (platform + "-target")
                target.mkdir()
                target.chmod(0o700)
                cache = self.root / platform

                def replace_cache():
                    cache.rmdir()
                    cache.symlink_to(target)

                chmod, close, report = self.initialize(
                    self.load_plugin(platform), cache, before_open=replace_cache)
                chmod.assert_not_called()
                close.assert_not_called()
                report.assert_called_once()
                self.assertEqual(
                    stat.S_IMODE(target.stat().st_mode), 0o700)

    def test_closes_directory_on_chmod_failure(self):
        for platform in PLATFORMS:
            with self.subTest(platform=platform):
                chmod, close, report = self.initialize(
                    self.load_plugin(platform), self.root / platform,
                    chmod_error=OSError(errno.EPERM, "Permission denied"))
                chmod.assert_called_once()
                close.assert_called_once_with(chmod.call_args.args[0])
                report.assert_called_once()
                with self.assertRaises(OSError) as caught:
                    os.fstat(close.call_args.args[0])
                self.assertEqual(caught.exception.errno, errno.EBADF)


if __name__ == "__main__":
    unittest.main()
