import sys
from unittest.mock import MagicMock, patch

import pytest

from . import common_test

sys.path.append("ctrmgr")
import container_startup

# container_startup test cases
# NOTE: Ensure state-db entry is complete in PRE as we need to
# overwrite any context left behind from last test run.
#
startup_test_data = {
    0: {
        common_test.DESCR: "local container starting",
        common_test.ARGS: "container_startup -f snmp -o local -v 20201230.11",
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
                        "system_state": "up"
                    }
                }
            }
        },
        common_test.POST: {
            common_test.STATE_DB_NO: {
                common_test.FEATURE_TABLE: {
                    "snmp": {
                        "current_owner": "local",
                        "container_id": "snmp",
                        "container_version": "20201230.11"
                    }
                },
                common_test.KUBE_LABEL_TABLE: {
                    "SET": {
                        "snmp_local": "20201230.11"
                    }
                }
            }
        }
    },
    1: {
        common_test.DESCR: "kube container starting with set_owner as local",
        common_test.ARGS: "container_startup -f snmp -o kube -v any",
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
                        "container_id": "no_change",
                        "container_version": "NO_CHANGE",
                        "current_owner": "no_change",
                        "remote_state": "no_change",
                        "system_state": "up"
                    }
                },
                common_test.KUBE_LABEL_TABLE: {
                    "SET": {
                        "snmp_enabled": "no_change"
                    }
                }
            }
        },
        common_test.POST: {
            common_test.STATE_DB_NO: {
                common_test.FEATURE_TABLE: {
                    "snmp": {
                        "container_id": "no_change",
                        "container_version": "NO_CHANGE",
                        "current_owner": "no_change",
                        "remote_state": "no_change",
                        "system_state": "up"
                    }
                },
                common_test.KUBE_LABEL_TABLE: {
                    "SET": {
                        "snmp_enabled": "no_change"
                    }
                }
            }
        }
    },
    2: {
        common_test.DESCR: "kube container starting when system not up",
        common_test.ARGS: "container_startup -f snmp -o kube -v any",
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
                        "container_id": "no_change",
                        "container_version": "NO_CHANGE",
                        "current_owner": "no_change",
                        "remote_state": "no_change",
                        "system_state": "down"
                    }
                },
                common_test.KUBE_LABEL_TABLE: {
                    "SET": {
                        "snmp_enabled": "no_change"
                    }
                }
            }
        },
        common_test.POST: {
            common_test.STATE_DB_NO: {
                common_test.FEATURE_TABLE: {
                    "snmp": {
                        "container_id": "no_change",
                        "container_version": "NO_CHANGE",
                        "current_owner": "no_change",
                        "remote_state": "no_change",
                        "system_state": "down"
                    }
                },
                common_test.KUBE_LABEL_TABLE: {
                    "SET": {
                        "snmp_enabled": "no_change"
                    }
                }
            }
        }
    },
    3: {
        common_test.DESCR: "kube container starting with lower version",
        common_test.ARGS: "container_startup -f snmp -o kube -v 20201230.11",
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
                        "container_id": "no_change",
                        "container_version": "20201230.77",
                        "current_owner": "no_change",
                        "remote_state": "no_change",
                        "system_state": "up"
                    }
                },
                common_test.KUBE_LABEL_TABLE: {
                    "SET": {
                        "snmp_enabled": "no_change"
                    }
                }
            }
        },
        common_test.POST: {
            common_test.STATE_DB_NO: {
                common_test.FEATURE_TABLE: {
                    "snmp": {
                        "container_id": "no_change",
                        "container_version": "20201230.11",
                        "current_owner": "no_change",
                        "remote_state": "no_change",
                        "system_state": "up"
                    }
                },
                common_test.KUBE_LABEL_TABLE: {
                    "SET": {
                        "snmp_enabled": "no_change"
                    }
                }
            }
        }
    },
    4: {
        common_test.DESCR: "kube container starting with mode set to pending",
        common_test.ARGS: "container_startup -f snmp -o kube -v 20201230.11",
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
                        "container_id": "no_change",
                        "container_version": "20201230.10",
                        "current_owner": "no_change",
                        "remote_state": "none",
                        "system_state": "up"
                    }
                },
                common_test.KUBE_LABEL_TABLE: {
                    "SET": {
                        "snmp_enabled": "no_change"
                    }
                }
            }
        },
        common_test.POST: {
            common_test.STATE_DB_NO: {
                common_test.FEATURE_TABLE: {
                    "snmp": {
                        "container_id": "no_change",
                        "container_version": "20201230.11",
                        "current_owner": "no_change",
                        "remote_state": "pending",
                        "system_state": "up"
                    }
                },
                common_test.KUBE_LABEL_TABLE: {
                    "SET": {
                        "snmp_enabled": "no_change"
                    }
                }
            }
        }
    },
    5: {
        common_test.DESCR: "kube container starting with mode set to ready",
        common_test.ARGS: "container_startup -f snmp -o kube -v 20201230.11",
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
                        "container_id": "any",
                        "container_version": "20201230.10",
                        "current_owner": "any",
                        "remote_state": "ready",
                        "system_state": "up"
                    }
                },
                common_test.KUBE_LABEL_TABLE: {
                    "SET": {
                        "snmp_enabled": "no_change"
                    }
                }
            }
        },
        common_test.POST: {
            common_test.STATE_DB_NO: {
                common_test.FEATURE_TABLE: {
                    "snmp": {
                        "container_version": "20201230.11",
                        "current_owner": "kube",
                        "remote_state": "running",
                        "system_state": "up"
                    }
                },
                common_test.KUBE_LABEL_TABLE: {
                    "SET": {
                        "snmp_enabled": "no_change"
                    }
                }
            }
        }
    },
    6: {
        common_test.DESCR: "kube container starting with no current version",
        common_test.ARGS: "container_startup -f snmp -o kube -v 20201230.11",
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
                        "container_id": "any",
                        "container_version": "",
                        "current_owner": "any",
                        "remote_state": "ready",
                        "system_state": "up"
                    }
                },
                common_test.KUBE_LABEL_TABLE: {
                    "SET": {
                        "snmp_enabled": "no_change"
                    }
                }
            }
        },
        common_test.POST: {
            common_test.STATE_DB_NO: {
                common_test.FEATURE_TABLE: {
                    "snmp": {
                        "container_version": "20201230.11",
                        "current_owner": "kube",
                        "remote_state": "running",
                        "system_state": "up"
                    }
                },
                common_test.KUBE_LABEL_TABLE: {
                    "SET": {
                        "snmp_enabled": "no_change"
                    }
                }
            }
        }
    },
    7: {
        common_test.DESCR: "kube container starting with this version blocked",
        common_test.ARGS: "container_startup -f snmp -o kube -v 20201230.11",
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
                        "container_id": "no change",
                        "container_version": "no change",
                        "current_owner": "no change",
                        "remote_state": "no change",
                        "system_state": "up"
                    }
                },
                common_test.KUBE_LABEL_TABLE: {
                    "SET": {
                        "snmp_local": "20201230.11"
                    }
                }
            }
        },
        common_test.POST: {
            common_test.STATE_DB_NO: {
                common_test.FEATURE_TABLE: {
                    "snmp": {
                        "container_id": "no change",
                        "container_version": "no change",
                        "current_owner": "no change",
                        "remote_state": "no change",
                        "system_state": "up"
                    }
                },
                common_test.KUBE_LABEL_TABLE: {
                    "SET": {
                        "snmp_local": "20201230.11"
                    }
                }
            }
        }
    }
}


class TestContainerStartup(object):

    @patch("container_startup.swsscommon.DBConnector")
    @patch("container_startup.swsscommon.Table")
    def test_start(self, mock_table, mock_conn):
        container_startup.UNIT_TESTING = 1
        common_test.set_mock(mock_table, mock_conn)
        for (i, ct_data) in startup_test_data.items():
            common_test.do_start_test("container_startup", i, ct_data)

            with patch('sys.argv', ct_data[common_test.ARGS].split()):
                container_startup.main()

            ret = common_test.check_tables_returned()
            assert ret == 0
