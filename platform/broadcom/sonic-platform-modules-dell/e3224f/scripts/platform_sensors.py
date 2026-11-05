#!/usr/bin/python3
"""
  This provides support for the following objects:
   * Onboard temperature sensors
   * FAN trays
   * PSU
"""

import subprocess

output = ""
try:
    rc = 0
    output = subprocess.check_output('/usr/bin/sensors', stderr=subprocess.STDOUT, \
        encoding="utf-8").splitlines()

    valid = False
    for line in output:
        if line.startswith('acpitz') or line.startswith('coretemp'):
            valid = True
        if valid:
            print(line)
            if line == '':
                valid = False

    print("Onboard Temperature Sensors:")
    idx = 0
    for line in output:
        if line.startswith('tmp75'):
            print('\t' + output[idx+2].split('(')[0])
        idx += 1

    print("\nFan Trays:")
    idx = 0
    found_emc = False
    fan_status = [' Normal', ' Abnormal']
    for line in output:
        if line.startswith('emc'):
            found_emc = True
            with open('/sys/devices/platform/dell-e3224f-cpld.0/fan0_prs') as f:
                line = f.readline()
            present = int(line, 0)
            if present:
                print('\t' + 'Fan Tray 1:')
                with open('/sys/bus/i2c/devices/7-002c/fan1_fault') as f:
                    line = f.readline()
                    status = int(line, 0)
                    print('\t\t' + 'Fan State:' + fan_status[status])
                print('\t\t' + 'Fan Speed:' + (output[idx+2].split('(')[0]).split(':')[1])
                with open('/sys/devices/platform/dell-e3224f-cpld.0/fan0_dir') as f:
                    line = f.readline()
                direction = 'Intake' if line[:-1] == 'B2F' else 'Exhaust'
                print('\t\t' + 'Airflow:\t' + direction)
            else:
                print('\t' + 'Fan Tray 1:\tNot Present')

            with open('/sys/devices/platform/dell-e3224f-cpld.0/fan1_prs') as f:
                line = f.readline()
            present = int(line, 0)
            if present:
                print('\t' + 'Fan Tray 2:')
                with open('/sys/bus/i2c/devices/7-002c/fan2_fault') as f:
                    line = f.readline()
                    status = int(line, 0)
                    print('\t\t' + 'Fan State:' + fan_status[status])
                print('\t\t' + 'Fan Speed:' + (output[idx+3].split('(')[0]).split(':')[1])
                with open('/sys/devices/platform/dell-e3224f-cpld.0/fan1_dir') as f:
                    line = f.readline()
                direction = 'Intake' if line[:-1] == 'B2F' else 'Exhaust'
                print('\t\t' + 'Airflow:\t' + direction)
            else:
                print('\t' + 'Fan Tray 2:\tNot Present')

            with open('/sys/devices/platform/dell-e3224f-cpld.0/fan2_prs') as f:
                line = f.readline()
            present = int(line, 0)
            if present:
                print('\t' + 'Fan Tray 3:')
                with open('/sys/bus/i2c/devices/7-002c/fan3_fault') as f:
                    line = f.readline()
                    status = int(line, 0)
                    print('\t\t' + 'Fan State:' + fan_status[status])
                print('\t\t' + 'Fan Speed:' + (output[idx+4].split('(')[0]).split(':')[1])
                with open('/sys/devices/platform/dell-e3224f-cpld.0/fan2_dir') as f:
                    line = f.readline()
                direction = 'Intake' if line[:-1] == 'B2F' else 'Exhaust'
                print('\t\t' + 'Airflow:\t' + direction)
            else:
                print('\t' + 'Fan Tray 3:\tNot Present')
        idx += 1
    if not found_emc:
        print('\t' + 'Fan Tray 1:\tNot Present')
        print('\t' + 'Fan Tray 2:\tNot Present')
        print('\t' + 'Fan Tray 3:\tNot Present')

    print('\nPSUs:')
    idx = 0
    with open('/sys/devices/platform/dell-e3224f-cpld.0/psu0_prs') as f:
        line = f.readline()
    found_psu1 = int(line, 0)
    if not found_psu1:
        print('\tPSU1:\tNot Present')
    else:
        for line in output:
            if line.startswith('dps460-i2c-10'):
                with open('/sys/devices/platform/dell-e3224f-cpld.0/psu0_status') as f:
                    line = f.readline()
                status = int(line, 0)
                if not status:
                    print('\tPSU1:\tNot OK')
                    break
                with open('/sys/bus/i2c/devices/10-0056/eeprom', encoding="ISO-8859-1") as f:
                    line = f.readline()
                direction = 'Exhaust' if 'FORWARD' in line else 'Intake'
                print('\tPSU1:')
                print('\t\t' + output[idx+2].split('(')[0])
                print('\t\t' + output[idx+4].split('(')[0])
                print('\t\t' + output[idx+6].split('(')[0])
                print('\t\t' + output[idx+7].split('(')[0])
                print('\t\t' + output[idx+9].split('(')[0])
                print('\t\t' + output[idx+11].split('(')[0])
                print('\t\t' + output[idx+13].split('(')[0])
                print('\t\t' + output[idx+14].split('(')[0])
                print('\t\t' + output[idx+16].split('(')[0])
                print('\t\t' + output[idx+17].split('(')[0])
                print('\t\t' + 'Airflow:\t\t  ' + direction)
            idx += 1
    idx = 0
    with open('/sys/devices/platform/dell-e3224f-cpld.0/psu1_prs') as f:
        line = f.readline()
    found_psu2 = int(line, 0)
    if not found_psu2:
        print('\tPSU2:\tNot Present')
    else:
        for line in output:
            if line.startswith('dps460-i2c-11'):
                with open('/sys/devices/platform/dell-e3224f-cpld.0/psu1_status') as f:
                    line = f.readline()
                status = int(line, 0)
                if not status:
                    print('\tPSU2:\tNot OK')
                    break
                print('\tPSU2:')
                with open('/sys/bus/i2c/devices/11-0056/eeprom', encoding="ISO-8859-1") as f:
                    line = f.readline()
                direction = 'Exhaust' if 'FORWARD' in line else 'Intake'
                print('\t\t' + output[idx+2].split('(')[0])
                print('\t\t' + output[idx+4].split('(')[0])
                print('\t\t' + output[idx+6].split('(')[0])
                print('\t\t' + output[idx+7].split('(')[0])
                print('\t\t' + output[idx+9].split('(')[0])
                print('\t\t' + output[idx+11].split('(')[0])
                print('\t\t' + output[idx+13].split('(')[0])
                print('\t\t' + output[idx+14].split('(')[0])
                print('\t\t' + output[idx+16].split('(')[0])
                print('\t\t' + output[idx+17].split('(')[0])
                print('\t\t' + 'Airflow:\t\t  ' + direction)
            idx += 1

except subprocess.CalledProcessError as err:
    print("Exception when calling get_sonic_error -> %s\n" %(err))
    rc = err.returncode
