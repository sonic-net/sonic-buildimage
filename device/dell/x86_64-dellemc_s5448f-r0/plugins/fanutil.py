#!/usr/bin/env python
#############################################################################
# Dell
#
# Module contains an implementation of SONiC FAN Base API and
# provides various info about the FANs which are available in the platform
#
#############################################################################
import os.path
import logging
import commands

try:
    from sonic_fan.fan_base import FanBase
except ImportError as e:
    raise ImportError (str(e) + "- required module not found")

class FanUtil(FanBase):
    """Platform-specific FANutil class"""
    FANTRAY_NUM_ON_MAIN_BOARD = 6
    NUM_FANS_PERTRAY = 2
    FANTRAY_NUM_START_IDX = 1
    FRU_FAN_START_IDX = 1
    IPMI_FAN_PRESENCE = "ipmitool sensor get FAN{0}_prsnt"
    IPMI_FAN_FRONT_SPEED = "ipmitool sdr get Fan{0}_Front_rpm"
    IPMI_FAN_REAR_SPEED = "ipmitool sdr get Fan{0}_Rear_rpm"
    IPMI_FRU_DATA = "ipmitool fru print {0}"

    def __init__(self, log_level=logging.DEBUG):
        FanBase.__init__(self)
        self.num_fans = (self.FANTRAY_NUM_ON_MAIN_BOARD*self.NUM_FANS_PERTRAY)

    def get_fan_status(self,fan_id):
        try:
           ret_status, ipmi_cmd_ret = commands.getstatusoutput(self.IPMI_FAN_PRESENCE.format(fan_id))
           if ret_status == 0:
              return(ipmi_cmd_ret.splitlines()[5].strip(' ').strip('[]'))
        except Exception:
           logging.error('Failed to execute : %s'%self.IPMI_FAN_PRESENCE.format(fan_id))
   
    def get_front_fan_speed(self,fan_id):
        try:
           ret_status, ipmi_cmd_ret = commands.getstatusoutput(self.IPMI_FAN_FRONT_SPEED.format(fan_id))
           if ret_status == 0:
              rdata = ipmi_cmd_ret.splitlines()[3].split(':')[1].split(' ')[1]
              return rdata
        except Exception:
           logging.error('Failed to execute : %s'%self.IPMI_FAN_FRONT_SPEED.format(fan_id))
    
    def get_rear_fan_speed(self,fan_id):
        try:
           ret_status, ipmi_cmd_ret = commands.getstatusoutput(self.IPMI_FAN_REAR_SPEED.format(fan_id))
           if ret_status == 0:
              rdata = ipmi_cmd_ret.splitlines()[3].split(':')[1].split(' ')[1]
              return rdata

        except Exception:
           logging.error('Failed to execute : %s'%self.IPMI_FAN_REAR_SPEED.format(fan_id))


    # Read FAN FRU info
    def get_fan_direction_from_fru(self,fru_id,reg_name):
        output = None
        try:
           status, ipmi_fru_list = commands.getstatusoutput(self.IPMI_FRU_DATA.format(fru_id))
           if status == 0:
              for item in ipmi_fru_list.split("\n"):
                  if reg_name in item:
                     output = item.strip()
                     if output is None:
                        logging.error('\nFailed to fetch: ' +  reg_name + ' sensor ')
                     output = output.split(':')[1].strip(' ')
                     if output == 'F2B' or output == 'B2F':
                        return output
        except Exception:
           logging.error('Failed to execute:' + ipmi_fru_list)

    def get_num_fans(self):
        return self.num_fans

    def get_presence(self, index):
        if index is None:
           return False

        if index < self.FANTRAY_NUM_START_IDX or index > self.FANTRAY_NUM_START_IDX + self.num_fans - 1:
           logging.error('Invalid FAN index:%d', index)
           return False

        tray_index = ((index-1)/self.NUM_FANS_PERTRAY) + 1

        if (self.get_fan_status(tray_index) == 'Device Present'):  
            return True
        else:
            return False

    def get_status(self, index):
        if index is None:
           return False

        if index < self.FANTRAY_NUM_START_IDX or index > self.FANTRAY_NUM_START_IDX + self.num_fans - 1:
           logging.error('Invalid FAN index:%d', index)
           return False

        tray_index = ((index-1)/self.NUM_FANS_PERTRAY) + 1
        fantray_front_speed=self.get_front_fan_speed(tray_index)
        fantray_rear_speed=self.get_rear_fan_speed(tray_index)

        if (fantray_front_speed != '0' and fantray_rear_speed != '0'):
            return True
        else:
            return False


    def get_direction(self, index):
        if index is None:
           return None

        if index < self.FANTRAY_NUM_START_IDX or index > self.FANTRAY_NUM_START_IDX + self.num_fans - 1:
           logging.error('Invalid FAN index:%d', index)
           return None

        tray_index = ((index-1)/self.NUM_FANS_PERTRAY)
        fru_id = self.FRU_FAN_START_IDX + tray_index
        direction = self.get_fan_direction_from_fru(fru_id,'Board Extra')

        if direction == 'B2F':
           return "INTAKE"
        elif direction == 'F2B':
           return "EXHAUST"
        else:
           return None


    def get_speed(self, index):
        if index is None:
           return 0

        if index < self.FANTRAY_NUM_START_IDX or index > self.FANTRAY_NUM_START_IDX + self.num_fans - 1:
           logging.error('Invalid FAN index:%d', index)
           return 0

        tray_index = ((index-1)/self.NUM_FANS_PERTRAY) + 1

        if (index % 2 != 0):
           fantray_speed=self.get_front_fan_speed(tray_index)
        else:
           fantray_speed=self.get_rear_fan_speed(tray_index)
        
        if (self.get_presence(index) == True):
            return int(fantray_speed.strip())
        else:
            return 0

    def set_speed(self, val):
        logging.error("Not allowed to set fan speed!")

        return False
