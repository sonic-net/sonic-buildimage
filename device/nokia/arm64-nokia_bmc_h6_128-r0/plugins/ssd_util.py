try:
    from sonic_platform_base.sonic_storage.emmc import EmmcUtil
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")


class SsdUtil(EmmcUtil):
    """Primary storage is eMMC; use common sysfs-backed EmmcUtil."""

    def __init__(self, diskdev):
        EmmcUtil.__init__(self, diskdev)
