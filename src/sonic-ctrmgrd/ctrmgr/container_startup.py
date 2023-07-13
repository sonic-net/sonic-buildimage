#!/usr/bin/env python3

import argparse
import datetime
import inspect
import json
import syslog
import time

from swsscommon import swsscommon
from sonic_py_common.general import getstatusoutput_noshell_pipe

# DB field names
SET_OWNER = "set_owner"

CURRENT_OWNER = "current_owner"
UPD_TIMESTAMP = "update_time"
DOCKER_ID = "container_id"
REMOTE_STATE = "remote_state"
VERSION = "container_version"
SYSTEM_STATE = "system_state"

KUBE_LABEL_TABLE = "KUBE_LABELS"
KUBE_LABEL_SET_KEY = "SET"

UNIT_TESTING = 0


def debug_msg(m):
    msg = "{}: {}".format(inspect.stack()[1][3], m)
    syslog.syslog(syslog.LOG_DEBUG, msg)


def _get_version_key(feature, version):
    # Coin label for version control
    return "{}_{}_enabled".format(feature, version)


def _get_local_version_key(feature):
    # Coin label for track laster local version
    return "{}_local".format(feature)


def read_data(feature):
    state_data = {
            CURRENT_OWNER: "none",
            UPD_TIMESTAMP: "",
            DOCKER_ID: "",
            REMOTE_STATE: "none",
            VERSION: "0.0.0",
            SYSTEM_STATE: ""
            }

    set_owner = "local"

    # read owner from config-db and current state data from state-db.
    db = swsscommon.DBConnector("CONFIG_DB", 0)
    tbl = swsscommon.Table(db, 'FEATURE')
    data = dict(tbl.get(feature)[1])

    if (SET_OWNER in data):
        set_owner = data[SET_OWNER]

    state_db = swsscommon.DBConnector("STATE_DB", 0)
    tbl = swsscommon.Table(state_db, 'FEATURE')
    state_data.update(dict(tbl.get(feature)[1]))

    return (state_db, set_owner, state_data)


def read_fields(state_db, feature, fields):
    # Read directly from STATE-DB, given fields
    # for given feature.
    # Fields is a list of tuples (<field name>, <default val>)
    #
    tbl = swsscommon.Table(state_db, 'FEATURE')
    ret = []

    # tbl.get for non-existing feature would return
    # [False, {} ]
    #
    data = dict(tbl.get(feature)[1])
    for (field, default) in fields:
        val = data[field] if field in data else default
        ret += [val]

    return tuple(ret)


def check_version_blocked(state_db, feature, version):
    # Ensure this version is *not* blocked explicitly.
    #
    tbl = swsscommon.Table(state_db, KUBE_LABEL_TABLE)
    labels = dict(tbl.get(KUBE_LABEL_SET_KEY)[1])
    key = _get_local_version_key(feature)
    return (key in labels) and (labels[key].lower() == version.lower())


def drop_label(state_db, feature, version):
    # Mark given feature version as dropped in labels.
    # Update is done in state-db.
    # ctrmgrd sets it with kube API server per reaschability

    tbl = swsscommon.Table(state_db, KUBE_LABEL_TABLE)
    name = _get_local_version_key(feature)
    tbl.set(KUBE_LABEL_SET_KEY, [(name, version)])


def update_data(state_db, feature, data):
    # Update STATE-DB entry for this feature with given data
    #
    debug_msg("{}: {}".format(feature, str(data)))
    tbl = swsscommon.Table(state_db, "FEATURE")
    tbl.set(feature, list(data.items()))


def get_docker_id():
    # Read the container-id
    # Note: This script runs inside the context of container
    #
    cmd0 = ['cat', '/proc/self/cgroup']
    cmd1 = ['grep', '-e', ":memory:"]
    cmd2 = ['rev']
    cmd3 = ['cut', '-f1', '-d', '/']
    cmd4 = ['rev']
    _, output = getstatusoutput_noshell_pipe(cmd0, cmd1, cmd2, cmd3, cmd4)
    return output.strip()[:12]


def instance_higher(feature, ct_version, version):
    # Compares given version against current version in STATE-DB.
    # Return True if this version is higher than current.
    #
    ret = False
    if ct_version:
        ct = ct_version.split('.')
        nxt = version.split('.')
        for cs, ns in zip(ct, nxt):
            c = int(cs)
            n = int(ns)
            if n < c:
                break
            elif n > c:
                ret = True
                break
    else:
        # Empty version implies no one is running.
        ret = True

    debug_msg("compare version: new:{} current:{} res={}".format(
        version, ct_version, ret))
    return ret


def is_active(feature, system_state):
    # Check current system state of the feature
    if system_state == "up":
        return True
    else:
        syslog.syslog(syslog.LOG_ERR, "Found inactive for {}".format(feature))
        return False


def update_state(state_db, feature, owner=None, version=None):
    """
    sets owner, version & container-id for this feature in state-db.

    If owner is local, update label to block remote deploying same version or
    if kube, sets state to "running".

    """
    data = {
            CURRENT_OWNER: owner,
            DOCKER_ID: get_docker_id() if owner != "local" else feature,
            UPD_TIMESTAMP: str(datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S")),
            "hello": "world",
            VERSION: version
            }

    if (owner == "local"):
        # Disable deployment of this version as available locally
        drop_label(state_db, feature, version)
    else:
        data[REMOTE_STATE] = "running"

    debug_msg("{} up data:{}".format(feature, str(data)))
    update_data(state_db, feature,  data)


def do_freeze(feat, m):
    # Exiting will kick off the container to run.
    # So sleep forever with periodic logs.
    #
    while True:
        syslog.syslog(syslog.LOG_ERR, "Blocking .... feat:{} docker_id:{} msg:{}".format(
            feat, get_docker_id(), m))
        if UNIT_TESTING:
            break
        time.sleep(60)


def container_up(feature, owner, version):
    """
    This is called by container upon post start.

    The container will run its application, only upon this call
    complete.

    This call does the basic check for if this starting-container can be allowed
    to run based on current state, and owner & version of this starting
    container.

    If allowed to proceed, this info is recorded in state-db and return
    to enable container start the main application. Else it proceeds to
    sleep forever, blocking the container from starting the main application.

    """
    debug_msg("BEGIN")
    (state_db, set_owner, state_data) = read_data(feature)

    debug_msg("args: feature={}, owner={}, version={} DB: set_owner={} state_data={}".format(
        feature, owner, version, set_owner, json.dumps(state_data, indent=4)))

    if state_data[SYSTEM_STATE] == '':
        return

    if owner == "local":
        update_state(state_db, feature, owner, version)
    else:
        if (set_owner == "local"):
            do_freeze(feature, "bail out as set_owner is local")
            return

        if not is_active(feature, state_data[SYSTEM_STATE]):
            do_freeze(feature, "bail out as system state not active")
            return

        if check_version_blocked(state_db, feature, version):
            do_freeze(feature, "This version is marked disabled. Exiting ...")
            return

        update_data(state_db, feature, { VERSION: version })

        mode = state_data[REMOTE_STATE]
        if mode in ("none", "running", "stopped"):
            update_data(state_db, feature, { REMOTE_STATE: "pending" })
            mode = "pending"
        else:
            debug_msg("{}: Skip remote_state({}) update".format(feature, mode))


        i = 0
        while (mode != "ready"):
            if (i % 10) == 0:
                debug_msg("{}: remote_state={}. Waiting to go ready".format(feature, mode))
            i += 1

            time.sleep(2)
            mode, db_version = read_fields(state_db,
                    feature, [(REMOTE_STATE, "none"), (VERSION, "")])
            if version != db_version:
                # looks like another instance has overwritten. Exit for now.
                # If this happens to be higher version, next deploy by kube will fix
                # This is a very rare window of opportunity, for this version to be higher.
                #
                do_freeze(feature,
                        "bail out as current deploy version={} is different than {}. re-deploy higher one".
                        format(version, db_version))
                return
            if UNIT_TESTING:
                return


        update_state(state_db, feature, owner, version)

    debug_msg("END")


def main():
    parser = argparse.ArgumentParser(description="container_startup <feature> kube/local [<version>]")

    parser.add_argument("-f", "--feature", required=True)
    parser.add_argument("-o", "--owner", choices=["local", "kube"], required=True)
    parser.add_argument("-v", "--version", required=True)

    args = parser.parse_args()
    container_up(args.feature, args.owner, args.version)



if __name__ == "__main__":
    main()
