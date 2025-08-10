#
# psuutil.py
# Platform-specific PSU status interface for SONiC
#


import os.path

try:
    from sonic_psu.psu_base import PsuBase
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

attr_path = '/sys/class/hwmon/hwmon2/device/'

class PsuUtil(PsuBase):
    """Platform-specific PSUutil class"""

    SYSFS_PSU_DIR = ["/sys/bus/i2c/devices/i2c-10/10-0050",
                     "/sys/bus/i2c/devices/i2c-9/9-0050"]

    def __init__(self):
        PsuBase.__init__(self)
    
             
    # Get sysfs attribute
    def get_attr_value(self, path):
        
        retval = 'ERR'        
        if (not os.path.isfile(path)):
            return retval

        try:
            with open(path, 'r') as fd:
                retval = fd.read()
        except Exception as error:
            logging.error("Unable to open ", path, " file !")

        retval = retval.rstrip('\r\n')
        return retval

    def get_num_psus(self):
        """
        Retrieves the number of PSUs available on the device
        :return: An integer, the number of PSUs available on the device
         """
        MAX_PSUS = 2
        return MAX_PSUS

    def get_psu_status(self, index):
        """
        Retrieves the oprational status of power supply unit (PSU) defined
                by index <index>
        :param index: An integer, index of the PSU of which to query status
        :return: Boolean, True if PSU is operating properly, False if PSU is\
        faulty
        """
        status = 0
        attr_file = 'psu_status'        
        status_path = attr_path+'/ESC601_PSU/' + attr_file
        try:
            reg_file = open(status_path, 'r')
        except IOError as e:
            print( "Error: unable to open file: %s" % str(e))
            return False
        text_lines = reg_file.read()
        
        search_str = "PSU {} is power Good".format(index)
        
        if search_str in text_lines:
            status = 1

        reg_file.close() 

        return status

    def get_psu_presence(self, index):
        """
        Retrieves the presence status of power supply unit (PSU) defined
                by index <index>
        :param index: An integer, index of the PSU of which to query status
        :return: Boolean, True if PSU is plugged, False if not
        """
        status = 0
        attr_file ='psu_present'
        presence_path = attr_path+'ESC601_PSU/' + attr_file
        try:
            reg_file = open(presence_path, 'r')
        except IOError as e:
            print( "Error: unable to open file: %s" % str(e))
            return False
        text_lines = reg_file.read()
        
        search_str = "PSU {} is present".format(index)
        
        if search_str in text_lines:
            status = 1

        reg_file.close()    
    
        return status

