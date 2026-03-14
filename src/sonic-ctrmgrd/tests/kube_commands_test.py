import json
import os
import shutil
import sys
from unittest.mock import MagicMock, patch

import pytest

from . import common_test

sys.path.append("ctrmgr")
import kube_commands


KUBE_ADMIN_CONF = "/tmp/kube_admin.conf"
FLANNEL_CONF_FILE = "/tmp/flannel.conf"
CNI_DIR = "/tmp/cni/net.d"
AME_CRT = "/tmp/restapiserver.crt"
AME_KEY = "/tmp/restapiserver.key"

# kube_commands test cases
# NOTE: Ensure state-db entry is complete in PRE as we need to
# overwrite any context left behind from last test run.
#
read_labels_test_data = {
    0: {
        common_test.DESCR: "read labels",
        common_test.RETVAL: 0,
        common_test.PROC_CMD: ["\
kubectl --kubeconfig {} get nodes none --show-labels --no-headers |tr -s ' ' | cut -f6 -d' '".format(KUBE_ADMIN_CONF)],
        common_test.PROC_OUT: ["foo=bar,hello=world"],
        common_test.POST: {
            "foo": "bar",
            "hello": "world"
        },
        common_test.PROC_KILLED: 0
    },
    1: {
        common_test.DESCR: "read labels timeout",
        common_test.TRIGGER_THROW: True,
        common_test.RETVAL: -1,
        common_test.PROC_CMD: ["\
kubectl --kubeconfig {} get nodes none --show-labels --no-headers |tr -s ' ' | cut -f6 -d' '".format(KUBE_ADMIN_CONF)],
        common_test.POST: {
        },
        common_test.PROC_KILLED: 1
    },
    2: {
        common_test.DESCR: "read labels fail",
        common_test.RETVAL: -1,
        common_test.PROC_CMD: ["\
kubectl --kubeconfig {} get nodes none --show-labels --no-headers |tr -s ' ' | cut -f6 -d' '".format(KUBE_ADMIN_CONF)],
        common_test.PROC_OUT: [""],
        common_test.PROC_ERR: ["command failed"],
        common_test.POST: {
        },
        common_test.PROC_KILLED: 0
    }
}

write_labels_test_data = {
    0: {
        common_test.DESCR: "write labels: skip/overwrite/new",
        common_test.RETVAL: 0,
        common_test.ARGS: { "foo": "bar", "hello": "World!", "test": "ok" },
        common_test.PROC_CMD: [
"kubectl --kubeconfig {} get nodes none --show-labels --no-headers |tr -s ' ' | cut -f6 -d' '".format(KUBE_ADMIN_CONF),
"kubectl --kubeconfig {} label --overwrite nodes none hello-".format(
    KUBE_ADMIN_CONF),
"kubectl --kubeconfig {} label --overwrite nodes none hello=World! test=ok".format(
    KUBE_ADMIN_CONF)
 ],
        common_test.PROC_OUT: ["foo=bar,hello=world", "", ""]
    },
    1: {
        common_test.DESCR: "write labels: skip as no change",
        common_test.RETVAL: 0,
        common_test.ARGS: { "foo": "bar", "hello": "world" },
        common_test.PROC_CMD: [
"kubectl --kubeconfig {} get nodes none --show-labels --no-headers |tr -s ' ' | cut -f6 -d' '".format(KUBE_ADMIN_CONF)
 ],
        common_test.PROC_OUT: ["foo=bar,hello=world"]
    },
    2: {
        common_test.DESCR: "write labels",
        common_test.RETVAL: 0,
        common_test.ARGS: { "any": "thing" },
        common_test.RETVAL: -1,
        common_test.PROC_CMD: [
"kubectl --kubeconfig {} get nodes none --show-labels --no-headers |tr -s ' ' | cut -f6 -d' '".format(KUBE_ADMIN_CONF)
],
        common_test.PROC_ERR: ["read failed"]
    }
}

join_test_data = {
    0: {
        common_test.DESCR: "Regular insecure join",
        common_test.RETVAL: 0,
        common_test.ARGS: ["10.3.157.24", 6443, True, False],
        common_test.PROC_CMD: [
            "kubectl --kubeconfig {} --request-timeout 20s drain none \
--ignore-daemonsets".format(KUBE_ADMIN_CONF),
            "kubectl --kubeconfig {} --request-timeout 20s delete node \
none".format(KUBE_ADMIN_CONF),
            "kubeadm reset -f",
            "rm -rf {}".format(CNI_DIR),
            "systemctl stop kubelet",
            "modprobe br_netfilter",
            "mkdir -p {}".format(CNI_DIR),
            "cp {} {}".format(FLANNEL_CONF_FILE, CNI_DIR),
            "systemctl start kubelet",
            "kubeadm join --discovery-file {} --node-name none".format(
                KUBE_ADMIN_CONF)
        ],
        common_test.PROC_RUN: [True, True],
        common_test.REQ: {
            "data": {"ca.crt": "test"}
        }
    },
    1: {
        common_test.DESCR: "Regular secure join",
        common_test.RETVAL: 0,
        common_test.ARGS: ["10.3.157.24", 6443, False, False],
        common_test.PROC_CMD: [
            "kubectl --kubeconfig {} --request-timeout 20s drain none \
--ignore-daemonsets".format(KUBE_ADMIN_CONF),
            "kubectl --kubeconfig {} --request-timeout 20s delete node \
none".format(KUBE_ADMIN_CONF),
            "kubeadm reset -f",
            "rm -rf {}".format(CNI_DIR),
            "systemctl stop kubelet",
            "modprobe br_netfilter",
            "mkdir -p {}".format(CNI_DIR),
            "cp {} {}".format(FLANNEL_CONF_FILE, CNI_DIR),
            "systemctl start kubelet",
            "kubeadm join --discovery-file {} --node-name none".format(
                KUBE_ADMIN_CONF)
        ],
        common_test.PROC_RUN: [True, True],
        common_test.REQ: {
            "data": {"ca.crt": "test"}
        }
    },
    2: {
        common_test.DESCR: "Skip join as already connected",
        common_test.RETVAL: 0,
        common_test.ARGS: ["10.3.157.24", 6443, True, False],
        common_test.NO_INIT: True,
        common_test.PROC_CMD: [
            "systemctl start kubelet"
        ]
    },
    3: {
        common_test.DESCR: "Regular join: fail due to unable to lock",
        common_test.RETVAL: -1,
        common_test.ARGS: ["10.3.157.24", 6443, False, False],
        common_test.FAIL_LOCK: True
    }
}


reset_test_data = {
    0: {
        common_test.DESCR: "non force reset",
        common_test.RETVAL: 0,
        common_test.DO_JOIN: True,
        common_test.ARGS: [False],
        common_test.PROC_CMD: [
            "kubectl --kubeconfig {} --request-timeout 20s drain none \
--ignore-daemonsets".format(KUBE_ADMIN_CONF),
            "kubectl --kubeconfig {} --request-timeout 20s delete node \
none".format(KUBE_ADMIN_CONF),
            "kubeadm reset -f",
            "rm -rf {}".format(CNI_DIR),
            "rm -f {}".format(KUBE_ADMIN_CONF),
            "systemctl stop kubelet"
        ]
    },
    1: {
        common_test.DESCR: "force reset",
        common_test.RETVAL: 0,
        common_test.ARGS: [False],
        common_test.PROC_CMD: [
            "kubectl --kubeconfig {} --request-timeout 20s drain none \
--ignore-daemonsets".format(KUBE_ADMIN_CONF),
            "kubectl --kubeconfig {} --request-timeout 20s delete node \
none".format(KUBE_ADMIN_CONF),
            "kubeadm reset -f",
            "rm -rf {}".format(CNI_DIR),
            "rm -f {}".format(KUBE_ADMIN_CONF),
            "systemctl stop kubelet"
        ]
    },
    1: {
        common_test.DESCR: "force reset",
        common_test.RETVAL: 0,
        common_test.ARGS: [True],
        common_test.PROC_CMD: [
            "kubeadm reset -f",
            "rm -rf {}".format(CNI_DIR),
            "rm -f {}".format(KUBE_ADMIN_CONF),
            "systemctl stop kubelet"
        ]
    },
    2: {
        common_test.DESCR: "skip reset as not connected",
        common_test.RETVAL: -1,
        common_test.ARGS: [False],
        common_test.PROC_CMD: [
            "systemctl stop kubelet"
        ]
    }
}

tag_latest_test_data = {
    0: {
        common_test.DESCR: "Tag latest successfuly and remove origin local container",
        common_test.RETVAL: 0,
        common_test.ARGS: ["snmp", "123456", "v1"],
        common_test.PROC_CMD: [
            "docker ps |grep 123456",
            "docker inspect 123456 |jq -r .[].Image",
            "docker images |grep 5425bcbd23c5",
            "docker tag 5425bcbd23c5 snmp:latest",
            "docker inspect snmp |jq -r .[].State.Running",
            "docker rm snmp"
        ],
        common_test.PROC_OUT: [
            "",
            "sha256:5425bcbd23c54270d9de028c09634f8e9a014e9351387160c133ccf3a53ab3dc",
            "acr.io/snmp v1 5425bcbd23c5",
            "",
            "false",
            ""
        ]
    },
    1: {
        common_test.DESCR: "Tag latest successfuly and origin local container has been removed before",
        common_test.RETVAL: 0,
        common_test.ARGS: ["snmp", "123456", "v1"],
        common_test.PROC_CMD: [
            "docker ps |grep 123456",
            "docker inspect 123456 |jq -r .[].Image",
            "docker images |grep 5425bcbd23c5",
            "docker tag 5425bcbd23c5 snmp:latest",
            "docker inspect snmp |jq -r .[].State.Running",
            "docker rm snmp"
        ],
        common_test.PROC_OUT: [
            "",
            "sha256:5425bcbd23c54270d9de028c09634f8e9a014e9351387160c133ccf3a53ab3dc",
            "acr.io/snmp v1 5425bcbd23c5",
            "",
            "",
            ""
        ],
        common_test.PROC_ERR: [
            "",
            "",
            "",
            "",
            "Error: No such object",
            ""
        ]
    },
    2: {
        common_test.DESCR: "Tag a unstable container",
        common_test.RETVAL: -1,
        common_test.ARGS: ["snmp", "123456", "v1"],
        common_test.PROC_CMD: [
            "docker ps |grep 123456"
        ],
        common_test.PROC_CODE: [
            1
        ]
    },
    3: {
        common_test.DESCR: "Docker error",
        common_test.RETVAL: 1,
        common_test.ARGS: ["snmp", "123456", "v1"],
        common_test.PROC_CMD: [
            "docker ps |grep 123456"
        ],
        common_test.PROC_ERR: [
            "err"
        ]
    },
    4: {
        common_test.DESCR: "Find local container is still running",
        common_test.RETVAL: 1,
        common_test.ARGS: ["snmp", "123456", "v1"],
        common_test.PROC_CMD: [
            "docker ps |grep 123456",
            "docker inspect 123456 |jq -r .[].Image",
            "docker images |grep 5425bcbd23c5",
            "docker tag 5425bcbd23c5 snmp:latest",
            "docker inspect snmp |jq -r .[].State.Running",
            "docker rm snmp"
        ],
        common_test.PROC_OUT: [
            "",
            "sha256:5425bcbd23c54270d9de028c09634f8e9a014e9351387160c133ccf3a53ab3dc",
            "acr.io/snmp v1 5425bcbd23c5",
            "",
            "true",
            ""
        ]
    }
}

clean_image_test_data = {
    0: {
        common_test.DESCR: "Clean image successfuly(kube to kube)",
        common_test.RETVAL: 0,
        common_test.ARGS: ["snmp", "20201231.84", "20201231.74"],
        common_test.PROC_CMD: [
            "docker images |grep snmp |grep -v latest |awk '{print $1,$2,$3}'",
            "docker rmi 744d3a09062f --force"
        ],
        common_test.PROC_OUT: [
            "sonick8scue.azurecr.io/docker-sonic-telemetry 20201231.74 507f8d28bf6e\n\
             sonick8scue.azurecr.io/docker-sonic-telemetry 20201231.96 744d3a09062f\n\
             sonick8scue.azurecr.io/docker-sonic-telemetry 20201231.84 507f8d28bf6e",
            ""
        ],
        common_test.PROC_CODE: [
            0,
            0
        ]
    },
    1: {
        common_test.DESCR: "Clean image failed(delete image failed)",
        common_test.RETVAL: 1,
        common_test.ARGS: ["snmp", "20201231.84", "20201231.74"],
        common_test.PROC_CMD: [
            "docker images |grep snmp |grep -v latest |awk '{print $1,$2,$3}'",
            "docker rmi 744d3a09062f --force"
        ],
        common_test.PROC_OUT: [
            "sonick8scue.azurecr.io/docker-sonic-telemetry 20201231.74 507f8d28bf6e\n\
             sonick8scue.azurecr.io/docker-sonic-telemetry 20201231.96 744d3a09062f\n\
             sonick8scue.azurecr.io/docker-sonic-telemetry 20201231.84 507f8d28bf6e",
            ""
        ],
        common_test.PROC_CODE: [
            0,
            1
        ]
    },
    2: {
        common_test.DESCR: "Clean image failed(no image found)",
        common_test.RETVAL: 1,
        common_test.ARGS: ["snmp", "20201231.84", "20201231.74"],
        common_test.PROC_CMD: [
            "docker images |grep snmp |grep -v latest |awk '{print $1,$2,$3}'"
        ],
        common_test.PROC_OUT: [
            ""
        ]
    },
    3: {
        common_test.DESCR: "Clean image failed(current image doesn't exist)",
        common_test.RETVAL: 0,
        common_test.ARGS: ["snmp", "20201231.84", "20201231.74"],
        common_test.PROC_CMD: [
            "docker images |grep snmp |grep -v latest |awk '{print $1,$2,$3}'",
            ""
        ],
        common_test.PROC_OUT: [
            "sonick8scue.azurecr.io/docker-sonic-telemetry 20201231.74 507f8d28bf6e\n\
             sonick8scue.azurecr.io/docker-sonic-telemetry 20201231.96 744d3a09062f",
            ""
        ],
        common_test.PROC_CODE: [
            0
        ]
    },
    4: {
        common_test.DESCR: "Clean image successfuly(local to kube)",
        common_test.RETVAL: 0,
        common_test.ARGS: ["snmp", "20201231.84", ""],
        common_test.PROC_CMD: [
            "docker images |grep snmp |grep -v latest |awk '{print $1,$2,$3}'",
            "docker rmi docker-sonic-telemetry:20201231.74"
        ],
        common_test.PROC_OUT: [
            "docker-sonic-telemetry 20201231.74 507f8d28bf6e\n\
             sonick8scue.azurecr.io/docker-sonic-telemetry 20201231.84 507f8d28bf6e",
            ""
        ],
        common_test.PROC_CODE: [
            0,
            0
        ]
    },
    5: {
        common_test.DESCR: "Clean image successfuly(local to dry-kube to kube)",
        common_test.RETVAL: 0,
        common_test.ARGS: ["snmp", "20201231.84", "20201231.74"],
        common_test.PROC_CMD: [
            "docker images |grep snmp |grep -v latest |awk '{print $1,$2,$3}'",
            "docker rmi sonick8scue.azurecr.io/docker-sonic-telemetry:20201231.74 && docker tag 507f8d28bf6e sonick8scue.azurecr.io/docker-sonic-telemetry:20201231.74 && docker rmi docker-sonic-telemetry:20201231.74"
        ],
        common_test.PROC_OUT: [
            "docker-sonic-telemetry 20201231.74 507f8d28bf6e\n\
             sonick8scue.azurecr.io/docker-sonic-telemetry 20201231.74 507f8d28bf6f\n\
             sonick8scue.azurecr.io/docker-sonic-telemetry 20201231.84 507f8d28bf6g",
            ""
        ],
        common_test.PROC_CODE: [
            0,
            0
        ]
    },
}

class TestKubeCommands(object):

    def init(self):
        conf_str = "\
apiVersion: v1\n\
clusters:\n\
- cluster:\n\
    server: https://10.3.157.24:6443\n\
"
        self.admin_conf_file = "/tmp/kube_admin_url.info"
        with open(self.admin_conf_file, "w") as s:
            s.write(conf_str)
        kubelet_yaml = "/tmp/kubelet_config.yaml"
        with open(kubelet_yaml, "w") as s:
            s.close()
        with open(FLANNEL_CONF_FILE, "w") as s:
            s.close()
        with open(AME_CRT, "w") as s:
            s.close()
        with open(AME_KEY, "w") as s:
            s.close()
        kube_commands.KUBELET_YAML = kubelet_yaml
        kube_commands.CNI_DIR = CNI_DIR
        kube_commands.FLANNEL_CONF_FILE = FLANNEL_CONF_FILE
        kube_commands.SERVER_ADMIN_URL = "file://{}".format(self.admin_conf_file)
        kube_commands.KUBE_ADMIN_CONF = KUBE_ADMIN_CONF
        kube_commands.AME_CRT = AME_CRT
        kube_commands.AME_KEY = AME_KEY


    @patch("kube_commands.subprocess.Popen")
    def test_read_labels(self, mock_subproc):
        self.init()
        common_test.set_kube_mock(mock_subproc)

        for (i, ct_data) in read_labels_test_data.items():
            common_test.do_start_test("kube:read-labels", i, ct_data)

            (ret, labels) = kube_commands.kube_read_labels()
            if common_test.RETVAL in ct_data:
                assert ret == ct_data[common_test.RETVAL]

            if common_test.PROC_KILLED in ct_data:
                assert (common_test.procs_killed ==
                        ct_data[common_test.PROC_KILLED])

            if (common_test.POST in ct_data and
                    (ct_data[common_test.POST] != labels)):
                print("expect={} labels={} mismatch".format(
                    json.dumps(ct_data[common_test.POST], indent=4),
                    json.dumps(labels, indent=4)))
                assert False

        # Exercist through main
        common_test.do_start_test("kube:main:read-labels", 0,
                read_labels_test_data[0])
        with patch('sys.argv', "kube_commands get-labels".split()):
            ret = kube_commands.main()
            assert ret == 0

        # Exercist through main with no args
        common_test.do_start_test("kube:main:none", 0, read_labels_test_data[0])
        with patch('sys.argv', "kube_commands".split()):
            ret = kube_commands.main()
            assert ret == -1


    @patch("kube_commands.subprocess.Popen")
    def test_write_labels(self, mock_subproc):
        self.init()
        common_test.set_kube_mock(mock_subproc)

        for (i, ct_data) in write_labels_test_data.items():
            common_test.do_start_test("kube:write-labels", i, ct_data)

            ret = kube_commands.kube_write_labels(ct_data[common_test.ARGS])
            if common_test.RETVAL in ct_data:
                assert ret == ct_data[common_test.RETVAL]

            if common_test.PROC_KILLED in ct_data:
                assert (common_test.procs_killed ==
                        ct_data[common_test.PROC_KILLED])

            if (common_test.POST in ct_data and
                    (ct_data[common_test.POST] != labels)):
                print("expect={} labels={} mismatch".format(
                    json.dumps(ct_data[common_test.POST], indent=4),
                    json.dumps(labels, indent=4)))
                assert False

    @patch("kube_commands.requests.get")
    @patch("kube_commands.swsscommon.DBConnector")
    @patch("kube_commands.swsscommon.Table")
    @patch("kube_commands.subprocess.Popen")
    def test_join(self, mock_subproc, mock_table, mock_conn, mock_reqget):
        self.init()
        common_test.set_kube_mock(mock_subproc, mock_table, mock_conn, mock_reqget)

        for (i, ct_data) in join_test_data.items():
            lock_file = ""
            common_test.do_start_test("kube:join", i, ct_data)

            if not ct_data.get(common_test.NO_INIT, False):
                os.system("rm -f {}".format(KUBE_ADMIN_CONF))


            if ct_data.get(common_test.FAIL_LOCK, False):
                lock_file = kube_commands.LOCK_FILE
                kube_commands.LOCK_FILE = "/xxx/yyy/zzz"

            args = ct_data[common_test.ARGS]
            (ret, _, _) = kube_commands.kube_join_master(
                    args[0], args[1], args[2], args[3])
            if common_test.RETVAL in ct_data:
                assert ret == ct_data[common_test.RETVAL]

            if lock_file:
                kube_commands.LOCK_FILE = lock_file

        # Exercist through main is_connected
        common_test.do_start_test("kube:main:is_connected", 0, join_test_data[0])
        with patch('sys.argv', "kube_commands connected".split()):
            ret = kube_commands.main()
            assert ret == 1

        # test to_str()
        f = "abcd"
        f == kube_commands.to_str(str.encode(f))


    @patch("kube_commands.subprocess.Popen")
    def test_reset(self, mock_subproc):
        self.init()
        common_test.set_kube_mock(mock_subproc)

        for (i, ct_data) in reset_test_data.items():
            common_test.do_start_test("kube:reset", i, ct_data)

            if ct_data.get(common_test.DO_JOIN, False):
                shutil.copyfile(self.admin_conf_file, KUBE_ADMIN_CONF)
            else:
                os.system("rm -f {}".format(KUBE_ADMIN_CONF))

            (ret, _) = kube_commands.kube_reset_master(
                    ct_data[common_test.ARGS][0])
            if common_test.RETVAL in ct_data:
                assert ret == ct_data[common_test.RETVAL]

    @patch("kube_commands.subprocess.Popen")
    def test_tag_latest(self, mock_subproc):
        common_test.set_kube_mock(mock_subproc)

        for (i, ct_data) in tag_latest_test_data.items():
            common_test.do_start_test("tag:latest", i, ct_data)

            ret = kube_commands.tag_latest(*ct_data[common_test.ARGS])
            if common_test.RETVAL in ct_data:
                assert ret == ct_data[common_test.RETVAL]

    @patch("kube_commands.subprocess.Popen")
    def test_clean_image(self, mock_subproc):
        common_test.set_kube_mock(mock_subproc)

        for (i, ct_data) in clean_image_test_data.items():
            common_test.do_start_test("clean:image", i, ct_data)

            ret = kube_commands.clean_image(*ct_data[common_test.ARGS])
            if common_test.RETVAL in ct_data:
                assert ret == ct_data[common_test.RETVAL]
