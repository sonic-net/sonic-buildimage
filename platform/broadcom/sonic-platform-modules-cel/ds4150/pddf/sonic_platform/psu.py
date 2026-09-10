try:
    from sonic_platform_pddf_base.pddf_psu import PddfPsu
except ImportError as e:
    raise ImportError (str(e) + "- required module not found")

class Psu(PddfPsu):
    """PDDF Platform-Specific PSU class"""

    # Map of PSU models to their power type
    PSU_MODEL_TYPE_MAP = {
        "TDPS1500AB25": "DC",
        "TDPS1500AB26": "DC"
    }

    def __init__(self, index, pddf_data=None, pddf_plugin_data=None):
        PddfPsu.__init__(self, index, pddf_data, pddf_plugin_data)
        # Initializing the cache
        self._psu_type = 'N/A'
        self._presence = False

    # Provide the functions/variables below for which implementation is to be overwritten
    def get_capacity(self):
        return 550

    def get_status_led(self):
        """
        Gets the state of the PSU status LED

        Returns:
            A string, one of the predefined STATUS_LED_COLOR_* strings above
        """
        # PSU LED is controlled by the PSU firmware, so soft
        # simulating the LED in a generic way based on the PSU status
        if self.get_presence():
            if self.get_powergood_status():
                return self.STATUS_LED_COLOR_GREEN
            else:
                return self.STATUS_LED_COLOR_AMBER

        return self.STATUS_LED_COLOR_OFF

    def get_presence(self):
        """
        Retrieves presence and resets cache if a new PSU is inserted.
        """
        presence = PddfPsu.get_presence(self)

        if presence != self._presence:
            if not presence:
                self._psu_type = 'N/A'
            self._presence = presence

        return self._presence

    def get_model(self):
        """
        Retrieves the model from the base class and caches the type.
        """
        # Call the base class get_model to get the string
        model_name = PddfPsu.get_model(self)
        if model_name:
            self._psu_type = self.PSU_MODEL_TYPE_MAP.get(model_name, 'AC')
        else:
            self._psu_type = 'N/A'

        return model_name

    def get_type(self):
        """
        Returns the cached Power Type of PSU

        """
        if self._psu_type == 'N/A':
            self.get_model()

        return self._psu_type
