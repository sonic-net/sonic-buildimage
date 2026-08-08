from sonic_platform_base.sonic_ssd.ssd_generic import SsdUtil as SsdUtilGeneric

class SsdUtil(SsdUtilGeneric):
    def parse_innodisk_info(self):
        super().parse_innodisk_info()
        if self.vendor_ssd_info:
            # fix too lazy pattern 'Health:\s*(.+?)%?'
            self.health = self._parse_re('Health:\s*(.+?)%', self.vendor_ssd_info)
