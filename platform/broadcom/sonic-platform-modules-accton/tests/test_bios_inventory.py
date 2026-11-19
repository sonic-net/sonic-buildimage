"""Read-only PDDF BIOS inventory on AS7326, AS7726 and AS7816."""

import json
import sys
import unittest
from unittest.mock import Mock

from platform_test_utils import DEVICE, SOURCE, definitions, platform_directory

sys.path.insert(0, str(SOURCE / "platform/pddf/platform-api-pddf-base"))


class BiosInventory(unittest.TestCase):
    model = "as7326_56x"

    def test_reports_real_version_and_unavailable_read_without_update_commands(self):
        from sonic_platform_pddf_base.pddf_component import PddfComponent
        cls = definitions(platform_directory(self.model) + 'component.py',
                          PddfComponent=PddfComponent)['Component']
        config = json.loads((SOURCE / (DEVICE + self.model + '-r0/pddf/pddf-device.json')).read_text())
        self.assertEqual(config['PLATFORM']['num_components'], 1)
        pddf = Mock(data=config)
        pddf.get_cmd_output.return_value = Mock(returncode=0, stdout=b'v40.01.01.00\n')
        component = cls(0, pddf, {'platform': 'test'})
        self.assertEqual(component.get_name(), 'BIOS')
        self.assertEqual(component.get_firmware_version(), 'v40.01.01.00')
        self.assertTrue(component.get_status())
        self.assertTrue(component.get_presence())
        self.assertFalse(component.is_replaceable())
        self.assertEqual(component.get_position_in_parent(), 1)
        pddf.get_cmd_output.assert_called_with('cat /sys/class/dmi/id/bios_version')
        self.assertIsNone(component._get_cmd('update'))
        pddf.get_cmd_output.return_value = Mock(returncode=1)
        self.assertEqual(component.get_firmware_version(), 'Unknown')
        self.assertFalse(component.get_status())
        pddf.runcmd.assert_not_called()


class As7726BiosInventory(BiosInventory):
    model = "as7726_32x"


class As7816BiosInventory(BiosInventory):
    model = "as7816_64x"
