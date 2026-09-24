#!/usr/bin/python

from sonic_platform.chassis import Chassis


def main():
    print("------------------------")
    print("Chassis eeprom Unit Test")
    print("------------------------")

    chassis = Chassis()

    eeprom = chassis.get_eeprom()

    print("    Model: {}, Service Tag: {}".format(eeprom.modelstr(),
                                             eeprom. service_tag_str()))
    print("    Part#: {}, Serial#: {}".format(eeprom.part_number_str(),
                                              eeprom.serial_number_str()))
    print("    Base MAC: {}".format(eeprom.base_mac_addr()))

    return


if __name__ == '__main__':
    main()
