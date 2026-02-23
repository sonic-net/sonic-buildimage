import re

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
            return True
        asns = None
        for key_inside, value in data.items():
            if key_inside == "t2_group_asns":
                asns = value
                break
        new_asns = set()
        if asns is not None:
            for asn in asns.split(","):
                new_asns.add(asn)
        old_asns = {}
        regex = re.compile(r"bgp as-path access-list T2_GROUP_ASNS seq \d+ permit _(\d+)_")
        # Read current FRR configuration and get as-path already configured
        self.cfg_mgr.update()
        for line in self.cfg_mgr.get_text():
            match = regex.match(line)
            if match:
                old_asns[match.group(1)] = line
        # Delete as-path no longer needed
        for deleted_asn in (set(old_asns.keys()) - new_asns):
            self.cfg_mgr.push("no {}".format(old_asns[deleted_asn]))
            log_info("AsPathMgr: Deleted as-path: {}".format(deleted_asn))
        # Add new as-path
        for added_asn in (new_asns - set(old_asns.keys())):
            self.cfg_mgr.push("bgp as-path access-list {} permit _{}_".format(T2_GROUP_ASNS, added_asn))
            log_info("AsPathMgr: Added as-path: {}".format(added_asn))
        return True

    def del_handler(self, key):
        if key != "localhost":
            return True
        # It would be trigger when we delete all `localhost` entry in DEVICE_METADATA, then clear t2 group asns
        log_info("AsPathMgr: Clear asns group {}".format(T2_GROUP_ASNS))
        self.cfg_mgr.push("no bgp as-path access-list {}".format(T2_GROUP_ASNS))
        return True
