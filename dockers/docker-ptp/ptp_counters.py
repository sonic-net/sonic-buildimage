#!/usr/bin/env python3
"""
Poll ptp4l PORT_STATS_NP and PORT_SERVICE_STATS_NP via pmc and publish
per-interface counters to COUNTERS_DB.

Redis key: COUNTERS_PTP:<interface>
Hash fields match linuxptp pmc output (rx_Sync, tx_Delay_Req, ...).
"""

import os
import re
import subprocess
import sys
import time

PTP_TABLE = "PTP"
PTP_PORT_TABLE = "PTP_PORT"
COUNTERS_PTP_TABLE = "COUNTERS_PTP"
PTP4L_CONF = "/etc/ptp4l/ptp4l.conf"
PTP4L_UDS = "/var/run/ptp4l"
PMC_TIMEOUT_SEC = 5
KV_RE = re.compile(r"^\s+([A-Za-z0-9_.]+)\s+(\S.*)$")
IFACE_SECTION_RE = re.compile(r"^\[([^\]]+)\]\s*$")
DOMAIN_RE = re.compile(r"^\s*domainNumber\s+(\d+)\s*$")

STAT_FIELDS = (
    "rx_Sync",
    "rx_Delay_Req",
    "rx_Pdelay_Req",
    "rx_Pdelay_Resp",
    "rx_Follow_Up",
    "rx_Delay_Resp",
    "rx_Pdelay_Resp_Follow_Up",
    "rx_Announce",
    "rx_Signaling",
    "rx_Management",
    "tx_Sync",
    "tx_Delay_Req",
    "tx_Pdelay_Req",
    "tx_Pdelay_Resp",
    "tx_Follow_Up",
    "tx_Delay_Resp",
    "tx_Pdelay_Resp_Follow_Up",
    "tx_Announce",
    "tx_Signaling",
    "tx_Management",
)

# linuxptp pmc GET PORT_SERVICE_STATS_NP
SERVICE_STAT_FIELDS = (
    "announce_timeout",
    "sync_timeout",
    "delay_timeout",
    "unicast_service_timeout",
    "unicast_request_timeout",
    "master_announce_timeout",
    "master_sync_timeout",
    "qualification_timeout",
    "sync_mismatch",
    "followup_mismatch",
)


def log(msg):
    print(msg, flush=True)


def parse_pmc_records(text):
    """Parse pmc TLV dumps into a list of {field: value} dicts."""
    records = []
    current = {}
    for line in text.splitlines():
        match = KV_RE.match(line)
        if not match:
            continue
        key, value = match.group(1), match.group(2).strip()
        if key == "portIdentity" and current:
            records.append(current)
            current = {}
        current[key] = value
    if current:
        records.append(current)
    return records


def port_number_from_identity(port_identity):
    if not port_identity or "-" not in port_identity:
        return None
    tail = port_identity.rsplit("-", 1)[-1]
    return tail if tail.isdigit() else None


def interfaces_from_ptp4l_conf(path=PTP4L_CONF):
    """Interface sections in ptp4l.conf, in file order (ptp4l port 1..N)."""
    if not os.path.exists(path):
        return []
    skip = {"global", "unicast_master_table"}
    names = []
    with open(path, encoding="utf-8") as handle:
        for line in handle:
            match = IFACE_SECTION_RE.match(line.strip())
            if not match:
                continue
            name = match.group(1)
            if name.lower() in skip or name.lower().startswith("unicast_master_table"):
                continue
            names.append(name)
    return names


def domain_from_ptp4l_conf(path=PTP4L_CONF):
    if not os.path.exists(path):
        return None
    with open(path, encoding="utf-8") as handle:
        for line in handle:
            match = DOMAIN_RE.match(line)
            if match:
                return int(match.group(1))
    return None


def run_pmc(domain, tlv):
    # linuxptp pmc treats each argv word as a command; GET and the TLV
    # must be one argument: pmc -u -d 44 "GET PORT_STATS_NP"
    cmd = ["pmc", "-u", "-s", PTP4L_UDS, "-d", str(domain), "GET " + tlv]
    result = subprocess.run(
        cmd,
        capture_output=True,
        text=True,
        timeout=PMC_TIMEOUT_SEC,
        check=False,
    )
    out = (result.stdout or "") + (result.stderr or "")
    if result.returncode != 0:
        log("ptp_counters: pmc {} rc={} out={}".format(
            tlv, result.returncode, out.strip()[:300]))
    return out


def domain_number(cfgdb):
    conf_domain = domain_from_ptp4l_conf()
    if conf_domain is not None:
        return conf_domain
    entry = cfgdb.get_entry(PTP_TABLE, "global") or {}
    value = entry.get("domain_number", "0")
    try:
        return int(value)
    except (TypeError, ValueError):
        return 0


def _fields_from_record(record, field_names):
    fields = {}
    pid = record.get("portIdentity", "")
    if pid:
        fields["portIdentity"] = pid
    for field in field_names:
        fields[field] = record.get(field, "0")
    return fields


def interface_for_stats(stats, if_by_pid, port_ifaces):
    pid = stats.get("portIdentity")
    ifname = if_by_pid.get(pid)
    if ifname:
        return ifname
    port_no = port_number_from_identity(pid)
    if port_no and port_ifaces:
        idx = int(port_no) - 1
        if 0 <= idx < len(port_ifaces):
            return port_ifaces[idx]
    if port_no:
        return "port-{}".format(port_no)
    return None


def publish_once(cfgdb, counters_db):
    domain = domain_number(cfgdb)
    props_text = run_pmc(domain, "PORT_PROPERTIES_NP")
    stats_text = run_pmc(domain, "PORT_STATS_NP")
    service_text = run_pmc(domain, "PORT_SERVICE_STATS_NP")

    if_by_pid = {}
    for record in parse_pmc_records(props_text):
        pid = record.get("portIdentity")
        iface = record.get("interface")
        if pid and iface:
            if_by_pid[pid] = iface

    port_ifaces = interfaces_from_ptp4l_conf()
    if not port_ifaces:
        port_ifaces = sorted(str(k) for k in (cfgdb.get_keys(PTP_PORT_TABLE) or []))

    by_iface = {}
    stats_records = parse_pmc_records(stats_text)
    service_records = parse_pmc_records(service_text)

    for stats in stats_records:
        ifname = interface_for_stats(stats, if_by_pid, port_ifaces)
        if not ifname:
            log("ptp_counters: dropping PORT_STATS_NP with no interface map: {}".format(
                stats.get("portIdentity")))
            continue
        by_iface.setdefault(ifname, {}).update(_fields_from_record(stats, STAT_FIELDS))

    for service in service_records:
        ifname = interface_for_stats(service, if_by_pid, port_ifaces)
        if not ifname:
            log("ptp_counters: dropping PORT_SERVICE_STATS_NP with no interface map: {}".format(
                service.get("portIdentity")))
            continue
        by_iface.setdefault(ifname, {}).update(
            _fields_from_record(service, SERVICE_STAT_FIELDS))

    sep = counters_db.get_db_separator(counters_db.COUNTERS_DB)
    now = str(int(time.time()))
    published = 0

    for ifname, fields in by_iface.items():
        fields["last_update"] = now
        key = "{}{}{}".format(COUNTERS_PTP_TABLE, sep, ifname)
        counters_db.hmset(counters_db.COUNTERS_DB, key, fields)
        published += 1

    if published == 0:
        log("ptp_counters: published 0 (domain={} props={} stats={} service={} ifaces={})".format(
            domain, len(if_by_pid), len(stats_records), len(service_records),
            port_ifaces))
        if stats_text.strip():
            log("ptp_counters: pmc PORT_STATS_NP snippet: {}".format(
                stats_text.strip()[:400]))
        if service_text.strip():
            log("ptp_counters: pmc PORT_SERVICE_STATS_NP snippet: {}".format(
                service_text.strip()[:400]))
    else:
        log("ptp_counters: published {} port(s) domain={}".format(published, domain))

    return published


def main():
    from swsscommon.swsscommon import ConfigDBConnector, SonicV2Connector

    interval = int(os.environ.get("PTP_COUNTERS_INTERVAL", "2"))
    cfgdb = ConfigDBConnector(use_unix_socket_path=True)
    cfgdb.connect()
    counters_db = SonicV2Connector(use_unix_socket_path=True)
    counters_db.connect(counters_db.COUNTERS_DB)

    log("ptp_counters: publishing PORT_STATS_NP and PORT_SERVICE_STATS_NP "
        "to COUNTERS_DB every {}s".format(interval))
    while True:
        try:
            publish_once(cfgdb, counters_db)
        except subprocess.TimeoutExpired:
            log("ptp_counters: pmc timed out")
        except Exception as exc:
            log("ptp_counters: {}".format(exc))
        time.sleep(interval)


if __name__ == "__main__":
    main()
