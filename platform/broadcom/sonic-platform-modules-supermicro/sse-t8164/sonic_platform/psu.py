#!/usr/bin/env python
#


try:
    import time
    from sonic_platform_pddf_base.pddf_psu import PddfPsu
except ImportError as e:
    raise ImportError (str(e) + "- required module not found")


class Psu(PddfPsu):
    """PDDF Platform-Specific PSU class"""

    def __init__(self, index, pddf_data=None, pddf_plugin_data=None):
        PddfPsu.__init__(self, index, pddf_data, pddf_plugin_data)

    # Provide the functions/variables below for which implementation is to be overwritten
    def get_type(self):
        model = self.get_model()
        if model:
            parts = model.split('-')
            if len(parts) > 1 and parts[1].endswith('D'):
                return 'DC'
        return 'AC'

    def get_capacity(self):
        return self.get_maximum_supplied_power()
    def get_status(self):
        if super().get_status():
            # Ensure get_model is ready for psud to update
            for _ in range(10):
                if self.get_model() != 'N/A':
                    return True
                time.sleep(0.5)
            return super().get_status()
        else:
            return False
