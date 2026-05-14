# coding:utf-8


monitor = {
    "openloop": {
        "linear": {
            "name": "linear",
            "flag": 0,
            "pwm_min": 0x66,
            "pwm_max": 0xcc,
            "K": 11,
            "tin_min": 38,
        },
        "curve": {
            "name": "curve",
            "flag": 1,
            "pwm_min": 0x66,
            "pwm_max": 0xcc,
            "a": -0.0249,
            "b": 8.1615,
            "c": -60.326,
            "tin_min": 25,
        },
    },

    "pid": {
        "CPU_TEMP": {
            "name": "CPU_TEMP",
            "flag": 1,
            "type": "duty",
            "pwm_min": 0x66,
            "pwm_max": 0xcc,
            "Kp": 0.8,
            "Ki": 0.4,
            "Kd": 0.3,
            "target": 85,
            "value": [None, None, None],
        },
        "SWITCH_TEMP": {
            "name": "SWITCH_TEMP",
            "flag": 1,
            "type": "duty",
            "pwm_min": 0x66,
            "pwm_max": 0xcc,
            "Kp": 1.5,
            "Ki": 1,
            "Kd": 0.3,
            "target": 80,
            "value": [None, None, None],
        },
        "OUTLET_TEMP": {
            "name": "OUTLET_TEMP",
            "flag": 1,
            "type": "duty",
            "pwm_min": 0x66,
            "pwm_max": 0xcc,
            "Kp": 2,
            "Ki": 0.4,
            "Kd": 0.3,
            "target": 65,
            "value": [None, None, None],
        },
        "RLM_TEMP": {
            "name": "RLM_TEMP",
            "flag": 1,
            "type": "duty",
            "pwm_min": 0x66,
            "pwm_max": 0xcc,
            "Kp": 0.1,
            "Ki": 0.4,
            "Kd": 0,
            "target": 40,
            "value": [None, None, None],
        },
        "BOARD_TEMP": {
            "name": "BOARD_TEMP",
            "flag": 1,
            "type": "duty",
            "pwm_min": 0x66,
            "pwm_max": 0xcc,
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
            "pwm_min": 0x66,
            "pwm_max": 0xcc,
            "Kp": 1,
            "Ki": 0.4,
            "Kd": 0.3,
            "target": 85,
            "value": [None, None, None],
        },
        "Tout-Tin": {
            "name": "Tout-Tin",
            "flag": 0,
            "type": "duty",
            "pwm_min": 0x66,
            "pwm_max": 0xcc,
            "Kp": 8,
            "Ki": 0.1,
            "Kd": 0.4,
            "target": 12,
            "value": [None, None, None],
        },
        "OE_TEMP": {
            "name": "OE_TEMP",
            "flag": 1,
            "type": "duty",
            "pwm_min": 0x66, # 40%(0x66 = 102, 102/255=0.4)
            "pwm_max": 0xcc, # 80%(0xcc = 204, 204/255=0.8)
            "Kp": 1.5,
            "Ki": 1,
            "Kd": 0.3,
            "target": 85,
            "value": [None, None, None],
        }
    },

    "temps_threshold": {
        "SWITCH_TEMP": {"name": "SWITCH_TEMP", "warning": 100, "critical": 105, "invalid": -100000, "error": -99999},
        "INLET_TEMP": {"name": "INLET_TEMP", "warning": 50, "critical": 55},
        "OUTLET_TEMP": {"name": "OUTLET_TEMP", "warning": 70, "critical": 75},
        "CPU_TEMP": {"name": "CPU_TEMP", "warning": 95, "critical": 99},
        "RLM_TEMP": {"name": "RLM_TEMP", "warning": 50, "critical": 55, "invalid": -100000, "error": -99999},
        #"Tout-Tin": {"name": "Tout-Tin", "warning": 15, "critical": 20},
        "OE_TEMP": {"name": "OE_TEMP", "warning": 95, "critical": 100, "invalid": -100000, "error": -99999},
        "BOARD_TEMP": {"name": "BOARD_TEMP", "warning": 70, "critical": 75},
        "MOS_TEMP": {"name": "MOS_TEMP", "warning": 110, "critical": 125},
    },

    "fancontrol_para": {
        "interval": 5,
        "max_pwm": 0xff,
        "min_pwm": 0x66,
        "abnormal_pwm": 0xcc,
        "warning_pwm": 0xcc,
        "critical_pwm": 0xcc,
        "temp_invalid_pid_pwm": 0x66,
        "temp_error_pid_pwm": 0x66,
        "temp_fail_num": 3,
        "check_temp_fail": [
            {"temp_name": "INLET_TEMP"},
            {"temp_name": "SWITCH_TEMP"},
            {"temp_name": "CPU_TEMP"},
            {"temp_name": "MOS_TEMP"},
            {"temp_name": "BOARD_TEMP"},
            {"temp_name": "OUTLET_TEMP"},
            {"temp_name": "OE_TEMP"},
        ],
        "check_temp_critical_reboot": [
            ["SWITCH_TEMP"],
            ["OE_TEMP"],
            ["INLET_TEMP", "OUTLET_TEMP", "CPU_TEMP", "RLM_TEMP", "BOARD_TEMP", "MOS_TEMP"],
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
        "psu_fan_control": 1,
        "psu_absent_fullspeed_num": 0xFF,
        "fan_absent_fullspeed_num": 1,
        "rotor_error_fullspeed_num": 1,
        "deal_over_temp_reboot_cmd": [
            {"way": "devfile", "loc": "/dev/cpld5", "offset": 0x22, "value": [0x00]}, # disable fan watchdog
            {"way": "devfile", "loc": "/dev/cpld6", "offset": 0x22, "value": [0x00]}, # disable fan watchdog
            {"way": "devfile", "loc": "/dev/cpld1", "offset": 0x15, "value": [0x0c]}, # cpu powroff
        ],
        "deal_over_temp_reboot_pwm": 0x80, # set pwm=50%
    },

    "ledcontrol_para": {
        "interval": 5,
        "checkpsu": 1,  # 0: sys led don't follow psu led
        "checkfan": 1,  # 0: sys led don't follow fan led
        "checksmb": 1,  # 0: sys led don't follow smb led
        "psu_amber_num": 2,
        "psu_green_num": 1,  # only psuerrnum <= this, psu led will be set green
        "fan_amber_num": 1,
        "sysled_check_temp": 0,
        "sysled_check_fw_up": 1,
        "smbled_ctrl": 1,
        "board_sys_led": [
            {"led_name": "FRONT_SYS_LED"},
        ],
        "board_psu_led": [
            {"led_name": "FRONT_PSU_LED"},
        ],
        "board_fan_led": [
            {"led_name": "FRONT_FAN_LED"},
        ],
        "board_smb_led": [
            {"led_name": "SMB_FRU_LED"},
        ],
        "board_smb_fru_led": [
            {"led_name": "FRONT_SMB_LED"},
        ],
        "board_smb_fru_dcdc_sensors": [
            "^MAC_", "^OE", "^RLM", "^SMB_",
        ],
        "board_smb_fru_temps": [
            "SWITCH_TEMP", "BOARD_TEMP", "MOS_TEMP", "OE_TEMP", "RLM_TEMP",
        ],
        "board_scm_fru_led": [
            {"led_name": "SCM_FRU_LED"},
        ],
        "board_scm_fru_dcdc_sensors": [
            "^SCM_", "^OCM_",
        ],
        "board_scm_fru_temps": [
            "CPU_TEMP", "INLET_TEMP",
        ],
        "psu_air_flow_monitor": 0,
        "fan_air_flow_monitor": 0,
        "psu_air_flow_amber_num": 0,
        "fan_air_flow_amber_num": 0,
    },

    "fw_upgrade_check": [
        [
            {"gettype": "file_exist", "judge_file": "/etc/sonic/.doing_fw_upg", "okval": True},
        ],
    ],

    "otp_reboot_judge_file": {
        "otp_switch_reboot_judge_file": "/etc/.otp_switch_reboot_flag",
        "otp_other_reboot_judge_file": "/etc/.otp_other_reboot_flag",
    },
}
