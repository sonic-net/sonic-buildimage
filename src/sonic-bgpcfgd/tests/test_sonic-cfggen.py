import os
import subprocess

from bgpcfgd.config import ConfigMgr
from .util import resolve_expected_output, CONSTANTS_PATH


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


def _render_aggregate_conf(json_path, constants_path=CONSTANTS_PATH):
    template_path = os.path.join(TEMPLATE_PATH, "bgpd/bgpd.aggregate.conf.j2")
    json_full_path = os.path.join(DATA_PATH, json_path)
    command = ['sonic-cfggen', "-T", TEMPLATE_PATH, "-t", template_path,
               "-y", constants_path, "-y", json_full_path]
    p = subprocess.Popen(command, shell=False, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    stdout, stderr = p.communicate()
    assert p.returncode == 0, "sonic-cfggen returned %d. stderr=%r" % (p.returncode, stderr)
    rendered = stdout.decode("ascii")
    assert "None" not in rendered, "Unexpected 'None' in output, got:\n%s" % rendered
    return rendered


def run_aggregate_test(name, json_path, match_path, constants_path=CONSTANTS_PATH):
    template_path = os.path.join(TEMPLATE_PATH, "bgpd/bgpd.aggregate.conf.j2")
    json_path = os.path.join(DATA_PATH, json_path)
    command = ['sonic-cfggen', "-T", TEMPLATE_PATH, "-t", template_path,
               "-y", constants_path, "-y", json_path]
    p = subprocess.Popen(command, shell=False, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    stdout, stderr = p.communicate()
    assert p.returncode == 0, "sonic-cfggen for %s test returned %d code. stderr='%s'" % (name, p.returncode, stderr)
    raw_generated_result = stdout.decode("ascii")
    assert "None" not in raw_generated_result, "Test %s" % name
    canonical_generated_result = ConfigMgr.to_canonical(raw_generated_result)
    match_path = os.path.join(DATA_PATH, match_path)
    with open(match_path) as result_fp:
        raw_saved_result = result_fp.read()
    canonical_saved_result = ConfigMgr.to_canonical(raw_saved_result)
    assert canonical_saved_result == canonical_generated_result, "Test %s" % name

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


def test_aggregate_conf_ipv4_basic():
    """IPv4 aggregate-address with summary-only must be rendered into bgpd config."""
    run_aggregate_test("bgpd.aggregate.conf.j2 IPv4 basic",
                       "bgpd.aggregate.conf.j2/ipv4_basic.json",
                       "bgpd.aggregate.conf.j2/ipv4_basic.conf")


def test_aggregate_conf_ipv4_with_prefix_lists():
    """IPv4 aggregate with aggregate/contributing prefix-list names must generate prefix-list stanzas."""
    run_aggregate_test("bgpd.aggregate.conf.j2 IPv4 with prefix-lists",
                       "bgpd.aggregate.conf.j2/ipv4_with_prefix_lists.json",
                       "bgpd.aggregate.conf.j2/ipv4_with_prefix_lists.conf")


def test_aggregate_conf_ipv6_with_prefix_lists():
    """IPv6 aggregate and prefix-list commands must use IPv6 syntax and bounds."""
    run_aggregate_test("bgpd.aggregate.conf.j2 IPv6 with prefix-lists",
                       "bgpd.aggregate.conf.j2/ipv6_with_prefix_lists.json",
                       "bgpd.aggregate.conf.j2/ipv6_with_prefix_lists.conf")


def test_aggregate_conf_accepts_ipv6_expanded_strict_prefix():
    """An IPv6 strict network in expanded form must pass normalized prefix validation."""
    run_aggregate_test("bgpd.aggregate.conf.j2 IPv6 expanded strict prefix",
                       "bgpd.aggregate.conf.j2/ipv6_expanded_strict_prefix.json",
                       "bgpd.aggregate.conf.j2/ipv6_expanded_strict_prefix.conf")


def test_aggregate_conf_rejects_invalid_prefixes():
    """Unparseable prefixes and prefixes with host bits must not be rendered."""
    run_aggregate_test("bgpd.aggregate.conf.j2 invalid prefixes",
                       "bgpd.aggregate.conf.j2/invalid_prefixes.json",
                       "bgpd.aggregate.conf.j2/invalid_prefixes.conf")


def test_aggregate_conf_bare_aggregate_without_optional_attributes():
    """An aggregate without summary-only/as-set/prefix-lists must render a bare aggregate-address."""
    run_aggregate_test("bgpd.aggregate.conf.j2 bare aggregate",
                       "bgpd.aggregate.conf.j2/ipv4_bare.json",
                       "bgpd.aggregate.conf.j2/ipv4_bare.conf")


def test_aggregate_conf_bbr_required_enabled():
    """A bbr-required aggregate must be rendered when BGP_BBR|all status == 'enabled'."""
    run_aggregate_test("bgpd.aggregate.conf.j2 BBR required + enabled",
                       "bgpd.aggregate.conf.j2/bbr_required_enabled.json",
                       "bgpd.aggregate.conf.j2/bbr_required_enabled.conf")


def test_aggregate_conf_bbr_required_disabled():
    """A bbr-required aggregate must NOT be rendered when BGP_BBR|all status == 'disabled'."""
    run_aggregate_test("bgpd.aggregate.conf.j2 BBR required + disabled",
                       "bgpd.aggregate.conf.j2/bbr_required_disabled.json",
                       "bgpd.aggregate.conf.j2/bbr_required_disabled.conf")


def test_aggregate_conf_bbr_required_uses_constants_default_disabled():
    """A bbr-required aggregate must be suppressed when constants.yml defaults BBR to disabled."""
    run_aggregate_test("bgpd.aggregate.conf.j2 BBR required + constants default disabled",
                       "bgpd.aggregate.conf.j2/bbr_required_no_explicit_bbr.json",
                       "bgpd.aggregate.conf.j2/bbr_required_constants_default_disabled.conf")


def test_aggregate_conf_bbr_required_uses_constants_default_state():
    """A bbr-required aggregate must be rendered when constants.yml defaults BBR to enabled."""
    constants_path = os.path.join(DATA_PATH,
                                  "bgpd.aggregate.conf.j2/bbr_default_enabled_constants.yml")
    run_aggregate_test("bgpd.aggregate.conf.j2 BBR required + constants default enabled",
                       "bgpd.aggregate.conf.j2/bbr_required_no_explicit_bbr.json",
                       "bgpd.aggregate.conf.j2/bbr_required_constants_default_enabled.conf",
                       constants_path)


def test_aggregate_conf_explicit_bbr_status_overrides_constants_default():
    """Explicit BGP_BBR|all status must override constants.yml default BBR state."""
    constants_path = os.path.join(DATA_PATH,
                                  "bgpd.aggregate.conf.j2/bbr_default_enabled_constants.yml")
    run_aggregate_test("bgpd.aggregate.conf.j2 explicit BBR disabled overrides constants default enabled",
                       "bgpd.aggregate.conf.j2/bbr_required_config_disabled_overrides_constants_enabled.json",
                       "bgpd.aggregate.conf.j2/bbr_required_config_disabled_overrides_constants_enabled.conf",
                       constants_path)


def test_aggregate_conf_bbr_required_requires_constants_peer_group_mapping():
    """A bbr-required aggregate must be suppressed when constants.yml has no BBR peer-group mapping."""
    constants_path = os.path.join(DATA_PATH,
                                  "bgpd.aggregate.conf.j2/bbr_default_enabled_no_peer_group_constants.yml")
    run_aggregate_test("bgpd.aggregate.conf.j2 BBR required + constants enabled without peer-group mapping",
                       "bgpd.aggregate.conf.j2/bbr_required_no_explicit_bbr.json",
                       "bgpd.aggregate.conf.j2/bbr_required_constants_enabled_no_peer_groups.conf",
                       constants_path)


def test_aggregate_conf_explicit_bbr_enabled_requires_constants_peer_group_mapping():
    """Explicit BGP_BBR|all enabled still requires a constants.yml BBR peer-group mapping."""
    constants_path = os.path.join(DATA_PATH,
                                  "bgpd.aggregate.conf.j2/bbr_default_enabled_no_peer_group_constants.yml")
    run_aggregate_test("bgpd.aggregate.conf.j2 BBR explicit enabled without peer-group mapping",
                       "bgpd.aggregate.conf.j2/bbr_required_enabled.json",
                       "bgpd.aggregate.conf.j2/bbr_required_constants_enabled_no_peer_groups.conf",
                       constants_path)


def test_aggregate_conf_no_bbr_required_renders_regardless():
    """An aggregate without bbr-required must be rendered regardless of BGP_BBR state."""
    rendered = _render_aggregate_conf("bgpd.aggregate.conf.j2/ipv4_basic.json")
    assert "aggregate-address 192.168.0.0/24" in rendered, \
        "Expected aggregate-address in output, got:\n%s" % rendered


def test_aggregate_conf_bbr_gate_blocks_bbr_required_when_disabled():
    """BBR gate: bbr-required=true aggregate is suppressed when BBR is disabled."""
    rendered = _render_aggregate_conf("bgpd.aggregate.conf.j2/bbr_required_disabled.json")
    assert not any(line.strip().startswith("aggregate-address 192.168.0.0/24")
                   for line in rendered.splitlines()), \
        "bbr-required aggregate must not appear when BBR is disabled, got:\n%s" % rendered
