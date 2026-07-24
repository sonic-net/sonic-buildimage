#!/usr/bin/env python_nos
import importlib
import json
import os
import re
import sys
import tarfile
import time
import threading
import logging
import shutil
from pathlib import Path
from tech_support.collector_base import CollectorBase
from tech_support.collector_common import logger
from platform_tech_config import COLLECT_MODULES
from public.platform_common_config import S3IP_SYSFS_NAME
from platform_util import read_s3ip_sysfs
from platform_tech_config import COLLECT_TYPE_MAP
from platform_tech_config import LOG_LINK_TARGET
from platform_tech_config import RUNNINGDATA_TYPE
from platform_tech_config import COMPONENT_TYPE

SPINNER = ['|', '/', '-', '\\']
collect_dir = "/var/log/bsp_tech_support/"
DEBUG_FILE = "/etc/.platform_tech_debug_flag"

CPU_PWR_DOWN_STATUS = 1
CPU_BOARD_STATUS_FILE = f"/sys/{S3IP_SYSFS_NAME}/system/cpu_board_status"
PROGRESS_DIR = "/var/run/onekey_collect/"
PROGRESS_FILE = os.path.join(PROGRESS_DIR, "platform_tech_progress")

def debug_init():
    if os.path.exists(DEBUG_FILE):
        logger.setLevel(logging.DEBUG)
    else:
        logger.setLevel(logging.INFO)


def _write_pretty(f, data):
    if not isinstance(data, dict):
        f.write(str(data) + '')
        return
    for k, v in data.items():
        f.write(f'============================> {k} <============================\n')
        if isinstance(v, (dict, list)):
            try:
                f.write(json.dumps(v, indent=2, ensure_ascii=False) + '\n')
            except Exception:
                f.write(str(v) + '\n')
        else:
            f.write(str(v) + '\n')


def clear_collect_dir():
    if os.path.exists(collect_dir):
        shutil.rmtree(collect_dir)
    os.makedirs(collect_dir)


def write_progress(state, percent, current):
    os.makedirs(PROGRESS_DIR, exist_ok=True)
    tmp_file = f'{PROGRESS_FILE}.{os.getpid()}.tmp'
    content = [
        f'state={state}',
        f'percent={max(0, min(100, int(percent)))}',
        f'current={current or ""}',
    ]

    with open(tmp_file, 'w', encoding='utf-8') as f:
        f.write('\n'.join(content) + '\n')
        f.flush()
        os.fsync(f.fileno())

    os.replace(tmp_file, PROGRESS_FILE)


def get_progress_percent(default=0):
    try:
        with open(PROGRESS_FILE, 'r', encoding='utf-8') as f:
            for line in f:
                if line.startswith('percent='):
                    return int(line.split('=', 1)[1].strip())
    except Exception:
        pass
    return default


def calc_percent(done_modules, total_modules):
    if total_modules <= 0:
        return 0
    return int((done_modules / total_modules) * 100)


def save_output(results):
    timestamp = time.strftime('%Y%m%d_%H%M%S')
    out_dir = Path("%s%s_%s" % (collect_dir, "bsp_tech_support", timestamp))
    out_dir.mkdir(parents=True, exist_ok=True)

    runningdata_dir = out_dir / RUNNINGDATA_TYPE
    component_dir = out_dir / COMPONENT_TYPE
    log_dir = out_dir / 'log'
    runningdata_dir.mkdir(parents=True, exist_ok=True)
    component_dir.mkdir(parents=True, exist_ok=True)

    if log_dir.exists() or log_dir.is_symlink():
        if log_dir.is_symlink() or log_dir.is_file():
            log_dir.unlink()
        else:
            shutil.rmtree(log_dir)
    os.symlink(LOG_LINK_TARGET, log_dir)

    runningdata_modules = COLLECT_TYPE_MAP.get(RUNNINGDATA_TYPE, [])
    component_modules = COLLECT_TYPE_MAP.get(COMPONENT_TYPE, [])

    # write collect result to file
    for r in results:
        name = r.get('name', 'unknown')
        target_dir = out_dir
        if name in runningdata_modules:
            target_dir = runningdata_dir
        elif name in component_modules:
            target_dir = component_dir

        fn = target_dir / f'{name}.txt'
        with fn.open('w', encoding='utf-8') as f:
            if r.get('status') == 'ok':
                data = r.get('data')
                _write_pretty(f, data)
            else:
                f.write('ERROR:\n')
                f.write(r.get('error', '') + '\n')
                f.write('TRACEBACK:\n')
                f.write(r.get('traceback', '\n'))

    tar_path = str(out_dir) + '.tar.gz'
    with tarfile.open(tar_path, 'w:gz') as tar:
        tar.add(out_dir, arcname=os.path.basename(out_dir))

    print(f'[+] saved results to {tar_path}')


def load_collectors(modules):
    collectors = []
    base_pkg = 'tech_support'
    allowed_modules = set(COLLECT_MODULES)

    for module_name in modules:
        if module_name not in allowed_modules or re.fullmatch(r"[A-Za-z0-9_]+", module_name) is None:
            print(f'invalid module name: {module_name}')
            continue
        mod_name = f'{base_pkg}.{module_name}_collector'
        try:
            mod = __import__(mod_name, fromlist=['*'])
        except Exception as e:
            print(f'load module {mod_name} failed: {e}')
            continue

        for attr in dir(mod):
            obj = getattr(mod, attr)
            try:
                is_cls = isinstance(obj, type)
            except Exception:
                is_cls = False

            if is_cls and issubclass(obj, CollectorBase) and obj is not CollectorBase:
                try:
                    collectors.append(obj())
                except Exception as e:
                    print(f'init collector {attr} failed: {e}')
    return collectors


def platform_tech_help(modules):
    print('Usage: platform_tech.py [all|module_name...]')
    module_name =  ','.join(modules)
    print('Support module_name: %s ' % module_name)
    print('-s: save old log ')
    sys.exit(1)


def start_progress_indicator(start_time, module_index, total_modules):
    """Start a background spinner + elapsed time + percent."""
    stop_event = threading.Event()

    def run():
        i = 0
        while not stop_event.is_set():
            elapsed = int(time.time() - start_time)
            percent = calc_percent(module_index, total_modules)
            spinner = SPINNER[i % len(SPINNER)]
            print(f'\r[{spinner}] {percent}% elapsed: {elapsed}s', end='', flush=True)
            i += 1
            time.sleep(1)

    thread = threading.Thread(target=run)
    thread.daemon = True
    thread.start()
    return stop_event


def main():
    if os.geteuid() != 0:
        print("Root privileges are required for this operation")
        sys.exit(1)

    if isinstance(COLLECT_MODULES, list):
        all_modules = COLLECT_MODULES
    elif isinstance(COLLECT_MODULES, dict):
        ret, val = read_s3ip_sysfs(CPU_BOARD_STATUS_FILE)
        if ret is False:
            print(f'read {CPU_BOARD_STATUS_FILE} fail')
            sys.exit(1)

        cpu_pwr_status = int(val)
        if cpu_pwr_status == CPU_PWR_DOWN_STATUS:
            print("note: cpu power down, collect a portion of info")
            all_modules = [key for key, value in COLLECT_MODULES.items() if value == 1]
        else:
            all_modules = list(COLLECT_MODULES.keys())
    else:
        print("unsupport COLLECT_MODULES type, need dict or list")
        sys.exit(1)

    if len(all_modules) == 0:
        print("Invalid COLLECT_MODULES, please check product tech support config")
        sys.exit(1)

    if len(sys.argv) < 2:
        platform_tech_help(all_modules)

    debug_init()

    args = sys.argv[1:]
    skip_clear = '-s' in args
    if skip_clear:
        args.remove('-s')

    if 'all' in args and len(args) != 1:
        print("Invalid parameters")
        platform_tech_help(all_modules)

    if 'all' in args:
        modules = all_modules
    else:
        modules = []
        for module_name in args:
            if module_name not in all_modules:
                print("Invalid module name: %s" % module_name)
                platform_tech_help(all_modules)
            else:
                modules.append(module_name)

    print(f'collecting modules: {modules}')
    write_progress('pending', 0, 'loading collectors')

    collectors = load_collectors(modules)
    results = []

    total = len(collectors)
    if total == 0:
        print("No collectors found")
        platform_tech_help(all_modules)

    if not skip_clear:
        clear_collect_dir()

    write_progress('running', calc_percent(0, total), 'preparing collection')

    for idx, c in enumerate(collectors, start=1):
        percent_before = calc_percent(idx - 1, total)
        write_progress('running', percent_before, c.name)
        print(f'\ncollecting {c.name}...')
        start_time = time.time()

        # start indicator: percent is based on module index / total modules
        stop_event = start_progress_indicator(start_time, idx, total)

        try:
            result = c.safe_collect()
            results.append(result)
        except Exception as e:
            print(f'\ncollector {c.name} exception: {e}')
        finally:
            stop_event.set()
            time.sleep(0.1)

        # final print for this module (overwrite spinner line)
        elapsed = int(time.time() - start_time)
        percent = calc_percent(idx, total)
        write_progress('running', percent, c.name)
        print(f'\r[✔] {percent}% elapsed: {elapsed}s')

    print("\nSaving results...")
    write_progress('running', calc_percent(total, total), 'saving results')
    save_output(results)
    write_progress('completed', 100, 'completed')
    print("Done.")



if __name__ == '__main__':
    try:
        main()
    except Exception as e:
        try:
            write_progress('failed', get_progress_percent(), str(e))
        except Exception:
            pass
        raise
