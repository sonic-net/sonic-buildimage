#!/bin/bash
# ciena-8140-led-fpga-init.sh
#
# Rudra40 FPGA side of the front-panel SFP/QSFP LED bring-up.
#
# LED datapath background
# -----------------------
# The Rudra40 FPGA drives the physical front-panel RGB LEDs (2 LEDs per SFP/QSFP
# cage).  It supports two source modes, selected by RUDRA40_GLUE_LED_SYS_STATUS:
#
#   * Production/BCM-frame path (sw_override_all_leds bit26 = 0): the FPGA takes
#     per-port ON/OFF (link + activity) from the Broadcom M0 microcode serial LED
#     frame, and per-port COLOUR from the *_RATE_* registers (colour-by-speed).
#
#   * Software override path (bit26 = 1): the FPGA drives each port's LED directly
#     from the per-colour registers RUDRA40_GLUE_LED_{SFP,QSFP}_{RED,GREEN,BLUE}_*
#     (1 bit per port per LED), ignoring both the BCM frame and the *_RATE_* colour.
#
# Why this platform uses the software override path
# -------------------------------------------------
# SONiC never feeds a per-port control-data frame into the M0 (led_proc_init.soc
# only loads the microcode and runs "led auto on / start"; it never writes any
# per-port control data).  SAOS keeps that frame current with a registered
# bcm_linkscan callback (drvDnxLedProcConfigureUc).  Without it the BCM-frame path
# never gates ports off, so every configured cage lights regardless of SFP
# presence or link state (the "stuck blue" symptom), and zeroing the *_RATE_*
# colour for down ports does NOT turn them off (RATE does not control on/off).
#
# This script therefore drives the LEDs via the software override path,
#
# Colour-by-speed encoding 
# --------------------------------------------------------------------------------
# Each port's colour is a 6-bit value = two 3-bit per-LED codes: [LED2:3][LED1:3].
# Each 3-bit code is [Blue:bit2][Green:bit1][Red:bit0] (001=R 010=G 100=B 011=Y
# 111=W).  Per-speed 6-bit values (this board: 100G is 2-lane):
#     400G=0x12(GG) 200G=0x02(G) 100G=0x22(BG) 50G=0x02(G)
#      40G=0x3c(WB)  25G=0x03(Y)  10G=0x03(Y)   other=0x00(off)
# In override mode LED1 = low 3 bits -> the *_{RED,GREEN,BLUE}_1 (SFP 1..20) /
# *_3 (SFP 21..40) / QSFP *_1 registers;  LED2 = high 3 bits -> the *_2 / *_4 /
# QSFP *_2 registers.  1 bit per port; bit0=first port in the register's range.
#
# Override colour-register layout (from rudra40_regmap.h)
# -------------------------------------------------------
#   SFP  {RED,GREEN,BLUE}_1 = LED1 for SFP  ports  1..20  (bit0=port1 .. bit19=port20)
#   SFP  {RED,GREEN,BLUE}_2 = LED2 for SFP  ports  1..20
#   SFP  {RED,GREEN,BLUE}_3 = LED1 for SFP  ports 21..40  (bit0=port21 .. bit19=port40)
#   SFP  {RED,GREEN,BLUE}_4 = LED2 for SFP  ports 21..40
#   QSFP {RED,GREEN,BLUE}_1 = LED1 for QSFP ports  1..8   (bit0=port1 .. bit7=port8)
#   QSFP {RED,GREEN,BLUE}_2 = LED2 for QSFP ports  1..8
#
# Port mapping (faceplate order, confirmed via presence-GPIO mapping):
#   Ethernet<k>   k=0..39  -> physical SFP  port (k+1)
#   Ethernet<40+j> j=0..7  -> physical QSFP port (j+1)
# NOTE: this 1:1 logical-to-physical mapping assumes the default HWSKU port
#   layout (4x10G + 36x100G + 8x400G, no breakout).  If a QSFP cage is broken out
#   into multiple logical Ethernet interfaces the index->cage mapping must change.
#
# Modes:
#   oneshot (default) : set override mode, clear all colour registers, program from
#                       the current CONFIG_DB speed + APPL_DB oper_status, exit.
#   watch [interval]  : as oneshot, then poll and re-programme whenever a port's
#                       speed or oper_status changes.  Long-lived systemd service
#                       (ciena-8140-led-fpga.service).

PLATFORM="CN8140"
LOG_TAG="ciena-8140-led"
PLREG="${PLREG:-/usr/local/bin/plreg}"

SFP_PORT_COUNT=40      # Ethernet0..39
QSFP_PORT_COUNT=8      # Ethernet40..47
QSFP_ETH_BASE=40

# ---- Per-speed 6-bit rate/colour values (SAOS cn8140 HostmodeNone) -----------
# See header for the [LED2:3][LED1:3] / [B][G][R] encoding.  100G is 2-lane here.
readonly LED_RATE_OFF=0x00
readonly LED_RATE_400G=0x12   # GG
readonly LED_RATE_200G=0x02   #  G
readonly LED_RATE_100G=0x22   # BG (2-lane)
readonly LED_RATE_50G=0x02    #  G
readonly LED_RATE_40G=0x3c    # WB
readonly LED_RATE_25G=0x03    #  Y
readonly LED_RATE_10G=0x03    #  Y

# ---- Global gate: enable_all_leds(bit24)=1 + sw_override_all_leds(bit26)=1 ----
# bit27 (sw_override_phy_leds) is deliberately left 0 so only the front-panel port
# LEDs are software-driven; PHY / system LEDs keep their normal source.
readonly LED_SYS_STATUS_ENABLE=0x05000000

# ---- watch-mode tuning ------------------------------------------------------
LED_WATCH_INTERVAL="${LED_WATCH_INTERVAL:-3}"   # seconds between DB polls
LED_WATCH_RESYNC="${LED_WATCH_RESYNC:-10}"      # force full rewrite every N polls
# While this sentinel exists the watcher leaves ALL LED registers untouched, so
# the interactive LED test script (ciena-8140-led-test.sh) can drive the SW
# override path without the watcher fighting it.
PAUSE_SENTINEL="${LED_PAUSE_SENTINEL:-/run/ciena-8140-led.pause}"

log_info()  { logger -t "$LOG_TAG" -p user.info "$1"; echo "$1"; }
log_notice() { logger -t "$LOG_TAG" -p user.notice "$1"; echo "$1"; }
log_error() { logger -t "$LOG_TAG" -p user.err  "$1"; echo "ERROR: $1" >&2; }

# Map an Ethernet speed in Mbps to the FPGA 6-bit rate/colour value.
speed_to_regval() {
    case "$1" in
        400000) echo $((LED_RATE_400G)) ;;
        200000) echo $((LED_RATE_200G)) ;;
        100000) echo $((LED_RATE_100G)) ;;
         50000) echo $((LED_RATE_50G)) ;;
         40000) echo $((LED_RATE_40G)) ;;
         25000) echo $((LED_RATE_25G)) ;;
         10000) echo $((LED_RATE_10G)) ;;
         *)     echo $((LED_RATE_OFF)) ;;   # unknown/unconfigured -> off
    esac
}

# ---- CONFIG_DB speed read ---------------------------------------------------
# Populates SPEEDS[k]=mbps for every configured PORT|Ethernet<k>.  Returns 0 on a
# successful DB read (even with zero ports), 1 on a DB/read failure.
declare -A SPEEDS
read_all_speeds() {
    SPEEDS=()
    command -v sonic-db-cli >/dev/null 2>&1 || return 1

    local out rc line key k spd
    out="$(sonic-db-cli CONFIG_DB EVAL \
        'local r={}; local ks=redis.call("keys","PORT|Ethernet*"); for _,k in ipairs(ks) do local s=redis.call("hget",k,"speed"); r[#r+1]=k.."="..(s or "") end; return r' \
        0 2>/dev/null)"
    rc=$?

    if [ $rc -ne 0 ]; then
        local keys
        keys="$(sonic-db-cli CONFIG_DB KEYS "PORT|Ethernet*" 2>/dev/null)"
        [ $? -ne 0 ] && return 1            # DB genuinely unreachable
        out=""
        for key in $keys; do
            spd="$(sonic-db-cli CONFIG_DB HGET "$key" speed 2>/dev/null)"
            out+="${key}=${spd}"$'\n'
        done
    fi

    while IFS= read -r line; do
        [ -z "$line" ] && continue
        key="${line%%=*}"; spd="${line#*=}"
        k="${key#PORT|Ethernet}"
        case "$k" in ''|*[!0-9]*) continue ;; esac
        [ -n "$spd" ] && SPEEDS[$k]="$spd"
    done <<EOF
$out
EOF
    return 0
}

# ---- APPL_DB oper_status read -----------------------------------------------
# Populates OPER[k]=up|down for every PORT_TABLE:Ethernet<k>.  A port lights only
# when APPL_DB explicitly reports oper_status=up; every other case (down, missing
# key, unconfigured, absent SFP) leaves it dark.  On any DB/read failure returns
# non-zero and the caller RETAINS the current register state (never blanks a
# legitimately-lit LED on a transient Redis hiccup).
declare -A OPER
read_all_oper() {
    OPER=()
    command -v sonic-db-cli >/dev/null 2>&1 || return 1

    local out rc line key k st
    out="$(sonic-db-cli APPL_DB EVAL \
        'local r={}; local ks=redis.call("keys","PORT_TABLE:Ethernet*"); for _,k in ipairs(ks) do local s=redis.call("hget",k,"oper_status"); r[#r+1]=k.."="..(s or "") end; return r' \
        0 2>/dev/null)"
    rc=$?

    if [ $rc -ne 0 ]; then
        local keys
        keys="$(sonic-db-cli APPL_DB KEYS "PORT_TABLE:Ethernet*" 2>/dev/null)"
        [ $? -ne 0 ] && return 1            # DB genuinely unreachable
        out=""
        for key in $keys; do
            st="$(sonic-db-cli APPL_DB HGET "$key" oper_status 2>/dev/null)"
            out+="${key}=${st}"$'\n'
        done
    fi

    while IFS= read -r line; do
        [ -z "$line" ] && continue
        key="${line%%=*}"; st="${line#*=}"
        k="${key#PORT_TABLE:Ethernet}"
        case "$k" in ''|*[!0-9]*) continue ;; esac
        [ -n "$st" ] && OPER[$k]="$st"
    done <<EOF
$out
EOF
    return 0
}

# ---- target register computation -------------------------------------------
# Reduces SPEEDS (colour) gated by OPER (on/off) into the per-colour override
# register targets.  Keys are "<C><n>" with C in R/G/B and n the register index:
#   SFP:  n=1 (LED1 p1..20) 2 (LED2 p1..20) 3 (LED1 p21..40) 4 (LED2 p21..40)
#   QSFP: n=1 (LED1 p1..8)  2 (LED2 p1..8)
declare -A SFP_TGT QSFP_TGT
_set_led_bits() {   # $1=assoc-name $2=idx $3=3bit-code $4=bitmask
    local __name="$1" idx="$2" code="$3" mask="$4"
    if (( code & 1 )); then eval "$__name[R$idx]=\$(( \${$__name[R$idx]:-0} | mask ))"; fi
    if (( code & 2 )); then eval "$__name[G$idx]=\$(( \${$__name[G$idx]:-0} | mask ))"; fi
    if (( code & 4 )); then eval "$__name[B$idx]=\$(( \${$__name[B$idx]:-0} | mask ))"; fi
}
compute_targets() {
    SFP_TGT=(); QSFP_TGT=()
    local c n
    for c in R G B; do
        for n in 1 2 3 4; do SFP_TGT[$c$n]=0; done
        for n in 1 2;     do QSFP_TGT[$c$n]=0; done
    done

    local k spd rv led1 led2 p bit mask i1 i2
    for k in "${!OPER[@]}"; do
        [ "${OPER[$k]}" = "up" ] || continue          # on/off gate: only oper-up lights
        spd="${SPEEDS[$k]:-}"
        rv=$(speed_to_regval "$spd")
        [ "$rv" -eq 0 ] && continue                   # up but unknown speed -> off
        led1=$(( rv & 7 )); led2=$(( (rv >> 3) & 7 ))

        if [ "$k" -lt "$SFP_PORT_COUNT" ]; then
            p=$((k + 1))                              # SFP physical port 1..40
            if [ "$p" -le 20 ]; then bit=$((p - 1));  i1=1; i2=2
            else                     bit=$((p - 21)); i1=3; i2=4; fi
            mask=$((1 << bit))
            _set_led_bits SFP_TGT "$i1" "$led1" "$mask"
            _set_led_bits SFP_TGT "$i2" "$led2" "$mask"
        elif [ "$k" -lt "$((QSFP_ETH_BASE + QSFP_PORT_COUNT))" ]; then
            p=$((k - QSFP_ETH_BASE + 1))              # QSFP physical port 1..8
            bit=$((p - 1)); mask=$((1 << bit))
            _set_led_bits QSFP_TGT 1 "$led1" "$mask"
            _set_led_bits QSFP_TGT 2 "$led2" "$mask"
        fi
        # k outside 0..47 -> not part of the 1:1 layout -> ignored.
    done
}

all_targets_off() {
    local c n
    SFP_TGT=(); QSFP_TGT=()
    for c in R G B; do
        for n in 1 2 3 4; do SFP_TGT[$c$n]=0; done
        for n in 1 2;     do QSFP_TGT[$c$n]=0; done
    done
}

# ---- programming ------------------------------------------------------------
# Writes each override colour register, skipping ones already known to hold the
# value (unless force=1).  On a write failure the cache is NOT updated so the next
# poll retries.  Returns non-zero if any write failed.
declare -A LAST_SFP LAST_QSFP
_colour_name() { case "$1" in R) echo RED ;; G) echo GREEN ;; B) echo BLUE ;; esac; }
program_targets() {
    local force="${1:-0}" rc=0 c n key val cur regname hex cname
    for c in R G B; do
        cname="$(_colour_name "$c")"
        for n in 1 2 3 4; do
            key="$c$n"; val=${SFP_TGT[$key]:-0}; cur=${LAST_SFP[$key]:-__unset__}
            if [ "$force" = "1" ] || [ "$cur" != "$val" ]; then
                regname="RUDRA40_GLUE_LED_SFP_${cname}_${n}"
                hex="$(printf '0x%08X' "$val")"
                if $PLREG write "$regname" "$hex" >/dev/null 2>&1; then
                    [ "$cur" != "$val" ] && log_info "${regname} = ${hex}"
                    LAST_SFP[$key]=$val
                else
                    log_error "Failed to write ${regname}"; rc=1
                fi
            fi
        done
        for n in 1 2; do
            key="$c$n"; val=${QSFP_TGT[$key]:-0}; cur=${LAST_QSFP[$key]:-__unset__}
            if [ "$force" = "1" ] || [ "$cur" != "$val" ]; then
                regname="RUDRA40_GLUE_LED_QSFP_${cname}_${n}"
                hex="$(printf '0x%08X' "$val")"
                if $PLREG write "$regname" "$hex" >/dev/null 2>&1; then
                    [ "$cur" != "$val" ] && log_info "${regname} = ${hex}"
                    LAST_QSFP[$key]=$val
                else
                    log_error "Failed to write ${regname}"; rc=1
                fi
            fi
        done
    done
    return $rc
}

# One-time clear of every colour and rate register 
# so no stale FPGA state can leave a phantom LED lit.  Seeds the write
# caches to known-zero so program_targets only writes registers that must change.
clear_all_led_regs() {
    local c n cname
    for c in R G B; do
        cname="$(_colour_name "$c")"
        for n in 1 2 3 4; do $PLREG write "RUDRA40_GLUE_LED_SFP_${cname}_${n}"  0x00000000 >/dev/null 2>&1; done
        for n in 1 2;     do $PLREG write "RUDRA40_GLUE_LED_QSFP_${cname}_${n}" 0x00000000 >/dev/null 2>&1; done
    done
    for n in 1 2 3 4 5 6 7 8; do $PLREG write "RUDRA40_GLUE_LED_SFP_RATE_${n}"  0x00000000 >/dev/null 2>&1; done
    for n in 1 2;             do $PLREG write "RUDRA40_GLUE_LED_QSFP_RATE_${n}" 0x00000000 >/dev/null 2>&1; done
    LAST_SFP=(); LAST_QSFP=()
    for c in R G B; do
        for n in 1 2 3 4; do LAST_SFP[$c$n]=0; done
        for n in 1 2;     do LAST_QSFP[$c$n]=0; done
    done
}

# Enable the global LED output in software-override mode.  Idempotent (re-asserted
# on the periodic resync so the mode self-heals if something clears it); only logs
# when the value actually changes.
LAST_SYS_STATUS=""
enable_led_output() {
    if [ -e "$PAUSE_SENTINEL" ]; then
        log_info "LED pause sentinel present ($PAUSE_SENTINEL); leaving LED_SYS_STATUS unchanged"
        return 0
    fi
    if $PLREG write "RUDRA40_GLUE_LED_SYS_STATUS" "$LED_SYS_STATUS_ENABLE" >/dev/null 2>&1; then
        if [ "$LAST_SYS_STATUS" != "$LED_SYS_STATUS_ENABLE" ]; then
            log_info "LED_SYS_STATUS = ${LED_SYS_STATUS_ENABLE} (enable_all_leds=1, sw_override_all_leds=1)"
            LAST_SYS_STATUS="$LED_SYS_STATUS_ENABLE"
        fi
    else
        log_error "Failed to write LED_SYS_STATUS"
        return 1
    fi
}

# ---- modes ------------------------------------------------------------------
do_oneshot() {
    log_info "Starting $PLATFORM FPGA LED initialization (oneshot)..."
    enable_led_output
    clear_all_led_regs
    if read_all_speeds; then
        read_all_oper
        compute_targets
    else
        log_notice "CONFIG_DB not yet available at boot (expected this early); leaving all front-panel LEDs OFF until the watcher applies per-port colour"
        all_targets_off
    fi
    program_targets 1 || log_error "One or more LED colour registers failed to program"
    log_info "$PLATFORM FPGA LED initialization complete"
}

do_watch() {
    local interval="${1:-$LED_WATCH_INTERVAL}"
    case "$interval" in ''|*[!0-9]*) interval="$LED_WATCH_INTERVAL" ;; esac
    log_info "Starting $PLATFORM FPGA LED watcher (interval=${interval}s, resync every ${LED_WATCH_RESYNC} polls)"
    trap 'log_info "$PLATFORM FPGA LED watcher exiting"; exit 0' INT TERM

    local have_cache=0 count=0 force
    if [ ! -e "$PAUSE_SENTINEL" ]; then
        enable_led_output
        clear_all_led_regs
    fi
    while true; do
        if [ -e "$PAUSE_SENTINEL" ]; then
            # LED test in progress -> hands off entirely; re-init on resume.
            sleep "$interval"; count=0; have_cache=0; continue
        fi
        if read_all_speeds; then
            read_all_oper
            compute_targets
            force=0
            [ "$have_cache" -eq 0 ] && force=1
            [ $((count % LED_WATCH_RESYNC)) -eq 0 ] && force=1
            if [ "$force" -eq 1 ]; then
                enable_led_output                 # self-heal the override mode
                [ "$have_cache" -eq 0 ] && clear_all_led_regs
            fi
            program_targets "$force"
            have_cache=1
        else
            log_error "CONFIG_DB read failed; retaining current LED state"
        fi
        count=$((count + 1))
        sleep "$interval"
    done
}

main() {
    if [ ! -x "$PLREG" ]; then
        log_error "plreg utility not found at $PLREG; skipping LED init"
        exit 1
    fi
    case "${1:-oneshot}" in
        oneshot) do_oneshot ;;
        watch)   do_watch "$2" ;;
        *) log_error "usage: $0 [oneshot | watch [interval_sec]]"; exit 2 ;;
    esac
    exit 0
}

main "$@"
