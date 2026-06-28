#!/usr/bin/env python

#############################################################################
# Celestica
#
# Component contains an implementation of SONiC Platform Base API and
# provides the components firmware management function
#
#############################################################################

try:
    from sonic_platform_base.component_base import ComponentBase
    from common import Common
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")


class Component(ComponentBase):
    """Platform-specific Component class"""

    def __init__(self, component_index, conf):
        ComponentBase.__init__(self)

        self._index = component_index
        self._config = conf
        self._api_common = Common(self._config)

    def get_name(self):
        """
        Retrieves the name of the component
        Returns:
            A string containing the name of the component
        """
        default = 'N/A'
        config = self._config["get_name"]
        return self._api_common.get_output(self._index, config, default)

    def get_description(self):
        """
        Retrieves the description of the component
        Returns:
            A string containing the description of the component
        """
        default = 'N/A'
        config = self._config["get_description"]
        return self._api_common.get_output(self._index, config, default)

    def get_firmware_version(self):
        """
        Retrieves the firmware version of the component
        Note: the firmware version will be read from HW
        Returns:
            A string containing the firmware version of the component
        """
        default = 'N/A'
        config = self._config["get_firmware_version"]
        return self._api_common.get_output(self._index, config, default)
