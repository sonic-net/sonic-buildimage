import re

from .manager import Manager
from .log import log_debug, log_err


ESI_TYPE_3 = "TYPE_3_MAC_BASED"
ESI_TYPE_0 = "TYPE_0_OPERATOR_CONFIGURED"

PORT_ID_RE = re.compile(r'[a-zA-Z]+(?P<port_id>[0-9_]+)')


def port_id_from_if_name(if_name):
    """ Extract the numeric port identifier used as the local ES id """
    match = PORT_ID_RE.search(if_name)
    if not match:
        return None
    port_id = match.group('port_id')
    return port_id.replace('_', '') if port_id else None


class EvpnMhEsMgr(Manager):
    """ This class pushes EVPN Ethernet Segment configuration to FRR.

    The CLI programs FRR only at the moment the command is typed, so without
    this manager an Ethernet Segment survives in CONFIG_DB but is lost from
    zebra on every restart. Two PEs then advertise the same MACs without a
    shared ESI, which BGP reads as a MAC move and turns into a sequence number
    war between them.
    """
    def __init__(self, common_objs, db, table):
        """
        Initialize the object
        :param common_objs: common object dictionary
        :param db: name of the db
        :param table: name of the table in the db
        """
        super(EvpnMhEsMgr, self).__init__(
            common_objs,
            [("CONFIG_DB", "PORTCHANNEL", "")],
            db,
            table,
        )
        self.es_cfg = {}

    def get_system_mac(self, interface_name):
        port_channels = self.directory.get_slot("CONFIG_DB", "PORTCHANNEL")
        return port_channels.get(interface_name, {}).get("system_mac")

    def build_cmds(self, interface_name, data):
        esi_type = data.get("type", "")
        esi = data.get("esi", "")
        df_pref = data.get("df_pref", "")

        cmds = []

        if esi_type == ESI_TYPE_0:
            if not esi or esi == "AUTO":
                log_err("EvpnMhEsMgr:: %s is type 0 but has no operator configured ESI"
                        % interface_name)
                return None
            cmds.append("evpn mh es-id %s" % esi)
        elif esi_type == ESI_TYPE_3:
            port_id = port_id_from_if_name(interface_name)
            if port_id is None:
                log_err("EvpnMhEsMgr:: cannot derive a port id from %s" % interface_name)
                return None
            cmds.append("evpn mh es-id %s" % port_id)
            system_mac = self.get_system_mac(interface_name)
            cmds.append("evpn mh es-sys-mac %s" % system_mac)
        else:
            log_err("EvpnMhEsMgr:: unknown ESI type '%s' for %s" % (esi_type, interface_name))
            return None

        if df_pref:
            cmds.append("evpn mh es-df-pref %s" % df_pref)

        return cmds

    def set_handler(self, key, data):
        interface_name = key

        # The system MAC makes the ESI identical on both PEs, and it comes from
        # a different table, so on a restart it can arrive after the segment.
        # Defer rather than programme an ESI the peer will never match.
        if data.get("type", "") == ESI_TYPE_3 and not self.get_system_mac(interface_name):
            log_debug("EvpnMhEsMgr:: %s has no system_mac yet, deferring" % interface_name)
            return False

        cmds = self.build_cmds(interface_name, data)
        if cmds is None:
            return True

        if self.es_cfg.get(interface_name) == cmds:
            return True

        self.es_cfg[interface_name] = cmds
        self.cfg_mgr.push_list(["interface %s" % interface_name] + cmds)
        log_debug("EvpnMhEsMgr:: configured Ethernet Segment on %s" % interface_name)
        return True

    def del_handler(self, key):
        interface_name = key

        if self.es_cfg.pop(interface_name, None) is None:
            return

        self.cfg_mgr.push_list([
            "interface %s" % interface_name,
            "no evpn mh es-sys-mac",
            "no evpn mh es-df-pref",
            "no evpn mh es-id",
        ])
        log_debug("EvpnMhEsMgr:: removed Ethernet Segment from %s" % interface_name)
