try:
    from sonic_platform_pddf_base.pddf_watchdog import PddfWatchdog
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")



class Watchdog(PddfWatchdog):
    """PDDF Platform-Specific Watchdog Class"""

    def __init__(self):
        PddfWatchdog.__init__(self)

    # Provide the functions/variables below for which implementation is to be overwritten
