#!/usr/bin/env python

import datetime
import pytest
import sys

from fixtures.test_helpers_adm1266 import create_raw_adm1266_blackbox_record
from fixtures.test_helpers_common import temp_file
from unittest.mock import Mock, patch, create_autospec


@pytest.fixture
def mock_dpm_logger():
    """Mock the DpmLogger class to avoid writing to filesystem."""
    dpm_logger = Mock()
    dpm_logger.DpmLogger = Mock()
    with patch.dict(sys.modules, {"sonic_platform.dpm_logger": dpm_logger}):
        yield dpm_logger


@pytest.fixture
def reboot_cause_manager_module(mock_dpm_logger):
    """Loads the module before each test. This is to let conftest.py run first."""
    from sonic_platform import reboot_cause_manager

    yield reboot_cause_manager


class TestRebootCauseManager:
    """Test class for RebootCauseManager functionality."""

    def test_summarize_reboot_causes_sw_reboot(self, reboot_cause_manager_module):
        # Given
        with temp_file(
            "User issued 'reboot' command [User: admin, Time: Thu Oct  2 11:22:56 PM UTC 2025]"
        ) as sw_reboot_cause_filepath:
            pddf_plugin_data = {"REBOOT_CAUSE": {"reboot_cause_file": sw_reboot_cause_filepath}}

            # When
            reboot_cause_manager = reboot_cause_manager_module.RebootCauseManager(pddf_plugin_data)
            reboot_causes = reboot_cause_manager.summarize_reboot_causes()

            # Then
            assert len(reboot_causes) == 1
            assert reboot_causes[0] == reboot_cause_manager_module.RebootCause(
                type=reboot_cause_manager_module.RebootCause.Type.SOFTWARE,
                source="SW",
                timestamp=datetime.datetime(2025, 10, 2, 23, 22, 56, tzinfo=datetime.timezone.utc),
                cause="reboot",
                description="User issued 'reboot' command [User: admin, Time: Thu Oct  2 11:22:56 PM UTC 2025]",
                chassis_reboot_cause_category="REBOOT_CAUSE_NON_HARDWARE",
            )

    def test_summarize_reboot_causes_sw_kernel_panic(self, reboot_cause_manager_module):
        # Given
        with temp_file(
            "Kernel Panic - Out of memory [Time: Thu Oct  21 11:22:59 PM UTC 2025]"
        ) as sw_reboot_cause_filepath:
            pddf_plugin_data = {"REBOOT_CAUSE": {"reboot_cause_file": sw_reboot_cause_filepath}}

            # When
            reboot_cause_manager = reboot_cause_manager_module.RebootCauseManager(pddf_plugin_data)
            reboot_causes = reboot_cause_manager.summarize_reboot_causes()

            # Then
            assert reboot_causes == [
                reboot_cause_manager_module.RebootCause(
                    type=reboot_cause_manager_module.RebootCause.Type.SOFTWARE,
                    source="SW",
                    timestamp=datetime.datetime(
                        2025, 10, 21, 23, 22, 59, tzinfo=datetime.timezone.utc
                    ),
                    cause="Kernel Panic - Out of memory",
                    description="Kernel Panic - Out of memory [Time: Thu Oct  21 11:22:59 PM UTC 2025]",
                    chassis_reboot_cause_category="REBOOT_CAUSE_NON_HARDWARE",
                )
            ]

    def test_summarize_reboot_causes_sw_others(self, reboot_cause_manager_module):
        # Given
        FULL_SW_CAUSE = "Some software causes that don't match known patterns"
        with temp_file(FULL_SW_CAUSE) as sw_reboot_cause_filepath:
            pddf_plugin_data = {"REBOOT_CAUSE": {"reboot_cause_file": sw_reboot_cause_filepath}}

            # When
            reboot_cause_manager = reboot_cause_manager_module.RebootCauseManager(pddf_plugin_data)
            reboot_causes = reboot_cause_manager.summarize_reboot_causes()

            # Then
            assert reboot_causes == [
                reboot_cause_manager_module.RebootCause(
                    type=reboot_cause_manager_module.RebootCause.Type.SOFTWARE,
                    source="SW",
                    timestamp=reboot_cause_manager_module.UNKNOWN_TIMESTAMP,
                    cause=FULL_SW_CAUSE,
                    description=FULL_SW_CAUSE,
                    chassis_reboot_cause_category="REBOOT_CAUSE_NON_HARDWARE",
                )
            ]

    def test_summarize_reboot_causes_sw_unknown(self, reboot_cause_manager_module):
        # Given
        with temp_file("Unknown") as sw_reboot_cause_filepath:
            pddf_plugin_data = {"REBOOT_CAUSE": {"reboot_cause_file": sw_reboot_cause_filepath}}

            # When
            reboot_cause_manager = reboot_cause_manager_module.RebootCauseManager(pddf_plugin_data)
            reboot_causes = reboot_cause_manager.summarize_reboot_causes()

            # Then
            assert reboot_causes == []

    def test_summarize_reboot_causes_hw_many_adm1266(
        self, mock_dpm_logger, reboot_cause_manager_module
    ):
        # Given
        # DPM1: [unknown         (p=10), fault_from_pdi1 (p=11)               ]; current_powerup_counter = 12
        # DPM2: [fault_from_pdi1 (p=1) , fault_from_pdi2 (p=2) , unknown (p=3)]; current_powerup_counter = 3
        DPM_1_RECORDS = [
            # 1st powerup - no fault.
            create_raw_adm1266_blackbox_record(uid=1, powerup_counter=10),  # No PDI
            # 2nd powerup - has fault.
            create_raw_adm1266_blackbox_record(uid=2, powerup_counter=11),  # No PDI
            create_raw_adm1266_blackbox_record(
                uid=3,
                powerup_counter=11,
                pdio_in=0b0000_0000_0000_0001,  # PDI1
                timestamp=datetime.datetime(2025, 10, 2, 23, 22, 56, tzinfo=datetime.timezone.utc),
            ),
        ]
        DPM_2_RECORDS = [
            # 1st powerup - has fault.
            create_raw_adm1266_blackbox_record(uid=1234, powerup_counter=1),  # No PDI
            create_raw_adm1266_blackbox_record(
                uid=1235,
                powerup_counter=1,
                pdio_in=0b0000_0000_0000_0001,  # PDI1
                timestamp=datetime.timedelta(seconds=70),
            ),
            # 2nd powerup - has fault.
            create_raw_adm1266_blackbox_record(
                uid=1236,
                powerup_counter=2,
                pdio_in=0b0000_0000_0001_0010,  # PDI5, PDI2
                timestamp=datetime.timedelta(seconds=140),
            ),
            # 3rd powerup - no fault.
            create_raw_adm1266_blackbox_record(uid=1237, powerup_counter=3),  # No PDI
            create_raw_adm1266_blackbox_record(uid=1238, powerup_counter=3),  # No PDI
        ]
        with (
            temp_file(content=b"".join(DPM_1_RECORDS)) as nvmem_path_1,
            temp_file(content=b"".join(DPM_2_RECORDS)) as nvmem_path_2,
            temp_file(content="") as rtc_epoch_offset_path_1,
            temp_file(content="") as rtc_epoch_offset_path_2,
            temp_file(content="12") as powerup_counter_path_1,
            temp_file(content="3") as powerup_counter_path_2,
        ):
            pddf_plugin_data = {
                "DPM": {
                    "test-dpm1": {
                        "type": "adm1266",
                        "nvmem_path": nvmem_path_1,
                        "rtc_epoch_offset_path": rtc_epoch_offset_path_1,
                        "powerup_counter_path": powerup_counter_path_1,
                        "dpm_signal_to_fault_cause": [
                            {
                                "pdio_mask": "0b0000_0000_0000_0001",  # PDI1
                                "gpio_mask": "0b0000_0000_0000_0000",
                                "pdio_value": "0b0000_0000_0000_0001",  # PDI1
                                "gpio_value": "0b0000_0000_0000_0000",
                                "hw_cause": "HW_CAUSE_OF_DPM1_PDI1",
                                "hw_desc": "HW_DESC_OF_DPM1_PDI1",
                                "reboot_cause": "REBOOT_CAUSE_OF_DPM1_PDI1",
                            }
                        ],
                    },
                    "test-dpm2": {
                        "type": "adm1266",
                        "nvmem_path": nvmem_path_2,
                        "rtc_epoch_offset_path": rtc_epoch_offset_path_2,
                        "powerup_counter_path": powerup_counter_path_2,
                        "dpm_signal_to_fault_cause": [
                            {
                                "pdio_mask": "0b0000_0000_0000_0001",  # PDI1
                                "gpio_mask": "0b0000_0000_0000_0000",
                                "pdio_value": "0b0000_0000_0000_0001",  # PDI1
                                "gpio_value": "0b0000_0000_0000_0000",
                                "hw_cause": "HW_CAUSE_OF_DPM2_PDI1",
                                "hw_desc": "HW_DESC_OF_DPM2_PDI1",
                                "reboot_cause": "REBOOT_CAUSE_OF_DPM2_PDI1",
                            },
                            {
                                "pdio_mask": "0b0000_0000_0000_0010",  # PDI2
                                "gpio_mask": "0b0000_0000_0000_0000",
                                "pdio_value": "0b0000_0000_0000_0010",  # PDI2
                                "gpio_value": "0b0000_0000_0000_0000",
                                "hw_cause": "HW_CAUSE_OF_DPM2_PDI2",
                                "hw_desc": "HW_DESC_OF_DPM2_PDI2",
                                "reboot_cause": "REBOOT_CAUSE_OF_DPM2_PDI2",
                            },
                        ],
                    },
                }
            }

            # When
            reboot_cause_manager = reboot_cause_manager_module.RebootCauseManager(pddf_plugin_data)
            reboot_causes = reboot_cause_manager.summarize_reboot_causes()

            # Then
            assert reboot_causes == [
                reboot_cause_manager_module.RebootCause(
                    type=reboot_cause_manager_module.RebootCause.Type.HARDWARE,
                    source="test-dpm2",
                    timestamp=datetime.timedelta(seconds=70),
                    cause="HW_CAUSE_OF_DPM2_PDI1",
                    description="HW_DESC_OF_DPM2_PDI1",
                    chassis_reboot_cause_category="REBOOT_CAUSE_OF_DPM2_PDI1",
                ),
                reboot_cause_manager_module.RebootCause(
                    type=reboot_cause_manager_module.RebootCause.Type.HARDWARE,
                    source="test-dpm1",
                    timestamp=datetime.datetime(
                        2025, 10, 2, 23, 22, 56, tzinfo=datetime.timezone.utc
                    ),
                    cause="HW_CAUSE_OF_DPM1_PDI1",
                    description="HW_DESC_OF_DPM1_PDI1",
                    chassis_reboot_cause_category="REBOOT_CAUSE_OF_DPM1_PDI1",
                ),
            ]
            # Verify that the DPM records are also saved to history.
            mock_dpm_logger.DpmLogger().save.assert_called_once()
            args, _ = mock_dpm_logger.DpmLogger().save.call_args
            causes, dpm_to_reboot_causes = args
            assert isinstance(causes, list), "First param to DpmLogger.save() should be a list"
            assert isinstance(
                dpm_to_reboot_causes, dict
            ), "Second param to DpmLogger.save() should be a dict"
            records = []
            for rc_entries in dpm_to_reboot_causes.values():
                for rc_entry in rc_entries:
                    records.extend(rc_entry.dpm_records)
            assert {
                "uid": "1",
                "dpm_name": "test-dpm1",
                "powerup_counter": "10",
                "pdio_in_[16:1]": "0b0000000000000000",
            }.items() <= records[0].as_dict().items()
            assert {
                "uid": "2",
                "dpm_name": "test-dpm1",
                "powerup_counter": "11",
                "pdio_in_[16:1]": "0b0000000000000000",
            }.items() <= records[1].as_dict().items()
            assert {
                "timestamp": "2025-10-02 23:22:56 UTC",
                "uid": "3",
                "dpm_name": "test-dpm1",
                "powerup_counter": "11",
                "pdio_in_[16:1]": "0b0000000000000001 [PDI1]",
            }.items() <= records[2].as_dict().items()
            assert {
                "uid": "1234",
                "dpm_name": "test-dpm2",
                "powerup_counter": "1",
                "pdio_in_[16:1]": "0b0000000000000000",
            }.items() <= records[3].as_dict().items()
            assert {
                "timestamp": "70.000000s after power-on",
                "uid": "1235",
                "dpm_name": "test-dpm2",
                "powerup_counter": "1",
                "pdio_in_[16:1]": "0b0000000000000001 [PDI1]",
            }.items() <= records[4].as_dict().items()
            assert {
                "timestamp": "140.000000s after power-on",
                "uid": "1236",
                "dpm_name": "test-dpm2",
                "powerup_counter": "2",
                "pdio_in_[16:1]": "0b0000000000010010 [PDI5, PDI2]",
            }.items() <= records[5].as_dict().items()
            assert {
                "uid": "1237",
                "dpm_name": "test-dpm2",
                "powerup_counter": "3",
                "pdio_in_[16:1]": "0b0000000000000000",
            }.items() <= records[6].as_dict().items()
            assert {
                "uid": "1238",
                "dpm_name": "test-dpm2",
                "powerup_counter": "3",
                "pdio_in_[16:1]": "0b0000000000000000",
            }.items() <= records[7].as_dict().items()

    def test_summarize_reboot_causes_hw_unknown(self, reboot_cause_manager_module):
        # Given
        DPM_RECORDS = [
            create_raw_adm1266_blackbox_record(uid=1, powerup_counter=10),  # No PDI
            create_raw_adm1266_blackbox_record(uid=2, powerup_counter=11),  # No PDI
        ]
        with (
            temp_file(content=b"".join(DPM_RECORDS)) as nvmem_path,
            temp_file(content="12") as powerup_counter_path,
        ):
            pddf_plugin_data = {
                "DPM": {
                    "test-dpm": {
                        "type": "adm1266",
                        "nvmem_path": nvmem_path,
                        "powerup_counter_path": powerup_counter_path,
                        "dpm_signal_to_fault_cause": [],
                    }
                }
            }

            # When
            reboot_cause_manager = reboot_cause_manager_module.RebootCauseManager(pddf_plugin_data)
            reboot_causes = reboot_cause_manager.summarize_reboot_causes()

            # Then
            assert reboot_causes == [
                reboot_cause_manager_module.UNKNOWN_HW_REBOOT_CAUSE,
                reboot_cause_manager_module.UNKNOWN_HW_REBOOT_CAUSE,
            ]

    def test_summarize_reboot_causes_hw_two_dpms_no_reboot(self, reboot_cause_manager_module):
        # Given
        DPM_1_RECORDS = []  # empty
        DPM_2_RECORDS = [
            create_raw_adm1266_blackbox_record(uid=1, powerup_counter=10)  # No PDI/faults
        ]
        with (
            temp_file(content=b"".join(DPM_1_RECORDS)) as nvmem_path_1,
            temp_file(content=b"".join(DPM_2_RECORDS)) as nvmem_path_2,
            temp_file(content="1") as powerup_counter_path_1,
            temp_file(content="10") as powerup_counter_path_2,
        ):
            pddf_plugin_data = {
                "DPM": {
                    "test-dpm1": {
                        "type": "adm1266",
                        "nvmem_path": nvmem_path_1,
                        "powerup_counter_path": powerup_counter_path_1,
                        "dpm_signal_to_fault_cause": [],
                    },
                    "test-dpm2": {
                        "type": "adm1266",
                        "nvmem_path": nvmem_path_2,
                        "powerup_counter_path": powerup_counter_path_2,
                        "dpm_signal_to_fault_cause": [],
                    },
                }
            }

            # When
            reboot_cause_manager = reboot_cause_manager_module.RebootCauseManager(pddf_plugin_data)
            reboot_causes = reboot_cause_manager.summarize_reboot_causes()

            # Then
            assert reboot_causes == []

    def test_summarize_reboot_causes_sw_and_hw(self, reboot_cause_manager_module):
        # Given
        DPM_RECORDS = [
            create_raw_adm1266_blackbox_record(
                uid=555,
                powerup_counter=1,
                pdio_in=0b0000_0000_0100_0001,  # PDI7, PDI1
                timestamp=datetime.timedelta(seconds=70),
            ),
        ]
        FULL_SW_CAUSE = (
            "User issued 'reboot' command [User: admin, Time: Thu Oct  2 11:22:56 PM UTC 2025]"
        )
        with (
            temp_file(content=FULL_SW_CAUSE) as sw_reboot_cause_filepath,
            temp_file(content=b"".join(DPM_RECORDS)) as nvmem_path,
            temp_file(content="2") as powerup_counter_path,
        ):
            pddf_plugin_data = {
                "REBOOT_CAUSE": {"reboot_cause_file": sw_reboot_cause_filepath},
                "DPM": {
                    "test-dpm": {
                        "type": "adm1266",
                        "nvmem_path": nvmem_path,
                        "powerup_counter_path": powerup_counter_path,
                        "dpm_signal_to_fault_cause": [
                            {
                                "pdio_mask": "0b0000_0000_0100_0000",  # PDI7
                                "gpio_mask": "0b0000_0000_0000_0000",
                                "pdio_value": "0b0000_0000_0100_0000",  # PDI7
                                "gpio_value": "0b0000_0000_0000_0000",
                                "hw_cause": "HW_CAUSE_OF_PDI7",
                                "hw_desc": "HW_DESC_OF_PDI7",
                                "reboot_cause": "REBOOT_CAUSE_OF_PDI7",
                            },
                        ],
                    }
                },
            }
            # When
            reboot_cause_manager = reboot_cause_manager_module.RebootCauseManager(pddf_plugin_data)
            reboot_causes = reboot_cause_manager.summarize_reboot_causes()

            # Then
            assert reboot_causes == [
                reboot_cause_manager_module.RebootCause(
                    type=reboot_cause_manager_module.RebootCause.Type.SOFTWARE,
                    source="SW",
                    timestamp=datetime.datetime(
                        2025, 10, 2, 23, 22, 56, tzinfo=datetime.timezone.utc
                    ),
                    cause="reboot",
                    description=FULL_SW_CAUSE,
                    chassis_reboot_cause_category="REBOOT_CAUSE_NON_HARDWARE",
                ),
                reboot_cause_manager_module.RebootCause(
                    type=reboot_cause_manager_module.RebootCause.Type.HARDWARE,
                    source="test-dpm",
                    timestamp=datetime.timedelta(seconds=70),
                    cause="HW_CAUSE_OF_PDI7",
                    description="HW_DESC_OF_PDI7",
                    chassis_reboot_cause_category="REBOOT_CAUSE_OF_PDI7",
                ),
            ]

    def test_summarize_reboot_causes_sw_and_unknown_hw(self, reboot_cause_manager_module):
        # Given
        FULL_SW_CAUSE = "Kernel Panic [Time: Thu Oct  21 11:22:59 PM UTC 2025]"
        DPM_RECORDS = [
            create_raw_adm1266_blackbox_record(uid=555, powerup_counter=1),  # No PDI/faults
        ]
        with (
            temp_file(content=FULL_SW_CAUSE) as sw_reboot_cause_filepath,
            temp_file(content=b"".join(DPM_RECORDS)) as nvmem_path,
            temp_file(content="2") as powerup_counter_path,
        ):
            pddf_plugin_data = {
                "REBOOT_CAUSE": {"reboot_cause_file": sw_reboot_cause_filepath},
                "DPM": {
                    "test-dpm": {
                        "type": "adm1266",
                        "nvmem_path": nvmem_path,
                        "powerup_counter_path": powerup_counter_path,
                        "dpm_signal_to_fault_cause": [],
                    }
                },
            }
            # When
            reboot_cause_manager = reboot_cause_manager_module.RebootCauseManager(pddf_plugin_data)
            reboot_causes = reboot_cause_manager.summarize_reboot_causes()

            # Then
            assert reboot_causes == [
                reboot_cause_manager_module.RebootCause(
                    type=reboot_cause_manager_module.RebootCause.Type.SOFTWARE,
                    source="SW",
                    timestamp=datetime.datetime(
                        2025, 10, 21, 23, 22, 59, tzinfo=datetime.timezone.utc
                    ),
                    cause="Kernel Panic",
                    description=FULL_SW_CAUSE,
                    chassis_reboot_cause_category="REBOOT_CAUSE_NON_HARDWARE",
                ),
                reboot_cause_manager_module.UNKNOWN_HW_REBOOT_CAUSE,
            ]

    def test_fill_in_missing_and_align_powerups(self, reboot_cause_manager_module):
        # Given
        #   - max_powerup = 65535
        #   - DPM1: [65531, 1, 2];     current_powerup = 5
        #   - DPM2: [65534, 2, 5, 6];  current_powerup = 6
        #   - DPM3: [];                current_powerup = 1
        #   - Notes: powerup numbers of DPM1 and DPM2 are independent.
        # Should return:
        #   - 9 power cycles (as evidenced from DPM1)
        #   - DPM1: [65531, None, None, None, None, 1, 2, None, None]
        #   - DPM2: [None, None, 65534, None, None, 2, None, None, 5]
        #   - DPM3: [None, None, None, None, None, None, None, None, None]
        dpm_1 = create_autospec(reboot_cause_manager_module.DpmBase)
        dpm_2 = create_autospec(reboot_cause_manager_module.DpmBase)
        dpm_3 = create_autospec(reboot_cause_manager_module.DpmBase)
        dpm_1.max_powerup_counter.return_value = 65535
        dpm_2.max_powerup_counter.return_value = 65535
        dpm_3.max_powerup_counter.return_value = 65535
        dpm_1.get_powerup_counter.return_value = 5
        dpm_2.get_powerup_counter.return_value = 6
        dpm_3.get_powerup_counter.return_value = 1

        DPM_1_POWERUPS = [
            reboot_cause_manager_module.DpmPowerUpEntry(
                powerup_counter=65531,
                power_fault_cause=None,
                dpm_records=[],
            ),
            reboot_cause_manager_module.DpmPowerUpEntry(
                powerup_counter=1,
                power_fault_cause=None,
                dpm_records=[],
            ),
            reboot_cause_manager_module.DpmPowerUpEntry(
                powerup_counter=2,
                power_fault_cause=None,
                dpm_records=[],
            ),
        ]
        DPM_2_POWERUPS = [
            reboot_cause_manager_module.DpmPowerUpEntry(
                powerup_counter=65534,
                power_fault_cause=None,
                dpm_records=[],
            ),
            reboot_cause_manager_module.DpmPowerUpEntry(
                powerup_counter=2,
                power_fault_cause=None,
                dpm_records=[],
            ),
            reboot_cause_manager_module.DpmPowerUpEntry(
                powerup_counter=5,
                power_fault_cause=None,
                dpm_records=[],
            ),
            reboot_cause_manager_module.DpmPowerUpEntry(
                powerup_counter=6,
                power_fault_cause=None,
                dpm_records=[],
            ),
        ]
        DPM_3_POWERUPS = []
        dpm_to_powerups = {
            dpm_1: DPM_1_POWERUPS,
            dpm_2: DPM_2_POWERUPS,
            dpm_3: DPM_3_POWERUPS,
        }

        # When
        result, num_power_cycles = reboot_cause_manager_module.fill_in_missing_and_align_powerups(
            dpm_to_powerups
        )

        # Then
        assert num_power_cycles == 9
        assert result == {
            dpm_1: [
                DPM_1_POWERUPS[0],  # 65531
                None,  # 65532
                None,  # 65533
                None,  # 65534
                None,  # 65535
                DPM_1_POWERUPS[1],  # 1
                DPM_1_POWERUPS[2],  # 2
                None,  # 3
                None,  # 4
            ],
            dpm_2: [
                None,  # 65532
                None,  # 65533
                DPM_2_POWERUPS[0],  # 65534
                None,  # 65535
                None,  # 1
                DPM_2_POWERUPS[1],  # 2
                None,  # 3
                None,  # 4
                DPM_2_POWERUPS[2],  # 5
            ],
            dpm_3: [
                None,  # 65527
                None,  # 65528
                None,  # 65529
                None,  # 65530
                None,  # 65531
                None,  # 65532
                None,  # 65533
                None,  # 65534
                None,  # 65535
            ],
        }

    def test_fill_in_missing_and_align_powerups_and_squash_dpm_powerups(
        self, reboot_cause_manager_module
    ):
        # Given
        #   - max_powerup = 65535
        #   - DPM1: [65533, 65534, 65535, 1, 2]; current_powerup = 3
        #   - DPM2: [6, 8];                      current_powerup = 10
        # Which means:
        #   - DPM1: [FAULT (p=65533), unknown (p=65534), FAULT (p=65535), unknown (p=1), FAULT (p=2)]
        #   - DPM2: [unknown (p=5),   FAULT (p=6),       unknown (p=7),   FAULT   (p=8), unknown (p=9)]
        dpm_1 = create_autospec(reboot_cause_manager_module.DpmBase)
        dpm_2 = create_autospec(reboot_cause_manager_module.DpmBase)
        dpm_1.max_powerup_counter.return_value = 65535
        dpm_2.max_powerup_counter.return_value = 65535
        dpm_1.get_powerup_counter.return_value = 3
        dpm_2.get_powerup_counter.return_value = 10

        DPM_1_POWERUPS = [
            reboot_cause_manager_module.DpmPowerUpEntry(
                powerup_counter=65533,
                power_fault_cause=reboot_cause_manager_module.RebootCause(
                    type=reboot_cause_manager_module.RebootCause.Type.HARDWARE,
                    source="test-dpm-1",
                    timestamp=datetime.datetime(2025, 10, 1, 2, 3, 4, tzinfo=datetime.timezone.utc),
                    cause="HW_CAUSE_DPM1_P65533",
                    description="HW_DESC_DPM1_P65533",
                    chassis_reboot_cause_category="REBOOT_CAUSE_DPM1_P65533",
                ),
                dpm_records=[],
            ),
            reboot_cause_manager_module.DpmPowerUpEntry(
                powerup_counter=65534,
                power_fault_cause=None,
                dpm_records=[],
            ),
            reboot_cause_manager_module.DpmPowerUpEntry(
                powerup_counter=65535,
                power_fault_cause=reboot_cause_manager_module.RebootCause(
                    type=reboot_cause_manager_module.RebootCause.Type.HARDWARE,
                    source="test-dpm-1",
                    timestamp=datetime.datetime(2025, 10, 1, 4, 5, 6, tzinfo=datetime.timezone.utc),
                    cause="HW_CAUSE_DPM1_P65535",
                    description="HW_DESC_DPM1_P65535",
                    chassis_reboot_cause_category="REBOOT_CAUSE_DPM1_P65535",
                ),
                dpm_records=[],
            ),
            reboot_cause_manager_module.DpmPowerUpEntry(
                powerup_counter=1,
                power_fault_cause=None,
                dpm_records=[],
            ),
            reboot_cause_manager_module.DpmPowerUpEntry(
                powerup_counter=2,
                power_fault_cause=reboot_cause_manager_module.RebootCause(
                    type=reboot_cause_manager_module.RebootCause.Type.HARDWARE,
                    source="test-dpm-1",
                    timestamp=datetime.datetime(2025, 10, 1, 7, 8, 9, tzinfo=datetime.timezone.utc),
                    cause="HW_CAUSE_DPM1_P2",
                    description="HW_DESC_DPM1_P2",
                    chassis_reboot_cause_category="REBOOT_CAUSE_DPM1_P2",
                ),
                dpm_records=[],
            ),
        ]
        DPM_2_POWERUPS = [
            reboot_cause_manager_module.DpmPowerUpEntry(
                powerup_counter=6,
                power_fault_cause=reboot_cause_manager_module.RebootCause(
                    type=reboot_cause_manager_module.RebootCause.Type.HARDWARE,
                    source="test-dpm-1",
                    timestamp=datetime.datetime(2025, 10, 1, 3, 4, 5, tzinfo=datetime.timezone.utc),
                    cause="HW_CAUSE_DPM2_P6",
                    description="HW_DESC_DPM2_P6",
                    chassis_reboot_cause_category="REBOOT_CAUSE_DPM2_P6",
                ),
                dpm_records=[],
            ),
            reboot_cause_manager_module.DpmPowerUpEntry(
                powerup_counter=8,
                power_fault_cause=reboot_cause_manager_module.RebootCause(
                    type=reboot_cause_manager_module.RebootCause.Type.HARDWARE,
                    source="test-dpm-1",
                    timestamp=datetime.datetime(2025, 10, 1, 5, 6, 7, tzinfo=datetime.timezone.utc),
                    cause="HW_CAUSE_DPM2_P8",
                    description="HW_DESC_DPM2_P8",
                    chassis_reboot_cause_category="REBOOT_CAUSE_DPM2_P8",
                ),
                dpm_records=[],
            ),
        ]
        dpm_to_powerups = {
            dpm_1: DPM_1_POWERUPS,
            dpm_2: DPM_2_POWERUPS,
        }

        # When
        result, num_power_cycles = reboot_cause_manager_module.fill_in_missing_and_align_powerups(
            dpm_to_powerups
        )

        # Then
        assert num_power_cycles == 5
        assert result == {
            dpm_1: [
                DPM_1_POWERUPS[0],
                DPM_1_POWERUPS[1],
                DPM_1_POWERUPS[2],
                DPM_1_POWERUPS[3],
                DPM_1_POWERUPS[4],
            ],
            dpm_2: [
                None,
                DPM_2_POWERUPS[0],
                None,
                DPM_2_POWERUPS[1],
                None,
            ],
        }

        # And When
        causes = reboot_cause_manager_module.squash_dpms_powerups_to_cause_per_reboot(
            dpm_to_powerups
        )

        # Then
        assert causes == [
            DPM_1_POWERUPS[0].power_fault_cause,
            DPM_2_POWERUPS[0].power_fault_cause,
            DPM_1_POWERUPS[2].power_fault_cause,
            DPM_2_POWERUPS[1].power_fault_cause,
            DPM_1_POWERUPS[4].power_fault_cause,
        ]

    def test_merge_sw_and_hw_causes(self, reboot_cause_manager_module):
        # Given
        DPM_RECORDS = [
            create_raw_adm1266_blackbox_record(
                uid=555,
                powerup_counter=1,
                pdio_in=0b0000_0000_0100_0001,  # PDI7, PDI1
                timestamp=datetime.datetime(2025, 10, 2, 9, 10, 11, tzinfo=datetime.timezone.utc),
            ),
            create_raw_adm1266_blackbox_record(
                uid=556,
                powerup_counter=2,
                pdio_in=0b0000_0000_0100_0000,  # PDI7
                timestamp=datetime.datetime(2025, 10, 3, 9, 10, 11, tzinfo=datetime.timezone.utc),
            ),
        ]
        FULL_SW_CAUSE = (
            "User issued 'reboot' command [User: admin, Time: Thu Oct  2 11:22:56 PM UTC 2025]"
        )
        with (
            temp_file(content=FULL_SW_CAUSE) as sw_reboot_cause_filepath,
            temp_file(content=b"".join(DPM_RECORDS)) as nvmem_path,
            temp_file(content="") as rtc_epoch_offset_path,
            temp_file(content="3") as powerup_counter_path,
        ):
            pddf_plugin_data = {
                "REBOOT_CAUSE": {"reboot_cause_file": sw_reboot_cause_filepath},
                "DPM": {
                    "test-dpm": {
                        "type": "adm1266",
                        "nvmem_path": nvmem_path,
                        "rtc_epoch_offset_path": rtc_epoch_offset_path,
                        "powerup_counter_path": powerup_counter_path,
                        "dpm_signal_to_fault_cause": [
                            {
                                "pdio_mask": "0b0000_0000_0100_0000",  # PDI7
                                "gpio_mask": "0b0000_0000_0000_0000",
                                "pdio_value": "0b0000_0000_0100_0000",  # PDI7
                                "gpio_value": "0b0000_0000_0000_0000",
                                "hw_cause": "HW_CAUSE_OF_PDI7",
                                "hw_desc": "HW_DESC_OF_PDI7",
                                "reboot_cause": "REBOOT_CAUSE_OF_PDI7",
                            },
                        ],
                    }
                },
            }
            # When
            reboot_cause_manager = reboot_cause_manager_module.RebootCauseManager(pddf_plugin_data)
            reboot_causes = reboot_cause_manager.summarize_reboot_causes()

            # Then
            assert reboot_causes == [
                reboot_cause_manager_module.RebootCause(
                    type=reboot_cause_manager_module.RebootCause.Type.HARDWARE,
                    source="test-dpm",
                    timestamp=datetime.datetime(
                        2025, 10, 2, 9, 10, 11, tzinfo=datetime.timezone.utc
                    ),
                    cause="HW_CAUSE_OF_PDI7",
                    description="HW_DESC_OF_PDI7",
                    chassis_reboot_cause_category="REBOOT_CAUSE_OF_PDI7",
                ),
                reboot_cause_manager_module.RebootCause(
                    type=reboot_cause_manager_module.RebootCause.Type.SOFTWARE,
                    source="SW",
                    timestamp=datetime.datetime(
                        2025, 10, 2, 23, 22, 56, tzinfo=datetime.timezone.utc
                    ),
                    cause="reboot",
                    description=FULL_SW_CAUSE,
                    chassis_reboot_cause_category="REBOOT_CAUSE_NON_HARDWARE",
                ),
                reboot_cause_manager_module.RebootCause(
                    type=reboot_cause_manager_module.RebootCause.Type.HARDWARE,
                    source="test-dpm",
                    timestamp=datetime.datetime(
                        2025, 10, 3, 9, 10, 11, tzinfo=datetime.timezone.utc
                    ),
                    cause="HW_CAUSE_OF_PDI7",
                    description="HW_DESC_OF_PDI7",
                    chassis_reboot_cause_category="REBOOT_CAUSE_OF_PDI7",
                ),
            ]

    def test_squash_sw_and_hw_causes(self, reboot_cause_manager_module):
        # Given
        FULL_SW_CAUSE = (
            "User issued 'reboot' command [User: admin, Time: Thu Oct  2 11:22:56 PM UTC 2025]"
        )
        DPM_RECORDS = [
            create_raw_adm1266_blackbox_record(
                uid=555,
                powerup_counter=1,
                pdio_in=0b0000_0001_0000_0000,  # PDI9
                timestamp=datetime.datetime(
                    2025, 10, 2, 23, 24, 56, tzinfo=datetime.timezone.utc  # 2 mins after SW reboot
                ),
            ),
        ]
        with (
            temp_file(content=FULL_SW_CAUSE) as sw_reboot_cause_filepath,
            temp_file(content=b"".join(DPM_RECORDS)) as nvmem_path,
            temp_file(content="") as rtc_epoch_offset_path,
            temp_file(content="2") as powerup_counter_path,
        ):
            pddf_plugin_data = {
                "REBOOT_CAUSE": {"reboot_cause_file": sw_reboot_cause_filepath},
                "DPM": {
                    "test-dpm": {
                        "type": "adm1266",
                        "nvmem_path": nvmem_path,
                        "rtc_epoch_offset_path": rtc_epoch_offset_path,
                        "powerup_counter_path": powerup_counter_path,
                        "dpm_signal_to_fault_cause": [
                            {
                                "pdio_mask": "0b0000_0001_0000_0000",  # PDI9
                                "gpio_mask": "0b0000_0000_0000_0000",
                                "pdio_value": "0b0000_0001_0000_0000",  # PDI9
                                "gpio_value": "0b0000_0000_0000_0000",
                                "hw_cause": "CPU_CMD_PCYC",
                                "hw_desc": "CPU card commanded power cycle",
                                "reboot_cause": "REBOOT_CAUSE_POWER_LOSS",
                            },
                        ],
                    }
                },
            }
            # When
            reboot_cause_manager = reboot_cause_manager_module.RebootCauseManager(pddf_plugin_data)
            reboot_causes = reboot_cause_manager.summarize_reboot_causes()

            # Then
            assert reboot_causes == [
                reboot_cause_manager_module.RebootCause(
                    type=reboot_cause_manager_module.RebootCause.Type.SOFTWARE,
                    source="SW",
                    timestamp=datetime.datetime(
                        2025, 10, 2, 23, 22, 56, tzinfo=datetime.timezone.utc
                    ),
                    cause="reboot",
                    description=FULL_SW_CAUSE,
                    chassis_reboot_cause_category="REBOOT_CAUSE_NON_HARDWARE",
                ),
            ]

    def test_squash_sw_and_hw_causes_longer_than_3m_10s(self, reboot_cause_manager_module):
        # Given
        FULL_SW_CAUSE = (
            "User issued 'reboot' command [User: admin, Time: Thu Oct  2 11:22:56 PM UTC 2025]"
        )
        DPM_RECORDS = [
            create_raw_adm1266_blackbox_record(
                uid=555,
                powerup_counter=1,
                pdio_in=0b0000_0001_0000_0000,  # PDI9
                timestamp=datetime.datetime(
                    2025, 10, 2, 23, 26, 7, tzinfo=datetime.timezone.utc  # 3 mins 11s after SW reboot
                ),
            ),
        ]
        with (
            temp_file(content=FULL_SW_CAUSE) as sw_reboot_cause_filepath,
            temp_file(content=b"".join(DPM_RECORDS)) as nvmem_path,
            temp_file(content="") as rtc_epoch_offset_path,
            temp_file(content="2") as powerup_counter_path,
        ):
            pddf_plugin_data = {
                "REBOOT_CAUSE": {"reboot_cause_file": sw_reboot_cause_filepath},
                "DPM": {
                    "test-dpm": {
                        "type": "adm1266",
                        "nvmem_path": nvmem_path,
                        "rtc_epoch_offset_path": rtc_epoch_offset_path,
                        "powerup_counter_path": powerup_counter_path,
                        "dpm_signal_to_fault_cause": [
                            {
                                "pdio_mask": "0b0000_0001_0000_0000",  # PDI9
                                "gpio_mask": "0b0000_0000_0000_0000",
                                "pdio_value": "0b0000_0001_0000_0000",  # PDI9
                                "gpio_value": "0b0000_0000_0000_0000",
                                "hw_cause": "CPU_CMD_PCYC",
                                "hw_desc": "CPU card commanded power cycle",
                                "reboot_cause": "REBOOT_CAUSE_POWER_LOSS",
                            },
                        ],
                    }
                },
            }
            # When
            reboot_cause_manager = reboot_cause_manager_module.RebootCauseManager(pddf_plugin_data)
            reboot_causes = reboot_cause_manager.summarize_reboot_causes()

            # Then
            assert reboot_causes == [
                reboot_cause_manager_module.RebootCause(
                    type=reboot_cause_manager_module.RebootCause.Type.SOFTWARE,
                    source="SW",
                    timestamp=datetime.datetime(
                        2025, 10, 2, 23, 22, 56, tzinfo=datetime.timezone.utc
                    ),
                    cause="reboot",
                    description=FULL_SW_CAUSE,
                    chassis_reboot_cause_category="REBOOT_CAUSE_NON_HARDWARE",
                ),
                reboot_cause_manager_module.RebootCause(
                    type=reboot_cause_manager_module.RebootCause.Type.HARDWARE,
                    source="test-dpm",
                    timestamp=datetime.datetime(
                        2025, 10, 2, 23, 26, 7, tzinfo=datetime.timezone.utc
                    ),
                    cause="CPU_CMD_PCYC",
                    description="CPU card commanded power cycle",
                    chassis_reboot_cause_category="REBOOT_CAUSE_POWER_LOSS",
                ),
            ]
