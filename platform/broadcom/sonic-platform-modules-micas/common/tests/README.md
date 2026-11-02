# Platform unit tests

These Linux tests exercise platform construction, endpoint I/O and temperature
polling using mocked board APIs and file-backed EEPROM/presence fixtures. They
do not require a switch, Redis instance or kernel driver.

Initialize `src/sonic-platform-common` and `src/sonic-py-common`, then run from the
buildimage repository root:

```sh
python3 -m pip install -r platform/broadcom/sonic-platform-modules-micas/common/tests/requirements.txt
make -C platform/broadcom/sonic-platform-modules-micas/common/tests test
```

The platform-common checkout must provide the Bailly OE/ELS factories and maps
used by this platform. GitHub Actions uses the component versions pinned by the
buildimage checkout; it does not download an unrelated PR branch. A prerequisite
job checks for the paired endpoint support. If it is absent, the workflow reports
that dependency in its summary and explicitly skips the platform test job; this
does not count as a successful test run. Merge the paired API support and update
the component pin to enable the test job. The Make entry fails normally when its
required imports are unavailable.

For local development with a separate component checkout, pass its source path:

```sh
make -C platform/broadcom/sonic-platform-modules-micas/common/tests test \
    SONIC_PLATFORM_COMMON_SRC=/path/to/sonic-platform-common
```

`PYTHON`, `PYTEST_ARGS` and `SONIC_PY_COMMON_SRC` may also be overridden. Public
factory/enum tests belong in platform-common; this suite retains tests that use
the platform endpoint classes or chassis integration.
