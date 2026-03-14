#!/usr/bin/python
# On S5212F, the BaseBoard Management Controller is an
# autonomous subsystem provides monitoring and management
# facility independent of the host CPU. IPMI standard
# protocol is used with ipmitool to fetch sensor details.
# Current script support X00 board only. X01 support will 
# be added soon. This provies support for the
# following objects:
#   * Onboard temperature sensors
#   * FAN trays
#   * PSU


import sys
import logging
from sonic_py_common.general import getstatusoutput_noshell

S5212F_MAX_FAN_TRAYS = 4
IPMI_SENSOR_DATA = ["ipmitool", "sdr", "list"]

switch_sku = {
  "0K6MG9":(' AC', ' Exhaust'),
  "0GKK8W":(' AC', ' Intake'),
  "0VK93C":(' AC', ' Exhaust'),
  "05JHDM":(' AC', ' Intake'),
  "0D72R7":(' AC', ' Exhaust'),
  "02PC9F":(' AC', ' Exhaust'),
  "0JM5DX":(' AC', ' Intake'),
  "0TPDP8":(' AC', ' Exhaust'),
  "0WND1V":(' AC', ' Exhaust'),
  "05672M":(' DC', ' Intake'),
  "0CJV4K":(' DC', ' Intake'),
  "0X41RN":(' AC', ' Exhaust'),
  "0Y3N82":(' AC', ' Intake'),
  "0W4CMG":(' DC', ' Exhaust'),
  "04T94Y":(' DC', ' Intake')
}

ipmi_status, ipmi_sdr_list = getstatusoutput_noshell(IPMI_SENSOR_DATA)

def get_pmc_register(reg_name):
    if ipmi_status:
        logging.error('Failed to execute:' + ipmi_sdr_list)
        sys.exit(0)
    for line in ipmi_sdr_list.splitlines():
        sdr = line.split('|')
        if reg_name in sdr[0] : return sdr[1]
    print('\nFailed to fetch: ' +  reg_name + ' sensor ')
    sys.exit(0)


# Print the information for temperature sensors


def print_temperature_sensors():

    print("\nOnboard Temperature Sensors:")

    print('  PT_Left_temp:                   ',\
        (get_pmc_register('PT_Left_temp')))
    print('  PT_Mid_temp:                    ',\
        (get_pmc_register('PT_Mid_temp')))
    print('  PT_Right_temp:                  ',\
        (get_pmc_register('PT_Right_temp')))
    print('  Broadcom Temp:                  ',\
        (get_pmc_register('NPU_Near_temp')))
    print('  Inlet Airflow Temp:             ',\
        (get_pmc_register('ILET_AF_temp')))
    print('  CPU Temp:                       ',\
        (get_pmc_register('CPU_temp')))

def get_switch_details():
    status, ipmi_fru = getstatusoutput_noshell(['/usr/bin/ipmitool', 'fru'])
    for line in ipmi_fru.splitlines():
        info = line.split(':')
        if 'Board Part Number' in info[0] : 
            partno =  info[1][1:-3]
            if (partno  in switch_sku): return switch_sku[partno]
    return None

file = '/sys/module/ipmi_si/parameters/kipmid_max_busy_us'
with open(file, 'w') as f:
    f.write('0\n')
print_temperature_sensors()

# Print the information for 1 Fan Tray


def print_fan_tray(tray):

    print('  Fan Tray ' + str(tray) + ':')

    if (tray == 1):
        print('    Fan1 Speed:                   ',\
            get_pmc_register('FAN1_Front_rpm'))
        print('    Fan2 Speed:                   ',\
            get_pmc_register('FAN1_Rear_rpm'))

    elif (tray == 2):
        print('    Fan1 Speed:                   ',\
            get_pmc_register('FAN2_Front_rpm'))
        print('    Fan2 Speed:                   ',\
            get_pmc_register('FAN2_Rear_rpm'))

    elif (tray == 3):
        print('    Fan1 Speed:                   ',\
            get_pmc_register('FAN3_Front_rpm'))
        print('    Fan2 Speed:                   ',\
            get_pmc_register('FAN3_Rear_rpm'))

    elif (tray == 4):
        print('    Fan1 Speed:                   ',\
            get_pmc_register('FAN4_Front_rpm'))
        print('    Fan2 Speed:                   ',\
            get_pmc_register('FAN4_Rear_rpm'))

type, dir = get_switch_details()
print('\nFan Trays(Fixed):')
print('  Fan Tray Direction:             ', dir)
for tray in range(1, S5212F_MAX_FAN_TRAYS + 1):
    print_fan_tray(tray)

print('\nPSU Tray(Fixed):')
print('  PSU Tray Direction:             ', dir)
print('  PSU Tray Type:                  ', type)

file = '/sys/module/ipmi_si/parameters/kipmid_max_busy_us'
with open(file, 'w') as f:
    f.write('1000\n')
