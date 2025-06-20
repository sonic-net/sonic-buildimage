#######################################################################
# Asterfusion CX-N Devices Components                                 #
#                                                                     #
# Component contains an implementation of SONiC Platform Base API and #
# provides the components firmware management function                #
#                                                                     #
#######################################################################

try:
    from .constants import *
    from .helper import Helper
    from .logger import Logger

    from sonic_platform_base.component_base import ComponentBase
except ImportError as err:
    raise ImportError(str(err) + "- required module not found")


class Component(ComponentBase):
    """Platform-specific Component class"""

    def __init__(self, component_index, hwsku, asic):
        # type: (int, str, str) -> None
        self._helper = Helper()
        self._logger = Logger()
        ComponentBase.__init__(self)
        self._component_index = component_index
        self._hwsku = hwsku
        self._asic = asic
        self._init_component_info()

    def _init_component_info(self):
        # type: () -> None
        component_info = COMPONENT_INFO.get(self._hwsku, {}).get(self._asic, None)
        if component_info is None:
            self._logger.log_fatal("Failed in initializing component info")
            raise RuntimeError("failed in initializing component info")
        if self._component_index >= len(component_info):
            self._logger.log_fatal("Failed in initializing component info")
            raise RuntimeError("failed in initializing component info")
        self._component_info = component_info[self._component_index]
        # Static information
        self._component_name = self._component_info.get("name", NOT_AVAILABLE)
        self._component_description = self._component_info.get("desc", NOT_AVAILABLE)
        self._component_version = self._helper.get_sysfs_content(
            self._component_info, "version"
        )
        self._logger.log_info("Initialized info for <{}>".format(self._component_name))

    def get_name(self):
        # type: () -> str
        """
        Retrieves the name of the component

        Returns:
            A string containing the name of the component
        """
        return self._component_name

    def get_description(self):
        # type: () -> str
        """
        Retrieves the description of the component

        Returns:
            A string containing the description of the component
        """
        return self._component_description

    def get_firmware_version(self):
        # type: () -> str
        """
        Retrieves the firmware version of the component

        Note: the firmware version will be read from HW

        Returns:
            A string containing the firmware version of the component
        """
        return self._component_version
