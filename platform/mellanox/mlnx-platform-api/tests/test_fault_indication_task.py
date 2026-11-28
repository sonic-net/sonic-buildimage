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

import contextlib
import os
import sys
from datetime import datetime

import pytest
from unittest import mock

test_path = os.path.dirname(os.path.abspath(__file__))
modules_path = os.path.dirname(test_path)
sys.path.insert(0, modules_path)

from sonic_platform import fault_indication_task as fit_module
from sonic_platform.fault_indication_task import (
    ELS_LANE_CODES_EEPROM_OFFSET,
    LAST_UPDATE_TIME_FIELD,
    STATE_DB_DOM_FLAG_SET_TIME_TABLE,
    STATE_DB_ELS_DOM_FLAG_SET_TIME_TABLE,
    STATE_DB_ELS_STATUS_FLAG_SET_TIME_TABLE,
    STATE_DB_ELS_STATUS_FLAG_TABLE,
    STATE_DB_STATUS_FLAG_SET_TIME_TABLE,
    STATE_DB_TRANSCEIVER_STATUS_SW_TABLE,
    XCVR_FAULT_FIELD,
    FaultIndicationTask,
)
from sonic_platform.xcvr_fault import ELS_LANE_CODES_LENGTH


# ---------------------------------------------------------------------------
# Shared helpers and fixtures
# ---------------------------------------------------------------------------

ASIC = 'asic0'


def vmod(oe_id, els_id, asic_id=ASIC):
    """The vModule key, spelled the way FaultIndicationTask._vmodule_key does."""
    return (asic_id, oe_id, els_id)


class _FakeCpo:
    """Minimal stand-in for sonic_platform.cpo.CpoPort.

    Carries only the attributes FaultIndicationTask actually reads:
    - get_asic_id(), oe_id, els_id : vModule key
    - index                        : write-fan-out key
    - bank_id, sdk_index           : logging on read_eeprom failure
    - read_eeprom()                : direct EEPROM read for pg1A:212-219
    - get_logical_port()           : first-split map build
    - port_mapping_all_splits      : class-level, as on ModuleXcvrMixin
    """

    #: Shared by every instance, exactly as the real class member is. Reset
    #: between tests by the `task` fixture.
    port_mapping_all_splits = {}

    def __init__(self, index, oe_id, els_id, logical_port,
                 bank_id=0, sdk_index=0, eeprom_bytes=None, asic_id=ASIC):
        self.index = index
        self.oe_id = oe_id
        self.els_id = els_id
        self.bank_id = bank_id
        self.sdk_index = sdk_index
        self.asic_id = asic_id
        self._logical_port = logical_port
        self._eeprom_bytes = eeprom_bytes

    def get_asic_id(self):
        return self.asic_id

    def get_logical_port(self):
        return self._logical_port

    def read_eeprom(self, offset, num_bytes):
        return self._eeprom_bytes


def set_chassis_cpo_ports(chassis, ports):
    """Point the chassis stub's _cpo_list at `ports`.

    Returns the ports grouped by vModule key, in _cpo_list order.
    """
    chassis._cpo_list = list(ports)
    by_vmodule = {}
    for port in ports:
        if port is not None:
            by_vmodule.setdefault(vmod(port.oe_id, port.els_id, port.asic_id), []).append(port)
    return by_vmodule


@pytest.fixture
def fake_chassis():
    """Chassis stub with no CPO ports - tests populate it themselves."""
    chassis = mock.MagicMock()
    chassis._cpo_list = []
    return chassis


@pytest.fixture
def task(fake_chassis):
    """Un-started FaultIndicationTask; enough for internal-helper tests."""
    _FakeCpo.port_mapping_all_splits = {}
    yield FaultIndicationTask(fake_chassis)
    _FakeCpo.port_mapping_all_splits = {}


def _ts(y=2026, mo=7, d=8, h=15, mi=7, s=0):
    """Datetime fixture (UTC) - concise call site for many tests."""
    return datetime(y, mo, d, h, mi, s)


def _ts_str(dt):
    """asctime string matching xcvr_fault._TS_FORMAT."""
    return dt.strftime("%a %b %d %H:%M:%S %Y")


TS_1508_STR = _ts_str(_ts(mi=8))
TS_1509_STR = _ts_str(_ts(mi=9))


# ---------------------------------------------------------------------------
# Lifecycle
# ---------------------------------------------------------------------------


class TestLifecycle:

    def test_constructor_initialises_thread_and_member_state(self, fake_chassis):
        t = FaultIndicationTask(fake_chassis)

        assert t.daemon is True
        assert t.name == 'FaultIndicationTask'
        assert t._chassis is fake_chassis
        assert t._stop_event.is_set() is False
        assert t._interrupt_queue.empty()
        assert t._first_split_to_cpo_port == {}
        assert t._vmodule_to_bank_ports == {}
        assert t._set_time_watermark == {}

    def test_stop_sets_event_and_is_idempotent(self, task):
        assert task._stop_event.is_set() is False
        task.stop()
        assert task._stop_event.is_set() is True
        task.stop()
        assert task._stop_event.is_set() is True

    def test_enqueue_interrupt_stores_sfp_arrival_tuple_in_fifo_order(self, task):
        sfp_a = _FakeCpo(index=1, oe_id=0, els_id=0, logical_port='Ethernet0')
        sfp_b = _FakeCpo(index=17, oe_id=1, els_id=0, logical_port='Ethernet16')
        t1 = _ts(mi=1)
        t2 = _ts(mi=2)

        task.enqueue_interrupt(sfp_a, t1)
        task.enqueue_interrupt(sfp_b, t2)

        assert task._interrupt_queue.get_nowait() == (sfp_a, t1)
        assert task._interrupt_queue.get_nowait() == (sfp_b, t2)


# ---------------------------------------------------------------------------
# _merge_tokens: RMW union preserving first-seen order
# ---------------------------------------------------------------------------


class TestMergeTokens:

    @pytest.mark.parametrize('existing, new, expected', [
        ([],                             [],                     ''),
        ([],                             ['xcvr_cpo_a'],         'xcvr_cpo_a'),
        (['xcvr_cpo_a'],                 [],                     'xcvr_cpo_a'),
        (['xcvr_cpo_a'],                 ['xcvr_cpo_b'],         'xcvr_cpo_a|xcvr_cpo_b'),
        (['xcvr_cpo_a', 'xcvr_cpo_b'],
         ['xcvr_cpo_b', 'xcvr_cpo_c'],
         'xcvr_cpo_a|xcvr_cpo_b|xcvr_cpo_c'),
        # Empty-string tokens are filtered (defensive against a bad split).
        (['', 'xcvr_cpo_a'], ['', 'xcvr_cpo_b'], 'xcvr_cpo_a|xcvr_cpo_b'),
    ])
    def test_union_preserves_order_dedups_and_filters_empties(
            self, existing, new, expected):
        assert FaultIndicationTask._merge_tokens(existing, new) == expected


# ---------------------------------------------------------------------------
# _setup_db_connections
# ---------------------------------------------------------------------------


class TestSetupDbConnections:

    @mock.patch.object(fit_module, 'DBConnector')
    @mock.patch.object(fit_module, 'swsscommon')
    def test_opens_state_db_tables_without_holding_a_subscription(
            self, mock_swss, mock_dbconn, task):
        state_handle = mock.MagicMock(name='state_db')
        mock_dbconn.return_value = state_handle

        table_mocks = []
        def fake_table(handle, name):
            m = mock.MagicMock(name='Table(%s)' % name)
            m._handle = handle
            m._name = name
            table_mocks.append(m)
            return m
        mock_swss.Table.side_effect = fake_table

        task._setup_db_connections()

        # Only STATE_DB is opened; no APPL_DB in this design.
        assert mock_dbconn.call_count == 1
        assert mock_dbconn.call_args[0][0] == 'STATE_DB'

        # The four _SET_TIME tables plus the write target, all bound to
        # STATE_DB. ELS_STATUS_FLAG is not among them - it is reached
        # through the per-wait subscriber's own snapshot rather than an
        # HGET.
        opened_names = {t._name for t in table_mocks}
        assert opened_names == {
            STATE_DB_DOM_FLAG_SET_TIME_TABLE,
            STATE_DB_STATUS_FLAG_SET_TIME_TABLE,
            STATE_DB_ELS_DOM_FLAG_SET_TIME_TABLE,
            STATE_DB_ELS_STATUS_FLAG_SET_TIME_TABLE,
            STATE_DB_TRANSCEIVER_STATUS_SW_TABLE,
        }
        for t in table_mocks:
            assert t._handle is state_handle

        # No subscription is held outside a wait: nothing would drain it
        # while the task idles, and Redis buffers pubsub without limit.
        mock_swss.SubscriberStateTable.assert_not_called()
        mock_swss.Select.assert_not_called()

        # Task-side references wired for downstream helpers.
        assert task._state_db is state_handle
        assert [t._name for t in task._set_time_tables] == [
            STATE_DB_DOM_FLAG_SET_TIME_TABLE,
            STATE_DB_STATUS_FLAG_SET_TIME_TABLE,
            STATE_DB_ELS_DOM_FLAG_SET_TIME_TABLE,
            STATE_DB_ELS_STATUS_FLAG_SET_TIME_TABLE,
        ]
        assert task._status_sw_table._name == STATE_DB_TRANSCEIVER_STATUS_SW_TABLE


# ---------------------------------------------------------------------------
# _build_first_split_map
# ---------------------------------------------------------------------------


class TestBuildFirstSplitMap:

    def test_indexes_only_cpo_ports_with_resolved_logical_port(self, task):
        cpo_ok    = _FakeCpo(index=1, oe_id=0, els_id=0, logical_port='Ethernet0')
        cpo_no_lp = _FakeCpo(index=9, oe_id=1, els_id=0, logical_port=None)
        # _cpo_list is port-indexed, with None where no CPO port is built.
        set_chassis_cpo_ports(task._chassis, [None, cpo_ok, None, cpo_no_lp])

        task._build_first_split_map()

        assert task._first_split_to_cpo_port == {'Ethernet0': cpo_ok}
        # The vModule with no resolvable port name is left out entirely
        # rather than mapped to an empty bank list.
        assert task._vmodule_to_bank_ports == {vmod(0, 0): ['Ethernet0']}

    def test_groups_every_bank_of_a_vmodule_in_bank_order(self, task):
        # A vModule spans CpoPort.NUMBER_OF_BANKS banks, each one its own
        # CpoPort with its own dom_mgr row. _cpo_list order is not bank
        # order, so grouping must sort.
        set_chassis_cpo_ports(task._chassis, [
            _FakeCpo(index=3, oe_id=0, els_id=0, bank_id=2, logical_port='Ethernet16'),
            _FakeCpo(index=1, oe_id=0, els_id=0, bank_id=0, logical_port='Ethernet0'),
            _FakeCpo(index=4, oe_id=0, els_id=0, bank_id=3, logical_port='Ethernet24'),
            _FakeCpo(index=2, oe_id=0, els_id=0, bank_id=1, logical_port='Ethernet8'),
            _FakeCpo(index=5, oe_id=1, els_id=1, bank_id=0, logical_port='Ethernet32'),
        ])

        task._build_first_split_map()

        assert task._vmodule_to_bank_ports == {
            vmod(0, 0): ['Ethernet0', 'Ethernet8', 'Ethernet16', 'Ethernet24'],
            vmod(1, 1): ['Ethernet32'],
        }

    def test_same_oe_and_els_on_another_asic_is_another_vmodule(self, task):
        # OE and ELS ids restart per ASIC, so without the ASIC in the key
        # two ASICs' banks would merge into one vModule.
        set_chassis_cpo_ports(task._chassis, [
            _FakeCpo(index=1, oe_id=0, els_id=0, logical_port='Ethernet0', asic_id='asic0'),
            _FakeCpo(index=2, oe_id=0, els_id=0, logical_port='Ethernet512', asic_id='asic1'),
        ])

        task._build_first_split_map()

        assert task._vmodule_to_bank_ports == {
            vmod(0, 0, 'asic0'): ['Ethernet0'],
            vmod(0, 0, 'asic1'): ['Ethernet512'],
        }


# ---------------------------------------------------------------------------
# _vmodule_bank_ports  (interrupt -> every bank of the vModule)
# ---------------------------------------------------------------------------


class TestVmoduleBankPorts:

    def test_returns_every_bank_port_of_the_vmodule(self, task):
        # The interrupt fd sits on bank 0 only, but all four banks have
        # to be examined.
        sfp = _FakeCpo(index=1, oe_id=0, els_id=0, logical_port='Ethernet0')
        task._vmodule_to_bank_ports = {
            vmod(0, 0): ['Ethernet0', 'Ethernet8', 'Ethernet16', 'Ethernet24']}

        assert task._vmodule_bank_ports(sfp) == [
            'Ethernet0', 'Ethernet8', 'Ethernet16', 'Ethernet24']

    def test_unmapped_vmodule_falls_back_to_the_interrupting_port(self, task):
        # Topology not grouped yet - one bank beats dropping the item.
        sfp = _FakeCpo(index=1, oe_id=0, els_id=0, logical_port='Ethernet0')
        assert task._vmodule_bank_ports(sfp) == ['Ethernet0']

    def test_returns_empty_when_nothing_resolves(self, task):
        sfp = _FakeCpo(index=1, oe_id=0, els_id=0, logical_port=None)
        assert task._vmodule_bank_ports(sfp) == []


# ---------------------------------------------------------------------------
# _first_split_to_cpo_port_name  (reverse lookup)
# ---------------------------------------------------------------------------


class TestFirstSplitReverseLookup:

    def test_returns_port_name_for_registered_sfp(self, task):
        cpo = _FakeCpo(index=1, oe_id=0, els_id=0, logical_port='Ethernet0')
        task._first_split_to_cpo_port = {'Ethernet0': cpo}
        assert task._first_split_to_cpo_port_name(cpo) == 'Ethernet0'

    def test_map_miss_falls_back_to_sfp_and_caches_the_result(self, task):
        # The map is built when the task starts, which can precede
        # CONFIG_DB carrying a usable PORT table, so a miss must still
        # resolve rather than drop the work item.
        cpo = _FakeCpo(index=1, oe_id=0, els_id=0, logical_port='Ethernet0')
        task._first_split_to_cpo_port = {}

        assert task._first_split_to_cpo_port_name(cpo) == 'Ethernet0'
        assert task._first_split_to_cpo_port == {'Ethernet0': cpo}

    def test_returns_none_when_sfp_cannot_resolve_its_port(self, task):
        cpo = _FakeCpo(index=1, oe_id=0, els_id=0, logical_port=None)
        task._first_split_to_cpo_port = {}

        assert task._first_split_to_cpo_port_name(cpo) is None
        assert task._first_split_to_cpo_port == {}


# ---------------------------------------------------------------------------
# _read_els_lane_code_tokens  (direct EEPROM read for pg1A:212-219)
# ---------------------------------------------------------------------------


class TestReadElsLaneCodeTokens:

    def test_none_read_returns_nothing(self, task):
        sfp = _FakeCpo(index=1, oe_id=0, els_id=0,
                       logical_port='Ethernet0', eeprom_bytes=None)
        assert task._read_els_lane_code_tokens(sfp) == ([], {})

    def test_all_zero_bytes_produce_no_tokens(self, task):
        sfp = _FakeCpo(index=1, oe_id=0, els_id=0,
                       logical_port='Ethernet0',
                       eeprom_bytes=bytes(ELS_LANE_CODES_LENGTH))
        assert task._read_els_lane_code_tokens(sfp) == ([], {})

    def test_lane1_fault_and_lane2_warn_produce_expected_tokens(self, task):
        # Layout: one byte per lane, fault code in the low nibble and
        # warning code in the high nibble of that lane's own byte.
        raw = bytearray(ELS_LANE_CODES_LENGTH)
        raw[0] = 0x07              # lane 1 fault code = 7
        raw[1] = 0x50              # lane 2 warn code  = 5
        sfp = _FakeCpo(index=1, oe_id=0, els_id=0,
                       logical_port='Ethernet0',
                       eeprom_bytes=bytes(raw))

        tokens, code_by_token = task._read_els_lane_code_tokens(sfp)

        # The token names the condition; the code rides beside it so the
        # syslog line can name the value without it entering xcvr_fault.
        assert 'xcvr_cpo_els_fault_code_lane1' in tokens
        assert 'xcvr_cpo_els_warning_code_lane2' in tokens
        assert code_by_token == {
            'xcvr_cpo_els_fault_code_lane1': 7,
            'xcvr_cpo_els_warning_code_lane2': 5,
        }

    def test_calls_read_eeprom_at_correct_offset_and_length(self, task):
        sfp = mock.MagicMock()
        sfp.read_eeprom.return_value = bytes(ELS_LANE_CODES_LENGTH)
        sfp.bank_id = 0
        sfp.sdk_index = 0

        task._read_els_lane_code_tokens(sfp)

        sfp.read_eeprom.assert_called_once_with(
            ELS_LANE_CODES_EEPROM_OFFSET, ELS_LANE_CODES_LENGTH)


# ---------------------------------------------------------------------------
# _read_merged_set_times
# ---------------------------------------------------------------------------


class TestReadMergedSetTimes:

    def _wire(self, task, *returns):
        """Wire one stub per _SET_TIME table, in _setup_db_connections order:
        DOM, STATUS, ELS DOM, ELS STATUS.
        """
        task._set_time_tables = []
        for value in returns:
            table = mock.MagicMock()
            table.get.return_value = value
            task._set_time_tables.append(table)

    def test_merges_disjoint_vocabularies_across_all_four_tables(self, task):
        # One field from each table: the optical engine owns the two temp
        # groups, the laser source the two els_ ones.
        self._wire(task,
                   (True, [('tempHAlarm', TS_1508_STR)]),
                   (True, [('txfault1', TS_1508_STR)]),
                   (True, [('temperature_alarm_high', TS_1509_STR)]),
                   (True, [('fault_flag_lane1', TS_1509_STR)]))

        assert task._read_merged_set_times('Ethernet0') == {
            'tempHAlarm':                TS_1508_STR,
            'txfault1':                  TS_1508_STR,
            'temperature_alarm_high': TS_1509_STR,
            'fault_flag_lane1':      TS_1509_STR,
        }

    def test_absent_row_contributes_nothing(self, task):
        self._wire(task,
                   (False, []),
                   (False, []),
                   (False, []),
                   (True, [('fault_flag_lane1', TS_1509_STR)]))

        assert task._read_merged_set_times('Ethernet0') == {
            'fault_flag_lane1': TS_1509_STR,
        }

    def test_els_tables_are_read_for_the_same_port_key(self, task):
        # dom_mgr writes the ELS rows against every subport of the bank,
        # so the bank's first split - the key we already hold - is a
        # valid key for them too.
        self._wire(task, (False, []), (False, []), (False, []), (False, []))

        task._read_merged_set_times('Ethernet8')

        for table in task._set_time_tables:
            table.get.assert_called_once_with('Ethernet8')


# ---------------------------------------------------------------------------
# _is_covered_by
# ---------------------------------------------------------------------------


class TestIsCoveredBy:

    _ARRIVAL = _ts(mi=5)

    @pytest.mark.parametrize('last, expected', [
        (None,          False),   # HGET returned nothing
        ('',            False),   # empty string
        ('never',       False),   # NEVER sentinel
        ('garbage',     False),   # malformed - can't parse
        ('Wed Jul 08 15:04:00 2026', False),  # older than arrival
        ('Wed Jul 08 15:05:00 2026', True),   # equal to arrival (>=, not strict)
        ('Wed Jul 08 15:06:00 2026', True),   # newer than arrival
    ])
    def test_all_representations_of_last_update_time(self, last, expected):
        assert FaultIndicationTask._is_covered_by(last, self._ARRIVAL) is expected


# ---------------------------------------------------------------------------
# _report_banks_as_they_tick
# ---------------------------------------------------------------------------


class TestReportBanksAsTheyTick:
    """Each bank is reported the moment its own row ticks, never held
    back for its siblings: dom_mgr staggers ports across its poll cycle,
    so the last bank can be most of a minute behind the first.

    The subscription is created per call and released on the way out, so
    nothing accumulates in Redis' pubsub buffer while the task idles. A
    fresh SubscriberStateTable pre-loads every row, so its first drain
    doubles as the "already covered?" check.

    Every bank is then swept once more before returning, so each of
    these timelines ends with one extra read per bank.
    """

    @staticmethod
    @contextlib.contextmanager
    def _wired(task, pops, times_out=False):
        """Hand the loop a scripted subscriber and Select.

        Only the two factories are patched, never the swsscommon module
        itself: Select.TIMEOUT / Select.OBJECT are compared by identity,
        so replacing the module would make every state unrecognizable.

        Yields the recorded ``_report_one_bank`` calls alongside the
        mocks, since which banks got reported - and when - is the whole
        point of this loop.
        """
        real_select = fit_module.swsscommon.Select

        sub = mock.MagicMock(name='subscriber')
        sub.pop.side_effect = pops

        sel = mock.MagicMock(name='select')
        sel.select.return_value = (
            real_select.TIMEOUT if times_out else real_select.OBJECT, None)

        select_cls = mock.MagicMock(name='Select', return_value=sel)
        select_cls.TIMEOUT = real_select.TIMEOUT
        select_cls.OBJECT = real_select.OBJECT

        task._state_db = mock.MagicMock(name='state_db')
        with mock.patch.object(fit_module.swsscommon, 'SubscriberStateTable',
                               return_value=sub) as sub_cls, \
             mock.patch.object(fit_module.swsscommon, 'Select', select_cls), \
             mock.patch.object(task, '_report_one_bank') as m_report:
            yield sub_cls, sub, sel, m_report

    @staticmethod
    def _reported(m_report):
        return [c.args[0] for c in m_report.call_args_list]

    @staticmethod
    def _sweep(*bank_ports):
        """The trailing pass over every bank, in bank order.

        It turns up nothing new in these tests - the watermark makes a
        re-read a no-op - but it still shows up as a call, so the
        expected timelines have to account for it.
        """
        return list(bank_ports)

    def test_preloaded_snapshot_reports_without_entering_the_select_loop(self, task):
        # The snapshot already carries a covering tick, so the bank is
        # reported straight away and we never block.
        pops = [
            ('Ethernet0', 'SET', [(LAST_UPDATE_TIME_FIELD, _ts_str(_ts(mi=6)))]),
            ('', '', []),                                   # snapshot done
        ]
        with self._wired(task, pops) as (sub_cls, sub, sel, m_report):
            task._report_banks_as_they_tick(['Ethernet0'], _ts(mi=5))
            assert self._reported(m_report) == (
                ['Ethernet0'] + self._sweep('Ethernet0'))
            sel.select.assert_not_called()
            # ELS_STATUS_FLAG, not STATUS_FLAG: it is the last table each
            # dom pass writes, so its tick is the one that says all four
            # _SET_TIME tables are refreshed for this port.
            sub_cls.assert_called_once_with(
                task._state_db, STATE_DB_ELS_STATUS_FLAG_TABLE)
            # Released on the way out even though nothing blocked.
            sel.removeSelectable.assert_called_once_with(sub)

    def test_each_bank_reports_on_its_own_tick_not_the_slowest_one(self, task):
        # Bank 0 is covered in the snapshot while bank 1 is still a tick
        # behind. Bank 0 must not wait for bank 1: on the switch they
        # tick ~23 s apart, so batching would cost a whole extra minute.
        pops = [
            ('Ethernet0', 'SET', [(LAST_UPDATE_TIME_FIELD, _ts_str(_ts(mi=6)))]),
            ('Ethernet8', 'SET', [(LAST_UPDATE_TIME_FIELD, _ts_str(_ts(mi=4)))]),
            ('', '', []),                                   # snapshot done
            ('Ethernet8', 'SET', [(LAST_UPDATE_TIME_FIELD, _ts_str(_ts(mi=6)))]),
            ('', '', []),
        ]
        with self._wired(task, pops) as (sub_cls, sub, sel, m_report):
            task._report_banks_as_they_tick(
                ['Ethernet0', 'Ethernet8'], _ts(mi=5))
            # Bank 0 before the first select call, bank 1 only after it.
            assert self._reported(m_report) == (
                ['Ethernet0', 'Ethernet8']
                + self._sweep('Ethernet0', 'Ethernet8'))
            assert sel.select.call_count == 1

    def test_repeated_rows_for_a_covered_bank_do_not_re_report(self, task):
        # dom_mgr keeps ticking every minute. Once a bank is covered it
        # leaves `pending`, so later rows for it are ignored rather than
        # re-diffed against a watermark that already absorbed them. Each
        # bank is therefore read twice - its own tick and the sweep -
        # not once per row dom_mgr wrote.
        pops = [
            ('Ethernet0', 'SET', [(LAST_UPDATE_TIME_FIELD, _ts_str(_ts(mi=6)))]),
            ('Ethernet8', 'SET', [(LAST_UPDATE_TIME_FIELD, _ts_str(_ts(mi=4)))]),
            ('', '', []),
            ('Ethernet0', 'SET', [(LAST_UPDATE_TIME_FIELD, _ts_str(_ts(mi=7)))]),
            ('Ethernet8', 'SET', [(LAST_UPDATE_TIME_FIELD, _ts_str(_ts(mi=7)))]),
            ('', '', []),
        ]
        with self._wired(task, pops) as (sub_cls, sub, sel, m_report):
            task._report_banks_as_they_tick(
                ['Ethernet0', 'Ethernet8'], _ts(mi=5))
            assert self._reported(m_report) == (
                ['Ethernet0', 'Ethernet8']
                + self._sweep('Ethernet0', 'Ethernet8'))

    def test_final_sweep_re_reads_a_bank_that_already_left_pending(self, task):
        # The regression this guards: a bank that ticks first leaves
        # `pending` early. dom_mgr can still stamp a latch against it later
        # in the same cycle, and without the sweep nothing would read it again.
        pops = [
            ('Ethernet0', 'SET', [(LAST_UPDATE_TIME_FIELD, _ts_str(_ts(mi=6)))]),
            ('Ethernet8', 'SET', [(LAST_UPDATE_TIME_FIELD, _ts_str(_ts(mi=4)))]),
            ('', '', []),
            ('Ethernet8', 'SET', [(LAST_UPDATE_TIME_FIELD, _ts_str(_ts(mi=6)))]),
            ('', '', []),
        ]
        with self._wired(task, pops) as (sub_cls, sub, sel, m_report):
            task._report_banks_as_they_tick(
                ['Ethernet0', 'Ethernet8'], _ts(mi=5))

        reported = self._reported(m_report)
        # Covered by the snapshot, so it left `pending` before Ethernet8
        # had even ticked - and is still read a second time.
        assert reported.count('Ethernet0') == 2
        assert reported[-2:] == ['Ethernet0', 'Ethernet8']

    def test_bank_with_no_row_at_all_is_dropped_rather_than_waited_on(self, task):
        # dom_mgr is publishing - Ethernet0 has a row - but Ethernet8 has
        # none. Waiting on it would stall the single-threaded main loop
        # forever, taking every other vModule's interrupts down with it.
        # The sweep still gives it a last read, which costs nothing when
        # it has no _SET_TIME rows either.
        pops = [
            ('Ethernet0', 'SET', [(LAST_UPDATE_TIME_FIELD, _ts_str(_ts(mi=6)))]),
            ('', '', []),
        ]
        with self._wired(task, pops) as (sub_cls, sub, sel, m_report):
            task._report_banks_as_they_tick(
                ['Ethernet0', 'Ethernet8'], _ts(mi=5))
            assert self._reported(m_report) == (
                ['Ethernet0'] + self._sweep('Ethernet0', 'Ethernet8'))
            sel.select.assert_not_called()

    def test_stop_event_aborts_the_loop_cleanly(self, task):
        # Snapshot is stale, select only times out, then stop() fires.
        original_is_set = task._stop_event.is_set
        calls = {'n': 0}
        def is_set_side_effect():
            calls['n'] += 1
            return calls['n'] > 1     # False on first check, True after
        task._stop_event.is_set = is_set_side_effect
        try:
            with self._wired(task, [('', '', [])] * 20,
                             times_out=True) as (sub_cls, sub, sel, m_report):
                task._report_banks_as_they_tick(['Ethernet0'], _ts(mi=5))
                # The sweep is gated on the stop event too, so a
                # shutdown does not turn into one last round of reads.
                assert self._reported(m_report) == []
                assert sel.select.call_count >= 1
                sel.removeSelectable.assert_called_once_with(sub)
        finally:
            task._stop_event.is_set = original_is_set

    def test_event_for_matching_port_triggers_the_report(self, task):
        # Snapshot drains empty - dom_mgr has not ticked yet, which is
        # worth waiting for - then a select fire brings two rows: one for
        # another port (skipped) and one covering our arrival.
        pops = [
            ('', '', []),                                   # empty snapshot
            ('Ethernet16', 'SET', [(LAST_UPDATE_TIME_FIELD, _ts_str(_ts(mi=6)))]),
            ('Ethernet0',  'SET', [(LAST_UPDATE_TIME_FIELD, _ts_str(_ts(mi=6)))]),
            ('', '', []),
        ]
        with self._wired(task, pops) as (sub_cls, sub, sel, m_report):
            task._report_banks_as_they_tick(['Ethernet0'], _ts(mi=5))
            assert self._reported(m_report) == (
                ['Ethernet0'] + self._sweep('Ethernet0'))
            assert sel.select.call_count == 1

    def test_stale_tick_for_our_port_does_not_trigger_the_report(self, task):
        # dom_mgr published, but its tick predates our arrival, so it
        # cannot have observed this fault. Keep waiting.
        pops = [
            ('Ethernet0', 'SET', [(LAST_UPDATE_TIME_FIELD, _ts_str(_ts(mi=4)))]),
            ('', '', []),                                   # snapshot done
            ('Ethernet0', 'SET', [(LAST_UPDATE_TIME_FIELD, _ts_str(_ts(mi=6)))]),
            ('', '', []),
        ]
        with self._wired(task, pops) as (sub_cls, sub, sel, m_report):
            task._report_banks_as_they_tick(['Ethernet0'], _ts(mi=5))
            assert self._reported(m_report) == (
                ['Ethernet0'] + self._sweep('Ethernet0'))
            assert sel.select.call_count == 1


# ---------------------------------------------------------------------------
# _merge_same_vmodule_arrivals
# ---------------------------------------------------------------------------


class TestMergeSameVmoduleArrivals:

    def _make(self, oe, els, logical_port='Ethernet0'):
        return _FakeCpo(index=1, oe_id=oe, els_id=els, logical_port=logical_port)

    def test_no_followups_returns_head_arrival(self, task):
        head_sfp = self._make(0, 0)
        assert task._merge_same_vmodule_arrivals(head_sfp, _ts(mi=5)) == _ts(mi=5)

    def test_same_vmodule_followups_collapse_to_min_arrival(self, task):
        head_sfp = self._make(0, 0)
        # Head arrival = 15:05. Follow-ups at 15:07 and 15:03.
        task._interrupt_queue.put((self._make(0, 0), _ts(mi=7)))
        task._interrupt_queue.put((self._make(0, 0), _ts(mi=3)))

        assert task._merge_same_vmodule_arrivals(head_sfp, _ts(mi=5)) == _ts(mi=3)
        assert task._interrupt_queue.empty()

    def test_different_vmodule_followups_are_requeued_preserving_order(self, task):
        head_sfp = self._make(0, 0)
        other_a  = self._make(1, 0)
        other_b  = self._make(2, 0)
        task._interrupt_queue.put((other_a, _ts(mi=6)))
        task._interrupt_queue.put((self._make(0, 0), _ts(mi=7)))  # same-vmod, consumed
        task._interrupt_queue.put((other_b, _ts(mi=8)))

        earliest = task._merge_same_vmodule_arrivals(head_sfp, _ts(mi=5))

        # Same-vmodule follow-up was consumed and did not lower arrival
        # (min(5, 7) = 5).
        assert earliest == _ts(mi=5)
        # Different-vmodule items were requeued in FIFO order.
        got = [task._interrupt_queue.get_nowait(),
               task._interrupt_queue.get_nowait()]
        assert got == [(other_a, _ts(mi=6)), (other_b, _ts(mi=8))]


# ---------------------------------------------------------------------------
# _process_one
# ---------------------------------------------------------------------------


class TestProcessOne:

    def _prime(self, task, ports=('Ethernet0',)):
        """Wire the minimal state needed by _process_one.

        ``ports`` is one first-split port name per bank, in bank order.
        Returns the bank-0 CpoPort - the one carrying the interrupt fd,
        and so the one Chassis enqueues.
        """
        splits = {}
        for bank_id, port in enumerate(ports):
            task._first_split_to_cpo_port[port] = _FakeCpo(
                index=bank_id + 1, oe_id=0, els_id=0, bank_id=bank_id,
                logical_port=port)
            splits[bank_id + 1] = [port]
        _FakeCpo.port_mapping_all_splits = splits
        task._vmodule_to_bank_ports[vmod(0, 0)] = list(ports)
        task._status_sw_table = mock.MagicMock()
        task._status_sw_table.get.return_value = (False, [])   # no prior tokens
        return task._first_split_to_cpo_port[ports[0]]

    def test_unresolvable_port_logs_and_returns_without_side_effects(
            self, task):
        # Not in the map and the SFP can't resolve its own port either,
        # so there is no row to read or write - nothing we can do.
        sfp = _FakeCpo(index=1, oe_id=0, els_id=0, logical_port=None)
        with mock.patch.object(task, '_read_els_lane_code_tokens') as m_eeprom, \
             mock.patch.object(task, '_report_banks_as_they_tick') as m_report, \
             mock.patch.object(task, '_emit') as m_emit:
            task._process_one(sfp, _ts(mi=5))

        m_eeprom.assert_not_called()
        m_report.assert_not_called()
        m_emit.assert_not_called()

    def test_eeprom_tokens_go_out_to_every_bank_before_any_tick_wait(
            self, task):
        # Page 0x1A is neither clear-on-read nor dom_mgr's to publish, so
        # it must not inherit dom_mgr's up-to-a-minute latency. It is
        # ELS-level, so one read serves every bank of the vModule.
        cpo = self._prime(task,
                          ports=('Ethernet0', 'Ethernet8'))
        order = []
        codes = {'xcvr_cpo_els_fault_code_lane1': 7}
        with mock.patch.object(task, '_read_els_lane_code_tokens',
                               return_value=(list(codes), codes)), \
             mock.patch.object(task, '_report_banks_as_they_tick',
                               side_effect=lambda *a: order.append('wait')), \
             mock.patch.object(task, '_emit',
                               side_effect=lambda p, t, c=None: order.append(p)):
            task._process_one(cpo, _ts(mi=5))

        assert order == ['Ethernet0', 'Ethernet8', 'wait']

    def test_eeprom_code_values_reach_emit_for_syslog(
            self, task):
        # The value is deliberately absent from the token, so it has to
        # travel separately or the operator never learns what code fired.
        cpo = self._prime(task)
        codes = {'xcvr_cpo_els_fault_code_lane1': 9}
        with mock.patch.object(task, '_read_els_lane_code_tokens',
                               return_value=(list(codes), codes)), \
             mock.patch.object(task, '_report_banks_as_they_tick'), \
             mock.patch.object(task, '_emit') as m_emit:
            task._process_one(cpo, _ts(mi=5))

        m_emit.assert_called_once_with(
            'Ethernet0', ['xcvr_cpo_els_fault_code_lane1'], codes)

    def test_no_eeprom_tokens_means_no_write_before_the_wait(
            self, task):
        cpo = self._prime(task)
        with mock.patch.object(task, '_read_els_lane_code_tokens',
                               return_value=([], {})), \
             mock.patch.object(task, '_report_banks_as_they_tick'), \
             mock.patch.object(task, '_emit') as m_emit:
            task._process_one(cpo, _ts(mi=5))

        m_emit.assert_not_called()

    def test_per_bank_reporting_is_handed_the_whole_vmodule(
            self, task):
        cpo = self._prime(task,
                          ports=('Ethernet0', 'Ethernet8'))
        with mock.patch.object(task, '_read_els_lane_code_tokens',
                               return_value=([], {})), \
             mock.patch.object(task, '_report_banks_as_they_tick') as m_report:
            task._process_one(cpo, _ts(mi=5))

        m_report.assert_called_once_with(
            ['Ethernet0', 'Ethernet8'], _ts(mi=5))

    def test_the_codes_read_here_are_recorded_for_the_tick_driven_pass(
            self, task):
        # The flags dom_mgr publishes a minute later name the same
        # lasers these codes do, so the pass that reads them needs to
        # know what has already gone out.
        cpo = self._prime(task)
        codes = {'xcvr_cpo_els_warning_code_lane1': 1}
        with mock.patch.object(task, '_read_els_lane_code_tokens',
                               return_value=(list(codes), codes)), \
             mock.patch.object(task, '_report_banks_as_they_tick'), \
             mock.patch.object(task, '_emit'):
            task._process_one(cpo, _ts(mi=5))

        assert task._codes_reported == {'xcvr_cpo_els_warning_code_lane1'}

    def test_a_quiet_vmodule_clears_the_previous_items_codes(self, task):
        # _codes_reported outlives one work item, so a stale entry would
        # silence a real flag on the next, unrelated interrupt.
        cpo = self._prime(task)
        task._codes_reported = {'xcvr_cpo_els_warning_code_lane1'}
        with mock.patch.object(task, '_read_els_lane_code_tokens',
                               return_value=([], {})), \
             mock.patch.object(task, '_report_banks_as_they_tick'), \
             mock.patch.object(task, '_emit'):
            task._process_one(cpo, _ts(mi=5))

        assert task._codes_reported == set()


# ---------------------------------------------------------------------------
# _report_one_bank  (diff one bank's _SET_TIME row and scope the fan-out)
# ---------------------------------------------------------------------------


class TestReportOneBank:
    """Reporting is driven by the _SET_TIME watermark - has this latch's
    timestamp moved since we last looked - bounded by an age window that
    keeps history from being pinned on whatever fires next.
    """

    _BANKS = ['Ethernet0', 'Ethernet8']

    @staticmethod
    def _with_hardware(task, port='Ethernet0'):
        """Give `port` a CpoPort, so the cause re-read can reach EEPROM."""
        task._first_split_to_cpo_port[port] = _FakeCpo(
            index=1, oe_id=0, els_id=0, logical_port=port)

    def test_parser_is_fed_the_port_row_watermark_and_arrival(self, task):
        with mock.patch.object(task, '_read_merged_set_times',
                               return_value={'tempHAlarm': TS_1508_STR}), \
             mock.patch.object(fit_module, 'parse_set_time_rows',
                               return_value=(
                                   ['xcvr_cpo_module_case_temp_high_alarm'],
                                   {'tempHAlarm': TS_1508_STR})) as m_parse, \
             mock.patch.object(task, '_emit') as m_emit:
            task._report_one_bank('Ethernet0', self._BANKS, _ts(mi=5))

        port, set_times, watermark, arrival = m_parse.call_args[0]
        assert port == 'Ethernet0'
        assert set_times == {'tempHAlarm': TS_1508_STR}
        assert watermark is task._set_time_watermark['Ethernet0']
        assert arrival == _ts(mi=5)
        # Advances are applied to the watermark by the caller.
        assert task._set_time_watermark['Ethernet0'] == {'tempHAlarm': TS_1508_STR}
        m_emit.assert_called_once_with(
            'Ethernet0', ['xcvr_cpo_module_case_temp_high_alarm'])

    def test_fault_stamped_before_arrival_is_still_reported(self, task):
        # Possible race: dom_mgr polls EEPROMs on its own schedule
        # and stamped this latch 10 s before chassis noticed the
        # interrupt. Nothing synchronizes the two, so selection must not
        # depend on that ordering.
        with mock.patch.object(task, '_read_merged_set_times',
                               return_value={'optical_power_alarm_high_lane1': TS_1508_STR}), \
             mock.patch.object(task, '_emit') as m_emit:
            task._report_one_bank('Ethernet0', ['Ethernet0'], _ts(mi=8, s=10))

        m_emit.assert_called_once_with(
            'Ethernet0', ['xcvr_cpo_els_high_power_alarm_lane1'])

    def test_latch_far_older_than_arrival_is_absorbed_silently(self, task):
        # A latch whose own interrupt we never processed must not be
        # pinned on whatever fault fires next, hours later. It is still
        # recorded so it stops being re-examined.
        with mock.patch.object(task, '_read_merged_set_times',
                               return_value={'optical_power_alarm_high_lane1': TS_1508_STR}), \
             mock.patch.object(task, '_emit') as m_emit:
            task._report_one_bank('Ethernet0', ['Ethernet0'], _ts(h=23, mi=59))

        m_emit.assert_not_called()
        assert task._set_time_watermark['Ethernet0'] == {
            'optical_power_alarm_high_lane1': TS_1508_STR}

    def test_unchanged_set_time_on_a_later_interrupt_reports_nothing(self, task):
        # The watermark contract: one latch is one report, however many
        # interrupts follow it. The clear-on-read cycle that follows
        # every latch touches _CLEAR_TIME only, so it cannot resurface
        # the fault here either.
        with mock.patch.object(task, '_read_merged_set_times',
                               return_value={'tempHAlarm': TS_1508_STR}), \
             mock.patch.object(task, '_emit') as m_emit:
            task._report_one_bank('Ethernet0', ['Ethernet0'], _ts(mi=5))
            task._report_one_bank('Ethernet0', ['Ethernet0'], _ts(mi=6))

        m_emit.assert_called_once_with(
            'Ethernet0', ['xcvr_cpo_module_case_temp_high_alarm'])
        assert task._set_time_watermark['Ethernet0'] == {'tempHAlarm': TS_1508_STR}

    def test_relatched_fault_reports_again(self, task):
        # A second latch of the same field carries a later _SET_TIME, so
        # it must surface as a fresh fault rather than being absorbed.
        with mock.patch.object(task, '_read_merged_set_times',
                               side_effect=[{'tempHAlarm': TS_1508_STR},
                                            {'tempHAlarm': TS_1509_STR}]), \
             mock.patch.object(task, '_emit') as m_emit:
            task._report_one_bank('Ethernet0', ['Ethernet0'], _ts(mi=5))
            task._report_one_bank('Ethernet0', ['Ethernet0'], _ts(mi=6))

        assert m_emit.call_count == 2
        assert task._set_time_watermark['Ethernet0'] == {'tempHAlarm': TS_1509_STR}

    def test_each_bank_keeps_its_own_watermark(self, task):
        # Every bank has its own dom_mgr row, so a latch on bank 1 must
        # not be credited to - or absorbed by - bank 0's bookkeeping.
        with mock.patch.object(task, '_read_merged_set_times',
                               return_value={'tempHAlarm': TS_1508_STR}), \
             mock.patch.object(task, '_emit') as m_emit:
            task._report_one_bank('Ethernet8', self._BANKS, _ts(mi=5))

        m_emit.assert_called_once_with(
            'Ethernet8', ['xcvr_cpo_module_case_temp_high_alarm'])
        assert task._set_time_watermark['Ethernet8'] == {'tempHAlarm': TS_1508_STR}
        assert 'Ethernet0' not in task._set_time_watermark

    def test_els_tokens_widen_to_the_vmodule_while_bank_tokens_stay_put(self, task):
        # All banks share one ELS but each has its own OE bank, and
        # dom_mgr records the ELS fields against the bank-0 row alone -
        # so only this fan-out puts them on the other banks' ports.
        with mock.patch.object(task, '_read_merged_set_times',
                               return_value={'tempHAlarm': TS_1508_STR,
                                             'optical_power_alarm_high_lane1': TS_1508_STR}), \
             mock.patch.object(task, '_emit') as m_emit:
            task._report_one_bank('Ethernet0', self._BANKS, _ts(mi=5))

        assert m_emit.call_args_list == [
            mock.call('Ethernet0', ['xcvr_cpo_module_case_temp_high_alarm']),
            mock.call('Ethernet0', ['xcvr_cpo_els_high_power_alarm_lane1']),
            mock.call('Ethernet8', ['xcvr_cpo_els_high_power_alarm_lane1']),
        ]

    def test_quiet_row_skips_emit_entirely(self, task):
        with mock.patch.object(task, '_read_merged_set_times', return_value={}), \
             mock.patch.object(task, '_emit') as m_emit:
            task._report_one_bank('Ethernet0', self._BANKS, _ts(mi=5))

        m_emit.assert_not_called()

    def test_a_lane_flag_whose_cause_was_already_reported_stays_quiet(self, task):
        # One APC warning reaches us twice: as byte 212's warning code,
        # read straight off the EEPROM and emitted on arrival, and a
        # minute later as byte 174's bit naming the same laser. The
        # second is the same fault, so the row here must fall silent
        # rather than add a second token to xcvr_fault.
        task._codes_reported = {'xcvr_cpo_els_warning_code_lane1'}
        with mock.patch.object(task, '_read_merged_set_times',
                               return_value={'warning_flag_lane1': TS_1508_STR}), \
             mock.patch.object(task, '_emit') as m_emit:
            task._report_one_bank('Ethernet0', self._BANKS, _ts(mi=5))

        m_emit.assert_not_called()
        # Still recorded, so the dropped flag is not re-examined forever.
        assert task._set_time_watermark['Ethernet0'] == {
            'warning_flag_lane1': TS_1508_STR}

    def test_a_lane_flag_with_no_cause_behind_it_is_reported(self, task):
        # Nothing came back from 212-219 this cycle, so the flag is the
        # only account of the fault there is.
        with mock.patch.object(task, '_read_merged_set_times',
                               return_value={'warning_flag_lane1': TS_1508_STR}), \
             mock.patch.object(task, '_emit') as m_emit:
            task._report_one_bank('Ethernet0', self._BANKS, _ts(mi=5))

        assert m_emit.call_args_list == [
            mock.call('Ethernet0', ['xcvr_cpo_els_warn_lane1']),
            mock.call('Ethernet8', ['xcvr_cpo_els_warn_lane1']),
        ]

    def test_a_flag_arriving_before_its_cause_triggers_a_re_read(self, task):
        # Two APC warnings 3 s apart put
        # lane 2's flag on this row while only lane 1's code had been
        # read, and xcvr_fault ended up holding warn_lane2 beside both
        # codes. The code is not clear-on-read, so looking again finds
        # it; _process_one's closing re-read is what publishes it.
        self._with_hardware(task)
        task._codes_reported = {'xcvr_cpo_els_warning_code_lane1'}
        with mock.patch.object(task, '_read_merged_set_times',
                               return_value={'warning_flag_lane2': TS_1508_STR}), \
             mock.patch.object(
                 task, '_read_els_lane_code_tokens',
                 return_value=(['xcvr_cpo_els_warning_code_lane1',
                                'xcvr_cpo_els_warning_code_lane2'], {})) as m_read, \
             mock.patch.object(task, '_emit') as m_emit:
            task._report_one_bank('Ethernet0', self._BANKS, _ts(mi=5))

        m_read.assert_called_once()
        m_emit.assert_not_called()
        assert 'xcvr_cpo_els_warning_code_lane2' in task._codes_reported

    def test_no_re_read_when_every_flag_already_has_its_cause(self, task):
        # The re-read is an EEPROM access on dom_mgr's tick; it earns
        # its place only when a flag turns up unaccounted for.
        self._with_hardware(task)
        task._codes_reported = {'xcvr_cpo_els_warning_code_lane1'}
        with mock.patch.object(task, '_read_merged_set_times',
                               return_value={'warning_flag_lane1': TS_1508_STR}), \
             mock.patch.object(task, '_read_els_lane_code_tokens') as m_read, \
             mock.patch.object(task, '_emit'):
            task._report_one_bank('Ethernet0', self._BANKS, _ts(mi=5))

        m_read.assert_not_called()

    def test_no_re_read_for_a_row_of_power_tokens(self, task):
        # Power faults have no cause code to go looking for.
        self._with_hardware(task)
        with mock.patch.object(
                task, '_read_merged_set_times',
                return_value={'optical_power_alarm_high_lane1': TS_1508_STR}), \
             mock.patch.object(task, '_read_els_lane_code_tokens') as m_read, \
             mock.patch.object(task, '_emit'):
            task._report_one_bank('Ethernet0', self._BANKS, _ts(mi=5))

        m_read.assert_not_called()

    def test_a_flag_the_re_read_cannot_explain_is_still_reported(self, task):
        # Looked again and the lane is holding no code, so the flag is
        # the only account of the fault - report it rather than lose it.
        self._with_hardware(task)
        with mock.patch.object(task, '_read_merged_set_times',
                               return_value={'warning_flag_lane1': TS_1508_STR}), \
             mock.patch.object(task, '_read_els_lane_code_tokens',
                               return_value=([], {})), \
             mock.patch.object(task, '_emit') as m_emit:
            task._report_one_bank('Ethernet0', self._BANKS, _ts(mi=5))

        assert m_emit.call_args_list == [
            mock.call('Ethernet0', ['xcvr_cpo_els_warn_lane1']),
            mock.call('Ethernet8', ['xcvr_cpo_els_warn_lane1']),
        ]

    def test_a_reported_cause_does_not_mute_the_rest_of_the_row(self, task):
        # Only the flag naming the laser the code already covered is
        # dropped; a power alarm on another lane has no code at all and
        # has to survive.
        task._codes_reported = {'xcvr_cpo_els_warning_code_lane1'}
        with mock.patch.object(task, '_read_merged_set_times',
                               return_value={'warning_flag_lane1': TS_1508_STR,
                                             'optical_power_alarm_high_lane2': TS_1508_STR}), \
             mock.patch.object(task, '_emit') as m_emit:
            task._report_one_bank('Ethernet0', self._BANKS, _ts(mi=5))

        assert m_emit.call_args_list == [
            mock.call('Ethernet0', ['xcvr_cpo_els_high_power_alarm_lane2']),
            mock.call('Ethernet8', ['xcvr_cpo_els_high_power_alarm_lane2']),
        ]


# ---------------------------------------------------------------------------
# _emit  (syslog per token + fan-out)
# ---------------------------------------------------------------------------


class TestEmit:

    def test_delegates_to_the_banks_own_cpo_port_without_logging_warnings(
            self, task):
        # The port name identifies the bank, so _emit resolves the
        # CpoPort itself rather than reusing the interrupting one.
        cpo = _FakeCpo(index=2, oe_id=0, els_id=0, bank_id=1,
                       logical_port='Ethernet8')
        task._first_split_to_cpo_port['Ethernet8'] = cpo

        with mock.patch.object(fit_module.logger, 'log_warning') as m_warn, \
             mock.patch.object(task, '_fan_out_xcvr_fault') as m_fan:
            task._emit('Ethernet8',
                       ['xcvr_cpo_a', 'xcvr_cpo_b', 'xcvr_cpo_a'])   # duplicate

        # WARNINGs are emitted by _fan_out_xcvr_fault (one per split
        # per token), so _emit itself must not log any.
        m_warn.assert_not_called()
        # Fan-out is called once with the original (un-deduped) list -
        # _merge_tokens handles dedup on the write side.
        m_fan.assert_called_once_with(
            cpo, ['xcvr_cpo_a', 'xcvr_cpo_b', 'xcvr_cpo_a'], {})

    def test_unknown_port_warns_and_skips_the_write(self, task):
        with mock.patch.object(fit_module.logger, 'log_warning') as m_warn, \
             mock.patch.object(task, '_fan_out_xcvr_fault') as m_fan:
            task._emit('Ethernet8', ['xcvr_cpo_a'])

        m_fan.assert_not_called()
        m_warn.assert_called_once()


# ---------------------------------------------------------------------------
# _fan_out_xcvr_fault
# ---------------------------------------------------------------------------


class TestFanOutXcvrFault:

    @mock.patch.object(fit_module, 'swsscommon')
    def test_writes_rmw_union_and_logs_warning_per_split_per_token(
            self, mock_swss, task):
        _FakeCpo.port_mapping_all_splits = {
            1: ['Ethernet0', 'Ethernet4'],
        }
        cpo = _FakeCpo(index=1, oe_id=0, els_id=0, logical_port='Ethernet0')

        def get_row(logical_port):
            if logical_port == 'Ethernet0':
                return (True, [('xcvr_fault', 'xcvr_cpo_a')])
            return (True, [('xcvr_fault', 'xcvr_cpo_b')])
        task._status_sw_table = mock.MagicMock()
        task._status_sw_table.get.side_effect = get_row

        with mock.patch.object(fit_module.logger, 'log_warning') as m_warn:
            task._fan_out_xcvr_fault(
                cpo, ['xcvr_cpo_c', 'xcvr_cpo_d', 'xcvr_cpo_c'])  # duplicate

        # 2 splits x 2 unique tokens = 4 WARNING lines total,
        # one per (split, token) pair.
        logged = [c[0][0] for c in m_warn.call_args_list]
        assert 'CPO fault: Ethernet0 xcvr_cpo_c' in logged
        assert 'CPO fault: Ethernet0 xcvr_cpo_d' in logged
        assert 'CPO fault: Ethernet4 xcvr_cpo_c' in logged
        assert 'CPO fault: Ethernet4 xcvr_cpo_d' in logged
        assert m_warn.call_count == 4

        assert task._status_sw_table.set.call_count == 2
        # Assert the union values written per split.
        fvp_calls = mock_swss.FieldValuePairs.call_args_list
        values = [c[0][0][0][1] for c in fvp_calls]
        assert 'xcvr_cpo_a|xcvr_cpo_c|xcvr_cpo_d' in values
        assert 'xcvr_cpo_b|xcvr_cpo_c|xcvr_cpo_d' in values
        # Field is always xcvr_fault.
        for c in fvp_calls:
            assert c[0][0][0][0] == XCVR_FAULT_FIELD

    @mock.patch.object(fit_module, 'swsscommon')
    def test_lane_code_value_and_meaning_appear_only_in_syslog(
            self, mock_swss, task):
        # The value cannot live in the token (xcvr_fault is a cumulative
        # set), so syslog is the only place an operator can learn that
        # the lane reported code 9 - and that 9 means vendor specific.
        _FakeCpo.port_mapping_all_splits = {
            1: ['Ethernet0'],
        }
        cpo = _FakeCpo(index=1, oe_id=0, els_id=0, logical_port='Ethernet0')
        task._status_sw_table = mock.MagicMock()
        task._status_sw_table.get.return_value = (False, [])

        with mock.patch.object(fit_module.logger, 'log_warning') as m_warn:
            task._fan_out_xcvr_fault(
                cpo,
                ['xcvr_cpo_els_fault_code_lane1', 'xcvr_cpo_tempHAlarm'],
                {'xcvr_cpo_els_fault_code_lane1': 9})

        logged = [c[0][0] for c in m_warn.call_args_list]
        assert ('CPO fault: Ethernet0 xcvr_cpo_els_fault_code_lane1 '
                '(code 9, vendor specific)') in logged
        # A boolean token has no value, so it gets no suffix.
        assert 'CPO fault: Ethernet0 xcvr_cpo_tempHAlarm' in logged
        # And the stored field carries the bare tokens either way.
        written = mock_swss.FieldValuePairs.call_args[0][0][0][1]
        assert written == 'xcvr_cpo_els_fault_code_lane1|xcvr_cpo_tempHAlarm'

    def test_missing_topology_logs_warning_and_skips_writes(
            self, task):
        _FakeCpo.port_mapping_all_splits = {}
        cpo = _FakeCpo(index=42, oe_id=0, els_id=0, logical_port='Ethernet0')
        task._status_sw_table = mock.MagicMock()

        task._fan_out_xcvr_fault(cpo, ['xcvr_cpo_a'])

        task._status_sw_table.set.assert_not_called()


# ---------------------------------------------------------------------------
# _read_existing_tokens  (RMW read side)
# ---------------------------------------------------------------------------


class TestReadExistingTokens:

    @pytest.mark.parametrize('table_return, expected', [
        ((False, []),                                             []),
        ((True,  [('status', 'OK')]),                             []),
        ((True,  [('xcvr_fault', '')]),                           []),
        ((True,  [('xcvr_fault', 'xcvr_cpo_a')]),                 ['xcvr_cpo_a']),
        ((True,  [('xcvr_fault', 'xcvr_cpo_a|xcvr_cpo_b')]),
         ['xcvr_cpo_a', 'xcvr_cpo_b']),
    ])
    def test_all_representations_of_stored_tokens(
            self, task, table_return, expected):
        task._status_sw_table = mock.MagicMock()
        task._status_sw_table.get.return_value = table_return

        assert task._read_existing_tokens('Ethernet0') == expected


# ---------------------------------------------------------------------------
# The interrupt-independent sweep
# ---------------------------------------------------------------------------


class TestSweep:
    """An interrupt can be dropped - two faults on one vModule arrive as two
    assertions of a single clear-on-read node, and the poll separately
    discards one it reads back as zero. _process_one is the only thing that
    reports, so without the sweep the fault is never published at all.
    """

    _BANKS = ['Ethernet0', 'Ethernet8']

    def _vmodule(self, task):
        """One vModule of two banks, wired into both topology maps."""
        for port in self._BANKS:
            task._first_split_to_cpo_port[port] = _FakeCpo(
                index=1, oe_id=0, els_id=0, logical_port=port)
        task._vmodule_to_bank_ports[vmod(0, 0)] = list(self._BANKS)

    def test_a_lane_code_missing_from_state_db_is_published(self, task):
        self._vmodule(task)
        token = 'xcvr_cpo_els_warning_code_lane1'

        with mock.patch.object(task, '_read_els_lane_code_tokens',
                               return_value=([token], {token: 1})), \
             mock.patch.object(task, '_read_existing_tokens', return_value=[]), \
             mock.patch.object(task, '_report_one_bank', return_value=[]), \
             mock.patch.object(task, '_emit') as m_emit:
            task._sweep_if_due()

        # ELS-scoped, so it reaches every bank of the vModule.
        assert m_emit.call_args_list == [
            mock.call(port, [token], {token: 1}) for port in self._BANKS]

    def test_a_lane_code_already_reported_stays_quiet(self, task):
        # Costs one EEPROM read per interval and nothing else: the RMW union
        # would swallow a repeated write, but the WARNING line would not.
        self._vmodule(task)
        token = 'xcvr_cpo_els_warning_code_lane1'

        with mock.patch.object(task, '_read_els_lane_code_tokens',
                               return_value=([token], {token: 1})), \
             mock.patch.object(task, '_read_existing_tokens',
                               return_value=[token]), \
             mock.patch.object(task, '_report_one_bank', return_value=[]), \
             mock.patch.object(task, '_emit') as m_emit:
            task._sweep_if_due()

        m_emit.assert_not_called()

    def test_a_dom_mgr_flag_whose_interrupt_was_lost_is_recovered(self, task):
        # The laser faulted and dom_mgr stamped the latch nine
        # seconds later, but the interrupt never reached _process_one, so
        # nothing read the stamp and xcvr_fault stayed empty for the whole
        # 120 s the test waited.
        self._vmodule(task)

        with mock.patch.object(task, '_read_els_lane_code_tokens',
                               return_value=([], {})), \
             mock.patch.object(task, '_read_merged_set_times',
                               return_value={'optical_power_alarm_high_lane1':
                                             TS_1508_STR}), \
             mock.patch.object(fit_module, 'datetime') as m_dt, \
             mock.patch.object(task, '_emit') as m_emit:
            m_dt.utcnow.return_value = _ts(mi=8, s=30)
            task._sweep_if_due()

        for port in self._BANKS:
            assert mock.call(
                port, ['xcvr_cpo_els_high_power_alarm_lane1']
            ) in m_emit.call_args_list

    def test_a_latch_older_than_the_age_window_is_not_resurrected(self, task):
        # What keeps the sweep from re-reporting history every interval.
        self._vmodule(task)

        with mock.patch.object(task, '_read_els_lane_code_tokens',
                               return_value=([], {})), \
             mock.patch.object(task, '_read_merged_set_times',
                               return_value={'optical_power_alarm_high_lane1':
                                             TS_1508_STR}), \
             mock.patch.object(fit_module, 'datetime') as m_dt, \
             mock.patch.object(task, '_emit') as m_emit:
            m_dt.utcnow.return_value = _ts(h=23, mi=59)
            task._sweep_if_due()

        m_emit.assert_not_called()

    def test_the_interval_is_shorter_than_the_age_window(self):
        # Otherwise a latch could be stamped and expire between two passes.
        from sonic_platform.xcvr_fault import MAX_LATCH_AGE_S
        assert fit_module.SWEEP_INTERVAL_S < MAX_LATCH_AGE_S

    def test_a_second_pass_waits_out_the_interval(self, task):
        self._vmodule(task)

        with mock.patch.object(task, '_sweep_one_vmodule') as m_one:
            task._sweep_if_due()
            task._sweep_if_due()

        assert m_one.call_count == 1

    def test_a_map_that_came_up_empty_is_rebuilt(self, task, fake_chassis):
        # The task now starts when the interrupt fds are registered, which
        # can precede CONFIG_DB carrying a usable PORT table - run() would
        # then have built nothing and the sweep would cover nothing for the
        # life of the process.
        assert task._vmodule_to_bank_ports == {}
        set_chassis_cpo_ports(fake_chassis, [
            _FakeCpo(index=1, oe_id=0, els_id=0, logical_port='Ethernet0')])

        with mock.patch.object(task, '_sweep_one_vmodule'):
            task._sweep_if_due()

        assert task._vmodule_to_bank_ports == {vmod(0, 0): ['Ethernet0']}

    def test_one_failing_vmodule_does_not_hide_the_rest(self, task):
        # A bad EEPROM read on one ELS would otherwise swallow every fault
        # behind it in the iteration order.
        task._vmodule_to_bank_ports = {
            vmod(0, 0): ['Ethernet0'],
            vmod(1, 0): ['Ethernet8'],
        }

        with mock.patch.object(task, '_sweep_one_vmodule',
                               side_effect=[RuntimeError('i2c'), None]) as m_one:
            task._sweep_if_due()

        assert m_one.call_count == 2

    def test_an_idle_main_loop_is_what_drives_it(self, task):
        # The sweep has no scheduler; it rides the queue's idle branch.
        task._stop_event.set()
        with mock.patch.object(task, '_sweep_if_due') as m_sweep:
            task._main_loop()
        # Stopped before the first wait, so nothing ran - the point is that
        # _main_loop owns the call site, asserted by the loop below.
        m_sweep.assert_not_called()

        task._stop_event.clear()

        def stop_after_first(*_a, **_kw):
            task._stop_event.set()
        with mock.patch.object(task, '_sweep_if_due',
                               side_effect=stop_after_first) as m_sweep:
            task._main_loop()
        m_sweep.assert_called_once_with()
