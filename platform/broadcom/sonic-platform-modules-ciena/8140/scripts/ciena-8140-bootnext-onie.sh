#!/bin/bash
# Re-arm UEFI BootNext to the ONIE boot entry.
#
# Rationale (CN8140-920, UEFI Secure Boot enabled):
#   SONiC is only bootable via the *unsecured* ONIE grub menu.  The signed
#   Evernight/ONEOS grub (grub.cfg + grub.cfg.p7b) refuses unsigned SONiC menu
#   entries, and the "SONiC-OS" shim is not enrolled in the platform key set,
#   so its own UEFI boot entry fails Secure Boot verification.  The only path
#   that boots SONiC is:  ONIE grubx64.efi -> (timeout) -> SONiC menuentry.
#
#   This platform firmware also rebuilds the UEFI BootOrder at every POST, so
#   a static "efibootmgr -o <ONIE first>" does not persist and the box keeps
#   defaulting back to the Evernight shim.
#
#   BootNext is a one-shot override that the firmware consumes on the very
#   next boot, *before* it reorders BootOrder, and is honored even by firmware
#   that regenerates BootOrder.  Re-arming BootNext=ONIE on every SONiC boot
#   therefore makes each subsequent reboot return to ONIE -> SONiC, without
#   touching any signed bootloader.
#
#   The ONIE UEFI entry number is resolved by label at runtime (not hard-coded)
#   so it survives boot-entry renumbering after re-installs.
set -u

if [ ! -d /sys/firmware/efi ]; then
    echo "ciena-8140-bootnext-onie: not a UEFI/EFI system; nothing to do" >&2
    exit 0
fi

if ! command -v efibootmgr >/dev/null 2>&1; then
    echo "ciena-8140-bootnext-onie: efibootmgr not found; nothing to do" >&2
    exit 0
fi

onie_num="$(efibootmgr 2>/dev/null \
  | sed -n 's/^Boot\([0-9A-Fa-f]\{4\}\)\*\{0,1\} ONIE: Open Network.*/\1/p' \
  | head -n1)"

if [ -z "${onie_num}" ]; then
    echo "ciena-8140-bootnext-onie: ONIE UEFI boot entry not found; leaving BootNext unchanged" >&2
    exit 0
fi

if efibootmgr -n "${onie_num}" >/dev/null 2>&1; then
    echo "ciena-8140-bootnext-onie: armed BootNext=${onie_num} (ONIE)"
else
    echo "ciena-8140-bootnext-onie: failed to set BootNext=${onie_num}" >&2
    exit 1
fi
