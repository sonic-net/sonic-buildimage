#!/usr/bin/python
# -*- coding: UTF-8 -*-
import json
import os
import subprocess

if __name__ == '__main__':
    subprocess.call(["sudo", "rm", "-rf", "/sys_switch"])
    subprocess.call(["sudo", "mkdir", "-p", "-m", "777", "/sys_switch"])

    with open('/etc/s3ip/s3ip_sysfs_conf.json', 'r') as jsonfile:
        json_string = json.load(jsonfile)
        for s3ip_sysfs_path in json_string['s3ip_syfs_paths']:
            if s3ip_sysfs_path['type'] == "string":
                (path, file) = os.path.split(s3ip_sysfs_path['path'])
                subprocess.call(["sudo", "mkdir", "-p", "-m", "777", path])
                with open(s3ip_sysfs_path['path'], "w") as f:
                    f.write(s3ip_sysfs_path['value'])
            elif s3ip_sysfs_path['type'] == "path":
                subprocess.call(["sudo", "ln", "-s", s3ip_sysfs_path['value'], s3ip_sysfs_path['path']])
            else:
                print('error type:' + s3ip_sysfs_path['type'])
        subprocess.call(["tree", "-l", "/sys_switch"])
