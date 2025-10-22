#!/usr/bin/python
# -*- coding: UTF-8 -*-
"""
Device config file
"""
try:
    import re
    import os
    import logging
    import syslog
    import subprocess
except ImportError as error:
    raise ImportError(str(error) + "- required module not found")


class Devcfg(object):
    """
    Device config
    """
    ##################################
    # directory
    ##################################
    MNT_DIR = "/mnt/"
    EFI_MOUNT_DIR = MNT_DIR
    EFI_DIR = EFI_MOUNT_DIR + "EFI/"
    EFI_PARTITION = "/dev/sda1"
    EFI_BOOT_DIR = EFI_DIR + "BOOT/"

    CLASS_DIR = "/sys/class/"
    SWITCH_DIR = "/sys/switch/"

    CPLD_DIR = SWITCH_DIR + "cpld/"
    DEBUG_DIR = SWITCH_DIR + "debug/"
    EEPROM_DIR = SWITCH_DIR + "syseeprom/"
    FAN_DIR = SWITCH_DIR + "fan/"
    SENSOR_DIR = SWITCH_DIR + "sensor/"
    SFP_DIR = SWITCH_DIR + "transceiver/"
    SLOT_DIR = SWITCH_DIR + "slot/"
    PSU_DIR = SWITCH_DIR + "psu/"
    WATCHDOG_DIR = SWITCH_DIR + "watchdog/"
    SYSLED_DIR = SWITCH_DIR + "sysled/"

    HWMON_DIR = CLASS_DIR + "hwmon/"
    I2C_DIR = CLASS_DIR + "i2c-adapter/i2c-{}/{}-0050/eeprom"

    DEBUG_CPLD_DIR = DEBUG_DIR + "cpld/"
    DEBUG_FAN_DIR = DEBUG_DIR + "fan/"
    DEBUG_SENSOR_DIR = DEBUG_DIR + "sensor/"

    EEPROM_DATA_DIR = EEPROM_DIR + "eeprom"

    PSU_SUB_PATH = PSU_DIR + "psu{}/"
    FAN_SUB_PATH = FAN_DIR + "fan{}/"

    SFP_BASH_PATH = SFP_DIR + 'eth{0}/'
    SFP_MAX_TEMP = SFP_DIR + 'max_temp'

    #MAX6696_SPOT_DIR = SENSOR_DIR + "temp%d/"

    SPEED_PWM_FILE = DEBUG_FAN_DIR + "fan_speed_pwm"
    SECOND_VOL_MAIN_PATH = DEBUG_SENSOR_DIR + "adm1166_0/"

    SENSOR_IN0_TEMPERATURE = SENSOR_DIR + "in0/in_temperature"

    #BASE_VAL_PATH = SFP_DIR + "Eth{0}GE{1}/"
    VMECPU_DIR = "/usr/local/bin/vmecpu"
    VME_DIR = "/usr/local/bin/vme"

    BIOS_VERSION_PATTERN = r"BIOS Version *([^\n]+)"

    ##################################
    # parameter value
    ##################################
    PORT_START = 1
    PORT_END = 56
    SFP_START = 0
    SFP_END = 48
    QSFP_START = 48
    QSFP_END = 56

    PSU_IS_1600W = True

    # SFP_TYPE, SFP_START, SFP_END
    SFP_INFO = [
        {"type": "SFP", "start": 1, "end": 48},
        {"type": "QSFP", "start": 49, "end": 56}
    ]

    # definitions of the offset and width for values in DOM info eeprom
    QSFP_CHANNEL_RX_LOS_STATUS_OFFSET = 3
    QSFP_CHANNEL_RX_LOS_STATUS_WIDTH = 1
    QSFP_CHANNEL_TX_FAULT_STATUS_OFFSET = 4
    QSFP_CHANNEL_TX_FAULT_STATUS_WIDTH = 1
    QSFP_CHANNEL_TX_DISABLE_OFFSET = 86
    QSFP_CHANNEL_TX_DISABLE_WIDTH = 1

    CHASSIS_COMPONENTS = [
        ["BIOS", ("Performs initialization of hardware components during booting")],
        ["CPU-CPLD", "Used for managing CPU board devices and power"],
        ["MAIN_BOARD-CPLD-1", ("Used for managing Fan, PSU, system LEDs, QSFP modules ")],
        ["MAIN_BOARD-CPLD-2", ("Used for managing Fan, PSU, system LEDs, QSFP modules ")],
        ["PORT_BOARD-CPLD-1", ("Used for managing Fan, PSU, system LEDs, QSFP modules ")],
        ["PORT_BOARD-CPLD-2", ("Used for managing Fan, PSU, system LEDs, QSFP modules ")],
        ["ALL_BOARD-CPLDS",   ("All board cplds ")],
        ["FPGA", ("Used for managing fpga modules")]
    ]
    COMPONENT_NUM = len(CHASSIS_COMPONENTS)

    MAC_SHUT = 101
    TVR_SHUT = 140

    FAN_ADJ_LIST = [
        {'type': 'max6696', 'name': "Switch Air Outlet", 'index': 0, 'spot': '', 'Th': 54, 'Tl': 30,
            'Nl': 0x14, 'Nh': 0x64, 'k': 3.3, 'crit': 67, 'max': 62, 'min': 0, 'Tshut': None},  # U50
        {'type': 'max6696', 'name': "Switch Air Inlet", 'index': 1, 'spot': '', 'Th': 60, 'Tl': 50,
            'Nl': 0x14, 'Nh': 0x64, 'k': 8.0, 'crit': 70, 'max': 65, 'min': 0, 'Tshut': None},  # Q24
        {'type': 'max6696', 'name': "Switch PCB Near NS FPGA", 'index': 2, 'spot': '', 'Th': 60, 'Tl': 50,
            'Nl': 0x14, 'Nh': 0x64, 'k': 8.0, 'crit': 70, 'max': 65, 'min': 0, 'Tshut': None},  # Q25
        {'type': 'max6696', 'name': "Switch PCB Near MAC", 'index': 3, 'spot': '', 'Th': 60, 'Tl': 49,
         'Nl': 0x14, 'Nh': 0x64, 'k': 7.3, 'crit': 80, 'max': 70, 'min': 0, 'Tshut': None},  # U1
        {'type': 'max6696', 'name': "Switch PWR2 Inlet", 'index': 4, 'spot': '', 'Th': 60, 'Tl': 35,
         'Nl': 0x14, 'Nh': 0x64, 'k': 3.2, 'crit': 71, 'max': 65, 'min': 0, 'Tshut': None},  # Q1
        {'type': 'max6696', 'name': "Switch PCB Near Port49", 'index': 5, 'spot': '', 'Th': 60, 'Tl': 40,
         'Nl': 0x14, 'Nh': 0x64, 'k': 4.0, 'crit': 72, 'max': 65, 'min': 0, 'Tshut': None},  # Q2
        {'type': 'max6696', 'name': "Switch PCB U68-0", 'index': 6, 'spot': '', 'Th': 60, 'Tl': 49,
         'Nl': 0x14, 'Nh': 0x64, 'k': 7.3, 'crit': 80, 'max': 70, 'min': 0, 'Tshut': None},  # U68
        {'type': 'max6696', 'name': "Switch MAC U68-1", 'index': 7, 'spot': '', 'Th': 81, 'Tl': 62,
         'Nl': 0x14, 'Nh': 0x64, 'k': 4.2, 'crit': 95, 'max': 85, 'min': 0, 'Tshut': None},  # u68 mac
        {'type': 'max6696', 'name': "Switch MAC U68-2", 'index': 8, 'spot': '', 'Th': 81, 'Tl': 62,
         'Nl': 0x14, 'Nh': 0x64, 'k': 4.2, 'crit': 95, 'max': 85, 'min': 0, 'Tshut': None},  # u68 mac
        {'type': 'ssd', 'name': 'SSD', 'index': 0, 'spot': 0, 'Th': 67, 'Tl': 52, 'Nl': 0x14,
         'Nh': 0x64, 'k': 5.3, 'crit': 75, 'max': 70, 'min': 0, 'Tshut': None},  # SSD
        {'type': 'coretemp', 'name': 'CPU Core', 'index': 0, 'spot': 0, 'Th': 90, 'Tl': 65,
         'Nl': 0x14, 'Nh': 0x64, 'k': 3.2, 'crit': 105, 'max': 100, 'min': 0, 'Tshut': None},  # CPU
        {'type': 'mac', 'name': 'Switch MAC', 'index': 0, 'spot': 0, 'Th': 81, 'Tl': 62, 'Nl': 0x14,
         'Nh': 0x64, 'k': 4.2, 'crit': 95, 'max': 85, 'min': 0, 'Tshut': MAC_SHUT},  # MAC inner
        {'type': 'sfp', 'name': 'Sfp', 'index': 0, 'spot': 0, 'Th': 70, 'Tl': 52, 'Nl': 0x14,
         'Nh': 0x64, 'k': 4.4, 'crit': 85, 'max': 80, 'min': 0, 'Tshut': None},  # SFP
        {'type': 'tvr', 'name': 'Tvr', 'index': 0, 'spot': 0, 'Th': 110, 'Tl': 70, 'Nl': 0x14,
         'Nh': 0x64, 'k': 2.0, 'crit': 130, 'max': 125, 'min': 0, 'Tshut': None}  # Tvr
    ]
    '''second_power_item_s ft_second_power1_item[SECOND_POWER_BUTT]=
    {
        {3300000, 3400000, 3150000, "VP1",SECOND_POWER_VP1},
        {1800000, 1890000, 1710000, "VP2",SECOND_POWER_VP2},
        {2500000, 2700000, 1300000, "VP3",SECOND_POWER_VP3},
        {1800000, 1890000, 1710000, "VP4",SECOND_POWER_VP4},
        {12000000, 13200000, 10800000, "VH",SECOND_POWER_VH},
        {1200000, 1260000, 1140000, "VX1",SECOND_POWER_VX1},
        {1150000, 1210000, 1090000, "VX2",SECOND_POWER_VX2},
        {1000000, 1050000, 950000, "VX3",SECOND_POWER_VX3},
    };
    second_power_item_s ft_second_power2_item[SECOND_POWER_BUTT]={
        {3300000, 3400000, 3150000, "VP1",SECOND_POWER_VP1},
        {3300000, 3465000, 3200000, "VP2",SECOND_POWER_VP2},
        {1200000, 1250000, 1160000, "VP3",SECOND_POWER_VP3},
        {1200000, 1250000, 1160000, "VP4",SECOND_POWER_VP4},
        {12000000, 13200000, 10800000, "VH",SECOND_POWER_VH},
        {1000000, 1050000, 950000, "VX1",SECOND_POWER_VX1},
        {750000, 800000, 720000, "VX2",SECOND_POWER_VX2},
        {800000, 950000, 700000, "VX3",SECOND_POWER_VX3},
    }; '''
    SECOND_VOL_LIST = [
        {'type': 'VP1', 'UpLimit': 3400, 'LowLimit': 3150, 'his_flag': 1},
        {'type': 'VP2', 'UpLimit': 1890, 'LowLimit': 1710, 'his_flag': 1},
        {'type': 'VP3', 'UpLimit': 2700, 'LowLimit': 1300, 'his_flag': 1},
        {'type': 'VP4', 'UpLimit': 1890, 'LowLimit': 1710, 'his_flag': 1},
        {'type': 'VH', 'UpLimit': 13200, 'LowLimit': 10800, 'his_flag': 1},
        {'type': 'VX1', 'UpLimit': 1260, 'LowLimit': 1140, 'his_flag': 1},
        {'type': 'VX2', 'UpLimit': 1210, 'LowLimit': 1090, 'his_flag': 1},
        {'type': 'VX3', 'UpLimit': 1050, 'LowLimit': 950, 'his_flag': 1},
    ]

    DEFAULT_MOTOR0_MAX_SPEED = 23000
    DEFAULT_MOTOR1_MAX_SPEED = 27000

    DEFAULT_PSUFAN_MOTOR_MAX_SPEED = 30000

    PWM_REG_MAX = 100
    PWM_REG_MIN = 20
    SPEED_TARGET_MAX = 100
    SPEED_TARGET_MIN = 20
    FAN_MOTOR_MAX_SPEED_DICT = {
        'DELTA': [27000, 23000],
        'FOXCONN': [28500, 29000],
        'DEFAULT': [28000, 26000],
    }

    CPU_THERMAL_IDX_START = 1
    CPU_THERMAL_NUM = 5

    THERMAL_INFO = [
        {'name': 'Inlet', 'alias': "Switch Air Outlet", 'LowLimit': 0, 'HighLimit': 62, 'Crit': 67},
        {'name': 'Outlet', 'alias': "Switch Air Inlet", 'LowLimit': 0, 'HighLimit': 65, 'Crit': 70},
        {'name': 'Board', 'alias': "Switch PCB U68-0", 'LowLimit': 0, 'HighLimit': 70, 'Crit': 80},
        {'name': 'CPU', 'alias': "coretemp", 'LowLimit': 0, 'HighLimit': 100, 'Crit': 105},
        {'name': 'Switch ASIC', 'alias': "Switch MAC", 'LowLimit': 0, 'HighLimit': 85, 'Crit': 95},
    ]

    THERMAL_NUM = len(THERMAL_INFO)

    DEFAULT_TEMPERATUR_FOR_SENSOR_FAULT = 80
    DEFAULT_TEMPERATUR_FOR_OPTIC_FAULT = 52

    FAN_DRAWER_NUM = 1
    FAN_NUM = 6
    PSU_NUM = 2

    STATUS_ABSENT = 0
    STATUS_OK = 1
    STATUS_NOT_OK = 2

    # presence change delay 30s, update status
    PRESENCE_CHANGE_DELAY = 30

    BMC_ENABLED = False
    ###hw-mon##

    PSU_MONITOR_ENABLE = False

    FAN_LED_DIR = SYSLED_DIR + "fan_led_status"
    PSU_LED_DIR = SYSLED_DIR + "psu_led_status"
    SYS_LED_DIR = SYSLED_DIR + "sys_led_status"

    FAN_ADJ_DIR = DEBUG_FAN_DIR + "fan_speed_pwm"

    CPU_INIT_OK_REG_DIR = CPLD_DIR + "cpld2/reg_cpu_init_ok"
    MAC_INIT_OK_REG_DIR = CPLD_DIR + "cpld2/reg_mac_init_ok"
    MAC_INNER_TEMP_REG_DIR = CPLD_DIR + "cpld2/reg_mac_inner_temp"
    MAC_WIDTH_TEMP_REG_DIR = CPLD_DIR + "cpld2/reg_mac_width_temp"
    MAC_INNER_TEMP_FORMULA_COEF = 0.23755
    CPU_INIT_OK_CODE = 1
    MAC_INIT_OK_CODE = 1

    LED_COLOR_CODE_RED = 3
    LED_COLOR_CODE_YELLOW = 2
    LED_COLOR_CODE_GREEN = 1
    LED_COLOR_CODE_DARK = 0

    COME_DIR_NUM = 1
    MONITOR_INTERVAL_SEC = 1
    MONITOR_INTERVAL_COUNT = 5
    ABNORMAL_LOG_TIME = 60
    DMESG_NEW_LOG_INTERVAL = 3600

    CURVE_PWM_MAX = 100
    TEMP_RATIO = 0.001

    CPU_CPLD_PATH = DEBUG_CPLD_DIR + "cpu_cpld"
    HW_CLR_RST = CPLD_DIR + "cpld1/reg_clr_rst"
    REBOOT_CAUSE_CPLD_PATH = CPLD_DIR + "cpld1/reg_reboot"
    LAST_REBOOT_CAUSE_CPLD_PATH = CPLD_DIR + "cpld1/reg_last_reboot"
    LAST_REBOOT_CAUSE_PATH = "/var/cache/last_reboot_cause"

    REBOOT_CAUSE_CODE_POWER_LOSS = 0
    REBOOT_CAUSE_CODE_THERMAL_OVERLOAD_CPU = 1
    REBOOT_CAUSE_CODE_THERMAL_OVERLOAD_ASIC = 2
    REBOOT_CAUSE_CODE_THERMAL_OVERLOAD_OTHER = 3
    REBOOT_CAUSE_CODE_INSUFFICIENT_FAN_SPEED = 4
    REBOOT_CAUSE_CODE_WATCHDOG = 5
    REBOOT_CAUSE_CODE_HARDWARE_OTHER = 6
    REBOOT_CAUSE_CODE_NON_HARDWARE = 7
    REBOOT_CAUSE_CODE_SW = 8
    REBOOT_CAUSE_CODE_CPLD = REBOOT_CAUSE_CODE_HARDWARE_OTHER
    REBOOT_CAUSE_NONE = 'N/A'

    SENSOR_NUM = 2
    adm1166_attr_sawp ={
        'ADM1166_0':{
            'PWR_3V3':'VP1', 'PWR_1V8':'VP2', 'VH_12V0':'VH'
        },
        'ADM1166_1':{
            'BRD_3V3':'VP1', 'DSFP_3V3':'VP2', 'MAC_1V2':'VP3',
            'MACVDD0_1V2':'VP4', 'VH_12V0':'VH',
            'FPGA_1V0': 'VX1', 'MAC_0V75':'VX2', 'MAC_AVS':'VX3'
        }
    }

    sensor_chip_dict = {
            'ADM1166_0': [
                ('PWR_3V3', 3.63, 3.63, 2.97, 2.97),
                ('PWR_1V8', 1.98, 1.98, 1.62, 1.62),
                ('VH_12V0', 13.2, 13.2, 10.8, 10.8),
            ],
            'ADM1166_1': [
                ('BRD_3V3', 3.63, 3.63, 2.97, 2.97),
                ('DSFP_3V3', 3.63, 3.63, 2.97, 2.97),
                ('MAC_1V2', 1.32, 1.32, 1.08, 1.08),
                ('MACVDD0_1V2', 1.32, 1.32, 1.08, 1.08),
                ('VH_12V0', 13.2, 13.2, 10.8, 10.8),
                ('FPGA_1V0', 1.1, 1.1, 0.9, 0.9),
                ('MAC_0V75', 0.825, 0.825, 0.675, 0.675),
                ('MAC_AVS', 0.965, 0.965, 0.63, 0.63),
            ],
    }
    """
    def __init__(self):
        '''
        Nothing to do
        '''
    def get1(self):
        return 1

    def get2(self):
        return 2
    """
