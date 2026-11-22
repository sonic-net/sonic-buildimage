"""Exercise the offline package manager's layout source without a Docker daemon."""
import importlib.util
import json
from pathlib import Path
import sys
import tempfile
from types import SimpleNamespace
import unittest
from unittest.mock import MagicMock, patch

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT))

from tools.bazel.image import merge_layers
from tools.bazel.oci import oci_layout
from tools.bazel.tests.oci_base_fixture import digest, image_fixture, oci_files, write_layout


class FakePackageSource:
    def __init__(self, database, docker, metadata_resolver):
        self.metadata_resolver = metadata_resolver

    def get_package(self):
        return SimpleNamespace(name=self.get_metadata()["name"], repository=None, entry=SimpleNamespace())


class BootPackageLayoutTest(unittest.TestCase):
    def setUp(self):
        temporary = tempfile.TemporaryDirectory()
        self.addCleanup(temporary.cleanup)
        self.root = Path(temporary.name)
        config, layers = image_fixture()
        self.tag = "docker-dhcp-relay:latest"
        config = json.loads(config)
        config["config"]["Labels"] = {"name": "dhcp-relay"}
        config = json.dumps(config).encode()
        self.image_id = digest(config)
        self.layout = self.root / "layout"
        write_layout(self.layout, oci_files(config, layers))
        modules = {
            "merge_layers": merge_layers, "oci_layout": oci_layout,
            "sonic_package_manager": SimpleNamespace(PackageManager=object),
            "sonic_package_manager.metadata": SimpleNamespace(MetadataResolver=object),
            "sonic_package_manager.source": SimpleNamespace(PackageSource=FakePackageSource),
        }
        imports = patch.dict(sys.modules, modules)
        imports.start()
        self.addCleanup(imports.stop)
        spec = importlib.util.spec_from_file_location("boot_packages", ROOT / "scripts/build_boot_packages.py")
        self.subject = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(self.subject)
        self.manager = MagicMock()
        self.manager.metadata_resolver.from_labels.side_effect = lambda labels: labels

    def test_package_registration_and_cli_files_come_from_selected_layout(self):
        destination = self.root / "host/config/plugins/dhcp_relay.py"
        def install(source, **kwargs):
            self.assertEqual(kwargs, {"enable": True, "default_owner": "local"})
            package = source.get_package()
            source.install(package)
            self.assertEqual(package.name, "dhcp-relay")
            self.assertEqual(package.entry.image_id, self.image_id)
            self.assertEqual(package.entry.docker_image_reference, self.tag)
            self.assertEqual(package.entry.repository, "docker-dhcp-relay")
            with patch("os.chown") as chown:
                self.manager.docker.extract(self.image_id, "/etc/base", destination)
            chown.assert_called_once_with(destination, 0, 0)
            self.assertEqual(destination.read_bytes(), b"updated by second layer\n")
            with self.assertRaisesRegex(ValueError, "Missing regular CLI plugin"):
                self.manager.docker.extract(self.image_id, "/etc/deleted", destination)
        self.manager.is_installed.return_value = False
        self.manager.install_from_source.side_effect = install
        original = self.manager.docker.extract
        self.subject.install_layout(self.manager, self.layout, self.tag, "linux/amd64",
                                    {self.tag: self.image_id}, {"enable": True, "default_owner": "local"})
        self.manager.install_from_source.assert_called_once()
        self.assertIs(self.manager.docker.extract, original)
        self.manager.docker.load.assert_not_called()

    def test_mismatched_bundle_fails_before_package_registration(self):
        with self.assertRaisesRegex(ValueError, "differs from the installed bundle"):
            self.subject.install_layout(self.manager, self.layout, self.tag, "linux/amd64",
                                        {self.tag: "sha256:" + "0" * 64}, {})
        self.manager.install_from_source.assert_not_called()
        self.manager.upgrade_from_source.assert_not_called()


if __name__ == "__main__":
    unittest.main()
