# error code
from enum import IntEnum, unique


NO_ERR = 0
IO_ERR = 1
TEST_FAIL_ERR = 2
NORUN_ERR = 3
NULL_ERR = 4
ABSENT_ERR = 5
OUT_THRD_ERR = 6
FCS_ERR = 7
DROP_ERR = 8
UNMATCH_ERR = 9
OTHER_ERR = 99


# error message
ERR_MSG = {
    NO_ERR: "no error",
    IO_ERR: "i/o error",
    TEST_FAIL_ERR: "test failed",
    NORUN_ERR: "not running",
    NULL_ERR: "empty content",
    ABSENT_ERR: "device not present",
    OUT_THRD_ERR: "value is out of threshold",
    FCS_ERR: "fcs error",
    DROP_ERR: "drop packets",
    UNMATCH_ERR: "content is not matched",
    OTHER_ERR: "other error"
}


@unique
class E(IntEnum):
    OK = 0                                # Pass
    EFAIL = 1                             # Fail
    EIO = 2                               # I/O error
    EEXIST = 3                            # files not exists
    ESUPPORT = 4                          # unsupported ops

    EUSB1000 = 1000                       # usb not found
    EUSB1001 = 1001                       # usb mount fail
    EUSB1002 = 1002                       # usb read error
    EUSB1003 = 1003                       # usb write error
    EUSB1004 = 1004                       # usb speed not match
    EUSB1005 = 1005                       # usb net card ping test drop packet
    EUSB1006 = 1006                       # usb stress fail

    ESSD2001 = 2001                       # ssd vendor info not match
    ESSD2002 = 2002                       # ssd read fail
    ESSD2003 = 2003                       # ssd write fail
    ESSD2004 = 2004                       # ssd smart error

    ECPU3001 = 3001                       # cpu vendor info not matched
    ECPU3002 = 3002                       # cpu stress test failed
    ECPU3003 = 3003                       # cpu cores info not matched
    ECPU3004 = 3004                       # cpu MHz info not match

    EMEM4001 = 4001                       # memory vendor info not matched
    EMEM4002 = 4002                       # memtester fail
    EMEM4003 = 4003                       # memory ecc happened
    EMEM4004 = 4004                       # free memory is too small
    EMEM4005 = 4005                       # memmory stress test eeror
    
    EPSU5000 = 5000                       # PSU error
    EPSU5001 = 5001                       # PSU vendor info not matched
    EPSU5002 = 5002                       # PSU input voltage out of threshold
    EPSU5003 = 5003                       # PSU input current out of threshold
    EPSU5004 = 5004                       # PSU ouput voltage out of threshold
    EPSU5005 = 5005                       # PSU output current out of threshold
    EPSU5006 = 5006                       # PSU absent
    EPSU5007 = 5007                       # PSU status invalid
    EPSU5008 = 5008                       # PSU input power out of threshold
    EPSU5009 = 5009                       # PSU output power out of threshold

    EFAN6000 = 6000                       # fan error
    EFAN6001 = 6001                       # fan vendor info not matched
    EFAN6002 = 6002                       # fan speed out of threshold
    EFAN6003 = 6003                       # fan stop or not running
    EFAN6004 = 6004                       # fan absent
    EFAN6005 = 6005                       # fan control failed

    ESSR7001 = 7001                       # sensor count not matched
    ESSR7002 = 7002                       # sensor access failed
    ESSR7003 = 7003                       # sensor value out of threshold

    EIIC8001 = 8001                       # I2C devices absent
    EIIC8002 = 8002                       # I2C read/write access failed
    EIIC8003 = 8003                       # I2C read value not match writed

    EPCI9001 = 9001                       # PCIe devices absent
    EPCI9002 = 9002                       # FPGA read/write fail
    EPCI9003 = 9003                       # FPGA read value not match writed
    EPCI9004 = 9004                       # PCIe speed not matched
    EPCI9005 = 9005                       # PCIe width not matched
    EPCI9006 = 9006                       # PCIe ue/ce happened
    EPCI9007 = 9007                       # FPGA number can't cat
    EPCI9008 = 9008                       # FPGA number can't cat

    ELPC10001 = 10001                     # LPC devices absent
    ELPC10002 = 10002                     # LPC access BMC fail
    ELPC10003 = 10003                     # LPC read value not match writed

    EMGMT11001 = 11001                    # mgmtport status invalid
    EMGMT11002 = 11002                    # cpu ping server drop packet
    EMGMT11003 = 11003                    # cpu ping bmc drop packet
    EMGMT11004 = 11004                    # bmc ping server drop packet
    EMGMT11005 = 11005                    # mgmtport firmware not matched
    EMGMT11006 = 11006                    # cpu MAC invalid
    EMGMT11007 = 11007                    # bmc MAC invalid
    EMGMT11008 = 11008                    # MAC check error

    ERTC12001 = 12001                     # rtc access fail
    ERTC12002 = 12002                     # rtc timing uncertainty

    EGPIO13001 = 13001                    # gpio pin access fail
    EGPIO13002 = 13002                    # jtag upgrade fail

    ELED14001 = 14001                     # SYS LED fault
    ELED14002 = 14002                     # Power LED fault
    ELED14003 = 14003                     # FAN LED fault
    ELED14004 = 14004                     # Location LED fault
    ELED14005 = 14005                     # BMC LED fault
    ELED14006 = 14006                     # Port LEDs fault
    ELED14007 = 14007                     # fan-module LED fault
    ELED14008 = 14008                     # PSU-module LED fault

    EUART15001 = 15001                    # console baud_rate not matched
    EUART15002 = 15002                    # cpu console output fault
    EUART15003 = 15003                    # bmc console output fault
    EUART15004 = 15004                    # bmc sol fault

    EFRU16001 = 16001                     # fru program fail
    EFRU16002 = 16002                     # fru access fail
    EFRU16003 = 16003                     # fru content not matched

    EFW17001 = 17001                      # firmware version get failed
    EFW17002 = 17002                      # firmware version not matched
    EFW17003 = 17003                      # firmware upgrade failed
    EFW17004 = 17004                      # master/slave switch failed
    EFW17005 = 17005                      # master flash boot failed
    EFW17006 = 17006                      # slave flash boot failed
    EFW17007 = 17007                      # get config firmware version failed

    ESFP18001 = 18001                     # sfp absent
    ESFP18002 = 18002                     # sfp tx fault
    ESFP18003 = 18003                     # sfp rx lose
    ESFP18004 = 18004                     # sfp tx disable
    ESFP18005 = 18005                     # sfp under low power mode
    ESFP18006 = 18006                     # sfp under reset
    ESFP18007 = 18007                     # sfp present changed
    ESFP18008 = 18008                     # sfp eeprom access failed
    ESFP18009 = 18009                     # sfp temperature over threshold
    ESFP18010 = 18010                     # sfp present signal not detected
    ESFP18011 = 18011                     # sfp tx_disable signal not detected
    ESFP18012 = 18012                     # sfp tx_fault signal not detected
    ESFP18013 = 18013                     # sfp rx_lose signal not detected
    ESFP18014 = 18014                     # qsfp interrupt signal not detected
    ESFP18015 = 18015                     # qsfp lpmode signal not detected
    ESFP18016 = 18016                     # qsfp reset signal not detected

    EPORT19001 = 19001                    # port link down
    EPORT19002 = 19002                    # sdk init failed
    EPORT19003 = 19003                    # sdk exit failed
    EPORT19004 = 19004                    # vlan config failed
    EPORT19005 = 19005                    # tx send packet failed
    EPORT19006 = 19006                    # port speed not matched >90%
    EPORT19007 = 19007                    # fcs error
    EPORT19008 = 19008                    # drop packet

    EPHY20001 = 20001                     # external phy register access failed

    EBMC21001 = 21001                     # BMC cpu test failed
    EBMC21002 = 21002                     # BMC memory test failed
    EBMC21003 = 21003                     # BMC emmc test failed
    EBMC21004 = 21004                     # BMC i2c test failed
    EBMC21005 = 21005                     # BMC rtc test failed
    EBMC21006 = 21006                     # BMC gpio test failed
    EBMC21007 = 21007                     # BMC spi test failed
    EBMC21008 = 21008                     # BMC mdio test failed

    ESPI22001 = 22001                     # spi bus devices access failed

    ESW23001 = 23001                      # switch chip tcam fault

    EKR24001 = 24001                      # kr not recognize
    EKR24002 = 24002                      # kr link down
    EKR24003 = 24003                      # kr drop packet

    EDISK25001 = 25001                    # disk capacity not enough
    EDISK25002 = 25002                    # disk size not enough

    ETIMEZONE26001 = 26001                # time zone not match

    EFWVERSION27001 = 27001               # bios version not match
    EFWVERSION27002 = 27002               # pit version not match
    # sonic node running version is lower than sonic node baseline version.
    EFWVERSION27003 = 27003
    # pit node running version is lower than sonic node baseline version.
    EFWVERSION27004 = 27004

    ENET28001 = 28001  # link speed not match
    ENET28002 = 28002  # ping test loss packet

    EFWBMCUPGRADE29001 = 29001  # bmc upgrade [slave] fail
    EFWBMCUPGRADE29002 = 29002  # bmc upgrade [master] fail
    EFWBMCUPGRADE29003 = 29003  # bmc set boot flash to [slave] fail
    EFWBMCUPGRADE29004 = 29004  # bmc set boot flash to [master] fail
    EFWBMCUPGRADE29005 = 29005  # bmc version not match
    EFWBMCUPGRADE29006 = 29006  # bmc flash match
    EFWBMCUPGRADE29007 = 29007  # get bmc version fail
    EFWBMCUPGRADE29008 = 29008  # get bmc flash fail
    EFWBMCUPGRADE29009 = 29009  # ping bmc fail,time out
    EFWBMCUPGRADE29010 = 29010  # get bmc upgrade file fail

    ESSDSTRESS30001 = 30001  # ssd total size not match.
    ESSDSTRESS30002 = 30002  # fio ssd stress test fail.
    ESSDSTRESS30003 = 30003  # ssd read counts error.

    ENETSTRESS31001 = 31001  # cpu ip not legal.
    ENETSTRESS31002 = 31002  # ip ping test loss packet.
    ENETSTRESS31003 = 31003  # iperf tool not exist.
    ENETSTRESS31004 = 31004  # iperf test loss packet.

    EBIOSME32001 = 32001  # BIOS recovery open fail.
    EBIOSME32002 = 32002  # BIOS recovery close fail.

    ESVID33001 = 33001  # high voltage not match.
    ESVID33002 = 33002  # low voltage not match.
    ESVID33003 = 33003  # standerd voltage not match.

    ESNAKE34001 = 34001  # snake sdk enter demon mode fail.
    ESNAKE34002 = 34002  # snake port status error.
    ESNAKE34003 = 34003  # snake port pkt conut fail.

    ESSH35001 = 35001   # [ remote ] connect fail ,PASSWORD OR HOSTNAME is wrong
    # [ remote ] connect timeout ,PASSWORD OR HOSTNAME is wrong
    ESSH35002 = 35002
    ESSH35003 = 35003   # [ remote ] connect fail ,EOF ERROR

    EUPG36001 = 36001   # save master running version fail or upgrade test firmware fail
    EUPG36002 = 36002   # check master test firmware version fail or upgrade original firmware fail
    EUPG36003 = 36003   # check master original firmware version fail
    EUPG36004 = 36004   # save slave running version fail or upgrade test firmware fail
    EUPG36005 = 36005   # check slave test firmware version fail or upgrade original firmware fail
    EUPG36006 = 36006   # check slave original firmware version fail

    ESYS37001 = 37001   # check sysdiag version fail
    ESYS37002 = 37002   # check system timezone fail

