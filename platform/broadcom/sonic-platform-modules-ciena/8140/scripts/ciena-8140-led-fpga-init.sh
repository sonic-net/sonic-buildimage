#!/bin/bash
# ciena-8140-led-fpga-init.sh
#
# This script drives the LEDs via the FPGA SW override.
#
# Two LEDs per cage: SPEED (LED1) and LINK/ACTIVITY (LED2)
# --------------------------------------------------------------------------------
# LED behaviour differs by cage type (see the "Link/Activity LED" block below for
# the full policy):
#   SFP  - LED1 = SPEED LED (colour-by-speed, lit only while up); LED2 = dedicated
#          LINK/ACTIVITY LED (green link, green blink on traffic, red on LOS).
#   QSFP - BOTH LEDs carry the rate colour; they blink together on traffic and
#          stay lit on LOS; off only when the port is disabled.
#
# Colour-by-speed encoding (SPEED LED / LED1)
# --------------------------------------------------------------------------------
# The per-speed colour is a 6-bit value = two 3-bit per-LED codes: [LED2:3][LED1:3].
# Each 3-bit code is [Blue:bit2][Green:bit1][Red:bit0] (001=R 010=G 100=B 011=Y
# 111=W).  Per-speed 6-bit values (this board: 100G is 2-lane):
#     400G=0x12(GG) 200G=0x02(G) 100G=0x22(BG) 50G=0x02(G)
#      40G=0x3c(WB)  25G=0x03(Y)  10G=0x03(Y)   other=0x00(off)
# The SPEED LED uses only the low 3 bits (LED1) of this value.  The high 3 bits
# (LED2) are consumed by the speed encoding only in legacy mode
# (ACTIVITY_LED_ENABLE=0); otherwise LED2 is driven by the activity policy below.
# LED1 = low 3 bits -> the *_{RED,GREEN,BLUE}_1 (SFP 1..20) / *_3 (SFP 21..40) /
# QSFP *_1 registers;  LED2 -> the *_2 / *_4 / QSFP *_2 registers.  1 bit per
# port; bit0=first port in the register's range.
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
#   into multiple logical Ethernet interfaces the index->cage mapping (and the
#   per-cage link/activity aggregation) must change.
#
# Modes:
#   oneshot (default) : set override mode, clear all colour registers, program from
#                       the current CONFIG_DB speed + APPL_DB oper_status +
#                       transceiver/counter state, exit.
#   watch [interval]  : as oneshot, then poll and re-programme whenever a port's
#                       speed, oper_status, presence, laser or traffic changes
#                       (the activity LED blinks at LED_BLINK_INTERVAL, decoupled
#                       from the DB poll).  Long-lived systemd service
#                       (ciena-8140-led-fpga.service).


PLATFORM="CN8140"
LOG_TAG="ciena-8140-led"
PLREG="${PLREG:-/usr/local/bin/plreg}"

SFP_PORT_COUNT=40      # Ethernet0..39
QSFP_PORT_COUNT=8      # Ethernet40..47
QSFP_ETH_BASE=40

# ---- Per-speed 6-bit rate/colour values  -----------
readonly LED_RATE_OFF=0x00
readonly LED_RATE_400G=0x12   # GG
readonly LED_RATE_200G=0x02   #  G
readonly LED_RATE_100G=0x22   # BG (2-lane)
readonly LED_RATE_50G=0x02    #  G
readonly LED_RATE_40G=0x3c    # WB
readonly LED_RATE_25G=0x03    #  Y
readonly LED_RATE_10G=0x03    #  Y

# ---- Link/Activity LED (SFP: 2nd LED per port) ------------------------------
# Each SFP/QSFP cage has two physical RGB LEDs.  LED1 is the SPEED LED and 
# keeps the colour-by-speed encoding above
#
# SFP ONLY: LED2 (override registers *_2 / *_4) is the dedicated LINK / ACTIVITY
# LED, driven entirely in software from SONiC DB state:
#
#     module absent                         -> OFF
#     laser disabled (tx*disable = True)    -> OFF
#     link up, no traffic since last poll   -> solid GREEN   (link established)
#     link up, traffic since last poll      -> BLINK GREEN   (link activity)
#     module present + laser on + link down
#            AND admin_status = up          -> RED           (LOS / no-signal)
#     otherwise (admin/unconfigured down)   -> OFF
#
# QSFP: BOTH LEDs encode the rate/breakout colour and behave per Heman's spec:
#   * port disabled (module absent, admin_status=down, or laser disabled) -> OFF
#   * otherwise (enabled) -> both LEDs lit with the rate colour, and they BLINK
#     together on traffic.  On LOS / link-down they REMAIN lit (steady rate
#     colour) - they are not turned off or turned red.  The rate colour uses the
#     CONFIG_DB configured speed, so it shows even before the link comes up.
#
# Why software-driven and not the BCM LED-uC bitstream: on this platform the
# real hardware activity flicker would come from the Broadcom M0 "custom LED"
# microcode shifting a per-port serial frame to the FPGA.  Under SONiC that path
# is inert - there is no HAL to populate the LED-uC control memory, so the
# microcode skips every port and the FPGA LED_FW_DATA* registers read back all
# zero.  We therefore synthesise the activity/link indication from CONFIG_DB
# (speed/admin), APPL_DB (oper_status), STATE_DB (transceiver presence + laser)
# and COUNTERS_DB (octet deltas), and drive it through the same SW-override
# colour registers already used for the speed LED.
#
# NOTE: the software blink runs at LED_BLINK_INTERVAL (default 0.25s on / 0.25s
# off), decoupled from the slower DB poll, so it is a coarse activity indicator
# (~2 Hz), not the smooth 30 Hz hardware flicker of the M0 bitstream path.
#
# Tunables (all overridable from the environment):
ACTIVITY_LED_ENABLE="${ACTIVITY_LED_ENABLE:-1}"   # 1: enable the SFP activity
                                                  #    LED and the QSFP blink/enable
                                                  #    policy.
                                                  # 0: legacy - both LEDs show the
                                                  #    speed colour, gated on link.
readonly ACT_COLOUR_OFF=0x0                       # 3-bit [B][G][R] per-LED codes
readonly ACT_COLOUR_LINK=0x2                      # green  = link up
readonly ACT_COLOUR_LOS=0x1                       # red    = laser on but no link
# Number of polls a port keeps blinking after its last observed traffic, so a
# single short burst produces a visible blink instead of one missed toggle.
ACTIVITY_HOLD="${ACTIVITY_HOLD:-2}"

# ---- Global gate: enable_all_leds(bit24)=1 + sw_override_all_leds(bit26)=1 ----
# bit27 (sw_override_phy_leds) is deliberately left 0 so only the front-panel port
# LEDs are software-driven; PHY / system LEDs keep their normal source.
readonly LED_SYS_STATUS_ENABLE=0x05000000

# ---- watch-mode tuning ------------------------------------------------------
LED_WATCH_INTERVAL="${LED_WATCH_INTERVAL:-2}"   # seconds between DB polls
# Blink half-period: how long each ON / OFF phase of the activity blink lasts.
# Decoupled from LED_WATCH_INTERVAL so the blink can be fast (default 0.25s ->
# 0.25s on / 0.25s off) without increasing the (expensive) Redis scan rate: the
# DB inputs are sampled once per LED_WATCH_INTERVAL and, only while a port is
# actively blinking, re-rendered at this cadence.  Must be <= LED_WATCH_INTERVAL;
# a fractional value (e.g. 0.25) is allowed.
LED_BLINK_INTERVAL="${LED_BLINK_INTERVAL:-0.25}"
LED_WATCH_RESYNC="${LED_WATCH_RESYNC:-10}"      # force full rewrite every N polls
# While this sentinel exists the watcher leaves ALL LED registers untouched, so
# the interactive LED test script (ciena-8140-led-test.sh) can drive the SW
# override path without the watcher fighting it.
PAUSE_SENTINEL="${LED_PAUSE_SENTINEL:-/run/ciena-8140-led.pause}"

log_info()  { logger -t "$LOG_TAG" -p user.info "$1"; echo "$1"; }
log_notice() { logger -t "$LOG_TAG" -p user.notice "$1"; echo "$1"; }
log_error() { logger -t "$LOG_TAG" -p user.err  "$1"; echo "ERROR: $1" >&2; }

# Map an Ethernet speed in Mbps to the FPGA 6-bit rate/colour value.  Result is
# returned in the global REG_VAL (no subshell/echo) so the per-port hot path in
# compute_targets stays fork-free.
REG_VAL=0
speed_to_regval() {
    case "$1" in
        400000) REG_VAL=$LED_RATE_400G ;;
        200000) REG_VAL=$LED_RATE_200G ;;
        100000) REG_VAL=$LED_RATE_100G ;;
         50000) REG_VAL=$LED_RATE_50G ;;
         40000) REG_VAL=$LED_RATE_40G ;;
         25000) REG_VAL=$LED_RATE_25G ;;
         10000) REG_VAL=$LED_RATE_10G ;;
         *)     REG_VAL=$LED_RATE_OFF ;;   # unknown/unconfigured -> off
    esac
}

# ---- CONFIG_DB speed read ---------------------------------------------------
# Populates SPEEDS[k]=mbps for every configured PORT|Ethernet<k>.  Returns 0 on a
# successful non-empty read, 1 on a DB/read failure OR an empty result (heavy-load
# glitch) - in which case the previous SPEEDS map is retained by the caller.
declare -A SPEEDS
read_all_speeds() {
    command -v sonic-db-cli >/dev/null 2>&1 || return 1

    local out rc line key k spd n=0
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

    local -A _ns=()
    while IFS= read -r line; do
        [ -z "$line" ] && continue
        key="${line%%=*}"; spd="${line#*=}"
        k="${key#PORT|Ethernet}"
        case "$k" in ''|*[!0-9]*) continue ;; esac
        [ -n "$spd" ] && { _ns[$k]="$spd"; n=$((n + 1)); }
    done <<EOF
$out
EOF
    [ "$n" -eq 0 ] && return 1              # retain last-known-good on empty result
    SPEEDS=()
    for k in "${!_ns[@]}"; do SPEEDS[$k]=${_ns[$k]}; done
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
    command -v sonic-db-cli >/dev/null 2>&1 || return 1

    local out rc line key k st n=0
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

    local -A _no=()
    while IFS= read -r line; do
        [ -z "$line" ] && continue
        key="${line%%=*}"; st="${line#*=}"
        k="${key#PORT_TABLE:Ethernet}"
        case "$k" in ''|*[!0-9]*) continue ;; esac
        [ -n "$st" ] && { _no[$k]="$st"; n=$((n + 1)); }
    done <<EOF
$out
EOF
    # Retain last-known-good on an empty result (e.g. a busy Redis / heavy-load
    # poll that returns success-but-empty) so a single glitchy read never marks
    # every port down and flickers all LEDs off / red.
    [ "$n" -eq 0 ] && return 1
    OPER=()
    for k in "${!_no[@]}"; do OPER[$k]=${_no[$k]}; done
    return 0
}

# ---- STATE_DB transceiver presence -----------------------------------------
# Populates PRESENT[k]=1 when TRANSCEIVER_STATUS_SW:Ethernet<k> status==1 (module
# inserted & software-ready), else 0.  Returns non-zero on a DB/read failure and
# leaves the previous map intact so a transient hiccup never blanks LED2.
#
# Reliability note: the TRANSCEIVER_STATUS_SW keys only DISAPPEAR entirely when
# xcvrd/pmon is not running (a genuine module removal keeps the key with
# status=0).  So a query that yields ZERO parsed rows means the data source is
# unavailable, NOT that every module was pulled.  In that case we treat it as a
# read failure and RETAIN the last-known presence map, so a pmon restart/crash
# does not blank every activity LED while the links are still up.
declare -A PRESENT
read_all_present() {
    command -v sonic-db-cli >/dev/null 2>&1 || return 1
    local out rc line key k st n=0
    out="$(sonic-db-cli STATE_DB EVAL \
        'local r={}; local ks=redis.call("keys","TRANSCEIVER_STATUS_SW|Ethernet*"); for _,k in ipairs(ks) do local s=redis.call("hget",k,"status"); r[#r+1]=k.."="..(s or "") end; return r' \
        0 2>/dev/null)"
    rc=$?
    [ $rc -ne 0 ] && return 1
    local -A _np=()
    while IFS= read -r line; do
        [ -z "$line" ] && continue
        key="${line%%=*}"; st="${line#*=}"
        k="${key#TRANSCEIVER_STATUS_SW|Ethernet}"
        case "$k" in ''|*[!0-9]*) continue ;; esac
        if [ "$st" = "1" ]; then _np[$k]=1; else _np[$k]=0; fi
        n=$((n + 1))
    done <<EOF
$out
EOF
    [ "$n" -eq 0 ] && return 1          # source unavailable (xcvrd down) -> retain last-known
    PRESENT=()
    for k in "${!_np[@]}"; do PRESENT[$k]=${_np[$k]}; done
    return 0
}

# ---- STATE_DB laser (tx disable) state -------------------------------------
# Populates TXDIS[k]=1 when the transceiver reports its laser disabled.  A module
# is treated as laser-off if tx1disable=True OR tx_disabled_channel!=0 (all-lane
# mask).  Missing key -> laser assumed on (0).  Retains state on read failure.
declare -A TXDIS
read_all_txdis() {
    command -v sonic-db-cli >/dev/null 2>&1 || return 1
    local out rc line key k v d ch n=0
    out="$(sonic-db-cli STATE_DB EVAL \
        'local r={}; local ks=redis.call("keys","TRANSCEIVER_STATUS|Ethernet*"); for _,k in ipairs(ks) do local d=redis.call("hget",k,"tx1disable"); local c=redis.call("hget",k,"tx_disabled_channel"); r[#r+1]=k.."="..(d or "")..","..(c or "") end; return r' \
        0 2>/dev/null)"
    rc=$?
    [ $rc -ne 0 ] && return 1
    local -A _nt=()
    while IFS= read -r line; do
        [ -z "$line" ] && continue
        key="${line%%=*}"; v="${line#*=}"
        k="${key#TRANSCEIVER_STATUS|Ethernet}"
        case "$k" in ''|*[!0-9]*) continue ;; esac
        d="${v%%,*}"; ch="${v#*,}"
        if [ "$d" = "True" ] || { [ -n "$ch" ] && [ "$ch" != "0" ]; }; then
            _nt[$k]=1
        else
            _nt[$k]=0
        fi
        n=$((n + 1))
    done <<EOF
$out
EOF
    [ "$n" -eq 0 ] && return 1              # retain last-known-good on empty result
    TXDIS=()
    for k in "${!_nt[@]}"; do TXDIS[$k]=${_nt[$k]}; done
    return 0
}

# ---- CONFIG_DB admin_status ------------------------------------------------
# Populates ADMIN[k]=up|down.  Used to decide whether a link-down port with a
# live laser should show the RED "LOS" state (only when admin=up) or stay dark
# (admin/unconfigured down).  Retains state on read failure or empty result.
declare -A ADMIN
read_all_admin() {
    command -v sonic-db-cli >/dev/null 2>&1 || return 1
    local out rc line key k st n=0
    out="$(sonic-db-cli CONFIG_DB EVAL \
        'local r={}; local ks=redis.call("keys","PORT|Ethernet*"); for _,k in ipairs(ks) do local s=redis.call("hget",k,"admin_status"); r[#r+1]=k.."="..(s or "") end; return r' \
        0 2>/dev/null)"
    rc=$?
    [ $rc -ne 0 ] && return 1
    local -A _na=()
    while IFS= read -r line; do
        [ -z "$line" ] && continue
        key="${line%%=*}"; st="${line#*=}"
        k="${key#PORT|Ethernet}"
        case "$k" in ''|*[!0-9]*) continue ;; esac
        [ -n "$st" ] && { _na[$k]="$st"; n=$((n + 1)); }
    done <<EOF
$out
EOF
    [ "$n" -eq 0 ] && return 1              # retain last-known-good on empty result
    ADMIN=()
    for k in "${!_na[@]}"; do ADMIN[$k]=${_na[$k]}; done
    return 0
}

# ---- COUNTERS_DB octet counters --------------------------------------------
# Populates OCTETS[k]=<in+out octets> for every mapped port, resolving the SAI
# counter OID via COUNTERS_PORT_NAME_MAP.  Octets (not packets) are used so any
# frame - including control traffic - registers as activity.  Retains state on
# read failure so blink state survives a transient DB hiccup.
declare -A OCTETS
read_all_octets() {
    command -v sonic-db-cli >/dev/null 2>&1 || return 1
    local out rc line key k v ic oc
    out="$(sonic-db-cli COUNTERS_DB EVAL \
        'local r={}; local m=redis.call("hgetall","COUNTERS_PORT_NAME_MAP"); for i=1,#m,2 do local n=m[i]; local oid=m[i+1]; local a=redis.call("hget","COUNTERS:"..oid,"SAI_PORT_STAT_IF_IN_OCTETS"); local b=redis.call("hget","COUNTERS:"..oid,"SAI_PORT_STAT_IF_OUT_OCTETS"); r[#r+1]=n.."="..((a or "0").."+"..(b or "0")) end; return r' \
        0 2>/dev/null)"
    rc=$?
    [ $rc -ne 0 ] && return 1
    OCTETS=()
    while IFS= read -r line; do
        [ -z "$line" ] && continue
        key="${line%%=*}"; v="${line#*=}"
        k="${key#Ethernet}"
        case "$k" in ''|*[!0-9]*) continue ;; esac
        ic="${v%%+*}"; oc="${v#*+}"
        case "$ic" in ''|*[!0-9]*) ic=0 ;; esac
        case "$oc" in ''|*[!0-9]*) oc=0 ;; esac
        OCTETS[$k]=$(( ic + oc ))
    done <<EOF
$out
EOF
    return 0
}

# ---- activity/traffic tracking ---------------------------------------------
# Compares the latest OCTETS against the previous sample to refresh a per-port
# "recently active" hold counter.  A positive delta re-arms ACT_HOLD[k] to
# ACTIVITY_HOLD; an idle poll decays it toward zero.  Counter reset/remap (delta
# < 0) and the first sample only seed the baseline (no false activity).
declare -A PREV_OCTETS ACT_HOLD
update_activity() {
    local k cur prev delta
    for k in "${!OCTETS[@]}"; do
        cur="${OCTETS[$k]}"
        prev="${PREV_OCTETS[$k]:-}"
        if [ -n "$prev" ]; then
            if [ "$cur" -ge "$prev" ]; then delta=$((cur - prev)); else delta=0; fi
            if [ "$delta" -gt 0 ]; then
                ACT_HOLD[$k]=$ACTIVITY_HOLD
            elif [ "${ACT_HOLD[$k]:-0}" -gt 0 ]; then
                ACT_HOLD[$k]=$(( ${ACT_HOLD[$k]} - 1 ))
            fi
        fi
        PREV_OCTETS[$k]="$cur"
    done
}

# True (0) when at least one port currently has an active traffic hold, i.e. the
# activity blink needs to run this poll.  Lets the watcher skip per-subtick
# rendering entirely when everything is idle/steady.
any_blinking() {
    local k
    for k in "${!ACT_HOLD[@]}"; do
        [ "${ACT_HOLD[$k]:-0}" -gt 0 ] && return 0
    done
    return 1
}

# Resolve the Link/Activity LED (LED2) 3-bit colour for logical port index k.
# BLINK_PHASE (toggled once per blink sub-tick) drives the on/off phase of the green blink.
# Result is returned in the global ACT_CODE (no subshell/echo).
BLINK_PHASE=0
ACT_CODE=0
activity_code() {
    local k="$1"
    [ "${PRESENT[$k]:-0}" = "1" ] || { ACT_CODE=$ACT_COLOUR_OFF; return; }   # no module
    [ "${TXDIS[$k]:-0}" = "1" ]   && { ACT_CODE=$ACT_COLOUR_OFF; return; }   # laser off
    if [ "${OPER[$k]:-}" = "up" ]; then
        if [ "${ACT_HOLD[$k]:-0}" -gt 0 ] && [ "$BLINK_PHASE" = "1" ]; then
            ACT_CODE=$ACT_COLOUR_OFF                                         # blink: off phase
        else
            ACT_CODE=$ACT_COLOUR_LINK                                        # green: link / on phase
        fi
    elif [ "${ADMIN[$k]:-}" = "up" ]; then
        ACT_CODE=$ACT_COLOUR_LOS                                             # red: laser on, no link (LOS)
    else
        ACT_CODE=$ACT_COLOUR_OFF                                            # admin/unconfigured down
    fi
}


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

    local k rv led1 led2 p bit mask i1 i2 last enabled
    last=$((QSFP_ETH_BASE + QSFP_PORT_COUNT - 1))
    for (( k=0; k<=last; k++ )); do
        led1=0; led2=0

        if [ "$k" -lt "$SFP_PORT_COUNT" ]; then
            # ================= SFP port =================
            # LED1 = SPEED LED: colour-by-speed, lit only when the port is up.
            rv=0
            [ "${OPER[$k]:-}" = "up" ] && { speed_to_regval "${SPEEDS[$k]:-}"; rv=$REG_VAL; }
            led1=$(( rv & 7 ))
            # LED2 = dedicated LINK/ACTIVITY LED (or legacy speed colour).
            if [ "$ACTIVITY_LED_ENABLE" = "1" ]; then
                activity_code "$k"; led2=$ACT_CODE
            else
                led2=$(( (rv >> 3) & 7 ))
            fi

            p=$((k + 1))                              # SFP physical port 1..40
            if [ "$p" -le 20 ]; then bit=$((p - 1));  i1=1; i2=2
            else                     bit=$((p - 21)); i1=3; i2=4; fi
            mask=$((1 << bit))
            _set_led_bits SFP_TGT "$i1" "$led1" "$mask"
            _set_led_bits SFP_TGT "$i2" "$led2" "$mask"

        elif [ "$k" -le "$last" ]; then
            # ================= QSFP port =================
            # Both LEDs carry the rate colour.  Per platform spec: OFF only when
            # the port is disabled (no module / admin down / laser disabled);
            # otherwise lit with the rate colour, blinking together on traffic
            # and remaining lit (not red/off) on LOS / link-down.
            if [ "$ACTIVITY_LED_ENABLE" = "1" ]; then
                enabled=0
                if [ "${PRESENT[$k]:-0}" = "1" ] && \
                   [ "${ADMIN[$k]:-}" = "up" ] && \
                   [ "${TXDIS[$k]:-0}" != "1" ]; then
                    enabled=1
                fi
                if [ "$enabled" = "1" ]; then
                    speed_to_regval "${SPEEDS[$k]:-}"; rv=$REG_VAL   # configured speed (shows on LOS)
                    led1=$(( rv & 7 )); led2=$(( (rv >> 3) & 7 ))
                    # Blink both LEDs together while there is recent traffic.
                    if [ "${ACT_HOLD[$k]:-0}" -gt 0 ] && [ "$BLINK_PHASE" = "1" ]; then
                        led1=0; led2=0
                    fi
                fi
            else
                # Legacy: rate colour gated on oper-up, no blink.
                rv=0
                [ "${OPER[$k]:-}" = "up" ] && { speed_to_regval "${SPEEDS[$k]:-}"; rv=$REG_VAL; }
                led1=$(( rv & 7 )); led2=$(( (rv >> 3) & 7 ))
            fi

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
_colour_name() { case "$1" in R) CNAME=RED ;; G) CNAME=GREEN ;; B) CNAME=BLUE ;; esac; }
CNAME=""
program_targets() {
    local force="${1:-0}" rc=0 c n key val cur regname hex cname
    for c in R G B; do
        _colour_name "$c"; cname="$CNAME"
        for n in 1 2 3 4; do
            key="$c$n"; val=${SFP_TGT[$key]:-0}; cur=${LAST_SFP[$key]:-__unset__}
            if [ "$force" = "1" ] || [ "$cur" != "$val" ]; then
                regname="RUDRA40_GLUE_LED_SFP_${cname}_${n}"
                printf -v hex '0x%08X' "$val"
                if $PLREG write "$regname" "$hex" >/dev/null 2>&1; then
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
                printf -v hex '0x%08X' "$val"
                if $PLREG write "$regname" "$hex" >/dev/null 2>&1; then
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
        _colour_name "$c"; cname="$CNAME"
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

# Refresh the Link/Activity LED inputs (transceiver presence, laser state, admin
# state, traffic counters) and update the per-port activity hold.  Best-effort:
# each read retains its previous map on failure so a transient DB hiccup never
# blanks or falsely blinks LED2.  No-op when the activity LED is disabled.
read_activity_inputs() {
    [ "$ACTIVITY_LED_ENABLE" = "1" ] || return 0
    read_all_present
    read_all_txdis
    read_all_admin
    read_all_octets && update_activity
}

# ---- modes ------------------------------------------------------------------
do_oneshot() {
    log_info "Starting $PLATFORM FPGA LED initialization (oneshot)..."
    enable_led_output
    clear_all_led_regs
    if read_all_speeds; then
        read_all_oper
        read_activity_inputs        # seeds counter baseline; no blink on first pass
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
    # Blink half-period (seconds per ON/OFF phase); fractional allowed, clamped to
    # the poll interval.  subticks = how many blink phases fit in one DB poll, so
    # the activity blink runs at LED_BLINK_INTERVAL while the DB is sampled once
    # per interval.
    local blink="$LED_BLINK_INTERVAL"
    case "$blink" in ''|*[!0-9.]*|.|*.*.*) blink=0.5 ;; esac
    local subticks
    subticks=$(awk -v i="$interval" -v b="$blink" \
        'BEGIN{ if(b<=0){print 1; exit} n=int(i/b + 0.5); if(n<1)n=1; print n }')
    log_info "Starting $PLATFORM FPGA LED watcher (interval=${interval}s, blink=${blink}s x${subticks}, resync every ${LED_WATCH_RESYNC} polls, activity_led=${ACTIVITY_LED_ENABLE})"
    trap 'log_info "$PLATFORM FPGA LED watcher exiting"; exit 0' INT TERM

    local have_cache=0 count=0 force t
    # Assert SW-override mode up front, but DON'T clear the LEDs here: the first
    # successful poll clears+repaints atomically (have_cache==0 path below).  This
    # way a restart during a heavy-load spike, where the first DB reads may glitch,
    # retains the existing LED state instead of blanking the panel until reads recover.
    [ ! -e "$PAUSE_SENTINEL" ] && enable_led_output
    while true; do
        if [ -e "$PAUSE_SENTINEL" ]; then
            # LED test in progress -> hands off entirely; re-init on resume.
            sleep "$blink"; count=0; have_cache=0; continue
        fi
        if read_all_speeds; then
            read_all_oper
            read_activity_inputs
            force=0
            [ "$have_cache" -eq 0 ] && force=1
            [ $((count % LED_WATCH_RESYNC)) -eq 0 ] && force=1
            if [ "$force" -eq 1 ]; then
                enable_led_output                 # self-heal the override mode
                [ "$have_cache" -eq 0 ] && clear_all_led_regs
            fi
            # Render the steady state once per poll (BLINK_PHASE=0 = "on" phase),
            # re-asserting colour for any speed/oper/presence change.
            BLINK_PHASE=0
            compute_targets
            program_targets "$force"
            have_cache=1
            # Only run the intra-poll blink sub-ticks when at least one port is
            # actively blinking (recent traffic).  When nothing is blinking - the
            # common idle case - the LED state is static for the whole interval,
            # so we just sleep it out (near-zero CPU: no per-subtick recompute).
            if any_blinking; then
                for (( t=1; t<subticks; t++ )); do
                    sleep "$blink"
                    BLINK_PHASE=$(( BLINK_PHASE ^ 1 ))
                    compute_targets
                    program_targets 0
                done
                sleep "$blink"
            else
                sleep "$interval"
            fi
        else
            log_error "CONFIG_DB read failed; retaining current LED state"
            sleep "$interval"
        fi
        count=$((count + 1))
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
