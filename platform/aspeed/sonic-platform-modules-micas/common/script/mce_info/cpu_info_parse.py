#!/usr/bin/env python_nos
# -*- coding: UTF-8 -*-

import sys
import os
import time
import syslog
import json
import logging
import traceback
import fcntl
import copy

import cpu_info_collect
import event_msg_queue
from platform_util import check_value, get_value, set_value, exec_os_cmd


logger = logging.getLogger(__name__)
logging.basicConfig(level=logging.INFO, format='%(asctime)s [%(levelname)s] %(message)s')


g_regs_info = []


class RegInfo():
    def __init__(self):
        self.parent_reg_info = None
        self.reg_desc = None
        self.reg_addr = None
        self.reg_type = None
        self.reg_val  = None
        self.reg_expected_val = None
        self.reg_bit_domain_desc = None


class CpuRegInfoParse():
    def __init__(self, event):
        self.cpu_reg_info_cfg = {}
        self.event = event

    def load_json_file(self):
        file = f"/tmp/cpu_reg_info_collect_{self.event}.json"
        if not os.path.exists(file):
            logger.info(f"{file} is not exist")
            return -1

        with open(file, "r", encoding='utf-8') as f:
            try:
                fcntl.flock(f.fileno(), fcntl.LOCK_SH)
                self.cpu_reg_info_cfg = json.load(f)
            except Exception as e:
                logger.error(f"Failed to load json file {file}: {e}")
                return -2
            finally:
                fcntl.flock(f.fileno(), fcntl.LOCK_UN)

            logger.info(f"load {file} ok")
            return 0



    def _init_and_add_reg_info(self, cpu_reg_info, parent_reg_info, i):
        reg_info = RegInfo()
        try:
            reg_info.reg_desc = cpu_reg_info.get("desc", [None])[i]
            reg_info.reg_type = cpu_reg_info.get("reg_type")
            reg_info.reg_addr = copy.deepcopy(cpu_reg_info.get("reg_addr", {}))
            if reg_info.reg_addr:
                base = reg_info.reg_addr.get("addr_range", {}).get("base", 0)
                width = reg_info.reg_addr.get("width", 0)
                reg_info.reg_addr.setdefault("addr_range", {})
                reg_info.reg_addr["addr_range"]["base"] = base + width * i
                reg_info.reg_addr["addr_range"]["len"] = width
            reg_info.reg_val = None
            get_val_list = cpu_reg_info.get("get_val", [])
            if len(get_val_list) > i:
                reg_info.reg_val = get_val_list[i]
            ok_val_list = cpu_reg_info.get("ok_val")
            if ok_val_list and len(ok_val_list) > i:
                reg_info.reg_expected_val = ok_val_list[i]
            reg_bit_domain_parse_info_list = cpu_reg_info.get("reg_bit_domain_parse_info", [])
            if len(reg_bit_domain_parse_info_list) > i:
                reg_info.reg_bit_domain_desc = reg_bit_domain_parse_info_list[i]
            reg_info.parent_reg_info = parent_reg_info
            g_regs_info.append(reg_info)
            return reg_info
        except Exception as e:
            logger.error(f"Failed to init reg info at index {i}: {e}")
            logger.debug(traceback.format_exc())
            return None

    def _reg_bit_domain_spread_parse(self, reg_bit_domain_spread, xor_val, get_val, parent_reg_info):
        try:
            # Traverse the bit field to parse the register information to be processed
            for cfg in reg_bit_domain_spread:
                # The bit positions related to the mask are not erroneous. Skip the parsing.
                mask = cfg.get("mask")
                if mask is None:
                    logger.warning("reg_bit_domain_spread entry missing 'mask', skipping")
                    continue
                logger.debug(f"  mask: 0x{mask:x}")
                if xor_val & mask == 0:
                    logger.debug("  mask bits no error, skip")
                    continue
                
                # The value of the bit related to the mask does not match the expected value. Skip the parsing.
                val = cfg.get("val")
                if val is None:
                    logger.warning("reg_bit_domain_spread entry missing 'val', skipping")
                    continue
                logger.debug(f"  val: 0x{val:x}")
                if (get_val & mask) != val:
                    logger.debug("  val not match, skip")
                    continue


                # The mask-related bit positions and values are all matched. Traverse the register group to be parsed
                regs_arr = cfg.get("regs_arr")
                if regs_arr is None:
                    logger.warning("regs_arr is None, skipping this config")
                    return -1

                for reg in regs_arr.values():
                    get_val_list = reg.get("get_val")
                    if not get_val_list:
                        logger.warning(f"reg entry missing 'get_val', skipping")
                        continue
                    for i in range(len(get_val_list)):
                        res = self._init_and_add_reg_info(reg, parent_reg_info, i)
                        if res is None:
                            logger.error("Failed to init reg info during reg_bit_domain_spread_parse")
                            return -3
            return 0
        except Exception as e:
            logger.error(f"_reg_bit_domain_spread_parse exception: {e}")
            logger.debug(traceback.format_exc())
            return -3

    def get_cpu_reg_parse_info(self):
        try:
            # Traverse the entire register information
            index = 0
            for cpu_reg_info in self.cpu_reg_info_cfg.values():
                # If it is not a leaf node, it indicates that it is a primary state register. Based on which bits of the primary state register are incorrect, 
                # the information collection of the register group that expands its bit field can be carried out.
                is_leaf_node = cpu_reg_info.get("is_leaf_node", True)
                logger.info(f"index{index}: is_leaf_node: {is_leaf_node}")
                if not is_leaf_node:
                    get_val_list = cpu_reg_info.get("get_val", [])
                    ok_val_list = cpu_reg_info.get("ok_val", [])
                    if not get_val_list or not ok_val_list:
                        logger.warning(f"index{index}: missing get_val or ok_val, skipping")
                        index += 1
                        continue
                    xor_val = get_val_list[0] ^ ok_val_list[0]
                    logger.info(f"index{index}: get_val:0x{get_val_list[0]:x}, ok_val:0x{ok_val_list[0]:x}, xor_val: 0x{xor_val:x}")
                    # The first-level overall status register indicates no error. Therefore, all the register information at this level does not need to be parsed.
                    if xor_val == 0:
                        logger.info("no error detected, skip reg_bit_domain_spread_parse")
                        index += 1
                        continue

                    # There is an error. The register information of the overall state of that level serves as the parent register information.
                    parent_reg_info = self._init_and_add_reg_info(cpu_reg_info, None, 0)
                    if parent_reg_info is None:
                        logger.error(f"Failed to init parent reg info at index {index}")
                        return -4

                    # Examine the register information that needs to be parsed for each bit field of the primary overall status register
                    reg_bit_domain_spread = cpu_reg_info.get("reg_bit_domain_spread")
                    if reg_bit_domain_spread is None:
                        logger.error(f"reg info {index} reg_bit_domain_spread is None")
                        return -2

                    logger.info(f"index{index}: start reg_bit_domain_spread parsing")
                    ret = self._reg_bit_domain_spread_parse(reg_bit_domain_spread, xor_val, get_val_list[0], parent_reg_info)
                    if ret != 0:
                        logger.error(f"reg info {index} reg_bit_domain_spread parsing failed")
                        return -3
                    logger.info(f"index{index}: reg_bit_domain_spread parsing completed")

                # It is a leaf node. Directly extract the information of the register group to be parsed one by one.
                else:
                    get_val_list = cpu_reg_info.get("get_val", [])
                    for i in range(len(get_val_list)):
                        res = self._init_and_add_reg_info(cpu_reg_info, None, i)
                        if res is None:
                            logger.error(f"Failed to init reg info at leaf node index {index}, item {i}")
                            return -5
                index += 1
            return 0
        except Exception as e:
            logger.error(f"get_cpu_reg_parse_info exception: {e}")
            logger.debug(traceback.format_exc())
            return -4


def print_regs_info():
    # TO DO:
    if not g_regs_info:
        logger.info("No registers info collected to print.")
        return
    logger.info("Collected Registers Info:")
    for idx, reg in enumerate(g_regs_info):
        try:
            addr_range = reg.reg_addr.get("addr_range") if reg.reg_addr else None
            base_addr = addr_range.get("base") if addr_range else None
            length = addr_range.get("len") if addr_range else None
            logger.info(
                f"Reg[{idx}]: desc={reg.reg_desc}, type={reg.reg_type}, "
                f"addr_base={base_addr}, addr_len={length}, "
                f"val=0x{reg.reg_val:x} if reg.reg_val is not None else 'None', "
                f"expected=0x{reg.reg_expected_val:x} if reg.reg_expected_val is not None else 'None'"
            )
        except Exception as e:
            logger.warning(f"Failed to print reg info at index {idx}: {e}")


def start():
    # Traverse the JSON files generated by each event, extract the parsing information within them, and store the extracted information in the variable "g_regs_info".
    global g_regs_info
    g_regs_info = []
    for event_type in range(event_msg_queue.EventType.EVENT_TYPE_A, event_msg_queue.EventType.EVENT_TYPE_END):
        cpu_reg_info_parse = CpuRegInfoParse(event_type)
        ret = cpu_reg_info_parse.load_json_file()
        if ret:
            logger.info(f"load json file fail, ret:{ret}")
            continue

        ret = cpu_reg_info_parse.get_cpu_reg_parse_info()
        if ret:
            logger.error(f"get cpu reg parse info fail, ret:{ret}")
            return -1

    # Print information
    print_regs_info()
    return 0


if __name__ == '__main__':
    exit_code = start()
    sys.exit(exit_code)

