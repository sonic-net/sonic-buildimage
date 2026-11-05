#############################################################################
# Edgecore
#
# Module contains an implementation of SONiC Platform Base API and
# provides the fan status which are available in the platform
# Base PCIe class
#############################################################################

try:
    from sonic_platform_base.sonic_pcie.pcie_common import PcieUtil
    from sonic_py_common import logger
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

SYSLOG_IDENTIFIER = "pcie.py"
log = logger.Logger(SYSLOG_IDENTIFIER)

class Pcie(PcieUtil):
    """Edgecore Platform-specific PCIe class"""

    def __init__(self, platform_path):
        PcieUtil.__init__(self, platform_path)
        self._conf_rev = self.__get_conf_rev()

    def __get_conf_rev(self):
        """
        Retrieves the EEPROM label revision and finds the matching pcie.yaml file.

        Uses the Tlv object to get the label revision (0x27) and checks for the
        corresponding pcie.yaml file. Returns the revision if found.

        Returns:
            str: The label revision if a matching pcie.yaml file is found.
            None: If the label revision is not found or any error occurs.
        """
        try:
            import os
            from sonic_platform.eeprom import Tlv, NULL

            eeprom = Tlv()
            if eeprom is None:
                log.log_warning("Initializing the EEPROM object has failed.")
                return None

            # Try to get the TLV field for the label revision
            label_rev = eeprom.get_revision()
            if label_rev != NULL:
                log.log_warning("Cannot retrieve Label Revision (0x27) from the system EEPROM.")
                return None

            for rev in (label_rev, label_rev[:-1]):
                pcie_yaml_file = os.path.join(self.config_path, f"pcie_{rev}.yaml")
                if os.path.exists(pcie_yaml_file):
                    return rev

        except Exception as e:
            log.log_warning(f"{str(e)}")
            pass

        return None
