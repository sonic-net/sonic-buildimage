"""Micas Bailly factory selection and community API lifecycle."""
import importlib.util
import os
from pathlib import Path
import unittest

from sonic_platform_base.sonic_xcvr.api.broadcom.bailly import BaillyApi
from sonic_platform_base.sonic_xcvr.api.broadcom.bailly_elsfp import BaillyElsfpApi
from sonic_platform_base.sonic_xcvr.api.broadcom.davisson_oe import DavissonTh6OeApi
from sonic_platform_base.sonic_xcvr.api.broadcom.davisson_elsfp import DavissonTh6ElsfpApi
from sonic_platform_base.sonic_xcvr.cpo.cpo_base import CpoHardwareInfo, OeId
from sonic_platform_base.sonic_xcvr.cpo.oe import OeBase
from sonic_platform_base.sonic_xcvr.cpo.elsfp import ElsfpBase
from sonic_platform_base.sonic_xcvr.fields import consts


ROOT = Path(os.environ.get("CPO_DRAFT_REPO", Path(__file__).resolve().parents[5]))
spec = importlib.util.spec_from_file_location(
    "micas_factory_under_test",
    ROOT / "platform/broadcom/sonic-platform-modules-micas/common/sonic_platform/cpo.py")
micas = importlib.util.module_from_spec(spec)
spec.loader.exec_module(micas)


class MemoryAccess:
    def read_eeprom(self, offset, size):
        return bytearray(self.data[offset:offset + size])

    def write_eeprom(self, offset, size, buffer):
        self.data[offset:offset + size] = buffer[:size]
        return True


class CommonMemoryOe(MemoryAccess, OeBase):
    def __init__(self, hardware_info, bank=0, base_page=0):
        self.base_page = base_page
        self.data = bytearray(1024 * 1024)
        super().__init__(hardware_info, bank=bank)


class CommonMemoryElsfp(MemoryAccess, ElsfpBase):
    def __init__(self, hardware_info, base_page=0):
        self.base_page = base_page
        self.data = bytearray(32768)
        self.reads = []
        super().__init__(hardware_info)

    def read_eeprom(self, offset, size):
        self.reads.append((offset, size))
        return super().read_eeprom(offset, size)


class BaillyMemoryOe(MemoryAccess, micas.Oe):
    def __init__(self, bank=0, base_page=0):
        self.data = bytearray(1024 * 1024)
        super().__init__(0, {}, bank=bank, base_page=base_page)


class BaillyMemoryElsfp(MemoryAccess, micas.Elsfp):
    def __init__(self, base_page=0):
        self.data = bytearray(32768)
        self.reads = []
        super().__init__(0, {"base_page": base_page}, "")

    def read_eeprom(self, offset, size):
        self.reads.append((offset, size))
        return super().read_eeprom(offset, size)


class TestBaillyCpoFactory(unittest.TestCase):
    def test_hardware_ids_preserve_davisson_value(self):
        self.assertEqual(OeId.BROADCOM_DAVISSON.value, 1)
        self.assertEqual(OeId.BROADCOM_BAILLY.value, 2)

    def test_bailly_oe_bank_and_els_page(self):
        oe = BaillyMemoryOe(bank=7, base_page=4)
        api = oe.get_api()
        self.assertIsInstance(api, BaillyApi)
        self.assertEqual(api.xcvr_eeprom.mem_map.bank, 7)
        self.assertIn(0xB4, [page.page for page in api.xcvr_eeprom.mem_map.pages])
        field = api.xcvr_eeprom.mem_map.get_field(consts.TX_DISABLE_FIELD)
        other = BaillyMemoryOe(bank=0, base_page=4).get_api().xcvr_eeprom.mem_map.get_field(consts.TX_DISABLE_FIELD)
        self.assertNotEqual(field.get_offset(), other.get_offset())

    def test_bailly_els_preserves_identification_and_rlm_pages(self):
        els = BaillyMemoryElsfp(base_page=4)
        api = els.get_api()
        self.assertIsInstance(api, BaillyElsfpApi)
        self.assertEqual(els.reads, [(129, 16), (148, 16)])
        self.assertEqual(api.xcvr_eeprom.mem_map.base_page, 4)
        self.assertEqual([page.page for page in api.xcvr_eeprom.mem_map.pages], [0xB4, 0xB5, 0xB6])
        with self.assertRaises(KeyError):
            api.xcvr_eeprom.mem_map.get_field(consts.ID_FIELD)

    def test_api_cache_refresh_and_remove(self):
        oe = BaillyMemoryOe()
        first = oe.get_api()
        self.assertIs(oe.get_api(), first)
        oe.refresh_api()
        self.assertIsNot(oe.get_api(), first)
        refreshed = oe.get_api()
        oe.remove_api()
        self.assertIsNot(oe.get_api(), refreshed)

    def test_davisson_factories_still_select_davisson(self):
        info = CpoHardwareInfo(OeId.BROADCOM_DAVISSON, None)
        self.assertIsInstance(CommonMemoryOe(info).get_api(), DavissonTh6OeApi)
        self.assertIsInstance(CommonMemoryElsfp(info).get_api(), DavissonTh6ElsfpApi)

    def test_unknown_hardware_still_fails(self):
        info = CpoHardwareInfo(None, None)
        with self.assertRaises(ValueError):
            CommonMemoryOe(info).get_api()
        with self.assertRaises(ValueError):
            CommonMemoryElsfp(info).get_api()

    def test_common_default_factories_select_bailly(self):
        info = CpoHardwareInfo(OeId.BROADCOM_BAILLY, None)
        self.assertIsInstance(CommonMemoryOe(info, bank=7, base_page=4).get_api(), BaillyApi)
        self.assertIsInstance(CommonMemoryElsfp(info, base_page=4).get_api(), BaillyElsfpApi)


if __name__ == "__main__":
    unittest.main()
