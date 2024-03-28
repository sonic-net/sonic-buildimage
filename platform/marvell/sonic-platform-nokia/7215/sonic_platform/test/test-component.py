#!/usr/bin/python

from sonic_platform.chassis import Chassis


def main():
    print("---------------------------")
    print("Chassis Component Unit Test")
    print("---------------------------")

    chassis = Chassis()

    for component in chassis.get_all_components():
        print("    Name: {}".format(component.get_name()))
        print("        Description: {}".format(component.get_description()))
        print("        FW version: {}\n".format(component.get_firmware_version()))

    return


if __name__ == '__main__':
    main()
