# coding:utf-8

monitor = {
    "openloop": {
        "linear": {
            "name": "linear",
            "flag": 0,
            "pwm_min": 0x66,
            "pwm_max": 0xff,
            "K": 11,
            "tin_min": 38,
        },
        "curve": {
            "name": "curve",
            "flag": 1,
            "pwm_min": 0x66,
            "pwm_max": 0xff,
            "a": 0.4,
            "b": -15.7,
            "c": 245.3,
            "tin_min": 25,
        },
    },

    "pid": {
        "CPU_TEMP": {
            "name": "CPU_TEMP",
            "flag": 1,
            "type": "duty",
            "pwm_min": 0x66,
            "pwm_max": 0xff,
            "Kp": 0.5,
            "Ki": 0.3,
            "Kd": 0.3,
            "target": 80,
            "value": [None, None, None],
        },
        "SWITCH_TEMP": {
            "name": "SWITCH_TEMP",
            "flag": 1,
            "type": "duty",
            "pwm_min": 0x66,
            "pwm_max": 0xff,
            "Kp": 0.5,
            "Ki": 0.3,
            "Kd": 0.3,
            "target": 90,
            "value": [None, None, None],
        },
        "OUTLET_TEMP": {
            "name": "OUTLET_TEMP",
            "flag": 1,
            "type": "duty",
            "pwm_min": 0x66,
            "pwm_max": 0xff,
            "Kp": 0,
            "Ki": 0.4,
            "Kd": 0.3,
            "target": 65,
            "value": [None, None, None],
        },
        "SFF_TEMP": {
            "name": "SFF_TEMP",
            "flag": 1,
            "type": "duty",
            "pwm_min": 0x66,
            "pwm_max": 0xff,
            "Kp": 2,
            "Ki": 0.3,
            "Kd": 0,
            "target": 63,
            "value": [None, None, None],
        },
        "BOARD_TEMP": {
            "name": "BOARD_TEMP",
            "flag": 1,
            "type": "duty",
            "pwm_min": 0x66,
            "pwm_max": 0xff,
            "Kp": 2,
            "Ki": 0.4,
            "Kd": 0.3,
            "target": 80,
            "value": [None, None, None],
        },
        "MOS_TEMP": {
            "name": "MOS_TEMP",
            "flag": 1,
            "type": "duty",
            "pwm_min": 0x66,
            "pwm_max": 0xff,
            "Kp": 1,
            "Ki": 0.1,
            "Kd": 0.3,
            "target": 95,
            "value": [None, None, None],
        },
    },

    "temps_threshold": {
        "CPU_TEMP": {"name": "CPU_TEMP", "warning": 90, "critical": 95},
        "SWITCH_TEMP": {"name": "SWITCH_TEMP", "warning": 100, "critical": 105, "alert": 110, "invalid": -100000, "error": -99999},
        "INLET_TEMP": {"name": "INLET_TEMP", "warning": 60, "critical": 70},
        "OUTLET_TEMP": {"name": "OUTLET_TEMP", "warning": 70, "critical": 75},
        "BOARD_TEMP": {"name": "BOARD_TEMP", "warning": 95, "critical": 105},
        "MOS_TEMP": {"name": "MOS_TEMP", "warning": 110, "critical": 125},
        "SFF_TEMP": {"name": "SFF_TEMP", "warning": 999, "critical": 1000, "ignore_threshold": 1, "invalid": -10000, "error": -9999},
    },

    "fancontrol_para": {
        "interval": 5,
        "max_pwm": 0xff,
        "min_pwm": 0x66,
        "abnormal_pwm": 0xff,
        "warning_pwm": 0xff,
        "temp_invalid_pid_pwm": 0x66,
        "temp_error_pid_pwm": 0x66,
        "temp_fail_num": 3,
        "check_temp_fail": [
            {"temp_name": "INLET_TEMP"},
            {"temp_name": "SWITCH_TEMP"},
            {"temp_name": "CPU_TEMP"},
            {"temp_name": "OUTLET_TEMP"},
            {"temp_name": "BOARD_TEMP"},
            {"temp_name": "MOS_TEMP"},
        ],
        "temp_warning_num": 3,  # temp over warning 3 times continuously
        "temp_critical_num": 3,  # temp over critical 3 times continuously
        "temp_alert_num": 1,  # temp over alert 1 times continuously
        "temp_warning_countdown": 60,  # 5 min warning speed after not warning
        "temp_critical_countdown": 60,  # 5 min full speed after not critical
        "rotor_error_count": 3,  # fan rotor error 3 times continuously
        "inlet_mac_diff": 999,
        "check_crit_reboot_flag": 1,
        "check_crit_reboot_num": 3,
        "check_crit_sleep_time": 5,
        "check_temp_critical_reboot": [
            ["SWITCH_TEMP"],
            ["INLET_TEMP", "OUTLET_TEMP", "BOARD_TEMP", "CPU_TEMP", "MOS_TEMP"],
        ],
        "check_alert_reboot_flag": 1,
        "check_alert_reboot_num": 3,
        "check_alert_sleep_time": 0.05,
        "check_temp_alert_reboot": [
            ["SWITCH_TEMP"],
        ],
        "psu_fan_control": 1,
        "psu_absent_fullspeed_num": 0xFF,
        "fan_absent_fullspeed_num": 3,
        "rotor_error_fullspeed_num": 6,
        "deal_over_temp_reboot_cmd": [
            {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x86, "value": 0x06},
        ]
    },

    "ledcontrol_para": {
        "interval": 5,
        "checkpsu": 0,  # 1: sys led follow psu led
        "checkfan": 0,  # 1: sys led follow fan led
        "psu_amber_num": 1,
        "fan_amber_num": 1,
        "bios_boot_source_check": 0,
        "bmcled_ctrl": 0,
        "board_sys_led": [
            {"led_name": "FRONT_SYS_LED"},
        ],
        "board_psu_led": [
            {"led_name": "FRONT_PSU_LED"},
        ],
        "board_fan_led": [
            {"led_name": "FRONT_FAN_LED"},
        ],
        "board_bmc_led": [
            {"led_name": "FRONT_BMC_LED"},
        ],
        "psu_air_flow_monitor": 0,
        "fan_air_flow_monitor": 0,
        "psu_air_flow_amber_num": 0,
        "fan_air_flow_amber_num": 0,
    },

    "otp_reboot_judge_file": {
        "otp_switch_reboot_judge_file": "/etc/.otp_switch_reboot_flag",
        "otp_other_reboot_judge_file": "/etc/.otp_other_reboot_flag",
    },
}
