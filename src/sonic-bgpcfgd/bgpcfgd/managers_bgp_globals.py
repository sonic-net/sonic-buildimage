from swsscommon import swsscommon

from .log import log_info, log_warn
from .manager import Manager

BGP_GLOBALS_TABLE = "BGP_GLOBALS"


class BgpGlobalsMgr(Manager):
    """Handles runtime changes to BGP_GLOBALS|default in ConfigDB.

    Presence of llgr_stale_time enables LLGR; absence disables it (RFC 9494).
      bgp long-lived-graceful-restart stale-time <N>   (enable)
      no bgp long-lived-graceful-restart stale-time    (disable)

    Only the 'default' VRF key is handled; other VRF keys are ignored.
    """

    def __init__(self, common_objs, db, table):
        super(BgpGlobalsMgr, self).__init__(
            common_objs,
            [("CONFIG_DB", swsscommon.CFG_DEVICE_METADATA_TABLE_NAME, "localhost/bgp_asn")],
            db,
            table,
        )
        self._llgr_active = False

    def set_handler(self, key, data):
        if key != "default":
            log_info("BgpGlobalsMgr: ignoring non-default VRF key '%s'" % key)
            return True

        bgp_asn = self._get_bgp_asn()
        if bgp_asn is None:
            log_info("BgpGlobalsMgr: no BGP ASN found, deferring")
            return False

        stale_time = data.get("llgr_stale_time")
        if stale_time is not None:
            cmds = self._build_llgr_enable_cmds(bgp_asn, stale_time)
            log_info("BgpGlobalsMgr: enabling LLGR stale-time=%s" % stale_time)
            self.cfg_mgr.push_list(cmds)
            self._llgr_active = True
        else:
            # Always send the no command — it is idempotent in FRR and closes the
            # crash-recovery window where bgpcfgd died after an operator removed
            # llgr_stale_time but before the disable command reached FRR.
            cmds = self._build_llgr_disable_cmds(bgp_asn)
            log_info("BgpGlobalsMgr: disabling LLGR")
            self.cfg_mgr.push_list(cmds)
            self._llgr_active = False

        return True

    def del_handler(self, key):
        if key != "default":
            return

        bgp_asn = self._get_bgp_asn()
        if bgp_asn is None:
            log_warn("BgpGlobalsMgr: no BGP ASN on delete, clearing state")
            self._llgr_active = False
            return

        if self._llgr_active:
            cmds = self._build_llgr_disable_cmds(bgp_asn)
            log_info("BgpGlobalsMgr: disabling LLGR (entry deleted)")
            self.cfg_mgr.push_list(cmds)
            self._llgr_active = False

    def _get_bgp_asn(self):
        slot = self.directory.get_slot("CONFIG_DB", swsscommon.CFG_DEVICE_METADATA_TABLE_NAME)
        return slot.get("localhost", {}).get("bgp_asn")

    @staticmethod
    def _build_llgr_enable_cmds(bgp_asn, stale_time):
        return [
            "router bgp %s" % bgp_asn,
            " bgp long-lived-graceful-restart stale-time %s" % stale_time,
            "exit",
        ]

    @staticmethod
    def _build_llgr_disable_cmds(bgp_asn):
        return [
            "router bgp %s" % bgp_asn,
            " no bgp long-lived-graceful-restart stale-time",
            "exit",
        ]
