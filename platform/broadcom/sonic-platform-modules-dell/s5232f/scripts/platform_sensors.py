#!/usr/bin/python3
# On S5232F, the BaseBoard Management Controller is an
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
from sonic_py_common.general import getstatusoutput_noshell, getstatusoutput_noshell_pipe

S5232F_MAX_FAN_TRAYS = 4
S5232F_MAX_PSUS = 2
IPMI_SENSOR_DATA = ["ipmitool", "sdr", "list"]
IPMI_SENSOR_DUMP = "/tmp/sdr"

PSU_PRESENCE = "PSU{0}_stat"
# Use this for older firmware
# PSU_PRESENCE="PSU{0}_prsnt"

IPMI_FAN_PRESENCE = ["ipmitool", "sensor", "get", ""]
IPMI_PSU1_DATA_DOCKER = ["ipmitool", "raw", "0x04", "0x2d", "0x31"]
IPMI_PSU2_DATA_DOCKER = ["ipmitool", "raw", "0x04", "0x2d", "0x32"]
awk_cmd = ['awk', '{print substr($0,9,1)}']
IPMI_RAW_STORAGE_READ = ["ipmitool", "raw", "0x0a", "0x11", "", "0", "0", "0xa0"]
IPMI_FRU = ["ipmitool", "fru"]
ipmi_sdr_list = ""

# Dump sensor registers


def ipmi_sensor_dump():

    status = 1
    global ipmi_sdr_list
    ipmi_cmd = IPMI_SENSOR_DATA
    status, ipmi_sdr_list = getstatusoutput_noshell(ipmi_cmd)

    if status:
        logging.error('Failed to execute: ' + ipmi_sdr_list)
        sys.exit(0)

# Fetch a Fan Status

def get_fan_status(fan_id):
    IPMI_FAN_PRESENCE[3] = "FAN" + str(fan_id) + "_prsnt"
    ret_status, ipmi_cmd_ret = getstatusoutput_noshell(IPMI_FAN_PRESENCE)
    if ret_status:
        logging.error('Failed to execute : %s' % ' '.join(IPMI_FAN_PRESENCE))
        sys.exit(0)
    return(' ' + ipmi_cmd_ret.splitlines()[5].strip(' ').strip('[]'))

# Fetch a BMC register


def get_pmc_register(reg_name):

    output = None
    for item in ipmi_sdr_list.split("\n"):
        if reg_name in item:
            output = item.strip()

    if output is None:
        print('\nFailed to fetch: ' + reg_name + ' sensor ')
        sys.exit(0)

    output = output.split('|')[1]

    logging.basicConfig(level=logging.DEBUG)
    return output

# Fetch FRU Data for given fruid
def get_psu_airflow(psu_id):
    fru_id = 'PSU' + str(psu_id) + '_fru'
    ret_status, ipmi_cmd_ret = getstatusoutput_noshell(IPMI_FRU)
    if ret_status:
        logging.error('Failed to execute ipmitool: ' + ' '.join(IPMI_FRU))
        sys.exit(0)
    found_fru = False
    for line in ipmi_cmd_ret.splitlines():
        if line.startswith('FRU Device Description') and fru_id in line.split(':')[1]:
            found_fru = True
        if found_fru and line.startswith(' Board Product '):
            return 'Intake' if 'PS/IO' in line else 'Exhaust'
    return ''

# Fetch FRU on given offset
def fetch_raw_fru(dev_id, offset):
    IPMI_RAW_STORAGE_READ[4] = str(dev_id)
    ret_status, ipmi_cmd_ret = getstatusoutput_noshell(IPMI_RAW_STORAGE_READ)
    if ret_status:
        logging.error('Failed to execute ipmitool :' + ' '.join(IPMI_RAW_STORAGE_READ))
        sys.exit(0)
    return int((ipmi_cmd_ret.splitlines()[offset//16]).split(' ')[(offset%16+1)])


def get_fan_airflow(fan_id):
    Airflow_Direction = ['Exhaust', 'Intake']
    return Airflow_Direction[fetch_raw_fru(fan_id+2, 0x46)]

# Print the information for temperature sensors
def print_temperature_sensors():

    print("\nOnboard Temperature Sensors:")

    print('  PT_Left_temp:                   ',
          get_pmc_register('PT_Left_temp'))
    print('  PT_Mid_temp:                    ',
          get_pmc_register('PT_Mid_temp'))
    print('  PT_Right_temp:                  ',
          get_pmc_register('PT_Right_temp'))
    print('  Broadcom Temp:                  ',
          get_pmc_register('NPU_Near_temp'))
    print('  Inlet Airflow Temp:             ',
          get_pmc_register('ILET_AF_temp'))
    print('  CPU Temp:                       ',
          get_pmc_register('CPU_temp'))

file = '/sys/module/ipmi_si/parameters/kipmid_max_busy_us'
with open(file, 'w') as f:
    f.write('0\n')
ipmi_sensor_dump()

print_temperature_sensors()

# Print the information for 1 Fan Tray


def print_fan_tray(tray):

    Fan_Status = [' Normal', ' Abnormal']

    print('  Fan Tray ' + str(tray) + ':')

    if (tray == 1):

        fan1_status = int(get_pmc_register('FAN1_Front_stat'), 16)
        fan2_status = int(get_pmc_register('FAN1_Rear_stat'), 16)

        print('    Fan1 Speed:                   ',
              get_pmc_register('FAN1_Front_rpm'))
        print('    Fan2 Speed:                   ',
              get_pmc_register('FAN1_Rear_rpm'))
        print('    Fan1 State:                   ',
              Fan_Status[fan1_status])
        print('    Fan2 State:                   ',
              Fan_Status[fan2_status])

    elif (tray == 2):

        fan1_status = int(get_pmc_register('FAN2_Front_stat'), 16)
        fan2_status = int(get_pmc_register('FAN2_Rear_stat'), 16)

        print('    Fan1 Speed:                   ',
              get_pmc_register('FAN2_Front_rpm'))
        print('    Fan2 Speed:                   ',
              get_pmc_register('FAN2_Rear_rpm'))
        print('    Fan1 State:                   ',
              Fan_Status[fan1_status])
        print('    Fan2 State:                   ',
              Fan_Status[fan2_status])

    elif (tray == 3):

        fan1_status = int(get_pmc_register('FAN3_Front_stat'), 16)
        fan2_status = int(get_pmc_register('FAN3_Rear_stat'), 16)

        print('    Fan1 Speed:                   ',
              get_pmc_register('FAN3_Front_rpm'))
        print('    Fan2 Speed:                   ',
              get_pmc_register('FAN3_Rear_rpm'))
        print('    Fan1 State:                   ',
              Fan_Status[fan1_status])
        print('    Fan2 State:                   ',
              Fan_Status[fan2_status])

    elif (tray == 4):

        fan1_status = int(get_pmc_register('FAN4_Front_stat'), 16)
        fan2_status = int(get_pmc_register('FAN4_Rear_stat'), 16)

        print('    Fan1 Speed:                   ',
              get_pmc_register('FAN4_Front_rpm'))
        print('    Fan2 Speed:                   ',
              get_pmc_register('FAN4_Rear_rpm'))
        print('    Fan1 State:                   ',
              Fan_Status[fan1_status])
        print('    Fan2 State:                   ',
              Fan_Status[fan2_status])
    print('    Airflow:                      ',
          get_fan_airflow(tray))


print('\nFan Trays:')

for tray in range(1, S5232F_MAX_FAN_TRAYS + 1):
    if (get_fan_status(tray) == ' Present'):
        print_fan_tray(tray)
    else:
        print('    Fan Tray {}:                      NOT PRESENT'.format(str(tray)))

    def get_psu_presence(index):
        """
        Retrieves the presence status of power supply unit (PSU) defined
                by index <index>
        :param index: An integer, index of the PSU of which to query status
        :return: Boolean, True if PSU is plugged, False if not
        """
        ret_status = 1

        if index == 1:
            ret_status, ipmi_cmd_ret = getstatusoutput_noshell_pipe(IPMI_PSU1_DATA_DOCKER, awk_cmd)
        elif index == 2:
            ret_status, ipmi_cmd_ret = getstatusoutput_noshell_pipe(IPMI_PSU2_DATA_DOCKER, awk_cmd)

        if ret_status:
            logging.error('Failed to execute ipmitool :' + ' '.join(IPMI_PSU1_DATA_DOCKER))
            sys.exit(0)

        psu_status = ipmi_cmd_ret
        return (int(psu_status, 16) & 1)

    def get_psu_status(index):
        """
        Retrieves the presence status of power supply unit (PSU) defined
                by index <index>
        :param index: An integer, index of the PSU of which to query status
        :return: Boolean, True if PSU is plugged, False if not
        """
        status = 0
        ret_status = 1
        ipmi_cmd_ret = 'f'

        if index == 1:
            ret_status, ipmi_cmd_ret = getstatusoutput_noshell_pipe(IPMI_PSU1_DATA_DOCKER, awk_cmd)
        elif index == 2:
            ret_status, ipmi_cmd_ret = getstatusoutput_noshell_pipe(IPMI_PSU2_DATA_DOCKER, awk_cmd)

        if ret_status:
            logging.error('Failed to execute ipmitool : ' + ' '.join(IPMI_PSU2_DATA_DOCKER))
            sys.exit(0)

        psu_status = ipmi_cmd_ret

        return (not int(psu_status, 16) > 1)


# Print the information for PSU1, PSU2
def print_psu(psu):
    Psu_Type = ['Normal', 'Mismatch']
    Psu_Input_Type = ['AC', 'DC']
    PSU_STATUS_TYPE_BIT = 4
    PSU_STATUS_INPUT_TYPE_BIT = 1
    PSU_FAN_PRESENT_BIT = 2
    PSU_FAN_STATUS_BIT = 1
    PSU_FAN_AIR_FLOW_BIT = 0
    Psu_Fan_Presence = ['Present', 'Absent']
    Psu_Fan_Status = ['Normal', 'Abnormal']

    # print '    Input:          ', Psu_Input_Type[psu_input_type]
    # print '    Type:           ', Psu_Type[psu_type]

    # PSU FAN details
    if (psu == 1):

        # psu1_fan_status = int(get_pmc_register('PSU1_status'),16)

        print('    PSU1:')
        print('       FAN Normal Temperature:       ',
              get_pmc_register('PSU1_temp'))
        print('       FAN AirFlow Temperature:      ',
              get_pmc_register('PSU1_AF_temp'))
        print('       FAN RPM:                      ',
              get_pmc_register('PSU1_rpm'))
        # print '    FAN Status:      ', Psu_Fan_Status[psu1_fan_status]

        # PSU input & output monitors
        print('       Input Voltage:                ',
              get_pmc_register('PSU1_In_volt'))
        print('       Output Voltage:               ',
              get_pmc_register('PSU1_Out_volt'))
        print('       Input Power:                  ',
              get_pmc_register('PSU1_In_watt'))
        print('       Output Power:                 ',
              get_pmc_register('PSU1_Out_watt'))
        print('       Input Current:                ',
              get_pmc_register('PSU1_In_amp'))
        print('       Output Current:               ',
              get_pmc_register('PSU1_Out_amp'))

    else:

        # psu2_fan_status = int(get_pmc_register('PSU1_status'),16)
        print('    PSU2:')
        print('       FAN Normal Temperature:       ',
              get_pmc_register('PSU2_temp'))
        print('       FAN AirFlow Temperature:      ',
              get_pmc_register('PSU2_AF_temp'))
        print('       FAN RPM:                      ',
              get_pmc_register('PSU2_rpm'))
        # print '    FAN Status:      ', Psu_Fan_Status[psu2_fan_status]

        # PSU input & output monitors
        print('       Input Voltage:                ',
              get_pmc_register('PSU2_In_volt'))
        print('       Output Voltage:               ',
              get_pmc_register('PSU2_Out_volt'))
        print('       Input Power:                  ',
              get_pmc_register('PSU2_In_watt'))
        print('       Output Power:                 ',
              get_pmc_register('PSU2_Out_watt'))
        print('       Input Current:                ',
              get_pmc_register('PSU2_In_amp'))
        print('       Output Current:               ',
              get_pmc_register('PSU2_Out_amp'))
    print('       Airflow:                      ',
          get_psu_airflow(psu))


print('\nPSUs:')
for psu in range(1, S5232F_MAX_PSUS + 1):
    #psu_presence = PSU_PRESENCE.format(psu)
    if not get_psu_presence(psu):
        print('    PSU{}:                      NOT PRESENT'.format(psu))
    elif not get_psu_status(psu):
        print('    PSU{}:                      NOT OK'.format(psu))
    else:
        print_psu(psu)

print('\n    Total Power:                     ',
      get_pmc_register('PSU_Total_watt'))

file = '/sys/module/ipmi_si/parameters/kipmid_max_busy_us'
with open(file, 'w') as f:
    f.write('1000\n')
