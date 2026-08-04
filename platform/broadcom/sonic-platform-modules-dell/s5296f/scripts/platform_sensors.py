#!/usr/bin/python
# On S5296F, the BaseBoard Management Controller is an
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

S5296F_MAX_FAN_TRAYS = 4
S5296F_MAX_PSUS = 2
IPMI_SENSOR_DATA = ["ipmitool", "sdr", "list"]
IPMI_SENSOR_DUMP = "/tmp/sdr"

FAN_PRESENCE = "FAN{0}_prsnt"
PSU_PRESENCE = "PSU{0}_stat"
# Use this for older firmware
# PSU_PRESENCE="PSU{0}_prsnt"

IPMI_PSU1_DATA_DOCKER = ["ipmitool", "raw", "0x04", "0x2d", "0x31"]
IPMI_PSU2_DATA_DOCKER = ["ipmitool", "raw", "0x04", "0x2d", "0x32"]
awk_cmd = ['awk', '{print substr($0,9,1)}']
ipmi_sdr_list = ""

# Dump sensor registers


def ipmi_sensor_dump():

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
        print('\nFailed to fetch: ' +  reg_name + ' sensor ')
        sys.exit(0)

    output = output.split('|')[1]

    logging.basicConfig(level=logging.DEBUG)
    return output


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

ipmi_sensor_dump()

print_temperature_sensors()

# Print the information for 1 Fan Tray


def print_fan_tray(tray):

    Fan_Status = [' Normal', ' Abnormal']

    print('  Fan Tray ' + str(tray) + ':')

    if (tray == 1):

        fan2_status = int(get_pmc_register('FAN1_Rear_stat'), 16)

        print('    Fan Speed:                   ',\
            get_pmc_register('FAN1_Rear_rpm'))
        print('    Fan State:                   ',\
            Fan_Status[fan2_status])

    elif (tray == 2):

        fan2_status = int(get_pmc_register('FAN2_Rear_stat'), 16)

        print('    Fan Speed:                   ',\
            get_pmc_register('FAN2_Rear_rpm'))
        print('    Fan State:                   ',\
            Fan_Status[fan2_status])

    elif (tray == 3):

        fan2_status = int(get_pmc_register('FAN3_Rear_stat'), 16)

        print('    Fan Speed:                   ',\
            get_pmc_register('FAN3_Rear_rpm'))
        print('    Fan State:                   ',\
            Fan_Status[fan2_status])

    elif (tray == 4):

        fan2_status = int(get_pmc_register('FAN4_Rear_stat'), 16)

        print('    Fan Speed:                   ',\
            get_pmc_register('FAN4_Rear_rpm'))
        print('    Fan State:                   ',\
            Fan_Status[fan2_status])


print('\nFan Trays:')

for tray in range(1, S5296F_MAX_FAN_TRAYS + 1):
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

    # print '    Input:          ', Psu_Input_Type[psu_input_type]
    # print '    Type:           ', Psu_Type[psu_type]

    # PSU FAN details
    if (psu == 1):

        # psu1_fan_status = int(get_pmc_register('PSU1_status'),16)

        print('    PSU1:')
        print('       FAN Normal Temperature:       ',\
            get_pmc_register('PSU1_temp'))
        print('       FAN AirFlow Temperature:      ',\
            get_pmc_register('PSU1_AF_temp'))
        print('       FAN RPM:                      ',\
            get_pmc_register('PSU1_rpm'))
        # print '    FAN Status:      ', Psu_Fan_Status[psu1_fan_status]

        # PSU input & output monitors
        print('       Input Voltage:                ',\
            get_pmc_register('PSU1_In_volt'))
        print('       Output Voltage:               ',\
            get_pmc_register('PSU1_Out_volt'))
        print('       Input Power:                  ',\
            get_pmc_register('PSU1_In_watt'))
        print('       Output Power:                 ',\
            get_pmc_register('PSU1_Out_watt'))
        print('       Input Current:                ',\
            get_pmc_register('PSU1_In_amp'))
        print('       Output Current:               ',\
            get_pmc_register('PSU1_Out_amp'))

    else:

        # psu2_fan_status = int(get_pmc_register('PSU1_status'),16)
        print('    PSU2:')
        print('       FAN Normal Temperature:       ',\
            get_pmc_register('PSU2_temp'))
        print('       FAN AirFlow Temperature:      ',\
            get_pmc_register('PSU2_AF_temp'))
        print('       FAN RPM:                      ',\
            get_pmc_register('PSU2_rpm'))
        # print '    FAN Status:      ', Psu_Fan_Status[psu2_fan_status]

        # PSU input & output monitors
        print('       Input Voltage:                ',\
            get_pmc_register('PSU2_In_volt'))
        print('       Output Voltage:               ',\
            get_pmc_register('PSU2_Out_volt'))
        print('       Input Power:                  ',\
            get_pmc_register('PSU2_In_watt'))
        print('       Output Power:                 ',\
            get_pmc_register('PSU2_Out_watt'))
        print('       Input Current:                ',\
            get_pmc_register('PSU2_In_amp'))
        print('       Output Current:               ',\
            get_pmc_register('PSU2_Out_amp'))


print('\nPSUs:')
for psu in range(1, S5296F_MAX_PSUS + 1):
    #psu_presence = PSU_PRESENCE.format(psu)
    if (get_psu_presence(psu)):
        print_psu(psu)
    else:
        print('\n  PSU ', psu, 'Not present')

print('\n    Total Power:                     ',\
    get_pmc_register('PSU_Total_watt'))

