import ruamel.yaml
import os
import re
import sys
import json
import subprocess
import logging
from ruamel.yaml.scalarstring import DoubleQuotedScalarString as dq

DEVICE_CX564P_N_PATH = '/usr/share/sonic/device/x86_64-asterfusion_cx564p_n-r0/CX564P-N/'
CONFIG_DB_TMP_PATH = '/tmp/config_db.json'
CONFIG_DB_PATH = '/etc/sonic/config_db.json'
YAML_PATH = DEVICE_CX564P_N_PATH + 'config_64x100G_asterfusion-cx564p-n.yaml'
YAML_TMP_PATH = '/tmp/config_64x100G_asterfusion-cx564p-n.yaml'
MOD_FILE_ERR = -1

breakout_pors_speed = ['25000','10000','100000']

breakout_alias_prefix = {
                            '25000' : 'Y',
                            '10000' : 'X'
                        }


def move_file(dst_path, tmp_path):
    cmd = 'mv ' + tmp_path + ' ' + dst_path
    os.system(cmd)


def mod_config_db(dict_target, port_id, flag, speed='100000'):
    breakout_port_name_list = ['Ethernet' + str(port_id),
                               'Ethernet' + str(port_id+1), 
                               'Ethernet' + str(port_id+2), 
                               'Ethernet' + str(port_id+3)
                                ]
    #PORT
    try:
        if dict_target.has_key('PORT'):
            if flag == 'True':
                cur_alias = dict_target['PORT'][breakout_port_name_list[0]]['alias']
                index = dict_target['PORT'][breakout_port_name_list[0]]['index']
                lanes = dict_target['PORT'][breakout_port_name_list[0]]['lanes'].split(",")
                fec = 'none' if speed == '10000' else 'rs'
                del dict_target['PORT'][breakout_port_name_list[0]]
                for key in range(0,4):
                    alias = cur_alias + '_' + breakout_alias_prefix[speed] + str(key+1)
                    new_dict = {
                                breakout_port_name_list[key] : {
                                    "admin_status" : 'up',
                                    "alias" : alias,
                                    "fec" : fec,
                                    "index" : index,
                                    "lanes" : lanes[key],
                                    "mtu" : '9216',
                                    "speed" : speed
                                }
                    }
                    dict_target['PORT'].update(new_dict)
            elif flag == 'False':
                cur_lane = int(dict_target['PORT'][breakout_port_name_list[0]]['lanes'])
                lanes = str(cur_lane) + ',' + str(cur_lane+1) + ',' + str(cur_lane+2) + ',' + str(cur_lane+3)
                alias = dict_target['PORT'][breakout_port_name_list[0]]['alias'].split("_")[0]
                index = dict_target['PORT'][breakout_port_name_list[0]]['index']
                for key in range(0,4):
                    del dict_target['PORT'][breakout_port_name_list[key]]
                new_dict = {
                            breakout_port_name_list[0] : {
                                "admin_status" : 'up',
                                "alias" : alias,
                                "fec" : 'rs',
                                "index" : index,
                                "lanes" : lanes,
                                "mtu" : '9216',
                                "speed" : '100000'
                            }
                }
                dict_target['PORT'].update(new_dict)
    except Exception:
        return MOD_FILE_ERR

    #PORT_QOS_MAP
    try:
        if dict_target.has_key('PORT_QOS_MAP'):
            if flag == 'True':
                for key in breakout_port_name_list[1:]:
                    new_dict = { key :  {
                                "tc_to_pg_map": "[TC_TO_PRIORITY_GROUP_MAP:default]",
                                "tc_to_queue_map": "[TC_TO_QUEUE_MAP:default]",
                                "dscp_to_tc_map": "[DSCP_TO_TC_MAP:default]",
                                "pfc_enable": "3,4"
                                }
                            }
                    dict_target['PORT_QOS_MAP'].update(new_dict)
            elif flag == 'False':
                for key in breakout_port_name_list[1:]:
                    del dict_target['PORT_QOS_MAP'][key]
    except Exception:
        return MOD_FILE_ERR

    #CABLE_LENGTH
    try:
        if dict_target.has_key('CABLE_LENGTH'):
            for group in dict_target['CABLE_LENGTH']:
                if flag == 'True':
                    for key in breakout_port_name_list[1:]:
                        new_dict = { key : "40m" }
                        dict_target['CABLE_LENGTH'][group].update(new_dict)
                elif flag == 'False':
                    for key in breakout_port_name_list[1:]:
                        del dict_target['CABLE_LENGTH'][group][key]
    except Exception:
        return MOD_FILE_ERR

    #BUFFER_QUEUE
    try:
        if dict_target.has_key('BUFFER_QUEUE'):
            if flag == 'True':
                for queue in range(0,8):
                    del dict_target['BUFFER_QUEUE'][breakout_port_name_list[0]+'|'+str(queue)]
                for interface in breakout_port_name_list:
                    for queue in range(0,8):
                        if queue == 3 or queue == 4:
                            new_dict = {
                                interface+'|'+str(queue) : {"profile": "[BUFFER_PROFILE|egress_lossless_profile]"}
                            }
                        else:
                            new_dict = {
                                interface+'|'+str(queue) : {"profile": "[BUFFER_PROFILE|egress_lossy_profile]"}
                            }
                        dict_target['BUFFER_QUEUE'].update(new_dict)
            elif flag == 'False':
                for interface in breakout_port_name_list:
                    for queue in range(0,8):
                        del dict_target['BUFFER_QUEUE'][interface+'|'+str(queue)]
                for queue in range(0,8):
                    if queue == 3 or queue == 4:
                        new_dict = {
                            breakout_port_name_list[0]+'|'+str(queue) : {
                                "profile": "[BUFFER_PROFILE|egress_lossless_profile]"
                                }
                        }
                    else:
                        new_dict = {
                            breakout_port_name_list[0]+'|'+str(queue) : {
                                "profile": "[BUFFER_PROFILE|egress_lossy_profile]"
                                }
                        }
                    dict_target['BUFFER_QUEUE'].update(new_dict)
    except Exception:
        return MOD_FILE_ERR

    #BUFFER_PG
    try:
        if dict_target.has_key('BUFFER_PG'):
            if flag == 'True':
                for queue in range(0,8):
                    del dict_target['BUFFER_PG'][breakout_port_name_list[0]+'|'+str(queue)]
                for interface in breakout_port_name_list:
                    for queue in range(0,8):
                        if queue == 3 or queue == 4:
                            new_dict = {
                                interface+'|'+str(queue) : {"profile": "[BUFFER_PROFILE|ingress_lossless_profile]"}
                            }
                        else:
                            new_dict = {
                                interface+'|'+str(queue) : {"profile": "[BUFFER_PROFILE|ingress_lossy_profile]"}
                            }
                        dict_target['BUFFER_PG'].update(new_dict)
            elif flag == 'False':
                for interface in breakout_port_name_list:
                    for queue in range(0,8):
                        del dict_target['BUFFER_PG'][interface+'|'+str(queue)]
                for queue in range(0,8):
                    if queue == 3 or queue == 4:
                        new_dict = {
                            breakout_port_name_list[0]+'|'+str(queue) : {
                                "profile": "[BUFFER_PROFILE|ingress_lossless_profile]"
                                }
                        }
                    else:
                        new_dict = {
                            breakout_port_name_list[0]+'|'+str(queue) : {
                                "profile": "[BUFFER_PROFILE|ingress_lossy_profile]"
                                }
                        }
                    dict_target['BUFFER_PG'].update(new_dict)
    except Exception:
        return MOD_FILE_ERR

    #QUEUE
    try:
        if dict_target.has_key('QUEUE'):
            for queue in dict_target['QUEUE'].keys():
                queue_interface = (queue.split('|'))[0]
                if queue_interface in breakout_port_name_list:
                    if dict_target['QUEUE'][queue].has_key('scheduler'):
                        scheduler = dict_target['QUEUE'][queue]['scheduler'].split('|')[1]
                        scheduler = scheduler.split(']')[0]
                    del dict_target['QUEUE'][queue]
                    if dict_target.has_key('SCHEDULER'):
                        for key in dict_target['SCHEDULER'].keys():
                            if key == scheduler:
                                del dict_target['SCHEDULER'][key]
                        if len(dict_target['SCHEDULER']) == 0:
                            del dict_target['SCHEDULER']
            if len(dict_target['QUEUE']) == 0:
                del dict_target['QUEUE']
    except Exception:
        return MOD_FILE_ERR
 
    return dict_target

def mod_yaml(target_lane, flag):
    with open(YAML_PATH, 'r') as f:
        file_content = f.read()
    content = ruamel.yaml.round_trip_load(file_content, preserve_quotes=True)
    devports_yaml = content['nodes'][0]['devports']
    target_devports = []
    if flag == 'True':
        for lane in devports_yaml:
            if lane['id'] != target_lane:
                target_devports.append(lane)
            else:
                index = int(lane['id'])
                if lane['lanes'] == '0:4':
                    ret = 0
                elif lane['lanes'] == '4:4':
                    ret = 4
                port1 = {'fec': dq('KRFEC'), 'id': dq(index), 'lanes': dq(str(ret)+":1"), 'serdes_group': lane['serdes_group'], 'speed': dq("25G"), 'sysport': dq(index), 'type': dq("eth")}
                port2 = {'fec': dq('KRFEC'), 'id': dq(index+1), 'lanes': dq(str(ret+1)+':1'), 'serdes_group': lane['serdes_group'], 'speed': dq('25G'), 'sysport': dq(index+1), 'type': dq('eth')}
                port3 = {'fec': dq('KRFEC'), 'id': dq(index+2), 'lanes': dq(str(ret+2)+':1'), 'serdes_group': lane['serdes_group'], 'speed': dq('25G'), 'sysport': dq(index+2), 'type': dq('eth')}
                port4 = {'fec': dq('KRFEC'), 'id': dq(index+3), 'lanes': dq(str(ret+3)+':1'), 'serdes_group': lane['serdes_group'], 'speed': dq('25G'), 'sysport': dq(index+3), 'type': dq('eth')}
                target_devports.append(port1)
                target_devports.append(port2)
                target_devports.append(port3)
                target_devports.append(port4)
    elif flag == 'False':
        for lane in devports_yaml:
            if lane['id'] == target_lane:
                index = int(lane['id'])
                if lane['lanes'] == '0:1':
                    ret = 0
                elif lane['lanes'] == '4:1':
                    ret = 4
                port = {'fec': dq('KRFEC'), 'id': dq(index), 'lanes': dq(str(ret)+":4"), 'serdes_group': lane['serdes_group'], 'speed': dq("100G"), 'sysport': dq(index), 'type': dq("eth")}
                target_devports.append(port)
            elif int(lane['id']) > int(target_lane) and int(lane['id']) < (int(target_lane)+4) :
                continue
            else:
                target_devports.append(lane)
    del content['nodes'][0]['devports']
    content['nodes'][0]['devports'] = target_devports
    with open(YAML_TMP_PATH, "w") as f1:
#        ruamel.yaml.round_trip_dump(content,f1,explicit_start=True)
        ruamel.yaml.round_trip_dump(content,f1)
    f1.close()
    f.close()


target_port = sys.argv[1]
flag = sys.argv[2]
if len(sys.argv) == 4:
    speed = sys.argv[3]
    if speed not in breakout_pors_speed:
        sys.exit(1)
else:
    speed = '100000'

pattern = re.compile(r'Ethernet[0-9]{1,3}')
if pattern.search(target_port) is None:
    sys.exit(1)


port_id = int(target_port[8:])

#CONFIG_DB
with open(CONFIG_DB_PATH) as cfg:
    cur_cfg = json.load(cfg)

target_lane = cur_cfg['PORT'][target_port]['lanes'].split(",")
if flag == 'True' and len(target_lane) != 4:
    sys.exit(1)
if flag == 'False' and len(target_lane) != 1:
    sys.exit(1)

cur_cfg = mod_config_db(cur_cfg, port_id, flag, speed)
if cur_cfg == MOD_FILE_ERR:
    sys.exit(1)

with open(CONFIG_DB_TMP_PATH, 'w') as fp:
    json.dump(cur_cfg, fp, sort_keys=True, indent=4, separators=(',',': '))

cfg.close()
fp.close()
try:
    cur_yaml = mod_yaml(target_lane[0], flag)
except Exception:
    sys.exit(1)


#CONFIG_64X100G_ASTERFUSION-CX564P-N.YAML
move_file(CONFIG_DB_PATH, CONFIG_DB_TMP_PATH)
move_file(YAML_PATH, YAML_TMP_PATH)
