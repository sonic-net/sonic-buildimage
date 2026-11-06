"""
sonic_alarm.constants -- All string constants, file paths, default intervals,
and hard limits for alarmd.

No logic lives here.  Every module imports from this file rather than
defining its own magic strings.
"""

# ---------------------------------------------------------------------------
# Identity
# ---------------------------------------------------------------------------
SYSLOG_IDENTIFIER = 'alarmd'

# ---------------------------------------------------------------------------
# Database
# ---------------------------------------------------------------------------
STATE_DB = 'STATE_DB'

# Event & Alarm Framework (eventd) integration
EVENT_DB_NAME = 'EVENT_DB'              # logical Redis DB 19 (ALARM/ALARM_STATS)
ALARM_TABLE_NAME = 'ALARM'             # eventd-owned current-alarms table
RECONCILE_INTERVAL = 60                # seconds between reconcile_active_set() passes

# alarmd publishes all its alarms under this event source (YANG module) and a
# bare event tag; the published type-id == alarm_id (HLD §7.5, §9.3).
EVENT_SOURCE = 'sonic-events-alarmd'
EVENT_TAG = 'alarm'
# Event-profile fragment shipped in the package (alarm_id -> severity); used to
# validate that every alarm_id is profiled so eventd will not drop it (§9.3).
EVENT_PROFILE_FILENAME = 'sonic_events_alarmd.json'

# ---------------------------------------------------------------------------
# File paths
# ---------------------------------------------------------------------------
MACHINE_CONF = '/host/machine.conf'
DEVICE_BASE = '/usr/share/sonic/device'
ALARM_DEFS_FILENAME = 'alarm_defs.json'
ALARM_SCRIPTS_DIR = '/etc/sonic/alarm_scripts'
SONIC_VERSION_FILE = '/etc/sonic/sonic_version.yml'
COMMON_ALARM_DEFS_FILENAME = 'common_alarm_defs.json'

# ---------------------------------------------------------------------------
# Default intervals
# ---------------------------------------------------------------------------
DEFAULT_STATEDB_INTERVAL = 3       # seconds between statedb polls
DEFAULT_SCRIPT_INTERVAL = 60       # seconds between script runs
SCRIPT_TIMEOUT = 10                # default per-script timeout in seconds

# ---------------------------------------------------------------------------
# Hard limits -- design guardrails that bound alarmd's resource use.
# Values below a floor are clamped up; values above a ceiling are clamped down.
# See the alarmd HLD §7.13 for the rationale behind each value.
# ---------------------------------------------------------------------------
MAX_STATEDB_CHECKS = 200           # absolute cap on statedb field evaluations
MAX_SCRIPT_CHECKS = 50             # absolute cap on script alarm definitions
MIN_STATEDB_INTERVAL = 2           # seconds -- floor for statedb poll interval
MIN_SCRIPT_INTERVAL = 30           # seconds -- floor for script poll interval
MAX_SCRIPT_TIMEOUT = 30            # seconds -- ceiling for any single script

# ---------------------------------------------------------------------------
# PID file -- used for single-instance enforcement via fcntl.flock()
# ---------------------------------------------------------------------------
PIDFILE_PATH = '/var/run/alarmd.pid'

# ---------------------------------------------------------------------------
# Validation
# ---------------------------------------------------------------------------
VALID_SEVERITIES = {'Critical', 'Major', 'Minor', 'Warning', 'Informational'}
VALID_OPERATORS = {'==', '!=', '<', '>', '<=', '>='}
