#!/usr/bin/python

from sonic_platform.chassis import Chassis


def main():
    print("---------------------")
    print("Chassis PSU Unit Test")
    print("---------------------")

    chassis = Chassis()

    for psu in chassis.get_all_psus():
        if not psu.get_presence():
            print("    Name: {} not present".format(psu.get_name()))
        else:
            print("    Name:", psu.get_name())
            print("        Presence: {}, Status: {}, LED: {}".format(psu.get_presence(),
                                                                     psu.get_status(),
                                                                     psu.get_status_led()))
            print("        Model: {}, Serial#: {}, Part#: {}".format(psu.get_model(),
                                                                     psu.get_serial(),
                                                                     psu.get_part_number()))
            try:
                current = psu.get_current()
            except NotImplementedError:
                current = "NA"
            try:
                power = psu.get_power()
            except NotImplementedError:
                power = "NA"

            print("        Voltage: {}, Current: {}, Power: {}\n".format(psu.get_voltage(),
                                                                         current,
                                                                         power))
    return


if __name__ == '__main__':
    main()
