"""Platform initialization tests with file-backed EEPROM and mocked board HAL."""
import copy
import importlib.util
import json
import logging
import os
from pathlib import Path
import struct
import tempfile
import types
import unittest
from unittest.mock import MagicMock, patch

from sonic_platform_base.chassis_base import ChassisBase
from sonic_platform_base.sonic_xcvr.cpo.cpo_base import CpoBase, OeId
from sonic_platform_base.sonic_xcvr.cpo.oe import OeBase
from sonic_platform_base.sonic_xcvr.cpo.elsfp import ElsfpBase
from sonic_platform_base.sonic_xcvr.api.broadcom.bailly import BaillyApi
from sonic_platform_base.sonic_xcvr.api.broadcom.bailly_elsfp import BaillyElsfpApi


ROOT = Path(os.environ.get("CPO_DRAFT_REPO", Path(__file__).resolve().parents[5]))
PLATFORM = ROOT / "platform/broadcom/sonic-platform-modules-micas/common/sonic_platform"


class FakeSfp:
    def __init__(self, index):
        self.index = int(index)

    def get_presence(self):
        return True


def load_source(name, path):
    spec = importlib.util.spec_from_file_location(name, path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


class TestCpoInitialization(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.cpo = load_source("micas_cpo_under_test", PLATFORM / "cpo.py")
        cls.device_info = MagicMock()
        cls.device_info.get_cpo_data.return_value = None
        modules = {"sonic_platform.cpo": cls.cpo}
        package = types.ModuleType("sonic_platform")
        package.__path__ = [str(PLATFORM)]
        modules["sonic_platform"] = package
        for name, classes in {
            "sfp": ["Sfp"], "psu": ["Psu"], "fan_drawer": ["FanDrawer"],
            "thermal": ["Thermal"], "watchdog": ["Watchdog"],
            "component": ["Component"], "eeprom": ["Eeprom"],
            "dcdc": ["Dcdc", "VoltageSensor", "CurrentSensor"],
        }.items():
            module = types.ModuleType("sonic_platform." + name)
            for class_name in classes:
                setattr(module, class_name, FakeSfp if class_name == "Sfp" else MagicMock())
            modules[module.__name__] = module
        hal = types.ModuleType("plat_hal")
        hal.__path__ = []
        modules["plat_hal"] = hal
        cls.baseutil = MagicMock()
        module = types.ModuleType("plat_hal.baseutil")
        module.baseutil = cls.baseutil
        modules[module.__name__] = module
        module = types.ModuleType("plat_hal.interface")
        cls.board = MagicMock()
        for method in ["get_fan_total_number", "get_psu_total_number", "get_temp_id_number",
                       "get_cpld_total_number", "get_dcdc_total_number"]:
            getattr(cls.board, method).return_value = 0
        module.interface = MagicMock(return_value=cls.board)
        modules[module.__name__] = module
        from sonic_py_common import device_info
        cls.modules_patch = patch.dict("sys.modules", modules)
        cls.modules_patch.start()
        cls.device_patch = patch.multiple(device_info, get_cpo_data=cls.device_info.get_cpo_data,
                                                get_platform_json_data=cls.device_info.get_platform_json_data)
        cls.device_patch.start()
        cls.base_device_patch = patch("sonic_platform_base.chassis_base.device_info", device_info)
        cls.base_device_patch.start()
        cls.chassis = load_source("micas_chassis_under_test", PLATFORM / "chassis.py")
        cls.log_patch = patch.object(cls.cpo.MicasCpo, "_setup_logging",
                                    lambda obj: setattr(obj, "logger", logging.getLogger("cpo-test")))
        cls.log_patch.start()

    @classmethod
    def tearDownClass(cls):
        cls.log_patch.stop()
        cls.base_device_patch.stop()
        cls.device_patch.stop()
        cls.modules_patch.stop()

    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.directory = Path(self.temp.name)
        self.eeprom = self.directory / "eeprom"
        self.eeprom.write_bytes(bytes(1024 * 1024))
        (self.directory / "max_bank_size").write_text("1", encoding="utf-8")
        self.presence = self.directory / "presence"
        self.presence.write_bytes(bytes(4))
        self.device_info.get_cpo_data.return_value = None
        self.device_info.get_platform_json_data.return_value = json.loads((ROOT /
            "device/micas/x86_64-micas_m2-w6940-128x1-fr4-r0/platform.json").read_text())
        self.baseutil.get_config.return_value = {"sfps": {"port_num": 2, "port_index_start": 1}}

    def topology(self, variant="x86_64-micas_m2-w6940-128x1-fr4-r0"):
        path = ROOT / "device/micas" / variant / "cpo.json"
        data = json.loads(path.read_text(encoding="utf-8"))
        for device in data["devices"].values():
            device["i2c_path"] = str(self.directory)
        for els in data["platform_data"]["devices"].values():
            els["els_presence"]["presence_file"] = str(self.presence)
            els["els_presence"]["presence_offset"] = "0x0"
        return data

    def make_chassis(self, data):
        self.device_info.get_cpo_data.return_value = data
        return self.chassis.Chassis()

    def endpoints(self, base_page=4, bank=7):
        oe = self.cpo.MicasOe(0, {"i2c_path": str(self.directory)}, bank, base_page)
        els_config = {"base_page": base_page, "els_presence": {
            "presence_file": str(self.presence), "presence_offset": "0x0",
            "presence_len": "4", "presence_bit": "1", "presence_value": "0"}}
        els = self.cpo.MicasElsfp(1, els_config, oe.get_eeprom_path())
        return oe, els

    def test_full_topology_uses_only_community_cpo_list(self):
        data = self.topology()
        with patch.object(self.chassis, "Sfp", side_effect=AssertionError("SFP created for pure CPO topology")) as sfp_constructor:
            chassis = self.make_chassis(data)
        sfp_constructor.assert_not_called()
        self.assertEqual(chassis._sfp_list, [])
        self.assertEqual(chassis.get_num_cpos(), 64)
        self.assertEqual(chassis.get_num_sfps(), 0)
        self.assertEqual(chassis.get_all_sfps(), [])
        self.assertEqual(len(chassis.get_all_cpos()), 64)
        self.assertIs(type(chassis).get_cpo, ChassisBase.get_cpo)
        chassis._validate_port_lists()
        self.assertFalse(hasattr(chassis, "_micas_cpo_list"))
        self.assertFalse(hasattr(self.cpo, "CPO"))
        self.assertIsNone(chassis.get_cpo(0))
        for name, config in data["interfaces"].items():
            port = int(self.device_info.get_platform_json_data.return_value["interfaces"][name]["index"].split(",")[0])
            cpo = chassis.get_cpo(port)
            self.assertIsInstance(cpo, CpoBase)
            self.assertIsInstance(cpo.oe, OeBase)
            self.assertIsInstance(cpo.elsfp, ElsfpBase)
            self.assertIs(chassis._cpo_list[port], cpo)
            self.assertIs(chassis.get_sfp(port), cpo)
            self.assertFalse(hasattr(cpo, "port"))
            self.assertFalse(hasattr(cpo.oe, "port"))
            self.assertFalse(hasattr(cpo.elsfp, "port"))
            self.assertEqual(cpo.get_name(), name)
            self.assertEqual(cpo.oe.bank, config["associated_devices"][0]["bank"])
            self.assertEqual(cpo.hardware_id.oe_id, OeId.BROADCOM_BAILLY)
            self.assertEqual(cpo.oe.base_page, data["platform_data"]["devices"][config["associated_devices"][1]["device_id"]]["base_page"])

    def test_hook_initializes_before_board_hal_without_double_construction(self):
        data = self.topology()
        original = self.cpo.MicasCpo
        mapping = self.chassis.Chassis._init_port_mappings
        ordering = []

        def record_order(obj):
            ordering.append(hasattr(obj, "int_case"))
            return mapping(obj)

        with patch.object(self.chassis, "MicasCpo", side_effect=original) as constructor, \
                patch.object(self.chassis.Chassis, "_init_port_mappings", record_order):
            chassis = self.make_chassis(data)
        self.assertEqual(constructor.call_count, 64)
        self.assertEqual(ordering, [False])
        self.assertTrue(chassis._cpo_initialized)

    def test_zero_based_physical_indices(self):
        data = self.topology()
        config = data["interfaces"]["Ethernet0"]
        self.device_info.get_platform_json_data.return_value["interfaces"]["Ethernet0"]["index"] = "0"
        data["interfaces"] = {"Ethernet0": config}
        chassis = self.make_chassis(data)
        self.assertEqual(chassis.get_num_cpos(), 1)
        self.assertEqual(chassis.get_cpo(0).get_name(), "Ethernet0")
        self.assertEqual(chassis.get_transceiver_change_event(), {0: "1"})

    def test_sparse_out_of_order_physical_indices(self):
        data = self.topology()
        source = data["interfaces"]
        data["interfaces"] = {"Ethernet16": source["Ethernet16"], "Ethernet0": source["Ethernet0"]}
        chassis = self.make_chassis(data)
        self.assertEqual(chassis.get_num_cpos(), 2)
        self.assertEqual(chassis.get_cpo(1).get_name(), "Ethernet0")
        self.assertIsNone(chassis.get_cpo(2))
        self.assertEqual(chassis._sfp_list, [])
        self.assertEqual(chassis.get_cpo(3).get_name(), "Ethernet16")
        self.assertEqual(chassis.get_transceiver_change_event(), {1: "1", 3: "1"})
        self.assertEqual(chassis.get_transceiver_change_event(), {})

    def test_mixed_sfp_and_cpo_lists_do_not_overlap(self):
        data = self.topology()
        data["interfaces"] = {"Ethernet0": data["interfaces"]["Ethernet0"],
                              "Ethernet8": {"associated_devices": []}}
        chassis = self.make_chassis(data)
        chassis._validate_port_lists()
        self.assertIs(chassis.get_sfp(1), chassis.get_cpo(1))
        self.assertIsNone(chassis._sfp_list[1])
        self.assertIsNone(chassis._cpo_list[2])
        self.assertEqual(chassis.get_num_sfps(), 1)
        self.assertEqual(chassis.get_all_cpos(), [chassis.get_cpo(1)])
        self.assertEqual(chassis.get_num_cpos(), 1)
        self.assertIsNone(chassis.get_cpo(2))
        self.assertIsInstance(chassis.get_sfp(2), FakeSfp)
        self.assertEqual(chassis.get_transceiver_change_event(), {1: "1", 2: "1"})

    def test_non_cpo_initialization_is_unchanged(self):
        chassis = self.make_chassis(None)
        self.assertEqual(chassis.get_num_cpos(), 0)
        self.assertIsNone(chassis.get_cpo(1))
        self.assertIsInstance(chassis.get_sfp(1), FakeSfp)
        self.assertEqual(chassis.get_transceiver_change_event(), {1: "1", 2: "1"})

    def test_non_cpo_get_sfp_uses_community_implementation(self):
        chassis = self.make_chassis(None)
        existing = chassis._sfp_list[1]
        with patch.object(chassis, "get_cpo", side_effect=AssertionError("CPO fallback on ordinary platform")):
            self.assertIs(chassis.get_sfp(1), existing)
            chassis._sfp_list[1] = None
            self.assertIsNone(chassis.get_sfp(1))

    def test_pure_cpo_get_sfp_bridge_never_accesses_empty_sfp_storage(self):
        chassis = self.make_chassis(self.topology())
        cpo = chassis.get_cpo(1)
        with patch.object(ChassisBase, "get_sfp", side_effect=AssertionError("empty SFP storage accessed")):
            self.assertIs(chassis.get_sfp(1), cpo)
            self.assertIsNone(chassis.get_sfp(0))
        self.assertEqual(chassis._sfp_list, [])

    def test_mixed_get_sfp_prefers_existing_sfp(self):
        data = self.topology()
        data["interfaces"] = {"Ethernet0": data["interfaces"]["Ethernet0"], "Ethernet8": {"associated_devices": []}}
        chassis = self.make_chassis(data)
        sfp = chassis._sfp_list[2]
        with patch.object(chassis, "get_cpo", side_effect=AssertionError("existing SFP replaced")):
            self.assertIs(chassis.get_sfp(2), sfp)

    def test_els_optoe_access_uses_shared_eeprom_offsets_directly(self):
        _, first = self.endpoints(0)
        _, second = self.endpoints(4)
        self.assertEqual(first.get_eeprom_path(), second.get_eeprom_path())
        # The raw driver access is independent of the API's selected RLM.
        for page in range(0xB0, 0xB8):
            offset = (page + 1) * 128
            payload = bytes([page]) * 128
            self.assertTrue(first.write_eeprom(offset, 128, payload))
            self.assertEqual(second.read_eeprom(offset, 128), payload)
        # A raw request crossing B3/B4 is passed through to Optoe too.
        self.assertTrue(second.write_eeprom(0x5A7F, 2, b"AB"))
        self.assertEqual(first.read_eeprom(0x5A7F, 2), b"AB")

    def test_presence_is_read_without_legacy_object(self):
        _, els = self.endpoints()
        self.assertTrue(els.get_presence())
        self.presence.write_bytes((2).to_bytes(4, "little"))
        self.assertFalse(els.get_presence())
        self.presence.write_bytes(bytes(1))
        self.assertFalse(els.get_presence())
        self.presence.unlink()
        self.assertFalse(els.get_presence())

    def test_real_factory_temperature_uses_selected_els_page(self):
        with self.eeprom.open("r+b") as fd:
            for page, value in [(0xB0, 25.5), (0xB4, 50.25)]:
                fd.seek(page * 128 + 150)
                fd.write(struct.pack(">h", int(value * 256)))
        for base_page, expected in [(0, 25.5), (4, 50.25)]:
            oe, els = self.endpoints(base_page)
            self.assertIsInstance(oe.get_api(), BaillyApi)
            self.assertIsInstance(els.get_api(), BaillyElsfpApi)
            self.assertEqual(els.get_api().get_module_temperature(), expected)

    def test_vmodule_controls_call_oe_api_and_not_els(self):
        oe, els = self.endpoints()
        api = MagicMock()
        api.get_max_supported_banks.return_value = 8
        api.get_lpmode.return_value = False
        api.set_lpmode.return_value = True
        api.reset.return_value = True
        api.get_tx_disable.return_value = [False] * 8
        api.tx_disable.return_value = True
        api.get_transceiver_info.return_value = {"manufacturer": "test"}
        oe._api = api
        cpo = self.cpo.MicasCpo(8, "Ethernet56", oe, els)
        with patch.object(els, "get_api", side_effect=AssertionError("ELS control used")):
            self.assertFalse(cpo.get_lpmode())
            self.assertTrue(cpo.set_lpmode(True))
            self.assertTrue(cpo.reset())
            self.assertEqual(cpo.get_tx_disable(), [False] * 8)
            self.assertTrue(cpo.tx_disable(True))
            self.assertEqual(cpo.get_transceiver_info(), {"manufacturer": "test"})
        api.set_lpmode.assert_called_once_with(True)
        api.reset.assert_called_once_with()
        api.tx_disable.assert_called_once_with(True)
        self.assertEqual(cpo.bank, 7)
        self.assertEqual((self.directory / "max_bank_size").read_text(), "8")
        with self.assertRaises(NotImplementedError):
            oe.set_lpmode(True)
        with self.assertRaises(NotImplementedError):
            els.reset()

    def test_missing_eeprom_retains_failure_returns(self):
        oe = self.cpo.MicasOe(0, {})
        self.assertIsNone(oe.read_eeprom(0, 1))
        self.assertFalse(oe.write_eeprom(0, 1, b"A"))
        self.assertFalse(oe.check_and_set_eeprom_bank_size())
        els = self.cpo.MicasElsfp(0, {}, "")
        start = (0xB0 + els.base_page + 1) * 128
        self.assertIsNone(els.read_eeprom(start, 1))
        self.assertFalse(els.write_eeprom(start, 1, b"A"))

    def test_oe_page_zero_read_restores_page_selector(self):
        oe, _ = self.endpoints()
        with self.eeprom.open("r+b") as fd:
            fd.seek(127)
            fd.write(b"\x07A")
        self.assertEqual(oe.read_eeprom(128, 1), b"A")
        self.assertEqual(oe.read_eeprom(127, 1), b"\x00")

    def test_els_rlm_read_does_not_write_oe_page_selector(self):
        _, els = self.endpoints()
        start = (0xB0 + els.base_page + 1) * 128
        with self.eeprom.open("r+b") as fd:
            fd.seek(127)
            fd.write(b"\x07")
            fd.seek(start)
            fd.write(b"Z")
        with patch.object(els, "write_eeprom", wraps=els.write_eeprom) as writes:
            self.assertEqual(els.read_eeprom(start, 1), b"Z")
        writes.assert_not_called()
        with self.eeprom.open("rb") as fd:
            fd.seek(127)
            self.assertEqual(fd.read(1), b"\x07")

    def test_endpoint_paths_are_owned_independently(self):
        oe, els = self.endpoints()
        other = self.directory / "other-eeprom"
        other.write_bytes(bytes(32768))
        start = (0xB0 + els.base_page + 1) * 128
        with other.open("r+b") as fd:
            fd.seek(start)
            fd.write(b"Z")
        els._eeprom_path = str(other)
        self.assertEqual(els.read_eeprom(start, 1), b"Z")
        self.assertEqual(oe.read_eeprom(start, 1), b"\x00")
        self.assertFalse(hasattr(self.cpo, "CpoEndpointMixin"))

    def test_shared_eeprom_write_targets_selected_oe_bank(self):
        from sonic_platform_base.sonic_xcvr.fields import consts
        first, _ = self.endpoints(bank=0)
        last, _ = self.endpoints(bank=7)
        field0 = first.get_api().xcvr_eeprom.mem_map.get_field(consts.TX_DISABLE_FIELD)
        field7 = last.get_api().xcvr_eeprom.mem_map.get_field(consts.TX_DISABLE_FIELD)
        self.assertNotEqual(field0.get_offset(), field7.get_offset())
        self.assertTrue(last.get_api().xcvr_eeprom.write(consts.TX_DISABLE_FIELD, 255))
        self.assertEqual(last.get_api().xcvr_eeprom.read(consts.TX_DISABLE_FIELD), 255)
        self.assertEqual(first.get_api().xcvr_eeprom.read(consts.TX_DISABLE_FIELD), 0)

    def test_dom_keeps_port_laser_fields_without_legacy_lookup(self):
        oe, els = self.endpoints()
        api = MagicMock()
        api.get_max_supported_banks.return_value = 1
        api.get_transceiver_dom_real_value.return_value = {"temperature": 40.0}
        api.get_rlm_monitor_values.return_value = {"els_temperature": 25.5}
        api.get_rlm_laser_current.return_value = {"Laser{}CurrentMonitor".format(i): float(i) for i in range(8)}
        api.get_rlm_laser_power.return_value = {"Laser{}OpticalPowerMonitor".format(i): 1.0 for i in range(8)}
        oe._api = api
        cpo = self.cpo.MicasCpo(1, "Ethernet0", oe, els, laser_ids=[4, 5])
        dom = cpo.get_transceiver_dom_real_value()
        self.assertEqual(dom["temperature"], 40.0)
        self.assertEqual(dom["els_temperature"], 25.5)
        self.assertEqual(dom["RLM1_Laser4_current"], "4.0 mA")
        self.assertEqual(dom["RLM1_Laser5_power"], "1.000 mW 0.000 dBm")
        self.assertNotIn("RLM1_Laser0_current", dom)

    def test_conflicting_physical_mapping_is_rejected(self):
        data = self.topology()
        first = copy.deepcopy(data["interfaces"]["Ethernet0"])
        second = copy.deepcopy(first)
        second["associated_devices"][0]["device_id"] = "OE1"
        self.device_info.get_platform_json_data.return_value["interfaces"]["Ethernet1"] = copy.deepcopy(
            self.device_info.get_platform_json_data.return_value["interfaces"]["Ethernet0"])
        data["interfaces"] = {"Ethernet0": first, "Ethernet1": second}
        with self.assertRaises(ValueError):
            self.make_chassis(data)


    def test_hld_reference_is_not_used_for_initialization(self):
        data = self.topology()
        data["legacy_bailly_reference"] = {"oes": {}, "elss": {}, "interfaces": {}}
        chassis = self.make_chassis(data)
        self.assertEqual(chassis.get_num_cpos(), 64)

    def test_missing_hld_devices_is_rejected(self):
        data = self.topology()
        del data["devices"]
        with self.assertRaisesRegex(ValueError, "non-empty devices"):
            self.make_chassis(data)

    def test_bank_is_not_silently_wrapped(self):
        data = self.topology()
        data["interfaces"]["Ethernet0"]["associated_devices"][0]["bank"] = 8
        with self.assertRaisesRegex(ValueError, "Invalid bank"):
            self.make_chassis(data)

    def test_precise_hld_lasers_override_coarse_platform_group(self):
        data = self.topology()
        data["interfaces"] = {"Ethernet0": data["interfaces"]["Ethernet0"]}
        data["devices"]["ELS0"]["laser_to_asic_lane_mapping"] = {"1": "1,2,3,4", "2": [5, 6, 7, 8]}
        chassis = self.make_chassis(data)
        self.assertEqual(chassis.get_cpo(1).get_port_laser_ids(), [0, 1])


if __name__ == "__main__":
    unittest.main()
