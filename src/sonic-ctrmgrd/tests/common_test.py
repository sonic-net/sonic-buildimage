import copy
import json
import os
import subprocess
import sys
import time


CONFIG_DB_NO = 4
STATE_DB_NO = 6
FEATURE_TABLE = "FEATURE"
MGMT_INTERFACE_TABLE = "MGMT_INTERFACE"
KUBE_LABEL_TABLE = "KUBE_LABELS"
KUBE_LABEL_SET_KEY = "SET"

SERVER_TABLE = "KUBERNETES_MASTER"
SERVER_KEY = "SERVER"
CFG_SER_IP = "ip"
CFG_SER_PORT = "port"
CFG_SER_INSECURE = "insecure"
CFG_SER_DISABLE = "disable"


PRE = "pre_test"
UPD = "update_test"
POST = "post_test"
ACTIONS = "container_actions"
RETVAL = "return_value"
ARGS = "args"
NO_INIT = "no_init"
DESCR = "description"
TRIGGER_THROW = "throw_on_call"
TRIGGER_GET_THROW = "throw_on_get"
TRIGGER_RM_THROW = "throw_on_rm"
ACTIVE = "active"
CONNECTED = "remote_connected"
KUBE_CMD = "kube_commands"
KUBE_JOIN = "join"
KUBE_RESET = "reset"
KUBE_WR = "write"
KUBE_RETURN = "kube_return"
IMAGE_TAG = "image_tag"
FAIL_LOCK = "fail_lock"
DO_JOIN = "do_join"
REQ = "req"

# subproc key words

# List all subprocess commands expected within the test.
# Each call to subproc-side effect (mock_subproc_side_effect) increment index
# Other key words influence how this proc command to be processed
# PROC_RUN having true at that index, implies run it instead of mocking it
# PROC_OUT, ERR, FAIL THROW provide data on how to mock
#
PROC_CMD = "subproc_cmd"
PROC_RUN = "skip_mock"
PROC_FAIL = "proc_fail"
PROC_THROW = "proc_throw"
PROC_OUT = "subproc_output"
PROC_ERR = "subproc_error"
PROC_CODE = "subproc_code"
PROC_KILLED = "procs_killed"

# container_start test cases
# test case 0 -- container start local
# test case 1 -- container start kube with fallback
# test case 2 -- container start kube with fallback but remote_state != none
#
sample_test_data = {
    0: {
        PRE: {
            CONFIG_DB_NO: {
                FEATURE_TABLE: {
                    "snmp": {
                        "set_owner": "local"
                    }
                }
            },
            STATE_DB_NO: {
                FEATURE_TABLE: {
                    "snmp": {
                        "remote_state": "xxx"
                    }
                }
            }
        },
        UPD: {
            CONFIG_DB_NO: {
                FEATURE_TABLE: {
                    "snmp": {
                        "set_owner": "local"
                    }
                }
            },
            STATE_DB_NO: {
                FEATURE_TABLE: {
                    "snmp": {
                        "remote_state": "xxx"
                    }
                }
            }
        },
        POST: {
            CONFIG_DB_NO: {
                FEATURE_TABLE: {
                    "snmp": {
                        "set_owner": "local"
                    }
                }
            },
            STATE_DB_NO: {
                FEATURE_TABLE: {
                    "snmp": {
                        "remote_state": "none",
                        "system_state": "up",
                        "current_owner": "local",
                        "container_id": "snmp"
                    }
                },
                KUBE_LABEL_TABLE: {
                    "SET": {
                        "snmp_enabled": "false"
                    }
                }
            }
        },
        ACTIONS: {
            "snmp": [ "start" ]
        }
    }
}


current_test_name = None
current_test_no = None
current_test_data = None

tables_returned = {}
mock_containers = {}

selector_returned = None
subscribers_returned = {}

kube_actions = {}


def do_start_test(tname, tno, ctdata):
    global current_test_name, current_test_no, current_test_data
    global tables_returned, mock_containers, selector_returned
    global subscribers_returned, kube_actions

    current_test_name = tname
    current_test_no = tno
    current_test_data = ctdata
    tables_returned = {}
    mock_containers = {}

    selector_returned = None
    subscribers_returned = {}
    kube_actions = {}

    mock_procs_init()
    print("Starting test case {} number={}".format(tname, tno))


class mock_image:
    def __init__(self, actions):
        self.actions = actions


    def tag(self, image_name, image_tag, force):
        print("mock_image: tag name={} tag={} force={}".format(image_name, image_tag, force))
        d = {}
        d[IMAGE_TAG] = {
                "image_name": image_name,
                "image_tag": image_tag,
                "force": force
                }
        self.actions.append(d)


class mock_container:

    def __init__(self, name):
        self.actions = []
        self.name = name
        self.image = mock_image(self.actions)
        self.attrs = {"Config": {"Env": ["IMAGE_VERSION=20201231.11"]}}


    def start(self):
        self.actions.append("start")


    def stop(self):
        self.actions.append("stop")


    def kill(self):
        self.actions.append("kill")


    def wait(self):
        self.actions.append("wait")


    def remove(self):
        if self.name == TRIGGER_RM_THROW:
            raise Exception("container {} can't be removed".format(self.name))
        else:
            self.actions.append("remove")


    def kill(self):
        self.actions.append("kill")


class mock_container_list:

    def get(self, feature):
        if feature == TRIGGER_GET_THROW:
            raise Exception("container not found")
        else:
            if not feature in mock_containers:
                mock_containers[feature] = mock_container(feature)
            return mock_containers[feature]


class dock_client:
    def __init__(self):
        self.containers = mock_container_list()


def docker_from_env_side_effect():
    return dock_client()


def mock_os_execv(cmd, args):
    # global current_test_name, current_test_no, current_test_data
    global mock_containers

    if cmd == "/usr/bin/docker-rs" and len(args) == 3 and args[1] == "wait":
        container_name = args[2]
        if container_name in mock_containers:
            mock_containers[container_name].wait()
            return 0

    print("Unexpected command in mock_os_execv: cmd={} args={}".format(cmd, args))
    return -1


def check_mock_containers():
    global current_test_data

    expected = current_test_data.get(ACTIONS, {})
    found = {}
    for k in mock_containers:
        found[k] = mock_containers[k].actions

    if expected != found:
        print("Failed test={} no={}".format(current_test_name, current_test_no))
        print("expected: {}".format(json.dumps(expected, indent=4)))
        print("found: {}".format(json.dumps(found, indent=4)))
        return -1
    return 0


def check_subset(d_sub, d_all):
    if type(d_sub) != type(d_all):
        return -1
    if not type(d_sub) is dict:
        ret = 0 if d_sub == d_all else -2
        return ret

    for (k, v) in d_sub.items():
        if not k in d_all:
            return -3
        ret = check_subset(v, d_all[k])
        if ret != 0:
            return ret
    return 0


def recursive_update(d, t):
    assert (type(t) is dict)
    for k in t.keys():
        if type(t[k]) is not dict:
            d.update(t)
            return

        if k not in d:
            d[k] = {}
        recursive_update(d[k], t[k])


class Table:

    def __init__(self, db, tbl):
        self.db = db
        self.tbl = tbl
        self.data = copy.deepcopy(self.get_val(current_test_data[PRE], [db, tbl]))
        # print("Table:init: db={} tbl={} data={}".format(db, tbl, json.dumps(self.data, indent=4)))


    def update(self):
        t = copy.deepcopy(self.get_val(current_test_data.get(UPD, {}),
            [self.db, self.tbl]))
        if t:
            recursive_update(self.data, t)
            return True
        return False


    def get_val(self, d, keys):
        for k in keys:
            d = d[k] if k in d else {}
        return d


    def getKeys(self):
        return list(self.data.keys())


    def get(self, key):
        ret = copy.deepcopy(self.data.get(key, {}))
        return (True, ret)


    def set(self, key, items):
        if key not in self.data:
            self.data[key] = {}
        d = self.data[key]
        for (k, v) in items:
            d[k] = v


    def check(self):
        expected = self.get_val(current_test_data, [POST, self.db, self.tbl])

        ret = check_subset(expected, self.data)

        if ret != 0:
            print("Failed test={} no={} ret={}".format(
                current_test_name, current_test_no, ret))
            print("Found: {}".format(json.dumps(self.data, indent=4)))
            print("expect: {}".format(json.dumps(expected, indent=4)))
            return ret
        return 0


db_conns = {"CONFIG_DB": CONFIG_DB_NO, "STATE_DB": STATE_DB_NO}
def conn_side_effect(arg, _):
    return db_conns[arg]


def table_side_effect(db, tbl):
    if not db in tables_returned:
        tables_returned[db] = {}
    if not tbl in tables_returned[db]:
        tables_returned[db][tbl] = Table(db, tbl)
    return tables_returned[db][tbl]


def check_tables_returned():
    for d in tables_returned:
        for t in tables_returned[d]:
            ret = tables_returned[d][t].check()
            if ret != 0:
                return ret
    return 0


class mock_selector:
    SLEEP_SECS = 0
    TIMEOUT = 1
    ERROR = 2
    MAX_CNT = 4

    def __init__(self):
        self.select_state = 0
        self.select_cnt = 0
        # print("Mock Selector constructed")


    def addSelectable(self, subs):
        return 0


    def select(self, timeout):
        # Toggle between good & timeout
        #
        state = self.select_state

        if self.select_state == 0:
            if self.SLEEP_SECS:
                time.sleep(self.SLEEP_SECS)
            self.select_state = self.TIMEOUT
        else:
            self.select_state = 0

        self.select_cnt += 1
        if self.select_cnt > self.MAX_CNT:
            state = self.ERROR


        print("select return ({}, None)".format(state))
        return (state, None)


class mock_db_conn:
    def __init__(self, db):
        self.db_name = None
        for (k, v) in db_conns.items():
            if v == db:
                self.db_name = k
        assert self.db_name != None

    def getDbName(self):
        return self.db_name


class mock_subscriber:
    def __init__(self, db, tbl):
        self.state = PRE
        self.db = db
        self.tbl = tbl
        self.dbconn = mock_db_conn(db)


    def pop(self):
        # Handles only one key; Good enough for our requirements.
        mock_tbl = table_side_effect(self.db, self.tbl)
        keys = []
        if self.state == PRE:
            self.state = UPD
            keys = list(mock_tbl.data.keys())
        elif self.state == UPD:
            if mock_tbl.update():
                keys = list(mock_tbl.data.keys())
            self.state = POST

        if keys:
            key = keys[0]
            return (key, "", mock_tbl.get(key)[1])
        else:
            return ("", "", {})


    def getDbConnector(self):
        return self.dbconn


    def getTableName(self):
        return self.tbl


def subscriber_side_effect(db, tbl):
    global subscribers_returned

    key = "db_{}_tbl_{}".format(db, tbl)
    if not key in subscribers_returned:
        subscribers_returned[key] = mock_subscriber(db, tbl)
    return subscribers_returned[key]


def select_side_effect():
    global selector_returned

    if not selector_returned:
        selector_returned = mock_selector()
    return selector_returned


def set_mock(mock_table, mock_conn, mock_docker=None):
    mock_conn.side_effect = conn_side_effect
    mock_table.side_effect = table_side_effect
    if mock_docker != None:
        mock_docker.side_effect = docker_from_env_side_effect


def set_mock_sel(mock_sel, mock_subs):
    mock_sel.side_effect = select_side_effect
    mock_subs.side_effect = subscriber_side_effect


kube_return = 0

def kube_labels_side_effect(labels):
    global kube_actions, kube_return

    if not KUBE_CMD in kube_actions:
        kube_actions[KUBE_CMD] = {}

    if not kube_return:
        if not KUBE_WR in kube_actions[KUBE_CMD]:
            kube_actions[KUBE_CMD][KUBE_WR] = {}

        kube_actions[KUBE_CMD][KUBE_WR].update(labels)
        kube_return = 1
        return 0
    else:
        kube_return = 0
        return 1


def kube_join_side_effect(ip, port, insecure):
    global kube_actions, kube_return

    if not KUBE_CMD in kube_actions:
        kube_actions[KUBE_CMD] = {}

    if not kube_return:
        if not KUBE_JOIN in kube_actions[KUBE_CMD]:
            kube_actions[KUBE_CMD][KUBE_JOIN] = {}

        kube_actions[KUBE_CMD][KUBE_JOIN][CFG_SER_IP] = ip
        kube_actions[KUBE_CMD][KUBE_JOIN][CFG_SER_PORT] = port
        kube_actions[KUBE_CMD][KUBE_JOIN][CFG_SER_INSECURE] = insecure
        kube_return = 1
        return (0, "joined", "no error")
    else:
        kube_return = 0
        return (1, "not joined", "error")


def kube_reset_side_effect(flag):
    global kube_actions

    if not KUBE_CMD in kube_actions:
        kube_actions[KUBE_CMD] = {}

    if not KUBE_RESET in kube_actions[KUBE_CMD]:
        kube_actions[KUBE_CMD][KUBE_RESET] = {}

    kube_actions[KUBE_CMD][KUBE_RESET]["flag"] = (
            "true" if flag else "false")
    return 0


def check_kube_actions():
    global kube_actions

    ret = 0
    expected = {}
    expected[KUBE_CMD] = current_test_data.get(KUBE_CMD, {})

    if expected[KUBE_CMD]:
        ret = check_subset(expected, kube_actions)

    if ret != 0:
        print("Failed test={} no={} ret={}".format(
            current_test_name, current_test_no, ret))
        print("Found: {}".format(json.dumps(kube_actions, indent=4)))
        print("expect: {}".format(json.dumps(expected, indent=4)))
        return -1
    return 0


def set_mock_kube(kube_labels, kube_join, kube_reset):
    kube_labels.side_effect = kube_labels_side_effect
    kube_join.side_effect = kube_join_side_effect
    kube_reset.side_effect = kube_reset_side_effect


def clean_image_side_effect(feat, current_version, last_version):
    return 0


def tag_latest_side_effect(feat, docker_id, image_ver):
    return 0


def set_mock_image_op(clean_image, tag_latest):
    clean_image.side_effect = clean_image_side_effect
    tag_latest.side_effect = tag_latest_side_effect


def str_comp(needle, hay):
    nlen = len(needle)
    hlen = len(hay)

    if needle and (needle[-1] == '*'):
        nlen -= 1
        hlen = nlen

    return (nlen == hlen) and (needle[0:nlen] == hay[0:nlen])


def check_mock_proc(index, tag):
    lst_run = current_test_data.get(tag, [])
    return lst_run[index] if len(lst_run) > index else False


class mock_proc:
    def __init__(self, cmd, index):
        self.index = index
        lst_cmd = current_test_data.get(PROC_CMD, [])
        assert len(lst_cmd) > index
        expect = lst_cmd[index]
        self.cmd = cmd
        self.returncode = 0
        if not str_comp(expect, cmd):
            print("PROC_CMD != cmd")
            print("PROC_CMD={}".format(expect))
            print("MIS  cmd={}".format(cmd))
            assert False

        self.skip_mock = check_mock_proc(index, PROC_RUN)
        self.fail_mock = check_mock_proc(index, PROC_FAIL)
        self.trigger_throw = check_mock_proc(index, PROC_THROW)


    def do_run(self, tout):
        print("do_run: {}".format(self.cmd))
        self.returncode = os.system(self.cmd)
        return ("", "")


    def communicate(self, timeout):
        if self.trigger_throw:
            raise IOError()

        if self.fail_mock:
            self.returncode = -1
            return ("", "{} failed".format(self.cmd))

        if self.skip_mock:
            return self.do_run(timeout)

        do_throw = current_test_data.get(TRIGGER_THROW, False)
        if do_throw:
            self.returncode = -1
            raise subprocess.TimeoutExpired(self.cmd, timeout)

        out_lst = current_test_data.get(PROC_OUT, None)
        err_lst = current_test_data.get(PROC_ERR, None)
        code_lst = current_test_data.get(PROC_CODE, None)
        if out_lst:
            assert (len(out_lst) > self.index)
            out = out_lst[self.index]
        else:
            out = ""
        if err_lst:
            assert (len(err_lst) > self.index)
            err = err_lst[self.index]
        else:
            err = ""
        if code_lst:
            assert (len(code_lst) > self.index)
            self.returncode = code_lst[self.index]
        else:
            self.returncode = 0 if not err else -1
        return (out, err)

    def kill(self):
        global procs_killed

        procs_killed += 1


procs_killed = 0
procs_index = 0

def mock_procs_init():
    global procs_killed, procs_index

    procs_killed = 0
    procs_index = 0


def mock_subproc_side_effect(cmd, shell=False, stdout=None, stderr=None):
    global procs_index

    assert shell == True
    assert stdout == subprocess.PIPE
    assert stderr == subprocess.PIPE
    index = procs_index
    procs_index += 1
    return mock_proc(cmd, index)


class mock_reqget:
    def __init__(self):
        self.ok = True

    def json(self):
        return current_test_data.get(REQ, "")


def mock_reqget_side_effect(url, cert, verify=True):
    return mock_reqget()


def set_kube_mock(mock_subproc, mock_table=None, mock_conn=None, mock_reqget=None):
    mock_subproc.side_effect = mock_subproc_side_effect
    if mock_table != None:
        mock_table.side_effect = table_side_effect
    if mock_conn != None:
        mock_conn.side_effect = conn_side_effect
    if mock_reqget != None:
        mock_reqget.side_effect = mock_reqget_side_effect


def create_remote_ctr_config_json():
    str_conf = '\
{\n\
    "join_latency_on_boot_seconds": 2,\n\
    "retry_join_interval_seconds": 0,\n\
    "retry_labels_update_seconds": 0,\n\
    "revert_to_local_on_wait_seconds": 5,\n\
    "tag_latest_image_on_wait_seconds": 0\n\
}\n'

    fname = "/tmp/remote_ctr.config.json"
    with open(fname, "w") as s:
        s.write(str_conf)

    return fname
