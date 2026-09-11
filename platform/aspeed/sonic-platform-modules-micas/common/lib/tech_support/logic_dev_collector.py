#!/usr/bin/env python_nos
import os
from tech_support.collector_base import CollectorBase
from tech_support.collector_common import logger, run_cmd

class LogicDevCollector(CollectorBase):
    name = "logic_dev"
    __collect_skip_flag = "collect_skip_flag"
    COLLECT_TIMES = 2

    def collect_once(self, root, result, iteration):
        """
        Collect logic device data once, iterate through all directories under root
        :param root: Base directory to walk through
        :param result: Dict to store collected data
        :param iteration: Iteration number for log distinction
        """
        logger.debug(f"===== Starting iteration {iteration} of logic device collection =====")
        for base, dirs, files in os.walk(root):
            dirs.sort()
            files.sort()

            rel = os.path.relpath(base, root)
            if rel == ".":
                rel = ""

            #if /sys/logic_dev/XXX/collect_skip_flag exists and value is 1, skip collection
            if self.__collect_skip_flag in files:
                path = os.path.join(base, self.__collect_skip_flag)
                try:
                    with open(path, 'r') as f:
                        content = f.read().strip()
                    if content == "1":
                        logger.debug(f"Collection disabled for {rel}, skipping this directory.")
                        continue
                except Exception as e:
                    logger.error(f"Error reading {path}: {e}")

            for f in files:
                path = os.path.join(base, f)
                key = os.path.join(rel, f)
                # Uncomment below line to avoid result overwrite between iterations
                # key = f"{key}_iter{iteration}"
                logger.debug(f"Iteration {iteration} collecting: {path}")

                try:
                    # Check read permission
                    mode = os.stat(path).st_mode
                    if not (mode & 0o444):
                        result[f"{key} iter{iteration}"] = f"<write-only_iter{iteration}>"
                        continue
                    status, content = run_cmd(f'cat {path}')
                except Exception as e:
                    content = f"<error_iter{iteration}: {str(e)}>"
                    logger.error(content)
                result[f"{key} iter{iteration}"] = content

            # Process /dev/cpldX or /dev/fpgaX devices
            dev_name = os.path.basename(base)
            dev_path = os.path.join("/dev", dev_name)
            dev_key = dev_path
            # Uncomment below line to avoid result overwrite between iterations
            # dev_key = f"{dev_path}_iter{iteration}"

            if os.path.exists(dev_path):
                try:
                    logger.debug(f"Iteration {iteration} hexdump: {dev_path}")
                    status, content = run_cmd(f'hexdump -C -v {dev_path}', timeout=20)
                except Exception as e:
                    content = f"<{dev_name}_read_error_iter{iteration}: {str(e)}>"
                    logger.error(content)
                result[f"{dev_key} iter{iteration}"] = content
        logger.debug(f"===== Iteration {iteration} of logic device collection completed =====")

    def collect(self):
        root = "/sys/logic_dev"
        result = {}

        if not os.path.exists(root):
            return {"logic_dev_error": f"{root} does not exist"}

        # Use loop to execute collection multiple times
        logger.debug(f"===== Starting total {self.COLLECT_TIMES} collection iterations =====")
        for iteration in range(1, self.COLLECT_TIMES + 1):
            self.collect_once(root, result, iteration)

        logger.debug("===== All logic device collection iterations completed =====")
        return result

