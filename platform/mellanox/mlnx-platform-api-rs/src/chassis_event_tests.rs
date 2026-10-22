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


//! `Chassis.get_change_event`: legacy polling and module host management,
//! the ASIC ready files, and module initialization on the way in.  A regular
//! file never raises POLLPRI, so `crate::poll::inject` stands in for the SDK.

use std::time::Instant;

use super::*;
use crate::poll::inject;
use crate::sfp::test_support::{eeprom, fifo, module, module_path, platform};

const ASIC1_READY: &str = "/var/run/hw-management/config/asic1_ready";

fn chassis() -> Arc<Chassis> {
    let c = Chassis::new(ChassisKind::Fixed).unwrap();
    c.initialize_sfp().unwrap();
    c
}

fn event(index: usize, attr: &str) {
    inject::event(&module_path(index, attr), libc::POLLPRI);
}

fn dict(a: ModeAnswer) -> String {
    match a {
        ModeAnswer::Tuple => "tuple".to_string(),
        ModeAnswer::Dict(d) => PyVal::Dict(d.into_iter().map(|(k, v)| (k, v.to_pyval())).collect()).py_repr(),
    }
}

fn set_state(c: &Chassis, index: i128, state: &'static str) {
    *c.sfp_at(index).unwrap().unwrap().state.lock().unwrap() = state;
}

fn state(c: &Chassis, index: i128) -> &'static str {
    c.sfp_at(index).unwrap().unwrap().get_state().unwrap()
}

fn types(ev: &EventState) -> Vec<(i128, &'static str)> {
    ev.registered.iter().map(|(_, r)| (r.sdk_index, r.fd_type)).collect()
}

// -- legacy ------------------------------------------------------------------

/// test_change_event.py's test_get_change_event_legacy.
#[test]
fn legacy_events_are_each_port_s_status_once_it_changes() {
    let root = platform("x86_64-nvidia_sn5600-r0", 3, false, false);
    for i in 0..3 {
        module(&root, i, &[("present", "1"), ("status", "1")]);
    }
    let c = chassis();
    let mut ev = EventState::default();
    // The first call registers every port's `present`, notes its status,
    // and times out.
    assert_eq!(dict(c.change_event_legacy(&mut ev, 10).unwrap()), "{'sfp': {}}");
    assert_eq!(types(&ev), [(0, ""), (1, ""), (2, "")]);
    // A port's first event with the status it had is the SDK's dummy.
    event(1, "present");
    assert_eq!(dict(c.change_event_legacy(&mut ev, 10).unwrap()), "{'sfp': {}}");
    // Removed, then an error with its vendor description.
    module(&root, 1, &[("status", "2")]);
    event(1, "present");
    assert_eq!(dict(c.change_event_legacy(&mut ev, 10).unwrap()), "{'sfp': {2: '0'}, 'sfp_error': {}}");
    module(&root, 0, &[("status", "3"), ("statuserror", "1")]);
    event(0, "present");
    assert_eq!(
        dict(c.change_event_legacy(&mut ev, 10).unwrap()),
        "{'sfp': {1: '65537'}, 'sfp_error': {1: 'Long range for non-Mellanox cable or module'}}"
    );
    // Unknown: two changes too close together; nothing to report.
    module(&root, 2, &[("status", "5")]);
    event(2, "present");
    assert_eq!(dict(c.change_event_legacy(&mut ev, 10).unwrap()), "{'sfp': {}}");
    // Two at once come in registration order.
    module(&root, 1, &[("status", "1")]);
    module(&root, 2, &[("status", "1")]);
    event(2, "present");
    event(1, "present");
    assert_eq!(dict(c.change_event_legacy(&mut ev, 10).unwrap()), "{'sfp': {2: '1', 3: '1'}, 'sfp_error': {}}");
    // A status the SDK table does not have: Python's KeyError.
    module(&root, 1, &[("status", "9")]);
    event(1, "present");
    assert_eq!(c.change_event_legacy(&mut ev, 10).err().unwrap().str(), "9");
}

/// test_get_change_event_legacy_seek_fails, and a read that fails.
#[test]
fn a_legacy_attribute_that_cannot_be_read_is_skipped_after_a_pause() {
    let root = platform("x86_64-nvidia_sn5600-r0", 2, false, false);
    let _writer = fifo(&root, &module_path(0, "present"));
    std::fs::create_dir_all(root.real(&module_path(1, "present"))).unwrap();
    module(&root, 0, &[("status", "1")]);
    module(&root, 1, &[("status", "1")]);
    let c = chassis();
    let mut ev = EventState::default();
    c.change_event_legacy(&mut ev, 10).unwrap();
    event(0, "present");
    event(1, "present");
    let begin = Instant::now();
    assert_eq!(dict(c.change_event_legacy(&mut ev, 10).unwrap()), "{'sfp': {}}");
    // MALFUNCTION_SYSFS_READ_ERROR_DELAY_SECS, less the round's own time.
    assert!(begin.elapsed() >= Duration::from_millis(500));
}

#[test]
fn a_port_without_present_is_too_early_to_poll() {
    let root = platform("x86_64-nvidia_sn5600-r0", 2, false, false);
    module(&root, 0, &[("present", "1"), ("status", "1")]);
    let c = chassis();
    let mut ev = EventState::default();
    assert_eq!(dict(c.change_event_legacy(&mut ev, 10).unwrap()), "tuple");
    assert!(ev.poll.is_none() && ev.registered.is_empty() && ev.before_first_poll.is_empty());
    // A port slot never built is Python's None.
    c.sfp.lock().unwrap().list[1] = None;
    assert_eq!(
        c.change_event_legacy(&mut ev, 10).err().unwrap().str(),
        "'NoneType' object has no attribute 'get_fd_for_polling_legacy'"
    );
}

// -- the whole call, and the ASIC ready files --------------------------------

#[test]
fn get_change_event_waits_for_the_ports_and_reports_an_asic_s_ports() {
    let root = platform("x86_64-nvidia_sn5600-r0", 2, false, false);
    for i in 0..2 {
        module(&root, i, &[("present", "1"), ("status", "1"), ("statuserror", "0")]);
        eeprom(&root, i);
    }
    root.write(ASIC1_READY, "1\n");
    let c = Chassis::new(ChassisKind::Fixed).unwrap();
    // The ports are built, found ready, and marked so; the ASIC's state
    // is noted.
    assert_eq!(c.get_change_event(10).unwrap().py_repr(), "(True, {'sfp': {}})");
    assert!(root.exists("/tmp/nv-syncd-shared/sfp_ready"));
    // The ASIC goes: its ports are removed, by their index as a string,
    // and no longer polled.
    root.write(ASIC1_READY, "0\n");
    assert_eq!(c.get_change_event(10).unwrap().py_repr(), "(True, {'sfp': {'1': '0', '2': '0'}})");
    assert!(c.events.lock().unwrap().registered.is_empty());
    assert_eq!(c.module_host_mgmt_initializer.initialized(), [false]);
    // And comes back: inserted, and polled again.
    root.write(ASIC1_READY, "1\n");
    assert_eq!(c.get_change_event(10).unwrap().py_repr(), "(True, {'sfp': {'1': '1', '2': '1'}})");
    assert_eq!(types(&c.events.lock().unwrap()), [(0, ""), (1, "")]);
    assert_eq!(c.module_host_mgmt_initializer.initialized(), [true]);
}

#[test]
fn too_early_with_asic_events_is_python_s_attribute_error() {
    let root = platform("x86_64-nvidia_sn5600-r0", 2, false, false);
    module(&root, 0, &[("present", "1"), ("status", "1"), ("statuserror", "0")]);
    module(&root, 1, &[("status", "1"), ("statuserror", "0")]);
    root.write("/tmp/nv-syncd-shared/sfp_ready", "");
    root.write(ASIC1_READY, "1\n");
    let c = Chassis::new(ChassisKind::Fixed).unwrap();
    // The mode's `return True, {'sfp': {}}` becomes the dict.
    assert_eq!(c.get_change_event(10).unwrap().py_repr(), "(True, (True, {'sfp': {}}))");
    root.write(ASIC1_READY, "0\n");
    assert_eq!(c.get_change_event(10).unwrap_err().str(), "'tuple' object has no attribute 'setdefault'");
}

#[test]
fn the_asic_ready_files_are_watched_and_compared() {
    let root = platform("x86_64-nvidia_sn5600-r0", 1, false, false);
    module(&root, 0, &[("present", "1"), ("status", "1")]);
    let c = chassis();
    let mut ev = EventState::default();
    // No hw-management config directory: nothing to watch.
    assert_eq!(c.get_asic_change_event(&mut ev, 100.0).unwrap_err().exc.name(), "InotifyError");
    // A write while watching is seen there; the port is reported, by its
    // index as a string.
    root.write(ASIC1_READY, "1\n");
    let real = root.real(ASIC1_READY);
    let writer = std::thread::spawn(move || {
        std::thread::sleep(Duration::from_millis(100));
        std::fs::write(real, "0\n").unwrap();
    });
    assert_eq!(c.get_asic_change_event(&mut ev, 100.0).unwrap().repr(), "{'1': '0'}");
    writer.join().unwrap();
    assert_eq!(ev.last_asic_ready, [("asic1_ready".to_string(), Some("0".to_string()))]);
    // A file gone is a state too: None.
    std::fs::remove_file(root.real(ASIC1_READY)).unwrap();
    assert_eq!(c.get_asic_change_event(&mut ev, 100.0).unwrap().repr(), "{'1': '0'}");
    assert_eq!(ev.last_asic_ready, [("asic1_ready".to_string(), None)]);
    // One it cannot read raises.
    std::fs::create_dir_all(root.real(ASIC1_READY)).unwrap();
    assert_eq!(c.get_asic_change_event(&mut ev, 100.0).unwrap_err().exc.name(), "IsADirectoryError");
    std::fs::remove_dir(root.real(ASIC1_READY)).unwrap();
    // An ASIC that was not there before: Python's KeyError on its name.
    root.write("/usr/share/sonic/platform/asic.conf", "NUM_ASIC=2\n");
    crate::utils::forget_caches();
    let e = c.get_asic_change_event(&mut ev, 100.0).unwrap_err();
    assert_eq!((e.exc.name(), e.str()), ("KeyError", "'asic2_ready'".to_string()));
}

// -- module host management --------------------------------------------------

/// test_get_change_event_for_module_host_management_mode.
#[test]
fn host_managed_modules_move_through_their_state_machine() {
    let root = platform("x86_64-nvidia_sn5600-r0", 4, true, false);
    root.write(ASIC1_READY, "1\n");
    for i in 0..4 {
        module(&root, i, &[("present", "1"), ("hw_present", "1"), ("power_good", "1"), ("control", "1"), ("power_on", "1"), ("hw_reset", "1")]);
    }
    let c = chassis();
    let task = crate::wait_sfp_ready_task::get();
    task.reset();
    set_state(&c, 0, sfpe::STATE_SW_CONTROL);
    set_state(&c, 1, sfpe::STATE_FW_CONTROL);
    set_state(&c, 2, sfpe::STATE_NOT_PRESENT);
    set_state(&c, 3, sfpe::STATE_SW_CONTROL);
    let mut ev = EventState::default();
    assert_eq!(dict(c.change_event_host_mgmt(&mut ev, 10).unwrap()), "{'sfp': {}}");
    assert_eq!(
        types(&ev),
        [(0, "hw_present"), (0, "power_good"), (1, "present"), (2, "hw_present"), (2, "power_good"), (3, "hw_present"), (3, "power_good")]
    );
    let call = |ev: &mut EventState| dict(c.change_event_host_mgmt(ev, 10).unwrap());
    // Present again while in software control: the dummy event.
    event(0, "hw_present");
    assert_eq!(call(&mut ev), "{'sfp': {}}");
    // Gone.
    module(&root, 0, &[("hw_present", "0")]);
    event(0, "hw_present");
    assert_eq!(call(&mut ev), "{'sfp': {1: '0'}, 'sfp_error': {}}");
    assert_eq!(state(&c, 0), sfpe::STATE_NOT_PRESENT);
    // Firmware control: `present` says error, inserted, then removed.
    module(&root, 1, &[("present", "2"), ("statuserror", "4")]);
    event(1, "present");
    assert_eq!(call(&mut ev), "{'sfp': {2: '131073'}, 'sfp_error': {2: 'Enforce part number list'}}");
    // Present under firmware control is the dummy event, as Python checks
    // for dummies first.
    module(&root, 1, &[("present", "1")]);
    event(1, "present");
    assert_eq!(call(&mut ev), "{'sfp': {}}");
    module(&root, 1, &[("present", "0")]);
    event(1, "present");
    assert_eq!(call(&mut ev), "{'sfp': {2: '0'}, 'sfp_error': {}}");
    assert_eq!(state(&c, 1), sfpe::STATE_NOT_PRESENT);
    // ...and is polled as software control is from then on.
    assert!(types(&ev).contains(&(1, "hw_present")) && !types(&ev).contains(&(1, "present")));
    // Power lost under software control.
    module(&root, 3, &[("power_good", "0")]);
    event(3, "power_good");
    assert_eq!(call(&mut ev), "{'sfp': {4: '5'}, 'sfp_error': {}}");
    assert_eq!(state(&c, 3), sfpe::STATE_POWER_BAD);
    // Inserted: reset, and waited for -- nothing to report yet.
    module(&root, 2, &[("power_good", "0")]);
    event(2, "hw_present");
    assert_eq!(call(&mut ev), "{'sfp': {}}");
    assert_eq!(state(&c, 2), sfpe::STATE_RESETTING);
    assert_eq!(task.waiting(), [2]);
    // Its wait is over, but its ASIC is not ready: ignored.
    task.mark_ready(2);
    root.write(ASIC1_READY, "0\n");
    assert_eq!(call(&mut ev), "{'sfp': {}}");
    assert_eq!(state(&c, 2), sfpe::STATE_RESETTING);
    // Now ready: power is bad, which is reported.
    root.write(ASIC1_READY, "1\n");
    task.mark_ready(2);
    assert_eq!(call(&mut ev), "{'sfp': {3: '5'}, 'sfp_error': {}}");
    assert_eq!(state(&c, 2), sfpe::STATE_POWER_BAD);
    task.reset();
    // Power back: the control type is sonic_xcvr's to decide.
    module(&root, 3, &[("power_good", "1")]);
    event(3, "power_good");
    assert!(c.change_event_host_mgmt(&mut ev, 10).err().unwrap().str().starts_with("not ported to the native platform API: SFP.get_xcvr_api"));
    // A value that is no number: skipped after a pause.
    module(&root, 0, &[("hw_present", "x")]);
    event(0, "hw_present");
    let begin = Instant::now();
    assert_eq!(call(&mut ev), "{'sfp': {}}");
    assert!(begin.elapsed() >= Duration::from_millis(500));
}

#[test]
fn get_change_event_in_host_management_cancels_and_restores_an_asic_s_ports() {
    let root = platform("x86_64-nvidia_sn5600-r0", 2, true, false);
    root.write(ASIC1_READY, "1\n");
    for i in 0..2 {
        module(&root, i, &[("hw_present", "1"), ("power_good", "1"), ("present", "1")]);
    }
    let c = Chassis::new(ChassisKind::Fixed).unwrap();
    let task = crate::wait_sfp_ready_task::get();
    task.reset();
    // On the host the ports are only built; both are polled as software
    // control polls them.
    assert_eq!(c.get_change_event(10).unwrap().py_repr(), "(True, {'sfp': {}})");
    assert_eq!(types(&c.events.lock().unwrap()), [(0, "hw_present"), (0, "power_good"), (1, "hw_present"), (1, "power_good")]);
    // The ASIC goes: port 0's pending reset is cancelled, nothing polled.
    task.schedule_wait(0);
    root.write(ASIC1_READY, "0\n");
    assert_eq!(c.get_change_event(10).unwrap().py_repr(), "(True, {'sfp': {'1': '0', '2': '0'}})");
    assert!(task.waiting().is_empty());
    assert!(c.events.lock().unwrap().registered.is_empty());
    // Back: polled again, by each port's state -- port 1 under firmware
    // control now.
    set_state(&c, 1, sfpe::STATE_FW_CONTROL);
    root.write(ASIC1_READY, "1\n");
    assert_eq!(c.get_change_event(10).unwrap().py_repr(), "(True, {'sfp': {'1': '1', '2': '1'}})");
    assert_eq!(types(&c.events.lock().unwrap()), [(0, "hw_present"), (0, "power_good"), (1, "present")]);
    // A ready port that the reset-done event leaves unstable: logged.
    set_state(&c, 0, sfpe::STATE_DOWN);
    task.mark_ready(0);
    let mut ev = EventState::default();
    assert_eq!(dict(c.change_event_host_mgmt(&mut ev, 10).unwrap()), "{'sfp': {}}");
    assert_eq!(state(&c, 0), sfpe::STATE_DOWN);
    task.reset();
}

#[test]
fn host_managed_polling_skips_a_not_ready_asic_and_waits_for_attributes() {
    let root = platform("x86_64-nvidia_sn5600-r0", 2, true, false);
    for i in 0..2 {
        module(&root, i, &[("hw_present", "1"), ("power_good", "1")]);
    }
    let c = chassis();
    crate::wait_sfp_ready_task::get().reset();
    // The ASIC's last state was not ready: its ports are not polled.
    let mut ev = EventState { last_asic_ready: vec![("asic1_ready".to_string(), Some("0".to_string()))], ..Default::default() };
    assert_eq!(dict(c.change_event_host_mgmt(&mut ev, 10).unwrap()), "{'sfp': {}}");
    assert!(ev.registered.is_empty());
    // A port under firmware control without `present`: too early.
    set_state(&c, 1, sfpe::STATE_FW_CONTROL);
    let mut ev = EventState::default();
    assert_eq!(dict(c.change_event_host_mgmt(&mut ev, 10).unwrap()), "tuple");
    assert!(ev.poll.is_none());
    // One that cannot seek: skipped after a pause.
    set_state(&c, 1, sfpe::STATE_SW_CONTROL);
    let _writer = fifo(&root, &module_path(1, "power_good"));
    let mut ev = EventState::default();
    c.change_event_host_mgmt(&mut ev, 10).unwrap();
    event(1, "power_good");
    let begin = Instant::now();
    assert_eq!(dict(c.change_event_host_mgmt(&mut ev, 10).unwrap()), "{'sfp': {}}");
    assert!(begin.elapsed() >= Duration::from_millis(500));
    // A firmware-controlled cable recovering from an error: inserted, with
    // no state change.
    module(&root, 0, &[("present", "1")]);
    set_state(&c, 0, sfpe::STATE_FCP_NOT_PRESENT);
    let mut ev = EventState::default();
    c.change_event_host_mgmt(&mut ev, 10).unwrap();
    event(0, "present");
    assert_eq!(dict(c.change_event_host_mgmt(&mut ev, 10).unwrap()), "{'sfp': {1: '1'}, 'sfp_error': {}}");
    assert_eq!(state(&c, 0), sfpe::STATE_FCP_NOT_PRESENT);
}

// -- initialization on the way in --------------------------------------------

#[test]
fn xcvrd_initializes_the_modules_of_a_ready_asic_once() {
    let root = platform("x86_64-nvidia_sn5600-r0", 3, true, true);
    root.write(ASIC1_READY, "1\n");
    for i in 0..3 {
        module(&root, i, &[("present", "0"), ("status", "1"), ("statuserror", "0"), ("power_on", "1")]);
    }
    module(&root, 0, &[("control", "0")]);
    eeprom(&root, 0);
    module(&root, 1, &[("control", "1"), ("hw_present", "0")]);
    module(&root, 2, &[("control", "1"), ("hw_present", "1"), ("hw_reset", "1"), ("power_good", "0")]);
    crate::wait_sfp_ready_task::get().reset();
    let before = platform_base::logger::argv0();
    platform_base::logger::set_argv0("/usr/local/bin/xcvrd");
    let c = Chassis::new(ChassisKind::Fixed).unwrap();
    let ready_file = crate::module_host_mgmt_initializer::get_asic_ready_file_path("asic0");
    assert_eq!(ready_file, "/tmp/asic_ready/module_host_mgmt_asic_ready_asic0");
    c.sfp_wait_ready_and_initialize().unwrap();
    assert_eq!((0..3).map(|i| state(&c, i)).collect::<Vec<_>>(), [sfpe::STATE_FW_CONTROL, sfpe::STATE_NOT_PRESENT, sfpe::STATE_POWER_BAD]);
    assert_eq!(root.read(&ready_file).as_deref(), Some("asic0\n"));
    assert_eq!(c.module_host_mgmt_initializer.initialized(), [true]);
    // Done: a second call changes nothing.
    std::fs::remove_file(root.real(&ready_file)).unwrap();
    c.sfp_wait_ready_and_initialize().unwrap();
    assert!(!root.exists(&ready_file));
    // An ASIC that is not ready: its ports are built, not initialized.
    root.write(ASIC1_READY, "0\n");
    let c = Chassis::new(ChassisKind::Fixed).unwrap();
    c.sfp_wait_ready_and_initialize().unwrap();
    assert_eq!(state(&c, 0), sfpe::STATE_DOWN);
    assert!(!root.exists(&ready_file));
    assert_eq!(c.module_host_mgmt_initializer.initialized(), [false]);
    // Any other daemon only builds the ports.
    root.write(ASIC1_READY, "1\n");
    platform_base::logger::set_argv0("thermalctld");
    let c = Chassis::new(ChassisKind::Fixed).unwrap();
    c.sfp_wait_ready_and_initialize().unwrap();
    assert_eq!(c.sfp_list().len(), 3);
    assert_eq!(state(&c, 0), sfpe::STATE_DOWN);
    platform_base::logger::set_argv0(&before);
}

#[test]
fn on_the_host_the_modules_are_only_built() {
    let root = platform("x86_64-nvidia_sn5600-r0", 2, true, false);
    let c = Chassis::new(ChassisKind::Fixed).unwrap();
    c.sfp_wait_ready_and_initialize().unwrap();
    assert_eq!(c.sfp_list().len(), 2);
    assert_eq!(state(&c, 1), sfpe::STATE_DOWN);
    drop(root);
}

// -- the pieces --------------------------------------------------------------

#[test]
fn removed_ports_forget_their_module_and_a_bad_index_is_logged() {
    let root = platform("x86_64-nvidia_sn5600-r0", 2, false, false);
    let c = chassis();
    let mut d = sfpe::PortDict::default();
    d.set(1, "0".into());
    d.set(2, "1".into());
    d.set(9, "0".into());
    d.set_val(PyVal::str("2"), PyVal::str("0"));
    d.set_val(PyVal::str("x"), PyVal::str("0"));
    c.reinit_sfps(&d);
    // Python's list indexing, negative from the end.
    assert_eq!(c.sfp_at(-1).unwrap().unwrap().sdk_index, 1);
    assert_eq!(c.sfp_at(2).err().unwrap().str(), "list index out of range");
    assert_eq!(c.asic_modules("asic0").len(), 2);
    assert!(c.asic_modules("asic7").is_empty());
    drop(root);
}

#[test]
fn the_poll_timeout_is_capped_and_a_read_error_slows_the_round() {
    assert_eq!(Chassis::poll_timeout(0), (true, 0.0));
    assert_eq!(Chassis::poll_timeout(250), (false, 250.0));
    assert_eq!(Chassis::poll_timeout(5000), (false, 1000.0));
    let now = Instant::now();
    assert!(!Chassis::round_ends(false, now, now, true, 0.0));
    assert!(Chassis::round_ends(false, now, now, false, 0.0));
    let begin = Instant::now();
    assert!(Chassis::round_ends(true, Instant::now(), begin, false, 0.0));
    assert!(begin.elapsed() >= Duration::from_millis(500));
}

#[test]
fn polling_an_asic_s_ports_on_and_off() {
    let root = platform("x86_64-nvidia_sn5600-r0", 3, false, false);
    module(&root, 0, &[("present", "1"), ("status", "1")]);
    module(&root, 1, &[("present", "1"), ("status", "9")]);
    let c = chassis();
    let ports = c.asic_modules("asic0");
    let mut ev = EventState::default();
    // No poll object yet: nothing to do either way.
    c.disable_polling_for_asic(&mut ev, &ports);
    c.enable_polling_for_asic(&mut ev, &ports).unwrap();
    assert!(ev.registered.is_empty());
    ev.poll = Some(PollSet::default());
    c.disable_polling_for_asic(&mut ev, &ports);
    // Legacy: `present` and the status; a status that raises is a warning,
    // a port without `present` is passed over.
    c.enable_polling_for_asic(&mut ev, &ports).unwrap();
    assert_eq!(types(&ev), [(0, ""), (1, "")]);
    assert_eq!(ev.before_first_poll, [(0, "1")]);
    c.disable_polling_for_asic(&mut ev, &[]);
    assert_eq!(ev.registered.iter().count(), 2);
    c.disable_polling_for_asic(&mut ev, &ports);
    assert!(ev.registered.is_empty());
    inject::clear();
}
