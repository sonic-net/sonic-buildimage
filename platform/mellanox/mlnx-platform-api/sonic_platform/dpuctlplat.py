#
# SPDX-FileCopyrightText: NVIDIA CORPORATION & AFFILIATES
# Copyright (c) 2024-2025 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
# Apache-2.0
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

"""Class Implementation for per DPU functionality"""
import os.path
import time
import multiprocessing
import subprocess
from contextlib import contextmanager
from select import poll, POLLPRI, POLLIN
from enum import Enum

try:
    from .inotify_helper import InotifyHelper
    from sonic_py_common.syslogger import SysLogger
    from . import utils
    from .device_data import DeviceDataManager, DpuInterfaceEnum
except ImportError as e:
    raise ImportError(str(e)) from e

HW_BASE = "/var/run/hw-management/"
EVENT_BASE = os.path.join(HW_BASE, "events/")
SYSTEM_BASE = os.path.join(HW_BASE, "system/")
PCI_BASE = "/sys/bus/pci/"
PCI_DEV_BASE = os.path.join(PCI_BASE, "devices/")

logger = SysLogger()

WAIT_FOR_SHTDN = 120
WAIT_FOR_DPU_READY = 180


class OperationType(Enum):
    CLR = "0"
    SET = "1"


class BootProgEnum(Enum):
    RST = 0
    BL2 = 1
    BL31 = 2
    UEFI = 3
    OS_START = 4
    OS_RUN = 5
    LOW_POWER = 6
    FW_UPDATE = 7
    OS_CRASH_PROG = 8
    OS_CRASH_DONE = 9
    FW_FAULT_PROG = 10
    FW_FAULT_DONE = 11
    SW_INACTIVE = 15


class DpuCtlPlat():
    """Class for Per DPU API Call"""
    def __init__(self, dpu_name):
        self.dpu_name = dpu_name
        self._name = self.get_hwmgmt_name()
        self.dpu_id = int(self.dpu_name[3:])
        self.rst_path = os.path.join(SYSTEM_BASE,
                                     f"{self._name}_rst")
        self.pwr_path = os.path.join(SYSTEM_BASE,
                                     f"{self._name}_pwr")
        self.pwr_f_path = os.path.join(SYSTEM_BASE,
                                       f"{self._name}_pwr_force")
        self.dpu_rdy_path = os.path.join(EVENT_BASE,
                                         f"{self._name}_ready")
        self.shtdn_ready_path = os.path.join(EVENT_BASE,
                                             f"{self._name}_shtdn_ready")
        self.boot_prog_path = os.path.join(HW_BASE,
                                           f"{self._name}/system/boot_progress")
        self.boot_prog_map = {
            BootProgEnum.RST.value: "Reset/Boot-ROM",
            BootProgEnum.BL2.value: "BL2 (from ATF image on eMMC partition)",
            BootProgEnum.BL31.value: "BL31 (from ATF image on eMMC partition)",
            BootProgEnum.UEFI.value: "UEFI (from UEFI image on eMMC partition)",
            BootProgEnum.OS_START.value: "OS Starting",
            BootProgEnum.OS_RUN.value: "OS is running",
            BootProgEnum.LOW_POWER.value: "Low-Power Standby",
            BootProgEnum.FW_UPDATE.value: "FW Update in progress",
            BootProgEnum.OS_CRASH_PROG.value: "OS Crash Dump in progress",
            BootProgEnum.OS_CRASH_DONE.value: "OS Crash Dump is complete",
            BootProgEnum.FW_FAULT_PROG.value: "FW Fault Crash Dump in progress",
            BootProgEnum.FW_FAULT_DONE.value: "FW Fault Crash Dump is complete",
            BootProgEnum.SW_INACTIVE.value: "Software is inactive"
        }
        self.boot_prog_state = None
        self.shtdn_state = None
        self.dpu_ready_state = None
        self.setup_logger()
        self.pci_dev_path = []
        self.verbosity = False

    def setup_logger(self, use_print=False):
        def print_with_time(msg):
            timestamp = time.strftime("%Y-%m-%d %H:%M:%S")
            print(f"[{timestamp}] {msg}")

        if use_print:
            self.logger_info = print_with_time
            self.logger_error = print_with_time 
            self.logger_debug = print_with_time
            return
        self.logger_debug = logger.log_debug
        self.logger_info = logger.log_info
        self.logger_error = logger.log_error

    def log_debug(self, msg=None):
        # Print only in verbose mode
        self.logger_debug(f"{self.dpu_name}: {msg}")

    def log_info(self, msg=None):
        self.logger_info(f"{self.dpu_name}: {msg}")

    def log_error(self, msg=None):
        self.logger_error(f"{self.dpu_name}: {msg}")

    def run_cmd_output(self, cmd, raise_exception=True):
        try:
            return subprocess.check_output(cmd).decode().strip()
        except Exception as err:
            if raise_exception:
                raise err
            else:
                self.log_debug(f"Failed to run cmd {' '.join(cmd)}")

    def dpu_pre_shutdown(self):
        """Method to execute shutdown activities for the DPU"""
        return self.dpu_pci_remove()

    def dpu_post_startup(self):
        """Method to execute all post startup activities for the DPU"""
        return self.dpu_pci_scan()

    @contextmanager
    def get_open_fd(self, path, flag):
        fd = os.open(path, flag)
        try:
            yield fd
        finally:
            os.close(fd)

    def get_pci_dev_path(self):
        """Parse the PCIE devices ID from platform.json, raise Runtime error if the device id is not available"""
        if self.pci_dev_path:
            return self.pci_dev_path
        
        pci_dev_id = DeviceDataManager.get_dpu_interface(self.dpu_name, DpuInterfaceEnum.PCIE_INT.value)
        rshim_pci_dev_id = DeviceDataManager.get_dpu_interface(self.dpu_name, DpuInterfaceEnum.RSHIM_PCIE_INT.value)
        if not pci_dev_id or not rshim_pci_dev_id:
            raise RuntimeError(f"Unable to obtain PCI device IDs for {self.dpu_name} from platform.json")

        self.pci_dev_path = [os.path.join(PCI_DEV_BASE, pci_dev_id),
                                os.path.join(PCI_DEV_BASE, rshim_pci_dev_id)]

        return self.pci_dev_path

    def write_file(self, file_name, content_towrite):
        """Write given value to file only if file exists"""
        try:
            if self.verbosity:
                self.log_debug(f'Writing {content_towrite} to file {file_name}')
            utils.write_file(file_name, content_towrite, raise_exception=True)
        except Exception as e:
            self.log_error(f'Failed to write {content_towrite} to file {file_name}')
            raise type(e)(f"{self.dpu_name}:{str(e)}")
        return True

    def get_hwmgmt_name(self):
        """Return name of the DPU in the HW Management mapping"""
        return f"{self.dpu_name[:3]}{str(int(self.dpu_name[3:])+1)}"

    def dpu_go_down(self):
        """Per DPU going down API"""
        self.write_file(self.rst_path, OperationType.CLR.value)
        try:
            get_shtdn_inotify = InotifyHelper(self.shtdn_ready_path)
            with self.time_check_context("going down"):
                dpu_shtdn_rdy = get_shtdn_inotify.wait_watch(WAIT_FOR_SHTDN, 1)
        except (FileNotFoundError, PermissionError) as inotify_exc:
            raise type(inotify_exc)(f"{self.dpu_name}:{str(inotify_exc)}")
        if not dpu_shtdn_rdy:
            self.log_error(f"Going Down Unsuccessful")
            return False
        return True

    def _power_off(self):
        """Per DPU Power off private function"""
        if not self.dpu_go_down():
            return self._power_off_force()
        self.write_file(self.pwr_path, OperationType.CLR.value)
        self.log_info(f"Power Off complete")
        return True

    def _power_off_force(self):
        """Per DPU Force Power off private function"""
        self.write_file(self.rst_path, OperationType.CLR.value)
        self.write_file(self.pwr_f_path, OperationType.CLR.value)
        self.log_info(f"Force Power Off complete")
        return True

    def _power_on_force(self, count=4, no_wait=False):
        """Per DPU Power on with force private function"""
        if count < 4:
            self.log_error(f"Failed Force Power on! Retry {4-count}..")
        self.write_file(self.pwr_f_path, OperationType.SET.value)
        self.write_file(self.rst_path, OperationType.SET.value)
        if no_wait:
            self.log_debug("Exiting without checking result of reboot command")
            return True
        get_rdy_inotify = InotifyHelper(self.dpu_rdy_path)
        with self.time_check_context("power on force"):
            dpu_rdy = get_rdy_inotify.wait_watch(WAIT_FOR_DPU_READY, 1)
        if not dpu_rdy:
            if count > 1:
                time.sleep(1)
                self._power_off_force()
                return self._power_on_force(count=count - 1)
            self.log_error(f"Failed Force power on! Exiting")
            return False
        self.log_info(f"Force Power on Successful!")
        return True

    def _power_on(self):
        """Per DPU Power on without force private function"""
        self.write_file(self.pwr_path, OperationType.SET.value)
        self.write_file(self.rst_path, OperationType.SET.value)
        get_rdy_inotify = InotifyHelper(self.dpu_rdy_path)
        with self.time_check_context("power on"):
            dpu_rdy = get_rdy_inotify.wait_watch(WAIT_FOR_DPU_READY, 1)
        if not dpu_rdy:
            self.log_error(f"Failed power on! Trying Force Power on")
            self._power_off_force()
            return self._power_on_force()
        self.log_info(f"Power on Successful!")
        return True

    def dpu_pci_remove(self):
        """Per DPU PCI remove API"""
        try:
            for pci_dev_path in self.get_pci_dev_path():
                remove_path = os.path.join(pci_dev_path, "remove")
                if os.path.exists(remove_path):
                    self.write_file(remove_path, OperationType.SET.value)
            return True
        except Exception as e:
            self.log_error(f"Failed PCI Removal with error {e}")
        return False

    def dpu_pci_scan(self):
        """PCI Scan API"""
        try:
            pci_scan_path = "/sys/bus/pci/rescan"
            self.write_file(pci_scan_path, OperationType.SET.value)
            return True
        except Exception as e:
            self.log_error(f"Failed to rescan with error {e}")
        return False

    def dpu_power_on(self, forced=False, skip_pre_post=False):
        """Per DPU Power on API"""
        with self.boot_prog_context():
            self.log_info(f"Power on with force = {forced}")
            if self.read_boot_prog() == BootProgEnum.OS_RUN.value:
                self.log_info(f"Skipping DPU power on as DPU is already powered on")
                return_value = True
            elif forced:
                return_value = self._power_on_force()
            elif self.read_force_power_path() == int(OperationType.CLR.value):
                self.log_info(f"Power on with Force=True since power off force sysfs is cleared")
                return_value = self._power_on_force()
            else:
                return_value = self._power_on()
            if not skip_pre_post:
                self.dpu_post_startup()
            return return_value

    def dpu_power_off(self, forced=False, skip_pre_post=False):
        """Per DPU Power off API"""
        with self.boot_prog_context():
            if not skip_pre_post:
                self.dpu_pre_shutdown()
            self.log_info(f"Power off with force = {forced}")
            if self.read_boot_prog() == BootProgEnum.RST.value:
                self.log_info(f"Skipping DPU power off as DPU is already powered off")
                return True
            elif forced:
                return self._power_off_force()
            return self._power_off()

    def _reboot(self, no_wait):
        """Per DPU Reboot Private function API"""
        if not self.dpu_go_down():
            self._power_off_force()
        self.write_file(self.rst_path, OperationType.SET.value)
        if no_wait:
            self.log_debug("Exiting without checking result of reboot command")
            return True
        get_rdy_inotify = InotifyHelper(self.dpu_rdy_path)
        with self.time_check_context("power on"):
            dpu_rdy = get_rdy_inotify.wait_watch(WAIT_FOR_DPU_READY, 1)
        return_value = True
        if not dpu_rdy:
            self._power_off_force()
            return_value = self._power_on_force()
        return return_value

    def _reboot_force(self, no_wait):
        """Per DPU Force Reboot Private function API"""
        self._power_off_force()
        return_value = self._power_on_force(no_wait=no_wait)
        return return_value

    def dpu_reboot(self, forced=False, no_wait=False, skip_pre_post=False):
        """Per DPU Power on API"""
        with self.boot_prog_context():
            if not skip_pre_post:
                self.dpu_pre_shutdown()
            self.log_info(f"Reboot with force = {forced}")
            if forced:
                return_value = self._reboot_force(no_wait)
            else:
                return_value = self._reboot(no_wait)
            # No Post startup as well for no_wait call
            if (not no_wait) and (not skip_pre_post):
                self.dpu_post_startup()
            if return_value:
                self.log_info("Reboot Complete")
            return return_value

    def dpu_boot_prog_update(self, read_value=None):
        """Monitor and read changes to boot_progress sysfs file and map it to corresponding indication"""
        try:
            if read_value:
                self.boot_prog_state = read_value
            else:
                self.boot_prog_state = self.read_boot_prog()
            self.boot_prog_indication = f"{self.boot_prog_state} - {self.boot_prog_map.get(self.boot_prog_state,'N/A')}"
        except Exception as e:
            self.log_error(f"Could not update boot_progress of DPU")
            raise e

    def dpu_ready_update(self):
        """Monitor and read changes to dpu_ready sysfs file and map it to corresponding indication"""
        try:
            self.dpu_ready_state = utils.read_int_from_file(self.dpu_rdy_path,
                                                            raise_exception=True)
            self.dpu_ready_indication = f"{False if self.dpu_ready_state == 0 else True if self.dpu_ready_state == 1 else str(self.dpu_ready_state)+' - N/A'}"
        except Exception as e:
            self.log_error(f"Could not update dpu_ready for DPU")
            raise e

    def dpu_shtdn_ready_update(self):
        """Monitor and read changes to dpu_shtdn_ready sysfs file and map it to corresponding indication"""
        try:
            self.dpu_shtdn_ready_state = utils.read_int_from_file(self.shtdn_ready_path,
                                                                  raise_exception=True)
            self.dpu_shtdn_ready_indication = f"{False if self.dpu_shtdn_ready_state == 0 else True if self.dpu_shtdn_ready_state == 1 else str(self.dpu_shtdn_ready_state)+' - N/A'}"
        except Exception as e:
            self.log_error(f"Could not update dpu_shtdn_ready for DPU")
            raise e

    def dpu_status_update(self):
        """Update status for all the three relevant sysfs files for DPU monitoring"""
        try:
            self.dpu_boot_prog_update()
            self.dpu_ready_update()
            self.dpu_shtdn_ready_update()
        except Exception as e:
            self.log_error(f"Could not obtain status of DPU")
            raise e

    def read_boot_prog(self):
        return utils.read_int_from_file(self.boot_prog_path, raise_exception=True)

    def read_force_power_path(self):
        return utils.read_int_from_file(self.pwr_f_path, raise_exception=True)

    def update_boot_prog_once(self, poll_var):
        """Read boot_progress and update the value once """
        poll_var.poll()
        read_value = self.read_boot_prog()
        if read_value != self.boot_prog_state:
            self.dpu_boot_prog_update(read_value)
            self.log_error(f"The boot_progress status is changed to = {self.boot_prog_indication}")

    def watch_boot_prog(self):
        """Read boot_progress and update the value in an infinite loop"""
        try:
            self.dpu_boot_prog_update()
            self.log_info(f"The initial boot_progress status is = {self.boot_prog_indication}")
            file = open(self.boot_prog_path, "r")
            p = poll()
            p.register(file.fileno(), POLLPRI)
            while True:
                self.update_boot_prog_once(p)
        except Exception:
            self.log_error(f"Exception occured during watch_boot_progress!")

    @contextmanager
    def boot_prog_context(self):
        """Context manager for boot_progress update"""
        if self.verbosity:
            self.boot_prog_proc = None
            try:
                self.boot_prog_proc = multiprocessing.Process(target=self.watch_boot_prog)
                self.boot_prog_proc.start()
                yield
            except Exception:
                self.log_error(f"Exception occured during creating boot_prog_context manager!")
                yield
            finally:
                if self.boot_prog_proc and self.boot_prog_proc.is_alive():
                    self.boot_prog_proc.terminate()
                    self.boot_prog_proc.join()
        else:
            yield

    @contextmanager
    def time_check_context(self, msg):
        if self.verbosity:
            start_time = time.monotonic()
            yield
            end_time = time.monotonic()
            self.log_info(f"Total time taken = {end_time - start_time} for {msg}")
            return
        yield
