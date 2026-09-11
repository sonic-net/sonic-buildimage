# coding:utf-8

class fan_pwn:
    FAN_PWN_MIN = 0x66
    FAN_PWN_MAX = 0xff
    FAN_PWN_ERROR = 0x99
    FAN_PWN_WARNING = 0xcc

monitor = {
    "openloop": {
        "linear": {
            "name": "linear",
            "flag": 0,
            "pwm_min": fan_pwn.FAN_PWN_MIN,
            "pwm_max": fan_pwn.FAN_PWN_MAX,
            "K": 11,
            "tin_min": 38,
        },
        "curve": {
            "name": "curve",
            "flag": 0,
            "pwm_min": fan_pwn.FAN_PWN_MIN,
            "pwm_max": fan_pwn.FAN_PWN_MAX,
            "a": 0.22,
            "b": -3.1,
            "c": 43,
            "tin_min": 27,
        },
    },

    "pid": {
        "OUTLET_TEMP": {
            "name": "OUTLET_TEMP",
            "flag": 1,
            "type": "duty",
            "pwm_min": fan_pwn.FAN_PWN_MIN,
            "pwm_max": fan_pwn.FAN_PWN_MAX,
            "Kp": 2,
            "Ki": 0.4,
            "Kd": 0.3,
            "target": 65,
            "value": [None, None, None],
        },
        "MOS_TEMP": {
            "name": "MOS_TEMP",
            "flag": 1,
            "type": "duty",
            "pwm_min": fan_pwn.FAN_PWN_MIN,
            "pwm_max": fan_pwn.FAN_PWN_MAX,
            "Kp": 1,
            "Ki": 0.4,
            "Kd": 0.3,
            "target": 95,
            "value": [None, None, None],
        },
        "BOARD_TEMP" : {
            "name": "BOARD_TEMP",
            "flag": 1,
            "type": "duty",
            "pwm_min": fan_pwn.FAN_PWN_MIN,
            "pwm_max": fan_pwn.FAN_PWN_MAX,
            "Kp": 2,
            "Ki": 0.4,
            "Kd": 0.3,
            "target": 65,
            "value": [None, None, None],
        },
    },

    "temps_threshold": {
        "SWITCH_TEMP": {"name": "SWITCH_TEMP", "warning": 100, "critical": 105, "invalid": -100000, "error": -99999},
        "OUTLET_TEMP": {"name": "OUTLET_TEMP", "warning": 70, "critical": 75},
        "CPU_TEMP": {"name": "CPU_TEMP", "warning": 95, "critical": 99},
        "MOS_TEMP": {"name": "MOS_TEMP", "warning": 110, "critical": 125},
        "BOARD_TEMP": {"name": "BOARD_TEMP", "warning": 70, "critical": 75},
        "SFF_TEMP": {"name": "SFF_TEMP", "warning": 65, "critical": 70, "invalid": -10000, "error": -9999},
    },

    "fancontrol_para": {
        "interval": 5,
        "max_pwm": fan_pwn.FAN_PWN_MAX,
        "min_pwm": fan_pwn.FAN_PWN_MIN,
        "abnormal_pwm": fan_pwn.FAN_PWN_MAX,
        "warning_pwm": fan_pwn.FAN_PWN_WARNING,
        "temp_invalid_pid_pwm": fan_pwn.FAN_PWN_MIN,
        "temp_error_pid_pwm": fan_pwn.FAN_PWN_MIN,
        "temp_fail_num": 3,
        "check_temp_fail": [
            {"temp_name": "SWITCH_TEMP"},
            {"temp_name": "CPU_TEMP"},
            {"temp_name": "MOS_TEMP"},
            {"temp_name": "OUTLET_TEMP"},
            {"temp_name": "BOARD_TEMP"},
        ],
        "temp_warning_num": 3,  # temp over warning 3 times continuously
        "temp_critical_num": 3,  # temp over critical 3 times continuously
        "temp_warning_countdown": 60,  # 5 min warning speed after not warning
        "temp_critical_countdown": 60,  # 5 min full speed after not critical
        "rotor_error_count": 6,  # fan rotor error 6 times continuously
        "check_inlet_mac_diff": 0,
        "inlet_mac_diff": 999,
        "check_crit_reboot_flag": 1,
        "check_crit_reboot_num": 3,
        "check_crit_sleep_time": 20,
        "psu_fan_control": 0,
        "psu_absent_fullspeed_num": 0xFF,
        "fan_absent_fullspeed_num": 1,
        "rotor_error_fullspeed_num": 1,
        "deal_all_fan_error_method_flag": 0,
    },

    "ledcontrol_para": {
        "interval":5,
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
