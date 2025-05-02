#!/usr/bin/env python3
#
# Copyright (c) 2024 NVIDIA CORPORATION & AFFILIATES.
# Apache-2.0
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

import shutil
import click
import re
import os
import subprocess
import glob
from pathlib import Path


class CMISHostMgmtActivator:
    PARAMS = {
        "sai_profile": {
            "file_name": "sai.profile",
            "enabled_param": "SAI_INDEPENDENT_MODULE_MODE=1",
            "disabled_param": "SAI_INDEPENDENT_MODULE_MODE=0"
        },
        "pmon_daemon_control": {
            "file_name": "pmon_daemon_control.json",
            "enabled_param": "\"skip_xcvrd_cmis_mgr\": false",
            "disabled_param": "\"skip_xcvrd_cmis_mgr\": true",
        },
        "sai_xml": {
            "file_name": "sai_<>.xml", # will be filled at main, since we can't know the SKU here 
            "enabled_param": "<late-create-all-ports>1</late-create-all-ports>",
            "disabled_param": "<late-create-all-ports>1</late-create-all-ports>" # Shouldn't be called
        }
    }

    @staticmethod
    def change_param(param, path, action):
        file_path = '{}/{}'.format(path, CMISHostMgmtActivator.PARAMS[param]["file_name"])
        lines = None

        try:
            with open(file_path, 'r') as param_file:
                lines = param_file.read()

                if lines:
                    if action == "disable":
                        lines = re.sub(CMISHostMgmtActivator.PARAMS[param]["enabled_param"],
                                    CMISHostMgmtActivator.PARAMS[param]["disabled_param"],
                                    lines)
                    elif action == "enable":
                        if param == "sai_profile" and not re.search(CMISHostMgmtActivator.PARAMS[param]["disabled_param"], lines):
                            if not re.search(CMISHostMgmtActivator.PARAMS[param]["enabled_param"], lines): 
                                with open(file_path, 'a') as param_file:
                                    param_file.write(CMISHostMgmtActivator.PARAMS[param]["enabled_param"] + '\n')
                                return

                        lines = re.sub(CMISHostMgmtActivator.PARAMS[param]["disabled_param"],
                                    CMISHostMgmtActivator.PARAMS[param]["enabled_param"],
                                    lines)

            with open(file_path, 'w') as param_file:
                param_file.write(lines)

        except FileNotFoundError as e:
            print('Missing file: {}'.format(e.filename))


    @staticmethod
    def parse_show_platform_summary():
        summary = subprocess.check_output(['show', 'platform', 'summary'])
        summary = summary.decode('utf-8')
        summary = [x for x in summary.split('\n') if x]

        for field in summary:
            key, value = field.split(": ")
            
            if key == 'Platform':
                platform = value

            elif key == 'HwSKU':
                sku = value

        return platform, sku


    @staticmethod
    def remove_file(file_path):
        if os.path.isfile(file_path):
            os.remove(file_path)


    @staticmethod
    def copy_file(src_path, dest_path):
        if os.path.isfile(src_path):
            shutil.copy(src_path, dest_path)


    @staticmethod
    def is_spc_supported(spc):
        return int(spc) >= 4000

    @staticmethod
    def disable():
        platform, sku = CMISHostMgmtActivator.parse_show_platform_summary()
        sku_path = '/usr/share/sonic/device/{0}/{1}'.format(platform, sku)
        platform_path = '/usr/share/sonic/device/{0}'.format(platform)
        CMISHostMgmtActivator.change_param("sai_profile", sku_path, 'disable')
        
        if os.path.isfile('{0}/{1}'.format(platform_path, 'pmon_daemon_control.json')):
            CMISHostMgmtActivator.change_param("pmon_daemon_control", platform_path, 'disable')
            CMISHostMgmtActivator.remove_file('{0}/{1}'.format(sku_path, 'pmon_daemon_control.json'))
        else:
            CMISHostMgmtActivator.change_param("pmon_daemon_control", sku_path, 'disable')

        CMISHostMgmtActivator.remove_file('{0}/{1}'.format(sku_path, 'media_settings.json'))
        CMISHostMgmtActivator.remove_file('{0}/{1}'.format(sku_path,'optics_si_settings.json'))
        CMISHostMgmtActivator.remove_file('{0}/{1}'.format(platform_path, 'media_settings.json'))
        CMISHostMgmtActivator.remove_file('{0}/{1}'.format(platform_path, 'optics_si_settings.json'))


    @staticmethod
    def enable(args):
        platform, sku = CMISHostMgmtActivator.parse_show_platform_summary()
        sku_path = '/usr/share/sonic/device/{0}/{1}'.format(platform, sku)
        platform_path = '/usr/share/sonic/device/{0}'.format(platform)

        sku_num = re.search('[0-9]{4}', sku).group()

        if not CMISHostMgmtActivator.is_spc_supported(sku_num):
            print("Error: unsupported platform - feature is supported on SPC3 and higher.")

        sai_profile_file = '{}/{}'.format(sku_path, CMISHostMgmtActivator.PARAMS["sai_profile"]["file_name"])
        lines = None
        with open(sai_profile_file, 'r') as saiprofile:
            lines = saiprofile.read()
        
        sai_xml_path = re.search("SAI_INIT_CONFIG_FILE.*", lines).group()

        if sai_xml_path:
            sai_xml_name = Path(sai_xml_path).name
            CMISHostMgmtActivator.PARAMS["sai_xml"]["file_name"] = sai_xml_name
        else:
            print("Error: no sai_*.xml file present")

        CMISHostMgmtActivator.copy_file(args[0], sku_path)
        CMISHostMgmtActivator.copy_file(args[1], sku_path)
        CMISHostMgmtActivator.copy_file('{0}/{1}'.format(platform_path, 'pmon_daemon_control.json'), sku_path)

        CMISHostMgmtActivator.change_param("sai_profile", sku_path, 'enable')
        CMISHostMgmtActivator.change_param("pmon_daemon_control", sku_path, 'enable')
        CMISHostMgmtActivator.change_param("sai_xml", sku_path, 'enable')
        

@click.command()
@click.option('--disable', is_flag=True, help='Disable CMIS Host Management')
@click.option('--enable', nargs=2, type=click.Path(), help='Enable CMIS Host Management, receives two arguments: media_settings.json path, and optics_si_settings.json path')
def main(disable, enable):

    if disable and enable:
        print("Error: can't use both options, please choose one.")
        return

    if disable:
        CMISHostMgmtActivator.disable()

    elif enable:
        CMISHostMgmtActivator.enable(enable)

    else:
        print("Error: no option was provided - nothing to execute.")

if __name__ == '__main__':
    main()
