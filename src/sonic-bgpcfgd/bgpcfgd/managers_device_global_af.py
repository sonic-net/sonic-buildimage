from swsscommon import swsscommon

from .log import log_info, log_err, log_warn
from .manager import Manager

BGP_DEVICE_GLOBAL_AF_TABLE = "BGP_DEVICE_GLOBAL_AF"

# Maps afi_safi ConfigDB keys to FRR (afi, safi) strings
AFI_SAFI_MAP = {
    "ipv4_unicast":   ("ipv4", "unicast"),
    "ipv6_unicast":   ("ipv6", "unicast"),
}

INSTALL_BACKUP_PATH_FIELD = "install_backup_path"


class DeviceGlobalAfMgr(Manager):
    """Manager for device-global per-AFI BGP config in BGP_DEVICE_GLOBAL_AF.

    Subscribes to BGP_DEVICE_GLOBAL_AF and translates the install_backup_path
    enum (disabled / pic / pic-ecmp) into FRR 'install backup-path [ecmp]'
    commands for the default-VRF BGP instance, one per afi_safi.

    BGP_DEVICE_GLOBAL_AF is scoped to the device (no VRF key), so all commands
    target 'router bgp <asn>' (default VRF only).
    """

    def __init__(self, common_objs, db, table):
        super(DeviceGlobalAfMgr, self).__init__(
            common_objs,
            [("CONFIG_DB", swsscommon.CFG_DEVICE_METADATA_TABLE_NAME, "localhost/bgp_asn")],
            db,
            table,
        )
        # Tracks the last applied mode per afi_safi to avoid redundant commands
        self._pic_state = {}

    def set_handler(self, key, data):
        # key is the afi_safi string (e.g. "ipv4_unicast")
        afi_safi = key

        if afi_safi not in AFI_SAFI_MAP:
            log_warn("DeviceGlobalAfMgr: unsupported afi_safi '%s', ignoring" % afi_safi)
            return True

        pic_mode = data.get(INSTALL_BACKUP_PATH_FIELD, "disabled")
        if pic_mode not in ("disabled", "pic", "pic-ecmp"):
            log_err("DeviceGlobalAfMgr: unknown install_backup_path value '%s' for afi_safi '%s'" % (pic_mode, afi_safi))
            return True

        prev_mode = self._pic_state.get(afi_safi, "disabled")
        if pic_mode == prev_mode:
            log_info("DeviceGlobalAfMgr: no change in install_backup_path for %s (%s), skipping" % (afi_safi, pic_mode))
            return True

        bgp_asn = self._get_bgp_asn()
        if bgp_asn is None:
            log_info("DeviceGlobalAfMgr: no BGP ASN found, deferring")
            return False  # will be retried by the Manager base class queue

        cmds = self._build_cmds(bgp_asn, afi_safi, pic_mode)
        log_info("DeviceGlobalAfMgr: applying install_backup_path='%s' for afi_safi=%s" % (pic_mode, afi_safi))
        self.cfg_mgr.push_list(cmds)
        self._pic_state[afi_safi] = pic_mode
        return True

    def del_handler(self, key):
        afi_safi = key

        if afi_safi not in self._pic_state:
            return

        bgp_asn = self._get_bgp_asn()
        if bgp_asn is None:
            log_warn("DeviceGlobalAfMgr: no BGP ASN on delete for afi_safi '%s', clearing state without FRR push" % afi_safi)
            self._pic_state.pop(afi_safi, None)
            return

        cmds = self._build_cmds(bgp_asn, afi_safi, "disabled")
        log_info("DeviceGlobalAfMgr: removing install_backup_path for afi_safi=%s" % afi_safi)
        self.cfg_mgr.push_list(cmds)
        self._pic_state.pop(afi_safi, None)

    # ------------------------------------------------------------------
    # Internal helpers
    # ------------------------------------------------------------------

    def _get_bgp_asn(self):
        """Return the BGP ASN string for the default VRF, or None if not found."""
        slot = self.directory.get_slot("CONFIG_DB", swsscommon.CFG_DEVICE_METADATA_TABLE_NAME)
        return slot.get("localhost", {}).get("bgp_asn")

    @staticmethod
    def _build_cmds(bgp_asn, afi_safi, pic_mode):
        """Build the vtysh command list for the requested pic_mode.

        pic_mode values:
          'disabled'  -> no install backup-path      (clears both flags)
          'pic'       -> install backup-path          (single backup, clears ecmp flag)
          'pic-ecmp'  -> install backup-path ecmp     (multiple backup paths)
        """
        af, safi = AFI_SAFI_MAP[afi_safi]

        if pic_mode == "pic-ecmp":
            backup_cmd = "install backup-path ecmp"
        elif pic_mode == "pic":
            backup_cmd = "install backup-path"
        else:
            backup_cmd = "no install backup-path"

        return [
            "router bgp %s" % bgp_asn,
            " address-family %s %s" % (af, safi),
            "  %s" % backup_cmd,
            " exit-address-family",
            "exit",
        ]
