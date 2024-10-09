#!/usr/bin/env python

try:
    import time
    from sonic_platform_pddf_base.pddf_sfp import PddfSfp
    import re
except ImportError as e:
    raise ImportError (str(e) + "- required module not found")


class Sfp(PddfSfp):
    """
    PDDF Platform-Specific Sfp class
    """

    def __init__(self, index, pddf_data=None, pddf_plugin_data=None):
        PddfSfp.__init__(self, index, pddf_data, pddf_plugin_data)

    # Provide the functions/variables below for which implementation is to be overwritten
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

            if status == 0:
                intr_status = True
            else:
                intr_status = False

        return intr_status

    def get_reset_status(self):
        """
        Retrieves the reset status of SFP
        Returns:
            A Boolean, True if reset enabled, False if disabled
        """
        reset_status = None
        if not self.get_presence():
            return reset_status

        device = 'PORT{}'.format(self.port_index)
        output = self.pddf_obj.get_attr_name_output(device, 'xcvr_reset')
        if not output:
            return False

        status = int(output['status'].rstrip())

        if status == 0:
            reset_status = True
        else:
            reset_status = False

        return reset_status

    def reset(self):
        """
        Reset SFP and return all user module settings to their default srate.
        Returns:
            A boolean, True if successful, False if not
        """
        status = False
        if not self.get_presence():
            return status

        device = 'PORT{}'.format(self.port_index)
        # TODO: Implement a wrapper set function to write the sequence
        path = self.pddf_obj.get_path(device, 'xcvr_reset')

        # TODO: put the optic based reset logic using EEPROM
        if path is None:
            pass
        else:
            try:
                f = open(path, 'r+')
            except IOError as e:
                return False

            try:
                f.seek(0)
                f.write('0')
                f.flush()
                time.sleep(1)
                f.seek(0)
                f.write('1')

                f.close()
                status = True
            except IOError as e:
                status = False

        return status

    def get_lpmode(self):
        """
        Retrieves the lpmode (low power mode) status of this SFP
        Returns:
            A Boolean, True if lpmode is enabled, False if disabled
        """
        lpmode = False

        if not self.get_presence():
            return lpmode

        device = 'PORT{}'.format(self.port_index)
        path_val = self.pddf_obj.get_path(device, 'xcvr_lpmode')

        if path_val is None:
            lpmode = super().get_lpmode()
        else:
            path_dir = re.sub(r'/value$', '/direction', path_val)
            try:
                with open(path_dir, 'r') as f_dir:
                    dir = f_dir.read()
            except IOError:
                lpmode = super().get_lpmode()
                return lpmode

            if not dir:
                lpmode = super().get_lpmode()
            else:
                if dir.rstrip() == 'in':
                    lpmode = True
                else:
                    lpmode = False

        return lpmode

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
        path_val = self.pddf_obj.get_path(device, 'xcvr_lpmode')

        # TODO: put the optic based reset logic using EEPROM
        if path_val is None:
            status = super().set_lpmode(lpmode)
        else:
            if not self.get_presence():
                return status
            path_dir = re.sub(r'/value$', '/direction', path_val)
            try:
                f_val = open(path_val, 'r+')
                f_dir = open(path_dir, 'r+')
            except IOError as e:
                return False

            try:
                if lpmode:
                    f_dir.write('in')
                else:
                    f_val.write('1')
                    f_dir.write('out')

                f_dir.close()
                f_val.close()
                status = True
            except IOError as e:
                status = False

        return status
