# Test Directory Structure and Usage

This directory contains the test suite for the Nexthop SONiC platform modules. The tests are organized into unit tests and integration tests with proper isolation between them.

## Directory Structure

```
test/
├── README.md                           # This file
├── conftest.py                         # Global test configuration
├── fixtures/                           # Test fixtures and mocking utilities
│   ├── mock_imports_common.py          # Common import mocks for all test types
│   ├── mock_imports_unit_tests.py      # Unit test specific import mocks
│   ├── fixtures_unit_test.py           # Unit test fixtures
│   └── test_helpers_eeprom.py          # EEPROM testing utilities
├── unit/                               # Unit tests (isolated, mocked environment)
│   ├── conftest.py                     # Unit test configuration
│   ├── nexthop/                        # Tests for nexthop modules
│   │   ├── test_eeprom_utils_unit.py   # EEPROM utilities unit tests
│   │   ├── test_fpga_utils.py          # FPGA utilities tests
│   │   └── test_led_control.py         # LED control tests
│   └── sonic_platform/                 # Tests for sonic_platform modules
│       ├── test_chassis.py             # Chassis functionality tests
│       └── test_pid.py                 # PID controller tests
└── integration/                        # Integration tests (real environment)
    ├── conftest.py                     # Integration test configuration
    └── nexthop/                        # Integration tests for nexthop
        └── test_eeprom_utils_integration.py  # EEPROM integration tests
```

## Test Types

### Unit Tests (`test/unit/`)

**Purpose**: Test individual components in isolation using mocks. These tests
can be directly run from the development environment.

**Characteristics**:
- No external dependencies
- All modules outside of the platform code are mocked
- Test business logic and component behavior
- Run in any environment

### Integration Tests (`test/integration/`)

**Purpose**: Test components with real SONiC environment dependencies.

**Characteristics**:
- Require SONiC build environment
- Use real SONiC modules when available

## Mock Isolation Strategy

The test suite uses a mock isolation strategy to ensure unit and integration tests don't interfere with each other:

### How It Works

1. **Unit Test Mocks**: Applied only when running unit tests via pytest fixture in `test/unit/conftest.py`
2. **Common Mocks**: Basic mocks applied to all test types via `mock_imports_common.py`
3. **No Global Mocks**: Unit test mocks are NOT applied globally, preventing interference

### Mock Files

- `mock_imports_common.py`: Safe mocks for all test types (e.g., click compatibility)
- `mock_imports_unit_tests.py`: Comprehensive SONiC mocks for unit tests only
- `fixtures_unit_test.py`: Pytest fixtures for unit test setup

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

### Configuration

Test behavior is configured in `pytest.ini`:

- **Test Discovery**: Automatically finds tests in `test/`
- **File Patterns**: Recognizes `test_*.py` files
- **Function Patterns**: Runs functions starting with `test_`
- **Default Options**: Verbose output, strict markers
- **Python Path**: Adds `test/` to import path for fixture imports
