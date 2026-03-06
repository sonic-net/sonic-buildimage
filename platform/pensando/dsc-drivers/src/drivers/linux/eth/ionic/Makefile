# SPDX-License-Identifier: GPL-2.0
# Copyright(c) 2017 - 2019 Pensando Systems, Inc

obj-$(CONFIG_IONIC) := ionic.o
obj-$(CONFIG_IONIC_MNIC) := ionic_mnic.o

ccflags-y := -g -I$(M)/../../common  -I$(M)/../../../common -I$(src)

ionic-y := ionic_main.o ionic_bus_pci.o ionic_dev.o ionic_ethtool.o \
	   ionic_lif.o ionic_rx_filter.o ionic_txrx.o ionic_debugfs.o \
	   ionic_api.o ionic_stats.o ionic_devlink.o kcompat.o ionic_fw.o \
	   dim.o net_dim.o
ionic-$(CONFIG_PTP_1588_CLOCK) += ionic_phc.o

ionic_mnic-y := ionic_main.o ionic_bus_platform.o ionic_dev.o ionic_ethtool.o \
	        ionic_lif.o ionic_rx_filter.o ionic_txrx.o ionic_debugfs.o \
	        ionic_api.o ionic_stats.o ionic_devlink.o kcompat.o ionic_fw.o \
		dim.o net_dim.o
ionic_mnic-$(CONFIG_PTP_1588_CLOCK) += ionic_phc.o ionic_phc_weak.o
