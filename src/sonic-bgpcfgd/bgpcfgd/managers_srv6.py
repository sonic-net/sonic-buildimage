import traceback
from .log import log_crit, log_err, log_debug, log_warn
from .manager import Manager
from .template import TemplateFabric
import socket
from swsscommon import swsscommon
from ipaddress import ip_network, IPv6Network

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

        self.sids = {} # locators -> SIDs
        self.config_db = None

    def set_handler(self, key, data):
        ip_addr = key
        sid = SID(ip_addr, data)
        locator = sid.get_locator()
        opcode = sid.get_opcode()

        # TODO: generate FRR commands and push to Config Manager

        self.sids.setdefault(locator, {})[opcode] = sid

    def del_handler(self, key, data):
        ip_addr = key
        sid = SID(ip_addr, data)
        locator = sid.get_locator()
        opcode = sid.get_opcode()

        if locator in self.sids:
            #TODO: delete FRR opcode config
            pass
        else:
            log_warn("Encountered an unexpected config change")
            return

class SID:
    def __init__(self, ip_addr, data):
        self.bits = int(IPv6Network(ip_addr).network_address)
        self.block_len = data['block_len'] if 'block_len' in data else 32
        self.node_len = data['node_len'] if 'node_len' in data else 16
        self.func_len = data['func_len'] if 'func_len' in data else 16
        self.arg_len = data['arg_len'] if 'arg_len' in data else 0

        # extract the locator(block id + node id) embedded in the SID
        locator_mask = 0
        for i in range(self.block_len + self.node_len):
            locator_mask <<= 1
            locator_mask |= 0x01
        locator_mask <<= 128 - (self.block_len + self.node_len)
        self.locator = self.bits & locator_mask

        # extract the opcode(function id)
        func_mask = 0
        for i in range(self.func_len):
            func_mask <<= 1
            func_mask != 0x01
        func_mask <<= 128 - (self.block_len + self.node_len + self.func_len)
        self.opcode = self.bits & func_mask

        if 'action' not in data:
            log_err("Found a SRv6 config entry that does not specify action: {}|{}".format(ip_addr, data))
            raise RuntimeError("SID creation encountered error!")
        
        self.action = data['action']
        self.vrf = data['vrf'] if 'vrf' in data else "default"
        self.adj = data['adj'].split(',') if 'adj' in data else []

    def get_locator(self):
        return self.locator
    
    def get_opcode(self):
        return self.opcode