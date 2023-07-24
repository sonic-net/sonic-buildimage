import os
import sys
from unittest.mock import MagicMock, patch

import pytest

from . import common_test

sys.path.append("ctrmgr")
import ctrmgrd
import ctrmgr.ctrmgr_iptables


# ctrmgrd test cases
# NOTE: Ensure state-db entry is complete in PRE as we need to
# overwrite any context left behind from last test run.
#
server_test_data = {
    0: {
        common_test.DESCR: "Connect using init config with join delay",
        common_test.ARGS: "ctrmgrd",
        common_test.KUBE_RETURN: 1,
        common_test.PRE: {
            common_test.CONFIG_DB_NO: {
                common_test.SERVER_TABLE: {
                    common_test.SERVER_KEY: {
                        "ip": "10.10.10.10"
                    }
                }
            }
        },
        common_test.UPD: {
            common_test.CONFIG_DB_NO: {
                common_test.SERVER_TABLE: {
                    common_test.SERVER_KEY: {
                        "ip": "10.10.10.10",
                        "insecure": "true"
                    }
                }
            }
        },
        common_test.POST: {
            common_test.STATE_DB_NO: {
                common_test.SERVER_TABLE: {
                    common_test.SERVER_KEY: {
                        "connected": "true",
                        "ip": "10.10.10.10",
                        "port": "6443"
                    }
                },
                common_test.KUBE_LABEL_TABLE: {
                    "SET": {
                        "sonic_version": "20201230.111",
                        "hwsku": "mock",
                        "deployment_type": "unknown"
                    }
                }
            }
        },
        common_test.KUBE_CMD: {
            common_test.KUBE_JOIN: {
                "ip": "10.10.10.10",
                "port": "6443",
                "insecure": "true"
            }
        }
    },
    1: {
        common_test.DESCR: "Connect using init config with no join delay",
        common_test.ARGS: "ctrmgrd",
        common_test.PRE: {
            common_test.CONFIG_DB_NO: {
                common_test.SERVER_TABLE: {
                    common_test.SERVER_KEY: {
                        "ip": "10.10.10.10"
                    }
                }
            },
            common_test.STATE_DB_NO: {
                common_test.SERVER_TABLE: {
                    common_test.SERVER_KEY: {
                        "update_time": "2020-12-03 23:18:06"
                    }
                }
            }
        },
        common_test.POST: {
            common_test.STATE_DB_NO: {
                common_test.SERVER_TABLE: {
                    common_test.SERVER_KEY: {
                        "connected": "true",
                        "ip": "10.10.10.10",
                        "port": "6443"
                    }
                },
                common_test.KUBE_LABEL_TABLE: {
                    "SET": {
                        "sonic_version": "20201230.111",
                        "hwsku": "mock",
                        "deployment_type": "unknown"
                    }
                }
            }
        },
        common_test.KUBE_CMD: {
            common_test.KUBE_JOIN: {
                "ip": "10.10.10.10",
                "port": "6443",
                "insecure": "true"
            }
        }
    },
    2: {
        common_test.DESCR: "Join followed by reset on next update",
        common_test.ARGS: "ctrmgrd",
        common_test.PRE: {
            common_test.CONFIG_DB_NO: {
                common_test.SERVER_TABLE: {
                    common_test.SERVER_KEY: {
                        "ip": "10.10.10.10"
                    }
                }
            },
            common_test.STATE_DB_NO: {
                common_test.SERVER_TABLE: {
                    common_test.SERVER_KEY: {
                        "update_time": "2020-12-03 23:18:06"
                    }
                }
            }
        },
        common_test.UPD: {
            common_test.CONFIG_DB_NO: {
                common_test.SERVER_TABLE: {
                    common_test.SERVER_KEY: {
                        "disable": "true"
                    }
                }
            }
        },
        common_test.POST: {
            common_test.STATE_DB_NO: {
                common_test.SERVER_TABLE: {
                    common_test.SERVER_KEY: {
                        "connected": "false"
                    }
                }
            }
        },
        common_test.KUBE_CMD: {
            common_test.KUBE_JOIN: {
                "ip": "10.10.10.10",
                "port": "6443",
                "insecure": "true"
            },
            common_test.KUBE_RESET: {
                "flag": "true"
            }
        }
    }
}


feature_test_data = {
    0: {
        common_test.DESCR: "set_owner = local with current_owner != local",
        common_test.ARGS: "ctrmgrd",
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
                        "current_owner": "not local"
                    }
                }
            }
        },
        common_test.UPD: {
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
                        "current_owner": "not local"
                    }
                }
            }
        },
        common_test.POST: {
            common_test.STATE_DB_NO: {
                common_test.FEATURE_TABLE: {
                    "snmp": {
                        "restart": "true"
                    }
                },
                common_test.KUBE_LABEL_TABLE: {
                    "SET": {
                        "snmp_enabled": "false"
                    }
                }
            }
        }
    },
    1: {
        common_test.DESCR: "set_owner = kube with pending remote state",
        common_test.ARGS: "ctrmgrd",
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
                        "current_owner": "any",
                        "remote_state": "pending"
                    }
                }
            }
        },
        common_test.POST: {
            common_test.STATE_DB_NO: {
                common_test.FEATURE_TABLE: {
                    "snmp": {
                        "restart": "true"
                    }
                },
                common_test.KUBE_LABEL_TABLE: {
                    "SET": {
                        "snmp_enabled": "true"
                    }
                }
            }
        }
    },
    2: {
        common_test.DESCR: "Run with systemstate inactive. No-op",
        common_test.ARGS: "ctrmgrd",
        common_test.ACTIVE: 1,
        common_test.PRE: {
            common_test.CONFIG_DB_NO: {
                common_test.FEATURE_TABLE: {
                    "snmp": {
                        "set_owner": "any"
                    }
                }
            },
            common_test.STATE_DB_NO: {
                common_test.FEATURE_TABLE: {
                    "snmp": {
                        "current_owner": "any"
                    }
                }
            }
        },
        common_test.POST: {
            common_test.STATE_DB_NO: {
                common_test.FEATURE_TABLE: {
                    "snmp": {
                        "current_owner": "any"
                    }
                }
            }
        }
    },
    3: {
        common_test.DESCR: "Tag image latest when remote_state changes to running",
        common_test.ARGS: "ctrmgrd",
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
                        "remote_state": "ready"
                    }
                }
            }
        },
        common_test.UPD: {
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
                        "remote_state": "running",
                        "container_version": "20201231.74",
                        "container_stable_version": "20201231.64"
                    }
                }
            }
        },
        common_test.POST: {
            common_test.STATE_DB_NO: {
                common_test.FEATURE_TABLE: {
                    "snmp": {
                        "container_last_version": "20201231.64",
                        "container_stable_version": "20201231.74"
                    }
                }
            }
        }
    },
    4: {
        common_test.DESCR: "Restart immediately to go back to local when remote_state changes to none from stopped",
        common_test.ARGS: "ctrmgrd",
        common_test.PRE: {
            common_test.STATE_DB_NO: {
                common_test.FEATURE_TABLE: {
                    "snmp": {
                        "remote_state": "stopped",
                    }
                }
            }
        },
        common_test.UPD: {
            common_test.STATE_DB_NO: {
                common_test.FEATURE_TABLE: {
                    "snmp": {
                        "remote_state": "none",
                    }
                }
            }
        },
        common_test.POST: {
            common_test.STATE_DB_NO: {
                common_test.FEATURE_TABLE: {
                    "snmp": {
                        "restart": "true"
                    }
                }
            }
        }
    }
}


labels_test_data = {
    0: {
        common_test.DESCR: "simple update",
        common_test.ARGS: "ctrmgrd",
        common_test.CONNECTED: True,
        common_test.PRE: {
            common_test.STATE_DB_NO: {
                common_test.KUBE_LABEL_TABLE: {
                    "SET": {
                        "foo": "bar",
                        "hello": "world"
                    }
                }
            }
        },
        common_test.UPD: {
            common_test.STATE_DB_NO: {
                "xyz": {
                    "xxx": {
                        "foo": "bar",
                        "hello": "world"
                    }
                }
            }
        },
        common_test.POST: {
            common_test.STATE_DB_NO: {
                common_test.KUBE_LABEL_TABLE: {
                    "SET": {
                        "foo": "bar",
                        "hello": "world"
                    }
                }
            }
        },
        common_test.KUBE_CMD: {
            common_test.KUBE_WR: {
                "foo": "bar",
                "hello": "world"
            }
        }
    },
    1: {
        common_test.DESCR: "simple update -  not connected",
        common_test.ARGS: "ctrmgrd",
        common_test.CONNECTED: False,
        common_test.PRE: {
            common_test.STATE_DB_NO: {
                common_test.KUBE_LABEL_TABLE: {
                    "SET": {
                        "foo": "bar",
                        "hello": "world"
                    }
                }
            }
        },
        common_test.POST: {
            common_test.STATE_DB_NO: {
                common_test.KUBE_LABEL_TABLE: {
                    "SET": {
                        "foo": "bar",
                        "hello": "world"
                    }
                }
            }
        }
    },
    2: {
        common_test.DESCR: "Emulate write failure",
        common_test.ARGS: "ctrmgrd",
        common_test.CONNECTED: True,
        common_test.KUBE_RETURN: 1,
        common_test.PRE: {
            common_test.STATE_DB_NO: {
                common_test.KUBE_LABEL_TABLE: {
                    "SET": {
                        "foo": "bar",
                        "hello": "world"
                    }
                }
            }
        },
        common_test.POST: {
            common_test.STATE_DB_NO: {
                common_test.KUBE_LABEL_TABLE: {
                    "SET": {
                        "foo": "bar",
                        "hello": "world"
                    }
                }
            }
        },
        common_test.KUBE_CMD: {
            common_test.KUBE_WR: {
                "foo": "bar",
                "hello": "world"
            }
        }
    }
}


class TestContainerStartup(object):

    def init(self):
        ctrmgrd.UNIT_TESTING = 1
        ctrmgrd.SONIC_CTR_CONFIG = (
                common_test.create_remote_ctr_config_json())
        ctrmgr.ctrmgr_iptables.UNIT_TESTING = 1


    def clear(self):
        ctrmgr.ctrmgr_iptables.UNIT_TESTING = 0


    @patch("ctrmgrd.swsscommon.DBConnector")
    @patch("ctrmgrd.swsscommon.Table")
    @patch("ctrmgrd.swsscommon.Select")
    @patch("ctrmgrd.swsscommon.SubscriberStateTable")
    @patch("ctrmgrd.kube_commands.kube_reset_master")
    @patch("ctrmgrd.kube_commands.kube_join_master")
    @patch("ctrmgrd.kube_commands.kube_write_labels")
    def test_server(self, mock_kube_wr, mock_kube_join, mock_kube_rst, mock_subs,
            mock_select, mock_table, mock_conn):
        self.init()
        ret = 0
        common_test.set_mock(mock_table, mock_conn)
        common_test.set_mock_sel(mock_select, mock_subs)
        common_test.set_mock_kube(mock_kube_wr, mock_kube_join, mock_kube_rst)
        common_test.mock_selector.SLEEP_SECS = 1
        for (i, ct_data) in server_test_data.items():
            common_test.do_start_test("ctrmgrd:server", i, ct_data)

            if common_test.KUBE_RETURN in ct_data:
                common_test.kube_return = ct_data[common_test.KUBE_RETURN]
            else:
                common_test.kube_return = 0

            with patch('sys.argv', ct_data[common_test.ARGS].split()):
                ret = ctrmgrd.main()

            ret = common_test.check_tables_returned()
            assert ret == 0

            ret = common_test.check_kube_actions()
            assert ret == 0
            common_test.mock_selector.SLEEP_SECS = 0
        self.clear()


    @patch("ctrmgrd.swsscommon.DBConnector")
    @patch("ctrmgrd.swsscommon.Table")
    @patch("ctrmgrd.swsscommon.Select")
    @patch("ctrmgrd.swsscommon.SubscriberStateTable")
    @patch("ctrmgrd.kube_commands.kube_reset_master")
    @patch("ctrmgrd.kube_commands.kube_join_master")
    @patch("ctrmgrd.kube_commands.kube_write_labels")
    @patch("ctrmgrd.kube_commands.tag_latest")
    @patch("ctrmgrd.kube_commands.clean_image")
    def test_feature(self, mock_clean_image, mock_tag_latest, mock_kube_wr, mock_kube_join, mock_kube_rst, mock_subs,
            mock_select, mock_table, mock_conn):
        self.init()
        ret = 0
        common_test.set_mock(mock_table, mock_conn)
        common_test.set_mock_sel(mock_select, mock_subs)
        common_test.set_mock_kube(mock_kube_wr, mock_kube_join, mock_kube_rst)
        common_test.set_mock_image_op(mock_clean_image, mock_tag_latest)

        for (i, ct_data) in feature_test_data.items():
            common_test.do_start_test("ctrmgrd:feature", i, ct_data)

            if common_test.ACTIVE in ct_data:
                ctrmgrd.UNIT_TESTING_ACTIVE = ct_data[common_test.ACTIVE]
                print("systemctl active = {}".format(ctrmgrd.UNIT_TESTING_ACTIVE))

            with patch('sys.argv', ct_data[common_test.ARGS].split()):
                ret = ctrmgrd.main()

            ret = common_test.check_tables_returned()
            assert ret == 0
        self.clear()


    @patch("ctrmgrd.swsscommon.DBConnector")
    @patch("ctrmgrd.swsscommon.Table")
    @patch("ctrmgrd.swsscommon.Select")
    @patch("ctrmgrd.swsscommon.SubscriberStateTable")
    @patch("ctrmgrd.kube_commands.kube_reset_master")
    @patch("ctrmgrd.kube_commands.kube_join_master")
    @patch("ctrmgrd.kube_commands.kube_write_labels")
    def test_labels(self, mock_kube_wr, mock_kube_join, mock_kube_rst, mock_subs,
            mock_select, mock_table, mock_conn):
        self.init()
        ret = 0
        common_test.set_mock(mock_table, mock_conn)
        common_test.set_mock_sel(mock_select, mock_subs)
        common_test.set_mock_kube(mock_kube_wr, mock_kube_join, mock_kube_rst)

        for (i, ct_data) in labels_test_data.items():
            common_test.do_start_test("ctrmgrd:feature", i, ct_data)

            ctrmgrd.remote_connected = ct_data[common_test.CONNECTED]
            print("remote_connected= {}".format(ctrmgrd.remote_connected))

            if common_test.KUBE_RETURN in ct_data:
                common_test.kube_return = ct_data[common_test.KUBE_RETURN]
            else:
                common_test.kube_return = 0

            with patch('sys.argv', ct_data[common_test.ARGS].split()):
                ret = ctrmgrd.main()

            ret = common_test.check_tables_returned()
            assert ret == 0

            ret = common_test.check_kube_actions()
            assert ret == 0
        self.clear()
