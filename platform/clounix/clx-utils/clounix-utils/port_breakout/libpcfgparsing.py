#import os.path
import re
import logging
import subprocess
#from pprint import pprint, pformat
from pprint import pprint
#from collections import OrderedDict

KEY_CLX_PORT_INFO = 'clx_port_info'
KEY_CLX_PORT_INFO_V_ETH_MACRO = 'eth-macro'
KEY_CLX_PORT_INFO_V_CLX_LANE = 'clx-lane'
KEY_CLX_PORT_INFO_V_MAX_SPEED = 'max-speed'
KEY_CLX_PORT_INFO_V_PLANE = 'plane'
KEY_CLX_PORT_INFO_V_PPID = 'ppid'

KEY_PRE_EMPHASIS = 'pre-emphasis'
KEY_POLARITY_REV = 'polarity-rev'
KEY_LANE_SWAP = 'lane-swap'
KEY_MEDIUM_TYPE = 'medium-type'
KEY_AN = 'an'
KEY_ADVER = 'adver'
KEY_ADMIN = 'admin'
KEY_CPI_PORT_INFO = 'cpi_port_info'

KEY_PHY_PORT_INFO = 'phy_port_info'
KEY_PHY_PORT_INFO_V_LANE_LIST = 'lane_list'
KEY_PHY_PORT_INFO_V_USER_PORT = 'user_port'
KEY_PHY_PORT_INFO_V_USER_PORT_V_ALIAS = 'alias'
KEY_PHY_PORT_INFO_V_USER_PORT_V_LANES = 'lanes'
KEY_PHY_PORT_INFO_V_USER_PORT_V_MAX_SPEED = 'max-speed'

KEY_LOGIC_LANE_2_PHY_PORT = 'logical_lane_2_phy_port' 
KEY_USER_PORT_2_PHY_PORT = 'user_port_2_phy_port'
KEY_CLX_PORT_2_LOGIC_LANE = 'clx_port_2_logic_lane'
KEY_LOGIC_LANE_2_CLX_PORT = 'logic_lane_2_clx_port'
KEY_CPI_PORT_MAP_DATA = 'cpi_port_map_data'
CPI_PORT_LIST = ["129", "130"]

logger=logging.getLogger()


def _run_diag_command(command, verbose=False):
    cmd = ['clx_diag', command ]
    mapping_str_buffer = subprocess.check_output(cmd)
    if verbose :
        pprint(mapping_str_buffer)
    return mapping_str_buffer


def _diag_show_info_port():
    """
    run clx_diag cmd to get clx_port and macro lane mapping
    :return:
    """

    # get clx_port to macro and lane mapping
    # unit/port eth-macro lane max-speed act gua bin plane hw-mac tm-mac mpid ppid
    #   0/  0        0    0    100000   1   0   0     1    16     0     0     0
    cmd = 'diag show info port'
    mapping_str_buffer = _run_diag_command(cmd)
    return mapping_str_buffer


def parse_diag_show_info_port():
    """
    parsing diag show info port output
    :param: None
    :return: 
        {'clx_port_info':{ clx_port: {
                                'eth-macro': eth-macro,
                                'clx-lane': clx-lane,
                                'max-speed': max-speed,
                                'plane': plane,
                                'ppid': ppid
                                }
                         }
        }
    """
    diag_show = _diag_show_info_port()
    # unit/port eth-macro lane max-speed act gua bin plane hw-mac tm-mac mpid ppid
    #   0/  0        0    0    100000   1   0   0     1    16     0     0     0
    pr_str = r'''^\s+(?P<unit>\d+)\s*/\s*(?P<port>\d+)\s+   #0/0
            (?P<macro>\d+)\s+   #eth-macro
            (?P<clx_lane>\d+)\s+    #lane
            (?P<speed>\d+)\s+   #max-speed
            \d+\s+  #act
            \d+\s+  #gua
            \d+\s+  #bin
            (?P<plane>\d+)\s+   #plane
            \d+\s+  #hw-mac
            \d+\s+  #tm-mac
            \d+\s+  #mpid
            (?P<ppid>\d+)   #ppid
            .*$'''
    pat = re.compile(pr_str, re.I | re.M | re.X)
    result = pat.finditer(diag_show)

    data = {
        KEY_CLX_PORT_INFO: dict()
    }
    for p in result:
        unit = p.group('unit')
        port = p.group('port')
        eth_macro = p.group('macro')
        clx_lane = p.group('clx_lane')
        max_speed = p.group('speed')
        plane = p.group('plane')
        ppid = p.group('ppid')

        data[KEY_CLX_PORT_INFO][(unit, port)] = {
            KEY_CLX_PORT_INFO_V_ETH_MACRO: eth_macro,
            KEY_CLX_PORT_INFO_V_CLX_LANE: clx_lane,
            KEY_CLX_PORT_INFO_V_MAX_SPEED: max_speed,
            KEY_CLX_PORT_INFO_V_PLANE: plane,
            KEY_CLX_PORT_INFO_V_PPID: ppid
        }
    return data


def parse_port_config_ini(port_config_file):
    """
    parse phy port to (eth-group, lane) mapping from config file
    :param: port_config_file
    :return: 
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
    """
    with open(port_config_file, mode='r') as f:
        config = f.read()

    # match content of port_config.ini:
    # name            lanes             alias             index     speed
    # Ethernet0       0,1,2,3           Ethernet1/1       0         100000
    title = []
    data_dict = {
        KEY_PHY_PORT_INFO: dict(),
        KEY_LOGIC_LANE_2_PHY_PORT: dict(),
        KEY_USER_PORT_2_PHY_PORT: dict()
    }
    for line in config.split('\n'):
        if re.search('^#', line):
            # The current format is: # name lanes alias index speed
            title = line.split()[1:]
            continue
        content = line.split()
        if len(content) == 0 or len(content) != len(title) :  # skip blank or invalid line
            continue
        user_port_name = content[title.index('name')]
        logic_lanes = content[title.index('lanes')]
        logic_lane_list = logic_lanes.split(',')
        user_port_alias = content[title.index('alias')]
        phy_port = content[title.index('index')]
        if 'speed' in title:
            max_speed = content[title.index('speed')]
        else:
            max_speed = str(25000 * len(logic_lane_list))

        # update user_port_2_phy_port_mapping
        data_dict[KEY_USER_PORT_2_PHY_PORT][user_port_name] = phy_port

        # update logical_lane_2_phy_port mapping
        for logic_lane in logic_lane_list:
            data_dict[KEY_LOGIC_LANE_2_PHY_PORT][logic_lane] = phy_port

        # -- update phy_port_info --
        # phy_port_info->value
        phy_port_info = data_dict[KEY_PHY_PORT_INFO]
        if phy_port not in phy_port_info:
            phy_port_info[phy_port] = dict()
        phy_port_info_value = phy_port_info[phy_port]

        # phy_port_info->value->user_port->value
        k = KEY_PHY_PORT_INFO_V_USER_PORT
        if k not in phy_port_info_value:
            phy_port_info_value[k] = OrderedDict()
        all_user_port_value = phy_port_info_value[k]

        # get phy_port_info->value->user_port->value->(port_name)->value
        if user_port_name not in all_user_port_value:
            all_user_port_value[user_port_name] = dict()
        cur_user_port_value = all_user_port_value[user_port_name]

        # update phy_port_info->value->user_port->value->(port_name)->value->alias->value
        cur_user_port_value[KEY_PHY_PORT_INFO_V_USER_PORT_V_ALIAS] = user_port_alias

        # update phy_port_info->value->user_port->value->(port_name)->value->max-speed->value
        cur_user_port_value[KEY_PHY_PORT_INFO_V_USER_PORT_V_MAX_SPEED] = max_speed

        # update phy_port_info->value->user_port->value->(port_name)->value->lanes->value
        cur_user_port_value[KEY_PHY_PORT_INFO_V_USER_PORT_V_LANES] = logic_lane_list[:]

        # updte phy_port_info->value->lane_list->value
        k=KEY_PHY_PORT_INFO_V_LANE_LIST
        if k not in phy_port_info_value:
            phy_port_info_value[k] = list()
        phy_port_info_value[k].extend(logic_lane_list)
       
    return data_dict


def _parse_port_map(config_file):
    """
    :return:
    {
        'clx_port_2_logical_lane': {
            clx_port(unit,port): [logic_lane1, logic_lane2]
        },
        'logical_lane_2_clx_port': {
            logic_lane: clx_port(unit,port)
        },
        'cpi_port_map_data':{
            'init set port-map port=129 eth-macro=0 lane=1 max-speed=10g active=true guarantee=true cpi=true',
            'init set port-map port=130 eth-macro=0 lane=0 max-speed=10g active=true guarantee=true cpi=true init-done=true'
        }
    }
    """
    with open(config_file, mode='r') as f:
        config_content = f.read()
    
    #init set port-map port=0  eth-macro=2  lane=0 max-speed=25g active=true
    pat_str = r'''^init\s+set\s+port-map\s+
            (unit=(?P<unit>\d+)\s+){0,1}   # unit=0 
            port=(?P<port>\d+)\s+  # port=0
            eth-macro=(?P<eth_macro>\d+)\s+ #eth-macro=4
            lane=(?P<lane>\d+)\s+    #lane=0
            .*$
            '''
    pat = re.compile(pat_str, re.M | re.I | re.X)
    result = pat.finditer(config_content)
    data_dict={
        KEY_CLX_PORT_2_LOGIC_LANE: dict(),
        KEY_LOGIC_LANE_2_CLX_PORT: dict(),
        KEY_CPI_PORT_MAP_DATA: list()
    }
    for pre in result:
        unit = pre.group('unit')
        if unit is None:
            unit = '0'
        
        port = pre.group('port')
        if port in CPI_PORT_LIST:  # skip cpi ports
            data_dict[KEY_CPI_PORT_MAP_DATA].append(pre.group(0))
            continue
        
        eth_macro = pre.group('eth_macro')

        lane = pre.group('lane')

        logic_lane = macro_lane_2_logic_lane(eth_macro, lane)

        clx_port_tuple = (unit, port)

        clx_port_2_logic_lane = data_dict[KEY_CLX_PORT_2_LOGIC_LANE]
        if clx_port_tuple not in clx_port_2_logic_lane:
            clx_port_2_logic_lane[clx_port_tuple] = list()
        clx_port_2_logic_lane[clx_port_tuple].append(logic_lane)

        data_dict[KEY_LOGIC_LANE_2_CLX_PORT][logic_lane] = clx_port_tuple
    
    return data_dict


def _parse_phy_attr_from_file(config_file, clx_port_2_logic_lane):
    """
    parse pre-emphasis|lane-swap/polarity-rev of phy port from clx/dsh file
    :param config_file:
    :return:
    {
        'pre-emphasis': {
            # key: value
            (logic_lane, property): hex_value,
            cpi_port_info: [
                'phy set pre-emphasis portlist=129 lane-cnt=1 property=c2 data=0x01',
                'phy set pre-emphasis portlist=130 lane-cnt=1 property=c1 data=0x03'
            ]
        },
        'polarity-rev': {
            # key: value
            (logic_lane, property): hex_value,
            cpi_port_info: [
                'phy set polarity-rev portlist=129 lane-cnt=1 property=tx data=0x0'
                'phy set polarity-rev portlist=130 lane-cnt=1 property=tx data=0x0'
            ]
        }
        'lane-swap': {
            # key: value
            (logic_lane, property): hex_value
            cpi_port_info: [
                'phy set lane-swap portlist=129 lane-cnt=1 property=tx data=0x1',
                'phy set lane-swap portlist=130 lane-cnt=1 property=tx data=0x0'
            ]
        }
    }
    """
    # content of lane-swap  config file:
    # phy set lane-swap portlist=48-53 lane-cnt=4 property=tx data=0x03.02.01.00
    # phy set polarity-rev portlist=53 lane-cnt=4 property=rx data=0x0.0.0.0
    with open(config_file, mode='r') as f:
        config_content = f.read()

    pat_str = r'''^phy\s+set\s+(?P<phy_attr>lane-swap|polarity-rev|pre-emphasis)\s+     #pre-emphasis|phy set lane-swap/polarity-rev
            (unit=(?P<unit>\d+)\s+){0,1}   #unit=0 
            portlist=(?P<portlist>[\d\-,]+)\s+  #portlist=0
            lane-cnt=(?P<lane_cnt>\d+)\s+ #lane-cnt=4
            property=(?P<property>\w+)\s+    #property=tx/rx/c2/c1/c0/cn1
            data=(?P<data>[\w\.]+)   #data=0x01.01.01.01
            .*?$
            '''
    pat = re.compile(pat_str, re.M | re.I | re.X)
    result = pat.finditer(config_content)
    data_dict={
        KEY_PRE_EMPHASIS: dict(),
        KEY_LANE_SWAP: dict(),
        KEY_POLARITY_REV: dict()
    }
    for pre in result:
        phy_attr = pre.group('phy_attr')
        unit = pre.group('unit')
        if unit is None:
            unit = '0'
        portlist = pre.group('portlist')
        lane_cnt = pre.group('lane_cnt')
        property_data = pre.group('property')
        data = pre.group('data')
        data_hl = [hex(y)[2:] for y in [int(x, 16) for x in data.split('.')]]
        if int(lane_cnt) != len(data_hl):
            logger.error("lane count mismatch value : {}\n".format(pre.group(0)))
            continue

        for p in parse_portlist_range(portlist):
            # if CPI ports, just mark down and continue parsing next match
            if p in CPI_PORT_LIST:
                if phy_attr == 'pre-emphasis':
                    attr_value = data_dict[KEY_PRE_EMPHASIS]
                elif phy_attr == 'lane-swap':
                    attr_value = data_dict[KEY_LANE_SWAP]
                elif phy_attr == 'polarity-rev':
                    attr_value = data_dict[KEY_POLARITY_REV]
                if KEY_CPI_PORT_INFO not in attr_value:
                    attr_value[KEY_CPI_PORT_INFO] = list()
                attr_value[KEY_CPI_PORT_INFO].append(pre.group(0))
                break

            # get first logic_lane number from clx_port(unit,port)
            k = (unit, p)
            if k not in clx_port_2_logic_lane:
                logger.error("(unit {},port {}) is not in clx_port_2_logic_lane\n".format(unit, p))
                continue
            logic_lane_start = int(clx_port_2_logic_lane[k][0])

            # store all data with key (logic_lane, property)
            for index, logic_lane in enumerate(range(logic_lane_start, logic_lane_start + int(lane_cnt))):
                if phy_attr == 'pre-emphasis':
                    attr_value = data_dict[KEY_PRE_EMPHASIS]
                elif phy_attr == 'lane-swap':
                    attr_value = data_dict[KEY_LANE_SWAP]
                elif phy_attr == 'polarity-rev':
                    attr_value = data_dict[KEY_POLARITY_REV]
                k = (str(logic_lane), property_data)
                attr_value[k] = data_hl[index]

    return data_dict


def parse_clx_dsh(config_file):
    """
    :return:
    {
        'clx_port_2_logical_lane': {
            clx_port(unit,port): [logic_lane1, logic_lane2]
        },
        'logical_lane_2_clx_port': {
            logic_lane: clx_port(unit,port)
        },
        'cpi_port_map_data':[
            'init set port-map port=129 eth-macro=0 lane=1 max-speed=10g active=true guarantee=true cpi=true',
            'init set port-map port=130 eth-macro=0 lane=0 max-speed=10g active=true guarantee=true cpi=true init-done=true'
        ],
        'pre-emphasis': {
            # key: value
            (logic_lane, property): hex_value,
            cpi_port_info: [
                'phy set pre-emphasis portlist=129 lane-cnt=1 property=c2 data=0x01',
                'phy set pre-emphasis portlist=130 lane-cnt=1 property=c1 data=0x03'
            ]
        },
        'polarity-rev': {
            # key: value
            (logic_lane, property): hex_value,
            cpi_port_info: [
                'phy set polarity-rev portlist=129 lane-cnt=1 property=tx data=0x0'
                'phy set polarity-rev portlist=130 lane-cnt=1 property=tx data=0x0'
            ]
        },
        'lane-swap': {
            # key: value
            (logic_lane, property): hex_value
            cpi_port_info: [
                'phy set lane-swap portlist=129 lane-cnt=1 property=tx data=0x1',
                'phy set lane-swap portlist=130 lane-cnt=1 property=tx data=0x0'
            ]
        },
        'medium-type': {
            clx_port(unit,port): 'sr',
            cpi_port_info: [
                'port set property portlist=129-130 medium-type=kr'
            ]
        },
        'an': {
            clx_port(unit,port): 'enable',
            cpi_port_info: [
                'port set property portlist=129-130 an=enable'
            ]
        },
        'adver': {
            clx_port(unit,port): ['speed-100g','speed-25g'],
            cpi_port_info: [
                'port set adver portlist=129-130 speed-10g-kr'
            ]
        },
        'admin': {
            clx_port(unit,port): 'enable',
            cpi_port_info: [
                'port set property portlist=129-130 admin=enable'
            ]
        }
    }
    """
    data_dict = dict()
    data_port_map = _parse_port_map(config_file)
    data_phy_attribute = _parse_phy_attr_from_file(config_file, data_port_map[KEY_CLX_PORT_2_LOGIC_LANE])
    data_port_property = _parse_port_property_from_file(config_file)
    data_dict.update(data_port_map)
    data_dict.update(data_phy_attribute)
    data_dict.update(data_port_property)
    return data_dict


def _parse_port_property_from_file(config_file):
    """
    parse adver | medium-type | an | admin status of port from clx/dsh file
    
    :param : config_file
    :return:
    {
        'medium-type': {
            clx_port(unit,port): 'sr',
            cpi_port_info: [
                'port set property portlist=129-130 medium-type=kr'
            ]
        },
        'an': {
            clx_port(unit,port): 'enable',
            cpi_port_info: [
                'port set property portlist=129-130 an=enable'
            ]
        },
        'adver': {
            clx_port(unit,port): ['speed-100g','speed-25g'],
            cpi_port_info: [
                'port set adver portlist=129-130 speed-10g-kr'
            ]
        },
        'admin': {
            clx_port(unit,port): 'enable',
            cpi_port_info: [
                'port set property portlist=129-130 admin=enable'
            ]
        }
    }
    """
    # content of config file:

    # port set property portlist=129-130 medium-type=kr
    # port set adver portlist=129-130 speed-10g-kr
    # port set property portlist=129-130 an=enable
    # port set property portlist=129-130 admin=enable

    with open(config_file, mode='r') as f:
        config_content = f.read()

    data_dict={
        KEY_MEDIUM_TYPE: dict(),
        KEY_AN: dict(),
        KEY_ADMIN: dict(),
        KEY_ADVER: dict()
    }

    pat_str = r'''^port\s+set\s+property\s+     #port set property
            (unit=(?P<unit>\d+)\s+){0,1}   #unit=0 
            portlist=(?P<portlist>[\d\-,]+)\s+  #portlist=0
            (?P<property>medium-type|an|admin)=(?P<data>[\w]+)
            .*?$
            '''
    pat = re.compile(pat_str, re.M | re.I | re.X)
    result = pat.finditer(config_content)
    for pre in result:
        unit = pre.group('unit')
        if unit is None:
            unit = '0'
        portlist = pre.group('portlist')
        property_data = pre.group('property')
        data = pre.group('data')

        for p in parse_portlist_range(portlist):
            if property_data == 'medium-type':
                attr_value = data_dict[KEY_MEDIUM_TYPE]
            elif property_data == 'an':
                attr_value = data_dict[KEY_AN]
            elif property_data == 'admin':
                attr_value = data_dict[KEY_ADMIN]
            # CPI ports
            if p in CPI_PORT_LIST:               
                if KEY_CPI_PORT_INFO not in attr_value:
                    attr_value[KEY_CPI_PORT_INFO] = list()
                attr_value[KEY_CPI_PORT_INFO].append(pre.group(0))
                break
            # clx ports
            attr_value[(unit, p)] = data

    pat_str = r'''^port\s+set\s+adver\s+     #port set adver
            (unit=(?P<unit>\d+)\s+){0,1}   #unit=0 
            portlist=(?P<portlist>[\d\-,]+)\s+  #portlist=0
            (?P<data>.*?$)
            '''
    pat = re.compile(pat_str, re.M | re.I | re.X)
    result = pat.finditer(config_content)
    for pre in result:
        unit = pre.group('unit')
        if unit is None:
            unit = '0'
        portlist = pre.group('portlist')
        data = pre.group('data')

        for p in parse_portlist_range(portlist):
            attr_value = data_dict[KEY_ADVER]
            # CPI ports
            if p in CPI_PORT_LIST:               
                if KEY_CPI_PORT_INFO not in attr_value:
                    attr_value[KEY_CPI_PORT_INFO] = list()
                attr_value[KEY_CPI_PORT_INFO].append(pre.group(0))
                break
            # clx ports
            attr_value[(unit, p)] = data.split()

    return data_dict

def parse_portlist_range(portlist_str):
    portlist=list()
    if not portlist_str:
        return portlist
    # validation check
    invaild_chr = r'[^\d,-]'
    if re.search(invaild_chr, portlist_str):
        raise Exception('Invalid portlist range! {}'.format(portlist_str))
    for p_range in portlist_str.split(','):
        res = p_range.split('-')
        if len(res) == 1:
            portlist.extend(res)
        elif len(res) != 2:
            continue
        else:
            portlist.extend(
                str(x) for x in range(int(res[0]), int(res[1])+1)
            )
    # sort and filter duplicate item
    portlist = sorted(set(portlist),key=int)
    return portlist


def create_portlist_range(portlist):
    if len(portlist) == 0:
        return ''
    plist = sorted(set(portlist),key=int)  # filter duplicate item
    tmp_result = list()
    tmp_range = list()
    tmp_result.append(tmp_range)
    for index, p in enumerate(plist):
        if index == 0 :
            tmp_range.append(p)
        elif int(plist[index-1]) +1 == int(p):
            tmp_range.append(p)
        else:
            tmp_range = list()
            tmp_range.append(p)
            tmp_result.append(tmp_range)
    portlist_str = ','.join(
            x[0] if len(x)==1 else x[0] + '-' + x[-1] for x in tmp_result
        )
    return portlist_str


def logic_lane_2_macro_lane(logic_lane):
    logic_lane = int(logic_lane)
    return (str(logic_lane // 4), str(logic_lane % 4))


def macro_lane_2_logic_lane(macro, clx_lane):
    return str(int(macro)*4 + int(clx_lane))
