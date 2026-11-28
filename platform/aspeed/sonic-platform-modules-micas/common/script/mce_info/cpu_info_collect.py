#!/usr/bin/env python_nos
# -*- coding: UTF-8 -*-

import sys
import os
import time
import syslog
import json
import threading
import logging
import traceback
import fcntl
import copy


import cpu_info_collect_cfg
import event_msg_queue
from platform_util import check_value, get_value, set_value, exec_os_cmd, CHECK_VALUE_OK


logger = logging.getLogger(__name__)
logging.basicConfig(level=logging.INFO, format='%(asctime)s [%(levelname)s] %(message)s')


class CpuRegInfoCollect:
    def __init__(self, event, cpu_reg_info_cfg):
        self.cpu_reg_info_cfg = cpu_reg_info_cfg
        self.event = event

    def _get_reg_val(self, get_cmd):
        get_val_list = []
        # TODO: Implement the logic for specifically obtaining the values of registers, such as executing commands and parsing the results.
        # Examples:
        # try:
        #     output = exec_os_cmd(get_cmd)
        #     # Parse output and fill in get_val_list
        # except Exception as e:
        #     logger.error(f"Failed to get reg val for cmd {get_cmd}: {e}")
        return get_val_list

    def _reg_bit_domain_spread(self, reg_bit_domain_spread, xor_val, get_val):
        try:
            for cfg in reg_bit_domain_spread:
                mask = cfg.get("mask")
                if mask is None:
                    logger.warning("reg_bit_domain_spread entry missing 'mask', skipping")
                    continue
                logger.debug(f"  mask: 0x{mask:x}")
                if xor_val & mask == 0:
                    logger.debug("  mask bits no error, skip")
                    continue

                val = cfg.get("val")
                if val is None:
                    logger.warning("reg_bit_domain_spread entry missing 'val', skipping")
                    continue
                logger.debug(f"  val: 0x{val:x}")
                if (get_val & mask) != val:
                    logger.debug("  val not match, skip")
                    continue

                regs_arr = cfg.get("regs_arr")
                if regs_arr is None:
                    logger.error("  regs_arr is None")
                    return -1

                for reg in regs_arr.values():
                    get_cmd = reg.get("get_cmd")
                    logger.debug(f"  get_cmd: {get_cmd}")
                    if get_cmd is None:
                        logger.error("  get_cmd is None")
                        return -2
                    val_list = self._get_reg_val(get_cmd)
                    logger.debug(f"  val_list: {val_list}")
                    reg["get_val"] = val_list

            return 0

        except Exception as e:
            logger.error(f"_reg_bit_domain_spread exception: {e}")
            logger.debug(traceback.format_exc())
            return -3

    def get_cpu_reg_info(self):
        if self.cpu_reg_info_cfg is None:
           logger.error(f"reg info {index} get_cmd is None or empty")
           return -1

        try:
            index = 0
            for reg_info in self.cpu_reg_info_cfg.values():
                get_cmd = reg_info.get("get_cmd")
                logger.info(f"index{index}: get_cmd: {get_cmd}")
                if not get_cmd:
                    logger.error(f"reg info {index} get_cmd is None or empty")
                    return -1

                val_list = self._get_reg_val(get_cmd)
                logger.info(f"index{index}: val_list: {val_list}")
                reg_info["get_val"] = val_list

                is_leaf_node = reg_info.get("is_leaf_node", True)
                logger.info(f"index{index}: is_leaf_node: {is_leaf_node}")
                if not is_leaf_node:
                    get_val0 = val_list[0] if val_list else None
                    ok_val_list = reg_info.get("ok_val", [])
                    ok_val0 = ok_val_list[0] if ok_val_list else None

                    if get_val0 is None or ok_val0 is None:
                        logger.error(f"reg info {index} missing get_val[0] or ok_val[0]")
                        return -4

                    xor_val = get_val0 ^ ok_val0
                    logger.info(f"index{index}: get_val:0x{get_val0:x}, ok_val:0x{ok_val0:x}, xor_val: 0x{xor_val:x}")

                    if xor_val == 0:
                        logger.info("no err, skip reg_bit_domain_spread")
                        index += 1
                        continue

                    reg_bit_domain_spread = reg_info.get("reg_bit_domain_spread")
                    if reg_bit_domain_spread is None:
                        logger.error(f"reg info {index} reg_bit_domain_spread is None")
                        return -2

                    logger.info(f"index{index}: do reg_bit_domain_spread begin")
                    ret = self._reg_bit_domain_spread(reg_bit_domain_spread, xor_val, get_val0)
                    if ret != 0:
                        logger.error(f"reg info {index} do _reg_bit_domain_spread fail")
                        return -3
                    logger.info(f"index{index}: do reg_bit_domain_spread ok")
                index += 1

            return 0

        except Exception as e:
            logger.error(f"get_cpu_reg_info exception: {e}")
            logger.debug(traceback.format_exc())
            return -4

    def export_json_file(self):
        file = f"/tmp/cpu_reg_info_collect_{self.event}.json"
        with open(file, "w", encoding='utf-8') as f:
            try:
                fcntl.flock(f.fileno(), fcntl.LOCK_EX)
                json.dump(self.cpu_reg_info_cfg, f)
                f.flush()
            except Exception as e:
                logger.error(f"Failed to export json file {file}: {e}")
                logger.debug(traceback.format_exc())
                return -1
            finally:
                fcntl.flock(f.fileno(), fcntl.LOCK_UN)

        logger.info(f"Exported CPU reg info to {file} ok")
        return 0


def get_cpu_status():
    try:
        # 1. Initialization
        is_in_power_off_status = False
        is_in_reboot_status = False
        cpu_status_para = cpu_info_collect_cfg.CPU_STATUS_PARAM.copy()

        # 2. Configuration for obtaining CPU status
        cfg1 = cpu_status_para.get('power_off_status_cfg')
        cfg2 = cpu_status_para.get('reboot_status_cfg')
        if cfg1 is None and cfg2 is None:
            logger.warning('CPU_STATUS: conf power_off_status_cfg and reboot_status_cfg do not exist.')
            return is_in_power_off_status, is_in_reboot_status

        # 3.  Verify the reading to determine if the CPU is in a powered-off state
        if cfg1 is not None:
            ret, val = check_value(cfg1)
            if val == CHECK_VALUE_OK:
                is_in_power_off_status = True

        # 4. Read the value to confirm whether the CPU is in the reset state
        if cfg2 is not None:
            ret, val = check_value(cfg2)
            if val == CHECK_VALUE_OK:
                is_in_reboot_status = True

    except Exception as e:
        logger.error(f"CPU_STATUS-3-EXCEPTION: get cpu status error. msg: {e}")
        logger.debug(traceback.format_exc())

    return is_in_power_off_status, is_in_reboot_status


def do_info_collect_event_type_a():
    cpu_reg_info = CpuRegInfoCollect(event_msg_queue.EventType.EVENT_TYPE_A, copy.deepcopy(cpu_info_collect_cfg.CPU_REG_INFO_CFG0))
    ret = cpu_reg_info.get_cpu_reg_info()
    if ret != 0:
        logger.error(f"get_cpu_reg_info fail, ret: {ret}")
        return

    pd_status, reboot_status = get_cpu_status()
    if not pd_status and not reboot_status:
        ret = cpu_reg_info.export_json_file()
        if ret != 0:
            logger.error(f"export_json_file failed, ret: {ret}")


def do_info_collect_event_type_b():
    # TODO: Implement business logic
    pass


def do_info_collect_event_type_c():
    # TODO: Implement business logic
    pass


def collect_info_by_event_type(event_type):
    """
    Continuously monitor the message queue corresponding to the event type and handle the events.
    """
    try:
        while True:
            event = event_msg_queue.get_event_from_event_msg_queue(event_type)

            pd_status, reboot_status = get_cpu_status()
            if pd_status or reboot_status:
                logger.debug("CPU is in power off or reboot status, skipping event processing")
                continue

            if event == event_msg_queue.EventType.EVENT_TYPE_A:
                do_info_collect_event_type_a()
            elif event == event_msg_queue.EventType.EVENT_TYPE_B:
                do_info_collect_event_type_b()
            elif event == event_msg_queue.EventType.EVENT_TYPE_C:
                do_info_collect_event_type_c()
            else:
                logger.warning(f"Unsupported event: {event}")

    except Exception as e:
        logger.error(f"collect_info_by_event_type exception: {e}")
        logger.debug(traceback.format_exc())


def start():
    """
    Start the event listening thread and keep the main thread running.
    """
    threads = []
    for event_type in range(event_msg_queue.EventType.EVENT_TYPE_A, event_msg_queue.EventType.EVENT_TYPE_END):
        thread = threading.Thread(target=collect_info_by_event_type, args=(event_type,), daemon=True)
        threads.append(thread)

    for thread in threads:
        thread.start()

    logger.info("All event listener threads started.")

    try:
        while True:
            time.sleep(1)
    except KeyboardInterrupt:
        logger.info("Received KeyboardInterrupt, exiting...")


if __name__ == '__main__':
    start()
