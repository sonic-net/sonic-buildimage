############################################################################
# Copyright 2024 Dell, Inc.
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
###########################################################################

try:
    import math
    from sonic_platform_base.sonic_sfp.sffbase import sffbase
except ImportError as err:
    raise ImportError(str(err) + "- required module not found")

class inf8628PfmDom(sffbase):

    version = '1.0'
    def calc_ber(self, eeprom_data, offset, size):
        """
        Calculate the BER (Bit Error Rate) based on the given EEPROM data.

        Parameters:
            eeprom_data (list): A list of hexadecimal values representing the EEPROM data.
            offset (int): The starting index of the data in the EEPROM.
            size (int): The size of the data to be processed.

        Returns:
            str: The calculated BER as a string in the format "{0}E{1:+}".
        """
        ret = "0"
        msb = int(eeprom_data[offset + 0], 16)
        lsb = int(eeprom_data[offset + 1], 16)
        exp = (msb >> 3)
        msa = ((msb & 0x7) << 8) | lsb
        if msa == 0:
            ret = "0"
        elif msa >= 1000:
            ret = "{0}E{1:+}".format(msa / 1000.0, exp - 21)
        elif msa >= 100:
            ret = "{0}E{1:+}".format(msa / 100.0, exp - 22)
        elif msa >= 10:
            ret = "{0}E{1:+}".format(msa / 10.0, exp - 23)
        else:
            ret = "{0}E{1:+}".format(msa, exp - 24)
        return ret

    def calc_s16(self, eeprom_data, offset, size):
        """
        Calculate the signed 16-bit integer value from the given EEPROM data.

        Parameters:
            eeprom_data (list): The EEPROM data as a list of hexadecimal values.
            offset (int): The starting index of the EEPROM data to calculate the value.
            size (int): The number of bytes to use for the calculation.

        Returns:
            int: The signed 16-bit integer value calculated from the EEPROM data.
        """
        result_bytes = []
        for i in range(0, size):
            result_bytes.append(int(eeprom_data[offset + i], 16))
        return int.from_bytes(bytes(result_bytes), "big", signed=True)

    def calc_u16(self, eeprom_data, offset, size):
        """
        Calculate the unsigned 16-bit integer value from the given `eeprom_data` starting from the
        specified `offset` and with the specified `size`.

        Parameters:
            eeprom_data (list): The list of hexadecimal values representing the EEPROM data.
            offset (int): The starting index of the EEPROM data from which to calculate the unsigned
                          16-bit integer value.
            size (int): The number of elements to consider for the calculation of the unsigned
                        16-bit integer value.

        Returns:
            int: The calculated unsigned 16-bit integer value.
        """
        result_bytes = []
        for i in range(0, size):
            result_bytes.append(int(eeprom_data[offset + i], 16))
        return int.from_bytes(bytes(result_bytes), "big", signed=False)

    def calc_chromatic_dispersion_short(self, eeprom_data, offset, size):
        """
        Calculate the chromatic dispersion based on the given EEPROM data, offset, and size.

        Args:
            eeprom_data (list): A list of hexadecimal values representing the EEPROM data.
            offset (int): The starting index of the data in the EEPROM.
            size (int): The size of the data to be processed.

        Returns:
            str: The calculated chromatic dispersion in the format "ps/nm".
        """
        result = str(self.calc_s16(eeprom_data, offset, size))
        result += "ps/nm"
        return result

    def calc_chromatic_dispersion_long(self, eeprom_data, offset, size):
        """
        Calculate the chromatic dispersion based on the given EEPROM data, offset, and size.

        Args:
            eeprom_data (list): A list of hexadecimal values representing the EEPROM data.
            offset (int): The starting index of the data in the EEPROM.
            size (int): The size of the data to be processed.

        Returns:
            str: The calculated chromatic dispersion in the format "ps/nm".
        """
        result = str((self.calc_s16(eeprom_data, offset, size) * 20))
        result += "ps/nm"
        return result

    def calc_differential_group_delay(self, eeprom_data, offset, size):
        """
        Calculate the differential group delay.

        Args:
            eeprom_data (list): A list of hexadecimal values representing the EEPROM data.
            offset (int): The starting index of the data in the EEPROM.
            size (int): The size of the data to be processed.

        Returns:
            str: The calculated differential group delay in ps.
        """
        result = ""
        val = (self.calc_u16(eeprom_data, offset, size)) * 0.01
        result = "{0:.2f}ps".format(val)
        return result

    def calc_polarization_dependent_loss(self, eeprom_data, offset, size):
        """
        Calculate the polarization-dependent loss based on the given EEPROM data, offset, and size.

        Parameters:
            eeprom_data (list): A list of hexadecimal values representing the EEPROM data.
            offset (int): The starting index of the data in the EEPROM.
            size (int): The size of the data to be processed.

        Returns:
            str: The calculated polarization-dependent loss in dB.
        """
        result = ""
        result = str(self.calc_u16(eeprom_data, offset, size))
        result += "dB"
        return result

    def calc_carrier_frequency(self, eeprom_data, offset, size):
        """
        Calculate the carrier frequency based on the given EEPROM data, offset, and size.

        Parameters:
            eeprom_data (list): A list of hexadecimal values representing the EEPROM data.
            offset (int): The starting index of the data in the EEPROM.
            size (int): The size of the data to be processed.

        Returns:
            str: The calculated carrier frequency in MHz.
        """
        result = str(self.calc_s16(eeprom_data, offset, size))
        result += "MHz"
        return result

    def calc_error_vector_magnitude(self, eeprom_data, offset, size):
        """
        Calculate the magnitude of the error vector.

        Args:
            eeprom_data (list): A list of hexadecimal values representing the EEPROM data.
            offset (int): The starting index of the data in the EEPROM.
            size (int): The size of the data to be processed.

        Returns:
            str: The magnitude of the error vector as a string.
        """
        result = ""
        val = (self.calc_u16(eeprom_data, offset, size)/65535)*100
        result = "{0:.2f}%".format(val)
        return result

    def calc_sop_rate_of_change(self, eeprom_data, offset, size):
        """
        Calculate the rate of change of the SOP value.

        Args:
            eeprom_data (list): A list of hexadecimal values representing the EEPROM data.
            offset (int): The starting index of the data in the EEPROM.
            size (int): The size of the data to be processed.

        Returns:
            str: The calculated rate of change of the SOP value.
        """
        result = ""
        result = str(self.calc_u16(eeprom_data, offset, size))
        result += "krad/s"
        return result

    def calc_snr(self, eeprom_data, offset, size):
        """
        Calculate the signal-to-noise ratio (SNR) based on the given EEPROM data, offset, and size.

        Parameters:
            eeprom_data (list): A list of hexadecimal values representing the EEPROM data.
            offset (int): The starting index of the data in the EEPROM.
            size (int): The size of the data to be processed.

        Returns:
            str: The calculated SNR value in dB.
        """
        result = ""
        val = self.calc_u16(eeprom_data, offset, size)/10
        result = "{0:.2f}dB".format(val)
        return result

    def calc_laser_temperature(self, eeprom_data, offset, size):
        """
        Calculate the laser temperature based on the given EEPROM data, offset, and size.

        Parameters:
            eeprom_data (list): A list of hexadecimal values representing the EEPROM data.
            offset (int): The starting index of the data in the EEPROM.
            size (int): The size of the data to be processed.

        Returns:
            str: The calculated laser temperature in Celsius with two decimal places.
        """
        val = self.calc_s16(eeprom_data, offset, size)/256
        result = "{0:.2f}C".format(val)
        return result

    def calc_tx_power(self, eeprom_data, offset, size):
        """
        Calculate the transmit power based on the given EEPROM data, offset, and size.

        Parameters:
            eeprom_data (list): A list of hexadecimal values representing the EEPROM data.
            offset (int): The starting index of the data in the EEPROM.
            size (int): The size of the data to be processed.

        Returns:
            str: The calculated transmit power in dBm.
        """
        val = self.calc_s16(eeprom_data, offset, size)
        if val <= 0:
            result = -200
        else:
            val = 10 * math.log(val/ 10000.0, 10)
        val = val/100
        result = "{0:.2f}dBm".format(val)
        return result

    def calc_rx_power(self, eeprom_data, offset, size):
        """
        Calculate the receive power based on the given EEPROM data, offset, and size.

        Parameters:
            eeprom_data (list): A list of hexadecimal values representing the EEPROM data.
            offset (int): The starting index of the data in the EEPROM.
            size (int): The size of the data to be processed.

        Returns:
            str: The calculated receive power in dBm.
        """
        val = self.calc_s16(eeprom_data, offset, size)
        if val <= 0:
            val = -200
        else:
            val = 10 * math.log(val/ 10000.0, 10)
        val = val/100
        result = "{0:.2f}dBm".format(val)
        return result

    def calc_rx_total_power(self, eeprom_data, offset, size):
        """
        Calculate the total power received based on the given EEPROM data, offset, and size.

        Args:
            eeprom_data (list): A list of hexadecimal values representing the EEPROM data.
            offset (int): The starting index of the data in the EEPROM.
            size (int): The size of the data to be processed.

        Returns:
            str: The total power received in dBm.
        """
        val = (self.calc_s16(eeprom_data, offset, size) * 0.01)/100
        result = "{0:.2f}dBm".format(val)
        return result

    def calc_modulation_bias(self, eeprom_data, offset, size):
        """
        Calculate the modulation bias of the given `eeprom_data` using the specified
        `offset` and `size`.

        Args:
            eeprom_data (list): A list of hexadecimal values representing the EEPROM data.
            offset (int): The starting index of the data in the EEPROM.
            size (int): The size of the data to be processed.

        Returns:
            str: The calculated modulation bias as a string with a percentage symbol.
        """
        val = (self.calc_u16(eeprom_data, offset, size)/65535)*100
        result = "{0:.2f}%".format(val)
        return result

    dom_vdm_parameter_units = {
        'preFECBER': "",
        'postFECerrframeratio': "",
        'chromaticDispersionShortlink': "ps/nm",
        'differentialGroupDelay': "ps",
        'polarizationDependentLoss': "dB",
        'carrierFrequencyOffset': "MHz",
        'errorVectorMagnitude': "%",
        'rxOsnrEstimate': "dB",
        'rxEsnrEstimate': "dB",
        'sopRateofChange': "krad/s",
        'chromaticDispersionLonglink': "ps/nm",
        'laserTemperature': "C",
        'txPower': "dBm",
        'rxChannelPower': "dBm",
        'rxTotalPower': "dBm",
        'modulationBiasXI': "%",
        'modulationBiasXQ': "%",
        'modulationBiasXphase': "%",
        'modulationBiasYI': "%",
        'modulationBiasYQ': "%",
        'modulationBiasYphase': "%",
        'laserGrid': "GHz",
        'oifChannelNumber': "",
        'laserFrequency': "THz",
        'laserTunable': ""
    }

    # Versatile diagnostics monitoring (VDM) parameters
    # offsets starting from page 0x24
    dom_vdm_parameters = {
        'preFECBER':
        {'offset':6,
         'size': 2,
         'type': 'func',
         'decode': {'func':calc_ber}},
        'postFECerrframeratio':
        {'offset':14,
         'size': 2,
         'type': 'func',
         'decode': {'func':calc_ber}},
        'chromaticDispersionShortlink':
        {'offset':128,
         'size': 2,
         'type': 'func',
         'decode': {'func':calc_chromatic_dispersion_short}},
        'differentialGroupDelay':
        {'offset':130,
         'size': 2,
         'type': 'func',
         'decode': {'func':calc_differential_group_delay}},
        'polarizationDependentLoss':
        {'offset':132,
         'size': 2,
         'type': 'func',
         'decode': {'func':calc_polarization_dependent_loss}},
        'carrierFrequencyOffset':
        {'offset':134,
         'size': 2,
         'type': 'func',
         'decode': {'func': calc_carrier_frequency}},
        'errorVectorMagnitude':
        {'offset':136,
         'size': 2,
         'type': 'func',
         'decode': {'func': calc_error_vector_magnitude}},
        'rxOsnrEstimate':
        {'offset':138,
         'size': 2,
         'type': 'func',
         'decode': {'func': calc_snr}},
        'rxEsnrEstimate':
        {'offset':140,
         'size': 2,
         'type': 'func',
         'decode': {'func': calc_snr}},
        'sopRateofChange':
        {'offset':144,
         'size': 2,
         'type': 'func',
         'decode': {'func': calc_sop_rate_of_change}},
        'chromaticDispersionLonglink':
        {'offset':146,
         'size': 2,
         'type': 'func',
         'decode': {'func':calc_chromatic_dispersion_long}},
        'laserTemperature':
        {'offset':256,
         'size': 2,
         'type': 'func',
         'decode': {'func': calc_laser_temperature}},
        'txPower':
        {'offset': 258,
         'size': 2,
         'type': 'func',
         'decode': {'func': calc_tx_power}},
        'rxChannelPower':
        {'offset': 260,
         'size': 2,
         'type': 'func',
         'decode': {'func':calc_rx_power}},
        'rxTotalPower':
        {'offset': 262,
         'size': 2,
         'type': 'func',
         'decode': {'func': calc_rx_total_power}},
        'modulationBiasXI':
        {'offset': 264,
         'size': 2,
         'type': 'func',
         'decode': {'func': calc_modulation_bias}},
        'modulationBiasXQ':
        {'offset': 266,
         'size': 2,
         'type': 'func',
         'decode': {'func': calc_modulation_bias}},
        'modulationBiasXphase':
        {'offset': 268,
         'size': 2,
         'type': 'func',
         'decode': {'func': calc_modulation_bias}},
        'modulationBiasYI':
        {'offset': 270,
         'size': 2,
         'type': 'func',
         'decode': {'func': calc_modulation_bias}},
        'modulationBiasYQ':
        {'offset': 272,
         'size': 2,
         'type': 'func',
         'decode': {'func': calc_modulation_bias}},
        'modulationBiasYphase':
        {'offset': 274,
         'size': 2,
         'type': 'func',
         'decode': {'func': calc_modulation_bias}}}
    # Versatile diagnostics monitoring (VDM)
    def parse_vdm_values(self, eeprom_raw_data, start_pos):
        return sffbase.parse(self, self.dom_vdm_parameters, eeprom_raw_data, start_pos)
