from unittest.mock import call, patch

import pytest

CPU_BDF = "0000:03:00.0"
SWITCH_BDF = "0000:04:00.0"
CPU_CHASSIS = {"fpga_bdf": CPU_BDF, "reg_offset": "0x14", "enable_word": "0xa5a5a5a5"}
SWITCH_CHASSIS = {"fpga_bdf": SWITCH_BDF, "reg_offset": "0x3c", "enable_word": "0x5a5a5a5a"}
CHASSIS_ONLY = {
    "cpu_card_fpga_0_chassis_pwrcyc": CPU_CHASSIS,
    "switch_card_fpga_0_chassis_pwrcyc": SWITCH_CHASSIS,
}

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


def test_a_failed_cpu_write_still_reaches_the_switch_card():
    from nexthop import fpga_lib, powercycle

    with (
        patch.object(fpga_lib, "write_32", autospec=True) as write,
        patch.object(fpga_lib.time, "sleep", autospec=True),
        patch.object(powercycle, "log_to_kernel", autospec=True),
    ):
        write.side_effect = [OSError("bus error"), None]
        with pytest.raises(Exception, match="Failed to initiate reboot"):
            powercycle.powercycle(config_for(reboot_conf=CHASSIS_ONLY))
    assert write.call_args_list == CHASSIS_WRITES
