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
KSRC ?= ${NICDIR}/buildroot/output/${ASIC}/linux-headers
KMOD_OUT_DIR ?= ${BLD_OUT_DIR}/drivers_submake
KMOD_SRC_DIR ?= ${TOPDIR}/platform/drivers/linux-ionic
ETH_KOPT += CONFIG_IONIC_MNIC=m
ETH_KOPT += CONFIG_MDEV=m
ETH_KOPT += CONFIG_MNET_UIO_PDRV_GENIRQ=m
KOPT += ARCH=arm64
KCFLAGS += -DCONFIG_IONIC_MNIC
KCFLAGS += -DCONFIG_MDEV
KCFLAGS += -DCONFIG_MNET_UIO_PDRV_GENIRQ
ALL = mnic
ALL += mnet_uio_pdrv_genirq
ALL += mdev
export PATH := $(PATH):$(TOOLCHAIN_DIR)/bin

KSYMS_MNIC = "KBUILD_EXTRA_SYMBOLS=${KMOD_OUT_DIR}/Module.symvers.mnic"
KSYMS = "${KSYMS_MNIC} ${KMOD_OUT_DIR}/Module.symvers.uio"


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
    DVER = "22.11.1-001"
endif
KCFLAGS += -Ddrv_ver=\\\"$(DVER)\\\"

KOPT += KCFLAGS="$(KCFLAGS)"

all: $(ALL)

KBUILD_RULE = $(MAKE) -C $(KSRC) $(KOPT) M=$(CURDIR)

mnic: KOPT+=$(ETH_KOPT)
mnic:
	@echo "===> Building MNIC driver "
	touch $(KMOD_OUT_DIR)/Makefile || true
	$(MAKE) -C $(KSRC) V=1 M=$(KMOD_OUT_DIR) src=$(KMOD_SRC_DIR)/eth/ionic $(KOPT)
	mv ${KMOD_OUT_DIR}/Module.symvers ${KMOD_OUT_DIR}/Module.symvers.mnic

mnet_uio_pdrv_genirq: KOPT+=$(ETH_KOPT)
mnet_uio_pdrv_genirq:
	@echo "===> Building MNET_UIO driver "
	$(MAKE) -C $(KSRC) V=1 M=$(KMOD_OUT_DIR) src=$(KMOD_SRC_DIR)/mnet_uio_pdrv_genirq $(KOPT)
	mv ${KMOD_OUT_DIR}/Module.symvers ${KMOD_OUT_DIR}/Module.symvers.uio

mdev: KOPT+=$(ETH_KOPT)
mdev:
	@echo "===> Building MDEV driver "
	$(MAKE) -C $(KSRC) $(KSYMS) V=1 M=$(KMOD_OUT_DIR) src=$(KMOD_SRC_DIR)/mdev $(KOPT)

eth: KOPT+=$(ETH_KOPT)
eth:
	$(KBUILD_RULE)

clean: KOPT+=$(ETH_KOPT)
clean:
	$(KBUILD_RULE) clean

install: modules_install
modules_install: KOPT+=$(ETH_KOPT)
modules_install:
	$(KBUILD_RULE) modules_install

cscope:
	find $(IONIC_ETH_SRC) -name '*.[ch]' > cscope.files
	cscope -bkq

.PHONY: default all mnic mdev mnet_uio_pdrv_genirq eth clean install modules_install cscope

endif
