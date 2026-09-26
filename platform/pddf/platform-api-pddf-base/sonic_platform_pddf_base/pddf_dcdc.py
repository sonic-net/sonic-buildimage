#############################################################################
# PDDF
#
# PDDF dcdc base class inherited from the base class
#############################################################################


try:
    from sonic_platform_base.dcdc_base import DcdcBase
except ImportError:
    try:
        # DcdcBase is not yet part of sonic-platform-common; platforms that
        # support DC/DC devices ship it inside their sonic_platform package.
        from sonic_platform.dcdc_base import DcdcBase
    except ImportError as e:
        raise ImportError(str(e) + "- required module not found")


class PddfDcdc(DcdcBase):
    """PDDF generic DC/DC class"""

    pddf_obj = {}
    plugin_data = {}

    def __init__(self, index, pddf_data=None, pddf_plugin_data=None):
        DcdcBase.__init__(self)
        if not pddf_data or not pddf_plugin_data:
            raise ValueError('PDDF JSON data error')

        self.pddf_obj = pddf_data
        self.plugin_data = pddf_plugin_data
        self.platform = self.pddf_obj.get_platform()

        self.dcdc_index = index # 0-indexed
        self.dcdc_obj_name = "DCDC{0}".format(self.dcdc_index)
        self.dcdc_obj = self.pddf_obj.data[self.dcdc_obj_name]

    def get_name(self):
        """
        Retrieves the name of the device

        Returns:
            string: The name of the device
        """
        return self.dcdc_obj_name
    

    def get_presence(self):
        """
        Retrieves the presence of the device

        DC/DC converters are onboard, non-removable regulators, so they are
        always present. Platforms with detectable DC/DCs may override this.

        Returns:
            bool: True if device is present, False if not
        """
        return True
