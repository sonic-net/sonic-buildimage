#!/usr/bin/env python_nos
import os
from tech_support.collector_base import CollectorBase
from tech_support.collector_common import logger, run_cmd
from public.platform_common_config import S3IP_SYSFS_NAME

binary_file_list = ['syseeprom', 'eeprom', 'data']

class S3IPCollector(CollectorBase):
    name = "s3ip"

    def collect(self):
        root = f"/sys/{S3IP_SYSFS_NAME}"
        result = {}
        logger.info("Starting s3ip collect...")

        if not os.path.exists(root):
            return {"s3ip_error": f"{root} does not exist"}

        for base, dirs, files in os.walk(root):
            dirs.sort()
            files.sort()

            rel = os.path.relpath(base, root)
            if rel == ".":
                rel = ""

            for f in files:
                path = os.path.join(base, f)
                key = os.path.join(rel, f)
                logger.debug(f"collect: {path}")
                # Check read permission
                try:
                    mode = os.stat(path).st_mode
                    if not (mode & 0o444):
                        result[key] = "<write-only>"
                        continue
                except Exception as e:
                    msg = f"<stat error: {e}>"
                    logger.error(msg)
                    result[key] = msg
                    continue

                if f not in binary_file_list:
                    is_text = True
                else:
                    is_text = False

                # Full content read with timeout
                if is_text:
                    try:
                        status, content = run_cmd(f'cat {path}')
                    except Exception as e:
                        content = f"<read error: {e}>"
                        logger.error(content)
                else:
                    # Binary: hexdump
                    try:
                        status, content = run_cmd(f'hexdump -C -v {path}', timeout=5)
                    except Exception as e:
                        content = f"<hexdump error: {e}>"
                        logger.error(content)
                result[key] = content
        logger.info("Finish s3ip collect...")
        return result
