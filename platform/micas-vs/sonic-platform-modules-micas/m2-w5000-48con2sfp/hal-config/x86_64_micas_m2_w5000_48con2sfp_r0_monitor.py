# coding:utf-8


monitor = {
    "openloop": {
        "linear": {
            "name": "linear",
            "flag": 0,
            "pwm_min": 0x55,
            "pwm_max": 0xff,
            "K": 11,
            "tin_min": 38,
        },
        "curve": {
            "name": "curve",
            "flag": 1,
            "pwm_min": 0x66,
            "pwm_max": 0xcd,
            "a": 0.047,
            "b": 1.8621,
            "c": 26.091,
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
            "Kp": 0.8,
            "Ki": 0.4,
            "Kd": 0.3,
            "target": 80,
            "value": [None, None, None],
        },
    },

    "temps_threshold": {
        "INLET_TEMP": {"name": "INLET_TEMP", "warning": 50, "critical": 55},
        "OUTLET_TEMP": {"name": "OUTLET_TEMP", "warning": 80, "critical": 85},
        "CPU_TEMP": {"name": "CPU_TEMP", "warning": 90, "critical": 95},
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
            {"temp_name": "OUTLET_TEMP"},
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
        "psu_absent_fullspeed_num": 1,
        "fan_absent_fullspeed_num": 1,
        "rotor_error_fullspeed_num": 1,
        "psu_fan_control": 0,
    },

    "otp_reboot_judge_file": {
        "otp_switch_reboot_judge_file": "/etc/.otp_switch_reboot_flag",
        "otp_other_reboot_judge_file": "/etc/.otp_other_reboot_flag",
    },
}
