import os
from unittest.mock import MagicMock, patch

import pytest
from sonic_py_common.general import load_module_from_source

from . import common_test


load_module_from_source("docker",
        os.path.join(os.path.dirname(os.path.realpath(__file__)), "mock_docker.py"))
container = load_module_from_source("container",
        os.path.join(os.path.dirname(os.path.realpath(__file__)), "../ctrmgr/container"))


# container_start test cases
#
start_test_data = {
    0: {
        common_test.DESCR: "container start for local",
        common_test.PRE: {
            common_test.CONFIG_DB_NO: {
                common_test.FEATURE_TABLE: {
                    "snmp": {
                        "set_owner": "local"
                    }
                }
            },
            common_test.STATE_DB_NO: {
                common_test.FEATURE_TABLE: {
                    "snmp": {
                        "remote_state": "xxx"
                    }
                }
            }
            },
        common_test.POST: {
            common_test.CONFIG_DB_NO: {
                common_test.FEATURE_TABLE: {
                    "snmp": {
                        "set_owner": "local"
                    }
                }
            },
            common_test.STATE_DB_NO: {
                common_test.FEATURE_TABLE: {
                    "snmp": {
                        "remote_state": "none",
                        "system_state": "up",
                        "current_owner": "local",
                        "container_id": "snmp"
                    }
                },
                common_test.KUBE_LABEL_TABLE: {
                    "SET": {
                        "snmp_enabled": "false"
                    }
                }
            }
        },
        common_test.ACTIONS: {
            "snmp": [ "start" ]
        }
    },
    1: {
        common_test.DESCR: "container start for kube with fallback possible",
        common_test.PRE: {
            common_test.CONFIG_DB_NO: {
                common_test.FEATURE_TABLE: {
                    "snmp": {
                        "set_owner": "kube"
                    }
                }
            },
            common_test.STATE_DB_NO: {
                common_test.FEATURE_TABLE: {
                    "snmp": {
                        "remote_state": "none"
                    }
                }
            }
        },
        common_test.POST: {
            common_test.CONFIG_DB_NO: {
                common_test.FEATURE_TABLE: {
                    "snmp": {
                        "set_owner": "kube"
                    }
                }
            },
            common_test.STATE_DB_NO: {
                common_test.FEATURE_TABLE: {
                    "snmp": {
                        "remote_state": "none",
                        "system_state": "up",
                        "current_owner": "local",
                        "container_id": "snmp"
                    }
                },
                common_test.KUBE_LABEL_TABLE: {
                    "SET": {
                        "snmp_enabled": "true"
                    }
                }
            }
        },
        common_test.ACTIONS: {
            "snmp": [ "start" ]
        }
    },
    2: {
        common_test.DESCR: "start for kube with fallback *not* possible",
        common_test.PRE: {
            common_test.CONFIG_DB_NO: {
                common_test.FEATURE_TABLE: {
                    "snmp": {
                        "set_owner": "kube"
                    }
                }
            },
            common_test.STATE_DB_NO: {
                common_test.FEATURE_TABLE: {
                    "snmp": {
                        "remote_state": "stopped",
                        "current_owner": "none",
                        "container_id": ""
                    }
                },
                common_test.SERVER_TABLE: {
                    "SERVER": {
                        "connected": "true"
                    }
                }
            }
        },
        common_test.POST: {
            common_test.CONFIG_DB_NO: {
                common_test.FEATURE_TABLE: {
                    "snmp": {
                        "set_owner": "kube"
                    }
                }
            },
            common_test.STATE_DB_NO: {
                common_test.FEATURE_TABLE: {
                    "snmp": {
                        "remote_state": "stopped",
                        "system_state": "up",
                        "current_owner": "none",
                        "container_id": ""
                    }
                },
                common_test.KUBE_LABEL_TABLE: {
                    "SET": {
                        "snmp_enabled": "true"
                    }
                }
            }
        },
        common_test.ACTIONS: {
        }
    }
}


# container_stop test cases
# test case 0 -- container stop local
# test case 1 -- container stop kube
#
stop_test_data = {
    0: {
        common_test.DESCR: "container stop for local",
        common_test.PRE: {
            common_test.CONFIG_DB_NO: {
                common_test.FEATURE_TABLE: {
                    "snmp": {
                        "set_owner": "local"
                    }
                }
            },
            common_test.STATE_DB_NO: {
                common_test.FEATURE_TABLE: {
                    "snmp": {
                        "remote_state": "none",
                        "system_state": "up",
                        "current_owner": "local",
                        "container_id": "snmp",
                        "container_version": "20201230.0.15"
                    }
                }
            }
        },
        common_test.POST: {
            common_test.STATE_DB_NO: {
                common_test.FEATURE_TABLE: {
                    "snmp": {
                        "remote_state": "none",
                        "system_state": "down",
                        "current_owner": "none",
                        "container_id": "",
                        "container_version": "20201230.0.15"
                    }
                },
                common_test.KUBE_LABEL_TABLE: {
                    "SET": {
                        "snmp_enabled": "false"
                    }
                }
            }
        },
        common_test.ACTIONS: {
            "snmp": [ "stop" ]
        }
    },
    1: {
        common_test.DESCR: "container stop for kube",
        common_test.PRE: {
            common_test.CONFIG_DB_NO: {
                common_test.FEATURE_TABLE: {
                    "snmp": {
                        "set_owner": "kube"
                    }
                }
            },
            common_test.STATE_DB_NO: {
                common_test.FEATURE_TABLE: {
                    "snmp": {
                        "container_id": "xxx",
                        "system_state": "up",
                        "current_owner": "kube",
                        "remote_state": "running",
                        "container_version": "20201230.1.15"
                    }
                }
            }
        },
        common_test.POST: {
            common_test.STATE_DB_NO: {
                common_test.FEATURE_TABLE: {
                    "snmp": {
                        "remote_state": "stopped",
                        "system_state": "down",
                        "current_owner": "none",
                        "container_id": "",
                        "container_version": "20201230.1.15"
                    }
                }
            }
        },
        common_test.ACTIONS: {
            "xxx": [ "stop" ]
        }
    }
}


# container_kill test cases
# test case 0 -- container kill local
#   -- no change in state-db
#   -- no label update
# test case 1 -- container kill kube -- set label
#   -- no change in state-db
#   -- label update
#
kill_test_data = {
    0: {
        common_test.DESCR: "container kill for local",
        common_test.PRE: {
            common_test.CONFIG_DB_NO: {
                common_test.FEATURE_TABLE: {
                    "snmp": {
                        "set_owner": "local",
                        "state": "enabled"
                    }
                }
            },
            common_test.STATE_DB_NO: {
                common_test.FEATURE_TABLE: {
                    "snmp": {
                        "remote_state": "none",
                        "system_state": "up",
                        "current_owner": "local",
                        "container_id": "snmp"
                    }
                }
            }
        },
        common_test.POST: {
            common_test.STATE_DB_NO: {
                common_test.FEATURE_TABLE: {
                    "snmp": {
                        "remote_state": "none",
                        "system_state": "up",
                        "current_owner": "local",
                        "container_id": "snmp"
                    }
                },
                common_test.KUBE_LABEL_TABLE: {
                    "SET": {
                    }
                }
            }
        },
        common_test.ACTIONS: {
            "snmp": [ "kill" ]
        }
    },
    1: {
        common_test.DESCR: "container kill for kube",
        common_test.PRE: {
            common_test.CONFIG_DB_NO: {
                common_test.FEATURE_TABLE: {
                    "snmp": {
                        "set_owner": "kube"
                    }
                }
            },
            common_test.STATE_DB_NO: {
                common_test.FEATURE_TABLE: {
                    "snmp": {
                        "container_id": "xxx",
                        "system_state": "up",
                        "current_owner": "kube",
                        "remote_state": "running"
                    }
                }
            }
        },
        common_test.POST: {
            common_test.STATE_DB_NO: {
                common_test.FEATURE_TABLE: {
                    "snmp": {
                        "container_id": "xxx",
                        "system_state": "up",
                        "current_owner": "kube",
                        "remote_state": "running"
                    }
                },
                common_test.KUBE_LABEL_TABLE: {
                    "SET": {
                        "snmp_enabled": "false"
                    }
                }
            }
        },
        common_test.ACTIONS: {
            "xxx": [ "kill" ]
        }
    }
}

# container_kill test cases
# test case 0 -- container kill local disabled container
#   -- no change in state-db
#   -- no label update
#
invalid_kill_test_data = {
    0: {
        common_test.DESCR: "container kill for local disabled container",
        common_test.PRE: {
            common_test.CONFIG_DB_NO: {
                common_test.FEATURE_TABLE: {
                    "sflow": {
                        "set_owner": "local"
                    }
                }
            }
        },
        common_test.POST: {
        },
        common_test.ACTIONS: {
        }
    }
}


# container_wait test cases
# test case 0 -- container wait local
#   -- no change in state-db
#   -- no label update
# test case 1 -- container wait kube with fallback
#   -- change in state-db
#   -- no label update
#
wait_test_data = {
    0: {
        common_test.DESCR: "container wait for local",
        common_test.PRE: {
            common_test.CONFIG_DB_NO: {
                common_test.FEATURE_TABLE: {
                    "snmp": {
                        "set_owner": "local"
                    }
                }
            },
            common_test.STATE_DB_NO: {
                common_test.FEATURE_TABLE: {
                    "snmp": {
                        "remote_state": "none",
                        "system_state": "up",
                        "current_owner": "local",
                        "container_id": "snmp"
                    }
                }
            }
        },
        common_test.POST: {
            common_test.STATE_DB_NO: {
                common_test.FEATURE_TABLE: {
                    "snmp": {
                        "remote_state": "none",
                        "system_state": "up",
                        "current_owner": "local",
                        "container_id": "snmp",
                        "container_stable_version": "20201231.11"
                    }
                }
            }
        },
        common_test.ACTIONS: {
            "snmp": [ "wait" ]
        }
    },
    1: {
        common_test.DESCR: "container wait for kube",
        common_test.PRE: {
            common_test.CONFIG_DB_NO: {
                common_test.FEATURE_TABLE: {
                    "snmp": {
                        "set_owner": "kube"
                    }
                }
            },
            common_test.STATE_DB_NO: {
                common_test.FEATURE_TABLE: {
                    "snmp": {
                        "container_id": "",
                        "system_state": "up",
                        "current_owner": "kube",
                        "remote_state": "pending"
                    }
                }
            }
        },
        common_test.POST: {
            common_test.STATE_DB_NO: {
                common_test.FEATURE_TABLE: {
                    "snmp": {
                        "container_id": "",
                        "system_state": "up",
                        "current_owner": "kube",
                        "remote_state": "none"
                    }
                }
            }
        },
        common_test.ACTIONS: {
        }
    }
}


class TestContainer(object):

    def init(self):
        container.CTRMGRD_SERVICE_PATH = __file__
        container.SONIC_CTR_CONFIG = (
                common_test.create_remote_ctr_config_json())

    @patch("container.swsscommon.DBConnector")
    @patch("container.swsscommon.Table")
    @patch("container.docker.from_env")
    def test_start(self, mock_docker, mock_table, mock_conn):
        self.init()
        common_test.set_mock(mock_table, mock_conn, mock_docker)

        for (i, ct_data) in start_test_data.items():
            common_test.do_start_test("container_test:container_start", i, ct_data)

            ret = container.container_start("snmp")
            assert ret == 0

            ret = common_test.check_tables_returned()
            assert ret == 0

            ret = common_test.check_mock_containers()
            assert ret == 0


    @patch("container.swsscommon.DBConnector")
    @patch("container.swsscommon.Table")
    @patch("container.docker.from_env")
    def test_stop_ct(self, mock_docker, mock_table, mock_conn):
        self.init()
        common_test.set_mock(mock_table, mock_conn, mock_docker)

        for (i, ct_data) in stop_test_data.items():
            common_test.do_start_test("container_test:container_stop", i, ct_data)

            ret = container.container_stop("snmp")

            ret = common_test.check_tables_returned()
            assert ret == 0

            ret = common_test.check_mock_containers()
            assert ret == 0


    @patch("container.swsscommon.DBConnector")
    @patch("container.swsscommon.Table")
    @patch("container.docker.from_env")
    def test_kill(self, mock_docker, mock_table, mock_conn):
        self.init()
        common_test.set_mock(mock_table, mock_conn, mock_docker)

        for (i, ct_data) in kill_test_data.items():
            common_test.do_start_test("container_test:container_kill", i, ct_data)

            ret = container.container_kill("snmp")

            ret = common_test.check_tables_returned()
            assert ret == 0

            ret = common_test.check_mock_containers()
            assert ret == 0

    @patch("container.swsscommon.DBConnector")
    @patch("container.swsscommon.Table")
    @patch("container.docker.from_env")
    def test_invalid_kill(self, mock_docker, mock_table, mock_conn):
        self.init()
        common_test.set_mock(mock_table, mock_conn, mock_docker)

        for (i, ct_data) in invalid_kill_test_data.items():
            common_test.do_start_test("container_test:container_kill", i, ct_data)

            ret = container.container_kill("sflow")
            assert ret != 0

            ret = common_test.check_tables_returned()
            assert ret == 0

            ret = common_test.check_mock_containers()
            assert ret == 0

    @patch("container.swsscommon.DBConnector")
    @patch("container.swsscommon.Table")
    @patch("container.docker.from_env")
    def test_wait(self, mock_docker, mock_table, mock_conn):
        self.init()
        common_test.set_mock(mock_table, mock_conn, mock_docker)

        for (i, ct_data) in wait_test_data.items():
            common_test.do_start_test("container_test:container_wait", i, ct_data)

            ret = container.container_wait("snmp")

            ret = common_test.check_tables_returned()
            assert ret == 0

            ret = common_test.check_mock_containers()
            assert ret == 0


    @patch("container.swsscommon.DBConnector")
    @patch("container.swsscommon.Table")
    @patch("container.docker.from_env")
    def test_main(self, mock_docker, mock_table, mock_conn):
        self.init()
        common_test.set_mock(mock_table, mock_conn, mock_docker)

        for (k,v) in [ ("start", start_test_data),
                ("stop", stop_test_data),
                ("kill", kill_test_data),
                ("wait",  wait_test_data),
                ("id", wait_test_data)]:
            common_test.do_start_test("container_main:{}".format(k), 0, v[0])

            with patch('sys.argv', ['container', k, 'snmp']):
                container.main()
