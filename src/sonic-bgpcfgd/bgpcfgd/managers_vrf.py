from .manager import Manager
from .log import log_debug, log_err


class VrfMgr(Manager):
    """ This class binds an L3VNI to a VRF when the VRF table is updated """
    def __init__(self, common_objs, db, table):
        """
        Initialize the object
        :param common_objs: common object dictionary
        :param db: name of the db
        :param table: name of the table in the db
        """
        super(VrfMgr, self).__init__(
            common_objs,
            [],
            db,
            table,
        )
        self.vrf_vni = {}

    def set_handler(self, key, data):
        vrf = key
        vni = data.get("vni", "0")

        if not vni.isdigit():
            log_err("VrfMgr:: invalid vni '%s' for vrf %s" % (vni, vrf))
            return True

        old_vni = self.vrf_vni.get(vrf)

        # A vni of 0 is how the VRF to VNI mapping is withdrawn.
        if vni == "0":
            if old_vni is None:
                return True
            del self.vrf_vni[vrf]
            self.cfg_mgr.push_list(["vrf %s" % vrf, "no vni %s" % old_vni])
            log_debug("VrfMgr:: removed vni %s from vrf %s" % (old_vni, vrf))
            return True

        if old_vni == vni:
            return True

        cmds = ["vrf %s" % vrf]
        if old_vni is not None:
            cmds.append("no vni %s" % old_vni)
        cmds.append("vni %s" % vni)

        self.vrf_vni[vrf] = vni
        self.cfg_mgr.push_list(cmds)
        log_debug("VrfMgr:: set vni %s on vrf %s" % (vni, vrf))
        return True

    def del_handler(self, key):
        vrf = key
        old_vni = self.vrf_vni.pop(vrf, None)
        if old_vni is None:
            return
        self.cfg_mgr.push_list(["vrf %s" % vrf, "no vni %s" % old_vni])
        log_debug("VrfMgr:: removed vni %s with vrf %s" % (old_vni, vrf))
