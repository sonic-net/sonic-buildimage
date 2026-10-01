# Required eMMC DMA fix patches for the RD-AC5X (internal-CPU) board with
# >2GB DRAM -- without them the board's eMMC enumerates as 0 B and it never
# finds its root filesystem at boot. See non-upstream-patches/README.md.
# Default to "y" (unlike mellanox's opt-in use of this same mechanism for
# truly optional extras) since these are mandatory hardware fixes for this
# platform. Plain assignment (not "?=") so it wins over rules/linux-kernel.mk's
# "?= n" default regardless of makefile include order, while still being a
# normal (non-override) variable a real command-line/SONIC_OVERRIDE_BUILD_VARS
# INCLUDE_EXTERNAL_PATCHES=n can turn back off if ever needed.
INCLUDE_EXTERNAL_PATCHES = y
override EXTERNAL_KERNEL_PATCH_LOC := $(BUILD_WORKDIR)/$(PLATFORM_PATH)/non-upstream-patches/

include $(PLATFORM_PATH)/sai.mk
include $(PLATFORM_PATH)/docker-syncd-mrvl-prestera.mk
include $(PLATFORM_PATH)/docker-syncd-mrvl-prestera-rpc.mk
include $(PLATFORM_PATH)/docker-saiserver-mrvl-prestera.mk
include $(PLATFORM_PATH)/libsaithrift-dev.mk
include $(PLATFORM_PATH)/one-image.mk
include $(PLATFORM_PATH)/platform-marvell.mk
ifeq ($(CONFIGURED_ARCH),$(filter $(CONFIGURED_ARCH),arm64 armhf))
include $(PLATFORM_PATH)/mrvl-prestera.mk
include $(PLATFORM_PATH)/platform-nokia.mk
endif

SONIC_ALL += $(SONIC_ONE_IMAGE) \
             $(DOCKER_FPM) 	\
             $(DOCKER_SYNCD_MRVL_RPC)

# Inject mrvl-prestera sai into syncd
$(SYNCD)_DEPENDS += $(MRVL_SAI)
$(SYNCD)_UNINSTALLS += $(MRVL_SAI)

ifeq ($(ENABLE_SYNCD_RPC),y)
$(SYNCD)_DEPENDS := $(filter-out $(LIBTHRIFT_DEV),$($(SYNCD)_DEPENDS))
$(SYNCD)_DEPENDS += $(LIBSAITHRIFT_DEV)
endif

# Runtime dependency on mrvl-prestera sai is set only for syncd
$(SYNCD)_RDEPENDS += $(MRVL_SAI)
