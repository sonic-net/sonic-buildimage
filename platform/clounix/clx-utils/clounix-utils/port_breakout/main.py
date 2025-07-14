#!/usr/bin/python3
#
# Terminology
# fpport:       Front Panel Port
# user_port:    Port name which configured in port_config.ini
# clx_port:     Port index which used by clx sdk

import sys
import os
import os.path
import logging
import logging.handlers
import shutil
import subprocess
import re
import jinja2
import datetime
import json
from pprint import pformat
from collections import OrderedDict, defaultdict
from tabulate import tabulate
import click

import libpcfgparsing as PCFG
#from sonic_py_common.device_info import get_machine_info
from sonic_py_common.device_info import get_platform_info
#from swsssdk import ConfigDBConnector, SonicV2Connector
from swsssdk import SonicV2Connector

LOG_IDENTIFIER = 'clx_port_breakout'
LOG_FORMAT = "%(levelname)s: [%(funcName)s] %(message)s"
SCRIPT_DIR = os.path.dirname(__file__)

PORT_BREAKOUT_TYPE_NONE = "NO_BREAKOUT"
PORT_BREAKOUT_TYPE_2X = "BREAKOUT_2X"
PORT_BREAKOUT_TYPE_4X = "BREAKOUT_4X"
PORT_BREAKOUT_TYPE_2_NUM_MAP = {
    PORT_BREAKOUT_TYPE_NONE: 1,
    PORT_BREAKOUT_TYPE_2X: 2,
    PORT_BREAKOUT_TYPE_4X: 4
}
PORT_BREAKOUT_NUM_2_TYPE_MAP = dict(zip(PORT_BREAKOUT_TYPE_2_NUM_MAP.values(), PORT_BREAKOUT_TYPE_2_NUM_MAP.keys()))
PORT_BREAKOUT_TYPE_LIST = PORT_BREAKOUT_TYPE_2_NUM_MAP.keys()

PLANE_PORTS_MAX_COUNT = 32
DEFAULT_MEDIA_PREFIX = 'sr'  # for fiber
DEFAULT_MDIO_DEVAD = '0x1E'
DEFAULT_MDIO_ADDR = '0x2'
DEFAULT_MDIO_VALUE = '0x8000'  # for fiber

PLATFORM_PATH_TEMPLATE = "/usr/share/sonic/device/{platform}"
HWSKU_PATH_TEMPLATE = PLATFORM_PATH_TEMPLATE + "/{hwsku}"
USER_DEFINED_HWSKU_TEMPLATE = "{hwsku}-UD"
PORT_CONFIG_INI = 'port_config.ini'
PORT_CONFIG_CLX = 'port_config.clx'
DEFAULT_SKU = 'default_sku'
ORIG_DEFAULT_SKU = 'default_sku.origin'
CONFIG_DB_JSON_FILE = '/etc/sonic/config_db.json'
BAK_CONFIG_DB_JSON_FILE = CONFIG_DB_JSON_FILE + '-' + LOG_IDENTIFIER

TMP_USER_PORT_NAME_PREFIX = 'tmp_port'
TMP_USER_PORT_NAME_TEMPLATE = TMP_USER_PORT_NAME_PREFIX + '{fpport}/{usr_port_num}'

# Return Value Enum
NOOP = None
BREAK_SUCCESS = 0
ERR_NOTSUPPORT = -1
ERR_UNKOWN_BREAKOUT_TYPE = -2
ERR_OVER_MAXCOUNT = -3
ERR_UNKNOWN_PORT = -4
ERR_PORT_DISABLED = -5
ERR_VALIDATION_CHECK_FAILED = -6

# Render Template
PORT_CONFIG_CLX_TEMPLATE = 'port_config.clx.j2'

PORT_BREAKOUT_TABLE_NAME = 'PORT_BREAKOUT'
PORT_BREAKOUT_JSON_FILE = '/etc/sonic/port_breakout.json'

# STATE DB Schema
PORT_CFG_STATE_NAME = 'PORT_CFG_STATE'
STATE_MODIFIED = 'modified'


# --- init logger ---
logger = logging.getLogger(LOG_IDENTIFIER)
logger.setLevel(logging.INFO)
logger.addHandler(logging.NullHandler())


def init_logger(log_level=logging.INFO, stdout=True):
    global logger
    if stdout:
        stdout_handler=logging.StreamHandler(sys.stdout)
        stdout_handler.setFormatter(logging.Formatter(LOG_FORMAT))
        logger.addHandler(stdout_handler)
    logger.setLevel(log_level)


def m_print(msg, *arg, **kwargs):
    click.echo(msg)


def get_hwsku_from_default_sku(default_sku):
    with open(default_sku, 'r') as fh:
        content = fh.read()
        hwsku, _ = content.split(' ')
    return hwsku


def get_port_config_ini(platform, hwsku):
    hwsku_dir = HWSKU_PATH_TEMPLATE.format(platform=platform, hwsku=hwsku)
    return os.path.join(hwsku_dir, PORT_CONFIG_INI)


def get_port_config_clx(platform, hwsku):
    hwsku_dir = HWSKU_PATH_TEMPLATE.format(platform=platform, hwsku=hwsku)
    return os.path.join(hwsku_dir, PORT_CONFIG_CLX)


def get_platform():
    #machine_info = get_machine_info()
    platform = get_platform_info()
    return platform


def render(src_tpl, *args, **kargs):
    env = jinja2.Environment(loader=jinja2.FileSystemLoader(os.path.dirname(src_tpl)))
    t = env.get_template(os.path.basename(src_tpl))
    return t.render(*args, **kargs)


def write_file(dst_file, content):
    logger.debug("write content to {}".format(dst_file))
    with open(dst_file, 'w+') as fh:
        fh.write(content)


def logic_lane_2_macro_lane(logic_lane):
    return PCFG.logic_lane_2_macro_lane(logic_lane)


def make_tabulate(list1, list2, headers=None):
    if not headers:
        headers=['From', 'to']
    data = [('\n'.join(list1), '\n'.join(list2))]
    return tabulate(data,
                headers=headers,
                tablefmt="simple",
                missingval="",
                disable_numparse=True)

def state_db(op, name, value=''):
    db = SonicV2Connector()
    db.connect(db.STATE_DB)
    res = -1
    client = db.redis_clients[db.STATE_DB]
    if op == 'set':
        res = client.set(name, value)
    elif op == 'del':
        res = client.delete(name, value)
    elif op == 'get':
        res = client.get(name)
    if res == -1:
        raise Exception('Unsupport action!')
    return res


def set_state_db(name, value):
    return state_db('set', name, value)


def get_state_db(name):
    return state_db('get', name)


class PortBreakout:
    def __init__(self):
        self._dryrun_ = True
        self.platform = get_platform()
        self.platform_dir = PLATFORM_PATH_TEMPLATE.format(platform=self.platform)
        self.default_sku = os.path.join(self.platform_dir, DEFAULT_SKU)
        self.orig_default_sku = os.path.join(self.platform_dir, ORIG_DEFAULT_SKU)
        if self.is_firsttime_running:
            self._create_usr_defined_hwsku_dir()
        self.default_hwsku = get_hwsku_from_default_sku(self.default_sku)
        self.default_hwsku_dir = HWSKU_PATH_TEMPLATE.format(
                                        platform=self.platform,
                                        hwsku=self.default_hwsku)
        self.orig_default_hwsku = get_hwsku_from_default_sku(self.orig_default_sku)
        self.orig_default_hwsku_dir = HWSKU_PATH_TEMPLATE.format(
                                                    platform=self.platform,
                                                    hwsku=self.orig_default_hwsku)
        self._init_port_profile()

    def _init_port_profile(self):
        self.cur_port_profile = PortProfile(self.platform, self.default_hwsku)
        self.orig_port_profile = PortProfile(self.platform, self.orig_default_hwsku)

    # --- Breakout APIs ---
    @property
    def is_firsttime_running(self):
        return not os.path.exists(self.orig_default_sku)

    def show(self, fpport_index_list=None, show_index_0based=True, print_msg=True):
        if fpport_index_list and len(fpport_index_list) != 0:
            self.validation_check(fpport_index_list, is_valid=True)
            all_fp_list = fpport_index_list
        else:
            all_fp_list = self.orig_port_profile.get_fpports()
        cur_fp_list = self.cur_port_profile.get_fpports()
        headers = ['FrontPannelPort(0-based)' if show_index_0based else 'FrontPannelPort(1-based)', 'BreakoutPorts', 'Lanes']
        data = []

        for fp in all_fp_list:
            #FrontPannelPort
            item= [fp] if show_index_0based else [str(int(fp)+1)]
            if fp not in cur_fp_list:  # disabled ports
                item[0] = item[0] + ' (disabled)'
                data.append(item)
                continue
            fp_breakout_ports = self.cur_port_profile.get_fpport_user_ports(fp)
            if TMP_USER_PORT_NAME_PREFIX in fp_breakout_ports[0]:  # modified(enable/breakout) ports
                item[0] = item[0] + ' (modified)'
            # BreakoutPorts
            item.append("\n".join(fp_breakout_ports))
            # Lanes
            item.append("\n".join(
                            ",".join(self.cur_port_profile.get_user_port_lanes(p, fp)) for p in fp_breakout_ports))
            data.append(item)

        msg = tabulate(data,
                       headers=headers,
                       tablefmt="simple",
                       missingval="",
                       disable_numparse=True)
        if print_msg:
            m_print(msg)
        return msg

    def is_port_breakable(self, fpport_index):
        """
        Breakable port should have more than 1 logic lane
        :param : fpport_index
        :return:
        """
        return self.is_valid_port(fpport_index) and len(self.orig_port_profile.get_fpport_lanes(fpport_index)) > 1

    def is_valid_port(self, fpport_index):
        """
        Return true if port in original port config ini
        """
        return fpport_index in self.orig_port_profile.get_fpports()

    def is_enabled_port(self, fpport_index):
        fp_ports = self.cur_port_profile.get_fpports()
        return fpport_index in fp_ports

    def breakout_fpports(self, fpport_index_list, breakout_type=PORT_BREAKOUT_TYPE_4X):
        self.validation_check(fpport_index_list, is_valid=True, is_enabled=True, is_breakable=True)
        for fpport_index in fpport_index_list:
            if breakout_type not in PORT_BREAKOUT_TYPE_2_NUM_MAP:
                logger.error('Unsupport breakout type {}!'.format(breakout_type))
                return ERR_UNKOWN_BREAKOUT_TYPE
            if self.cur_port_profile.is_over_plane_ports_cnt_limit(fpport_index, breakout_type):
                logger.error('Over limitation of max plane ports({}) when try to breakout fp port {}(0-based), Abort!'.format(PLANE_PORTS_MAX_COUNT, fpport_index))
                return ERR_OVER_MAXCOUNT
            self._do_breakout(fpport_index, breakout_type)
        self._genereate_cfg()

    def _do_breakout(self, fpport_index, breakout_type):
        fp_usr_ports = self.cur_port_profile.get_fpport_user_ports(fpport_index)
        cur_port_breakout_count = len(fp_usr_ports)
        exp_port_breakout_count = PORT_BREAKOUT_TYPE_2_NUM_MAP[breakout_type]
        if cur_port_breakout_count == exp_port_breakout_count:
            logger.warning('Front pannel port {}(0-based) has been setted to be {}).'.format(fpport_index, breakout_type))
            return NOOP
        logger.info('Front pannel port {}(0-based) breakout type change from {} to {} '.format(fpport_index, PORT_BREAKOUT_NUM_2_TYPE_MAP[cur_port_breakout_count], breakout_type))
        fp_max_speed = sum(int(self.cur_port_profile.get_user_port_speed(usr_port, fpport_index)) for usr_port in fp_usr_ports)
        fp_lanes_reverse = sorted(self.cur_port_profile.get_fpport_lanes(fpport_index), key=int, reverse=True)
        p_alias_prefix = self.cur_port_profile.get_user_port_alias(fp_usr_ports[0], fpport_index).rpartition('/')[0]
        p_lane_count = len(fp_lanes_reverse) / PORT_BREAKOUT_TYPE_2_NUM_MAP[breakout_type]
        tmp_usr_port_infos = OrderedDict()
        for port_cnt in range(1, PORT_BREAKOUT_TYPE_2_NUM_MAP[breakout_type] + 1):
            p_name = TMP_USER_PORT_NAME_TEMPLATE.format(fpport=fpport_index, usr_port_num=str(port_cnt))
            p_lanes = [fp_lanes_reverse.pop() for i in range(p_lane_count)]
            p_alias = p_alias_prefix + '/' + str(port_cnt)
            p_speed = fp_max_speed / PORT_BREAKOUT_TYPE_2_NUM_MAP[breakout_type]
            tmp_usr_port_infos[p_name] = {
                'lanes': p_lanes,
                'alias': p_alias,
                'max-speed': p_speed
            }
        self.cur_port_profile.update_fpport_user_port_info(fpport_index, tmp_usr_port_infos)
        logger.info('Breakout front panel port {}(0-based): \n'.format(fpport_index) + make_tabulate(fp_usr_ports, tmp_usr_port_infos.keys()))


    def disable_fpports(self, fpport_index_list):
        self.validation_check(fpport_index_list, is_valid=True)
        for fpport_index in fpport_index_list:
            if not self.is_enabled_port(fpport_index):
                logger.warning('Front panel port {}(0-based) has been disabled!!'.format(fpport_index))
                continue
            self._disable_fpport(fpport_index)
        self._genereate_cfg()

    def _disable_fpport(self, fpport_index):
        self.cur_port_profile.disable_fpport(fpport_index)
        logger.info('Disable Front panel port {}(0-based)'.format(fpport_index))

    def enable_fpports(self, fpport_index_list):
        self.validation_check(fpport_index_list, is_valid=True)
        for fpport_index in fpport_index_list:
            if self.is_enabled_port(fpport_index):
                logger.warning('Front panel port {}(0-based) has been enabled!!'.format(fpport_index))
                continue
            # check is over plane ports limitation after enable a fp port.
            exp_breakout_count = PORT_BREAKOUT_TYPE_2_NUM_MAP[PORT_BREAKOUT_TYPE_NONE]
            fp_plane = self.orig_port_profile.get_fpport_plane(fpport_index)
            cur_plane_ports_cnt = self.cur_port_profile.get_plane_port_count(fp_plane)
            if cur_plane_ports_cnt + exp_breakout_count > PLANE_PORTS_MAX_COUNT:
                logger.error('Over limitation of max plane ports({}) when try to enable fp port {}(0-based), Abort!'.format(PLANE_PORTS_MAX_COUNT, fpport_index))
                return ERR_OVER_MAXCOUNT
            self._enable_fpport(fpport_index)
        self._genereate_cfg()

    def _enable_fpport(self, fpport_index):
        # generate user ports info of the fp port.
        # use front panel port info from origin port_config.ini
        fp_usr_ports = self.orig_port_profile.get_fpport_user_ports(fpport_index)
        fp_max_speed = sum(int(self.orig_port_profile.get_user_port_speed(usr_port, fpport_index)) for usr_port in fp_usr_ports)
        fp_lanes = self.orig_port_profile.get_fpport_lanes(fpport_index)
        p_alias_prefix = self.orig_port_profile.get_user_port_alias(fp_usr_ports[0], fpport_index).rpartition('/')[0]
        tmp_usr_port_infos = OrderedDict()
        tmp_usr_port_name = TMP_USER_PORT_NAME_TEMPLATE.format(fpport=fpport_index, usr_port_num=1)
        tmp_usr_port_infos[tmp_usr_port_name] = {
            'lanes': fp_lanes,
            'alias': p_alias_prefix + '/1',
            'max-speed': fp_max_speed
        }
        # update current port profile with the new user ports info
        self.cur_port_profile.update_fpport_user_port_info(fpport_index, tmp_usr_port_infos)
        logger.info('Enable Front panel port {}(0-based)'.format(fpport_index))

    def reset_fpports(self, fpport_index_list):
        self.validation_check(fpport_index_list, is_valid=True, is_enabled=True)
        for fpport_index in fpport_index_list:
            self._do_breakout(fpport_index, breakout_type=PORT_BREAKOUT_TYPE_NONE)
            logger.info('Reset Front panel port {}(0-based)'.format(fpport_index))
        self._genereate_cfg()

    def reset_all(self):
        shutil.rmtree(self.default_hwsku_dir)
        shutil.copytree(self.orig_default_hwsku_dir, self.default_hwsku_dir)
        self.reset_config_db_json()
        logger.warning('Reset all front panel ports breakout configuration!')

    def reset_config_db_json(self):
        """
        Trigger to regenerate configuration as firsttime on next booting.
        Last configuration will be backuped.
        :return:
        """
        # backup config db json
        if os.path.exists(CONFIG_DB_JSON_FILE):
            now = datetime.datetime.now()
            timestamp = str(now.year) + str(now.month) + str(now.day) + str(now.hour) + str(now.minute) + "_" + str(now.second)
            bak_cfg_json = BAK_CONFIG_DB_JSON_FILE + '-' + timestamp
            shutil.copy(CONFIG_DB_JSON_FILE, bak_cfg_json)
            os.remove(CONFIG_DB_JSON_FILE)
            logger.warning("Set default!! Previous config_db.json is stored to - {}.".format(bak_cfg_json))
        else:
            logger.info("config_db.json not found! Skip backup!")

        # touch firsttime
        res = subprocess.check_output('sonic_installer list', shell=True, stderr=subprocess.STDOUT)
        boot_image = re.search(r"Current: SONiC-OS-([^\s]*)", res).group(1)
        first_time_flag = '/host/image-{}/platform/firsttime'.format(boot_image)
        with open(first_time_flag, 'w+') as fh:
            pass
        logger.warning("Touch firsttime booting flag!")

    def _generate_port_config_ini(self):
        header = ('# name', 'lanes', 'alias', 'index', 'speed')
        name_template = "Ethernet{}"
        next_name_index = 0
        alias_tpl = "Ethernet{}/{}"
        data = list()
        cur_fpports = self.cur_port_profile.get_fpports()
        orig_fpports = self.orig_port_profile.get_fpports()
        for fpport_index in orig_fpports:
            if fpport_index not in cur_fpports:
                next_name_index += len(self.orig_port_profile.get_fpport_lanes(fpport_index))
                continue
            usr_ports = self.cur_port_profile.get_fpport_user_ports(fpport_index)
            # user ports of front panel port should be stored in a OrderedDict
            for index, usr_p in enumerate(usr_ports, start=1):
                usr_port_lanes = self.cur_port_profile.get_user_port_lanes(usr_p, fpport_index)
                item = [
                    name_template.format(next_name_index),
                    ','.join(usr_port_lanes),
                    alias_tpl.format(int(fpport_index)+1, index),
                    fpport_index,
                    self.cur_port_profile.get_user_port_speed(usr_p, fpport_index)
                ]
                data.append(item)
                next_name_index += len(usr_port_lanes)
        self.new_port_config_ini = tabulate(data, headers=header, tablefmt='plain')
        logger.info('Generated new port config ini content.')
        logger.debug('New port config ini content \n' + self.new_port_config_ini)

    def _generate_port_config_clx(self):
        port_map_list = list()
        lane_swap_list = list()
        polarity_rev_list = list()
        pre_emphasis_list = list()
        port_mdio_list = list()
        port_medium_list = list()
        port_speed_list = list()

        user_port_2_clx_port = dict()
        next_unit_port_index = dict()

        # generate clx_port id
        for fpport_index in self.cur_port_profile.get_fpports():
            for usr_p in self.cur_port_profile.get_fpport_user_ports(fpport_index):
                unit = self.cur_port_profile.get_user_port_unit(usr_p, fpport_index)
                if unit not in next_unit_port_index:
                    next_unit_port_index[unit] = 0
                port = next_unit_port_index[unit]
                user_port_2_clx_port[usr_p] = (unit, port)
                next_unit_port_index[unit] += 1

        for fpport_index in self.cur_port_profile.get_fpports():
            user_ports = self.cur_port_profile.get_fpport_user_ports(fpport_index)
            # port-map & mdio & speed & medium
            for usr_p in user_ports:
                p_unit, p_port = user_port_2_clx_port[usr_p]
                p_lanes = self.cur_port_profile.get_user_port_lanes(usr_p, fpport_index)
                p_macro, p_lane = logic_lane_2_macro_lane(p_lanes[0])
                p_max_speed = int(self.cur_port_profile.get_user_port_speed(usr_p, fpport_index)) // 1000  # convert speed unit from 1 to k
                port_map_list.append(
                    {'unit': p_unit,
                     'clx_port': p_port,
                     'macro': p_macro,
                     'clx_lane': p_lane,
                     'max_speed': p_max_speed
                     }
                )
                # mdio
                port_mdio_list.append(
                    {'unit': p_unit,
                     'clx_port': p_port,
                     'devad': DEFAULT_MDIO_DEVAD,
                     'addr': DEFAULT_MDIO_ADDR,
                     'value': DEFAULT_MDIO_VALUE
                     }
                )
                # speed
                port_speed_list.append(
                    {'unit': p_unit,
                     'clx_port': p_port,
                     'speed': p_max_speed
                     }
                )
                # medium
                p_medium = DEFAULT_MEDIA_PREFIX
                if len(p_lanes) > 1:
                    p_medium += str(len(p_lanes))  # sr2, sr4
                port_medium_list.append(
                    {'unit': p_unit,
                     'clx_port': p_port,
                     'medium': p_medium
                     }
                )

            fp_lane_list = self.cur_port_profile.get_fpport_lanes(fpport_index)
            fp_1st_p_unit, fp_1st_p_clx_port= user_port_2_clx_port[user_ports[0]]
            # lane-swap
            fp_lane_swap_data = self.cur_port_profile.get_fpport_lane_swap(fpport_index)
            lane_swap_list.append(
                {"unit": fp_1st_p_unit,
                 "clx_port": fp_1st_p_clx_port,
                 "lane_cnt": len(fp_lane_list),
                 "data": fp_lane_swap_data}
            )
            # polarity-rev
            fp_polarity_rev_data = self.cur_port_profile.get_fpport_polarity_rev(fpport_index)
            polarity_rev_list.append(
                {"unit": fp_1st_p_unit,
                 "clx_port": fp_1st_p_clx_port,
                 "lane_cnt": len(fp_lane_list),
                 "data": fp_polarity_rev_data}
            )
            # pre-emphasis
            fp_pre_emphasis_data = self.cur_port_profile.get_fpport_pre_emphasis(fpport_index)
            pre_emphasis_list.append(
                {"unit": fp_1st_p_unit,
                 "clx_port": fp_1st_p_clx_port,
                 "lane_cnt": len(fp_lane_list),
                 "data": fp_pre_emphasis_data}
            )
        cpi_data = self._generate_cpi_port_config_clx()
        self.new_port_config_clx = render(
            src_tpl=os.path.join(SCRIPT_DIR, PORT_CONFIG_CLX_TEMPLATE),
            port_map=port_map_list,
            lane_swap=lane_swap_list,
            polarity_rev=polarity_rev_list,
            pre_emphasis=pre_emphasis_list,
            port_speed=port_speed_list,
            port_medium=port_medium_list,
            port_mdio=port_mdio_list,
            **cpi_data
        )
        logger.info('Generated new port config clx content.')
        logger.debug("New port config clx content \n" + self.new_port_config_clx )

    def _generate_cpi_port_config_clx(self):
        data_dict = {
            'cpi_port_map': self.cur_port_profile.port_clx[PCFG.KEY_CPI_PORT_MAP_DATA],
            'cpi_lane_swap': self.cur_port_profile.port_clx[PCFG.KEY_LANE_SWAP].get(PCFG.KEY_CPI_PORT_INFO, []),
            'cpi_polarity_rev': self.cur_port_profile.port_clx[PCFG.KEY_POLARITY_REV].get(PCFG.KEY_CPI_PORT_INFO, []),
            'cpi_pre_emphasis': self.cur_port_profile.port_clx[PCFG.KEY_PRE_EMPHASIS].get(PCFG.KEY_CPI_PORT_INFO, []),
            'cpi_medium_type':  self.cur_port_profile.port_clx[PCFG.KEY_MEDIUM_TYPE].get(PCFG.KEY_CPI_PORT_INFO, []),
            'cpi_adver': self.cur_port_profile.port_clx[PCFG.KEY_ADVER].get(PCFG.KEY_CPI_PORT_INFO, []),
            'cpi_an': self.cur_port_profile.port_clx[PCFG.KEY_AN].get(PCFG.KEY_CPI_PORT_INFO, []),
            'cpi_admin': self.cur_port_profile.port_clx[PCFG.KEY_ADMIN].get(PCFG.KEY_CPI_PORT_INFO, [])
        }
        return data_dict

    def save_breakout_cfg(self, write_to_json=True, set_state=True):
        p_ini_file = get_port_config_ini(self.platform, self.default_hwsku)
        p_clx_file = get_port_config_clx(self.platform, self.default_hwsku)
        logger.info('save port config ini to {}'.format(p_ini_file))
        write_file(p_ini_file, self.new_port_config_ini)
        logger.info('save port config clx to {}'.format(p_clx_file))
        write_file(p_clx_file, self.new_port_config_clx)
        if set_state:
            logger.info('Set STATE_DB(6) {}:{}'.format(PORT_CFG_STATE_NAME, STATE_MODIFIED))
            self._set_portcfg_state(STATE_MODIFIED)
        if write_to_json:
            self.write_to_json()

    def _create_usr_defined_hwsku_dir(self):
        if not os.path.exists(self.default_sku):
            raise Exception('default_sku not found: {}'.format(self.default_sku))
        cur_hwsku = get_hwsku_from_default_sku(self.default_sku)
        cur_hwsku_dir = HWSKU_PATH_TEMPLATE.format(platform=self.platform, hwsku=cur_hwsku)
        usr_defined_hwsku = USER_DEFINED_HWSKU_TEMPLATE.format(hwsku=cur_hwsku)
        usr_defined_hwsku_dir = HWSKU_PATH_TEMPLATE.format(platform=self.platform, hwsku=usr_defined_hwsku)

        # bakup origin default_sku file
        shutil.copy(self.default_sku, self.orig_default_sku)
        # update hwsku to user defined hwsku in default_sku
        with open(self.default_sku, "w") as fh:
            fh.write(usr_defined_hwsku + " t1")
        logger.info("Change hwsku from '{}' to '{}' ".format(cur_hwsku, usr_defined_hwsku))
        # copy hwsku dir to user defined hwsku dir for later use
        shutil.copytree(cur_hwsku_dir, usr_defined_hwsku_dir)
        logger.info("Copy hwsku dir from '{}' to '{}'".format(cur_hwsku_dir, usr_defined_hwsku_dir))


    def _genereate_cfg(self):
        self._generate_port_config_ini()
        self._generate_port_config_clx()
        if not self.dryrun:
            self.save_breakout_cfg()
            self.reset_config_db_json()
        else:
            logger.info('Dryrun mode, skip saving the breakout configuration!')

    @property
    def dryrun(self):
        return self._dryrun_

    @dryrun.setter
    def dryrun(self, val):
        self._dryrun_ = bool(val)

    def validation_check(self, fpport_index_list, is_valid=False, is_enabled=False, is_breakable=False, is_disabled=False):
        validation_msg = 'Front pannel port should be -'
        validation_msg += ' Valid,' if is_valid else ''
        validation_msg += ' Enabled,' if is_enabled else ''
        validation_msg += ' Breakable,' if is_breakable else ''
        validation_msg += ' Disabled,' if is_disabled else ''
        logger.info(validation_msg)
        logger.info("Checking front panel ports index(0-based) - {}".format(fpport_index_list))
        check_pass = True
        check_result_dict = {
            'invalid_port_list': list(),
            'enabled_port_list': list(),
            'disabled_port_list': list(),
            'unbreakable_port_list': list()
        }
        for fpport_index in fpport_index_list:
            if is_valid and not self.is_valid_port(fpport_index):
                check_result_dict['invalid_port_list'].append(fpport_index)
                check_pass = False
                continue
            if is_enabled and not self.is_enabled_port(fpport_index):
                check_result_dict['disabled_port_list'].append(fpport_index)
                check_pass = False
                continue
            if is_disabled and self.is_enabled_port(fpport_index):
                check_result_dict['enabled_port_list'].append(fpport_index)
                check_pass = False
                continue
            if is_breakable and not self.is_port_breakable(fpport_index):
                check_result_dict['unbreakable_port_list'].append(fpport_index)
                check_pass = False
                continue
        if not check_pass:
            logger.warning("Validation check failed !")
            for k, v in check_result_dict.iteritems():
                if len(v) == 0 :
                    continue
                if k == 'invalid_port_list':
                    logger.warning('Invalid front panel port index(0-based) - {}'.format(str(v)))
                if k == 'enabled_port_list':
                    logger.warning('Invalid action for enabled front panel port index(0-based) - {}'.format(str(v)))
                if k == 'disabled_port_list':
                    logger.warning('Invalid action for disabled front panel port index(0-based) - {}'.format(str(v)))
                if k == 'unbreakable_port_list':
                    logger.warning('Unbreakable front panel port index(0-based)  - {}'.format(str(v)))
            raise Exception('Validation check failed !')
        else:
            logger.info("Validation check pass.")
        return True

    def recover_from_json(self, json_file=PORT_BREAKOUT_JSON_FILE):
        '''
        recover port breakout configuration from presaved port breakout json file.
        this method should be called only by clounix service.
        this method should be executed only on sonic first time booting.
        '''
        logger.info('Start recover port breakout configuration from {}'.format(json_file))
        if not os.path.exists(json_file):
            raise Exception('Not found {}!!'.format(json_file))
        with open(json_file, 'r') as fh:
            pb_cfg_from_db = json.load(fh).get(PORT_BREAKOUT_TABLE_NAME)
        if not pb_cfg_from_db:
            return

        #all_fp_list = self.orig_port_profile.get_fpports()
        params = pb_cfg_from_db['parameters']
        fp_index_0based = params['fp_index_0based_flag']
        fpports_cfg = pb_cfg_from_db['fpports']
        disabled_fp_list = []
        enabled_fp_list = []
        breakout_fp_dict = defaultdict(list)
        for fp, fv in fpports_cfg.iteritems():
            fp_index = fp if fp_index_0based else str(int(fp) - 1)  # convert fp to 0based fp_index
            if not self.is_valid_port(fp_index):
                raise Exception('Unknown front pannel {}(0based)'.format(fp_index))
            if fv['status'] == 'disabled':
                disabled_fp_list.append(fp_index)
            else:
                enabled_fp_list.append(fp_index)
                breakou_type = PORT_BREAKOUT_NUM_2_TYPE_MAP[int(fv['breakout_num'])]
                breakout_fp_dict[breakou_type].append(fp_index)

        logger.info('Disable front panel port list(0based): {} '.format(PCFG.create_portlist_range(disabled_fp_list)))
        for p in sorted(disabled_fp_list, key=int):
            self._disable_fpport(p)
        logger.info('Enable front panel port list(0based): {}' .format(PCFG.create_portlist_range(enabled_fp_list)))
        for p in sorted(enabled_fp_list, key=int):
            self._enable_fpport(p)
        for b_type, p_list in breakout_fp_dict.iteritems():
            logger.info('Beakout type: {}, breakout front panel port list(0based): {}'.format(b_type, PCFG.create_portlist_range(p_list)))
            for p in sorted(p_list, key=int):
                self._do_breakout(p, b_type)

        # Should not reset config db and wirte port breakout configuration to json as it's recovering configuration
        self._generate_port_config_ini()
        self._generate_port_config_clx()
        self.save_breakout_cfg(write_to_json=False, set_state=False)
        logger.info('End recover port breakout configuration from config_db.json')


    def write_to_json(self, fp_index_0based=False):
        '''
        write port breakout configuration to json file;
        :params: bool fp_index_0based, write with 0based fp port index if set True else with 1based port index.
        '''
        logger.info('write breakout configuration to {}. Use `config save -y` if you want to save the configuration.'.format(PORT_BREAKOUT_JSON_FILE))
        port_breakout_table = { PORT_BREAKOUT_TABLE_NAME: { 'parameters': {'fp_index_0based_flag': fp_index_0based},
                                                            'fpports': {} } }
        all_fp_list = self.orig_port_profile.get_fpports()
        for fp in all_fp_list:
            fv={}
            if self.is_enabled_port(fp):  # disabled front panel ports
                fv['status'] = 'enabled'
                fv['breakout_num'] = len(self.cur_port_profile.get_fpport_user_ports(fp))
            else:  # modified or enabled front panel ports
                fv['status'] = 'disabled'
            fp_1based_index = fp if fp_index_0based else  str(int(fp) + 1)
            port_breakout_table[PORT_BREAKOUT_TABLE_NAME]['fpports'][fp_1based_index] = fv
        if not os.path.exists(os.path.dirname(PORT_BREAKOUT_JSON_FILE)):
            os.makedirs(os.path.dirname(PORT_BREAKOUT_JSON_FILE))
        with open(PORT_BREAKOUT_JSON_FILE, 'w+') as fh:
            fh.write(json.dumps(port_breakout_table))

    def _set_portcfg_state(self, state):
        set_state_db(PORT_CFG_STATE_NAME, state)


class PortProfile:
    """
        # parse_port_config_ini(config_file_path)
        {
            'phy_port_info':{
                physical port:{
                    'user_port':{ name:{
                                        'lanes':[logic_lane1, logic_lane2...],
                                        'alias':'',
                                        'max-speed':dd
                                }
                    }
                    'lane_list': [logic_lane1, logic_lane2]
                }
            }
            'logical_lane_2_phy_port':{
                logic_lane: physical port
            }
            'user_port_2_phy_port': {
                user_port: phy port
            }
        }

        # parse_clx_dsh(config_file_path)
        {
        'clx_port_2_logic_lane': {
            clx_port(unit,port): [logic_lane1, logic_lane2]
        },
        'logic_lane_2_clx_port': {
            logic_lane: clx_port(unit,port)
        },
        'pre-emphasis': {
            # key: value
            (logic_lane, property): hex
        },
        'polarity-rev':
            # key: value
            (logic_lane, property): hex
        'lane-swap':
            # key: value
            (logic_lane, property): hex
        }
    """
    def __init__(self, platform, hwsku):
        PCFG.logger = logger
        self.platform = platform
        self.hwsku = hwsku
        self.port_config = PCFG.parse_port_config_ini(get_port_config_ini(self.platform, self.hwsku))
        self.phy_port_info = self.port_config[PCFG.KEY_PHY_PORT_INFO]
        self.port_clx = PCFG.parse_clx_dsh(get_port_config_clx(self.platform, self.hwsku))
        logger.debug( "{} - port_config - \n {}".format(self.hwsku,pformat(self.port_config)))
        logger.debug( "{} - port_clx - \n {}".format(self.hwsku, pformat(self.port_clx)))

    def get_fpports(self):
        return sorted(self.phy_port_info.keys(), key=int)

    def is_over_plane_ports_cnt_limit(self, fpport_index, breakout_type):
        port_plane = self.get_fpport_plane(fpport_index)
        plane_port_count = self.get_plane_port_count(port_plane)
        fp_breakout_count = len(self.get_fpport_user_ports(fpport_index))
        fp_expect_breakout_count = PORT_BREAKOUT_TYPE_2_NUM_MAP[breakout_type]
        return (plane_port_count - fp_breakout_count + fp_expect_breakout_count) > PLANE_PORTS_MAX_COUNT

    def get_fpport_info(self, fpport_index):
        return self.phy_port_info.get(fpport_index)

    def get_user_port_lanes(self, user_port_name, fpport_index=None):
        v = self.get_user_port_item(user_port_name,
                                     PCFG.KEY_PHY_PORT_INFO_V_USER_PORT_V_LANES,
                                     fpport_index)
        return sorted(v, key=int)

    def get_user_port_alias(self, user_port_name, fpport_index=None):
        v = self.get_user_port_item(user_port_name,
                                     PCFG.KEY_PHY_PORT_INFO_V_USER_PORT_V_ALIAS,
                                     fpport_index)
        return v

    def get_user_port_speed(self, user_port_name, fpport_index=None):
        v = self.get_user_port_item(user_port_name,
                                     PCFG.KEY_PHY_PORT_INFO_V_USER_PORT_V_MAX_SPEED,
                                     fpport_index)
        if not v:
            v ='0'
        return v

    def get_user_port_unit(self, user_port_name, fpport_index=None):
        if not fpport_index:
            fpport_index = self.port_config[PCFG.KEY_USER_PORT_2_PHY_PORT].get(user_port_name)
        if not fpport_index:
            raise Exception('Can not find front panel index of user port {}'.format(user_port_name))
        fp_lanes = self.get_fpport_lanes(fpport_index)
        clx_port = None
        for logic_lane in fp_lanes:
            clx_port = self.port_clx[PCFG.KEY_LOGIC_LANE_2_CLX_PORT].get(logic_lane)
            if clx_port:
                return clx_port[0]
        return None

    def get_user_port_clx_port(self, user_port_name, fpport_index=None):
        usr_p_lanes = self.get_user_port_lanes(user_port_name, fpport_index)
        clx_port = None
        for logic_lane in usr_p_lanes:
            clx_port = self.port_clx[PCFG.KEY_LOGIC_LANE_2_CLX_PORT].get(logic_lane)
            if clx_port:
                return clx_port[1]
        return None

    def get_user_port_item(self, user_port_name, key, fpport_index=None):
        user_port = self.get_fpport_user_port_info(user_port_name, fpport_index)
        return user_port.get(key)

    def get_fpport_item(self, fpport_index, key):
        fp = self.phy_port_info[fpport_index]
        return fp.get(key)

    def get_fpport_lanes(self, fpport_index):
        return sorted( self.get_fpport_item(fpport_index, PCFG.KEY_PHY_PORT_INFO_V_LANE_LIST), key=int)

    def get_fpport_lane_swap(self, fpport_index):
        lane_list = self.get_fpport_lanes(fpport_index)
        data = {
            'rx': [],
            'tx': []
        }
        t_lane_swap = self.port_clx[PCFG.KEY_LANE_SWAP]
        for logic_lane in lane_list:
            data['rx'].append(t_lane_swap.get((logic_lane, 'rx')))
            data['tx'].append(t_lane_swap.get((logic_lane, 'tx')))
        return data

    def get_fpport_polarity_rev(self, fpport_index):
        lane_list = self.get_fpport_lanes(fpport_index)
        data = {
            'rx': [],
            'tx': []
        }
        t_polarity_rev = self.port_clx[PCFG.KEY_POLARITY_REV]
        for logic_lane in lane_list:
            data['rx'].append(t_polarity_rev.get((logic_lane, 'rx')))
            data['tx'].append(t_polarity_rev.get((logic_lane, 'tx')))
        return data

    def get_fpport_pre_emphasis(self, fpport_index):
        """
        return pre-emphasis configuration from default config file(clounix_opt.dsh).
        """
        DEFAULT_PRE_EMPHASIS_OPT_CFG_FILE_NAME = 'clounix_opt.dsh'
        pre_emphasis_cfg_file = os.path.join(
                                    '/usr/share/sonic/device',
                                    self.platform,
                                    self.hwsku,
                                    DEFAULT_PRE_EMPHASIS_OPT_CFG_FILE_NAME
                                    )
        t_pre_emphasis = PCFG.parse_clx_dsh(pre_emphasis_cfg_file)[PCFG.KEY_PRE_EMPHASIS]
        lane_list = self.get_fpport_lanes(fpport_index)
        data = {
            'c2': [],
            'cn1': [],
            'c1': [],
            'c0': []
        }
        for logic_lane in lane_list:
            for p in data.keys():
                data[p].append(t_pre_emphasis.get((logic_lane, p)))
        return data

    def get_fpport_user_port_info(self, user_port_name, fpport_index=None):
        if fpport_index:
            fp = self.phy_port_info[fpport_index]
            return fp[PCFG.KEY_PHY_PORT_INFO_V_USER_PORT].get(user_port_name)
        else:
            user_port = None
            for v in self.phy_port_info.itervalues():
                user_port = v[PCFG.KEY_PHY_PORT_INFO_V_USER_PORT].get(user_port_name)
                if user_port:
                    break
            return user_port

    def get_fpport_user_ports(self, fpport_index):
        fp = self.phy_port_info[fpport_index]
        usr_p_infos = fp[PCFG.KEY_PHY_PORT_INFO_V_USER_PORT].items()
        # sort user port name by logic_lane
        usr_p_infos.sort(
            key=lambda x: int(x[1][PCFG.KEY_PHY_PORT_INFO_V_USER_PORT_V_LANES][0])
        )
        usr_ports = [p[0] for p in usr_p_infos]
        return usr_ports

    def get_fpport_plane(self, fpport_index):
        fp_1st_lane = self.get_fpport_lanes(fpport_index)[0]
        fp_plane = str(int(fp_1st_lane)//64)
        return fp_plane

    def get_plane_port_count(self, plane):
        count = 0
        for fp in self.get_fpports():
            if self.get_fpport_plane(fp) == plane:
                count += len(self.get_fpport_user_ports(fp))
        return count

    def set_user_port_item(self, user_port_name, key, value, fpport_index=None):
        u = self.get_fpport_user_port_info(user_port_name, fpport_index)
        u[key] = value

    def update_fpport_user_port_info(self, fpport_index, usr_port_infos):
        tmp_usr_port_odict = OrderedDict()
        tmp_fp_lane_list = []
        for usr_p, v in usr_port_infos.iteritems():
            tmp_usr_port_odict[usr_p] = {
                PCFG.KEY_PHY_PORT_INFO_V_USER_PORT_V_LANES: v['lanes'],
                PCFG.KEY_PHY_PORT_INFO_V_USER_PORT_V_ALIAS: v['alias'],
                PCFG.KEY_PHY_PORT_INFO_V_USER_PORT_V_MAX_SPEED: v['max-speed']
            }
            for lane in v['lanes']:
                if lane not in tmp_fp_lane_list:
                    tmp_fp_lane_list.append(lane)
        tmp_fp_lane_list.sort(key=int)
        fp = self.get_fpport_info(fpport_index)
        if not fp:  # disabled fp
            fp = dict()
            fp[PCFG.KEY_PHY_PORT_INFO_V_LANE_LIST] = tmp_fp_lane_list
            self.phy_port_info[fpport_index] = fp
        fp[PCFG.KEY_PHY_PORT_INFO_V_USER_PORT] = tmp_usr_port_odict

    def disable_fpport(self, fpport_index):
        if fpport_index in self.phy_port_info:
            self.phy_port_info.pop(fpport_index)

def get_fp_ports_4_click(ctx, args, incomplete):
    # available until click version >=7.0
    pb = ctx.obj['pb']
    cur_fpports = pb.cur_port_profile.get_fpports()
    orig_fpports = pb.orig_port_profile.get_fpports()
    content = 'Current front panel ports(0-based): {}\n'.format(PCFG.create_portlist_range(cur_fpports))
    content += 'All front panel ports(0-based): {}'.format(PCFG.create_portlist_range(orig_fpports))


#
# 'cli' group (root group)
#

@click.group()
@click.pass_context
@click.option('-l','--log-level',
              type=click.Choice(('DEBUG', 'INFO', 'WARN', 'ERROR')),
              help='Set log level. Default is WARN.')
@click.option('-0','--fp-index-0based', is_flag=True, default=False,
              help='Default use 1-based port index.')
@click.option('-d','--dryrun', is_flag=True,
              help='Change would not be saved.')
def cli(ctx, log_level, fp_index_0based, dryrun):
    """
    Port breakout related configuration.
    """
    if os.geteuid() != 0:
        exit("Root privileges are required for this operation")

    context = {
        "pb": PortBreakout()
    }
    context.update(ctx.params)   # store parameters for subcommand used.
    ctx.obj = context

    LOG_LEVEL_MAP = {
        'ERROR': logging.ERROR,
        'WARN': logging.WARN,
        'INFO': logging.INFO,
        'DEBUG': logging.DEBUG
    }
    log_level = LOG_LEVEL_MAP.get(str(log_level), logging.WARN)
    init_logger(log_level)
    pb = ctx.obj['pb']
    pb.dryrun = dryrun

#
# 'port-breakout' command ("port-breakout breakout")
#

@cli.command()
@click.pass_context
@click.argument('fpport_range',
                metavar='<Front-Panel-Port-Range>',
                required=True)
@click.argument('breakout_num',
                metavar='[<Breakout-Num>]',
                default=PORT_BREAKOUT_TYPE_2_NUM_MAP[PORT_BREAKOUT_TYPE_4X],
                type=click.Choice(map(str, PORT_BREAKOUT_TYPE_2_NUM_MAP.values())),
                required=False)
def breakout(ctx, fpport_range, breakout_num):
    """
    Port breakout configuration.
    Breakout front panel ports to 2 or 4 sub-ports.
    """
    pb = ctx.obj["pb"]
    fpport_list  = PCFG.parse_portlist_range(fpport_range)
    # convert front panel port index from 1based to 0based
    if not ctx.obj['fp_index_0based']:
        fpport_list = map(lambda x: str(int(x)-1), fpport_list)
    breakout_type = PORT_BREAKOUT_NUM_2_TYPE_MAP[int(breakout_num)]
    pb.breakout_fpports(fpport_list, breakout_type)

#
# 'port-breakout' command ("port-breakout enable")
#

@cli.command()
@click.pass_context
@click.argument('fpport_range',
                metavar='<Front-Panel-Port-Range>',
                required=True)
def enable(ctx, fpport_range):
    """
    Enable front panel ports.
    Disabe front panel ports would not get initialized.
    """
    pb = ctx.obj["pb"]
    fpport_list  = PCFG.parse_portlist_range(fpport_range)
    # convert front panel port index from 1based to 0based
    if not ctx.obj['fp_index_0based']:
        fpport_list = map(lambda x: str(int(x)-1), fpport_list)
    pb.enable_fpports(fpport_list)

#
# 'port-breakout' command ("port-breakout disable")
#

@cli.command()
@click.pass_context
@click.argument('fpport_range',
                metavar='<Front-Panel-Port-Range>',
                required=True)
def disable(ctx, fpport_range):
    """
    Disable front panel ports.
    Disabe front panel ports would not get initialized.
    """
    pb = ctx.obj["pb"]
    fpport_list = PCFG.parse_portlist_range(fpport_range)
    # convert front panel port index from 1based to 0based
    if not ctx.obj['fp_index_0based']:
        fpport_list = map(lambda x: str(int(x)-1), fpport_list)
    pb.disable_fpports(fpport_list)

#
# 'port-breakout' command ("port-breakout reset")
#

@cli.command()
@click.pass_context
@click.argument('fpport_range',
                metavar='<Front-Panel-Port-Range>',
                required=True)
def reset(ctx, fpport_range):
    """
    Reset breakout configuration for specified ports.
    """
    pb = ctx.obj["pb"]
    fpport_list = PCFG.parse_portlist_range(fpport_range)
    # convert front panel port index from 1based to 0based
    if not ctx.obj['fp_index_0based']:
        fpport_list = map(lambda x: str(int(x)-1), fpport_list)
    pb.reset_fpports(fpport_list)

#
# 'port-breakout' command ("port-breakout reset-all")
#

@cli.command('reset-all')
@click.pass_context
@click.option('-y','--yes', is_flag=True)
def reset_all(ctx, yes):
    """
    Reset breakout configuration for all ports.
    """
    if not yes:
        click.confirm('Reset all ports breakout configuration?', abort=True, default=False)
    pb = ctx.obj["pb"]
    pb.reset_all()

#
# 'port-breakout' command ("port-breakout show")
#

@cli.command()
@click.pass_context
@click.argument('fpport_range',
                metavar='<Front-Panel-Port-Range>',
                required=False)
def show(ctx, fpport_range):
    """Show port breakout related information."""
    pb = ctx.obj["pb"]
    fp_index_0based = ctx.obj['fp_index_0based']
    fpport_list = PCFG.parse_portlist_range(fpport_range)
    if not fp_index_0based:
        fpport_list = map(lambda x: str(int(x)-1), fpport_list)
    pb.show(fpport_list, fp_index_0based)

#
# 'port-breakout' command ("port-breakout recover-from-json")
#

@cli.command('recover-from-json')
@click.pass_context
@click.option('-j', '--json-file',
              metavar='<Breakout-json-file>',
              required=False,
              default=PORT_BREAKOUT_JSON_FILE)
def recover_from_json(ctx, json_file):
    """Recover port breakout configuration from json."""
    pb = ctx.obj["pb"]
    pb.recover_from_json(json_file)

if __name__=='__main__':
    cli()

