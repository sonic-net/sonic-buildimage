from .log import log_err, log_debug, log_warn, log_notice
from .manager import Manager
from ipaddress import IPv6Address

SRV6_GLOBAL_TABLE_NAME = "SRV6_GLOBAL"

class SRv6GlobalCfgMgr(Manager):
    """ This class updates SRv6 configurations when SRV6_GLOBAL table is updated """

    SRV6_ENCAP_SRC_ADDR_DEFAULTS = "::"

    def __init__(self, common_objs, db, table):
        """
        Initialize the object
        :param common_objs: common object dictionary
        :param db: name of the db
        :param table: name of the table in the db
        """
        super(SRv6GlobalCfgMgr, self).__init__(
            common_objs,
            set(),
            db,
            table,
            wait_for_all_deps=False
        )

        # By default the encapsulation source address is not set
        if not self.directory.path_exist(self.db_name, self.table_name, "encap_src_addr"):
            self.directory.put(self.db_name, self.table_name, "encap_src_addr", self.SRV6_ENCAP_SRC_ADDR_DEFAULTS)

    def set_handler(self, key, data):
        """ Handle SRv6 global configuration change """
        log_debug("SRv6GlobalCfgMgr:: set handler")

        # Source address for SRv6 encapsulation
        ret = self.configure_srv6_encap_src_addr(data)

        return ret

    def del_handler(self, key):
        """ Handle SRv6 global configuration clear """
        log_debug("SRv6GlobalCfgMgr:: del handler")

        # Source address for SRv6 encapsulation
        ret = self.configure_srv6_encap_src_addr(None)

        return ret

    def is_update_required(self, key, value):
        if self.directory.path_exist(self.db_name, self.table_name, key):
            return value != self.directory.get(self.db_name, self.table_name, key)
        return True

    def configure_srv6_encap_src_addr(self, data=None):
        """ Configure SRv6 encapsulation source address """

        encap_src_addr = self.SRV6_ENCAP_SRC_ADDR_DEFAULTS

        if data is not None:
            encap_src_addr = data.get("encap_src_addr")

        if self.is_update_required("encap_src_addr", encap_src_addr):
            if self.set_srv6_encap_src_addr(encap_src_addr):
                self.directory.put(self.db_name, self.table_name, "encap_src_addr", encap_src_addr)
            else:
                log_err("SRv6GlobalCfgMgr:: Failed to set SRv6 encapsulation source address to {}".format(encap_src_addr))
                return False
        else:
            log_notice("SRv6GlobalCfgMgr:: SRv6 encapsulation source address configuration is up-to-date")

        return True

    def set_srv6_encap_src_addr(self, encap_src_addr):
        """ API to set/unset SRv6 encapsulation source address """

        try:
            IPv6Address(encap_src_addr)
        except ValueError:
            log_err("SRv6GlobalCfgMgr:: Invalid SRv6 encapsulation source address: {}".format(encap_src_addr))
            return False

        if encap_src_addr != self.SRV6_ENCAP_SRC_ADDR_DEFAULTS:
            log_debug("SRv6GlobalCfgMgr:: Setting SRv6 encapsulation source address to {}".format(encap_src_addr))
        else:
            log_notice("SRv6GlobalCfgMgr:: Restoring default SRv6 encapsulation source address")

        cmd_list = ["segment-routing", "srv6", "encapsulation", "source-address {}".format(encap_src_addr)]

        self.cfg_mgr.push_list(cmd_list)
        log_debug("SRv6GlobalCfgMgr::Done")

        return True
