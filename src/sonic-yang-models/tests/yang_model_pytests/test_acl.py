import pytest


class TestACL:
    def test_valid_data_trimming(self, yang_model):
        data = {
            "sonic-port:sonic-port": {
                "sonic-port:PORT": {
                    "PORT_LIST": [
                        {
                            "name": "Ethernet0",
                            "lanes": "0,1,2,3",
                            "speed": "100000"
                        }
                    ]
                }
            },
            "sonic-acl:sonic-acl": {
                "sonic-acl:ACL_TABLE_TYPE": {
                    "ACL_TABLE_TYPE_LIST": [
                        {
                            "ACL_TABLE_TYPE_NAME": "TRIMMING_L3",
                            "BIND_POINTS": [ "PORT" ],
                            "MATCHES": [ "SRC_IP" ],
                            "ACTIONS": [ "DISABLE_TRIM_ACTION" ]
                        }
                    ]
                },
                "sonic-acl:ACL_TABLE": {
                    "ACL_TABLE_LIST": [
                        {
                            "ACL_TABLE_NAME": "TRIM_TABLE",
                            "type": "TRIMMING_L3",
                            "stage": "INGRESS",
                            "ports": [ "Ethernet0" ]
                        }
                    ]
                },
                "sonic-acl:ACL_RULE": {
                    "ACL_RULE_LIST": [
                        {
                            "ACL_TABLE_NAME": "TRIM_TABLE",
                            "RULE_NAME": "TRIM_RULE",
                            "PRIORITY": "999",
                            "SRC_IP": "1.1.1.1/32",
                            "PACKET_ACTION": "DISABLE_TRIM"
                        }
                    ]
                }
            }
        }

        yang_model.load_data(data)

    @pytest.mark.parametrize(
        "action,error_message", [
            pytest.param('INVALID_VALUE', 'Invalid enumeration value', id="invalid-value")
        ]
    )
    def test_neg_rule_packet_action(self, yang_model, action, error_message):
        data = {
            "sonic-port:sonic-port": {
                "sonic-port:PORT": {
                    "PORT_LIST": [
                        {
                            "name": "Ethernet0",
                            "lanes": "0,1,2,3",
                            "speed": "100000"
                        }
                    ]
                }
            },
            "sonic-acl:sonic-acl": {
                "sonic-acl:ACL_TABLE_TYPE": {
                    "ACL_TABLE_TYPE_LIST": [
                        {
                            "ACL_TABLE_TYPE_NAME": "TRIMMING_L3",
                            "BIND_POINTS": [ "PORT" ],
                            "MATCHES": [ "SRC_IP" ],
                            "ACTIONS": [ "DISABLE_TRIM_ACTION" ]
                        }
                    ]
                },
                "sonic-acl:ACL_TABLE": {
                    "ACL_TABLE_LIST": [
                        {
                            "ACL_TABLE_NAME": "TRIM_TABLE",
                            "type": "TRIMMING_L3",
                            "stage": "INGRESS",
                            "ports": [ "Ethernet0" ]
                        }
                    ]
                },
                "sonic-acl:ACL_RULE": {
                    "ACL_RULE_LIST": [
                        {
                            "ACL_TABLE_NAME": "TRIM_TABLE",
                            "RULE_NAME": "TRIM_RULE",
                            "PRIORITY": "999",
                            "SRC_IP": "1.1.1.1/32",
                            "PACKET_ACTION": action
                        }
                    ]
                }
            }
        }

        yang_model.load_data(data, error_message)
