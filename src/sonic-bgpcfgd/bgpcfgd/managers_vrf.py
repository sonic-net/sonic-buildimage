from .log import log_err, log_debug, log_warn
from .manager import Manager

class VRFMgr(Manager):
    """ This class populates the Directory with VRF table data from APPL_DB.
        Other managers (e.g. SRv6) can use this to check VRF dependency before
        pushing configuration that requires the VRF to exist.
    """
    def __init__(self, common_objs, db, table):
        """
        Initialize the object
        :param common_objs: common object dictionary
        :param db: name of the db
        :param table: name of the table in the db
        """
        super(VRFMgr, self).__init__(
            common_objs,
            [],
            db,
            table,
        )

    def set_handler(self, key, data):
        """ Implementation of 'SET' command - store VRF in directory """
        log_debug("VRFMgr VRF config: {} | {}".format(key, data))
        self.directory.put(self.db_name, self.table_name, key, data)
        return True

    def del_handler(self, key):
        """ Implementation of 'DEL' command - remove VRF from directory """
        log_debug("VRFMgr VRF de-config: {} | {}".format(key, data))
        self.directory.remove(self.db_name, self.table_name, key)
