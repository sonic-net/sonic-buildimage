import traceback
from .log import log_crit, log_err, log_debug
from .manager import Manager
from .template import TemplateFabric
import socket
from swsscommon import swsscommon
from ipaddress import ip_network, IPv4Network

class StaticRouteMgr(Manager):
    """ This class updates static routes when STATIC_ROUTE table is updated """
    def __init__(self, common_objs, db, table):
        """
        Initialize the object
        :param common_objs: common object dictionary
        :param db: name of the db
        :param table: name of the table in the db
        """
        super(StaticRouteMgr, self).__init__(
            common_objs,
            [],
            db,
            table,
        )

        self.directory.subscribe([("CONFIG_DB", swsscommon.CFG_DEVICE_METADATA_TABLE_NAME, "localhost/bgp_asn"),], self.on_bgp_asn_change)
        self.static_routes = {}
        self.vrf_pending_redistribution = set()
        self.config_db = None

    OP_DELETE = 'DELETE'
    OP_ADD = 'ADD'
    ROUTE_ADVERTISE_ENABLE_TAG = '1'
    ROUTE_ADVERTISE_DISABLE_TAG = '2'

    def set_handler(self, key, data):
        vrf, ip_prefix = self.split_key(key)
        is_ipv6 = TemplateFabric.is_ipv6(ip_prefix)

        arg_list    = lambda v: v.split(',') if len(v.strip()) != 0 else None
        bkh_list    = arg_list(data['blackhole']) if 'blackhole' in data else None
        nh_list     = arg_list(data['nexthop']) if 'nexthop' in data else None
        intf_list   = arg_list(data['ifname']) if 'ifname' in data else None
        dist_list   = arg_list(data['distance']) if 'distance' in data else None
        nh_vrf_list = arg_list(data['nexthop-vrf']) if 'nexthop-vrf' in data else None
        bfd_enable  = arg_list(data['bfd']) if 'bfd' in data else None
        route_tag   = self.ROUTE_ADVERTISE_DISABLE_TAG if 'advertise' in data and data['advertise'] == "false" else self.ROUTE_ADVERTISE_ENABLE_TAG

        # bfd enabled route would be handled in staticroutebfd, skip here
        if bfd_enable and bfd_enable[0].lower() == "true":
            log_debug("{} static route {} bfd flag is true".format(self.db_name, key))
            tmp_nh_set, tmp_route_tag = self.static_routes.get(vrf, {}).get(ip_prefix, (IpNextHopSet(is_ipv6), route_tag))
            if tmp_nh_set: #clear nexthop set if it is not empty
                log_debug("{} static route {} bfd flag is true, cur_nh is not empty, clear it".format(self.db_name, key))
                self.static_routes.setdefault(vrf, {}).pop(ip_prefix, None)
            return True

        try:
            ip_nh_set = IpNextHopSet(is_ipv6, bkh_list, nh_list, intf_list, dist_list, nh_vrf_list)
            cur_nh_set, cur_route_tag = self.static_routes.get(vrf, {}).get(ip_prefix, (IpNextHopSet(is_ipv6), route_tag))
            cmd_list = self.static_route_commands(ip_nh_set, cur_nh_set, ip_prefix, vrf, route_tag, cur_route_tag)
        except Exception as exc:
            log_crit("Got an exception %s: Traceback: %s" % (str(exc), traceback.format_exc()))
            return False

        # Enable redistribution of static routes when it is the first one get set
        if not self.static_routes.get(vrf, {}):
            if self.directory.path_exist("CONFIG_DB", swsscommon.CFG_DEVICE_METADATA_TABLE_NAME, "localhost/bgp_asn"):
                cmd_list.extend(self.enable_redistribution_command(vrf))
            else:
                self.vrf_pending_redistribution.add(vrf)

        if cmd_list:
            self.cfg_mgr.push_list(cmd_list)
            log_debug("{} Static route {} is scheduled for updates. {}".format(self.db_name, key, str(cmd_list)))
        else:
            log_debug("{} Nothing to update for static route {}".format(self.db_name, key))

        self.static_routes.setdefault(vrf, {})[ip_prefix] = (ip_nh_set, route_tag)

        return True


    def skip_appl_del(self, vrf, ip_prefix):
        """
        If a static route is bfd enabled, the processed static route is written into application DB by staticroutebfd.
        When we disable bfd for that route at runtime, that static route entry will be removed from APPL_DB STATIC_ROUTE_TABLE.
        In the case, the StaticRouteMgr(appl_db) cannot uninstall the static route from FRR if the static route is still in CONFIG_DB,
        so need this checking (skip appl_db deletion) to avoid race condition between StaticRouteMgr(appl_db) and StaticRouteMgr(config_db)
        For more detailed information:
        https://github.com/sonic-net/SONiC/blob/master/doc/static-route/SONiC_static_route_bfd_hld.md#bfd-field-changes-from-true-to-false
        but if the deletion is caused by no nexthop available (bfd field is true but all the sessions are down), need to allow this deletion.
        :param vrf: vrf from the split_key(key) return
        :param ip_prefix: ip_prefix from the split_key(key) return
        :return: True if the deletion comes from APPL_DB and the vrf|ip_prefix exists in CONFIG_DB, otherwise return False
        """
        if self.db_name == "CONFIG_DB":
            return False

        if self.config_db is None:
            self.config_db = swsscommon.SonicV2Connector()
            self.config_db.connect(self.config_db.CONFIG_DB)

        #just pop local cache if the route exist in config_db
        cfg_key = "STATIC_ROUTE|" + vrf + "|" + ip_prefix
        if vrf == "default":
            default_key = "STATIC_ROUTE|" + ip_prefix
            bfd = self.config_db.get(self.config_db.CONFIG_DB, default_key, "bfd")
            if bfd == "true":
                log_debug("skip_appl_del: {}, key {}, bfd flag {}".format(self.db_name, default_key, bfd))
                return False
        bfd = self.config_db.get(self.config_db.CONFIG_DB, cfg_key, "bfd")
        if bfd == "true":
            log_debug("skip_appl_del: {}, key {}, bfd flag {}".format(self.db_name, cfg_key, bfd))
            return False

        nexthop = self.config_db.get(self.config_db.CONFIG_DB, cfg_key, "nexthop")
        if nexthop and len(nexthop)>0:
            self.static_routes.setdefault(vrf, {}).pop(ip_prefix, None)
            return True

        if vrf == "default":
            cfg_key = "STATIC_ROUTE|" + ip_prefix
        nexthop = self.config_db.get(self.config_db.CONFIG_DB, cfg_key, "nexthop")
        if nexthop and len(nexthop)>0:
            self.static_routes.setdefault(vrf, {}).pop(ip_prefix, None)
            return True

        return False

    def del_handler(self, key):
        vrf, ip_prefix = self.split_key(key)
        is_ipv6 = TemplateFabric.is_ipv6(ip_prefix)

        if self.skip_appl_del(vrf, ip_prefix):
            log_debug("{} ignore appl_db static route deletion because of key {} exist in config_db and bfd is not true".format(self.db_name, key))
            return

        ip_nh_set = IpNextHopSet(is_ipv6)
        cur_nh_set, route_tag = self.static_routes.get(vrf, {}).get(ip_prefix, (IpNextHopSet(is_ipv6), self.ROUTE_ADVERTISE_DISABLE_TAG))
        cmd_list = self.static_route_commands(ip_nh_set, cur_nh_set, ip_prefix, vrf, route_tag, route_tag)

        # Disable redistribution of static routes when it is the last one to delete
        if self.static_routes.get(vrf, {}).keys() == {ip_prefix}:
            if self.directory.path_exist("CONFIG_DB", swsscommon.CFG_DEVICE_METADATA_TABLE_NAME, "localhost/bgp_asn"):
                cmd_list.extend(self.disable_redistribution_command(vrf))
            self.vrf_pending_redistribution.discard(vrf)

        if cmd_list:
            self.cfg_mgr.push_list(cmd_list)
            log_debug("{} Static route {} is scheduled for updates. {}".format(self.db_name, key, str(cmd_list)))
        else:
            log_debug("{} Nothing to update for static route {}".format(self.db_name, key))

        self.static_routes.setdefault(vrf, {}).pop(ip_prefix, None)

    @staticmethod
    def split_key(key):
        """
        Split key into vrf name and prefix.
        :param key: key to split
        :return: vrf name extracted from the key, ip prefix extracted from the key
        key example: APPL_DB   vrf:5.5.5.0/24, 5.5.5.0/24, vrf:2001::0/64, 2001::0/64
                     CONFIG_DB vrf|5.5.5.0/24, 5.5.5.0/24, vrf|2001::0/64, 2001::0/64
        """
        vrf = ""
        prefix = ""

        if '|' in key:
            return tuple(key.split('|', 1))
        else:
            try:
                _ = ip_network(key)
                vrf, prefix = 'default', key
            except ValueError:
                # key in APPL_DB
                log_debug("static route key {} is not prefix only formart, split with ':'".format(key))
                output = key.split(':', 1)
                if len(output) < 2:
                    log_debug("invalid input in APPL_DB {}".format(key))
                    raise ValueError
                vrf = output[0]
                prefix = key[len(vrf)+1:]
        return vrf, prefix

    def static_route_commands(self, ip_nh_set, cur_nh_set, ip_prefix, vrf, route_tag, cur_route_tag):
        op_cmd_list = {}
        if route_tag != cur_route_tag:
            for ip_nh in cur_nh_set:
                op_cmds = op_cmd_list.setdefault(self.OP_DELETE, [])
                op_cmds.append(self.generate_command(self.OP_DELETE, ip_nh, ip_prefix, vrf, cur_route_tag))
            for ip_nh in ip_nh_set:
                op_cmds = op_cmd_list.setdefault(self.OP_ADD, [])
                op_cmds.append(self.generate_command(self.OP_ADD, ip_nh, ip_prefix, vrf, route_tag))
        else:
            diff_set = ip_nh_set.symmetric_difference(cur_nh_set)

            for ip_nh in diff_set:
                if ip_nh in cur_nh_set:
                    op = self.OP_DELETE
                else:
                    op = self.OP_ADD

                op_cmds = op_cmd_list.setdefault(op, [])
                op_cmds.append(self.generate_command(op, ip_nh, ip_prefix, vrf, route_tag))

        cmd_list = op_cmd_list.get(self.OP_DELETE, [])
        cmd_list += op_cmd_list.get(self.OP_ADD, [])

        return cmd_list

    def generate_command(self, op, ip_nh, ip_prefix, vrf, route_tag):
        return '{}{} route {}{}{}{}'.format(
            'no ' if op == self.OP_DELETE else '',
            'ipv6' if ip_nh.af == socket.AF_INET6 else 'ip',
            ip_prefix,
            ip_nh,
            ' vrf {}'.format(vrf) if vrf != 'default' else '',
            ' tag {}'.format(route_tag)
        )

    def enable_redistribution_command(self, vrf):
        log_debug("Enabling static route redistribution")
        cmd_list = []
        cmd_list.append("route-map STATIC_ROUTE_FILTER permit 10")
        cmd_list.append(" match tag %s" % self.ROUTE_ADVERTISE_ENABLE_TAG)
        bgp_asn = self.directory.get_slot("CONFIG_DB", swsscommon.CFG_DEVICE_METADATA_TABLE_NAME)["localhost"]["bgp_asn"]
        if vrf == 'default':
            cmd_list.append("router bgp %s" % bgp_asn)
        else:
            cmd_list.append("router bgp %s vrf %s" % (bgp_asn, vrf))
        for af in ["ipv4", "ipv6"]:
            cmd_list.append(" address-family %s" % af)
            cmd_list.append("  redistribute static route-map STATIC_ROUTE_FILTER")
            cmd_list.append(" exit-address-family")
        cmd_list.append("exit")
        return cmd_list

    def disable_redistribution_command(self, vrf):
        log_debug("Disabling static route redistribution")
        cmd_list = []
        bgp_asn = self.directory.get_slot("CONFIG_DB", swsscommon.CFG_DEVICE_METADATA_TABLE_NAME)["localhost"]["bgp_asn"]
        if vrf == 'default':
            cmd_list.append("router bgp %s" % bgp_asn)
        else:
            cmd_list.append("router bgp %s vrf %s" % (bgp_asn, vrf))
        for af in ["ipv4", "ipv6"]:
            cmd_list.append(" address-family %s" % af)
            cmd_list.append("  no redistribute static route-map STATIC_ROUTE_FILTER")
            cmd_list.append(" exit-address-family")
        cmd_list.append("exit")
        cmd_list.append("no route-map STATIC_ROUTE_FILTER")
        return cmd_list

    def on_bgp_asn_change(self):
        if self.directory.path_exist("CONFIG_DB", swsscommon.CFG_DEVICE_METADATA_TABLE_NAME, "localhost/bgp_asn"):
            for vrf in self.vrf_pending_redistribution:
                self.cfg_mgr.push_list(self.enable_redistribution_command(vrf))
            self.vrf_pending_redistribution.clear()

class IpNextHop:
    def __init__(self, af_id, blackhole, dst_ip, if_name, dist, vrf):
        zero_ip = lambda af: '0.0.0.0' if af == socket.AF_INET else '::'
        self.af = af_id
        self.blackhole = 'false' if blackhole is None or blackhole == '' else blackhole
        self.distance = 0 if dist is None else int(dist)
        if self.blackhole == 'true':
            dst_ip = if_name = vrf = None
        self.ip = zero_ip(af_id) if dst_ip is None or dst_ip == '' else dst_ip
        self.interface = '' if if_name is None else if_name
        self.nh_vrf = '' if vrf is None else vrf
        if not self.is_portchannel():
            self.is_ip_valid()
        if self.blackhole != 'true' and self.is_zero_ip() and not self.is_portchannel() and len(self.interface.strip()) == 0:
            log_err('Mandatory attribute not found for nexthop')
            raise ValueError
    def __eq__(self, other):
        return (self.af == other.af and self.blackhole == other.blackhole and
                self.ip == other.ip and self.interface == other.interface and
                self.distance == other.distance and self.nh_vrf == other.nh_vrf)
    def __ne__(self, other):
        return (self.af != other.af or self.blackhole != other.blackhole or
                self.ip != other.ip or self.interface != other.interface or
                self.distance != other.distance or self.nh_vrf != other.nh_vrf)
    def __hash__(self):
        return hash((self.af, self.blackhole, self.ip, self.interface, self.distance, self.nh_vrf))
    def is_ip_valid(self):
        socket.inet_pton(self.af, self.ip)
    def is_zero_ip(self):
        try:
            return sum([x for x in socket.inet_pton(self.af, self.ip)]) == 0
        except socket.error:
            return False
    def is_portchannel(self):
        return True if self.ip.startswith('PortChannel') else False
    def __format__(self, format):
        ret_val = ''
        if self.blackhole == 'true':
            ret_val += ' blackhole'
        if not (self.ip is None or self.is_zero_ip()):
            ret_val += ' %s' % self.ip
        if not (self.interface is None or self.interface == ''):
            ret_val += ' %s' % self.interface
        if not (self.distance is None or self.distance == 0):
            ret_val += ' %d' % self.distance
        if not (self.nh_vrf is None or self.nh_vrf == ''):
            ret_val += ' nexthop-vrf %s' % self.nh_vrf
        return ret_val

class IpNextHopSet(set):
    def __init__(self, is_ipv6, bkh_list = None, ip_list = None, intf_list = None, dist_list = None, vrf_list = None):
        super(IpNextHopSet, self).__init__()
        af = socket.AF_INET6 if is_ipv6 else socket.AF_INET
        if bkh_list is None and ip_list is None and intf_list is None:
            # empty set, for delete case
            return
        nums = {len(x) for x in [bkh_list, ip_list, intf_list, dist_list, vrf_list] if x is not None}
        if len(nums) != 1:
            log_err("Lists of next-hop attribute have different sizes: %s" % nums)
            for x in [bkh_list, ip_list, intf_list, dist_list, vrf_list]:
                log_debug("List: %s" % x)
            raise ValueError
        nh_cnt = nums.pop()
        item = lambda lst, i: lst[i] if lst is not None else None
        for idx in range(nh_cnt):
            try:
                self.add(IpNextHop(af, item(bkh_list, idx), item(ip_list, idx), item(intf_list, idx),
                                   item(dist_list, idx), item(vrf_list, idx), ))
            except ValueError:
                continue
