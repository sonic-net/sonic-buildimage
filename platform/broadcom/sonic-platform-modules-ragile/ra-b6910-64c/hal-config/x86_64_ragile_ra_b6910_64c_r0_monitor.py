# coding:utf-8


monitor = {
    "openloop": {
        "linear": {
            "name": "linear",
            "flag": 1,
            "pwm_min": 0x80,
            "pwm_max": 0xff,
            "K": 14,
            "tin_min": 34,
        },
        "curve": {
            "name": "curve",
            "flag": 0,
            "pwm_min": 0x80,
            "pwm_max": 0xff,
            "a": 0.183,
            "b": -6.88,
            "c": 120,
            "tin_min": 25,
        },
    },

    "pid": {
        "CPU_TEMP": {
            "name": "CPU_TEMP",
            "flag": 0,
            "type": "duty",
            "pwm_min": 0x80,
            "pwm_max": 0xff,
            "Kp": 1.5,
            "Ki": 1,
            "Kd": 0.3,
            "target": 90,
            "value": [None, None, None],
        },
        "SWITCH_TEMP": {
            "name": "SWITCH_TEMP",
            "flag": 0,
            "type": "duty",
            "pwm_min": 0x80,
            "pwm_max": 0xff,
            "Kp": 1.5,
            "Ki": 1,
            "Kd": 0.3,
            "target": 90,
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
            "target": 68,
            "value": [None, None, None],
        },
    },

    "temps_threshold": {
        "SWITCH_TEMP": {"name": "SWITCH_TEMP", "warning": 100, "critical": 105},
        "INLET_TEMP": {"name": "INLET_TEMP", "warning": 70, "critical": 80},
        "BOARD_TEMP": {"name": "BOARD_TEMP", "warning": 85, "critical": 90},
        "OUTLET_TEMP": {"name": "OUTLET_TEMP", "warning": 85, "critical": 90},
        "CPU_TEMP": {"name": "CPU_TEMP", "warning": 85, "critical": 100},
        "SFF_TEMP": {"name": "SFF_TEMP", "warning": 999, "critical": 1000, "ignore_threshold": 1, "invalid": -10000, "error": -9999},
    },

    "fancontrol_para": {
        "interval": 20,
        "max_pwm": 0xff,
        "min_pwm": 0x80,
        "abnormal_pwm": 0xbb,
        "temp_fail_num": 3,
        "check_temp_fail": [
            {"temp_name": "INLET_TEMP"},
        ],
        "inlet_mac_diff": 40,
        "check_crit_reboot_num": 3,
        "check_crit_sleep_time": 20,
        "psu_absent_fullspeed_num": 1,
        "fan_absent_fullspeed_num": 1,
        "rotor_error_fullspeed_num": 1,
        "psu_fan_control": 1,
        "temp_warning_countdown": 15,  # 5 min warning speed after not warning
        "temp_critical_countdown": 15,  # 5 min full speed after not critical
    },

    "ledcontrol_para": {
        "interval": 10,
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
    },

    "otp_reboot_judge_file": {
        "otp_switch_reboot_judge_file": "/etc/.otp_switch_reboot_flag",
        "otp_other_reboot_judge_file": "/etc/.otp_other_reboot_flag",
    },
}
