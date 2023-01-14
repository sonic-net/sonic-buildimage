try:
    from sonic_platform_pddf_base.pddf_psu import PddfPsu
    from sonic_py_common.general import getstatusoutput_noshell
    import time
except ImportError as e:
    raise ImportError (str(e) + "- required module not found")


class Psu(PddfPsu):
    """PDDF Platform-Specific PSU class"""
    
    PLATFORM_PSU_CAPACITY = 1200

    def __init__(self, index, pddf_data=None, pddf_plugin_data=None):
        PddfPsu.__init__(self, index, pddf_data, pddf_plugin_data)
        
    # Provide the functions/variables below for which implementation is to be overwritten
    def get_maximum_supplied_power(self):
        """
        Retrieves the maximum supplied power by PSU (or PSU capacity)
        Returns:
            A float number, the maximum power output in Watts.
            e.g. 1200.1
        """
        return float(self.PLATFORM_PSU_CAPACITY)

    def get_type(self):
        """
        Gets the type of the PSU
        Returns:
        A string, the type of PSU (AC/DC)
        """
        return "DC"
    
    def runcmd(self, cmd):
        time_retry = 6
        result_msg = ""
        time_delay = 0.01
        while time_retry:
            try:
                val, result_msg = getstatusoutput_noshell(cmd.split(" "))
                if val is False:
                    time_retry -=1
                    time.sleep(time_delay)
                    continue
                else:
                    return val, result_msg
            except Exception as e:
                time_retry -= 1
                result_msg = str(e)
                time.sleep(time_delay)
        
        return False, result_msg
    
    def get_voltage(self):
        """
        Retrieves current PSU voltage output

        Returns:
            A float number, the output voltage in volts,
            e.g. 12.1
        """
        
        v_out = 0
        label_t = "psu_v_out"
        device = "PSU{}".format(self.psu_index)
        #print(device)
        pddf_obj_data = self.pddf_obj.data
        
        if device in pddf_obj_data.keys():
            pmbusloc = pddf_obj_data[device]['i2c']['interface']
            
            for val in pmbusloc:
               dev_name = val['dev']
               pmbus_loc = pddf_obj_data[dev_name]
               i2cloc = pmbus_loc['i2c']['attr_list']
               parentbus = pmbus_loc['i2c']['topo_info']
               
               for item_t in i2cloc:
                    if item_t['attr_name'] == label_t:
                        parentbus_id = int(parentbus['parent_bus'], 16)
                        vout_mode_cmd = "i2cget -f -y {} {} 0x20 bp".format(parentbus_id, parentbus['dev_addr'])
                        ret_t, val_voutmode  = self.runcmd(vout_mode_cmd)
                        if ret_t is False:
                            return 0.0
                        v_out_cmd = "i2cget -f -y {} {} {} wp".format(parentbus_id, parentbus['dev_addr'], item_t['attr_offset'])
                        ret_t, val_p_out = self.runcmd(v_out_cmd)
                        if ret_t is False:
                            return 0.0
                        val_voutmode_t = int(val_voutmode, 16)
                        val_p_out_t = int(val_p_out, 16) * 1000
                        
                        import ctypes
                        val_voutmode_t_t = ctypes.c_int8(val_voutmode_t << 3).value >>3
                        
                        if (val_voutmode_t_t) < 0:
                            val_p_out_t_f  =  val_p_out_t>> (-val_voutmode_t_t)
                        else:
                            val_p_out_t_f =  val_p_out_t << val_voutmode_t_t

                        return float(val_p_out_t_f)/1000

        return float(v_out)/1000

