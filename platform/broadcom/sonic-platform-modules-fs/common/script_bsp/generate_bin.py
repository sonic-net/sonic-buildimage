#!/usr/bin/python3
# -*- coding: UTF-8 -*-
import os
import shutil

CACHE_FILE_PATH_SYSFS = "/sys/logic_dev/%s/cache_file_path"
MASK_FILE_PATH_SYSFS = "/sys/logic_dev/%s/mask_file_path"
I2C_CACHE_FILE_PATH_SYSFS = "/sys/bus/i2c/devices/%d-%04x/cache_file_path"
I2C_MASK_FILE_PATH_SYSFS = "/sys/bus/i2c/devices/%d-%04x/mask_file_path"
 
RAW_FILE_SYSFS = "/dev/%s"
I2C_RAW_FILE_SYSFS = "/sys/bus/i2c/devices/%d-%04x/%s"

G_BIN_CONFIG = []
G_I2C_DEV_CONFIG = []

def read_file(file_path):
    try:
        with open(file_path, 'r') as file:
            content = file.read()
        return True, content
    except FileNotFoundError:
        return False, f"file not find: {file_path}"
    except IOError as e:
        return False, f"read file err: {e}"

def parse_config(config, len):
    bits = [0] * len
    for item in config:
        cut_flag = 0
        if '-' in item:
            cut_flag = 1
            start, end = map(str, item.split('-'))
        elif '_' in item:
            cut_flag = 1
            start, end = map(str, item.split('_'))
        elif '~' in item:
            cut_flag = 1
            start, end = map(str, item.split('~'))
        else:
            bits[int(item, 16)] = 1

        if cut_flag:
            if start.startswith('0x'):
                start = int(start, 16)
            else:
                start = int(start)
            if end.startswith('0x'):
                end = int(end, 16)
            else:
                end = int(end)

            for i in range(start, end + 1):
                bits[i] = 1
    return bits

def generate_binary_file(mask_bits, raw_file, mask_file, cache_file):
    shutil.copyfile(raw_file, cache_file)
    with open(mask_file, 'wb') as f:
        f.write(bytes(mask_bits))

def run():
    for config in G_BIN_CONFIG:
        name = config.get("name")
        if not name:
            continue
        lens = config.get("lens", 256)
        bit = config.get("offset", [])
        bits = parse_config(bit, lens)
        mask_file_sysfs = MASK_FILE_PATH_SYSFS % name
        cache_file_sysfs = CACHE_FILE_PATH_SYSFS % name
        ret, mask_file = read_file(mask_file_sysfs)
        if ret is False:
            print("read_file %s fail, reson: %s" % (mask_file_sysfs, mask_file))
            continue
        ret, cache_file = read_file(cache_file_sysfs)
        if ret is False:
            print("read_file %s fail, reson: %s" % (cache_file_sysfs, cache_file))
            continue
        raw_file = RAW_FILE_SYSFS % name
        mask_file = mask_file.strip()
        cache_file = cache_file.strip()

        generate_binary_file(bits, raw_file, mask_file, cache_file)
        print("generate %s debug bin file success, cache_file: %s, mask_file: %s" % (name, cache_file, mask_file))
    for config in G_I2C_DEV_CONFIG:
        name = config.get("name")
        lens = config.get("lens", 256)
        bit = config.get("offset", [])
        bus = config.get("bus")
        addr = config.get("addr")
        raw_sysfs_name = config.get("raw_sysfs", "eeprom")
        if bus is None or addr is None:
            print("%s config err, please check" % (name))
            continue
        bits = parse_config(bit, lens)
        mask_file_sysfs = I2C_MASK_FILE_PATH_SYSFS % (bus, addr)
        cache_file_sysfs = I2C_CACHE_FILE_PATH_SYSFS % (bus, addr)
        ret, mask_file = read_file(mask_file_sysfs)
        if ret is False:
            print("read_file %s fail, reson: %s" % (mask_file_sysfs, mask_file))
            continue
        ret, cache_file = read_file(cache_file_sysfs)
        if ret is False:
            print("read_file %s fail, reson: %s" % (cache_file_sysfs, cache_file))
            continue
        raw_file = I2C_RAW_FILE_SYSFS % (bus, addr, raw_sysfs_name)
        mask_file = mask_file.strip()
        cache_file = cache_file.strip()

        generate_binary_file(bits, raw_file, mask_file, cache_file)
        print("generate %s debug bin file success, cache_file: %s, mask_file: %s" % (name, cache_file, mask_file))

if __name__ == "__main__":
    if os.path.exists("/usr/local/bin/generate_bin_config.py"):
        module_product = __import__("generate_bin_config", globals(), locals(), [], 0)
        if hasattr(module_product, "GENERATE_MASK_CONFIG"):
            G_BIN_CONFIG = module_product.GENERATE_MASK_CONFIG
        if hasattr(module_product, "GENERATE_I2C_DEV_CONFIG"):
            G_I2C_DEV_CONFIG = module_product.GENERATE_I2C_DEV_CONFIG
    if len(G_BIN_CONFIG) == 0 and len(G_I2C_DEV_CONFIG) == 0:
        print("read config fail, please check file:/usr/local/bin/generate_bin_config.py")
    else:
        run()
