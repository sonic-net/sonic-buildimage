# NXP-LX2160ARDB-VS

SONiC on the NXP LX2160ARDB with the switch ASIC emulated by SAI-VS.

| SONiC port | Linux interface | DPAA2 | Physical port |
|---|---|---|---|
| Ethernet0  | dpmac17 | dpni.0 -> dpmac17 | RGMII 1G (AR8035) |
| Ethernet4  | dpmac18 | dpni.2 -> dpmac18 | RGMII 1G (AR8035) |
| Ethernet8  | dpmac3  | dpni.3 -> dpmac3  | 10GBASE-T (AQR107) |
| Ethernet12 | dpmac4  | dpni.4 -> dpmac4  | 10GBASE-T (AQR107) |
| eth0 (mgmt) | eth0   | dpni.1 -> dpmac2  | 40G QSFP, uncabled placeholder |

The Linux names come from the udev rule in the sonic-platform-nxp-lx2160ardb
package. The DPNI layout comes from the board's DPL (DPAA2 data path layout,
in NOR at 0xd00000), which must create dpni.0-4 connected to dpmac17,
dpmac2, dpmac18, dpmac3 and dpmac4. An example DPL:
https://github.com/wrouwet/lx2160-sonic/tree/main/firmware/dpl Traffic on the front-panel ports is real; SAI-VS forwards it
between each DPAA2 interface and its SONiC EthernetN tap device.
