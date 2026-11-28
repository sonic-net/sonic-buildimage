#!/usr/bin/env python_nos
import shlex
import subprocess
import sys
import os


SYS_PATH_BIN = '/usr/local/bin/'
if SYS_PATH_BIN not in sys.path:
    sys.path.insert(0, SYS_PATH_BIN)

from platform_util import setup_logger, BSP_COMMON_LOG_DIR

LOG_FILE = BSP_COMMON_LOG_DIR + "platform_tech_debug.log"
logger = setup_logger(LOG_FILE)


def run_cmd(cmd, ignore_error=False, timeout=1, use_shell=False):
    try:
        if use_shell:
            proc = subprocess.run(
                cmd,
                shell=use_shell,
                capture_output=True,
                text=True,
                timeout=timeout
            )
        else:
            if isinstance(cmd, str):
                argv = shlex.split(cmd)
            else:
                argv = cmd

            proc = subprocess.run(
                argv,
                capture_output=True,
                text=True,
                timeout=timeout
            )

        if proc.returncode != 0 and not ignore_error:
            raise RuntimeError(proc.stderr.strip() or f"returncode {proc.returncode}")

        return True, (proc.stdout or proc.stderr or '').strip()

    except Exception as e:
        return False, f"Error: {e}"


def collect_driver_info(root_dir, binary_file_list=[]):
    skip_dirs = {"module", "driver", "power", "subsystem", "device"}
    skip_files = {"bind", "unbind", "uevent"}
    result = {}

    logger.info(f"Starting collect {root_dir} info...")
    if not os.path.exists(root_dir):
        logger.info(f"{root_dir} not exists, skip")
        return result

    for dev in sorted(os.listdir(root_dir)):
        dev_path = os.path.join(root_dir, dev)
        if not os.path.islink(dev_path):
            continue

        if dev in skip_dirs or dev in skip_files:
            continue

        real = os.path.realpath(dev_path)  # real sysfs device path
        for base, dirs, files in os.walk(real):
            dirs[:] = [d for d in dirs if d not in skip_dirs]
            dirs.sort()
            files.sort()

            rel = os.path.relpath(base, real)
            if rel == ".":
                rel = dev  # group under device name
            else:
                rel = os.path.join(dev, rel)

            for f in files:
                if f in skip_files:
                    continue

                path = os.path.join(base, f)
                key = os.path.join(rel, f)
                logger.debug(f"collect: {path}")
                # check read permission
                try:
                    mode = os.stat(path).st_mode
                    if not (mode & 0o444):
                        result[key] = "<write-only>"
                        continue
                except Exception as e:
                    result[key] = f"<stat error: {e}>"
                    continue

                if f not in binary_file_list:
                    is_text = True
                else:
                    is_text = False

                if is_text:
                    try:
                        status, content = run_cmd(f'cat {path}')
                    except Exception as e:
                        content = f"<read error: {e}>"
                        logger.error(content)
                else:
                    # Binary: hexdump
                    try:
                        status, content = run_cmd(f'hexdump -C -v {path}', timeout=3)
                    except Exception as e:
                        content = f"<hexdump error: {e}>"
                        logger.error(content)
                result[key] = content

    logger.info(f"Finish collect {root_dir} info...")
    if not result:
        return {f"{root_dir}": f"{root_dir} device not found"}
    return result

