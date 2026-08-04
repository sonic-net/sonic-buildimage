#!/usr/bin/env python

class Common():
    INFO_PREFIX     = "/usr/share/sonic/platform"
    I2C_PREFIX      = "/sys/bus/i2c/devices"

class FanConst():
    #fan vpd info
    FAN_VPD_CHANNEL= 1
    FAN_VPD_ADDR_BASE=0x52
    #fru status
    TLV_PRODUCT_INFO_OFFSET_IDX=5
    TLV_PRODUCT_INFO_AREA_START=3
    TLV_ATTR_TYPE_SERIAL=5
    TLV_ATTR_TYPE_MODEL=2    

    PSU_FAN_START_INDEX = 5
    FAN_TYPE_LIST=["0:Normal Type","1:REVERSAL Type","2:UNPLUGGED","3:UNPLUGGED"] #defined in inv_cpld

class PsuConst():
    PSU_TYPE_LIST=["0:unpowered","1:normal","2:not installed","3:not installed"]  #defined in inv_cpld
    PSU_I2C_ADDR=["2-005a","2-005b"]