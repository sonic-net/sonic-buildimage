# Bazel builder image

Reproducible Debian bookworm container used to run `bazel build` of
SONiC components. Required because the upstream SONiC Bazel rules pull
Debian bookworm libstdc++ which depends on `arc4random@GLIBC_2.36`,
so the host glibc must be >= 2.36 (Ubuntu 22.04 ships glibc 2.35).

## Build

```bash
docker build -t sonic-bazel-builder:bookworm tools/bazel-builder/
```

## Used by

- `rules/docker-orchagent.mk` when `BAZEL_ORCHAGENT=y` is set.
  See that file's header for the full opt-in flow.
