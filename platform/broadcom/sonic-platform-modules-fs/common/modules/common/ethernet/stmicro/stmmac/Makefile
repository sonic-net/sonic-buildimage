# SPDX-License-Identifier: GPL-2.0
obj-$(CONFIG_STMMAC_ETH) += wb_stmmac.o
wb_stmmac-objs:= stmmac_main.o stmmac_ethtool.o stmmac_mdio.o ring_mode.o	\
	      chain_mode.o dwmac_lib.o dwmac1000_core.o dwmac1000_dma.o	\
	      dwmac100_core.o dwmac100_dma.o enh_desc.o norm_desc.o	\
	      mmc_core.o stmmac_hwtstamp.o stmmac_ptp.o dwmac4_descs.o	\
	      dwmac4_dma.o dwmac4_lib.o dwmac4_core.o dwmac5.o hwif.o \
	      stmmac_tc.o dwxgmac2_core.o dwxgmac2_dma.o dwxgmac2_descs.o

# Ordering matters. Generic driver must be last.
obj-m += wb_stmmac_platform.o
obj-m += wb_dwmac_generic.o
wb_stmmac_platform-objs:= stmmac_platform.o
wb_dwmac_generic-objs := dwmac-generic.o
