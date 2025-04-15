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
        if key != "localhost":
            return
        asns = None
        for key_inside, value in data.items():
            if key_inside == "t2_group_asns":
                asns = value
                break
        cmd = "no bgp as-path access-list {}".format(T2_GROUP_ASNS)
        log_info("AsPathMgr: Clear asns group with cmd: [{}]".format(cmd))
        self.cfg_mgr.push(cmd)
        if asns is not None:
            for asn in asns.split(","):
                cmd = "bgp as-path access-list {} permit _{}_".format(T2_GROUP_ASNS, asn)
                log_info("AsPathMgr: Add as-path with cmd: [{}]".format(cmd))
                self.cfg_mgr.push(cmd)
        return True

    def del_handler(self, key):
        if key != "localhost":
            return True
        # It would be trigger when we deleta all `localhost` entry in DEVICE_METADATA, then clear t2 group asns
        cmd = "no bgp as-path access-list {}".format(T2_GROUP_ASNS)
        log_info("AsPathMgr: Clear asns group with cmd: [{}]".format(cmd))
        self.cfg_mgr.push(cmd)
        return True
