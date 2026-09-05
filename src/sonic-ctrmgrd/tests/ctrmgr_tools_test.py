import os
import sys
from unittest.mock import MagicMock, patch

import pytest
from sonic_py_common.general import load_module_from_source

from . import common_test

load_module_from_source("docker",
        os.path.join(os.path.dirname(os.path.realpath(__file__)), "mock_docker.py"))

sys.path.append("ctrmgr")
import ctrmgr_tools


# ctr_image_names.json data for the test cases
#
str_ctr_image_names_json = '\
{\n\
"snmp" : "docker-snmp",\n\
"lldp" : "docker-lldp"\n\
}\n'

# ctrmgr_tools test cases
# NOTE: Ensure state-db entry is complete in PRE as we need to
# overwrite any context left behind from last test run.
#
tools_test_data = {
    0: {
        common_test.DESCR: "Tag all features",
        common_test.ARGS: "ctrmgr_tools tag-all",
        common_test.PRE: {
            common_test.STATE_DB_NO: {
                common_test.FEATURE_TABLE: {
                    "snmp": {
                        "current_owner": "kube",
                        "container_id": "snmp_docker_id"
                    },
                    "lldp": {
                        "current_owner": "kube",
                        "container_id": "lldp_docker_id"
                    }
                }
            }
        },
        common_test.POST: {
            common_test.STATE_DB_NO: {
                common_test.FEATURE_TABLE: {
                    "snmp": {
                        "current_owner": "kube",
                        "container_id": "snmp_docker_id"
                    },
                    "lldp": {
                        "current_owner": "kube",
                        "container_id": "lldp_docker_id"
                    }
                }
            }
        },
        common_test.ACTIONS: {
            "snmp_docker_id": [
                {
                    common_test.IMAGE_TAG: {
                        "image_name": "docker-snmp",
                        "image_tag": "latest",
                        "force": True
                    }
                }
            ],
            "snmp": ["remove"],
            "lldp_docker_id": [
                {
                    common_test.IMAGE_TAG: {
                        "image_name": "docker-lldp",
                        "image_tag": "latest",
                        "force": True
                    }
                }
            ],
            "lldp": ["remove"]
        }
    },
    1: {
        common_test.DESCR: "Tag a feature",
        common_test.ARGS: "ctrmgr_tools tag -f snmp",
        common_test.PRE: {
            common_test.STATE_DB_NO: {
                common_test.FEATURE_TABLE: {
                    "snmp": {
                        "current_owner": "kube",
                        "container_id": "snmp_docker_id"
                    }
                }
            }
        },
        common_test.POST: {
            common_test.STATE_DB_NO: {
                common_test.FEATURE_TABLE: {
                    "snmp": {
                        "current_owner": "kube",
                        "container_id": "snmp_docker_id"
                    }
                }
            }
        },
        common_test.ACTIONS: {
            "snmp_docker_id": [
                {
                    common_test.IMAGE_TAG: {
                        "image_name": "docker-snmp",
                        "image_tag": "latest",
                        "force": True
                    }
                }
            ],
            "snmp": ["remove"]
        }
    },
    2: {
        common_test.DESCR: "Skip tag local feature",
        common_test.ARGS: "ctrmgr_tools tag -f snmp",
        common_test.PRE: {
            common_test.STATE_DB_NO: {
                common_test.FEATURE_TABLE: {
                    "snmp": {
                        "current_owner": "local",
                        "container_id": "any"
                    }
                }
            }
        },
        common_test.POST: {
            common_test.STATE_DB_NO: {
                common_test.FEATURE_TABLE: {
                    "snmp": {
                        "current_owner": "local",
                        "container_id": "any"
                    }
                }
            }
        }
    },
    3: {
        common_test.DESCR: "Invoke missing required args",
        common_test.ARGS: "ctrmgr_tools",
        common_test.PRE: {
            common_test.STATE_DB_NO: {
                common_test.FEATURE_TABLE: {
                    "snmp": {
                        "current_owner": "local",
                        "container_id": "any"
                    }
                }
            }
        },
        common_test.POST: {
            common_test.STATE_DB_NO: {
                common_test.FEATURE_TABLE: {
                    "snmp": {
                        "current_owner": "local",
                        "container_id": "any"
                    }
                }
            }
        }
    },
    4: {
        common_test.DESCR: "Kill all features",
        common_test.ARGS: "ctrmgr_tools kill-all",
        common_test.PRE: {
            common_test.STATE_DB_NO: {
                common_test.FEATURE_TABLE: {
                    "snmp": {
                        "system_state": "up",
                        "current_owner": "kube",
                        "container_id": "snmp_docker_id"
                    },
                    "lldp": {
                        "system_state": "down",
                        "current_owner": "kube",
                        "container_id": "lldp_docker_id"
                    }
                }
            }
        },
        common_test.POST: {
            common_test.STATE_DB_NO: {
                common_test.FEATURE_TABLE: {
                    "snmp": {
                        "system_state": "up",
                        "current_owner": "kube",
                        "container_id": "snmp_docker_id"
                    },
                    "lldp": {
                        "system_state": "down",
                        "current_owner": "kube",
                        "container_id": "lldp_docker_id"
                    }
                }
            }
        },
        common_test.ACTIONS: {
            "snmp_docker_id": ["kill"]
        }
    },
    5: {
        common_test.DESCR: "Throw exception in container get",
        common_test.ARGS: "ctrmgr_tools tag-all",
        common_test.RETVAL: -1,
        common_test.PRE: {
            common_test.STATE_DB_NO: {
                common_test.FEATURE_TABLE: {
                    "snmp": {
                        "current_owner": "kube",
                        "container_id": common_test.TRIGGER_GET_THROW
                    }
                }
            }
        }
    },
    6: {
        common_test.DESCR: "Throw exception in container rm",
        common_test.ARGS: "ctrmgr_tools tag -f throw_on_rm -n test",
        common_test.RETVAL: -1,
        common_test.PRE: {
            common_test.STATE_DB_NO: {
                common_test.FEATURE_TABLE: {
                    common_test.TRIGGER_RM_THROW: {
                        "current_owner": "kube",
                        "container_id": "any"
                    }
                }
            }
        },
        common_test.ACTIONS: {
            "any": [
                {
                    common_test.IMAGE_TAG: {
                        "image_name": "test",
                        "image_tag": "latest",
                        "force": True
                    }
                }
            ],
            "throw_on_rm": []
        }
    },
    7: {
        common_test.DESCR: "Throw exception in container get",
        common_test.ARGS: "ctrmgr_tools kill-all",
        common_test.RETVAL: -1,
        common_test.PRE: {
            common_test.STATE_DB_NO: {
                common_test.FEATURE_TABLE: {
                    "snmp": {
                        "system_state": "up",
                        "current_owner": "kube",
                        "container_id": common_test.TRIGGER_GET_THROW
                    }
                }
            }
        }
    }
}


class TestCtrmgrTools(object):

    @patch("ctrmgr_tools.swsscommon.DBConnector")
    @patch("ctrmgr_tools.swsscommon.Table")
    @patch("ctrmgr_tools.docker.from_env")
    def test_tools(self, mock_docker, mock_table, mock_conn):
        fname = "/tmp/ctr_image_names.json"
        with open(fname, "w") as s:
            s.write(str_ctr_image_names_json)

        ctrmgr_tools.CTR_NAMES_FILE = fname
        common_test.set_mock(mock_table, mock_conn, mock_docker)

        for (i, ct_data) in tools_test_data.items():
            common_test.do_start_test("ctrmgr_tools", i, ct_data)

            with patch('sys.argv', ct_data[common_test.ARGS].split()):
                ret = ctrmgr_tools.main()
                if common_test.RETVAL in ct_data:
                    assert ret == ct_data[common_test.RETVAL]

            ret = common_test.check_tables_returned()
            assert ret == 0

            ret = common_test.check_mock_containers()
            assert ret == 0
