# SPDX-License-Identifier: GPL-2.0
#
# Makefile for CN8140 (Rudra40) Platform-Specific Modules
#
# Builds Rudra40 CFPGA modules and Sutra system controller modules.
# Common modules are built separately from ../../common/
#

# Kernel build environment
KERNEL_SRC ?= /lib/modules/$(shell uname -r)/build

# ===================================================================
# Common include paths and flags (shared by all modules in this dir)
# ===================================================================
EXTRA_CFLAGS += -DNO_IRQ=0
EXTRA_CFLAGS += -I$(src)
EXTRA_CFLAGS += -I$(src)/../include/linux
EXTRA_CFLAGS += -I$(src)/../include
EXTRA_CFLAGS += -I$(src)/../..
EXTRA_CFLAGS += -I$(src)/../../common
EXTRA_CFLAGS += -include $(src)/../../common/kcompat.h
EXTRA_CFLAGS += -Wall -Wno-unused-parameter

# ===================================================================
# Common PCI IDs
# ===================================================================
PCI_VENDOR_ID_CIENA := 0x16fc
PCI_DEVICE_ID_CIENA_RUDRA40 := 0x033b

PCI_FLAGS := -DPCI_VENDOR_ID_CIENA=$(PCI_VENDOR_ID_CIENA)

# ===================================================================
# Rudra40 CFPGA modules (PCI device 16fc:033b)
# ===================================================================
RUDRA40_FLAGS := $(PCI_FLAGS)
RUDRA40_FLAGS += -DPCI_DEVICE_ID_CIENA_RUDRA40=$(PCI_DEVICE_ID_CIENA_RUDRA40)
RUDRA40_FLAGS += -DUIO_NAME='"UIO Ciena Rudra40 FPGA"'
RUDRA40_FLAGS += -DCIC_NAME='"rudra40-cic-dev"'
RUDRA40_FLAGS += -DGPIO_NAME='"rudra40-gpio-dev"'
RUDRA40_FLAGS += -DGNSS_NAME='"sirilx_ttyGNSS"'
RUDRA40_FLAGS += -DCIENA_SIRIL_BAR=0
RUDRA40_FLAGS += -DMOD_NAME='"rudra40"'
RUDRA40_FLAGS += -DCIENA_CIC_REG_WIDTH=32
RUDRA40_FLAGS += -DCIENA_CIC_GET_MASTER_SUM='rudra40_get_master_sum'
RUDRA40_FLAGS += -DCIENA_CIC_GET_MASTER_MASK='rudra40_get_master_mask'
RUDRA40_FLAGS += -DCIENA_CIC_GET_INTERRUPT_TABLE='rudra40_get_interrupt_table'
RUDRA40_FLAGS += -DCIENA_CIC_GET_MSI_CTRL='rudra40_get_msi_ctrl'
RUDRA40_FLAGS += -DCIENA_CIC_PIN_TO_STR='rudra40_cic_pin_to_str'
RUDRA40_FLAGS += -DCIENA_GPIO_EXPORT_FUNC='rudra40_gpio_export'
RUDRA40_FLAGS += -DCIENA_GPIO_UNEXPORT_FUNC='rudra40_gpio_unexport'
RUDRA40_FLAGS += -DCIENA_REGMAP_CONFIG='rudra40_regmap_config'
RUDRA40_FLAGS += -DCIENA_FPGA_INFO='rudra40_fpga_id_read'
RUDRA40_FLAGS += -DCIENA_RUDRA40
RUDRA40_FLAGS += -DMOD_NAMECAP='"RUDRA40"'
RUDRA40_FLAGS += -DCIENA_CIC_GPIO_LKUP='rudra40_cic_add_gpio_lkup'
RUDRA40_FLAGS += -DCIENA_FPGA_FUNCTIONS='"cfpga,tfpga"'
RUDRA40_FLAGS += -DSIRIL_PRIV_MOD_INC='"rudra40_priv.h"'

# Per-object CFLAGS for Rudra40 modules
CFLAGS_rudra40_cic.o      := $(RUDRA40_FLAGS)
CFLAGS_rudra40_cic_utils.o := $(RUDRA40_FLAGS)
CFLAGS_sirilx_gpio.o      := $(RUDRA40_FLAGS)
CFLAGS_sirilx_uio.o       := $(RUDRA40_FLAGS)
CFLAGS_rudra40_qsfp_ctrl.o := $(RUDRA40_FLAGS)

obj-m += rudra40_cic.o
obj-m += sirilx_gpio.o
obj-m += sirilx_uio.o
obj-m += rudra40_qsfp_ctrl.o

# ===================================================================
# Sutra system controller modules (I2C FPGA on ACPI i2c-PRP0001:00)
# ===================================================================
SUTRA_FLAGS := -DUIO_NAME='"UIO Ciena Sutra FPGA"'
SUTRA_FLAGS += -DCIC_NAME='"sutra-cic-dev"'
SUTRA_FLAGS += -DGPIO_NAME='"sutra-gpio-dev"'
SUTRA_FLAGS += -DGNSS_NAME='"sirilx_ttyGNSS"'
SUTRA_FLAGS += -DCIENA_SIRIL_BAR=0
SUTRA_FLAGS += -DMOD_NAME='"sutra"'
SUTRA_FLAGS += -DCIENA_CIC_REG_WIDTH=8
SUTRA_FLAGS += -DCIENA_CIC_GET_MASTER_SUM='sutra_get_master_sum'
SUTRA_FLAGS += -DCIENA_CIC_GET_MASTER_MASK='sutra_get_master_mask'
SUTRA_FLAGS += -DCIENA_CIC_GET_INTERRUPT_TABLE='sutra_get_interrupt_table'
SUTRA_FLAGS += -DCIENA_CIC_GET_MSI_CTRL='sutra_get_msi_ctrl'
SUTRA_FLAGS += -DCIENA_CIC_PIN_TO_STR='sutra_cic_pin_to_str'
SUTRA_FLAGS += -DCIENA_GPIO_EXPORT_FUNC='sutra_gpio_export'
SUTRA_FLAGS += -DCIENA_GPIO_UNEXPORT_FUNC='sutra_gpio_unexport'
SUTRA_FLAGS += -DCIENA_REGMAP_CONFIG='sutra_regmap_config'
SUTRA_FLAGS += -DCIENA_FPGA_INFO='sutra_fpga_id_read'
SUTRA_FLAGS += -DCIENA_SUTRA
SUTRA_FLAGS += -DMOD_NAMECAP='"SUTRA"'
SUTRA_FLAGS += -DCIENA_CIC_GPIO_LKUP='sutra_cic_add_gpio_lkup'
SUTRA_FLAGS += -DCIENA_FPGA_FUNCTIONS='"bfpga"'
SUTRA_FLAGS += -DSIRIL_PRIV_MOD_INC='"sutra_priv.h"'

# Per-object CFLAGS for Sutra modules
CFLAGS_bfpga_i2c.o        := $(SUTRA_FLAGS)
CFLAGS_sutra_cic.o         := $(SUTRA_FLAGS)
CFLAGS_sutra_cic_utils.o   := $(SUTRA_FLAGS)
CFLAGS_sutra_sirilx_gpio.o := $(SUTRA_FLAGS)

# sutra_i2c module (I2C client driver for Sutra FPGA)
obj-m += sutra_i2c.o
sutra_i2c-objs := bfpga_i2c.o

# sutra-cic module (interrupt controller)
obj-m += sutra-cic.o
sutra-cic-objs := sutra_cic.o sutra_cic_utils.o

# sutra-gpio module (GPIO controller)
# Uses same sirilx_gpio.c but with sutra defines — compiled as separate object
obj-m += sutra-gpio.o
sutra-gpio-objs := sutra_sirilx_gpio.o

# ===================================================================
# BMC8140 OpenBMC FPGA modules (PCI device 16fc:0340)
# ===================================================================
BMC8140_FLAGS := $(PCI_FLAGS)
BMC8140_FLAGS += -DPCI_DEVICE_ID_CIENA_OBMC_8140=0x0340
BMC8140_FLAGS += -DUIO_NAME='"UIO Ciena BMC8140 FPGA"'
BMC8140_FLAGS += -DCIC_NAME='"bmc8140-cic-dev"'
BMC8140_FLAGS += -DGPIO_NAME='"bmc8140-gpio-dev"'
BMC8140_FLAGS += -DGNSS_NAME='"sirilx_ttyGNSS"'
BMC8140_FLAGS += -DCIENA_SIRIL_BAR=0
BMC8140_FLAGS += -DMOD_NAME='"bmc8140"'
BMC8140_FLAGS += -DCIENA_CIC_REG_WIDTH=32
BMC8140_FLAGS += -DCIENA_CIC_GET_MASTER_SUM='bmc8140_get_master_sum'
BMC8140_FLAGS += -DCIENA_CIC_GET_MASTER_MASK='bmc8140_get_master_mask'
BMC8140_FLAGS += -DCIENA_CIC_GET_INTERRUPT_TABLE='bmc8140_get_interrupt_table'
BMC8140_FLAGS += -DCIENA_CIC_GET_MSI_CTRL='bmc8140_get_msi_ctrl'
BMC8140_FLAGS += -DCIENA_CIC_PIN_TO_STR='bmc8140_cic_pin_to_str'
BMC8140_FLAGS += -DCIENA_GPIO_EXPORT_FUNC='bmc8140_gpio_export'
BMC8140_FLAGS += -DCIENA_GPIO_UNEXPORT_FUNC='bmc8140_gpio_unexport'
BMC8140_FLAGS += -DCIENA_REGMAP_CONFIG='bmc8140_regmap_config'
BMC8140_FLAGS += -DCIENA_FPGA_INFO='bmc8140_fpga_id_read'
BMC8140_FLAGS += -DCIENA_BMC8140
BMC8140_FLAGS += -DMOD_NAMECAP='"BMC8140"'
BMC8140_FLAGS += -DCIENA_CIC_GPIO_LKUP='bmc8140_cic_add_gpio_lkup'
BMC8140_FLAGS += -DCIENA_FPGA_FUNCTIONS='"obmc"'
BMC8140_FLAGS += -DSIRIL_PRIV_MOD_INC='"bmc8140_priv.h"'

# Per-object CFLAGS for BMC8140 modules
CFLAGS_bmc8140_cic.o       := $(BMC8140_FLAGS)
CFLAGS_bmc8140_cic_utils.o := $(BMC8140_FLAGS)
CFLAGS_bmc8140_sirilx_gpio.o := $(BMC8140_FLAGS)
CFLAGS_bmc8140_uio.o       := $(BMC8140_FLAGS)

# bmc8140-cic module (interrupt controller)
obj-m += bmc8140-cic.o
bmc8140-cic-objs := bmc8140_cic.o bmc8140_cic_utils.o

# bmc8140-gpio module (GPIO controller)
# Uses same sirilx_gpio.c but with BMC8140 defines — compiled as separate object
obj-m += bmc8140-gpio.o
bmc8140-gpio-objs := bmc8140_sirilx_gpio.o

# bmc8140_uio module (PCI/UIO driver - creates thermal, fan, chardev sub-devices)
obj-m += bmc8140_uio.o

# ===================================================================
# Build infrastructure
# ===================================================================
KBUILD_EXTRA_SYMBOLS += $(src)/../../common/Module.symvers

# ===================================================================
# plreg utility generation
# ===================================================================
GENERATE_PLREG := $(CURDIR)/../../generate_plreg.py
REGMAP_HTML := $(wildcard $(CURDIR)/../regmap/*_regmap.html)
PLREG_OUTPUT := $(CURDIR)/../utils/plreg

# ===================================================================
# Targets (all: MUST be first to be the default target)
# ===================================================================
all:
	$(MAKE) -C $(KERNEL_SRC) M=$(shell pwd) modules
	@if [ -n "$(REGMAP_HTML)" ]; then $(MAKE) plreg; fi

plreg: $(PLREG_OUTPUT)

$(PLREG_OUTPUT): $(REGMAP_HTML) $(GENERATE_PLREG)
	@mkdir -p $(dir $@)
	python3 $(GENERATE_PLREG) --device-id $(PCI_DEVICE_ID_CIENA_RUDRA40) $(REGMAP_HTML) $@

clean:
	$(MAKE) -C $(KERNEL_SRC) M=$(shell pwd) clean
	rm -f $(PLREG_OUTPUT)

install:
	$(MAKE) -C $(KERNEL_SRC) M=$(shell pwd) modules_install

help:
	@echo "CN8140 (Rudra40) Platform Modules"
	@echo "=================================="
	@echo "Targets:"
	@echo "  all     - Build all CN8140 kernel modules and plreg utility"
	@echo "  plreg   - Generate plreg register tool from regmap HTML"
	@echo "  clean   - Clean build artifacts"
	@echo "  install - Install modules to system"
	@echo ""
	@echo "Modules built:"
	@echo "  rudra40_cic.ko       - Rudra40 CFPGA interrupt controller"
	@echo "  sirilx_gpio.ko       - Rudra40 GPIO"
	@echo "  sirilx_uio.ko        - Rudra40 UIO/PCI driver"
	@echo "  rudra40_qsfp_ctrl.ko - Rudra40 QSFP control"
	@echo "  sutra_i2c.ko         - Sutra system controller I2C driver"
	@echo "  sutra-cic.ko         - Sutra interrupt controller"
	@echo "  sutra-gpio.ko        - Sutra GPIO"

.PHONY: all clean install help plreg
