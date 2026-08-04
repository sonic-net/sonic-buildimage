// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2022 Pensando Systems, Inc */

#include <linux/errno.h>
#include <linux/kernel.h>

struct device;
struct ptp_clock;
struct ptp_clock_event;
struct ptp_clock_info;
enum ptp_pin_function { PTP_PIN_DUMMY };

__weak struct ptp_clock *ptp_clock_register(struct ptp_clock_info *info, struct device *parent) { return NULL; }
__weak int ptp_clock_unregister(struct ptp_clock *ptp) { return 0; }
__weak void ptp_clock_event(struct ptp_clock *ptp, struct ptp_clock_event *event) { }
__weak int ptp_clock_index(struct ptp_clock *ptp) { return -1; }
__weak int ptp_find_pin(struct ptp_clock *ptp, enum ptp_pin_function func, unsigned int chan) { return -1; }
__weak int ptp_schedule_worker(struct ptp_clock *ptp, unsigned long delay) { return -EOPNOTSUPP; }
__weak void ptp_cancel_worker_sync(struct ptp_clock *ptp) { }
