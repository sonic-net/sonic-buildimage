try:
    import sonic_platform.platform
    import sonic_platform.chassis
    from sonic_psu.psu_base import PsuBase
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")


class PsuUtil(PsuBase):
    """Platform-specific PSUutil class"""

    def __init__(self):
        PsuBase.__init__(self)
        self.chassis = sonic_platform.platform.Platform().get_chassis()

    def get_num_psus(self):
        MAX_PSUS = 2
        return MAX_PSUS

    def get_psu_status(self, index):
        # print " psuUtil redirect to PMON 2.0  "
        if self.chassis is not None:
            return self.chassis.get_psu(index-1).get_status()
        else:
            return False

    def get_psu_presence(self, index):
        # print " psuUtil redirect to PMON 2.0  "
        if self.chassis is not None:
            return self.chassis.get_psu(index-1).get_presence()
        else:
            return False
