from .manager import Manager
from .managers_device_global import DeviceGlobalCfgMgr
from .log import log_err, log_debug, log_notice
import re
from swsscommon import swsscommon

class ChassisAppDbMgr(Manager):
    """This class responds to change in tsa_enabled state of the supervisor"""

    def __init__(self, common_objs, db, table):
        """
        Initialize the object
        :param common_objs: common object dictionary
        :param db: name of the db
        :param table: name of the table in the db
        """
        self.lc_tsa = ""
        self.directory = common_objs['directory']
        self.dev_cfg_mgr = DeviceGlobalCfgMgr(common_objs, "CONFIG_DB", swsscommon.CFG_BGP_DEVICE_GLOBAL_TABLE_NAME)
        self.directory.subscribe([("CONFIG_DB", swsscommon.CFG_BGP_DEVICE_GLOBAL_TABLE_NAME, "tsa_enabled"),], self.on_lc_tsa_status_change)
        super(ChassisAppDbMgr, self).__init__(
            common_objs,
            [],
            db,
            table,
        )

    def on_lc_tsa_status_change(self):
        if self.directory.path_exist("CONFIG_DB", swsscommon.CFG_BGP_DEVICE_GLOBAL_TABLE_NAME, "tsa_enabled"):
            self.lc_tsa = self.directory.get_slot("CONFIG_DB", swsscommon.CFG_BGP_DEVICE_GLOBAL_TABLE_NAME)["tsa_enabled"]
        log_debug("ChassisAppDbMgr:: LC TSA update handler status %s" % self.lc_tsa)

    def set_handler(self, key, data):
        log_debug("ChassisAppDbMgr:: set handler")

        if not data:
            log_err("ChassisAppDbMgr:: data is None")
            return False

        if "tsa_enabled" in data:
            if self.lc_tsa == "false":
                self.dev_cfg_mgr.cfg_mgr.commit()
                self.dev_cfg_mgr.cfg_mgr.update()
                self.dev_cfg_mgr.isolate_unisolate_device(data["tsa_enabled"])
            return True
        return False

    def del_handler(self, key):
        log_debug("ChassisAppDbMgr:: del handler")
        return True
