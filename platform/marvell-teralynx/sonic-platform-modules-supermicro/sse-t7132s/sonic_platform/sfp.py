#!/usr/bin/env python
"""
#############################################################################
# SuperMicro SSE-T7132S
#
# Sfp contains an implementation of SONiC Platform Base API and
# provides the sfp status which are available in the platform
#
#############################################################################
"""

try:
    import os
    import time
    from sonic_platform_base.sonic_xcvr.sfp_optoe_base import SfpOptoeBase
    from sonic_platform_base.thermal_base import ThermalBase
    from sonic_platform_base.sonic_xcvr.fields import consts

except ImportError as err:
    raise ImportError(str(err) + "- required module not found")

QSFP_INFO_OFFSET = 128
SFP_INFO_OFFSET = 0
QSFP_DD_PAGE0 = 0

SFP_TYPE_LIST = [
    '0x3' # SFP/SFP+/SFP28 and later
]
QSFP_TYPE_LIST = [
    '0x0c', # QSFP
    '0x0d', # QSFP+ or later
    '0x11'  # QSFP28 or later
]
QSFP_DD_TYPE_LIST = [
    '0x18' #QSFP_DD Type
]

OSFP_TYPE_LIST = [
    '0x19' # OSFP 8X Type
]

SFP_TYPE = "SFP"
QSFP_TYPE = "QSFP"
OSFP_TYPE = "OSFP"
QSFP_DD_TYPE = "QSFP_DD"
SFP_NAME = "Ethernet{}"

PORT_START = 0
PORT_END = 34
QSFP_PORT_START = 0
QSFP_PORT_END = 32

I2C_EEPROM_PATH = '/sys/bus/i2c/devices/i2c-{0}/{0}-0050/eeprom'
PORT_INFO_PATH= '/sys/class/t7132s_cpld'

irq_dict_keys = [
    'DP1State', 'DP2State', 'DP3State', 'DP4State', 'DP5State', 'DP6State', 'DP7State', 'DP8State',
    'Interrupt',
    'config_state_hostlane1', 'config_state_hostlane2', 'config_state_hostlane3', 'config_state_hostlane4',
    'config_state_hostlane5', 'config_state_hostlane6', 'config_state_hostlane7', 'config_state_hostlane8',
    'datapath_firmware_fault',
    'dpinit_pending_hostlane',
    'dpinit_pending_hostlane1', 'dpinit_pending_hostlane2', 'dpinit_pending_hostlane3', 'dpinit_pending_hostlane4',
    'dpinit_pending_hostlane5', 'dpinit_pending_hostlane6', 'dpinit_pending_hostlane7', 'dpinit_pending_hostlane8',
    'dpstatechanged_flag',
    'lasertemphighalarm_flag', 'lasertemphighwarning_flag', 'lasertemplowalarm_flag', 'lasertemplowwarning_flag',
    'module_fault_cause', 'module_firmware_fault', 'module_state', 'module_state_changed',
    'rxcdrlol', 'rxcdrlol1', 'rxcdrlol2', 'rxcdrlol3', 'rxcdrlol4', 'rxcdrlol5', 'rxcdrlol6', 'rxcdrlol7', 'rxcdrlol8',
    'rxlos', 'rxlos1', 'rxlos2', 'rxlos3', 'rxlos4', 'rxlos5', 'rxlos6', 'rxlos7', 'rxlos8',
    'rxoutput_status_hostlane', 'rxoutput_status_hostlane1', 'rxoutput_status_hostlane2',
    'rxoutput_status_hostlane3', 'rxoutput_status_hostlane4', 'rxoutput_status_hostlane5',
    'rxoutput_status_hostlane6', 'rxoutput_status_hostlane7', 'rxoutput_status_hostlane8',
    'rxpowerhighalarm_flag', 'rxpowerhighalarm_flag1', 'rxpowerhighalarm_flag2',
    'rxpowerhighalarm_flag3', 'rxpowerhighalarm_flag4', 'rxpowerhighalarm_flag5',
    'rxpowerhighalarm_flag6', 'rxpowerhighalarm_flag7', 'rxpowerhighalarm_flag8',
    'rxpowerhighwarning_flag', 'rxpowerhighwarning_flag1', 'rxpowerhighwarning_flag2',
    'rxpowerhighwarning_flag3', 'rxpowerhighwarning_flag4', 'rxpowerhighwarning_flag5',
    'rxpowerhighwarning_flag6', 'rxpowerhighwarning_flag7', 'rxpowerhighwarning_flag8',
    'rxpowerlowalarm_flag', 'rxpowerlowalarm_flag1', 'rxpowerlowalarm_flag2',
    'rxpowerlowalarm_flag3', 'rxpowerlowalarm_flag4', 'rxpowerlowalarm_flag5',
    'rxpowerlowalarm_flag6', 'rxpowerlowalarm_flag7', 'rxpowerlowalarm_flag8',
    'rxpowerlowwarning_flag', 'rxpowerlowwarning_flag1', 'rxpowerlowwarning_flag2',
    'rxpowerlowwarning_flag3', 'rxpowerlowwarning_flag4', 'rxpowerlowwarning_flag5',
    'rxpowerlowwarning_flag6', 'rxpowerlowwarning_flag7', 'rxpowerlowwarning_flag8',
    'temphighalarm_flag', 'temphighwarning_flag', 'templowalarm_flag', 'templowwarning_flag',
    'txadaptiveinputeqfault_flag',
    'txbiashighalarm_flag', 'txbiashighalarm_flag1', 'txbiashighalarm_flag2',
    'txbiashighalarm_flag3', 'txbiashighalarm_flag4', 'txbiashighalarm_flag5',
    'txbiashighalarm_flag6', 'txbiashighalarm_flag7', 'txbiashighalarm_flag8',
    'txbiashighwarning_flag', 'txbiashighwarning_flag1', 'txbiashighwarning_flag2',
    'txbiashighwarning_flag3', 'txbiashighwarning_flag4', 'txbiashighwarning_flag5',
    'txbiashighwarning_flag6', 'txbiashighwarning_flag7', 'txbiashighwarning_flag8',
    'txbiaslowalarm_flag', 'txbiaslowalarm_flag1', 'txbiaslowalarm_flag2',
    'txbiaslowalarm_flag3', 'txbiaslowalarm_flag4', 'txbiaslowalarm_flag5',
    'txbiaslowalarm_flag6', 'txbiaslowalarm_flag7', 'txbiaslowalarm_flag8',
    'txbiaslowwarning_flag', 'txbiaslowwarning_flag1', 'txbiaslowwarning_flag2',
    'txbiaslowwarning_flag3', 'txbiaslowwarning_flag4', 'txbiaslowwarning_flag5',
    'txbiaslowwarning_flag6', 'txbiaslowwarning_flag7', 'txbiaslowwarning_flag8',
    'txcdrlol_hostlane', 'txcdrlol_hostlane1', 'txcdrlol_hostlane2',
    'txcdrlol_hostlane3', 'txcdrlol_hostlane4', 'txcdrlol_hostlane5',
    'txcdrlol_hostlane6', 'txcdrlol_hostlane7', 'txcdrlol_hostlane8',
    'txfault', 'txfault1', 'txfault2', 'txfault3', 'txfault4', 'txfault5', 'txfault6', 'txfault7', 'txfault8',
    'txlos_hostlane', 'txlos_hostlane1', 'txlos_hostlane2',
    'txlos_hostlane3', 'txlos_hostlane4', 'txlos_hostlane5',
    'txlos_hostlane6', 'txlos_hostlane7', 'txlos_hostlane8',
    'txoutput_status', 'txoutput_status1', 'txoutput_status2',
    'txoutput_status3', 'txoutput_status4', 'txoutput_status5',
    'txoutput_status6', 'txoutput_status7', 'txoutput_status8',
    'txpowerhighalarm_flag', 'txpowerhighalarm_flag1', 'txpowerhighalarm_flag2',
    'txpowerhighalarm_flag3', 'txpowerhighalarm_flag4', 'txpowerhighalarm_flag5',
    'txpowerhighalarm_flag6', 'txpowerhighalarm_flag7', 'txpowerhighalarm_flag8',
    'txpowerhighwarning_flag', 'txpowerhighwarning_flag1', 'txpowerhighwarning_flag2',
    'txpowerhighwarning_flag3', 'txpowerhighwarning_flag4', 'txpowerhighwarning_flag5',
    'txpowerhighwarning_flag6', 'txpowerhighwarning_flag7', 'txpowerhighwarning_flag8',
    'txpowerlowalarm_flag', 'txpowerlowalarm_flag1', 'txpowerlowalarm_flag2',
    'txpowerlowalarm_flag3', 'txpowerlowalarm_flag4', 'txpowerlowalarm_flag5',
    'txpowerlowalarm_flag6', 'txpowerlowalarm_flag7', 'txpowerlowalarm_flag8',
    'txpowerlowwarning_flag', 'txpowerlowwarning_flag1', 'txpowerlowwarning_flag2',
    'txpowerlowwarning_flag3', 'txpowerlowwarning_flag4', 'txpowerlowwarning_flag5',
    'txpowerlowwarning_flag6', 'txpowerlowwarning_flag7', 'txpowerlowwarning_flag8',
    'vcchighalarm_flag', 'vcchighwarning_flag', 'vcclowalarm_flag', 'vcclowwarning_flag'
]

class Sfp(SfpOptoeBase):
    """Platform-specific Sfp class"""
    PLATFORM = "x86_64-supermicro_sse_t7132s-r0"
    HWSKU = "Supermicro_sse_t7132s"

    _port_to_offset = [11, 30, 12, 29, 13, 28, 14, 27, 15, 34,
                       16, 33, 17, 32, 18, 31, 19, 38, 20, 37,
                       21, 36, 22, 35, 23, 42, 24, 41, 25, 40,
                       26, 39,
                       43, 44]
    _port_to_lanes = [[241, 242, 243, 244, 245, 246, 247, 248],
                      [249, 250, 251, 252, 253, 254, 255, 256],
                      [225, 226, 227, 228, 229, 230, 231, 232],
                      [233, 234, 235, 236, 237, 238, 239, 240],
                      [217, 218, 219, 220, 221, 222, 223, 224],
                      [209, 210, 211, 212, 213, 214, 215, 216],
                      [201, 202, 203, 204, 205, 206, 207, 208],
                      [193, 194, 195, 196, 197, 198, 199, 200],
                      [185, 186, 187, 188, 189, 190, 191, 192],
                      [177, 178, 179, 180, 181, 182, 183, 184],
                      [169, 170, 171, 172, 173, 174, 175, 176],
                      [161, 162, 163, 164, 165, 166, 167, 168],
                      [153, 154, 155, 156, 157, 158, 159, 160],
                      [145, 146, 147, 148, 149, 150, 151, 152],
                      [137, 138, 139, 140, 141, 142, 143, 144],
                      [129, 130, 131, 132, 133, 134, 135, 136],
                      [121, 122, 123, 124, 125, 126, 127, 128],
                      [113, 114, 115, 116, 117, 118, 119, 120],
                      [105, 106, 107, 108, 109, 110, 111, 112],
                      [ 97,  98,  99, 100, 101, 102, 103, 104],
                      [ 89,  90,  91,  92,  93,  94,  95,  96],
                      [ 81,  82,  83,  84,  85,  86,  87,  88],
                      [ 73,  74,  75,  76,  77,  78,  79,  80],
                      [ 65,  66,  67,  68,  69,  70,  71,  72],
                      [ 57,  58,  59,  60,  61,  62,  63,  64],
                      [ 49,  50,  51,  52,  53,  54,  55,  56],
                      [ 41,  42,  43,  44,  45,  46,  47,  48],
                      [ 33,  34,  35,  36,  37,  38,  39,  40],
                      [ 25,  26,  27,  28,  29,  30,  31,  32],
                      [ 17,  18,  19,  20,  21,  22,  23,  24],
                      [  9,  10,  11,  12,  13,  14,  15,  16],
                      [  1,   2,   3,   4,   5,   6,   7,   8],
                      [257],
                      [258]]

    def __init__(self, index, names):
        SfpOptoeBase.__init__(self)

        self.index = index      # for sfputil show error-status --fetch-from-hardware
        self._master_port = self.index - 1
        # port_type is the native port type and sfp_type is the transceiver type
        # sfp_type will be detected in get_transceiver_info
        if self._master_port < QSFP_PORT_END:
            self.port_type = QSFP_DD_TYPE
            self.NUM_CHANNELS = 8
            self.port_name = "QSFP" + str(self.index)
        else:
            self.port_type = SFP_TYPE
            self.NUM_CHANNELS = 1
            self.port_name = "SFP" + str(self.index - QSFP_PORT_END)
        self._name = names if names else [self.port_name]   # in a list
        self.sfp_type = self.port_type
        self.sfp_eeprom_path = self.get_eeprom_path()
        self._initialize_media(delay=False)
        self.doing_reset = False          # to tell from CPLD auto reset when removed
        self.xcvr_id = 0

    def get_eeprom_path(self):
        """
        Returns SFP eeprom path
        """
        port_eeprom_path = I2C_EEPROM_PATH.format(self._port_to_offset[self._master_port])
        return port_eeprom_path

    def get_name(self):
        """
        Retrieves the name of the device
            Returns:
            string: The name of the device
        """
        return self._name[0]

    def get_names(self):
        """
        Retrieves the name list of the device
            Returns:
            string: The name list of the device
        """
        return self._name

    def _initialize_media(self, delay=False):
        """
        Initialize the media type and eeprom driver for SFP
        """
        if delay:
            time.sleep(1)
            self._xcvr_api = None
            self.get_xcvr_api()

        self.set_media_type()
        self.reinit_sfp_driver()

    def get_presence(self):
        """
        Retrieves the presence of the SFP
        Returns:
            bool: True if SFP is present, False if not
        """
        sysfs_filename = "sfp_modabs" if self.port_type == SFP_TYPE else "qsfp_modprs"
        reg_path = "/".join([PORT_INFO_PATH, self.port_name, sysfs_filename])

        # Read status
        try:
            with open(reg_path) as reg_file:
                content = reg_file.readline().rstrip()
                reg_value = int(content)
                # Module present is active low
                if reg_value == 0:
                    return True
        except IOError as e:
            print("Error: unable to open file: %s" % str(e))
            return False

        # not present
        return False

    def _compact_lanes(self, xcvrst, key):
        """
        Get key+"8" to key+"1" from xcvrst and return a string of binary of lane8 to lane1.
        True:1, Others:0
        Ex. key = rxlos,
            xcvrst['rxlos1'] = True,
            xcvrst['rxlos2'] = True,
            xcvrst['rxlos3'] = 'N/A',
            xcvrst['rxlos4'] = False,
            xcvrst['rxlos5'] = False,
            xcvrst['rxlos6'] = False,
            xcvrst['rxlos7'] = False,
            xcvrst['rxlos8'] = False,
        So return the string "00000011"
        Return None for fail.
        """
        s = ""
        for lane in range(1, 9):
            k = "{}{}".format(key, lane)
            try:
                b = "1" if xcvrst.get(k) == True else "0"
                s = b + s
            except Except:
                return None
        return s

    def get_transceiver_interrupt_info(self):
        """
        Retrieves transceiver interrupt info of this SFP.
        This function calls get_transceiver_status() and merges some values into a compact format.
        Returns:
            A dict which contains following keys/values : (by get_transceiver_status())
        ================================================================================
        key                          = TRANSCEIVER_STATUS|ifname        ; Error information for module on port
        ; field                      = value
        module_state                 = 1*255VCHAR                       ; current module state (ModuleLowPwr, ModulePwrUp, ModuleReady, ModulePwrDn, Fault)
        module_fault_cause           = 1*255VCHAR                       ; reason of entering the module fault state
        datapath_firmware_fault      = BOOLEAN                          ; datapath (DSP) firmware fault
        module_firmware_fault        = BOOLEAN                          ; module firmware fault
        module_state_changed         = BOOLEAN                          ; module state changed
        datapath_hostlane1           = 1*255VCHAR                       ; data path state indicator on host lane 1
        datapath_hostlane2           = 1*255VCHAR                       ; data path state indicator on host lane 2
        datapath_hostlane3           = 1*255VCHAR                       ; data path state indicator on host lane 3
        datapath_hostlane4           = 1*255VCHAR                       ; data path state indicator on host lane 4
        datapath_hostlane5           = 1*255VCHAR                       ; data path state indicator on host lane 5
        datapath_hostlane6           = 1*255VCHAR                       ; data path state indicator on host lane 6
        datapath_hostlane7           = 1*255VCHAR                       ; data path state indicator on host lane 7
        datapath_hostlane8           = 1*255VCHAR                       ; data path state indicator on host lane 8
        txoutput_status              = BOOLEAN                          ; tx output status on media lane
        rxoutput_status_hostlane1    = BOOLEAN                          ; rx output status on host lane 1
        rxoutput_status_hostlane2    = BOOLEAN                          ; rx output status on host lane 2
        rxoutput_status_hostlane3    = BOOLEAN                          ; rx output status on host lane 3
        rxoutput_status_hostlane4    = BOOLEAN                          ; rx output status on host lane 4
        rxoutput_status_hostlane5    = BOOLEAN                          ; rx output status on host lane 5
        rxoutput_status_hostlane6    = BOOLEAN                          ; rx output status on host lane 6
        rxoutput_status_hostlane7    = BOOLEAN                          ; rx output status on host lane 7
        rxoutput_status_hostlane8    = BOOLEAN                          ; rx output status on host lane 8
        txfault                      = BOOLEAN                          ; tx fault flag on media lane
        txlos_hostlane1              = BOOLEAN                          ; tx loss of signal flag on host lane 1
        txlos_hostlane2              = BOOLEAN                          ; tx loss of signal flag on host lane 2
        txlos_hostlane3              = BOOLEAN                          ; tx loss of signal flag on host lane 3
        txlos_hostlane4              = BOOLEAN                          ; tx loss of signal flag on host lane 4
        txlos_hostlane5              = BOOLEAN                          ; tx loss of signal flag on host lane 5
        txlos_hostlane6              = BOOLEAN                          ; tx loss of signal flag on host lane 6
        txlos_hostlane7              = BOOLEAN                          ; tx loss of signal flag on host lane 7
        txlos_hostlane8              = BOOLEAN                          ; tx loss of signal flag on host lane 8
        txcdrlol_hostlane1           = BOOLEAN                          ; tx clock and data recovery loss of lock on host lane 1
        txcdrlol_hostlane2           = BOOLEAN                          ; tx clock and data recovery loss of lock on host lane 2
        txcdrlol_hostlane3           = BOOLEAN                          ; tx clock and data recovery loss of lock on host lane 3
        txcdrlol_hostlane4           = BOOLEAN                          ; tx clock and data recovery loss of lock on host lane 4
        txcdrlol_hostlane5           = BOOLEAN                          ; tx clock and data recovery loss of lock on host lane 5
        txcdrlol_hostlane6           = BOOLEAN                          ; tx clock and data recovery loss of lock on host lane 6
        txcdrlol_hostlane7           = BOOLEAN                          ; tx clock and data recovery loss of lock on host lane 7
        txcdrlol_hostlane8           = BOOLEAN                          ; tx clock and data recovery loss of lock on host lane 8
        rxlos                        = BOOLEAN                          ; rx loss of signal flag on media lane
        rxcdrlol                     = BOOLEAN                          ; rx clock and data recovery loss of lock on media lane
        config_state_hostlane1       = 1*255VCHAR                       ; configuration status for the data path of host line 1
        config_state_hostlane2       = 1*255VCHAR                       ; configuration status for the data path of host line 2
        config_state_hostlane3       = 1*255VCHAR                       ; configuration status for the data path of host line 3
        config_state_hostlane4       = 1*255VCHAR                       ; configuration status for the data path of host line 4
        config_state_hostlane5       = 1*255VCHAR                       ; configuration status for the data path of host line 5
        config_state_hostlane6       = 1*255VCHAR                       ; configuration status for the data path of host line 6
        config_state_hostlane7       = 1*255VCHAR                       ; configuration status for the data path of host line 7
        config_state_hostlane8       = 1*255VCHAR                       ; configuration status for the data path of host line 8
        dpinit_pending_hostlane1     = BOOLEAN                          ; data path configuration updated on host lane 1
        dpinit_pending_hostlane2     = BOOLEAN                          ; data path configuration updated on host lane 2
        dpinit_pending_hostlane3     = BOOLEAN                          ; data path configuration updated on host lane 3
        dpinit_pending_hostlane4     = BOOLEAN                          ; data path configuration updated on host lane 4
        dpinit_pending_hostlane5     = BOOLEAN                          ; data path configuration updated on host lane 5
        dpinit_pending_hostlane6     = BOOLEAN                          ; data path configuration updated on host lane 6
        dpinit_pending_hostlane7     = BOOLEAN                          ; data path configuration updated on host lane 7
        dpinit_pending_hostlane8     = BOOLEAN                          ; data path configuration updated on host lane 8
        temphighalarm_flag           = BOOLEAN                          ; temperature high alarm flag
        temphighwarning_flag         = BOOLEAN                          ; temperature high warning flag
        templowalarm_flag            = BOOLEAN                          ; temperature low alarm flag
        templowwarning_flag          = BOOLEAN                          ; temperature low warning flag
        vcchighalarm_flag            = BOOLEAN                          ; vcc high alarm flag
        vcchighwarning_flag          = BOOLEAN                          ; vcc high warning flag
        vcclowalarm_flag             = BOOLEAN                          ; vcc low alarm flag
        vcclowwarning_flag           = BOOLEAN                          ; vcc low warning flag
        txpowerhighalarm_flag        = BOOLEAN                          ; tx power high alarm flag
        txpowerlowalarm_flag         = BOOLEAN                          ; tx power low alarm flag
        txpowerhighwarning_flag      = BOOLEAN                          ; tx power high warning flag
        txpowerlowwarning_flag       = BOOLEAN                          ; tx power low alarm flag
        rxpowerhighalarm_flag        = BOOLEAN                          ; rx power high alarm flag
        rxpowerlowalarm_flag         = BOOLEAN                          ; rx power low alarm flag
        rxpowerhighwarning_flag      = BOOLEAN                          ; rx power high warning flag
        rxpowerlowwarning_flag       = BOOLEAN                          ; rx power low warning flag
        txbiashighalarm_flag         = BOOLEAN                          ; tx bias high alarm flag
        txbiaslowalarm_flag          = BOOLEAN                          ; tx bias low alarm flag
        txbiashighwarning_flag       = BOOLEAN                          ; tx bias high warning flag
        txbiaslowwarning_flag        = BOOLEAN                          ; tx bias low warning flag
        lasertemphighalarm_flag      = BOOLEAN                          ; laser temperature high alarm flag
        lasertemplowalarm_flag       = BOOLEAN                          ; laser temperature low alarm flag
        lasertemphighwarning_flag    = BOOLEAN                          ; laser temperature high warning flag
        lasertemplowwarning_flag     = BOOLEAN                          ; laser temperature low warning flag
        prefecberhighalarm_flag      = BOOLEAN                          ; prefec ber high alarm flag
        prefecberlowalarm_flag       = BOOLEAN                          ; prefec ber low alarm flag
        prefecberhighwarning_flag    = BOOLEAN                          ; prefec ber high warning flag
        prefecberlowwarning_flag     = BOOLEAN                          ; prefec ber low warning flag
        postfecberhighalarm_flag     = BOOLEAN                          ; postfec ber high alarm flag
        postfecberlowalarm_flag      = BOOLEAN                          ; postfec ber low alarm flag
        postfecberhighwarning_flag   = BOOLEAN                          ; postfec ber high warning flag
        postfecberlowwarning_flag    = BOOLEAN                          ; postfec ber low warning flag

        # t7132 add
        dpinit_pending_hostlane      = BINARY STRING                    ; compact of dpinit_pending_hostlane8..1
        rxcdrlol                     = BINARY STRING                    ; compact of rxcdrlol8..1
        rxlos                        = BINARY STRING                    ; compact of rxlos8..1
        rxoutput_status_hostlane     = BINARY STRING                    ; compact of rxoutput_status_hostlane8..1
        rxpowerhighalarm_flag        = BINARY STRING                    ; compact of rxpowerhighalarm_flag8..1
        rxpowerhighwarning_flag      = BINARY STRING                    ; compact of rxpowerhighwarning_flag8..1
        rxpowerlowalarm_flag         = BINARY STRING                    ; compact of rxpowerlowalarm_flag8..1
        rxpowerlowwarning_flag       = BINARY STRING                    ; compact of rxpowerlowwarning_flag8..1
        txbiashighalarm_flag         = BINARY STRING                    ; compact of txbiashighalarm_flag8..1
        txbiashighwarning_flag       = BINARY STRING                    ; compact of txbiashighwarning_flag8..1
        txbiaslowalarm_flag          = BINARY STRING                    ; compact of txbiaslowalarm_flag8..1
        txbiaslowwarning_flag        = BINARY STRING                    ; compact of txbiaslowwarning_flag8..1
        txcdrlol_hostlane            = BINARY STRING                    ; compact of txcdrlol_hostlane8..1
        txfault                      = BINARY STRING                    ; compact of txfault8..1
        txlos_hostlane               = BINARY STRING                    ; compact of txlos_hostlane8..1
        txoutput_status              = BINARY STRING                    ; compact of txoutput_status8..1
        txpowerhighalarm_flag        = BINARY STRING                    ; compact of txpowerhighalarm_flag8..1
        txpowerhighwarning_flag      = BINARY STRING                    ; compact of txpowerhighwarning_flag8..1
        txpowerlowalarm_flag         = BINARY STRING                    ; compact of txpowerlowalarm_flag8..1
        txpowerlowwarning_flag       = BINARY STRING                    ; compact of txpowerlowwarning_flag8..1
        Interrupt                    = STRING                           ; 'Interrupt not asserted' / 'Interrupt asserted' / 'N/A'
        dpstatechanged_flag          = BINARY STRING                    ; compact of dpstatechanged_flag8..1
        txadaptiveinputeqfault_flag  = BINARY STRING                    ; compact of txadaptiveinputeqfault_flag8..1
        ================================================================================
        """

        transceiver_irq_dict = {}
        transceiver_irq_dict = dict.fromkeys(irq_dict_keys, 'N/A')

        if not self.get_presence():
            return transceiver_irq_dict

        self.set_media_type()
        if self.sfp_type == QSFP_DD_TYPE:
            # read interrupt status before read other flags
            try:
                raw = self.read_eeprom(0 * 128 + 3, 1)
                if raw and (raw[0] & 0x01) == 1:
                    interrupt_str = 'Interrupt not asserted'
                else:
                    interrupt_str = 'Interrupt asserted'
            except Except:
                interrupt_str = 'N/A'

            try:
                xcvrst = self.get_transceiver_status()
            except NotImplementedError:
                 return transceiver_irq_dict

            # add lane compact strings
            xcvrst['dpinit_pending_hostlane'] = self._compact_lanes(xcvrst, 'dpinit_pending_hostlane')
            xcvrst['rxcdrlol'] = self._compact_lanes(xcvrst, 'rxcdrlol')
            xcvrst['rxlos'] = self._compact_lanes(xcvrst, 'rxlos')
            xcvrst['rxoutput_status_hostlane'] = self._compact_lanes(xcvrst, 'rxoutput_status_hostlane')
            xcvrst['rxpowerhighalarm_flag'] = self._compact_lanes(xcvrst, 'rxpowerhighalarm_flag')
            xcvrst['rxpowerhighwarning_flag'] = self._compact_lanes(xcvrst, 'rxpowerhighwarning_flag')
            xcvrst['rxpowerlowalarm_flag'] = self._compact_lanes(xcvrst, 'rxpowerlowalarm_flag')
            xcvrst['rxpowerlowwarning_flag'] = self._compact_lanes(xcvrst, 'rxpowerlowwarning_flag')
            xcvrst['txbiashighalarm_flag'] = self._compact_lanes(xcvrst, 'txbiashighalarm_flag')
            xcvrst['txbiashighwarning_flag'] = self._compact_lanes(xcvrst, 'txbiashighwarning_flag')
            xcvrst['txbiaslowalarm_flag'] = self._compact_lanes(xcvrst, 'txbiaslowalarm_flag')
            xcvrst['txbiaslowwarning_flag'] = self._compact_lanes(xcvrst, 'txbiaslowwarning_flag')
            xcvrst['txcdrlol_hostlane'] = self._compact_lanes(xcvrst, 'txcdrlol_hostlane')
            xcvrst['txfault'] = self._compact_lanes(xcvrst, 'txfault')
            xcvrst['txlos_hostlane'] = self._compact_lanes(xcvrst, 'txlos_hostlane')
            xcvrst['txoutput_status'] = self._compact_lanes(xcvrst, 'txoutput_status')
            xcvrst['txpowerhighalarm_flag'] = self._compact_lanes(xcvrst, 'txpowerhighalarm_flag')
            xcvrst['txpowerhighwarning_flag'] = self._compact_lanes(xcvrst, 'txpowerhighwarning_flag')
            xcvrst['txpowerlowalarm_flag'] = self._compact_lanes(xcvrst, 'txpowerlowalarm_flag')
            xcvrst['txpowerlowwarning_flag'] = self._compact_lanes(xcvrst, 'txpowerlowwarning_flag')

            # add needed keys
            xcvrst['Interrupt'] = interrupt_str
            try:
                raw = self.read_eeprom(0x11 * 128 + 134, 1)
                xcvrst['dpstatechanged_flag'] = '{0:08b}'.format(raw[0])
            except Except:
                xcvrst['dpstatechanged_flag'] = 'N/A'
            try:
                raw = self.read_eeprom(0x11 * 128 + 138, 1)
                xcvrst['txadaptiveinputeqfault_flag'] = '{0:08b}'.format(raw[0])
            except Except:
                xcvrst['txadaptiveinputeqfault_flag'] = 'N/A'

        return xcvrst

    def get_lpmode(self):
        """
        Retrieves the lpmode (low power mode) status of this SFP
        Returns:
            A Boolean, True if lpmode is enabled, False if disabled
        """
        if self.port_type != QSFP_DD_TYPE:
            return False

        try:
            with open(
                "/".join([PORT_INFO_PATH, self.port_name, "qsfp_lpmode"])) as reg_file:
                # Read status
                content = reg_file.readline().rstrip()
                reg_value = int(content)
                # low power mode is active high
                if reg_value == 0:
                    return False
        except IOError as e:
            print("Error: unable to open file: %s" % str(e))
            return False

        return True

    def get_modirq(self):
        """
        Retrieves the qsfp_modirq (module interrupt) status of this SFP
        Returns:
            A Boolean, True if modirq is asserted, False if not asserted
        """
        if self.port_type != QSFP_DD_TYPE:
            return False

        try:
            with open(
                "/".join([PORT_INFO_PATH, self.port_name, "qsfp_modirq"])) as reg_file:
                # Read status
                content = reg_file.readline().rstrip()
                reg_value = int(content)
                # modirq is active low
                if reg_value == 0:
                    return True
        except IOError as e:
            print("Error: unable to open file: %s" % str(e))
            return False

        return False

    def get_reset_status(self):
        """
        Retrieves the reset status of SFP
        Returns:
            A Boolean, True if reset enabled, False if disabled
        """
        if self.port_type != QSFP_DD_TYPE:
            return False

        try:
            with open(
                "/".join([PORT_INFO_PATH, self.port_name, "qsfp_reset"])) as reg_file:
                # Read status
                content = reg_file.readline().rstrip()
                reg_value = int(content)
                # reset is active low
                if reg_value == 0:
                    return True
        except IOError as e:
            print("Error: unable to open file: %s" % str(e))
            return False

        return False

    def reset(self):
        """
        Reset SFP and return all user module settings to their default srate.
        Returns:
            A boolean, True if successful, False if not
        """
        if self.port_type != QSFP_DD_TYPE:
            return False

        try:
            with open(
                "/".join([PORT_INFO_PATH, self.port_name, "qsfp_reset"]), "w") as reg_file:
                # Convert our register value back to a hex string and write back
                reg_file.seek(0)
                self.doing_reset = True
                reg_file.write(hex(0))
        except IOError as e:
            print("Error: unable to open file: %s" % str(e))
            self.doing_reset = False
            return False

        # Sleep 1 second to allow it to settle
        time.sleep(1)

        # Flip the bit back high and write back to the register to take port out of reset
        try:
            with open(
                "/".join([PORT_INFO_PATH, self.port_name, "qsfp_reset"]), "w") as reg_file:
                reg_file.seek(0)
                reg_file.write(hex(1))
        except IOError as e:
            print("Error: unable to open file: %s" % str(e))
            self.doing_reset = False
            return False

        self.doing_reset = False
        return True

    def no_reset(self):
        """
        Set CPLD qsfp_reset to 1 for non-reset status.
        Returns:
            A boolean, True if successful, False if not
        """
        if self.port_type != QSFP_DD_TYPE:
            return False

        try:
            with open(
                "/".join([PORT_INFO_PATH, self.port_name, "qsfp_reset"]), "w") as reg_file:
                reg_file.seek(0)
                reg_file.write(hex(1))
        except IOError as e:
            print("Error: unable to open file: %s" % str(e))
            return False

        return True

    def set_lpmode(self, lpmode):
        """
        Sets the lpmode (low power mode) of SFP
        Args:
            lpmode: A Boolean, True to enable lpmode, False to disable it
            Note  : lpmode can be overridden by set_power_override
        Returns:
            A boolean, True if lpmode is set successfully, False if not
        """
        if self.port_type != QSFP_DD_TYPE:
            return False

        try:
            reg_file = open(
                "/".join([PORT_INFO_PATH, self.port_name, "qsfp_lpmode"]), "r+")
        except IOError as e:
            print("Error: unable to open file: %s" % str(e))
            return False

        content = hex(lpmode)

        reg_file.seek(0)
        reg_file.write(content)
        reg_file.close()

        return True

    def set_media_type(self):
        """
        Reads optic eeprom byte to determine media type inserted
        """
        eeprom_raw = []
        eeprom_raw = self._xcvr_api_factory._get_id()
        if eeprom_raw is not None:
            eeprom_raw = hex(eeprom_raw)
            if eeprom_raw in SFP_TYPE_LIST:
                self.sfp_type = SFP_TYPE
            elif eeprom_raw in QSFP_TYPE_LIST:
                self.sfp_type = QSFP_TYPE
            elif eeprom_raw in QSFP_DD_TYPE_LIST:
                self.sfp_type = QSFP_DD_TYPE
            else:
                #Set native port type if EEPROM type is not recognized/readable
                self.sfp_type = self.port_type
        else:
            self.sfp_type = self.port_type

        return self.sfp_type

    def reinit_sfp_driver(self):
        """
        Changes the driver based on media type detected
        """

        i2c_bus = self.sfp_eeprom_path[25:].split('/')[0]
        del_sfp_path = "/sys/bus/i2c/devices/i2c-{0}/delete_device".format(i2c_bus)
        new_sfp_path = "/sys/bus/i2c/devices/i2c-{0}/new_device".format(i2c_bus)
        driver_path = "/sys/bus/i2c/devices/i2c-{0}/{0}-0050/name".format(i2c_bus)

        if not os.path.isfile(driver_path):
            print(driver_path, "does not exist")
            return False

        try:
            with os.fdopen(os.open(driver_path, os.O_RDONLY)) as filed:
                driver_name = filed.read()
                driver_name = driver_name.rstrip('\r\n')
                driver_name = driver_name.lstrip(" ")

            #Avoid re-initialization of the QSFP/SFP optic on QSFP/SFP port.
            if self.sfp_type == SFP_TYPE and driver_name in ['optoe1', 'optoe3']:
                with open(del_sfp_path, 'w') as f:
                    f.write('0x50\n')
                time.sleep(0.2)
                with open(new_sfp_path, 'w') as f:
                    f.write('optoe2 0x50\n')
                time.sleep(2)
            elif self.sfp_type == OSFP_TYPE and driver_name in ['optoe2', 'optoe3']:
                with open(del_sfp_path, 'w') as f:
                    f.write('0x50\n')
                time.sleep(0.2)
                with open(new_sfp_path, 'w') as f:
                    f.write('optoe1 0x50\n')
                time.sleep(2)
            elif self.sfp_type == QSFP_DD_TYPE and driver_name in ['optoe1', 'optoe2']:
                with open(del_sfp_path, 'w') as f:
                    f.write('0x50\n')
                time.sleep(0.2)
                with open(new_sfp_path, 'w') as f:
                    f.write('optoe3 0x50\n')
                time.sleep(2)

        except IOError as err:
            print("Error: Unable to open file: %s" %str(err))
            return False

        return True

    def get_position_in_parent(self):
        """
        Retrieves 1-based relative physical position in parent device.
        Returns:
            integer: The 1-based relative physical position in parent
            device or -1 if cannot determine the position
        """
        return 0

    @staticmethod
    def is_replaceable():
        """
        Indicate whether this device is replaceable.
        Returns:
            bool: True if it is replaceable.
        """
        return True

    def get_error_description(self):
        """
        Retrives the error descriptions of the SFP module

        Returns:
            String that represents the current error descriptions of vendor specific errors
            In case there are multiple errors, they should be joined by '|',
            like: "Bad EEPROM|Unsupported cable"
        """
        if not self.get_presence():
            return self.SFP_STATUS_UNPLUGGED
        else:
            if not os.path.isfile(self.sfp_eeprom_path):
                return "EEPROM driver is not attached"

            if self.sfp_type == SFP_TYPE:
                offset = SFP_INFO_OFFSET
            elif self.sfp_type == OSFP_TYPE:
                offset = QSFP_INFO_OFFSET
            elif self.sfp_type == QSFP_TYPE:
                offset = QSFP_INFO_OFFSET
            elif self.sfp_type == QSFP_DD_TYPE:
                offset = QSFP_DD_PAGE0
            else:
                return "Invalid SFP type {}".format(self.sfp_type)

            try:
                with open(self.sfp_eeprom_path, mode="rb", buffering=0) as eeprom:
                    eeprom.seek(offset)
                    eeprom.read(1)
            except OSError as e:
                return "EEPROM read failed ({})".format(e.strerror)

        return self.SFP_STATUS_OK

    def set_application_noapply(self, channel, appl_code):
        """
        Update the selected application code to the specified lanes on the host side,
        without apply data path init.

        Args:
            channel:
                Integer, a bitmask of the lanes on the host side
                e.g. 0x5 for lane 0 and lane 2.
            appl_code:
                Integer, the desired application code

        Returns:
            Boolean, true if success otherwise false
        """
        # Update the application selection
        lane_first = -1
        for lane in range(self.get_xcvr_api().NUM_CHANNELS):
            if ((1 << lane) & channel) == 0:
                continue
            if lane_first < 0:
                lane_first = lane
            addr = "{}_{}_{}".format(consts.STAGED_CTRL_APSEL_FIELD, 0, lane + 1)
            data = (appl_code << 4) | (lane_first << 1)
            self.get_xcvr_api().xcvr_eeprom.write(addr, data)

        return True

    def set_apply_dpinit(self, channel=0xff):
        # Apply DataPathInit
        return self.get_xcvr_api().xcvr_eeprom.write("%s_%d" % (consts.STAGED_CTRL_APPLY_DPINIT_FIELD, 0), channel)

    def tx_disable_channel_by_hostlane(self, hostlane, disable):
        """
        Sets the tx_disable for specified SFP channels by host lanes.
        host lanes will be mapped to media lanes.
        Args:
            channel : A hex of 4 bits (bit 0 to bit 3) which represent channel 0 to 3,
                      e.g. 0x5 for channel 0 and channel 2.
            disable : A boolean, True to disable TX channels specified in channel,
                      False to enable
        Returns:
            A boolean, True if successful, False if not
        """
        api = self.get_xcvr_api()
        if api is None:
            return False

        host_lane_count = api.get_host_lane_count()
        media_lane_count = api.get_media_lane_count()

        # Credo CAC45X301D4PA0HW (split 4) has host_lane_count = 2 media_lane_count = 2
        # so it is the same with host_lane_count = 2*4 = 8 media_lane_count = 2*4 = 8
        if host_lane_count == 8:
            media_lane_count = media_lane_count * 1
        elif host_lane_count == 4:
            media_lane_count = media_lane_count * 2
        elif host_lane_count == 2:
            media_lane_count = media_lane_count * 4
        elif host_lane_count == 1:
            media_lane_count = media_lane_count * 8
        else:
            return False

        if media_lane_count == 8:
            channel = hostlane
        elif media_lane_count == 4:
            channel = 0
            for (h, m) in [(0x03, 0x01), (0x0c, 0x02), (0x30, 0x04), (0xc0, 0x08)]:
                if (hostlane & h):
                    channel = channel | m
        elif media_lane_count == 2:
            channel = 0
            for (h, m) in [(0x0f, 0x01), (0xf0, 0x02)]:
                if (hostlane & h):
                    channel = channel | m
        elif media_lane_count == 1:
            channel = 1
        else:
            return False

        return(api.tx_disable_channel(channel, disable))

    def get_subport(self, lanes):
        '''
        Example: 4x100G then subports are 1, 2, 3, 4
                 1x400G then subports are 0
        '''
        for port_index, port_lanes in enumerate(self._port_to_lanes):
            # use lanes[0] only, because others should have the same master port
            if lanes[0] in port_lanes:
                if len(lanes) == len(port_lanes):
                    # non-breakout port
                    return 0
                i = port_lanes.index(lanes[0])
                s = int(i / len(lanes)) + 1
                return s
        return None

    def get_front_port_number(self):
        """
        Gets the front port number.
        Args:
            None
        Returns:
            A integer
        """
        return self._port_num

    def get_xcvr_api(self):
        """
        Retrieves the XcvrApi associated with this SFP

        Returns:
            An object derived from XcvrApi that corresponds to the SFP
        """
        id_byte_raw = self.read_eeprom(0, 1)
        if id_byte_raw is None:
            # not present
            self._xcvr_api = None
            self.xcvr_id = 0
            return None
        xcvr_id = id_byte_raw[0]

        if self._xcvr_api:
            # if xcvrd_id is changed, then create a new xcvr_api
            if self.xcvr_id != xcvr_id:
                self._xcvr_api = None

        if self._xcvr_api is None:
            self.refresh_xcvr_api()
            self.xcvr_id = xcvr_id if self._xcvr_api else 0

        return self._xcvr_api

    def detect_thermals(self):
        """
        Detect SFP thermal support and update _thermal_list

        Returns:
            None
        """
        self._thermal_list = []
        try:
            temperature_supported = self.get_xcvr_api().get_temperature_support()
        except Exception:
            temperature_supported = False
        if temperature_supported:
            self._thermal_list.append(SfpThermal(self, 0))

    def get_num_thermals(self):
        """
        Retrieves the number of thermals available on this SFP

        Returns:
            An integer, the number of thermals available on this SFP
        """
        self.detect_thermals()
        return len(self._thermal_list)

    def get_all_thermals(self):
        """
        Retrieves all thermals available on this SFP

        Returns:
            A list of objects derived from ThermalBase representing all thermals
            available on this SFP
        """
        self.detect_thermals()
        return self._thermal_list

    def get_thermal(self, index):
        """
        Retrieves thermal unit represented by (0-based) index <index>

        Args:
            index: An integer, the index (0-based) of the thermal to
            retrieve

        Returns:
            An object derived from ThermalBase representing the specified thermal
        """
        self.detect_thermals()
        thermal = None

        try:
            thermal = self._thermal_list[index]
        except IndexError:
            sys.stderr.write("THERMAL index {} out of range (0-{})\n".format(
                             index, len(self._thermal_list)-1))

        return thermal


class SfpThermal(ThermalBase):
    """Platform-specific Thermal class for SFP """

    def __init__(self, sfp, index):
        self.sfp = sfp
        self.index = index
        self.minimum_thermal = 999
        self.maximum_thermal = 0
        self.threshold_info = self.sfp.get_transceiver_threshold_info()

    def get_temperature(self):
        """
        Retrieves current temperature reading from thermal

        Returns:
            A float number of current temperature in Celsius up to nearest thousandth
            of one degree Celsius, e.g. 30.125
        """
        temp = int(self.sfp.get_temperature())
        if temp > self.maximum_thermal:
            self.maximum_thermal = temp
        if temp < self.minimum_thermal:
            self.minimum_thermal = temp

        return temp

    def get_high_threshold(self):
        """
        Retrieves the high threshold temperature of thermal

        Returns:
            A float number, the high threshold temperature of thermal in Celsius
            up to nearest thousandth of one degree Celsius, e.g. 30.125
        """
        temphighwarning = self.threshold_info['temphighwarning']
        # some cables report unreasonable high threshold
        if isinstance(temphighwarning, float) and temphighwarning < 50:
            return None
        return temphighwarning

    def get_low_threshold(self):
        """
        Retrieves the low threshold temperature of thermal

        Returns:
            A float number, the low threshold temperature of thermal in Celsius
            up to nearest thousandth of one degree Celsius, e.g. 30.125
        """
        templowwarning = self.threshold_info['templowwarning']
        return templowwarning

    def set_high_threshold(self, temperature):
        """
        Sets the high threshold temperature of thermal

        Args :
            temperature: A float number up to nearest thousandth of one degree Celsius,
            e.g. 30.125

        Returns:
            A boolean, True if threshold is set successfully, False if not
        """
        raise NotImplementedError

    def set_low_threshold(self, temperature):
        """
        Sets the low threshold temperature of thermal

        Args :
            temperature: A float number up to nearest thousandth of one degree Celsius,
            e.g. 30.125

        Returns:
            A boolean, True if threshold is set successfully, False if not
        """
        raise NotImplementedError

    def get_high_critical_threshold(self):
        """
        Retrieves the high critical threshold temperature of thermal

        Returns:
            A float number, the high critical threshold temperature of thermal in Celsius
            up to nearest thousandth of one degree Celsius, e.g. 30.125
        """
        temphighalarm = self.threshold_info['temphighalarm']
        # some cables report unreasonable high threshold
        if isinstance(temphighalarm, float) and temphighalarm < 50:
            return None
        return temphighalarm

    def get_low_critical_threshold(self):
        """
        Retrieves the low critical threshold temperature of thermal

        Returns:
            A float number, the low critical threshold temperature of thermal in Celsius
            up to nearest thousandth of one degree Celsius, e.g. 30.125
        """
        templowalarm = self.threshold_info['templowalarm']
        return templowalarm

    def set_high_critical_threshold(self, temperature):
        """
        Sets the critical high threshold temperature of thermal

        Args :
            temperature: A float number up to nearest thousandth of one degree Celsius,
            e.g. 30.125

        Returns:
            A boolean, True if threshold is set successfully, False if not
        """
        raise NotImplementedError

    def set_low_critical_threshold(self, temperature):
        """
        Sets the critical low threshold temperature of thermal

        Args :
            temperature: A float number up to nearest thousandth of one degree Celsius,
            e.g. 30.125

        Returns:
            A boolean, True if threshold is set successfully, False if not
        """
        raise NotImplementedError

    def get_minimum_recorded(self):
        """
        Retrieves the minimum recorded temperature of thermal

        Returns:
            A float number, the minimum recorded temperature of thermal in Celsius
            up to nearest thousandth of one degree Celsius, e.g. 30.125
        """
        return self.minimum_thermal;

    def get_maximum_recorded(self):
        """
        Retrieves the maximum recorded temperature of thermal

        Returns:
            A float number, the maximum recorded temperature of thermal in Celsius
            up to nearest thousandth of one degree Celsius, e.g. 30.125
        """
        return self.maximum_thermal

    def get_name(self):
        """
        Retrieves the name of the thermal device
            Returns:
            string: The name of the thermal device
        """
        return self.sfp.get_name()

    def get_presence(self):
        """
        Retrieves the presence of the device
        Returns:
            bool: True if device is present, False if not
        """
        return self.sfp.get_presence()

    def get_model(self):
        """
        Retrieves the model number (or part number) of the device
        Returns:
            string: Model/part number of device
        """
        return self.sfp.get_model()

    def get_serial(self):
        """
        Retrieves the serial number of the device
        Returns:
            string: Serial number of device
        """
        return self.sfp.get_serial()

    def get_status(self):
        """
        Retrieves the operational status of the device
        Returns:
            A boolean value, True if device is operating properly, False if not
        """
        return self.sfp.get_status()

    def get_position_in_parent(self):
        """
        Retrieves 1-based relative physical position in parent device.
        Returns:
            integer: The 1-based relative physical position in parent
            device or -1 if cannot determine the position
        """
        return (self.index + 1)

    def is_replaceable(self):
        """
        Indicate whether this Thermal is replaceable.
        Returns:
            bool: True if it is replaceable.
        """
        return False
