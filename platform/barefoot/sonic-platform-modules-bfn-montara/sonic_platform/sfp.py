#!/usr/bin/env python

try:
    import os
    from sonic_platform_base.sonic_xcvr.sfp_optoe_base import SfpOptoeBase
    from sonic_platform.platform_thrift_client import thrift_try
    from sonic_platform.platform_thrift_client import pltfm_mgr_try
except ImportError as e:
    raise ImportError (str(e) + "- required module not found")

SFP_TYPE = "SFP"
QSFP_TYPE = "QSFP"
QSFP_DD_TYPE = "QSFP_DD"
EEPROM_PAGE_SIZE = 128


class Sfp(SfpOptoeBase):
    """
    BFN Platform-specific SFP class
    """

    def __init__(self, port_num):
        SfpOptoeBase.__init__(self)
        self.index = port_num
        self.port_num = port_num
        self.sfp_type = QSFP_TYPE
        self.SFP_EEPROM_PATH = "/var/run/platform/sfp/"
        self.eeprom_path = None
        self.__cached_api_supported = None

    @property
    def _cached_api_supported(self):
        def cached_num_bytes_get(client):
            return client.pltfm_mgr.pltfm_mgr_qsfp_cached_num_bytes_get(1, 0, 0, 0)
        if self.__cached_api_supported is None:
            try:
                thrift_try(cached_num_bytes_get, 1)
                self.__cached_api_supported = True
            except Exception as e:
                self.__cached_api_supported = False
                if not os.path.exists(self.SFP_EEPROM_PATH):
                    try:
                        os.makedirs(self.SFP_EEPROM_PATH)
                    except OSError as e:
                        if e.errno != errno.EEXIST:
                            raise
                self.eeprom_path = self.SFP_EEPROM_PATH + "sfp{}-eeprom-cache".format(self.index)
        return self.__cached_api_supported

    def get_presence(self):
        """
        Retrieves the presence of the sfp
        """
        presence = False

        def qsfp_presence_get(client):
            return client.pltfm_mgr.pltfm_mgr_qsfp_presence_get(self.index)

        try:
            presence = thrift_try(qsfp_presence_get)
        except Exception as e:
            print(e.__doc__)
            print(e.message)

        return presence

    def get_lpmode(self):
        """
        Retrieves the lpmode (low power mode) status of this SFP
        """
        def qsfp_lpmode_get(client):
            return client.pltfm_mgr.pltfm_mgr_qsfp_lpmode_get(self.index)

        return thrift_try(qsfp_lpmode_get)

    def set_lpmode(self, lpmode):
        """
        Sets the lpmode (low power mode) of SFP
        """
        def qsfp_lpmode_set(client):
            return client.pltfm_mgr.pltfm_mgr_qsfp_lpmode_set(self.index, lpmode)

        status = thrift_try(qsfp_lpmode_set)
        return (status == 0)

    def get_eeprom_path(self):
        def qsfp_info_get(client):
            return client.pltfm_mgr.pltfm_mgr_qsfp_info_get(self.index)

        eeprom_hex = thrift_try(qsfp_info_get)
        eeprom_raw = bytearray.fromhex(eeprom_hex)
        with open(self.eeprom_path, 'wb') as fp:
            fp.write(eeprom_raw)
        return self.eeprom_path

    def read_eeprom(self, offset, num_bytes):
        if not self.get_presence():
            return None

        if not self._cached_api_supported:
            return super().read_eeprom(offset, num_bytes)

        def cached_num_bytes_get(page, offset, num_bytes):
            def qsfp_cached_num_bytes_get(client):
                return client.pltfm_mgr.pltfm_mgr_qsfp_cached_num_bytes_get(self.index, page, offset, num_bytes)
            return bytearray.fromhex(thrift_try(qsfp_cached_num_bytes_get))

        page_offset = offset % EEPROM_PAGE_SIZE
        if page_offset + num_bytes > EEPROM_PAGE_SIZE:
            curr_page_num_bytes_left = EEPROM_PAGE_SIZE - page_offset
            curr_page_bytes = cached_num_bytes_get(offset // EEPROM_PAGE_SIZE, page_offset, curr_page_num_bytes_left)
            return curr_page_bytes + self.read_eeprom(offset + curr_page_num_bytes_left, num_bytes - curr_page_num_bytes_left)

        return cached_num_bytes_get(offset // EEPROM_PAGE_SIZE, page_offset, num_bytes)

    def write_eeprom(self, offset, num_bytes, write_buffer):
        # Not supported at the moment
        return False

    def get_name(self):
        """
        Retrieves the name of the device
            Returns:
            string: The name of the device
        """
        return "sfp{}".format(self.index)

    def get_reset_status(self):
        """
        Retrieves the reset status of SFP
        """
        def get_qsfp_reset(pltfm_mgr):
            return pltfm_mgr.pltfm_mgr_qsfp_reset_get(self.index)
        _, status = pltfm_mgr_try(get_qsfp_reset, False)
        return status

    def reset(self):
        """
        Reset SFP and return all user module settings to their default srate.
        """
        def qsfp_reset(client):
            client.pltfm_mgr.pltfm_mgr_qsfp_reset(self.index, True)
            return client.pltfm_mgr.pltfm_mgr_qsfp_reset(self.index, False)

        err = thrift_try(qsfp_reset)
        return not err

    def get_status(self):
        """
        Retrieves the operational status of the device
        """
        reset = self.get_reset_status()

        if reset:
            status = False
        else:
            status = True

        return status

    def get_position_in_parent(self):
        """
        Retrieves 1-based relative physical position in parent device.
        Returns:
            integer: The 1-based relative physical position in parent
            device or -1 if cannot determine the position
        """
        return self.index

    def is_replaceable(self):
        """
        Indicate whether this device is replaceable.
        Returns:
            bool: True if it is replaceable.
        """
        return True

    def get_error_description(self):
        """
        Retrives the error descriptions of the SFP module
        Returns:
            String that represents the current error descriptions of vendor specific errors
            In case there are multiple errors, they should be joined by '|',
            like: "Bad EEPROM|Unsupported cable"
        """
        if not self.get_presence():
            return self.SFP_STATUS_UNPLUGGED
        return self.SFP_STATUS_OK

    def tx_disable(self, tx_disable):
        """
        Disable SFP TX for all channels
        Args:
            tx_disable : A Boolean, True to enable tx_disable mode, False to disable
                         tx_disable mode.
        Returns:
            A boolean, True if tx_disable is set successfully, False if not
        """
        if self.sfp_type == QSFP_TYPE:
            return self.tx_disable_channel(0xF, tx_disable)
        return False

    def tx_disable_channel(self, channel, disable):
        """
        Sets the tx_disable for specified SFP channels

        Args:
            channel : A hex of 4 bits (bit 0 to bit 3) which represent channel 0 to 3,
                      e.g. 0x5 for channel 0 and channel 2.
            disable : A boolean, True to disable TX channels specified in channel,
                      False to enable

        Returns:
            A boolean, True if successful, False if not
        """
        def qsfp_tx_disable_channel(client):
            return client.pltfm_mgr.pltfm_mgr_qsfp_tx_disable(self.index, channel, disable)

        if self.sfp_type == QSFP_TYPE:
            status = thrift_try(qsfp_tx_disable_channel)
            return (status == 0)
        return False

    def get_power_override(self):
        def get_qsfp_power_override(pltfm_mgr):
            return pltfm_mgr.pltfm_mgr_qsfp_pwr_override_get(self.index)
        _, pwr_override = pltfm_mgr_try(get_qsfp_power_override)
        return pwr_override

    def set_power_override(self, power_override, power_set):
        def set_qsfp_power_override(pltfm_mgr):
            return pltfm_mgr.pltfm_mgr_qsfp_pwr_override_set(
                self.index, power_override, power_set
            )
        _, status = pltfm_mgr_try(set_qsfp_power_override)
        return status
