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

# Unit tests for the pure helpers in sonic_platform.xcvr_fault:
#   NEVER / _is_never / _TS_FORMAT / _parse_set_time,
#   _FLAG_TO_TOKEN,
#   parse_set_time_rows(),
#   parse_els_lane_codes() (+ ELS_LANE_CODES_LENGTH).

import os
import sys
from datetime import datetime, timedelta

import pytest

test_path = os.path.dirname(os.path.abspath(__file__))
modules_path = os.path.dirname(test_path)
sys.path.insert(0, modules_path)

from sonic_platform.xcvr_fault import (
    ELS_LANE_CODES_LENGTH,
    MAX_LATCH_AGE_S,
    NEVER,
    _FLAG_TO_TOKEN,
    _TS_FORMAT,
    _is_never,
    _parse_set_time,
    describe_lane_code,
    drop_locators_with_known_cause,
    has_locator_missing_a_cause,
    parse_els_lane_codes,
    parse_set_time_rows,
)


# ---------------------------------------------------------------------------
# Shared fixtures
# ---------------------------------------------------------------------------

TS_1508_STR = 'Wed Jul 08 15:08:00 2026'
TS_1509_STR = 'Wed Jul 08 15:09:00 2026'

# Interrupt arrival used by the parse_set_time_rows cases. Chosen so both
# timestamps above fall inside the MAX_LATCH_AGE_S window: TS_1508 is 45 s
# before it and TS_1509 15 s after it.
ARRIVAL = datetime(2026, 7, 8, 15, 8, 45)

# Comfortably outside the window - a latch from a previous hour.
TS_STALE_STR = 'Wed Jul 08 15:00:00 2026'

_PORT = 'Ethernet0'


# ---------------------------------------------------------------------------
# _is_never() and NEVER constant
# ---------------------------------------------------------------------------

class TestIsNever:

    def test_is_never_recognises_all_sentinels_and_rejects_real_values(self):
        # dom_mgr writes the literal "never" (utils.py:9); None handles
        # a missing HGETALL key; "" is a defensive guard against a
        # malformed producer. Real timestamps and case variants must NOT
        # be treated as sentinels.
        assert NEVER == 'never'
        for sentinel in (None, '', 'never'):
            assert _is_never(sentinel) is True, sentinel
        for real in (TS_1508_STR, 'Never', 'NEVER'):
            assert _is_never(real) is False, real


# ---------------------------------------------------------------------------
# _parse_set_time() and _TS_FORMAT constant
# ---------------------------------------------------------------------------

class TestParseSetTime:

    def test_valid_asctime_parses_and_format_constant_matches_dom_mgr(self):
        # dom_mgr's default in get_current_time (utils.py:161). If the
        # format drifts, every real timestamp becomes malformed on our side.
        assert _TS_FORMAT == "%a %b %d %H:%M:%S %Y"
        dt = _parse_set_time(TS_1508_STR)
        assert (dt.year, dt.month, dt.day, dt.hour, dt.minute) == (2026, 7, 8, 15, 8)

    @pytest.mark.parametrize('bad', [
        '2026-07-08T15:08:00',   # ISO format - guards a well-intentioned refactor
        '',                      # callers must _is_never('') before reaching here
        'garbage',
    ])
    def test_invalid_inputs_raise_valueerror(self, bad):
        with pytest.raises(ValueError):
            _parse_set_time(bad)


# ---------------------------------------------------------------------------
# _FLAG_TO_TOKEN mapping
# ---------------------------------------------------------------------------
#
# Pure-data locks - shape, coverage, invariants, and "must-not-be-here"
# fields. Any drift is caught in review.
#

class TestFlagToTokenModuleLevelGroups:
    """Non-per-lane groups on page 0."""

    def test_all_module_level_fields_map_to_expected_tokens(self):
        expected = {
            # page 0, byte 9, bits 0-3 - case-temperature alarms/warns.
            'tempHAlarm': 'xcvr_cpo_module_case_temp_high_alarm',
            'tempLAlarm': 'xcvr_cpo_module_case_temp_low_alarm',
            'tempHWarn':  'xcvr_cpo_module_case_temp_high_warning',
            'tempLWarn':  'xcvr_cpo_module_case_temp_low_warning',
            # page 0, byte 11, bits 0-3 - Aux3 laser-temp on CPO-ELS.
            'lasertempHAlarm': 'xcvr_cpo_module_laser_temp_high_alarm',
            'lasertempLAlarm': 'xcvr_cpo_module_laser_temp_low_alarm',
            'lasertempHWarn':  'xcvr_cpo_module_laser_temp_high_warning',
            'lasertempLWarn':  'xcvr_cpo_module_laser_temp_low_warning',
            # page 0, byte 11, bits 4-7 - ELS re-read of Custom Monitor bits.
            'temperature_alarm_high': 'xcvr_cpo_els_custom_mon_high_alarm',
            'temperature_alarm_low':  'xcvr_cpo_els_custom_mon_low_alarm',
            'temperature_warn_high':  'xcvr_cpo_els_custom_mon_high_warning',
            'temperature_warn_low':   'xcvr_cpo_els_custom_mon_low_warning',
        }
        for field, token in expected.items():
            assert _FLAG_TO_TOKEN.get(field) == token, field


class TestFlagToTokenPerLaneGroups:
    """Per-lane ELS groups (page 1A) - each has exactly 8 lanes, all
    following predictable field%d/token%d naming."""

    _TEMPLATES = [
        # page 1A, bytes 190-193 - per-lane power alarms/warns.
        ('optical_power_alarm_high_lane%d', 'xcvr_cpo_els_high_power_alarm_lane%d'),
        ('optical_power_alarm_low_lane%d',  'xcvr_cpo_els_low_power_alarm_lane%d'),
        ('optical_power_warn_high_lane%d',  'xcvr_cpo_els_high_power_warn_lane%d'),
        ('optical_power_warn_low_lane%d',   'xcvr_cpo_els_low_power_warn_lane%d'),
        # page 1A, bytes 166 / 174 - per-lane fault/warn summary flags.
        ('fault_flag_lane%d',   'xcvr_cpo_els_fault_lane%d'),
        ('warning_flag_lane%d', 'xcvr_cpo_els_warn_lane%d'),
    ]

    def test_every_group_has_exactly_eight_lanes_all_mapping_correctly(self):
        for field_tmpl, token_tmpl in self._TEMPLATES:
            # Off-by-one guard: exactly 8 fields with this prefix, no more.
            prefix = field_tmpl.replace('%d', '')
            matches = [k for k in _FLAG_TO_TOKEN if k.startswith(prefix)]
            assert len(matches) == 8, (
                "prefix %r has %d fields, expected 8" % (prefix, len(matches)))

            for lane in range(1, 9):
                field, expected_token = field_tmpl % lane, token_tmpl % lane
                assert _FLAG_TO_TOKEN.get(field) == expected_token, (
                    "field %r maps to %r, expected %r"
                    % (field, _FLAG_TO_TOKEN.get(field), expected_token))


class TestFlagToTokenInvariants:
    """Cross-cutting invariants that must hold for the whole mapping."""

    def test_mapping_shape_and_vocabulary_invariants(self):
        tokens = list(_FLAG_TO_TOKEN.values())

        # No two flag fields share a token - merging distinct faults into
        # one reported condition would be silently wrong.
        assert len(tokens) == len(set(tokens))

        # xcvr_fault is a '|'-joined set of xcvr_cpo_* tokens. Anything
        # else would be off-spec vocabulary.
        for token in tokens:
            assert token.startswith('xcvr_cpo_'), token

        # Non-empty string keys - sanity.
        for field in _FLAG_TO_TOKEN:
            assert isinstance(field, str) and field, field


class TestFlagToTokenExcluded:
    """dom_mgr publishes several fields into the same _SET_TIME tables
    that this feature deliberately does NOT surface as xcvr_fault tokens
    (voltage, TX/RX power, TX bias, ELS bias). If one slips in
    accidentally, catch it here."""

    def test_out_of_scope_dom_mgr_fields_are_not_mapped(self):
        excluded = [
            # Module-level voltage.
            'vccHAlarm', 'vccLAlarm', 'vccHWarn', 'vccLWarn',
            # Per-lane TX power (representative lanes only).
            'tx1powerHAlarm', 'tx8powerLWarn',
            # Per-lane RX power.
            'rx1powerHAlarm', 'rx8powerLWarn',
            # Per-lane TX bias.
            'tx1biasHAlarm', 'tx8biasLWarn',
            # Per-lane ELS bias (page 1A bytes 186-189).
            'els_HighBiasAlarm1', 'els_LowBiasAlarm8',
            'els_HighBiasWarn1',  'els_LowBiasWarn8',
        ]
        for field in excluded:
            assert field not in _FLAG_TO_TOKEN, (
                "%r unexpectedly present in _FLAG_TO_TOKEN" % field)


# ---------------------------------------------------------------------------
# parse_set_time_rows()
# ---------------------------------------------------------------------------

class TestParseSetTimeRows:
    """Rows-1-9 tokenizer. A field earns its token only if its _SET_TIME
    both differs from the value this task already accounted for and is
    no more than MAX_LATCH_AGE_S older than the interrupt's arrival.

    The first test is answered against our own bookkeeping rather than
    our wall clock, so detection is independent of whether dom_mgr
    polled the EEPROM before or after chassis noticed the interrupt. The
    second stops a latch whose interrupt we never processed from being
    attributed to an unrelated fault later on.
    """

    def test_empty_dict_reports_nothing(self):
        assert parse_set_time_rows(_PORT, {}, {}, ARRIVAL) == ([], {})

    def test_set_time_differing_from_watermark_reports_and_advances(self):
        tokens, advanced = parse_set_time_rows(
            _PORT, {'tempHAlarm': TS_1509_STR},
            {'tempHAlarm': TS_1508_STR}, ARRIVAL)
        assert tokens == ['xcvr_cpo_module_case_temp_high_alarm']
        assert advanced == {'tempHAlarm': TS_1509_STR}

    def test_set_time_equal_to_watermark_is_silent(self):
        # Already reported. Re-emitting on every subsequent interrupt
        # would turn one fault into an endless stream of warnings.
        assert parse_set_time_rows(
            _PORT, {'tempHAlarm': TS_1508_STR},
            {'tempHAlarm': TS_1508_STR}, ARRIVAL) == ([], {})

    def test_field_absent_from_watermark_reports_its_first_set_time(self):
        # A field we have never examined (module inserted after start-up)
        # must not swallow its first fault.
        tokens, advanced = parse_set_time_rows(
            _PORT, {'tempHAlarm': TS_1508_STR}, {}, ARRIVAL)
        assert tokens == ['xcvr_cpo_module_case_temp_high_alarm']
        assert advanced == {'tempHAlarm': TS_1508_STR}

    def test_latch_stamped_before_arrival_is_still_reported(self):
        # Possible race: dom_mgr polled the EEPROM and stamped the
        # fault 45 s before chassis noticed the interrupt. Nothing
        # synchronizes the two, so this ordering is normal and the fault
        # is real. Regression guard for the _SET_TIME >= arrival rule
        # that used to drop it.
        assert _parse_set_time(TS_1508_STR) < ARRIVAL
        tokens, _ = parse_set_time_rows(
            _PORT, {'tempHAlarm': TS_1508_STR}, {}, ARRIVAL)
        assert tokens == ['xcvr_cpo_module_case_temp_high_alarm']

    def test_latch_older_than_the_window_is_absorbed_without_reporting(self):
        # A latch from a previous hour cannot be what this interrupt is
        # about. It is still recorded, so it stops being re-examined.
        tokens, advanced = parse_set_time_rows(
            _PORT, {'tempHAlarm': TS_STALE_STR}, {}, ARRIVAL)
        assert tokens == []
        assert advanced == {'tempHAlarm': TS_STALE_STR}

    def test_absorbing_a_stale_latch_keeps_it_quiet_on_later_interrupts(self):
        # Two interrupts in a row against the same stale row: the first
        # absorbs it, the second sees no difference at all. Without the
        # absorb it would be re-evaluated forever.
        watermark = {}
        _, advanced = parse_set_time_rows(
            _PORT, {'tempHAlarm': TS_STALE_STR}, watermark, ARRIVAL)
        watermark.update(advanced)
        assert parse_set_time_rows(
            _PORT, {'tempHAlarm': TS_STALE_STR}, watermark,
            ARRIVAL + timedelta(hours=3)) == ([], {})

    def test_window_edge_is_inclusive(self):
        # A latch at exactly the window edge is the oldest one a dom_mgr
        # sweep covering this interrupt could have stamped, so it counts.
        edge = ARRIVAL - timedelta(seconds=MAX_LATCH_AGE_S)
        tokens, _ = parse_set_time_rows(
            _PORT, {'tempHAlarm': edge.strftime(_TS_FORMAT)}, {}, ARRIVAL)
        assert tokens == ['xcvr_cpo_module_case_temp_high_alarm']

    def test_unparseable_set_time_is_reported_rather_than_dropped(self):
        # A value we cannot place in time cannot be age-checked. Losing a
        # real fault is worse than one spurious line, and the watermark
        # keeps it to a single line.
        tokens, advanced = parse_set_time_rows(
            _PORT, {'tempHAlarm': 'not-a-timestamp'}, {}, ARRIVAL)
        assert tokens == ['xcvr_cpo_module_case_temp_high_alarm']
        assert advanced == {'tempHAlarm': 'not-a-timestamp'}

    @pytest.mark.parametrize('sentinel', ['never', '', None])
    def test_never_sentinel_reports_nothing(self, sentinel):
        # No latch has ever been recorded for this field, so there is
        # nothing to report and nothing to remember.
        assert parse_set_time_rows(
            _PORT, {'tempHAlarm': sentinel}, {}, ARRIVAL) == ([], {})

    def test_row_reset_to_never_leaves_the_watermark_alone(self):
        # dom_mgr re-initializes _SET_TIME to "never" when a module is
        # re-inserted. That is not a fault, and the previous value must
        # stay in the watermark so the next real latch still differs
        # from it.
        watermark = {'tempHAlarm': TS_1508_STR}
        assert parse_set_time_rows(
            _PORT, {'tempHAlarm': 'never'}, watermark, ARRIVAL) == ([], {})

    def test_output_is_sorted_and_covers_multiple_groups(self):
        # Two moved fields from different _FLAG_TO_TOKEN groups. Output
        # must be sorted regardless of dict iteration order.
        rows = {'fault_flag_lane3': TS_1508_STR, 'tempHAlarm': TS_1508_STR}
        tokens, advanced = parse_set_time_rows(_PORT, rows, {}, ARRIVAL)
        assert tokens == [
            'xcvr_cpo_els_fault_lane3',
            'xcvr_cpo_module_case_temp_high_alarm',
        ]
        assert advanced == {'fault_flag_lane3': TS_1508_STR,
                            'tempHAlarm': TS_1508_STR}

    def test_unknown_fields_are_ignored(self):
        # Fields dom_mgr publishes but we deliberately don't map
        # (voltage, TX/RX power, bias, ELS bias) contribute nothing.
        rows = {
            'vccHAlarm':          TS_1508_STR,   # unmapped
            'tx1powerHAlarm':     TS_1508_STR,   # unmapped
            'els_HighBiasAlarm1': TS_1508_STR,   # unmapped
            'tempHAlarm':         TS_1508_STR,   # mapped -> should surface
        }
        tokens, advanced = parse_set_time_rows(_PORT, rows, {}, ARRIVAL)
        assert tokens == ['xcvr_cpo_module_case_temp_high_alarm']
        assert advanced == {'tempHAlarm': TS_1508_STR}

    def test_watermark_argument_is_not_mutated(self):
        # The caller owns the watermark and applies the advances itself,
        # so this helper must leave its argument untouched.
        watermark = {'tempHAlarm': TS_1508_STR}
        parse_set_time_rows(
            _PORT, {'tempHAlarm': TS_1509_STR}, watermark, ARRIVAL)
        assert watermark == {'tempHAlarm': TS_1508_STR}


# ---------------------------------------------------------------------------
# parse_els_lane_codes()  (pg1A:212-219, direct EEPROM read)
# ---------------------------------------------------------------------------

class TestParseElsLaneCodes:
    """Decodes 8 EEPROM bytes into (token, code) pairs, one per non-zero
    per-lane fault/warn code.

    Byte layout, per the ELSFP memory map in sonic-platform-common
    (mem_maps/public/cmis_pages/elsfp_pages.py:205-229): one byte per
    lane, each holding both of that lane's codes.

      byte (lane - 1) bits 0-3 = lane's fault code
      byte (lane - 1) bits 4-7 = lane's warning code

    The code rides beside the token rather than inside it: xcvr_fault is
    a cumulative set, so a token has to keep a stable identity to be
    unioned into it.
    """

    def test_length_constant_matches_spec(self):
        # One byte per lane, lanes 1-8. If this changes, the decoder
        # needs a real audit; the constant is not just a magic number.
        assert ELS_LANE_CODES_LENGTH == 8

    @pytest.mark.parametrize('short_input', [None, b'', b'\x00' * 7])
    def test_none_or_short_input_returns_empty_list(self, short_input):
        # Caller may pass None on read failure or a truncated buffer;
        # both must be handled gracefully rather than crashing the task.
        assert parse_els_lane_codes(short_input) == []

    def test_all_zero_bytes_yield_no_tokens(self):
        assert parse_els_lane_codes(bytes(ELS_LANE_CODES_LENGTH)) == []

    def test_each_lane_is_decoded_from_its_own_byte(self):
        # Regression guard for a decoder that packed two lanes per byte:
        # it read this buffer as lanes 1 and 5, while the hardware means
        # lanes 1 and 3. An injection with mlxreg lane_mask=5 (lanes 1 and 3) 
        # produced exactly these
        # bytes, and dom_mgr's independent read of the byte-166 summary
        # flags reported fault_flag_lane1 + fault_flag_lane3.
        eeprom = bytes([0x09, 0x00, 0x09, 0x00, 0x00, 0x00, 0x00, 0x00])
        assert parse_els_lane_codes(eeprom) == [
            ('xcvr_cpo_els_fault_code_lane1', 9),
            ('xcvr_cpo_els_fault_code_lane3', 9),
        ]

    def test_one_byte_carries_both_of_its_lane_codes(self):
        # Low nibble is the fault code, high nibble the warning code -
        # for the *same* lane, not for two neighbouring lanes.
        eeprom = bytearray(ELS_LANE_CODES_LENGTH)
        eeprom[0] = (0x3 << 4) | 0x7
        assert parse_els_lane_codes(bytes(eeprom)) == [
            ('xcvr_cpo_els_fault_code_lane1', 7),
            ('xcvr_cpo_els_warning_code_lane1', 3),
        ]

    def test_last_lane_is_reachable(self):
        # Off-by-one guard: lane 8 owns the last byte of the window.
        eeprom = bytearray(ELS_LANE_CODES_LENGTH)
        eeprom[7] = (0xC << 4) | 0x5
        assert parse_els_lane_codes(bytes(eeprom)) == [
            ('xcvr_cpo_els_fault_code_lane8', 5),
            ('xcvr_cpo_els_warning_code_lane8', 12),
        ]

    def test_zero_code_is_suppressed_per_lane(self):
        # Only non-zero codes emit tokens, so a lane with a fault but no
        # warning emits exactly one token.
        eeprom = bytearray(ELS_LANE_CODES_LENGTH)
        eeprom[0] = 0x09
        assert parse_els_lane_codes(bytes(eeprom)) == [
            ('xcvr_cpo_els_fault_code_lane1', 9),
        ]

    def test_token_name_is_stable_across_code_values(self):
        # The whole point of keeping the value out of the token: two
        # different codes on one lane must name the same condition, or
        # the cumulative xcvr_fault set would end up holding both.
        first = bytearray(ELS_LANE_CODES_LENGTH)
        first[0] = 0x09
        second = bytearray(ELS_LANE_CODES_LENGTH)
        second[0] = 0x07
        assert ([t for t, _ in parse_els_lane_codes(bytes(first))]
                == [t for t, _ in parse_els_lane_codes(bytes(second))])

    def test_full_output_order_is_deterministic(self):
        # For a mixed input, verify the emission order is lane 1 fault,
        # lane 1 warn, lane 2 fault, lane 2 warn, ... . This is the
        # order the syslog line and RMW-union depend on.
        eeprom = bytearray(ELS_LANE_CODES_LENGTH)
        eeprom[0] = (0x3 << 4) | 0x1   # lane 1 fault=1, warn=3
        eeprom[1] = (0x4 << 4) | 0x2   # lane 2 fault=2, warn=4
        assert parse_els_lane_codes(bytes(eeprom)) == [
            ('xcvr_cpo_els_fault_code_lane1', 1),
            ('xcvr_cpo_els_warning_code_lane1', 3),
            ('xcvr_cpo_els_fault_code_lane2', 2),
            ('xcvr_cpo_els_warning_code_lane2', 4),
        ]


# ---------------------------------------------------------------------------
# describe_lane_code()
# ---------------------------------------------------------------------------


class TestDescribeLaneCode:
    """Mirrors CmisCodes.LANE_FAULT_CODE / LANE_WARNING_CODE, collapsed
    to the ranges the spec distinguishes. This is what tells an operator
    that code 9 means "vendor specific" rather than nothing at all.
    """

    @pytest.mark.parametrize('code,expected', [
        (0,  'no alarm'),
        (1,  'APC control loop'),
        (2,  'ACC control loop'),
        (3,  'reserved'),
        (8,  'reserved'),
        (9,  'vendor specific'),
        (15, 'vendor specific'),
    ])
    def test_every_range_boundary(self, code, expected):
        assert describe_lane_code(code) == expected


# ---------------------------------------------------------------------------
# drop_locators_with_known_cause()
# ---------------------------------------------------------------------------

class TestDropLocatorsWithKnownCause:
    """One laser source fault lands in two places on page 1A, and only
    one of them is worth a token.

    Per the SimX fault injection table, an APC / TEC / ramping fault
    sets a bit in byte 166 (alarm) or 174 (warning) naming the laser,
    and a nibble in 212+N naming the cause. The bit carries no detail
    the code token lacks, so reporting both makes one injected fault
    arrive as two tokens.

    The power faults are shaped differently - bytes 190-193 have no
    cause code at all - so their tokens have to survive this filter.
    """

    def test_a_warning_and_its_cause_collapse_to_the_cause(self):
        assert drop_locators_with_known_cause(
            ['xcvr_cpo_els_warn_lane1'],
            {'xcvr_cpo_els_warning_code_lane1'}) == []

    def test_an_alarm_and_its_cause_collapse_to_the_cause(self):
        assert drop_locators_with_known_cause(
            ['xcvr_cpo_els_fault_lane1'],
            {'xcvr_cpo_els_fault_code_lane1'}) == []

    def test_a_cause_on_one_lane_does_not_silence_another(self):
        # Two lasers faulting is two faults; only the lane that actually
        # reported a code may lose its flag.
        assert drop_locators_with_known_cause(
            ['xcvr_cpo_els_warn_lane1', 'xcvr_cpo_els_warn_lane2'],
            {'xcvr_cpo_els_warning_code_lane1'}
        ) == ['xcvr_cpo_els_warn_lane2']

    def test_an_alarm_cause_does_not_silence_the_warning_flag(self):
        # The two severities share byte 212+N but are separate nibbles,
        # so one says nothing about the other.
        assert drop_locators_with_known_cause(
            ['xcvr_cpo_els_warn_lane1'],
            {'xcvr_cpo_els_fault_code_lane1'}
        ) == ['xcvr_cpo_els_warn_lane1']

    def test_a_flag_with_no_cause_is_kept(self):
        # Nothing was read from 212-219, so the flag is all there is -
        # better a located fault than a silent one.
        assert drop_locators_with_known_cause(
            ['xcvr_cpo_els_warn_lane1'], set()
        ) == ['xcvr_cpo_els_warn_lane1']

    def test_power_tokens_are_untouched(self):
        # High / low power faults carry no cause code; which of bytes
        # 190-193 latched is itself the cause.
        tokens = ['xcvr_cpo_els_high_power_alarm_lane1',
                  'xcvr_cpo_els_low_power_warn_lane3']
        assert drop_locators_with_known_cause(
            tokens, {'xcvr_cpo_els_warning_code_lane1'}) == tokens

    def test_unrelated_tokens_are_untouched(self):
        tokens = ['xcvr_cpo_module_case_temp_high_alarm',
                  'xcvr_cpo_els_custom_mon_low_warning']
        assert drop_locators_with_known_cause(
            tokens, {'xcvr_cpo_els_fault_code_lane1'}) == tokens

    def test_order_is_preserved(self):
        tokens = ['xcvr_cpo_module_case_temp_high_alarm',
                  'xcvr_cpo_els_warn_lane1',
                  'xcvr_cpo_els_high_power_alarm_lane2']
        assert drop_locators_with_known_cause(
            tokens, {'xcvr_cpo_els_warning_code_lane1'}) == [
            'xcvr_cpo_module_case_temp_high_alarm',
            'xcvr_cpo_els_high_power_alarm_lane2',
        ]

    def test_every_lane_pairs_with_its_own_code(self):
        # Off-by-one guard across the full lane range, both severities.
        for lane in range(1, 9):
            assert drop_locators_with_known_cause(
                ['xcvr_cpo_els_warn_lane%d' % lane],
                {'xcvr_cpo_els_warning_code_lane%d' % lane}) == []
            assert drop_locators_with_known_cause(
                ['xcvr_cpo_els_fault_lane%d' % lane],
                {'xcvr_cpo_els_fault_code_lane%d' % lane}) == []


# ---------------------------------------------------------------------------
# has_locator_missing_a_cause()
# ---------------------------------------------------------------------------

class TestHasLocatorMissingACause:
    """Decides whether the lane codes are worth reading again.

    Faults on one vModule arrive in their own time, so a flag can reach
    us a tick after the EEPROM was last read and before its own code was
    ever seen. Answering True costs one EEPROM read; answering False
    wrongly costs a duplicate token.
    """

    def test_a_flag_with_no_cause_is_worth_another_look(self):
        assert has_locator_missing_a_cause(
            ['xcvr_cpo_els_warn_lane2'],
            {'xcvr_cpo_els_warning_code_lane1'}) is True

    def test_a_flag_whose_cause_is_known_is_not(self):
        assert has_locator_missing_a_cause(
            ['xcvr_cpo_els_warn_lane1'],
            {'xcvr_cpo_els_warning_code_lane1'}) is False

    def test_one_unexplained_flag_among_many_is_enough(self):
        assert has_locator_missing_a_cause(
            ['xcvr_cpo_els_warn_lane1', 'xcvr_cpo_els_warn_lane2'],
            {'xcvr_cpo_els_warning_code_lane1'}) is True

    def test_the_two_severities_are_asked_about_separately(self):
        assert has_locator_missing_a_cause(
            ['xcvr_cpo_els_warn_lane1'],
            {'xcvr_cpo_els_fault_code_lane1'}) is True

    def test_power_tokens_never_prompt_a_read(self):
        # Bytes 190-193 carry no cause code, so there is nothing to find.
        assert has_locator_missing_a_cause(
            ['xcvr_cpo_els_high_power_alarm_lane1',
             'xcvr_cpo_els_low_power_warn_lane3'], set()) is False

    def test_unrelated_tokens_never_prompt_a_read(self):
        assert has_locator_missing_a_cause(
            ['xcvr_cpo_module_case_temp_high_alarm'], set()) is False

    def test_an_empty_row_never_prompts_a_read(self):
        assert has_locator_missing_a_cause([], set()) is False
