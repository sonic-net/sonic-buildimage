# obmc_ipmi — BMC RTC updater 

## What it does

`obmc_rtc_updater` pushes the host's wall-clock time into the BMC RTC:

1. One host→BMC sync at startup via IPMI **Set SEL Time** (NetFn Storage 0x0a,
   cmd 0x49) issued over `/dev/ipmi0` (`obmc_ipmi_sel_time_set_now()`).
2. It then arms a `CLOCK_REALTIME` timerfd absolute/far-future with
   `TFD_TIMER_ABSTIME | TFD_TIMER_CANCEL_ON_SET` and blocks in `read()`. Any
   **step** of the system clock cancels the read (`ECANCELED`); the daemon
   re-informs the BMC and touches `/run/rtc-timestamp-trigger`.

The host SoC has no battery-backed RTC of its own; `rtc_cmos` is seeded
from the BMC RTC every boot, so keeping the BMC correct keeps the host correct
across reboots and re-installs.

## SONiC-specific integration (outside this shared source)

 SONiC configures chrony to **slew only** (never step) on purpose, so
the host would never produce the step the watcher waits for. The companion
`ciena-8140-bmc-rtc-bootstrap.sh` (run as the service's `ExecStartPre`) waits
(bounded) for chrony to synchronize and issues a single on-demand
`chronyc makestep` — the one step ONEOS's NTP would otherwise have produced —
without changing chrony's steady-state slew-only behaviour.

## Boot-time clock floor (`fake-hwclock`)

The BMC keeps time across host reboots, so a healthy unit always comes up with a
correct clock. If the BMC RTC ever loses time (e.g. supercap expiry) it seeds the
host with a bogus, far-in-the-past time at boot. When NTP is reachable the
bootstrap above corrects this and re-seeds the BMC; but if NTP is **not** yet
reachable the host would otherwise run at ~epoch until it is.

To bound that window the 8140 image ships Debian `fake-hwclock`, which runs
before `sysinit.target`:

- **save** (hourly timer + on shutdown) records the current UTC time to
  `/etc/fake-hwclock.data`.
- **load** (early boot) ratchets the system clock **forward only** — it sets the
  clock to the saved value only when the current clock is earlier, and never
  moves it backwards.

This is a monotonic floor, not a time source: it yields a recent-ish, sane time
until chrony refines it to real time over NTP. Because `load` runs before chrony
and sets the clock via `settimeofday` (not a chrony step), it does not trigger a
premature BMC push — only chrony's NTP-corrected step re-seeds the BMC.

### Version pin: fake-hwclock 0.15 (not trixie's 0.14)

The forward-only `load` behaviour above is only correct with fake-hwclock
**>= 0.15**. Debian **trixie** ships **0.14**, which has an inverted force-test
in its `load` path — Debian bug
[#1093227](https://bugs.debian.org/cgi-bin/bugreport.cgi?bug=1093227). With the
default `FORCE=false`, 0.14 restores the saved timestamp **unconditionally**
(even into the past):

```
# 0.14 (buggy):  if [ "$FORCE"x =  "false"x ] || [ $NOW_SEC -le $SAVED_SEC ]
# 0.15 (fixed):  if [ "$FORCE"x != "false"x ] || [ $NOW_SEC -le $SAVED_SEC ]
```

On the 8140 the kernel seeds `rtc_cmos` from the BMC RTC before `fake-hwclock`
runs, so the boot clock is already correct; 0.14 would roll it **backward** to a
stale floor, and `obmc_rtc_updater` would then push that wrong time to the BMC.
The bug was introduced in 0.14 and fixed in 0.15.

Because SONiC 20260616 branch trixie mirror only carries 0.14, the fixed **0.15**
 deb (`Architecture: all`, no hard dependencies) is vendored under
`platform/broadcom/extra-debs/` and installed on `x86_64-ciena-8140-r0` via
`SONIC_COPY_DEBS` + `_LAZY_INSTALLS` (see `platform-modules-ciena.mk` and
`one-image.mk`). No local override or logic workaround is required. Once the
mirror carries fake-hwclock >= 0.15, the vendored deb can be dropped.

