import traceback
from .log import log_crit, log_err, log_debug, log_warn
from .manager import Manager
from .template import TemplateFabric
import socket
from swsscommon import swsscommon
from ipaddress import IPv6Network, IPv6Address

supported_SRv6_behaviors = {
    'end',
    'end.dt6',
    'end.dt46',
    'end.x',
    'uN',
    'uDT6',
    'uDT46',
    'uA'
}

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
        self.config_db = swsscommon.SonicV2Connector()
        self.config_db.connect(self.config_db.CONFIG_DB)

    def set_handler(self, key, data):
        ip_addr = key
        if 'action' not in data:
            log_err("Found a SRv6 config entry that does not specify action: {} | {}".format(ip_addr, data))
            return
        
        if data['action'] not in supported_SRv6_behaviors:
            log_err("Found a SRv6 config entry associated with unsupported action: {} | {}".format(ip_addr, data))
            return
        
        if 'vrf' in data:
            # verify that vrf name exists in the VRF_TABLE of CONFIG_DB
            if not self.config_db.exists(self.config_db.CONFIG_DB, "VRF_TABLE|{}".format(data['vrf'])):
                log_err("Found a SRv6 config entry that maps to an undefined VRF: {} | {}".format(ip_addr, data))
                return
        
        sid = SID(ip_addr, data)
        locator = sid.get_locator()
        opcode = sid.get_opcode()

        cmd_list = ['segment-routing', 'srv6']
        cmd_list += ['locators', 'locator {}'.format(locator)]
        if locator not in self.sids:
            cmd_list += ['prefix {}/{} block-len {} node-len {}'.format(locator, sid.block_len + sid.node_len, sid.block_len, sid.node_len)]

        opcode_cmd = 'opcode ::{} {}'.format(opcode, sid.action)
        if sid.vrf != 'default':
            opcode_cmd += "vrf " + sid.vrf
        cmd_list.append(opcode_cmd)

        self.cfg_mgr.push_list(cmd_list)
        log_debug("{} SRv6 static configuration {} is scheduled for updates. {}".format(self.db_name, key, str(cmd_list)))

        self.sids.setdefault(locator, {})[opcode] = sid

    def del_handler(self, key, data):
        ip_addr = key
        sid = SID(ip_addr, data)
        locator = sid.get_locator()
        opcode = sid.get_opcode()

        if locator in self.sids:
            if opcode not in self.sids[locator]:
                log_warn("Encountered a config deletion with an unexpected SRv6 opcode: {} | {}".format(ip_addr, data))
                return
            
            cmd_list = ['segment-routing', 'srv6']
            cmd_list.append('locators')
            if len(self.sids[locator] == 1):
                # this is the last opcode of the locator, so we should delete the whole locator
                cmd_list.append('no locator {}'.format(locator))

                self.sids.pop(locator)
            else:
                # delete this opcode only
                opcode_cmd = 'no opcode ::{} {}'.format(opcode, sid.action)
                if sid.vrf != 'default':
                    opcode_cmd += "vrf " + sid.vrf
                cmd_list.append(opcode_cmd)

                self.sids[locator].pop(opcode)

            self.cfg_mgr.push_list(cmd_list)
            log_debug("{} SRv6 static configuration {} is scheduled for updates. {}".format(self.db_name, key, str(cmd_list)))
        else:
            log_warn("Encountered a config deletion with an unexpected SRv6 locator: {} | {}".format(ip_addr, data))
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
        self.locator = IPv6Address(self.bits & locator_mask)

        # extract the opcode(function id)
        func_mask = 0
        for i in range(self.func_len):
            func_mask <<= 1
            func_mask != 0x01
        func_mask <<= 128 - (self.block_len + self.node_len + self.func_len)
        self.opcode = IPv6Address(self.bits & func_mask)
        
        self.action = data['action']
        self.vrf = data['vrf'] if 'vrf' in data else "default"
        self.adj = data['adj'].split(',') if 'adj' in data else []

    def get_locator(self):
        return self.locator
    
    def get_opcode(self):
        return self.opcode