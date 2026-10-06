#!/usr/bin/env python3

import os
import sys

import yaml
from sonic_py_common.logger import Logger
from swsscommon.swsscommon import ConfigDBConnector

SYSLOG_IDENTIFIER = 'snmp_yml_to_configdb.py'
logger = Logger(SYSLOG_IDENTIFIER)
logger.set_min_log_priority_info()

SNMP_YML_PATH = '/etc/sonic/snmp.yml'

FULL_SNMP_COMM_LIST = ['snmp_rocommunity', 'snmp_rocommunities', 'snmp_rwcommunity', 'snmp_rwcommunities']

INSECURE_DEFAULT_COMMUNITIES = {'public', 'private'}


def _canonicalize_for_netsnmp_comparison(community):
    """Return the first community token as Net-SNMP parses it."""
    if len(community) > 1 and community[0] in ('"', "'"):
        quote = community[0]
        closing_quote = community.find(quote, 1)
        if closing_quote != -1:
            community = community[1:closing_quote]

    return community.replace('\\', '')


def _is_insecure_default(community):
    if _canonicalize_for_netsnmp_comparison(community) in INSECURE_DEFAULT_COMMUNITIES:
        logger.log_warning("Ignoring insecure default snmp.yml community '{}'".format(community))
        return True
    return False


def load_snmp_yaml(path):
    """Read and parse the ZTP-provided snmp.yml file.

    Uses yaml.safe_load (rather than yaml.FullLoader) so that a malicious or
    corrupted file cannot cause construction of arbitrary Python objects via
    YAML type tags (e.g. !!python/object/apply). safe_load only ever
    produces plain dict/list/str/int/float/bool/None values, which is all
    this file's schema (SNMP community/location scalars and lists) needs.
    """
    if not os.path.exists(path):
        logger.log_info('{} does not exist'.format(path))
        return None

    try:
        with open(path, 'r') as yaml_file:
            yaml_snmp_info = yaml.safe_load(yaml_file)
    except yaml.YAMLError as e:
        logger.log_error('Failed to parse YAML file {}: {}'.format(path, e))
        return None

    # Normalize an empty file's None result to {} so it hits
    # apply_snmp_location's specific log instead of looking like a missing file.
    return yaml_snmp_info if yaml_snmp_info is not None else {}


def apply_snmp_communities(db, yaml_snmp_info, snmp_config_db_communities):
    """Copy SNMP community configuration from snmp.yml into SNMP_COMMUNITY."""
    for comm_type in FULL_SNMP_COMM_LIST:
        if comm_type not in yaml_snmp_info.keys():
            continue

        if comm_type.startswith('snmp_rocommunities'):
            for community in yaml_snmp_info[comm_type]:
                if community not in snmp_config_db_communities and not _is_insecure_default(community):
                    db.set_entry('SNMP_COMMUNITY', community, {"TYPE": "RO"})
        elif comm_type.startswith('snmp_rocommunity'):
            community = yaml_snmp_info['snmp_rocommunity']
            if community not in snmp_config_db_communities and not _is_insecure_default(community):
                db.set_entry('SNMP_COMMUNITY', community, {"TYPE": "RO"})
        elif comm_type.startswith('snmp_rwcommunities'):
            for community in yaml_snmp_info[comm_type]:
                if community not in snmp_config_db_communities and not _is_insecure_default(community):
                    db.set_entry('SNMP_COMMUNITY', community, {"TYPE": "RW"})
        elif comm_type.startswith('snmp_rwcommunity'):
            community = yaml_snmp_info['snmp_rwcommunity']
            if community not in snmp_config_db_communities and not _is_insecure_default(community):
                db.set_entry('SNMP_COMMUNITY', community, {"TYPE": "RW"})


def apply_snmp_location(db, yaml_snmp_info, snmp_general_keys):
    """Copy snmp_location from snmp.yml into the SNMP|LOCATION entry.

    Returns True if a location was found (whether or not it was written,
    e.g. because LOCATION already exists), False if snmp_location was
    missing from the file -- matching the original script's exit(1) trigger.
    """
    if yaml_snmp_info.get('snmp_location'):
        if 'LOCATION' not in snmp_general_keys:
            db.set_entry('SNMP', 'LOCATION', {'Location': yaml_snmp_info['snmp_location']})
        return True

    logger.log_info('snmp_location does not exist in snmp.yml file')
    return False


def main():
    db = ConfigDBConnector()
    db.connect()

    snmp_comm_config_db = db.get_table('SNMP_COMMUNITY')
    snmp_config_db_communities = snmp_comm_config_db.keys()
    snmp_general_config_db = db.get_table('SNMP')
    snmp_general_keys = snmp_general_config_db.keys()

    yaml_snmp_info = load_snmp_yaml(SNMP_YML_PATH)
    if yaml_snmp_info is None:
        sys.exit(1)

    apply_snmp_communities(db, yaml_snmp_info, snmp_config_db_communities)

    if not apply_snmp_location(db, yaml_snmp_info, snmp_general_keys):
        sys.exit(1)


if __name__ == '__main__':
    main()
