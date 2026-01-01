#!/usr/bin/env python
#


try:
    from sonic_platform_pddf_base.pddf_psu import PddfPsu
    import subprocess
    import re
except ImportError as e:
    raise ImportError (str(e) + "- required module not found")


class Psu(PddfPsu):
    """PDDF Platform-Specific PSU class"""

    PLATFORM_PSU_CAPACITY = 750

    def __init__(self, index, pddf_data=None, pddf_plugin_data=None):
        PddfPsu.__init__(self, index, pddf_data, pddf_plugin_data)

    # Provide the functions/variables below for which implementation is to be overwritten
    def get_capacity(self):
        """
        Gets the capacity (maximum output power) of the PSU in watts
        Returns:
            An integer, the capacity of PSU
        """
        return (self.PLATFORM_PSU_CAPACITY)

    def get_type(self):
        """
        Gets the type of the PSU
        Returns:
            A string, the type of PSU (AC/DC)
        """
        ptype = "AC"
        # Currently the platform supports only AC type of PSUs
        result = ""
        command = "ipmitool fru print {}".format(self.psu_index)
        try:
            proc = subprocess.Popen(command.split(), stdout=subprocess.PIPE,
                                    stderr=subprocess.STDOUT)
            stdout = proc.communicate()[0]
            proc.wait()
            if not proc.returncode:
                result = stdout.rstrip('\n')
                info='Board Product'
                info_req = re.search(r"%s\s*:(.*)"%info, result)
                if info_req:
                    output=info_req.group(1).strip()
                    if 'AC' in output: ptype='AC'
                    if 'DC' in output: ptype='DC'
        except:
            pass

        return ptype

    def get_revision(self):
        """
        Retrives thehardware revision of the device
        Returns:
            String: revision value of device
        """
        revision = "NA"
        try:
            command = "ipmitool fru print {}".format(self.psu_index)
            status, output = subprocess.getstatusoutput(command)
            if not status:
                board_info = re.search(r"%s\s*:(.*)" % "Board Serial", output)
                result = board_info.group(1).strip()
                if result is not None and len(result) == 23:
                     revision = result[-3:]

        except:
            pass

        return revision
