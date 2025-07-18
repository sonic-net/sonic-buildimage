#############################################################################
# PDDF
#
# PDDF sfp base class inherited from the base class
#############################################################################

try:
    from sonic_platform_base.sonic_xcvr.sfp_optoe_base import SfpOptoeBase
    import time
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

QSFP_PWR_CTRL_ADDR = 93


class PddfSfp(SfpOptoeBase):
    """
    PDDF generic Sfp class
    """

    pddf_obj = {}
    plugin_data = {}
    _port_start = 0
    _port_end = 0


    def __init__(self, index, pddf_data=None, pddf_plugin_data=None):
        if not pddf_data or not pddf_plugin_data:
            raise ValueError('PDDF JSON data error')

        self.pddf_obj = pddf_data
        self.plugin_data = pddf_plugin_data

        self.platform = self.pddf_obj.get_platform()

        # index is 0-based
        self._port_start = 0
        self._port_end = int(self.platform['num_ports'])
        if index < self._port_start or index >= self._port_end:
            print("Invalid port index %d" % index)
            return

	# 1-based port index
        self.port_index = index+1
        self.device = 'PORT{}'.format(self.port_index)
        self.sfp_type = self.pddf_obj.get_device_type(self.device)
        self.eeprom_path = self.pddf_obj.get_path(self.device, 'eeprom')

        SfpOptoeBase.__init__(self)

    def get_eeprom_path(self):
        return self.eeprom_path

    def get_name(self):
        """
        Retrieves the name of the device
            Returns:
            string: The name of the device
        """
        # Name of the port/sfp ?
        return 'PORT{}'.format(self.port_index)

    def get_presence(self):
        """
        Retrieves the presence of the PSU
        Returns:
            bool: True if PSU is present, False if not
        """
        output = self.pddf_obj.get_attr_name_output(self.device, 'xcvr_present')
        if not output:
            return False

        mode = output['mode']
        modpres = output['status'].rstrip()
        if 'XCVR' in self.plugin_data:
            if 'xcvr_present' in self.plugin_data['XCVR']:
                ptype = self.sfp_type
                vtype = 'valmap-'+ptype
                if vtype in self.plugin_data['XCVR']['xcvr_present'][mode]:
                    vmap = self.plugin_data['XCVR']['xcvr_present'][mode][vtype]
                    if modpres in vmap:
                        return vmap[modpres]
                    else:
                        return False
        # if self.plugin_data doesn't specify anything regarding Transceivers
        if modpres == '1':
            return True
        else:
            return False

    def get_reset_status(self):
        """
        Retrieves the reset status of SFP
        Returns:
            A Boolean, True if reset enabled, False if disabled
        """
        reset_status = None
        device = 'PORT{}'.format(self.port_index)
        output = self.pddf_obj.get_attr_name_output(device, 'xcvr_reset')

        if output:
            status = int(output['status'].rstrip())

            if status == 1:
                reset_status = True
            else:
                reset_status = False

        return reset_status

    def get_rx_los(self):
        """
        Retrieves the RX LOS (lost-of-signal) status of SFP
        Returns:
            A Boolean, True if SFP has RX LOS, False if not.
            Note : RX LOS status is latched until a call to get_rx_los or a reset.
        """
        rx_los = None
        device = 'PORT{}'.format(self.port_index)
        output = self.pddf_obj.get_attr_name_output(device, 'xcvr_rxlos')

        if output:
            status = int(output['status'].rstrip())

            if status == 1:
                rx_los = True
            else:
                rx_los = False
        else:
            # Use common SfpOptoeBase implementation for get_rx_los
            rx_los = super().get_rx_los()

        return rx_los

    def get_tx_fault(self):
        """
        Retrieves the TX fault status of SFP
        Returns:
            A Boolean, True if SFP has TX fault, False if not
            Note : TX fault status is lached until a call to get_tx_fault or a reset.
        """
        tx_fault = None
        device = 'PORT{}'.format(self.port_index)
        output = self.pddf_obj.get_attr_name_output(device, 'xcvr_txfault')

        if output:
            status = int(output['status'].rstrip())

            if status == 1:
                tx_fault = True
            else:
                tx_fault = False
        else:
            # Use common SfpOptoeBase implementation for get_tx_fault
            tx_fault = super().get_tx_fault()

        return tx_fault

    def get_tx_disable(self):
        """
        Retrieves the tx_disable status of this SFP
        Returns:
            A Boolean, True if tx_disable is enabled, False if disabled
        """
        tx_disable = False
        device = 'PORT{}'.format(self.port_index)
        output = self.pddf_obj.get_attr_name_output(device, 'xcvr_txdisable')

        if output:
            status = int(output['status'].rstrip())

            if status == 1:
                tx_disable = True
            else:
                tx_disable = False
        else:
            # Use common SfpOptoeBase implementation for get_tx_disable
            tx_disable = super().get_tx_disable()

        return tx_disable

    def get_lpmode(self):
        """
        Retrieves the lpmode (low power mode) status of this SFP
        Returns:
            A Boolean, True if lpmode is enabled, False if disabled
        """
        lpmode = False
        device = 'PORT{}'.format(self.port_index)
        output = self.pddf_obj.get_attr_name_output(device, 'xcvr_lpmode')

        if output:
            status = int(output['status'].rstrip())

            if status == 1:
                lpmode = True
            else:
                lpmode = False
        else:
            xcvr_id = self._xcvr_api_factory._get_id()
            if xcvr_id is not None:
                if xcvr_id == 0x18 or xcvr_id == 0x19 or xcvr_id == 0x1e:
                    # QSFP-DD or OSFP
                    # Use common SfpOptoeBase implementation for get_lpmode
                    lpmode = super().get_lpmode()
                elif xcvr_id == 0x11 or xcvr_id == 0x0d or xcvr_id == 0x0c:
                    # QSFP28, QSFP+, QSFP
                    # get_power_set() is not defined in the optoe_base class
                    api = self.get_xcvr_api()
                    power_set = api.get_power_set()
                    power_override = self.get_power_override()
                    # By default the lpmode pin is pulled high as mentioned in the sff community
                    return power_set if power_override else True

        return lpmode

    def get_intr_status(self):
        """
        Retrieves the interrupt status for this transceiver
        Returns:
            A Boolean, True if there is interrupt, False if not
        """
        intr_status = False

        # Interrupt status can be checked for absent ports too
        device = 'PORT{}'.format(self.port_index)
        output = self.pddf_obj.get_attr_name_output(device, 'xcvr_intr_status')

        if output:
            status = int(output['status'].rstrip())

            if status == 1:
                intr_status = True
            else:
                intr_status = False

        return intr_status

    def reset(self):
        """
        Reset SFP and return all user module settings to their default srate.
        Returns:
            A boolean, True if successful, False if not
        """
        status = False
        device = 'PORT{}'.format(self.port_index)
        path = self.pddf_obj.get_path(device, 'xcvr_reset')
        if path:
            try:
                with open(path, 'r+') as f:
                    f.seek(0)
                    f.write('1')
                    time.sleep(1)
                    f.seek(0)
                    f.write('0')
                    status = True
            except (IOError, OSError):
                status = False
        else:
            # Use common SfpOptoeBase implementation for reset
            status = super().reset()

        return status

    def tx_disable(self, tx_disable):
        """
        Disable SFP TX for all channels
        Args:
            tx_disable : A Boolean, True to enable tx_disable mode, False to disable
                         tx_disable mode.
        Returns:
            A boolean, True if tx_disable is set successfully, False if not
        """
        # find out a generic implementation of tx_disable for SFP, QSFP and OSFP
        status = False
        device = 'PORT{}'.format(self.port_index)
        path = self.pddf_obj.get_path(device, 'xcvr_txdisable')

        # TODO: put the optic based reset logic using EEPROM
        if path:
            try:
                f = open(path, 'r+')
            except IOError as e:
                return False

            try:
                if tx_disable:
                    f.write('1')
                else:
                    f.write('0')
                f.close()
                status = True
            except IOError as e:
                status = False
        else:
            # Use common SfpOptoeBase implementation for tx_disable
            status = super().tx_disable(tx_disable)


        return status

    def set_lpmode(self, lpmode):
        """
        Sets the lpmode (low power mode) of SFP
        Args:
            lpmode: A Boolean, True to enable lpmode, False to disable it
            Note  : lpmode can be overridden by set_power_override
        Returns:
            A boolean, True if lpmode is set successfully, False if not
        """
        status = False
        device = 'PORT{}'.format(self.port_index)
        path = self.pddf_obj.get_path(device, 'xcvr_lpmode')

        if path:
            try:
                f = open(path, 'r+')
            except IOError as e:
                return False

            try:
                if lpmode:
                    f.write('1')
                else:
                    f.write('0')

                f.close()
                status = True
            except IOError as e:
                status = False
        else:
            xcvr_id = self._xcvr_api_factory._get_id()
            if xcvr_id is not None:
                if xcvr_id == 0x18 or xcvr_id == 0x19 or xcvr_id == 0x1e:
                    # QSFP-DD or OSFP
                    # Use common SfpOptoeBase implementation for set_lpmode
                    status = super().set_lpmode(lpmode)
                elif xcvr_id == 0x11 or xcvr_id == 0x0d or xcvr_id == 0x0c:
                    # QSFP28, QSFP+, QSFP
                    if lpmode is True:
                        status = self.set_power_override(True, True)
                    else:
                        status = self.set_power_override(True, False)

        return status

    def get_position_in_parent(self):
        """
        Retrieves 1-based relative physical position in parent device.
        Returns:
            integer: The 1-based relative physical position in parent
            device or -1 if cannot determine the position
        """
        return self.port_index

    def is_replaceable(self):
        """
        Indicate whether the SFP is replaceable.
        Returns:
            bool: True if it is replaceable.
        """
        return True

    def dump_sysfs(self):
        return self.pddf_obj.cli_dump_dsysfs('xcvr')
