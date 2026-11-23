# SPDX-License-Identifier: GPL-2.0
obj-$(CONFIG_AMD_XGBE) += wb_amd_xgbe.o

wb_amd_xgbe-objs := xgbe-main.o xgbe-drv.o xgbe-dev.o \
		 xgbe-desc.o xgbe-ethtool.o xgbe-mdio.o \
		 xgbe-ptp.o \
		 xgbe-i2c.o xgbe-phy-v1.o xgbe-phy-v2.o \
		 xgbe-platform.o

wb_amd_xgbe-$(CONFIG_PCI) += xgbe-pci.o
wb_amd_xgbe-$(CONFIG_AMD_XGBE_DCB) += xgbe-dcb.o
wb_amd_xgbe-$(CONFIG_DEBUG_FS) += xgbe-debugfs.o
