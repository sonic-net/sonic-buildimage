# DHCP utilities source wheel

`//src/sonic-dhcp-utilities:sonic_dhcp_utilities_wheel` produces the existing
`sonic-dhcp-utilities` version 1.0 distribution. It preserves the three package
directories, DHCP option CSV, both console scripts, and setup.py metadata.
It does not build a DEB or resolve the image's runtime dependencies.

Build and test with the repository's Bazel 8.5.1 and Trixie configuration:

```sh
bazel build //src/sonic-dhcp-utilities:sonic_dhcp_utilities_wheel
bazel test //src/sonic-dhcp-utilities:upstream_tests \
  //src/sonic-dhcp-utilities:installed_wheel_test
```

The full upstream pytest suite runs through the existing component test runner
and Common's declared native runtime in the matching Trixie userspace. The
declared pytest-cov plugin retains setup.cfg's 80% coverage threshold. The installed-wheel test checks source
bytes, distribution metadata, CSV contents, entry-point imports and validation
behavior from the installed wheel, excluding the component source directory.

`requirements.in` and `requirements_lock.txt` pin build/test imports only.
The wheel retains setup.py's `psutil` and `freezegun` runtime requirements;
the Docker owner resolves the complete image closure from wheel metadata.
To update the test lock, use uv 0.12.18 and preserve existing locked versions:

```sh
uv pip compile src/sonic-dhcp-utilities/requirements.in \
  --python-version 3.13 --generate-hashes --no-emit-index-url \
  --output-file src/sonic-dhcp-utilities/requirements_lock.txt
```

Docker consumers install this wheel through the shared `wheel_layer` rule and
validate its runtime closure. No parallel direct-source installation is needed.
