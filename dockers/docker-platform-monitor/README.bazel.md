# PMON Bazel images

The native AMD64 Trixie VS/FIPS profile builds PMON from the existing
config-engine base, source-built lm-sensors and Platform Daemons, the
platform-common wheel, and the locked PMON Python and Debian runtime sets.

Inside a configured AMD64 Trixie VS/FIPS slave, build the installer inputs
with the same image version and manifests that Make uses for the installer.

```sh
make -f slave.mk BUILD_WITH_BAZEL_WHEN_AVAILABLE=y \
  target/docker-platform-monitor.gz target/docker-platform-monitor-dbg.gz
```

For local development, Bazel can also build the archives with its default
image metadata and run the source checks.

```sh
bazel build //dockers/docker-platform-monitor:docker-platform-monitor.gz \
  //dockers/docker-platform-monitor:docker-platform-monitor-dbg.gz
bazel test //dockers/docker-platform-monitor/bazel:smartcmd_compatibility_test \
  //platform/pddf/platform-api-pddf-base:wheel_contract_test
```

The archives keep the installer tags `docker-platform-monitor:latest` and
`docker-platform-monitor-dbg:latest`. The debug image extends the exact runtime
image with its source symbols, debug tools and the source-built OpenSSH client.
The original SmartCmd vendor executable is static i386. The selector accepts
only its declared path and SHA-256 on AMD64, and checks that it has neither an
interpreter nor a shared-library dependency. Other ELF32 payloads retain the
existing rejection.

The `rules/docker-platform-monitor.mk` bridge selects these archives with
`BUILD_WITH_BAZEL_WHEN_AVAILABLE=y` for native AMD64 Trixie VS, `INCLUDE_FIPS=y`,
`DBG_IMAGE_MARK=dbg`, and ASAN disabled. The default VS image includes the
PDDF Python API and its pinned jsonschema wheel. With `SONIC_USE_PDDF_FRAMEWORK=n`,
Make passes `--//dockers/docker-platform-monitor:pddf=false` to omit that layer.

Both Python layers check their producer wheel metadata against the committed
lock files. Regenerate the affected lock with uv 0.11.0.

```sh
bazel run //dockers/docker-platform-monitor/bazel/python:update -- \
  --uv "$(command -v uv)"
bazel run //dockers/docker-platform-monitor/bazel/pddf:update -- \
  --uv "$(command -v uv)"
```

The registry supplies Platform Daemons' eleven source-built service wheels
and lm-sensors' native files and package metadata. PMON checks the Python
distributions it inherits from config-engine and installs its own service
wheels and locked dependencies in the child layer.
