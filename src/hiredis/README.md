# Hiredis package

Build Debian Trixie's `hiredis` source `1.2.0-6` with its existing Debian patches
and build configuration. The local patch creates Unix and TCP connection sockets
with `SOCK_CLOEXEC`, retaining the `libhiredis.so.1.1.0` ABI. Downloaded source
archives are verified against `source.sha256`.

```sh
make DEST=/absolute/path/to/artifacts CONFIGURED_ARCH=amd64 SONIC_CONFIG_MAKE_JOBS=4
```

After installing the runtime and development packages, run `make check` in this
directory. The descriptor check covers Unix and TCP connection creation,
reconnect and child execution. It requires a C compiler, Redis server and CLI,
and Python 3.
