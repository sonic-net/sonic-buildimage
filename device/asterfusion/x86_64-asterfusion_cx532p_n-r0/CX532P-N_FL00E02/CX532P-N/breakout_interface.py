import ruamel.yaml
import os
import re
import sys
import json
import subprocess
import logging
from ruamel.yaml.scalarstring import DoubleQuotedScalarString as dq

DEVICE_CX532P_PATH = '/usr/share/sonic/device/x86_64-asterfusion_cx532p_n-r0/CX532P-N/'
CONFIG_DB_TMP_PATH = '/tmp/config_db.json'
CONFIG_DB_PATH = '/etc/sonic/config_db.json'
falcon_3T2_32x100_port_profile_PATH = DEVICE_CX532P_PATH + 'falcon_3T2_32x100_port_profile'
PLATFORM_INI_PATH = DEVICE_CX532P_PATH + 'platform.ini_bak'

MOD_FILE_ERR = -1

breakout_pors_speed = ['25000', '10000', '100000']

breakout_alias_prefix = {
    '25000': 'Y',
    '10000': 'X'
}


def move_file(dst_path, tmp_path):
    cmd = 'mv ' + tmp_path + ' ' + dst_path
    os.system(cmd)


def mod_config_db(dict_target, port_id, flag, speed='100000'):
    breakout_port_name_list = ['Ethernet' + str(port_id),
                               'Ethernet' + str(port_id + 1),
                               'Ethernet' + str(port_id + 2),
                               'Ethernet' + str(port_id + 3)
                               ]
    # PORT
    try:
        if 'PORT' in dict_target:
            if flag == 'True':
                cur_alias = dict_target['PORT'][breakout_port_name_list[0]]['alias']
                index = dict_target['PORT'][breakout_port_name_list[0]]['index']
                lanes = dict_target['PORT'][breakout_port_name_list[0]]['lanes'].split(",")
                fec = 'none' if speed == '10000' else 'rs'
                del dict_target['PORT'][breakout_port_name_list[0]]
                for key in range(0, 4):
                    alias = cur_alias + '_' + breakout_alias_prefix[speed] + str(key + 1)
                    new_dict = {
                        breakout_port_name_list[key]: {
                            "admin_status": 'up',
                            "alias": alias,
                            "fec": fec,
                            "index": index,
                            "lanes": lanes[key],
                            "mtu": '9216',
                            "speed": speed
                        }
                    }
                    dict_target['PORT'].update(new_dict)
            elif flag == 'False':
                cur_lane = int(dict_target['PORT'][breakout_port_name_list[0]]['lanes'])
                lanes = str(cur_lane) + ',' + str(cur_lane + 1) + ',' + str(cur_lane + 2) + ',' + str(cur_lane + 3)
                alias = dict_target['PORT'][breakout_port_name_list[0]]['alias'].split("_")[0]
                index = dict_target['PORT'][breakout_port_name_list[0]]['index']
                for key in range(0, 4):
                    del dict_target['PORT'][breakout_port_name_list[key]]
                new_dict = {
                    breakout_port_name_list[0]: {
                        "admin_status": 'up',
                        "alias": alias,
                        "fec": 'rs',
                        "index": index,
                        "lanes": lanes,
                        "mtu": '9216',
                        "speed": '100000'
                    }
                }
                dict_target['PORT'].update(new_dict)
    except Exception:
        return MOD_FILE_ERR

    # PORT_QOS_MAP
    try:
        if 'PORT_QOS_MAP' in dict_target:
            if flag == 'True':
                for key in breakout_port_name_list[1:]:
                    new_dict = {key: {
                        "tc_to_pg_map": "default",
                        "tc_to_queue_map": "default",
                        "dscp_to_tc_map": "default",
                        "pfc_enable": "3,4"
                    }
                    }
                    dict_target['PORT_QOS_MAP'].update(new_dict)
            elif flag == 'False':
                for key in breakout_port_name_list[1:]:
                    del dict_target['PORT_QOS_MAP'][key]
    except Exception:
        return MOD_FILE_ERR

        # CABLE_LENGTH
    try:
        if 'CABLE_LENGTH' in dict_target:
            for group in dict_target['CABLE_LENGTH']:
                if flag == 'True':
                    for key in breakout_port_name_list[1:]:
                        new_dict = {key: "40m"}
                        dict_target['CABLE_LENGTH'][group].update(new_dict)
                elif flag == 'False':
                    for key in breakout_port_name_list[1:]:
                        del dict_target['CABLE_LENGTH'][group][key]
    except Exception:
        return MOD_FILE_ERR

    # BUFFER_QUEUE
    try:
        if 'BUFFER_QUEUE' in dict_target:
            if flag == 'True':
                for queue in range(0, 8):
                    try:
                        del dict_target['BUFFER_QUEUE'][breakout_port_name_list[0] + '|' + str(queue)]
                    except Exception:
                        continue

                for interface in breakout_port_name_list:
                    # print('done')
                    for queue in range(0, 8):
                        if queue == 3 or queue == 4:
                            new_dict = {
                                interface + '|' + str(queue): {"profile": "egress_lossless_profile"}
                            }
                        else:
                            new_dict = {
                                interface + '|' + str(queue): {"profile": "egress_lossy_profile"}
                            }
                        dict_target['BUFFER_QUEUE'].update(new_dict)
            elif flag == 'False':
                for interface in breakout_port_name_list:
                    for queue in range(0, 8):
                        try:
                            del dict_target['BUFFER_QUEUE'][interface + '|' + str(queue)]
                        except Exception:
                            continue
                for queue in range(0, 8):
                    if queue == 3 or queue == 4:
                        new_dict = {
                            breakout_port_name_list[0] + '|' + str(queue): {
                                "profile": "egress_lossless_profile"
                            }
                        }
                    else:
                        new_dict = {
                            breakout_port_name_list[0] + '|' + str(queue): {
                                "profile": "egress_lossy_profile"
                            }
                        }
                    dict_target['BUFFER_QUEUE'].update(new_dict)
    except Exception:
        return MOD_FILE_ERR

        # BUFFER_PG
    try:
        if 'BUFFER_PG' in dict_target:
            if flag == 'True':
                for queue in range(0, 8):
                    try:
                        del dict_target['BUFFER_PG'][breakout_port_name_list[0] + '|' + str(queue)]
                    except Exception:
                        continue
                for interface in breakout_port_name_list:
                    for queue in range(0, 8):
                        if queue == 3 or queue == 4:
                            new_dict = {
                                interface + '|' + str(queue): {"profile": "ingress_lossless_profile"}
                            }
                        else:
                            new_dict = {
                                interface + '|' + str(queue): {"profile": "ingress_lossy_profile"}
                            }
                        dict_target['BUFFER_PG'].update(new_dict)
            elif flag == 'False':
                for interface in breakout_port_name_list:
                    for queue in range(0, 8):
                        del dict_target['BUFFER_PG'][interface + '|' + str(queue)]
                for queue in range(0, 8):
                    if queue == 3 or queue == 4:
                        new_dict = {
                            breakout_port_name_list[0] + '|' + str(queue): {
                                "profile": "ingress_lossless_profile"
                            }
                        }
                    else:
                        new_dict = {
                            breakout_port_name_list[0] + '|' + str(queue): {
                                "profile": "ingress_lossy_profile"
                            }
                        }
                    dict_target['BUFFER_PG'].update(new_dict)
    except Exception:
        return MOD_FILE_ERR

    '''
    # PORT_CHANNEL_MEMBER
    try:
        if 'PORTCHANNEL_MEMBER' in dict_target:
            for member in list(dict_target['PORTCHANNEL_MEMBER'].keys()):
                portname = (member.split('|'))[1]
                if portname in breakout_port_name_list:
                    del dict_target['PORTCHANNEL_MEMBER'][member]
                    logging.debug("PORTCHANNEL_MEMEBER.{} -> deleted".format(member))
            if len(dict_target['PORTCHANNEL_MEMBER']) == 0:
                del dict_target['PORTCHANNEL_MEMBER']
    except Exception:
        return MOD_FILE_ERR

    # VLAN
    try:
        if 'VLAN' in dict_target:
            for vlan in list(dict_target['VLAN'].keys()):
                plist = dict_target['VLAN'][vlan]['members'] if 'members' in dict_target['VLAN'][vlan] else []
                for interface in breakout_port_name_list:
                    if interface in plist:
                        dict_target['VLAN'][vlan]['members'].remove(interface)
    except Exception:
        return MOD_FILE_ERR

    # VLAN_MEMBER
    try:
        if 'VLAN_MEMBER' in dict_target:
            for member in list(dict_target['VLAN_MEMBER'].keys()):
                portname = (member.split('|'))[1]
                if portname in breakout_port_name_list:
                    del dict_target['VLAN_MEMBER'][member]
                    logging.debug("VLAN_MEMBER.{} -> deleted".format(member))
            if len(dict_target['VLAN_MEMBER']) == 0:
                del dict_target['VLAN_MEMBER']
    except Exception:
        return MOD_FILE_ERR

    # ACL_TABLE
    try:
        if 'ACL_TABLE' in dict_target:
            for acl_table in list(dict_target['ACL_TABLE'].keys()):
                if 'ports' in dict_target['ACL_TABLE'][acl_table]:
                    for breakout_port in breakout_port_name_list:
                        if breakout_port in dict_target['ACL_TABLE'][acl_table]['ports']:
                            dict_target['ACL_TABLE'][acl_table]['ports'].remove(breakout_port)
            #         if len(dict_target['ACL_TABLE'][acl_table]['ports']) == 0:
            #             del dict_target['ACL_TABLE'][acl_table]
            #             if 'ACL_RULE' in dict_target:
            #                 for acl_rule in list(dict_target['ACL_RULE'].keys()):
            #                     if (acl_rule.split('|'))[0] == acl_table:
            #                         del dict_target['ACL_RULE'][acl_rule]
            # if len(dict_target['ACL_TABLE']) == 0:
            #     del dict_target['ACL_TABLE']
            # if len(dict_target['ACL_RULE']) == 0:
            #     del dict_target['ACL_RULE']
    except Exception:
        return MOD_FILE_ERR

    # INTERFACE
    try:
        if 'INTERFACE' in dict_target:
            for intf in list(dict_target['INTERFACE'].keys()):
                portname = (intf.split('|'))[0]
                if portname in breakout_port_name_list:
                    del dict_target['INTERFACE'][intf]
                    logging.debug("INTERFACE.{} -> deleted".format(intf))
            if len(dict_target['INTERFACE']) == 0:
                del dict_target['INTERFACE']
    except Exception:
        return MOD_FILE_ERR
    '''

    # QUEUE
    try:
        if 'QUEUE' in dict_target:
            for queue in list(dict_target['QUEUE'].keys()):
                queue_interface = (queue.split('|'))[0]
                if queue_interface in breakout_port_name_list:
                    if 'scheduler' in dict_target['QUEUE'][queue]:
                        scheduler = dict_target['QUEUE'][queue]['scheduler'].split('|')[1]
                        scheduler = scheduler.split(']')[0]
                    del dict_target['QUEUE'][queue]
                    if 'SCHEDULER' in dict_target:
                        for key in list(dict_target['SCHEDULER'].keys()):
                            if key == scheduler:
                                del dict_target['SCHEDULER'][key]
                        if len(dict_target['SCHEDULER']) == 0:
                            del dict_target['SCHEDULER']
            if len(dict_target['QUEUE']) == 0:
                del dict_target['QUEUE']
    except Exception:
        return MOD_FILE_ERR

    return dict_target

target_port = sys.argv[1]
flag = sys.argv[2]
speed = '25000'
if len(sys.argv) == 4:
    speed = sys.argv[3]
    if speed not in breakout_pors_speed:
        sys.exit(1)
    # else:
    #     speed = '100000'

pattern = re.compile('Ethernet[0-9]{1,3}')
if pattern.search(target_port) is None:
    sys.exit(2)

port_id = int(target_port[8:])

# CONFIG_DB
with open(CONFIG_DB_PATH) as cfg:
    cur_cfg = json.load(cfg)


target_lane = cur_cfg['PORT'][target_port]['lanes'].split(",")
if flag == 'True' and len(target_lane) != 4:
    sys.exit(3)
if flag == 'False' and len(target_lane) != 1:
    sys.exit(4)

cur_cfg = mod_config_db(cur_cfg, port_id, flag, speed)
if cur_cfg == MOD_FILE_ERR:
    sys.exit(5)

with open(CONFIG_DB_TMP_PATH, 'w') as fp:
    json.dump(cur_cfg, fp, sort_keys=True, indent=4, separators=(',', ': '))
cfg.close()
fp.close()

port_mac_dict = {}
with open(falcon_3T2_32x100_port_profile_PATH, 'r') as To_C:
    for line in To_C.readlines():
        port_num = int(line.split()[1].strip())
        lane_num = int(line.split()[0].strip())
        if port_num >= 124:
            port_mac_dict[str(port_num)] = lane_num
        if not (port_num % 4) and port_num < 124:
            port_mac_dict[str(port_num)] = lane_num
            port_mac_dict[str(port_num + 1)] = lane_num + 1
            port_mac_dict[str(port_num + 2)] = lane_num + 2
            port_mac_dict[str(port_num + 3)] = lane_num + 3

port_profile_list = []

for key, value in cur_cfg['PORT'].items():
    #port_speed = int(int(value['speed']) / 100)
    if value['speed'] == '40000' or value['speed'] == '100000':
        port_speed = 1000
    else:
        port_speed = 250
    port_num = int(key[8:])
    if port_num <= 124:
        port_profile_list.append([port_mac_dict[str(port_num)], int(key[8:]), port_speed])
    else:
        port_profile_list.append([port_mac_dict[str(port_num + 3)], port_num + 3, 100])

port_profile_list.sort(key= lambda x:x[1])

with open(falcon_3T2_32x100_port_profile_PATH, 'w') as To_C:
    for line in port_profile_list:
        To_C.write(str(line[0]) + '    ' + str(line[1]) + '   ' + str(line[2]) + '\n')

To_C.close()

with open(PLATFORM_INI_PATH, 'r') as pt:
    platform_list = []
    for line in pt.readlines():
        l = line.split()
        platform_list.append(l)
    platform_list = platform_list[1:]
    for k, v in cur_cfg['PORT'].items():
        k = int(k[8:])
        if k < 60 and k % 4 == 0:
            if v['speed'] == '100000':
                platform_list[k][0], platform_list[k + 1][0], platform_list[k + 2][0], platform_list[k + 3][0] = (
                    str(k), str(k), str(k), str(k))
                platform_list[k][2], platform_list[k + 1][2], platform_list[k + 2][2], platform_list[k + 3][2] = (
                    '100000', '100000', '100000', '100000')
            if v['speed'] == '25000':
                platform_list[k][0], platform_list[k + 1][0], platform_list[k + 2][0], platform_list[k + 3][0] = (
                    str(k), str(k + 1), str(k + 2), str(k + 3))
                platform_list[k][2], platform_list[k + 1][2], platform_list[k + 2][2], platform_list[k + 3][2] = (
                    '25000', '25000', '25000', '25000')
        elif k >= 60 and k < 124 and k % 4 == 0:
            if v['speed'] == '100000':
                platform_list[k+4][0], platform_list[k + 5][0], platform_list[k + 6][0], platform_list[k + 7][0] = (
                    str(k + 4), str(k + 4), str(k + 4), str(k + 4))
                platform_list[k+4][2], platform_list[k + 5][2], platform_list[k + 6][2], platform_list[k + 7][2] = (
                    '100000', '100000', '100000', '100000')
            if v['speed'] == '25000':
                platform_list[k+4][0], platform_list[k + 5][0], platform_list[k + 6][0], platform_list[k + 7][0] = (
                    str(k + 4), str(k + 5), str(k + 6), str(k + 7))
                platform_list[k+4][2], platform_list[k + 5][2], platform_list[k + 6][2], platform_list[k + 7][2] = (
                    '25000', '25000', '25000', '25000')
        else:
            continue
    with open(DEVICE_CX532P_PATH + "platform.ini", 'w') as pt_tmp:
        pt_tmp.write("# Port  lane    speed   DC  BW  HF  LF  sqlch   minLf   maxLf   minHf   maxHf" + '\n')
        for d in platform_list:
            st = '    '.join(d)
            pt_tmp.write('  ' + st + '\n')
pt.close()
pt_tmp.close()

move_file(CONFIG_DB_PATH, CONFIG_DB_TMP_PATH)


