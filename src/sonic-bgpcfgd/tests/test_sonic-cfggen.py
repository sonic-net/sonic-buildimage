import os
import subprocess

from bgpcfgd.config import ConfigMgr


TEMPLATE_PATH = os.path.abspath('../../dockers/docker-fpm-frr/frr')
DATA_PATH = "tests/data/sonic-cfggen/"
CONSTANTS_PATH = os.path.abspath('../../files/image_config/constants/constants.yml')


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