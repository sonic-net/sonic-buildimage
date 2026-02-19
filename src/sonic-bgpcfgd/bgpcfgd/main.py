import os
import signal
import subprocess
import sys
import syslog
import threading
import time
import traceback

from swsscommon.swsscommon import ConfigDBConnector
from swsscommon import swsscommon
from sonic_py_common import device_info

from .config import ConfigMgr
from .directory import Directory
from .log import log_notice, log_crit, log_warn
from .managers_advertise_rt import AdvertiseRouteMgr
from .managers_aggregate_address import AggregateAddressMgr, BGP_AGGREGATE_ADDRESS_TABLE_NAME
from .managers_allow_list import BGPAllowListMgr
from .managers_bbr import BBRMgr, BGP_BBR_TABLE_NAME
from .managers_bgp import BGPPeerMgrBase
from .managers_db import BGPDataBaseMgr
from .managers_intf import InterfaceMgr
from .managers_setsrc import ZebraSetSrc
from .managers_static_rt import StaticRouteMgr
from .managers_rm import RouteMapMgr
from .managers_device_global import DeviceGlobalCfgMgr
from .managers_chassis_app_db import ChassisAppDbMgr
from .managers_bfd import BfdMgr
from .managers_srv6 import SRv6Mgr
from .managers_prefix_list import PrefixListMgr
from .managers_as_path import AsPathMgr
from .static_rt_timer import StaticRouteTimer
from .runner import Runner, signal_handler
from .template import TemplateFabric
from .utils import read_constants
from .frr import FRR
from .vars import g_debug


def do_work():
    """ Main function """
    st_rt_timer = StaticRouteTimer()
    thr = threading.Thread(target = st_rt_timer.run)
    thr.start()
    frr = FRR(["bgpd", "zebra", "staticd"])
    frr.wait_for_daemons(seconds=20)

    # Wait for mgmtd initial config load to avoid "Lock already taken on DS" error
    log_notice("Checking mgmtd datastore readiness...")
    for attempt in range(10):  # Max ~5 seconds
        try:
            out = subprocess.check_output(['vtysh', '-c', 'show mgmt datastore all'],
                                        stderr=subprocess.DEVNULL, timeout=2, text=True)
            # Check if any datastore is locked (FRR topotest approach)
            locked_lines = [line for line in out.splitlines() if 'Locked:' in line and 'True' in line]
            if not locked_lines:
                log_notice("mgmtd datastores are ready (attempt %d)" % (attempt + 1))
                break
            else:
                log_warn("mgmtd datastores still locked (attempt %d): %s" % (attempt + 1, ', '.join(locked_lines)))
        except Exception as e:
            log_warn("mgmtd check failed (attempt %d): %s" % (attempt + 1, str(e)))
        time.sleep(0.5)
    #
    common_objs = {
        'directory': Directory(),
        'cfg_mgr':   ConfigMgr(frr),
        'tf':        TemplateFabric(),
        'constants': read_constants(),
        'state_db_conn': swsscommon.DBConnector("STATE_DB", 0)
    }
    managers = [
        # Config DB managers
        BGPDataBaseMgr(common_objs, "CONFIG_DB", swsscommon.CFG_DEVICE_METADATA_TABLE_NAME),
        BGPDataBaseMgr(common_objs, "CONFIG_DB", swsscommon.CFG_DEVICE_NEIGHBOR_METADATA_TABLE_NAME),
        # Interface managers
        InterfaceMgr(common_objs, "CONFIG_DB", swsscommon.CFG_INTF_TABLE_NAME),
        InterfaceMgr(common_objs, "CONFIG_DB", swsscommon.CFG_LOOPBACK_INTERFACE_TABLE_NAME),
        InterfaceMgr(common_objs, "CONFIG_DB", swsscommon.CFG_VLAN_INTF_TABLE_NAME),
        InterfaceMgr(common_objs, "CONFIG_DB", swsscommon.CFG_LAG_INTF_TABLE_NAME),
        InterfaceMgr(common_objs, "CONFIG_DB", swsscommon.CFG_VOQ_INBAND_INTERFACE_TABLE_NAME),
        InterfaceMgr(common_objs, "CONFIG_DB", swsscommon.CFG_VLAN_SUB_INTF_TABLE_NAME),
        # State DB managers
        ZebraSetSrc(common_objs, "STATE_DB", swsscommon.STATE_INTERFACE_TABLE_NAME),
        # Peer Managers
        BGPPeerMgrBase(common_objs, "CONFIG_DB", swsscommon.CFG_BGP_NEIGHBOR_TABLE_NAME, "general", True),
        BGPPeerMgrBase(common_objs, "CONFIG_DB", swsscommon.CFG_BGP_INTERNAL_NEIGHBOR_TABLE_NAME, "internal", False),
        BGPPeerMgrBase(common_objs, "CONFIG_DB", "BGP_MONITORS", "monitors", False),
        BGPPeerMgrBase(common_objs, "CONFIG_DB", "BGP_PEER_RANGE", "dynamic", False),
        BGPPeerMgrBase(common_objs, "CONFIG_DB", "BGP_VOQ_CHASSIS_NEIGHBOR", "voq_chassis", False),
        BGPPeerMgrBase(common_objs, "CONFIG_DB", "BGP_SENTINELS", "sentinels", False),
        # AllowList Managers
        BGPAllowListMgr(common_objs, "CONFIG_DB", "BGP_ALLOWED_PREFIXES"),
        # BBR Manager
        BBRMgr(common_objs, "CONFIG_DB", BGP_BBR_TABLE_NAME),
        # Static Route Managers
        StaticRouteMgr(common_objs, "CONFIG_DB", "STATIC_ROUTE"),
        StaticRouteMgr(common_objs, "APPL_DB", "STATIC_ROUTE"),
        # Route Advertisement Managers
        AdvertiseRouteMgr(common_objs, "STATE_DB", swsscommon.STATE_ADVERTISE_NETWORK_TABLE_NAME),
        RouteMapMgr(common_objs, "APPL_DB", swsscommon.APP_BGP_PROFILE_TABLE_NAME),
        # Device Global Manager
        DeviceGlobalCfgMgr(common_objs, "CONFIG_DB", swsscommon.CFG_BGP_DEVICE_GLOBAL_TABLE_NAME),
        # Bgp Aggregate Address Manager
        AggregateAddressMgr(common_objs, "CONFIG_DB", BGP_AGGREGATE_ADDRESS_TABLE_NAME),
        # SRv6 Manager
        SRv6Mgr(common_objs, "CONFIG_DB", "SRV6_MY_SIDS"),
        SRv6Mgr(common_objs, "CONFIG_DB", "SRV6_MY_LOCATORS"),
    ]

    if device_info.is_chassis():
        managers.append(ChassisAppDbMgr(common_objs, "CHASSIS_APP_DB", "BGP_DEVICE_GLOBAL"))

    config_db = ConfigDBConnector()
    config_db.connect()
    sys_defaults = config_db.get_table('SYSTEM_DEFAULTS')
    if 'software_bfd' in sys_defaults and 'status' in sys_defaults['software_bfd'] and sys_defaults['software_bfd']['status'] == 'enabled':
        log_notice("software_bfd feature is enabled, starting bfd manager")
        managers.append(BfdMgr(common_objs, "STATE_DB", swsscommon.STATE_BFD_SOFTWARE_SESSION_TABLE_NAME))

    device_metadata = config_db.get_table("DEVICE_METADATA")
    # Enable Prefix List Manager and AsPath Manager for UpperSpineRouter/UpstreamLC
    is_upstream_lc = ("localhost" in device_metadata and "type" in device_metadata["localhost"] and "subtype" in device_metadata["localhost"] and
                      device_metadata["localhost"]["type"] == "SpineRouter" and device_metadata["localhost"]["subtype"] == "UpstreamLC")
    is_upper_spine_router = ("localhost" in device_metadata and "type" in device_metadata["localhost"] and
                             device_metadata["localhost"]["type"] == "UpperSpineRouter")
    if is_upstream_lc or is_upper_spine_router:
        # Prefix List Manager
        managers.append(PrefixListMgr(common_objs, "CONFIG_DB", "PREFIX_LIST"))
        managers.append(AsPathMgr(common_objs, "CONFIG_DB", "DEVICE_METADATA"))
        log_notice("Prefix List Manager and AsPath Manager are enabled for UpperSpineRouter/UpstreamLC")

    runner = Runner(common_objs['cfg_mgr'])
    for mgr in managers:
        runner.add_manager(mgr)
    runner.run()


def main():
    rc = 0
    try:
        syslog.openlog('bgpcfgd')
        signal.signal(signal.SIGTERM, signal_handler)
        signal.signal(signal.SIGINT, signal_handler)
        do_work()
    except KeyboardInterrupt:
        log_notice("Keyboard interrupt")
    except RuntimeError as exc:
        log_crit(str(exc))
        rc = -2
        if g_debug:
            raise
    except Exception as exc:
        log_crit("Got an exception %s: Traceback: %s" % (str(exc), traceback.format_exc()))
        rc = -1
        if g_debug:
            raise
    finally:
        syslog.closelog()
    try:
        sys.exit(rc)
    except SystemExit:
        os._exit(rc)

