from swsscommon import swsscommon

from .log import log_info, log_err
from .manager import Manager
from .managers_bbr import BGP_BBR_TABLE_NAME, BGP_BBR_STATUS_KEY, BGP_BBR_STATUS_ENABLED, BGP_BBR_STATUS_DISABLED

CONFIG_DB_NAME = "CONFIG_DB"
BGP_AGGREGATE_ADDRESS_TABLE_NAME = "BGP_AGGREGATE_ADDRESS"
BBR_REQUIRED_KEY = "bbr-required"
AS_SET_KEY = "as-set"
SUMMARY_ONLY_KEY = "summary-only"
AGGREGATE_ADDRESS_PREFIX_LIST_KEY = "aggregate-address-prefix-list"
CONTRIBUTING_ADDRESS_PREFIX_LIST_KEY = "contributing-address-prefix-list"
COMMON_TRUE_STRING = "true"
COMMON_FALSE_STRING = "false"
ADDRESS_STATE_KEY = "state"
ADDRESS_ACTIVE_STATE = "active"
ADDRESS_INACTIVE_STATE = "inactive"


class AggregateAddressMgr(Manager):
    """ This class is to subscribe BGP_AGGREGATE_ADDRESS in CONFIG_DB """

    def __init__(self, common_objs, db, table):
        """
        Initialize the object
        :param common_objs: common object dictionary
        :param db: name of the db
        :param table: name of the table in the db
        """
        super(AggregateAddressMgr, self).__init__(
            common_objs,
            [
                ("CONFIG_DB", swsscommon.CFG_DEVICE_METADATA_TABLE_NAME, "localhost/bgp_asn"),
            ],
            db,
            table,
        )
        self.directory.subscribe([(CONFIG_DB_NAME, BGP_BBR_TABLE_NAME, BGP_BBR_STATUS_KEY)], self.on_bbr_change)
        self.state_db_conn = common_objs['state_db_conn']
        self.address_table = swsscommon.Table(self.state_db_conn, BGP_AGGREGATE_ADDRESS_TABLE_NAME)
        self.remove_all_state_of_address()

    def on_bbr_change(self):
        bbr_status = self.directory.get(CONFIG_DB_NAME, BGP_BBR_TABLE_NAME, BGP_BBR_STATUS_KEY)
        addresses = self.get_addresses_from_state_db(bbr_required_only=True)
        if bbr_status == BGP_BBR_STATUS_ENABLED:
            log_info("AggregateAddressMgr::BBR state changed to %s with bbr_required addresses %s" % (bbr_status, addresses))
            for address in addresses:
                if self.address_set_handler(address[0], address[1]):
                    self.set_address_state(address[0], address[1], ADDRESS_ACTIVE_STATE)
        elif bbr_status == BGP_BBR_STATUS_DISABLED:
            log_info("AggregateAddressMgr::BBR state changed to %s with bbr_required addresses %s" % (bbr_status, addresses))
            for address in addresses:
                if self.address_del_handler(address[0], address[1]):
                    self.set_address_state(address[0], address[1], ADDRESS_INACTIVE_STATE)
        else:
            log_info("AggregateAddressMgr::BBR state changed to unknown with bbr_required addresses %s" % addresses)

    def set_handler(self, key, data):
        data = dict(data)
        bbr_status = self.directory.get(CONFIG_DB_NAME, BGP_BBR_TABLE_NAME, BGP_BBR_STATUS_KEY)
        if bbr_status not in (BGP_BBR_STATUS_ENABLED, BGP_BBR_STATUS_DISABLED):
            log_info("AggregateAddressMgr::BBR state is unknown. Skip the address %s" % key2prefix(key))
            self.set_address_state(key, data, ADDRESS_INACTIVE_STATE)
        elif bbr_status == BGP_BBR_STATUS_DISABLED and data.get(BBR_REQUIRED_KEY, COMMON_FALSE_STRING) == COMMON_TRUE_STRING:
            log_info("AggregateAddressMgr::BBR is disabled and bbr-required is set to true. Skip the address %s" % key2prefix(key))
            self.set_address_state(key, data, ADDRESS_INACTIVE_STATE)
        else:
            if self.address_set_handler(key, data):
                self.set_address_state(key, data, ADDRESS_ACTIVE_STATE)
            else:
                log_info("AggregateAddressMgr::set address %s failed" % key2prefix(key))
                self.set_address_state(key, data, ADDRESS_INACTIVE_STATE)
        return True

    def address_set_handler(self, key, data):
        bgp_asn = self.directory.get_slot(CONFIG_DB_NAME, swsscommon.CFG_DEVICE_METADATA_TABLE_NAME)["localhost"]["bgp_asn"]
        prefix = key2prefix(key)
        is_v4 = '.' in prefix
        cmd_list = []

        aggregates_cmds = generate_aggregate_address_commands(
            asn=bgp_asn,
            prefix=prefix,
            is_v4=is_v4,
            is_remove=False,
            summary_only=data.get(SUMMARY_ONLY_KEY, COMMON_FALSE_STRING),
            as_set=data.get(AS_SET_KEY, COMMON_FALSE_STRING)
        )
        cmd_list.extend(aggregates_cmds)

        if AGGREGATE_ADDRESS_PREFIX_LIST_KEY in data and data[AGGREGATE_ADDRESS_PREFIX_LIST_KEY]:
            append_agg_address_cmd = generate_prefix_list_commands(
                prefix_list_name=data[AGGREGATE_ADDRESS_PREFIX_LIST_KEY],
                prefix=prefix,
                is_v4=is_v4,
                is_con=False,
                is_remove=False
            )
            cmd_list.extend(append_agg_address_cmd)

        if CONTRIBUTING_ADDRESS_PREFIX_LIST_KEY in data and data[CONTRIBUTING_ADDRESS_PREFIX_LIST_KEY]:
            append_con_address_cmd = generate_prefix_list_commands(
                prefix_list_name=data[CONTRIBUTING_ADDRESS_PREFIX_LIST_KEY],
                prefix=prefix,
                is_v4=is_v4,
                is_con=True,
                is_remove=False
            )
            cmd_list.extend(append_con_address_cmd)

        log_info("AggregateAddressMgr::cmd_list: %s" % cmd_list)
        self.cfg_mgr.push_list(cmd_list)
        return True

    def del_handler(self, key):
        address_state = self.get_address_from_state_db(key)
        if self.address_del_handler(key, address_state):
            log_info("AggregateAddressMgr::delete address %s success" % key)
            self.del_address_state(key)
        return True

    def address_del_handler(self, key, data):
        bgp_asn = self.directory.get_slot(CONFIG_DB_NAME, swsscommon.CFG_DEVICE_METADATA_TABLE_NAME)["localhost"]["bgp_asn"]
        prefix = key2prefix(key)
        is_v4 = '.' in prefix
        cmd_list = []

        aggregates_cmds = generate_aggregate_address_commands(
            asn=bgp_asn,
            prefix=prefix,
            is_v4=is_v4,
            is_remove=True
        )
        cmd_list.extend(aggregates_cmds)

        if AGGREGATE_ADDRESS_PREFIX_LIST_KEY in data and data[AGGREGATE_ADDRESS_PREFIX_LIST_KEY]:
            rm_agg_address_cmds = generate_prefix_list_commands(
                prefix_list_name=data[AGGREGATE_ADDRESS_PREFIX_LIST_KEY],
                prefix=prefix,
                is_v4=is_v4,
                is_con=False,
                is_remove=True
            )
            cmd_list.extend(rm_agg_address_cmds)

        if CONTRIBUTING_ADDRESS_PREFIX_LIST_KEY in data and data[CONTRIBUTING_ADDRESS_PREFIX_LIST_KEY]:
            rm_con_address_cmds = generate_prefix_list_commands(
                prefix_list_name=data[CONTRIBUTING_ADDRESS_PREFIX_LIST_KEY],
                prefix=prefix,
                is_v4=is_v4,
                is_con=True,
                is_remove=True
            )
            cmd_list.extend(rm_con_address_cmds)

        log_info("AggregateAddressMgr::cmd_list: %s" % cmd_list)
        self.cfg_mgr.push_list(cmd_list)
        return True

    def get_addresses_from_state_db(self, bbr_required_only=False):
        addresses = []
        for key in self.address_table.getKeys():
            data = self.get_address_from_state_db(key)
            if not bbr_required_only or data[BBR_REQUIRED_KEY] == COMMON_TRUE_STRING:
                addresses.append((key, data))
        return addresses

    def get_address_from_state_db(self, key):
        (success, data) = self.address_table.get(key)
        if not success:
            log_err("AggregateAddressMgr::Failed to get data from state db for key %s" % key)
            return {}
        data = dict(data)
        return data

    def remove_all_state_of_address(self):
        for address in list(self.address_table.getKeys()):
            self.address_table.delete(address)
        log_info("AggregateAddressMgr::All the state of aggregate address is removed")
        return True

    def set_address_state(self, key, data, address_state):
        self.address_table.hset(key, BBR_REQUIRED_KEY, data.get(BBR_REQUIRED_KEY, COMMON_FALSE_STRING))
        self.address_table.hset(key, SUMMARY_ONLY_KEY, data.get(SUMMARY_ONLY_KEY, COMMON_FALSE_STRING))
        self.address_table.hset(key, AS_SET_KEY, data.get(AS_SET_KEY, COMMON_FALSE_STRING))
        self.address_table.hset(key, AGGREGATE_ADDRESS_PREFIX_LIST_KEY, data.get(AGGREGATE_ADDRESS_PREFIX_LIST_KEY, ""))
        self.address_table.hset(key, CONTRIBUTING_ADDRESS_PREFIX_LIST_KEY, data.get(CONTRIBUTING_ADDRESS_PREFIX_LIST_KEY, ""))
        self.address_table.hset(key, ADDRESS_STATE_KEY, address_state)
        log_info("AggregateAddressMgr::State of aggregate address %s is set with bbr_required %s and state %s " % (key, data.get(BBR_REQUIRED_KEY, COMMON_FALSE_STRING), address_state))

    def del_address_state(self, key):
        self.address_table.delete(key)
        log_info("AggregateAddressMgr::State of aggregate address %s is removed" % key)


def key2prefix(key):
    prefix = key.split("|")[-1]
    return prefix


def generate_aggregate_address_commands(asn, prefix, is_v4, is_remove, summary_only=COMMON_FALSE_STRING, as_set=COMMON_FALSE_STRING):
    ret_cmds = []
    ret_cmds.append("router bgp %s" % asn)
    ret_cmds.append("address-family ipv4" if is_v4 else "address-family ipv6")
    agg_cmd = "no " if is_remove else ""
    agg_cmd += "aggregate-address %s" % prefix
    if not is_remove and summary_only == COMMON_TRUE_STRING:
        agg_cmd += " %s" % SUMMARY_ONLY_KEY
    if not is_remove and as_set == COMMON_TRUE_STRING:
        agg_cmd += " %s" % AS_SET_KEY
    ret_cmds.append(agg_cmd)
    ret_cmds.append("exit-address-family")
    ret_cmds.append("exit")
    return ret_cmds


def generate_prefix_list_commands(prefix_list_name, prefix, is_v4, is_con, is_remove):
    ret_cmds = []
    prefix_list_cmd = "no " if is_remove else ""
    prefix_list_cmd += "ip" if is_v4 else "ipv6"
    prefix_list_cmd += " prefix-list %s" % prefix_list_name
    prefix_list_cmd += " permit %s" % prefix
    if is_con:
        prefix_list_cmd += " le" + (" 32" if is_v4 else " 128")
    ret_cmds.append(prefix_list_cmd)
    return ret_cmds
