#!/usr/bin/python

from sonic_platform.chassis import Chassis


def main():
    print("---------------------")
    print("Chassis Fan Unit Test")
    print("---------------------")

    chassis = Chassis()

    for fan in chassis.get_all_fans():
        if not fan.get_presence():
            print("    Name: {} not present".format(fan.get_name()))
        else:
            print("    Name:", fan.get_name())
            print("        Presence: {}, Status: {}, LED: {}".format(fan.get_presence(),
                                                                     fan.get_status(),
                                                                     fan.get_status_led()))
            print("        Model: {}, Serial#: {}".format(fan.get_model(),
                                                          fan.get_serial()))
            print("        Part#: {}, Service Tag: {}".format(fan.get_part_number(),
                                                              fan.get_service_tag()))
            print("        Direction: {}, Speed: {}%, Target Speed: {}%\n".format(fan.get_direction(),
                                                                                    str(fan.get_speed()),
                                                                                    str(fan.get_target_speed())))
    return


if __name__ == '__main__':
    main()
