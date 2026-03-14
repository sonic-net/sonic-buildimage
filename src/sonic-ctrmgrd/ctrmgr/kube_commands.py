#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import argparse
import fcntl
import inspect
import json
import os
import shutil
import ssl
import subprocess
import sys
import syslog
import tempfile
import urllib.request
import base64
from urllib.parse import urlparse

import yaml
import requests
from sonic_py_common import device_info
from jinja2 import Template
from swsscommon import swsscommon

KUBE_ADMIN_CONF = "/etc/sonic/kube_admin.conf"
KUBELET_YAML = "/var/lib/kubelet/config.yaml"
SERVER_ADMIN_URL = "https://{}/admin.conf"
LOCK_FILE = "/var/lock/kube_join.lock"
FLANNEL_CONF_FILE = "/usr/share/sonic/templates/kube_cni.10-flannel.conflist"
CNI_DIR = "/etc/cni/net.d"
K8S_CA_URL = "https://{}:{}/api/v1/namespaces/default/configmaps/kube-root-ca.crt"
AME_CRT = "/etc/sonic/credentials/restapiserver.crt"
AME_KEY = "/etc/sonic/credentials/restapiserver.key"

def log_debug(m):
    msg = "{}: {}".format(inspect.stack()[1][3], m)
    print(msg)
    syslog.syslog(syslog.LOG_DEBUG, msg)


def log_error(m):
    msg = "{}: {}".format(inspect.stack()[1][3], m)
    print(msg)
    syslog.syslog(syslog.LOG_ERR, m)


def to_str(s):
    if isinstance(s, str):
        return s

    if isinstance(s, bytes):
        return s.decode('utf-8')

    return str(s)

def get_device_name():
    return str(device_info.get_hostname()).lower()


def _run_command(cmd, timeout=5):
    """ Run shell command and return exit code, along with stdout. """
    ret = 0
    try:
        proc = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE,
                stderr=subprocess.PIPE)
        (o, e) = proc.communicate(timeout)
        output = to_str(o)
        err = to_str(e)
        ret = proc.returncode
    except subprocess.TimeoutExpired as error:
        proc.kill()
        output = ""
        err = str(error)
        ret = -1

    log_debug("cmd:{}\nret={}".format(cmd, ret))
    if output:
        log_debug("out:{}".format(output))
    if err:
        log_debug("err:{}".format(err))

    return (ret, output.strip(), err.strip())


def kube_read_labels():
    """ Read current labels on node and return as dict. """
    KUBECTL_GET_CMD = "kubectl --kubeconfig {} get nodes {} --show-labels --no-headers |tr -s ' ' | cut -f6 -d' '"

    labels = {}
    ret, out, _ = _run_command(KUBECTL_GET_CMD.format(
        KUBE_ADMIN_CONF, get_device_name()))

    if ret == 0:
        lst = out.split(",")

        for label in lst:
            tmp = label.split("=")
            labels[tmp[0]] = tmp[1]

    # log_debug("{} kube labels {} ret={}".format(
        # "Applied" if ret == 0 else "Failed to apply",
        # json.dumps(labels, indent=4), ret))

    return (ret, labels)


def kube_write_labels(set_labels):
    """ Set given set_labels.
    """
    KUBECTL_SET_CMD = "kubectl --kubeconfig {} label --overwrite nodes {} {}"

    ret, node_labels = kube_read_labels()
    if ret != 0:
        log_debug("Read before set failed. Hence skipping set {}".
                format(str(set_labels)))
        return ret

    del_label_str = ""
    add_label_str = ""
    for (name, val) in set_labels.items():
        skip = False
        if name in node_labels:
            if val != node_labels[name]:
                # label value can't be modified. Remove it first
                # and then add
                del_label_str += "{}- ".format(name)
            else:
                # Already exists with same value.
                skip = True
        if not skip:
            # Add label
            add_label_str += "{}={} ".format(name, val)


    if add_label_str:
        # First remove if any
        if del_label_str:
            (ret, _, _) = _run_command(KUBECTL_SET_CMD.format(
            KUBE_ADMIN_CONF, get_device_name(), del_label_str.strip()))
        (ret, _, _) = _run_command(KUBECTL_SET_CMD.format(
            KUBE_ADMIN_CONF, get_device_name(), add_label_str.strip()))

        log_debug("{} kube labels {} ret={}".format(
            "Applied" if ret == 0 else "Failed to apply", add_label_str, ret))
    else:
        log_debug("Given labels are in sync with node labels. Hence no-op")

    return ret


def func_get_labels(args):
    """ args parser default function for get labels"""
    ret, node_labels = kube_read_labels()
    if ret != 0:
        log_debug("Labels read failed.")
        return ret

    log_debug(json.dumps(node_labels, indent=4))
    return 0


def is_connected(server=""):
    """ Check if we are currently connected """

    if (os.path.exists(KUBELET_YAML) and os.path.exists(KUBE_ADMIN_CONF)):
        with open(KUBE_ADMIN_CONF, 'r') as s:
            d = yaml.load(s, yaml.SafeLoader)
            d = d['clusters'] if 'clusters' in d else []
            d = d[0] if len(d) > 0 else {}
            d = d['cluster'] if 'cluster' in d else {}
            d = d['server'] if 'server' in d else ""
            if d:
                o = urlparse(d)
                if o.hostname:
                    return not server or server == o.hostname
    return False


def func_is_connected(args):
    """ Get connected state """
    connected = is_connected()
    log_debug("Currently {} to Kube master".format(
        "connected" if connected else "not connected"))
    return 0 if connected else 1


def _take_lock():
    """ Take a lock to block concurrent calls """
    lock_fd = None
    try:
        lock_fd = open(LOCK_FILE, "w")
        fcntl.lockf(lock_fd, fcntl.LOCK_EX | fcntl.LOCK_NB)
        log_debug("Lock taken {}".format(LOCK_FILE))
    except IOError as e:
        lock_fd = None
        log_error("Lock {} failed: {}".format(LOCK_FILE, str(e)))
    return lock_fd


def _download_file(server, port, insecure):
    """ Download file from Kube master to assist join as node. """

    if insecure:
        r = urllib.request.urlopen(SERVER_ADMIN_URL.format(server),
                context=ssl._create_unverified_context())
    else:
        r = urllib.request.urlopen(SERVER_ADMIN_URL.format(server))

    (h, fname) = tempfile.mkstemp(suffix="_kube_join")
    data = r.read()
    os.write(h, data)
    os.close(h)
    log_debug("Downloaded = {}".format(fname))

    shutil.copyfile(fname, KUBE_ADMIN_CONF)

    log_debug("{} downloaded".format(KUBE_ADMIN_CONF))


def _gen_cli_kubeconf(server, port, insecure):
    """generate identity which can help authenticate and
       authorization to k8s cluster
    """
    client_kubeconfig_template = """
apiVersion: v1
clusters:
- cluster:
    certificate-authority-data: {{ k8s_ca }}
    server: https://{{ vip }}:{{ port }}
  name: kubernetes
contexts:
- context:
    cluster: kubernetes
    user: user
  name: user@kubernetes
current-context: user@kubernetes
kind: Config
preferences: {}
users:
- name: user
  user:
    client-certificate-data: {{ ame_crt }}
    client-key-data: {{ ame_key }}
    """
    if insecure:
        r = requests.get(K8S_CA_URL.format(server, port), cert=(AME_CRT, AME_KEY), verify=False)
    else:
        r = requests.get(K8S_CA_URL.format(server, port), cert=(AME_CRT, AME_KEY))
    if not r.ok:
        raise requests.RequestException("Something wrong with AME cert or something wrong about sonic role in k8s cluster")
    k8s_ca = r.json()["data"]["ca.crt"]
    k8s_ca_b64 = base64.b64encode(k8s_ca.encode("utf-8")).decode("utf-8")
    ame_crt_raw = open(AME_CRT, "rb")
    ame_crt_b64 = base64.b64encode(ame_crt_raw.read()).decode("utf-8")
    ame_key_raw = open(AME_KEY, "rb")
    ame_key_b64 = base64.b64encode(ame_key_raw.read()).decode("utf-8")
    client_kubeconfig_template_j2 = Template(client_kubeconfig_template)
    client_kubeconfig = client_kubeconfig_template_j2.render(
        k8s_ca=k8s_ca_b64, vip=server, port=port, ame_crt=ame_crt_b64, ame_key=ame_key_b64)
    (h, fname) = tempfile.mkstemp(suffix="_kube_join")
    os.write(h, client_kubeconfig.encode("utf-8"))
    os.close(h)
    log_debug("Downloaded = {}".format(fname))

    shutil.copyfile(fname, KUBE_ADMIN_CONF)

    log_debug("{} downloaded".format(KUBE_ADMIN_CONF))


def _get_local_ipv6():
    try:
        config_db = swsscommon.DBConnector("CONFIG_DB", 0)
        mgmt_ip_data = swsscommon.Table(config_db, 'MGMT_INTERFACE')
        for key in mgmt_ip_data.getKeys():
            if key.find(":") >= 0:
                return key.split("|")[1].split("/")[0]
        raise IOError("IPV6 not find from MGMT_INTERFACE table")
    except Exception as e:
        raise IOError(str(e))


def _troubleshoot_tips():
    """ log troubleshoot tips which could be handy,
        when in trouble with join
    """
    msg = """
if join fails, check the following

a.  Ensure both master & node run same or compatible k8s versions

b.  Check if this node already exists in master
    Use 'sudo kubectl --kubeconfig=${KUBE_ADMIN_CONF} get nodes' to list nodes at master.

    If yes, delete it, as the node is attempting a new join.
    'kubectl --kubeconfig=${KUBE_ADMIN_CONF} drain <node name> --ignore-daemonsets'
    'kubectl --kubeconfig=${KUBE_ADMIN_CONF} delete node <node name>'

c.  In Master check if all system pods are running good.
    'kubectl get pods --namespace kube-system'

    If any not running properly, say READY column has 0/1, decribe pod for more detail.
    'kubectl --namespace kube-system describe pod <pod name>'

    For additional details, look into pod's logs.
    @ node: /var/log/pods/<podname>/...
    @ master: 'kubectl logs -n kube-system <pod name>'
    """

    (h, fname) = tempfile.mkstemp(suffix="kube_hints_")
    os.write(h, str.encode(msg))
    os.close(h)

    log_error("Refer file {} for troubleshooting tips".format(fname))


def _do_reset(pending_join = False):
    # Drain & delete self from cluster. If not, the next join would fail
    #
    if os.path.exists(KUBE_ADMIN_CONF):
        _run_command(
                "kubectl --kubeconfig {} --request-timeout 20s drain {} --ignore-daemonsets".
                format(KUBE_ADMIN_CONF, get_device_name()))

        _run_command("kubectl --kubeconfig {} --request-timeout 20s delete node {}".
                format(KUBE_ADMIN_CONF, get_device_name()))

    _run_command("kubeadm reset -f", 10)
    _run_command("rm -rf {}".format(CNI_DIR))
    if not pending_join:
        _run_command("rm -f {}".format(KUBE_ADMIN_CONF))
    _run_command("systemctl stop kubelet")


def _do_join(server, port, insecure):
    KUBEADM_JOIN_CMD = "kubeadm join --discovery-file {} --node-name {}"
    err = ""
    out = ""
    ret = 0
    try:
        _gen_cli_kubeconf(server, port, insecure)
        _do_reset(True)
        _run_command("modprobe br_netfilter")
        # Copy flannel.conf
        _run_command("mkdir -p {}".format(CNI_DIR))
        _run_command("cp {} {}".format(FLANNEL_CONF_FILE, CNI_DIR))
        (ret, _, _) = _run_command("systemctl start kubelet")

        if ret == 0:
            (ret, out, err) = _run_command(KUBEADM_JOIN_CMD.format(
                KUBE_ADMIN_CONF, get_device_name()), timeout=60)
            log_debug("ret = {}".format(ret))

    except IOError as e:
        err = "Join failed: {}".format(str(e))
        ret = -1
        out = ""

    _troubleshoot_tips()

    if (ret != 0):
        log_error(err)

    return (ret, out, err)


def kube_join_master(server, port, insecure, force=False):
    """ The main function that initiates join to master """

    out = ""
    err = ""
    ret = 0

    log_debug("join: server:{} port:{} insecure:{} force:{}".
            format(server, port, insecure, force))

    lock_fd = _take_lock()
    if not lock_fd:
        log_error("Lock {} is active; Bail out".format(LOCK_FILE))
        return (-1, "", "")

    if ((not force) and is_connected(server)):
        _run_command("systemctl start kubelet")
        err = "Master {} is already connected. "
        err += "Reset or join with force".format(server)
    else:
        (ret, out, err) = _do_join(server, port, insecure)

    log_debug("join: ret={} out:{} err:{}".format(ret, out, err))
    return (ret, out, err)


def kube_reset_master(force):
    err = ""
    ret = 0

    lock_fd = _take_lock()
    if not lock_fd:
        log_error("Lock {} is active; Bail out".format(LOCK_FILE))
        return (-1, "")

    if not force:
        if not is_connected():
            err = "Currently not connected to master. "
            err += "Use force reset if needed"
            log_debug("Not connected ... bailing out")
            ret = -1

    if ret == 0:
        _do_reset()
    else:
        _run_command("systemctl stop kubelet")

    return (ret, err)

def _do_tag(docker_id, image_ver):
    err = ""
    out = ""
    ret = 1
    status, _, err = _run_command("docker ps |grep {}".format(docker_id))
    if status == 0:
        _, image_item, err = _run_command("docker inspect {} |jq -r .[].Image".format(docker_id))
        if image_item:
            image_id = image_item.split(":")[1][:12]
            _, image_info, err = _run_command("docker images |grep {}".format(image_id))
            if image_info:
                # Only need the docker repo name without acr domain
                image_rep = image_info.split()[0].split("/")[-1]
                tag_res, _, err = _run_command("docker tag {} {}:latest".format(image_id, image_rep))
                if tag_res == 0:
                    out = "docker tag {} {}:latest successfully".format(image_id, image_rep)
                    ret = 0
                else:
                    err = "Failed to tag {}:{} to latest. Err: {}".format(image_rep, image_ver, err)
            else:
                err = "Failed to docker images |grep {} to get image repo. Err: {}".format(image_id, err)
        else:
            err = "Failed to inspect container:{} to get image id. Err: {}".format(docker_id, err)
    elif err:
        err = "Error happens when execute docker ps |grep {}. Err: {}".format(docker_id, err)
    else:
        out = "New version {} is not running.".format(image_ver)
        ret = -1

    return (ret, out, err)

def _remove_container(feat):
    err = ""
    out = ""
    ret = 0
    _, feat_status, err = _run_command("docker inspect {} |jq -r .[].State.Running".format(feat))
    if feat_status:
        if feat_status == 'true':
            err = "Feature {} container is running, it's unexpected".format(feat)
            ret = 1
        else:
            rm_res, _, err = _run_command("docker rm {}".format(feat))
            if rm_res == 0:
                out = "Remove origin local {} container successfully".format(feat)
            else:
                err = "Failed to docker rm {}. Err: {}".format(feat, err)
                ret = 1
    elif err.startswith("Error: No such object"):
        out = "Origin local {} container has been removed before".format(feat)
        err = ""
    else:
        err = "Failed to docker inspect {} |jq -r .[].State.Running. Err: {}".format(feat, err)
        ret = 1

    return (ret, out, err)

def tag_latest(feat, docker_id, image_ver):
    ret, out, err = _do_tag(docker_id, image_ver)
    if ret == 0:
        log_debug(out)
        ret, out, err = _remove_container(feat)
        if ret == 0:
            log_debug(out)
        else:
            log_error(err)
    elif ret == -1:
        log_debug(out)
    else:
        log_error(err)
    return ret

def _do_clean(feat, current_version, last_version):
    err = ""
    out = ""
    ret = 0
    IMAGE_ID = "image_id"
    REPO = "repo"
    _, image_info, err = _run_command("docker images |grep {} |grep -v latest |awk '{{print $1,$2,$3}}'".format(feat))
    if image_info:
        remote_image_version_dict = {}
        local_image_version_dict = {}
        for info in image_info.split("\n"):
            rep, version, image_id = info.split()
            if len(rep.split("/")) == 1:
                local_image_version_dict[version] = {IMAGE_ID: image_id, REPO: rep}
            else:
                remote_image_version_dict[version] = {IMAGE_ID: image_id, REPO: rep}

        if current_version in remote_image_version_dict:
            image_prefix = remote_image_version_dict[current_version][REPO]
            del remote_image_version_dict[current_version]
        else:
            out = "Current version {} doesn't exist.".format(current_version)
            ret = 0
            return ret, out, err
        # should be only one item in local_image_version_dict
        for k, v in local_image_version_dict.items():
            local_version, local_repo, local_image_id = k, v[REPO], v[IMAGE_ID]
            # if there is a kube image with same version, need to remove the kube version
            # and tag the local version to kube version for fallback preparation
            # and remove the local version
            if local_version in remote_image_version_dict:
                tag_res, _, err = _run_command("docker rmi {}:{} && docker tag {} {}:{} && docker rmi {}:{}".format(
                image_prefix, local_version, local_image_id, image_prefix, local_version, local_repo, local_version))
            # if there is no kube image with same version, just remove the local version
            else:
                tag_res, _, err = _run_command("docker rmi {}:{}".format(local_repo, local_version))
            if tag_res == 0:
                msg = "Tag {} local version images successfully".format(feat)
                log_debug(msg)
            else:
                ret = 1
                err = "Failed to tag {} local version images. Err: {}".format(feat, err)
            return ret, out, err

        if last_version in remote_image_version_dict:
            del remote_image_version_dict[last_version]

        image_id_remove_list = [item[IMAGE_ID] for item in remote_image_version_dict.values()]
        if image_id_remove_list:
            clean_res, _, err = _run_command("docker rmi {} --force".format(" ".join(image_id_remove_list)))
        else:
            clean_res = 0
        if clean_res == 0:
            out = "Clean {} old version images successfully".format(feat)
        else:
            err = "Failed to clean {} old version images. Err: {}".format(feat, err)
            ret = 1
    else:
        err = "Failed to docker images |grep {} |awk '{{print $3}}'. Error: {}".format(feat, err)
        ret = 1

    return ret, out, err

def clean_image(feat, current_version, last_version):
    ret, out, err = _do_clean(feat, current_version, last_version)
    if ret == 0:
        log_debug(out)
    else:
        log_error(err)
    return ret

def main():
    syslog.openlog("kube_commands")
    parser=argparse.ArgumentParser(description=
            "get-labels")
    subparsers = parser.add_subparsers(title='actions')

    parser_get_labels = subparsers.add_parser("get-labels",
            help="Get current labels on node")
    parser_get_labels.set_defaults(func=func_get_labels)

    parser_is_connected = subparsers.add_parser("connected",
            help="Get connnected status")
    parser_is_connected.set_defaults(func=func_is_connected)

    if len(sys.argv) < 2:
        parser.print_help()
        return -1

    args = parser.parse_args()
    ret = args.func(args)

    syslog.closelog()
    return ret


if __name__ == "__main__":
    if os.geteuid() != 0:
        exit("Please run as root. Exiting ...")
    main()
    sys.exit(0)
