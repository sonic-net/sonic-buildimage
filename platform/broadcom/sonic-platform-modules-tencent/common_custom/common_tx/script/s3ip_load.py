#!/usr/bin/python3
import json
import os

if __name__ == '__main__':
    os.system("sudo rm -rf /sys_switch/*;sudo mkdir -p -m 777 /sys_switch")

    with open('/etc/s3ip/customer_sysfs.json', 'r') as jsonfile:
        json_string = json.load(jsonfile)
        for s3ip_sysfs_path in json_string['s3ip_syfs_paths']:
            #print('path:' + s3ip_sysfs_path['path'])
            #print('type:' + s3ip_sysfs_path['type'])
            #print('value:' + s3ip_sysfs_path['value'])

            if s3ip_sysfs_path['type'] == "string" :
                (path, file) = os.path.split(s3ip_sysfs_path['path'])
                command = "sudo mkdir -p -m 777 " + path
                #print(command)
                os.system(command)
                command = "sudo echo " +  "\"" + s3ip_sysfs_path['value'] + "\"" + " > " + s3ip_sysfs_path['path']
                #print(command)
                os.system(command)
            elif s3ip_sysfs_path['type'] == "path" :
                command = "sudo ln -s " + s3ip_sysfs_path['value'] + " " + s3ip_sysfs_path['path']
                #print(command)
                os.system(command)
            else:
                print('error type:' + s3ip_sysfs_path['type'])
        #os.system("tree -l /sys_switch")

