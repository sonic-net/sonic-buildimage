# SONiC Configuration Database Manual


**Table of Contents**

* [Introduction](#introduction)
* [Configuration](#configuration)
* [<strong>Config Load and Save</strong>](#config-load-and-save)
  * [Incremental Configuration](#incremental-configuration)
* [<strong>Redis and Json Schema</strong>](#redis-and-json-schema)
  * [ACL and Mirroring](#acl-and-mirroring)
  * [BGP BBR](#bgp-bbr)
  * [ASIC SDK health event](#asic-sdk-health-event)
  * [BGP Device Global](#bgp-device-global)
  * [BGP Sessions](#bgp-sessions)
  * [BUFFER_PG](#buffer_pg)
  * [Buffer pool](#buffer-pool)
  * [Buffer profile](#buffer-profile)
  * [Buffer queue](#buffer-queue)
  * [Buffer port ingress profile list](#buffer-port-ingress-profile-list)
  * [Buffer port egress profile list](#buffer-port-egress-profile-list)
  * [Cable length](#cable-length)
  * [Chassis module](#chassis-module)
  * [COPP_TABLE](#copp_table)
  * [Console](#console)
  * [CRM](#crm)
  * [CRM DASH](#crm-dash)
  * [DEBUG_COUNTER and DEBUG_COUNTER_DROP_REASON](#debug_counter-and-debug_counter_drop_reason)
  * [DEBUG_DROP_MONITOR](#debug_drop_monitor)
  * [DEFAULT_LOSSLESS_BUFFER_PARAMETER](#DEFAULT_LOSSLESS_BUFFER_PARAMETER)
  * [Device Metadata](#device-metadata)
  * [Device neighbor metada](#device-neighbor-metada)
  * [DHCP_RELAY](#dhcp_relay)
  * [DHCPV4_RELAY](#dhcpv4_relay)
  * [DHCP Server IPV4](#dhcp_server_ipv4)
  * [BMP](#bmp)
  * [DSCP_TO_TC_MAP](#dscp_to_tc_map)
  * [FG_NHG](#fg_nhg)
  * [FG_NHG_MEMBER](#fg_nhg_member)
  * [FG_NHG_PREFIX](#fg_nhg_prefix)
  * [FABRIC_MONITOR](#fabric-monitor)
  * [FABRIC_PORT](#fabric-port)
  * [FLEX_COUNTER_TABLE](#flex_counter_table)
  * [GRPCCLIENT](#grpcclient)
  * [Hash](#hash)
  * [KDUMP](#kdump)
  * [Kubernetes Master](#kubernetes-master)
  * [L2 Neighbors](#l2-neighbors)
  * [L3 Interfaces](#l3-interfaces)
  * [Loopback Interface](#loopback-interface)
  * [LOSSLESS_TRAFFIC_PATTERN](#LOSSLESS_TRAFFIC_PATTERN)
  * [Memory Statistics](#memory-statistics)
  * [Management Interface](#management-interface)
  * [Management port](#management-port)
  * [Management VRF](#management-vrf)
  * [MAP_PFC_PRIORITY_TO_QUEUE](#map_pfc_priority_to_queue)
  * [MUX_CABLE](#mux_cable)
  * [MUX_LINKMGR](#mux_linkmgr)
  * [NEIGH](#neigh)
  * [NTP Global Configuration](#ntp-global-configuration)
  * [NTP Servers](#ntp-servers)
  * [Peer Switch](#peer-switch)
  * [Policer](#policer)
  * [Port](#port)
  * [Port Channel](#port-channel)
  * [Portchannel member](#portchannel-member)
  * [Scheduler](#scheduler)
  * [Port QoS Map](#port-qos-map)
  * [Queue](#queue)
  * [Syslog Global Configuration](#syslog-global-configuration)
  * [Syslog Servers](#syslog-servers)
  * [Sflow](#sflow)
  * [Restapi](#restapi)
  * [System Port](#system-port)
  * [Tacplus Server](#tacplus-server)
  * [TC to DSCP map](#tc-to-dscp-map)
  * [TC to Priority group map](#tc-to-priority-group-map)
  * [TC to Queue map](#tc-to-queue-map)
  * [Telemetry](#telemetry)
  * [Telemetry client](#telemetry-client)
  * [Tunnel](#tunnel)
  * [Trimming](#trimming)
  * [Versions](#versions)
  * [VLAN](#vlan)
  * [VLAN_MEMBER](#vlan_member)
  * [VNET](#vnet)
  * [VNET_ROUTE_TUNNEL](#vnet_route_tunnel)
  * [VOQ Inband Interface](#voq-inband-interface)
  * [VXLAN](#vxlan)
  * [Virtual router](#virtual-router)
  * [LOGGER](#logger)
  * [WRED_PROFILE](#wred_profile)
  * [XCVRD_LOG](#xcvrd_log)
  * [PASSWORD_HARDENING](#password_hardening)
  * [SSH_SERVER](#ssh_server)
  * [SUBNET_DECAP](#subnet_decap)
  * [SYSTEM_DEFAULTS table](#systemdefaults-table)
  * [RADIUS](#radius)
  * [Static DNS](#static-dns)
  * [ASIC_SENSORS](#asic_sensors)  
  * [SRv6](#srv6)
  * [DPU](#dpu-configuration)
  * [REMOTE_DPU](#remote_dpu-configuration)
  * [VDPU](#vdpu-configuration)
  * [DASH HA Global Configuration](#dash-ha-global-configuration)
  * [Prefix List](#prefix-list)
* [For Developers](#for-developers)
  * [Generating Application Config by Jinja2 Template](#generating-application-config-by-jinja2-template)
  * [Incremental Configuration by Subscribing to ConfigDB](#incremental-configuration-by-subscribing-to-configdb)


# Introduction
This document lists the configuration commands schema applied in the SONiC eco system. All these commands find relevance in collecting system information, analysis and even for trouble shooting. All the commands are categorized under relevant topics with corresponding examples.

# Configuration

SONiC is managing configuration in a single source of truth - a redisDB
instance that we refer as ConfigDB. Applications subscribe to ConfigDB
and generate their running configuration correspondingly.

(Before Sep 2017, we were using an XML file named minigraph.xml to
configure SONiC devices. For historical documentation, please refer to
[Configuration with
Minigraph](https://github.com/Azure/SONiC/wiki/Configuration-with-Minigraph-(~Sep-2017)))

# **Config Load and Save**

In current version of SONiC, ConfigDB is implemented as database 4 of
local redis. When system boots, configurations will be loaded from
/etc/sonic/config_db.json file into redis. Please note that ConfigDB
content won't be written back into /etc/sonic/config_db.json file
automatically. In order to do that, a config save command need to be
manually executed from CLI. Similarly, config load will trigger a force
load of json file into DB. Generally, content in
/etc/sonic/config_db.json can be considered as starting config, and
content in redisDB running config.

We keep a way to load configuration from minigraph and write into
ConfigDB for backward compatibility. To do that, run `config
load_minigraph`.

### Incremental Configuration

The design of ConfigDB supports incremental configuration - application
could subscribe to changes in ConfigDB and response correspondingly.
However, this feature is not implemented by all applications yet. By Sep
2017 now, the only application that supports incremental configuration
is BGP (docker-fpm-quagga). For other applications, a manual restart is
required after configuration changes in ConfigDB.

# **Redis and Json Schema**

ConfigDB uses a table-object schema that is similar with
[AppDB](https://github.com/Azure/sonic-swss/blob/4c56d23b9ff4940bdf576cf7c9e5aa77adcbbdcc/doc/swss-schema.md),
and `config_db.json` is a straight-forward serialization of DB. As an
example, the following fragments could be BGP-related configuration in
redis and json, correspondingly:


***Redis format***

```
127.0.0.1:6379[4]> keys BGP_NEIGHBOR|*

1) "BGP_NEIGHBOR|10.0.0.31"
2) "BGP_NEIGHBOR|10.0.0.39"
3) "BGP_NEIGHBOR|10.0.0.11"
4) "BGP_NEIGHBOR|10.0.0.7"

...

127.0.0.1:6379[4]> hgetall BGP_NEIGHBOR|10.0.0.3

1) "admin_status"
2) "up"
3) "peer_addr"
4) "10.0.0.2"
5) "asn"
6) "65200"
7) "name"
8) "ARISTA07T2"
```

***Json format***

```
"BGP_NEIGHBOR": {
    "10.0.0.57": {
        "rrclient": "0",
        "name": "ARISTA01T1",
        "local_addr": "10.0.0.56",
        "nhopself": "0",
        "holdtime": "10",
        "asn": "64600",
        "keepalive": "3"
    },
    "10.0.0.59": {
        "rrclient": "0",
        "name": "ARISTA02T1",
        "local_addr": "10.0.0.58",
        "nhopself": "0",
        "holdtime": "10",
        "asn": "64600",
        "keepalive": "3"
    },
}
```

Full sample config_db.json files are availables at
[here](https://github.com/Azure/SONiC/blob/gh-pages/doc/config_db.json)
and
[here](https://github.com/Azure/SONiC/blob/gh-pages/doc/config_db_t0.json).


### ACL and Mirroring

ACL and mirroring related configuration are defined in
**MIRROR_SESSION**, **ACL_TABLE** and **ACL_RULE** tables. Those
tables are in progress of migrating from APPDB. Please refer to their
schema in APPDB
[here](https://github.com/Azure/sonic-swss/blob/4c56d23b9ff4940bdf576cf7c9e5aa77adcbbdcc/doc/swss-schema.md)
and migration plan
[here](https://github.com/Azure/SONiC/wiki/ACL-Configuration-Requirement-Description).

```
{
"MIRROR_SESSION": {
        "everflow0": {
                "src_ip": "10.1.0.32",
                "dst_ip": "2.2.2.2"
        }
    },

"ACL_TABLE": {
        "DATAACL": {
                "policy_desc" : "data_acl",
                "type": "l3",
                "ports": [
                        "Ethernet0",
                        "Ethernet4",
                        "Ethernet8",
                        "Ethernet12"
                ]
        }
    }
}
```

***Below ACL table added as per the mail***
```
{
"ACL_TABLE": {
        "aaa": {
                "type": "L3",
                "ports": "Ethernet0"
        }
   },
"ACL_RULE": {
        "aaa|rule_0": {
        "PRIORITY": "55",
        "PACKET_ACTION": "DROP",
        "L4_SRC_PORT": "0"
        },
        "aaa|rule_1": {
        "PRIORITY": "55",
        "PACKET_ACTION": "DROP",
        "L4_SRC_PORT": "1"
        }
   }
}
```

***Below ACL table added by comparig minigraph.xml & config_db.json***

```
{
"ACL_TABLE": {
		"EVERFLOW": {
		"type": "MIRROR",
		"policy_desc": "EVERFLOW",
		"ports": [
		  "PortChannel0001",
		  "PortChannel0002",
		  "PortChannel0003",
		  "PortChannel0004"
		]
	  },
		"EVERFLOWV6": {
        "type": "MIRRORV6",
        "policy_desc": "EVERFLOWV6",
        "ports": [
          "PortChannel0001",
          "PortChannel0002",
          "PortChannel0003",
          "PortChannel0004"
        ]
      },
        "SNMP_ACL": {
          "services": [
            "SNMP"
        ],
        "type": "CTRLPLANE",
        "policy_desc": "SNMP_ACL"
      },
        "SSH_ONLY": {
          "services": [
            "SSH"
          ],
          "type": "CTRLPLANE",
          "policy_desc": "SSH_ONLY"
      }
   },

"ACL_RULE": {
        "SNMP_ACL|DEFAULT_RULE": {
            "PRIORITY": "1",
            "PACKET_ACTION": "DROP",
            "ETHER_TYPE": "2048"
        },
        "SNMP_ACL|RULE_1": {
            "PRIORITY": "9999",
            "PACKET_ACTION": "ACCEPT",
            "SRC_IP": "1.1.1.1/32",
            "IP_PROTOCOL": "17"
        },
        "SNMP_ACL|RULE_2": {
            "PRIORITY": "9998",
            "PACKET_ACTION": "ACCEPT",
            "SRC_IP": "2.2.2.2/32",
            "IP_PROTOCOL": "17"
        },
        "SSH_ONLY|DEFAULT_RULE": {
            "PRIORITY": "1",
            "PACKET_ACTION": "DROP",
            "ETHER_TYPE": "2048"
        },
        "SSH_ONLY|RULE_1": {
            "PRIORITY": "9999",
            "PACKET_ACTION": "ACCEPT",
            "SRC_IP": "4.4.4.4/8",
            "IP_PROTOCOL": "6"
        }
    }
}

```

***ACL table type configuration example***
```
{
    "ACL_TABLE_TYPE": {
        "CUSTOM_L3": {
            "MATCHES": [
                "IN_PORTS",
                "OUT_PORTS",
                "SRC_IP",
                "TUNNEL_TERM"
            ],
            "ACTIONS": [
                "PACKET_ACTION",
                "MIRROR_INGRESS_ACTION"
            ],
            "BIND_POINTS": [
                "PORT",
                "PORTCHANNEL"
            ]
        }
    },
    "ACL_TABLE": {
        "DATAACL": {
            "STAGE": "INGRESS",
            "TYPE": "CUSTOM_L3",
            "PORTS": [
                "Ethernet0",
                "PortChannel1"
            ]
        }
    },
    "ACL_RULE": {
        "DATAACL|RULE0": {
            "PRIORITY": "999",
            "PACKET_ACTION": "DROP",
            "SRC_IP": "1.1.1.1/32",
            "TUNNEL_TERM": "true"
        }
    }
}
```

***ACL fine-grained packet trimming control with disable trimming action configuration example***
```
{
    "ACL_TABLE_TYPE": {
        "TRIMMING_L3": {
            "MATCHES": [
                "SRC_IP"
            ],
            "ACTIONS": [
                "DISABLE_TRIM_ACTION"
            ],
            "BIND_POINTS": [
                "PORT"
            ]
        }
    },
    "ACL_TABLE": {
        "TRIM_TABLE": {
            "POLICY_DESC": "Packet trimming",
            "TYPE": "TRIMMING_L3",
            "STAGE": "INGRESS",
            "PORTS": [
                "Ethernet0"
            ]
        }
    },
    "ACL_RULE": {
        "TRIM_TABLE|TRIM_RULE": {
            "PRIORITY": "999",
            "SRC_IP": "1.1.1.1/32",
            "PACKET_ACTION": "DISABLE_TRIM"
        }
    }
}
```

### BGP BBR

The **BGP_BBR** table contains device-level BBR state.
```
{
        "BGP_BBR": {
            "all": {
                "status": "enabled"/"disabled"
            }
        }
}
```

### ASIC SDK health event

ASIC/SDK health event related configuration is defined in **SUPPRESS_ASIC_SDK_HEALTH_EVENT** table.

```
"SUPPRESS_ASIC_SDK_HEALTH_EVENT": {
    "notice": {
        "categories": [
            "asic_hw"
        ],
        "max_events": "1000"
    },
    "warning": {
        "categories": [
            "software",
            "cpu_hw"
        ]
    }
}
```

### BGP Device Global

The **BGP_DEVICE_GLOBAL** table contains device-level BGP global state.  
It has a STATE object containing device state like **tsa_enabled**, **wcmp_enabled** and **idf_isolation_state**.

When **tsa_enabled** is set to true, the device is isolated using traffic-shift-away (TSA) route-maps in BGP.

```json
{
"BGP_DEVICE_GLOBAL": {
    "STATE": {
        "tsa_enabled": "true"
    }
}
```

When **wcmp_enabled** is set to true, the device is configured to use BGP Link Bandwidth Extended Community.  
Weighted ECMP load balances traffic between the equal cost paths in proportion to the capacity of the local links.

```json
{
"BGP_DEVICE_GLOBAL": {
    "STATE": {
        "wcmp_enabled": "true"
    }
}
```

The IDF isolation state **idf_isolation_state** could be one of isolated_no_export, isolated_withdraw_all or unisolated.

```json
{
"BGP_DEVICE_GLOBAL": {
    "STATE": {
        "idf_isolation_state": "isolated_no_export"
    }
}
```

### BGP Sessions

BGP session configuration is defined in **BGP_NEIGHBOR** table. BGP
neighbor address is used as key of bgp neighbor objects. Object
attributes include remote AS number, neighbor router name, and local
peering address. Dynamic neighbor is also supported by defining peer
group name and IP ranges in **BGP_PEER_RANGE** table.

```
{
"BGP_NEIGHBOR": {
        "10.0.0.61": {
                "local_addr": "10.0.0.60",
                "asn": 64015,
                "name": "ARISTA15T0"
        },
        "10.0.0.49": {
                "local_addr": "10.0.0.48",
                "asn": 64009,
                "name": "ARISTA09T0"
        },

        "10.0.0.63": {
                "rrclient": "0",
				"name": "ARISTA04T1",
				"local_addr": "10.0.0.62",
				"nhopself": "0",
				"holdtime": "10",
				"asn": "64600",
				"keepalive": "3"
        }

"BGP_PEER_RANGE": {
    "BGPSLBPassive": {
        "name": "BGPSLBPassive",
        "ip_range": [
            "10.250.0.0/27"
        ]
    },
    "BGPVac": {
        "name": "BGPVac",
        "ip_range": [
            "10.2.0.0/16"
        ]
    },
    "BGPSentinel": {
        "name": "BGPSentinel",
        "ip_range": [
            "10.1.0.0/24"
        ],
        "src_address": "10.1.0.32"
     }
  }
}
```

### BUFFER_PG

When the system is running in traditional buffer model, profiles needs to explicitly configured:

```
{
"BUFFER_PG": {
    "Ethernet0|3-4": {
        "profile": "pg_lossless_40000_5m_profile"
    },
    "Ethernet1|3-4": {
        "profile": "pg_lossless_40000_5m_profile"
    },
    "Ethernet2|3-4": {
        "profile": "pg_lossless_40000_5m_profile"
    }
  }
}

```

When the system is running in dynamic buffer model, profiles can be:

 - either calculated dynamically according to ports' configuration and just configured as "NULL";
 - or configured explicitly.

```
{
"BUFFER_PG": {
    "Ethernet0|3-4": {
        "profile": "NULL"
    },
    "Ethernet1|3-4": {
        "profile": "NULL"
    },
    "Ethernet2|3-4": {
        "profile": "static_profile"
    }
  }
}

```

### Buffer pool

When the system is running in traditional buffer model, the size of all of the buffer pools and xoff of ingress_lossless_pool need to be configured explicitly.

```
{
"BUFFER_POOL": {
    "egress_lossless_pool": {
        "type": "egress",
        "mode": "static",
        "size": "15982720"
    },
    "egress_lossy_pool": {
        "type": "egress",
        "mode": "dynamic",
        "size": "9243812"
    },
    "ingress_lossless_pool": {
        "xoff": "4194112",
        "type": "ingress",
        "mode": "dynamic",
        "size": "10875072"
    }
  }
}

```

When the system is running in dynamic buffer model, the size of some of the buffer pools can be omitted and will be dynamically calculated.
In this case, A percentage can be configured on a pool, representing how many the available buffer can be allloced to the pool.

```
{
"BUFFER_POOL": {
    "egress_lossless_pool": {
        "type": "egress",
        "mode": "static",
        "size": "15982720"
    },
    "egress_lossy_pool": {
        "type": "egress",
        "mode": "dynamic"
    },
    "ingress_lossless_pool": {
        "type": "ingress",
        "mode": "dynamic",
        "percentage": "80"
    }
  }
}

```


### Buffer profile

```
{
"BUFFER_PROFILE": {
    "egress_lossless_profile": {
        "static_th": "3995680",
        "pool": "egress_lossless_pool",
        "size": "1518"
    },
    "egress_lossy_profile": {
        "dynamic_th": "3",
        "pool": "egress_lossy_pool",
        "size": "1518"
    },
    "ingress_lossy_profile": {
        "dynamic_th": "3",
        "pool": "ingress_lossless_pool",
        "size": "0"
    },
    "pg_lossless_40000_5m_profile": {
        "xon_offset": "2288",
        "dynamic_th": "-3",
        "xon": "2288",
        "xoff": "66560",
        "pool": "ingress_lossless_pool",
        "size": "1248"
    },
    "pg_lossless_40000_40m_profile": {
        "xon_offset": "2288",
        "dynamic_th": "-3",
        "xon": "2288",
        "xoff": "71552",
        "pool": "ingress_lossless_pool",
        "size": "1248"
    }
  }
}

```

When the system is running in dynamic buffer model and the headroom_type is dynamic, only dynamic_th needs to be configured and rest of fields can be omitted.
This kind of profiles will be handled by buffer manager and won't be applied to SAI.

```
{
  {
    "non_default_dynamic_th_profile": {
        "dynamic_th": 1,
        "headroom_type": "dynamic"
    }
  }
}
```

***Packet trimming configuration example***
```
{
    "q_lossy_profile": {
        "dynamic_th": "3",
        "pool": "egress_lossy_pool",
        "size": "0",
        "packet_discard_action": "drop"
    }
}
```

### Buffer queue

```
{
"BUFFER_QUEUE": {
    "Ethernet50,Ethernet52,Ethernet54,Ethernet56|0-2": {
        "profile": "egress_lossy_profile"
    },
    "Ethernet50,Ethernet52,Ethernet54,Ethernet56|3-4": {
        "profile": "egress_lossless_profile"
    },
    "Ethernet50,Ethernet52,Ethernet54,Ethernet56|5-6": {
        "profile": "egress_lossy_profile"
    }
  }
}

```

### Buffer port ingress profile list

```
{
"BUFFER_PORT_INGRESS_PROFILE_LIST": {
    "Ethernet50": {
        "profile_list": "ingress_lossy_profile,ingress_lossless_profile"
    },
    "Ethernet52": {
        "profile_list": "ingress_lossy_profile,ingress_lossless_profile"
    },
    "Ethernet56": {
        "profile_list": "ingress_lossy_profile,ingress_lossless_profile"
    }
  }
}

```

### Buffer port egress profile list

```
{
"BUFFER_PORT_EGRESS_PROFILE_LIST": {
    "Ethernet50": {
        "profile_list": "egress_lossy_profile,egress_lossless_profile"
    },
    "Ethernet52": {
        "profile_list": "egress_lossy_profile,egress_lossless_profile"
    },
    "Ethernet56": {
        "profile_list": "egress_lossy_profile,egress_lossless_profile"
    }
  }
}

```

### Cable length

```
{
"CABLE_LENGTH": {
    "AZURE": {
        "Ethernet8": "5m",
        "Ethernet9": "5m",
        "Ethernet2": "5m",
        "Ethernet58": "5m",
        "Ethernet59": "5m",
        "Ethernet50": "40m",
        "Ethernet51": "5m",
        "Ethernet52": "40m",
        "Ethernet53": "5m",
        "Ethernet54": "40m",
        "Ethernet55": "5m",
        "Ethernet56": "40m"
    }
  }
}

```

### Chassis Module

CHASSIS_MODULE table holds the list and configuration of linecard and fabric modules in a SONiC chassis.
It currently allows user to administratively bring down a line-card or fabric-card

```
{
    "CHASSIS_MODULE": {
        "LINE-CARD0": {
            "admin_status": "down"
        },
        "FABRIC-CARD1": {
            "admin_status": "down"
        }
    }
}

```

### COPP_TABLE

```
{
"COPP_TABLE": {
     "default": {
         "cbs": "600",
         "cir": "600",
	 "meter_type": "packets",
	 "mode": "sr_tcm",
	 "queue": "0",
	 "red_action": "drop"
     },

     "trap.group.arp": {
         "cbs": "600",
         "cir": "600",
	 "meter_type": "packets",
	 "mode": "sr_tcm",
	 "queue": "4",
	 "red_action": "drop",
	 "trap_action": "trap",
	 "trap_ids": "arp_req,arp_resp,neigh_discovery",
	 "trap_priority": "4"
      },

     "trap.group.lldp.dhcp.udld": {
         "queue": "4",
         "trap_action": "trap",
	 "trap_ids": "lldp,dhcp,udld",
	 "trap_priority": "4"
      },

     "trap.group.bgp.lacp": {
         "queue": "4",
         "trap_action": "trap",
	 "trap_ids": "bgp,bgpv6,lacp",
	 "trap_priority": "4"
      },

     "trap.group.ip2me": {
         "cbs": "600",
         "cir": "600",
	 "meter_type": "packets",
	 "mode": "sr_tcm",
	 "queue": "1",
	 "red_action": "drop",
	 "trap_action": "trap",
	 "trap_ids": "ip2me",
	 "trap_priority": "1"
      }
    }
}
```

### Console

```
{
"CONSOLE_PORT": {
    "1": {
        "baud_rate": "115200",
        "flow_control": "0",
        "remote_device": "host-1"
    },
    "2": {
        "baud_rate": "9600",
        "flow_control": "1"
    }
  },
"CONSOLE_SWITCH": {
    "console_mgmt": {
        "enabled": "yes"
    }
  }
}
```

### CRM

```
{
"CRM": {
    "Config": {
        "acl_table_threshold_type": "percentage",
        "nexthop_group_threshold_type": "percentage",
        "fdb_entry_high_threshold": "85",
        "acl_entry_threshold_type": "percentage",
        "ipv6_neighbor_low_threshold": "70",
        "nexthop_group_member_low_threshold": "70",
        "acl_group_high_threshold": "85",
        "ipv4_route_high_threshold": "85",
        "acl_counter_high_threshold": "85",
        "ipv4_route_low_threshold": "70",
        "ipv4_route_threshold_type": "percentage",
        "ipv4_neighbor_low_threshold": "70",
        "acl_group_threshold_type": "percentage",
        "ipv4_nexthop_high_threshold": "85",
        "ipv6_route_threshold_type": "percentage",
        "snat_entry_threshold_type": "percentage",
        "snat_entry_high_threshold": "85",
        "snat_entry_low_threshold": "70",
        "dnat_entry_threshold_type": "percentage",
        "dnat_entry_high_threshold": "85",
        "dnat_entry_low_threshold": "70",
        "ipmc_entry_threshold_type": "percentage",
        "ipmc_entry_high_threshold": "85",
        "ipmc_entry_low_threshold": "70"
    }
  }
}

```

### CRM DASH

```json
{
"CRM": {
    "Config": {
        "dash_vnet_threshold_type": "percentage",
        "dash_vnet_low_threshold": "70",
        "dash_vnet_high_threshold": "85",
        "dash_eni_threshold_type": "percentage",
        "dash_eni_low_threshold": "70",
        "dash_eni_high_threshold": "85",
        "dash_eni_ether_address_map_threshold_type": "percentage",
        "dash_eni_ether_address_map_low_threshold": "70",
        "dash_eni_ether_address_map_high_threshold": "85",
        "dash_ipv4_inbound_routing_threshold_type": "percentage",
        "dash_ipv4_inbound_routing_low_threshold": "70",
        "dash_ipv4_inbound_routing_high_threshold": "85",
        "dash_ipv6_inbound_routing_threshold_type": "percentage",
        "dash_ipv6_inbound_routing_low_threshold": "70",
        "dash_ipv6_inbound_routing_high_threshold": "85",
        "dash_ipv4_outbound_routing_threshold_type": "percentage",
        "dash_ipv4_outbound_routing_low_threshold": "70",
        "dash_ipv4_outbound_routing_high_threshold": "85",
        "dash_ipv6_outbound_routing_threshold_type": "percentage",
        "dash_ipv6_outbound_routing_low_threshold": "70",
        "dash_ipv6_outbound_routing_high_threshold": "85",
        "dash_ipv4_pa_validation_threshold_type": "percentage",
        "dash_ipv4_pa_validation_low_threshold": "70",
        "dash_ipv4_pa_validation_high_threshold": "85",
        "dash_ipv6_pa_validation_threshold_type": "percentage",
        "dash_ipv6_pa_validation_low_threshold": "70",
        "dash_ipv6_pa_validation_high_threshold": "85",
        "dash_ipv4_outbound_ca_to_pa_threshold_type": "percentage",
        "dash_ipv4_outbound_ca_to_pa_low_threshold": "70",
        "dash_ipv4_outbound_ca_to_pa_high_threshold": "85",
        "dash_ipv6_outbound_ca_to_pa_threshold_type": "percentage",
        "dash_ipv6_outbound_ca_to_pa_low_threshold": "70",
        "dash_ipv6_outbound_ca_to_pa_high_threshold": "85",
        "dash_ipv4_acl_group_threshold_type": "percentage",
        "dash_ipv4_acl_group_low_threshold": "70",
        "dash_ipv4_acl_group_high_threshold": "85",
        "dash_ipv6_acl_group_threshold_type": "percentage",
        "dash_ipv6_acl_group_low_threshold": "70",
        "dash_ipv6_acl_group_high_threshold": "85",
        "dash_ipv4_acl_rule_threshold_type": "percentage",
        "dash_ipv4_acl_rule_low_threshold": "70",
        "dash_ipv4_acl_rule_high_threshold": "85",
        "dash_ipv6_acl_rule_threshold_type": "percentage",
        "dash_ipv6_acl_rule_low_threshold": "70",
        "dash_ipv6_acl_rule_high_threshold": "85"
    }
  }
}
```

### DEBUG_COUNTER and DEBUG_COUNTER_DROP_REASON

These tables contain information on drop counters which have been added

DEBUG_COUNTER:
```
; DEBUG_COUNTER table

key                      = DEBUG_COUNTER_TABLE:name
name                     = string
type                     = (SWITCH_INGRESS_DROPS|PORT_INGRESS_DROPS|SWITCH_EGRESS_DROPS|PORT_EGRESS_DROPS)
alias                    = string (optional)
description              = string (optional)
group                    = string (optional)
drop_monitor_status      = admin_mode (enabled/disabled, default disabled)
drop_count_threshold     = uint64 (optional)
incident_count_threshold = uint64 (optional)
window                   = uint64 (optional)

"DEBUG_COUNTER": {
    "DEBUG_4": {
        "alias": "BAD_DROPS",
        "desc": "More port ingress drops",
        "group": "BAD",
        "type": "SWITCH_INGRESS_DROPS",
        "drop_monitor_status": "disabled",
        "drop_count_threshold": "10",
        "incident_count_threshold": "2",
        "window": "300"
    }
}
```
```
; DEBUG_COUNTER_DROP_REASON table

key     = DEBUG_COUNTER_DROP_REASON_TABLE:name:reason
name    = name of a counter in the DEBUG_COUNTER_TABLE
reason  = a valid drop reason without the 'SAI_IN/OUT_DROP_REASON_' prefix (https://github.com/sonic-net/sonic-swss/blob/7a965caf4c7211afca5303191cf731858c791bcd/orchagent/debug_counter/drop_counter.cpp#L20)

"DEBUG_COUNTER_DROP_REASON": {
    "DEBUG_4|DIP_LINK_LOCAL": {},
    "DEBUG_4|SIP_LINK_LOCAL": {}
}
```

### DEBUG_DROP_MONITOR
Ingress and eggress port debug counter flows can be monitored for persistent drops using
debug drop monitor feature. This table shows the status and configurations of the feature which can
be modified via the CLI.

```
{
    "DEBUG_DROP_MONITOR": {
        "CONFIG": {
            "status": "disabled",
        }
    }
}
```

### DEFAULT_LOSSLESS_BUFFER_PARAMETER

This table stores the default lossless buffer parameters for dynamic buffer calculation.

```
{
    "DEFAULT_LOSSLESS_BUFFER_PARAMETER": {
        "AZURE": {
            "default_dynamic_th": "0",
            "over_subscribe_ratio": "2"
        }
    }
}
```

### Device Metadata

The **DEVICE_METADATA** table contains only one object named
*localhost*. In this table the device metadata such as hostname, hwsku,
deployment envionment id and deployment type are specified. BGP local AS
number is also specified in this table as current only single BGP
instance is supported in SONiC.

```
{
"DEVICE_METADATA": {
        "localhost": {
        "asic_id": "06:00.0",
        "asic_name": "asic0",
        "hwsku": "Force10-S6100",
        "default_bgp_status": "up",
        "docker_routing_config_mode": "unified",
        "hostname": "sonic-s6100-01",
        "platform": "x86_64-dell_s6100_c2538-r0",
        "mac": "4c:76:25:f4:70:82",
        "default_pfcwd_status": "disable",
        "bgp_asn": "65100",
        "deployment_id": "1",
        "type": "ToRRouter",
        "bgp_adv_lo_prefix_as_128" : "true",
        "buffer_model": "traditional",
        "yang_config_validation": "disable",
        "rack_mgmt_map": "dummy_value",
        "timezome": "Europe/Kiev",
        "bgp_router_id": "8.8.8.8"
    }
  }
}

```


### Device neighbor metada

```
{
"DEVICE_NEIGHBOR_METADATA": {
    "ARISTA01T1": {
        "cluster": "AAA00PrdStr00",
        "lo_addr": "None",
        "mgmt_addr": "10.11.150.45",
        "hwsku": "Arista-VM",
        "type": "LeafRouter"
    },
    "ARISTA02T1": {
        "cluster": "AAA00PrdStr00",
        "lo_addr": "None",
        "mgmt_addr": "10.11.150.46",
        "hwsku": "Arista-VM",
        "type": "LeafRouter"
    }
  }
}

```

### DHCP_RELAY

```
{
"DHCP_RELAY": {
    "dhcpv6_servers": [
        "fc02:2000::1",
        "fc02:2000::2",
        "fc02:2000::3",
        "fc02:2000::4"
    ],
    "rfc6939_support": "true",
    "interface_id": "true"
}

```

### DHCPV4_RELAY

```
{
"DHCPV4_RELAY": {
    "Vlan1000": {
        "dhcpv4_servers": [
            "192.168.0.1",
            "192.168.0.2"
        ],
        "server_vrf": "Vrf_RED",
        "source_interface": "Loopback0",
        "link_selection": "enable",
        "vrf_selection": "enable",
        "server_id_override": "enable",
        "agent_relay_mode": "append",
        "max_hop_count": 10
    }
}

```

### BMP
BMP related configuration are defined in **bgp_neighbor_table**,**bgp_rib_in_table**, **bgp_rib_out_table** tables.

```
{
    "BMP": {
        "table": {
            "bgp_neighbor_table": "true",
            "bgp_rib_in_table": "false",
            "bgp_rib_out_table": "false"
        }
    }
}
```

### DHCP_SERVER_IPV4
IPV4 DHPC Server related configuration are defined in **DHCP_SERVER_IPV4**, **DHCP_SERVER_IPV4_CUSTOMIZED_OPTIONS**, **DHCP_SERVER_IPV4_RANGE**, **DHCP_SERVER_IPV4_PORT** tables.
```
{
    "DHCP_SERVER_IPV4": {
        "Vlan100": {
            "gateway": "100.1.1.1",
            "lease_time": 3600,
            "mode": "PORT",
            "netmask": "255.255.255.0",
            "customized_options": [
                "option60"
            ],
            "state": "enabled"
        }
    },
    "DHCP_SERVER_IPV4_CUSTOMIZED_OPTIONS": {
        "option60": {
            "id": 60,
            "type": "string",
            "value": "dummy_value"
        }
    },
    "DHCP_SERVER_IPV4_RANGE": {
        "range1": {
            "ip_start": "100.1.1.3",
            "ip_end": "100.1.1.5"
        }
    },
    "DHCP_SERVER_IPV4_PORT": {
        "Vlan100|PortChannel0003": {
            "ips": [
                "100.1.1.10"
            ]
        },
        "Vlan100|PortChannel2": {
            "ranges": [
                "range1"
            ]
        }
    }
}
```

### DSCP_TO_TC_MAP
```
{
"DSCP_TO_TC_MAP": {
    "AZURE": {
        "1": "1",
        "0": "1",
        "3": "3",
        "2": "1",
        "5": "2",
        "4": "4",
        "7": "1",
        "6": "1",
        "9": "1",
        "8": "0"
    }
  }
}

```

### FG_NHG

The FG_NHG table provides information on Next Hop Groups, including a specified Hash Bucket Size (bucket_size), match mode for each group, an optional max-next-hops attribute for prefix_based match_ mode.

```
"FG_NHG": {
    "fgnhg_v4": {
        "bucket_size": "120",
        "match_mode": "nexthop-based"
    },
    "fgnhg_v6": {
        "bucket_size": "120",
        "match_mode": "nexthop-based"
    },
    "dynamic_fgnhg_v4": {
        "bucket_size": "120",
        "match_mode": "prefix-based",
        "max_next_hops": "6"
    },
    "dynamic_fgnhg_v6": {
        "bucket_size": "120",
        "match_mode": "prefix-based",
        "max_next_hops": "6"
    }    
}
```

### FG_NHG_MEMBER

The FG_NHG_MEMBER table provides information about the members of a next hop group, including the group name (FG_NHG), the index at which redistribution is performed (bank), and the link associated with the next-hop-ip (link).

```
"FG_NHG_MEMBER": {
    "200.200.200.4": {
        "FG_NHG": "fgnhg_v4",
        "bank": "0",
        "link": "Ethernet8"
    },
    "200.200.200.5": {
        "FG_NHG": "fgnhg_v4",
        "bank": "1",
        "link": "Ethernet12"
    }
}
```

### FG_NHG_PREFIX

The FG_NHG_PREFIX table provides the FG_NHG_PREFIX for which FG behavior is desired, and Fine Grained next-hop group name.

```
"FG_NHG_PREFIX": {
    "100.50.25.12/32": {
	    "FG_NHG": "fgnhg_v4"
	},
    "fc:05::/128": {
	    "FG_NHG": "fgnhg_v6"
	},
    "200.175.150.125/32": {
        "FG_NHG": "dynamic_fgnhg_v4"
    },
    "fd:06::/128": {
        "FG_NHG": "dynamic_fgnhg_v6"
	}    
}
```

### FABRIC_MONITOR
```
{
"FABRIC_MONITOR": {
    "FABRIC_MONITOR_DATA": {
        "monErrThreshCrcCells": "1",
        "monErrThreshRxCells": "61035156",
        "monPollThreshIsolation": "1",
        "monPollThreshRecovery": "8",
        "monCapacityThreshWarn": "10",
        "monState": "enable"
    }
  }
}

```

### FABRIC_PORT
```
{
"FABRIC_PORT": {
    "Fabric0": {
        "alias": "Fabric0",
        "isolateStatus": "False",
        "lanes": "0",
        "forceUnisolateStatus": "0"
    },
    "Fabric1": {
        "alias": "Fabric1",
        "isolateStatus": "False",
        "lanes": "1",
        "forceUnisolateStatus": "0"
    }
  }
}

```

### MPLS_TC_TO_TC_MAP
```
{
"MPLS_TC_TO_TC_MAP": {
    "AZURE": {
        "0": "0",
        "1": "1",
        "2": "1",
        "3": "2",
        "4": "2",
        "5": "3",
        "6": "3",
        "7": "4"
    }
  }
}

```

### FLEX_COUNTER_TABLE

```
{
	"FLEX_COUNTER_TABLE": {
		"PFCWD": {
			"FLEX_COUNTER_STATUS": "enable",
			"POLL_INTERVAL": "10000"
		},
		"PORT": {
			"FLEX_COUNTER_STATUS": "enable",
			"POLL_INTERVAL": "1000"
		},
		"QUEUE": {
			"FLEX_COUNTER_STATUS": "enable",
			"POLL_INTERVAL": "10000"
		},
		"TUNNEL": {
			"FLEX_COUNTER_STATUS": "enable",
			"POLL_INTERVAL": "10000"
		},
		"WRED_ECN_QUEUE": {
			"FLEX_COUNTER_STATUS": "enable",
			"POLL_INTERVAL": "10000"
		},
		"WRED_ECN_PORT": {
			"FLEX_COUNTER_STATUS": "enable",
			"POLL_INTERVAL": "1000"
		},
		"SWITCH": {
			"FLEX_COUNTER_STATUS": "enable",
			"POLL_INTERVAL": "1000"
		}
	}
}

```

### Hash

Generic hash allows user to configure various aspects of hashing algorithm.
The configuration is applied globally for each ECMP and LAG on a switch.

***ECMP/LAG HASH***

```
{
    "SWITCH_HASH": {
        "GLOBAL": {
            "ecmp_hash": [
                "DST_MAC",
                "SRC_MAC",
                "ETHERTYPE",
                "IP_PROTOCOL",
                "DST_IP",
                "SRC_IP",
                "L4_DST_PORT",
                "L4_SRC_PORT",
                "INNER_DST_MAC",
                "INNER_SRC_MAC",
                "INNER_ETHERTYPE",
                "INNER_IP_PROTOCOL",
                "INNER_DST_IP",
                "INNER_SRC_IP",
                "INNER_L4_DST_PORT",
                "INNER_L4_SRC_PORT",
                "IPV6_FLOW_LABEL"
            ],
            "lag_hash": [
                "DST_MAC",
                "SRC_MAC",
                "ETHERTYPE",
                "IP_PROTOCOL",
                "DST_IP",
                "SRC_IP",
                "L4_DST_PORT",
                "L4_SRC_PORT",
                "INNER_DST_MAC",
                "INNER_SRC_MAC",
                "INNER_ETHERTYPE",
                "INNER_IP_PROTOCOL",
                "INNER_DST_IP",
                "INNER_SRC_IP",
                "INNER_L4_DST_PORT",
                "INNER_L4_SRC_PORT",
                "IPV6_FLOW_LABEL"
            ],
            "ecmp_hash_algorithm": "CRC",
            "lag_hash_algorithm": "CRC"
        }
    }
}
```

### KDUMP

```
{
    "KDUMP": {
        "config": {
            "enabled": "true",
            "num_dumps": "3",
            "memory": "0M-2G:256M,2G-4G:256M,4G-8G:384M,8G-:448M",
            "remote": "true",
            "ssh_string": "username@ipaddress",
            "ssh_path": "a/b/c"
         }
     }
}

```

### Kubernetes Master

Kubernetes Master related configurations are stored in
**KUBERNETES_MASTER** table. These configurations are used mainly
for CTRMGR service. CTRMGR service will interactive with
kubernetes master according to these configurations.

```
{
    "KUBERNETES_MASTER": {
        "SERVER": {
            "disable": "False",
            "insecure": "True",
            "ip": "k8s.apiserver.com",
            "port": "6443"
        }
    }
}

```

### L2 Neighbors

The L2 neighbor and connection information can be configured in
**DEVICE_NEIGHBOR** table. Those information are used mainly for LLDP.
While mandatory fields include neighbor name acting as object key and
remote port / local port information in attributes, optional information
about neighbor device such as device type, hwsku, management address and
loopback address can also be defined.

```
{
"DEVICE_NEIGHBOR": {
        "ARISTA04T1": {
                "mgmt_addr": "10.20.0.163",
                "hwsku": "Arista",
                "lo_addr": null,
                "local_port": "Ethernet124",
                "type": "LeafRouter",
                "port": "Ethernet1"
        },
        "ARISTA03T1": {
                "mgmt_addr": "10.20.0.162",
                "hwsku": "Arista",
                "lo_addr": null,
                "local_port": "Ethernet120",
                "type": "LeafRouter",
                "port": "Ethernet1"
        },
        "ARISTA02T1": {
                "mgmt_addr": "10.20.0.161",
                "hwsku": "Arista",
                "lo_addr": null,
                "local_port": "Ethernet116",
                "type": "LeafRouter",
                "port": "Ethernet1"
        },
        "ARISTA01T1": {
                "mgmt_addr": "10.20.0.160",
                "hwsku": "Arista",
                "lo_addr": null,
                "local_port": "Ethernet112",
                "type": "LeafRouter",
                "port": "Ethernet1"
        }
    }
}
```

### L3 Interfaces

Configuration for L3 data plane interfaces are defined in the `INTERFACE`,
`VLAN_INTERFACE`, `VLAN_SUB_INTERFACE` and `PORTCHANNEL_INTERFACE` tables,
respectively.

The objects in all four tables have the interface as the key with the following
required patterns:
- `INTERFACE`: Any valid port name from the `PORT` table.  Typically `Ethernet{0-999}`.
- `VLAN_INTERFACE`: `Vlan{1-4095}`
- `PORTCHANNEL_INTERFACE`: `PortChannel{0-9999}`
- `VLAN_SUB_INTERFACE`: Any valid `INTERFACE` or `PORTCHANNEL_INTERFACE` name followed by a `.` and a number between 1 and 4094.  E.g. `Ethernet1.4`


These tables have a number of shared attributes as described below:
 * `vrf_name`: Valid VRF name from the `VRF` table.  Default: `default`
 * `nat_zone`: NAT Zone for this interface. `0..3`
 * `mpls`: Enable/disable MPLS routing for the interface. `enable` or `disable`. Default `disable`.
 * `ipv6_use_link_local_only`: Enable/Disable IPv6 link local address on interface. `enable` or `disable`. Default `disable`.
 * `mac_addr`: Assign administrator-provided MAC address to Interface.  If not specified will use the system MAC (same for all interfaces). Not applicable to `VLAN_SUB_INTERFACE` as it will use the parent interface's mac address.
 * `loopback_action`: Packet action when a packet ingress and gets routed on the same IP interface. `drop` or `forward`.


```json

{
    "INTERFACE": {
        "Ethernet0": {
            "ipv6_use_link_local_only": "enable",
            "mac_addr": "12:34:56:78:90:ab"
        },
        "Ethernet1": {
            "loopback_action": "drop"
        }
    },
    "VLAN_INTERFACE": {
        "Vlan201": {
            "vrf_name": "red",
            "mac_addr": "AB:CD:EF:12:34:56"
        }
    },
    "PORTCHANNEL_INTERFACE": {
        "PortChannel101": {
            "mac_addr": "1c:23:a8:56:de:2f"
        }
    },
    "VLAN_SUB_INTERFACE": {
        "Ethernet0.555": {
            "vrf_name": "Blue",
            "vlan": "555"
        }
    }
}
```

#### Assigning IP addresses to L3 Interfaces

The interface name and IP prefix act as multi-level key for the interface
objects.  The IP prefixes are stored in the same tables as the interface
attributes: `INTERFACE`, `VLAN_INTERFACE`, `VLAN_SUB_INTERFACE` and `PORTCHANNEL_INTERFACE`.

In the example below we will show one interface with attributes for clarity,
but otherwise this is simply an example of how IP addresses may be configured.

```json
{
    "INTERFACE": {
        "Ethernet0": {
            "ipv6_use_link_local_only": "enable",
            "mac_addr": "12:34:56:78:90:ab"
        },
        "Ethernet0|10.0.0.0/31": {},
        "Ethernet4|10.0.0.2/31": {},
        "Ethernet8|10.0.0.4/31": {},
    },
    "PORTCHANNEL_INTERFACE": {
        "PortChannel01|10.0.0.56/31": {},
        "PortChannel01|FC00::71/126": {},
        "PortChannel02|10.0.0.58/31": {},
        "PortChannel02|FC00::75/126": {}
    },
    "VLAN_INTERFACE": {
        "Vlan1000|192.168.0.1/27": {}
    },
    "VLAN_SUB_INTERFACE": {
        "Ethernet4.1|10.0.0.2/31": {}
    }
}
```

### Loopback Interface

Loopback interface configuration lies in **LOOPBACK_INTERFACE** table
and has similar schema with data plane interfaces. The loopback device
name and loopback IP prefix act as multi-level key for loopback
interface objects.
By default SONiC advertises Loopback interface IPv6 /128 subnet address
as prefix with /64 subnet. To overcome this set "bgp_adv_lo_prefix_as_128"
to true in DEVICE_METADATA

```
{
"LOOPBACK_INTERFACE": {
        "Loopback0|10.1.0.32/32": {},
        "Loopback0|FC00:1::32/128": {}
  }
}

```

### LOSSLESS_TRAFFIC_PATTERN

The LOSSLESS_TRAFFIC_PATTERN table stores parameters related to
lossless traffic for dynamic buffer calculation

```
{
    "LOSSLESS_TRAFFIC_PATTERN": {
        "AZURE": {
            "mtu": "1024",
            "small_packet_percentage": "100"
        }
    }
}
```

### Memory Statistics
The memory statistics configuration is stored in the **MEMORY_STATISTICS** table. This table is used by the memory statistics daemon to manage memory monitoring settings. The configuration allows enabling or disabling memory collection, specifying how frequently memory statistics are sampled, and defining how long the memory data is retained. 

```
{
    "MEMORY_STATISTICS": {
        "memory_statistics": {
            "enabled": "false",
            "sampling_interval": "5",
            "retention_period":  "15"
        }
    }
}

```
- **enabled**: Defines whether the memory statistics collection is active (true or false).
- **sampling_interval**: Interval between data collection.
- **retention_period**: Time to retain collected data.

### Management Interface

Management interfaces are defined in **MGMT_INTERFACE** table. Object
key is composed of management interface name and IP prefix. Attribute
***gwaddr*** specify the gateway address of the prefix.
***forced_mgmt_routes*** attribute can be used to specify addresses /
prefixes traffic to which are forced to go through management network
instead of data network.

```
{
"MGMT_INTERFACE": {
        "eth0|10.11.150.11/16": {
        "gwaddr": "10.11.0.1"
    },
    "eth0|FC00:2::32/64": {
        "forced_mgmt_routes": [
            "10.0.0.100/31",
            "10.250.0.8",
                "10.255.0.0/28"
        ],
        "gwaddr": "fc00:2::1"
    }
  }
}

```

### Management port

```
{
"MGMT_PORT": {
    "eth0": {
        "alias": "eth0",
        "admin_status": "up"
    }
  }
}

```


### Management VRF

```
{
"MGMT_VRF_CONFIG": {
    "vrf_global": {
        "mgmtVrfEnabled": "true"
     }
  }
}
```

### MAP_PFC_PRIORITY_TO_QUEUE

```
{
"MAP_PFC_PRIORITY_TO_QUEUE": {
    "AZURE": {
        "1": "1",
        "0": "0",
        "3": "3",
        "2": "2",
        "5": "5",
        "4": "4",
        "7": "7",
        "6": "6"
    }
  }
}
```
### MUX_CABLE

The **MUX_CABLE** table is used for dualtor interface configuration. The `cable_type` and `soc_ipv4` objects are optional.

```
{
    "MUX_CABLE": {
        "Ethernet4": {
            "cable_type": "active-active",
            "server_ipv4": "192.168.0.2/32",
            "server_ipv6": "fc02:1000::30/128",
            "soc_ipv4": "192.168.0.3/32",
            "state": "auto"
        }
    }
}
```

### MUX_LINKMGR
The **MUX_LINKMGR** table is used for dualtor device configuration.
```
{
    "MUX_LINKMGR": {
        "LINK_PROBER": {
            "interval_v4": "100",
            "interval_v6": "1000",
            "positive_signal_count": "1",
            "negative_signal_count": "3",
            "suspend_timer": "500",
            "use_well_known_mac": "enabled",
            "src_mac": "ToRMac",
            "interval_pck_loss_count_update": "3"
        },
        "MUXLOGGER": {
            "log_verbosity": "debug"
        },
        "SERVICE_MGMT": {
            "kill_radv": "True"
        }
    }
}
```

### NEIGH

The **NEIGH** table is used to keep track of resolved and static neighbors.

Resolve case:
```
{
    "NEIGH": {
        "Vlan100|100.1.1.3": {
            "family": "IPv4"
        }
    }
}
```
Static Nbr:
```
{
    "NEIGH": {
        "Vlan100|100.1.1.5": {
            "neigh": "00:02:02:03:04:05",
            "family": "IPv4"
        }
    }
}
```

### NTP Global Configuration

These configuration options are used to modify the way that
ntp binds to the ports on the switch and which port it uses to
make ntp update requests from.

***NTP Admin state***

If this option is set to `enabled` then ntp client will try to sync system time with configured NTP servers.
Otherwise, NTP client feature will be disabled.
```
{
    "NTP": {
        "global": {
            "admin_state": "enabled"
        }
    }
}
```

***NTP Server role***

This option is used to control NTP server state on the switch.
If this option is set to `enabled` switch will act as NTP server.
By default `server_role` is `disabled`.
```
{
    "NTP": {
        "global": {
            "server_role": "enabled"
        }
    }
}
```

***NTP VRF***

If this option is set to `default` then ntp will run within the default vrf
**when the management vrf is enabled**. If the mgmt vrf is enabled and this value is
not set to default then ntp will run within the mgmt vrf.

This option **has no effect** if the mgmt vrf is not enabled.

```
{
"NTP": {
    "global": {
        "vrf": "default"
        }
    }
}
```


***NTP Source Port***

This option sets the port which ntp will choose to send time update requests from by.

NOTE: If a Loopback interface is defined on the switch ntp will choose this by default, so this setting
is **required** if the switch has a Loopback interface and the ntp peer does not have defined routes
for that address.

```
{
"NTP": {
    "global": {
        "src_intf": "Ethernet1"
        }
    }
}
```

***NTP Authentication***

If this option is set to `enabled` then ntp will try to verify NTP servers it connects to.
This option **has no effect** if key is not set for NTP server.
By default it is `disabled`
```
{
    "NTP": {
        "global": {
            "authentication": "enabled"
        }
    }
}
```

***NTP DHCP leases***

If this option is set to `enabled` then ntp client will try to use NTP servers provided by DHCP server.
If this option is set to `disabled` you will be able to use the user-configured NTP servers.
By default it is `enabled`
```
{
    "NTP": {
        "global": {
            "dhcp": "enabled"
        }
    }
}
```

### NTP servers

These information are configured in individual tables. Domain name or IP
address of the server is used as object key. Currently there are no
attributes in those objects.

***NTP server***
```
{
    "NTP_SERVER": {
        "2.debian.pool.ntp.org": {
            "association_type": "pool",
            "iburst": "on",
            "admin_state": "enabled",
            "version": 4
        },
        "1.debian.pool.ntp.org": {
            "association_type": "pool",
            "iburst": "off",
            "admin_state": "enabled",
            "version": 3
        },
        "3.debian.pool.ntp.org": {
            "association_type": "pool",
            "iburst": "on",
            "admin_state": "disabled",
            "version": 4
        },
        "0.debian.pool.ntp.org": {
            "association_type": "pool",
            "iburst": "off",
            "admin_state": "disabled",
            "version": 3
        }
    },

    "NTP_SERVER": {
        "23.92.29.245": {
            "association_type": "server",
            "iburst": "on",
            "admin_state": "enabled",
            "version": 4,
            "key": 3,
            "trusted": "yes"
        },
        "204.2.134.164": {
            "association_type": "server",
            "iburst": "on",
            "admin_state": "enabled",
            "version": 3
        }
    }
}
```
* `association_type` - is used to control the type of the server. It can be `server` or `pool`.
* `iburst` - agressive server polling `{on, off}`.
* `version` - NTP protool version to use `[3..4]`.
* `key` - authentication key id `[1..65535]` to use to auth the server.
* `admin_state` - enable or disable specific server.
* `trusted` - trust this server when auth is enabled.

***NTP keys***
```
{
    "NTP_KEY": {
        "1": {
            "type": "md5",
            "value": "bXlwYXNzd29yZA==",
            "trusted": "yes"
        },
        "42": {
            "type": "sha1",
            "value": "dGhlYW5zd2Vy",
            "trusted": "no"
        }
    }
}
```
* `type` - key type to use `{md5, sha1, sha256, sha384, sha512}`.
* `value` - base64 encoded key value.
* `trusted` - trust this NTP key `{yes, no}`.

### Peer Switch

Below is an exmaple of the peer switch table configuration.
```
{
    "PEER_SWITCH": {
        "vlab-05": {
            "address_ipv4":  "10.1.0.33"
        }
    }
}
```

### Policer

Below is an example of the policer table configuration.
```
{
    "POLICER": {
        "everflow_static_policer": {
            "meter_type": "bytes",
            "mode": "sr_tcm",
            "cir": "12500000",
            "cbs": "12500000",
	    "pir": "17500000",
            "pbs": "17500000",
            "color": "aware",
            "red_packet_action": "drop",
	    "yellow_packet_action": "drop"
	    "green_packet_action": "forward"
       }
    }
}

```
Key to the table defines policer name Below are the fields
-   meter_type - Mandatory field. Defines how the metering is done. values - bytes, packets
-   mode - Mandatory field. Defines one of the three modes support. values - sr_tcm, tr_tcm, storm
-   cir  - Committed information rate bytes/sec or packets/sec based on meter_type
-   cbs - Committed burst size in bytes or packets based on meter_type
-   pir - Peak information rate in bytes/sec or packets/sec based on meter_type
-   pbs - Peak burst size in bytes or packets based on meter_type
-   color - Defines the color source for the policer. values - aware, blind
-   red_packet_action - Defines the action to be taken for red color packets
-   yellow_packet_action - Defines the action to be taken for yellow color packets
-   green_packet_action - Defines the action to be taken for green color packets.

The packet action could be:

-   'drop'
-   'forward'
-   'copy'
-   'copy_cancel'
-   'trap'
-   'log'
-   'deny'
-   'transit'
### Port

In this table the physical port configurations are defined. Each object
will have port name as its key, and port name alias and port speed as
optional attributes.

```
{
"PORT": {
        "Ethernet0": {
            "index": "0",
            "lanes": "101,102",
            "description": "fortyGigE1/1/1",
            "mtu": "9100",
            "alias": "fortyGigE1/1/1",
            "speed": "40000",
            "link_training": "off",
            "laser_freq": "191300",
            "tx_power": "-27.3",
            "dom_polling": "enabled",
            "core_id": "1",
            "core_port_id": "1",
            "num_voq": "8"
        },
        "Ethernet1": {
            "index": "1",
            "lanes": "103,104",
            "description": "fortyGigE1/1/2",
            "mtu": "9100",
            "alias": "fortyGigE1/1/2",
            "admin_status": "up",
            "speed": "40000",
            "link_training": "on",
            "laser_freq": "191300",
            "tx_power": "-27.3",
            "dom_polling": "enabled",
            "core_id": "0",
            "core_port_id": "14",
            "num_voq": "8"
        },
        "Ethernet63": {
            "index": "63",
            "lanes": "87,88",
            "description": "fortyGigE1/4/16",
            "mtu": "9100",
            "alias": "fortyGigE1/4/16",
            "speed": "40000",
            "laser_freq": "191300",
            "tx_power": "-27.3",
            "dom_polling": "disabled",
            "core_id": "0",
            "core_port_id": "15",
            "num_voq": "8"
        }
    }
}

2x100G port breakout
{
"PORT": {
        "Ethernet0": {
            "admin_status": "up",
            "index": "1",
            "lanes": "101,102,103,104",
            "description": "etp1a",
            "mtu": "9100",
            "alias": "etp1a",
            "speed": "100000",
            "subport": 1,
            "dom_polling": "enabled"
        },
        "Ethernet4": {
            "admin_status": "up",
            "index": "1",
            "lanes": "105,106,107,108",
            "description": "etp1b",
            "mtu": "9100",
            "alias": "etp1b",
            "speed": "100000",
            "subport": 2,
            "dom_polling": "enabled"
        },
    }
}


```

### Port Channel

Port channels are defined in **PORTCHANNEL** table with port channel
name as object key and member list as attribute.

```
{
"PORTCHANNEL": {
        "PortChannel0003": {
                "admin_status": "up",
        "min_links": "1",
        "members": [
            "Ethernet54"
        ],
        "mtu": "9100"
    },
    "PortChannel0004": {
        "admin_status": "up",
        "min_links": "1",
        "members": [
            "Ethernet56"
        ],
        "mtu": "9100",
        "fallback": "false",
        "fast_rate": "true"
    }
  }
}
```


### Portchannel member

```
{
"PORTCHANNEL_MEMBER": {
    "PortChannel0001|Ethernet50": {},
    "PortChannel0002|Ethernet52": {},
    "PortChannel0003|Ethernet54": {},
    "PortChannel0004|Ethernet56": {}
  }
}

```
### Scheduler

```
{
"SCHEDULER": {
    "scheduler.0": {
        "type": "STRICT"
    },
    "scheduler.1": {
        "type": "WRR"
        "weight": "1",
        "meter_type": "bytes",
        "pir": "1250000000",
        "pbs": "8192"
    },
    "scheduler.port": {
        "meter_type": "bytes",
        "pir": "1000000000",
        "pbs": "8192"
    }
  }
}
```

### Port QoS Map

```
{
"PORT_QOS_MAP": {
    "Ethernet50,Ethernet52,Ethernet54,Ethernet56": {
        "tc_to_pg_map": "AZURE",
        "tc_to_queue_map": "AZURE",
        "pfc_enable": "3,4",
        "pfc_to_queue_map": "AZURE",
        "dscp_to_tc_map": "AZURE",
        "dscp_to_fc_map": "AZURE",
        "exp_to_fc_map": "AZURE",
        "scheduler": "scheduler.port"
    }
  }
}
```

### Queue
```
{
"QUEUE": {
	"Ethernet56|4": {
        "wred_profile": "AZURE_LOSSLESS",
        "scheduler": "scheduler.1"
    },
    "Ethernet56|5": {
        "scheduler": "scheduler.0"
    },
    "Ethernet56|6": {
        "scheduler": "scheduler.0"
    }
  }
}
```

### Restapi
```
{
"RESTAPI": {
    "certs": {
        "ca_crt": "/etc/sonic/credentials/ame_root.pem",
        "server_key": "/etc/sonic/credentials/restapiserver.key",
        "server_crt": "/etc/sonic/credentials/restapiserver.crt",
        "client_crt_cname": "client.sonic.net"
    },
    "config": {
        "client_auth": "true",
        "log_level": "trace",
        "allow_insecure": "false"
    }
}
```
### Sflow

The below are the tables and their schema for SFLOW feature

SFLOW

| Field            | Description                                                                             | Mandatory   | Default   | Reference                                 |
|------------------|-----------------------------------------------------------------------------------------|-------------|-----------|-------------------------------------------|
| admin_state      | Global sflow admin state                                                                |             | down      |
| sample_direction | Global sflow sample direction                                                           |             | rx        |                                        |
| polling_interval | The interval within which sFlow data is collected and sent to the configured collectors |             | 20        |                                           |
| agent_id         | Interface name                                                                          |             |           | PORT:name,PORTCHANNEL:name,MGMT_PORT:name, VLAN:name |

SFLOW_SESSION

key - port
| Field       | Description                                                                                                             | Mandatory   | Default   | Reference   |
|-------------|-------------------------------------------------------------------------------------------------------------------------|-------------|-----------|-------------|
| port        | Sets sflow session table attributes for either all interfaces or a specific Ethernet interface.                         |             |           | PORT:name   |
| admin_state | Per port sflow admin state                                                                                              |             | up        |             |
| sample_rate | Sets the packet sampling rate.  The rate is expressed as an integer N, where the intended sampling rate is 1/N packets. |             |           |             |
| sample_direction| Per port sflow sample direction                                                                                               |             |   rx
SFLOW_COLLECTOR

key - name
| Field          | Description                                                                             | Mandatory   | Default   | Reference   |
|----------------|-----------------------------------------------------------------------------------------|-------------|-----------|-------------|
| name           | Name of the Sflow collector                                                             |             |           |             |
| collector_ip   | IPv4/IPv6 address of the Sflow collector                                                | true        |           |             |
| collector_port | Destination L4 port of the Sflow collector                                              |             | 6343      |             |
| collector_vrf  | Specify the Collector VRF. In this revision, it is either default VRF or Management VRF.|             |           |             |

### Storage Monitoring Daemon Interval Configuration

These options are used to configure the daemon polling and sync-to-disk interval
of the Storage Monitoring Daemon (stormond)

**Config Sample**
```
{
    "STORMOND_CONFIG": {
        "INTERVALS": {
            "daemon_polling_interval" : "60",
            "fsstats_sync_interval"   : "360"
        }
    }
}
```

*   `daemon_polling_interval` - Determines how often stormond queries the disk for relevant information and posts to STATE_DB
*   `fsstats_sync_interval`   - Determines how often key information from the STATE_DB is synced to a file on disk


### Syslog Global Configuration

These configuration options are used to configure rsyslog utility and the way
the system generates logs.

***Configuration sample***
```
{
    "SYSLOG_CONFIG": {
        "GLOBAL": {
            "rate_limit_interval": "5",
            "rate_limit_burst": "100",
            "format": "welf",
            "welf_firewall_name": "bla",
            "severity": "info"
        }
    }
}
```

* `rate_limit_interval` - determines the amount of time that is being measured for rate limiting: `unsigned integer`
* `rate_limit_burst` - defines the amount of messages, that have to occur in the time limit: `unsigned integer`
* `format` - syslog log format: `{standard, welf}`
* `welf_firewall_name` - WELF format firewall name: `string`
* `severity` - global log severity: `{emerg, alert, crit, error, warning, notice, info, debug}`

***Syslog Rate Limit***

Host side configuration:

```
{
"SYSLOG_CONFIG": {
    "GLOBAL": {
        "rate_limit_interval": "300",
        "rate_limit_burst": "20000"
    }
  }
}
```

Container side configuration:

```
{
"SYSLOG_CONFIG_FEATURE": {
    "bgp": {
        "rate_limit_interval": "300",
        "rate_limit_burst": "20000"
    },
    "pmon": {
        "rate_limit_interval": "300",
        "rate_limit_burst": "20000"
    }
  }
}
```

### Syslog servers

These information are configured in individual tables. Domain name or IP
address of the server is used as object key. Each server can be configurable.

***Configuration sample***
```
{
    "SYSLOG_SERVER": {
        "10.0.0.5": {},
        "10.0.0.6": {},
        "10.11.150.5": {}
    },

    "SYSLOG_SERVER" : {
        "4.4.4.4": {
            "source": "3.3.3.3",
            "port": "514",
            "vrf": "mgmt"
        },
        "2222::2222": {
            "source": "1111::1111",
            "port": "514",
            "vrf": "Vrf-Data"
        },
        "somehostname": {
            "filter": "include",
            "filter_regex": "ololo",
            "port": "514",
            "protocol": "tcp",
            "severity": "notice",
            "vrf": "default"
        }
    }
}
```

* `filter` - determines if syslog will include or exclude messages specified by regex: `{include, exclude}`
* `filter_regex` - filter messages by this regex: `string`
* `port` - network port to use to connect to remote server: `integer: 1..65535`
* `protocol` - network protocol to use to connect to remote server: `{tcp, udp}`
* `severity` - per-server log severity, overrifes global one: `{emerg, alert, crit, error, warning, notice, info, debug}`


### System Port
Every port on the system requires a global representation, known as a System Port,
and is listed in this table.

```
{
"SYSTEM_PORT": {
    "host227-4|asic0|Ethernet0": {
        "core_index": "1",
        "core_port_index": "1",
        "num_voq": "8",
        "speed": "100000",
        "switch_id": "0",
        "system_port_id": "1"
    },
    "host227-4|asic0|Ethernet4": {
        "core_index": "1",
        "core_port_index": "2",
        "num_voq": "8",
        "speed": "100000",
        "switch_id": "0",
        "system_port_id": "2"
    },
    "host227-5|asic0|Ethernet0": {
        "core_index": "1",
        "core_port_index": "1",
        "num_voq": "8",
        "speed": "100000",
        "switch_id": "4",
        "system_port_id": "80"
    },
    "host227-5|asic0|Ethernet4": {
        "core_index": "1",
        "core_port_index": "2",
        "num_voq": "8",
        "speed": "100000",
        "switch_id": "4",
        "system_port_id": "81"
    }
  }
}
```

### Tacplus Server

```
{
"TACPLUS_SERVER": {
    "10.0.0.8": {
        "priority": "1",
        "tcp_port": "49"
    },
    "10.0.0.9": {
        "priority": "1",
        "tcp_port": "49"
    }
  }
}
```

### TC to DSCP map

```json
{
    "TC_TO_DSCP_MAP": {
        "AZURE": {
            "5": "10",
            "6": "20"
        }
    }
}
```

**Note:**
* configuration is mandatory when packet trimming Asymmetric DSCP mode is used

### TC to Priority group map

```
{
"TC_TO_PRIORITY_GROUP_MAP": {
    "AZURE": {
        "1": "1",
        "0": "0",
        "3": "3",
        "2": "2",
        "5": "5",
        "4": "4",
        "7": "7",
        "6": "6"
    }
  }
}
```

### TC to Queue map

```
{
"TC_TO_QUEUE_MAP": {
    "AZURE": {
        "1": "1",
        "0": "0",
        "3": "3",
        "2": "2",
        "5": "5",
        "4": "4",
        "7": "7",
        "6": "6"
    }
  }
}
```

### Telemetry

```
{
    "TELEMETRY": {
        "certs": {
            "ca_crt": "/etc/sonic/telemetry/dsmsroot.cer",
            "server_crt": "/etc/sonic/telemetry/streamingtelemetryserver.cer",
            "server_key": "/etc/sonic/telemetry/streamingtelemetryserver.key"
        },
        "gnmi": {
            "client_auth": "true",
            "log_level": "2",
            "port": "50051",
            "save_on_set": "false"
        }
    }
}
```

### Telemetry client

```
{
    "TELEMETRY_CLIENT": {
        "Global": {
            "encoding": "JSON_IETF",
            "retry_interval": "30",
            "src_ip": "30.57.185.38",
            "unidirectional": "true"
        },
        "DestinationGroup|HS": {
            "dst_addr": "30.57.186.214:8081,30.57.185.39:8081"
        },
        "Subscription|HS_RDMA": {
            "dst_group": "HS",
            "path_target": "COUNTERS_DB",
            "paths": "COUNTERS/Ethernet*,COUNTERS_PORT_NAME_MAP",
            "report_interval": "5000",
            "report_type": "periodic"
        }
    }
}
```

### Tunnel

This table configures the MUX tunnel for Dual-ToR setup
```
{
    "TUNNEL": {
        "MuxTunnel0": {
            "dscp_mode": "uniform",
            "dst_ip": "10.1.0.32",
            "ecn_mode": "copy_from_outer",
            "encap_ecn_mode": "standard",
            "ttl_mode": "pipe",
            "tunnel_type": "IPINIP"
        }
    }
}
```

different example for configuring MUX tunnel
```
{
    "TUNNEL": {
        "MuxTunnel0": {
            "dscp_mode": "pipe",
            "dst_ip": "10.1.0.32",
            "ecn_mode": "standard",
            "encap_ecn_mode": "standard",
            "ttl_mode": "uniform",
            "tunnel_type": "IPINIP"
        }
    }
}
```

example mux tunnel configuration for when tunnel_qos_remap is enabled
```
{
    "TUNNEL": {
        "MuxTunnel0": {
            "tunnel_type": "IPINIP",
            "src_ip": "10.1.0.33",
            "dst_ip": "10.1.0.32",
            "dscp_mode": "pipe",
            "encap_ecn_mode": "standard",
            "ecn_mode": "copy_from_outer",
            "ttl_mode": "uniform",
            "decap_dscp_to_tc_map": "DecapDscpToTcMap",
            "decap_tc_to_pg_map": "DecapTcToPgMap",
            "encap_tc_to_dscp_map": "EncapTcToQueueMap",
            "encap_tc_to_queue_map": "EncapTcToDscpMap"
        }
    }
}
```

### Trimming

When the lossy queue exceeds a buffer threshold, it drops packets without any notification to the destination host.

When a packet is lost, it can be recovered through fast retransmission or by using timeouts.  
Retransmission triggered by timeouts typically incurs significant latency.

To help the host recover data more quickly and accurately, packet trimming is introduced.  
This feature upon a failed packet admission to a shared buffer, will trim a packet to a configured size,  
and try sending it on a different queue to deliver a packet drop notification to an end host.

***TRIMMING***

Symmetric DSCP and static queue:
```json
{
    "SWITCH_TRIMMING": {
        "GLOBAL": {
            "size": "128",
            "dscp_value": "48",
            "queue_index": "6"
        }
    }
}
```

Asymmetric DSCP and dynamic queue:
```json
{
    "SWITCH_TRIMMING": {
        "GLOBAL": {
            "size": "128",
            "dscp_value": "from-tc",
            "tc_value": "8",
            "queue_index": "dynamic"
        }
    }
}
```

**Note:**
* when `dscp_value` is set to `from-tc`, the `tc_value` is used for mapping to DSCP
* when `queue_index` is set to `dynamic`, the `dscp_value` is used for mapping to queue

### Versions

This table is where the curret version of the software is recorded.
```
{
    "VERSIONS": {
        "DATABASE": {
            "VERSION": "version_1_0_1"
        }
    }
}
```

### VLAN

This table is where VLANs are defined. VLAN name is used as object key,
and member list as well as an integer id are defined as attributes. If a
DHCP relay is required for this VLAN, a dhcp_servers attribute must be
specified for that VLAN, the value of which is a list that must contain
the domain name or IP address of one or more DHCP servers.

```
{
"VLAN": {
	"Vlan1000": {
		"dhcp_servers": [
			"192.0.0.1",
			"192.0.0.2",
			"192.0.0.3",
			"192.0.0.4"
		],
		"members": [
			"Ethernet0",
			"Ethernet4",
			"Ethernet8",
			"Ethernet12"
		],
		"vlanid": "1000"
	}
  }
}
```

### VLAN_MEMBER

VLAN member table has Vlan name together with physical port or port
channel name as object key, and tagging mode as attributes.

```
{
"VLAN_MEMBER": {
	"Vlan1000|PortChannel47": {
		"tagging_mode": "untagged"
	},
	"Vlan1000|Ethernet8": {
		"tagging_mode": "untagged"
	},
	"Vlan2000|PortChannel47": {
		"tagging_mode": "tagged"
	}
  }
}
```

### VNET

VNET table has Vnet name as the object key, and vxlan_tunnel name, scope, vni, peer list, advertised prefix, src mac, and overlay dest mac as attributes.
The vxlan_tunnel name (mandatory) is the tunnel name from the VXLAN table. scope (optional) must "default", vni (mandatory) is the vxlan tunnel vni, peer_list (optional) is for Vnet
peering, advertise_prefix (optional) is used to allow advertisement of this vnet's routes, overlay_dmac (optional) is the mac address which is used for VNET ping
monitoring sessions for the vnet routes and is optional.

```
{
"VNET": {
	"Vnet1-1": {
	    "vxlan_tunnel": "vtep1",
		"scope": "default",
		"vni": "10011",
		"peer_list": "",
		"advertise_prefix": "true",
		"overlay_dmac": "22:33:44:55:66:77"
	},
    "Vnetv4_v4-0": {
	    "vxlan_tunnel": "vtep2",
		"scope": "default",
		"vni": "10011",
	}
  }
}
```

### VNET_ROUTE_TUNNEL

VNET_ROUTE_TUNNEL table has vnet_name|prefix as the object key, where vnet_name is the name of the VNet and prefix is the ip4 prefix associated with the route tunnel. The table includes the following attributes:
- ENDPOINT: Comma-separated endpoint/nexthop tunnel IPs (mandatory). They are used to identify the endpoints of the tunnel.
- MAC_ADDRESS: Comma-separated inner destination MAC addresses in the encapsulated packet (optional).  They should be 12-hexadecimal digit values.
- VNI: Comma-separated VNI values in the encapsulated packet (optional). They should be numeric values.
- CONSISTENT_HASHING_BUCKETS: Number of consistent hashing buckets to use, if consistent hashing is desired (optional). It should be a numeric value.

```
{
  "VNET_ROUTE_TUNNEL": {
        "Vnet_1000|100.200.1.1/32": {
        "endpoint": "192.174.1.1,192.174.1.2",
        "mac_address": "f8:25:84:98:22:a1,f8:25:84:98:22:a2",
        "vni": "10010,10011",
        "consistent_hashing_buckets": "10"
    },
    "Vnet_2000|100.100.1.1/32": {
        "endpoint": "192.168.1.1",
        "mac_address": "f9:22:83:99:22:a2"
    },
    "Vnetv4_v4-0|10.0.1.0/24": {
        "endpoint": "192.168.1.2",
        "mac_address": "f8:22:83:99:22:a2",
        "vni": "10012"
    }
  }
}
```

### VOQ INBAND INTERFACE

VOQ_INBAND_INTERFACE holds the name of the inband system port dedicated for cpu communication. At this time, only inband_type of "port" is supported

```
"VOQ_INBAND_INTERFACE": {
    "Ethernet-IB0": {
	   "inband_type": "port"
	},
	"Ethernet-IB0|3.3.3.1/32": {},
    "Ethernet-IB0|3333::3:5/128": {}
}
```

### VXLAN

VXLAN_TUNNEL holds the VTEP source ip configuration (maximum 2 tunnels).
VXLAN_TUNNEL_MAP holds the vlan to vni and vni to vlan mapping configuration.
VXLAN_EVPN_NVO holds the VXLAN_TUNNEL object to be used for BGP-EVPN discovered tunnels.

Single tunnel example:
```
{
    "VXLAN_TUNNEL": {
        "vtep1": {
            "src_ip": "10.10.10.10",
            "dst_ip": "12.12.12.12",
            "ttl_mode": "pipe"
        }
    },
    "VXLAN_TUNNEL_MAP": {
        "vtep1|map_1000_Vlan100": {
            "vni": "1000",
            "vlan": "100"
        },
        "vtep1|map_22000_Vlan70": {
            "vni": "22000",
            "vlan": "70"
        }
    },
    "VXLAN_EVPN_NVO": {
        "nvo1": {
            "source_vtep": "vtep1"
        }
    }
}
```

Dual tunnel example:
```
{
    "VXLAN_TUNNEL": {
        "vtep1": {
            "src_ip": "10.10.10.10",
            "dst_ip": "12.12.12.12",
            "ttl_mode": "uniform"
        },
        "vtep2": {
            "src_ip": "10.20.10.10",
            "dst_ip": "20.20.20.20"
        }
    },
    "VXLAN_TUNNEL_MAP": {
        "vtep1|map_1000_Vlan100": {
            "vni": "1000",
            "vlan": "100"
        },
        "vtep1|map_22000_Vlan70": {
            "vni": "22000",
            "vlan": "70"
        },
        "vtep2|map_2000_Vlan200": {
            "vni": "2000",
            "vlan": "200"
        },
        "vtep2|map_3000_Vlan300": {
            "vni": "3000",
            "vlan": "300"
        }
    },
    "VXLAN_EVPN_NVO": {
        "nvo1": {
            "source_vtep": "vtep1"
        }
    }
}
```

### Virtual router

The virtual router table allows to insert or update a new virtual router
instance. The key of the instance is its name. The attributes in the
table allow to change properties of a virtual router. Attributes:

-   'v4' contains boolean value 'true' or 'false'. Enable or
    disable IPv4 in the virtual router
-   'v6' contains boolean value 'true' or 'false'. Enable or
    disable IPv6 in the virtual router
-   'src_mac' contains MAC address. What source MAC address will be
    used for packets egressing from the virtual router
-   'ttl_action' contains packet action. Defines the action for
    packets with TTL == 0 or TTL == 1
-   'ip_opt_action' contains packet action. Defines the action for
    packets with IP options
-   'l3_mc_action' contains packet action. Defines the action for
    unknown L3 multicast packets
-   'vni' contains L3 VNI value. VNI associated Virtual router instance.

The packet action could be:

-   'drop'
-   'forward'
-   'copy'
-   'copy_cancel'
-   'trap'
-   'log'
-   'deny'
-   'transit'


***TBD***
```
'VRF:rid1': {
	'v4': 'true',
	'v6': 'false',
	'src_mac': '02:04:05:06:07:08',
	'ttl_action': 'copy',
	'ip_opt_action': 'deny',
	'l3_mc_action': 'drop',
	'vni': '100'
}
```


### WRED_PROFILE

```
{
"WRED_PROFILE": {
    "AZURE_LOSSLESS": {
        "red_max_threshold": "2097152",
        "wred_green_enable": "true",
        "ecn": "ecn_all",
        "green_min_threshold": "1048576",
        "red_min_threshold": "1048576",
        "wred_yellow_enable": "true",
        "yellow_min_threshold": "1048576",
        "green_max_threshold": "2097152",
        "green_drop_probability": "5",
        "yellow_max_threshold": "2097152",
        "wred_red_enable": "true",
        "yellow_drop_probability": "5",
        "red_drop_probability": "5"
    }
  }
}
```

### Logger

In this table, the loglevel and logoutput of the components are defined. Each component
will have the component name as its key; and LOGLEVEL and LOGOUTPUT as attributes.
The LOGLEVEL attribute will define the verbosity of the component.
The LOGOUTPUT attribute will define the file of printing the logs.

```
{
    "LOGGER": {
        "orchagent": {
                "LOGLEVEL": "NOTICE",
                "LOGOUTPUT": "SYSLOG"
            },
            "syncd": {
                "LOGLEVEL": "DEBUG",
                "LOGOUTPUT": "STDOUT"
            },
            "SAI_API_LAG": {
                "LOGLEVEL": "ERROR",
                "LOGOUTPUT": "STDERR"
            }
    }
}

```

### PASSWORD_HARDENING

Password Hardening, a user password is the key credential used in order to verify the user accessing the switch and acts as the first line of defense in regards to securing the switch. PASSWORD_HARDENING - support the enforce strong policies.

-   state - Enable/Disable password hardening feature
-   len_min - The minimum length of the PW should be subject to a user change.
-   expiration - PW Age Change Once a PW change takes place - the DB record for said PW is updated with the new PW value and a fresh new age (=0).
-   expiration_warning - The switch will provide a warning for PW change before and (this is to allow a sufficient warning for upgrading the PW which might be relevant to numerous switches).
-   history_cnt - remember last passwords, and reject to use the old passw
-   reject_user_passw_match - reject to set same username and passw
-   PW classes -  are the type of characters the user is required to enter when setting/updating a PW.
There are 4 classes
    -   lower_class - Small characters - a-z
    -   upper_class - Big characters - A-Z
    -   digits_class -Numbers - 0-9
    -   special_class - Special Characters `~!@#$%^&*()-_+=|[{}];:',<.>/? and white space
```
{
"PASSW_HARDENING": {
    "POLICIES": {
        "state": "disabled",
        "expiration": "180",
        "expiration_warning": "15",
        "history_cnt": "10",
        "len_min": "8",
        "reject_user_passw_match": "true",
        "lower_class": "true",
        "upper_class": "true",
        "digits_class": "true",
        "special_class": "true"
    }
  }
}
```

### SERIAL_CONSOLE

In this table collected configuration of the next serial-console attributes:
-   inactivity_timeout - Inactivity timeout for serial-console session, allowed values: 0-35000 (minutes), default value: 15
-   sysrq_capabilities - Enabling or disabling SysRq functionality for serial-console session, allowed values: enabled/disabled, default value disabled

```
{
    SERIAL_CONSOLE:{
        "POLICIES":{
            "inactivity_timeout": 15
            "sysrq_capabilities": "disabled"
        }
    }
}
```

### SSH_SERVER

In this table, we allow configuring ssh server global settings. This will feature includes 5 configurations:

-   authentication_retries - number of login attepmts 1-100
-   login_timeout - Timeout in seconds for login session for user to connect 1-600
-   ports - Ssh port numbers - string of port numbers seperated by ','
-   inactivity_timeout - Inactivity timeout for SSH session, allowed values: 0-35000 (min), default value: 15 (min)
-   max_sessions - Max number of concurrent logins, allowed values: 0-100 (where 0 means no limit), default value: 0
-   permit_root_login - Whether or not to allow root login. Default value: "prohibit-password"
    - "yes"
    - "prohibit-password"
    - "forced-commands-only"
    - "no"
-   password_authentication - Whether or not to allow password authentication. Boolean.
-   ciphers - Ciphers to allow.  See `ssh -Q ciphers`
-   kex_algorithms - Key Exchange algorithms to allow.  See `ssh -Q kex_algorithms`
-   macs - MAC algorithms to allow.  See `ssh -Q macs`
```
{
    "SSH_SERVER": {
        "POLICIES":{
            "authentication_retries": "6",
            "login_timeout": "120",
            "ports": "22",
            "inactivity_timeout": "15",
            "max_sessions": "0",
            "permit_root_login": "false",
            "password_authentication": "true",
            "ciphers": [ "chacha20-poly1305@openssh.com", "aes256-gcm@openssh.com" ],
            "kex_algorithms": [ "sntrup761x25519-sha512", "curve25519-sha256", "ecdh-sha2-nistp521" ],
            "macs": [ "hmac-sha2-512-etm@openssh.com", "hmac-sha2-512" ]
        }
    }
}
```

### BREAKOUT_CFG

This table is introduced as part of Dynamic Port Breakout(DPB) feature.
It shows the current breakout mode of all ports(root ports).
The list of root ports, all possible breakout modes, and default breakout modes
 are obtained/derived from platform.json and hwsku.json files.

```
"BREAKOUT_CFG": {
    "Ethernet0": {
        "brkout_mode": "4x25G[10G]"
    },
    "Ethernet4": {
        "brkout_mode": "4x25G[10G]"
    },
    "Ethernet8": {
        "brkout_mode": "4x25G[10G]"
    },

        ......

    "Ethernet116": {
        "brkout_mode": "2x50G"
    },
    "Ethernet120": {
        "brkout_mode": "2x50G"
    },
    "Ethernet124": {
        "brkout_mode": "2x50G"
    }
}
```

### AAA

The AAA table defined the method SONiC used for Authentication, Authorization and Accounting.
The method could be:
-   default
-   local
-   tacacs+
-   radius

```
"AAA": {
    "authentication": {
       "login": "local"
    },
    "authorization": {
       "login": "local"
    },
    "accounting": {
       "login": "local"
    }
}
```

### SUBNET_DECAP

The **SUBNET_DECAP** table is used for subnet decap configuration.

```
"SUBNET_DECAP": {
    "AZURE": {
        "status": "enable",
        "src_ip": "10.10.10.0/24",
        "src_ip_v6": "20c1:ba8::/64"
    }
}
```

### SYSTEM_DEFAULTS table
To have a better management of the features in SONiC, a new table `SYSTEM_DEFAULTS` is introduced.

```
"SYSTEM_DEFAULTS": {
        "tunnel_qos_remap": {
            "status": "enabled"
        }
        "default_bgp_status": {
            "status": "down"
        }
        "synchronous_mode": {
            "status": "enable"
        }
        "dhcp_server": {
            "status": "enable"
        }
    }
```
The default value of flags in `SYSTEM_DEFAULTS` table can be set in `init_cfg.json` and loaded into db at system startup. These flags are usually set at image being build, and are unlikely to change at runtime.

If the values in `config_db.json` is changed by user, it will not be rewritten back by `init_cfg.json` as `config_db.json` is loaded after `init_cfg.json` in [docker_image_ctl.j2](https://github.com/Azure/sonic-buildimage/blob/master/files/build_templates/docker_image_ctl.j2)

For the flags that can be changed by reconfiguration, we can update entries in `minigraph.xml`, and parse the new values in to config_db with minigraph parser at reloading minigraph. If there are duplicated entries in `init_cfg.json` and `minigraph.xml`, the values in `minigraph.xml` will overwritten the values defined in `init_cfg.json`.

### RADIUS

The RADIUS and RADIUS_SERVER tables define RADIUS configuration parameters. RADIUS table carries global configuration while RADIUS_SERVER table carries per server configuration.

```
   "RADIUS": {
       "global": {
              "auth_type": "pap",
              "timeout": "5"
        }
    }

    "RADIUS_SERVER": {
        "192.168.1.2": {
               "priority": "4",
               "retransmit": "2",
               "timeout": "5"
        }
    }
```

### Static DNS

The DNS_NAMESERVER table introduces static DNS nameservers configuration.

```json
{
	"DNS_NAMESERVER": {
		"1.1.1.1": {},
		"fe80:1000:2000:3000::1": {}
	}
}
```

DNS configuration options can also be set when nameservers are defined:
```json
{
    "DNS_OPTIONS": {
        "search": [ "d1.example.com", "d2.example.com", "d3.example.com" ],
        "ndots": 0,
        "timeout": 1,
        "attempts": 2
    }
}
```

### SRv6

The **SRV6_MY_SIDS** and **SRV6_MY_LOCATORS** tables introduce Segment Routing over IPv6 configuration.
An example is as follows:
```
{
    "SRV6_MY_LOCATORS" : {
        "loc1" : {
            "prefix" : "FCBB:BBBB:20::"
        }
    }
    "SRV6_MY_SIDS" : {
        "loc1|FCBB:BBBB:20::" : {
           "action": "uN"
        },
        "loc1|FCBB:BBBB:20:F1::" : {
           "action": "uDT46",
           "decap_vrf": "default",
           "decap_dscp_mode": "pipe"
        }
    }
}
```

### Prefix List
Prefix list table stores a list of prefixes with type and prefix separated by `|`. The specific configuration for the prefix type are then rendered by the PrefixListMgr. Currently ANCHOR_PREFIX is supported to add RADIAN configuration.

An example is as follows:
```json
{
    "PREFIX_LIST": {
        "ANCHOR_PREFIX|fc00::/48": {}
    }
}
```

### FIPS

The FIPS table introduces FIPS  configuration.

```json
{
    "FIPS": {
        "global" : {
            "enable": "true",
            "enforce": "false"
        }
    }
}
```
### MID_PLANE_BRIDGE"

The MID_PLANE_BRIDGE" table introduces the configuration for the midplane bridge interface for Smart Switch.

```json
{
    "MID_PLANE_BRIDGE": {
        "GLOBAL" : {
            "bridge": "bridge-midplane",
            "ip_prefix": "169.254.200.254/24"
        }
    }
}
```

### DPUS

The DPUS table introduces the information on the DPUs (Data Processing Unit) available on the platform.

```json
{
    "DPUS": {
        "dpu0": {
            "midplane_interface": "dpu0"
        },
        "dpu1": {
            "midplane_interface": "dpu1"
        }
    }
}
```

### ASIC_SENSORS

The ASIC_SENSORS table introduces the asic sensors polling configuration when they are available on the platform.

```json
{
    "ASIC_SENSORS": {
        "ASIC_SENSORS_POLLER_INTERVAL": {
            "interval": "10"
        },
        "ASIC_SENSORS_POLLER_STATUS": {
            "admin_status": "enable"
        }
    }
}
```

### DPU Configuration

The **DPU** table introduces the configuration for the DPUs (Data Processing Unit) information available on the platform.

```json
{
    "DPU": {
        "str-8102-t1-dpu0": {
            "state": "up",
            "local_port": "Ethernet228",
            "vip_ipv4": "192.168.1.1",
            "vip_ipv6": "2001:db8::10",
            "pa_ipv4": "192.168.1.10",
            "pa_ipv6": "2001:db8::10",
            "midplane_ipv4": "169.254.200.245",
            "dpu_id": "0",
            "vdpu_id": "vdpu0",
            "gnmi_port": "50052",
            "orchagent_zmq_port": "50"
        },
        "str-8102-t1-dpu1": {
            "state": "down",
            "local_port": "Ethernet232",
            "vip_ipv4": "192.168.1.2",
            "vip_ipv6": "2001:db8::20",
            "pa_ipv4": "192.168.1.20",
            "pa_ipv6": "2001:db8::20",
            "midplane_ipv4": "169.254.150.20",
            "dpu_id": "1",
            "vdpu_id": "vdpu1",
            "gnmi_port": "50052",
            "orchagent_zmq_port": "50"
        }
    }
}
```

**state**: Administrative status of the DPU (`up` or `down`).

**local_port**: local port mapped to DPU port on the switch.

**vip_ipv4**: VIP IPv4 address from minigraph.

**vip_ipv6**: VIP IPv6 address from minigraph.

**pa_ipv4**: PA IPv4 address from minigraph.

**pa_ipv6**: PA IPv6 address from minigraph.

**dpu_id**: Id of the DPU from minigraph.

**vdpu_id**: ID of VDPUs from minigraph.

**gnmi_port**: TCP listening port for gnmi service on DPU.

**orchagent_zmq_port**: TCP listening port for ZMQ service on DPU orchagent.

### REMOTE_DPU Configuration

The **REMOTE_DPU** table introduces the configuration for the remote DPUs (Data Processing Unit) accessible on other machines.

```json
{
    "REMOTE_DPU": {
        "str-8103-t1-dpu0": {
            "type": "typeA",
            "pa_ipv4": "192.168.2.1",
            "pa_ipv6": "2001:db8::30",
            "npu_ipv4": "192.168.2.10",
            "npu_ipv6": "2001:db8::40",
            "dpu_id": "0",
            "swbus_port": "23606"
        },
        "str-8103-t1-dpu1": {
            "type": "typeB",
            "pa_ipv4": "192.168.2.2",
            "pa_ipv6": "2001:db8::50",
            "npu_ipv4": "192.168.2.20",
            "npu_ipv6": "2001:db8::60",
            "dpu_id": "1",
            "swbus_port": "23607"
        }
    }
}
```

**type**: Type of the DPU.

**pa_ipv4**: DPU IPv4 physical address.

**pa_ipv6**: DPU IPv6 physical address.

**npu_ipv4**: Loopback IPv4 address of remote NPU.

**npu_ipv6**: Loopback IPv6 address of remote NPU.

**dpu_id**: ID of the DPU from minigraph.

**swbus_port**: TCP listening port for swbus service for this DPU. Must be 23606 + dpu_id.

### VDPU Configuration

The **VDPU** table introduces the configuration for the VDPUs (Virtual Data Processing Unit) information available on the platform.

```json
{
    "VDPU": {
        "vdpu0": {
            "profile": "",
            "tier": "",
            "main_dpu_ids": ["dpu0"]
        },
        "vdpu1": {
            "profile": "",
            "tier": "",
            "main_dpu_ids": ["dpu1"]
        },
        "vdpu2": {
            "profile": "",
            "tier": "",
            "main_dpu_ids": ["dpu2"]
        },
        "vdpu3": {
            "profile": "",
            "tier": "",
            "main_dpu_ids": ["dpu3"]
        }
    }
}
```

**profile**: VDPU profile. Currently unused, reserved for future use.

**tier**: VDPU tier. Currently unused, reserved for future use.

**main_dpu_ids**: Main DPUs involved in this VDPU.

### DASH HA Global Configuration

The **DASH_HA_GLOBAL_CONFIG** table introduces the configuration for the DASH High Availability global settings available on the platform.
Like NTP global configuration, DASH HA global configuration must have one entry with the key "global".

```json
{
    "DASH_HA_GLOBAL_CONFIG": {
        "global": {
            "vnet_name": "Vnet55",
            "cp_data_channel_port": "11362",
            "dp_channel_dst_port": "11368",
            "dp_channel_src_port_min": "49152",
            "dp_channel_src_port_max": "53247",
            "dp_channel_probe_interval_ms": "100",
            "dp_channel_probe_fail_threshold": "3",
            "dpu_bfd_probe_interval_in_ms": "100",
            "dpu_bfd_probe_multiplier": "3"
        }
    }
}
```

**vnet_name**: Vnet name used in SmartSwitch HA scenarios.

**cp_data_channel_port**: Control plane data channel port, used for bulk sync.

**dp_channel_dst_port**: Destination port when tunneling packets via DPU-to-DPU data plane channel.

**dp_channel_src_port_min**: Minimum source port used when tunneling packets via DPU-to-DPU data plane channel.

**dp_channel_src_port_max**: Maximum source port used when tunneling packets via DPU-to-DPU data plane channel.

**dp_channel_probe_interval_ms**: Interval in milliseconds for sending each DPU-to-DPU data path probe.

**dp_channel_probe_fail_threshold**: Number of probe failures needed to consider data plane channel as dead.

**dpu_bfd_probe_interval_in_ms**: Interval in milliseconds for DPU BFD probe.

**dpu_bfd_probe_multiplier**: Number of DPU BFD probe failures before considering the probe as down.

# For Developers

## Generating Application Config by Jinja2 Template

To be added.

## Incremental Configuration by Subscribing to ConfigDB

Detail instruction to be added. A sample could be found in this
[PR](https://github.com/Azure/sonic-buildimage/pull/861) that
implemented dynamic configuration for BGP.
