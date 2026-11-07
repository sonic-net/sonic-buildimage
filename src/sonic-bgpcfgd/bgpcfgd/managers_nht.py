import jinja2
from swsscommon import swsscommon

from .log import log_err, log_info, log_warn
from .manager import Manager


class NhtMgr(Manager):
    """
    Manager for the NEXTHOP_TRACKING table.

    Key: "vrf_name|afi". Field: resolve_via_default ("true" | "false").
    Renders zebra/zebra.nht.db.conf.j2 and pushes it to FRR.

    Handles the default VRF and user VRFs. An absent field, or a deleted
    entry, applies the platform default so that zebra always reflects
    CONFIG_DB.
    """

    DEFAULT_VRF = 'default'
    AFIS = ('ipv4', 'ipv6')

    def __init__(self, common_objs, db, table):
        """
        :param common_objs: common object dictionary
        :param db: name of the db
        :param table: name of the table in the db
        """
        super(NhtMgr, self).__init__(
            common_objs,
            [],
            db,
            table,
        )
        self.nht_template = common_objs['tf'].from_file("zebra/zebra.nht.db.conf.j2")

    def platform_default(self, vrf_name, afi):
        """
        The value zebra holds when CONFIG_DB carries no resolve_via_default.
        Mirrors zebra.interfaces.conf.j2 for the default VRF and zebra's
        compiled-in default for user VRFs.
        """
        if vrf_name != self.DEFAULT_VRF:
            return 'true'
        if afi == 'ipv6':
            return 'false'
        return 'false' if self.__is_public_cloudtype() else 'true'

    def __is_public_cloudtype(self):
        table = swsscommon.CFG_DEVICE_METADATA_TABLE_NAME
        if not self.directory.path_exist("CONFIG_DB", table, "localhost/cloudtype"):
            return False
        cloudtype = self.directory.get_slot("CONFIG_DB", table)["localhost"]["cloudtype"]
        return str(cloudtype).lower() == 'public'

    def __parse_key(self, key, what):
        key_parts = key.split('|')
        if len(key_parts) != 2:
            log_err("NhtMgr: invalid key format '%s' on %s, expected 'vrf_name|afi'" % (key, what))
            return None, None
        vrf_name, afi = key_parts
        if not vrf_name:
            log_err("NhtMgr: empty vrf_name in key '%s' on %s" % (key, what))
            return None, None
        if afi not in self.AFIS:
            log_err("NhtMgr: invalid AFI '%s' in key '%s' on %s" % (afi, key, what))
            return None, None
        return vrf_name, afi

    def __push(self, vrf_name, afi, value, key):
        try:
            txt = self.nht_template.render(
                vrf_name=vrf_name,
                afi=afi,
                resolve_via_default=value
            )
        except jinja2.TemplateError as e:
            log_err("NhtMgr: error rendering template for key '%s': %s" % (key, str(e)))
            return False
        self.cfg_mgr.push(txt)
        log_info("NhtMgr: resolve_via_default=%s scheduled for (vrf=%s, afi=%s)" %
                 (value, vrf_name, afi))
        return True

    def set_handler(self, key, data):
        """
        Implementation of 'SET' command for the NEXTHOP_TRACKING table.

        :param key: "vrf_name|afi"
        :param data: dictionary that may carry 'resolve_via_default'
        :return: True on success
        """
        vrf_name, afi = self.__parse_key(key, "set")
        if vrf_name is None:
            return True

        value = data.get('resolve_via_default')
        if value is None:
            value = self.platform_default(vrf_name, afi)
        elif value not in ('true', 'false'):
            log_warn("NhtMgr: unexpected resolve_via_default value '%s' for key '%s', ignoring" %
                     (value, key))
            return True

        if self.__push(vrf_name, afi, value, key):
            self.directory.put(self.db_name, self.table_name, key, data)
        return True

    def del_handler(self, key):
        """
        Implementation of 'DEL' command for the NEXTHOP_TRACKING table.
        Restores the platform default for the (vrf_name, afi).

        :param key: "vrf_name|afi"
        :return: True on success
        """
        vrf_name, afi = self.__parse_key(key, "delete")
        if vrf_name is None:
            return True

        if self.__push(vrf_name, afi, self.platform_default(vrf_name, afi), key):
            self.directory.remove(self.db_name, self.table_name, key)
        return True
