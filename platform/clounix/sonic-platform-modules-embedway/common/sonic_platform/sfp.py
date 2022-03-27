#!/usr/bin/env python

try:
    import time
    from sonic_platform_pddf_base.pddf_sfp import PddfSfp
except ImportError as e:
    raise ImportError (str(e) + "- required module not found")


class Sfp(PddfSfp):
    """
    PDDF Platform-Specific Sfp class
    """

    def __init__(self, index, pddf_data=None, pddf_plugin_data=None):
        PddfSfp.__init__(self, index, pddf_data, pddf_plugin_data)
        self._index = self.port_index
    
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
                lpmode = False
            else:
                lpmode = True
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
                reset_status = False
            else:
                reset_status = True

        return reset_status
    
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
                    f.write('0')
                else:
                    f.write('1')

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
                f = open(path, 'r+')
            except IOError as e:
                return False

            try:
                f.seek(0)
                f.write('0')
                time.sleep(1)
                f.seek(0)
                f.write('1')

                f.close()
                status = True
            except IOError as e:
                status = False
        else:
            # Use common SfpOptoeBase implementation for reset
            status = super().reset()

        return status
    # Provide the functions/variables below for which implementation is to be overwritten
