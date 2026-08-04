#!/usr/bin/python
"""
# Module on Z9432F, the BaseBoard Management Controller is an
# autonomous subsystem provides monitoring and management
# facility independent of the host CPU. IPMI standard
# protocol is used with ipmitool to fetch sensor details.
# This provies support for the following objects:
#   * Onboard temperature sensors
#   * FAN trays
#   * PSU
"""

import sys
import logging
from sonic_py_common.general import getstatusoutput_noshell

Z9432F_MAX_FAN_TRAYS = 7
Z9432F_MAX_PSUS = 2
IPMI_SENSOR_DATA = ["ipmitool", "sdr", "elist"]

IPMI_RAW_STORAGE_READ = ["ipmitool", "raw", "0x0a", "0x11", "", "", "0", "1"]

# Dump sensor registers
class SdrStatus(object):
    """ Contains IPMI SDR List """
    def __init__(self):
        ipmi_cmd = IPMI_SENSOR_DATA
        status, resp = getstatusoutput_noshell(ipmi_cmd)
        if status:
            logging.error('Failed to execute: ' + ipmi_cmd)
            sys.exit(0)
        self.ipmi_sdr_dict = {}
        for sdr_status in resp.split('\n'):
            sdr_status_l = sdr_status.split('|')
            sensor = sdr_status_l[0].strip()
            value = sdr_status_l[4].strip()
            self.ipmi_sdr_dict[sensor] = value


    def get(self):
        """ Returns SDR List """
        return self.ipmi_sdr_dict

# Fetch a Fan Status
SDR_STATUS = SdrStatus()
# Fetch a BMC register


def get_pmc_register(reg_name):
    """ Returns the value of BMC Regster """

    output = None
    sdr_status = SDR_STATUS.get()
    if reg_name in sdr_status:
        output = sdr_status[reg_name]
    else:
        print('\nFailed to fetch: ' +  reg_name + ' sensor ')
        sys.exit(0)

    logging.basicConfig(level=logging.DEBUG)
    return output

# Fetch FRU on given offset
def fetch_raw_fru(dev_id, offset):
    """ Fetch RAW value from FRU (dev_id) @ given offset """
    IPMI_RAW_STORAGE_READ[4] = str(dev_id)
    IPMI_RAW_STORAGE_READ[5] = offset
    status, cmd_ret = getstatusoutput_noshell(IPMI_RAW_STORAGE_READ)
    if status:
        logging.error('Failed to execute ipmitool :' + ' '.join(IPMI_RAW_STORAGE_READ))
        return -1
    return int(cmd_ret.strip().split(' ')[1])




def get_fan_airflow(fan_id):
    """ Return Airflow of direction of FANTRAY(fan_id) """
    airflow_direction = ['Exhaust', 'Intake']
    dir_idx = fetch_raw_fru(fan_id+2, "0x45")
    if dir_idx == -1:
        return 'N/A'
    return airflow_direction[dir_idx]

#Fetch FRU Data for given fruid
def get_psu_airflow(psu_id):
    """ Return Airflow Direction of psu_id """
    airflow_direction = ['Exhaust', 'Intake']
    dir_idx = fetch_raw_fru(psu_id, "0x2F")
    if dir_idx == -1:
        return 'N/A'
    return airflow_direction[dir_idx]

# Print the information for temperature sensors


def print_temperature_sensors():
    """ Prints Temperature Sensor """

    print("\nOnboard Temperature Sensors:")

    print('  PT Left Temp:                   ',\
        (get_pmc_register('PT_Left_temp')))
    print('  NPU Rear Temp:                  ',\
        (get_pmc_register('NPU_Rear_temp')))
    print('  PT Right Temp:                  ',\
        (get_pmc_register('PT_Right_temp')))
    print('  NPU Front Temp:                 ',\
        (get_pmc_register('NPU_Front_temp')))
    print('  FAN Right Temp:                 ',\
        (get_pmc_register('FAN_Right_temp')))
    print('  NPU Temp:                       ',\
        (get_pmc_register('NPU_temp')))
    print('  CPU Temp:                       ',\
        (get_pmc_register('CPU_temp')))
    print('  PSU1 AF Temp:                   ',\
        (get_pmc_register('PSU1_AF_temp')))
    print('  PSU1 Mid Temp:                  ',\
        (get_pmc_register('PSU1_Mid_temp')))
    print('  PSU1 Rear Temp:                 ',\
        (get_pmc_register('PSU1_Rear_temp')))
    print('  PSU2 AF Temp:                   ',\
        (get_pmc_register('PSU2_AF_temp')))
    print('  PSU2 Mid Temp:                  ',\
        (get_pmc_register('PSU2_Mid_temp')))
    print('  PSU2 Rear Temp:                 ',\
        (get_pmc_register('PSU2_Rear_temp')))


    file = '/sys/module/ipmi_si/parameters/kipmid_max_busy_us'
    with open(file, 'w') as f:
        f.write('0\n')

print_temperature_sensors()

# Print the information for 1 Fan Tray


def print_fan_tray(fan_tray):
    """ Prints given Fan Tray information """
    fan_status = ['Abnormal', 'Normal']

    print('  Fan Tray ' + str(fan_tray) + ':')

    fan_front_status = (get_pmc_register('Fan{0}_Front_state'.format(str(fan_tray))) == '')
    fan_rear_status = (get_pmc_register('Fan{0}_Rear_state'.format(str(fan_tray))) == '')
    print('    Fan1 Speed:                   ', \
                        get_pmc_register('FAN{0}_Front_rpm'.format(str(fan_tray))))
    print('    Fan2 Speed:                   ',\
                        get_pmc_register('FAN{0}_Rear_rpm'.format(str(fan_tray))))
    print('    Fan1 State:                   ', fan_status[fan_front_status])
    print('    Fan2 State:                   ', fan_status[fan_rear_status])
    print('    Airflow:                      ', get_fan_airflow(fan_tray))


print('\nFan Trays:')

for tray in range(1, Z9432F_MAX_FAN_TRAYS + 1):
    if get_pmc_register('FAN{0}_prsnt'.format(str(tray))) == 'Present':
        print_fan_tray(tray)
    else:
        print('    Fan Tray {}:                      NOT PRESENT'.format(str(tray)))

def get_psu_status(index):
    """
    Retrieves the presence status of power supply unit (PSU) defined
            by index <index>
    :param index: An integer, index of the PSU of which to query status
    :return: Boolean, True if PSU is plugged, False if not
    """

    status = get_pmc_register('PSU{0}_state'.format(str(index)))
    if len(status.split(',')) > 1:
        return 'NOT OK'
    elif 'Presence' not in status:
        return 'NOT PRESENT'
    return None


# Print the information for PSU1, PSU2
def print_psu(psu_id):
    """ Print PSU information od psu_id """


    # PSU FAN details
    if psu_id == 1:
        print('    PSU1:')
        print('       AF Temperature:           ',\
            get_pmc_register('PSU1_AF_temp'))
        print('       Mid Temperature:          ',\
            get_pmc_register('PSU1_Mid_temp'))
        print('       Rear Temperature:         ',\
            get_pmc_register('PSU1_Rear_temp'))
        print('       FAN RPM:                  ',\
            get_pmc_register('PSU1_rpm'))

        # PSU input & output monitors
        print('       Input Voltage:            ',\
            get_pmc_register('PSU1_In_volt'))
        print('       Output Voltage:           ',\
            get_pmc_register('PSU1_Out_volt'))
        print('       Input Power:              ',\
            get_pmc_register('PSU1_In_watt'))
        print('       Output Power:             ',\
            get_pmc_register('PSU1_Out_watt'))
        print('       Input Current:            ',\
            get_pmc_register('PSU1_In_amp'))
        print('       Output Current:           ',\
            get_pmc_register('PSU1_Out_amp'))

    else:

        print('    PSU2:')
        print('       AF Temperature:           ',\
            get_pmc_register('PSU2_AF_temp'))
        print('       Mid Temperature:          ',\
            get_pmc_register('PSU2_Mid_temp'))
        print('       Rear Temperature:         ',\
            get_pmc_register('PSU2_Rear_temp'))
        print('       FAN RPM:                  ',\
            get_pmc_register('PSU2_rpm'))

        # PSU input & output monitors
        print('       Input Voltage:            ',\
            get_pmc_register('PSU2_In_volt'))
        print('       Output Voltage:           ',\
            get_pmc_register('PSU2_Out_volt'))
        print('       Input Power:              ',\
            get_pmc_register('PSU2_In_watt'))
        print('       Output Power:             ',\
            get_pmc_register('PSU2_Out_watt'))
        print('       Input Current:            ',\
            get_pmc_register('PSU2_In_amp'))
        print('       Output Current:           ',\
            get_pmc_register('PSU2_Out_amp'))
    print('       Airflow:                  ',\
        get_psu_airflow(psu_id))


print('\nPSUs:')
for psu in range(1, Z9432F_MAX_PSUS + 1):
    psu_status = get_psu_status(psu)
    if psu_status is not None:
        print('    PSU{0}:                         {1}'.format(psu, psu_status))
    else:
        print_psu(psu)

print('\n    Total Power:                 ',\
get_pmc_register('PSU_Total_watt'))
file = '/sys/module/ipmi_si/parameters/kipmid_max_busy_us'
with open(file, 'w') as f:
    f.write('1000\n')
