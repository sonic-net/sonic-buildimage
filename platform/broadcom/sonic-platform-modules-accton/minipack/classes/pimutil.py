# Copyright (c) 2019 Edgecore Networks Corporation
#
# Licensed under the Apache License, Version 2.0 (the "License"); you may
# not use this file except in compliance with the License. You may obtain
# a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
#
# THIS CODE IS PROVIDED ON AN  *AS IS* BASIS, WITHOUT WARRANTIES OR
# CONDITIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING WITHOUT
# LIMITATION ANY IMPLIED WARRANTIES OR CONDITIONS OF TITLE, FITNESS
# FOR A PARTICULAR PURPOSE, MERCHANTABLITY OR NON-INFRINGEMENT.
#
# See the Apache Version 2.0 License for specific language governing
# permissions and limitations under the License.

# ------------------------------------------------------------------
# HISTORY:
#    mm/dd/yyyy (A.D.)
#    5/29/2019:  Jostar create for minipack
# -----------------------------------------------------------
try:
    import os
    import sys, getopt
    import subprocess
    import click
    import imp
    import logging
    import logging.config
    import logging.handlers
    import types
    import time  # this is only being used as part of the example
    import traceback
    from tabulate import tabulate
    import fbfpgaio
    import re
    import time
    from select import select
    #from ctypes import fbfpgaio 

except ImportError as e:
    raise ImportError('%s - required module not found' % str(e))


# pimutil.py
#
# Platform-specific PIM interface for SONiC
#

iob = {
    "revision": 0x0,
    "scratchpad": 0x4,
    "interrupt_status": 0x2C,
    "pim_status": 0x40,
    "pim_present_intr_mask": 0x44,
}

dom_base = [
 0xFFFFFFFF, # Padding
 0x40000,
 0x48000,
 0x50000,
 0x58000,
 0x60000,
 0x68000,
 0x70000,
 0x78000,
]


dom = {
  "revision": 0x0,
  "system_led": 0xC,
  "intr_status": 0x2C,
  "qsfp_present": 0x48,
  "qsfp_present_intr": 0x50,
  "qsfp_present_intr_mask": 0x58,
  "qsfp_intr": 0x60,
  "qsfp_intr_mask": 0x68,
  "qsfp_reset": 0x70,
  "qsfp_lp_mode": 0x78,
  "device_power_bad_status": 0x90,
  "port_led_color_profile": {
    0: 0x300,
    1: 0x300,
    2: 0x304,
    3: 0x304,
    4: 0x308,
    5: 0x308,
    6: 0x30C,
    7: 0x30C,
  },
  "port_led_control": {
    1: 0x310,
    2: 0x314,
    3: 0x318,
    4: 0x31C,
    5: 0x320,
    6: 0x324,
    7: 0x328,
    8: 0x32C,
    9: 0x330,
    10: 0x334,
    11: 0x338,
    12: 0x33C,
    13: 0x340,
    14: 0x344,
    15: 0x348,
    16: 0x34C,
  },
  "dom_control_config": 0x410,
  "dom_global_status": 0x414,
  "dom_data": 0x4000,
  

  "mdio": {
    "config":  0x0200,
    "command": 0x0204,
    "write":   0x0208,
    "read":    0x020C,
    "status":  0x0210,
    "intr_mask": 0x0214,
    "source_sel": 0x0218,
  }, # mdio
}

mdio_read_cmd = 0x1
mdio_write_cmd = 0x0
mdio_device_type = 0x1F


#fbfpgaio=cdll.LoadLibrary('./fbfpgaio.so')

def init_resources():
  fbfpgaio.hw_init()
  return

def release_resources():
  fbfpgaio.hw_release()
  return

def fpga_io(offset, data=None):
  if data is None:
    return fbfpgaio.hw_io(offset)
  else:
    fbfpgaio.hw_io(offset, data)
    return

def pim_io(pim, offset, data=None):
  global dom_base
  target_offset = dom_base[pim]+offset
  if data is None:
    retval = fpga_io(target_offset)
    #print ("0x%04X" % retval) # idebug
    return retval
  else:
    retval = fpga_io(target_offset, data)
    return retval


def show_pim_present():
  pim_status = fpga_io(iob["pim_status"])
  
  header =     "PIM # "
  status_str = "      "
  for shift in range(0,8):
    status = pim_status & (0x10000 << shift)   #[23:16] from pim_0 to pim_7
    header += " %d " % (shift+1)
    if status:
      status_str += (" | ")
    else:
      status_str += (" X ")
  print(header)
  print(status_str)

def show_qsfp_present_status(pim_num):
     status = fpga_io(dom_base[pim_num]+dom["qsfp_present"])
     interrupt = fpga_io(dom_base[pim_num]+dom["qsfp_present_intr"])
     mask = fpga_io(dom_base[pim_num]+dom["qsfp_present_intr_mask"])

     print()
     print("    (0x48)      (0x50)      (0x58)")
     print(("    0x%08X  0x%08X  0x%08X" %(status, interrupt, mask)))
     print("    Status      Interrupt   Mask")
     for row in range(8):
         output_str = str()
         status_left = bool(status & (0x1 << row*2))
         status_right = bool(status & (0x2 << row*2))
         interrupt_left = bool(interrupt & (0x1 << row*2))
         interrupt_right = bool(interrupt & (0x2 << row*2))
         mask_left = bool(mask & (0x1 << row*2))
         mask_right = bool(mask & (0x2 << row*2))
         print(("%2d:  %d  %d         %d  %d       %d %d" % \
                 (row*2+1, status_left, status_right, \
                        interrupt_left, interrupt_right, \
                        mask_left, mask_right)))
         print()



#pim_index start from 0 to 7
#port_index start from 0 to 127. Each 16-port is to one pim card.
class PimUtil(object):

    PORT_START = 0
    PORT_END = 127

    def __init__(self):
        self.value=1        

    def __del__(self):
        self.value=0
        
    def init_pim_fpga(self):
        init_resources()    
    
    def release_pim_fpga(self):
        release_resources()

    def get_pim_by_port(self, port_num):
        if port_num < self.PORT_START or port_num > self.PORT_END:
            return False
        pim_num=port_num/16
        return True, pim_num+1
    
    def get_onepimport_by_port(self, port_num):
        if port_num < self.PORT_START or port_num > self.PORT_END:
            return False
        if port_num < 16:
            return True, port_num
        else:
            return True, port_num%16   

    def get_pim_presence(self, pim_num):
        if pim_num <0 or pim_num > 7:
            return 0
        pim_status = fpga_io(iob["pim_status"])
        status = pim_status & (0x10000 << pim_num)
        if status:
            return 1 #present
        else:
            return 0 #not present

    #return code=0:100G. return code=1:400G
    def get_pim_board_id(self, pim_num):
        if pim_num <0 or pim_num > 7:
            return False
        board_id = fpga_io(dom_base[pim_num+1]+dom["revision"])
        board_id = board_id & 0x1
        if board_id==0x0:
            return 0
        else:
            return 1


    def get_pim_status(self, pim_num):
        if pim_num <0 or pim_num > 7:
            return 0xFF
        power_status =0
        #device_power_bad_status
        status=fpga_io(dom_base[pim_num+1]+dom["device_power_bad_status"])
        
        for x in range(0, 5):
            if status & ( (0xf) << (4*x) ) :
                power_status = power_status | (0x1 << x)
        
        if ( status & 0x1000000):
            power_status=power_status | (0x1 << 5)
        if ( status & 0x2000000):
            power_status=power_status | (0x1 << 6)
        if ( status & 0x8000000):
            power_status=power_status | (0x1 << 7)
        if ( status & 0x10000000):
            power_status=power_status | (0x1 << 8)
        if ( status & 0x40000000):
            power_status=power_status | (0x1 << 9)
        if ( status & 0x80000000):
            power_status=power_status | (0x1 << 10)

        return power_status
    #path=0:MDIO path is set on TH3. path=1:MDIO path is set on FPGA.
    def set_pim_mdio_source_sel(self, pim_num, path):
        if pim_num <0 or pim_num > 7:
            return False
        status= pim_io(pim_num+1, dom["mdio"]["source_sel"])
        
        if path==1:
            status = status | 0x2
        else:
            status = status & 0xfffffffd
        
        pim_io(pim_num+1, dom["mdio"]["source_sel"], status)
        return True
    #retrun code=0, path is TH3. retrun code=1, path is FPGA
    def get_pim_mdio_source_sel(sefl, pim_num):
        if pim_num <0 or pim_num > 7:
            return False
        path= pim_io(pim_num+1, dom["mdio"]["source_sel"])
        path = path & 0x2
        if path:
            return 1
        else:
            return 0
    
    #This api will set mdio path to MAC side.(At default, mdio path is set to FPGA side).
    def pim_init(self, pim_num):
        if pim_num <0 or pim_num > 7:
            return False
        status=self.set_pim_mdio_source_sel(pim_num, 0)
        #put init phy cmd here
    
    
    #return code="pim_dict[pim_num]='1' ":insert evt. return code="pim_dict[pim_num]='0' ":remove evt
    def get_pim_change_event(self, timeout=0):
        start_time = time.time()
        pim_dict = {}
        forever = False

        if timeout == 0:
            forever = True
        elif timeout > 0:
            timeout = timeout / float(1000) # Convert to secs
        else:
            print("get_transceiver_change_event:Invalid timeout value", timeout)
            return False, {}

        end_time = start_time + timeout
        if start_time > end_time:
            print('get_transceiver_change_event:' \
                       'time wrap / invalid timeout value', timeout)

            return False, {} # Time wrap or possibly incorrect timeout

        pim_mask_status = fpga_io(iob["pim_present_intr_mask"], 0xffff00000)
        
        while timeout >= 0: 
            new_pim_status=0           
            pim_status = fpga_io(iob["pim_status"])
            present_status= pim_status & 0xff0000
            change_status=pim_status & 0xff 
            interrupt_status = fpga_io(iob["interrupt_status"])
            
            for pim_num in range(0,8):
                if change_status & (0x1 << pim_num) :
                     status = present_status & (0x10000 << pim_num)
                     new_pim_status = new_pim_status | (0x1 << pim_num) #prepare to W1C to clear                     
                     if  status:
                        pim_dict[pim_num]='1'
                     else:
                        pim_dict[pim_num]='0'

            if change_status:
                new_pim_status = pim_status | new_pim_status #Write one to clear interrupt bit
                fpga_io(iob["pim_status"], new_pim_status)
                return True, pim_dict
            if forever:
                time.sleep(1)
            else:
                timeout = end_time - time.time()
                if timeout >= 1:
                    time.sleep(1) # We poll at 1 second granularity
                else:
                    if timeout > 0:
                        time.sleep(timeout)
                    return True, {}
        print("get_evt_change_event: Should not reach here.")
        return False, {}


    def get_pim_max_number(self):
        return 8
        
    #pim_num start from 0 to 7
    #color:0=amber, 1=blue
    #contrl:off(0),on(1), flash(2)    
    def set_pim_led(self, pim_num, color, control):
        if pim_num <0 or pim_num > 7:
            return False
        
        led_val=fpga_io(dom_base[pim_num+1]+dom["system_led"])
        
        if color==1:
            led_val = led_val | (0x8000 | 0x4000) #blue
        elif color==0:
            led_val = (led_val & ( ~ 0x8000)) | 0x4000 #amber
        else:
            print("Set RGB control to Green1")
            led_val = led_val & (~ 0x4000)
            led_val = led_val & (~ 0xfff)
            led_val = led_val | 0x0f0 #B.G.R Birghtness, set to Green
        
        if control==0:
           led_val = led_val & ( ~ 0x3000) #Off
        elif control==1:
           led_val = led_val & ( ~ 0x3000) #Off
           led_val = led_val | 0x1000 #On
        else:
            led_val = led_val | 0x3000  #Flash
        
        fpga_io(dom_base[pim_num+1]+dom["system_led"], led_val)
     
    def get_qsfp_presence(self, port_num):
         #xlate port to get pim_num
         status, pim_num=self.get_pim_by_port(port_num)

         if status==0:
            return False
         else:
            present = fpga_io(dom_base[pim_num]+dom["qsfp_present"])
         status, shift = self.get_onepimport_by_port(port_num)
         if status==0:
             return False
         else:
             if bool(present & (0x1 << shift)):
                 return 1 #present
             else:
                 return 0 #not present

    #return code: low_power(1) or high_power(0)
    def get_low_power_mode(self, port_num):
        status, pim_num=self.get_pim_by_port(port_num)
        
        if status==0:
            return False
        else:
            lp_mode = fpga_io(dom_base[pim_num]+dom["qsfp_lp_mode"])
        
        status, shift=self.get_onepimport_by_port(port_num)
        if status==0:
            return False
        else:
            if (lp_mode & (0x1 << shift)):
                return 1 #low
            else:
                return 0 #high

    #lpmode=1 to hold QSFP in low power mode. lpmode=0 to release QSFP from low power mode.
    def set_low_power_mode(self, port_num, mode):
        status, pim_num=self.get_pim_by_port(port_num)        
        if status==0:
            return False        
        val = fpga_io(dom_base[pim_num]+dom["qsfp_lp_mode"])        
        status, shift=self.get_onepimport_by_port(port_num)        
        if status==0:
            return False
        else:
            if mode==0:
                new_val = val & (~(0x1 << shift))
            else:
                new_val=val|(0x1 << shift)
        status=fpga_io(dom_base[pim_num]+dom["qsfp_lp_mode"], new_val)
        return status
    
    #port_dict[idx]=1 means get interrupt(change evt), port_dict[idx]=0 means no get interrupt
    def get_qsfp_interrupt(self):
        port_dict={}
        #show_qsfp_present_status(1)
        for pim_num in range(0, 8):
            fpga_io(dom_base[pim_num+1]+dom["qsfp_present_intr_mask"], 0xffff0000)
            fpga_io(dom_base[pim_num+1]+dom["qsfp_intr_mask"], 0xffff0000)
        for pim_num in range(0, 8):            
            clear_bit=0            
            qsfp_present_intr_status = fpga_io(dom_base[pim_num+1]+dom["qsfp_present_intr"])
            interrupt_status = qsfp_present_intr_status & 0xffff
            #time.sleep(2)            
            if interrupt_status:
                for idx in range (0,16):
                    port_idx=idx + (pim_num*16)
                    if interrupt_status & (0x1<<idx):
                        port_dict[port_idx]=1
                        clear_bit=clear_bit | (0x1<<idx)  #W1C to clear
                    else:
                        port_dict[port_idx]=0
                
                #W1C to clear
                fpga_io(dom_base[pim_num+1]+dom["qsfp_present_intr"], qsfp_present_intr_status | clear_bit) 
                
        return port_dict
        
    def reset(self, port_num):
        status, pim=self.get_pim_by_port(port_num)
        if status==0:
            return False
        
        val=fpga_io(dom_base[pim]+dom["qsfp_reset"])        
        status, shift=self.get_onepimport_by_port(port_num)
        if status==0:
            return False
        else:               
            val = val & (~(0x1 << shift))
        fpga_io(dom_base[pim]+dom["qsfp_reset"], val)
        return True

    #color:white(0), blue(1),red(2), orange(3),green(4)
    def set_port_led(self, port_num, color, control):
        status, pim_num=self.get_pim_by_port(port_num)        
        if status==0:
            return False
        status, port=self.get_onepimport_by_port(port_num)
        port=port+1
        led_val=fpga_io(dom_base[pim_num] + dom["port_led_control"][port])
        if control==0:
            led_val = led_val & (~ 0x3) #Off
        elif control==1:
            led_val = led_val & (~ 0x3) #Off
            led_val = led_val | 0x1     #On
        else:
            led_val = led_val | 0x3     #Flash        
            
        led_val=led_val & (~ 0x1C)    
        if color==0:
            led_val=led_val & (~ 0x1C) #white
        elif color==1:            
            led_val=led_val | 0x8  #blue
        elif color==2:
            led_val=led_val | 0x10 #red
        elif color==3:
            led_val=led_val | 0x14 #oragne
        else:
            led_val=led_val | 0x1C #green
        
        fpga_io(dom_base[pim_num] + dom["port_led_control"][port], led_val)
        
        return True
    
    def get_port_led(self, port_num):
        status, pim_num=self.get_pim_by_port(port_num)        
        if status==0:
            return False
        status, port=self.get_onepimport_by_port(port_num)
        led_val=fpga_io(dom_base[pim_num] + dom["port_led_control"][port+1])
        control=led_val & 0x3
        if control==0x3:
            control=2
        elif control==0x1:
            control=1
        else:
            control=0
        
        color = led_val & 0x1C
        if color==0:
            color=0 #white
        elif color==0xC:            
            color=1 #blue
        elif color==0x14:
            color=2 #red
        elif color==0x18:
            color=3 #oragne
        elif color==0x1C:
            color=4 #green

        print("color=%d, control=%d"%(color, control))         
        return color, control
        
def main(argv):
    init_resources()    
    pim=PimUtil()
    print("Test Board ID")
    for x in range(0,8):
        val=pim.get_pim_board_id(x)
        print("pim=%d"%x)
        if val==0:
            print("100G board")
        else:
            print("400G board")
    
    print("Test pim presence")
    for x in range(0,8):
        pres=pim.get_pim_presence(x)
        print("pim=%d, presence=%d"%(x, pres))   
        
    print("Test pim status")
    for x in range(0,8):
        power_status=pim.get_pim_status(x)
        print("pim=%d power_status=0x%x"%(x, power_status))
    
    release_resources()

if __name__ == '__main__':
    main(sys.argv[1:])
