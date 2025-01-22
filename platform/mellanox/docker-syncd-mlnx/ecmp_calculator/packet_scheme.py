#!/usr/bin/env python3
#
# Copyright (c) 2022-2023 NVIDIA CORPORATION & AFFILIATES.
# Apache-2.0
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

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
