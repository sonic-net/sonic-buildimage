#
# SPDX-FileCopyrightText: NVIDIA CORPORATION & AFFILIATES
# Copyright (c) 2026 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
# Apache-2.0
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

try:
    from datetime import datetime, timedelta
except ImportError as e:
    raise ImportError(str(e) + " - required module not found")


# ---------------------------------------------------------------------------
# Timestamp constants and helpers
# ---------------------------------------------------------------------------
#
# dom_mgr writes _SET_TIME values via:
#     datetime.utcnow().strftime("%a %b %d %H:%M:%S %Y")
# and uses the literal "never" as a sentinel for the initial state.
#   see sonic-xcvrd/xcvrd/dom/utilities/db/utils.py:
#       NEVER = "never"           (line 9)
#       get_current_time()        (line 161)
#
# We MUST use the exact same format and sentinel. The constants are
# deliberately duplicated - importing from dom_mgr would create a runtime
# dependency from mlnx-platform-api on sonic-xcvrd, which we don't want.
#
# "%a %b %d %H:%M:%S %Y" is NOT lexicographically comparable - "Feb" < "Jan"
# lex-wise even though February follows January in real time. We therefore
# always parse to datetime before comparing.
#

_TS_FORMAT = "%a %b %d %H:%M:%S %Y"  # keep in sync with dom_mgr.get_current_time
NEVER = "never"                      # keep in sync with dom_mgr.NEVER

# How far back from an interrupt's arrival a latch may be stamped and
# still be attributed to that interrupt.
MAX_LATCH_AGE_S = 90

def _is_never(ts):
    """True iff `ts` represents "no _SET_TIME observed yet".

    Treats three inputs as equivalent to the sentinel:
      - None                : missing field (HGETALL returned no key)
      - ""                  : defensive guard against a malformed producer
      - "never"             : dom_mgr's actual initialization value

    Case-sensitive: "Never" / "NEVER" are treated as real (malformed)
    strings, not sentinels, so they will surface as ValueError on parse
    and be reported by the caller instead of silently swallowed.
    """
    return ts is None or ts == "" or ts == NEVER


def _parse_set_time(ts):
    """Parse an asctime-formatted _SET_TIME string. Raises ValueError on bad input."""
    return datetime.strptime(ts, _TS_FORMAT)

# ---------------------------------------------------------------------------
# _FLAG_TO_TOKEN: field-name -> xcvr_cpo_* token
# ---------------------------------------------------------------------------
#
# Keys are the exact field names dom_mgr writes into both the _FLAG value
# tables and their _SET_TIME companion tables (identical vocabulary -
# dom_mgr writes both from the same curr_flag_dict).
#
# Values are the canonical xcvr_cpo_* tokens the FaultIndicationTask emits
# into TRANSCEIVER_STATUS_SW|<port>.xcvr_fault (union-joined with '|').
#
# parse_set_time_rows() iterates over _FLAG_TO_TOKEN.items(), so unlisted fields are silently skipped.

_FLAG_TO_TOKEN = {
    # page 0, byte 9, bits 0-3 (case-temp alarms/warns) ------------------
    # In:  TRANSCEIVER_DOM_FLAG        (values)
    # We read: TRANSCEIVER_DOM_FLAG_SET_TIME
    'tempHAlarm':                        'xcvr_cpo_module_case_temp_high_alarm',
    'tempLAlarm':                        'xcvr_cpo_module_case_temp_low_alarm',
    'tempHWarn':                         'xcvr_cpo_module_case_temp_high_warning',
    'tempLWarn':                         'xcvr_cpo_module_case_temp_low_warning',

    # page 0, byte 11, bits 0-3 (Aux3 = laser temp on CPO-ELS) ----------
    # In:  TRANSCEIVER_DOM_FLAG        (values)
    # We read: TRANSCEIVER_DOM_FLAG_SET_TIME
    'lasertempHAlarm':                   'xcvr_cpo_module_laser_temp_high_alarm',
    'lasertempLAlarm':                   'xcvr_cpo_module_laser_temp_low_alarm',
    'lasertempHWarn':                    'xcvr_cpo_module_laser_temp_high_warning',
    'lasertempLWarn':                    'xcvr_cpo_module_laser_temp_low_warning',

    # page 0, byte 11, bits 4-7 (Custom Mon, ELS re-read) ---------------
    # In:  TRANSCEIVER_ELS_DOM_FLAG    (values)
    # We read: TRANSCEIVER_ELS_DOM_FLAG_SET_TIME
    # NvidiaCpoElsCmisApi publishes this nibble as the ELS temperature
    # flags, since the NVIDIA laser source reports its temperature in the
    # Custom Monitor slot.
    'temperature_alarm_high':            'xcvr_cpo_els_custom_mon_high_alarm',
    'temperature_alarm_low':             'xcvr_cpo_els_custom_mon_low_alarm',
    'temperature_warn_high':             'xcvr_cpo_els_custom_mon_high_warning',
    'temperature_warn_low':              'xcvr_cpo_els_custom_mon_low_warning',

    # page 1A, bytes 190-193 (per-lane ELS power alarms/warns) ------
    # In:  TRANSCEIVER_ELS_DOM_FLAG    (values)
    # We read: TRANSCEIVER_ELS_DOM_FLAG_SET_TIME
    **{'optical_power_alarm_high_lane%d' % lane: 'xcvr_cpo_els_high_power_alarm_lane%d' % lane for lane in range(1, 9)},
    **{'optical_power_alarm_low_lane%d'  % lane: 'xcvr_cpo_els_low_power_alarm_lane%d'  % lane for lane in range(1, 9)},
    **{'optical_power_warn_high_lane%d'  % lane: 'xcvr_cpo_els_high_power_warn_lane%d'  % lane for lane in range(1, 9)},
    **{'optical_power_warn_low_lane%d'   % lane: 'xcvr_cpo_els_low_power_warn_lane%d'   % lane for lane in range(1, 9)},

    # page 1A, bytes 166 / 174 (per-lane fault/warn locator flags) --
    # In:  TRANSCEIVER_ELS_STATUS_FLAG     (values)
    # We read: TRANSCEIVER_ELS_STATUS_FLAG_SET_TIME
    # Bit N names the laser and nothing else; what went wrong is in that
    # lane's code byte. See drop_locators_with_known_cause below.
    **{'fault_flag_lane%d'   % lane:     'xcvr_cpo_els_fault_lane%d' % lane for lane in range(1, 9)},
    **{'warning_flag_lane%d' % lane:     'xcvr_cpo_els_warn_lane%d'  % lane for lane in range(1, 9)},
}


# Tokens whose field belongs to the ELS rather than to one OE bank.
#
# Every logical port of a vModule shares a single ELS while each pair of
# ports has its own OE bank - in optical_devices.json all eight ports of
# OE0 carry `ELS0` bank 0, and only the OE bank varies (0-3). An ELS
# fault therefore affects the whole vModule, but dom_mgr records these
# fields on the bank-0 row alone, so the fan-out has to widen them.
#
# The non-ELS fields stay bank-scoped: dom_mgr reads them from each OE
# bank's own pages and publishes them per bank already.
ELS_SCOPED_TOKENS = frozenset(
    token for token in _FLAG_TO_TOKEN.values()
    if token.startswith('xcvr_cpo_els_'))


def parse_set_time_rows(logical_port, merged_set_times, watermark, arrival):
    """Walk _FLAG_TO_TOKEN and return the tokens for latches that are
    both unreported and recent enough to belong to this interrupt.

    dom_mgr stamps a field's _SET_TIME every time it observes that flag
    go from clear to set, so each distinct value is one latch it saw. A
    field earns a token only if it passes two independent tests:

      1. The value must differ from what we recorded for that field
         last time.
      2. The latch must be no older than MAX_LATCH_AGE_S before
         `arrival`.

    Args:
        logical_port: port name (e.g. "Ethernet0").
        merged_set_times: {field_name: set_time_string} dict spanning
            all four _SET_TIME tables for this port - DOM_FLAG,
            STATUS_FLAG, ELS_DOM_FLAG and ELS_STATUS_FLAG.
        watermark: {field_name: set_time_string} - what this task has
            already accounted for on this port.
        arrival: datetime (UTC) at which chassis observed the interrupt.

    Returns:
        (tokens, advanced) where `tokens` is the sorted, deduplicated
        list of xcvr_cpo_* token names to report, and `advanced` is
        {field_name: set_time_string} for the caller to merge into
        `watermark`.
    """
    tokens = set()
    advanced = {}
    oldest_attributable = arrival - timedelta(seconds=MAX_LATCH_AGE_S)
    for field, token in _FLAG_TO_TOKEN.items():
        set_time = merged_set_times.get(field)
        if _is_never(set_time):
            continue
        # Same stamp as last time means dom_mgr has not seen the flag
        # latch again, so there is nothing new to report.
        if set_time == watermark.get(field):
            continue
        advanced[field] = set_time
        try:
            if _parse_set_time(set_time) < oldest_attributable:
                continue
        except (TypeError, ValueError):
            # A stamp we cannot read tells us nothing about its age, so
            # report it rather than drop it. Losing a real fault is worse
            # than one spurious token.
            pass
        tokens.add(token)
    return sorted(tokens), advanced


# ---------------------------------------------------------------------------
# ELS per-lane fault/warning codes (pg1A:212-219)
# ---------------------------------------------------------------------------
#
# One byte per lane: pg1A byte 212 + (lane - 1) carries BOTH of that
# lane's 4-bit codes.
#
#   bits 0-3 (low nibble)  = fault code
#   bits 4-7 (high nibble) = warning code
#
# The above codes are not Clear-on-Read, so they are not included in the _SET_TIME tables.

ELS_LANE_CODES_LENGTH = 8  # one byte per lane, lanes 1-8

_FAULT_CODE_BITPOS = 0
_WARNING_CODE_BITPOS = 4


def _extract_lane_code(eeprom_bytes, lane, bitpos):
    """Extract one 4-bit code for `lane` (1-8) from that lane's byte.

    `bitpos` selects which of the two codes the byte holds: 0 for the
    fault nibble, 4 for the warning nibble.
    """
    return (eeprom_bytes[lane - 1] >> bitpos) & 0x0F


def describe_lane_code(code):
    """Short meaning of a 4-bit ELS lane code.
    """
    if code == 0:
        return "no alarm"
    if code == 1:
        return "APC control loop"
    if code == 2:
        return "ACC control loop"
    if 3 <= code <= 8:
        return "reserved"
    return "vendor specific"


def parse_els_lane_codes(eeprom_bytes):
    """Decode 8 bytes read directly from EEPROM (pg1A:212-219) into
    per-lane ELS fault/warning tokens.

    Args:
        eeprom_bytes: bytes-like object of length ELS_LANE_CODES_LENGTH
            (8 bytes) read directly from EEPROM.
    Returns:
        list[(token, code)] pairs, one per non-zero lane code:
          xcvr_cpo_els_fault_code_laneN    for non-zero fault codes
          xcvr_cpo_els_warning_code_laneN  for non-zero warn codes
        Deterministic order (lane 1 fault, lane 1 warn, lane 2 fault, ...).

    These tokens re-emit on every interrupt because the interrupt itself
    is HW's assertion that something changed. The RMW-union fan-out in
    FaultIndicationTask deduplicates repeated tokens in STATE_DB, so
    only the syslog line repeats - not the DB write.
    """
    if not eeprom_bytes or len(eeprom_bytes) < ELS_LANE_CODES_LENGTH:
        return []

    pairs = []
    for lane in range(1, 9):
        fault_code = _extract_lane_code(eeprom_bytes, lane, _FAULT_CODE_BITPOS)
        if fault_code:
            pairs.append(
                ("xcvr_cpo_els_fault_code_lane%d" % lane, fault_code))
        warn_code = _extract_lane_code(eeprom_bytes, lane, _WARNING_CODE_BITPOS)
        if warn_code:
            pairs.append(
                ("xcvr_cpo_els_warning_code_lane%d" % lane, warn_code))
    return pairs


# ---------------------------------------------------------------------------
# Locator flags superseded by a cause code
# ---------------------------------------------------------------------------
#
# A laser source fault - APC, TEC or ramping - is recorded twice on page
# 1A: a bit in byte 166 (alarm) or 174 (warning) naming the laser, and a
# nibble in 212+N naming the cause. The bit says only "laser N has a
# fault", which the code token already carries along with what went
# wrong, so emitting both turns one fault into two tokens.
#
# The power faults keep their own flag token: bytes 190-193 have no
# cause code, and which of those four bytes latched is itself the cause.

_LOCATOR_TO_CODE_TOKEN = {
    **{'xcvr_cpo_els_fault_lane%d' % lane:
       'xcvr_cpo_els_fault_code_lane%d' % lane for lane in range(1, 9)},
    **{'xcvr_cpo_els_warn_lane%d' % lane:
       'xcvr_cpo_els_warning_code_lane%d' % lane for lane in range(1, 9)},
}


def drop_locators_with_known_cause(tokens, reported_code_tokens):
    """Drop the per-lane flags whose cause code was already reported.

    Args:
        tokens: what parse_set_time_rows turned up for one bank.
        reported_code_tokens: the pg1A:212-219 code tokens already
            emitted for this vModule on this interrupt.

    Returns:
        `tokens` in order, without the per-lane fault/warn flags a code
        token has accounted for. A flag whose lane produced no code is
        kept, so a fault still surfaces if the code could not be read.
    """
    return [token for token in tokens
            if _LOCATOR_TO_CODE_TOKEN.get(token) not in reported_code_tokens]


def has_locator_missing_a_cause(tokens, reported_code_tokens):
    """True iff some lane flag in `tokens` has no cause code reported yet.

    A second fault on the same vModule can latch its code after the
    EEPROM was last read, so its flag reaches us while its cause is
    still unaccounted for. The codes are not clear-on-read, so a True
    here means another read would find it.
    """
    return any(token in _LOCATOR_TO_CODE_TOKEN
               and _LOCATOR_TO_CODE_TOKEN[token] not in reported_code_tokens
               for token in tokens)
