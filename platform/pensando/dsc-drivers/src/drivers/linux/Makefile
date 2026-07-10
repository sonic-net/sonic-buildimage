ifneq ($(KERNELRELEASE),)
obj-$(CONFIG_IONIC) += eth/ionic/
obj-$(CONFIG_IONIC_MNIC) += eth/ionic/
obj-$(CONFIG_MDEV) += mdev/
obj-$(CONFIG_MNET_UIO_PDRV_GENIRQ) += mnet_uio_pdrv_genirq/
else

IONIC_ETH_SRC = $(CURDIR)/eth/ionic

#KOPT += V=1		# verbose build
#KOPT += W=1		# extra warnings
#KOPT += C=1		# static analysis
#KOPT += CHECK=sparse	# static analysis tool
#KOPT += CHECK=scripts/coccicheck

default: all

# Discover kernel configuration.
#
# Override running kernel with
# `make KSRC=/path/to/your/sources` or
# `export KSRC=/path/to/your/sources`
#

ifeq ($(ARCH),aarch64)

# Ionic mnic and mdev for drivers ARM
#include ${MKINFRA}/config_${ARCH}.mk
KSRC ?= ${NICDIR}/buildroot/output/${ASIC}/linux-headers
KMOD_OUT_DIR ?= ${BLD_OUT_DIR}/drivers_submake
KMOD_SRC_DIR ?= ${TOPDIR}/platform/drivers/linux-ionic
ETH_KOPT += CONFIG_IONIC_MNIC=m
ETH_KOPT += CONFIG_MDEV=m
ETH_KOPT += CONFIG_MNET_UIO_PDRV_GENIRQ=m
KOPT += CROSS_COMPILE=aarch64-linux-gnu-
KOPT += ARCH=arm64
KCFLAGS += -DCONFIG_IONIC_MNIC
KCFLAGS += -DCONFIG_MDEV
KCFLAGS += -DCONFIG_MNET_UIO_PDRV_GENIRQ
ALL = mnic
ALL += mnet_uio_pdrv_genirq
ALL += mdev
export PATH := $(PATH):$(TOOLCHAIN_DIR)/bin

KSYMS_MNIC = $(KMOD_OUT_DIR)/Module.symvers.mnic
KSYMS_UIO = $(KMOD_OUT_DIR)/Module.symvers.uio

DVER = $(shell cd $(KMOD_SRC_DIR) ; git describe --tags 2>/dev/null)

else

DVER = $(shell git describe --tags 2>/dev/null)

# Ionic driver for host
include linux_ver.mk

KSRC ?= /lib/modules/$(shell uname -r)/build
ETH_KOPT += CONFIG_IONIC=m
ETH_KOPT += CONFIG_IONIC_MNIC=_
ETH_KOPT += CONFIG_MDEV=_
ETH_KOPT += CONFIG_MNET_UIO_PDRV_GENIRQ=_
KCFLAGS += -DCONFIG_IONIC

KCFLAGS = -Werror
KCFLAGS += $(EXTRA_CFLAGS)

ALL = eth

endif

ifeq ($(DVER),)
    DVER = "23.12.2-001"
endif
KCFLAGS += -Ddrv_ver=\\\"$(DVER)\\\"

KOPT += KCFLAGS="$(KCFLAGS)" KBUILD_EXTRA_SYMBOLS="$(KSYMS)"

all: $(ALL)

KBUILD_RULE = $(MAKE) -C $(KSRC) $(KOPT) M=$(CURDIR) KMOD_SRC_DIR=$(CURDIR)

mnic: KOPT+=$(ETH_KOPT)
mnic:
	@echo "===> Building MNIC driver "
	$(MAKE) -C $(KSRC) V=1 M=$(KMOD_SRC_DIR)/eth/ionic $(KOPT)
	mkdir -p $(KMOD_OUT_DIR)
	mv $(KMOD_SRC_DIR)/eth/ionic/Module.symvers \
           $(KMOD_OUT_DIR)/Module.symvers.mnic

mnet_uio_pdrv_genirq: KOPT+=$(ETH_KOPT)
mnet_uio_pdrv_genirq:
	@echo "===> Building MNET_UIO driver "
	$(MAKE) -C $(KSRC) V=1 M=$(KMOD_SRC_DIR)/mnet_uio_pdrv_genirq $(KOPT)
	#$(MAKE) -C $(KSRC) V=1 M=$(KMOD_OUT_DIR) src=$(KMOD_SRC_DIR)/mnet_uio_pdrv_genirq $(KOPT)
	mv ${KMOD_SRC_DIR}/mnet_uio_pdrv_genirq/Module.symvers ${KMOD_OUT_DIR}/Module.symvers.uio

mdev: KOPT+=$(ETH_KOPT)
mdev: KSYMS+=$(KSYMS_MNIC)
mdev: KSYMS+=$(KSYMS_UIO)
mdev:
	@echo "===> Building MDEV driver "
	#$(MAKE) -C $(KSRC) V=1 M=$(KMOD_SRC_DIR)/mdev KBUILD_OUTPUT=$(KMOD_OUT_DIR) $(KOPT)
	$(MAKE) -C $(KSRC) V=1 M=$(KMOD_SRC_DIR)/mdev $(KOPT)

eth: KOPT+=$(ETH_KOPT)
eth:
	@echo "===> Building ETH driver "
	+$(KBUILD_RULE)
	mv $(CURDIR)/Module.symvers $(CURDIR)/Module.symvers.ionic

clean: KOPT+=$(ETH_KOPT)
clean:
	$(KBUILD_RULE) clean
	rm -f $(CURDIR)/Module.symvers*

KBUILD_INSTALL_RULE = $(MAKE) -C $(KSRC) $(KOPT) \
		      $(if ${DISABLE_MODULE_SIGNING},CONFIG_MODULE_SIG=n) \
		      $(if ${DISABLE_MODULE_SIGNING},CONFIG_MODULE_SIG_ALL=) \
		      INSTALL_MOD_DIR=updates \
		      M=$(CURDIR)

install: modules_install
modules_install: KOPT+=$(ETH_KOPT)
modules_install:
	@$(call warn_signed_modules)
	$(KBUILD_INSTALL_RULE) modules_install
	$(call cmd_depmod)

cscope:
	find $(IONIC_ETH_SRC) -name '*.[ch]' > cscope.files
	cscope -bkq

.PHONY: default all mnic mdev mnet_uio_pdrv_genirq eth clean install modules_install cscope

endif
