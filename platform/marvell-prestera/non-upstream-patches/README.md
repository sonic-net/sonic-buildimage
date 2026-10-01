## Marvell Prestera (AC5x) non-upstream linux kernel patches ##

These are mandatory hardware-enablement patches for the RD-AC5X
(internal-CPU) board with >2GB DRAM, fixing the eMMC controller
enumerating as 0 B (board never finds its root filesystem at boot:
"Gave up waiting for root file system device ... UUID=... does not
exist"). Without them, the kernel built for marvell-prestera does not
include the eMMC DMA fix, regardless of the loose patch files that may
be sitting elsewhere in the tree -- this directory plus
`INCLUDE_EXTERNAL_PATCHES=y`/`EXTERNAL_KERNEL_PATCH_LOC` (set in
`platform/marvell-prestera/rules.mk`) is what actually wires them into
the src/sonic-linux-kernel build. See rules/linux-kernel.mk and
src/sonic-linux-kernel/Makefile's `INCLUDE_EXTERNAL_PATCHES` handling,
and platform/mellanox/non-upstream-patches/README.md for the general
mechanism this follows.

### Patches

- `0204-sdhci-4G-rd-dts.patch`: widen `ac5-98dx35xx-rd.dts` `mmc_dma`
  `dma-ranges` to identity-map the full 4GiB DRAM window.
- `0205-sdhci-xenon-34-bit-DMA-twice.patch`: `sdhci-xenon.c` `set_dma_mask`
  hook so the 34-bit DMA mask is applied before `sdhci_add_host()`.

Both patches are applied against `src/sonic-linux-kernel` (branch 202605).

### Enabling

`INCLUDE_EXTERNAL_PATCHES` defaults to `y` for this platform in
`platform/marvell-prestera/rules.mk` (these are required fixes, not
optional extras), so a normal marvell-prestera build already picks them
up. Override with `SONIC_OVERRIDE_BUILD_VARS=' INCLUDE_EXTERNAL_PATCHES=n '`
only if you explicitly need to build without them.
