COMMON_TEST_DATA = [
    [
        "ipv6_with_header",
        {
            "config_db": {
                "DHCP_RELAY": {
                    "Vlan1000": {
                        "dhcpv6_servers": [
                            "fc02:2000::1",
                            "fc02:2000::2"
                        ],
                        "dhcpv6_option|rfc6939_support": "true"
                    }
                }
            }
        },
    ],
    [
        "ipv6_without_header",
        {
            "config_db": {
                "DHCP_RELAY": {
                    "Vlan1000": {
                        "dhcpv6_servers": [
                            "fc02:2000::1",
                            "fc02:2000::2"
                        ],
                        "dhcpv6_option|rfc6939_support": "true"
                    }
                }
            }
        },
    ],
    [
        "ipv4_with_header",
        {
            "config_db": {
                "VLAN": {
                    "Vlan1000": {
                        "dhcp_servers": [
                            "192.0.0.1",
                            "192.0.0.2"
                        ]
                    }
                },
                "DHCPV4_RELAY": {
                    "Vlan1000": {
                        "dhcpv4_servers": [
                            "192.0.0.1",
                            "192.0.0.2"
                        ]
                    }
                }
            }
        }
    ],
    [
        "ipv4_with_disabled_dhcp_server_with_header",
        {
            "config_db": {
                "VLAN": {
                    "Vlan1000": {
                        "dhcp_servers": [
                            "192.0.0.1",
                            "192.0.0.2"
                        ]
                    }
                },
                "FEATURE": {
                    "dhcp_server": {
                        "state": "disabled"
                    }
                }
            }
        }
    ],
    [
        "ipv4_with_enabled_dhcp_server_with_header",
        {
            "config_db": {
                "VLAN": {
                    "Vlan1000": {
                        "dhcp_servers": [
                            "192.0.0.1",
                            "192.0.0.2"
                        ]
                    }
                },
                "FEATURE": {
                    "dhcp_server": {
                        "state": "enabled"
                    }
                }
            }
        }
    ]
]

NEW_ADDED_TEST_DATA = [
    [
        "ipv6",
        {
            "config_db": {
                "DHCP_RELAY": {
                    "Vlan1000": {
                        "dhcpv6_servers": [
                            "fc02:2000::1",
                            "fc02:2000::2"
                        ],
                        "dhcpv6_option|rfc6939_support": "true"
                    }
                }
            }
        },
    ],
    [
        "ipv4",
        {
            "config_db": {
                "VLAN": {
                    "Vlan1000": {
                        "dhcp_servers": [
                            "192.0.0.1",
                            "192.0.0.2"
                        ]
                    },
                    "Vlan1001": {
                        "vlanid": "1001"
                    }
                }
            }
        }
    ]
]

MULTI_TEST_DATA = [
    [
        "ipv6",
        {
            "config_db": {
                "DHCP_RELAY": {
                    "Vlan1000": {
                        "dhcpv6_servers": [
                            "fc02:2000::1",
                            "fc02:2000::2"
                        ],
                        "dhcpv6_option|rfc6939_support": "true"
                    },
                    "Vlan1001": {
                        "dhcpv6_servers": [
                            "fc02:2000::3",
                            "fc02:2000::4"
                        ],
                        "dhcpv6_option|rfc6939_support": "true"
                    }
                }
            }
        },
    ],
    [
        "ipv4",
        {
            "config_db": {
                "VLAN": {
                    "Vlan1000": {
                        "dhcp_servers": [
                            "192.0.0.1",
                            "192.0.0.2"
                        ]
                    },
                    "Vlan1001": {
                        "vlanid": "1001",
                        "dhcp_servers": [
                            "192.0.0.3",
                            "192.0.0.4"
                        ]
                    }
                }
            }
        }
    ],
    [
        "ipv4_dhcp",
        {
            "config_db": {
                "DHCPV4_RELAY": {
                    "Vlan1000": {
                        "dhcpv4_servers": [
                            "192.0.0.1",
                            "192.0.0.2"
                        ],
                        "source_interface": "Ethernet112",
                        "link_selection": "enable",
                        "server_vrf": "default",
                        "vrf_selection": "enable"
                    },
                    "Vlan1001": {
                        "vlanid": "1001",
                        "dhcpv4_servers": [
                            "192.0.0.3",
                            "192.0.0.4"
                        ],
                        "agent_relay_mode": "discard",
                        "max_hop_count": "5"
                    }
                },
                'DEVICE_METADATA': {
                    'localhost': {
                        'has_sonic_dhcpv4_relay' : "False"
                    }
                }
            }
        }
    ]
]
