#!/usr/bin/env python3

import json
import subprocess

INIT_CFG_FILE_PATH = '/etc/sonic/init_cfg.json'
WARM_OR_FAST_BOOT_DATAPLANE_NEEDED_SERVICES = ['database', 'swss', 'syncd', 'teamd', 'bgp']

with open(INIT_CFG_FILE_PATH) as init_cfg_file:
    init_cfg = json.load(init_cfg_file)
    if 'FEATURE' in init_cfg:
        for feature_name, feature_props in init_cfg['FEATURE'].items():
            # For warm/fast boot we want to have all crtical dataplane needed service 
            # to start immediately before hostcfgd can render `state` field unless the `state` field is marked disabled
            # explicitly during build time rendering of init_cfg.json
            if feature_name in WARM_OR_FAST_BOOT_DATAPLANE_NEEDED_SERVICES:
                if 'state' in feature_props and (feature_props['state'] == 'disabled' or feature_props['state'] == 'always_disabled'):
                    subprocess.run(['systemctl', 'mask', '{}.service'.format(feature_name)])
            # For other services by default mask out the service if not enable explicitly. This service can get enable later on when 
            # hostcfgd render the state as enable. This should not cause dataplane impact.
            else:
                if 'state' in feature_props and feature_props['state'] != 'enabled' and feature_props['state'] != 'always_enabled':
                    subprocess.run(['systemctl', 'mask', '{}.service'.format(feature_name)])
