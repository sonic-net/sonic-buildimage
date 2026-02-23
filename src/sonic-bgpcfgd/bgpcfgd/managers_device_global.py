import re
import jinja2

from .manager import Manager
from .log import log_err, log_debug, log_notice
from swsscommon import swsscommon
from sonic_py_common import device_info

class DeviceGlobalCfgMgr(Manager):
    """This class responds to change in device-specific state"""

    TSA_DEFAULTS = "false"
    WCMP_DEFAULTS = "false"
    IDF_DEFAULTS = "unisolated"

    def __init__(self, common_objs, db, table):
        """
        Initialize the object
        :param common_objs: common object dictionary
        :param db: name of the db
        :param table: name of the table in the db
        """        
        self.switch_role = ""
        self.chassis_tsa = ""
        self.directory = common_objs['directory']
        self.cfg_mgr = common_objs['cfg_mgr']
        self.constants = common_objs['constants']
        self.tsa_template = common_objs['tf'].from_file("bgpd/tsa/bgpd.tsa.isolate.conf.j2")
        self.tsb_template = common_objs['tf'].from_file("bgpd/tsa/bgpd.tsa.unisolate.conf.j2")
        self.wcmp_template = common_objs['tf'].from_file("bgpd/wcmp/bgpd.wcmp.conf.j2")
        self.idf_isolate_template = common_objs['tf'].from_file("bgpd/idf_isolate/idf_isolate.conf.j2")
        self.idf_unisolate_template = common_objs['tf'].from_file("bgpd/idf_isolate/idf_unisolate.conf.j2")        
        self.directory.subscribe([("CONFIG_DB", swsscommon.CFG_DEVICE_METADATA_TABLE_NAME, "localhost/type"),], self.handle_type_update)
        super(DeviceGlobalCfgMgr, self).__init__(
            common_objs,
            [],
            db,
            table,
        )

        # By default TSA feature is disabled
        if not self.directory.path_exist(self.db_name, self.table_name, "tsa_enabled"):
            self.directory.put(self.db_name, self.table_name, "tsa_enabled", self.TSA_DEFAULTS)
        # By default W-ECMP feature is disabled
        if not self.directory.path_exist(self.db_name, self.table_name, "wcmp_enabled"):
            self.directory.put(self.db_name, self.table_name, "wcmp_enabled", self.WCMP_DEFAULTS)
        # By default IDF feature is unisolated
        if not self.directory.path_exist(self.db_name, self.table_name, "idf_isolation_state"):
            self.directory.put(self.db_name, self.table_name, "idf_isolation_state", self.IDF_DEFAULTS)

    def handle_type_update(self):
        log_debug("DeviceGlobalCfgMgr:: Switch role update handler")
        if self.directory.path_exist("CONFIG_DB", swsscommon.CFG_DEVICE_METADATA_TABLE_NAME, "localhost/type"):
            self.switch_role = self.directory.get_slot("CONFIG_DB", swsscommon.CFG_DEVICE_METADATA_TABLE_NAME)["localhost"]["type"]
        log_debug("DeviceGlobalCfgMgr:: Switch role: %s" % self.switch_role)

    def set_handler(self, key, data):
        """ Handle device TSA/W-ECMP state change """
        log_debug("DeviceGlobalCfgMgr:: set handler")

        if not data:
            log_err("DeviceGlobalCfgMgr:: data is None")
            return False

        # TSA configuration
        self.configure_tsa(data)
        # W-ECMP configuration
        self.configure_wcmp(data)
        # IDF configuration
        self.configure_idf(data)

        return True

    def del_handler(self, key):
        log_debug("DeviceGlobalCfgMgr:: del handler")

        # TSA configuration
        self.configure_tsa()
        # W-ECMP configuration
        self.configure_wcmp()
        # IDF configuration
        self.configure_idf()

        return True

    def is_update_required(self, key, value):
        if self.directory.path_exist(self.db_name, self.table_name, key):
            return value != self.directory.get(self.db_name, self.table_name, key)
        return True

    def configure_tsa(self, data=None):
        """ Configure TSA feature"""

        state = self.TSA_DEFAULTS

        if data is not None:
            if "tsa_enabled" in data:
                state = data["tsa_enabled"]

        self.chassis_tsa = self.get_chassis_tsa_status()
        requires_update = self.is_update_required("tsa_enabled", state)

        if state in ["true", "false"] and self.directory.path_exist(self.db_name, self.table_name, "tsa_enabled"):
            self.directory.put(self.db_name, self.table_name, "tsa_enabled", state)

        if requires_update and self.chassis_tsa == "false":
            self.cfg_mgr.commit()
            self.cfg_mgr.update()
            self.isolate_unisolate_device(state)
        else:
            log_notice("DeviceGlobalCfgMgr:: TSA configuration is up-to-date")

    def configure_wcmp(self, data=None):
        """ Configure W-ECMP feature"""

        state = self.WCMP_DEFAULTS

        if data is not None:
            if "wcmp_enabled" in data:
                state = data["wcmp_enabled"]

        if self.is_update_required("wcmp_enabled", state):
            if self.set_wcmp(state):
                self.directory.put(self.db_name, self.table_name, "wcmp_enabled", state)
        else:
            log_notice("DeviceGlobalCfgMgr:: W-ECMP configuration is up-to-date")

    def configure_idf(self, data=None):
        """ Configure IDF feature"""

        state = self.IDF_DEFAULTS

        if data is not None:
            if "idf_isolation_state" in data:
                state = data["idf_isolation_state"]

        if self.is_update_required("idf_isolation_state", state):
            if self.downstream_isolate_unisolate(state):
                self.directory.put(self.db_name, self.table_name, "idf_isolation_state", state)
        else:
            log_notice("DeviceGlobalCfgMgr:: IDF configuration is up-to-date")

    def set_wcmp(self, status):
        """ API to set/unset W-ECMP """

        if status not in ["true", "false"]:
            log_err("W-ECMP: invalid value({}) is provided".format(status))
            return False

        if status == "true":
            log_notice("DeviceGlobalCfgMgr:: Enabling W-ECMP...")
        else:
            log_notice("DeviceGlobalCfgMgr:: Disabling W-ECMP...")

        cmd = "\n"

        try:
            cmd += self.wcmp_template.render(wcmp_enabled=status)
        except jinja2.TemplateError as e:
            msg = "W-ECMP: error in template rendering"
            log_err("%s: %s" % (msg, str(e)))
            return False

        self.cfg_mgr.push(cmd)

        log_debug("DeviceGlobalCfgMgr::Done")

        return True

    def check_state_and_get_tsa_routemaps(self, cfg):
        """ API to get TSA route-maps if device is isolated"""
        cmd = ""
        if self.directory.path_exist("CONFIG_DB", swsscommon.CFG_BGP_DEVICE_GLOBAL_TABLE_NAME, "tsa_enabled"):
            tsa_status = self.directory.get_slot("CONFIG_DB", swsscommon.CFG_BGP_DEVICE_GLOBAL_TABLE_NAME)["tsa_enabled"]
            chassis_tsa = self.get_chassis_tsa_status()

            if tsa_status == "true" or chassis_tsa == "true":
                cmds = cfg.replace("#012", "\n").split("\n")
                log_notice("DeviceGlobalCfgMgr:: Device is isolated. Applying TSA route-maps")
                cmd = self.get_ts_routemaps(cmds, self.tsa_template)
        return cmd

    def isolate_unisolate_device(self, tsa_status):
        """ API to get TSA/TSB route-maps and apply configuration"""

        if tsa_status not in ["true", "false"]:
            log_err("TSA: invalid value({}) is provided".format(tsa_status))
            return False

        cmd = "\n"
        if tsa_status == "true":
            log_notice("DeviceGlobalCfgMgr:: Device isolated. Executing TSA")
            cmd += self.get_ts_routemaps(self.cfg_mgr.get_text(), self.tsa_template)
        else:
            log_notice("DeviceGlobalCfgMgr:: Device un-isolated. Executing TSB")
            cmd += self.get_ts_routemaps(self.cfg_mgr.get_text(), self.tsb_template)

        self.cfg_mgr.push(cmd)
        log_debug("DeviceGlobalCfgMgr::Done")

        return True

    def get_ts_routemaps(self, cmds, ts_template):
        if not cmds:
            return ""

        route_map_names = self.__extract_out_route_map_names(cmds)
        return self.__generate_routemaps_from_template(route_map_names, ts_template)

    def __generate_routemaps_from_template(self, route_map_names, template):
        cmd = "\n"
        for rm in sorted(route_map_names):
            # For packet-based chassis, the bgp session between the linecards are also considered internal sessions
            # While isolating a single linecard, these sessions should not be skipped
            if "_INTERNAL_" in rm or "VOQ_" in rm:
                is_internal="1"
            else:
                is_internal="0"
            if "V4" in rm:
                ipv="V4" ; ipp="ip"
            elif "V6" in rm:
                ipv="V6" ; ipp="ipv6"
            else:
                continue
            cmd += template.render(route_map_name=rm,ip_version=ipv,ip_protocol=ipp,internal_route_map=is_internal, constants=self.constants)
            cmd += "\n"
        return cmd

    def __extract_out_route_map_names(self, cmds):
        route_map_names = set()
        out_route_map = re.compile(r'^\s*neighbor \S+ route-map (\S+) out$')
        for line in cmds:
            result = out_route_map.match(line)
            if result:
                route_map_names.add(result.group(1))
        return route_map_names

    def get_chassis_tsa_status(self):
        chassis_tsa_status = "false"

        if not device_info.is_chassis():
            return chassis_tsa_status

        try:
            ch = swsscommon.SonicV2Connector(use_unix_socket_path=False)
            ch.connect(ch.CHASSIS_APP_DB, False)
            chassis_tsa_status = ch.get(ch.CHASSIS_APP_DB, "BGP_DEVICE_GLOBAL|STATE", 'tsa_enabled')
        except Exception as e:
            log_err("Got an exception {}".format(e))

        return chassis_tsa_status

    def downstream_isolate_unisolate(self, idf_isolation_state):
        """ API to apply IDF configuration """

        if idf_isolation_state not in ["unisolated", "isolated_withdraw_all", "isolated_no_export"]:
            log_err("IDF: invalid value({}) is provided".format(idf_isolation_state))
            return False

        if self.switch_role and self.switch_role not in ["SpineRouter", "LowerSpineRouter", "UpperSpineRouter"]:
            log_debug("DeviceGlobalCfgMgr:: Skipping IDF isolation configuration on %s" % self.switch_role)
            return True

        cmd = "\n"
        if idf_isolation_state == "unisolated":
            cmd += self.idf_unisolate_template.render(constants=self.constants)
            log_notice("DeviceGlobalCfgMgr:: IDF un-isolated")
        else:
            cmd += self.idf_isolate_template.render(isolation_status=idf_isolation_state, constants=self.constants)
            log_notice("DeviceGlobalCfgMgr:: IDF isolated, {} policy applied".format(idf_isolation_state))

        self.cfg_mgr.push(cmd)
        log_debug("DeviceGlobalCfgMgr::Done")

        return True

    def check_state_and_get_idf_isolation_routemaps(self):
        """ API to get TSA route-maps if device is isolated"""

        cmd = ""
        if self.directory.path_exist("CONFIG_DB", swsscommon.CFG_BGP_DEVICE_GLOBAL_TABLE_NAME, "idf_isolation_state"):
            idf_isolation_state = self.directory.get_slot("CONFIG_DB", swsscommon.CFG_BGP_DEVICE_GLOBAL_TABLE_NAME)["idf_isolation_state"]
            if idf_isolation_state != "unisolated":
                log_notice("DeviceGlobalCfgMgr:: IDF is isolated. Applying required route-maps")
                cmd = self.idf_isolate_template.render(isolation_status=idf_isolation_state, constants=self.constants)

        return cmd
