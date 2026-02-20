#!/usr/bin/env python

import datetime
import json
import os
import pytest
import tempfile

from fixtures.test_helpers_adm1266 import create_raw_adm1266_blackbox_record
from unittest.mock import patch


@pytest.fixture
def dpm_logger_module():
    """Loads the module before each test. This is to let conftest.py run first."""
    from sonic_platform import dpm_logger

    yield dpm_logger


@pytest.fixture
def adm1266_module():
    """Loads the module before each test. This is to let conftest.py run first."""
    from sonic_platform import adm1266

    yield adm1266


def get_fake_data(gen_time: str):
    return {
        "gen_time": gen_time,
        "schema_version": 1,
        "causes": [
            {
                "source": "test-dpm",
                "timestamp": "2025-10-02 23:26:07 UTC",
                "cause": "WATCHDOG",
                "description": "FPGA watchdog expired",
            },
        ],
        "dpms": [
            {
                "name": "test-dpm-1",
                "type": "adm1266",
                "records": [
                    {
                        "timestamp": "2025-10-02 23:26:07 UTC",
                        "dpm_name": "test-dpm-1",
                        "power_fault_cause": "WATCHDOG (FPGA watchdog expired), under_voltage: VH1(POS12V), over_voltage: n/a",
                        "uid": "12345",
                        "powerup_counter": "65533",
                        "vh_under_voltage_[4:1]": "0b0001 [VH1(POS12V)]",
                        "pdio_in_[16:1]": "0b0000000010000000 [PDI8]",
                    }
                ],
            },
        ],
    }


def get_expected_loaded_data(dpm_logger_module, gen_time: str):
    fake_records = get_fake_data(gen_time)["dpms"][0]["records"]
    return dpm_logger_module.DataV1(
        gen_time=gen_time,
        schema_version=1,
        causes=[
            dpm_logger_module.CauseV1(
                source="test-dpm",
                timestamp="2025-10-02 23:26:07 UTC",
                cause="WATCHDOG",
                description="FPGA watchdog expired",
            ),
        ],
        dpms=[
            dpm_logger_module.DpmV1(
                name="test-dpm-1",
                type="adm1266",
                records=fake_records,
            ),
        ],
    )


class TestDpmLogger:
    """Test class for DpmLogger functionality."""

    def test_save_and_load_v1(self, dpm_logger_module, adm1266_module):
        # Given
        CAUSES = [
            dpm_logger_module.RebootCause(
                type=dpm_logger_module.RebootCause.Type.SOFTWARE,
                source="SW",
                timestamp=datetime.datetime(2025, 10, 2, 23, 22, 56, tzinfo=datetime.timezone.utc),
                cause="reboot",
                description="User issued 'reboot' command [User: admin, Time: Thu Oct  2 11:22:56 PM UTC 2025]",
                chassis_reboot_cause_category="REBOOT_CAUSE_NON_HARDWARE",
            ),
            dpm_logger_module.RebootCause(
                type=dpm_logger_module.RebootCause.Type.HARDWARE,
                source="test-dpm-1",
                timestamp=datetime.datetime(2025, 10, 2, 23, 26, 7, tzinfo=datetime.timezone.utc),
                cause="CPU_CMD_PCYC",
                description="CPU card commanded power cycle",
                chassis_reboot_cause_category="REBOOT_CAUSE_POWER_LOSS",
            ),
        ]
        DPM_1_POWERUPS = [
            dpm_logger_module.DpmPowerUpEntry(
                powerup_counter=65533,
                power_fault_cause=None,
                dpm_records=[
                    adm1266_module.Adm1266BlackBoxRecord.from_bytes(
                        create_raw_adm1266_blackbox_record(
                            uid=12345,
                            powerup_counter=65533,
                        ),
                        "test-dpm-1",
                    )
                ],
            ),
            dpm_logger_module.DpmPowerUpEntry(
                powerup_counter=65534,
                power_fault_cause=None,
                dpm_records=[
                    adm1266_module.Adm1266BlackBoxRecord.from_bytes(
                        create_raw_adm1266_blackbox_record(
                            uid=12346,
                            powerup_counter=65534,
                        ),
                        "test-dpm-1",
                    )
                ],
            ),
        ]
        DPM_2_POWERUPS = [
            dpm_logger_module.DpmPowerUpEntry(
                powerup_counter=6,
                power_fault_cause=None,
                dpm_records=[
                    adm1266_module.Adm1266BlackBoxRecord.from_bytes(
                        create_raw_adm1266_blackbox_record(
                            uid=1000,
                            powerup_counter=6,
                        ),
                        "test-dpm-2",
                    ),
                    adm1266_module.Adm1266BlackBoxRecord.from_bytes(
                        create_raw_adm1266_blackbox_record(
                            uid=1001,
                            powerup_counter=6,
                        ),
                        "test-dpm-2",
                    ),
                ],
            ),
            dpm_logger_module.DpmPowerUpEntry(
                powerup_counter=7,
                power_fault_cause=None,
                dpm_records=[
                    adm1266_module.Adm1266BlackBoxRecord.from_bytes(
                        create_raw_adm1266_blackbox_record(
                            uid=1002,
                            powerup_counter=7,
                        ),
                        "test-dpm-2",
                    ),
                ],
            ),
        ]
        DPM_1 = adm1266_module.Adm1266(
            "test-dpm-1", {"nvmem_path": "/dummy/path", "powerup_counter_path": "/dummy/path"}
        )
        DPM_2 = adm1266_module.Adm1266(
            "test-dpm-2", {"nvmem_path": "/dummy/path", "powerup_counter_path": "/dummy/path"}
        )
        with (
            tempfile.TemporaryDirectory() as tmpdir,
            patch.object(dpm_logger_module.DpmLogger, "HISTORY_DIR", new=tmpdir),
        ):
            # When
            logger = dpm_logger_module.DpmLogger()
            logger.save(CAUSES, {DPM_1: DPM_1_POWERUPS, DPM_2: DPM_2_POWERUPS})
            loaded_data = logger.load()

            # Then
            EXPECTED_DATA = dpm_logger_module.DataV1(
                gen_time=loaded_data.gen_time,
                schema_version=1,
                causes=[
                    dpm_logger_module.CauseV1(
                        source="SW",
                        timestamp="2025-10-02 23:22:56 UTC",
                        cause="reboot",
                        description="n/a",
                    ),
                    dpm_logger_module.CauseV1(
                        source="test-dpm-1",
                        timestamp="2025-10-02 23:26:07 UTC",
                        cause="CPU_CMD_PCYC",
                        description="CPU card commanded power cycle",
                    ),
                ],
                dpms=[
                    dpm_logger_module.DpmV1(
                        name="test-dpm-1",
                        type="adm1266",
                        records=[
                            record.as_dict()
                            for powerup in DPM_1_POWERUPS
                            for record in powerup.dpm_records
                        ],
                    ),
                    dpm_logger_module.DpmV1(
                        name="test-dpm-2",
                        type="adm1266",
                        records=[
                            record.as_dict()
                            for powerup in DPM_2_POWERUPS
                            for record in powerup.dpm_records
                        ],
                    ),
                ],
            )
            assert loaded_data == EXPECTED_DATA

    def test_load_all(self, dpm_logger_module):
        # Given
        with (
            tempfile.TemporaryDirectory() as tmpdir,
            patch.object(dpm_logger_module.DpmLogger, "HISTORY_DIR", new=tmpdir),
        ):
            # 1st file
            with open(
                os.path.join(tmpdir, "reboot-cause-2025_10_02_23_27_07.json"), "w"
            ) as file_handle:
                json.dump(get_fake_data("2025-10-02 23:27:07 UTC"), file_handle)

            # 2nd file
            with open(
                os.path.join(tmpdir, "reboot-cause-2025_10_02_23_29_07.json"), "w"
            ) as file_handle:
                json.dump(get_fake_data("2025-10-02 23:29:07 UTC"), file_handle)

            # 3rd file
            with open(
                os.path.join(tmpdir, "reboot-cause-2025_10_02_23_31_07.json"), "w"
            ) as file_handle:
                json.dump(get_fake_data("2025-10-02 23:31:07 UTC"), file_handle)

            # Create symlink to latest
            os.symlink(
                os.path.join(tmpdir, "reboot-cause-2025_10_02_23_31_07.json"),
                os.path.join(tmpdir, "previous-reboot-cause.json"),
            )

            # When
            logger = dpm_logger_module.DpmLogger()
            loaded_datas = logger.load_all()

            # Then
            assert loaded_datas == [
                get_expected_loaded_data(dpm_logger_module, "2025-10-02 23:27:07 UTC"),
                get_expected_loaded_data(dpm_logger_module, "2025-10-02 23:29:07 UTC"),
                get_expected_loaded_data(dpm_logger_module, "2025-10-02 23:31:07 UTC"),
            ]

    def test_retention(self, dpm_logger_module):
        # Given
        with (
            tempfile.TemporaryDirectory() as tmpdir,
            patch.object(dpm_logger_module.DpmLogger, "HISTORY_DIR", new=tmpdir),
            patch.object(dpm_logger_module.DpmLogger, "RETENTION", new=3),
        ):
            # 1st file
            with open(
                os.path.join(tmpdir, "reboot-cause-2025_10_02_23_27_07.json"), "w"
            ) as file_handle:
                json.dump(get_fake_data("2025-10-02 23:27:07 UTC"), file_handle)

            # 2nd file
            with open(
                os.path.join(tmpdir, "reboot-cause-2025_10_02_23_29_07.json"), "w"
            ) as file_handle:
                json.dump(get_fake_data("2025-10-02 23:29:07 UTC"), file_handle)

            # 3rd file
            with open(
                os.path.join(tmpdir, "reboot-cause-2025_10_02_23_31_07.json"), "w"
            ) as file_handle:
                json.dump(get_fake_data("2025-10-02 23:31:07 UTC"), file_handle)


            # When
            logger = dpm_logger_module.DpmLogger()
            logger.save([], {})

            # Then
            assert not os.path.exists(os.path.join(tmpdir, "reboot-cause-2025_10_02_23_27_07.json"))
            assert os.path.exists(os.path.join(tmpdir, "reboot-cause-2025_10_02_23_29_07.json"))
            assert os.path.exists(os.path.join(tmpdir, "reboot-cause-2025_10_02_23_31_07.json"))
