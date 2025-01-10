from .log import log_err, log_debug, log_warn
from .manager import Manager
from ipaddress import IPv6Address
from swsscommon import swsscommon

supported_SRv6_behaviors = {
    'uN',
    'uDT46',
}

DEFAULT_VRF = "default"
SRV6_MY_SIDS_TABLE_NAME = "SRV6_MY_SIDS"

class SRv6Mgr(Manager):
    """ This class updates SRv6 configurations when SRV6_MY_SID_TABLE table is updated """
    def __init__(self, common_objs, db, table):
        """
        Initialize the object
        :param common_objs: common object dictionary
        :param db: name of the db
        :param table: name of the table in the db
        """
        super(SRv6Mgr, self).__init__(
            common_objs,
            [],
            db,
            table,
        )

    def set_handler(self, key, data):
        if self.table_name == SRV6_MY_SIDS_TABLE_NAME:
            return self.sids_set_handler(key, data)
        else:
            return self.locators_set_handler(key, data)

    def locators_set_handler(self, key, data):
        locator_name = key

        locator = Locator(locator_name, data)
        cmd_list = ["segment-routing", "srv6"]
        cmd_list += ['locators',
                     'locator {}'.format(locator_name),
                     'prefix {} block-len {} node-len {} func-bits {}'.format(
                     locator.prefix,
                     locator.block_len, locator.node_len, locator.func_len),
                     "behavior usid"
        ]

        self.cfg_mgr.push_list(cmd_list)
        log_debug("{} SRv6 static configuration {}|{} is scheduled for updates. {}".format(self.db_name, self.table_name, key, str(cmd_list)))

        self.directory.put(self.db_name, self.table_name, key, locator)
        return True

    def sids_set_handler(self, key, data):
        locator_name = key.split("|")[0]
        ip_addr = key.split("|")[1].lower()
        key = "{}|{}".format(locator_name, ip_addr)

        if not self.directory.path_exist(self.db_name, "SRV6_MY_LOCATORS", locator_name):
            log_err("Found a SRv6 SID config entry with a locator that does not exist: {} | {}".format(key, data))
            return False
        
        locator = self.directory.get(self.db_name, "SRV6_MY_LOCATORS", locator_name)

        if 'action' not in data:
            log_err("Found a SRv6 SID config entry that does not specify action: {} | {}".format(key, data))
            return False
        
        if data['action'] not in supported_SRv6_behaviors:
            log_err("Found a SRv6 SID config entry associated with unsupported action: {} | {}".format(key, data))
            return False
        
        sid = SID(locator_name, ip_addr, data) # the information in data will be parsed into SID's attributes

        cmd_list = ['segment-routing', 'srv6', 'static-sids']
        sid_cmd = 'sid {}/{} locator {} behavior {}'.format(ip_addr, locator.block_len + locator.node_len + locator.func_len, locator_name, sid.action)
        if sid.decap_vrf != DEFAULT_VRF:
            sid_cmd += ' vrf {}'.format(sid.decap_vrf)
        cmd_list.append(sid_cmd)

        self.cfg_mgr.push_list(cmd_list)
        log_debug("{} SRv6 static configuration {}|{} is scheduled for updates. {}".format(self.db_name, self.table_name, key, str(cmd_list)))

        self.directory.put(self.db_name, self.table_name, key, (sid, sid_cmd))
        return True

    def del_handler(self, key):
        if self.table_name == SRV6_MY_SIDS_TABLE_NAME:
            self.sids_del_handler(key)
        else:
            self.locators_del_handler(key)

    def locators_del_handler(self, key):
        locator_name = key
        cmd_list = ['segment-routing', 'srv6', 'locators', 'no locator {}'.format(locator_name)]

        self.cfg_mgr.push_list(cmd_list)
        log_debug("{} SRv6 static configuration {}|{} is scheduled for updates. {}".format(self.db_name, self.table_name, key, str(cmd_list)))
        self.directory.remove(self.db_name, self.table_name, key)

    def sids_del_handler(self, key):
        locator_name = key.split("|")[0]
        ip_addr = key.split("|")[1].lower()
        key = "{}|{}".format(locator_name, ip_addr)

        if not self.directory.path_exist(self.db_name, self.table_name, key):
            log_warn("Encountered a config deletion with a SRv6 SID that does not exist: {}".format(key))
            return

        _, sid_cmd = self.directory.get(self.db_name, self.table_name, key)
        cmd_list = ['segment-routing', 'srv6', "static-sids"]
        no_sid_cmd = 'no ' + sid_cmd
        cmd_list.append(no_sid_cmd)

        self.cfg_mgr.push_list(cmd_list)
        log_debug("{} SRv6 static configuration {}|{} is scheduled for updates. {}".format(self.db_name, self.table_name, key, str(cmd_list)))
        self.directory.remove(self.db_name, self.table_name, key)

class Locator:
    def __init__(self, name, data):
        self.name = name
        self.block_len = int(data['block_len'] if 'block_len' in data else 32)
        self.node_len = int(data['node_len'] if 'node_len' in data else 16)
        self.func_len = int(data['func_len'] if 'func_len' in data else 16)
        self.arg_len = int(data['arg_len'] if 'arg_len' in data else 0)
        self.prefix = data['prefix'].lower() + "/{}".format(self.block_len + self.node_len)

class SID:
    def __init__(self, locator, ip_addr, data):
        self.locator_name = locator
        self.ip_addr = ip_addr
        
        self.action = data['action']
        self.decap_vrf = data['decap_vrf'] if 'decap_vrf' in data else DEFAULT_VRF
        self.adj = data['adj'].split(',') if 'adj' in data else []