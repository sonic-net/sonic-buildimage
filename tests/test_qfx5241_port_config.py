"""QFX5241 SKU selection through the real SONiC port configuration parser.

Run with: python3 -B -m unittest discover -s tests -p test_qfx5241_port_config.py
JUNIPER_PLATFORM_SOURCE optionally selects a separate platform checkout.
"""

from contextlib import ExitStack
import functools
import importlib
import importlib.util
import os
from pathlib import Path
import shutil
import sys
import tempfile
import types
import unittest
from unittest import mock


ROOT = Path(__file__).resolve().parents[1]
PLATFORM = "x86_64-juniper_qfx5241-r0"
OD = "Juniper-QFX5241-64-OD"
QD = "Juniper-QFX5241-64-QD"
PLATFORM_SOURCE = Path(os.environ.get(
    "JUNIPER_PLATFORM_SOURCE",
    ROOT / "platform/broadcom/sonic-platform-modules-juniper",
)).resolve()


def load_module(name, path):
    specification = importlib.util.spec_from_file_location(name, path)
    module = importlib.util.module_from_spec(specification)
    sys.modules[name] = module
    specification.loader.exec_module(module)
    return module


class Qfx5241NormalPortConfigTest(unittest.TestCase):
    def setUp(self):
        self.context = ExitStack()
        self.addCleanup(self.context.close)
        self.temporary = Path(self.context.enter_context(tempfile.TemporaryDirectory()))
        self.device_root = self.temporary / "device"
        self.platform_dir = self.device_root / PLATFORM
        shutil.copytree(ROOT / "device/juniper" / PLATFORM, self.platform_dir)
        self.dmi_path = self.temporary / "product_name"
        self._load_sources()
        for name, value in {
            "get_platform": mock.Mock(return_value=PLATFORM),
            "get_path_to_platform_dir": mock.Mock(return_value=str(self.platform_dir)),
            "get_localhost_info": mock.Mock(return_value=None),
            "get_sonic_version_info": mock.Mock(return_value={"asic_type": "broadcom"}),
        }.items():
            self.context.enter_context(mock.patch.object(self.device_info, name, value))
        for name, value in {
            "db_connect_configdb": mock.Mock(return_value=None),
            "PLATFORM_ROOT_PATH": str(self.device_root),
            "PLATFORM_ROOT_PATH_DOCKER": str(self.temporary / "container-platform"),
            "SONIC_ROOT_PATH": str(self.temporary),
            "HWSKU_ROOT_PATH": str(self.temporary / "installed-hwsku"),
        }.items():
            self.context.enter_context(mock.patch.object(self.portconfig, name, value))
        self.context.enter_context(mock.patch.object(
            self.utils, "setup_sku_symlinks",
            functools.partial(self.utils.setup_sku_symlinks, base_dir=str(self.device_root)),
        ))

    def _load_sources(self):
        self.context.enter_context(mock.patch.dict(sys.modules))
        self.context.enter_context(mock.patch.object(sys, "dont_write_bytecode", True))
        for name in list(sys.modules):
            if name == "portconfig" or name.split(".", 1)[0] in (
                "sonic_py_common", "sonic_platform", "swsscommon",
            ):
                del sys.modules[name]
        self.context.enter_context(mock.patch.object(sys, "path", [
            str(ROOT / "src/sonic-py-common"),
            str(ROOT / "src/sonic-config-engine"),
        ] + sys.path))
        self.context.enter_context(mock.patch.dict(os.environ))
        os.environ.pop("CFGGEN_UNIT_TESTING", None)
        swss_package = types.ModuleType("swsscommon")
        swss_module = types.ModuleType("swsscommon.swsscommon")
        swss_module.ConfigDBConnector = object
        swss_module.SonicV2Connector = object
        swss_package.swsscommon = swss_module
        logger = types.ModuleType("sonic_py_common.logger")
        logger.Logger = mock.Mock()
        platform_package = types.ModuleType("sonic_platform")
        platform_package.__path__ = [str(PLATFORM_SOURCE / "sonic_platform")]
        sys.modules.update({
            "swsscommon": swss_package,
            "swsscommon.swsscommon": swss_module,
            "sonic_py_common.logger": logger,
            "sonic_platform": platform_package,
        })
        self.portconfig = importlib.import_module("portconfig")
        self.device_info = importlib.import_module("sonic_py_common.device_info")
        self.utils = importlib.import_module("sonic_platform.utils")
        self.initializer = load_module(
            "qfx5241_regression_platform_init",
            PLATFORM_SOURCE / "qfx5241/utils/qfx5241_platform_init.py",
        )
        self.assertEqual(Path(self.portconfig.__file__).resolve(),
                         ROOT / "src/sonic-config-engine/portconfig.py")
        self.assertEqual(Path(self.device_info.__file__).resolve(),
                         ROOT / "src/sonic-py-common/sonic_py_common/device_info.py")

    def _select_and_check(self, hwsku, dmi_product):
        select_sku = getattr(self.initializer, "select_sku", None)
        self.assertTrue(callable(select_sku),
                        "Platform source must include the SKU-selection fix from Juniper PR #2")
        self.dmi_path.write_text(dmi_product, encoding="ascii")
        selected = select_sku(dmi_path=str(self.dmi_path),
                              cache=str(self.temporary / "missing-eeprom-cache"))
        self.assertEqual(selected, hwsku)
        sku_dir = self.platform_dir / hwsku
        for root_name, sku_name in (
            ("platform.json", "platform.json"),
            ("default_sku", "hwsku_name"),
            ("media_settings.json", "media_settings.json"),
        ):
            link = self.platform_dir / root_name
            self.assertTrue(link.is_symlink(), root_name)
            self.assertEqual(link.resolve(), sku_dir / sku_name)
        default_sku = (self.platform_dir / "default_sku").read_text().split()[0]
        self.assertEqual(default_sku, hwsku)
        selected_file = self.device_info.get_path_to_port_config_file(hwsku=default_sku)
        self.assertEqual(selected_file, str(self.platform_dir / "platform.json"))
        self.assertEqual(self.portconfig.get_hwsku_file_name(default_sku, PLATFORM),
                         str(sku_dir / "hwsku.json"))
        ports, aliases, _ = self.portconfig.get_port_config(hwsku=default_sku, platform=PLATFORM)
        expected, expected_aliases, _ = self.portconfig.parse_port_config_file(
            str(sku_dir / "port_config.ini"))
        expected_names = {f"Ethernet{index}" for index in range(0, 512, 8)}
        expected_names.update(("Ethernet512", "Ethernet513"))
        self.assertEqual(set(ports), expected_names)
        self.assertEqual(len(ports), 66)
        self.assertEqual(set(ports), set(expected))
        self.assertEqual(aliases, expected_aliases)
        for interface in expected_names:
            for field in ("lanes", "speed", "alias"):
                with self.subTest(hwsku=hwsku, interface=interface, field=field):
                    self.assertEqual(ports[interface][field], expected[interface][field])
        first_lane = 25 if hwsku == OD else 1
        self.assertEqual(ports["Ethernet0"]["lanes"],
                         ",".join(str(lane) for lane in range(first_lane, first_lane + 8)))
        self.assertEqual(ports["Ethernet0"]["speed"], "800000")

    def test_od_normal_selector_uses_real_od_metadata(self):
        self._select_and_check(OD, "0F5C")

    def test_qd_normal_selector_uses_real_qd_metadata(self):
        self._select_and_check(QD, "0F5D")

    def test_reselecting_sku_updates_normal_port_configuration(self):
        for hwsku, product in ((OD, "0F5C"), (QD, "0F5D"), (OD, "0F5C")):
            with self.subTest(hwsku=hwsku, product=product):
                self._select_and_check(hwsku, product)

    def test_unselected_qd_exposes_the_original_od_metadata_mismatch(self):
        with self.assertRaisesRegex(
            RuntimeError, r"Unsupported breakout mode 1x800G\[400G,200G,100G\]!",
        ):
            self.portconfig.get_port_config(hwsku=QD, platform=PLATFORM)


if __name__ == "__main__":
    unittest.main()