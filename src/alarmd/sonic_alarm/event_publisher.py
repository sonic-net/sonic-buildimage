"""
sonic_alarm.event_publisher -- New-design alarm output path (eventd producer).

Under the Event & Alarm Framework design, alarmd owns no alarm table.  Rather
than writing a private ``SYSTEM_ALARMS`` table in STATE_DB, ``EventPublisher``
is an event **producer**: it publishes ``RAISE`` / ``CLEAR`` actions through
eventd's
libswsscommon API (``events_init_publisher`` / ``event_publish``).  eventd's
Event Consumer (the ``eventdb`` service) is the sole writer of the ``ALARM`` /
``ALARM_STATS`` tables in ``EVENT_DB`` (Redis logical DB 19).

Contract verified against the merged eventd source
(``sonic-eventd/src/eventconsume.cpp`` -> ``fetchFieldValues`` /
``staticInfoExists`` / ``cal_lookup_map``):

  * Event params eventd reads:
        type-id   -> alarm identity id      (== alarmd alarm_id)
        resource  -> alarm object/instance  (== alarmd object_name)
        action    -> RAISE | CLEAR | ACKNOWLEDGE | UNACKNOWLEDGE
        text      -> free-form message
  * Alarm identity  = "type-id|resource"  (eventd's cal_lookup_map key).
  * Severity is assigned by eventd from the event profile
    (``/etc/evprofile/default.json``) and is **NOT** sent by the publisher.
  * eventd DROPS any event whose type-id is not in the profile
    (``staticInfoExists``), so every alarm_id must have a profile entry.
  * eventd de-dup is weak (it only throttles an event identical to the one
    immediately before it), so idempotency is alarmd's responsibility -- hence
    the thread-safe ``_active`` cache and ``reconcile_active_set()``.

The module imports swsscommon lazily and defensively so that it can be
imported, unit-tested (with a fake event API), and run in a standalone
prototype harness on a host that has no swsscommon installed.
"""

import logging
import threading

from logging.handlers import SysLogHandler
from sonic_py_common.syslogger import SysLogger

from sonic_alarm.constants import EVENT_SOURCE, EVENT_TAG


logger = SysLogger(
    log_identifier='alarmd#publisher',
    log_facility=SysLogHandler.LOG_DAEMON,
    log_level=logging.INFO,
    enable_runtime_config=False,
)

# ---------------------------------------------------------------------------
# eventd contract constants.  The source/tag names live in constants.py
# (imported above; HLD §7.1); the action and param tokens below are eventd's
# Event-Consumer wire contract (eventconsume.cpp) and stay local here.
# ---------------------------------------------------------------------------
ACTION_RAISE = 'RAISE'
ACTION_CLEAR = 'CLEAR'

# eventd param keys (must match eventconsume.cpp::fetchFieldValues exactly)
PARAM_TYPE_ID = 'type-id'
PARAM_ACTION = 'action'
PARAM_RESOURCE = 'resource'
PARAM_TEXT = 'text'

# ALARM table (in EVENT_DB) fields used for startup sync / reconcile
ALARM_FIELD_TYPE_ID = 'type-id'
ALARM_FIELD_RESOURCE = 'resource'


# ---------------------------------------------------------------------------
# Event API adapter -- isolates swsscommon so the class is testable.
# ---------------------------------------------------------------------------
class SwssEventApi:
    """Thin adapter over the swsscommon events producer API.

    Instantiating this class imports swsscommon; if that fails (e.g. running on
    a dev host without the library), ``available`` is False and publishes are
    logged no-ops.  A fake with the same ``init_publisher`` / ``publish``
    surface can be injected in tests and prototypes.
    """

    def __init__(self):
        self.available = False
        self._event_publish = None
        self._init_publisher = None
        self._deinit_publisher = None
        self._field_value_map = None
        try:
            from swsscommon.swsscommon import (
                events_init_publisher,
                events_deinit_publisher,
                event_publish,
                FieldValueMap,
            )
            self._init_publisher = events_init_publisher
            self._deinit_publisher = events_deinit_publisher
            self._event_publish = event_publish
            self._field_value_map = FieldValueMap
            self.available = True
        except Exception as exc:  # pragma: no cover - env dependent
            logger.log_warning(
                f"swsscommon events API unavailable "
                f"({exc}); EventPublisher will no-op publishes")

    def init_publisher(self, source):
        if not self.available:
            return None
        return self._init_publisher(source)

    def deinit_publisher(self, handle):
        if self.available and handle is not None:
            self._deinit_publisher(handle)

    def publish(self, handle, tag, params):
        """Publish ``params`` (a plain dict) under ``tag``.

        Returns the event_publish() return code (0 == success) or -1 when the
        API is unavailable.
        """
        if not self.available or handle is None:
            return -1
        fvm = self._field_value_map()
        for key, value in params.items():
            fvm[str(key)] = str(value)
        return self._event_publish(handle, tag, fvm)


class EventPublisher:
    """Publishes alarm RAISE/CLEAR actions to eventd (the single output path).

    All input adapters -- StateDBPoller, ScriptRunner and EventSubscriber --
    call ``set_alarm()`` on this class.

    Parameters
    ----------
    alarm_defs : dict
        Parsed alarm definitions -- used to build alarm_id -> metadata
        (category, description_template, source) for the event ``text``.
        NOTE: severity is intentionally *not* used for publishing; eventd
        assigns severity from the event profile.
    event_api : object, optional
        Object exposing ``init_publisher(source)`` and
        ``publish(handle, tag, params_dict)``.  Defaults to a real
        swsscommon-backed adapter; inject a fake for tests/prototypes.
    alarm_table : object, optional
        swsscommon Table-like object bound to the ALARM table in EVENT_DB,
        exposing ``getKeys()`` and ``get(key) -> (bool, iterable_of_pairs)``.
        Used only by ``sync_active_set()`` / ``reconcile_active_set()``.
        When None, sync/reconcile are best-effort no-ops.
    source, tag : str
        Event source (YANG module) and tag.  Defaults per the HLD.
    """

    def __init__(self, alarm_defs, event_api=None, alarm_table=None,
                 source=EVENT_SOURCE, tag=EVENT_TAG):
        self._source = source
        self._tag = tag
        self._api = event_api if event_api is not None else SwssEventApi()
        self._alarm_table = alarm_table

        self._lock = threading.Lock()
        self._active = set()          # set of "alarm_id|object_name"
        self._meta = {}               # alarm_id -> metadata dict
        self._rebuild_meta(alarm_defs)

        self._handle = self._api.init_publisher(self._source)
        if self._handle is None:
            logger.log_warning(
                "EventPublisher: no publisher handle "
                "(events API unavailable) — running in no-op mode")

    # -- metadata management -------------------------------------------------

    def _rebuild_meta(self, alarm_defs):
        """(Re)build alarm_id -> metadata from alarm definitions.

        Called at init and again on SIGHUP reload.  Only fields used to
        compose the event ``text`` are kept; severity is owned by eventd.
        """
        meta = {}
        for table in alarm_defs.get('alarm_tables', []):
            source = table.get('table_name',
                               table.get('group_name', self._source))
            for check in table.get('checks', []):
                aid = check.get('alarm_id')
                if aid and aid not in meta:
                    meta[aid] = {
                        'category': check.get('category', 'System'),
                        'description_template': check.get(
                            'description_template', '{alarm_id}'),
                        'source': source,
                    }
        with self._lock:
            self._meta = meta

    def known_alarm_ids(self):
        """Return the set of alarm_ids currently in the metadata lookup."""
        with self._lock:
            return set(self._meta.keys())

    # -- publish helpers -----------------------------------------------------

    def _compose_text(self, alarm_id, object_name, description):
        if description is not None:
            return description
        meta = self._meta.get(alarm_id, {})
        tmpl = meta.get('description_template', '{alarm_id}')
        return (tmpl.replace('{object_name}', object_name)
                    .replace('{alarm_id}', alarm_id))

    def _publish(self, alarm_id, object_name, action, text):
        params = {
            PARAM_TYPE_ID: alarm_id,
            PARAM_ACTION: action,
            PARAM_RESOURCE: object_name,
            PARAM_TEXT: text,
        }
        return self._api.publish(self._handle, self._tag, params)

    # -- public API (adapter-facing) -----------------------------------------

    def set_alarm(self, alarm_id, object_name, is_fault, description=None):
        """Raise or clear based on *is_fault*.  Primary adapter entry point."""
        if is_fault:
            self.publish_alarm(alarm_id, object_name, description=description)
        else:
            self.publish_clear(alarm_id, object_name)

    def publish_alarm(self, alarm_id, object_name, description=None,
                      action=ACTION_RAISE):
        """Publish a RAISE.  Idempotent: no-op if already active."""
        tag = f"{alarm_id}|{object_name}"
        with self._lock:
            if tag in self._active:
                return
            self._active.add(tag)

        text = self._compose_text(alarm_id, object_name, description)
        rc = self._publish(alarm_id, object_name, action, text)
        if rc == 0 or rc == -1:
            # rc == -1 => events API unavailable (no-op mode); keep cache so
            # behaviour is consistent for prototype/unit tests.
            logger.log_warning(
                f"ALARM RAISED: {alarm_id} on {object_name} — {text}")
        else:
            logger.log_error(
                f"event_publish RAISE failed rc={rc} for {tag}")
            with self._lock:
                self._active.discard(tag)

    def publish_clear(self, alarm_id, object_name):
        """Publish a CLEAR.  No-op if not active."""
        tag = f"{alarm_id}|{object_name}"
        with self._lock:
            if tag not in self._active:
                return
            self._active.discard(tag)

        rc = self._publish(alarm_id, object_name, ACTION_CLEAR, '')
        if rc == 0 or rc == -1:
            logger.log_info(f"ALARM CLEARED: {alarm_id} on {object_name}")
        else:
            logger.log_error(
                f"event_publish CLEAR failed rc={rc} for {tag}")
            with self._lock:
                self._active.add(tag)     # rollback so next cycle retries

    # raise_alarm/clear_alarm aliases so ScriptRunner (which calls these
    # directly) works with EventPublisher unchanged.
    def raise_alarm(self, alarm_id, object_name, description=None):
        self.publish_alarm(alarm_id, object_name, description=description)

    def clear_alarm(self, alarm_id, object_name):
        self.publish_clear(alarm_id, object_name)

    def purge_stale_alarms(self, valid_alarm_ids):
        """After SIGHUP reload, CLEAR active alarms whose id is gone.

        Returns the list of tags purged.
        """
        with self._lock:
            stale = [t for t in self._active
                     if t.split('|', 1)[0] not in valid_alarm_ids]
        for tag in stale:
            alarm_id, object_name = tag.split('|', 1)
            self.publish_clear(alarm_id, object_name)
        return stale

    def clear_all(self):
        """Publish CLEAR for every alarm this instance believes is active."""
        with self._lock:
            tags = list(self._active)
        for tag in tags:
            alarm_id, object_name = tag.split('|', 1)
            self.publish_clear(alarm_id, object_name)

    # -- startup sync / periodic reconcile against eventd's ALARM table ------

    def _read_authoritative_set(self):
        """Read ALARM table and return the set of "type-id|resource" tags
        whose type-id is one of alarmd's own alarm_ids.

        The ALARM table has no source column, so we filter by known alarm_ids
        to avoid adopting alarms raised by other producers.
        """
        if self._alarm_table is None:
            return None
        known = self.known_alarm_ids()
        authoritative = set()
        try:
            for key in self._alarm_table.getKeys():
                status, fvs = self._alarm_table.get(key)
                if not status:
                    continue
                fields = dict(fvs)
                type_id = fields.get(ALARM_FIELD_TYPE_ID)
                resource = fields.get(ALARM_FIELD_RESOURCE, '')
                if type_id in known:
                    authoritative.add(f"{type_id}|{resource}")
        except Exception as exc:
            logger.log_warning(f"Could not read ALARM table: {exc}")
            return None
        return authoritative

    def sync_active_set(self):
        """On startup, adopt already-active alarms from eventd's ALARM table
        so alarmd does not republish alarms that survived an alarmd restart.
        """
        authoritative = self._read_authoritative_set()
        if authoritative is None:
            return
        with self._lock:
            self._active = set(authoritative)
        logger.log_info(
            f"sync_active_set: adopted {len(authoritative)} active alarm(s) "
            f"from eventd ALARM table")

    def reconcile_active_set(self):
        """Periodically repair drift between the local cache and eventd.

        Level-triggered: adopt eventd's authoritative set as the cache.  A tag
        that eventd no longer has is dropped (so a still-true condition
        re-RAISEs on the next adapter cycle); a tag eventd has that we lost is
        adopted (so we don't duplicate-RAISE, and a since-cleared condition
        CLEARs on the next cycle).
        """
        authoritative = self._read_authoritative_set()
        if authoritative is None:
            return
        with self._lock:
            before = set(self._active)
            self._active = set(authoritative)
        added = authoritative - before
        removed = before - authoritative
        if added or removed:
            logger.log_info(
                f"reconcile: +{len(added)} adopted, -{len(removed)} dropped")

    # -- introspection -------------------------------------------------------

    @property
    def active_count(self):
        with self._lock:
            return len(self._active)

    @property
    def active_tags(self):
        with self._lock:
            return frozenset(self._active)

    def close(self):
        self._api.deinit_publisher(self._handle)
        self._handle = None
