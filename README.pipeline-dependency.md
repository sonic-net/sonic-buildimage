# SONiC Submodule Pipeline Dependency Graph

This document describes the artifact download dependencies between submodule CI pipelines in sonic-buildimage.

## Pipeline ID Mapping

### Artifact Source Pipelines

These pipelines produce artifacts that other submodule pipelines download:

| Pipeline ID / Alias                          | Description                                       | Artifact Examples                        |
|----------------------------------------------|---------------------------------------------------|------------------------------------------|
| `Azure.sonic-buildimage.common_libs`         | sonic-buildimage common libs                      | libyang, libnl, libpcre debs             |
| `Azure.sonic-buildimage.official.vs`         | sonic-buildimage VS image                         | all debs + python wheels                 |
| `Azure.sonic-swss-common`                    | sonic-swss-common                                 | libswsscommon, python3-swsscommon debs    |
| `Azure.sonic-sairedis`                       | sonic-sairedis                                    | libsairedis, libsaimetadata debs         |
| `sonic-net.sonic-dash-api`                   | sonic-dash-api                                    | libdashapi debs                          |
| `sonic-net.sonic-platform-vpp`               | sonic-platform-vpp                                | libvppinfra-dev, vpp debs                |

### Consumer-Only Pipelines

These submodule pipelines download artifacts from the sources above but are not themselves referenced by other pipelines:

| Pipeline                    | Submodule Repo              |
|-----------------------------|-----------------------------|
| `Azure.sonic-swss`          | sonic-swss                  |
| `Azure.sonic-gnmi`          | sonic-gnmi                  |
| `Azure.sonic-utilities`     | sonic-utilities             |
| `Azure.sonic-bmp`           | sonic-bmp                   |
| `Azure.sonic-dash-ha`       | sonic-dash-ha               |
| `Azure.linkmgrd`            | linkmgrd                    |
| `Azure.dhcpmon`             | dhcpmon                     |
| `Azure.dhcprelay`           | dhcprelay                   |
| `Azure.sonic-stp`           | sonic-stp                   |
| `Azure.sonic-wpa-supplicant`| wpasupplicant               |
| `Azure.sonic-host-services` | sonic-host-services         |
| `Azure.sonic-mgmt-common`   | sonic-mgmt-common           |
| `Azure.sonic-mgmt-framework`| sonic-mgmt-framework        |
| `Azure.sonic-platform-common`| sonic-platform-common      |
| `Azure.sonic-platform-daemons`| sonic-platform-daemons    |
| `Azure.sonic-snmpagent`     | sonic-snmpagent             |
| `Azure.sonic-dbsyncd`       | sonic-dbsyncd               |

### Pipelines With No Artifact Downloads (No Upstream Dependencies)

These submodules have pipeline definitions but do not download artifacts from other pipelines (they may still be artifact sources for other pipelines):

| Submodule             |
|-----------------------|
| sonic-py-swsssdk      |
| sonic-linux-kernel    |
| sonic-ztp             |
| sonic-dash-api        |

## Dependency Graph

```mermaid
graph TD
    CL["sonic-buildimage<br/>common_libs"]
    VS["sonic-buildimage<br/>official.vs"]
    SWC["sonic-swss-common"]
    SAI["sonic-sairedis"]
    SWSS["sonic-swss"]
    GNMI["sonic-gnmi"]
    UTIL["sonic-utilities"]
    BMP["sonic-bmp"]
    DHA["sonic-dash-ha"]
    LMG["linkmgrd"]
    DHM["dhcpmon"]
    DHR["dhcprelay"]
    STP["sonic-stp"]
    WPA["sonic-wpa-supplicant"]
    HSV["sonic-host-services"]
    MGC["sonic-mgmt-common"]
    MGF["sonic-mgmt-framework"]
    PLC["sonic-platform-common"]
    PLD["sonic-platform-daemons"]
    SNM["sonic-snmpagent"]
    DBS["sonic-dbsyncd"]
    DASH["sonic-dash-api"]
    VPP["sonic-platform-vpp"]

    CL --> SWC
    CL --> SAI
    CL --> GNMI
    CL --> LMG
    CL --> DHM
    CL --> DHR
    CL --> STP
    CL --> WPA
    CL --> DHA
    CL --> SWSS
    CL --> VPP

    VS --> SWC
    VS --> GNMI
    VS --> BMP
    VS --> UTIL
    VS --> HSV
    VS --> MGC
    VS --> MGF
    VS --> PLC
    VS --> PLD
    VS --> SNM
    VS --> DBS

    SWC --> SAI
    SWC --> SWSS
    SWC --> GNMI
    SWC --> UTIL
    SWC --> BMP
    SWC --> DHA
    SWC --> LMG
    SWC --> DHM
    SWC --> DHR
    SWC --> STP
    SWC --> WPA

    SAI --> SWSS
    DASH --> SWSS
    DASH --> UTIL
    VPP --> SAI
    VPP --> SWSS
```

> Arrows mean **"provides artifacts to"** — e.g., `CL --> SWC` means sonic-swss-common downloads from common_libs.

## Build Order (Topological)

```
Level 0 (pure producers - build from source, no pipeline artifact deps):
  ├── common_libs
  ├── sonic-buildimage.official.vs
  └── sonic-dash-api

Level 1 (need only Level-0 artifacts):
  ├── platform-vpp              ← common_libs (libnl)
  ├── sonic-swss-common         ← common_libs + official.vs + dash-api
  └── official.vs-only group    ← official.vs (+ dash-api where noted):
      sonic-host-services, sonic-mgmt-common, sonic-mgmt-framework,
      sonic-platform-common, sonic-platform-daemons,
      sonic-snmpagent, sonic-dbsyncd

Level 2 (need swss-common and/or vpp):
  ├── sonic-sairedis       ← common_libs + swss-common + official.vs + dash-api (+ platform-vpp)
  ├── sonic-gnmi           ← common_libs + official.vs + swss-common
  ├── sonic-bmp            ← swss-common + official.vs
  ├── sonic-utilities      ← official.vs + swss-common + dash-api
  └── sonic-dash-ha, linkmgrd, dhcpmon, dhcprelay,
      sonic-stp, sonic-wpa-supplicant   ← common_libs + swss-common

Level 3:
  └── sonic-swss           ← swss-common + sairedis + common_libs + dash-api (+ platform-vpp)
```

## Detailed Dependency Table

| Submodule Pipeline         | Downloads Artifacts From                                                        |
|----------------------------|---------------------------------------------------------------------------------|
| **sonic-swss-common**      | `common_libs` (libyang) + `buildimage.vs` (yang wheels)                    |
| **sonic-sairedis**         | `common_libs` (libyang, libnl) + `sonic-swss-common`  + `sonic-platform-vpp`|
| **sonic-swss**             | `sonic-swss-common` + `sonic-sairedis` + `common_libs` + `sonic-dash-api` + `sonic-platform-vpp` |
| **sonic-gnmi**             | `common_libs` + `buildimage.vs`  + `sonic-swss-common`                 |
| **sonic-utilities**        | `buildimage.vs`  + `sonic-swss-common`  + `sonic-dash-api`             |
| **sonic-bmp**              | `buildimage.vs` (libyang/libnl) + `sonic-swss-common`                     |
| **sonic-dash-ha**          | `common_libs` (libnl) + `sonic-swss-common`                                    |
| **linkmgrd**               | `common_libs` (libyang) + `sonic-swss-common`                               |
| **dhcpmon**                | `common_libs` (libyang/libnl) + `sonic-swss-common`                            |
| **dhcprelay**              | `common_libs` (libyang) + `sonic-swss-common`                               |
| **sonic-stp**              | `sonic-swss-common`  + `common_libs` (465, libyang/libnl)                   |
| **sonic-wpa-supplicant**   | `common_libs` (libyang) + `sonic-swss-common`                               |
| **sonic-host-services**    | `buildimage.vs`                                                            |
| **sonic-mgmt-common**      | `buildimage.vs`                                                            |
| **sonic-mgmt-framework**   | `buildimage.vs`                                                            |
| **sonic-platform-common**  | `buildimage.vs`                                                            |
| **sonic-platform-daemons** | `buildimage.vs`                                                            |
| **sonic-snmpagent**        | `buildimage.vs`                                                            |
| **sonic-dbsyncd**          | `buildimage.vs`                                                            |
