#!/usr/bin/env python3
# -*- coding: UTF-8 -*-
import inspect
import sys
import json
import time
from plat_hal.interface import interface
from tabulate import tabulate

ERROR_RET = -99999

class Command():
    def __init__(self, name, f):
        self.name = name
        self.f = f
        self.paramcount = self.f.__code__.co_argcount

    def dofun(self, args):
        fn = self.f.__call__
        fn(*args)


class Group():
    def __init__(self, name, f):
        self.groups = []
        self.commands = []
        self.name = name
        self.f = f

    def add_groups(self, command):
        self.groups.append(command)

    def add_commands(self, commnad):
        x = Command(commnad.__name__, commnad)
        self.commands.append(x)

    def find_valuebyname(self, name):
        for item in self.groups:
            if name == item.name:
                return item
        for item in self.commands:
            if name == item.name:
                return item
        return None

    def deal(self, args):
        if len(args) <= 0:
            return self.print_help()
        funclevel = args[0]
        val = self.find_valuebyname(funclevel)
        if val is None:
            return self.print_help()
        if isinstance(val, Command):
            if len(args) < (val.paramcount + 1):
                return self.print_help()
            inputargs = args[1: (1 + val.paramcount)]
            return val.dofun(inputargs)
        if isinstance(val, Group):
            args = args[1:]
            return val.deal(args)
        return self.print_help()

    def get_max(self, arr):
        lentmp = 0
        for ar in arr:
            lentmp = len(ar) if (len(ar) > lentmp) else lentmp
        return lentmp

    def print_help(self):

        namesize = []
        for item in self.groups:
            namesize.append(item.name)
        for item in self.commands:
            namesize.append(item.name)
        maxvalue = self.get_max(namesize)

        if len(self.groups) > 0:
            print("Groups:")
            for item in self.groups:
                print("   %-*s    %s" % (maxvalue, item.name, item.f.__doc__ or ''))
        if len(self.commands) > 0:
            print("Commands:")
            for item in self.commands:
                print("   %-*s   %s" % (maxvalue, item.name, item.f.__doc__ or ''))


class clival():
    @staticmethod
    def Fire(val=None):
        group = Group("top", 'mainlevel')
        clival.iterGroup(val, group)
        # context = {}
        # caller = inspect.stack()[1]
        # caller_frame = caller[0]
        # caller_globals = caller_frame.f_globals
        # caller_locals = caller_frame.f_locals
        # context.update(caller_globals)
        # context.update(caller_locals)
        args = sys.argv[1:]
        group.deal(args)

    @staticmethod
    def iterGroup(val, group):
        for key, item in val.items():
            if item is None:  # first level
                if inspect.isfunction(key):
                    group.add_commands(key)
            else:
                group1 = Group(key.__name__, key)
                clival.iterGroup(item, group1)
                group.add_groups(group1)


def psu():
    r'''test psu '''


def fan():
    r'''test fan '''


def sensor():
    r'''test sensor '''


def dcdc():
    r'''test dcdc '''

def temp():
    r'''test temp '''
    pass

def led():
    r'''test led '''


def e2():
    r'''test onie eeprom '''


def temps():
    r'''test temps sensor'''

def cpu():
    r'''test cpu'''


int_case = interface()


def get_total_number():
    r'''psu  get_total_number '''
    print("=================get_total_number======================")
    print(int_case.get_psu_total_number())


def get_presence():
    r'''psu  get_presence '''
    print("=================get_presence======================")
    psus = int_case.get_psus()
    for psu_item in psus:
        print(psu_item.name, end=' ')
        print(int_case.get_psu_presence(psu_item.name))


def get_fru_info():
    r'''psu  get_fru_info '''
    print("=================get_fru_info======================")
    psus = int_case.get_psus()
    for psu_item in psus:
        print(psu_item.name, end=' ')
        print(json.dumps(int_case.get_psu_fru_info(psu_item.name), ensure_ascii=False, indent=4))


def get_status():
    r'''psu  get_status '''
    print("=================get_status======================")
    psus = int_case.get_psus()
    for psu_item in psus:
        print(psu_item.name, end=' ')
        print(json.dumps(int_case.get_psu_status(psu_item.name), ensure_ascii=False, indent=4))


def set_psu_fan_speed_pwm(realspeed):
    r'''set_psu_fan_speed_pwm pwm'''
    print("=================set_psu_fan_speed_pwm======================")
    psus = int_case.get_psus()
    for psu_item in psus:
        print(psu_item.name, end=' ')
        print(int_case.set_psu_fan_speed_pwm(psu_item.name, int(realspeed)))


def get_psu_fan_speed_pwm():
    r'''get_psu_fan_speed_pwm'''
    print("=================get_psu_fan_speed_pwm======================")
    psus = int_case.get_psus()
    for psu_item in psus:
        print(psu_item.name, end=' ')
        print(json.dumps(int_case.get_psu_fan_speed_pwm(psu_item.name)))


def get_psu_power_status():
    r'''psu  get_psu_power_status '''
    print("=================get_psu_power_status======================")
    psus = int_case.get_psus()
    for psu_item in psus:
        print(psu_item.name, end=' ')
        print(json.dumps(int_case.get_psu_power_status(psu_item.name), ensure_ascii=False, indent=4))


def get_psu_info_all():
    r'''get_psu_info_all '''
    psus = int_case.get_psu_info_all()
    header = ['NAME', 'Presence', 'PN', 'SN', 'InCurrent(A)', 'InVoltage(V)', 'InPower(W)', 'OutCurrent(A)', \
        'OutVoltage(V)', 'OutPower(W)', 'Temperature(C)', 'FanSpeed']
    table = []

    psu_names = [ k for k in psus.keys() if k.startswith('PSU') ]
    psu_names.sort()
 
    for psu_name in psu_names:
        psu = psus[psu_name]      
        presence = psu.get('Present')
        pn = psu.get('PN')
        sn = psu.get('SN')
        if not presence:
            continue
        
        if presence == 'no':
            presence = 'NOT_PRESENT'
            pn = 'N/A'
            sn = 'N/A'
            in_c = 'N/A'
            in_v = 'N/A'
            in_p = 'N/A'
            out_c = 'N/A'
            out_v = 'N/A'
            out_p = 'N/A'
            temp = 'N/A'
            speed = 'N/A'
        elif presence == 'yes':
            presence = 'PRESENT'
            temp = psu.get('Temperature', {}).get('Value')
            speed = psu.get('FanSpeed', {}).get('Value')
            in_c = psu.get('Inputs', {}).get('Current', {}).get('Value')
            in_v = psu.get('Inputs', {}).get('Voltage', {}).get('Value')
            in_p = psu.get('Inputs', {}).get('Power', {}).get('Value')
            out_c = psu.get('Outputs', {}).get('Current', {}).get('Value')
            out_v = psu.get('Outputs', {}).get('Voltage', {}).get('Value')
            out_p = psu.get('Outputs', {}).get('Power', {}).get('Value')
        else:
            continue
        table.append([psu_name, presence, pn, sn, in_c, in_v, in_p, out_c, out_v, out_p, temp, speed])
    if table:
        print(tabulate(table, header, tablefmt='simple', stralign='left'))
    else:
        print('No psu status data available\n')


def get_fan_total_number():
    print("=================get_info_all======================")
    print(json.dumps(int_case.get_fan_total_number(), ensure_ascii=False, indent=4))


def get_fan_rotor_number():
    r'''get_fan_rotor_number'''
    print("=================get_fan_rotor_number======================")
    fans = int_case.get_fans()
    for fan in fans:
        print(fan.name, end=' ')
        print(int_case.get_fan_rotor_number(fan.name))



def get_fan_speed():
    r'''get_fan_speed'''
    print("=================get_fan_speed======================")
    fans = int_case.get_fans()
    for fan in fans:
        rotors = fan.rotor_list
        for rotor in rotors:
            index = rotors.index(rotor)
            print("%s rotor%d" % (fan.name, index + 1), end='  ')
            print(int_case.get_fan_speed(fan.name, index + 1))
    pass


def get_fan_speed_pwm():
    r'''get_fan_speed_pwm'''
    print("=================get_fan_speed_pwm======================")
    fans = int_case.get_fans()
    for fan in fans:
        rotors = fan.rotor_list
        for rotor in rotors:
            index = rotors.index(rotor)
            print("%s rotor%d" % (fan.name, index + 1), end='  ')
            print(int_case.get_fan_speed_pwm(fan.name, index + 1))


def set_fan_speed_pwm(pwm):
    r'''set_fan_speed_pwm pwm'''
    print("=================set_fan_speed_pwm======================")
    fans = int_case.get_fans()
    for fan in fans:
        rotors = fan.rotor_list
        for rotor in rotors:
            index = rotors.index(rotor)
            print("%s %s" % (fan.name, rotor.name), end='  ')
            val = int_case.set_fan_speed_pwm(fan.name, index + 1, pwm)
            print(val)


def get_fan_watchdog_status():
    r'''get_fan_watchdog_status'''
    print("=================get_fan_watchdog_status======================")
    print(int_case.get_fan_watchdog_status())


def enable_fan_watchdog():
    r'''enable_fan_watchdog'''
    print("=================enable_fan_watchdog======================")
    print('enable', int_case.enable_fan_watchdog())


def disable_fan_watchdog():
    r'''disable_fan_watchdog'''
    print("=================disable_fan_watchdog======================")
    print('disable', int_case.enable_fan_watchdog(enable=False))


def get_fan_speed1():
    r'''get_fan_speed'''
    print("=================get_fan_speed======================")
    fans = int_case.get_fans()
    for fan in fans:
        total = int_case.get_fan_rotor_number(fan.name)
        rotors = fan.rotor_list
        for rotor in rotors:
            print("%s %s" % (fan.name, rotor.name), end='  ')
            print(int_case.get_fan_speed(fan.name, rotor.name))
    pass


def fan_feed_watchdog():
    r'''fan_feed_watchdog'''
    print("=================fan_feed_watchdog======================")
    get_fan_speed()
    print(int_case.feed_fan_watchdog(None))
    time.sleep(2)
    get_fan_speed()


def set_fan_led(color):
    r'''set_fan_led color'''
    print("=================set_fan_led======================")
    fans = int_case.get_fans()
    for fan in fans:
        print("%s" % fan.name)
        print(color, int_case.set_fan_led(fan.name, color))

def get_fan_led():
    r'''fan_get_led'''
    print("=================fan_get_led======================")
    fans = int_case.get_fans()
    for fan_item in fans:
        print("%s" % fan_item.name)
        print(int_case.get_fan_led(fan_item.name))


def get_fan_presence():
    r'''get_fan_presence'''
    print("=================get_fan_presence======================")
    fans = int_case.get_fans()
    for fan in fans:
        print("%s" % fan.name)
        print(int_case.get_fan_presence(fan.name))


def get_fan_fru_info():
    r'''get_fan_fru_info'''
    print("=================get_fan_fru_info======================")
    fans = int_case.get_fans()
    for fan in fans:
        print("%s" % fan.name)
        print(json.dumps(int_case.get_fan_info(fan.name), ensure_ascii=False, indent=4))


def get_fan_status():
    r'''get_fan_status'''
    print("=================get_fan_status======================")
    fans = int_case.get_fans()
    for fan in fans:
        print("%s" % fan.name)
        print(json.dumps(int_case.get_fan_status(fan.name), ensure_ascii=False, indent=4))


def get_fan_info_all():
    r'''get_fan_info_all '''
    fans = int_case.get_fan_info_all()
    header = ['NAME', 'Presence', 'HW', 'SN', 'AirFlow', 'Rotor1Speed', 'Rotor2Speed']
    table = []

    fan_names = [ k for k in fans.keys() if k.startswith('FAN') ]
    fan_names.sort()
 
    for fan_name in fan_names:
        fan = fans[fan_name]      
        presence = fan.get('Present')
        sn = fan.get('SN')
        hw = fan.get('HW')
        air_flow = fan.get('AirFlow')
        if not presence:
            continue
        if presence == 'no':
            presence = 'NOT_PRESENT'
            sn = 'N/A'
            hw = 'N/A'
            air_flow = 'N/A'
            speed1 = 'N/A'
            speed2 = 'N/A'
        elif presence == 'yes':
            presence = 'PRESENT'
            speed1 = fan.get('Rotor1', {}).get('Speed', 'N/A')
            speed2 = fan.get('Rotor2', {}).get('Speed', 'N/A')
        else:
            continue
        table.append([fan_name, presence, hw, sn, air_flow, speed1, speed2])
    if table:
        print(tabulate(table, header, tablefmt='simple', stralign='left'))
    else:
        print('No fan status data available\n')
    pass


def get_sensor_info():
    r'''get_sensor_info'''
    header = ['Name', 'Min', 'Max', 'Value', 'Unit', 'status']
    table = []
    sensors = int_case.get_sensor_info()
    for name, value in sensors.items():
        status = "OK"
        if (value['Value'] == ERROR_RET or value['Value'] is None):
            status = "NOT OK"
            value['Value'] = ERROR_RET
        if value['Min'] is None:
            value['Min'] = "NA"
        else:
            if (float(value['Value']) < float(value['Min'])):
                status = "NOT OK"
        if value['Max'] is None:
            value['Max'] = "NA"
        else:
            if (float(value['Value']) > float(value['Max'])):
                status = "NOT OK"
        table.append((name, value['Min'], value['Max'], value['Value'], value['Unit'], status))
    if table:
        print(tabulate(table, header, tablefmt='simple', stralign='left'))
    else:
        print('No sensor status data available\n')

def get_temp_info():
    r'''get_temp_info'''
    header = ['Name', 'Min', 'Max', 'Value', 'Unit', 'status']
    table = []
    temps = int_case.get_temp_info()
    for name, value in temps.items():
        status = "OK"
        if (value['Value'] == ERROR_RET or value['Value'] is None):
            status = "NOT OK"
            value['Value'] = ERROR_RET
        if value['Min'] is None:
            value['Min'] = "NA"
        else:
            if (float(value['Value']) < float(value['Min'])):
                status = "NOT OK"
        if value['Max'] is None:
            value['Max'] = "NA"
        else:
            if (float(value['Value']) > float(value['Max'])):
                status = "NOT OK"
        table.append((name, value['Min'], value['Max'], value['Value'], value['Unit'], status))
    if table:
        print(tabulate(table, header, tablefmt='simple', stralign='left'))
    else:
        print('No temp status data available\n')

def get_dcdc_all_info():
    r'''get_dcdc_all_info'''
    header = ['Name', 'Min', 'Max', 'Value', 'Unit', 'status']
    table = []
    dcdcs = int_case.get_dcdc_all_info()
    for name, value in dcdcs.items():
        status = "OK"
        if (value['Value'] == ERROR_RET or value['Value'] is None):
            status = "NOT OK"
            value['Value'] = ERROR_RET
        if value['Min'] is None:
            value['Min'] = "NA"
        else:
            if (float(value['Value']) < float(value['Min'])):
                status = "NOT OK"
        if value['Max'] is None:
            value['Max'] = "NA"
        else:
            if (float(value['Value']) > float(value['Max'])):
                status = "NOT OK"
        table.append((name, value['Min'], value['Max'], value['Value'], value['Unit'], status))
    if table:
        print(tabulate(table, header, tablefmt='simple', stralign='left'))
    else:
        print('No dcdc status data available\n')


def set_all_led_color(color):
    r'''set_all_led_color color'''
    print("=================set_all_led_color======================")
    leds = int_case.get_leds()
    for led_item in leds:
        print("%s" % led_item.name)
        print(color, int_case.set_led_color(led_item.name, color))


def get_all_led_color():
    r'''get_all_led_color'''
    print("=================get_all_led_color======================")
    leds = int_case.get_leds()
    for led_item in leds:
        print("%s" % led_item.name)
        print(int_case.get_led_color(led_item.name))


def set_single_led_color(led_name, color):
    r'''set_single_led_color led_name color'''
    print("=================set_single_led_color======================")
    leds = int_case.get_leds()
    for led_item in leds:
        if led_name == led_item.name:
            print("%s" % led_item.name)
            print(color, int_case.set_led_color(led_item.name, color))


def get_single_led_color(led_name):
    r'''get_single_led_color led_name'''
    print("=================get_single_led_color======================")
    leds = int_case.get_leds()
    for led_item in leds:
        if led_name == led_item.name:
            print("%s" % led_item.name)
            print(int_case.get_led_color(led_item.name))


def get_onie_e2_path():
    r'''get_onie_e2_path'''
    print("=================get_onie_e2_path======================")
    path = int_case.get_onie_e2_path("ONIE_E2")
    print("%s" % path)


def get_device_airflow():
    r'''get_device_airflow'''
    print("=================get_device_airflow======================")
    airflow = int_case.get_device_airflow("ONIE_E2")
    print("%s" % airflow)


def get_temps_sensor():
    r'''get_temps_sensor'''
    print("=================get_temps_sensor======================")
    temp_list = int_case.get_temps()
    for temp in temp_list:
        print("id: %s, name: %s, API name: %s, value: %s, Min: %s, Low: %s, High: %s, Max: %s, Invalid: %s, Error: %s" %
            (temp.temp_id, temp.name, temp.api_name, temp.Value, temp.Min, temp.Low, temp.High, temp.Max, temp.temp_invalid, temp.temp_error))

def get_cpu_reset_num():
    r'''get_cpu_reset_num'''
    print("=================get_cpu_reset_num======================")
    print(int_case.get_cpu_reset_num())

def get_cpu_reboot_cause():
    r'''get_cpu_reboot_cause'''
    print("=================get_cpu_reboot_cause======================")
    print(int_case.get_cpu_reboot_cause())


def run_cli_man():
    clival.Fire(
        {
            psu: {
                get_total_number: None,
                get_presence: None,
                get_fru_info: None,
                set_psu_fan_speed_pwm: None,
                get_psu_fan_speed_pwm: None,
                get_status: None,
                get_psu_power_status: None,
                get_psu_info_all: None
            },
            fan: {
                get_fan_total_number: None,
                get_fan_rotor_number: None,
                get_fan_speed: None,
                get_fan_speed_pwm: None,
                set_fan_speed_pwm: None,
                get_fan_watchdog_status: None,
                enable_fan_watchdog: None,
                disable_fan_watchdog: None,
                fan_feed_watchdog: None,
                set_fan_led: None,
                get_fan_led: None,
                get_fan_presence: None,
                get_fan_fru_info: None,
                get_fan_status: None,
                get_fan_info_all: None
            },
            sensor: {
                get_sensor_info: None
            },
            dcdc: {
                get_dcdc_all_info: None
            },
            temp: {
                get_temp_info: None
            },
            led: {
                set_all_led_color: None,
                set_single_led_color: None,
                get_all_led_color: None,
                get_single_led_color: None,
            },
            e2: {
                get_onie_e2_path: None,
                get_device_airflow: None,
            },
            cpu: {
                get_cpu_reset_num: None,
                get_cpu_reboot_cause: None,
            }
        }
    )


if __name__ == '__main__':
    run_cli_man()
