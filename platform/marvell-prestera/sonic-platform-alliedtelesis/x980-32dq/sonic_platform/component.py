try:
    from sonic_platform_pddf_base.pddf_component import PddfComponent
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")


class Component(PddfComponent):
    """Platform-specific Component class. Used for programmables"""

    DEVICE_TYPE = "component"

    def __init__(self, idx, pddf_data, pddf_plugin_data=None):
        PddfComponent.__init__(self, idx, pddf_data, pddf_plugin_data)
