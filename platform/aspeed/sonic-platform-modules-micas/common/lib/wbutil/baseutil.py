#!/usr/bin/env python_nos
import os
import re
import json
from public.platform_common_config import SUB_VERSION_FILE, PRODUCT_NAME_PATH, BOARD_ID_PATH, BOARD_AIRFLOW_PATH
from public.platform_common_config import DFD_BOARD_ID_PATH
from public.platform_common_config import HOST_MACHINE


def get_info_from_file(file_path):
    if not os.path.exists(file_path):
        return False, "%s not exist" % file_path

    try:
        with open(file_path) as fd:
            info = fd.read().strip()
        if len(info) == 0:
            return False, "%s is empty" % file_path
        return True, info.replace("-","_").lower()
    except Exception as e:
        msg = str(e)
        return False, msg


def get_machine_info():
    machine_vars = {}
    if os.path.isfile(HOST_MACHINE):
        with open(HOST_MACHINE) as machine_file:
            for line in machine_file:
                tokens = line.split('=')
                if len(tokens) < 2:
                    continue
                machine_vars[tokens[0]] = tokens[1].strip()
        return machine_vars
    return None


def get_platform_info():
    return get_info_from_file(PRODUCT_NAME_PATH)


def get_board_id():
    if os.path.exists(DFD_BOARD_ID_PATH):
        try:
            with open(DFD_BOARD_ID_PATH) as fd:
                board_id = fd.read().strip()
            if len(board_id) == 0:
                return False, "%s is empty" % DFD_BOARD_ID_PATH
            return True, "0x%x" % int(board_id, 10)
        except Exception as e:
            return False, str(e)

    status, info = get_info_from_file(BOARD_ID_PATH)
    return status, info


def get_sub_version():
    return get_info_from_file(SUB_VERSION_FILE)


def get_board_airflow():
    if not os.path.isfile(BOARD_AIRFLOW_PATH):
        return False, "%s not exist" % BOARD_AIRFLOW_PATH

    try:
        with open(BOARD_AIRFLOW_PATH) as fd:
            airflow_str = fd.read().strip()
        data = json.loads(airflow_str)
        airflow = data.get("board", "")
        if len(airflow) == 0:
            return False, "board airflow not found"
        return True, airflow.replace("-","_").lower()
    except Exception as e:
        msg = str(e)
        return False, msg
