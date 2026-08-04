try:
    import subprocess
    from sonic_platform_pddf_base.pddf_component import PddfComponent
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

class Component(PddfComponent):

    def __init__(self, component_index, pddf_data=None, pddf_plugin_data=None):
        PddfComponent.__init__(self, component_index, pddf_data, pddf_plugin_data)

    # Provide the functions/variables below for which implementation is to be overwritten
