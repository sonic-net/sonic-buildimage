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
            "pwm_min": 0x5a,
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
            "pwm_min": 0x5a,
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
            "pwm_min": 0x5a,
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
            "pwm_min": 0x5a,
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
        "OUTLET_TEMP": {"name": "OUTLET_TEMP", "warning": 70, "critical": 75},
        "CPU_TEMP": {"name": "CPU_TEMP", "warning": 100, "critical": 102},
        "SFF_TEMP": {"name": "SFF_TEMP", "warning": 999, "critical": 1000, "ignore_threshold": 1, "invalid": -10000, "error": -9999},
    },

    "fancontrol_para": {
        "interval": 5,
        "fan_air_flow_monitor": 1,
        "max_pwm": 0xff,
        "min_pwm": 0x5a,
        "abnormal_pwm": 0xff,
        "warning_pwm": 0xff,
        "temp_invalid_pid_pwm": 0x5a,
        "temp_error_pid_pwm": 0x5a,
        "temp_fail_num": 3,
        "check_temp_fail" : [
            {"temp_name" : "INLET_TEMP"},
            {"temp_name" : "SWITCH_TEMP"},
            {"temp_name" : "CPU_TEMP"},
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
        "checkpsu": 1,
        "checkfan": 1,
        "psu_yellow_num": 1,
        "fan_yellow_num": 1,
        "board_sys_led" : [
            {"led_name" : "FRONT_SYS_LED"},
        ],
        "board_psu_led" : [
            {"led_name" : "FRONT_PSU_LED"},
        ],
        "board_fan_led" : [
            {"led_name" : "FRONT_FAN_LED"},
        ],
        "psu_air_flow_monitor": 1,
        "fan_air_flow_monitor": 1,
    },

    "intelligent_monitor_para": {
        "interval": 60,
    },

    "dcdc_monitor_whitelist": {             #not monitor when checkbit equal okval
        "UPORT_QSFP56_VDD3.3V_A": {"gettype": "i2c", "bus": 60, "addr": 0x3d, "offset": 0x80, "checkbit": 0, "okval": 0},
        "UPORT_QSFP56_VDD3.3V_B": {"gettype": "i2c", "bus": 60, "addr": 0x3d, "offset": 0x80, "checkbit": 1, "okval": 0},
        "UPORT_QSFP56_VDD3.3V_C": {"gettype": "i2c", "bus": 60, "addr": 0x3d, "offset": 0x80, "checkbit": 2, "okval": 0},
        "UPORT_QSFP56_VDD3.3V_D": {"gettype": "i2c", "bus": 60, "addr": 0x3d, "offset": 0x80, "checkbit": 3, "okval": 0},
        "DPORT_QSFP56_VDD3.3V_A": {"gettype": "i2c", "bus": 111, "addr": 0x3d, "offset": 0x80, "checkbit": 0, "okval": 0},
        "DPORT_QSFP56_VDD3.3V_B": {"gettype": "i2c", "bus": 111, "addr": 0x3d, "offset": 0x80, "checkbit": 1, "okval": 0},
        "DPORT_QSFP56_VDD3.3V_C": {"gettype": "i2c", "bus": 111, "addr": 0x3d, "offset": 0x80, "checkbit": 2, "okval": 0},
        "DPORT_QSFP56_VDD3.3V_D": {"gettype": "i2c", "bus": 111, "addr": 0x3d, "offset": 0x80, "checkbit": 3, "okval": 0},
        "MAC_QSFPDD_VDD3.3V_A": {"gettype": "i2c", "bus": 77, "addr": 0x1d, "offset": 0x7c, "checkbit": 0, "okval": 0},
        "MAC_QSFPDD_VDD3.3V_B": {"gettype": "i2c", "bus": 77, "addr": 0x1d, "offset": 0x7c, "checkbit": 1, "okval": 0},
        "MAC_QSFPDD_VDD3.3V_C": {"gettype": "i2c", "bus": 77, "addr": 0x1d, "offset": 0x7c, "checkbit": 2, "okval": 0},
        "MAC_QSFPDD_VDD3.3V_D": {"gettype": "i2c", "bus": 77, "addr": 0x1d, "offset": 0x7c, "checkbit": 3, "okval": 0},
        "MAC_QSFPDD_VDD3.3V_E": {"gettype": "i2c", "bus": 77, "addr": 0x1d, "offset": 0x7c, "checkbit": 4, "okval": 0},
        "MAC_QSFPDD_VDD3.3V_F": {"gettype": "i2c", "bus": 77, "addr": 0x1d, "offset": 0x7c, "checkbit": 5, "okval": 0},
        "MAC_QSFPDD_VDD3.3V_G": {"gettype": "i2c", "bus": 77, "addr": 0x1d, "offset": 0x7c, "checkbit": 6, "okval": 0},
        "MAC_QSFPDD_VDD3.3V_H": {"gettype": "i2c", "bus": 77, "addr": 0x1d, "offset": 0x7c, "checkbit": 7, "okval": 0},
    },
}
