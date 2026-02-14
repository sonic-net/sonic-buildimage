# coding:utf-8


monitor = {
    "openloop": {
        "linear": {
            "name": "linear",
            "flag": 0,
            "pwm_min": 0x8d,
            "pwm_max": 0xff,
            "K": 11,
            "tin_min": 28,
        },
        "curve": {
            "name": "curve",
            "flag": 1,
            "pwm_min": 0x8d,
            "pwm_max": 0xff,
            "a": 0.1255,
            "b": -1.2036,
            "c": 76,
            "tin_min": 25,
        },
    },

    "pid": {
        "CPU_TEMP": {
            "name": "CPU_TEMP",
            "flag": 1,
            "type": "duty",
            "pwm_min": 0x8d,
            "pwm_max": 0xff,
            "Kp": 1.5,
            "Ki": 1,
            "Kd": 0.3,
            "target": 80,
            "value": [None, None, None],
        },
        "SWITCH_TEMP": {
            "name": "SWITCH_TEMP",
            "flag": 1,
            "type": "duty",
            "pwm_min": 0x8d,
            "pwm_max": 0xff,
            "Kp": 1.5,
            "Ki": 0.5,
            "Kd": 0.3,
            "target": 78,
            "value": [None, None, None],
        },
        "OUTLET_TEMP": {
            "name": "OUTLET_TEMP",
            "flag": 1,
            "type": "duty",
            "pwm_min": 0x8d,
            "pwm_max": 0xff,
            "Kp": 2,
            "Ki": 0.4,
            "Kd": 0.3,
            "target": 65,
            "value": [None, None, None],
        },
        "BOARD_TEMP": {
            "name": "BOARD_TEMP",
            "flag": 1,
            "type": "duty",
            "pwm_min": 0x8d,
            "pwm_max": 0xff,
            "Kp": 2,
            "Ki": 0.4,
            "Kd": 0.3,
            "target": 83,
            "value": [None, None, None],
        },
        "SFF_TEMP": {
            "name": "SFF_TEMP",
            "flag": 1,
            "type": "duty",
            "pwm_min": 0x8d,
            "pwm_max": 0xff,
            "Kp": 0.3,
            "Ki": 0.4,
            "Kd": 0,
            "target": 65,
            "value": [None, None, None],
        },
    },

    "temps_threshold": {
        "SWITCH_TEMP": {"name": "SWITCH_TEMP", "warning": 100, "critical": 110},
        "INLET_TEMP": {"name": "INLET_TEMP", "warning": 45, "critical": 50, "fix": -4},
        "BOARD_TEMP": {"name": "BOARD_TEMP", "warning": 90, "critical": 95},
        "OUTLET_TEMP": {"name": "OUTLET_TEMP", "warning": 70, "critical": 75},
        "CPU_TEMP": {"name": "CPU_TEMP", "warning": 87, "critical": 93},
        "SFF_TEMP": {"name": "SFF_TEMP", "warning": 999, "critical": 1000, "ignore_threshold": 1, "invalid": -10000, "error": -9999},
    },

    "fancontrol_para": {
        "interval": 5,
        "fan_status_interval": 0.5,
        "max_pwm": 0xff,
        "min_pwm": 0x8d,
        "abnormal_pwm": 0xff,
        "warning_pwm": 0xff,
        "temp_invalid_pid_pwm": 0x8d,
        "temp_error_pid_pwm": 0x8d,
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

        "deal_all_fan_error_method_flag": 1,
        "all_fan_error_switch_temp_critical_temp": 95,
        "all_fan_error_recover_log": "Power off base and mac board.",
        "all_fan_error_recover_cmd": "dfd_debug io_wr 0x947 0xfa",
        "all_fan_error_check_crit_reboot_num": 3,
        "all_fan_error_check_crit_sleep_time": 2,

        "psu_absent_fullspeed_num": 0xFF,  #Full-speed switchover - Number of Psus absent - Disables the full-speed switchover of Psus absent
        "fan_absent_fullspeed_num": 2,  # Full speed rotation of the system -fan Indicates the number of absent bits
        "rotor_error_fullspeed_num": 3,  # System full speed - Number of motor failures
        "psu_fan_control": 1,  # Enable the psu fan control function
        "deal_fan_error": 1,  # Handling Fan anomalies
        "deal_fan_error_conf": {
            "countdown": 2,     # max time:(2-1)*Speed regulation period
            "FAN1": [
                {"name": "FAN1", "pwm": 0xff},
                {"name": "FAN2", "pwm": 0x99},
                {"name": "FAN3", "pwm": 0x99},
                {"name": "FAN4", "pwm": 0x99},
                {"name": "FAN5", "pwm": 0x99},
                {"name": "FAN6", "pwm": 0x99},
            ],
            "FAN2": [
                {"name": "FAN1", "pwm": 0x99},
                {"name": "FAN2", "pwm": 0xff},
                {"name": "FAN3", "pwm": 0x99},
                {"name": "FAN4", "pwm": 0x99},
                {"name": "FAN5", "pwm": 0x99},
                {"name": "FAN6", "pwm": 0x99},
            ],
            "FAN3": [
                {"name": "FAN1", "pwm": 0x99},
                {"name": "FAN2", "pwm": 0x99},
                {"name": "FAN3", "pwm": 0xff},
                {"name": "FAN4", "pwm": 0x99},
                {"name": "FAN5", "pwm": 0x99},
                {"name": "FAN6", "pwm": 0x99},
            ],
            "FAN4": [
                {"name": "FAN1", "pwm": 0x99},
                {"name": "FAN2", "pwm": 0x99},
                {"name": "FAN3", "pwm": 0x99},
                {"name": "FAN4", "pwm": 0xff},
                {"name": "FAN5", "pwm": 0x99},
                {"name": "FAN6", "pwm": 0x99},
            ],
            "FAN5": [
                {"name": "FAN1", "pwm": 0x99},
                {"name": "FAN2", "pwm": 0x99},
                {"name": "FAN3", "pwm": 0x99},
                {"name": "FAN4", "pwm": 0x99},
                {"name": "FAN5", "pwm": 0xff},
                {"name": "FAN6", "pwm": 0x99},
            ],
            "FAN6": [
                {"name": "FAN1", "pwm": 0x99},
                {"name": "FAN2", "pwm": 0x99},
                {"name": "FAN3", "pwm": 0x99},
                {"name": "FAN4", "pwm": 0x99},
                {"name": "FAN5", "pwm": 0x99},
                {"name": "FAN6", "pwm": 0xff},
            ],
        },
    },

    "ledcontrol_para": {
        "interval": 5,
        "checkpsu": 0,  # 0: sys led don't follow psu led
        "checkfan": 0,  # 0: sys led don't follow fan led
        "psu_amber_num": 1,
        "fan_amber_num": 1,
        "board_sys_led": [
            {"led_name": "BOARD_SYS_LED"},
        ],
        "board_psu_led": [
            {"led_name": "BOARD_PSU_LED"},
        ],
        "board_fan_led": [
            {"led_name": "BOARD_FAN_LED"},
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
