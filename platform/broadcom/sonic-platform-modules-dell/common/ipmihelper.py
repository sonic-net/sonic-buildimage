#!/usr/bin/python3

########################################################################
# DellEMC
#
# Module contains implementation of IpmiSensor and IpmiFru classes that
# provide Sensor's and FRU's information respectively.
#
########################################################################

import subprocess
import re

# IPMI Request Network Function Codes
NetFn_SensorEvent = 0x04
NetFn_Storage = 0x0A

# IPMI Sensor Device Commands
Cmd_GetSensorReadingFactors = 0x23
Cmd_GetSensorThreshold = 0x27
Cmd_GetSensorReading = 0x2D

# IPMI FRU Device Commands
Cmd_ReadFRUData = 0x11

def get_ipmitool_raw_output(args):
    """
    Returns a list the elements of which are the individual bytes of
    ipmitool raw <cmd> command output.
    """
    result_bytes = list()
    result = ""
    command = "ipmitool raw {}".format(args)
    try:
        proc = subprocess.Popen(command.split(), stdout=subprocess.PIPE,
                                universal_newlines=True, stderr=subprocess.STDOUT)
        stdout = proc.communicate()[0]
        proc.wait()
        if not proc.returncode:
            result = stdout.rstrip('\n')
    except EnvironmentError:
        pass

    for i in result.split():
        result_bytes.append(int(i, 16))

    return result_bytes

class IpmiSensor(object):

    # Sensor Threshold types and their respective bit masks
    THRESHOLD_BIT_MASK = {
        "LowerNonCritical"    : 0,
        "LowerCritical"       : 1,
        "LowerNonRecoverable" : 2,
        "UpperNonCritical"    : 3,
        "UpperCritical"       : 4,
        "UpperNonRecoverable" : 5
    }

    def __init__(self, sensor_id, is_discrete=False):
        self.id = sensor_id
        self.is_discrete = is_discrete

    def _get_converted_sensor_reading(self, raw_value):
        """
        Returns a 2 element tuple(bool, int) in which first element
        provides the validity of the reading and the second element is
        the converted sensor reading
        """
        # Get Sensor Reading Factors
        cmd_args = "{} {} {} {}".format(NetFn_SensorEvent,
                                        Cmd_GetSensorReadingFactors,
                                        self.id, raw_value)
        factors = get_ipmitool_raw_output(cmd_args)

        if len(factors) != 7:
            return False, 0

        # Compute Twos complement
        def get_twos_complement(val, bits):
            if val & (1 << (bits - 1)):
                val = val - (1 << bits)
            return val

        # Calculate actual sensor value from the raw sensor value
        # using the sensor reading factors.
        M = get_twos_complement(((factors[2] & 0xC0) << 8) | factors[1], 10)
        B = get_twos_complement(((factors[4] & 0xC0) << 8) | factors[3], 10)
        R_exp = get_twos_complement((factors[6] & 0xF0) >> 4, 4)
        B_exp = get_twos_complement(factors[6] & 0x0F, 4)

        if R_exp < 0:
            converted_reading = ((M * raw_value) + (B * 10**B_exp)) / 10**(-R_exp)
        else:
            converted_reading = ((M * raw_value) + (B * 10**B_exp)) * 10**R_exp

        return True, converted_reading

    def get_reading(self):
        """
        For Threshold sensors, returns the sensor reading.
        For Discrete sensors, returns the state value.

        Returns:
            A tuple (bool, int) where the first element provides the
            validity of the reading and the second element provides the
            sensor reading/state value.
        """
        # Get Sensor Reading
        cmd_args = "{} {} {}".format(NetFn_SensorEvent, Cmd_GetSensorReading,
                                     self.id)
        output = get_ipmitool_raw_output(cmd_args)
        if len(output) != 4:
            return False, 0

        # Check reading/state unavailable
        if output[1] & 0x20:
            return False, 0

        if self.is_discrete:
            state = ((output[3] & 0x7F) << 8) | output[2]
            return True, state
        else:
            return self._get_converted_sensor_reading(output[0])

    def get_threshold(self, threshold_type):
        """
        Returns the sensor's threshold value for a given threshold type.

        Args:
            threshold_type (str) - one of the below mentioned
                                   threshold type strings

                "LowerNonCritical"
                "LowerCritical"
                "LowerNonRecoverable"
                "UpperNonCritical"
                "UpperCritical"
                "UpperNonRecoverable"
        Returns:
            A tuple (bool, int) where the first element provides the
            validity of that threshold and second element provides the
            threshold value.
        """
        # Thresholds are not valid for discrete sensors
        if self.is_discrete:
            raise TypeError("Threshold is not applicable for Discrete Sensor")

        if threshold_type not in list(self.THRESHOLD_BIT_MASK.keys()):
            raise ValueError("Invalid threshold type {} provided. Valid types "
                             "are {}".format(threshold_type,
                                             list(self.THRESHOLD_BIT_MASK.keys())))

        bit_mask = self.THRESHOLD_BIT_MASK[threshold_type]

        # Get Sensor Threshold
        cmd_args = "{} {} {}".format(NetFn_SensorEvent, Cmd_GetSensorThreshold,
                                     self.id)
        thresholds = get_ipmitool_raw_output(cmd_args)
        if len(thresholds) != 7:
            return False, 0

        valid_thresholds = thresholds.pop(0)
        # Check whether particular threshold is readable
        if valid_thresholds & (1 << bit_mask):
            return self._get_converted_sensor_reading(thresholds[bit_mask])
        else:
            return False, 0

class IpmiFru(object):

    def __init__(self, fru_id):
        self.id = fru_id

    def _get_ipmitool_fru_print(self):
        result = ""
        command = "ipmitool fru print {}".format(self.id)
        try:
            proc = subprocess.Popen(command.split(), stdout=subprocess.PIPE,
                                    universal_newlines=True, stderr=subprocess.STDOUT)
            stdout = proc.communicate()[0]
            proc.wait()
            if not proc.returncode:
                result = stdout.rstrip('\n')
        except EnvironmentError:
            pass

        return result

    def _get_from_fru(self, info):
        """
        Returns a string containing the info from FRU
        """
        fru_output = self._get_ipmitool_fru_print()
        if not fru_output:
            return "NA"

        info_req = re.search(r"%s\s*:(.*)" % info, fru_output)
        if not info_req:
            return "NA"

        return info_req.group(1).strip()

    def get_board_serial(self):
        """
        Returns a string containing the Serial Number of the device.
        """
        return self._get_from_fru('Board Serial')

    def get_board_part_number(self):
        """
        Returns a string containing the Part Number of the device.
        """
        return self._get_from_fru('Board Part Number')

    def get_board_mfr_id(self):
        """
        Returns a string containing the manufacturer id of the FRU.
        """
        return self._get_from_fru('Board Mfg')

    def get_board_product(self):
        """
        Returns a string containing the manufacturer id of the FRU.
        """
        return self._get_from_fru('Board Product')

    def get_fru_data(self, offset, count=1):
        """
        Reads and returns the FRU data at the provided offset.

        Args:
            offset (int) - FRU offset to read
            count (int) - Number of bytes to read [optional, default = 1]
        Returns:
            A tuple (bool, list(int)) where the first element provides
            the validity of the data read and the second element is a
            list, the elements of which are the individual bytes of the
            FRU data read.
        """
        result_bytes = list()
        is_valid = True
        result = ""

        offset_LSB = offset & 0xFF
        offset_MSB = offset & 0xFF00
        command = "ipmitool raw {} {} {} {} {} {}".format(NetFn_Storage,
                                                          Cmd_ReadFRUData,
                                                          self.id, offset_LSB,
                                                          offset_MSB, count)
        try:
            proc = subprocess.Popen(command.split(), stdout=subprocess.PIPE,
                                    universal_newlines=True, stderr=subprocess.STDOUT)
            stdout = proc.communicate()[0]
            proc.wait()
            if not proc.returncode:
                result = stdout.rstrip('\n')
        except EnvironmentError:
            is_valid = False

        if (not result) or (not is_valid):
            return False, result_bytes

        for i in result.split():
            result_bytes.append(int(i, 16))

        read_count = result_bytes.pop(0)
        if read_count != count:
            return False, result_bytes
        else:
            return True, result_bytes
