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



//! `sonic_platform/bmc.py`: the switch's BMC -- BMCBase with the NOS
//! account's name from the build configuration and its password from the
//! TPM, which hw-management's `hw_management_redfish_client.py` derives.
//!
//! That module is hw-management's and runs as it is, through the embedded
//! interpreter, loaded from its path as `bmc.py` loads it (see
//! [`crate::hw_mgmt`]).  hw-management installs it in the host's /usr/bin,
//! which pmon does not have, so in pmon the load fails as Python's does, and
//! with it the TPM password and the NOS account's provisioning.  None of the
//! pmon daemons reaches the BMC.
//!
//! The first login that the BMC refuses (401) tries to provision the NOS
//! account through that module, once per BMC object.
//!
//! The BMC's firmware ID is whichever known ID its Redfish inventory
//! answers a version for, cached once found.

use std::sync::atomic::{AtomicBool, Ordering};
use std::sync::{Arc, Mutex};

use platform_base::bmc::{Bmc as BaseBmc, BmcVendor};
use platform_base::py::{ops, PyResult, PyVal};
use platform_base::redfish_client::{self as rf, is_exception};
use platform_base::device_info;

use crate::device_data;
use crate::utils;

/// `BMC_FIRMWARE_IDS`: in resolution order.
const BMC_FIRMWARE_IDS: [&str; 2] = ["FW_BMC_0", "MGX_FW_BMC_0"];
const BMC_EEPROM_ID: &str = "BMC_eeprom";

/// `BMC`: BMCBase with the Mellanox callbacks.
pub type Bmc = BaseBmc<MlnxBmc>;

pub struct MlnxBmc {
    bmc_nos_account_username: PyVal,
    bmc_root_account_default_password: PyVal,
    /// `_nos_account_provisioning_tried`.
    provisioning_tried: AtomicBool,
    /// `_firmware_id`: None until an ID is resolved.
    firmware_id: Mutex<PyVal>,
    /// `_get_tpm_password`, which a test replaces.
    tpm_password: fn() -> PyResult<PyVal>,
}

/// `BMC._instance`.
static INSTANCE: Mutex<Option<Arc<Bmc>>> = Mutex::new(None);

/// `_get_bmc_values()`: address, NOS user and root's default password, or
/// None with the reason logged.
fn get_bmc_values() -> PyResult<Option<(PyVal, PyVal, PyVal)>> {
    if !device_data::is_platform_with_bmc()? {
        return Ok(None);
    }
    let bmc_data = device_info::get_bmc_data().unwrap_or(PyVal::None);
    let bmc_addr = ops::get(&bmc_data, "bmc_addr", PyVal::None)?;
    if !bmc_addr.truthy() {
        utils::log_error("BMC address not found in bmc_data");
        return Ok(None);
    }
    let bmc_config = device_info::get_bmc_build_config().unwrap_or(PyVal::None);
    if !bmc_config.truthy() {
        utils::log_error("BMC build configuration not found");
        return Ok(None);
    }
    let user = ops::get(&bmc_config, "bmc_nos_account_username", PyVal::None)?;
    if !user.truthy() {
        utils::log_error("BMC NOS account username not found in build configuration");
        return Ok(None);
    }
    let root_pw = ops::get(&bmc_config, "bmc_root_account_default_password", PyVal::None)?;
    Ok(Some((bmc_addr, user, root_pw)))
}

/// `BMC.get_instance()`: built once there is a BMC to build; asked again
/// -- and logging why not again -- until then.
pub fn get_instance() -> PyResult<Option<Arc<Bmc>>> {
    crate::imports::bmc();
    let mut inst = utils::lock(&INSTANCE);
    if inst.is_none() {
        let Some((addr, user, root_pw)) = get_bmc_values()? else {
            return Ok(None);
        };
        *inst = Some(Arc::new(BaseBmc::new(
            addr,
            MlnxBmc {
                bmc_nos_account_username: user,
                bmc_root_account_default_password: root_pw,
                provisioning_tried: AtomicBool::new(false),
                firmware_id: Mutex::new(PyVal::None),
                tpm_password,
            },
        )));
    }
    Ok(inst.clone())
}

/// `BMC._instance = None`, for a test that starts on another platform.
#[cfg(test)]
pub(crate) fn forget_instance() {
    *utils::lock(&INSTANCE) = None;
}

/// A BMC at 169.254.0.1 whose Redfish client runs `curl_path`, with a
/// fixed password for its NOS user, for a test.
#[cfg(test)]
pub(crate) fn test_bmc(curl_path: &str) -> Bmc {
    BaseBmc::with_curl(
        PyVal::str("169.254.0.1"),
        MlnxBmc {
            bmc_nos_account_username: PyVal::str("nosuser"),
            bmc_root_account_default_password: PyVal::str("0penBmc"),
            provisioning_tried: AtomicBool::new(false),
            firmware_id: Mutex::new(PyVal::None),
            tpm_password: || Ok(PyVal::str("tpm-pw")),
        },
        curl_path,
    )
}

/// `_get_tpm_password()`: `BMCAccessor().get_login_password()`; a failure
/// is logged and raised again.
fn tpm_password() -> PyResult<PyVal> {
    crate::hw_mgmt::redfish_login_password().inspect_err(|e| {
        utils::log_error(&format!("Error getting TPM password from hw_management_redfish_client.py: {}", e.str()));
    })
}

impl MlnxBmc {

    /// `_configure_nos_account()`: hw-management's `BMCAccessor().login()`
    /// provisions the account.  What it prints is kept from the CLI's output
    /// and logged a line at a time -- after the error, if it raised, as the
    /// `finally` does -- and anything it answers but 0 is a failure.
    fn configure_nos_account(&self) -> bool {
        utils::log_notice("Configuring the BMC NOS account from hw_management_redfish_client.py");
        let login = crate::hw_mgmt::redfish_login();
        if let Err(e) = &login.result {
            utils::log_error(&format!(
                "Error configuring the BMC NOS account from hw_management_redfish_client.py: {}",
                e.str()
            ));
        }
        for line in &login.printed {
            utils::log_notice(&format!("hw_management_redfish_client: {line}"));
        }
        let Ok((ret, failed)) = login.result else {
            return false;
        };
        if failed {
            utils::log_error(&format!(
                "Failed to configure the BMC NOS account from hw_management_redfish_client.py: {ret}"
            ));
            return false;
        }
        utils::log_notice("BMC NOS account configured successfully");
        true
    }
}

/// `_get_firmware_inventory_entry()`: `(fw_id, version)` of the first known
/// ID with a version, or `(None, None)`.  It logs in and out itself unless
/// the caller already has a session; an exception is logged and answers
/// `(None, None)`, but one from the logout in `finally` escapes.
fn get_firmware_inventory_entry(bmc: &Bmc) -> PyResult<(PyVal, PyVal)> {
    let caller_has_session = bmc.rf_client.has_login();
    let r = (|| -> PyResult<(PyVal, PyVal)> {
        if !caller_has_session {
            let ret = bmc.login()?;
            if ret != rf::ERR_CODE_OK {
                utils::log_error(&format!("Failed to get the BMC firmware inventory: {ret}"));
                return Ok((PyVal::None, PyVal::None));
            }
        }
        for fw_id in BMC_FIRMWARE_IDS {
            let (ret, version) = bmc.rf_client.redfish_api_get_firmware_version(bmc, fw_id)?;
            // `version != 'N/A'`: any other type is unequal.
            if ret == rf::ERR_CODE_OK && version.truthy() && !version.py_eq(&PyVal::str("N/A")) {
                return Ok((PyVal::str(fw_id), version));
            }
        }
        let ids = PyVal::List(BMC_FIRMWARE_IDS.iter().map(|id| PyVal::str(*id)).collect());
        utils::log_error(&format!(
            "None of the known BMC firmware IDs {} reported a version from the BMC firmware inventory",
            ids.py_repr()
        ));
        Ok((PyVal::None, PyVal::None))
    })();
    let r = match r {
        Err(e) if is_exception(&e) => {
            utils::log_error(&format!("Exception in _get_firmware_inventory_entry: {}", e.str()));
            Ok((PyVal::None, PyVal::None))
        }
        other => other,
    };
    if !caller_has_session {
        bmc.logout()?;
    }
    r
}

impl BmcVendor for MlnxBmc {
    fn py_class(&self) -> &'static str {
        "BMC"
    }
    fn get_login_user(&self) -> PyResult<PyVal> {
        Ok(self.bmc_nos_account_username.clone())
    }
    fn get_login_password(&self) -> PyResult<PyVal> {
        (self.tpm_password)()
    }
    fn get_default_root_password(&self) -> PyResult<PyVal> {
        Ok(self.bmc_root_account_default_password.clone())
    }
    /// `get_firmware_id()`: resolved on first use and cached; while it does
    /// not resolve, `BMC_FIRMWARE_IDS[0]`, asked again next time.  The
    /// lock is not held across the Redfish queries.
    fn get_firmware_id(bmc: &Bmc) -> PyResult<PyVal> {
        let lock = || utils::lock(&bmc.vendor.firmware_id);
        let mut fw_id = lock().clone();
        if fw_id.is_none() {
            fw_id = get_firmware_inventory_entry(bmc)?.0;
            *lock() = fw_id.clone();
        }
        Ok(if fw_id.truthy() { fw_id } else { PyVal::str(BMC_FIRMWARE_IDS[0]) })
    }
    /// `get_version()`: the version the inventory lookup already carries.
    fn get_version(bmc: &Bmc) -> PyResult<PyVal> {
        let (fw_id, version) = get_firmware_inventory_entry(bmc)?;
        Ok(if fw_id.truthy() { version } else { PyVal::str("N/A") })
    }
    fn get_eeprom_id(&self) -> PyResult<PyVal> {
        Ok(PyVal::str(BMC_EEPROM_ID))
    }
    /// `BMC._login()`: a 401 on the first login is the NOS account not
    /// provisioned yet, so it is not an error until provisioning fails.
    fn login(bmc: &Bmc) -> PyResult<i128> {
        let v = &bmc.vendor;
        if v.provisioning_tried.load(Ordering::Relaxed) {
            return bmc.base_login();
        }
        let ret = bmc.rf_client.login(bmc, false)?;
        if ret == rf::ERR_CODE_OK {
            return Ok(ret);
        }
        if ret != rf::ERR_CODE_AUTH_FAILURE {
            utils::log_error(&format!("Failed to login to the BMC: {ret}"));
            return Ok(ret);
        }
        v.provisioning_tried.store(true, Ordering::Relaxed);
        if !v.configure_nos_account() {
            return Ok(ret);
        }
        bmc.base_login()
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use platform_base::py::PyErr;
    use platform_base::testing::FakeCurl;
    use platform_base::PyObject;

    const NEW: &str = "/redfish/v1/UpdateService/FirmwareInventory/FW_BMC_0";
    const OLD: &str = "/redfish/v1/UpdateService/FirmwareInventory/MGX_FW_BMC_0";
    const LOGIN: &str = "POST /redfish/v1/SessionService/Sessions";
    const LOGOUT: &str = "DELETE /redfish/v1/SessionService/Sessions/s1";

    fn tpm() -> PyResult<PyVal> {
        Ok(PyVal::str("tpm-pw"))
    }

    fn no_tpm() -> PyResult<PyVal> {
        Err(PyErr::other("Exception", "no TPM"))
    }

    fn bmc(curl_path: &str, tpm_password: fn() -> PyResult<PyVal>) -> Bmc {
        BaseBmc::with_curl(
            PyVal::str("169.254.0.1"),
            MlnxBmc {
                bmc_nos_account_username: PyVal::str("nosuser"),
                bmc_root_account_default_password: PyVal::str("0penBmc"),
                provisioning_tried: AtomicBool::new(false),
                firmware_id: Mutex::new(PyVal::None),
                tpm_password,
            },
            curl_path,
        )
    }

    fn get(path: &str) -> String {
        format!("GET {path}")
    }

    #[test]
    fn new_bmc_firmware_is_found_first_and_its_id_kept() {
        let curl = FakeCurl::new();
        curl.reply("GET", NEW, 200, r#"{"Version": "89.0001.0100"}"#);
        let b = bmc(&curl.path(), tpm);
        assert_eq!(b.get_firmware_id().unwrap().py_repr(), "'FW_BMC_0'");
        assert_eq!(curl.take_requests(), [LOGIN, &get(NEW), LOGOUT]);
        assert_eq!(b.get_firmware_id().unwrap().py_repr(), "'FW_BMC_0'");
        assert!(curl.take_requests().is_empty());
        // The version is asked each time.
        assert_eq!(b.get_version().unwrap().py_repr(), "'89.0001.0100'");
        assert_eq!(curl.take_requests(), [LOGIN, &get(NEW), LOGOUT]);
    }

    #[test]
    fn old_bmc_firmware_is_found_second() {
        let curl = FakeCurl::new();
        curl.reply("GET", OLD, 200, r#"{"Version": "88.0002.1200"}"#);
        let b = bmc(&curl.path(), tpm);
        assert_eq!(b.get_version().unwrap().py_repr(), "'88.0002.1200'");
        assert_eq!(curl.take_requests(), [LOGIN, &get(NEW), &get(OLD), LOGOUT]);
        // A 400 for the new ID, as some BMCs answer, is passed over too.
        curl.reply("GET", NEW, 400, r#"{"error": {"code": "Base.1.18.ResourceMissingAtURI"}}"#);
        assert_eq!(b.get_firmware_id().unwrap().py_repr(), "'MGX_FW_BMC_0'");
    }

    #[test]
    fn without_a_version_the_first_id_is_named_and_asked_again() {
        let curl = FakeCurl::new();
        // 'N/A' and '' are no version.
        curl.reply("GET", NEW, 200, r#"{"Version": "N/A"}"#);
        curl.reply("GET", OLD, 200, r#"{"Version": ""}"#);
        let b = bmc(&curl.path(), tpm);
        assert_eq!(b.get_version().unwrap().py_repr(), "'N/A'");
        assert_eq!(b.get_firmware_id().unwrap().py_repr(), "'FW_BMC_0'");
        curl.take_requests();
        assert_eq!(b.get_firmware_id().unwrap().py_repr(), "'FW_BMC_0'");
        assert_eq!(curl.take_requests(), [LOGIN, &get(NEW), &get(OLD), LOGOUT]);
    }

    #[test]
    fn inside_a_session_the_lookup_neither_logs_in_nor_out() {
        let curl = FakeCurl::new();
        curl.reply("GET", NEW, 200, r#"{"Version": "89.0001.0100"}"#);
        let b = bmc(&curl.path(), tpm);
        assert_eq!(b.login().unwrap(), rf::ERR_CODE_OK);
        curl.take_requests();
        assert_eq!(b.get_firmware_id().unwrap().py_repr(), "'FW_BMC_0'");
        assert_eq!(curl.take_requests(), [get(NEW)]);
        assert!(b.rf_client.has_login());
        // A firmware update resolves the ID inside its own session.
        let b = bmc(&curl.path(), tpm);
        let _ = b.update_firmware("/nonexistent/bmc.fwpkg").unwrap();
        assert_eq!(
            curl.take_requests(),
            [LOGIN, &get(NEW), "POST /redfish/v1/UpdateService/update-multipart", LOGOUT]
        );
    }

    /// In pmon the client is not there and the load raises, as Python's
    /// does; on the host it is loaded and asked.  A client that failed to
    /// run stays in sys.modules, half loaded, as Python leaves it.
    #[test]
    fn the_tpm_password_is_hw_management_s_client_s() {
        let root = crate::testing::Root::new();
        let e = tpm_password().unwrap_err();
        assert_eq!(
            (e.exc.name(), e.str().as_str()),
            ("ImportError", "hw_management_redfish_client not found at /usr/bin/hw_management_redfish_client.py")
        );
        crate::hw_mgmt::testing::install_redfish_client(&root);
        assert_eq!(tpm_password().unwrap(), PyVal::str("tpm-derived"));
        crate::hw_mgmt::testing::redfish_set("password_error", "'tpm2_createprimary failed'");
        utils::tests::LOGGED.with(|l| l.borrow_mut().clear());
        let e = tpm_password().unwrap_err();
        assert_eq!((e.exc.name(), e.str().as_str()), ("Exception", "tpm2_createprimary failed"));
        assert_eq!(
            utils::tests::LOGGED.with(|l| l.borrow().clone()),
            ["Error getting TPM password from hw_management_redfish_client.py: tpm2_createprimary failed"]
        );

        drop(root);
        let root = crate::testing::Root::new();
        root.write(crate::hw_mgmt::REDFISH_CLIENT_PATH, "raise RuntimeError('broken')\n");
        assert_eq!(tpm_password().unwrap_err().str(), "broken");
        crate::hw_mgmt::testing::install_redfish_client(&root);
        let e = tpm_password().unwrap_err();
        assert_eq!((e.exc.name(), e.str().as_str()), ("AttributeError", "module 'hw_management_redfish_client' has no attribute 'BMCAccessor'"));
    }

    #[test]
    fn a_failed_login_or_an_exception_answers_no_id() {
        let curl = FakeCurl::new();
        // No TPM password: no login, nothing asked.
        let b = bmc(&curl.path(), no_tpm);
        assert_eq!(b.get_version().unwrap().py_repr(), "'N/A'");
        assert_eq!(b.get_firmware_id().unwrap().py_repr(), "'FW_BMC_0'");
        assert!(curl.take_requests().is_empty());
        // No cURL at all: the login raises, which is logged and caught.
        let b = bmc("/nonexistent/curl", tpm);
        assert_eq!(b.get_version().unwrap().py_repr(), "'N/A'");
        assert!(!b.rf_client.has_login());
    }

    const SESSIONS: &str = "/redfish/v1/SessionService/Sessions";
    const REFUSED: &str = r#"{"error": {"code": "Base.1.8.GeneralError", "message": "Invalid username or password"}}"#;

    #[test]
    fn a_bmc_platform_s_bmc_is_built_once_from_its_configuration() {
        let root = crate::testing::Root::new();
        forget_instance();
        // Not a BMC platform.
        assert!(get_instance().unwrap().is_none());
        root.write("/usr/share/sonic/platform/platform_env.conf", "switch_host=1\n");
        root.write("/etc/sonic/bmc.json", r#"{"bmc_if_name": "usb0"}"#);
        // No address, no build configuration, no NOS user: none.
        assert!(get_instance().unwrap().is_none());
        root.write("/etc/sonic/bmc.json", r#"{"bmc_addr": "169.254.0.1"}"#);
        assert!(get_instance().unwrap().is_none());
        root.write("/etc/sonic/bmc_config.json", r#"{"bmc_root_account_default_password": "0penBmc"}"#);
        assert!(get_instance().unwrap().is_none());
        root.write(
            "/etc/sonic/bmc_config.json",
            r#"{"bmc_nos_account_username": "nosuser", "bmc_root_account_default_password": "0penBmc"}"#,
        );
        let b = get_instance().unwrap().unwrap();
        assert!(Arc::ptr_eq(&b, &get_instance().unwrap().unwrap()));
        assert_eq!(b.py_class(), "BMC");
        assert_eq!(
            [b.vendor.get_login_user().unwrap(), b.vendor.get_default_root_password().unwrap(), b.vendor.get_eeprom_id().unwrap()],
            [PyVal::str("nosuser"), PyVal::str("0penBmc"), PyVal::str("BMC_eeprom")]
        );
        forget_instance();
    }

    /// The first login refused: provisioning is tried, hw-management's
    /// client is not there, and the login stays refused.  Tried once: the
    /// next login is the base class's, refused as well.
    #[test]
    fn a_first_login_refused_tries_provisioning_once() {
        let _root = crate::testing::Root::new();
        let curl = FakeCurl::new();
        curl.reply("POST", SESSIONS, 401, REFUSED);
        let b = bmc(&curl.path(), tpm);
        assert_eq!(b.login().unwrap(), rf::ERR_CODE_AUTH_FAILURE);
        assert!(b.vendor.provisioning_tried.load(Ordering::Relaxed));
        assert_eq!(curl.take_requests(), [LOGIN]);
        assert_eq!(b.login().unwrap(), rf::ERR_CODE_AUTH_FAILURE);
        assert_eq!(curl.take_requests(), [LOGIN]);
    }

    /// With the client there: what it prints is logged, and the login is
    /// tried again once it has provisioned the account -- not when it
    /// answers anything but 0, or raises.
    #[test]
    fn hw_management_s_client_provisions_the_account() {
        let root = crate::testing::Root::new();
        crate::hw_mgmt::testing::install_redfish_client(&root);
        let curl = FakeCurl::new();
        curl.reply("POST", SESSIONS, 401, REFUSED);
        let logged = |b: &Bmc| {
            utils::tests::LOGGED.with(|l| l.borrow_mut().clear());
            assert_eq!(b.login().unwrap(), rf::ERR_CODE_AUTH_FAILURE);
            utils::tests::LOGGED.with(|l| l.borrow().clone())
        };
        let b = bmc(&curl.path(), tpm);
        let lines = logged(&b);
        assert_eq!(
            lines[..4],
            [
                "Configuring the BMC NOS account from hw_management_redfish_client.py",
                "hw_management_redfish_client: Login to BMC",
                "hw_management_redfish_client: -- BMC Login Pass, Flow: A",
                "BMC NOS account configured successfully",
            ]
        );
        assert_eq!(curl.take_requests(), [LOGIN, LOGIN]);

        crate::hw_mgmt::testing::redfish_set("login_ret", "3");
        let lines = logged(&bmc(&curl.path(), tpm));
        assert_eq!(
            lines,
            [
                "Configuring the BMC NOS account from hw_management_redfish_client.py",
                "hw_management_redfish_client: Login to BMC",
                "hw_management_redfish_client: -- BMC Login Fail, Flow: A",
                "Failed to configure the BMC NOS account from hw_management_redfish_client.py: 3",
            ]
        );
        assert_eq!(curl.take_requests(), [LOGIN]);

        crate::hw_mgmt::testing::redfish_set("login_error", "'no usb0'");
        let lines = logged(&bmc(&curl.path(), tpm));
        assert_eq!(
            lines,
            [
                "Configuring the BMC NOS account from hw_management_redfish_client.py",
                "Error configuring the BMC NOS account from hw_management_redfish_client.py: no usb0",
                "hw_management_redfish_client: Login to BMC",
            ]
        );
        assert_eq!(curl.take_requests(), [LOGIN]);
    }

    /// Anything but a refusal is an error as it is, with no provisioning.
    #[test]
    fn a_login_that_fails_otherwise_provisions_nothing() {
        let curl = FakeCurl::new();
        curl.fail("POST", SESSIONS, 7, "curl: (7) Failed to connect");
        let b = bmc(&curl.path(), tpm);
        let ret = b.login().unwrap();
        assert!(ret != rf::ERR_CODE_OK && ret != rf::ERR_CODE_AUTH_FAILURE, "{ret}");
        assert!(!b.vendor.provisioning_tried.load(Ordering::Relaxed));
    }
}
