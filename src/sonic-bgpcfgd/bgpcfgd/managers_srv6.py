from .log import log_err, log_debug, log_warn
from .manager import Manager
from ipaddress import IPv6Address
from swsscommon import swsscommon

supported_SRv6_behaviors = {
    'uN',
    'uDT46',
}

DEFAULT_VRF = "default"

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

        self.sids = {} # locators -> sid_ip_addr -> sid_objs
        self.config_db = swsscommon.SonicV2Connector()
        self.config_db.connect(self.config_db.CONFIG_DB)

    def set_handler(self, key, data):
        locator = key.split("|")[0]
        ip_addr = key.split("|")[1]

        if not self.directory.path_exist("CONFIG_DB", "SRV6_MY_LOCATORS", locator):
            log_err("Found a SRv6 SID config entry with a locator that does not exist: {} | {}".format(key, data))
            return False
        
        locator_data = self.config_db.get_all(self.config_db.CONFIG_DB, "SRV6_MY_LOCATORS|" + locator)
        if 'action' not in data:
            log_err("Found a SRv6 SID config entry that does not specify action: {} | {}".format(key, data))
            return False
        
        if data['action'] not in supported_SRv6_behaviors:
            log_err("Found a SRv6 SID config entry associated with unsupported action: {} | {}".format(key, data))
            return False
        
        sid = SID(locator, ip_addr, locator_data=locator_data, sid_data=data) # the information in data will be parsed into SID's attributes

        cmd_list = ['segment-routing', 'srv6']
        cmd_list += ['locators', 'locator {}'.format(locator)]
        if locator not in self.sids:
            cmd_list += ['prefix {}/{} block-len {} node-len {} func-bits {}'.format(sid.get_locator_prefix(), sid.block_len + sid.node_len, sid.block_len, sid.node_len, sid.func_len)]

        sid_cmd = 'sid {}/{} behavior {}'.format(ip_addr, sid.block_len + sid.node_len + sid.func_len, sid.action)
        if sid.decap_vrf != DEFAULT_VRF:
            sid_cmd += ' vrf {}'.format(sid.decap_vrf)
        cmd_list.append(sid_cmd)

        self.cfg_mgr.push_list(cmd_list)
        log_debug("{} SRv6 static configuration {} is scheduled for updates. {}".format(self.db_name, key, str(cmd_list)))

        self.sids.setdefault(locator, {})[ip_addr] = sid
        return True

    def del_handler(self, key):
        locator = key.split("|")[0]
        ip_addr = key.split("|")[1]

        if not self.directory.path_exist("CONFIG_DB", "SRV6_MY_LOCATORS", locator):
            log_err("Encountered a config deletion with a SRv6 locator that does not exist: {}".format(key))
            return

        if locator in self.sids:
            if ip_addr not in self.sids[locator]:
                log_warn("Encountered a config deletion with an unexpected SRv6 SID: {}".format(key))
                return

            sid = self.sids[locator][ip_addr]
            cmd_list = ['segment-routing', 'srv6']
            cmd_list.append('locators')
            if len(self.sids[locator]) == 1:
                # this is the last sid of the locator, so we should delete the whole locator
                cmd_list.append('no locator {}'.format(locator))

                self.sids.pop(locator)
            else:
                # delete this sid only
                cmd_list.append('locator {}'.format(locator))
                no_sid_cmd = 'no sid {}/{} behavior {}'.format(ip_addr,  sid.block_len + sid.node_len + sid.func_len, sid.action)
                if sid.decap_vrf != DEFAULT_VRF:
                    no_sid_cmd += ' vrf {}'.format(sid.decap_vrf)
                cmd_list.append(no_sid_cmd)

                self.sids[locator].pop(ip_addr)

            self.cfg_mgr.push_list(cmd_list)
            log_debug("{} SRv6 static configuration {} is scheduled for updates. {}".format(self.db_name, key, str(cmd_list)))
        else:
            log_warn("Encountered a config deletion with an unexpected SRv6 locator: {}".format(key))
            return

class SID:
    def __init__(self, locator, ip_addr, locator_data, sid_data):
        self.locator_name = locator
        self.bits = int(IPv6Address(ip_addr))
        self.block_len = locator_data['block_len'] if 'block_len' in locator_data else 32
        self.node_len = locator_data['node_len'] if 'node_len' in locator_data else 16
        self.func_len = locator_data['func_len'] if 'func_len' in locator_data else 16
        self.arg_len = locator_data['arg_len'] if 'arg_len' in locator_data else 0

        # extract the locator(block id + node id) embedded in the SID
        locator_mask = 0
        for i in range(self.block_len + self.node_len):
            locator_mask <<= 1
            locator_mask |= 0x01
        locator_mask <<= 128 - (self.block_len + self.node_len)
        self.locator_prefix = IPv6Address(self.bits & locator_mask)

        # extract the func_bits (function id)
        func_mask = 0
        for i in range(self.func_len):
            func_mask <<= 1
            func_mask |= 0x01
        func_mask <<= 128 - (self.block_len + self.node_len + self.func_len)
        self.func_bits = IPv6Address(self.bits & func_mask)
        
        self.action = sid_data['action']
        self.decap_vrf = sid_data['decap_vrf'] if 'decap_vrf' in sid_data else DEFAULT_VRF
        self.adj = sid_data['adj'].split(',') if 'adj' in sid_data else []

    def get_locator_name(self):
        return self.locator_name

    def get_locator_prefix(self):
        return self.locator_prefix
    
    def get_func_bits(self):
        return self.func_bits