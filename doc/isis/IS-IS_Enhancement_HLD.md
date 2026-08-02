# SONiC IS-IS Protocol Integration and Enhancement HLD

## 1. Requirement & Scope

IS-IS (Intermediate System to Intermediate System) is a link-state Interior Gateway Protocol (IGP) widely used in enterprise, service provider, and data center networks. This High-Level Design (HLD) document specifies the integration, management framework architecture, and dual-stack enhancements of the IS-IS routing protocol into the SONiC ecosystem.

### Key Objectives
- **Daemon Lifecycle Management**: Automatic activation and supervisord monitoring of the FRR `isisd` daemon inside `docker-fpm-frr`.
- **ConfigDB Control Plane Synchronization**: Real-time configuration translation via `frrcfgd.py` from Redis ConfigDB (DB Index 4) to FRR `vtysh`.
- **IPv6 Dual-Stack & Multi-Topology Support**: Automated multi-topology routing (`topology ipv6-unicast`) and interface-level address family bindings (`ipv6 router isis`).
- **Passive Interface Support**: Enabling `isis passive` mode on interfaces (such as Loopback) to advertise connected prefixes without initiating neighbor adjacencies.
- **YANG Model Validation**: Full schema enforcement using `sonic-isis-global.yang` and `sonic-isis-interface.yang`.
- **Host-Level Click CLI Suite**: Providing user-facing commands (`config isis`, `config interface isis`, `show isis`).

---

## 2. Architecture & Data Flow

Configuration is populated into Redis ConfigDB (Index 4) via CLI, JSON, or gNMI, translated dynamically by `frrcfgd.py`, and applied to the `isisd` daemon through `vtysh`:

```
+-------------------------------------------------+
|   Click CLI / JSON Import / gNMI Management     |
+-------------------------------------------------+
                        |
                        v
+-------------------------------------------------+
|         Redis ConfigDB (Index 4 Tables)         |
|   - ISIS_GLOBALS|default                        |
|   - ISIS_INTERFACE|<ifname>                     |
|   - ISIS_GLOBALS_TIMERS|default                 |
+-------------------------------------------------+
                        |
                        v
+-------------------------------------------------+
|             frrcfgd.py (Config Daemon)          |
|  (Parses tables, evaluates maps, builds vtysh)  |
+-------------------------------------------------+
                        |
                        v
+-------------------------------------------------+
|                  vtysh CLI                      |
|  (router isis / topology / isis passive)        |
+-------------------------------------------------+
                        |
                        v
+-------------------------------------------------+
|             FRR isisd Daemon Container          |
+-------------------------------------------------+
```

---

## 3. ConfigDB Table Definitions & Schema

### 3.1 `ISIS_GLOBALS` Table
Key: `ISIS_GLOBALS|vrf_name` (e.g., `ISIS_GLOBALS|default`)

| Field Name | Type | Allowed Values | Description |
| :--- | :--- | :--- | :--- |
| `net_title` | String | ISO NET (e.g., `49.0001.0000.0000.0001.00`) | Network Entity Title for IS-IS process |
| `level` | String | `level-1`, `level-2`, `level-1-2` | IS-IS router type |
| `dynamic_hostname` | String | `true`, `false` | Enable/disable dynamic hostname resolution in LSPs |

### 3.2 `ISIS_INTERFACE` Table
Key: `ISIS_INTERFACE|interface_name` (e.g., `ISIS_INTERFACE|Ethernet0`, `ISIS_INTERFACE|Loopback0`)

| Field Name | Type | Allowed Values | Description |
| :--- | :--- | :--- | :--- |
| `circuit_type` | String | `p2p`, `lan` | IS-IS link circuit style |
| `metric` | String | `1..16777215` | Interface link cost metric |
| `passive` | String | `true`, `false` | Enable passive interface mode |

---

## 4. YANG Model Specification

### 4.1 Global Model (`sonic-isis-global.yang`)
Defines the `ISIS_GLOBALS` and `ISIS_GLOBALS_TIMERS` tables with NET title regex constraints and level enumerations.

### 4.2 Interface Model (`sonic-isis-interface.yang`)
Defines `ISIS_INTERFACE` schema with passive interface attribute support:
```yang
leaf passive {
    type boolean;
    description "Enable passive mode on this interface for IS-IS.";
}
```

---

## 5. FRR Management Translation (`frrcfgd.py`)

1. **Global Configuration Handler**:
   When `ISIS_GLOBALS|default` is updated, `frrcfgd.py` constructs prefix commands:
   ```text
   configure terminal
   router isis default
   topology ipv6-unicast
   ```
   And executes KeyMaps for `net_title`, `is-type`, and `hostname`.

2. **Interface Configuration Handler**:
   When `ISIS_INTERFACE|<ifname>` is updated, `frrcfgd.py` constructs prefix commands:
   ```text
   configure terminal
   interface <ifname>
   ip router isis default
   ipv6 router isis default
   ```
   And executes KeyMaps for `isis metric`, `isis network point-to-point`, and `isis passive`.

---

## 6. CLI Command Reference

### Config Commands (`sonic-utilities`)
- `config isis net <net_title>`: Configure ISO NET address.
- `config isis level <level-1|level-2|level-1-2>`: Configure router level.
- `config isis hostname <enable|disable>`: Enable/disable dynamic hostname.
- `config interface isis enable <ifname>`: Enable IS-IS on an interface.
- `config interface isis disable <ifname>`: Disable IS-IS on an interface.
- `config interface isis passive <ifname> <enable|disable>`: Configure passive interface mode.
- `config interface isis circuit-type <ifname> <p2p|lan>`: Configure P2P/LAN network type.
- `config interface isis metric <ifname> <1-16777215>`: Configure link cost metric.

### Show Commands (`sonic-utilities`)
- `show isis neighbor [--json]`: Display IS-IS neighbor adjacencies.
- `show isis database [detail] [--json]`: Display Link-State Database (LSDB).
- `show isis summary [--json]`: Display IS-IS daemon runtime summary.

---

## 7. Multi-Node Verification & Test Results

The implementation was validated on a live 3-node GNS3 triangle topology (`SONiC-1 <-> SONiC-2 <-> SONiC-3`):

1. **Neighbor Adjacencies**: Established point-to-point Level-1/Level-2 neighbors on physical interfaces `Ethernet0` and `Ethernet4` (`show isis neighbor`).
2. **Dual-Stack Passive Reachability**:
   - IPv4 Loopback0 passive interface ping (`2.2.2.2`, `3.3.3.3`): **0% packet loss**.
   - IPv6 Loopback0 passive interface ping (`2001:db8:2::1`, `2001:db8:3::1`): **0% packet loss**.
3. **ECMP Route Calculation**: Verified dual-path ECMP routes for inter-switch subnets (`2001:db8:23::/64`) across `Ethernet0` and `Ethernet4`.
