#!/usr/bin/env python3

PACKET_SCHEME = {
    "type" : "object",
    "properties" :
    {
        "packet_info" : 
        {
            "type" : "object",
            "properties" : 
            {
                "outer" : 
                {
                    "type" : "object",
                    "properties" : 
                    {
                        "layer2" : 
                        {
                            "type" : "object",
                            "properties" : 
                            {
                                "smac": {"type": "string"},
                                "dmac": {"type": "string"},
                                "ethertype": {"type": "number"},
                                "outer_vid": {"type": "number"},
                                "outer_pcp": {"type": "number"},
                                "outer_dei": {"type": "number"},
                                "inner_vid": {"type": "number"},
                                "inner_pcp": {"type": "number"},
                                "inner_dei": {"type": "number"}
                            }
                        },
                        "arp" :
                        {
                            "type" : "object",
                            "properties" : 
                            {
                                "spa": {"type": "string"},
                                "tpa": {"type": "string"}
                            }
                        },
                        "ipv4" : 
                        {
                            "type" : "object",
                            "properties" : 
                            {
                                "sip": {"type": "string"},
                                "dip": {"type": "string"},
                                "proto": {"type": "number"}
                            },
                            "required": ["dip"]
                        },
                        "ipv6" : 
                        {
                            "type" : "object",
                            "properties" : 
                            {
                                "sip": {"type": "string"},
                                "dip": {"type": "string"},
                                "mflag": {"type": "number"},
                                "next_header": {"type": "number"},
                                "dscp": {"type": "number"},
                                "ecn": {"type": "number"},
                                "l3_length": {"type": "number"},
                                "flow_label": {"type": "number"}
                            },
                            "required": ["dip"]
                        },
                        "tcp_udp" : 
                        {
                            "type" : "object",
                            "properties" : 
                            {
                                "sport": {"type": "number"},
                                "dport": {"type": "number"}
                            }
                        },
                        "vxlan_nvgre" : 
                        {
                            "type" : "object",
                            "properties" : 
                            {
                                "vni": {"type": "number"}
                            }
                        }
                    }
                },                      
                "inner" : 
                {
                    "type" : "object",
                    "properties" : 
                    {
                        "layer2" : 
                        {
                            "type" : "object",
                            "properties" : 
                            {
                                "smac": {"type": "string"},
                                "dmac": {"type": "string"},
                                "ethertype": {"type": "number"}
                            }
                        },
                        "ipv4" : 
                        {
                            "type" : "object",
                            "properties" : 
                            {
                                "sip": {"type": "string"},
                                "dip": {"type": "string"},
                                "mflag": {"type": "number"},
                                "proto": {"type": "number"}
                            }
                        },
                        "ipv6" : 
                        {
                            "type" : "object",
                            "properties" : 
                            {
                                "sip": {"type": "string"},
                                "dip": {"type": "string"},
                                "mflag": {"type": "number"},
                                "next_header": {"type": "number"},
                                "flow_label": {"type": "number"}
                            }
                        },
                        "tcp_udp" : 
                        {
                            "type" : "object",
                            "properties" : 
                            {
                                "sport": {"type": "number"},
                                "dport": {"type": "number"}
                            }
                        }
                    }
                }                        
            }
        }
    }
}
