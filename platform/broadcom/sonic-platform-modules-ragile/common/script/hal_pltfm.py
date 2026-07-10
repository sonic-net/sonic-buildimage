#!/usr/bin/env python3
# -*- coding: UTF-8 -*-
import inspect
import sys
import json
import time
from plat_hal.interface import interface


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
    r'''set_psu_fan_speed_pwm'''
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


def get_info_all():
    r'''psu  get_info_all '''
    print("=================get_info_all======================")
    print(json.dumps(int_case.get_psu_info_all(), ensure_ascii=False, indent=4))


def fan_get_total_number():
    print("=================get_info_all======================")
    print(json.dumps(int_case.get_fan_total_number(), ensure_ascii=False, indent=4))


def fan_get_rotor_number():
    r'''fan_get_rotor_number'''
    print("=================fan_get_rotor_number======================")
    fans = int_case.get_fans()
    for fan_item in fans:
        print(fan_item.name, end=' ')
        print(int_case.get_fan_rotor_number(fan_item.name))


def fan_get_speed():
    r'''fan_get_speed'''
    print("=================fan_get_speed======================")
    fans = int_case.get_fans()
    for fan_item in fans:
        rotors = fan_item.rotor_list
        for rotor in rotors:
            index = rotors.index(rotor)
            print("%s rotor%d" % (fan_item.name, index + 1), end='  ')
            print(int_case.get_fan_speed(fan_item.name, index + 1))


def fan_get_speed_pwm():
    r'''fan_get_speed_pwm'''
    print("=================fan_get_speed_pwm======================")
    fans = int_case.get_fans()
    for fan_item in fans:
        rotors = fan_item.rotor_list
        for rotor in rotors:
            index = rotors.index(rotor)
            print("%s rotor%d" % (fan_item.name, index + 1), end='  ')
            print(int_case.get_fan_speed_pwm(fan_item.name, index + 1))


def fan_set_speed_pwm(pwm):
    r'''fan_set_speed_pwm'''
    print("=================fan_set_speed_pwm======================")
    fans = int_case.get_fans()
    for fan_item in fans:
        rotors = fan_item.rotor_list
        for rotor in rotors:
            index = rotors.index(rotor)
            print("%s %s" % (fan_item.name, rotor.name), end='  ')
            val = int_case.set_fan_speed_pwm(fan_item.name, index + 1, pwm)
            print(val)


def fan_get_watchdog_status():
    r'''fan_get_watchdog_status'''
    print("=================fan_get_watchdog_status======================")
    print(int_case.get_fan_watchdog_status())


def fan_enable_watchdog():
    r'''fan_enable_watchdog'''
    print("=================fan_enable_watchdog======================")
    print('enable', int_case.enable_fan_watchdog())


def fan_disable_watchdog():
    r'''fan_disable_watchdog'''
    print("=================fan_disable_watchdog======================")
    print('disable', int_case.enable_fan_watchdog(enable=False))


def fan_get_speed1():
    r'''fan_get_speed'''
    print("=================fan_get_speed======================")
    fans = int_case.get_fans()
    for fan_item in fans:
        rotors = fan_item.rotor_list
        for rotor in rotors:
            print("%s %s" % (fan_item.name, rotor.name), end='  ')
            print(int_case.get_fan_speed(fan_item.name, rotor.name))


def fan_feed_watchdog():
    r'''fan_feed_watchdog'''
    print("=================fan_feed_watchdog======================")
    fan_get_speed()
    print(int_case.feed_fan_watchdog())
    time.sleep(2)
    fan_get_speed()


def fan_set_led(color):
    r'''fan_set_led'''
    print("=================fan_set_led======================")
    fans = int_case.get_fans()
    for fan_item in fans:
        print("%s" % fan_item.name)
        print(color, int_case.set_fan_led(fan_item.name, color))

def fan_get_led():
    r'''fan_get_led'''
    print("=================fan_get_led======================")
    fans = int_case.get_fans()
    for fan_item in fans:
        print("%s" % fan_item.name)
        print(int_case.get_fan_led(fan_item.name))


def fan_get_presence():
    r'''fan_get_presence'''
    print("=================fan_get_presence======================")
    fans = int_case.get_fans()
    for fan_item in fans:
        print("%s" % fan_item.name)
        print(int_case.get_fan_presence(fan_item.name))


def fan_get_fru_info():
    r'''fan_get_fru_info'''
    print("=================fan_get_fru_info======================")
    fans = int_case.get_fans()
    for fan_item in fans:
        print("%s" % fan_item.name)
        print(json.dumps(int_case.get_fan_info(fan_item.name), ensure_ascii=False, indent=4))


def fan_get_status():
    r'''fan_get_status'''
    print("=================fan_get_status======================")
    fans = int_case.get_fans()
    for fan_item in fans:
        print("%s" % fan_item.name)
        print(json.dumps(int_case.get_fan_status(fan_item.name), ensure_ascii=False, indent=4))


def fan_get_info_all():
    r'''fan_get_info_all'''
    print("=================fan_get_info_all======================")
    print(json.dumps(int_case.get_fan_info_all(), ensure_ascii=False, indent=4))


def get_sensor_info():
    r'''get_sensor_info'''
    print("=================get_sensor_info======================")
    print(json.dumps(int_case.get_sensor_info(), ensure_ascii=False, indent=4))


def get_dcdc_all_info():
    r'''get_dcdc_all_info'''
    print("=================get_dcdc_all_info======================")
    print(json.dumps(int_case.get_dcdc_all_info(), ensure_ascii=False, indent=4))


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
    r'''get_single_led_color'''
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
        print("id: %s, name: %s, API name: %s, value: %s" % (temp.temp_id, temp.name, temp.api_name, temp.Value))

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
                get_info_all: None
            },
            fan: {
                fan_get_total_number: None,
                fan_get_rotor_number: None,
                fan_get_speed: None,
                fan_get_speed_pwm: None,
                fan_set_speed_pwm: None,
                fan_get_watchdog_status: None,
                fan_enable_watchdog: None,
                fan_disable_watchdog: None,
                fan_feed_watchdog: None,
                fan_set_led: None,
                fan_get_led: None,
                fan_get_presence: None,
                fan_get_fru_info: None,
                fan_get_status: None,
                fan_get_info_all: None
            },
            sensor: {
                get_sensor_info: None
            },
            dcdc: {
                get_dcdc_all_info: None
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
            temps: {
                get_temps_sensor: None,
            },
            cpu: {
                get_cpu_reset_num: None,
                get_cpu_reboot_cause: None,
            }
        }
    )


if __name__ == '__main__':
    run_cli_man()
