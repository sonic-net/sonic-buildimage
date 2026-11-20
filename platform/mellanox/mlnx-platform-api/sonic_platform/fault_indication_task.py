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

"""FaultIndicationTask - Mellanox-only CPO fault-indication thread.

Started by Chassis.get_change_event() as soon as the CPO interrupt nodes
are registered, and runs as a daemon=True thread for the rest of the
xcvrd process' lifetime.

One interrupt fd covers a whole vModule but dom_mgr publishes one row
per bank, so every work item is resolved against all of its banks.

For each (sfp, arrival_time) work item enqueued by Chassis:

  1. Read pg1A:212-219 directly from EEPROM via ``sfp.read_eeprom`` and
     tokenize with ``parse_els_lane_codes``. These bytes are not
     clear-on-read and need no dom_mgr tick, so they are emitted
     immediately, to every port of the vModule - page 0x1A belongs to
     the shared ELS.

  2. Subscribe to ``TRANSCEIVER_ELS_STATUS_FLAG`` and report each bank as
     soon as *its own* ``last_update_time`` advances past
     ``arrival_time`` (dom_mgr staggers ports across its poll cycle
     instead of writing them in one burst, so waiting for the last bank
     would hold the first bank's fault for a whole extra minute).

  3. Per bank, merge the four ``_SET_TIME`` tables at that bank's
     first-split port and hand them to ``parse_set_time_rows``, then
     drop the per-lane flags naming a laser whose cause code step 1
     already reported.

  4. Log one WARNING line per token, then RMW-union the tokens into
     ``TRANSCEIVER_STATUS_SW.xcvr_fault``.

Interrupts are not assumed to be reliable. Every ``SWEEP_INTERVAL_S`` an idle
main loop reconciles both fault sources of every vModule against STATE_DB and
publishes whatever is missing, which is the only thing that reports a fault
whose interrupt was coalesced away or read back as zero.
"""

try:
    import queue
    import threading
    import time
    from datetime import datetime

    from swsscommon import swsscommon
    from swsscommon.swsscommon import DBConnector
    from sonic_py_common import multi_asic
    from sonic_py_common.logger import Logger

    from sonic_platform.xcvr_fault import (
        ELS_LANE_CODES_LENGTH,
        ELS_SCOPED_TOKENS,
        _is_never,
        _parse_set_time,
        describe_lane_code,
        drop_locators_with_known_cause,
        has_locator_missing_a_cause,
        parse_els_lane_codes,
        parse_set_time_rows,
    )
except ImportError as e:
    raise ImportError(str(e) + " - required module not found")


logger = Logger()


# ---------------------------------------------------------------------------
# STATE_DB tables and fields
# ---------------------------------------------------------------------------

# dom_mgr's per-tick heartbeat, subscribed for the duration of each
# _report_banks_as_they_tick call. Last table it writes in a pass, so a row
# stamped after an interrupt means every _SET_TIME table below is fresh.
STATE_DB_ELS_STATUS_FLAG_TABLE = "TRANSCEIVER_ELS_STATUS_FLAG"


STATE_DB_DOM_FLAG_SET_TIME_TABLE        = "TRANSCEIVER_DOM_FLAG_SET_TIME"
STATE_DB_STATUS_FLAG_SET_TIME_TABLE     = "TRANSCEIVER_STATUS_FLAG_SET_TIME"
STATE_DB_ELS_DOM_FLAG_SET_TIME_TABLE    = "TRANSCEIVER_ELS_DOM_FLAG_SET_TIME"
STATE_DB_ELS_STATUS_FLAG_SET_TIME_TABLE = "TRANSCEIVER_ELS_STATUS_FLAG_SET_TIME"

# RMW target for xcvr_fault writes.
STATE_DB_TRANSCEIVER_STATUS_SW_TABLE = "TRANSCEIVER_STATUS_SW"

# Field on the flag table rows carrying dom_mgr's per-tick heartbeat
# (advances every ~60 s per port).
LAST_UPDATE_TIME_FIELD = "last_update_time"

# The single field on TRANSCEIVER_STATUS_SW that carries our tokens.
XCVR_FAULT_FIELD = "xcvr_fault"


# ---------------------------------------------------------------------------
# EEPROM location of pg1A:212-219 (per-lane ELS fault/warning codes)
# ---------------------------------------------------------------------------
#
# Mellanox SFP flat-offset convention (see sfp._get_page_and_page_offset):
#   page 0        -> flat [0, 256)
#   page N (>=1)  -> flat [256 + (N-1)*128, 256 + N*128)
#   bank routing  -> bank = flat // CMIS_BYTES_PER_BANK (= flat // 32768)

ELS_LANE_CODES_EEPROM_OFFSET = 256 + (0x1A - 1) * 128 + (212 - 128)  # 3540

# ---------------------------------------------------------------------------
# Loop timing
# ---------------------------------------------------------------------------

# Caps how long the blocking waits (queue.get, Select.select) sleep, so
# the thread notices stop() instead of blocking forever.
POLL_TIMEOUT_MS = 1000
POLL_TIMEOUT_S  = POLL_TIMEOUT_MS / 1000.0

# When the thread has nothing to do, re-check every vModule, in case we missed
# an interrupt. Keep under MAX_LATCH_AGE_S, or a fault gets too old to report.
SWEEP_INTERVAL_S = 60


def _code_suffix(code_by_token, token):
    """Render the " (code 9, vendor specific)" tail of a syslog line.

    Returns "" for tokens that carry no code, so callers can append it
    unconditionally.
    """
    code = (code_by_token or {}).get(token)
    if code is None:
        return ""
    return " (code %d, %s)" % (code, describe_lane_code(code))


class FaultIndicationTask(threading.Thread):
    """Mellanox-only thread that handles CPO fault interrupts end-to-end.

    Life-cycle:
      - Constructed by Chassis once the CPO interrupt fds are registered,
        rather than on the first interrupt, so the sweep is running before
        any fault can be missed.
      - ``start()`` is called once - Python's threading.Thread launches
        ``run()`` in a new thread.
      - ``stop()`` sets the stop event; ``run()`` observes it within one
        ``POLL_TIMEOUT_MS`` bound and exits cleanly.
      - Because ``daemon=True``, the Python runtime reaps the thread if
        xcvrd exits without ``stop()`` being called.
    """

    def __init__(self, chassis):
        """
        Args:
            chassis: the Mellanox `Chassis` instance. Used at run-time
                to enumerate CpoPorts (for the first-split map and the
                write fan-out).
        """
        super().__init__(daemon=True, name="FaultIndicationTask")

        self._chassis = chassis

        # Signal from stop() to run(). threading.Event is thread-safe.
        self._stop_event = threading.Event()

        # Interrupt fan-in from Chassis.get_change_event(): one item
        # per interrupt observation, holding a ``(sfp, arrival_time)``
        # tuple. queue.Queue is thread-safe.
        self._interrupt_queue = queue.Queue()

        # Map: first-split logical port name
        # (e.g. "Ethernet0") -> owning CpoPort instance.
        self._first_split_to_cpo_port = {}

        # Map: vModule -> first-split port name of each of
        # its banks, in bank order. One interrupt covers the whole vModule but
        # dom_mgr publishes a row per bank, so this turns the interrupt into
        # the set of rows to examine. {} until run() builds it.
        self._vmodule_to_bank_ports = {}

        # Latch timestamp already accounted for, per port:
        # {logical_port: {field_name: set_time_string}}. Starts empty and
        # fills as _process_one examines fields, so a field is reported
        # at most once per latch.
        self._set_time_watermark = {}

        # Lane code tokens emitted for the work item being processed, so
        # the dom_mgr-driven pass can tell which of its per-lane flags
        # are a second account of a fault already reported. Reset by
        # _process_one; only ever touched from the main loop thread.
        self._codes_reported = set()

        # Monotonic stamp of the last lane-code sweep. Zero so the first
        # idle moment sweeps instead of waiting out a whole interval.
        self._last_sweep = 0.0

    # -----------------------------------------------------------------
    # Public entry points (called from the Chassis thread)
    # -----------------------------------------------------------------

    def stop(self):
        """Request a graceful shutdown.

        Idempotent. ``run()`` observes the stop event within
        ``POLL_TIMEOUT_MS`` and returns. Safe to call from any thread.
        """
        self._stop_event.set()

    def enqueue_interrupt(self, cpo_port, arrival_time):
        """Enqueue one CPO interrupt observation for processing.

        Called from Chassis.get_change_event() on every interrupt
        observation. Non-blocking. If run() is not yet in its main loop
        the item just sits in the queue until it is.

        Args:
            cpo_port: the CpoPort whose ``interrupt`` sysfs was observed
                asserted - one per vModule, the one with the smallest
                bank_id.
            arrival_time: ``datetime.datetime`` (UTC) captured by
                Chassis at the exact instant it observed
                ``interrupt == 1``.
        """
        self._interrupt_queue.put((cpo_port, arrival_time))

    def run(self):
        """Thread entry point.

        Sets up STATE_DB handles and builds the first-split map from
        ``chassis._cpo_list``, then loops over work items until
        ``stop()`` is signalled.
        """
        logger.log_notice("FaultIndicationTask: starting")
        try:
            self._setup_db_connections()
            self._build_first_split_map()
        except Exception as e:
            logger.log_error("FaultIndicationTask: setup failed: %s" % e)
            return

        self._main_loop()

        logger.log_notice("FaultIndicationTask: stopped")

    # -----------------------------------------------------------------
    # Setup
    # -----------------------------------------------------------------

    def _setup_db_connections(self):
        """Open the STATE_DB handle and the tables read per work item.

        Called once at the start of ``run()``. Connections are created
        here (not in ``__init__``) so this thread owns them -
        swsscommon connections are single-threaded.
        """
        # The only CPO platform today is single-ASIC, where the default
        # namespace is the only one. A multi-ASIC CPO platform would have
        # to take each port's own namespace instead.
        ns = multi_asic.DEFAULT_NAMESPACE

        self._state_db = DBConnector("STATE_DB", 0, True, ns)

        self._set_time_tables = [
            swsscommon.Table(self._state_db, name)
            for name in (STATE_DB_DOM_FLAG_SET_TIME_TABLE,
                         STATE_DB_STATUS_FLAG_SET_TIME_TABLE,
                         STATE_DB_ELS_DOM_FLAG_SET_TIME_TABLE,
                         STATE_DB_ELS_STATUS_FLAG_SET_TIME_TABLE)
        ]

        self._status_sw_table = swsscommon.Table(
            self._state_db, STATE_DB_TRANSCEIVER_STATUS_SW_TABLE)

    def _build_first_split_map(self):
        """Populate the two topology maps from the chassis CPO ports.

        dom_mgr keys every row by a bank's first-split port name, so
        that name is what both maps are built around:

          _first_split_to_cpo_port  row key   -> the CpoPort behind it
          _vmodule_to_bank_ports    vModule   -> row keys of its banks

        The first gets us from a row back to the hardware, the second
        turns one interrupt into the set of rows to examine.
        """
        # {vModule: {bank_id: row key}}. Scratch space only: the banks
        # cannot be put in order until every port has been seen.
        banks_by_vmodule = {}

        for cpo_port in self._chassis._cpo_list:
            if cpo_port is None:
                continue
            # There is one CpoPort per bank, and its first split is the
            # key dom_mgr files that bank's rows under.
            logical_port = cpo_port.get_logical_port()
            if logical_port is None:
                continue
            self._first_split_to_cpo_port[logical_port] = cpo_port
            banks_by_vmodule.setdefault(
                self._vmodule_key(cpo_port), {})[cpo_port.bank_id] = logical_port

        for vmodule, ports_by_bank in banks_by_vmodule.items():
            self._vmodule_to_bank_ports[vmodule] = [
                ports_by_bank[bank_id] for bank_id in sorted(ports_by_bank)]

        logger.log_debug(
            "FaultIndicationTask: first-split map has %d entries across "
            "%d vModules" % (len(self._first_split_to_cpo_port),
                             len(self._vmodule_to_bank_ports)))

    @staticmethod
    def _vmodule_key(cpo_port):
        """The vModule a CPO port belongs to: (asic, OE, laser source)."""
        return (cpo_port.get_asic_id(), cpo_port.oe_id, cpo_port.els_id)

    # -----------------------------------------------------------------
    # Main loop
    # -----------------------------------------------------------------

    def _main_loop(self):
        """Pop one work item at a time; fold in any pending arrivals for
        the same vModule; process each item to completion.

        Idle time doubles as the sweep's slot, so a fault whose interrupt
        was lost is still reported instead of going unnoticed.
        """
        while not self._stop_event.is_set():
            try:
                sfp, arrival = self._interrupt_queue.get(timeout=POLL_TIMEOUT_S)
            except queue.Empty:
                self._sweep_if_due()
                continue

            earliest_arrival = self._merge_same_vmodule_arrivals(sfp, arrival)

            try:
                self._process_one(sfp, earliest_arrival)
            except Exception as e:
                # Isolate per-item failures so one bad work item can't stall the whole main loop.
                logger.log_error(
                    "FaultIndicationTask: failed to process (oe=%r, els=%r): %s"
                    % (getattr(sfp, 'oe_id', None),
                       getattr(sfp, 'els_id', None), e))

    def _merge_same_vmodule_arrivals(self, head_sfp, head_arrival):
        """Fold pending queue items for the same vModule into the head;
        requeue everything else.

        Same-vModule follow-ups collapse into the head with ``min(arrival)``
        - widest observation window. Different-vModule items are requeued in
        the order they were popped, so they keep FIFO fairness.

        Args:
            head_sfp: CpoPort popped as the head of this cycle.
            head_arrival: its arrival timestamp.

        Returns:
            ``datetime.datetime`` - the earliest arrival for this
            vModule this cycle (equals ``head_arrival`` if no
            same-vModule follow-ups were queued).
        """
        earliest_arrival = head_arrival
        head_vmodule = self._vmodule_key(head_sfp)
        others = []
        while True:
            try:
                other_sfp, other_arrival = self._interrupt_queue.get_nowait()
            except queue.Empty:
                break
            if self._vmodule_key(other_sfp) == head_vmodule:
                if other_arrival < earliest_arrival:
                    earliest_arrival = other_arrival
            else:
                others.append((other_sfp, other_arrival))

        for item in others:
            self._interrupt_queue.put(item)

        return earliest_arrival

    # -----------------------------------------------------------------
    # Per-item processing
    # -----------------------------------------------------------------

    def _process_one(self, sfp, arrival):
        """Handle one merged work item end-to-end.

        The interrupt is raised per vModule, so every bank of it is
        examined, each one as soon as dom_mgr publishes its row.

        pg1A:212-219 (non-COR per-lane ELS codes) is read directly from
        EEPROM and emitted straight away, then read once more at the end
        for the codes a coalesced second fault raised in the meantime.
        The rest of the fault vocabulary requires dom_mgr's next tick to
        publish fresh ``_SET_TIME`` metadata.
        """
        bank_ports = self._vmodule_bank_ports(sfp)
        if not bank_ports:
            logger.log_warning(
                "FaultIndicationTask: no logical ports for CpoPort "
                "(oe=%r, els=%r); dropping work item" %
                (sfp.oe_id, sfp.els_id))
            return

        # Direct EEPROM read: not clear-on-read and independent of
        # dom_mgr, so it reports at sub-second latency. Page 0x1A belongs
        # to the ELS every bank shares, hence the whole-vModule fan-out.
        eeprom_tokens, code_by_token = self._read_els_lane_code_tokens(sfp)
        self._codes_reported = set(eeprom_tokens)
        if eeprom_tokens:
            for logical_port in bank_ports:
                self._emit(logical_port, eeprom_tokens, code_by_token)

        self._report_banks_as_they_tick(bank_ports, arrival)

        # Two faults on one vModule arrive as two interrupts and merge into a
        # single work item, so a laser that raised its code after the read
        # above would otherwise never be reported at all. The codes are not
        # clear-on-read, so the laser is still holding it and a second look
        # costs one EEPROM read; only what the first look missed is emitted,
        # to keep the WARNING lines one-per-fault.
        late_tokens, late_codes = self._read_els_lane_code_tokens(sfp)
        missed = [token for token in late_tokens if token not in set(eeprom_tokens)]
        if missed:
            for logical_port in bank_ports:
                self._emit(logical_port, missed, late_codes)

    def _vmodule_bank_ports(self, sfp):
        """First-split port name of every bank of ``sfp``'s vModule.

        Falls back to just this CpoPort's own port if the vModule is
        absent from the map, which keeps a partially-resolved topology
        usable instead of dropping the work item outright.
        """
        bank_ports = self._vmodule_to_bank_ports.get(self._vmodule_key(sfp))
        if bank_ports:
            return bank_ports

        logical_port = self._first_split_to_cpo_port_name(sfp)
        return [logical_port] if logical_port else []

    def _first_split_to_cpo_port_name(self, sfp):
        """Reverse-lookup: CpoPort instance -> first-split port name.

        The first-split map is keyed by port name; here we walk it to
        find the entry whose CpoPort matches ``sfp``. One entry per bank,
        so a few hundred at most.

        The map is built when the task starts, which can precede
        CONFIG_DB carrying a usable PORT table, so a miss falls back to
        resolving the name from the SFP itself and records it for next
        time.
        """
        for name, cpo in self._first_split_to_cpo_port.items():
            if cpo is sfp:
                return name

        logical_port = sfp.get_logical_port()
        if logical_port is not None:
            self._first_split_to_cpo_port[logical_port] = sfp
        return logical_port

    def _read_els_lane_code_tokens(self, sfp):
        """Read pg1A:212-219 from EEPROM and tokenize.

        Returns:
            (tokens, code_by_token) - the tokens for every non-zero
            per-lane fault/warning code, plus the code value behind
            each so ``_emit`` can name it in syslog. Empty on failed
            read or all-zero content.
        """
        eeprom_bytes = sfp.read_eeprom(
            ELS_LANE_CODES_EEPROM_OFFSET, ELS_LANE_CODES_LENGTH)
        if eeprom_bytes is None:
            logger.log_warning(
                "FaultIndicationTask: read_eeprom returned None for "
                "sdk_index=%r (bank=%r); skipping ELS lane codes" %
                (getattr(sfp, 'sdk_index', None),
                 getattr(sfp, 'bank_id', None)))
            return [], {}
        code_by_token = dict(parse_els_lane_codes(eeprom_bytes))
        return list(code_by_token), code_by_token

    # -----------------------------------------------------------------
    # Interrupt-independent sweep
    # -----------------------------------------------------------------

    def _sweep_if_due(self):
        """Reconcile every vModule against STATE_DB once per interval.

        Rate-limited rather than scheduled, because the only caller is the
        queue's idle branch and that fires every POLL_TIMEOUT_S.

        One vModule failing is not allowed to skip the rest, so each is
        isolated - a bad EEPROM read on one ELS would otherwise hide every
        fault behind it in the iteration order.
        """
        now = time.monotonic()
        if now - self._last_sweep < SWEEP_INTERVAL_S:
            return
        self._last_sweep = now

        # The task now starts as soon as the interrupt fds are registered,
        # which can be before chassis._cpo_list carries logical port names -
        # run() would then have built an empty map and the sweep would cover
        # nothing for the life of the process. Retry until it takes.
        if not self._vmodule_to_bank_ports:
            self._build_first_split_map()

        for bank_ports in self._vmodule_to_bank_ports.values():
            if self._stop_event.is_set():
                return
            try:
                self._sweep_one_vmodule(bank_ports)
            except Exception as e:
                logger.log_error(
                    "FaultIndicationTask: sweep failed for %s: %s"
                    % (bank_ports[0] if bank_ports else None, e))

    def _sweep_one_vmodule(self, bank_ports):
        """Reconcile one vModule's two fault sources against STATE_DB.

        Both of them, because a lost interrupt costs the whole work item
        and the two halves of _process_one fail together. They need
        different treatment though: the lane codes live in the EEPROM and
        are diffed against what was published, while the dom_mgr flags are
        already in STATE_DB and are diffed against the watermark.
        """
        if not bank_ports:
            return
        lead_port = bank_ports[0]
        cpo_port = self._first_split_to_cpo_port.get(lead_port)
        if cpo_port is None:
            return

        arrival = datetime.utcnow()

        tokens, code_by_token = self._read_els_lane_code_tokens(cpo_port)
        # Doubles as the cause set _report_one_bank below consults when it
        # decides which locator flags are a second account of a known fault.
        self._codes_reported = set(tokens)
        if tokens:
            self._sweep_lane_codes(bank_ports, tokens, code_by_token)

        # The same per-bank diff the interrupt path runs, with now as the
        # arrival. Its two guards are what make it safe on a timer: the
        # watermark drops anything already reported, and parse_set_time_rows
        # ignores latches older than MAX_LATCH_AGE_S, so this recovers a
        # missed latch without resurrecting historical ones. The sweep
        # interval is kept under that age so no latch falls between passes.
        for logical_port in bank_ports:
            recovered = self._report_one_bank(logical_port, bank_ports, arrival)
            if recovered:
                logger.log_warning(
                    "FaultIndicationTask: %s had unreported flags %s; "
                    "recovering a lost interrupt"
                    % (logical_port, recovered))

    def _sweep_lane_codes(self, bank_ports, tokens, code_by_token):
        """Publish any lane code this vModule holds that STATE_DB lacks.

        Only the missing codes are emitted, so a vModule whose faults were
        reported normally produces no syslog. The RMW union in
        _fan_out_xcvr_fault would already swallow a repeated write, but the
        WARNING line per token would not.

        Only the lead port is checked for what is already reported. These
        are ELS-scoped codes, always written to all four banks at once, so
        the banks normally hold the same set. A bank that lost its copy on
        its own is not noticed here.
        """
        lead_port = bank_ports[0]
        reported = set(self._read_existing_tokens(lead_port))
        missing = [token for token in tokens if token not in reported]
        if not missing:
            return

        logger.log_warning(
            "FaultIndicationTask: %s holds lane codes %s that were never "
            "reported; recovering a lost interrupt" % (lead_port, missing))
        for logical_port in bank_ports:
            self._emit(logical_port, missing, code_by_token)

    def _read_merged_set_times(self, logical_port):
        """Merge the four ``_SET_TIME`` tables into one
        ``{field: set_time_string}`` dict.

        No field name appears in more than one of the four tables, so no
        table can overwrite a timestamp another one already merged in.
        """
        merged = {}
        for table in self._set_time_tables:
            ok, fvp = table.get(logical_port)
            if not ok:
                continue
            merged.update(dict(fvp))
        return merged

    # -----------------------------------------------------------------
    # Per-bank reporting, driven by dom_mgr's ticks
    # -----------------------------------------------------------------

    def _report_banks_as_they_tick(self, bank_ports, arrival):
        """Report each bank as soon as dom_mgr refreshes it.

        A bank is ready once its ELS_STATUS_FLAG row is stamped after
        ``arrival``. dom_mgr writes that table last, so a fresh row there
        means all four ``_SET_TIME`` tables are current for the port.

        A bank with no row at all is skipped rather than waited for, so
        one missing row cannot block the loop. No rows at all instead
        means dom_mgr has not run yet, which is worth waiting for.

        The subscriber is opened and closed here: an idle one would pile
        up unread messages in Redis. A new one starts with every row
        loaded, which is also how we tell which banks are covered.
        """
        ours    = set(bank_ports)
        pending = set(bank_ports)
        seen    = set()

        subscriber = swsscommon.SubscriberStateTable(self._state_db, STATE_DB_ELS_STATUS_FLAG_TABLE)
        select = swsscommon.Select()
        select.addSelectable(subscriber)
        try:
            covered = self._drain_covered_banks(
                subscriber, ours, pending, seen, arrival)
            self._report_covered_banks(bank_ports, covered, arrival)

            unpublished = pending - seen
            if unpublished and seen:
                logger.log_warning(
                    "FaultIndicationTask: no %s row for %s; not waiting on it"
                    % (STATE_DB_ELS_STATUS_FLAG_TABLE, sorted(unpublished)))
                pending -= unpublished

            while pending and not self._stop_event.is_set():
                state, _ = select.select(POLL_TIMEOUT_MS)
                if state == swsscommon.Select.TIMEOUT:
                    continue
                if state != swsscommon.Select.OBJECT:
                    logger.log_warning(
                        "FaultIndicationTask: unexpected Select state %r"
                        % state)
                    continue
                covered = self._drain_covered_banks(
                    subscriber, ours, pending, seen, arrival)
                self._report_covered_banks(bank_ports, covered, arrival)

            # A bank leaves `pending` on its first covering tick, so a
            # latch dom_mgr records after that is never re-read. That is
            # the common case rather than a corner one: the ELS fields
            # land on the bank-0 row alone, and bank 0 is typically the
            # first to tick. Sweep every bank once more before leaving;
            # the watermark makes a re-read with nothing new a no-op.
            if not self._stop_event.is_set():
                self._report_covered_banks(bank_ports, ours, arrival)
        finally:
            select.removeSelectable(subscriber)

    def _drain_covered_banks(self, subscriber, ours, pending, seen, arrival):
        """Read every queued row; return the banks that are now ready.

        A bank whose row is stamped after ``arrival`` is ready and leaves
        ``pending``. Any of our banks that has a row at all joins
        ``seen``, so the caller can tell "no row yet" from "no row ever".
        Both sets are changed in place.

        Every port writes to this table, so most rows are not ours.
        """
        covered = set()
        while True:
            key, op, fvp = subscriber.pop()
            if not key:
                return covered
            if op != "SET" or key not in ours:
                continue
            seen.add(key)
            last = dict(fvp).get(LAST_UPDATE_TIME_FIELD)
            if key in pending and self._is_covered_by(last, arrival):
                pending.discard(key)
                covered.add(key)

    def _report_covered_banks(self, bank_ports, covered, arrival):
        """Report each newly covered bank, in bank order for readable logs."""
        for logical_port in bank_ports:
            if logical_port in covered:
                self._report_one_bank(logical_port, bank_ports, arrival)

    def _report_one_bank(self, logical_port, bank_ports, arrival):
        """Diff one bank's ``_SET_TIME`` row and emit what it turned up.

        Bank-scoped tokens go to this bank's own breakout splits.
        ELS-scoped ones widen to every port of the vModule: the banks
        share a single ELS, and dom_mgr records its fields against the
        bank-0 row alone, so nothing else would surface them elsewhere.

        The watermark advances over every field the row turned up, even
        the ones dropped as a duplicate account - a flag held back here
        is reported, just under its cause code's name.

        Returns the tokens emitted, so the sweep can tell a recovery from
        an ordinary quiet pass.
        """
        merged_set_times = self._read_merged_set_times(logical_port)
        watermark = self._set_time_watermark.setdefault(logical_port, {})
        tokens, advanced = parse_set_time_rows(
            logical_port, merged_set_times, watermark, arrival)
        watermark.update(advanced)

        self._catch_up_on_causes(logical_port, tokens)
        tokens = drop_locators_with_known_cause(tokens, self._codes_reported)
        if not tokens:
            return []

        els_tokens = [t for t in tokens if t in ELS_SCOPED_TOKENS]
        bank_tokens = [t for t in tokens if t not in ELS_SCOPED_TOKENS]

        if bank_tokens:
            self._emit(logical_port, bank_tokens)
        if els_tokens:
            for port in bank_ports:
                self._emit(port, els_tokens)
        return tokens

    def _catch_up_on_causes(self, logical_port, tokens):
        """Re-read the lane codes when a flag arrives with no cause yet.

        A fault raised after _process_one read the EEPROM still reaches
        this row a tick later, and its cause would be unknown here -
        leaving the flag to go out as a token of its own, and the code
        to follow it from the closing re-read. One fault, two tokens.

        The codes are not clear-on-read, so the laser is still holding
        the one we missed. Only the lookup happens here; publishing it
        stays with the closing re-read in _process_one, which already
        emits whatever the first read did not.
        """
        if not has_locator_missing_a_cause(tokens, self._codes_reported):
            return
        cpo_port = self._first_split_to_cpo_port.get(logical_port)
        if cpo_port is None:
            return
        late_tokens, _ = self._read_els_lane_code_tokens(cpo_port)
        self._codes_reported.update(late_tokens)

    @staticmethod
    def _is_covered_by(last_update_time_str, arrival):
        """True iff ``last_update_time_str`` parses to a datetime >= arrival.

        Returns False for NEVER / None / empty / malformed input, so
        callers can safely feed HGETALL results without a try/except.
        """
        if _is_never(last_update_time_str):
            return False
        try:
            return _parse_set_time(last_update_time_str) >= arrival
        except (TypeError, ValueError):
            return False

    # -----------------------------------------------------------------
    # Emission (syslog + STATE_DB fan-out)
    # -----------------------------------------------------------------

    def _emit(self, logical_port, tokens, code_by_token=None):
        """Fan out the token set to every breakout split of
        ``logical_port``'s bank.

        WARNING syslog is emitted per-split by ``_fan_out_xcvr_fault``
        so every logical port sharing this bank gets both a Redis write
        and its own WARNING line.

        ``code_by_token`` maps a lane-code token to its numeric value
        (e.g. 9) for the syslog line to quote. Tokens that carry no
        value are simply not in the map.
        """
        cpo_port = self._first_split_to_cpo_port.get(logical_port)
        if cpo_port is None:
            logger.log_warning(
                "FaultIndicationTask: no CpoPort for %s; xcvr_fault write "
                "skipped" % logical_port)
            return

        self._fan_out_xcvr_fault(cpo_port, tokens, code_by_token or {})

    def _fan_out_xcvr_fault(self, cpo_port, new_tokens, code_by_token=None):
        """Read-Modify-Write ``new_tokens`` into
        ``TRANSCEIVER_STATUS_SW.xcvr_fault`` for every breakout split
        of ``cpo_port``'s physical port, and log one WARNING line per
        token per split.

        Every logical port sharing this physical index receives the
        same merged token set and WARNING syslog lines.
        """
        every_split = cpo_port.port_mapping_all_splits.get(cpo_port.index, [])
        if not every_split:
            logger.log_warning(
                "FaultIndicationTask: no logical ports registered for "
                "phys index %d; xcvr_fault write skipped" % cpo_port.index)
            return

        unique_new_tokens = list(dict.fromkeys(t for t in new_tokens if t))
        for logical_port in every_split:
            existing = self._read_existing_tokens(logical_port)
            merged   = self._merge_tokens(existing, new_tokens)
            for token in unique_new_tokens:
                logger.log_warning(
                    "CPO fault: %s %s%s"
                    % (logical_port, token, _code_suffix(code_by_token, token)))
            self._status_sw_table.set(logical_port,
                swsscommon.FieldValuePairs([(XCVR_FAULT_FIELD, merged)]))

    def _read_existing_tokens(self, logical_port):
        """Return the existing xcvr_fault tokens on TRANSCEIVER_STATUS_SW
        for ``logical_port`` as a list, or [] if the field is absent.
        """
        ok, fvp = self._status_sw_table.get(logical_port)
        if not ok:
            return []
        value = dict(fvp).get(XCVR_FAULT_FIELD, "")
        if not value:
            return []
        return value.split("|")

    @staticmethod
    def _merge_tokens(existing, new_tokens):
        """Union of ``existing`` and ``new_tokens`` preserving
        first-seen order, rendered as the ``|``-joined string stored
        in ``xcvr_fault``.
        """
        seen = set()
        out  = []
        for token in list(existing) + list(new_tokens):
            if token and token not in seen:
                seen.add(token)
                out.append(token)
        return "|".join(out)
