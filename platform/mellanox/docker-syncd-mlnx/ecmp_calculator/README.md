# SONiC ECMP Calculator

## Description
An equal cost multipath (ECMP) is formed when routing table contains multiple next hop addresses for the same destination IP address.
ECMP will load balance the outbound traffic between the IP interfaces. 
The purpose of ECMP calculator is to calculate which IP interface ECMP will choose and return the physical interface packet will egress from. 
Packet is defined by a JSON file given as an argument to the tool.

## Usage notes
1.  ECMP calculator performs its calculations based on the current operational state of the router. In order to calculate the egress port, it fetches routes from HW. Routes exist in HW only for next hops with a resolved ARP.
2.  ECMP calculator supports only routed packets.
-   IPv4/IPv6 TCP/UDP packets
-   IPinIP and VXLAN encapsulated packets
3.  Changes done in the packet classification (e.g. ACL, PBR) are not taken into consideration during calculation.

## Command line interface
1.  User shall enter syncd container to run ECMP calculator.
2.  User shall provide the following input parameters:
- JSON file describing a packet
- Ingress port (e.g. "Ethernet0", must pe a physical interface)
- Debug option for debug purposes (optional)
- VRF name (optional)
3.  Usage example:
```
$ docker exec -it syncd bash
$ /usr/bin/ecmp_calc.py -i Ethernet0 -p ./packet.json -v Vrf_red
Egress port: Ethernet4
```
```
$ docker exec -it syncd bash
$ /usr/bin/ecmp_calc.py -h
usage: ecmp_calc.py [-h] -i INTERFACE -p PACKET [-v VRF] [-d]

ECMP calculator

optional arguments:
  -h, --help            show this help message and exit
  -i INTERFACE, --interface INTERFACE
                        Ingress interface
  -p PACKET, --packet PACKET
                        Packet description
  -v VRF, --vrf VRF     VRF name
  -d, --debug           Flag for debug

```

## Packet JSON
1.  Numbers in packet JSON must be in base-ten.
2.  For packets with single header, outer header shall be provided.
3.  The following table defines the structure of a packet JSON file.

| ecmp_hash |             |       |             |             |        |                                                                                  |
|-----------|-------------|-------|-------------|-------------|--------|----------------------------------------------------------------------------------|
|           | packet_info |       |             |             | object |                                                                                  |
|           |             | outer |             |             | object |                                                                                  |
|           |             |       | layer2      |             | object |                                                                                  |
|           |             |       |             | smac        | string |                                                                                  |
|           |             |       |             | dmac        | string |                                                                                  |
|           |             |       |             | ethertype   | number | 16bits, needed for IPv4 or IPv6 packet                                           |
|           |             |       |             | outer_vid   | number | 12bits                                                                           |
|           |             |       |             | outer_pcp   | number | 3bits                                                                            |
|           |             |       |             | outer_dei   | number | 1bits                                                                            |
|           |             |       |             | inner_vid   | number | QinQ                                                                             |
|           |             |       |             | inner_pcp   | number | QinQ                                                                             |
|           |             |       |             | inner_dei   | number | QinQ                                                                             |
|           |             |       | ipv4        |             | object |                                                                                  |
|           |             |       |             | sip         | string |                                                                                  |
|           |             |       |             | dip         | string |                                                                                  |
|           |             |       |             | proto       | number | 8bits                                                                            |
|           |             |       |             | dscp        | number | 6bits                                                                            |
|           |             |       |             | ecn         | number | 2bits                                                                            |
|           |             |       |             | mflag       | number | 1bit                                                                             |
|           |             |       |             | l3_length   | number | 16bits                                                                           |
|           |             |       | ipv6        |             | object | should not co-exist with ipv4 field                                              |
|           |             |       |             | sip         | string |                                                                                  |
|           |             |       |             | dip         | string |                                                                                  |
|           |             |       |             | mflag       | number | 1bit                                                                             |
|           |             |       |             | next_header | number | 8bits                                                                            |
|           |             |       |             | dscp        | number | 6bits                                                                            |
|           |             |       |             | ecn         | number | 2bits                                                                            |
|           |             |       |             | l3_length   | number | 16bits                                                                           |
|           |             |       |             | flow_label  | number | 20bits                                                                           |
|           |             |       | tcp_udp     |             | object |                                                                                  |
|           |             |       |             | sport       | number | 16bits                                                                           |
|           |             |       |             | dport       | number | 16bits                                                                           |
|           |             |       | vxlan_nvgre |             | object |                                                                                  |
|           |             |       |             | vni         | number | 24bits                                                                           |
|           |             | inner |             |             | object | overlay                                                                          |
|           |             |       | layer2      |             | object |                                                                                  |
|           |             |       |             | smac        | string |                                                                                  |
|           |             |       |             | dmac        | string |                                                                                  |
|           |             |       |             | ethertype   | number | 16bits                                                                           |
|           |             |       | ipv4        |             | object |                                                                                  |
|           |             |       |             | sip         | string |                                                                                  |
|           |             |       |             | dip         | string |                                                                                  |
|           |             |       |             | mflag       | number | 1bit                                                                             |
|           |             |       |             | proto       | number | 8bits                                                                            |
|           |             |       | ipv6        |             | object | should not co-exist with ipv4 field                                              |
|           |             |       |             | sip         | string |                                                                                  |
|           |             |       |             | dip         | string |                                                                                  |
|           |             |       |             | mflag       | number | 1bit                                                                             |
|           |             |       |             | next_header | number | 8bits                                                                            |
|           |             |       |             | flow_label  | number | 20bits                                                                           |
|           |             |       | tcp_udp     |             | object |                                                                                  |
|           |             |       |             | sport       | number | 16bits                                                                           |
|           |             |       |             | dport       | number | 16bits                                                                           |

4. Packet JSON file example

```json
{
    "packet_info": {
        "outer": {
            "ipv4": {
                "sip": "10.10.10.10",
                "dip": "3.3.3.3",
                "proto": 17
            },                  
            "layer2": {
                "smac": "24:8a:07:1e:82:ed",
                "dmac": "1c:34:da:1c:a1:00",
                "ethertype": 2048
            },                  
            "tcp_udp": {
                "sport": 100,
                "dport": 4789
            },
            "vxlan_nvgre": {
                "vni": 100
            }       
        },
        "inner": {
            "layer2": {
                "smac": "11:11:11:11:11:11",
                "dmac": "22:22:22:22:22:22",
                "ethertype": 2048
            },
            "ipv4": {
                "sip": "1.1.1.1",
                "dip": "2.2.2.3",
                "proto": 17,
                "mflag": 0
            },
            "tcp_udp": {
                "sport": 100,
                "dport": 200 
            }
        }       
    }
}