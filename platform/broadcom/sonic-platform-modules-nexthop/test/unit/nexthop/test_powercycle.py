from unittest.mock import call, patch

import pytest

CPU_BDF = "0000:03:00.0"
SWITCH_BDF = "0000:04:00.0"
CPU_CHASSIS = {"fpga_bdf": CPU_BDF, "reg_offset": "0x14", "enable_word": "0xa5a5a5a5"}
SWITCH_CHASSIS = {"fpga_bdf": SWITCH_BDF, "reg_offset": "0x3c", "enable_word": "0x5a5a5a5a"}
SWITCH_HOST = {"fpga_bdf": CPU_BDF, "reg_offset": "0x60", "enable_word": "0xc0ffee", "timeout_ms": 2500}
CHASSIS_ONLY = {
    "cpu_card_fpga_0_chassis_pwrcyc": CPU_CHASSIS,
    "switch_card_fpga_0_chassis_pwrcyc": SWITCH_CHASSIS,
}
FULL = {**CHASSIS_ONLY, "switch_host_pwrcyc": SWITCH_HOST}
EMPTY_SWITCH_HOST = {**CHASSIS_ONLY, "switch_host_pwrcyc": {}}

CHASSIS_WRITES = [call(CPU_BDF, 0x14, 0xA5A5A5A5), call(SWITCH_BDF, 0x3C, 0x5A5A5A5A)]


def config_for(switch_name="SWITCHCARD_FPGA", bdf=SWITCH_BDF, reboot_conf=None):
    def fpga(name, address):
        return {"dev_info": {"device_type": "MULTIFPGAPCIE", "device_name": name, "device_bdf": address}}

    config = {
        "CPU": fpga("CPUCARD_FPGA", CPU_BDF),
        "SC": fpga(switch_name, bdf),
    }
    if reboot_conf is not None:
        config["FPGA_REBOOT_CONF"] = reboot_conf
    return config


def run_powercycle(config, switch_host=True):
    from nexthop import fpga_lib, powercycle

    with (
        patch.object(powercycle.device_info, "is_switch_host", return_value=switch_host),
        patch.object(fpga_lib, "write_32", autospec=True) as write,
        patch.object(fpga_lib.time, "sleep", autospec=True) as sleep,
        patch.object(powercycle, "log_to_kernel", autospec=True),
        pytest.raises(Exception, match="Failed to initiate reboot"),
    ):
        powercycle.powercycle(config)
    return write, sleep


def test_a_switch_host_that_fails_to_reset_never_falls_back_to_the_chassis_cycle():
    write, sleep = run_powercycle(config_for(reboot_conf=FULL))
    assert write.call_args_list == [call(CPU_BDF, 0x60, 0xC0FFEE)]
    assert sleep.call_args_list == [call(2.5)]


@pytest.mark.parametrize(
    "switch_host, reboot_conf",
    [
        (False, FULL),
        (True, EMPTY_SWITCH_HOST),  # firmware lacks the register
    ],
)
def test_the_chassis_cycles_without_a_switch_host_reset(switch_host, reboot_conf):
    write, _ = run_powercycle(config_for(reboot_conf=reboot_conf), switch_host=switch_host)
    assert write.call_args_list == CHASSIS_WRITES


def test_a_failed_cpu_write_still_reaches_the_switch_card():
    from nexthop import fpga_lib, powercycle

    with (
        patch.object(fpga_lib, "write_32", autospec=True) as write,
        patch.object(fpga_lib.time, "sleep", autospec=True),
        patch.object(powercycle, "log_to_kernel", autospec=True),
        patch.object(powercycle.device_info, "is_switch_host", return_value=False),
    ):
        write.side_effect = [OSError("bus error"), None]
        with pytest.raises(Exception, match="Failed to initiate reboot"):
            powercycle.powercycle(config_for(reboot_conf=CHASSIS_ONLY))
    assert write.call_args_list == CHASSIS_WRITES
