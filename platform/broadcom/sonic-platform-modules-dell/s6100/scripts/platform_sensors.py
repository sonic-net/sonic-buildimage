#!/usr/bin/python3

# On S6100, the Platform Management Controller runs the
# thermal algorithm. It provides a mailbox for the Host
# to query relevant thermals. The dell_mailbox module
# provides the sysfs support for the following objects:
#   * onboard temperature sensors
#   * FAN trays
#   * PSU
#
import os
import logging

S6100_MAX_FAN_TRAYS = 4
S6100_MAX_PSUS = 2
S6100_MAX_IOMS = 4

HWMON_DIR = "/sys/devices/platform/SMF.512/hwmon/"
HWMON_NODE = os.listdir(HWMON_DIR)[0]
MAILBOX_DIR = HWMON_DIR + HWMON_NODE
iom_status_list = []

# Get a mailbox register


def get_pmc_register(reg_name):
    retval = 'ERR'
    mb_reg_file = MAILBOX_DIR+'/'+reg_name

    if (not os.path.isfile(mb_reg_file)):
        print(mb_reg_file,  'not found !')
        return retval

    try:
        with open(mb_reg_file, 'r') as fd:
            retval = fd.read()
    except Exception as error:
        logging.error("Unable to open ", mb_reg_file, "file !")

    retval = retval.rstrip('\r\n')
    retval = retval.lstrip(" ")
    return retval

logging.basicConfig(level=logging.DEBUG)

if (os.path.isdir(MAILBOX_DIR)):
    print('dell-s6100-lpc')
    print('Adapter: S6100 Platform Management Controller')
else:
    logging.error('S6100 Platform Management Controller module not loaded !')
    # sys.exit(0)

# Print the information for temperature sensors


def print_temperature_sensors():
    print("\nOnboard Temperature Sensors:")
    print('  CPU:                            ',
          int(get_pmc_register('temp1_input'))/1000, 'C')
    print('  BCM56960 (PSU side):            ',
          int(get_pmc_register('temp2_input'))/1000, 'C')
    print('  System Outlet 1 (switch board): ',
          int(get_pmc_register('temp3_input'))/1000, 'C')
    print('  BCM56960 (IO side):             ',
          int(get_pmc_register('temp4_input'))/1000, 'C')
    print('  System Outlet 2 (CPU board):    ',
          int(get_pmc_register('temp9_input'))/1000, 'C')
    print('  System Inlet Left (IO side):    ',
          int(get_pmc_register('temp10_input'))/1000, 'C')
    print('  System Inlet Right (IO side):   ',
          int(get_pmc_register('temp11_input'))/1000, 'C')

    iom_status = get_pmc_register('iom_presence')
    iom_status = int(iom_status, 16)

    iom_presence = get_pmc_register('iom_status')

    if (iom_presence != 'ERR'):
        iom_presence = int(iom_presence, 16)

        # IOM presence : 0 => IOM present
        # Temperature sensors 5..8 correspond to IOM 1..4
        for iom in range(0, S6100_MAX_IOMS):
            if (~iom_presence & (1 << iom)):
                iom_sensor_indx = iom + 5
                print('  IOM ' + str(iom + 1) + ':\t\t\t  ',
                      int(get_pmc_register('temp' + str(iom_sensor_indx) +
                          '_input'))/1000, 'C')

                # Save the IOM Status for later use
                if (~iom_status & (1 << iom)):
                    iom_status_list.append('ON')
                else:
                    iom_status_list.append('OFF')
            else:
                iom_status_list.append('Not present')
                print('  IOM ' + str(iom + 1) + ':\t\t\t  ', 'Not present')
    else:
        logging.error('Unable to check IOM presence')

print_temperature_sensors()


# Print the information for voltage sensors


def print_voltage_sensors():
    print("\nOnboard Voltage Sensors:")

    print('  CPU XP3R3V_EARLY                ',
          float(get_pmc_register('in1_input'))/1000, 'V')
    print('  CPU XP5R0V_CP                   ',
          float(get_pmc_register('in2_input'))/1000, 'V')
    print('  CPU XP3R3V_STD                  ',
          float(get_pmc_register('in3_input'))/1000, 'V')
    print('  CPU XP3R3V_CP                   ',
          float(get_pmc_register('in4_input'))/1000, 'V')
    print('  CPU XP0R75V_VTT_A               ',
          float(get_pmc_register('in5_input'))/1000, 'V')
    print('  CPU XPPR75V_VTT_B               ',
          float(get_pmc_register('in6_input'))/1000, 'V')
    print('  CPU XP1R07V_CPU                 ',
          float(get_pmc_register('in7_input'))/1000, 'V')
    print('  CPU XP1R0V_CPU                  ',
          float(get_pmc_register('in8_input'))/1000, 'V')
    print('  CPU XP12R0V                     ',
          float(get_pmc_register('in9_input'))/1000, 'V')
    print('  CPU VDDR_CPU_2                  ',
          float(get_pmc_register('in10_input'))/1000, 'V')
    print('  CPU VDDR_CPU_1                  ',
          float(get_pmc_register('in11_input'))/1000, 'V')
    print('  CPU XP1R5V_CLK                  ',
          float(get_pmc_register('in12_input'))/1000, 'V')
    print('  CPU XP1R8V_CPU                  ',
          float(get_pmc_register('in13_input'))/1000, 'V')
    print('  CPU XP1R0V_CPU_VNN              ',
          float(get_pmc_register('in14_input'))/1000, 'V')
    print('  CPU XP1R0V_CPU_VCC              ',
          float(get_pmc_register('in15_input'))/1000, 'V')
    print('  CPU XP1R5V_EARLY                ',
          float(get_pmc_register('in16_input'))/1000, 'V')
    print('  SW XP3R3V_MON                   ',
          float(get_pmc_register('in17_input'))/1000, 'V')
    print('  SW XP1R25V_MON                  ',
          float(get_pmc_register('in19_input'))/1000, 'V')
    print('  SW XP1R2V_MON                   ',
          float(get_pmc_register('in20_input'))/1000, 'V')
    print('  SW XP1R0V_SW_MON                ',
          float(get_pmc_register('in21_input'))/1000, 'V')
    print('  SW XP1R0V_ROV_SW_MON            ',
          float(get_pmc_register('in22_input'))/1000, 'V')
    print('  SW XR1R0V_BCM84752_MON          ',
          float(get_pmc_register('in23_input'))/1000, 'V')
    print('  SW XP5V_MB_MON                  ',
          float(get_pmc_register('in24_input'))/1000, 'V')
    print('  SW XP3R3V_FPGA_MON              ',
          float(get_pmc_register('in26_input'))/1000, 'V')
    print('  SW XP3R3V_EARLY_MON             ',
          float(get_pmc_register('in27_input'))/1000, 'V')

print_voltage_sensors()

# Print the information for 1 Fan Tray


def print_fan_tray(tray):

    Fan_Status = ['Normal', 'Abnormal']
    Airflow_Direction = ['B2F', 'F2B']

    print('  Fan Tray ' + str(tray) + ':')

    if (tray == 1):
        fan1_speed = int(get_pmc_register('fan1_input'))
        air_flow_reg = int(get_pmc_register('fan1_airflow'), 16)
        fan1_status = 0 if fan1_speed >= 1000 else 1

        print('    Fan Speed:    ', fan1_speed, 'RPM')
        print('    Fan State:       ', Fan_Status[fan1_status])
        print('    Air Flow:           ', Airflow_Direction[air_flow_reg])
    elif (tray == 2):
        fan1_speed = int(get_pmc_register('fan3_input'))
        air_flow_reg = int(get_pmc_register('fan3_airflow'), 16)
        fan1_status = 0 if fan1_speed >= 1000 else 1

        print('    Fan Speed:    ', fan1_speed, 'RPM')
        print('    Fan State:       ', Fan_Status[fan1_status])
        print('    Air Flow:           ', Airflow_Direction[air_flow_reg])
    elif (tray == 3):
        fan1_speed = int(get_pmc_register('fan5_input'))
        air_flow_reg = int(get_pmc_register('fan5_airflow'), 16)
        fan1_status = 0 if fan1_speed >= 1000 else 1

        print('    Fan Speed:    ', fan1_speed, 'RPM')
        print('    Fan State:       ', Fan_Status[fan1_status])
        print('    Air Flow:           ', Airflow_Direction[air_flow_reg])
    elif (tray == 4):
        fan1_speed = int(get_pmc_register('fan7_input'))
        air_flow_reg = int(get_pmc_register('fan7_airflow'), 16)
        fan1_status = 0 if fan1_speed >= 1000 else 1

        print('    Fan Speed:    ', fan1_speed, 'RPM')
        print('    Fan State:       ', Fan_Status[fan1_status])
        print('    Air Flow:           ', Airflow_Direction[air_flow_reg])

print('\nFan Trays:')
fan_tray_presence = get_pmc_register('fan_tray_presence')

if (fan_tray_presence != 'ERR'):
    fan_tray_presence = int(fan_tray_presence, 16)

    for tray in range(0, S6100_MAX_FAN_TRAYS):
        if (fan_tray_presence & (1 << tray)):
            print_fan_tray(tray + 1)
        else:
            print('\n  Fan Tray ' + str(tray + 1) + ':  Not present')
else:
    logging.error('Unable to read FAN presence')

# Print the information for PSU1, PSU2


def print_psu(psu):
    Psu_Type = ['Mismatch', 'Normal']
    Psu_Input_Type = ['AC', 'DC']
    PSU_STATUS_TYPE_BIT = 4
    PSU_STATUS_INPUT_TYPE_BIT = 1
    PSU_FAN_PRESENT_BIT = 2
    PSU_FAN_STATUS_BIT = 1
    PSU_FAN_AIR_FLOW_BIT = 0
    Psu_Fan_Presence = ['Present', 'Absent']
    Psu_Fan_Status = ['Normal', 'Abnormal']
    Psu_Fan_Airflow = ['B2F', 'F2B']

    print('  PSU ' + str(psu) + ':')
    if (psu == 1):
        psu_status = int(get_pmc_register('psu1_presence'), 16)
    else:
        psu_status = int(get_pmc_register('psu2_presence'), 16)

    psu_input_type = (psu_status & (1 << PSU_STATUS_INPUT_TYPE_BIT)) >>\
        PSU_STATUS_INPUT_TYPE_BIT
    psu_type = (psu_status & (1 << PSU_STATUS_TYPE_BIT)) >>\
        PSU_STATUS_TYPE_BIT

    print('    Input:          ', Psu_Input_Type[psu_input_type])
    print('    Type:           ', Psu_Type[psu_type])

    # PSU FAN details
    if (psu == 1):
        print('    FAN Speed:      ', get_pmc_register('fan11_input'), 'RPM')
        psu_fan_airflow = int(get_pmc_register('fan11_airflow'))
        psu_fan_status = int(get_pmc_register('fan11_alarm'))
        psu_fan_present = int(get_pmc_register('fan11_fault'))
        input_voltage = float(get_pmc_register('in29_input')) / 1000
        output_voltage = float(get_pmc_register('in30_input')) / 1000
        input_current = float(get_pmc_register('curr601_input')) / 1000
        output_current = float(get_pmc_register('curr602_input')) / 1000
        input_power = float(get_pmc_register('power1_input')) / 1000000
        output_power = float(get_pmc_register('power2_input')) / 1000000
        if (input_power != 0):
            psu_fan_temp = int(get_pmc_register('temp14_input'))/1000
    else:
        print('    FAN Speed:      ', get_pmc_register('fan12_input'), 'RPM')
        psu_fan_airflow = int(get_pmc_register('fan12_airflow'))
        psu_fan_status = int(get_pmc_register('fan12_alarm'))
        psu_fan_present = int(get_pmc_register('fan12_fault'))
        input_voltage = float(get_pmc_register('in31_input')) / 1000
        output_voltage = float(get_pmc_register('in32_input')) / 1000
        input_current = float(get_pmc_register('curr701_input')) / 1000
        output_current = float(get_pmc_register('curr702_input')) / 1000
        input_power = float(get_pmc_register('power3_input')) / 1000000
        output_power = float(get_pmc_register('power4_input')) / 1000000
        if (input_power != 0):
            psu_fan_temp = int(get_pmc_register('temp15_input'))/1000
    print('    FAN:            ', Psu_Fan_Presence[psu_fan_present])
    print('    FAN Status:     ', Psu_Fan_Status[psu_fan_status])
    print('    FAN AIRFLOW:    ', Psu_Fan_Airflow[psu_fan_airflow])

    # PSU input & output monitors
    print('    Input Voltage:   %6.2f' % (input_voltage), 'V')

    print('    Output Voltage:  %6.2f' % (output_voltage), 'V')

    print('    Input Current:   %6.2f' % (input_current), 'A')

    print('    Output Current:  %6.2f' % (output_current), 'A')

    print('    Input Power:     %6.2f' % (input_power), 'W')

    print('    Output Power:    %6.2f' % (output_power), 'W')

    # PSU firmware gives spurious temperature reading without input power
    if (input_power != 0):
        print('    Temperature:    ', psu_fan_temp, 'C')
    else:
        print('    Temperature:    ', 'NA')

print('\nPSUs:')
for psu in range(1, S6100_MAX_PSUS + 1):

    if (psu == 1):
        psu_status = get_pmc_register('psu1_presence')
    else:
        psu_status = get_pmc_register('psu2_presence')

    if (psu_status != 'ERR'):
        psu_status = int(psu_status, 16)
        if (~psu_status & 0b1):
            print_psu(psu)
        else:
            print('\n  PSU ', psu, 'Not present')
    else:
        logging.error('Unable to check PSU presence')

print('\n  Total Power:      ', get_pmc_register('current_total_power'), 'W')

print('\nIO Modules:')

for iom in range(1, S6100_MAX_IOMS+1):
    print('  IOM ' + str(iom) + ' :' + iom_status_list[iom - 1])

print('\n')
