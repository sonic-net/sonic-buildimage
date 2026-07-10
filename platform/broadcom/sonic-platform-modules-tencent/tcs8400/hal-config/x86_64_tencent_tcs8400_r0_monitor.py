#!/usr/bin/python3

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
            "flag": 1,
            "pwm_min": 0x80,
            "pwm_max": 0xff,
            "a": 0.369,
            "b": -15.657,
            "c": 289,
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
            "pwm_min": 0x80,
            "pwm_max": 0xff,
            "Kp": 1.5,
            "Ki": 1,
            "Kd": 0.3,
            "target": 90,
            "value": [None, None, None],
        },
        "OUTLET_TEMP": {
            "name": "OUTLET_TEMP",
            "flag": 1,
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

    "temps_threshold": {
        "SWITCH_TEMP": {"name": "SWITCH_TEMP", "warning": 100, "critical": 105, "invalid": -100000, "error": -99999},
        "INLET_TEMP": {"name": "INLET_TEMP", "warning": 40, "critical": 50, "fix": -3},
        "BOARD_TEMP": {"name": "BOARD_TEMP", "warning": 70, "critical": 75},
        "OUTLET_TEMP": {"name": "OUTLET_TEMP", "warning": 70, "critical": 75},
        "CPU_TEMP": {"name": "CPU_TEMP", "warning": 100, "critical": 102},
        "SFF_TEMP": {"name": "SFF_TEMP", "warning": 999, "critical": 1000, "ignore_threshold": 1, "invalid": -10000, "error": -9999},
    },

    "fancontrol_para": {
        "interval": 5,
        "fan_status_interval": 0.5,
        "fan_air_flow_monitor": 1,
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
        "rotor_error_count": 2,  # fan rotor error 2 times continuously
        "inlet_mac_diff": 999,
        "check_crit_reboot_flag": 1,
        "check_crit_reboot_num": 3,
        "check_crit_sleep_time": 20,
        "psu_absent_fullspeed_num": 0xFF,
        "fan_absent_fullspeed_num": 1,
        "rotor_error_fullspeed_num": 1,
        "fan_plug_in_default_countdown": 0,  # no use
        "fan_plug_in_pwm": 0x80,  # fan plug in pwd
        "deal_fan_error": 1,
        "deal_fan_error_conf": {
            "countdown": 2,
            "FAN1": [
                {"name":"FAN1", "pwm":0xff},
                {"name":"FAN2", "pwm":0x80},
                {"name":"FAN3", "pwm":0x80},
                {"name":"FAN4", "pwm":0x80},
                {"name":"FAN5", "pwm":0x80},
                {"name":"FAN6", "pwm":0x80},
            ],
            "FAN2": [
                {"name":"FAN1", "pwm":0x80},
                {"name":"FAN2", "pwm":0xff},
                {"name":"FAN3", "pwm":0x80},
                {"name":"FAN4", "pwm":0x80},
                {"name":"FAN5", "pwm":0x80},
                {"name":"FAN6", "pwm":0x80},
            ],
            "FAN3": [
                {"name":"FAN1", "pwm":0x80},
                {"name":"FAN2", "pwm":0x80},
                {"name":"FAN3", "pwm":0xff},
                {"name":"FAN4", "pwm":0x80},
                {"name":"FAN5", "pwm":0x80},
                {"name":"FAN6", "pwm":0x80},
            ],
            "FAN4": [
                {"name":"FAN1", "pwm":0x80},
                {"name":"FAN2", "pwm":0x80},
                {"name":"FAN3", "pwm":0x80},
                {"name":"FAN4", "pwm":0xff},
                {"name":"FAN5", "pwm":0x80},
                {"name":"FAN6", "pwm":0x80},
            ],
            "FAN5": [
                {"name":"FAN1", "pwm":0x80},
                {"name":"FAN2", "pwm":0x80},
                {"name":"FAN3", "pwm":0x80},
                {"name":"FAN4", "pwm":0x80},
                {"name":"FAN5", "pwm":0xff},
                {"name":"FAN6", "pwm":0x80},
            ],
            "FAN6": [
                {"name":"FAN1", "pwm":0x80},
                {"name":"FAN2", "pwm":0x80},
                {"name":"FAN3", "pwm":0x80},
                {"name":"FAN4", "pwm":0x80},
                {"name":"FAN5", "pwm":0x80},
                {"name":"FAN6", "pwm":0xff},
            ],
        },
    },

    "ledcontrol_para": {
        "interval": 5,
        "checkpsu": 1,
        "checkfan": 1,
        "psu_yellow_num": 1,
        "fan_yellow_num": 1,
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
        "fan_mix_list": [
            {"FAN24K4056-F", "FAN24K4056S-F"}
        ]
    },
    "intelligent_monitor_para": {
        "interval": 60,
    },

    "dcdc_monitor_whitelist": {             #not monitor when checkbit equal okval
        "QSFP56_VDD3.3V_A": {"gettype": "i2c", "bus": 2, "addr": 0x2d, "offset": 0x45, "checkbit": 0, "okval": 0},
        "QSFP56_VDD3.3V_B": {"gettype": "i2c", "bus": 2, "addr": 0x2d, "offset": 0x45, "checkbit": 1, "okval": 0},
        "QSFPDD_VDD3.3V_A": {"gettype": "i2c", "bus": 2, "addr": 0x2d, "offset": 0x46, "checkbit": 0, "okval": 0},
        "QSFPDD_VDD3.3V_B": {"gettype": "i2c", "bus": 2, "addr": 0x2d, "offset": 0x46, "checkbit": 1, "okval": 0},
    },
}
