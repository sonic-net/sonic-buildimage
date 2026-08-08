import os
import pytest
import subprocess

from unittest.mock import patch


@pytest.fixture(scope="function", autouse=True)
def spi_lib_module():
    """Loads the module before each test. This is to let conftest.py inject deps first."""
    from nexthop import spi_lib

    yield spi_lib


@pytest.mark.parametrize(
    "device_name, expected_component",
    [
        ("CPUCARD_CONFIG_FLASH", "CPUCARD_FPGA"),
        ("SWITCHCARD_CONFIG_FLASH_1", "SWITCHCARD_FPGA_1"),
        ("MEZZCARD_CONFIG_FLASH", "MEZZCARD_FPGA"),
        ("ASIC_BOOT_FLASH", "ASIC_PCIE"),
        ("ASIC_BOOT_FLASH_D1", "ASIC_PCIE_D1"),
        ("UNKNOWN_DEVICE", None),
    ]
)
def test_spi_device_to_component(spi_lib_module, device_name, expected_component):
    """Test conversion from SPI device name to component name"""
    result = spi_lib_module.spi_device_to_component(device_name)
    assert result == expected_component


@pytest.mark.parametrize(
    "component_name, expected_device",
    [
        ("CPUCARD_FPGA", "CPUCARD_CONFIG_FLASH"),
        ("SWITCHCARD_FPGA_0", "SWITCHCARD_CONFIG_FLASH_0"),
        ("MEZZCARD_FPGA", "MEZZCARD_CONFIG_FLASH"),
        ("ASIC_PCIE", "ASIC_BOOT_FLASH"),
        ("ASIC_PCIE_D1", "ASIC_BOOT_FLASH_D1"),
        ("UNKNOWN_COMPONENT", None),
    ]
)
def test_component_to_spi_device(spi_lib_module, component_name, expected_device):
    """Test conversion from component name to SPI device name"""
    result = spi_lib_module.component_to_spi_device(component_name)
    assert result == expected_device


def test_get_spi_device_info_basic(spi_lib_module):
    """Test basic get_spi_device_info functionality"""
    mock_pddf_config = {
        "CPUCARD_SPI_CONTROLLER0": {
            "dev_info": {
                "device_type": "SPI_CONTROLLER",
            },
            "dev_attr": {
                "spi_controller_idx": 1,
            }
        },
        "SWITCHCARD_CONFIG_FLASH_0": {
            "dev_info": {
                "device_type": "SPI_DEVICE",
                "device_parent": "CPUCARD_SPI_CONTROLLER0",
            },
            "dev_attr": {
                "chip_select": 0,
            }
        }
    }

    result = spi_lib_module.get_spi_device_info("SWITCHCARD_CONFIG_FLASH_0", mock_pddf_config)

    assert result is not None
    assert result.controller_name == "CPUCARD_SPI_CONTROLLER0"
    assert result.controller_idx == 1
    assert result.component_name == "SWITCHCARD_FPGA_0"
    assert result.device_cs == 0
    assert result.mtd_partition_label is None


def test_get_spi_device_info_mtd_partition_label(spi_lib_module):
    mock_pddf_config = {
        "CPUCARD_SPI_CONTROLLER": {
            "dev_info": {"device_type": "SPI_CONTROLLER"},
            "dev_attr": {"spi_controller_idx": 1},
        },
        "SWITCHCARD_CONFIG_FLASH": {
            "dev_info": {
                "device_type": "SPI_DEVICE",
                "device_parent": "CPUCARD_SPI_CONTROLLER",
            },
            "dev_attr": {
                "chip_select": 0,
                "mtd_partition_label": "swcf-update",
            },
        },
    }
    result = spi_lib_module.get_spi_device_info("SWITCHCARD_CONFIG_FLASH", mock_pddf_config)
    assert result is not None
    assert result.mtd_partition_label == "swcf-update"


def test_get_spi_device_info_missing_device(spi_lib_module):
    """Test get_spi_device_info with non-existent device"""
    mock_pddf_config = {}

    result = spi_lib_module.get_spi_device_info("NONEXISTENT_DEVICE", mock_pddf_config)

    assert result is None


def test_get_spi_device_info_missing_controller(spi_lib_module):
    """Test get_spi_device_info when controller is missing"""
    mock_pddf_config = {
        "SWITCHCARD_CONFIG_FLASH": {
            "dev_info": {
                "device_type": "SPI_DEVICE",
                "device_parent": "NONEXISTENT_CONTROLLER",
            },
            "dev_attr": {
                "chip_select": 1,
            }
        }
    }

    result = spi_lib_module.get_spi_device_info("SWITCHCARD_CONFIG_FLASH", mock_pddf_config)

    assert result is not None
    assert result.controller_name is None
    assert result.controller_idx is None
    assert result.component_name == "SWITCHCARD_FPGA"
    assert result.device_cs == 1


def test_get_spi_device_info_asic_boot_flash(spi_lib_module):
    """Test get_spi_device_info with ASIC boot flash"""
    mock_pddf_config = {
        "SPI_CONTROLLER1": {
            "dev_info": {
                "device_type": "SPI_CONTROLLER",
            },
            "dev_attr": {
                "spi_controller_idx": 1,
            }
        },
        "ASIC_BOOT_FLASH": {
            "dev_info": {
                "device_type": "SPI_DEVICE",
                "device_parent": "SPI_CONTROLLER1",
            },
            "dev_attr": {
                "chip_select": 2,
            }
        }
    }

    result = spi_lib_module.get_spi_device_info("ASIC_BOOT_FLASH", mock_pddf_config)

    assert result is not None
    assert result.controller_name == "SPI_CONTROLLER1"
    assert result.controller_idx == 1
    assert result.component_name == "ASIC_PCIE"
    assert result.device_cs == 2


def test_get_spi_device_info_switchcard(spi_lib_module):
    """Test get_spi_device_info with switchcard config flash"""
    mock_pddf_config = {
        "SPI_CONTROLLER0": {
            "dev_info": {
                "device_type": "SPI_CONTROLLER",
            },
            "dev_attr": {
                "spi_controller_idx": 0,
            }
        },
        "SWITCHCARD_CONFIG_FLASH_0": {
            "dev_info": {
                "device_type": "SPI_DEVICE",
                "device_parent": "SPI_CONTROLLER0",
            },
            "dev_attr": {
                "chip_select": 3,
            }
        }
    }

    result = spi_lib_module.get_spi_device_info("SWITCHCARD_CONFIG_FLASH_0", mock_pddf_config)

    assert result is not None
    assert result.controller_name == "SPI_CONTROLLER0"
    assert result.controller_idx == 0
    assert result.component_name == "SWITCHCARD_FPGA_0"
    assert result.device_cs == 3


@pytest.fixture
def mock_asic_boot_flash_info(spi_lib_module):
    """Stub get_spi_device_info to return a fixed ASIC_BOOT_FLASH SpiDeviceInfo."""
    info = spi_lib_module.SpiDeviceInfo(
        controller_name="SPI_CONTROLLER1",
        controller_idx=1,
        component_name="ASIC_PCIE",
        device_cs=2,
        mtd_partition_label=None,
    )
    with patch.object(spi_lib_module, "get_spi_device_info", return_value=info):
        yield info


def test_get_mtd_device_path_success(spi_lib_module, mock_asic_boot_flash_info):
    """Happy path: directory exists with exactly one writable mtd entry."""
    spi_dev_path = os.path.join(spi_lib_module.SPI_DEV_DIR, "spi1.2")
    mtd_dev_dir = os.path.join(spi_dev_path, "mtd")

    def fake_isdir(path):
        return path in (spi_dev_path, mtd_dev_dir)

    def fake_listdir(path):
        assert path == mtd_dev_dir
        return ["mtd5", "mtd5ro"]

    def fake_exists(path):
        return path == "/dev/mtd5"

    with patch("os.path.isdir", side_effect=fake_isdir), \
         patch("os.listdir", side_effect=fake_listdir), \
         patch("os.path.exists", side_effect=fake_exists):
        assert spi_lib_module.get_mtd_device_path("ASIC_BOOT_FLASH") == "/dev/mtd5"


def test_get_mtd_device_path_missing_spi_dev(spi_lib_module, mock_asic_boot_flash_info):
    """Raises if the spi device path doesn't exist."""
    with patch("os.path.isdir", return_value=False):
        with pytest.raises(Exception, match=r"SPI device spi1\.2 not found"):
            spi_lib_module.get_mtd_device_path("ASIC_BOOT_FLASH")


def test_get_mtd_device_path_missing_mtd_dir(spi_lib_module, mock_asic_boot_flash_info):
    """Raises if the spi dev path exists but has no mtd subdirectory."""
    spi_dev_path = os.path.join(spi_lib_module.SPI_DEV_DIR, "spi1.2")

    def fake_isdir(path):
        return path == spi_dev_path

    with patch("os.path.isdir", side_effect=fake_isdir):
        with pytest.raises(Exception, match=r"mtd directory .* not created"):
            spi_lib_module.get_mtd_device_path("ASIC_BOOT_FLASH")


def test_get_mtd_device_path_multiple_entries(spi_lib_module, mock_asic_boot_flash_info):
    """Raises if mtd dir has more than one writable entry (ambiguous mapping)."""
    with patch("os.path.isdir", return_value=True), \
         patch("os.listdir", return_value=["mtd5", "mtd6", "mtd5ro"]):
        with pytest.raises(Exception, match=r"Expected exactly one mtd device"):
            spi_lib_module.get_mtd_device_path("ASIC_BOOT_FLASH")


def test_get_mtd_device_path_no_writable_entries(spi_lib_module, mock_asic_boot_flash_info):
    """Raises if mtd dir only contains read-only entries."""
    with patch("os.path.isdir", return_value=True), \
         patch("os.listdir", return_value=["mtd5ro"]):
        with pytest.raises(Exception, match=r"Expected exactly one mtd device"):
            spi_lib_module.get_mtd_device_path("ASIC_BOOT_FLASH")


def test_get_mtd_device_path_partition_by_label(spi_lib_module):
    """Select writable MTD by partition name when PDDF sets mtd_partition_label."""
    info = spi_lib_module.SpiDeviceInfo(
        controller_name="CPUCARD_SPI_CONTROLLER",
        controller_idx=1,
        component_name="SWITCHCARD_FPGA",
        device_cs=0,
        mtd_partition_label="swcf-update",
    )
    spi_dev_path = os.path.join(spi_lib_module.SPI_DEV_DIR, "spi1.0")
    mtd_dev_dir = os.path.join(spi_dev_path, "mtd")

    def fake_isdir(path):
        return path in (spi_dev_path, mtd_dev_dir)

    def fake_listdir(path):
        assert path == mtd_dev_dir
        return ["mtd6", "mtd7", "mtd6ro"]

    def fake_exists(path):
        return path in ("/dev/mtd6", "/dev/mtd7")

    def fake_read_mtd_name(mtd_dir, mtd_entry):
        assert mtd_dir == mtd_dev_dir
        return "spi32766.0" if mtd_entry == "mtd6" else "swcf-update" if mtd_entry == "mtd7" else None

    with patch.object(spi_lib_module, "get_spi_device_info", return_value=info), \
            patch("os.path.isdir", side_effect=fake_isdir), \
            patch("os.listdir", side_effect=fake_listdir), \
            patch("os.path.exists", side_effect=fake_exists), \
            patch.object(spi_lib_module, "_read_mtd_name", side_effect=fake_read_mtd_name):
        assert spi_lib_module.get_mtd_device_path("SWITCHCARD_CONFIG_FLASH") == "/dev/mtd7"


def test_get_mtd_device_path_partition_label_missing(spi_lib_module):
    """Raises when mtd_partition_label is set but no matching MTD name exists."""
    info = spi_lib_module.SpiDeviceInfo(
        controller_name="CPUCARD_SPI_CONTROLLER",
        controller_idx=1,
        component_name="SWITCHCARD_FPGA",
        device_cs=0,
        mtd_partition_label="swcf-update",
    )
    spi_dev_path = os.path.join(spi_lib_module.SPI_DEV_DIR, "spi1.0")
    mtd_dev_dir = os.path.join(spi_dev_path, "mtd")

    def fake_isdir(path):
        return path in (spi_dev_path, mtd_dev_dir)

    with patch.object(spi_lib_module, "get_spi_device_info", return_value=info), \
            patch("os.path.isdir", side_effect=fake_isdir), \
            patch("os.listdir", return_value=["mtd6"]), \
            patch.object(spi_lib_module, "_read_mtd_name", return_value="wrong-name"):
        with pytest.raises(Exception, match=r"No MTD partition named 'swcf-update'"):
            spi_lib_module.get_mtd_device_path("SWITCHCARD_CONFIG_FLASH")


def test_get_mtd_device_path_no_pddf_info(spi_lib_module):
    """Raises if get_spi_device_info returns None."""
    with patch.object(spi_lib_module, "get_spi_device_info", return_value=None):
        with pytest.raises(Exception, match=r"Failed to get SPI device info"):
            spi_lib_module.get_mtd_device_path("UNKNOWN_DEVICE")


def test_create_spi_subtree_applies_enables_then_pddfparse(spi_lib_module):
    """create_spi_subtree applies PDDF enables, then pddfparse create."""
    call_order = []

    def track_enable(*_args, **_kwargs):
        call_order.append("enable")

    def track_pddf(*_args, **_kwargs):
        call_order.append("pddf")

    with patch.object(spi_lib_module, "_run_pddfparse_subtree", side_effect=track_pddf) as mock_pddf, \
            patch.object(spi_lib_module, "_apply_pddf_spi_enable_commands", side_effect=track_enable) as mock_enable, \
            patch.object(spi_lib_module, "_assert_update_partition_only"):
        spi_lib_module.create_spi_subtree("SWITCHCARD_CONFIG_FLASH")
        mock_enable.assert_called_once_with("SWITCHCARD_CONFIG_FLASH", None)
        mock_pddf.assert_called_once_with("create", "SWITCHCARD_CONFIG_FLASH")
        assert call_order == ["enable", "pddf"]


def test_create_spi_subtree_rejects_unpartitioned_master(spi_lib_module):
    """create_spi_subtree tears down and raises if the whole-chip master is exposed."""
    info = spi_lib_module.SpiDeviceInfo(
        controller_name="SWITCHCARD_SPI_CONTROLLER0",
        controller_idx=1,
        component_name="SWITCHCARD_FPGA",
        device_cs=0,
        mtd_partition_label="swcf-update",
    )
    mtd_dir = os.path.join(spi_lib_module.SPI_DEV_DIR, "spi1.0", "mtd")
    with patch.object(spi_lib_module, "_apply_pddf_spi_enable_commands"), \
            patch.object(spi_lib_module, "_run_pddfparse_subtree") as mock_pddf, \
            patch.object(spi_lib_module, "get_spi_device_info", return_value=info), \
            patch("os.path.isdir", return_value=True), \
            patch("os.listdir", return_value=["mtd5"]), \
            patch.object(spi_lib_module, "_read_mtd_name", return_value="spi1.0"):
        with pytest.raises(RuntimeError, match=r"unpartitioned master spi1\.0 exposed"):
            spi_lib_module.create_spi_subtree("SWITCHCARD_CONFIG_FLASH0")
        assert ("delete", "SWITCHCARD_CONFIG_FLASH0") in [c.args for c in mock_pddf.call_args_list]


def test_get_spi_device_info_cpucard_mtd_partition(spi_lib_module):
    mock_pddf_config = {
        "CPUCARD_SPI_CONTROLLER3": {
            "dev_info": {"device_type": "SPI_CONTROLLER"},
            "dev_attr": {"spi_controller_idx": 3},
        },
        "CPUCARD_CONFIG_FLASH": {
            "dev_info": {
                "device_type": "SPI_DEVICE",
                "device_parent": "CPUCARD_SPI_CONTROLLER3",
            },
            "dev_attr": {
                "chip_select": 0,
                "mtd_partition_label": "cpucf-update",
            },
        },
    }
    result = spi_lib_module.get_spi_device_info("CPUCARD_CONFIG_FLASH", mock_pddf_config)
    assert result is not None
    assert result.controller_name == "CPUCARD_SPI_CONTROLLER3"
    assert result.controller_idx == 3
    assert result.component_name == "CPUCARD_FPGA"
    assert result.mtd_partition_label == "cpucf-update"


def test_get_mtd_device_path_cpucf_partition_by_label(spi_lib_module):
    """Select cpucf-update MTD on spi3.0."""
    info = spi_lib_module.SpiDeviceInfo(
        controller_name="CPUCARD_SPI_CONTROLLER3",
        controller_idx=3,
        component_name="CPUCARD_FPGA",
        device_cs=0,
        mtd_partition_label="cpucf-update",
    )
    spi_dev_path = os.path.join(spi_lib_module.SPI_DEV_DIR, "spi3.0")
    mtd_dev_dir = os.path.join(spi_dev_path, "mtd")

    def fake_isdir(path):
        return path in (spi_dev_path, mtd_dev_dir)

    def fake_listdir(path):
        assert path == mtd_dev_dir
        return ["mtd8", "mtd9", "mtd8ro"]

    def fake_exists(path):
        return path in ("/dev/mtd8", "/dev/mtd9")

    def fake_read_mtd_name(mtd_dir, mtd_entry):
        assert mtd_dir == mtd_dev_dir
        return "spi32766.0" if mtd_entry == "mtd8" else "cpucf-update" if mtd_entry == "mtd9" else None

    with patch.object(spi_lib_module, "get_spi_device_info", return_value=info), \
            patch("os.path.isdir", side_effect=fake_isdir), \
            patch("os.listdir", side_effect=fake_listdir), \
            patch("os.path.exists", side_effect=fake_exists), \
            patch.object(spi_lib_module, "_read_mtd_name", side_effect=fake_read_mtd_name):
        assert spi_lib_module.get_mtd_device_path("CPUCARD_CONFIG_FLASH") == "/dev/mtd9"


def test_delete_spi_subtree_calls_pddfparse(spi_lib_module):
    """delete_spi_subtree removes the PDDF subtree."""
    with patch.object(spi_lib_module, "_run_pddfparse_subtree") as mock_pddf:
        spi_lib_module.delete_spi_subtree("CPUCARD_CONFIG_FLASH")
        mock_pddf.assert_called_once_with("delete", "CPUCARD_CONFIG_FLASH")
