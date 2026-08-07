#!/usr/bin/env python_nos
# -*- coding: UTF-8 -*-

from event_common import SignalType, SignalCpuEventType, SignalGpuEventType, SignalIoEventType, EventLevel, AssertType, EVENT_TRIG_BY_GPIO, EVENT_TRIG_BY_POLL

# app cfg: event_notify.py
# set event trig way
DEV_EVENT_TRIG_WAY = EVENT_TRIG_BY_GPIO | EVENT_TRIG_BY_POLL

###### cfgs: gpio intterupt trig event notify cfg ###########
AST2700_GPIOCHIP0_BASE = 512
AST2700_GPIOCHIP1_BASE = 524

# 18A0 18B1
# A0 B1 C2 D3 E4 F5 G6 H7 I8 J9 K10 L11 M12 N13 O14 P15 Q16 R17 S18 T19 U20 V21 W22 X23 Y24 Z25 AA26
GPIOA0 = AST2700_GPIOCHIP1_BASE + 0 * 8 + 0      # THERMTRIP#: main cpu over temp warning signal, low active
GPIOD1 = AST2700_GPIOCHIP1_BASE + 3 * 8 + 1      # SMERR#: main cpu system error signal, low active
GPIOC1 = AST2700_GPIOCHIP1_BASE + 2 * 8 + 1      # UID Button long press int check signal, low active

INIT_CFG_PARAS = [
    {"gettype": "devfile", "path": "/dev/cpld0", "offset": 0x89, "value": [0x1]},
    {"gettype": "devfile", "path": "/dev/cpld0", "offset": 0x8a, "value": [0x1]},
    {"gettype": "devfile", "path": "/dev/cpld0", "offset": 0x8c, "value": 0x06, "mask": 0x06},
]

GPIO_CFG_PARAS = {
    # trig event notify by gpio int --- GPIO Init cfg: set gpio input mode and int way + Events_info
    "gpio_cfg0" : {
        "gpio_num" : GPIOA0,
        "edge"     : "falling",
        "check_status" : {"gettype": "devfile", "path": "/dev/cpld0", "offset": 0x83, "mask": 0x04, "okval": 0x00, "read_len": 1},
        "events_info" : [
            {
                "signal_type" : SignalType.SIGNAL_TYPE_CPU,
                "event_type"  : SignalCpuEventType.CPU_EVENT_TYPE_OVER_TEMP,
                "event_level" : EventLevel.EVENT_LEVEL_WARNING,
                "assert_type" : AssertType.ASSERT_TYPE_ALARM_OCCUR,
                "event_detail" : "CPU over temp warning",
            },
        ],
        "clear_status" : [
            {"gettype": "devfile", "path": "/dev/cpld0", "offset": 0x83, "value": 0x04, "mask": 0x04},
        ],
    },

    "gpio_cfg1" : {
        "gpio_num" : GPIOD1,
        "edge"     : "falling",
        "check_status" : {"gettype": "devfile", "path": "/dev/cpld0", "offset": 0x83, "mask": 0x02, "okval": 0x00, "read_len": 1},
        "events_info" : [
            {
                "signal_type" : SignalType.SIGNAL_TYPE_CPU,
                "event_type"  : SignalCpuEventType.CPU_EVENT_TYPE_SYS_CHECK_ERR,
                "event_level" : EventLevel.EVENT_LEVEL_CRITICAL,
                "assert_type" : AssertType.ASSERT_TYPE_ALARM_OCCUR,
                "event_detail" : "CPU has happened SYSTEM ERROR",
            },
        ],
        "clear_status" : [
            {"gettype": "devfile", "path": "/dev/cpld0", "offset": 0x83, "value": 0x02, "mask": 0x02},
        ],
    },

    "gpio_cfg2" : {
        "gpio_num" : GPIOC1,
        "edge"     : "falling",
        "check_status" : {"gettype": "devfile", "path": "/dev/cpld0", "offset": 0x81, "mask": 0x01, "okval": 0x00, "read_len": 1},
        "events_info" : [
            {
                "signal_type" : SignalType.SIGNAL_TYPE_IO,
                "event_type"  : SignalIoEventType.IO_EVENT_TYPE_UID_BUTTON_LONG_PRESS,
                "event_level" : EventLevel.EVENT_LEVEL_OK,
                "assert_type" : AssertType.ASSERT_TYPE_ALARM_OCCUR,
                "event_detail" : "UID button long press happened",
            },
        ],
        "clear_status" : [
            {"gettype": "devfile", "path": "/dev/cpld0", "offset": 0x81, "value": 0x01, "mask": 0x01},
        ],
    },
}

# trig event notify by poll
POLL_CFG_PARAS = {
    "poll_time" : 1,
    "poll_cfgs" : {
        "cfg0" : {
            "check_status" : {"gettype": "devfile", "path": "/dev/cpld0", "offset": 0x80, "mask": 0x03, "okval": 0x01, "read_len": 1},
            "events_info" : [
                  {
                      "signal_type" : SignalType.SIGNAL_TYPE_IO,
                      "event_type"  : SignalIoEventType.IO_EVENT_TYPE_PWR_BUTTON_LONG_PRESS,
                      "event_level" : EventLevel.EVENT_LEVEL_OK,
                      "assert_type" : AssertType.ASSERT_TYPE_ALARM_OCCUR,
                      "event_detail" : "power button long press happened",
                  },
            ],
            "clear_status" : [
                {"gettype": "devfile", "path": "/dev/cpld0", "offset": 0x80, "value": 0x03, "mask": 0x03},
            ],
        },

        "cfg1" : {
            "check_status" : {"gettype": "devfile", "path": "/dev/cpld0", "offset": 0x80, "mask": 0x03, "okval": 0x02, "read_len": 1},
            "events_info" : [
                  {
                      "signal_type" : SignalType.SIGNAL_TYPE_IO,
                      "event_type"  : SignalIoEventType.IO_EVENT_TYPE_PWR_BUTTON_SHORT_PRESS,
                      "event_level" : EventLevel.EVENT_LEVEL_OK,
                      "assert_type" : AssertType.ASSERT_TYPE_ALARM_OCCUR,
                      "event_detail" : "power button short press happened",
                  },
            ],
            "clear_status" : [
                {"gettype": "devfile", "path": "/dev/cpld0", "offset": 0x80, "value": 0x03, "mask": 0x03},
            ],
        },
    },
}
