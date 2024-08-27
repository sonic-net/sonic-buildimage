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
