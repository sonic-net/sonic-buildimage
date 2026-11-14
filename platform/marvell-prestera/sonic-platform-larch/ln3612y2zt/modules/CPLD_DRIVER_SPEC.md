# Virgo (Larch LN3612Y2ZT) CPLD Kernel Driver — Specification

## 1. Overview

Linux kernel I2C driver for the Virgo main-board CPLD (register map: `Virgo_MB_CPLD register table_V03C.PDF`, rev V03C).

| Item | Value |
|---|---|
| Module name | `virgo_cpld.ko` |
| Source location | `platform/marvell-prestera/sonic-platform-larch/ln3612y2zt/modules/virgo_cpld.c` |
| Driver / I2C device-id strings | `virgo2_cpld` (plus `cpldA`, `cpldB` ids) |
| DT compatible | `custom  ,virgo2_cpld` (note: **two spaces** before the comma — must byte-match the board DTB) |
| CPLD I2C address | `0x75` (7-bit), on parent bus **i2c-18** (pca9548 channel behind i2c-0 → i2c-10) |
| Target kernel | 6.12.41 (SONiC kernel headers) |
| License | GPL v2 |

The driver provides:
1. Flat sysfs attributes under the i2c device directory (`/sys/bus/i2c/devices/18-0075/`).
2. An I2C mux with 14 child adapters pinned to bus numbers **i2c-31 … i2c-44** (SFP28 P1–P12 → i2c-31…42, QSFP28 P13/P14 → i2c-43/44), implemented via the ZIMCR register (0x0A).
3. Probe-time default hardware configuration (see §6).

A `Makefile` exists in `modules/` (kbuild style, `obj-m := virgo_cpld.o`). Debian packaging hookup is out of scope (separate task).

## 2. Device binding

Both instantiation paths must work:

* **Device tree** (primary; how the board actually instantiates both CPLDs): `of_match_table` with compatible `custom  ,virgo2_cpld`; the cpldB node is a child of the i2c-18 adapter at `reg = <0x75>`, the cpldA node a child of i2c-3 at `reg = <0x18>` (§12).
* **Userspace**: `i2c_device_id` table so that
  `echo virgo2_cpld 0x75 > /sys/bus/i2c/devices/i2c-18/new_device` binds the driver (only when the device is not already DT-instantiated).

Probe dispatches on `client->addr` (§12.2): `0x75` → MB/cpldB path (§3–§6), `0x18` → CPU/cpldA path (§12), other addresses → `-ENODEV`.

## 3. I2C mux (ZIMCR, 0x0A)

* Register `i2c_mux_core` with 14 channels; use **forced bus numbers 31–44** (`i2c_mux_add_adapter(muxc, force_nr = 31 + chan, chan)`), channel `chan` 0–13 maps to ZIMCR select value `chan + 1` (0x01 = SFP28-P1 … 0x0E = QSFP28-P14).
* **select**: write `chan + 1` to ZIMCR.
* **deselect (idle-disconnect)**: after every transaction on a child bus, write `0x00` to ZIMCR (all module SCL disconnected).
* Probe must fail gracefully (cleanup, `-EBUSY`) if any forced bus number is already taken.
* The base bus number (31) must stay in sync with the platform init script (`ln3612y2zt-init.sh`, `SFP_MUX_START=30`, port N → bus 30+N) and `sonic_platform/sfp.py` (`SFP_MUX_BASE_BUS = 30`).

## 4. Sysfs interface

All attributes live in the CPLD device dir. Style: **flat, type-prefixed per-port attributes**. Values are **polarity-normalized logical values** (1 = present / asserted / true) unless noted. Read-only attrs `0444`, read-write `0644`. All register access serialized by a single driver mutex (shared with the mux select/deselect path).

### 4.1 Identification (CCHWVR, 0x00) — formatted strings

| Attribute | Access | Format |
|---|---|---|
| `cpld_version` | RO | `V%02u` for release builds, `T%02u` for test builds (bit4: 0=release, 1=test; bits[3:0]=version). Example: `V03`, `T03` |
| `hw_revision` | RO | `R1.00` (bits[7:5]=001), `R2.00` (=010); unknown codes: `R?.?? (raw)` with raw value |

### 4.2 Fan / PSU status (MCR1 0x01, PSR1 0x03)

| Attribute | Access | Source | Normalized value |
|---|---|---|---|
| `fan_present` | RO | MCR1 bit1 (FAN_PRESENTn) | 1 = fan installed |
| `psu1_present` | RO | PSR1 bit4 (PSU-A Present_N) | 1 = inserted |
| `psu2_present` | RO | PSR1 bit5 (PSU-B Present_N) | 1 = inserted |
| `psu1_ac_ok` | RO | PSR1 bit0 (PSU_A_ACOK) | 1 = AC power good |
| `psu2_ac_ok` | RO | PSR1 bit1 (PSU_B_ACOK) | 1 = AC power good |
| `psu1_dc_ok` | RO | PSR1 bit2 (PSU-A PWOK) | 1 = 12V power good |
| `psu2_dc_ok` | RO | PSR1 bit3 (PSU-B PWOK) | 1 = 12V power good |

PSU1 = PSU-A, PSU2 = PSU-B.

### 4.3 LED control (LEDCR1 0x05, LEDCR2 0x06, LEDCR3 0x07) — raw numeric 0–6

Shared 3-bit encoding (write of value > 6 → `-EINVAL`; 7 is undefined):

`0`=green on, `1`=red on, `2`=off, `3`=green+red on, `4`=green blink, `5`=red blink, `6`=green+red blink

| Attribute | Access | Register / field |
|---|---|---|
| `led_system` | RW | LEDCR1 bits[7:5] |
| `led_power` | RW | LEDCR1 bits[4:2] |
| `led_fan` | RW | LEDCR2 bits[2:0] |
| `led_gnss` | RW | LEDCR3 bits[5:3] |
| `led_sync` | RW | LEDCR3 bits[2:0] |

Writes are read-modify-write preserving other fields (e.g. SLED_EN in LEDCR2, reserved bits in LEDCR3).

### 4.4 PTP status (PTPCSR1, 0x08) — single raw hex attribute

| Attribute | Access | Format |
|---|---|---|
| `ptp_status` | RO | `0x%02x` — whole PTPCSR1 register |

Note: bit7 (PTP_ALERTn) is R/C; a read of this attribute clears the latched alert. Bit layout (for consumers): b7 PTP_ALERT, b6 RCVR1_CLK LOS, b5 GNSS_10MHz LOS, b4 SMA_10MHz LOS, b3 RCVR0_CLK LOS, b2 GNSS_1PPS LOS, b1 ToD_1PPS LOS, b0 SMA_1PPS LOS.

### 4.5 SFP28 ports 1–12

| Attribute (N = 1…12) | Access | Register | Normalized value |
|---|---|---|---|
| `sfpN_present` | RO | ZMPSCR1 0x11 (P1–8), ZMPSCR2 0x12 (P9–12) | 1 = module inserted |
| `sfpN_interrupt` | RO | MISRR2 0x0C (P1–8), MISRR3 0x0D (P9–12) — R/C, cached (§5) | 1 = interrupt occurred since last read |
| `sfpN_rx_los` | RO | ZMLSCR1 0x13 / ZMLSCR2 0x14 | 1 = loss of signal |
| `sfpN_tx_fault` | RO | ZMTFSCR1 0x15 / ZMTFSCR2 0x16 | 1 = TX fault |
| `sfpN_tx_disable` | RW | ZTDSCR1 0x17 / ZTDSCR2 0x18 | 1 = transmitter disabled |
| `sfpN_speed_led` | RW | ZLSR1 0x23 / ZLSR2 0x24 | 0 = green (25G), 1 = amber (1G/10G) |

No `sfpN_reset` — the CPLD has no SFP reset control.

### 4.6 QSFP28 ports 13–14

| Attribute (N = 13, 14) | Access | Register | Value |
|---|---|---|---|
| `qsfpN_present` | RO | ZSQCR2 0x0F bits0/1 | 1 = module inserted |
| `qsfpN_interrupt` | RO | MISRR1 0x0B bits0/1 — R/C, cached (§5) | 1 = interrupt occurred since last read |
| `qsfpN_reset` | RW | ZSQCR1 0x0E, 2-bit fields (P13 bits[1:0], P14 bits[3:2]) | **Write-1-to-pulse**: write `1` → driver writes field to `10` (HW pulses reset, auto-clears to `01`). Write `0` → no-op. Read: `1` if field is currently `00` (held in reset) or `10` (pulsing), `0` if `01` (normal operation) |
| `qsfpN_lpmode` | RW | ZSQCR3 0x10 bits0/1 | 1 = low power mode enabled |
| `qsfpN_speed_led` | RW | ZQLSR 0x22 bits0/1 | 0 = green (100G/50G/25G-breakout), 1 = amber (40G/20G/1-10G-breakout) |

## 5. Interrupt latch handling (R/C registers)

Registers MISRR1 (0x0B), MISRR2 (0x0C), MISRR3 (0x0D) are clear-on-read; one hardware read clears latches for all ports sharing the register. Strategy: **on-demand read with software latch cache**.

* Driver keeps a cached latch bitmap per register (`u8 misrr_cache[3]`), protected by the driver mutex.
* On any `sfpN_interrupt` / `qsfpN_interrupt` read:
  1. Read the corresponding hardware register (this clears HW latches).
  2. Fold newly-asserted bits into the cache: `cache |= ~hw_value` (bits are active-low: 0 = interrupt active).
  3. Report the requested port's cached bit (normalized: 1 = interrupt occurred).
  4. Clear only that port's bit in the cache.
* No polling timer, no IRQ line.

## 6. Probe sequence

1. Verify CPLD responds: read CCHWVR (0x00); log version/revision (`dev_info`). Probe fails with `-ENODEV` on read error.
2. Apply default configuration (exact writes):
   1. `ZSQCR1 (0x0E) ← 0x05` — both QSFP reset fields = `01` (release from reset, normal operation)
   2. `ZSQCR3 (0x10) ← 0x00` — LPMODE disabled on both QSFP
   3. `ZTDSCR1 (0x17) ← 0x00` — TX enabled, SFP P1–P8
   4. `ZTDSCR2 (0x18) ← 0x00` — TX enabled, SFP P9–P12
   5. `LEDCR2 (0x06)`: read-modify-write, set bit3 (SLED_EN) = 1, preserve other bits
   6. `ZIMCR (0x0A) ← 0x00` — mux disconnected
3. Create sysfs attribute group.
4. Register the i2c mux with 14 forced-numbered child adapters (31–44).
5. On any failure: full cleanup in reverse order, return error.

`remove()`: delete mux adapters, remove sysfs group. Do not touch tx_disable/reset state on removal.

## 7. Architecture & data handling

### 7.1 Source layout

Single source file `virgo_cpld.c` plus a minimal kbuild `Makefile` matching the
other `sonic-platform-larch` modules (kbuild supplies the `modules`/`clean`
targets):

```makefile
obj-m := virgo_cpld.o
```

The module is compiled against the SONiC kernel headers built from
`src/sonic-linux-kernel` (installed as `linux-headers-*.deb` in the build
docker). The platform `debian/rules` (`platform/marvell-prestera/sonic-platform-larch/debian/rules`)
drives the build for each entry in `MODULE_DIRS_ARM64` via:

```sh
make modules -C $(KERNEL_SRC)/build M=$(MOD_SRC_DIR)/ln3612y2zt/modules
```

`ln3612y2zt` is already listed in `MODULE_DIRS_ARM64`; its `override_dh_auto_build`
branch must also invoke the `modules/` build line above (as `cn9130-eps6800`
does) so `virgo_cpld.ko` is produced and packaged. Wiring that line into
`debian/rules` is handled by the separate packaging task.

### 7.2 Private data

```c
struct virgo_cpld_data {
    struct i2c_client *client;
    struct mutex      lock;          /* serializes ALL CPLD register access */
    struct i2c_mux_core *muxc;
    u8                misrr_cache[3];/* sw interrupt latches: [0]=MISRR1, [1]=MISRR2, [2]=MISRR3 */
};
```

* `mutex` is taken by every sysfs show/store, the mux `select`/`deselect` callbacks, and probe defaults. No nested locking; the mux callbacks run in i2c transfer context of the *child* adapter, which never re-enters the CPLD attribute paths.
* Interrupt latch caches store **normalized** (active-high) bits.
* No other register caching — every attribute read hits hardware (values are cheap single-byte SMBus reads and hardware state can change asynchronously).

### 7.3 Attribute implementation pattern

* Use `SENSOR_DEVICE_ATTR_2`-style or a custom `dev_attr` array with per-attr `(reg, bit/field-shift, width, polarity)` descriptors so all 80+ attributes share ≈4 generic show/store functions (bit-status show, bit RW show/store, LED field show/store, plus special-cased `cpld_version`, `hw_revision`, `ptp_status`, `qsfpN_reset`, `sfpN/qsfpN_interrupt`).
* Register all attributes with a single `struct attribute_group` via `sysfs_create_group()` (or `devm` equivalent).
* Read-modify-write for any partial-register store; entire RMW under the mutex.

### 7.4 Data validation

| Input | Rule |
|---|---|
| LED attrs | `kstrtou8`; accept 0–6, else `-EINVAL` |
| Boolean RW attrs (`tx_disable`, `lpmode`, `speed_led`, `reset`) | `kstrtobool` semantics: accept `0/1/y/n`; else `-EINVAL` |
| All shows | return `sysfs_emit()` formatted values with trailing newline |

## 8. Error handling strategy

| Condition | Behavior |
|---|---|
| CCHWVR read fails at probe | `dev_err`, return `-ENODEV` (no retry loop) |
| Default-config write fails at probe | `dev_err` naming the register, abort probe with the SMBus error code |
| Forced mux bus number 31–44 already in use | `i2c_mux_add_adapter` failure → delete already-added adapters, full cleanup, propagate error (typically `-EBUSY`) |
| SMBus error in sysfs show/store | propagate negative errno to userspace (read()/write() fails); no dmesg spam — use `dev_dbg` only |
| SMBus error in mux `select` | return the error; the i2c core fails the child-bus transaction. Do not leave stale channel: `deselect` still attempts ZIMCR=0x00 |
| SMBus error in mux `deselect` | log `dev_warn` (rate-limited) — a stuck-selected channel can block other ports; return error to core |
| Invalid userspace input | `-EINVAL`, hardware untouched |
| Write of `0` to `qsfpN_reset` | accepted, no-op, returns count |
| Concurrent access | all paths mutex-serialized; no partial RMW visible |
| `remove()` | always succeeds: `i2c_mux_del_adapters()`, `sysfs_remove_group()`; hardware left in last-configured state |

Use `devm_` APIs (`devm_kzalloc`, `devm_i2c_mux_alloc` if available) so error unwinding is mostly automatic; anything manual must unwind in reverse probe order.

## 9. Testing plan

Target: LN3612Y2ZT board running SONiC (kernel 6.12.41). All tests from root shell.

### 9.1 Build & load

1. `make KDIR=<sonic-kernel-headers>` — builds clean, no warnings (`-Werror` recommended locally).
2. Primary path: boot with the board DTB — both CPLD nodes auto-probe via the `custom  ,virgo2_cpld` compatible. Manual path (non-DT device only): `insmod virgo_cpld.ko; echo virgo2_cpld 0x75 > /sys/bus/i2c/devices/i2c-18/new_device`.
3. Verify: dmesg shows probe log with CPLD version/revision; `/sys/bus/i2c/devices/18-0075/` exists and contains all attributes; `i2cdetect -l` lists 14 new adapters i2c-31…i2c-44.
4. Unbind (`echo 18-0075 > /sys/bus/i2c/drivers/virgo2_cpld/unbind`) and rebind — no errors, no leaked adapters (`ls /sys/class/i2c-adapter`).
5. `rmmod virgo_cpld` with device bound — clean removal.

### 9.2 Probe defaults verification

After probe, verify via `i2cget -y 18 0x75 <reg>` (note: raw i2cget races the driver and requires `-f` once the driver owns the address; prefer checking via attrs):

* `qsfp13_reset`/`qsfp14_reset` read `0` (ZSQCR1 = 0x05), `qsfpN_lpmode` = 0, all `sfpN_tx_disable` = 0, LEDCR2 bit3 (SLED_EN) = 1, ZIMCR = 0x00.

### 9.3 Sysfs functional tests

| Test | Expected |
|---|---|
| `cat cpld_version`, `cat hw_revision` | Formatted strings matching label/known CPLD build (e.g. `V03`, `R2.00`) |
| `cat fan_present` with fan tray in/out | `1` / `0` |
| `cat psu1_present psu1_ac_ok psu1_dc_ok` with PSU-A inserted/powered, then AC pulled, then removed | `1 1 1` → `1 0 0` → `0 0 0` (repeat for psu2/PSU-B) |
| Write each of 0–6 to every `led_*` attr | LED visually matches encoding; readback matches; write `7` → `EINVAL`, register unchanged |
| LED RMW isolation | Set `led_fan=1`, verify LEDCR2 SLED_EN still 1; set `led_gnss`, verify `led_sync` field unchanged |
| `cat sfpN_present` for each port with module in/out | `1`/`0` per port, correct port→bit mapping (test at least P1, P8, P9, P12 for register-boundary correctness) |
| `qsfp13_present`, `qsfp14_present` | correct per-slot |
| `sfpN_rx_los`, `sfpN_tx_fault` | with live optics + fiber pull: rx_los 0→1; loopback restores 0 |
| `echo 1 > sfpN_tx_disable` | peer sees link down / RX power drops; readback 1; `echo 0` restores |
| `echo 1 > qsfp13_reset` | module resets (I2C to module briefly unresponsive, then recovers); readback returns 0 after pulse |
| `echo 1 > qsfpN_lpmode` | module power class drops (verify via DOM/temperature); readback 1 |
| `sfpN_speed_led`, `qsfpN_speed_led` | port LED color toggles green/amber |
| `cat ptp_status` | hex value; LOS bits change when SMA/GNSS inputs connected/disconnected |

### 9.4 Interrupt latch cache tests

1. Trigger an interrupt on SFP-P1 (e.g. module hot-plug); `cat sfp1_interrupt` → `1`; immediate second read → `0`.
2. Trigger interrupts on P1 and P2 simultaneously (both in MISRR2); read `sfp1_interrupt` (clears HW latch); then `cat sfp2_interrupt` → must still report `1` (cache preserved it).
3. Repeat across register boundary: P8 (MISRR2) and P9 (MISRR3) independent.

### 9.5 Mux tests

1. With module in SFP-P1: `i2cdetect -y 31` shows 0x50; `i2cdump -y 31 0x50` returns valid SFF-8472 data.
2. Verify channel mapping end-to-end: module only in P14 → EEPROM visible on i2c-44 only, all other child buses empty.
3. Idle-disconnect: after any child-bus transaction, ZIMCR reads 0x00.
4. Concurrency: parallel `i2cdump` loops on i2c-31 and i2c-44 plus a `cat`-all-attrs loop on the CPLD — no corruption, no lockups, EEPROM data stays valid (run ≥10 min).
5. Bus-number collision: instantiate a dummy adapter occupying bus 31 first → probe fails cleanly with error, no partial adapters left.

### 9.6 Stress / soak

* `while true; do cat /sys/bus/i2c/devices/18-0075/* &>/dev/null; done` for 1 hour — no dmesg errors, no memory growth (`slabtop`).
* Repeated bind/unbind (100×) — no leaks (kmemleak enabled build recommended once).
* Verify xcvrd/pmon integration afterwards: `show interfaces transceiver presence` agrees with sysfs.

## 10. Implementation notes

* Use plain `i2c_smbus_read_byte_data` / `i2c_smbus_write_byte_data`; no regmap dependency required.
* Out of scope (may be added later): SRR1 (0x20) reset controls, MCR2 (0x02) misc controls, PSR2 (0x04) PSU power-on, LEDCR2 LED_Force_On, PTPCSR2.
* `MODULE_AUTHOR("Larch Networks")`, `MODULE_DESCRIPTION("Virgo MB CPLD driver")`, `MODULE_LICENSE("GPL")`.

## 11. Register map quick reference (V03C)

| Reg | Addr | Type | Use in driver |
|---|---|---|---|
| CCHWVR | 0x00 | R | cpld_version / hw_revision |
| MCR1 | 0x01 | R | fan_present (bit1) |
| PSR1 | 0x03 | R | PSU present / AC OK / DC OK |
| LEDCR1 | 0x05 | RW | led_system, led_power |
| LEDCR2 | 0x06 | RW | led_fan, SLED_EN (probe) |
| LEDCR3 | 0x07 | RW | led_gnss, led_sync |
| PTPCSR1 | 0x08 | R(/C b7) | ptp_status |
| ZIMCR | 0x0A | RW | i2c mux select |
| MISRR1 | 0x0B | R/C | qsfp13/14_interrupt |
| MISRR2 | 0x0C | R/C | sfp1–8_interrupt |
| MISRR3 | 0x0D | R/C | sfp9–12_interrupt |
| ZSQCR1 | 0x0E | RW | qsfp13/14_reset |
| ZSQCR2 | 0x0F | R | qsfp13/14_present |
| ZSQCR3 | 0x10 | RW | qsfp13/14_lpmode |
| ZMPSCR1/2 | 0x11/0x12 | R | sfp1–12_present |
| ZMLSCR1/2 | 0x13/0x14 | R | sfp1–12_rx_los |
| ZMTFSCR1/2 | 0x15/0x16 | R | sfp1–12_tx_fault |
| ZTDSCR1/2 | 0x17/0x18 | RW | sfp1–12_tx_disable |
| ZQLSR | 0x22 | RW | qsfp13/14_speed_led |
| ZLSR1/2 | 0x23/0x24 | RW | sfp1–12_speed_led |

## 12. cpldA — CPU board CPLD (0x18)

### 12.1 Overview

The same driver also handles the Virgo **CPU board CPLD** ("cpldA", spec:
`Virgo_CPU_BD_V300_CPLD Function List_V0.03_20231215.pdf`), located at address
**0x18** on **i2c-3**. Scope is deliberately minimal: expose **version,
revision and the CPU watchdog timer** — nothing else (reset control, boot
select, etc. are out of scope for now).

| Item | cpldA | cpldB (MB, §1–§11) |
|---|---|---|
| I2C address | `0x18`, bus i2c-3 | `0x75`, bus i2c-18 |
| Sysfs dir | `/sys/bus/i2c/devices/3-0018/` | `/sys/bus/i2c/devices/18-0075/` |
| I2C mux | none | 14 channels |
| Probe defaults | **none** (hands-off) | §6 transceiver defaults |
| Attributes | `cpld_version`, `hw_revision`, `wd_*` | §4 |

### 12.2 Binding & dispatch

* Both CPLDs are DT-instantiated with the **same compatible** string, so
  `client->name` is identical for both — the driver **dispatches on
  `client->addr`**: `0x18` → cpldA path, `0x75` → cpldB path (existing
  behavior, unchanged). Any other address → `-ENODEV`.
* The `i2c_device_id` table keeps distinct `cpldA` / `cpldB` entries (with
  `driver_data` `CPLDA`/`CPLDB`) for manual `new_device` instantiation, but DT
  matching must not rely on it.
* Each variant registers its **own attribute group**; cpldA must not expose
  any cpldB attribute and vice versa.

### 12.3 cpldA register map

| Reg | Addr | Type | Layout |
|---|---|---|---|
| CCBHVR | 0x00 | R | bits[7:5] HW_Ver (001=v100, 010=v200, 011=v300/v310); bits[4:0] CPLD code version. Default 0x63 |
| WTR | 0x08 | RW | bit7 `WDT_CPU_Flag` (R, timeout flag, cleared by WTR read); bit6 `WDT_CPU_TRI` (R, WD-triggered status, cleared by WTR read); bits[3:1] `WDT_Value` (000=1 s, 001=3 s, 010=5 s default, 011=10 s, 100=30 s, 101=60 s, 110=120 s, 111=180 s); bit0 `WDT_CPU_ENBn` (0=enable, 1=disable, default disabled) |
| WTD | 0x09 | R | bits[7:0] remaining countdown time in seconds |

### 12.4 Sysfs attributes (cpldA)

All in `/sys/bus/i2c/devices/3-0018/`; same locking/validation rules as §4/§7.

| Attribute | Access | Source | Semantics |
|---|---|---|---|
| `cpld_version` | RO | CCBHVR bits[4:0] | Same name/format as cpldB: `V%02u` (5-bit code, no T/V flag; e.g. `V03`) |
| `hw_revision` | RO | CCBHVR bits[7:5] | `R1.00` (001), `R2.00` (010), `R3.00` (011); unknown → `R?.?? (raw)` |
| `wd_enable` | RW | WTR bit0 (inverted) | 1 = watchdog running (writes ENBn=0), 0 = stopped. RMW preserves WDT_Value |
| `wd_timeout` | RW | WTR bits[3:1] | Seconds. Write: accept 1–180, **round up** to the next supported step (1/3/5/10/30/60/120/180; e.g. 45 → 60); >180 or ≤0 → `-EINVAL`. Read: effective value in seconds. RMW preserves enable bit |
| `wd_kick` | WO (0200) | WTR | Write `1`: rewrite current WTR value to reload the countdown ("feed"). Write `0`: no-op (returns count). Reads not supported |
| `wd_timeout_flag` | RO | WTR bit7 (cached, §12.5) | 1 = watchdog timed out since last read of this attribute; read-to-clear (cache only) |
| `wd_triggered` | RO | WTR bit6 (cached, §12.5) | 1 = WD reset triggered since last read of this attribute; read-to-clear (cache only) |
| `wd_remaining` | RO | WTD | Remaining countdown in seconds (raw) |

### 12.5 WTR flag latch handling

Hardware clears WTR bits 6–7 on **any** WTR read — including the driver's
internal reads for `wd_enable`/`wd_timeout` shows and every RMW. Same software
latch strategy as §5:

* Driver keeps a cached WTR flag bitmap (`u8 wtr_cache`) under the driver mutex.
* Every internal WTR read folds bits 6–7 into the cache **before** using the
  value: `wtr_cache |= val & (BIT(7) | BIT(6))` (bits are active-high).
* `wd_timeout_flag` / `wd_triggered` report from the cache and clear **only
  their own** cached bit; no other attribute consumes the flags.
* RMW writes to WTR must mask bits 6–7 to 0 in the written value (read-only
  flags; avoid unintended side effects).

### 12.6 Probe / remove (cpldA path)

1. Verify device: read CCBHVR; `dev_err` + `-ENODEV` on failure.
2. `dev_info` the parsed version/revision.
3. Create the cpldA attribute group.
4. **No register writes at probe or remove** — the watchdog (and everything
   else) is left exactly as the bootloader/previous state configured it.
5. `remove()`: remove the attribute group only.

### 12.7 Error handling & validation (cpldA additions)

| Input / condition | Rule |
|---|---|
| `wd_enable` | `kstrtobool`; else `-EINVAL` |
| `wd_timeout` | `kstrtou32`; 1–180 rounded up to next step; out of range → `-EINVAL` |
| `wd_kick` | `kstrtobool`; `1` reloads, `0` no-op |
| SMBus errors | propagate errno, as §8 |

### 12.8 Testing plan (cpldA)

1. Boot / bind: dmesg shows cpldA probe log with version (expect `V03`,
   `R3.00` for CCBHVR default 0x63); `/sys/bus/i2c/devices/3-0018/` contains
   exactly the 8 attributes of §12.4; no mux adapters created for 3-0018.
2. Hands-off check: `i2cdump` WTR/WTD before insmod and read attrs after —
   WTR value unchanged by probe.
3. `cat wd_timeout` → default 5; `echo 45 > wd_timeout` → readback 60;
   `echo 200 > wd_timeout` → `EINVAL`.
4. `echo 1 > wd_enable`; `cat wd_remaining` counts down; `echo 1 > wd_kick`
   reloads it to the full timeout.
5. Let the WD expire on a test box (or with reset line disconnected):
   `wd_timeout_flag`/`wd_triggered` read `1` once then `0`; reading
   `wd_enable`/`wd_timeout` in between must **not** consume the flags.
6. cpldB regression: 18-0075 attrs and mux buses unaffected; both devices
   bound simultaneously.

