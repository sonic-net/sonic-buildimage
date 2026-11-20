"""AS5835 dual-rotor fan drawers using PDDF."""
from sonic_platform_pddf_base.pddf_fan_drawer import PddfFanDrawer


class FanDrawer(PddfFanDrawer):
    def get_status(self):
        return all(fan.get_status() for fan in self._fan_list)
