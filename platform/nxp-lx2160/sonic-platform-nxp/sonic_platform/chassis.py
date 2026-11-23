# LX2160ARDB chassis: the board itself. It has no fans, PSUs or optics under
# SONiC's control (an external BMC/switch board would own fans and PSUs);
# what it does have is a system EEPROM (built in software, see eeprom.py),
# temperature sensors (thermal.py) and the SoC watchdog (watchdog.py).
from sonic_platform_base.chassis_base import ChassisBase
from sonic_platform.eeprom import Eeprom
from sonic_platform.thermal import THERMALS, Thermal
from sonic_platform.watchdog import Watchdog

MODEL = "LX2160ARDB"


class Chassis(ChassisBase):
    def __init__(self):
        ChassisBase.__init__(self)
        self._eeprom = Eeprom()
        self._thermal_list = [Thermal(i) for i in range(len(THERMALS))]
        self._watchdog = Watchdog()

    def get_name(self):
        return MODEL

    def get_model(self):
        return self._eeprom.modelstr()

    def get_serial(self):
        return self._eeprom.serial_number_str()

    def get_base_mac(self):
        return self._eeprom.base_mac_addr()

    def get_system_eeprom_info(self):
        return self._eeprom.get_eeprom_dict()

    def get_presence(self):
        return True

    def get_status(self):
        return True

    def get_revision(self):
        return "B"

    def is_replaceable(self):
        return False

    def get_position_in_parent(self):
        return -1

    # No system LED under SONiC's control on the reference board: keep the
    # state system-health sets, so it can be read back.
    def initizalize_system_led(self):  # (sic) name used by show system-health
        return True

    def set_status_led(self, color):
        self._status_led = color
        return True

    def get_status_led(self):
        return getattr(self, "_status_led", self.STATUS_LED_COLOR_OFF)

    def get_reboot_cause(self):
        # No reset-cause register is read yet; SONiC falls back to its
        # software reboot-cause record.
        return (self.REBOOT_CAUSE_NON_HARDWARE, None)
