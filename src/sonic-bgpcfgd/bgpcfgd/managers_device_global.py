from .manager import Manager
from .log import log_err, log_debug, log_notice
import re
from swsscommon import swsscommon

class DeviceGlobalCfgMgr(Manager):
    """This class responds to change in device-specific state"""

    def __init__(self, common_objs, db, table):
        """
        Initialize the object
        :param common_objs: common object dictionary
        :param db: name of the db
        :param table: name of the table in the db
        """
        self.switch_type = ""
        self.directory = common_objs['directory']
        self.cfg_mgr = common_objs['cfg_mgr']
        self.constants = common_objs['constants']
        self.tsa_template = common_objs['tf'].from_file("bgpd/tsa/bgpd.tsa.isolate.conf.j2")
        self.tsb_template = common_objs['tf'].from_file("bgpd/tsa/bgpd.tsa.unisolate.conf.j2")
        self.directory.subscribe([("CONFIG_DB", swsscommon.CFG_DEVICE_METADATA_TABLE_NAME, "localhost/switch_type"),], self.on_switch_type_change)
        super(DeviceGlobalCfgMgr, self).__init__(
            common_objs,
            [],
            db,
            table,
        )

    def on_switch_type_change(self):
        log_debug("DeviceGlobalCfgMgr:: Switch type update handler")
        if self.directory.path_exist("CONFIG_DB", swsscommon.CFG_DEVICE_METADATA_TABLE_NAME, "localhost/switch_type"):
            self.switch_type = self.directory.get_slot("CONFIG_DB", swsscommon.CFG_DEVICE_METADATA_TABLE_NAME)["localhost"]["switch_type"]
        log_debug("DeviceGlobalCfgMgr:: Switch type: %s" % self.switch_type)

    def set_handler(self, key, data):
        log_debug("DeviceGlobalCfgMgr:: set handler")
        if self.switch_type:
            log_debug("DeviceGlobalCfgMgr:: Switch type: %s" % self.switch_type)
        """ Handle device tsa_enabled state change """
        if not data:
            log_err("DeviceGlobalCfgMgr:: data is None")
            return False

        if "tsa_enabled" in data:
            self.cfg_mgr.commit()
            self.cfg_mgr.update()
            self.isolate_unisolate_device(data["tsa_enabled"])
            self.directory.put(self.db_name, self.table_name, "tsa_enabled", data["tsa_enabled"])
            return True
        return False

    def del_handler(self, key):
        log_debug("DeviceGlobalCfgMgr:: del handler")
        return True

    def check_state_and_get_tsa_routemaps(self, cfg):
        """ API to get TSA route-maps if device is isolated"""
        cmd = ""
        if self.directory.path_exist("CONFIG_DB", swsscommon.CFG_BGP_DEVICE_GLOBAL_TABLE_NAME, "tsa_enabled"):
            tsa_status = self.directory.get_slot("CONFIG_DB", swsscommon.CFG_BGP_DEVICE_GLOBAL_TABLE_NAME)["tsa_enabled"]
            if tsa_status == "true":
                cmds = cfg.replace("#012", "\n").split("\n")
                log_notice("DeviceGlobalCfgMgr:: Device is isolated. Applying TSA route-maps")
                cmd = self.get_ts_routemaps(cmds, self.tsa_template)
        return cmd

    def isolate_unisolate_device(self, tsa_status):
        """ API to get TSA/TSB route-maps and apply configuration"""
        cmd = "\n"
        if tsa_status == "true":
            log_notice("DeviceGlobalCfgMgr:: Device isolated. Executing TSA")
            cmd += self.get_ts_routemaps(self.cfg_mgr.get_text(), self.tsa_template)
        else:
            log_notice("DeviceGlobalCfgMgr:: Device un-isolated. Executing TSB")
            cmd += self.get_ts_routemaps(self.cfg_mgr.get_text(), self.tsb_template)

        self.cfg_mgr.push(cmd)
        log_debug("DeviceGlobalCfgMgr::Done")

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

