import re

from swsscommon import swsscommon

from .log import log_err, log_info
from .manager import Manager

BGP_BBR_TABLE_NAME = "BGP_BBR"
BGP_BBR_STATUS_KEY = "status"
BGP_BBR_STATUS_ENABLED = "enabled"
BGP_BBR_STATUS_DISABLED = "disabled"


class BBRMgr(Manager):
    """ This class initialize "BBR" feature for  """
    def __init__(self, common_objs, db, table):
        """
        Initialize the object
        :param common_objs: common object dictionary
        :param db: name of the db
        :param table: name of the table in the db
        """
        super(BBRMgr, self).__init__(
            common_objs,
            [("CONFIG_DB", swsscommon.CFG_DEVICE_METADATA_TABLE_NAME, "localhost/bgp_asn"),],
            db,
            table,
        )
        self.enabled = False
        self.bbr_enabled_pgs = {}
        self.directory.put(self.db_name, self.table_name, 'status', "disabled")
        self.__init()

    def set_handler(self, key, data):
        """ Implementation of 'SET' command for this class """
        if not self.enabled:
            log_info("BBRMgr::BBR is disabled. Drop the request")
            return True
        if not self.__set_validation(key, data):
            return True
        cmds, peer_groups_to_restart = self.__set_prepare_config(data['status'])
        self.cfg_mgr.push_list(cmds)
        self.cfg_mgr.restart_peer_groups(peer_groups_to_restart)
        log_info("BBRMgr::Scheduled BBR update")
        self.directory.put(self.db_name, self.table_name, BGP_BBR_STATUS_KEY, data[BGP_BBR_STATUS_KEY])
        return True

    def del_handler(self, key):
        """ Implementation of 'DEL' command for this class """
        log_err("The '%s' table shouldn't be removed from the db" % self.table_name)
        self.directory.remove(self.db_name, self.table_name, BGP_BBR_STATUS_KEY)

    def __init(self):
        """ Initialize BBRMgr. Extracted from constructor """
        # Check BGP_BBR table from config_db first
        bbr_status_from_config_db = self.get_bbr_status_from_config_db()

        if bbr_status_from_config_db is None:
            if not 'bgp' in self.constants:
                log_err("BBRMgr::Disabled: 'bgp' key is not found in constants")
                return
            if 'bbr' in self.constants['bgp'] \
                    and 'enabled' in self.constants['bgp']['bbr'] \
                    and self.constants['bgp']['bbr']['enabled']:
                self.bbr_enabled_pgs = self.__read_pgs()
                if self.bbr_enabled_pgs:
                    self.enabled = True
                    if 'default_state' in self.constants['bgp']['bbr'] \
                            and self.constants['bgp']['bbr']['default_state'] == 'enabled':
                        default_status = "enabled"
                    else:
                        default_status = "disabled"
                    self.directory.put(self.db_name, self.table_name, 'status', default_status)
                    log_info("BBRMgr::Initialized and enabled from constants. Default state: '%s'" % default_status)
                else:
                    log_info("BBRMgr::Disabled: no BBR enabled peers")
            else:
                log_info("BBRMgr::Disabled: no bgp.bbr.enabled in the constants")
        else:
            self.bbr_enabled_pgs = self.__read_pgs()
            if self.bbr_enabled_pgs:
                self.enabled = True
                self.directory.put(self.db_name, self.table_name, 'status', bbr_status_from_config_db)
                log_info("BBRMgr::Initialized and enabled from config_db. Default state: '%s'" % bbr_status_from_config_db)
            else:
                log_info("BBRMgr::Disabled: no BBR enabled peers")

    def __read_pgs(self):
        """
        Read peer-group bbr settings from constants file
        :return: return bbr information from constant peer-group settings
        """
        if 'peers' not in self.constants['bgp']:
            log_info("BBRMgr::no 'peers' was found in constants")
            return {}
        res = {}
        for peer_name, value in self.constants['bgp']['peers'].items():
            if 'bbr' not in value:
                continue
            for pg_name, pg_afs in value['bbr'].items():
                res[pg_name] = pg_afs
        return res

    def get_bbr_status_from_config_db(self):
        """
        Read BBR status from CONFIG_DB
        :return: BBR status from CONFIG_DB or None if not found
        """
        try:
            config_db = swsscommon.ConfigDBConnector()
            if config_db is None:
                log_info("BBRMgr::Failed to connect to CONFIG_DB, get BBR default state from constants.yml")
                return None
            config_db.connect()
        except Exception as e:
            log_info("BBRMgr::Failed to connect to CONFIG_DB with exception %s, get BBR default state from constants.yml" % str(e))
            return None

        try:
            bbr_table_data = config_db.get_table(self.table_name)
            if bbr_table_data and 'all' in bbr_table_data and 'status' in bbr_table_data["all"]:
                if bbr_table_data["all"]["status"] == "enabled":
                    return "enabled"
                else:
                    return "disabled"
            else:
                log_info("BBRMgr::BBR status is not found in CONFIG_DB, get BBR default state from constants.yml")
                return None
        except Exception as e:
            log_info("BBRMgr::Failed to read BBR status from CONFIG_DB with exception %s, get BBR default state from constants.yml" % str(e))
            return None

    def __set_validation(self, key, data):
        """ Validate set-command arguments
        :param key: key of 'set' command
        :param data: data of 'set' command
        :return: True is the parameters are valid, False otherwise
        """
        if key != 'all':
            log_err("Invalid key '%s' for table '%s'. Only key value 'all' is supported" % (key, self.table_name))
            return False
        if 'status' not in data:
            log_err("Invalid value '%s' for table '%s', key '%s'. Key 'status' in data is expected" % (data, self.table_name, key))
            return False
        if data['status'] != "enabled" and data['status'] != "disabled":
            log_err("Invalid value '%s' for table '%s', key '%s'. Only 'enabled' and 'disabled' are supported" % (data, self.table_name, key))
            return False
        return True

    def __set_prepare_config(self, status):
        """
        Generate FFR configuration to apply changes
        :param status: either "enabled" or "disabled"
        :return: list of commands prepared for FRR
        """
        bgp_asn = self.directory.get_slot("CONFIG_DB", swsscommon.CFG_DEVICE_METADATA_TABLE_NAME)["localhost"]["bgp_asn"]
        available_peer_groups = self.__get_available_peer_groups()
        cmds = ["router bgp %s" % bgp_asn]
        prefix_of_commands = "" if status == "enabled" else "no "
        peer_groups_to_restart = set()
        for af in ["ipv4", "ipv6"]:
            cmds.append(" address-family %s" % af)
            for pg_name in sorted(self.bbr_enabled_pgs.keys()):
                for peer_group_name in available_peer_groups:
                    if peer_group_name.startswith(pg_name) and af in self.bbr_enabled_pgs[pg_name]:
                        cmds.append("  %sneighbor %s allowas-in 1" % (prefix_of_commands, peer_group_name))
                        peer_groups_to_restart.add(peer_group_name)
            cmds.append(" exit-address-family")
        cmds.append("exit")
        return cmds, list(peer_groups_to_restart)

    def __get_available_peer_groups(self):
        """
        Extract configured peer-groups from the config
        :return: set of available peer-groups
        """
        re_pg = re.compile(r'^\s*neighbor\s+(\S+)\s+peer-group\s*$')
        res = set()
        self.cfg_mgr.update()
        for line in self.cfg_mgr.get_text():
            m = re_pg.match(line)
            if m:
                res.add(m.group(1))
        return res