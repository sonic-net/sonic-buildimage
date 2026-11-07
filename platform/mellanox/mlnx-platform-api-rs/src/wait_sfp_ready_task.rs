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



//! `sonic_platform/wait_sfp_ready_task.py`: a thread that holds modules for
//! the 3 seconds their firmware takes to load after a reset, and hands them
//! back as a set.
//!
//! The set is a Python set of port indices, and the order it iterates in
//! reaches the change events' dict and its log line, so it is CPython's
//! (`platform_base::py::intset`).

use std::sync::{Arc, Condvar, Mutex, OnceLock};
use std::thread;
use std::time::{Duration, Instant};

use platform_base::py::intset::IntSet;
use platform_base::py::{PyErr, PyResult};

use crate::utils;

const WAIT_TIME: Duration = Duration::from_secs(3);

struct Inner {
    /// `_wait_dict`: port index and expiry, in insertion order.
    wait: Vec<(u64, Instant)>,
    /// `_ready_set`.
    ready: IntSet,
}

/// `threading.Event`.
struct Event {
    flag: Mutex<bool>,
    cv: Condvar,
}

pub struct WaitSfpReadyTask {
    inner: Mutex<Inner>,
    event: Event,
    started_once: Mutex<bool>,
    alive: Mutex<bool>,
}

/// `SFP.wait_ready_task`: one per process.
static TASK: OnceLock<Arc<WaitSfpReadyTask>> = OnceLock::new();

/// `SFP.get_wait_ready_task()`.
pub fn get() -> Arc<WaitSfpReadyTask> {
    TASK.get_or_init(|| {
        crate::imports::wait_sfp_ready_task();
        Arc::new(WaitSfpReadyTask {
            inner: Mutex::new(Inner { wait: Vec::new(), ready: IntSet::new() }),
            event: Event { flag: Mutex::new(false), cv: Condvar::new() },
            started_once: Mutex::new(false),
            alive: Mutex::new(false),
        })
    })
    .clone()
}

impl WaitSfpReadyTask {
    /// `start_once()`: the thread, the first time only.  A thread that
    /// cannot be had raises, as `Thread.start()` does, and the next call
    /// tries again.
    pub fn start_once(self: &Arc<Self>) -> PyResult<()> {
        let mut started = utils::lock(&self.started_once);
        if *started {
            return Ok(());
        }
        let me = self.clone();
        thread::Builder::new()
            .spawn(move || me.run())
            .map_err(|_| PyErr::runtime_error("can't start new thread"))?;
        *utils::lock(&self.alive) = true;
        *started = true;
        Ok(())
    }

    /// `is_alive()`.
    pub fn is_alive(&self) -> bool {
        *utils::lock(&self.alive)
    }

    /// `schedule_wait(sfp_index)`: due in 3 seconds; the thread is woken if
    /// it was idle.
    pub fn schedule_wait(&self, sfp_index: u64) {
        let is_empty = {
            let mut g = utils::lock(&self.inner);
            let is_empty = g.wait.is_empty();
            let due = Instant::now() + WAIT_TIME;
            match g.wait.iter_mut().find(|(i, _)| *i == sfp_index) {
                Some(e) => e.1 = due,
                None => g.wait.push((sfp_index, due)),
            }
            is_empty
        };
        if is_empty {
            self.set_event();
        }
    }

    /// `cancel_wait(sfp_index)`.
    pub fn cancel_wait(&self, sfp_index: u64) {
        let mut g = utils::lock(&self.inner);
        g.wait.retain(|(i, _)| *i != sfp_index);
        g.ready.discard(sfp_index);
    }

    /// `get_ready_set()`: a deep copy, the original cleared.
    pub fn get_ready_set(&self) -> Vec<u64> {
        let mut g = utils::lock(&self.inner);
        if g.ready.is_empty() {
            return Vec::new();
        }
        let copy = g.ready.deepcopy();
        g.ready.clear();
        copy.iter().collect()
    }

    /// `empty()`.
    pub fn empty(&self) -> bool {
        utils::lock(&self.inner).wait.is_empty()
    }

    fn set_event(&self) {
        *utils::lock(&self.event.flag) = true;
        self.event.cv.notify_all();
    }

    /// `run()`: idle until something is scheduled, then once a second move
    /// what is due from the wait dict to the ready set.
    fn run(&self) {
        let mut is_empty = true;
        loop {
            if is_empty {
                // `event.wait(); event.clear()`
                let mut f = utils::lock(&self.event.flag);
                while !*f {
                    f = self.event.cv.wait(f).unwrap_or_else(std::sync::PoisonError::into_inner);
                }
                *f = false;
            }
            let now = Instant::now();
            {
                let mut g = utils::lock(&self.inner);
                // `pending_remove_set` is a set too, so what is due joins
                // the ready set in that set's order.
                let due = IntSet::from_iter_ordered(g.wait.iter().filter(|(_, t)| now >= *t).map(|(i, _)| *i));
                for i in due.iter() {
                    g.wait.retain(|(k, _)| *k != i);
                    g.ready.add(i);
                }
                is_empty = g.wait.is_empty();
            }
            thread::sleep(Duration::from_secs(1));
        }
    }
}

#[cfg(test)]
impl WaitSfpReadyTask {
    /// A task of its own, apart from the process's.
    pub(crate) fn new_for_test() -> Arc<WaitSfpReadyTask> {
        Arc::new(WaitSfpReadyTask {
            inner: Mutex::new(Inner { wait: Vec::new(), ready: IntSet::new() }),
            event: Event { flag: Mutex::new(false), cv: Condvar::new() },
            started_once: Mutex::new(false),
            alive: Mutex::new(false),
        })
    }

    /// `_wait_dict`'s ports.
    pub(crate) fn waiting(&self) -> Vec<u64> {
        self.inner.lock().map(|g| g.wait.iter().map(|(i, _)| *i).collect()).unwrap_or_default()
    }

    /// `_ready_set.add(index)`, as the thread does when a wait is over.
    pub(crate) fn mark_ready(&self, index: u64) {
        if let Ok(mut g) = self.inner.lock() {
            g.ready.add(index);
        }
    }

    /// Nothing waiting, nothing ready.
    pub(crate) fn reset(&self) {
        if let Ok(mut g) = self.inner.lock() {
            g.wait.clear();
            g.ready.clear();
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn a_scheduled_wait_is_cancelled_and_leaves_nothing() {
        let task = WaitSfpReadyTask::new_for_test();
        assert!(task.empty());
        task.schedule_wait(0);
        assert!(!task.empty());
        // A second schedule moves the due time, not adds an entry.
        task.schedule_wait(0);
        task.schedule_wait(5);
        assert_eq!(task.waiting(), [0, 5]);
        task.cancel_wait(0);
        task.cancel_wait(5);
        assert!(task.empty());
        // An index never scheduled is fine to cancel.
        task.cancel_wait(42);
        assert!(task.empty());
        assert!(task.get_ready_set().is_empty());
    }

    #[test]
    fn cancelling_drops_a_port_from_the_ready_set_and_the_set_comes_back_in_set_order() {
        let task = WaitSfpReadyTask::new_for_test();
        task.mark_ready(7);
        task.cancel_wait(7);
        assert!(task.get_ready_set().is_empty());
        for i in [33, 1, 9] {
            task.mark_ready(i);
        }
        // CPython's {33, 1, 9}: 33 lands in slot 1 of the 8-slot table.
        assert_eq!(task.get_ready_set(), [33, 1, 9]);
        assert!(task.get_ready_set().is_empty());
        task.mark_ready(3);
        task.reset();
        assert!(task.get_ready_set().is_empty() && task.empty());
    }

    #[test]
    fn a_port_is_ready_three_seconds_after_it_is_scheduled() {
        let task = WaitSfpReadyTask::new_for_test();
        assert!(!task.is_alive());
        task.start_once().unwrap();
        task.start_once().unwrap();
        assert!(task.is_alive());
        task.schedule_wait(0);
        let begin = Instant::now();
        let mut ready = Vec::new();
        while begin.elapsed() < Duration::from_secs(6) && ready.is_empty() {
            thread::sleep(Duration::from_millis(250));
            ready = task.get_ready_set();
        }
        assert_eq!(ready, [0]);
        assert!(begin.elapsed() >= WAIT_TIME);
        assert!(task.empty());
        // Idle again, the thread wakes for the next one.
        task.schedule_wait(1);
        assert_eq!(task.waiting(), [1]);
        task.cancel_wait(1);
    }

    #[test]
    fn the_process_has_one_task() {
        assert!(Arc::ptr_eq(&get(), &get()));
    }
}
