//
// SPDX-FileCopyrightText: NVIDIA CORPORATION & AFFILIATES
// Copyright (c) 2026 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
// SPDX-License-Identifier: Apache-2.0
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//


//! `sonic_platform/module.py`: `DpuModule`, one per DPU of a SmartSwitch.
//!
//! Its names and VPD fields, the operational status from boot progress, and
//! what chassisd drives: reboot and admin state through `DpuCtlPlat`, the reboot cause
//! from the MRSI register and hw-management's reset files, the midplane and
//! the PCI functions.  `Module`, the line card of a modular chassis, is
//! linecard.rs.

use std::sync::{Arc, Mutex, OnceLock};

use platform_base::logger::SysLogger;
use platform_base::py::{PyErr, PyResult, PyVal};
use platform_base::{BaseClass, PyObject};

use crate::dpuctlplat::{self, DpuCtlPlat, BOOT_PROG_OS_RUN};
use crate::vpd_parser::VpdParser;

const MODULE_TYPE_DPU: &str = "DPU";
const MODULE_STATUS_ONLINE: &str = "Online";
const MODULE_STATUS_OFFLINE: &str = "Offline";

/// The module's `logger = SysLogger()`, named after `sys.argv[0]` at import.
static LOGGER: OnceLock<SysLogger> = OnceLock::new();

/// `from .module import DpuModule`.
pub fn import() {
    crate::imports::sonic_platform_base();
    dpuctlplat::import();
    crate::imports::utils();
    crate::imports::device_data();
    crate::imports::vpd_parser();
    LOGGER.get_or_init(|| SysLogger::new(None));
}

fn logger() -> &'static SysLogger {
    LOGGER.get_or_init(|| SysLogger::new(None))
}

/// `DpuModule(dpu_id)`.
pub struct DpuModule {
    dpu_id: i128,
    name: String,
    pub dpuctl: DpuCtlPlat,
    vpd: VpdParser,
    vpd_prefix: String,
    midplane: Mutex<Option<String>>,
}

impl DpuModule {
    pub fn new(dpu_id: i128) -> Arc<DpuModule> {
        import();
        let name = format!("DPU{dpu_id}");
        let dpuctl = DpuCtlPlat::new(&name.to_lowercase());
        let vpd_prefix = dpuctl.hw_name.to_uppercase();
        Arc::new(DpuModule {
            dpu_id,
            dpuctl,
            vpd: VpdParser::new("/var/run/hw-management/eeprom/vpd_data"),
            vpd_prefix,
            name,
            midplane: Mutex::new(None),
        })
    }

    /// `get_oper_status()`.
    pub fn oper_status(&self) -> PyResult<&'static str> {
        Ok(if self.dpuctl.read_boot_prog()? == BOOT_PROG_OS_RUN {
            MODULE_STATUS_ONLINE
        } else {
            MODULE_STATUS_OFFLINE
        })
    }

    /// `DpuVpdParser.get_dpu_data(key)`: `<DPU<N+1>>_<key>`.
    fn dpu_data(&self, key: &str) -> PyVal {
        PyVal::Str(self.vpd.get_entry_value(&format!("{}_{key}", self.vpd_prefix)))
    }
}

impl PyObject for DpuModule {
    fn py_class(&self) -> &'static str {
        "DpuModule"
    }
    fn base_class(&self) -> BaseClass {
        BaseClass::ModuleBase
    }
    fn get_base_mac(&self) -> PyResult<PyVal> {
        Ok(self.dpu_data("BASE_MAC"))
    }
    fn get_model(&self) -> PyResult<PyVal> {
        Ok(self.dpu_data("PN"))
    }
    fn get_serial(&self) -> PyResult<PyVal> {
        Ok(self.dpu_data("SN"))
    }
    fn get_revision(&self) -> PyResult<PyVal> {
        Ok(self.dpu_data("REV"))
    }
    fn get_type(&self) -> PyResult<PyVal> {
        Ok(PyVal::str(MODULE_TYPE_DPU))
    }
    fn get_name(&self) -> PyResult<PyVal> {
        Ok(PyVal::Str(self.name.clone()))
    }
    fn get_description(&self) -> PyResult<PyVal> {
        Ok(PyVal::str("NVIDIA BlueField-3 DPU"))
    }
    fn get_oper_status(&self) -> PyResult<PyVal> {
        Ok(PyVal::str(self.oper_status()?))
    }
    fn get_dpu_id(&self) -> PyResult<PyVal> {
        Ok(PyVal::Int(self.dpu_id))
    }

    /// `reboot(reboot_type)`: a DPU reboot waits for the DPU to come back;
    /// a SmartSwitch one -- the NPU going down too -- does not.
    fn reboot(&self, reboot_type: &str) -> PyResult<PyVal> {
        logger().log_notice(&format!("Rebooting {} with type {reboot_type}", self.name));
        let rv = match reboot_type {
            "DPU" => self.dpuctl.dpu_reboot(false, false, true)?,
            "SMARTSWITCH" => self.dpuctl.dpu_reboot(false, true, true)?,
            other => return Err(PyErr::runtime_error(format!("Reboot called with unsupported reboot_type = {other}"))),
        };
        logger().log_notice(&format!(
            "Rebooted {} with type {reboot_type} and return value {}",
            self.name,
            PyVal::Bool(rv).py_str()
        ));
        Ok(PyVal::Bool(rv))
    }

    /// `set_admin_state(up)`: power the DPU on or off, PCI left to the
    /// caller's pre-shutdown and post-startup.  `up` as the caller passed it,
    /// and logged that way: chassisd's 1 or 0, the base class's True or False.
    fn set_admin_state(&self, up: PyVal) -> PyResult<PyVal> {
        let up_s = up.py_str();
        logger().log_notice(&format!("Setting the admin state for {} to {up_s}", self.name));
        if up.truthy() {
            if self.dpuctl.dpu_power_on(false, true)? {
                logger().log_notice(&format!("Completed the admin state change for {} to {up_s}", self.name));
                return Ok(PyVal::Bool(true));
            }
            logger().log_error(&format!("Failed to set the admin state for {}", self.name));
            return Ok(PyVal::Bool(false));
        }
        let rv = self.dpuctl.dpu_power_off(false, true)?;
        logger().log_notice(&format!("Completed the admin state change for {} to {up_s}", self.name));
        Ok(PyVal::Bool(rv))
    }

    /// `get_reboot_cause()`: a watchdog reset as the DPU's MRSI register
    /// reports it, else the first hw-management reset file that is set.
    fn get_reboot_cause(&self) -> PyResult<PyVal> {
        let dev = crate::dpuctlplat::dpu_interface(&self.name.to_lowercase(), "bus_info")?;
        let Some(PyVal::Str(dev)) = dev else {
            return Err(PyErr::type_error(format!(
                "expected str, bytes or os.PathLike object, not {}",
                dev.map_or_else(|| "NoneType".to_string(), |v| v.type_name())
            )));
        };
        let pcie_path = platform_base::io::join(crate::dpuctlplat::PCI_DEV_BASE, &dev);
        if platform_base::io::exists(&pcie_path) {
            let argv = ["mlxreg", "-d", dev.as_str(), "--reg_name", "MRSI", "-g", "-indexes", "device=1"];
            let out = command(argv[0])
                .args(&argv[1..])
                .stdin(std::process::Stdio::inherit())
                .stderr(std::process::Stdio::inherit())
                .output()
                .map_err(|e| PyErr::from_io(&e, Some("mlxreg")))?;
            let op = if out.status.success() {
                out.stdout
            } else {
                let e = platform_base::subprocess::called_process_error(
                    &argv,
                    platform_base::subprocess::returncode(out.status),
                );
                logger().log_error(&format!("Failed to check watchdog reason for {}! {}", self.name, e.str()));
                Vec::new()
            };
            let text = platform_base::py::codec::decode(platform_base::py::codec::Codec::Utf8, &op)?;
            let mut reason: Option<String> = None;
            for line in text.split('\n') {
                if line.contains("reset_reason") {
                    let field = line.split('|').nth(1).ok_or_else(|| PyErr::index_error("list index out of range"))?;
                    reason = Some(platform_base::py::strip(field).to_string());
                    break;
                }
            }
            if let Some(r) = reason.filter(|r| !r.is_empty()) {
                if platform_base::pcie::int_hex(&PyVal::Str(r))? == MLX_DPU_REBOOT_CAUSE_WATCHDOG {
                    logger().log_notice(&format!("Reset reason for {} is Watchdog", self.name));
                    return Ok(cause("Watchdog", "Watchdog reboot"));
                }
            }
        }
        if let Some((c, d)) = self.reset_file_cause(true) {
            return Ok(cause(c, d));
        }
        Ok(cause("Non-Hardware", ""))
    }

    /// `get_midplane_down_reason()`: the same reset files.
    fn get_midplane_down_reason(&self) -> PyResult<PyVal> {
        if let Some((c, d)) = self.reset_file_cause(false) {
            return Ok(cause(c, d));
        }
        Ok(cause("Non-Hardware", ""))
    }

    fn get_midplane_ip(&self) -> PyResult<PyVal> {
        Ok(PyVal::Str(format!("169.254.200.{}", self.dpu_id + 1)))
    }

    /// `is_midplane_reachable()`: the midplane link up, and one ping
    /// answered within a second.
    fn is_midplane_reachable(&self) -> PyResult<PyVal> {
        let iface = self.midplane_interface()?;
        if crate::utils::read_int(&format!("/sys/class/net/{iface}/carrier"), 0, crate::utils::Log::Off) != 1 {
            return Ok(PyVal::Bool(false));
        }
        let ip = format!("169.254.200.{}", self.dpu_id + 1);
        match command("ping")
            .args(["-c", "1", "-W", "1", &ip])
            .stdout(std::process::Stdio::null())
            .stderr(std::process::Stdio::null())
            .status()
        {
            Ok(st) => Ok(PyVal::Bool(st.code() == Some(0))),
            Err(e) => {
                let e = PyErr::from_io(&e, Some("ping"));
                logger().log_error(&format!("Failed to check midplane reachability for {}! {}", self.name, e.str()));
                Ok(PyVal::Bool(false))
            }
        }
    }

    /// `get_pci_bus_info()`: the two functions' addresses, cached once known.
    fn get_pci_bus_info(&self) -> PyResult<PyVal> {
        let (pci, rshim) = self.dpuctl.pci_dev_paths()?;
        let last = |p: &str| PyVal::str(p.rsplit('/').next().unwrap_or(""));
        Ok(PyVal::List(vec![last(&pci), last(&rshim)]))
    }

    fn pci_detach(&self) -> PyResult<PyVal> {
        Ok(PyVal::Bool(self.dpuctl.dpu_pci_remove()))
    }

    fn pci_reattach(&self) -> PyResult<PyVal> {
        Ok(PyVal::Bool(self.dpuctl.dpu_pci_scan()))
    }
}

const MLX_DPU_REBOOT_CAUSE_WATCHDOG: i128 = 2;

/// Where a test keeps stand-ins for `mlxreg` and `ping`, each run as
/// `sh <file>`; one the directory lacks is not found.  A switch has no such
/// directory and finds the tools on PATH, as Python's subprocess does.
#[cfg(test)]
pub(crate) static TOOL_DIR: Mutex<Option<std::path::PathBuf>> = Mutex::new(None);

/// The `subprocess` program `prog`.
fn command(prog: &str) -> std::process::Command {
    #[cfg(test)]
    if let Some(dir) = TOOL_DIR.lock().ok().and_then(|d| d.clone()) {
        let script = dir.join(prog);
        if script.is_file() {
            let mut c = std::process::Command::new("sh");
            c.arg(script);
            return c;
        }
        return std::process::Command::new(script);
    }
    std::process::Command::new(prog)
}

fn cause(c: &str, d: &str) -> PyVal {
    PyVal::Tuple(vec![PyVal::str(c), PyVal::str(d)])
}

impl DpuModule {
    /// `reboot_cause_map`, in its order: the first reset file reading 1.
    fn reset_file_cause(&self, reboot: bool) -> Option<(&'static str, &'static str)> {
        let base = format!("/var/run/hw-management/{}/system/", self.dpuctl.hw_name);
        let map: [(&str, &str, &str); 5] = [
            ("reset_aux_pwr_or_reload", "Power Loss", "power auxiliary outage or reload"),
            ("reset_comex_pwr_fail", "Power Loss", "Power failed to comex module"),
            ("reset_from_main_board", "Non-Hardware", "Reset from Main board"),
            ("reset_dpu_thermal", "Thermal Overload: Other", "Thermal shutdown of the DPU"),
            ("reset_pwr_off", "Non-Hardware", "Reset due to Power off"),
        ];
        for (file, c, d) in map {
            if crate::utils::read_int(&format!("{base}{file}"), 0, crate::utils::Log::Error) == 1 {
                let what = if reboot { "Reset reason" } else { "Midplane down reason" };
                logger().log_notice(&format!("{what} for {} is {c}", self.name));
                return Some((c, d));
            }
        }
        None
    }

    /// `get_midplane_interface()`: from platform.json, cached once found.
    fn midplane_interface(&self) -> PyResult<String> {
        if let Some(i) = crate::utils::lock(&self.midplane).as_ref() {
            return Ok(i.clone());
        }
        let i = crate::dpuctlplat::dpu_interface(&self.name.to_lowercase(), "midplane_interface")?
            .filter(PyVal::truthy)
            .ok_or_else(|| {
                PyErr::runtime_error(format!(
                    "Unable to obtain midplane interface information from platform.json for {}",
                    self.name
                ))
            })?
            .py_str();
        *crate::utils::lock(&self.midplane) = Some(i.clone());
        Ok(i)
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::test_support_d::{platform, Syslog};
    use crate::testing::Root;

    const DPUS: &str = r#"{"DPUS": {"dpu0": {"bus_info": "0000:08:00.0", "rshim_bus_info": "0000:08:00.1", "midplane_interface": "dpu0"}}}"#;
    const SYS: &str = "/var/run/hw-management/system";
    const RESET: &str = "/var/run/hw-management/dpu1/system";

    /// DPU0 booted to `boot`, every hw-management file in place, and the
    /// events saying ready.
    fn dpu0(root: &Root, boot: &str, ready: &str) -> Arc<DpuModule> {
        platform(root, "x86_64-nvidia_sn4280-r0", DPUS);
        root.write(&format!("{RESET}/boot_progress"), boot);
        for f in ["dpu1_rst", "dpu1_pwr"] {
            root.write(&format!("{SYS}/{f}"), "0");
        }
        root.write(&format!("{SYS}/dpu1_pwr_force"), "1");
        root.write("/var/run/hw-management/events/dpu1_ready", ready);
        root.write("/var/run/hw-management/events/dpu1_shtdn_ready", "1");
        DpuModule::new(0)
    }

    /// Stand-ins for `mlxreg` and `ping`, forgotten when dropped.
    struct Tools(std::path::PathBuf);

    impl Tools {
        fn new(root: &Root) -> Tools {
            let dir = root.dir.join("tools");
            std::fs::create_dir_all(&dir).unwrap();
            *TOOL_DIR.lock().unwrap() = Some(dir.clone());
            Tools(dir)
        }
        fn tool(&self, name: &str, script: &str) {
            std::fs::write(self.0.join(name), script).unwrap();
        }
        fn remove(&self, name: &str) {
            let _ = std::fs::remove_file(self.0.join(name));
        }
    }

    impl Drop for Tools {
        fn drop(&mut self) {
            *TOOL_DIR.lock().unwrap() = None;
        }
    }

    fn module_lines(log: &Syslog) -> Vec<String> {
        log.take().into_iter().filter(|l| l.contains("DPU0")).collect()
    }

    fn s(v: PyResult<PyVal>) -> String {
        match v {
            Ok(v) => v.py_repr(),
            Err(e) => format!("{}: {}", e.exc.name(), e.str()),
        }
    }

    #[test]
    fn a_dpu_is_named_and_described_from_its_vpd() {
        let root = Root::new();
        let m = dpu0(&root, "5", "1");
        root.write(
            "/var/run/hw-management/eeprom/vpd_data",
            "DPU1_BASE_MAC: 00:11:22:33:44:55\nDPU1_PN: 900-9D3B4\nDPU1_SN: MT2401X00042\nDPU2_REV: B1\n",
        );
        assert_eq!(
            [m.get_base_mac(), m.get_model(), m.get_serial(), m.get_revision()].map(s),
            ["'00:11:22:33:44:55'", "'900-9D3B4'", "'MT2401X00042'", "'N/A'"]
        );
        assert_eq!(
            [m.get_type(), m.get_name(), m.get_description(), m.get_dpu_id(), m.get_midplane_ip()].map(s),
            ["'DPU'", "'DPU0'", "'NVIDIA BlueField-3 DPU'", "0", "'169.254.200.1'"]
        );
        assert_eq!(m.py_class(), "DpuModule");
        assert_eq!(m.base_class(), BaseClass::ModuleBase);
        assert_eq!(s(m.get_oper_status()), "'Online'");
        root.write(&format!("{RESET}/boot_progress"), "3");
        assert_eq!(s(m.get_oper_status()), "'Offline'");
        assert_eq!(s(m.get_pci_bus_info()), "['0000:08:00.0', '0000:08:00.1']");
        assert_eq!((s(m.pci_detach()), s(m.pci_reattach())), ("True".into(), "True".into()));
    }

    #[test]
    fn reboot_and_admin_state_drive_the_dpu() {
        let root = Root::new();
        let log = Syslog::listen(&root);
        let m = dpu0(&root, "5", "1");
        assert_eq!(s(m.reboot("DPU")), "True");
        assert_eq!(s(m.reboot("SMARTSWITCH")), "True");
        assert_eq!(s(m.reboot("CPU")), "RuntimeError: Reboot called with unsupported reboot_type = CPU");
        let l = module_lines(&log);
        for want in [
            "NOTICE Rebooting DPU0 with type DPU",
            "NOTICE Rebooted DPU0 with type DPU and return value True",
            "NOTICE Rebooted DPU0 with type SMARTSWITCH and return value True",
            "NOTICE Rebooting DPU0 with type CPU",
        ] {
            assert!(l.contains(&want.to_string()), "{want}: {l:?}");
        }
        // Up while running: nothing to do.  Down: graceful.
        assert_eq!(s(m.set_admin_state(PyVal::Bool(true))), "True");
        assert_eq!(s(m.set_admin_state(PyVal::Bool(false))), "True");
        let l = module_lines(&log);
        assert!(l.contains(&"NOTICE Setting the admin state for DPU0 to True".to_string()));
        assert!(l.contains(&"NOTICE Completed the admin state change for DPU0 to True".to_string()));
        assert!(l.contains(&"NOTICE Completed the admin state change for DPU0 to False".to_string()));
        // chassisd hands it 1 and 0, and that is what the log says.
        assert_eq!(s(m.set_admin_state(PyVal::Int(1))), "True");
        assert!(module_lines(&log).contains(&"NOTICE Setting the admin state for DPU0 to 1".to_string()));
        // Up, and it never comes: False.
        root.write(&format!("{RESET}/boot_progress"), "0");
        root.write("/var/run/hw-management/events/dpu1_ready", "0");
        assert_eq!(s(m.set_admin_state(PyVal::Bool(true))), "False");
        assert!(module_lines(&log).contains(&"ERROR Failed to set the admin state for DPU0".to_string()));
    }

    #[test]
    fn a_reboot_cause_is_the_watchdog_or_the_first_reset_file_set() {
        let root = Root::new();
        let log = Syslog::listen(&root);
        let m = dpu0(&root, "5", "1");
        let tools = Tools::new(&root);
        // No PCI device: the reset files decide.
        assert_eq!(s(m.get_reboot_cause()), "('Non-Hardware', '')");
        assert_eq!(s(m.get_midplane_down_reason()), "('Non-Hardware', '')");
        root.write(&format!("{RESET}/reset_pwr_off"), "1");
        root.write(&format!("{RESET}/reset_dpu_thermal"), "1");
        assert_eq!(s(m.get_reboot_cause()), "('Thermal Overload: Other', 'Thermal shutdown of the DPU')");
        assert_eq!(s(m.get_midplane_down_reason()), "('Thermal Overload: Other', 'Thermal shutdown of the DPU')");
        root.write(&format!("{RESET}/reset_aux_pwr_or_reload"), "1");
        assert_eq!(s(m.get_reboot_cause()), "('Power Loss', 'power auxiliary outage or reload')");
        let l = module_lines(&log);
        assert!(l.contains(&"NOTICE Reset reason for DPU0 is Thermal Overload: Other".to_string()));
        assert!(l.contains(&"NOTICE Midplane down reason for DPU0 is Thermal Overload: Other".to_string()));
        // The PCI device there: the DPU's MRSI register first.
        root.write("/sys/bus/pci/devices/0000:08:00.0/x", "");
        tools.tool("mlxreg", "echo 'Field Name   | Data'\necho 'reset_reason | 0x00000002'\n");
        assert_eq!(s(m.get_reboot_cause()), "('Watchdog', 'Watchdog reboot')");
        assert!(module_lines(&log).contains(&"NOTICE Reset reason for DPU0 is Watchdog".to_string()));
        tools.tool("mlxreg", "echo 'reset_reason | 0x1'\n");
        assert_eq!(s(m.get_reboot_cause()), "('Power Loss', 'power auxiliary outage or reload')");
        tools.tool("mlxreg", "echo 'reset_reason |   '\n");
        assert_eq!(s(m.get_reboot_cause()), "('Power Loss', 'power auxiliary outage or reload')");
        tools.tool("mlxreg", "echo 'reset_reason 2'\n");
        assert_eq!(s(m.get_reboot_cause()), "IndexError: list index out of range");
        tools.tool("mlxreg", "echo 'reset_reason | zz'\n");
        assert_eq!(s(m.get_reboot_cause()), "ValueError: invalid literal for int() with base 16: 'zz'");
        tools.tool("mlxreg", "exit 3\n");
        assert_eq!(s(m.get_reboot_cause()), "('Power Loss', 'power auxiliary outage or reload')");
        assert!(module_lines(&log).contains(&"ERROR Failed to check watchdog reason for DPU0! Command '['mlxreg', '-d', '0000:08:00.0', '--reg_name', 'MRSI', '-g', '-indexes', 'device=1']' returned non-zero exit status 3.".to_string()));
        // No mlxreg at all: what subprocess raises.
        tools.remove("mlxreg");
        assert_eq!(s(m.get_reboot_cause()), "FileNotFoundError: [Errno 2] No such file or directory: 'mlxreg'");
    }

    #[test]
    fn without_a_pci_id_the_reboot_cause_cannot_join_its_path() {
        let root = Root::new();
        platform(&root, "x86_64-nvidia_sn4280-r0", r#"{"DPUS": {"dpu0": {}}}"#);
        let m = DpuModule::new(0);
        assert_eq!(s(m.get_reboot_cause()), "TypeError: expected str, bytes or os.PathLike object, not NoneType");
        assert_eq!(
            s(m.is_midplane_reachable()),
            "RuntimeError: Unable to obtain midplane interface information from platform.json for DPU0"
        );
    }

    #[test]
    fn the_midplane_is_reachable_when_its_link_is_up_and_a_ping_answers() {
        let root = Root::new();
        let log = Syslog::listen(&root);
        let m = dpu0(&root, "5", "1");
        let tools = Tools::new(&root);
        assert_eq!(s(m.is_midplane_reachable()), "False");
        root.write("/sys/class/net/dpu0/carrier", "1");
        tools.tool("ping", "[ \"$*\" = '-c 1 -W 1 169.254.200.1' ]\n");
        assert_eq!(s(m.is_midplane_reachable()), "True");
        tools.tool("ping", "exit 1\n");
        assert_eq!(s(m.is_midplane_reachable()), "False");
        tools.remove("ping");
        assert_eq!(s(m.is_midplane_reachable()), "False");
        assert!(module_lines(&log).contains(
            &"ERROR Failed to check midplane reachability for DPU0! [Errno 2] No such file or directory: 'ping'".to_string()
        ));
        // The interface is read once.
        platform(&root, "x86_64-nvidia_sn4280-r0", r#"{"DPUS": {"dpu0": {}}}"#);
        crate::utils::forget_caches();
        assert_eq!(s(m.is_midplane_reachable()), "False");
    }
}
