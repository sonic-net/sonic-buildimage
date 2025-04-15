from .manager import Manager
from .log import log_debug, log_warn, log_info
from swsscommon import swsscommon

T2_GROUP_ASNS = "T2_GROUP_ASNS"


class AsPathMgr(Manager):
    """This class responds to initialize as-path"""

    def __init__(self, common_objs, db, table):
        """
        Initialize the object
        :param common_objs: common object dictionary
        :param db: name of the db
        :param table: name of the table in the db
        """
        self.directory = common_objs['directory']
        self.cfg_mgr = common_objs['cfg_mgr']
        self.constants = common_objs['constants']
        super(AsPathMgr, self).__init__(
            common_objs,
            [],
            db,
            table,
        )
        
    
    def set_handler(self, key, data):
        log_info("AsPathMgr: set handler")
        if not "|" in key:
            log_info("AsPathMgr: no \"|\" in {}".format(key))
            return True
        splits = key.split("|")
        if splits != 2 or splits[0] != "localhost" or "t2_group_asns" not in splits[1]:
            log_info("AsPathMgr: Cannot find t2_group_asns")
            return True
        for asn in splits[1]:
            log_info("AsPathMgr: Add asn {} to {}".format(asn, T2_GROUP_ASNS))
            self.cfg_mgr.push("bgp as-path access-list {} permit _{}_".format(T2_GROUP_ASNS, asn))
        return True

    def del_handler(self, key, data):
        log_info("AsPathMgr: del handler")
        if not "|" in key:
            log_info("AsPathMgr: no \"|\" in {}".format(key))
            return True
        splits = key.split("|")
        if splits != 2 or splits[0] != "localhost" or "t2_group_asns" not in splits[1]:
            log_info("AsPathMgr: Cannot find t2_group_asns")
            return True
        for asn in splits[1]:
            log_info("AsPathMgr: Add asn {} to {}".format(asn, T2_GROUP_ASNS))
            self.cfg_mgr.push("bgp as-path access-list {} permit _{}_".format(T2_GROUP_ASNS, asn))
        return True
