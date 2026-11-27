# Broadcom SDK initialization diagnostics

Tracks [sonic-buildimage #30781](https://github.com/sonic-net/sonic-buildimage/issues/30781).

The XGS syncd image runs its normal `config_syncd` setup before handing the
resulting command and final `-p` profile to `broadcom_sdk_logging.py`. For a
`.bcm` profile, the helper creates private runtime copies under
`/etc/sai.d/sdk-logging`. It does not edit HwSKU files, the shared merged source
configuration, ConfigDB, `stable_size`, or other ASIC resource settings.
YAML/DNX/LTSW profiles are passed through unchanged.

The generated pre-init script enables the SDK's existing append-mode file
logger, selects SOC COMMON error messages, records the SDK `version` output,
and retains the contents of any existing vendor pre-init script in order.
Existing vendor commands may override SDK logging; the helper does not remove
or rewrite those commands. An unreadable vendor script causes diagnostics to
be skipped rather than starting with an incomplete replacement script.

## Retained files and syslog

Inside the container:

- `/var/log/broadcom-sdk/startup.log`: current SDK output and startup context.
- `/var/log/broadcom-sdk/startup.log.1` and `.2`: retained captures.
- `/var/log/broadcom-sdk/startup.json`: current source profile/BCM paths,
  configured stable size, timestamp and installed SAI package version.

The container-control template binds this directory to host
`/var/log/broadcom-sdk/default` for single-ASIC switches, or a separate numeric
subdirectory for each ASIC. This mount is limited to `docker-syncd-brcm` and
its debug image. The existing techsupport host-log collection includes these
files; no sonic-utilities submodule change is needed. Without the host mount
(for example an already-created container), logs remain container-local.

The monitor checks the file every 250 ms and copy-truncates it at 1 MiB,
retaining two backups of at most 1 MiB each. The active file can exceed its
threshold between polls. This is normal copytruncate retention, not a hard
filesystem quota; a small concurrent-write loss window is possible. Append
mode lets the SDK continue using its open descriptor after truncation.
Old captures are rotated at process startup rather than discarded.

Only the explicit SDK diagnostic `Scache out of space.max=...` generates a
stable-cache-exhaustion summary on stderr, which the existing supervisord
configuration forwards to syslog. The summary includes maximum, used,
requested and available bytes, the source BCM path, stable-size setting,
SAI package version and retained log path. A subsequent failing SDK module
is reported separately. Each is reported once per process to avoid flooding.
The SDK release/build information is retained by the pre-init `version`
command; the ordinary SAI initialization banner also records the SDK version.
Generic `init bcm` or SAI failures are never classified as scache exhaustion.

## Failure and process behavior

No new Python dependency is required. The original syncd/dsserve arguments,
child exit status, stdout and stderr are preserved; SIGTERM/SIGINT are
forwarded to the child. If preparation fails, the original command is executed
with its original profile. Runtime capture I/O errors are reported without
stopping the child. Host log-directory creation failure omits the diagnostic
mount rather than preventing container creation.

## Validation

From the buildimage root:

```bash
python3 -m pytest -q platform/broadcom/docker-syncd-brcm/tests/test_sdk_logging.py
```

Tests use temporary files and fake SDK children. They exercise immutable source
configuration, existing pre-init preservation, profile selection, fragmented
SDK messages, negative controls, rotation with an open append writer, error
bursts, fail-open behavior, exit status, signal forwarding and container
script rendering for Broadcom and other platforms.

The wrapper and supervisor integration were tested directly on AS7326 with
SAI 15.2.1.0.0.0.50.1 / SDK 6.5.35-SP1. With `stable_size=0x5500000`, the
unmodified SDK emitted `Scache out of space.max=89128960 bytes,
used=88887228 bytes, alloc_size=458756 bytes` into the retained raw log.
The wrapper reported the matching exhaustion summary (241732 bytes available)
and subsequent failing module `mim` through the existing supervisor/syslog
path. The original BCM and supervisor files were restored byte-for-byte,
including the working `stable_size=0x8000000`; temporary instrumentation was
removed afterward. All 56 ports returned, the connected spine IPv4/EVPN
sessions re-established, and final system health and all Monit checks were OK.

This was a temporary installation into the existing container. A complete
SONiC image build and the new host log mount/techsupport collection have not
yet been validated on hardware.
