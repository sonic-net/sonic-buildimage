#!/usr/bin/python

from sonic_platform.chassis import Chassis


def main():
    print("---------------------")
    print("Chassis Watchdog Test")
    print("---------------------")

    chassis = Chassis()

    watchdog = chassis.get_watchdog()

    print("    Armed: {}".format(watchdog.is_armed()))
    print("    Time Left: {}".format(watchdog.get_remaining_time()))

    return


if __name__ == '__main__':
    main()
