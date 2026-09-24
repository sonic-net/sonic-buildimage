"""
    led_control.py

    Platform-specific LED control functionality for SONiC
"""

try:
    from sonic_led.led_control_base import LedControlBase
    from sonic_py_common import daemon_base
    from sonic_py_common import multi_asic
    from sonic_py_common import logger
    from sonic_py_common.interface import backplane_prefix, inband_prefix, recirc_prefix
    from swsscommon import swsscommon
    import os
    import time
    import sonic_platform.platform
    import sonic_platform.chassis
except ImportError as e:
    raise ImportError(str(e) + " - required module not found")

SELECT_TIMEOUT = 1000
FAN_DRAWERS = 3
QSFP_NUMS = 36
REG_DIR = "/sys/bus/pci/devices/0000:01:00.0/"
PORT_DIR = "/sys/bus/pci/devices/0000:05:00.0/"
FILE_OPERSTATE = "/sys/class/net/eth0/operstate"
FILE_DUPLEX = "/sys/class/net/eth0/duplex"
FILE_RX_PACKETS = "/sys/class/net/eth0/statistics/rx_packets"
FILE_TX_PACKETS = "/sys/class/net/eth0/statistics/tx_packets"
FILE_RX_ERRORS = "/sys/class/net/eth0/statistics/rx_errors"
FILE_TX_ERRORS = "/sys/class/net/eth0/statistics/tx_errors"

SYSLOG_IDENTIFIER = "nokia-ledd"
sonic_logger = logger.Logger(SYSLOG_IDENTIFIER)
sonic_logger.set_min_log_priority_info()

class LedControl(LedControlBase):
    """Platform specific LED control class"""

    # Constructor
    def __init__(self):
        self.chassis = sonic_platform.platform.Platform().get_chassis()
        self._initDefaultConfig()

    def _initDefaultConfig(self):
        # The fan tray leds and system led managed by new chassis class API
        # leaving only a couple other front panel leds to be done old style
        sonic_logger.log_info("starting system leds")

        if multi_asic.is_multi_asic():
            # Load the namespace details first from the database_global.json file.
            if not swsscommon.SonicDBConfig.isGlobalInit():
                swsscommon.SonicDBConfig.initializeGlobalConfig()

        # Get the namespaces in the platform. For multi-asic devices we get the namespaces
        # of front-end ascis which have front-panel interfaces.
        namespaces = multi_asic.get_front_end_namespaces()

        # Subscribe to PORT table notifications in the Application DB
        appl_db = {}
        self.sst = {}
        self.sel = swsscommon.Select()

        for namespace in namespaces:
            # Open a handle to the Application database, in all namespaces
            appl_db[namespace] = daemon_base.db_connect("APPL_DB", namespace=namespace)
            self.sst[namespace] = swsscommon.SubscriberStateTable(appl_db[namespace], swsscommon.APP_PORT_TABLE_NAME)
            self.sel.addSelectable(self.sst[namespace])

        self._pre_port_led_stat = ['off'] * QSFP_NUMS
        
        self._initSystemLed()

    def _read_sysfs_file(self, sysfs_file):
        # On successful read, returns the value read from given
        # reg_name and on failure returns 'ERR'
        rv = 'ERR'

        if (not os.path.isfile(sysfs_file)):
            return rv
        try:
            with open(sysfs_file, 'r') as fd:
                rv = fd.read()
                fd.close()
        except Exception as e:
            rv = 'ERR'

        rv = rv.rstrip('\r\n')
        rv = rv.lstrip(" ")
        return rv
    
    def _write_sysfs_file(self, sysfs_file, value):
        # On successful write, the value read will be written on
        # reg_name and on failure returns 'ERR'
        rv = 'ERR'

        if (not os.path.isfile(sysfs_file)):
            return rv
        try:
            with open(sysfs_file, 'w') as fd:
                rv = fd.write(value)
                fd.close()
        except Exception as e:
            rv = 'ERR'

        return rv
    
    def _initSystemLed(self):
        self.oldfan = 'off'
        self.oldpsu = 'off'
        count = 0
        self.mgmt_link = 'off'
        self.mgmt_actv = 'off'
        self.mgmt_rx_packets = int(self._read_sysfs_file(FILE_RX_PACKETS))
        self.mgmt_tx_packets = int(self._read_sysfs_file(FILE_TX_PACKETS))

        # Timer loop to monitor and set Port Leds and 
        # front panel Status, Fan, and PSU LEDs
        while True:
            self.port_state_check()
            self.mgmt_check()
            count = count + 1
            if count == 30:
                self.fp_check()
                count = 0
                
    def port_state_check(self):
        # Use timeout to prevent ignoring the signals we want to handle
        # in signal_handler() (e.g. SIGTERM for graceful shutdown)
        (state, selectableObj) = self.sel.select(SELECT_TIMEOUT)

        if state == swsscommon.Select.TIMEOUT:
            # Do not flood log when select times out
            return 1

        if state != swsscommon.Select.OBJECT:
            sonic_logger.log_warning("sel.select() did not return swsscommon.Select.OBJECT")
            return 2

        # Get the redisselect object from selectable object
        redisSelectObj = swsscommon.CastSelectableToRedisSelectObj(selectableObj)

        # Get the corresponding namespace from redisselect db connector object
        namespace = redisSelectObj.getDbConnector().getNamespace()

        (key, op, fvp) = self.sst[namespace].pop()
        if fvp:
            # TODO: Once these flag entries have been removed from the DB,
            # we can remove this check
            if key in ["PortConfigDone", "PortInitDone"]:
                return 3

            fvp_dict = dict(fvp)

            if op == "SET" and "oper_status" in fvp_dict:
                if not key.startswith((backplane_prefix(), inband_prefix(), recirc_prefix())):
                    self.port_link_state_change(key, fvp_dict["oper_status"])
        else:
            return 4

        return 0
    
    def port_link_state_change(self, port, state):
        """
        Called when port link state changes. Update port link state LED here.

        :param port: A string, SONiC port name (e.g., "Ethernet0")
        :param state: A string, the port link state (either "up" or "down")
        """
        intf_prefix = 'Ethernet'
        if port.startswith(intf_prefix) is False:
            return
        else:
            port_idx = int(port[len(intf_prefix):]) // 8 + 1
            if port_idx < 1 or port_idx > QSFP_NUMS:
                return

        if state == 'up':
            if self._pre_port_led_stat[port_idx-1] != 'green':
                self._write_sysfs_file(PORT_DIR + f"port_{port_idx}_led", '0x1')
                self._pre_port_led_stat[port_idx-1] = 'green'
        elif state == 'down':
            if self._pre_port_led_stat[port_idx-1] != 'off':
                self._write_sysfs_file(PORT_DIR + f"port_{port_idx}_led", '0x0')
                self._pre_port_led_stat[port_idx-1] = 'off'
        else:
            return
    
    def fp_check(self):
        # Front Panel FAN Panel LED setting
        good_fan_drawer = 0
        for fan_drawer in self.chassis._fan_drawer_list:
            if fan_drawer.get_status() == True:
                good_fan_drawer = good_fan_drawer + 1
                fan_drawer.set_status_led('green')
            else:
                fan_drawer.set_status_led('amber')
        
        if (good_fan_drawer == FAN_DRAWERS):
            if self.oldfan != 'green':
                self._write_sysfs_file(REG_DIR + 'led_fan', '0x6400')
                self.oldfan = 'green'
        else:
            if self.oldfan != 'amber':
                self._write_sysfs_file(REG_DIR + 'led_fan', '0xa4c700')
                self.oldfan = 'amber'
        
        # Front Panel PSU Panel LED setting
        if (self.chassis.get_psu(0).get_status() == self.chassis.get_psu(1).get_status() == True):
            if self.oldpsu != 'green':
                self._write_sysfs_file(REG_DIR + 'led_psu', '0x6400')
                self.oldpsu = 'green'
        else:
            if self.oldpsu != 'amber':
                self._write_sysfs_file(REG_DIR + 'led_psu', '0xa4c700')
                self.oldpsu = 'amber'
    
    def mgmt_check(self):
        link_stat = self._read_sysfs_file(FILE_OPERSTATE)
        if link_stat == 'up':
            duplex = self._read_sysfs_file(FILE_DUPLEX)
            if duplex == 'full':
                if self.mgmt_link != 'green':
                    self._write_sysfs_file(REG_DIR + 'led_mgmt_link', '0x1')
                    self.mgmt_link = 'green'
            elif duplex == 'half':
                if self.mgmt_link != 'blink_green':
                    self._write_sysfs_file(REG_DIR + 'led_mgmt_link', '0x0')
                    self._write_sysfs_file(REG_DIR + 'led_mgmt_link', '0xf5')
                    self.mgmt_link = 'blink_green'
            else:
                if self.mgmt_link != 'off':
                    self._write_sysfs_file(REG_DIR + 'led_mgmt_link', '0x0')
                    self.mgmt_link = 'off'
            
            mgmt_rx_packets = int(self._read_sysfs_file(FILE_RX_PACKETS))
            mgmt_tx_packets = int(self._read_sysfs_file(FILE_TX_PACKETS))
            mgmt_rx_errors = int(self._read_sysfs_file(FILE_RX_ERRORS))
            mgmt_tx_errors = int(self._read_sysfs_file(FILE_TX_ERRORS))
            if mgmt_rx_errors > 0 or mgmt_tx_errors > 0:
                if self.mgmt_actv != 'amber':
                    self._write_sysfs_file(REG_DIR + 'led_mgmt_actv', '0x2')
                    self.mgmt_actv = 'amber'
            elif mgmt_rx_packets > self.mgmt_rx_packets or mgmt_tx_packets > self.mgmt_tx_packets:
                self.mgmt_rx_packets = mgmt_rx_packets
                self.mgmt_tx_packets = mgmt_tx_packets
                if self.mgmt_actv != 'fast_blink_green':
                    self._write_sysfs_file(REG_DIR + 'led_mgmt_actv', '0x0')
                    self._write_sysfs_file(REG_DIR + 'led_mgmt_actv', '0x75')
                    self.mgmt_actv = 'fast_blink_green'
            else:
                if self.mgmt_actv != 'off':
                    self._write_sysfs_file(REG_DIR + 'led_mgmt_actv', '0x0')
                    self.mgmt_actv = 'off'
        else:
            if self.mgmt_link != 'off':
                    self._write_sysfs_file(REG_DIR + 'led_mgmt_link', '0x0')
                    self.mgmt_link = 'off'
            if self.mgmt_actv != 'off':
                    self._write_sysfs_file(REG_DIR + 'led_mgmt_actv', '0x0')
                    self.mgmt_actv = 'off'
