#!/usr/bin/python3
"""
# Module on S5448F, the BaseBoard Management Controller is an
# autonomous subsystem provides monitoring and management
# facility independent of the host CPU. IPMI standard
# protocol is used with ipmitool to fetch sensor details.
# This provides support for the following objects:
#   * Onboard temperature sensors
#   * FAN trays
#   * PSU
"""

import sys
import logging
import subprocess

S5448F_MAX_FAN_TRAYS = 6
S5448F_MAX_PSUS = 2
IPMI_SENSOR_DATA = "ipmitool sdr elist"

IPMI_FAN_PRESENCE = "ipmitool sensor get FAN{0}_prsnt"
IPMI_RAW_STORAGE_READ = "ipmitool raw 0x0a 0x11 {0} {1} 0 1"

# Dump sensor registers
class SdrStatus(object):
    """ Contains IPMI SDR List """
    def __init__(self):
        ipmi_cmd = IPMI_SENSOR_DATA
        try:
            status, resp = subprocess.getstatusoutput(ipmi_cmd)
            if status:
                logging.error('Failed to execute: %s', ipmi_cmd)
                sys.exit(0)
        except Exception:
            logging.error('Failed to execute: %s', ipmi_cmd)
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
def _get_bmc_register(reg_name):
    """ Returns the value of BMC Register """

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
def _fetch_raw_fru(dev_id, offset):
    """ Fetch RAW value from FRU (dev_id) @ given offset """
    try:
        status, cmd_ret = subprocess.getstatusoutput(IPMI_RAW_STORAGE_READ.format(dev_id, offset))
        if status != 0:
            logging.error('Failed to execute ipmitool : %s', IPMI_RAW_STORAGE_READ.format(dev_id))
            sys.exit(0)
        else:
            return int(cmd_ret.strip().split(' ')[1])

    except Exception:
        logging.error('Failed to execute ipmitool : %s', IPMI_RAW_STORAGE_READ.format(dev_id))
        sys.exit(0)


def _get_fan_airflow(fan_id):
    """ Return Airflow of direction of FANTRAY(fan_id) """
    airflow_direction = ['Exhaust', 'Intake']
    dir_idx = _fetch_raw_fru(fan_id+2, 0x45)
    if dir_idx == -1:
        return 'N/A'
    return airflow_direction[dir_idx]

#Fetch FRU Data for given fruid
def _get_psu_airflow(psu_id):
    """ Return Airflow Direction of psu_id """
    airflow_direction = ['Exhaust', 'Intake']
    dir_idx = _fetch_raw_fru(psu_id, 0x2F)
    if dir_idx == -1:
        return 'N/A'
    return airflow_direction[dir_idx]

# Print the information for temperature sensors


def _print_temperature_sensors():
    """ Prints Temperature Sensor """

    print("\nOnboard Temperature Sensors:")

    print('  PT Left Temp:                   ',\
        (_get_bmc_register('PT_Left_temp')))
    print('  NPU Rear Temp:                  ',\
        (_get_bmc_register('NPU_Rear_temp')))
    print('  PT Right Temp:                  ',\
        (_get_bmc_register('PT_Right_temp')))
    print('  FAN Right Temp:                 ',\
        (_get_bmc_register('FAN_Right_temp')))
    print('  NPU Temp:                       ',\
        (_get_bmc_register('NPU_temp')))
    print('  CPU Temp:                       ',\
        (_get_bmc_register('CPU_temp')))
    print('  CPUCD Rear Temp:                ',\
        (_get_bmc_register('CPUCD_Rear_temp')))
    print('  PSU1 AF Temp:                   ',\
        (_get_bmc_register('PSU1_AF_temp')))
    print('  PSU1 Mid Temp:                  ',\
        (_get_bmc_register('PSU1_Mid_temp')))
    print('  PSU1 Rear Temp:                 ',\
        (_get_bmc_register('PSU1_Rear_temp')))
    print('  PSU2 AF Temp:                   ',\
        (_get_bmc_register('PSU2_AF_temp')))
    print('  PSU2 Mid Temp:                  ',\
        (_get_bmc_register('PSU2_Mid_temp')))
    print('  PSU2 Rear Temp:                 ',\
        (_get_bmc_register('PSU2_Rear_temp')))

try:
    RET_STATUS, IPMI_CMD_RET = \
        subprocess.getstatusoutput('echo 0 > /sys/module/ipmi_si/parameters/kipmid_max_busy_us')
    if RET_STATUS:
        logging.error('Failed to execute ipmitool : %d', RET_STATUS)

except Exception:
    logging.error('Failed to execute ipmitool :%d', RET_STATUS)

_print_temperature_sensors()

# Print the information for 1 Fan Tray


def _print_fan_tray(fan_tray):
    """ Prints given Fan Tray information """
    fan_status = ['Abnormal', 'Normal']

    print('  Fan Tray ' + str(fan_tray) + ':')

    fan_front_status = (_get_bmc_register('Fan{0}_Front_state'.format(str(fan_tray))) == '')
    fan_rear_status = (_get_bmc_register('Fan{0}_Rear_state'.format(str(fan_tray))) == '')
    print('    Fan1 Speed:                   ', \
                        _get_bmc_register('FAN{0}_Front_rpm'.format(str(fan_tray))))
    print('    Fan2 Speed:                   ',\
                        _get_bmc_register('FAN{0}_Rear_rpm'.format(str(fan_tray))))
    print('    Fan1 State:                   ', fan_status[fan_front_status])
    print('    Fan2 State:                   ', fan_status[fan_rear_status])
    print('    Airflow:                      ', _get_fan_airflow(fan_tray))


print('\nFan Trays:')

for tray in range(1, S5448F_MAX_FAN_TRAYS + 1):
    if _get_bmc_register('FAN{0}_prsnt'.format(str(tray))) == 'Present':
        _print_fan_tray(tray)
    else:
        print('    Fan Tray {}:                      NOT PRESENT'.format(str(tray)))

def _get_psu_status(index):
    """
    Retrieves the presence status of power supply unit (PSU) defined
            by index <index>
    :param index: An integer, index of the PSU of which to query status
    :return: Boolean, True if PSU is plugged, False if not
    """

    status = _get_bmc_register('PSU{0}_state'.format(str(index)))
    if len(status.split(',')) > 1:
        return 'NOT OK'
    if 'Presence' not in status:
        return 'NOT PRESENT'
    return None


# Print the information for PSU1, PSU2
def _print_psu(psu_id):
    """ Print PSU information od psu_id """


    # PSU FAN details
    if psu_id == 1:
        print('    PSU1:')
        print('       AF Temperature:           ',\
            _get_bmc_register('PSU1_AF_temp'))
        print('       Mid Temperature:          ',\
            _get_bmc_register('PSU1_Mid_temp'))
        print('       Rear Temperature:         ',\
            _get_bmc_register('PSU1_Rear_temp'))
        print('       FAN RPM:                  ',\
            _get_bmc_register('PSU1_rpm'))

        # PSU input & output monitors
        print('       Input Voltage:            ',\
            _get_bmc_register('PSU1_In_volt'))
        print('       Output Voltage:           ',\
            _get_bmc_register('PSU1_Out_volt'))
        print('       Input Power:              ',\
            _get_bmc_register('PSU1_In_watt'))
        print('       Output Power:             ',\
            _get_bmc_register('PSU1_Out_watt'))
        print('       Input Current:            ',\
            _get_bmc_register('PSU1_In_amp'))
        print('       Output Current:           ',\
            _get_bmc_register('PSU1_Out_amp'))

    else:

        print('    PSU2:')
        print('       AF Temperature:           ',\
            _get_bmc_register('PSU2_AF_temp'))
        print('       Mid Temperature:          ',\
            _get_bmc_register('PSU2_Mid_temp'))
        print('       Rear Temperature:         ',\
            _get_bmc_register('PSU2_Rear_temp'))
        print('       FAN RPM:                  ',\
            _get_bmc_register('PSU2_rpm'))

        # PSU input & output monitors
        print('       Input Voltage:            ',\
            _get_bmc_register('PSU2_In_volt'))
        print('       Output Voltage:           ',\
            _get_bmc_register('PSU2_Out_volt'))
        print('       Input Power:              ',\
            _get_bmc_register('PSU2_In_watt'))
        print('       Output Power:             ',\
            _get_bmc_register('PSU2_Out_watt'))
        print('       Input Current:            ',\
            _get_bmc_register('PSU2_In_amp'))
        print('       Output Current:           ',\
            _get_bmc_register('PSU2_Out_amp'))
    print('       Airflow:                  ',\
        _get_psu_airflow(psu_id))


print('\nPSUs:')
for psu in range(1, S5448F_MAX_PSUS + 1):
    psu_status = _get_psu_status(psu)
    if psu_status is not None:
        print('    PSU{0}:                         {1}'.format(psu, psu_status))
    else:
        _print_psu(psu)

print('\n    Total Power:                 ',\
    _get_bmc_register('PSU_Total_watt'))

try:
    RET_STATUS, IPMI_CMD_RET = \
        subprocess.getstatusoutput('echo 3000 > /sys/module/ipmi_si/parameters/kipmid_max_busy_us')
    if RET_STATUS:
        logging.error('Failed to execute ipmitool : %s', RET_STATUS)

except Exception:
    logging.error('Failed to execute ipmitool :%s', RET_STATUS)
