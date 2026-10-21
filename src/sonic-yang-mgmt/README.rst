"
This package includes python yang libraries which will be used with sonic utilities
pacakge to validate the config. This python libraries are written on top of libyang
and also provides functionality to translate the config from SONiC ConfigDB to SONiC
YANG and vice-versa.
"
Building with Bazel
-------------------

From the sonic-buildimage repository root, build the installable wheel and run
the complete existing test suite on native Debian Trixie AMD64 or ARM64::

    bazel build //src/sonic-yang-mgmt:sonic_yang_mgmt_wheel
    bazel test //tools/bazel/tests:sonic_yang_mgmt_full_test \
               //tools/bazel/tests:sonic_yang_mgmt_wheel_test

On ARM64, add ``--platforms=@sonic_build_infra//platforms:aarch64_trixie``
and ``--host_platform=@sonic_build_infra//platforms:aarch64_trixie`` to each
Bazel command. Native CI supplies both flags explicitly.

The wheel includes the three management modules, the ``sonic-cfg-help`` command,
and the Python packages selected by ``setup.py``. The installed-wheel test uses
the shared ``wheel_test_helpers`` contract and installation checks in
`tools/bazel/tests <../../tools/bazel/tests/README.md#shared-wheel-checks>`_.
The package probe uses a temporary environment outside the source checkout.
It loads the production YANG models, round-trips a valid ConfigDB entry and
rejects an invalid port.

``requirements.in`` declares this package's Python dependencies. Config-engine,
py-common and this package use the single generated lock in
``src/sonic-config-engine/requirements_lock.txt``. Refresh that lock from the
repository root with::

    uv pip compile src/sonic-config-engine/requirements.in --python-version 3.13 \
        --generate-hashes --no-emit-index-url \
        --output-file src/sonic-config-engine/requirements_lock.txt

The ordinary Make build and installed paths remain available. Bazel's source
suite supplies declared model and sample-config paths to the existing test
fixtures, so tests need no installation under ``/usr/local`` on the build host.
The native archive CI jobs retain the wheel, hashes, test logs, collected-case
inventory, installation receipt and generated Bazel dependency state.
