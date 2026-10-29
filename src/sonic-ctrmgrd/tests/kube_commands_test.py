import json
import os
import socket
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
REQUEST_VERIFY = "request_verify"

# kube_commands test cases
# NOTE: Ensure state-db entry is complete in PRE as we need to
# overwrite any context left behind from last test run.
#
read_labels_test_data = {
    0: {
        common_test.DESCR: "read labels",
        common_test.RETVAL: 0,
        common_test.PROC_CMD: [
            ["kubectl", "--kubeconfig", KUBE_ADMIN_CONF, "get", "nodes",
             "--show-labels", "--no-headers", "--", "none"]
        ],
        common_test.PROC_OUT: [
            "none Ready <role> 10d v1.28.0 foo=bar,hello=world"
        ],
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
        common_test.PROC_CMD: [
            ["kubectl", "--kubeconfig", KUBE_ADMIN_CONF, "get", "nodes",
             "--show-labels", "--no-headers", "--", "none"]
        ],
        common_test.POST: {
        },
        common_test.PROC_KILLED: 1
    },
    2: {
        common_test.DESCR: "read labels fail",
        common_test.RETVAL: -1,
        common_test.PROC_CMD: [
            ["kubectl", "--kubeconfig", KUBE_ADMIN_CONF, "get", "nodes",
             "--show-labels", "--no-headers", "--", "none"]
        ],
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
        common_test.ARGS: { "foo": "bar", "hello": "World", "test": "ok" },
        common_test.PROC_CMD: [
["kubectl", "--kubeconfig", KUBE_ADMIN_CONF, "get", "nodes", "--show-labels", "--no-headers", "--", "none"],
["kubectl", "--kubeconfig", KUBE_ADMIN_CONF, "label", "--overwrite", "nodes", "--", "none", "hello-"],
["kubectl", "--kubeconfig", KUBE_ADMIN_CONF, "label", "--overwrite", "nodes", "--", "none", "hello=World", "test=ok"]
 ],
        common_test.PROC_OUT: ["none Ready <role> 10d v1.28.0 foo=bar,hello=world", "", ""]
    },
    1: {
        common_test.DESCR: "write labels: skip as no change",
        common_test.RETVAL: 0,
        common_test.ARGS: { "foo": "bar", "hello": "world" },
        common_test.PROC_CMD: [
["kubectl", "--kubeconfig", KUBE_ADMIN_CONF, "get", "nodes", "--show-labels", "--no-headers", "--", "none"]
 ],
        common_test.PROC_OUT: ["none Ready <role> 10d v1.28.0 foo=bar,hello=world"]
    },
    2: {
        common_test.DESCR: "write labels",
        common_test.ARGS: { "any": "thing" },
        common_test.RETVAL: -1,
        common_test.PROC_CMD: [
["kubectl", "--kubeconfig", KUBE_ADMIN_CONF, "get", "nodes", "--show-labels", "--no-headers", "--", "none"]
],
        common_test.PROC_ERR: ["read failed"]
    },
    3: {
        common_test.DESCR: "write labels: injection attempt in name and value is skipped as invalid",
        common_test.RETVAL: 0,
        common_test.ARGS: { "foo; id>/tmp/pwned #": "bar; rm -rf / #" },
        common_test.PROC_CMD: [
["kubectl", "--kubeconfig", KUBE_ADMIN_CONF, "get", "nodes", "--show-labels", "--no-headers", "--", "none"]
 ],
        common_test.PROC_OUT: [""]
    }
}

join_test_data = {
    0: {
        common_test.DESCR: "Regular insecure join",
        common_test.RETVAL: 0,
        common_test.ARGS: ["10.3.157.24", 6443, "true", False],
        common_test.PROC_CMD: [
            ["kubectl", "--kubeconfig", KUBE_ADMIN_CONF,
             "--request-timeout", "20s", "drain", "--ignore-daemonsets",
             "--", "none"],
            ["kubectl", "--kubeconfig", KUBE_ADMIN_CONF,
             "--request-timeout", "20s", "delete", "node", "--", "none"],
            ["kubeadm", "reset", "-f"],
            ["rm", "-rf", CNI_DIR],
            ["systemctl", "stop", "kubelet"],
            ["modprobe", "br_netfilter"],
            ["mkdir", "-p", CNI_DIR],
            ["cp", FLANNEL_CONF_FILE, CNI_DIR],
            ["systemctl", "start", "kubelet"],
            ["kubeadm", "join", "--discovery-file", KUBE_ADMIN_CONF,
             "--node-name", "none"]
        ],
        common_test.REQ: {
            "data": {"ca.crt": "test"}
        },
        REQUEST_VERIFY: False
    },
    1: {
        common_test.DESCR: "Regular secure join",
        common_test.RETVAL: 0,
        common_test.ARGS: ["10.3.157.24", 6443, "false", False],
        common_test.PROC_CMD: [
            ["kubectl", "--kubeconfig", KUBE_ADMIN_CONF,
             "--request-timeout", "20s", "drain", "--ignore-daemonsets",
             "--", "none"],
            ["kubectl", "--kubeconfig", KUBE_ADMIN_CONF,
             "--request-timeout", "20s", "delete", "node", "--", "none"],
            ["kubeadm", "reset", "-f"],
            ["rm", "-rf", CNI_DIR],
            ["systemctl", "stop", "kubelet"],
            ["modprobe", "br_netfilter"],
            ["mkdir", "-p", CNI_DIR],
            ["cp", FLANNEL_CONF_FILE, CNI_DIR],
            ["systemctl", "start", "kubelet"],
            ["kubeadm", "join", "--discovery-file", KUBE_ADMIN_CONF,
             "--node-name", "none"]
        ],
        common_test.REQ: {
            "data": {"ca.crt": "test"}
        },
        REQUEST_VERIFY: True
    },
    2: {
        common_test.DESCR: "Skip join as already connected",
        common_test.RETVAL: 0,
        common_test.ARGS: ["10.3.157.24", 6443, "true", False],
        common_test.NO_INIT: True,
        common_test.PROC_CMD: [
            ["kubectl", "--kubeconfig", KUBE_ADMIN_CONF, "get", "nodes",
             "--no-headers", "--", "none"],
            ["systemctl", "start", "kubelet"]
        ],
        common_test.PROC_OUT: ["none   Ready   <role>   10d   v1.28.0", ""]
    },
    3: {
        common_test.DESCR: "Regular join: fail due to unable to lock",
        common_test.RETVAL: -1,
        common_test.ARGS: ["10.3.157.24", 6443, "false", False],
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
            ["kubectl", "--kubeconfig", KUBE_ADMIN_CONF, "get", "nodes",
             "--no-headers", "--", "none"],
            ["kubectl", "--kubeconfig", KUBE_ADMIN_CONF,
             "--request-timeout", "20s", "drain", "--ignore-daemonsets",
             "--", "none"],
            ["kubectl", "--kubeconfig", KUBE_ADMIN_CONF,
             "--request-timeout", "20s", "delete", "node", "--", "none"],
            ["kubeadm", "reset", "-f"],
            ["rm", "-rf", CNI_DIR],
            ["rm", "-f", KUBE_ADMIN_CONF],
            ["systemctl", "stop", "kubelet"]
        ],
        common_test.PROC_OUT: ["none   Ready   <role>   10d   v1.28.0", "", "", "", "", "", ""]
    },
    1: {
        common_test.DESCR: "force reset",
        common_test.RETVAL: 0,
        common_test.ARGS: [False],
        common_test.PROC_CMD: [
            ["kubectl", "--kubeconfig", KUBE_ADMIN_CONF,
             "--request-timeout", "20s", "drain", "--ignore-daemonsets",
             "--", "none"],
            ["kubectl", "--kubeconfig", KUBE_ADMIN_CONF,
             "--request-timeout", "20s", "delete", "node", "--", "none"],
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
            ["kubeadm", "reset", "-f"],
            ["rm", "-rf", CNI_DIR],
            ["rm", "-f", KUBE_ADMIN_CONF],
            ["systemctl", "stop", "kubelet"]
        ]
    },
    2: {
        common_test.DESCR: "skip reset as not connected",
        common_test.RETVAL: -1,
        common_test.ARGS: [False],
        common_test.PROC_CMD: [
            ["systemctl", "stop", "kubelet"]
        ]
    }
}

tag_latest_test_data = {
    0: {
        common_test.DESCR: "Tag latest successfuly and remove origin local container",
        common_test.RETVAL: 0,
        common_test.ARGS: ["snmp", "123456", "v1"],
        common_test.PROC_CMD: [
            ["docker", "ps"],
            ["docker", "inspect", "123456"],
            ["docker", "images"],
            ["docker", "tag", "5425bcbd23c5", "snmp:latest"],
            ["docker", "inspect", "snmp"],
            ["docker", "rm", "snmp"]
        ],
        common_test.PROC_OUT: [
            "abc 123456 snmp",
            '[{"Image": "sha256:5425bcbd23c54270d9de028c09634f8e9a014e9351387160c133ccf3a53ab3dc"}]',
            "acr.io/snmp v1 5425bcbd23c5",
            "",
            '[{"State": {"Running": false}}]',
            ""
        ]
    },
    1: {
        common_test.DESCR: "Tag latest successfuly and origin local container has been removed before",
        common_test.RETVAL: 0,
        common_test.ARGS: ["snmp", "123456", "v1"],
        common_test.PROC_CMD: [
            ["docker", "ps"],
            ["docker", "inspect", "123456"],
            ["docker", "images"],
            ["docker", "tag", "5425bcbd23c5", "snmp:latest"],
            ["docker", "inspect", "snmp"]
        ],
        common_test.PROC_OUT: [
            "abc 123456 snmp",
            '[{"Image": "sha256:5425bcbd23c54270d9de028c09634f8e9a014e9351387160c133ccf3a53ab3dc"}]',
            "acr.io/snmp v1 5425bcbd23c5",
            "",
            ""
        ],
        common_test.PROC_ERR: [
            "",
            "",
            "",
            "",
            "Error: No such object"
        ]
    },
    2: {
        common_test.DESCR: "Tag a unstable container",
        common_test.RETVAL: -1,
        common_test.ARGS: ["snmp", "123456", "v1"],
        common_test.PROC_CMD: [
            ["docker", "ps"]
        ],
        common_test.PROC_OUT: [
            "abc other_container"
        ]
    },
    3: {
        common_test.DESCR: "Docker error",
        common_test.RETVAL: 1,
        common_test.ARGS: ["snmp", "123456", "v1"],
        common_test.PROC_CMD: [
            ["docker", "ps"]
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
            ["docker", "ps"],
            ["docker", "inspect", "123456"],
            ["docker", "images"],
            ["docker", "tag", "5425bcbd23c5", "snmp:latest"],
            ["docker", "inspect", "snmp"]
        ],
        common_test.PROC_OUT: [
            "abc 123456 snmp",
            '[{"Image": "sha256:5425bcbd23c54270d9de028c09634f8e9a014e9351387160c133ccf3a53ab3dc"}]',
            "acr.io/snmp v1 5425bcbd23c5",
            "",
            '[{"State": {"Running": true}}]'
        ]
    }
}

DOCKER_IMAGES_CMD = ["docker", "images", "--format", "{{.Repository}} {{.Tag}} {{.ID}}"]

clean_image_test_data = {
    0: {
        common_test.DESCR: "Clean image successfuly(kube to kube)",
        common_test.RETVAL: 0,
        common_test.ARGS: ["snmp", "20201231.84", "20201231.74"],
        common_test.PROC_CMD: [
            DOCKER_IMAGES_CMD,
            ["docker", "rmi", "744d3a09062f", "--force"]
        ],
        common_test.PROC_OUT: [
            "sonick8scue.azurecr.io/docker-sonic-snmp 20201231.74 507f8d28bf6e\n"
            "sonick8scue.azurecr.io/docker-sonic-snmp 20201231.96 744d3a09062f\n"
            "sonick8scue.azurecr.io/docker-sonic-snmp 20201231.84 507f8d28bf6e",
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
            DOCKER_IMAGES_CMD,
            ["docker", "rmi", "744d3a09062f", "--force"]
        ],
        common_test.PROC_OUT: [
            "sonick8scue.azurecr.io/docker-sonic-snmp 20201231.74 507f8d28bf6e\n"
            "sonick8scue.azurecr.io/docker-sonic-snmp 20201231.96 744d3a09062f\n"
            "sonick8scue.azurecr.io/docker-sonic-snmp 20201231.84 507f8d28bf6e",
            ""
        ],
        common_test.PROC_CODE: [
            0,
            1
        ]
    },
    2: {
        common_test.DESCR: "Clean image failed(docker images command failed)",
        common_test.RETVAL: 1,
        common_test.ARGS: ["snmp", "20201231.84", "20201231.74"],
        common_test.PROC_CMD: [
            DOCKER_IMAGES_CMD
        ],
        common_test.PROC_OUT: [
            ""
        ],
        common_test.PROC_ERR: [
            "Cannot connect to the Docker daemon"
        ],
        common_test.PROC_CODE: [
            1
        ]
    },
    3: {
        common_test.DESCR: "Clean image (current image doesn't exist)",
        common_test.RETVAL: 0,
        common_test.ARGS: ["snmp", "20201231.84", "20201231.74"],
        common_test.PROC_CMD: [
            DOCKER_IMAGES_CMD
        ],
        common_test.PROC_OUT: [
            "sonick8scue.azurecr.io/docker-sonic-snmp 20201231.74 507f8d28bf6e\n"
            "sonick8scue.azurecr.io/docker-sonic-snmp 20201231.96 744d3a09062f"
        ],
        common_test.PROC_CODE: [
            0
        ]
    },
    4: {
        common_test.DESCR: "Clean image (no images match feat, unrelated images present)",
        common_test.RETVAL: 0,
        common_test.ARGS: ["snmp", "20201231.84", "20201231.74"],
        common_test.PROC_CMD: [
            DOCKER_IMAGES_CMD
        ],
        common_test.PROC_OUT: [
            "sonick8scue.azurecr.io/docker-sonic-telemetry 20201231.74 507f8d28bf6e\n"
            "sonick8scue.azurecr.io/docker-sonic-bgp 20201231.96 744d3a09062f"
        ],
        common_test.PROC_CODE: [
            0
        ]
    },
    5: {
        common_test.DESCR: "Clean image successfuly(local to kube)",
        common_test.RETVAL: 0,
        common_test.ARGS: ["snmp", "20201231.84", ""],
        common_test.PROC_CMD: [
            DOCKER_IMAGES_CMD,
            ["docker", "rmi", "docker-sonic-snmp:20201231.74"]
        ],
        common_test.PROC_OUT: [
            "docker-sonic-snmp 20201231.74 507f8d28bf6e\n"
            "sonick8scue.azurecr.io/docker-sonic-snmp 20201231.84 507f8d28bf6e",
            ""
        ],
        common_test.PROC_CODE: [
            0,
            0
        ]
    },
    6: {
        common_test.DESCR: "Clean image successfuly(local to dry-kube to kube)",
        common_test.RETVAL: 0,
        common_test.ARGS: ["snmp", "20201231.84", "20201231.74"],
        common_test.PROC_CMD: [
            DOCKER_IMAGES_CMD,
            ["docker", "rmi", "sonick8scue.azurecr.io/docker-sonic-snmp:20201231.74"],
            ["docker", "tag", "507f8d28bf6e", "sonick8scue.azurecr.io/docker-sonic-snmp:20201231.74"],
            ["docker", "rmi", "docker-sonic-snmp:20201231.74"]
        ],
        common_test.PROC_OUT: [
            "docker-sonic-snmp 20201231.74 507f8d28bf6e\n"
            "sonick8scue.azurecr.io/docker-sonic-snmp 20201231.74 507f8d28bf6f\n"
            "sonick8scue.azurecr.io/docker-sonic-snmp 20201231.84 507f8d28bf6g",
            "",
            "",
            ""
        ],
        common_test.PROC_CODE: [
            0,
            0,
            0,
            0
        ]
    },
    7: {
        common_test.DESCR: "Clean image failed(dry-kube step1: remove remote failed, no further steps run)",
        common_test.RETVAL: 1,
        common_test.ARGS: ["snmp", "20201231.84", "20201231.74"],
        common_test.PROC_CMD: [
            DOCKER_IMAGES_CMD,
            ["docker", "rmi", "sonick8scue.azurecr.io/docker-sonic-snmp:20201231.74"]
        ],
        common_test.PROC_OUT: [
            "docker-sonic-snmp 20201231.74 507f8d28bf6e\n"
            "sonick8scue.azurecr.io/docker-sonic-snmp 20201231.74 507f8d28bf6f\n"
            "sonick8scue.azurecr.io/docker-sonic-snmp 20201231.84 507f8d28bf6g",
            ""
        ],
        common_test.PROC_CODE: [
            0,
            1
        ]
    },
    8: {
        common_test.DESCR: "Clean image failed(dry-kube step2: tag failed, no further steps run)",
        common_test.RETVAL: 1,
        common_test.ARGS: ["snmp", "20201231.84", "20201231.74"],
        common_test.PROC_CMD: [
            DOCKER_IMAGES_CMD,
            ["docker", "rmi", "sonick8scue.azurecr.io/docker-sonic-snmp:20201231.74"],
            ["docker", "tag", "507f8d28bf6e", "sonick8scue.azurecr.io/docker-sonic-snmp:20201231.74"]
        ],
        common_test.PROC_OUT: [
            "docker-sonic-snmp 20201231.74 507f8d28bf6e\n"
            "sonick8scue.azurecr.io/docker-sonic-snmp 20201231.74 507f8d28bf6f\n"
            "sonick8scue.azurecr.io/docker-sonic-snmp 20201231.84 507f8d28bf6g",
            "",
            ""
        ],
        common_test.PROC_CODE: [
            0,
            0,
            1
        ]
    },
    9: {
        common_test.DESCR: "Clean image failed(dry-kube step3: remove local failed)",
        common_test.RETVAL: 1,
        common_test.ARGS: ["snmp", "20201231.84", "20201231.74"],
        common_test.PROC_CMD: [
            DOCKER_IMAGES_CMD,
            ["docker", "rmi", "sonick8scue.azurecr.io/docker-sonic-snmp:20201231.74"],
            ["docker", "tag", "507f8d28bf6e", "sonick8scue.azurecr.io/docker-sonic-snmp:20201231.74"],
            ["docker", "rmi", "docker-sonic-snmp:20201231.74"]
        ],
        common_test.PROC_OUT: [
            "docker-sonic-snmp 20201231.74 507f8d28bf6e\n"
            "sonick8scue.azurecr.io/docker-sonic-snmp 20201231.74 507f8d28bf6f\n"
            "sonick8scue.azurecr.io/docker-sonic-snmp 20201231.84 507f8d28bf6g",
            "",
            "",
            ""
        ],
        common_test.PROC_CODE: [
            0,
            0,
            0,
            1
        ]
    },
    10: {
        common_test.DESCR: "Clean image failed(malformed docker images output)",
        common_test.RETVAL: 1,
        common_test.ARGS: ["snmp", "20201231.84", "20201231.74"],
        common_test.PROC_CMD: [
            DOCKER_IMAGES_CMD
        ],
        common_test.PROC_OUT: [
            "sonick8scue.azurecr.io/docker-sonic-snmp 20201231.74\n"
            "sonick8scue.azurecr.io/docker-sonic-snmp 20201231.96 744d3a09062f"
        ],
        common_test.PROC_CODE: [
            0
        ]
    },
    11: {
        common_test.DESCR: "Clean image successfuly(no stale images to remove)",
        common_test.RETVAL: 0,
        common_test.ARGS: ["snmp", "20201231.84", "20201231.74"],
        common_test.PROC_CMD: [
            DOCKER_IMAGES_CMD
        ],
        common_test.PROC_OUT: [
            "sonick8scue.azurecr.io/docker-sonic-snmp 20201231.84 507f8d28bf6e\n"
            "sonick8scue.azurecr.io/docker-sonic-snmp 20201231.74 507f8d28bf6f"
        ],
        common_test.PROC_CODE: [
            0
        ]
    },
    12: {
        common_test.DESCR: "Clean image successfuly(multiple stale images removed as independent argv elements)",
        common_test.RETVAL: 0,
        common_test.ARGS: ["snmp", "20201231.84", "20201231.74"],
        common_test.PROC_CMD: [
            DOCKER_IMAGES_CMD,
            ["docker", "rmi", "id2", "id3", "--force"]
        ],
        common_test.PROC_OUT: [
            "sonick8scue.azurecr.io/docker-sonic-snmp 20201231.84 id0\n"
            "sonick8scue.azurecr.io/docker-sonic-snmp 20201231.74 id1\n"
            "sonick8scue.azurecr.io/docker-sonic-snmp 20201231.60 id2\n"
            "sonick8scue.azurecr.io/docker-sonic-snmp 20201231.50 id3",
            ""
        ],
        common_test.PROC_CODE: [
            0,
            0
        ]
    },
    13: {
        common_test.DESCR: "Clean image (feat contains quote/semicolon injection payload, not interpolated)",
        common_test.RETVAL: 0,
        common_test.ARGS: ['snmp"; rm -rf / #', "20201231.84", "20201231.74"],
        common_test.PROC_CMD: [
            DOCKER_IMAGES_CMD
        ],
        common_test.PROC_OUT: [
            "sonick8scue.azurecr.io/docker-sonic-telemetry 20201231.74 507f8d28bf6e"
        ],
        common_test.PROC_CODE: [
            0
        ]
    },
    14: {
        common_test.DESCR: "Clean image (feat contains pipe injection payload, not interpolated)",
        common_test.RETVAL: 0,
        common_test.ARGS: ["snmp | cat /etc/passwd", "20201231.84", "20201231.74"],
        common_test.PROC_CMD: [
            DOCKER_IMAGES_CMD
        ],
        common_test.PROC_OUT: [
            "sonick8scue.azurecr.io/docker-sonic-telemetry 20201231.74 507f8d28bf6e"
        ],
        common_test.PROC_CODE: [
            0
        ]
    },
    15: {
        common_test.DESCR: "Clean image (feat contains command substitution injection payload, not interpolated)",
        common_test.RETVAL: 0,
        common_test.ARGS: ["snmp$(reboot)", "20201231.84", "20201231.74"],
        common_test.PROC_CMD: [
            DOCKER_IMAGES_CMD
        ],
        common_test.PROC_OUT: [
            "sonick8scue.azurecr.io/docker-sonic-telemetry 20201231.74 507f8d28bf6e"
        ],
        common_test.PROC_CODE: [
            0
        ]
    },
    16: {
        common_test.DESCR: "Clean image (feat contains backtick injection payload, not interpolated)",
        common_test.RETVAL: 0,
        common_test.ARGS: ["snmp`reboot`", "20201231.84", "20201231.74"],
        common_test.PROC_CMD: [
            DOCKER_IMAGES_CMD
        ],
        common_test.PROC_OUT: [
            "sonick8scue.azurecr.io/docker-sonic-telemetry 20201231.74 507f8d28bf6e"
        ],
        common_test.PROC_CODE: [
            0
        ]
    },
    17: {
        common_test.DESCR: "Clean image (feat contains spaces/newline injection payload, not interpolated)",
        common_test.RETVAL: 0,
        common_test.ARGS: ["snmp\nreboot", "20201231.84", "20201231.74"],
        common_test.PROC_CMD: [
            DOCKER_IMAGES_CMD
        ],
        common_test.PROC_OUT: [
            "sonick8scue.azurecr.io/docker-sonic-telemetry 20201231.74 507f8d28bf6e"
        ],
        common_test.PROC_CODE: [
            0
        ]
    },
    18: {
        common_test.DESCR: "Clean image (repository containing shell metacharacters stays one argv element)",
        common_test.RETVAL: 0,
        common_test.ARGS: ["snmp", "20201231.84", ""],
        common_test.PROC_CMD: [
            DOCKER_IMAGES_CMD,
            ["docker", "rmi", "docker-sonic-snmp$(reboot):20201231.74"]
        ],
        common_test.PROC_OUT: [
            "sonick8scue.azurecr.io/docker-sonic-snmp 20201231.84 507f8d28bf6f\n"
            "docker-sonic-snmp$(reboot) 20201231.74 507f8d28bf6e",
            ""
        ],
        common_test.PROC_CODE: [
            0,
            0
        ]
    },
    19: {
        common_test.DESCR: "Clean image failed(docker images times out)",
        common_test.RETVAL: 1,
        common_test.ARGS: ["snmp", "20201231.84", "20201231.74"],
        common_test.PROC_CMD: [
            DOCKER_IMAGES_CMD
        ],
        common_test.TRIGGER_THROW: True
    },
}

is_ready_as_k8s_node_test_data = {
    0: {
        common_test.DESCR: "node is ready",
        common_test.RETVAL: True,
        common_test.PROC_CMD: [
            ["kubectl", "--kubeconfig", KUBE_ADMIN_CONF, "get", "nodes",
             "--no-headers", "--", "none"]
        ],
        common_test.PROC_OUT: ["none   Ready   <role>   10d   v1.28.0"],
        common_test.PROC_KILLED: 0
    },
    1: {
        common_test.DESCR: "node is not ready",
        common_test.RETVAL: False,
        common_test.PROC_CMD: [
            ["kubectl", "--kubeconfig", KUBE_ADMIN_CONF, "get", "nodes",
             "--no-headers", "--", "none"]
        ],
        common_test.PROC_OUT: ["none   NotReady   <role>   10d   v1.28.0"],
        common_test.PROC_KILLED: 0
    },
    2: {
        common_test.DESCR: "kubectl fails (ret != 0)",
        common_test.RETVAL: False,
        common_test.PROC_CMD: [
            ["kubectl", "--kubeconfig", KUBE_ADMIN_CONF, "get", "nodes",
             "--no-headers", "--", "none"]
        ],
        common_test.PROC_ERR: ["connection refused"],
        common_test.PROC_KILLED: 0
    },
    3: {
        common_test.DESCR: "empty output",
        common_test.RETVAL: False,
        common_test.PROC_CMD: [
            ["kubectl", "--kubeconfig", KUBE_ADMIN_CONF, "get", "nodes",
             "--no-headers", "--", "none"]
        ],
        common_test.PROC_OUT: [""],
        common_test.PROC_KILLED: 0
    },
    4: {
        common_test.DESCR: "kubectl timeout",
        common_test.TRIGGER_THROW: True,
        common_test.RETVAL: False,
        common_test.PROC_CMD: [
            ["kubectl", "--kubeconfig", KUBE_ADMIN_CONF, "get", "nodes",
             "--no-headers", "--", "none"]
        ],
        common_test.PROC_KILLED: 1
    }
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
        kube_commands.KUBE_ADMIN_CONF = KUBE_ADMIN_CONF
        kube_commands.AME_CRT = AME_CRT
        kube_commands.AME_KEY = AME_KEY



    @patch("kube_commands.subprocess.Popen")
    def test_is_ready_as_k8s_node(self, mock_subproc):
        self.init()
        common_test.set_kube_mock(mock_subproc)

        for (i, ct_data) in is_ready_as_k8s_node_test_data.items():
            common_test.do_start_test("kube:is_ready_as_k8s_node", i, ct_data)

            result = kube_commands.is_ready_as_k8s_node()

            if common_test.RETVAL in ct_data:
                assert result == ct_data[common_test.RETVAL], (
                    "Test {}: expected {} got {}".format(i, ct_data[common_test.RETVAL], result))

            if common_test.PROC_KILLED in ct_data:
                assert common_test.procs_killed == ct_data[common_test.PROC_KILLED]

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

    @patch("kube_commands.requests.Session")
    @patch("kube_commands.swsscommon.DBConnector")
    @patch("kube_commands.swsscommon.Table")
    @patch("kube_commands.subprocess.Popen")
    def test_join(self, mock_subproc, mock_table, mock_conn, mock_session):
        self.init()
        common_test.set_kube_mock(mock_subproc, mock_table, mock_conn,
                                  mock_session)

        for (i, ct_data) in join_test_data.items():
            lock_file = ""
            common_test.do_start_test("kube:join", i, ct_data)
            mock_session.reset_mock()

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

            if REQUEST_VERIFY in ct_data:
                mock_session.assert_called_once()
                request_get = mock_session.return_value.get
                request_get.assert_called_once()
                request_kwargs = request_get.call_args[1]
                assert request_kwargs["cert"] == (AME_CRT, AME_KEY)
                assert request_kwargs["timeout"] == \
                    kube_commands.K8S_CA_TIMEOUT
                if ct_data[REQUEST_VERIFY]:
                    assert request_kwargs.get("verify", True) is True
                else:
                    assert request_kwargs["verify"] is False
            else:
                mock_session.assert_not_called()

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

    def test_tls_verification_failure_does_not_create_kubeconfig(self):
        tls_error = kube_commands.requests.exceptions.SSLError(
                "certificate verify failed")

        with patch("kube_commands.requests.Session") as request_session, \
                patch("kube_commands.tempfile.mkstemp") as mock_mkstemp, \
                patch("kube_commands.shutil.copyfile") as mock_copyfile:
            request_session.return_value.get.side_effect = tls_error
            with pytest.raises(kube_commands.requests.exceptions.SSLError):
                kube_commands._gen_cli_kubeconf(
                        "10.3.157.24", 6443, "false")

        request_session.return_value.close.assert_called_once_with()
        mock_mkstemp.assert_not_called()
        mock_copyfile.assert_not_called()


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

    def test_reset_hostname_cannot_inject_commands(self):
        self.init()
        hostile_hostname = "sonic; touch /tmp/pwned #"

        with patch("kube_commands.os.path.exists", return_value=True), \
                patch("kube_commands.get_device_name",
                      return_value=hostile_hostname), \
                patch("kube_commands._run_command_list") as run_list:
            kube_commands._do_reset()

        # An invalid (non DNS-1123) hostname must not reach kubectl/kubeadm
        # at all; _do_reset() should refuse and no-op instead of passing
        # the hostile value through argv.
        assert run_list.call_args_list == []

    def test_run_command_list_disables_shell_and_enforces_timeout(self):
        proc = MagicMock()
        proc.communicate.return_value = (b"", b"")
        proc.returncode = 0

        with patch("kube_commands.subprocess.Popen",
                   return_value=proc) as popen:
            kube_commands._run_command_list(["kubectl", "version"],
                                            timeout=17)

        popen.assert_called_once_with(
            ["kubectl", "version"], shell=False, stdout=kube_commands.subprocess.PIPE,
            stderr=kube_commands.subprocess.PIPE)
        proc.communicate.assert_called_once_with(timeout=17)

    @pytest.mark.parametrize("server, expected_server", [
        ("10.3.157.24", "10.3.157.24"),
        ("192.168.122.11", "192.168.122.11"),
        ("fd00::10", "fd00::10"),
        ("[fd00::10]", "fd00::10"),
    ])
    def test_validate_server_destination_allows_private_addresses(
            self, server, expected_server):
        assert kube_commands._validate_server_destination(
            server, "6443") == (
                expected_server, 6443, (expected_server,))

    @pytest.mark.parametrize("server", [
        "127.0.0.1",
        "169.254.169.254",
        "0.0.0.0",
        "224.0.0.1",
        "::1",
        "fe80::1",
        "::",
        "ff02::1",
        "::ffff:169.254.169.254",
    ])
    def test_validate_server_destination_rejects_prohibited_addresses(
            self, server):
        with pytest.raises(IOError, match="prohibited address"):
            kube_commands._validate_server_destination(server, 6443)

    def test_validate_server_destination_rejects_unsafe_dns_resolution(self):
        addrinfo = [
            (socket.AF_INET, socket.SOCK_STREAM, 6, "",
             ("10.3.157.24", 6443)),
            (socket.AF_INET, socket.SOCK_STREAM, 6, "",
             ("169.254.169.254", 6443)),
        ]
        with patch("kube_commands.socket.getaddrinfo",
                   return_value=addrinfo):
            with pytest.raises(IOError, match="prohibited address"):
                kube_commands._validate_server_destination(
                    "k8s.example.com", 6443)

    def test_validate_server_destination_rejects_malformed_hostname(self):
        server = "{}.example.com".format("a" * 64)
        with patch("kube_commands.socket.getaddrinfo",
                   side_effect=UnicodeError("label empty or too long")):
            with pytest.raises(IOError, match="cannot be resolved"):
                kube_commands._validate_server_destination(server, 6443)

    def test_do_join_handles_malformed_hostname(self):
        server = "{}.example.com".format("a" * 64)
        with patch("kube_commands._get_validated_device_name",
                   return_value="sonic"), \
                patch("kube_commands.socket.getaddrinfo",
                      side_effect=UnicodeError("label empty or too long")), \
                patch("kube_commands._troubleshoot_tips"), \
                patch("kube_commands._do_reset") as reset:
            ret, out, error = kube_commands._do_join(
                server, 6443, "false")

        assert ret == -1
        assert out == ""
        assert "cannot be resolved" in error
        reset.assert_called_once_with()

    def test_validate_server_destination_preserves_safe_dns_addresses(self):
        addrinfo = [
            (socket.AF_INET6, socket.SOCK_STREAM, 6, "",
             ("2001:4860:4860::8888", 6443, 0, 0)),
            (socket.AF_INET, socket.SOCK_STREAM, 6, "",
             ("10.3.157.24", 6443)),
            (socket.AF_INET, socket.SOCK_STREAM, 6, "",
             ("10.3.157.24", 6443)),
        ]
        with patch("kube_commands.socket.getaddrinfo",
                   return_value=addrinfo):
            assert kube_commands._validate_server_destination(
                "k8s.example.com", "6443") == (
                    "k8s.example.com", 6443,
                    ("2001:4860:4860::8888", "10.3.157.24"))

    @pytest.mark.parametrize("server", [
        "[10.3.157.24]",
        "[k8s.example.com]",
        "[fd00::10",
        "fd00::10]",
    ])
    def test_validate_server_destination_rejects_invalid_brackets(
            self, server):
        with pytest.raises(IOError, match="bracket"):
            kube_commands._validate_server_destination(server, 6443)

    @pytest.mark.parametrize("server", [
        "2606:4700:4700::1111%eth0",
        "[2606:4700:4700::1111%eth0]",
    ])
    def test_validate_server_destination_rejects_ipv6_scope(
            self, server):
        with pytest.raises(IOError, match="scope identifiers"):
            kube_commands._validate_server_destination(server, 6443)

    def test_validate_server_destination_rejects_scoped_dns_answer(self):
        addrinfo = [
            (socket.AF_INET6, socket.SOCK_STREAM, 6, "",
             ("2606:4700:4700::1111", 6443, 0, 2)),
        ]
        with patch("kube_commands.socket.getaddrinfo",
                   return_value=addrinfo):
            with pytest.raises(IOError, match="scope identifiers"):
                kube_commands._validate_server_destination(
                    "k8s.example.com", 6443)

    @pytest.mark.parametrize("port", [0, 65536, "invalid", None])
    def test_validate_server_destination_rejects_invalid_port(self, port):
        with pytest.raises(IOError, match="port"):
            kube_commands._validate_server_destination(
                "10.3.157.24", port)

    @pytest.mark.parametrize("insecure", ["false", "true"])
    def test_gen_cli_kubeconf_rejects_prohibited_destination(
            self, insecure):
        with patch("kube_commands.requests.Session") as request_session:
            with pytest.raises(IOError, match="prohibited address"):
                kube_commands._gen_cli_kubeconf(
                    "169.254.169.254", 443, insecure)
        request_session.assert_not_called()

    @pytest.mark.parametrize("insecure, expected_verify", [
        ("false", True),
        ("true", False),
    ])
    @pytest.mark.parametrize("status_code", [300, 301, 305, 307, 308])
    def test_gen_cli_kubeconf_pins_dns_and_rejects_redirect(
            self, insecure, expected_verify, status_code):
        response = MagicMock(
            ok=True, is_redirect=status_code != 300,
            status_code=status_code)
        addrinfo = [
            (socket.AF_INET, socket.SOCK_STREAM, 6, "",
             ("10.3.157.24", 6443)),
        ]
        with patch("kube_commands.socket.getaddrinfo",
                   return_value=addrinfo), \
                patch("kube_commands.requests.Session") as request_session:
            request_session.return_value.get.return_value = response
            with pytest.raises(IOError, match="refused a redirect"):
                kube_commands._gen_cli_kubeconf(
                    "k8s.example.com", 6443, insecure)

        request_session.return_value.get.assert_called_once()
        args, kwargs = request_session.return_value.get.call_args
        assert args[0] == (
            "https://10.3.157.24:6443" + kube_commands.K8S_CA_PATH)
        assert kwargs["headers"] == {"Host": "k8s.example.com:6443"}
        assert kwargs["allow_redirects"] is False
        assert kwargs.get("verify", True) is expected_verify
        adapter = request_session.return_value.mount.call_args[0][1]
        assert adapter._server_hostname == "k8s.example.com"
        request_session.return_value.close.assert_called_once_with()

    @pytest.mark.parametrize("server", [
        "2001:4860:4860::8888",
        "[2001:4860:4860::8888]",
    ])
    def test_gen_cli_kubeconf_formats_pinned_ipv6_endpoint(
            self, server):
        response = MagicMock(
            ok=True, is_redirect=True, status_code=302)
        with patch("kube_commands.requests.Session") as request_session:
            request_session.return_value.get.return_value = response
            with pytest.raises(IOError, match="refused a redirect"):
                kube_commands._gen_cli_kubeconf(
                    server, 6443, "false")

        args, kwargs = request_session.return_value.get.call_args
        assert args[0] == (
            "https://[2001:4860:4860::8888]:6443" +
            kube_commands.K8S_CA_PATH)
        assert kwargs["headers"] == {
            "Host": "[2001:4860:4860::8888]:6443"}
        adapter = request_session.return_value.mount.call_args[0][1]
        assert adapter._server_hostname == "2001:4860:4860::8888"

    def test_gen_cli_kubeconf_writes_bracketed_ipv6_server(
            self, tmp_path):
        response = MagicMock(
            ok=True, is_redirect=False, status_code=200)
        response.json.return_value = {"data": {"ca.crt": "test"}}
        request_session = MagicMock()
        request_session.get.return_value = response
        cert = tmp_path / "client.crt"
        key = tmp_path / "client.key"
        kubeconf = tmp_path / "kube_admin.conf"
        generated = tmp_path / "generated"
        cert.write_bytes(b"client certificate")
        key.write_bytes(b"client key")
        generated_fd = os.open(
            str(generated), os.O_CREAT | os.O_RDWR, 0o600)

        with patch("kube_commands.requests.Session",
                   return_value=request_session), \
                patch.object(kube_commands, "AME_CRT", str(cert)), \
                patch.object(kube_commands, "AME_KEY", str(key)), \
                patch.object(kube_commands, "KUBE_ADMIN_CONF",
                             str(kubeconf)), \
                patch("kube_commands.tempfile.mkstemp",
                      return_value=(generated_fd, str(generated))):
            kube_commands._gen_cli_kubeconf(
                "[2001:4860:4860::8888]", 6443, "false")

        assert "server: https://[2001:4860:4860::8888]:6443" in \
            kubeconf.read_text()

    @pytest.mark.parametrize("first_error", [
        kube_commands.requests.ConnectionError("first address failed"),
        kube_commands.requests.ReadTimeout("first address timed out"),
    ])
    def test_request_k8s_ca_tries_each_validated_address(
            self, first_error):
        response = MagicMock(ok=True, is_redirect=True)
        request_session = MagicMock()
        request_session.get.side_effect = [
            first_error,
            response,
        ]
        with patch("kube_commands.requests.Session",
                   return_value=request_session):
            assert kube_commands._request_k8s_ca(
                "k8s.example.com", 6443,
                ("2001:4860:4860::8888", "10.3.157.24"),
                "false") is response

        assert [
            call[0][0] for call in request_session.get.call_args_list
        ] == [
            "https://[2001:4860:4860::8888]:6443" +
            kube_commands.K8S_CA_PATH,
            "https://10.3.157.24:6443" + kube_commands.K8S_CA_PATH,
        ]
        request_session.close.assert_called_once_with()

    def test_request_k8s_ca_uses_timeout_for_each_address(self):
        response = MagicMock(ok=True, is_redirect=True)
        request_session = MagicMock()
        request_session.get.side_effect = [
            kube_commands.requests.ReadTimeout("first address timed out"),
            response,
        ]
        with patch("kube_commands.requests.Session",
                   return_value=request_session):
            assert kube_commands._request_k8s_ca(
                "k8s.example.com", 6443,
                ("2001:4860:4860::8888", "10.3.157.24"),
                "false") is response

        timeouts = [
            call[1]["timeout"]
            for call in request_session.get.call_args_list
        ]
        assert timeouts == [
            kube_commands.K8S_CA_TIMEOUT,
            kube_commands.K8S_CA_TIMEOUT,
        ]

    def test_request_k8s_ca_does_not_retry_proxy_failure(self):
        request_session = MagicMock()
        request_session.get.side_effect = \
            kube_commands.requests.exceptions.ProxyError(
                "proxy unavailable")
        with patch("kube_commands.requests.Session",
                   return_value=request_session):
            with pytest.raises(
                    kube_commands.requests.exceptions.ProxyError):
                kube_commands._request_k8s_ca(
                    "k8s.example.com", 6443,
                    ("2001:4860:4860::8888", "10.3.157.24"),
                    "false")

        request_session.get.assert_called_once()
        request_session.close.assert_called_once_with()

    def test_request_k8s_ca_preserves_hostname_proxy_selection(self):
        response = MagicMock(ok=True, is_redirect=True)
        request_session = MagicMock()
        request_session.get.return_value = response
        proxies = {"https": "http://proxy.example.com:8080"}
        original_url = (
            "https://k8s.example.com:6443" + kube_commands.K8S_CA_PATH)
        with patch("kube_commands.requests.Session",
                   return_value=request_session), \
                patch("kube_commands.requests.utils.get_environ_proxies",
                      return_value=proxies) as get_proxies:
            assert kube_commands._request_k8s_ca(
                "k8s.example.com", 6443, ("10.3.157.24",),
                "false") is response

        get_proxies.assert_called_once_with(original_url)
        assert request_session.get.call_args[1]["proxies"] == proxies

    def test_pinned_https_adapter_preserves_tls_hostname(self):
        adapter = kube_commands._PinnedHTTPSAdapter("k8s.example.com")
        assert adapter.poolmanager.connection_pool_kw["assert_hostname"] == \
            "k8s.example.com"
        assert adapter.poolmanager.connection_pool_kw["server_hostname"] == \
            "k8s.example.com"

    def test_request_k8s_ca_normalizes_absolute_tls_hostname(self):
        response = MagicMock(ok=True, is_redirect=False, status_code=200)
        request_session = MagicMock()
        request_session.get.return_value = response
        original_url = (
            "https://k8s.example.com.:6443" + kube_commands.K8S_CA_PATH)
        with patch("kube_commands.requests.Session",
                   return_value=request_session), \
                patch("kube_commands.requests.utils.get_environ_proxies",
                      return_value={}) as get_proxies:
            assert kube_commands._request_k8s_ca(
                "k8s.example.com.", 6443, ("10.3.157.24",),
                "false") is response

        adapter = request_session.mount.call_args[0][1]
        assert adapter._server_hostname == "k8s.example.com"
        assert adapter.poolmanager.connection_pool_kw["assert_hostname"] == \
            "k8s.example.com"
        assert adapter.poolmanager.connection_pool_kw["server_hostname"] == \
            "k8s.example.com"
        get_proxies.assert_called_once_with(original_url)
        assert request_session.get.call_args[1]["headers"] == {
            "Host": "k8s.example.com.:6443"}

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
