#!/usr/bin/env python


try:
    from sonic_platform_pddf_base.pddf_fan import PddfFan
    import subprocess
    import re
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

switch_sku = {
  "0K6MG9":('AC', 'Exhaust'),
  "0GKK8W":('AC', 'Intake'),
  "0VK93C":('AC', 'Exhaust'),
  "05JHDM":('AC', 'Intake'),
  "0D72R7":('AC', 'Exhaust'),
  "02PC9F":('AC', 'Exhaust'),
  "0JM5DX":('AC', 'Intake'),
  "0TPDP8":('AC', 'Exhaust'),
  "0WND1V":('AC', 'Exhaust'),
  "05672M":('DC', 'Intake'),
  "0CJV4K":('DC', 'Intake'),
  "0X41RN":('AC', 'Exhaust'),
  "0Y3N82":('AC', 'Intake'),
  "0W4CMG":('DC', 'Exhaust'),
  "04T94Y":('DC', 'Intake'),
  "0MYYKP":('AC', 'exhaust'),
  "061C11":('AC', 'intake'),
  "03C99H":('AC', 'intake'),
  "0MTYT2":('AC', 'exhaust')
}

class Fan(PddfFan):
    """PDDF Platform-Specific Fan class"""

    def __init__(self, tray_idx, fan_idx=0, pddf_data=None, pddf_plugin_data=None, is_psu_fan=False, psu_index=0):
        # idx is 0-based 
        PddfFan.__init__(self, tray_idx, fan_idx, pddf_data, pddf_plugin_data, is_psu_fan, psu_index)

    # Provide the functions/variables below for which implementation is to be overwritten e.g.
    def get_speed_tolerance(self):
        """
        Retrieves the speed tolerance of the fan

        Returns:
            An integer, the percentage of variance from target speed which is
            considered tolerable
        """
        return 25

    def set_speed(self, speed):
        print("Setting fan speed is not allowed")
        return False

    def get_direction(self):
        """
        Retrieves the fan airfow direction
        Returns:
            A string, either FAN_DIRECTION_INTAKE or FAN_DIRECTION_EXHAUST
            depending on fan direction

        Notes:
            In DellEMC platforms,
            - Forward/Exhaust : Air flows from Port side to Fan side.
            - Reverse/Intake  : Air flows from Fan side to Port side.
        """
        direction='Unknown'
        result = ""
        info = "Board Part Number"
        try:
            status, output = subprocess.getstatusoutput('/usr/bin/ipmitool fru')
            if not status:
                board_info = re.search(r"%s\s*:(.*)" % info, output)
                board_info = board_info.group(1).strip()
                if board_info is not None:
                    board_part_no = board_info[0:6]
                    if board_part_no in switch_sku:
                        direction = switch_sku[board_part_no][1]
        except:
             pass

        return direction

    def get_presence(self):
        """
        Retrieves the presence of the FAN
        Returns:
            bool: True if fan is present, False if not
        """
        # In S5212F, Fans are fixed.
        return True

    def get_model(self):
        """
        Retrieves the part number of the FAN
        Returns:
            String: Part number of FAN
        """
        return 'NA'

    def get_serial(self):
        """
        Retrieves the serial number of the FAN
        Returns:
            String: Serial number of FAN
        """
        return 'NA'
