#!/usr/bin/env python3

import os
import sys 

import yaml
from sonic_py_common.logger import Logger
from swsscommon.swsscommon import ConfigDBConnector

db = ConfigDBConnector()
db.connect()


SYSLOG_IDENTIFIER = 'snmp_yml_to_configdb.py'
logger = Logger(SYSLOG_IDENTIFIER)
logger.set_min_log_priority_info()

snmp_comm_config_db = db.get_table('SNMP_COMMUNITY')
snmp_config_db_communities = snmp_comm_config_db.keys()
snmp_general_config_db = db.get_table('SNMP')
snmp_general_keys = snmp_general_config_db.keys()

full_snmp_comm_list = ['snmp_rocommunity', 'snmp_rocommunities', 'snmp_rwcommunity', 'snmp_rwcommunities']

if not os.path.exists('/etc/sonic/snmp.yml'):
    logger.log_info('/etc/sonic/snmp.yml does not exist')
    sys.exit(1)

with open('/etc/sonic/snmp.yml', 'r') as yaml_file:
    yaml_snmp_info = yaml.load(yaml_file, Loader=yaml.FullLoader)

for comm_type in full_snmp_comm_list:
    if comm_type in yaml_snmp_info.keys():
        if comm_type.startswith('snmp_rocommunities'):
            for community in yaml_snmp_info[comm_type]:
                if community not in snmp_config_db_communities:
                    db.set_entry('SNMP_COMMUNITY', community, {"TYPE": "RO"})
        elif comm_type.startswith('snmp_rocommunity'):
            community = yaml_snmp_info['snmp_rocommunity']
            if community not in snmp_config_db_communities:
                db.set_entry('SNMP_COMMUNITY', community, {"TYPE": "RO"})
        elif comm_type.startswith('snmp_rwcommunities'):
            for community in yaml_snmp_info[comm_type]:
                if community not in snmp_config_db_communities:
                    db.set_entry('SNMP_COMMUNITY', community, {"TYPE": "RW"})
        elif comm_type.startswith('snmp_rwcommunity'):
            community = yaml_snmp_info['snmp_rwcommunity']
            if community not in snmp_config_db_communities:
                db.set_entry('SNMP_COMMUNITY', community, {"TYPE": "RW"})

if yaml_snmp_info.get('snmp_location'):
    if 'LOCATION' not in snmp_general_keys:
        db.set_entry('SNMP', 'LOCATION', {'Location': yaml_snmp_info['snmp_location']})
else:
    logger.log_info('snmp_location does not exist in snmp.yml file')
    sys.exit(1)
