import typing
from natsort import natsorted
import datetime
import pickle
import os
import copy
import json
import re
import click
from tabulate import tabulate

import utilities_common.multi_asic as multi_asic_util
from swsscommon.swsscommon import CounterTable, MacsecCounter, SonicV2Connector
from utilities_common.cli import UserCache
from sonic_py_common import device_info

CACHE_MANAGER = UserCache(app_name="macsec")
CACHE_FILE = os.path.join(CACHE_MANAGER.get_directory(), "macsecstats{}")

DB_CONNECTOR = None
COUNTER_TABLE = None

MKA_SESSION_TABLE = "MACSEC_MKA_SESSION_TABLE"
MKA_PARTICIPANT_TABLE = "MACSEC_MKA_PARTICIPANT_TABLE"
MKA_STALE_THRESHOLD_SECONDS = 60
MKA_SESSION_FIELDS = (
    "profile",
    "kay_status",
    "authenticated",
    "secured",
    "failed",
    "actor_sci",
    "key_server_sci",
    "actor_priority",
    "key_server_priority",
    "is_key_server",
    "keys_distributed",
    "keys_received",
    "mka_hello_time_ms",
    "query_status",
    "last_updated",
    "config_status",
    "config_error",
)
MKA_PARTICIPANT_FIELDS = (
    "mi",
    "mn",
    "active",
    "is_principal",
    "is_primary",
    "live_peers",
    "potential_peers",
    "is_key_server",
    "is_elected",
)


def _allowlisted_fields(entry, allowlist):
    return {field: entry[field] for field in allowlist if field in entry}


def _parse_utc_timestamp(timestamp):
    if not timestamp:
        return None
    try:
        value = timestamp
        if value.endswith("Z"):
            value = value[:-1] + "+00:00"
        parsed = datetime.datetime.fromisoformat(value)
        if parsed.tzinfo is None:
            parsed = parsed.replace(tzinfo=datetime.timezone.utc)
        return parsed.astimezone(datetime.timezone.utc)
    except (AttributeError, TypeError, ValueError):
        return None


def _freshness(last_updated, query_status, now=None):
    parsed = _parse_utc_timestamp(last_updated)
    if parsed is None:
        return "never", "never"

    if now is None:
        now = datetime.datetime.now(datetime.timezone.utc)
    age = max(0, (now - parsed).total_seconds())
    flags = []
    if age > MKA_STALE_THRESHOLD_SECONDS:
        flags.append("stale")
    if query_status == "error":
        flags.append("retained")
    elif query_status != "ok":
        flags.append("query-unknown")

    suffix = " ({})".format(", ".join(flags)) if flags else ""
    compact = "{}s{}".format(int(age), suffix)
    detail_suffix = "; {}".format(", ".join(flags)) if flags else ""
    detail = "{} ({}s ago{})".format(last_updated, int(age), detail_suffix)
    return compact, detail


def _age_seconds(last_updated, now=None):
    parsed = _parse_utc_timestamp(last_updated)
    if parsed is None:
        return None
    if now is None:
        now = datetime.datetime.now(datetime.timezone.utc)
    return max(0, (now - parsed).total_seconds())


def _age_label(age):
    return "never" if age is None else "{}s".format(int(age))


def _compact_status(session, age):
    flags = []
    query_status = session.get("query_status")
    config_status = session.get("config_status")

    if query_status == "error":
        flags.append("query-error")
    elif query_status != "ok":
        flags.append("query-unknown")

    if age is None:
        flags.append("age-unknown")
    elif age > MKA_STALE_THRESHOLD_SECONDS:
        flags.append("stale")

    if config_status == "degraded":
        flags.append("config-degraded")
    elif config_status != "in-sync":
        flags.append("config-unknown")

    return ",".join(flags) if flags else "ok"


def _safe_enum(value, values):
    return value if value in values else "-"


def _safe_bool(value):
    return _safe_enum(value, ("true", "false"))


def _safe_uint(value):
    try:
        parsed = int(value)
        return str(parsed) if parsed >= 0 else "-"
    except (TypeError, ValueError):
        return "-"


def _safe_hex(value, lengths, secrets=()):
    if not isinstance(value, str) or len(value) not in lengths:
        return "-"
    if any(
        value.lower() == secret.lower() or
        (len(secret) in (66, 130) and value.lower() == secret[2:].lower())
        for secret in secrets
    ):
        return "-"
    if re.fullmatch(r"[0-9a-fA-F]+", value) is None:
        return "-"
    return value.lower()


def _format_milliseconds(value):
    sanitized = _safe_uint(value)
    return "{} ms".format(sanitized) if sanitized != "-" else "-"


def _safe_sci(value, secrets=(), hide_zero=False):
    sci = _safe_hex(value, (16,), secrets)
    if hide_zero and sci == "0000000000000000":
        return "-"
    return sci


def _controlled_port_mode(session):
    kay_status = session.get("kay_status")
    authenticated = session.get("authenticated")
    secured = session.get("secured")
    failed = session.get("failed")

    if kay_status not in ("active", "not-active"):
        return "-"
    if any(value not in ("true", "false")
           for value in (authenticated, secured, failed)):
        return "-"
    if failed == "true":
        return "failed"
    if kay_status == "active":
        if authenticated == "false" and secured == "true":
            return "secured"
        if authenticated == "true" and secured == "false":
            return "authenticated-only"
        return "inconsistent"
    if authenticated == "false" and secured == "false":
        return "inactive"
    return "inconsistent"


def _redact_known_secrets(value, secrets):
    result = str(value)
    expanded_secrets = set()
    for secret in secrets:
        if not secret:
            continue
        expanded_secrets.add(secret)
        if (len(secret) in (66, 130) and
                re.fullmatch(r"[0-9a-fA-F]+", secret)):
            expanded_secrets.add(secret[2:])
    for secret in sorted(expanded_secrets, key=len, reverse=True):
        result = re.sub(
            re.escape(secret), "[redacted]", result, flags=re.IGNORECASE
        )
    result = re.sub(
        r"(?i)(?<![0-9a-f])(?:[0-9a-f]{130}|[0-9a-f]{128}|[0-9a-f]{66})(?![0-9a-f])",
        "[redacted]",
        result,
    )
    result = re.sub(
        r"(?i)(\b(?:(?:(?:primary|fallback|new|old|previous|stale|decoded|raw)[_ -]?)?cak|"
        r"(?:decoded|raw|secret)[_ -]?key|key[_ -]?material)\b"
        r"\s*(?:[:=]|\bis\b)?\s*)"
        r"(?<![0-9a-f])[0-9a-f]{64}(?![0-9a-f])",
        r"\1[redacted]",
        result,
    )
    return result


def _compact_ckn(ckn, secrets=()):
    safe_ckn = _safe_hex(ckn, (32, 64), secrets)
    if safe_ckn == "-":
        return safe_ckn
    return "{}...{}".format(safe_ckn[:6], safe_ckn[-6:])


class MACsecCfgMeta(object):
    def __init__(self, *args) -> None:
        SEPARATOR = DB_CONNECTOR.get_db_separator(DB_CONNECTOR.CONFIG_DB)
        self.key = self.__class__.get_cfg_table_name() + SEPARATOR + \
            SEPARATOR.join(args)
        self.cfgMeta = DB_CONNECTOR.get_all(
            DB_CONNECTOR.CONFIG_DB, self.key)
        if len(self.cfgMeta) == 0:
            raise ValueError("No such MACsecCfgMeta: {}".format(self.key))
        for k, v in self.cfgMeta.items():
            setattr(self, k, v)

class MACsecAppMeta(object):
    def __init__(self, *args) -> None:
        SEPARATOR = DB_CONNECTOR.get_db_separator(DB_CONNECTOR.APPL_DB)
        self.key = self.__class__.get_appl_table_name() + SEPARATOR + \
            SEPARATOR.join(args)
        self.meta = DB_CONNECTOR.get_all(
            DB_CONNECTOR.APPL_DB, self.key)
        if len(self.meta) == 0:
            raise ValueError("No such MACsecAppMeta: {}".format(self.key))
        for k, v in self.meta.items():
            setattr(self, k, v)


class MACsecCounters(object):
    def __init__(self, *args) -> None:
        _, fvs = COUNTER_TABLE.get(MacsecCounter(), ":".join(args))
        self.counters = dict(fvs)


class MACsecSA(MACsecAppMeta, MACsecCounters):
    def __init__(self, port_name: str, sci: str, an: str) -> None:
        self.port_name = port_name
        self.sci = sci
        self.an = an
        MACsecAppMeta.__init__(self, port_name, sci, an)
        MACsecCounters.__init__(self, port_name, sci, an)

    def dump_str(self, cache = None) -> str:
        buffer = self.get_header()
        meta = sorted(self.meta.items(), key=lambda x: x[0])
        counters = copy.deepcopy(self.counters)
        if cache:
            for k, v in counters.items():
                if k in cache.counters and k.startswith("SAI_MACSEC_SA_STAT"):
                    counters[k] = int(counters[k]) - int(cache.counters[k])
        counters = sorted(counters.items(), key=lambda x: x[0])
        buffer += tabulate(meta + counters)
        buffer = "\n".join(["\t\t" + line for line in buffer.splitlines()])
        return buffer


class MACsecIngressSA(MACsecSA):
    def __init__(self, port_name: str, sci: str, an: str) -> None:
        super(MACsecIngressSA, self).__init__(port_name, sci, an)

    @classmethod
    def get_appl_table_name(cls) -> str:
        return "MACSEC_INGRESS_SA_TABLE"

    def get_header(self):
        return "MACsec Ingress SA ({})\n".format(self.an)


class MACsecEgressSA(MACsecSA):
    def __init__(self, port_name: str, sci: str, an: str) -> None:
        super(MACsecEgressSA, self).__init__(port_name, sci, an)

    @classmethod
    def get_appl_table_name(cls) -> str:
        return "MACSEC_EGRESS_SA_TABLE"

    def get_header(self):
        return "MACsec Egress SA ({})\n".format(self.an)


class MACsecSC(MACsecAppMeta):
    def __init__(self, port_name: str, sci: str) -> None:
        self.port_name = port_name
        self.sci = sci
        super(MACsecSC, self).__init__(port_name, sci)


class MACsecIngressSC(MACsecSC):
    def __init__(self, port_name: str, sci: str) -> None:
        super(MACsecIngressSC, self).__init__(port_name, sci)

    @classmethod
    def get_appl_table_name(cls) -> str:
        return "MACSEC_INGRESS_SC_TABLE"

    def dump_str(self, cache = None) -> str:
        buffer = self.get_header()
        buffer = "\n".join(["\t" + line for line in buffer.splitlines()])
        return buffer

    def get_header(self):
        return "MACsec Ingress SC ({})\n".format(self.sci)


class MACsecEgressSC(MACsecSC):
    def __init__(self, port_name: str, sci: str) -> None:
        super(MACsecEgressSC, self).__init__(port_name, sci)

    @classmethod
    def get_appl_table_name(cls) -> str:
        return "MACSEC_EGRESS_SC_TABLE"

    def dump_str(self, cache = None) -> str:
        buffer = self.get_header()
        buffer += tabulate(sorted(self.meta.items(), key=lambda x: x[0]))
        buffer = "\n".join(["\t" + line for line in buffer.splitlines()])
        return buffer

    def get_header(self):
        return "MACsec Egress SC ({})\n".format(self.sci)


class MACsecPort(MACsecAppMeta, MACsecCfgMeta):
    def __init__(self,  port_name: str) -> None:
        self.port_name = port_name
        MACsecAppMeta.__init__(self, port_name)
        MACsecCfgMeta.__init__(self, port_name)

    @classmethod
    def get_appl_table_name(cls) -> str:
        return "MACSEC_PORT_TABLE"

    @classmethod
    def get_cfg_table_name(cls) -> str:
        return "PORT"

    def dump_str(self, cache = None) -> str:
        buffer = self.get_header()

        # Add the profile information to the meta dict from config meta dict
        self.meta["profile"] = self.cfgMeta["macsec"]

        buffer += tabulate(sorted(self.meta.items(), key=lambda x: x[0]))
        return buffer

    def get_header(self) -> str:
        return "MACsec port({})\n".format(self.port_name)

class MACsecProfile(MACsecCfgMeta):
    def __init__(self, profile_name: str) -> None:
        self.profile_name = profile_name
        super(MACsecProfile, self).__init__(profile_name)

    @classmethod
    def get_cfg_table_name(cls) -> str:
        return "MACSEC_PROFILE"

    def dump_str(self, cache = None) -> str:
        buffer = self.get_header()

        # Don't display the primary and fallback CAK
        if 'primary_cak' in self.cfgMeta: del self.cfgMeta['primary_cak']
        if 'fallback_cak' in self.cfgMeta: del self.cfgMeta['fallback_cak']

        t_buffer = tabulate(sorted(self.cfgMeta.items(), key=lambda x: x[0]))
        t_buffer = "\n".join(["\t" + line for line in t_buffer.splitlines()])
        buffer += t_buffer
        return buffer

    def get_header(self) -> str:
        return "MACsec profile : {}\n".format(self.profile_name)

def create_macsec_obj(key: str) -> MACsecAppMeta:
    attr = key.split(":")
    try:
        if attr[0] == MACsecPort.get_appl_table_name():
            return MACsecPort(attr[1])
        elif attr[0] == MACsecIngressSC.get_appl_table_name():
            return MACsecIngressSC(attr[1], attr[2])
        elif attr[0] == MACsecEgressSC.get_appl_table_name():
            return MACsecEgressSC(attr[1], attr[2])
        elif attr[0] == MACsecIngressSA.get_appl_table_name():
            return MACsecIngressSA(attr[1], attr[2], attr[3])
        elif attr[0] == MACsecEgressSA.get_appl_table_name():
            return MACsecEgressSA(attr[1], attr[2], attr[3])
        raise TypeError("Unknown MACsec object type")
    except ValueError as e:
        return None

def create_macsec_profile_obj(key: str) -> MACsecCfgMeta:
    attr = key.split("|")
    try:
        if attr[0] == MACsecProfile.get_cfg_table_name():
            return MACsecProfile(attr[1])
        raise TypeError("Unknown MACsec object type")
    except ValueError as e:
        return None

def create_macsec_objs(interface_name: str) -> typing.List[MACsecAppMeta]:
    objs = []
    objs.append(create_macsec_obj(MACsecPort.get_appl_table_name() + ":" + interface_name))
    egress_scs = DB_CONNECTOR.keys(DB_CONNECTOR.APPL_DB, MACsecEgressSC.get_appl_table_name() + ":" + interface_name + ":*")
    for sc_name in natsorted(egress_scs):
        sc = create_macsec_obj(sc_name)
        if sc is None:
            continue
        objs.append(sc)
        egress_sas = DB_CONNECTOR.keys(DB_CONNECTOR.APPL_DB, MACsecEgressSA.get_appl_table_name() + ":" + ":".join(sc_name.split(":")[1:]) + ":*")
        for sa_name in natsorted(egress_sas):
            sa = create_macsec_obj(sa_name)
            if sa is None:
                continue
            objs.append(sa)
    ingress_scs = DB_CONNECTOR.keys(DB_CONNECTOR.APPL_DB, MACsecIngressSC.get_appl_table_name() + ":" + interface_name + ":*")
    for sc_name in natsorted(ingress_scs):
        sc = create_macsec_obj(sc_name)
        if sc is None:
            continue
        objs.append(sc)
        ingress_sas = DB_CONNECTOR.keys(DB_CONNECTOR.APPL_DB, MACsecIngressSA.get_appl_table_name() + ":" + ":".join(sc_name.split(":")[1:]) + ":*")
        for sa_name in natsorted(ingress_sas):
            sa = create_macsec_obj(sa_name)
            if sa is None:
                continue
            objs.append(sa)
    return objs


def create_macsec_profiles_objs(profile_name: str) -> typing.List[MACsecCfgMeta]:
    objs = []
    objs.append(create_macsec_profile_obj(MACsecProfile.get_cfg_table_name() + "|" + profile_name))
    return objs


def cache_find(cache: dict, target: MACsecAppMeta) -> MACsecAppMeta:
    if not cache or not cache["objs"]:
        return None
    for obj in cache["objs"]:
        if type(obj) == type(target) and obj.key == target.key:
            # MACsec SA may be refreshed by a cycle that use the same key
            # So, use the SA as the identifier
            if isinstance(obj, MACsecSA) and obj.sak != target.sak:
                continue
            return obj
    return None


@click.command()
@click.argument('interface_name', required=False)
@click.option('--profile', is_flag=True, required=False, default=False, help="show all macsec profiles")
@click.option('--dump-file', is_flag=True, required=False, default=False, help="store show output to a file")
@click.option('--post-status', is_flag=True, required=False, default=False, help="show macsec FIPS POST(Pre-Operational Self-Test) status")
@click.option('--fips-module', is_flag=True, required=False, default=False, help="show macsec FIPS module")
@click.option('--mka', is_flag=True, required=False, default=False, help="show MACsec MKA operational state")
@multi_asic_util.multi_asic_click_options
def macsec(interface_name, dump_file, namespace, display, profile, post_status,
           fips_module, mka):
    if post_status:
        if interface_name is not None or profile or dump_file or fips_module or mka:
            click.echo('POST status is not valid with other options/arguments')
            return
        MacsecContext(namespace, display).show_post_status()
        return
    if fips_module:
        if interface_name is not None or profile or dump_file or post_status or mka:
            click.echo('fips-module is not valid with other options/arguments')
            return
        MacsecContext(namespace, display).show_fips_module()
        return
    if mka:
        if profile or dump_file:
            click.echo('mka is not valid with profile or dump-file')
            return
        context = MacsecContext(namespace, display)
        context.collect_mka(interface_name)
        context.show_mka(interface_name)
        return
    if interface_name is not None and profile:
        click.echo('Interface name is not valid with profile option')
        return
    MacsecContext(namespace, display).show(interface_name, dump_file, profile)

class MacsecContext(object):

    def __init__(self, namespace_option, display_option):
        self.db = None
        self.multi_asic = multi_asic_util.MultiAsic(
            display_option, namespace_option)
        self.macsec_profiles = []
        self.mka_records = []

    @multi_asic_util.run_on_multi_asic
    def show(self, interface_name, dump_file, profile):
        global DB_CONNECTOR
        global COUNTER_TABLE
        DB_CONNECTOR = self.db

        if not profile:
            COUNTER_TABLE = CounterTable(self.db.get_redis_client(self.db.COUNTERS_DB))

            separator = self.db.get_db_separator(self.db.APPL_DB)
            port_key_prefix = MACsecPort.get_appl_table_name() + separator
            interface_names = [
                name[len(port_key_prefix):]
                for name in self.db.keys(self.db.APPL_DB, port_key_prefix + "*")
            ]
            if interface_name is not None:
                if interface_name not in interface_names:
                    return
                interface_names = [interface_name]
            objs = []

            for interface_name in natsorted(interface_names):
                objs += create_macsec_objs(interface_name)
        else:
            separator = self.db.get_db_separator(self.db.CONFIG_DB)
            profile_key_prefix = MACsecProfile.get_cfg_table_name() + separator
            profile_names = [
                name[len(profile_key_prefix):]
                for name in self.db.keys(self.db.CONFIG_DB, profile_key_prefix + "*")
            ]
            objs = []

            for profile_name in natsorted(profile_names):
                # Check if this macsec profile is already added to profile list. This is in case of
                # multi-asic devices where all namespaces will have the same macsec profile defined.
                if profile_name not in self.macsec_profiles and not dump_file:
                    self.macsec_profiles.append(profile_name)
                    objs += create_macsec_profiles_objs(profile_name)

        cache = {}
        if os.path.isfile(CACHE_FILE.format(self.multi_asic.current_namespace)):
            cache = pickle.load(open(CACHE_FILE.format(self.multi_asic.current_namespace), "rb"))

        if not dump_file:
            if cache and cache["time"] and objs:
                print("Last cached time was {}".format(cache["time"]))
            for obj in objs:
                cache_obj = cache_find(cache, obj)
                print(obj.dump_str(cache_obj))
        else:
            dump_obj = {
                "time": datetime.datetime.now(),
                "objs": objs
            }
            with open(CACHE_FILE.format(self.multi_asic.current_namespace), 'wb') as dump_file:
                pickle.dump(dump_obj, dump_file)
                dump_file.flush()

    @multi_asic_util.run_on_multi_asic
    def collect_mka(self, interface_name):
        separator = self.db.get_db_separator(self.db.STATE_DB)
        session_prefix = MKA_SESSION_TABLE + separator
        session_keys = self.db.keys(
            self.db.STATE_DB, session_prefix + "*"
        ) or []

        for session_key in natsorted(session_keys):
            current_interface = session_key[len(session_prefix):]
            if interface_name is not None and current_interface != interface_name:
                continue

            raw_session = self.db.get_all(self.db.STATE_DB, session_key)
            session = _allowlisted_fields(raw_session, MKA_SESSION_FIELDS)
            participant_prefix = separator.join(
                (MKA_PARTICIPANT_TABLE, current_interface)
            ) + separator
            participant_keys = self.db.keys(
                self.db.STATE_DB, participant_prefix + "*"
            ) or []
            participants = []
            for participant_key in natsorted(participant_keys):
                ckn = participant_key[len(participant_prefix):]
                raw_participant = self.db.get_all(
                    self.db.STATE_DB, participant_key
                )
                participant = _allowlisted_fields(
                    raw_participant, MKA_PARTICIPANT_FIELDS
                )
                participant["ckn"] = ckn
                participants.append(participant)

            port = self.config_db.get_entry("PORT", current_interface)
            profile_name = port.get("macsec") or session.get("profile")
            profile = (
                self.config_db.get_entry("MACSEC_PROFILE", profile_name)
                if profile_name else {}
            )
            encoded_secrets = [
                profile.get("primary_cak"),
                profile.get("fallback_cak"),
            ]
            secrets = []
            for secret in encoded_secrets:
                if not secret:
                    continue
                secrets.append(secret)
                if len(secret) in (66, 130):
                    secrets.append(secret[2:])
            self.mka_records.append({
                "namespace": self.multi_asic.current_namespace,
                "interface": current_interface,
                "session": session,
                "participants": participants,
                "secrets": secrets,
            })

    def show_mka(self, interface_name):
        records = natsorted(
            self.mka_records,
            key=lambda record: (record["interface"], record["namespace"] or ""),
        )
        if not records:
            if interface_name is None:
                click.echo("No MACsec MKA session state found")
            else:
                click.echo("MACsec MKA session state for {} is missing".format(interface_name))
            return

        if interface_name is None:
            self._show_mka_compact(records)
        else:
            self._show_mka_detail(records)

    def _show_mka_compact(self, records):
        show_namespace = any(record["namespace"] for record in records)
        rows = []
        for record in records:
            session = record["session"]
            principals = [
                participant for participant in record["participants"]
                if participant.get("is_principal") == "true"
            ]
            principal = principals[0] if len(principals) == 1 else {}
            principal_ckn = (
                "ambiguous" if len(principals) > 1
                else _compact_ckn(principal.get("ckn"), record["secrets"])
            )
            role = {
                "true": "primary",
                "false": "fallback",
            }.get(principal.get("is_primary"), "-")
            age = _age_seconds(session.get("last_updated"))
            row = [
                record["interface"],
                _safe_enum(session.get("kay_status"), ("active", "not-active")),
                _safe_bool(session.get("secured")),
                principal_ckn,
                role,
                _safe_uint(principal.get("live_peers")),
                _safe_sci(
                    session.get("key_server_sci"),
                    record["secrets"],
                    hide_zero=True,
                ),
                _safe_bool(session.get("is_key_server")),
                _compact_status(session, age),
                _age_label(age),
            ]
            if show_namespace:
                row.insert(0, record["namespace"] or "-")
            rows.append(row)

        headers = [
            "Interface",
            "KaY",
            "Secured",
            "Principal CKN",
            "Role",
            "Live",
            "Key-server SCI",
            "Local-KS",
            "Status",
            "Age",
        ]
        if show_namespace:
            headers.insert(0, "Namespace")
        click.echo(tabulate(
            rows,
            headers=headers,
        ))

    def _show_mka_detail(self, records):
        for index, record in enumerate(records):
            if index:
                click.echo("")
            if record["namespace"]:
                click.echo("Namespace:            {}".format(record["namespace"]))

            session = record["session"]
            _, freshness = _freshness(
                session.get("last_updated"), session.get("query_status")
            )
            fields = [
                ("Interface", record["interface"]),
                ("Profile", _redact_known_secrets(
                    session.get("profile", "-"), record["secrets"]
                )),
                ("PAE KaY status", _safe_enum(
                    session.get("kay_status"), ("active", "not-active")
                )),
                ("Controlled port mode", _controlled_port_mode(session)),
                ("Failed", _safe_bool(session.get("failed"))),
                ("Actor SCI", _safe_sci(
                    session.get("actor_sci"), record["secrets"]
                )),
                ("Key server SCI", _safe_sci(
                    session.get("key_server_sci"), record["secrets"]
                )),
                ("Actor priority", _safe_uint(session.get("actor_priority"))),
                ("Key server priority", _safe_uint(
                    session.get("key_server_priority")
                )),
                ("Local key server", _safe_bool(session.get("is_key_server"))),
                ("Keys distributed", _safe_uint(
                    session.get("keys_distributed")
                )),
                ("Keys received", _safe_uint(session.get("keys_received"))),
                ("MKA hello time", _format_milliseconds(
                    session.get("mka_hello_time_ms")
                )),
                ("Query status", _safe_enum(
                    session.get("query_status"), ("ok", "error")
                )),
                ("Config status", _safe_enum(
                    session.get("config_status"), ("in-sync", "degraded")
                )),
                ("Last updated", freshness),
            ]
            for label, value in fields:
                click.echo("{:<22} {}".format(label + ":", value))

            config_error = session.get("config_error")
            if config_error:
                click.echo("CONFIG ERROR:          {}".format(
                    _redact_known_secrets(config_error, record["secrets"])
                ))

            participant_rows = []
            for participant in record["participants"]:
                role = {
                    "true": "primary",
                    "false": "fallback",
                }.get(participant.get("is_primary"), "-")
                participant_rows.append([
                    _safe_hex(
                        participant.get("ckn"), (32, 64), record["secrets"]
                    ),
                    role,
                    _safe_bool(participant.get("is_principal")),
                    _safe_bool(participant.get("active")),
                    _safe_uint(participant.get("live_peers")),
                    _safe_uint(participant.get("potential_peers")),
                    _safe_bool(participant.get("is_key_server")),
                    _safe_bool(participant.get("is_elected")),
                    _safe_hex(
                        participant.get("mi"), (24,), record["secrets"]
                    ),
                    _safe_uint(participant.get("mn")),
                ])

            click.echo("")
            if participant_rows:
                click.echo(tabulate(
                    participant_rows,
                    headers=[
                        "CKN",
                        "Role",
                        "Principal",
                        "Active",
                        "Live",
                        "Potential",
                        "Key-server",
                        "Elected",
                        "MI",
                        "MN",
                    ],
                ))
            else:
                click.echo("No MACsec MKA participant state found")

    @multi_asic_util.run_on_multi_asic
    def show_post_status(self):
        """Show POST (Pre-Operational Self-Test) status"""
        # Define the table name
        table_name = "FIPS_MACSEC_POST_TABLE"

        def format_module_status(module, namespace):
            # Get all fields from the table
            post_data = state_db.get_all("STATE_DB", table_name+"|"+module)

            # Format according to SONiC CLI guidelines
            output = []
            indent = "  " if namespace else ""
            output.append(f"{indent}{'Module'.ljust(11)} : {module}")

            for field in post_data:
                value = post_data[field]
                # Format field name for consistent alignment (capitalize and pad to 11 chars)
                display_name = field.capitalize().ljust(11)
                output.append(f"{indent}{display_name} : {value}")

            return "\n".join(output)

        namespace = self.multi_asic.current_namespace
        # Connect to STATE_DB
        state_db = SonicV2Connector(use_unix_socket_path=True, namespace=namespace)
        state_db.connect(state_db.STATE_DB)

        # Get all keys in the FIPS_MACSEC_POST_TABLE
        all_keys = state_db.keys(state_db.STATE_DB, table_name + "|*")
        if not len(all_keys):
            click.echo("")  # Add blank line for separation
            if namespace:
                click.echo(f"Namespace ({namespace})")
                click.echo("  No entries found")
            else:
                click.echo("No entries found")
            return

        # Extract module names from the keys and sort them
        modules = []
        for key in all_keys:
            # Key format is "FIPS_MACSEC_POST_TABLE|module_name"
            if "|" in key:
                module = key.split("|", 1)[1]
                modules.append(module)

        # Sort modules for consistent output
        modules.sort()

        display_output = [""]
        if namespace:
            display_output.append(f"Namespace ({namespace})")
        for i, module in enumerate(modules):
            if i > 0:
                display_output.append("")  # Add separator between modules
            module_output = format_module_status(module, namespace)
            display_output.append(module_output)

        if display_output:
            click.echo("\n".join(display_output))

    def show_fips_module(self):
        json_file = device_info.get_path_to_platform_dir() + '/' + device_info.PLATFORM_JSON_FILE
        if not os.path.exists(json_file):
            return
        try:
            with open(json_file, 'r') as file:
                platform_data = json.load(file)
        except (json.JSONDecodeError, IOError, TypeError, ValueError):
            return
        if 'fips_module' in platform_data:
            click.echo(platform_data['fips_module'])

def register(cli):
    cli.add_command(macsec)


if __name__ == '__main__':
    macsec(None)
