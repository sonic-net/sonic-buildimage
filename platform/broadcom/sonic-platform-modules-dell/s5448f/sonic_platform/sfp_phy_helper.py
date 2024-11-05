#!/usr/bin/env python3

#############################################################################
# DELLEMC
#
# Module contains implementation of APIs for SONiC Platform SFP external
# PHY specific access (eg. Copper SFPs )
#
#############################################################################

import time
import re
import threading
import subprocess
import syslog
from swsscommon import swsscommon
from sonic_py_common import daemon_base



PHY_MAILBOX_MMD_OFFSET = 256 + 250
PHY_MAILBOX_DATA_OFFSET = 256 + 253
PHY_MAILBOX_CTRL_OFFSET = 256 + 255
PHY_MAILBOX_STATUS_OFFSET = PHY_MAILBOX_CTRL_OFFSET
PHY_MAILBOX_RD_SIZE = 3
PHY_MAILBOX_WR_SIZE = 5
#PHY_MAILBOX_RW_STATUS_IDLE = 0x0
PHY_MAILBOX_STATUS_DONE = 0x4
PHY_MAILBOX_STATUS_ERROR = 0x8
PHY_MAILBOX_RD_BYTE = bytes([0x01])
PHY_MAILBOX_WR_BYTE = bytes([0x02])

PHY_MAILBOX_DIS_DELAY = 1   # 1000ms
PHY_MAILBOX_EN_DELAY = 0.01 #   10ms

PHY_RD_TX_STATE_CMD = 0
PHY_WR_TX_DISABLE_CMD = 1
PHY_WR_TX_ENABLE_CMD = 2
PHY_WR_SPEED_1G_CMD = 3
PHY_WR_SPEED_10G_CMD = 4
PHY_RD_LINK_STATE_CMD = 5
PHY_WR_SPEED_ALL_CMD = 6


PHY_SPEED_AUTONEG = 0

autoneg_task = None
autoneg_lock = threading.Lock()
autoneg_ports = {}
stopping_event = threading.Event()
cusfp_1g_task = None
cusfp_1g_lock = threading.Lock()
cusfp_1g_ports = {}
cusfp_1g_stopping_event = threading.Event()
MVL88E1111_PHY_I2CADDR = ['0x56']
MVL88E1111_PHY_STATUS_REG = ['17']
MVL88E1111_PHY_EXT_ADDRESS_REG = ['29']
MVL88E1111_PHY_EXT_CTRL_REG = ['30']

MVL88E1111_PHY_LINK_STATUS_BIT = 10
MVL88E1111_PHY_LINK_STATUS_LEN = 1
MVL88E1111_PHY_LINE_SPEED_BIT = 14
MVL88E1111_PHY_LINE_SPEED_LEN = 2 
MVL88E1111_PHY_LINK_UP = 1
# MVL88E1111 register data
EXT_ADD_SERDES_TX_RX_CONTROL = ['0x00', '0x1f']
EXT_PHY_CTRL_SERDES_TX_RX_ENABLE = ['0x00', '0x00']
EXT_PHY_CTRL_SERDES_TX_RX_DISABLE = ['0x20', '0x01']

LINK_STATUS_REG_READ_RETRY_COUNT = 3
mvl88e1111_speed_map = {2:1000}

appl_db = None
state_db = None


I2C_GET = ['/usr/sbin/i2cget', '-y', '-f']

def _phy_mailbox_read(eeprom_path, buf):
    eeprom = None
    value = 0
    ret = (False, value)

    try:
        if len(buf) != PHY_MAILBOX_RD_SIZE:
            return ret

        eeprom = open(eeprom_path, mode="rb+", buffering=0)

        # Write 3 bytes MMD and MDIO address
        eeprom.seek(PHY_MAILBOX_MMD_OFFSET)
        eeprom.write(buf)

        # Write the READ Control Byte
        eeprom.seek(PHY_MAILBOX_CTRL_OFFSET)
        eeprom.write(PHY_MAILBOX_RD_BYTE)

        # Read the status of MDIO operation
        eeprom.seek(PHY_MAILBOX_STATUS_OFFSET)
        status = int(hex(eeprom.read(1)[0]), 16)
        if status & PHY_MAILBOX_STATUS_DONE:
            # Read 2 bytes of data
            eeprom.seek(PHY_MAILBOX_DATA_OFFSET)
            data = eeprom.read(2)
            value = int(hex(data[1]), 16)
            value |= int(hex(data[0]), 16) << 8
            ret = (True, value)
        elif status & PHY_MAILBOX_STATUS_ERROR:
            pass
        else:
            # Retry status read after 100ms
            time.sleep(0.1)
            eeprom.seek(PHY_MAILBOX_STATUS_OFFSET)
            status = int(hex(eeprom.read(1)[0]), 16)
            if status & PHY_MAILBOX_STATUS_DONE:
                # Read 2 bytes of data
                eeprom.seek(PHY_MAILBOX_DATA_OFFSET)
                data = eeprom.read(2)
                value = int(hex(data[1]), 16)
                value |= int(hex(data[1]), 16) << 8
                ret = (True, value)
    except:
        pass

    if eeprom != None:
        eeprom.close()

    return ret

def _phy_mailbox_write(eeprom_path, buf):
    eeprom = None
    ret = False

    try:
        if len(buf) != PHY_MAILBOX_WR_SIZE:
            return ret

        eeprom = open(eeprom_path, mode="rb+", buffering=0)

        # Write 3 bytes MMD and MDIO address and 2 bytes of Data
        eeprom.seek(PHY_MAILBOX_MMD_OFFSET)
        eeprom.write(buf)

        # Write the WRITE Control Byte
        eeprom.seek(PHY_MAILBOX_CTRL_OFFSET)
        eeprom.write(PHY_MAILBOX_WR_BYTE)

        # Read the status of MDIO operation
        eeprom.seek(PHY_MAILBOX_STATUS_OFFSET)
        status = int(hex(eeprom.read(1)[0]), 16)
        if status & PHY_MAILBOX_STATUS_DONE:
            ret = True
        elif status & PHY_MAILBOX_STATUS_ERROR:
            pass
        else:
            # Retry status read after 100ms
            time.sleep(0.1)
            eeprom.seek(PHY_MAILBOX_STATUS_OFFSET)
            status = int(hex(eeprom.read(1)[0]), 16)
            if status & PHY_MAILBOX_STATUS_DONE:
                ret = True
    except:
        pass

    if eeprom != None:
        eeprom.close()

    return ret

def _phy_get_admin_state(eeprom_path, cmd_list):
    (ret, value) = _phy_mailbox_read(eeprom_path, cmd_list[PHY_RD_TX_STATE_CMD])
    # Tx disabled state is converted to Admin State
    if ret:
        value = False if value else True

    return (ret, value)

def _phy_set_admin_state(eeprom_path, enable, cmd_list):
    if enable:
        ret = _phy_mailbox_write(eeprom_path, cmd_list[PHY_WR_TX_ENABLE_CMD])
    else:
        ret = _phy_mailbox_write(eeprom_path, cmd_list[PHY_WR_TX_DISABLE_CMD])
        time.sleep(PHY_MAILBOX_DIS_DELAY)

    return ret

def _phy_set_speed(eeprom_path, speed, cmd_list):
    if speed == 10000:
        reg_configs = cmd_list[PHY_WR_SPEED_10G_CMD]
    elif speed == 1000:
        reg_configs = cmd_list[PHY_WR_SPEED_1G_CMD]
    elif speed == PHY_SPEED_AUTONEG:
        reg_configs = cmd_list[PHY_WR_SPEED_ALL_CMD]
    else:
        return False

    cur_admin_state = False
    (ret, value) = _phy_get_admin_state(eeprom_path, cmd_list)
    if ret:
        cur_admin_state = value
        try:
            if _phy_set_admin_state(eeprom_path, False, cmd_list):
                for buf in reg_configs:
                    if not _phy_mailbox_write(eeprom_path, buf):
                        ret = False
                        break
                time.sleep(PHY_MAILBOX_EN_DELAY)
                if cur_admin_state:
                    if not _phy_set_admin_state(eeprom_path, cur_admin_state, cmd_list):
                        return False
            else:
                syslog.syslog(syslog.LOG_INFO, "Failed to disable port for speed config(%s)" % (eeprom_path))
                return False

        except:
            ret = False
            pass

    return ret


# APIs for Aquantia PHY (Gen2)
AQ_PHY_RD_TX_STATE_CMD = bytes([0x01, 0x00, 0x09])
AQ_PHY_WR_TX_DISABLE_CMD = bytes([0x01, 0x00, 0x09, 0x00, 0x01])
AQ_PHY_WR_TX_ENABLE_CMD = bytes([0x01, 0x00, 0x09, 0x00, 0x00])
AQ_PHY_WR_SPEED_1G_CMD = [bytes([0x07, 0x00, 0x00, 0x20, 0x00]), \
                          bytes([0x07, 0x00, 0x10, 0x80, 0x01]), \
                          bytes([0x07, 0xC4, 0x00, 0x80, 0x40]), \
                          bytes([0x07, 0x00, 0x20, 0x00, 0x01]), \
                          bytes([0x07, 0x00, 0x00, 0x30, 0x00])]
AQ_PHY_WR_SPEED_10G_CMD = [bytes([0x07, 0x00, 0x00, 0x20, 0x00]), \
                           bytes([0x07, 0x00, 0x10, 0x90, 0x01]), \
                           bytes([0x07, 0xC4, 0x00, 0x00, 0x40]), \
                           bytes([0x07, 0x00, 0x20, 0x10, 0x01]), \
                           bytes([0x07, 0x00, 0x00, 0x30, 0x00])]

AQ_PHY_CMD_LIST = [AQ_PHY_RD_TX_STATE_CMD, \
                   AQ_PHY_WR_TX_DISABLE_CMD, \
                   AQ_PHY_WR_TX_ENABLE_CMD, \
                   AQ_PHY_WR_SPEED_1G_CMD, \
                   AQ_PHY_WR_SPEED_10G_CMD]

def aq_set_admin_state(logical_port, eeprom_path, enable):
    return _phy_set_admin_state(eeprom_path, enable, AQ_PHY_CMD_LIST)

def aq_set_speed(eeprom_path, speed):
    return _phy_set_speed(eeprom_path, speed, AQ_PHY_CMD_LIST)

def aq_set_autoneg(logical_port, eeprom_path, enable):
    return True

# APIs for Broadcom 84881 and 84891 PHY (Gen3 and Gen1.5.1)
BCM848XX_PHY_RD_TX_STATE_CMD = bytes([0x01, 0x00, 0x09])
BCM848XX_PHY_RD_LINK_STATE_CMD = bytes([0x1E, 0x40, 0x0D])
BCM848XX_PHY_WR_TX_DISABLE_CMD = bytes([0x01, 0x00, 0x09, 0x00, 0x01])
BCM848XX_PHY_WR_TX_ENABLE_CMD = bytes([0x01, 0x00, 0x09, 0x00, 0x00])
BCM848XX_PHY_WR_AN_CMD = bytes([0x07, 0xFF, 0xE0, 0x13, 0x00])

BCM848XX_PHY_WR_SPEED_1G_CMD = [bytes([0x07, 0x00, 0x20, 0x00, 0x03]), \
                           bytes([0x07, 0xFF, 0xE9, 0x02, 0x00]), \
                           bytes([0x07, 0xFF, 0xE4, 0x90, 0x01]), \
                           bytes([0x07, 0xFF, 0xE0, 0x13, 0x00])]

BCM848XX_PHY_WR_SPEED_10G_CMD = [bytes([0x07, 0x00, 0x20, 0x10, 0x03]), \
                            bytes([0x07, 0xFF, 0xE9, 0x00, 0x00]), \
                            bytes([0x07, 0xFF, 0xE4, 0x90, 0x01]), \
                            bytes([0x07, 0x00, 0x00, 0x32, 0x00])]
BCM848XX_PHY_WR_SPEED_ALL_CMD = [bytes([0x07, 0x00, 0x20, 0x11, 0x83]), \
                            bytes([0x07, 0xFF, 0xE9, 0x02, 0x00]), \
                            bytes([0x07, 0xFF, 0xE4, 0x91, 0x01]), \
                            bytes([0x07, 0x00, 0x00, 0x32, 0x00])]

BCM848XX_PHY_CMD_LIST = [BCM848XX_PHY_RD_TX_STATE_CMD, \
                   BCM848XX_PHY_WR_TX_DISABLE_CMD, \
                   BCM848XX_PHY_WR_TX_ENABLE_CMD, \
                   BCM848XX_PHY_WR_SPEED_1G_CMD, \
                   BCM848XX_PHY_WR_SPEED_10G_CMD, \
                   BCM848XX_PHY_RD_LINK_STATE_CMD, \
                   BCM848XX_PHY_WR_SPEED_ALL_CMD]

bcm848xx_link_rate_speed_map = {1:2500, 2:100, 3:5000, 4:1000, 6:10000}
BCM848XX_PHY_LINK_UP = 1
BCM848XX_PHY_LINK_STATUS_BIT = 5
BCM848XX_PHY_LINK_STATUS_LEN = 1
BCM848XX_PHY_LINK_RATE_BIT = 2 # bits 2,3,4
BCM848XX_PHY_LINK_RATE_BIT_LEN = 3 # bits 2,3,4
STATE_PORT_TABLE = 'PORT_TABLE'

def get_oper_status(logical_port):
    global appl_db
    if appl_db is None:
        appl_db = daemon_base.db_connect("APPL_DB")
    app_port_tbl = swsscommon.Table(appl_db, STATE_PORT_TABLE)
    oper_status = dict(app_port_tbl.get(logical_port)[1]).get('oper_status')
    return oper_status

def bcm848xx_set_admin_state(logical_port, eeprom_path, enable):
    max_retry = 10
    if enable:
        for retry in range(max_retry):
            if get_oper_status(logical_port) == 'up':
                if not _phy_mailbox_write(eeprom_path, BCM848XX_PHY_WR_AN_CMD):
                    return False
                break
            else:
                time.sleep(0.1)
    return _phy_set_admin_state(eeprom_path, enable, BCM848XX_PHY_CMD_LIST)

def bcm848xx_set_speed(eeprom_path, speed):
    return _phy_set_speed(eeprom_path, speed, BCM848XX_PHY_CMD_LIST)
#start bit from  LSB, bits starts from 0
def _get_bits(number, start_bit, num_of_bits):
    bit_val = (((1 << (num_of_bits)) - 1)  &  (number >> start_bit))
    return bit_val

def mvl88e1111_serdes_enable(eeprom_path, enable):
    i2c_bus = int(re.search(r"(?<=i2c-)[0-9]+", eeprom_path).group(0))
    _mvl88e1111_serdes_control(i2c_bus, enable)

def _mvl88e1111_serdes_control(i2c_bus, enable):
    if enable:
        reg_data = EXT_PHY_CTRL_SERDES_TX_RX_ENABLE
    else:
        reg_data = EXT_PHY_CTRL_SERDES_TX_RX_DISABLE
    try:
        cmd_out = subprocess.check_output(I2C_SET+ [str(i2c_bus)] + MVL88E1111_PHY_I2CADDR +\
                    MVL88E1111_PHY_EXT_ADDRESS_REG + EXT_ADD_SERDES_TX_RX_CONTROL + ['i']).strip()
    except Exception as err:
        syslog.syslog(syslog.LOG_ERR, "ext address reg set failed with err %s" % str(err))

    try:
        cmd_out = subprocess.check_output(I2C_SET+ [str(i2c_bus)] + MVL88E1111_PHY_I2CADDR +\
                          MVL88E1111_PHY_EXT_CTRL_REG+ reg_data + ['i']).strip()
    except Exception as err:
        syslog.syslog(syslog.LOG_ERR, "control reg set failed with err %s" % str(err))

def _mvl88e1111_line_side_link_monitor():
    global cusfp_1g_task
    global cusfp_1g_lock
    global cusfp_1g_stopping_event
    while not cusfp_1g_stopping_event.is_set():
        cusfp_1g_lock.acquire()
        for port in cusfp_1g_ports:
            eeprom_path = cusfp_1g_ports[port]["eeprom_path"]
            i2c_bus = int(re.search(r"(?<=i2c-)[0-9]+", eeprom_path).group(0))
            phy_link_status = cusfp_1g_ports[port]["phy_link_status"]
            try:
                cmd_out = subprocess.check_output(I2C_GET + [str(i2c_bus)] +\
                          MVL88E1111_PHY_I2CADDR + MVL88E1111_PHY_STATUS_REG + ['w']).strip()
                # cmd_out will be in bytes (example b'0x40ac')
                cmd_out_str = cmd_out.decode()
                hex_bytes = bytes.fromhex(cmd_out_str[2:])
                reg_val = int.from_bytes(hex_bytes, byteorder='little')
                link_status = _get_bits(reg_val, (MVL88E1111_PHY_LINK_STATUS_BIT),\
                                         MVL88E1111_PHY_LINK_STATUS_LEN)
                line_speed_val = _get_bits(reg_val, (MVL88E1111_PHY_LINE_SPEED_BIT),\
                                         MVL88E1111_PHY_LINE_SPEED_LEN)
                line_speed = mvl88e1111_speed_map.get(line_speed_val)
                if link_status != phy_link_status:
                    if link_status:
                        ser_des_enable = True
                        if line_speed is not None:
                            _notify_phy_negotiated_speed(port, line_speed, True)
                    else:
                        ser_des_enable = False
                    if cusfp_1g_ports[port]["retry_count"] == 0:
                         _mvl88e1111_serdes_control(i2c_bus, ser_des_enable)
                         cusfp_1g_ports[port]["phy_link_status"] = link_status
                    else:
                        cusfp_1g_ports[port]["retry_count"] =  cusfp_1g_ports[port]["retry_count"] - 1
                else:
                    cusfp_1g_ports[port]["retry_count"] = LINK_STATUS_REG_READ_RETRY_COUNT
            except Exception as err:
                syslog.syslog(syslog.LOG_ERR, "_mvl88e1111_line_side_link_monitor {}".format(err))
                continue
        cusfp_1g_lock.release()
        time.sleep(1)
    cusfp_1g_stopping_event.clear()

def mvl88e1111_set_admin_state(logical_port, eeprom_path, enable):
    global cusfp_1g_task
    global cusfp_1g_lock
    if enable:
        if cusfp_1g_task is None:
            cusfp_1g_task = threading.Thread(target=_mvl88e1111_line_side_link_monitor)
            cusfp_1g_task.start()

        cusfp_1g_lock.acquire()
        cusfp_1g_ports[logical_port] = {"eeprom_path":eeprom_path, "phy_link_status":-1, "retry_count":LINK_STATUS_REG_READ_RETRY_COUNT}
        cusfp_1g_lock.release()
    else:
        cusfp_1g_lock.acquire()
        port = cusfp_1g_ports.pop(logical_port, None)
        cusfp_1g_lock.release()
        if len(cusfp_1g_ports) == 0:
            if  cusfp_1g_task is not None:
                cusfp_1g_stopping_event.set()
                cusfp_1g_task = None

def _notify_phy_negotiated_speed(logical_port, neg_speed, notify):
    global appl_db
    global state_db
    if appl_db is None:
        appl_db = daemon_base.db_connect("APPL_DB")
    if state_db is None:
        state_db = daemon_base.db_connect("STATE_DB")

    app_status_port_tbl = swsscommon.ProducerStateTable(appl_db,
                                                        swsscommon.APP_PORT_APP_STATUS_TABLE_NAME)
    state_port_tbl = swsscommon.Table(state_db, STATE_PORT_TABLE)
    if notify:
        fvs = swsscommon.FieldValuePairs([("autoneg_line_speed", str(neg_speed)),\
                                           ("phy_autoneg", "true")])

        state_port_tbl.set(logical_port, fvs)
        app_status_port_tbl.set(logical_port, fvs)
    else:
        if logical_port is not None:
            state_port_tbl.hdel(logical_port, "autoneg_line_speed")
            app_status_port_tbl.hdel(logical_port, "autoneg_line_speed")

            fvs = swsscommon.FieldValuePairs([("phy_autoneg", "false")])
            state_port_tbl.set(logical_port, fvs)
            app_status_port_tbl.set(logical_port, fvs)

def _bcm848xx_line_side_link_monitor():
    global autoneg_task
    global autoneg_lock
    global stopping_event
    while not stopping_event.is_set():
        autoneg_lock.acquire()
        for port in autoneg_ports:
            eeprom_path = autoneg_ports[port]["eeprom_path"]
            neg_speed = autoneg_ports[port]["speed"]
            (ret, value) = _phy_mailbox_read(eeprom_path,\
                            BCM848XX_PHY_CMD_LIST[PHY_RD_LINK_STATE_CMD])
            if ret:
                link_status = _get_bits(value, BCM848XX_PHY_LINK_STATUS_BIT,\
                                        BCM848XX_PHY_LINK_STATUS_LEN)
                link_rate = _get_bits(value, BCM848XX_PHY_LINK_RATE_BIT,\
                                        BCM848XX_PHY_LINK_RATE_BIT_LEN)
                if link_status == BCM848XX_PHY_LINK_UP:
                    line_speed = bcm848xx_link_rate_speed_map[link_rate]
                    if line_speed != neg_speed:
                        _notify_phy_negotiated_speed(port, line_speed, True)
                        autoneg_ports[port]["speed"] = line_speed
        autoneg_lock.release()
        time.sleep(1)
    stopping_event.clear()


def bcm848xx_set_autoneg(logical_port, eeprom_path, enable):
    global autoneg_task
    global autoneg_lock
    if enable:
        if autoneg_task is None:
            autoneg_task = threading.Thread(target=_bcm848xx_line_side_link_monitor)
            autoneg_task.start()

        autoneg_lock.acquire()
        autoneg_ports[logical_port] = {"eeprom_path":eeprom_path, "speed":0}
        autoneg_lock.release()
    else:
        autoneg_lock.acquire()
        port = autoneg_ports.pop(logical_port, None)
        if port is not None:
            _notify_phy_negotiated_speed(logical_port, None, False)
        autoneg_lock.release()
        if len(autoneg_ports) == 0:
            if  autoneg_task is not None:
                stopping_event.set()
                autoneg_task = None
    return True

# APIs for Broadcom 84581 PHY (Gen1)
I2C_SET = ['/usr/sbin/i2cset', '-y', '-f']
BCM845XX_PHY_I2CADDR = ['0x56']

BCM845XX_PHY_DEV_TX_WR = ['0x01']
BCM845XX_PHY_DEV_TX_RD = ['0x21']
BCM845XX_PHY_DEV_SPEED_WR = ['0x07']
BCM845XX_PHY_DEV_SPEED_RD = ['0x27']

BCM845XX_PHY_TX_DISABLE = ['0x00', '0x09', '0x00', '0x01']
BCM845XX_PHY_TX_ENABLE = ['0x00', '0x09', '0x00', '0x00']

BCM845XX_PHY_SPEED_1G = [['0x00', '0x20', '0x00', '0x03'],
                         ['0xFF', '0xE9', '0x03', '0x00'],
                         ['0xFF', '0xE4', '0x91', '0x81'],
                         ['0xFF', '0xE0', '0x13', '0x00']]
BCM845XX_PHY_SPEED_10G = [['0x00', '0x20', '0x10', '0x03'],
                          ['0xFF', '0xE9', '0x00', '0x00'],
                          ['0xFF', '0xE4', '0x90', '0x01'],
                          ['0x00', '0x00', '0x32', '0x00']]

def bcm845xx_set_admin_state(logical_port, eeprom_path, enable):
    if enable:
        reg = BCM845XX_PHY_TX_ENABLE
    else:
        reg = BCM845XX_PHY_TX_DISABLE

    i2c_bus = int(re.search(r"(?<=i2c-)[0-9]+", eeprom_path).group(0))

    try:
        subprocess.run(I2C_SET + [str(i2c_bus)] + BCM845XX_PHY_I2CADDR +\
                       BCM845XX_PHY_DEV_TX_WR + reg + ['i'], \
                       stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    except Exception as err:
        syslog.syslog(syslog.LOG_ERR, "bcm845xx_set_admin_state {}".format(err))
        return False

    return True

def bcm845xx_set_speed(eeprom_path, speed):
    if speed == 10000:
        reg_list = BCM845XX_PHY_SPEED_10G
    elif speed == 1000:
        reg_list = BCM845XX_PHY_SPEED_1G
    else:
        return False

    i2c_bus = int(re.search(r"(?<=i2c-)[0-9]+", eeprom_path).group(0))
    for reg in reg_list:
        try:
            subprocess.run(I2C_SET + [str(i2c_bus)] + BCM845XX_PHY_I2CADDR +\
                           BCM845XX_PHY_DEV_SPEED_WR + reg + ['i'], \
                           stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        except Exception as err:
            syslog.syslog(syslog.LOG_ERR, "bcm845xx_set_speed {}".format(err))
            return False

    return True

def bcm845xx_set_autoneg(logical_port, eeprom_path, enable):
    return True
