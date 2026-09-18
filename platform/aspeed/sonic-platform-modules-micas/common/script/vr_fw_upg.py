#!/usr/bin/env python_nos
# -*- coding: UTF-8 -*-
import os
import sys
import click
from renesas_fw_upg import do_renesas_fw_upg
from xdpe122_fw_upg import do_xdpe122_fw_upg
from xdpe1x2xx_fw_upg import do_xdpe1x2xx_fw_upg
from platform_util import AliasedGroup, CONTEXT_SETTINGS

VR_DEVICE_NAME_PATH = "/sys/bus/i2c/devices/%d-%04x/name"

UPGRADE_RENESAS = 1
UPGRADE_XDPE12284 = 2
UPGRADE_XDPE132G5C_PMBUS = 3

VR_UPGRADE_MAP = {
    "wb_raa228228": UPGRADE_RENESAS,
    "wb_isl69260": UPGRADE_RENESAS,
    "wb_xdpe12284": UPGRADE_XDPE12284,
    "wb_xdpe12254": UPGRADE_XDPE12284,
    "wb_xdpe1a2g5b_pmbus": UPGRADE_XDPE132G5C_PMBUS,
    "wb_xdpe19284c_pmbus": UPGRADE_XDPE132G5C_PMBUS,
    "wb_xdpe192c4b_pmbus": UPGRADE_XDPE132G5C_PMBUS,
}


def get_vr_chip_type(bus, addr):
    path = VR_DEVICE_NAME_PATH % (bus, addr)
    if not os.path.isfile(path):
        msg = "%s not found" % path
        return False, msg
    try:
        with open(path, 'r') as fd:
            retval = fd.read()
        vr_dev_name = retval.strip()
        vr_chip_type = VR_UPGRADE_MAP.get(vr_dev_name)
        if vr_chip_type is None:
            return False, "Unknown vr_dev_name: %s" % vr_dev_name
        return True, vr_chip_type
    except Exception as e:
        msg = "get_vr_chip_type raise exception, errmsg: %s" % str(e)
        return False, msg


def do_fw_upg(file, bus, addr, chip_type):
    if chip_type is None:
        ret, chip_type = get_vr_chip_type(bus, addr)
        if ret is False:
            return ret, chip_type

    if chip_type == UPGRADE_RENESAS:
        ret, log = do_renesas_fw_upg(file, bus, addr)
        return ret, log

    if chip_type == UPGRADE_XDPE12284:
        ret, log = do_xdpe122_fw_upg(file, bus, addr)
        return ret, log

    if chip_type == UPGRADE_XDPE132G5C_PMBUS:
        ret, log = do_xdpe1x2xx_fw_upg(file, bus, addr)
        return ret, log

    return False, "Unsupport chip_type: %s" % chip_type


def do_upgrade(firmware_file, bus, addr, chip_type=None):
    print("+================================+")
    print("|  Doing upgrade, please wait... |")
    ret, log = do_fw_upg(firmware_file, bus, addr, chip_type)
    if ret is True:
        print("|       upgrade succeeded!       |")
        print("+================================+")
        sys.exit(0)
    else:
        print("|        upgrade failed!         |")
        print("+================================+")
        print("FAILED REASON:")
        print("%s" % log)
        sys.exit(1)


class IntOrHexParamType(click.ParamType):
    name = "int_or_hex"
    def convert(self, value, param, ctx):
        try:
            # Automatically detect base: decimal, hex (0x..), octal (0o..), binary (0b..)
            return int(value, 0)
        except ValueError:
            self.fail(f"{value!r} is not a valid integer (supports decimal or 0x hex)", param, ctx)

INT_OR_HEX = IntOrHexParamType()


@click.group(cls=AliasedGroup, context_settings=CONTEXT_SETTINGS)
def main():
    '''Upgrade renesas firmware script'''


# xdpe1x2xx firmware upgrade
@main.command()
@click.argument('firmware_file', required=True)
@click.argument('bus', type=INT_OR_HEX, required=True)
@click.argument('addr', type=INT_OR_HEX, required=True)
def xdpe1x2xx(firmware_file, bus, addr):
    '''xdpe1x2xx firmware upgrade'''
    do_upgrade(firmware_file, bus, addr, UPGRADE_XDPE132G5C_PMBUS)


# xdpe122 firmware upgrade
@main.command()
@click.argument('firmware_file', required=True)
@click.argument('bus', type=INT_OR_HEX, required=True)
@click.argument('addr', type=INT_OR_HEX, required=True)
def xdpe122(firmware_file, bus, addr):
    '''xdpe122 firmware upgrade'''
    do_upgrade(firmware_file, bus, addr, UPGRADE_XDPE12284)


# renesas firmware upgrade
@main.command()
@click.argument('firmware_file', required=True)
@click.argument('bus', type=INT_OR_HEX, required=True)
@click.argument('addr', type=INT_OR_HEX, required=True)
def renesas(firmware_file, bus, addr):
    '''renesas firmware upgrade'''
    do_upgrade(firmware_file, bus, addr, UPGRADE_RENESAS)


# common vr firmware upgrade
@main.command()
@click.argument('firmware_file', required=True)
@click.argument('bus', type=INT_OR_HEX, required=True)
@click.argument('addr', type=INT_OR_HEX, required=True)
def upgrade(firmware_file, bus, addr):
    '''common vr firmware upgrade'''
    do_upgrade(firmware_file, bus, addr)


if __name__ == "__main__":
    main()