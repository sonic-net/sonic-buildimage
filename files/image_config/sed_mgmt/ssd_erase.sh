#!/bin/bash

# ssd_erase.sh - graceful SSD wipe orchestrator (per doc/sed/wipe_ssd_hld.md)
#
# Phases:
#   1) Pre-pivot preparation - prereqs + stop SONiC target + stop non-essential services
#   2) Ramdisk pivot         - load_fs_to_ramfs -> pivot_to_ramfs -> umount_disk
#   3) Erase                 - store TPM bank A -> PSID revert -> store TPM bank B -> nvme sanitize
#
# Arguments:
#   -a <tpm_bank_a>       Persistent TPM handle for SED password bank A (e.g. 0x81010001)
#   -b <tpm_bank_b>       Persistent TPM handle for SED password bank B (e.g. 0x81010002)
#   -p <default_password> Platform default SED password (read from TPM bank 3 by the caller)
#   -s <psid>             PSID; required for the crypto erase
#
# Exit codes:
#     0  success
#     2  bad / missing args
#     3  required tool missing (rsync, sedutil-cli, nvme, tpm2_*)
#     4  insufficient RAM (< 6 GiB available)
#     5  SED prereqs failed (PSID, default pw, NVMe disk, OPAL 2.0, LockingEnabled, sanitize)
#     7  failed to stop sonic.target
#     8  stop_services failed
#    10  umount /oldroot failed after pivot (wipe aborted before erase)
#    11  crypto erase failed (sedutil-cli PSID revert)
#    12  nvme sanitize start failed OR sanitize-log reports failure (SSTAT=0x3)
#    13  nvme sanitize timed out (no SPROG progress for 10 min, or > 180 min absolute cap)

set -o pipefail

TMPFS_SIZE="${SSD_ERASE_TMPFS_SIZE:-6G}"
TMPFS_MIN_AVAIL_GB=6

# NVMe sanitize polling (see NVMe Base Spec §5.24; approach mirrors CL cumulus-tools bug #4927196).
# SSTAT low 3 bits: 1 = completed OK, 2 = in progress, 3 = completed with failure.
SSTAT_DONE=1
SSTAT_FAILED=3
# Typical block erase on switch SSDs finishes in 2-5 min; skip busy polling for the first stretch.
SANITIZE_INITIAL_WAIT_S=240
SANITIZE_POLL_S=10
# Progress-based stall detection is more robust than a fixed wall-clock cap on full/large drives.
SANITIZE_STALL_TIMEOUT_S=600
# Absolute safety cap so a wedged device can't hang us in the ramdisk forever.
SANITIZE_ABS_MAX_S=10800

# Systemd units that must survive `stop_services` (SSH, journald, dbus, etc.).
KEEP_SERVICES=(
    ssh.service
    dbus.service
    systemd-udevd.service
    systemd-journald.service
    systemd-logind.service
    getty@tty1.service
    pam-auth.service
    acpid.service
)

# Systemd units that were running before the pivot but must NOT be restarted after
# because their state (log dir, docker socket, monit config) no longer exists.
NO_RESTART=(
    serial-getty@ttyS0.service
    rsyslog.service
    docker.service
    containerd.service
    monit.service
)

usage() {
    echo "Usage: $0 -a <tpm_bank_a> -b <tpm_bank_b> -p <default_password> -s <psid>"
    exit 2
}

tpm_reg=""
tpm_reg_2=""
default_pw=""
psid_val=""
while getopts "a:b:p:s:" opt; do
    case "$opt" in
        a) tpm_reg=$OPTARG ;;
        b) tpm_reg_2=$OPTARG ;;
        p) default_pw=$OPTARG ;;
        s) psid_val=$OPTARG ;;
        *) usage ;;
    esac
done
if [ -z "$tpm_reg" ] || [ -z "$tpm_reg_2" ] || [ -z "$default_pw" ] || [ -z "$psid_val" ]; then
    usage
fi

source /usr/local/bin/sed_pw_utils.sh

########################
# Helper functions
########################

is_in_list() {
    local needle=$1
    shift
    local item
    for item in "$@"; do
        [ "$item" = "$needle" ] && return 0
    done
    return 1
}

# Return the list of currently-running .service units (one per line).
running_services() {
    systemctl list-units --type=service --state=running --no-legend --plain 2>/dev/null \
        | awk '{print $1}'
}

check_tools() {
    local missing=()
    for t in rsync sedutil-cli nvme tpm2_unseal tpm2_create tpm2_load tpm2_evictcontrol tpm2_createprimary; do
        if ! command -v "$t" >/dev/null 2>&1; then
            missing+=("$t")
        fi
    done
    if [ ${#missing[@]} -gt 0 ]; then
        log_error "Required tools missing: ${missing[*]}"
        return 3
    fi
    return 0
}

check_memory() {
    local avail_gb
    avail_gb=$(free -g | awk '/^Mem:/ {print $7}')
    if [ -z "$avail_gb" ] || [ "$avail_gb" -lt "$TMPFS_MIN_AVAIL_GB" ]; then
        log_error "Insufficient available RAM: ${avail_gb} GiB (need >= ${TMPFS_MIN_AVAIL_GB} GiB). Reboot and retry."
        return 4
    fi
    log_info "RAM check OK: ${avail_gb} GiB available"
    return 0
}

stop_sonic() {
    log_info "Stopping SONiC application stack"
    wall "SSD wipe in progress. SONiC is being stopped." >/dev/null 2>&1 || true
    monit unmonitor container_checker >/dev/null 2>&1 || true
    if ! systemctl stop sonic.target --job-mode=replace-irreversibly; then
        log_error "systemctl stop sonic.target failed"
        return 7
    fi
    return 0
}

# Stop every running .service unit that is NOT in KEEP_SERVICES. Records the list
# so restart_services can re-enable them after the pivot.
RUNNING_BEFORE_STOP=()
stop_services() {
    log_info "Stopping non-essential services (keeping: ${KEEP_SERVICES[*]})"
    local svc
    RUNNING_BEFORE_STOP=()
    while IFS= read -r svc; do
        [ -z "$svc" ] && continue
        if is_in_list "$svc" "${KEEP_SERVICES[@]}"; then
            continue
        fi
        RUNNING_BEFORE_STOP+=("$svc")
        systemctl stop "$svc" >/dev/null 2>&1 || \
            log_warn "systemctl stop $svc returned non-zero"
    done < <(running_services)
    log_info "Stopped ${#RUNNING_BEFORE_STOP[@]} services"
    return 0
}

restart_services() {
    log_info "Restarting services (skipping: ${NO_RESTART[*]})"
    local svc
    for svc in "${RUNNING_BEFORE_STOP[@]}"; do
        if is_in_list "$svc" "${NO_RESTART[@]}"; then
            log_debug "skip restart: $svc"
            continue
        fi
        systemctl start "$svc" >/dev/null 2>&1 || \
            log_warn "systemctl start $svc returned non-zero"
    done
    return 0
}

load_fs_to_ramfs() {
    log_info "Loading minimal userspace into tmpfs at /newroot (size=$TMPFS_SIZE)"
    swapoff -a >/dev/null 2>&1 || true
    mkdir -p /newroot
    if ! mount -t tmpfs -o "size=$TMPFS_SIZE" tmpfs /newroot; then
        log_error "mount tmpfs /newroot failed (size=$TMPFS_SIZE)"
        return 1
    fi

    # Full copies of tool/config trees. Failure here means the ramdisk is
    # missing tools we need after pivot (sedutil-cli, nvme, tpm2_*), so bail
    # before pivot_root leaves us stranded.
    local d
    for d in bin etc mnt sbin lib lib64; do
        [ -e "/$d" ] || continue
        log_debug "cp -ax /$d /newroot/$d"
        cp -ax "/$d" "/newroot/$d" || { log_error "cp -ax /$d /newroot/$d failed"; return 1; }
    done
    for d in usr/bin usr/sbin usr/lib usr/lib64 usr/libexec usr/share usr/local; do
        [ -e "/$d" ] || continue
        mkdir -p "/newroot/$(dirname "$d")"
        log_debug "cp -ax /$d /newroot/$d"
        cp -ax "/$d" "/newroot/$d" || { log_error "cp -ax /$d /newroot/$d failed"; return 1; }
    done

    # Userspace scratch dirs plus the bind-mount targets pivot_to_ramfs will
    # populate. Missing dev/proc/sys/run leaves the ramdisk without /dev/null,
    # /proc/*, /sys/*, /run/*, which breaks every subsequent syscall and
    # prevents `reboot -f` from firing.
    mkdir -p /newroot/root /newroot/home /newroot/tmp /newroot/var/log \
             /newroot/dev /newroot/proc /newroot/sys /newroot/run /newroot/oldroot

    if [ -f /var/log/syslog ]; then
        cp -a /var/log/syslog /newroot/var/log/syslog
    fi

    if [ -d /home/admin ]; then
        rsync -a --max-size=100M /home/admin/ /newroot/home/admin/ 2>/dev/null || \
            log_warn "rsync /home/admin -> /newroot/home/admin returned non-zero"
    fi

    # Selective /var copy: everything except the huge Docker/apt/redis subtrees.
    if [ -d /var/lib ]; then
        rsync -a \
            --exclude=docker \
            --exclude=containerd \
            --exclude=dpkg \
            --exclude=apt \
            --exclude=redis \
            /var/lib/ /newroot/var/lib/ 2>/dev/null || \
            log_warn "rsync /var/lib -> /newroot/var/lib returned non-zero"
    fi
    for d in local opt spool lock run; do
        if [ -d "/var/$d" ]; then
            rsync -a "/var/$d/" "/newroot/var/$d/" 2>/dev/null || true
        fi
    done

    return 0
}

pivot_to_ramfs() {
    log_info "pivot_root /newroot /newroot/oldroot"
    mount --make-private / >/dev/null 2>&1 || true
    mount --make-private /newroot >/dev/null 2>&1 || true
    mkdir -p /newroot/oldroot
    if ! pivot_root /newroot /newroot/oldroot; then
        log_error "pivot_root failed"
        return 1
    fi
    cd / || return 1
    local m
    for m in dev proc sys run; do
        # Defensive: ensure target exists even if load_fs_to_ramfs did not create it.
        mkdir -p "/$m"
        if [ -e "/oldroot/$m" ]; then
            mount --move "/oldroot/$m" "/$m" || log_warn "mount --move /oldroot/$m failed"
        fi
    done
    mkdir -p /home
    return 0
}

umount_disk() {
    if [ ! -d /oldroot ]; then
        log_warn "/oldroot does not exist"
        return 0
    fi

    log_info "Enumerating mounts under /oldroot"
    local mounts
    mounts=$(findmnt -R -o TARGET -n /oldroot 2>/dev/null)
    if [ -z "$mounts" ]; then
        log_info "No mounts under /oldroot"
        return 0
    fi

    # Sort by depth descending so children unmount first.
    local sorted
    sorted=$(echo "$mounts" | awk '{ n=gsub("/","/"); print n"\t"$0 }' | sort -k1,1 -rn | cut -f2-)

    local mp
    while IFS= read -r mp; do
        [ -z "$mp" ] && continue
        log_debug "umount $mp"
        if ! umount "$mp" 2>/dev/null; then
            log_warn "umount $mp busy, trying lazy"
            umount -l "$mp" 2>/dev/null || log_warn "umount -l $mp also failed"
        fi
    done <<< "$sorted"

    # Final check: /oldroot itself must be gone or unmounted; otherwise abort.
    if findmnt /oldroot >/dev/null 2>&1; then
        log_error "/oldroot is still mounted after best-effort umount; aborting wipe"
        return 10
    fi
    log_info "OS disk unmounted"
    return 0
}

########################
# Erase phase
########################

do_crypto_erase() {
    log_info "Storing default password in TPM bank A ($tpm_reg) before PSID revert"
    if ! store_sed_pwd_in_tpm "$tpm_reg" "$default_pw"; then
        log_error "store_sed_pwd_in_tpm bank A failed"
        return 11
    fi

    log_info "sedutil-cli PSID revert on $nvme_disk_name"
    if ! sedutil-cli --yesIreallywanttoERASEALLmydatausingthePSID "$psid_val" "$nvme_disk_name"; then
        log_error "sedutil-cli PSID revert failed"
        return 11
    fi
    log_info "Crypto erase (PSID revert) OK"

    log_info "Storing default password in TPM bank B ($tpm_reg_2) after PSID revert"
    if ! store_sed_pwd_in_tpm "$tpm_reg_2" "$default_pw"; then
        log_warn "store_sed_pwd_in_tpm bank B failed - continuing to block erase (bank A holds default)"
    fi

    return 0
}

do_block_erase() {
    log_info "Starting NVMe block erase (sanact=0x02) on $disk_name"
    if ! nvme sanitize "$disk_name" --sanact=0x02; then
        log_error "nvme sanitize start failed"
        return 12
    fi

    # Skip busy polling for the initial stretch; typical erase completes here.
    sleep "$SANITIZE_INITIAL_WAIT_S"

    local elapsed=$SANITIZE_INITIAL_WAIT_S
    local last_sprog=-1
    local stall_secs=0

    while true; do
        local log_out sstat_raw sprog sstat
        log_out=$(nvme sanitize-log "$disk_name" 2>/dev/null)
        # Mask SSTAT low 3 bits; handles hex/decimal nvme-cli encoding.
        sstat_raw=$(sed -ne 's/^Sanitize Status.*:[[:space:]]*\([0-9xXa-fA-F]*\).*/\1/p' <<<"$log_out")
        sprog=$(sed -ne 's/^Sanitize Progress.*:[[:space:]]*\([0-9]*\).*/\1/p' <<<"$log_out")
        sstat=$(( ${sstat_raw:-0} & 0x7 ))
        [[ "$sprog" =~ ^[0-9]+$ ]] || sprog=-1
        log_debug "sanitize-log t=${elapsed}s sstat=$sstat sprog=$sprog"

        if [ "$sstat" -eq "$SSTAT_DONE" ]; then
            log_info "NVMe block erase completed successfully"
            return 0
        fi
        if [ "$sstat" -eq "$SSTAT_FAILED" ]; then
            log_error "NVMe sanitize reported failure (SSTAT=0x3). Data may still be present."
            return 12
        fi

        if [ "$sprog" -gt "$last_sprog" ]; then
            last_sprog=$sprog
            stall_secs=0
        else
            stall_secs=$((stall_secs + SANITIZE_POLL_S))
        fi

        if [ "$stall_secs" -ge "$SANITIZE_STALL_TIMEOUT_S" ]; then
            log_error "NVMe sanitize made no progress for $((SANITIZE_STALL_TIMEOUT_S / 60)) min (SPROG stuck at ${last_sprog}). Data may still be present."
            return 13
        fi
        if [ "$elapsed" -ge "$SANITIZE_ABS_MAX_S" ]; then
            log_error "NVMe sanitize did not complete within $((SANITIZE_ABS_MAX_S / 60)) min absolute cap (SPROG ${last_sprog}). Data may still be present."
            return 13
        fi

        sleep "$SANITIZE_POLL_S"
        elapsed=$((elapsed + SANITIZE_POLL_S))
    done
}

########################
# Main
########################

log_info "===== SSD wipe start ====="

check_tools                          || exit $?
check_memory                         || exit $?

if ! check_sed_crypto_erase_prereqs; then
    log_error "SED crypto erase prereqs failed"
    exit 5
fi

stop_sonic                           || exit $?
stop_services                        || exit $?

dmesg -D >/dev/null 2>&1 || true

if ! load_fs_to_ramfs; then
    log_error "load_fs_to_ramfs failed"
    exit 1
fi

if ! pivot_to_ramfs; then
    log_error "pivot_to_ramfs failed - reboot to recover"
    exit 1
fi

umount_disk                          || exit $?

restart_services

do_crypto_erase                      || exit $?
do_block_erase                       || exit $?

log_info "===== SSD wipe complete - reboot with sudo /sbin/reboot ====="
exit 0
