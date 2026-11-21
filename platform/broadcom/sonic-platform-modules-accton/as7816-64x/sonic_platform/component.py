"""Read-only firmware inventory using the standard PDDF component API."""

from sonic_platform_pddf_base.pddf_component import PddfComponent


class Component(PddfComponent):
    def get_presence(self):
        return True

    def get_status(self):
        return self.get_firmware_version() not in ('', 'Unknown')

    def get_position_in_parent(self):
        return self.index + 1

    def is_replaceable(self):
        return False
