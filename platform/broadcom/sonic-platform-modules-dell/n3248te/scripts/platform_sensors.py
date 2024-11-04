#!/usr/bin/python
# This provies support for the following objects:
#   * Onboard temperature sensors
#   * FAN trays
#   * PSU


import subprocess

output = ""
try:
    rc = 0
    output = subprocess.check_output('/usr/bin/sensors', text=True).splitlines()

    valid = False
    for line in output:
        if line.startswith('acpitz') or line.startswith('coretemp'):
            valid = True
        if valid:
            print(line)
            if line == '': valid = False

    print("Onboard Temperature Sensors:")
    idx = 0
    for line in output:
        if line.startswith('tmp75'):
            print('\t' + output[idx+2].split('(')[0])
        idx += 1

    print("\nFanTrays:")
    idx = 0
    found_emc = False
    for line in output:
        if line.startswith('emc'):
            found_emc = True
            with open('/sys/devices/platform/dell-n3248te-cpld.0/fan0_prs') as f:
                line = f.readline()
            present = int(line, 0)
            if present :
                print('\t' + 'FanTray1:')
                print('\t\t' + 'Fan Speed:' + (output[idx+2].split('(')[0]).split(':')[1])
                with open('/sys/devices/platform/dell-n3248te-cpld.0/fan0_dir') as f:
                    line = f.readline()
                dir = 'Intake' if line[:-1] == 'B2F' else 'Exhaust'
                print('\t\t' + 'Airflow:\t' + dir)
            else : print('\t' + 'FanTray1:\tNot Present')

            with open('/sys/devices/platform/dell-n3248te-cpld.0/fan1_prs') as f:
                line = f.readline()
            present = int(line, 0)
            if present :
                print('\t' + 'FanTray2:')
                print('\t\t' + 'Fan Speed:' + (output[idx+3].split('(')[0]).split(':')[1])
                with open('/sys/devices/platform/dell-n3248te-cpld.0/fan1_dir') as f:
                    line = f.readline()
                dir = 'Intake' if line[:-1] == 'B2F' else 'Exhaust'
                print('\t\t' + 'Airflow:\t' + dir)
            else : print('\t' + 'FanTray2:\tNot Present')

            with open('/sys/devices/platform/dell-n3248te-cpld.0/fan2_prs') as f:
                line = f.readline()
            present = int(line, 0)
            if present :
                print('\t' + 'FanTray3:')
                print('\t\t' + 'Fan Speed:' + (output[idx+4].split('(')[0]).split(':')[1])
                with open('/sys/devices/platform/dell-n3248te-cpld.0/fan2_dir') as f:
                    line = f.readline()
                dir = 'Intake' if line[:-1] == 'B2F' else 'Exhaust'
                print('\t\t' + 'Airflow:\t' + dir)
            else : print('\t' + 'FanTray3:\tNot Present')
        idx += 1
    if not found_emc :
        print('\t' + 'FanTray1:\tNot Present')
        print('\t' + 'FanTray2:\tNot Present')
        print('\t' + 'FanTray3:\tNot Present')

    print('\nPSUs:')
    idx = 0
    with open('/sys/devices/platform/dell-n3248te-cpld.0/psu0_prs') as f:
        line = f.readline()
    found_psu1 = int(line, 0)
    if not found_psu1 :
        print('\tPSU1:\tNot Present')
    with open('/sys/devices/platform/dell-n3248te-cpld.0/psu1_prs') as f:
        line = f.readline()
    found_psu2 = int(line, 0)
    for line in output:
        if line.startswith('dps460-i2c-10'):
            with open('/sys/devices/platform/dell-n3248te-cpld.0/psu0_status') as f:
                line = f.readline()
            status = int(line, 0)
            if not status :
                print('\tPSU1:\tNot OK')
                break
            with open('/sys/bus/i2c/devices/10-0056/eeprom', encoding='iso-8859-1') as f:
                line = f.readline()
            dir = 'Exhaust' if 'FORWARD' in line else 'Intake'
            print('\tPSU1:')
            print('\t\t' + output[idx+2].split('(')[0])
            print('\t\t' + output[idx+4].split('(')[0])
            print('\t\t' + output[idx+6].split('(')[0])
            print('\t\t' + output[idx+7].split('(')[0])
            print('\t\t' + output[idx+9].split('(')[0])
            print('\t\t' + output[idx+11].split('(')[0])
            print('\t\t' + output[idx+12].split('(')[0])
            print('\t\t' + output[idx+14].split('(')[0])
            print('\t\t' + output[idx+15].split('(')[0])
            print('\t\t' + 'Airflow:\t\t  ' + dir)
        if line.startswith('dps460-i2c-11'):
            with open('/sys/devices/platform/dell-n3248te-cpld.0/psu1_status') as f:
                line = f.readline()
            status = int(line, 0)
            if not status :
                print('\tPSU2:\tNot OK')
                break
            print('\tPSU2:')
            with open('/sys/bus/i2c/devices/11-0056/eeprom', encoding='iso-8859-1') as f:
                line = f.readline()
            dir = 'Exhaust' if 'FORWARD' in line else 'Intake'
            print('\t\t' + output[idx+2].split('(')[0])
            print('\t\t' + output[idx+4].split('(')[0])
            print('\t\t' + output[idx+6].split('(')[0])
            print('\t\t' + output[idx+7].split('(')[0])
            print('\t\t' + output[idx+9].split('(')[0])
            print('\t\t' + output[idx+11].split('(')[0])
            print('\t\t' + output[idx+12].split('(')[0])
            print('\t\t' + output[idx+14].split('(')[0])
            print('\t\t' + output[idx+15].split('(')[0])
            print('\t\t' + 'Airflow:\t\t  ' + dir)
        idx += 1
    if not found_psu2 :
        print('\tPSU2:\tNot Present')

except subprocess.CalledProcessError as err:
    print ("Exception when calling get_sonic_error -> %s\n" %(err))
    rc = err.returncode
