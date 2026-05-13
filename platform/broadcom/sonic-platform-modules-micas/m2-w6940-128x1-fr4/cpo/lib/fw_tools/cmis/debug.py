import os
import sys
from array import array
from GS_timing import *
from util import UtilObj as u
import cmis_mcu_i2c as cmis_smb
import logging
import math
from array import array
from oe_cmis_rw import oe_cmis_i2c_rw as oe_cmis_util


class DebugObj(cmis_smb.cmis_mcu_mezz_smb):

    def __init__(self, twi_dev=None, oe_mcu=0):
        self.twi_dev = twi_dev
        self.oe_mcu = oe_mcu
        self.cmis_util = oe_cmis_util()
        super().__init__(twi_dev=twi_dev, oe_mcu=oe_mcu)

    def __enter__(self):
        self.open()
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        self.close()
        return

    def debug_enable_inline_debug(self):
        self.enter_debug_mode()
    
    def debug_dump_laser_input_power(self):
        self.debug_enable_inline_debug()
        pwr_payload = array('B', 32 * [0])
        self.write(msa_page=0xBF, offset=130, data=[0x00, 0x00])
        self.write(msa_page=0xBF, offset=128, data=[0x00, 0x02])
        delay(1)
        self.poll_status(status_reg=255, status_val=0x01)
        self.read_from_data_page(data_page=0xC0, data=pwr_payload)
        laser_pwr = []
        for ch in range(16):
            laser_pwr.append(u.uint8_to_uint16(pwr_payload[2*ch:2*ch+2]))
        for ch in range(16):
            PmW = laser_pwr[ch]/100
            if PmW > 0:
                Pdb = 10 * math.log(PmW, 10)
            else:
                Pdb = -40
            print('EIC Laser%d Input Power: %.03f mW %.03f dBm' % (ch, PmW, Pdb))
        print('')
        return laser_pwr

class DebugException(Exception):
    def __init__(self, detailed_error_message):
        super(DebugException, self).__init__(detailed_error_message)
