# Test Directory Structure and Usage

This directory contains the test suite for the Nexthop SONiC platform modules. The tests are organized into unit tests and integration tests with proper isolation between them.

## Directory Structure

```
test/
├── README.md                           # This file
├── conftest.py                         # Global test configuration
├── fixtures/                           # Test fixtures and mocking utilities
│   ├── mock_imports_unit_tests.py      # Unit test specific import mocks
│   ├── adm1266_test_spec.json          # Input & expected output for ADM1266 testings
│   ├── fake_swsscommon.py              # Fake implementations of swsscommon DB.
│   └── test_helpers_adm1266.py         # ADM1266 testing utilities
│   └── test_helpers_chassis.py         # Chassis testing utilities
│   └── test_helpers_common.py          # Common testing utilities
│   └── test_helpers_eeprom.py          # EEPROM testing utilities
├── unit/                               # Unit tests (isolated, mocked environment)
│   ├── conftest.py                     # Unit test configuration
│   ├── nexthop/                        # Tests for nexthop modules
│   │   ├── test_eeprom_utils_unit.py   # EEPROM utilities unit tests
│   │   ├── test_fpga_cli.py            # fpga_cli unit tests
│   │   ├── test_fpga_lib.py            # FPGA library unit tests
│   │   ├── test_gen_cli.py             # gen_cli unit tests
│   │   └── test_led_control.py         # LED control tests
│   │   └── test_pddf_config_parser.py  # PDDF config extraction utilities tests
│   └── sonic_platform/                 # Tests for sonic_platform modules
│       ├── test_adm1266.py             # ADM1266 functionality tests
│       ├── test_chassis.py             # Chassis functionality tests
│       ├── test_fan.py                 # Fan functionality tests
│       └── test_thermal.py             # Thermal & PID controller tests
│       └── test_watchdog.py            # Watchdog functionality tests
└── integration/                        # Integration tests (real environment)
    ├── conftest.py                     # Integration test configuration
    ├── nexthop/                        # Integration tests for nexthop
    │   ├── test_adm1266_chassis_integration.py  # ADM1266 <-> Chassis integration tests
    │   └── test_eeprom_utils_integration.py     # EEPROM integration tests
    └── sonic_platform/                 # Integration tests for sonic_platform
        └── test_chassis_sfp_integration.py      # Chassis <-> SFP integration tests
```

## Test Types

### Unit Tests (`test/unit/`)

**Purpose**: Test individual components in isolation using mocks and fakes. These tests
can be directly run from the development environment.

**Characteristics**:
- No external dependencies
- All modules outside of the platform code are mocked/faked
- Test business logic and component behavior
- Run in any environment

### Integration Tests (`test/integration/`)

**Purpose**: Test components with real SONiC environment dependencies.

**Characteristics**:
- Require SONiC build environment
- Use real SONiC modules when available
- Use no mocks or as minimal mocks as possible


## Running Tests

### Prerequisites

```bash
# Install test dependencies
pip install pytest pytest-asyncio

# For integration tests, ensure SONiC build environment is available
```

### Basic Usage

```bash
# Run all unit tests
pytest test/unit/

# Run all integration tests  
pytest test/integration/

# Run specific test file
pytest test/unit/nexthop/test_led_control.py

# Run specific test function
pytest test/unit/nexthop/test_led_control.py::TestLedControl::test_led_control
```

## Mock Isolation Strategy

The test suite uses a mock isolation strategy to ensure unit and integration tests don't interfere with each other:

### How It Works

1. **Common Configuration**: Applied to all test types via `test/conftest.py`
2. **Unit Test Mocked/Faked Dependencies**: Applied only when running unit tests via pytest fixture in `test/unit/conftest.py`
3. **Integration Test Real Dependencies**: Applied only when running integration tests via pytest fixture in `test/integration/conftest.py`
4. **Testcase Overrides**: Individual tests can override dependencies if needed via pytest fixtures in their respective test files.

### Unit Test Mock Example

**Goal**: Ensure all dependencies are mocked/faked before importing any platform modules.

**Implementation**: It relies on the correct order of pytest fixtures.
- `test/conftest.py` runs first, which exposes `'../common'` directory to the Python path, so all tests can see `sonic_platform`, `nexthop`, and etc.
- `test/unit/conftest.py` runs next and patches required dependencies as mocks or fakes. We need all SONiC dependencies here because `../common/sonic_platform/__init__.py` loads every module, even if the test wants to import just some modules from sonic_platform. This fixture yields so the patch stays in the context of the test run and tears down after each test run.
- Individual test files may override the dependencies if needed, using pytest fixtures. Then, they can import platform modules after all dependencies are patched.

All pytest fixtures are done in a `function` scope, so each testcase will load the modules freshly with the correct mocks/fakes for their usecase.

Below is an example of how `test/unit/sonic_platform/test_fan.py` tests the `sonic_platform.fan` module, which needs a mock of PddfFan. Execution order follows top to bottom:

```python
# test/conftest.py
common_path = os.path.join(os.path.dirname(__file__), "../common")
sys.path.insert(0, common_path)


# test/unit/conftest.py
@pytest.fixture(scope="function", autouse=True)
def patch_dependencies():
    swsscommon = Mock()
    swsscommon.swsscommon.DBConnector = FakeDBConnector
    swsscommon.swsscommon.FieldValuePairs = FakeFieldValuePairs
    swsscommon.swsscommon.Table = FakeTable
    swsscommon.swsscommon.SonicV2Connector = FakeSonicV2Connector

    with patch.dict(sys.modules, {  # <---- All dependencies are patched here.
        "sonic_platform_pddf_base": Mock(),
        "sonic_platform_pddf_base.pddf_fan": Mock(),
        ...
        "swsscommon": swsscommon,
        ...
    }):
        yield  # <---- Help keep the patch active while the test runs.


# test/unit/sonic_platform/test_fan.py
class MockPddfFan:
    """Mock implementation of PddfFan for testing."""

    # Mock methods
    get_presence = Mock()

    def __init__(self, *args, **kwargs):
        self.is_psu_fan = kwargs.get("is_psu_fan", False)
        pass

@pytest.fixture
def mock_pddf_fan():
    pddf_fan = Mock()
    pddf_fan.PddfFan = MockPddfFan
    with patch.dict(sys.modules, {"sonic_platform_pddf_base.pddf_fan": pddf_fan}):  # <---- pddf_fan module is overridden here.
        yield pddf_fan.PddfFan  # <---- Help keep the patch active while the test runs. It also returns MockPddfFan, so the test can use it if needed.

@pytest.fixture
def fan_module(mock_pddf_fan):  # <---- pass mock_pddf_fan as a parameter here, so fan_module runs after it.
    from sonic_platform import fan

    yield fan

def test_fan_get_presence(mock_pddf_fan, fan_module):
    fan = fan_module.Fan(tray_idx=0)

    mock_pddf_fan.get_presence.return_value = True
    assert fan.get_presence() == True
```

### Integration Test Dependency Example

**Goal**: Ensure all dependencies are imported before importing any platform modules.

**Implementation**:
- `test/conftest.py` runs first, which exposes `'../common'` directory to the Python path, so all tests can see `sonic_platform`, `nexthop`, and etc.
- `test/integration/conftest.py` runs next and exposes more SONiC dependencies to the Python path. We use `patch` here under a `module` scope fixture, so it doesn't interfere with unit tests.
- Individual test files may define minimal mocks if needed, using pytest fixtures. Then, they can import platform modules after all dependencies are patched.

```python
# test/conftest.py
common_path = os.path.join(os.path.dirname(__file__), "../common")
sys.path.insert(0, common_path)


# test/integration/conftest.py
@pytest.fixture(scope="module", autouse=True)
def patch_dependencies():
    TEST_DIR = os.path.dirname(os.path.realpath(__file__))
    sonic_platform_common = os.path.join(
        TEST_DIR, "../../../../../src/sonic-platform-common/"
    )
    pddf_base = os.path.join(
        TEST_DIR, "../../../../../platform/pddf/platform-api-pddf-base"
    )

    with patch.object(sys, "path", [sonic_platform_common, pddf_base] + sys.path):  # <---- Patch real dependencies.
        yield  # <---- Help keep the patch active while the test runs.

# Individual test file - can be similar to the unit test example above.
```

## Helper Files

- `fixtures/mock_imports_unit_tests.py`: Comprehensive SONiC mocks/fakes for unit tests only
- `fixtures/fake_swsscommon.py`: Fake implementation of swsscommon DBs
- `fixtures/test_helpers_common.py`: Common test helpers for all tests

## Configuration

Test behavior is configured in `pytest.ini`:

- **Test Discovery**: Automatically finds tests in `test/`
- **File Patterns**: Recognizes `test_*.py` files
- **Function Patterns**: Runs functions starting with `test_`
- **Default Options**: Verbose output, strict markers
- **Python Path**: Adds `test/` to import path for fixture imports
