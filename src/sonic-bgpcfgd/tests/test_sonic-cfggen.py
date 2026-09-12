import os
import subprocess

from bgpcfgd.config import ConfigMgr
from .util import resolve_expected_output


TEMPLATE_PATH = os.path.abspath('../../dockers/docker-fpm-frr/frr')
DATA_PATH = "tests/data/sonic-cfggen/"


def run_test(name, template_path, json_path, match_path):
    template_path = os.path.join(TEMPLATE_PATH, template_path)
    json_path = os.path.join(DATA_PATH, json_path)
    command = ['sonic-cfggen', "-T", TEMPLATE_PATH, "-t", template_path, "-y", json_path]
    p = subprocess.Popen(command, shell=False, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    stdout, stderr = p.communicate()
    assert p.returncode == 0, "sonic-cfggen for %s test returned %d code. stderr='%s'" % (name, p.returncode, stderr)
    raw_generated_result = stdout.decode("ascii")
    assert "None" not in raw_generated_result, "Test %s" % name
    canonical_generated_result = ConfigMgr.to_canonical(raw_generated_result)
    match_path = os.path.join(DATA_PATH, match_path)
    match_path = resolve_expected_output(match_path)
    # only for development write_result(match_path, raw_generated_result)
    with open(match_path) as result_fp:
        raw_saved_result = result_fp.read()
    canonical_saved_result = ConfigMgr.to_canonical(raw_saved_result)
    assert canonical_saved_result == canonical_generated_result, "Test %s" % name


def test_bgpd_main_conf_base():
    run_test("Base bgpd.main.conf.j2",
             "bgpd/bgpd.main.conf.j2",
             "bgpd.main.conf.j2/base.json",
             "bgpd.main.conf.j2/base.conf")

def test_bgpd_main_conf_comprehensive():
    run_test("Comprehensive bgpd.main.conf.j2",
             "bgpd/bgpd.main.conf.j2",
             "bgpd.main.conf.j2/all.json",
             "bgpd.main.conf.j2/all.conf")

def test_bgpd_main_conf_defaults():
    run_test("Defaults bgpd.main.conf.j2",
             "bgpd/bgpd.main.conf.j2",
             "bgpd.main.conf.j2/defaults.json",
             "bgpd.main.conf.j2/defaults.conf")

def test_bgpd_main_conf_voq_chassis():
    run_test("VOQ bgpd.main.conf.j2",
             "bgpd/bgpd.main.conf.j2",
             "bgpd.main.conf.j2/voq_chassis.json",
             "bgpd.main.conf.j2/voq_chassis.conf")

def test_bgpd_main_conf_packet_chassis():
    run_test("Chassi packet bgpd.main.conf.j2",
             "bgpd/bgpd.main.conf.j2",
             "bgpd.main.conf.j2/packet_chassis.json",
             "bgpd.main.conf.j2/packet_chassis.conf")

def test_bgpd_lo_ipv6_conf_base():
    run_test("IPv6 Loopback bgpd.main.conf.j2",
             "bgpd/bgpd.main.conf.j2",
             "bgpd.main.conf.j2/base.json",
             "bgpd.main.conf.j2/base.conf")

def test_idf_isolated_no_export():
    run_test("IDF isolation with no export",
             "bgpd/idf_isolate/idf_isolate.conf.j2",
             "idf_isolate/idf_isolated_no_export.json",
             "idf_isolate/idf_isolated_no_export.conf")

def test_idf_isolated_withdraw_all():
    run_test("IDF isolation withdraw all",
             "bgpd/idf_isolate/idf_isolate.conf.j2",
             "idf_isolate/idf_isolated_withdraw_all.json",
             "idf_isolate/idf_isolated_withdraw_all.conf")

def test_idf_unisolated():
    run_test("IDF unisolated",
             "bgpd/idf_isolate/idf_unisolate.conf.j2",
             "idf_isolate/idf_unisolated.json",
             "idf_isolate/idf_unisolated.conf")

def test_tsa_isolate():
    run_test("tsa/bgpd.tsa.isolate.conf.j2",
             "bgpd/tsa/bgpd.tsa.isolate.conf.j2",
             "tsa/isolate.json",
             "tsa/isolate.conf")

def test_tsa_unisolate():
    run_test("tsa/bgpd.tsa.unisolate.conf.j2",
             "bgpd/tsa/bgpd.tsa.unisolate.conf.j2",
             "tsa/unisolate.json",
             "tsa/unisolate.conf")

def test_common_daemons():
    run_test("daemons.common.conf.j2",
             "common/daemons.common.conf.j2",
             "common/daemons.common.conf.json",
             "common/daemons.common.conf")

def test_common_functions():
    run_test("functions.conf.j2",
             "common/functions.conf.j2",
             "common/functions.conf.json",
             "common/functions.conf")

def test_staticd_loopback_route():
    run_test("staticd.loopback_route.conf.j2",
             "staticd/staticd.loopback_route.conf.j2",
             "staticd/staticd.loopback_route.conf.json",
             "staticd/staticd.loopback_route.conf")

def test_staticd_loopback_ipv6_128_route():
    run_test("staticd.loopback_ipv6_128_route.conf.j2",
             "staticd/staticd.loopback_route.conf.j2",
             "staticd/staticd.loopback_ipv6_128_route.conf.json",
             "staticd/staticd.loopback_ipv6_128_route.conf")

def test_staticd():
    run_test("staticd.conf.j2",
             "staticd/staticd.conf.j2",
             "staticd/staticd.conf.json",
             "staticd/staticd.conf")

def test_zebra_interfaces():
    run_test("zebra.interfaces.conf.j2",
             "zebra/zebra.interfaces.conf.j2",
             "zebra/interfaces.json",
             "zebra/interfaces.conf")

def test_zebra_interfaces_public_cloudtype():
    """For cloudtype=Public, IPv4 NHT resolve-via-default is explicitly disabled
    ('no ip nht resolve-via-default') rather than omitted, since FRR's zebra
    defaults this to enabled (true) under the 'traditional' defaults profile
    that SONiC's FRR is built with. IPv6 NHT resolve-via-default is also
    explicitly disabled ('no ipv6 nht resolve-via-default') for the same
    reason, for all cloudtypes."""
    run_test("zebra.interfaces.conf.j2 (Public cloudtype)",
             "zebra/zebra.interfaces.conf.j2",
             "zebra/interfaces_public.json",
             "zebra/interfaces_public.conf")

def test_zebra_interfaces_resolve_disabled():
    """NEXTHOP_TRACKING|default|ipv4 resolve_via_default=false overrides the
    IPv4 platform default; IPv6 keeps its platform default (disabled)."""
    run_test("zebra.interfaces.conf.j2 (resolve_via_default ipv4 disabled)",
             "zebra/zebra.interfaces.conf.j2",
             "zebra/interfaces_resolve_disabled.json",
             "zebra/interfaces_resolve_disabled.conf")

def test_zebra_interfaces_resolve_ipv6_enabled():
    """NEXTHOP_TRACKING|default|ipv6 resolve_via_default=true turns IPv6
    resolution through the default route back on."""
    run_test("zebra.interfaces.conf.j2 (resolve_via_default ipv6 enabled)",
             "zebra/zebra.interfaces.conf.j2",
             "zebra/interfaces_resolve_ipv6_enabled.json",
             "zebra/interfaces_resolve_ipv6_enabled.conf")

def test_zebra_interfaces_resolve_public_override():
    """An explicit resolve_via_default=true wins over the Public cloudtype
    platform default."""
    run_test("zebra.interfaces.conf.j2 (Public cloudtype, resolve_via_default override)",
             "zebra/zebra.interfaces.conf.j2",
             "zebra/interfaces_resolve_public_override.json",
             "zebra/interfaces_resolve_public_override.conf")

def test_zebra_interfaces_resolve_noop():
    """User-VRF rows, rows without the field and non-boolean values leave the
    default-VRF output identical to the no-table case."""
    run_test("zebra.interfaces.conf.j2 (NEXTHOP_TRACKING rows that do not apply)",
             "zebra/zebra.interfaces.conf.j2",
             "zebra/interfaces_resolve_noop.json",
             "zebra/interfaces.conf")

def test_zebra_nht_user_vrf_resolve():
    """One vrf block per user VRF with resolve_via_default set; the default
    VRF row is left to zebra.interfaces.conf.j2."""
    run_test("zebra.nht.conf.j2 (user VRF resolve_via_default)",
             "zebra/zebra.nht.conf.j2",
             "zebra/nht/user_vrf_resolve.json",
             "zebra/nht/user_vrf_resolve.conf")

def test_zebra_nht_empty():
    run_test("zebra.nht.conf.j2 (no NEXTHOP_TRACKING table)",
             "zebra/zebra.nht.conf.j2",
             "zebra/nht/empty.json",
             "zebra/nht/empty.conf")

def test_zebra_nht_empty_table():
    run_test("zebra.nht.conf.j2 (empty NEXTHOP_TRACKING table)",
             "zebra/zebra.nht.conf.j2",
             "zebra/nht/empty_table.json",
             "zebra/nht/empty_table.conf")

def test_zebra_nht_no_field():
    """User-VRF rows without resolve_via_default, non-boolean values, default
    VRF rows and malformed keys all render nothing."""
    run_test("zebra.nht.conf.j2 (rows that do not apply)",
             "zebra/zebra.nht.conf.j2",
             "zebra/nht/no_field.json",
             "zebra/nht/no_field.conf")

def test_zebra_set_src():
    run_test("zebra.set_src.conf.j2",
             "zebra/zebra.set_src.conf.j2",
             "zebra/set_src.json",
             "zebra/set_src.conf")

def test_zebra():
    run_test("zebra.conf.j2",
             "zebra/zebra.conf.j2",
             "zebra/zebra.conf.json",
             "zebra/zebra.conf")

def test_zebra_nht_full_config():
    """zebra.conf.j2 end to end with NEXTHOP_TRACKING rows: the default-VRF
    override lands in the interfaces section and each user VRF gets its own
    block, on its own lines, after it."""
    run_test("zebra.conf.j2 (NEXTHOP_TRACKING rows)",
             "zebra/zebra.conf.j2",
             "zebra/zebra.conf.nht.json",
             "zebra/zebra.conf.nht.conf")

def test_isolate():
    run_test("isolate.j2",
             "isolate.j2",
             "isolate/isolate.json",
             "isolate/isolate")

def test_unisolate():
    run_test("unisolate.j2",
             "unisolate.j2",
             "isolate/unisolate.json",
             "isolate/unisolate")

def test_frr_conf():
    run_test("frr.conf.j2",
             "frr.conf.j2",
             "frr.conf.j2/all.json",
             "frr.conf.j2/all.conf")

def test_l3vpn_base():
    run_test("bgpd spine_chassis_frontend_router.conf.j2",
             "bgpd/bgpd.spine_chassis_frontend_router.conf.j2",
             "bgpd.spine_chassis_frontend_router.conf.j2/base.json",
             "bgpd.spine_chassis_frontend_router.conf.j2/base.conf")

def test_bgp_conf_all():
    run_test("bgpd/bgpd.conf",
             "bgpd/bgpd.conf.j2",
             "bgpd.conf.j2/all.json",
             "bgpd.conf.j2/all.conf")

def test_bgp_conf_packet_chassis_ipv6_lo4096():
    run_test("packet chassis ipv6 loopback4096 bgpd.main.conf.j2",
             "bgpd/bgpd.main.conf.j2",
             "bgpd.main.conf.j2/packet_chassis_ipv6_lo4096.json",
             "bgpd.main.conf.j2/packet_chassis_ipv6_lo4096.conf")

def test_bgp_conf_packet_chassis_ipv6_lo4096_router_id():
    run_test("packet chassis ipv6 loopback4096 with router_id bgpd.main.conf.j2",
             "bgpd/bgpd.main.conf.j2",
             "bgpd.main.conf.j2/packet_chassis_ipv6_lo4096_router_id.json",
             "bgpd.main.conf.j2/packet_chassis_ipv6_lo4096_router_id.conf")

def test_bgp_conf_packet_chassis_router_id():
    run_test("packet chassis with router_id bgpd.main.conf.j2",
             "bgpd/bgpd.main.conf.j2",
             "bgpd.main.conf.j2/packet_chassis_router_id.json",
             "bgpd.main.conf.j2/packet_chassis_router_id.conf")

def test_bgpd_main_conf_lo0_ipv6_only():
    run_test("Base bgpd.main.conf.j2",
             "bgpd/bgpd.main.conf.j2",
             "bgpd.main.conf.j2/lo0_ipv6_only.json",
             "bgpd.main.conf.j2/lo0_ipv6_only.conf")

def test_bgpd_main_conf_lo0_ipv6_only_router_id():
    run_test("Base bgpd.main.conf.j2",
             "bgpd/bgpd.main.conf.j2",
             "bgpd.main.conf.j2/lo0_ipv6_only_router_id.json",
             "bgpd.main.conf.j2/lo0_ipv6_only_router_id.conf")

def test_bgpd_main_conf_defaults_router_id():
    run_test("Defaults bgpd.main.conf.j2",
             "bgpd/bgpd.main.conf.j2",
             "bgpd.main.conf.j2/defaults_router_id.json",
             "bgpd.main.conf.j2/defaults_router_id.conf")

def test_prefix_list_add_radian():
    run_test("Add radian configuration",
             "bgpd/radian/add_radian.conf.j2",
             "radian/add_radian.json",
             "radian/add_radian.conf")
    
def test_prefix_list_del_radian():
    run_test("Del radian configuration",
             "bgpd/radian/del_radian.conf.j2",
             "radian/del_radian.json",
             "radian/del_radian.conf")

def test_bgp_confed_ut2_multi_asic():
    run_test("BGP Confederation Upper T2 Multi-ASIC",
             "bgpd/bgpd.main.conf.j2",
             "bgpd.main.conf.j2/multi_asic_upper_t2.json",
             "bgpd.main.conf.j2/multi_asic_upper_t2.conf")

def test_bgp_confed_ut2_single_asic():
    run_test("BGP Confederation Upper T2 Single-ASIC",
             "bgpd/bgpd.main.conf.j2",
             "bgpd.main.conf.j2/single_asic_upper_t2.json",
             "bgpd.main.conf.j2/single_asic_upper_t2.conf")

def test_bgp_confed_lt2_single_asic():
    run_test("BGP Confederation Lower T2 Single-ASIC",
             "bgpd/bgpd.main.conf.j2",
             "bgpd.main.conf.j2/single_asic_lt2.json",
             "bgpd.main.conf.j2/single_asic_lt2.conf")

def test_bgp_confed_ft2_single_asic():
    run_test("BGP Confederation Fabric T2 Single-ASIC",
             "bgpd/bgpd.main.conf.j2",
             "bgpd.main.conf.j2/single_asic_ft2.json",
             "bgpd.main.conf.j2/single_asic_ft2.conf")

def test_bgp_confed_lrh_single_asic():
    run_test("BGP Confederation LowerRegionalHub Single-ASIC",
             "bgpd/bgpd.main.conf.j2",
             "bgpd.main.conf.j2/single_asic_lrh.json",
             "bgpd.main.conf.j2/single_asic_lrh.conf")

def test_bgp_confed_frh_single_asic():
    run_test("BGP Confederation FabricRegionalHub Single-ASIC",
             "bgpd/bgpd.main.conf.j2",
             "bgpd.main.conf.j2/single_asic_frh.json",
             "bgpd.main.conf.j2/single_asic_frh.conf")

def test_bgp_confed_urh_single_asic():
    run_test("BGP Confederation UpperRegionalHub Single-ASIC",
             "bgpd/bgpd.main.conf.j2",
             "bgpd.main.conf.j2/single_asic_urh.json",
             "bgpd.main.conf.j2/single_asic_urh.conf")

def _render_bgpd_main(json_path):
    template_path = os.path.join(TEMPLATE_PATH, "bgpd/bgpd.main.conf.j2")
    json_full_path = os.path.join(DATA_PATH, json_path)
    command = ['sonic-cfggen', "-T", TEMPLATE_PATH, "-t", template_path, "-y", json_full_path]
    p = subprocess.Popen(command, shell=False, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    stdout, stderr = p.communicate()
    assert p.returncode == 0, "sonic-cfggen returned %d. stderr=%r" % (p.returncode, stderr)
    return stdout.decode("ascii")

def test_bgpd_main_llgr_helper_emitted_on_urh():
    """LLGR helper-only block must be emitted for UpperRegionalHub."""
    rendered = _render_bgpd_main("bgpd.main.conf.j2/single_asic_urh.json")
    assert "bgp graceful-restart-disable" in rendered, \
        "Expected 'bgp graceful-restart-disable' on UpperRegionalHub, got:\n%s" % rendered
    assert "bgp long-lived-graceful-restart stale-time 864000" in rendered, \
        "Expected 'bgp long-lived-graceful-restart stale-time 864000' on UpperRegionalHub, got:\n%s" % rendered

def test_bgpd_main_llgr_helper_absent_on_non_urh():
    """LLGR helper-only block must NOT be emitted for any non-UpperRegionalHub type."""
    non_urh_fixtures = [
        "bgpd.main.conf.j2/all.json",                  # ToRRouter
        "bgpd.main.conf.j2/defaults.json",             # ToRRouter
        "bgpd.main.conf.j2/single_asic_lt2.json",      # LowerSpineRouter
        "bgpd.main.conf.j2/single_asic_ft2.json",      # FabricSpineRouter
        "bgpd.main.conf.j2/single_asic_lrh.json",      # LowerRegionalHub
        "bgpd.main.conf.j2/single_asic_frh.json",      # FabricRegionalHub
        "bgpd.main.conf.j2/single_asic_upper_t2.json", # UpperSpineRouter
        "bgpd.main.conf.j2/voq_chassis.json",          # SpineRouter
        "bgpd.main.conf.j2/base.json",                 # type unset
    ]
    for fixture in non_urh_fixtures:
        rendered = _render_bgpd_main(fixture)
        assert "bgp graceful-restart-disable" not in rendered, \
            "%s must not contain 'bgp graceful-restart-disable'" % fixture
        assert "long-lived-graceful-restart" not in rendered, \
            "%s must not contain 'long-lived-graceful-restart'" % fixture
