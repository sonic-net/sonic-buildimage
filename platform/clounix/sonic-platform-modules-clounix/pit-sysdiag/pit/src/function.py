import subprocess
from subprocess import check_output
import imp
import os
import json
import requests


PLATFORM_ROOT_PATH = "/usr/share/sonic/device"
PLATFORM_ROOT_PATH_DOCKER = "/usr/share/sonic/platform"

SONIC_CFGGEN_PATH = "/usr/local/bin/sonic-cfggen"
HWSKU_KEY = "DEVICE_METADATA['localhost']['hwsku']"
PLATFORM_KEY = "DEVICE_METADATA['localhost']['platform']"


def get_platform_and_hwsku():
    try:
        proc = subprocess.Popen([SONIC_CFGGEN_PATH, '-H', '-v', PLATFORM_KEY],
                                stdout=subprocess.PIPE,
                                shell=False,
                                stderr=subprocess.STDOUT)
        stdout = proc.communicate()[0].decode()
        proc.wait()
        platform = stdout.rstrip("\n")
        # platform = platform.lower().rstrip("-r0")

        proc = subprocess.Popen([SONIC_CFGGEN_PATH, '-d', '-v', HWSKU_KEY],
                                stdout=subprocess.PIPE,
                                shell=False,
                                stderr=subprocess.STDOUT)
        stdout = proc.communicate()[0].decode()
        proc.wait()
        hwsku = stdout.rstrip("\n")
    except OSError as e:
        raise OSError("Cannot detect platform")
    
    return (platform, hwsku)

def load_platform_util(module_name):
    platform_util_module = None
    
    (platform, hwsku) = get_platform_and_hwsku()
    platform_path = ''
    if len(platform) != 0:
        platform_path = "/".join([PLATFORM_ROOT_PATH, platform])
    else:
        platform_path = PLATFORM_ROOT_PATH_DOCKER
    
    hwsku_path = "/".join([platform_path, hwsku])
    
    try:
        module_file = "/".join([platform_path, "plugins", module_name + ".py"])
        platform_util_module = imp.load_source(module_name, module_file)
    except IOError as e:
        return None
    
    return platform_util_module

def load_platform_util_module(module_name):
    platform_util_module = None
    
    current_dir = os.path.dirname(os.path.abspath(__file__))
    (platform, hwsku) = get_platform_and_hwsku()
    platform_path = os.path.join(
                current_dir,
                "../config/platform",
                platform)    
    try:
        module_file = "/".join([platform_path, "plugins", module_name + ".py"])
        platform_util_module = imp.load_source(module_name, module_file)
    except IOError as e:
        return None
    
    return platform_util_module


def convert_unicode(input):
    if isinstance(input, dict):
        return {convert_unicode(key): convert_unicode(val) for key, val in input.items()}
    elif isinstance(input, str):
        return input.encode("utf-8")
    else:
        return input


def exec_cmd(cmd_str):
    output = None
    try:
        output = check_output(cmd_str, shell=True)
    except Exception as err:
        print("{} exec cmd failed.{}".format(cmd_str, err))
    return output


def run_command(cmd):
    proc = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    out, err = proc.communicate()
    if err and proc.returncode != 0:
        return proc.returncode, err
    return 0, out.decode().rstrip('\n')


def restful_command(cmd):
    url = "http://240.1.1.1:8080/api/hw/rawcmd"
    data = json.dumps({
        "Command" : cmd
    })
    
    for i in range(3):
        try:
            r = requests.post(url, data)
            r.url
            response_json = r.json()

            if "status" in response_json and response_json["status"] == "OK":
                ret = response_json["data"]["Outputs"]
                return True, ret
            else:
                return False, ""
        except Exception as e:
            print(e)
            continue
    return False, ""
