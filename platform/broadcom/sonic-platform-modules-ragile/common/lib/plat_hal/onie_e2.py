#!/usr/bin/env python3
#######################################################
#
# onie_e2.py
# Python implementation of the Class onie_e2
#
#######################################################
from plat_hal.devicebase import devicebase
from eepromutil.onietlv import onie_tlv


class onie_e2(devicebase):

    def __init__(self, conf=None):
        self._cardid = ""
        self._productname = ""
        self._partnum = ""
        self._serialnum = ""
        self._macbase = ""
        self._manufdate = ""
        self._deviceversion = ""
        self._labelrevision = ""
        self._platformname = ""
        self._onieversion = ""
        self._macsize = ""
        self._manufname = ""
        self._manufcountry = ""
        self._vendorname = ""
        self._diagname = ""
        self._servicetag = ""

        if conf is not None:
            self.name = conf.get('name', None)
            self.e2loc = conf.get('e2loc', None)
            self.e2_path = self.e2loc.get('loc', None)
            self.airflow = conf.get('airflow', "intake")

    @property
    def cardid(self):
        return self._cardid

    @property
    def productname(self):
        return self._productname

    @property
    def partnum(self):
        return self._partnum

    @property
    def serialnum(self):
        return self._serialnum

    @property
    def macbase(self):
        return self._macbase

    @property
    def manufdate(self):
        return self._manufdate

    @property
    def deviceversion(self):
        return self._deviceversion

    @property
    def labelrevision(self):
        return self._labelrevision

    @property
    def platformname(self):
        return self._platformname

    @property
    def onieversion(self):
        return self._onieversion

    @property
    def macsize(self):
        return self._macsize

    @property
    def manufname(self):
        return self._manufname

    @property
    def manufcountry(self):
        return self._manufcountry

    @property
    def vendorname(self):
        return self._vendorname

    @property
    def diagname(self):
        return self._diagname

    @property
    def servicetag(self):
        return self._servicetag

    def get_onie_e2_info(self):
        try:
            eeprom = self.get_eeprom_info(self.e2loc)
            if eeprom is None:
                raise Exception("%s: value is none" % self.name)
            onietlv = onie_tlv()
            onietlv.decode(eeprom)
            self._cardid = onietlv.cardid
            self._productname = onietlv.productname
            self._partnum = onietlv.partnum
            self._serialnum = onietlv.serialnum
            self._macbase = onietlv.macbase
            self._manufdate = onietlv.manufdate
            self._deviceversion = onietlv.deviceversion
            self._labelrevision = onietlv.labelrevision
            self._platformname = onietlv.platformname
            self._onieversion = onietlv.onieversion
            self._macsize = onietlv.macsize
            self._manufname = onietlv.manufname
            self._manufcountry = onietlv.manufcountry
            self._vendorname = onietlv.vendorname
            self._diagname = onietlv.diagname
            self._servicetag = onietlv.servicetag
        except Exception:
            return False
        return True
