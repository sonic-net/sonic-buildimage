#!/usr/bin/env python

try:
    from sonic_platform.chassis import Chassis
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")


def main():
    print("---------------------")
    print("Chassis SFP Unit Test")
    print("---------------------")

    chassis = Chassis()

    PORT_START = 1
    PORT_END = 52

    for physical_port in range(PORT_START, PORT_END+1):
        print(" ")
        print(" SFP transceiver tests  PORT = ", physical_port)
        name = chassis.get_sfp(physical_port).get_name()
        print(" SFP transceiver tests  NAME = ", name)

        presence = chassis.get_sfp(physical_port).get_presence()
        print("TEST 1 - sfp presence       [ True ] ", physical_port, presence)

        status = chassis.get_sfp(physical_port).get_reset_status()
        print("TEST 2 - sfp reset status   [ False ] ", physical_port, status)

        txdisable = chassis.get_sfp(physical_port).get_tx_disable()
        print("TEST 3 - sfp tx_disable     [ False ] ", physical_port, txdisable)

        rxlos = chassis.get_sfp(physical_port).get_rx_los()
        print("TEST 4 - sfp status rxlos   [ False ] ", physical_port, rxlos)

        txfault = chassis.get_sfp(physical_port).get_tx_fault()
        print("TEST 5 - sfp status txfault [ False ] ", physical_port, txfault)

        lpmode = chassis.get_sfp(physical_port).get_lpmode()
        print("TEST 6 - sfp enable lpmode  [ False ] ", physical_port, lpmode)

        trans_info = chassis.get_sfp(physical_port).get_transceiver_info()
        print("TEST 7 - sfp transceiver info for port:", physical_port, trans_info)

        trans_status = chassis.get_sfp(physical_port).get_transceiver_bulk_status()
        print("TEST 8 - sfp bulk status for port:", physical_port, trans_status)

        threshold = chassis.get_sfp(physical_port).get_transceiver_threshold_info()
        print("TEST 9 - sfp bulk status for port:", physical_port, threshold)

    return


if __name__ == '__main__':
    main()
