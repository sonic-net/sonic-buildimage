"""
sonic_alarm.event_subscriber -- eventd event-subscription input adapter.

This is the primary (forward-looking) input adapter described in HLD §7.3.1.
It subscribes to eventd's ZMQ proxy via ``events_init_subscriber()``, receives
events published by other daemons, evaluates the ``event``-type checks in the
alarm definitions against each event payload, and drives ``EventPublisher``
(RAISE/CLEAR) with the same OR-logic aggregation ``StateDBPoller`` uses for
STATE_DB.

Status: there are **no in-tree event producers yet** -- the standard pmon
daemons (psud, thermalctld, sensormond, pcied) publish STATE_DB, not
``sonic-events-*``.  So this adapter is inert unless a platform declares
``event`` checks (and/or an ``event_source_mapping``); until then the STATE_DB
polling adapter carries the corresponding checks.  It is implemented so checks
can migrate from the polling adapter with **no change to alarmd's output**.
See HLD §7.3.1 and §15 item 5.

swsscommon is imported lazily/defensively (mirroring ``event_publisher``) so
this module imports, unit-tests (with a fake subscriber API), and runs on a
host without the library.
"""

import logging

from logging.handlers import SysLogHandler
from sonic_py_common.syslogger import SysLogger

from sonic_alarm.statedb_poller import evaluate_condition


logger = SysLogger(
    log_identifier='alarmd#subscriber',
    log_facility=SysLogHandler.LOG_DAEMON,
    log_level=logging.INFO,
    enable_runtime_config=False,
)


# ---------------------------------------------------------------------------
# Subscriber API adapter -- isolates swsscommon so the class is testable.
# ---------------------------------------------------------------------------
class SwssSubscriberApi:
    """Thin adapter over the swsscommon events subscriber API.

    Verified against sonic-swss-common (``common/events.h``): the receive call
    is ``int event_receive(handle, event_receive_op_t &evt)`` -- construct an
    ``event_receive_op_t``, pass it in, then read ``evt.key`` / ``evt.params``
    / ``evt.missed_cnt`` (rc == 0 on success; non-zero on timeout).  A finite
    ``recv_timeout`` (ms) lets the receive loop poll for shutdown instead of
    blocking forever.

    Instantiating this class imports swsscommon; if that fails (e.g. a dev host
    without the library), ``available`` is False and receives are no-ops.  A
    fake with the same ``init_subscriber`` / ``receive`` / ``deinit_subscriber``
    surface can be injected in tests.
    """

    RECV_TIMEOUT_MS = 1000

    def __init__(self):
        self.available = False
        self._init_subscriber = None
        self._deinit_subscriber = None
        self._event_receive = None
        self._op_type = None
        try:
            from swsscommon.swsscommon import (
                events_init_subscriber,
                events_deinit_subscriber,
                event_receive,
                event_receive_op_t,
            )
            self._init_subscriber = events_init_subscriber
            self._deinit_subscriber = events_deinit_subscriber
            self._event_receive = event_receive
            self._op_type = event_receive_op_t
            self.available = True
        except Exception as exc:  # pragma: no cover - env dependent
            logger.log_warning(
                f"swsscommon events subscriber API unavailable "
                f"({exc}); EventSubscriber will no-op receives")

    def init_subscriber(self, sources, use_cache=True):
        """Initialise a subscriber handle.

        Tries a source-filtered subscription first (broker-side filter); on any
        marshalling error falls back to subscribe-all -- the EventSubscriber
        index filters by (source, tag) regardless, so behaviour is identical.
        """
        if not self.available:
            return None
        if sources:
            try:
                return self._init_subscriber(
                    use_cache, self.RECV_TIMEOUT_MS, list(sources))
            except Exception:  # pragma: no cover - env dependent
                pass
        try:
            return self._init_subscriber(use_cache, self.RECV_TIMEOUT_MS)
        except Exception as exc:  # pragma: no cover - env dependent
            logger.log_warning(f"events_init_subscriber failed: {exc}")
            return None

    def receive(self, handle):
        """Block (up to ``RECV_TIMEOUT_MS``) for the next event.

        Returns ``(key, params_dict, missed_cnt)`` or ``None`` on timeout /
        error / unavailability.
        """
        if not self.available or handle is None:
            return None
        try:
            evt = self._op_type()
            rc = self._event_receive(handle, evt)
        except Exception:  # pragma: no cover - env dependent
            return None
        if rc != 0:
            return None            # timeout or receive error
        key = getattr(evt, 'key', '') or ''
        if not key:
            return None
        try:
            params = dict(evt.params)
        except Exception:
            try:
                params = {k: evt.params[k] for k in evt.params}
            except Exception:
                params = {}
        missed = getattr(evt, 'missed_cnt', 0) or 0
        return key, params, missed

    def deinit_subscriber(self, handle):
        if self.available and handle is not None:
            try:
                self._deinit_subscriber(handle)
            except Exception:  # pragma: no cover - env dependent
                pass


class EventSubscriber:
    """Receives eventd events and raises/clears alarms via EventPublisher.

    Parameters
    ----------
    event_tables : list[dict]
        The ``event``-type alarm_tables from the merged alarm definitions.
        Each declares ``event_source``, ``event_tag``, an optional
        ``object_key_field``, and a ``checks`` list.
    publisher : EventPublisher
        The eventd output path (``set_alarm(alarm_id, object_name, is_fault)``).
    sources : list[str], optional
        eventd source (YANG module) names to subscribe to
        (``event_source_mapping``).  ``None`` subscribes to everything.
    event_api : object, optional
        Object exposing ``init_subscriber(sources)`` / ``receive(handle)`` /
        ``deinit_subscriber(handle)``.  Defaults to a swsscommon-backed
        adapter; inject a fake for tests.
    """

    def __init__(self, event_tables, publisher, sources=None, event_api=None):
        self._publisher = publisher
        self._sources = sources
        self._api = event_api if event_api is not None else SwssSubscriberApi()
        self._handle = None
        self._index = self._build_index(event_tables)

    @staticmethod
    def _build_index(event_tables):
        """Index event tables by ``(event_source, event_tag)`` for lookup."""
        index = {}
        for table in event_tables or []:
            source = table.get('event_source')
            tag = table.get('event_tag')
            if source and tag:
                index[(source, tag)] = table
        return index

    def start(self):
        """Initialise the subscriber handle.  Returns True on success."""
        self._handle = self._api.init_subscriber(self._sources)
        return self._handle is not None

    @staticmethod
    def _parse_key(key):
        """Split an event key ``"<source>:<tag>"`` into ``(source, tag)``."""
        if key and ':' in key:
            source, tag = key.split(':', 1)
            return source, tag
        return None, None

    def process_event(self, key, params):
        """Evaluate one received event against its matching checks.

        Aggregates per ``(alarm_id, object_name)`` with OR-logic (HLD §7.8)
        and publishes RAISE/CLEAR through ``EventPublisher``.
        """
        source, tag = self._parse_key(key)
        table = self._index.get((source, tag))
        if table is None:
            return

        key_field = table.get('object_key_field')
        object_name = params.get(key_field) if key_field else None
        if not object_name:
            # Fall back to the event's resource param, then the source name.
            object_name = params.get('resource') or source or 'SYSTEM'

        fault_map = {}   # alarm_id -> bool
        for check in table.get('checks', []):
            alarm_id = check.get('alarm_id')
            cond = check.get('condition', {})
            field_name = cond.get('field', '')
            operator = cond.get('operator', '==')
            expected = cond.get('value', '')

            if not alarm_id or not field_name:
                continue

            try:
                is_fault = evaluate_condition(
                    params.get(field_name), operator, expected)
            except Exception as exc:
                logger.log_error(
                    f"{source}:{tag}.{alarm_id}: condition error: {exc}")
                is_fault = False

            if alarm_id not in fault_map:
                fault_map[alarm_id] = is_fault
            elif is_fault:
                fault_map[alarm_id] = True

        for alarm_id, is_fault in fault_map.items():
            try:
                self._publisher.set_alarm(
                    alarm_id, object_name, is_fault=is_fault)
            except Exception as exc:
                logger.log_error(
                    f"Failed to set_alarm({alarm_id}, {object_name}, "
                    f"{is_fault}): {exc}")

    def receive_once(self):
        """Receive and process a single event.  Returns True if one was handled."""
        result = self._api.receive(self._handle)
        if result is None:
            return False
        key, params, missed = result
        if missed:
            logger.log_warning(
                f"event subscriber missed {missed} event(s) before {key} "
                f"(ZMQ overflow / subscriber lag); periodic reconcile repairs "
                f"any resulting drift")
        try:
            self.process_event(key, params)
        except Exception as exc:
            logger.log_error(f"process_event({key}) failed: {exc}")
        return True

    def run(self, should_continue):
        """Blocking receive loop; runs until ``should_continue()`` is False."""
        if self._handle is None and not self.start():
            logger.log_warning(
                "EventSubscriber: no subscriber handle (events API "
                "unavailable) — event adapter disabled")
            return
        logger.log_info("EventSubscriber started")
        while should_continue():
            try:
                self.receive_once()
            except Exception as exc:
                logger.log_error(f"event receive error: {exc}")

    def stop(self):
        self._api.deinit_subscriber(self._handle)
        self._handle = None
