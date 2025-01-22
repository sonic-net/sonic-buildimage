from .manager import Manager
from swsscommon import swsscommon
from .log import log_err, log_debug

ROUTE_MAPS = ["FROM_SDN_SLB_ROUTES", "FROM_SDN_APPLIANCE_ROUTES"]
FROM_SDN_SLB_DEPLOYMENT_ID = '2'

class RouteMapMgr(Manager):
    """This class add route-map when BGP_PROFILE_TABLE in APPL_DB is updated"""

    def __init__(self, common_objs, db, table):
        """
        Initialize the object
        :param common_objs: common object dictionary
        :param db: name of the db
        :param table: name of the table in the db
        """
        super(RouteMapMgr, self).__init__(
            common_objs,
            [],
            db,
            table,
        )

    def set_handler(self, key, data):
        log_debug("BGPRouteMapMgr:: set handler")
        """Only need a name as the key, and community id as the data"""
        if not self.__set_handler_validate(key, data):
            return True

        self.__update_rm(key, data)
        return True

    def del_handler(self, key):
        log_debug("BGPRouteMapMgr:: del handler")
        if not self.__del_handler_validate(key):
            return
        self.__remove_rm(key)

    def __remove_rm(self, rm):
        cmds = ["no route-map %s permit 100" % ("%s_RM" % rm)]
        log_debug("BGPRouteMapMgr:: remove route-map %s" % ("%s_RM" % rm))
        self.cfg_mgr.push_list(cmds)
        log_debug("BGPRouteMapMgr::Done")

    def __set_handler_validate(self, key, data):
        if key not in ROUTE_MAPS:
            log_err("BGPRouteMapMgr:: Invalid key for route-map %s" % key)
            return False

        if not data:
            log_err("BGPRouteMapMgr:: data is None")
            return False
        community_ids = data["community_id"].split(":")
        try:
            if (
                len(community_ids) != 2
                or int(community_ids[0]) not in range(0, 65536)
                or int(community_ids[1]) not in range(0, 65536)
            ):
                log_err("BGPRouteMapMgr:: data %s doesn't include valid community id %s" % (data, community_ids))
                return False
        except ValueError:
            log_err("BGPRouteMapMgr:: data %s includes illegal input" % (data))
            return False

        return True

    def __del_handler_validate(self, key):
        if key not in ROUTE_MAPS:
            log_err("BGPRouteMapMgr:: Invalid key for route-map %s" % key)
            return False
        return True

    def __read_asn(self):
        if not 'deployment_id_asn_map' in self.constants:
            log_err("BGPRouteMapMgr:: 'deployment_id_asn_map' key is not found in constants")
            return None
        if FROM_SDN_SLB_DEPLOYMENT_ID in self.constants['deployment_id_asn_map']:
            return self.constants['deployment_id_asn_map'][FROM_SDN_SLB_DEPLOYMENT_ID]
        log_err("BGPRouteMapMgr:: deployment id %s is not found in constants" % (FROM_SDN_SLB_DEPLOYMENT_ID))
        return None

    def __update_rm(self, rm, data):
        cmds = []
        if rm in ROUTE_MAPS :
            cmds.append("route-map %s permit 100" % ("%s_RM" % rm))
            bgp_asn = self.__read_asn()
            if bgp_asn is None or bgp_asn is '':
                log_debug("BGPRouteMapMgr:: update route-map %s, but asn is not found in constants" % ("%s_RM" % rm))
                return
            cmds.append(" set as-path prepend %s %s" % (bgp_asn, bgp_asn))
            cmds.append(" set community %s" % data["community_id"])
            cmds.append(" set origin incomplete")
            log_debug("BGPRouteMapMgr:: update route-map %s community %s origin incomplete as-path prepend %s %s" % \
                      ("%s_RM" % rm, data["community_id"], bgp_asn, bgp_asn))
        if cmds:
            self.cfg_mgr.push_list(cmds)
        log_debug("BGPRouteMapMgr::Done")
