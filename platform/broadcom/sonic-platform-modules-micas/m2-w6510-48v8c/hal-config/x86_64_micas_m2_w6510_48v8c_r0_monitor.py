# coding:utf-8


monitor = {
    "openloop": {
        "linear": {
            "name": "linear",
            "flag": 0,
            "pwm_min": 0x80,
            "pwm_max": 0xff,
            "K": 11,
            "tin_min": 38,
        },
        "curve": {
            "name": "curve",
            "flag": 0,
            "pwm_min": 0x5a,
            "pwm_max": 0xff,
            "a": 0.086,
            "b": 0.318,
            "c": 28,
            "tin_min": 25,
        },
    },

    "pid": {
        "CPU_TEMP": {
            "name": "CPU_TEMP",
            "flag": 1,
            "type": "duty",
            "pwm_min": 0x80,
            "pwm_max": 0xff,
            "Kp": 3,
            "Ki": 0.5,
            "Kd": 0.5,
            "target": 89,
            "value": [None, None, None],
        },
        "SWITCH_TEMP": {
            "name": "SWITCH_TEMP",
            "flag": 1,
            "type": "duty",
            "pwm_min": 0x80,
            "pwm_max": 0xff,
            "Kp": 3,
            "Ki": 0.4,
            "Kd": 0.4,
            "target": 82,
            "value": [None, None, None],
        },
        "OUTLET_TEMP": {
            "name": "OUTLET_TEMP",
            "flag": 0,
            "type": "duty",
            "pwm_min": 0x80,
            "pwm_max": 0xff,
            "Kp": 2,
            "Ki": 0.4,
            "Kd": 0.3,
            "target": 65,
            "value": [None, None, None],
        },
        "BOARD_TEMP": {
            "name": "BOARD_TEMP",
            "flag": 0,
            "type": "duty",
            "pwm_min": 0x80,
            "pwm_max": 0xff,
            "Kp": 2,
            "Ki": 0.4,
            "Kd": 0.3,
            "target": 65,
            "value": [None, None, None],
        },
        "SFF_TEMP": {
            "name": "SFF_TEMP",
            "flag": 1,
            "type": "duty",
            "pwm_min": 0x80,
            "pwm_max": 0xff,
            "Kp": 0.1,
            "Ki": 0.4,
            "Kd": 0,
            "target": 60,
            "value": [None, None, None],
        },
    },

    "hyst": {
        "INLET_TEMP": {
            "name": "INLET_TEMP",
            "flag": 1,
            "type": "duty",
            "hyst_min": 50,        # duty
            "hyst_max": 100,       # duty
            "last_hyst_value": 50,  # duty
            "temp_min": 23,
            "temp_max": 40,
            "value": [None, None],
            "rising": {
                23: 50,
                24: 50,
                25: 50,
                26: 53,
                27: 56,
                28: 59,
                29: 62,
                30: 65,
                31: 68,
                32: 71,
                33: 74,
                34: 77,
                35: 80,
                36: 84,
                37: 88,
                38: 92,
                39: 96,
                40: 100,
            },
            "descending": {
                23: 50,
                24: 53,
                25: 56,
                26: 59,
                27: 62,
                28: 65,
                29: 68,
                30: 71,
                31: 74,
                32: 77,
                33: 80,
                34: 84,
                35: 88,
                36: 92,
                37: 96,
                38: 100,
                39: 100,
                40: 100,
            },
        }
    },

    "temps_threshold": {
        "SWITCH_TEMP": {"name": "SWITCH_TEMP", "warning": 100, "critical": 105},
        "INLET_TEMP": {"name": "INLET_TEMP", "warning": 40, "critical": 50},
        "BOARD_TEMP": {"name": "BOARD_TEMP", "warning": 70, "critical": 80},
        "OUTLET_TEMP": {"name": "OUTLET_TEMP", "warning": 70, "critical": 80},
        "CPU_TEMP": {"name": "CPU_TEMP", "warning": 100, "critical": 102},
        "SFF_TEMP": {"name": "SFF_TEMP", "warning": 999, "critical": 1000, "ignore_threshold": 1, "invalid": -10000, "error": -9999},
    },

    "fancontrol_para": {
        "interval": 5,
        "fan_air_flow_monitor": 1,
        "psu_air_flow_monitor": 1,
        "max_pwm": 0xff,
        "min_pwm": 0x80,
        "abnormal_pwm": 0xff,
        "warning_pwm": 0xff,
        "temp_invalid_pid_pwm": 0x80,
        "temp_error_pid_pwm": 0x80,
        "temp_fail_num": 3,
        "check_temp_fail": [
            {"temp_name": "INLET_TEMP"},
            {"temp_name": "SWITCH_TEMP"},
            {"temp_name": "CPU_TEMP"},
        ],
        "temp_warning_num": 3,  # temp over warning 3 times continuously
        "temp_critical_num": 3,  # temp over critical 3 times continuously
        "temp_warning_countdown": 60,  # 5 min warning speed after not warning
        "temp_critical_countdown": 60,  # 5 min full speed after not critical
        "rotor_error_count": 6,  # fan rotor error 6 times continuously
        "inlet_mac_diff": 999,
        "check_crit_reboot_flag": 1,
        "check_crit_reboot_num": 3,
        "check_crit_sleep_time": 20,
        "psu_absent_fullspeed_num": 0xFF,
        "fan_absent_fullspeed_num": 1,
        "rotor_error_fullspeed_num": 1,
    },

    "ledcontrol_para": {
        "interval": 5,
        "checkpsu": 0,  # 0: sys led don't follow psu led
        "checkfan": 0,  # 0: sys led don't follow fan led
        "psu_amber_num": 1,
        "fan_amber_num": 1,
        "board_sys_led": [
            {"led_name": "FRONT_SYS_LED"},
        ],
        "board_psu_led": [
            {"led_name": "FRONT_PSU_LED"},
        ],
        "board_fan_led": [
            {"led_name": "FRONT_FAN_LED"},
        ],
        "psu_air_flow_monitor": 1,
        "fan_air_flow_monitor": 1,
        "psu_air_flow_amber_num": 1,
        "fan_air_flow_amber_num": 1,
    },
    "otp_reboot_judge_file": {
        "otp_switch_reboot_judge_file": "/etc/.otp_switch_reboot_flag",
        "otp_other_reboot_judge_file": "/etc/.otp_other_reboot_flag",
    },
}
