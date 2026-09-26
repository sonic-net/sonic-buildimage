from swsscommon import swsscommon
from sonic_py_common.bgp import validate_asn

from .log import log_err
from .manager import Manager


class BGPDataBaseMgr(Manager):
    """ This class updates the Directory object when db table is updated """
    def __init__(self, common_objs, db, table):
        """
        Initialize the object
        :param common_objs: common object dictionary
        :param db: name of the db
        :param table: name of the table in the db
        """
        super(BGPDataBaseMgr, self).__init__(
            common_objs,
            [],
            db,
            table,
        )

    def set_handler(self, key, data):
        """ Implementation of 'SET' command for this class """
        if (self.table_name == swsscommon.CFG_DEVICE_METADATA_TABLE_NAME
                and key == "localhost" and data and "bgp_asn" in data
                and not self.__is_valid_asn(data["bgp_asn"])):
            log_err("BGPDataBaseMgr::Invalid bgp_asn value: %r. Row ignored" % data["bgp_asn"])
            return True

        self.directory.put(self.db_name, self.table_name, key, data)

        return True

    def del_handler(self, key):
        """ Implementation of 'DEL' command for this class """
        self.directory.remove(self.db_name, self.table_name, key)

    @staticmethod
    def __is_valid_asn(value):
        """Return True when the common BGP ASN validator accepts *value*."""
        # CONFIG_DB values are strings. Keep rejecting non-string values here
        # while sharing the syntax and range checks with other callers that
        # legitimately receive integers (for example, template rendering).
        if not isinstance(value, str):
            return False
        try:
            validate_asn(value)
        except ValueError:
            return False
        return True
