#!/usr/bin/python3
# On Z9332F, the BaseBoard Management Controller is an
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

Z9332F_MAX_FAN_TRAYS = 7
Z9332F_MAX_PSUS = 2
IPMI_SENSOR_DATA = ["ipmitool", "sdr", "list"]
IPMI_SENSOR_DUMP = "/tmp/sdr"

FAN_PRESENCE = "Fan{0}_Status"
PSU_PRESENCE = "PSU{0}_Status"
# Use this for older firmware
# PSU_PRESENCE="PSU{0}_prsnt"

IPMI_PSU1_DATA_DOCKER = ["ipmitool", "raw", "0x04", "0x2d", "0x2f"]
IPMI_PSU2_DATA_DOCKER = ["ipmitool", "raw", "0x04", "0x2d", "0x39"]
awk_cmd = ['awk', '{print substr($0,9,1)}']
ipmi_sdr_list = ""

# Dump sensor registers


def ipmi_sensor_dump():

    status = 1
    global ipmi_sdr_list
    ipmi_cmd = IPMI_SENSOR_DATA
    status, ipmi_sdr_list = getstatusoutput_noshell(ipmi_cmd)

    if status:
        logging.error('Failed to execute:' + ipmi_sdr_list)
        sys.exit(0)

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


# Print the information for temperature sensors


def print_temperature_sensors():

    print("\nOnboard Temperature Sensors:")

    for x in (('TEMP_FAN_U52',     'Fan U52'),
              ('TEMP_FAN_U17',     'Fan U17'),
              ('TEMP_SW_U52',      'SW U52'),
              ('TEMP_SW_U16',      'SW U16'),
              ('TEMP_BB_U3',       'Baseboard U3'),
              ('TEMP_CPU',         'Near CPU'),
              ('TEMP_SW_Internal', 'SW interal'),
              ('PSU1_Temp1',       'PSU1 inlet'),
              ('PSU1_Temp2',       'PSU1 hotspot'),
              ('PSU2_Temp1',       'PSU2 inlet'),
              ('PSU2_Temp2',       'PSU2 hotspot'),
              ('SW_U04_Temp',      'SW U04'),
              ('SW_U14_Temp',      'SW U14'),
              ('SW_U4403_Temp',    'SW U4403')
              ):
        print('  {0:32}{1}'.format(x[1] + ':', get_pmc_register(x[0])))

ipmi_sensor_dump()

print_temperature_sensors()

# Print the information for 1 Fan Tray


def print_fan_tray(tray):

    Fan_Status = [' Normal', ' Abnormal']
    Airflow_Direction = ['B2F', 'F2B']

    print('  Fan Tray ' + str(tray) + ':')

    print('    Fan1 Speed:                   ',
          get_pmc_register('Fan{}_Front'.format(tray)))
    print('    Fan2 Speed:                   ',
          get_pmc_register('Fan{}_Rear'.format(tray)))
    print('    Fan State:                    ',
          Fan_Status[int(get_pmc_register('Fan{}_Status'.format(tray)), 16)])


print('\nFan Trays:')

for tray in range(1, Z9332F_MAX_FAN_TRAYS + 1):
    fan_presence = FAN_PRESENCE.format(tray)
    if (get_pmc_register(fan_presence)):
        print_fan_tray(tray)
    else:
        print('\n  Fan Tray ' + str(tray + 1) + ':     Not present')

    def get_psu_presence(index):
        """
        Retrieves the presence status of power supply unit (PSU) defined
                by index <index>
        :param index: An integer, index of the PSU of which to query status
        :return: Boolean, True if PSU is plugged, False if not
        """
        status = 0
        ret_status = 1

        if index == 1:
            status, ipmi_cmd_ret = getstatusoutput_noshell_pipe(IPMI_PSU1_DATA_DOCKER, awk_cmd)
        elif index == 2:
            ret_status, ipmi_cmd_ret = getstatusoutput_noshell_pipe(IPMI_PSU2_DATA_DOCKER, awk_cmd)

        #if ret_status:
         #   print ipmi_cmd_ret
         #   logging.error('Failed to execute ipmitool')
         #   sys.exit(0)

        psu_status = ipmi_cmd_ret

        if psu_status == '1':
            status = 1

        return status


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
    Psu_Fan_Airflow = ['B2F', 'F2B']

    # print '    Input:          ', Psu_Input_Type[psu_input_type]
    # print '    Type:           ', Psu_Type[psu_type]

    print('    PSU{}:'.format(psu))
    print('       Inlet Temperature:            ',
          get_pmc_register('PSU{}_Temp1'.format(psu)))
    print('       Hotspot Temperature:          ',
          get_pmc_register('PSU{}_Temp2'.format(psu)))
    print('       FAN RPM:                      ',
          get_pmc_register('PSU{}_Fan'.format(psu)))
    # print '    FAN Status:      ', Psu_Fan_Status[psu1_fan_status]

    # PSU input & output monitors
    print('       Input Voltage:                ',
          get_pmc_register('PSU{}_VIn'.format(psu)))
    print('       Output Voltage:               ',
          get_pmc_register('PSU{}_VOut'.format(psu)))
    print('       Input Power:                  ',
          get_pmc_register('PSU{}_PIn'.format(psu)))
    print('       Output Power:                 ',
          get_pmc_register('PSU{}_POut'.format(psu)))
    print('       Input Current:                ',
          get_pmc_register('PSU{}_CIn'.format(psu)))
    print('       Output Current:               ',
          get_pmc_register('PSU{}_COut'.format(psu)))


print('\nPSUs:')
for psu in range(1, Z9332F_MAX_PSUS + 1):
    #psu_presence = PSU_PRESENCE.format(psu)
    if (get_psu_presence(psu)):
        print_psu(psu)
    else:
        print('\n  PSU ', psu, 'Not present')
