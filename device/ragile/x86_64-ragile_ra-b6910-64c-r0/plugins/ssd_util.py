#
# ssd_util.py
#
# Generic implementation of the SSD health API
# SSD models supported:
#  - InnoDisk
#  - StorFly
#  - Virtium

try:
    import re
    import os
    import subprocess
    from sonic_platform_base.sonic_ssd.ssd_base import SsdBase
except ImportError as e:
    raise ImportError (str(e) + "- required module not found")

SMARTCTL = "smartctl {} -a"
INNODISK = "iSmart -d {}"
VIRTIUM  = "SmartCmd -m {}"
DISK_LIST_CMD = "fdisk -l -o Device"
DISK_FREE_CMD = "df -h"
MOUNT_CMD = "mount"

NOT_AVAILABLE = "N/A"
PE_CYCLE = 3000
FAIL_PERCENT = 95

# Set Vendor Specific IDs
INNODISK_HEALTH_ID = 169
INNODISK_TEMPERATURE_ID = 194

class SsdUtil(SsdBase):
    """
    Generic implementation of the SSD health API
    """
    model = NOT_AVAILABLE
    serial = NOT_AVAILABLE
    firmware = NOT_AVAILABLE
    temperature = NOT_AVAILABLE
    health = NOT_AVAILABLE
    remaining_life = NOT_AVAILABLE
    sata_rate = NOT_AVAILABLE
    ssd_info = NOT_AVAILABLE
    vendor_ssd_info = NOT_AVAILABLE

    def __init__(self, diskdev):
        self.vendor_ssd_utility = {
            "Generic"  : { "utility" : SMARTCTL, "parser" : self.parse_generic_ssd_info },
            "InnoDisk" : { "utility" : INNODISK, "parser" : self.parse_innodisk_info },
            "M.2"      : { "utility" : INNODISK, "parser" : self.parse_innodisk_info },
            "StorFly"  : { "utility" : VIRTIUM,  "parser" : self.parse_virtium_info },
            "Virtium"  : { "utility" : VIRTIUM,  "parser" : self.parse_virtium_info }
        }

        """
        The dict model_attr keys relate the vendors
        LITEON : "ER2-GD","AF2MA31DTDLT"
        Intel  : "SSDSCKKB"
        SMI    : "SM619GXC"
        samsung: "MZNLH"
        ADATA  : "IM2S3134N"
        """
        self.model_attr = {
             "ER2-GD"       : { "temperature" : "\n190\s+(.+?)\n", "remainingLife" : "\n202\s+(.+?)\n" },
             "AF2MA31DTDLT" : { "temperature" : "\n194\s+(.+?)\n", "remainingLife" : "\n202\s+(.+?)\n" },
             "SSDSCK"       : { "temperature" : "\n194\s+(.+?)\n", "remainingLife" : "\n233\s+(.+?)\n" },
             "SM619GXC"     : { "temperature" : "\n194\s+(.+?)\n", "remainingLife" : "\n169\s+(.+?)\n" },
             "MZNLH"        : { "temperature" : "\n190\s+(.+?)\n", "remainingLife" : "\n245\s+(.+?)\n" },
             "IM2S3134N"    : { "temperature" : "\n194\s+(.+?)\n", "remainingLife" : "\n231\s+(.+?)\n" }
        }

        self.key_list = list(self.model_attr.keys())
        self.attr_info_rule = "[\s\S]*SMART Attributes Data Structure revision number: 1|SMART Error Log Version[\s\S]*"
        self.dev = diskdev
        # Generic part
        self.fetch_generic_ssd_info(diskdev)
        self.parse_generic_ssd_info()
        self.fetch_vendor_ssd_info(diskdev, "Generic")
        
        # Known vendor part
        if self.model:
            model_short = self.model.split()[0]
            if model_short in self.vendor_ssd_utility:
                self.fetch_vendor_ssd_info(diskdev, model_short)
                self.parse_vendor_ssd_info(model_short)
            else:
                # No handler registered for this disk model
                pass
        else:
            # Failed to get disk model
            self.model = "Unknown"

    def _execute_shell(self, cmd):
        process = subprocess.Popen(cmd.split(), universal_newlines=True, stdout=subprocess.PIPE)
        output, error = process.communicate()
        exit_code = process.returncode
        if exit_code:
            return None
        return output

    def _parse_re(self, pattern, buffer):
        res_list = re.findall(pattern, str(buffer))
        return res_list[0] if res_list else NOT_AVAILABLE

    def fetch_generic_ssd_info(self, diskdev):
        self.ssd_info = self._execute_shell(self.vendor_ssd_utility["Generic"]["utility"].format(diskdev))

    # Health and temperature values may be overwritten with vendor specific data
    def parse_generic_ssd_info(self):
        if "nvme" in self.dev:
            self.model = self._parse_re('Model Number:\s*(.+?)\n', self.ssd_info)

            health_raw = self._parse_re('Percentage Used\s*(.+?)\n', self.ssd_info)
            if health_raw == NOT_AVAILABLE:
                self.health = NOT_AVAILABLE
            else:
                health_raw = health_raw.split()[-1]
                self.health = 100 - float(health_raw.strip('%'))

            temp_raw = self._parse_re('Temperature\s*(.+?)\n', self.ssd_info)
            if temp_raw == NOT_AVAILABLE:
                self.temperature = NOT_AVAILABLE
            else:
                temp_raw = temp_raw.split()[-2]
                self.temperature = float(temp_raw)
        else:
            self.model = self._parse_re('Device Model:\s*(.+?)\n', self.ssd_info)
            model_key = ""
            for key in self.key_list:
                if re.search(key, self.model):
                    model_key = key
                    break
            if  model_key != "":
                self.remaining_life = self._parse_re(self.model_attr[model_key]["remainingLife"], re.sub(self.attr_info_rule,"",self.ssd_info)).split()[2]
                self.temperature = self._parse_re(self.model_attr[model_key]["temperature"], re.sub(self.attr_info_rule,"",self.ssd_info)).split()[8]
                self.health = self.remaining_life
            # Get the LITEON ssd health value by (PE CYCLE - AVG ERASE CYCLE )/(PE CYCLE)
            if model_key in ["ER2-GD", "AF2MA31DTDLT"]:
                avg_erase = int(self._parse_re('\n173\s+(.+?)\n' ,re.sub(self.attr_info_rule,"",self.ssd_info)).split()[-1])
                self.health = int(round((PE_CYCLE - avg_erase)/PE_CYCLE*100,0))
            if self.remaining_life != NOT_AVAILABLE and  int(self.remaining_life) < FAIL_PERCENT:
                self.remaining_life = "Fail"
        self.sata_rate = self._parse_re('SATA Version is:.*current: (.+?)\)\n', self.ssd_info)
        self.serial = self._parse_re('Serial Number:\s*(.+?)\n', self.ssd_info)
        self.firmware = self._parse_re('Firmware Version:\s*(.+?)\n', self.ssd_info)

    def parse_innodisk_info(self):
        if self.vendor_ssd_info:
            self.health = self._parse_re('Health:\s*(.+?)%', self.vendor_ssd_info)
            self.temperature = self._parse_re('Temperature\s*\[\s*(.+?)\]', self.vendor_ssd_info)
        else:
            if self.health == NOT_AVAILABLE:
                health_raw = self.parse_id_number(INNODISK_HEALTH_ID)
                self.health = health_raw.split()[-1]
            if self.temperature == NOT_AVAILABLE:
                temp_raw = self.parse_id_number(INNODISK_TEMPERATURE_ID)
                self.temperature = temp_raw.split()[-6]

    def parse_virtium_info(self):
        if self.vendor_ssd_info:
            self.temperature = self._parse_re('Temperature_Celsius\s*\d*\s*(\d+?)\s+', self.vendor_ssd_info)
            nand_endurance = self._parse_re('NAND_Endurance\s*\d*\s*(\d+?)\s+', self.vendor_ssd_info)
            avg_erase_count = self._parse_re('Average_Erase_Count\s*\d*\s*(\d+?)\s+', self.vendor_ssd_info)
            try:
                self.health = 100 - (float(avg_erase_count) * 100 / float(nand_endurance))
            except (ValueError, ZeroDivisionError):
                # Invalid avg_erase_count or nand_endurance.
                pass

    def fetch_vendor_ssd_info(self, diskdev, model):
        self.vendor_ssd_info = self._execute_shell(self.vendor_ssd_utility[model]["utility"].format(diskdev))

    def parse_vendor_ssd_info(self, model):
        self.vendor_ssd_utility[model]["parser"]()

    def check_readonly2(self, partition, filesystem):
        # parse mount cmd output info
        mount_info = self._execute_shell(MOUNT_CMD)
        for line in mount_info.split('\n'):
            column_list = line.split()
            if line == '':
                continue
            if column_list[0] == partition and column_list[2] == filesystem:
                if column_list[5].split(',')[0][1:] == "ro":
                    return partition
                else:
                    return NOT_AVAILABLE
        return NOT_AVAILABLE

    def check_readonly(self, partition, filesystem):
        ret = os.access(filesystem, os.W_OK)
        if ret == False:
            return partition
        else:
            return NOT_AVAILABLE
        
    def get_health(self):
        """
        Retrieves current disk health in percentages

        Returns:
            A float number of current ssd health
            e.g. 83.5
        """
        return float(self.health)

    def get_temperature(self):
        """
        Retrieves current disk temperature in Celsius

        Returns:
            A float number of current temperature in Celsius
            e.g. 40.1
        """
        return float(self.temperature)

    def get_model(self):
        """
        Retrieves model for the given disk device

        Returns:
            A string holding disk model as provided by the manufacturer
        """
        return self.model

    def get_firmware(self):
        """
        Retrieves firmware version for the given disk device

        Returns:
            A string holding disk firmware version as provided by the manufacturer
        """
        return self.firmware

    def get_serial(self):
        """
        Retrieves serial number for the given disk device

        Returns:
            A string holding disk serial number as provided by the manufacturer
        """
        return self.serial
    def get_sata_rate(self):
        """
        Retrieves SATA rate for the given disk device
        Returns:
            A string holding current SATA rate as provided by the manufacturer
        """
        return self.sata_rate
    def get_remaining_life(self):
        """
        Retrieves remaining life for the given disk device
        Returns:
            A string holding disk remaining life as provided by the manufacturer
        """
        return self.remaining_life
    def get_vendor_output(self):
        """
        Retrieves vendor specific data for the given disk device

        Returns:
            A string holding some vendor specific disk information
        """
        return self.vendor_ssd_info

    def parse_id_number(self, id):
        return self._parse_re('{}\s*(.+?)\n'.format(id), self.ssd_info)

    def get_readonly_partition(self):
        """
        Check the partition mount filesystem is readonly status,then output the result.
        Returns:
            The readonly partition list
        """

        ro_partition_list = []
        partition_list = []

        # parse fdisk cmd output info
        disk_info = self._execute_shell(DISK_LIST_CMD)
        begin_flag = False
        for line in disk_info.split('\n'):
            if line == "Device":
                begin_flag = True
                continue
            if begin_flag:
                if line != "":
                    partition_list.append(line)
                else:
                    break

        # parse df cmd output info
        disk_free = self._execute_shell(DISK_FREE_CMD)
        disk_dict = {}
        line_num = 0
        for line in disk_free.split('\n'):
            line_num = line_num + 1
            if line_num == 1 or line == "":
                continue
            column_list = line.split()
            disk_dict[column_list[0]] = column_list[5]

        # get partition which is readonly
        for partition in partition_list:
            if partition in disk_dict:
                ret = self.check_readonly(partition, disk_dict[partition])
                if (ret != NOT_AVAILABLE):
                    ro_partition_list.append(ret)

        return ro_partition_list
