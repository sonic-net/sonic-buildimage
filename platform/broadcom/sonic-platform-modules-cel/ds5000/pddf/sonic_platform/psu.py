#!/usr/bin/env python

import subprocess

#############################################################################
# Celestica
#
# Component contains an implementation of SONiC Platform Base API and
# provides the psu management function
#
#############################################################################

try:
    from sonic_platform_pddf_base.pddf_psu import PddfPsu
    from .helper import APIHelper
except ImportError as e:
    raise ImportError (str(e) + "- required module not found")

PSU_PRESENCE_SYSFS_PATH = "/sys/devices/platform/sys_cpld/psu{}_presence"
PSU_PWRGOOD_SYSFS_PATH = "/sys/devices/platform/sys_cpld/psu{}_pwrgood"

class Psu(PddfPsu):
    """PDDF Platform-Specific PSU class"""

    def __init__(self, index, pddf_data=None, pddf_plugin_data=None):
        PddfPsu.__init__(self, index, pddf_data, pddf_plugin_data)
        self._api_helper = APIHelper()

        self._presence = None
        self._model = None
        self._mfr_id = None
        self._serial = None
        self._revision = None
        
    # Provide the functions/variables below for which implementation is to be overwritten

    def get_presence(self):
        presence = False

        try:
            with open(PSU_PRESENCE_SYSFS_PATH.format(self.psu_index)) as fd:
                data = fd.read().strip()

            if data == "1":
                presence = True
        except (FileNotFoundError, IOError):
            pass

        self._presence = presence
        return self._presence

    def get_powergood_status(self):
        pwrgood = False

        try:
            with open(PSU_PWRGOOD_SYSFS_PATH.format(self.psu_index)) as fd:
                data = fd.read().strip()

            if data == "1":
                pwrgood = True
        except (FileNotFoundError, IOError):
            pass

        return pwrgood

    def get_type(self):
        return 'AC'

    def get_capacity(self):
        return 2000

    def get_voltage_low_threshold(self):
        return 10

    def get_voltage_high_threshold(self):
        return 14

    def get_temperature_high_threshold(self):
        return 60

    def get_status_led(self):
        if self._presence:
            if self.get_powergood_status():
                return "blue"
            else:
                return "amber"
        else:
            return "N/A"

    def _get_revision(self):
        """
        Retrieves the revision of the device
        Returns:
            string: revision of device
        """
        if not self._api_helper.with_bmc():
            output = self._api_helper.i2c_read(84 + self.psu_index - 1, 0x50, 0x40, 3)
            return bytes.fromhex(output.replace('0x', '').replace(" ", "")).decode("utf-8")

        return 'N/A'

    # In Open BMC FRU ID changes on FRU replacement hence we fetch the
    # FRU fields in groups and cache them for optimization.
    # Cached value is returned when fetched.

    def _get_fru_info(self, field):
        if self._presence == None:
            self.get_presence()

        if self._presence == False:
            if self._model != None:
                self._model = None
                self._mfr_id = None
                self._serial = None
                self._revision = None
                
            return 'N/A'
        else:
            if self._model == None:
                try:
                    ipmi_cmd = ['ipmitool', 'fru', 'list']
                    output = subprocess.check_output(ipmi_cmd, universal_newlines=True)
                    if output:
                        output = output.split('FRU Device Description : ')
                        for fru_data in output:
                            if fru_data.startswith('PSU{}_FRU'.format(self.psu_index)):
                                for field_data in fru_data.split('\n'):
                                    if 'Product Manufacturer' in field_data:
                                        self._mfr_id = field_data.split(':')[1].strip()
                                    elif 'Product Name' in field_data:
                                        self._model = field_data.split(':')[1].strip()
                                    elif 'Product Version' in field_data:
                                        self._revision = field_data.split(':')[1].strip()
                                    elif 'Product Serial' in field_data:
                                        self._serial = field_data.split(':')[1].strip()
                except subprocess.CalledProcessError:
                    pass

        if field == 'mfr_id':
            return self._mfr_id
        elif field == 'model':
            return self._model
        elif field == 'revision':
            return self._revision
        elif field == 'serial':
            return self._serial

        return 'N/A'
    def get_model(self):
        if self._api_helper.with_bmc():
            return self._get_fru_info('model')
        else:
            return super().get_model()

    def get_mfr_id(self):
        if self._api_helper.with_bmc():
            return self._get_fru_info('mfr_id')
        else:
            return super().get_mfr_id()

    def get_serial(self):
        if self._api_helper.with_bmc():
            return self._get_fru_info('serial')
        else:
            return super().get_serial()

    def get_revision(self):
        if self._api_helper.with_bmc():
            return self._get_fru_info('revision')
        else:
            return self._get_revision()

