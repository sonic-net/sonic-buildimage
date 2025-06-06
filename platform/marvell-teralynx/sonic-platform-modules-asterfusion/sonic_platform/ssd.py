############################################################################
# Asterfusion CX-N Devices SSD                                             #
#                                                                          #
# Platform and model specific ssd subclass, inherits from the base class,  #
# provides the ssd info which are available in the platform                #
#                                                                          #
############################################################################

try:
    from .constants import *
    from .helper import Helper
    from .logger import Logger

    from sonic_platform_base.sonic_ssd.ssd_generic import SsdUtil
except ImportError as err:
    raise ImportError(str(err) + "- required module not found")


class Ssd(SsdUtil):
    """Platform-specific Ssd class"""

    def __init__(self, diskdev):
        self._helper = Helper()
        self._logger = Logger()
        # fmt: off
        self.vendor_ssd_utility = {
            "Generic"  : { "utility" : "smartctl {} -a", "parser" : self.parse_generic_ssd_info },
            "SATA"     : { "utility" : "smartctl {} -a", "parser" : self.parse_generic_ssd_info },
            "InnoDisk" : { "utility" : "iSmart -d {}",   "parser" : self.parse_innodisk_info },
            "M.2"      : { "utility" : "iSmart -d {}",   "parser" : self.parse_innodisk_info },
            "StorFly"  : { "utility" : "SmartCmd -m {}", "parser" : self.parse_virtium_info },
            "Virtium"  : { "utility" : "SmartCmd -m {}", "parser" : self.parse_virtium_info }
        }
        # fmt: on

        self.dev = diskdev
        # Generic part
        self.fetch_generic_ssd_info(diskdev)
        self.parse_generic_ssd_info()
        # Vendor part
        model_short = self.model.split()[0]
        self.fetch_vendor_ssd_info(diskdev, model_short)
        self.parse_vendor_ssd_info(model_short)

    def fetch_generic_ssd_info(self, diskdev):
        self.ssd_info = self._execute_shell(
            self.vendor_ssd_utility["Generic"]["utility"].format(diskdev)
        )

    def fetch_vendor_ssd_info(self, diskdev, model):
        self.vendor_ssd_info = self._execute_shell(
            self.vendor_ssd_utility.get(model, self.vendor_ssd_utility["Generic"])[
                "utility"
            ].format(diskdev)
        )

    # Health and temperature values may be overwritten with vendor specific data
    def parse_generic_ssd_info(self):
        header = self._parse_re(r"ID#.*RAW_VALUE", self.ssd_info)
        flag_index = header.find("FLAG")
        raw_value_index = header.find("RAW_VALUE")
        if all(index != -1 for index in (flag_index, raw_value_index)):
            value_index = raw_value_index - flag_index
        else:
            value_index = 0
        if "nvme" in self.dev:
            self.model = self._parse_re(r"Model Number:\s*(.+?)\n", self.ssd_info)

            health_raw = self._parse_re(r"Percentage Used\s*(.+?)\n", self.ssd_info)
            if health_raw != NOT_AVAILABLE:
                if value_index != 0:
                    health_raw = health_raw[value_index:].split()[0]
                else:
                    health_raw = health_raw.split()[-1]
                self.health = 100 - float(health_raw.rstrip("%"))

            temp_raw = self._parse_re(r"Temperature\s*(.+?)\n", self.ssd_info)
            if temp_raw != NOT_AVAILABLE:
                if value_index != 0:
                    temp_raw = temp_raw[value_index:].split()[0]
                else:
                    temp_raw = temp_raw.split()[-2]
                self.temperature = float(temp_raw)
        else:
            self.model = self._parse_re(r"Device Model:\s*(.+?)\n", self.ssd_info)

            health_raw = self._parse_re(
                r"Remaining_Lifetime_Perc\s*(.+?)\n", self.ssd_info
            )
            if health_raw != NOT_AVAILABLE:
                if value_index != 0:
                    self.health = health_raw[value_index:].split()[0]
                else:
                    self.health = health_raw.split()[-1]

            temp_raw = self._parse_re(r"Temperature_Celsius\s*(.+?)\n", self.ssd_info)
            if temp_raw != NOT_AVAILABLE:
                if value_index != 0:
                    self.temperature = temp_raw[value_index:].split()[0]
                else:
                    self.temperature = temp_raw.split()[-3]

        self.serial = self._parse_re(r"Serial Number:\s*(.+?)\n", self.ssd_info)
        self.firmware = self._parse_re(r"Firmware Version:\s*(.+?)\n", self.ssd_info)

    def parse_innodisk_info(self):
        self.health = self._parse_re(r"Health:\s+(\d*\.?\d+)%?", self.vendor_ssd_info)
        self.temperature = self._parse_re(
            r"Temperature\s*\[\s*(.+?)\]", self.vendor_ssd_info
        )

    def parse_virtium_info(self):
        self.temperature = self._parse_re(
            r"Temperature_Celsius\s*\d*\s*(\d+?)\s+", self.vendor_ssd_info
        )
        nand_endurance = self._parse_re(
            r"NAND_Endurance\s*\d*\s*(\d+?)\s+", self.vendor_ssd_info
        )
        avg_erase_count = self._parse_re(
            r"Average_Erase_Count\s*\d*\s*(\d+?)\s+", self.vendor_ssd_info
        )
        try:
            self.health = 100 - (float(avg_erase_count) * 100 / float(nand_endurance))
        except ValueError:
            pass

    def parse_vendor_ssd_info(self, model):
        if self.vendor_ssd_info:
            self.vendor_ssd_utility.get(model, self.vendor_ssd_utility["Generic"])[
                "parser"
            ]()
