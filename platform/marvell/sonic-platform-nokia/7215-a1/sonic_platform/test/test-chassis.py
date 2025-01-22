#!/usr/bin/env python

try:
    import sonic_platform.platform
    import sonic_platform.chassis
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")


def main():
    print("-----------------")
    print("Chassis Unit Test")
    print("-----------------")

    chassis = sonic_platform.platform.Platform().get_chassis()
    print("  Chassis name: {}".format(chassis.get_name()))

    print("  Chassis presence: {}".format(chassis.get_presence()))

    print("  Chassis model: {}".format(chassis.get_model()))

    print("  Chassis serial: {}".format(chassis.get_serial()))

    print("  Chassis revision: {}".format(chassis.get_revision()))

    print("  Chassis status: {}".format(chassis.get_status()))

    print("  Chassis base_mac: {}".format(chassis.get_base_mac()))

    print("  Chassis reboot cause: {}\n".format(chassis.get_reboot_cause()))

    print("  Chassis watchdog: {}".format(chassis.get_watchdog()))

    print("  Chassis num_components: {}".format(chassis.get_num_components()))

    print("  Chassis all_components: {}\n".format(chassis.get_all_components()))

    print("  Chassis num_modules: {}".format(chassis.get_num_modules()))

    print("  Chassis all_modules: {}\n".format(chassis.get_all_modules()))

    print("  Chassis num_fans: {}".format(chassis.get_num_fans()))

    print("  Chassis all_fans: {}\n".format(chassis.get_all_fans()))

    print("  Chassis num_psus: {}".format(chassis.get_num_psus()))

    print("  Chassis all_psus: {}\n".format(chassis.get_all_psus()))

    print("  Chassis num_thermals: {}".format(chassis.get_num_thermals()))

    print("  Chassis all_thermals: {}\n".format(chassis.get_all_thermals()))

    print("  Chassis num_sfps: {}".format(chassis.get_num_sfps()))

    print("  Chassis all_sfps: {}\n".format(chassis.get_all_sfps()))

    print("  Chassis eeprom: {}".format(chassis.get_eeprom()))

    print("  Chassis system_eeprom_info: {}\n".format(chassis.get_system_eeprom_info()))

    print("  Chassis get_status_led start : {}\n".format(chassis.get_status_led()))
    chassis.set_status_led('amber')
    print("  Chassis get_status_led amber: {}\n".format(chassis.get_status_led()))
    chassis.set_status_led('green')
    print("  Chassis get_status_led green: {}\n".format(chassis.get_status_led()))

    return


if __name__ == '__main__':
    main()
