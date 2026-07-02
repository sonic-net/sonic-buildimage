#!/usr/bin/python

from sonic_platform.chassis import Chassis

def main():
    print("-------------------------")
    print("Chassis Thermal Unit Test")
    print("-------------------------")

    chassis = Chassis()

    for thermal in chassis.get_all_thermals():
        if not thermal.get_presence():
            print("    Name: {} not present".format(thermal.get_name()))
        else:
            print("    Name:", thermal.get_name())
            print("        Presence: {}, Status: {}".format(thermal.get_presence(),
                                                            thermal.get_status()))
            print("        Model: {}, Serial#: {}".format(thermal.get_model(),
                                                          thermal.get_serial()))
            print("        Temperature(C): {}".format(thermal.get_temperature()))

            try:
                low_thresh = thermal.get_low_threshold()
            except NotImplementedError:
                low_thresh = "NA"
            try:
                high_thresh = thermal.get_high_threshold()
            except NotImplementedError:
                high_thresh = "NA"

            print("        Low Threshold(C): {}, High Threshold(C): {}".format(low_thresh,
                                                                               high_thresh))

            try:
                crit_low_thresh = thermal.get_low_critical_threshold()
            except NotImplementedError:
                crit_low_thresh = "NA"
            try:
                crit_high_thresh = thermal.get_high_critical_threshold()
            except NotImplementedError:
                crit_high_thresh = "NA"

            print("        Crit Low Threshold(C): {}, Crit High Threshold(C): {}\n".format(crit_low_thresh,
                                                                                           crit_high_thresh))
    return


if __name__ == '__main__':
    main()
