include $(PLATFORM_PATH)/sai.mk
include $(PLATFORM_PATH)/platform-modules-cel.mk
include $(PLATFORM_PATH)/platform-modules-supermicro.mk
include $(PLATFORM_PATH)/platform-modules-wistron.mk
include $(PLATFORM_PATH)/platform-modules-marvell.mk
include $(PLATFORM_PATH)/docker-syncd-mrvl-teralynx.mk
include $(PLATFORM_PATH)/docker-syncd-mrvl-teralynx-rpc.mk
include $(PLATFORM_PATH)/one-image.mk
include $(PLATFORM_PATH)/docker-saiserver-mrvl-teralynx.mk
include $(PLATFORM_PATH)/libsaithrift-dev.mk
include $(PLATFORM_PATH)/python-saithrift.mk
include $(PLATFORM_PATH)/mrvl-teralynx.mk
include $(PLATFORM_PATH)/or-tools.mk

SONIC_ALL += $(SONIC_MRVL_TERALYNX_ONE_IMAGE) \
             $(DOCKER_FPM) \
             $(DOCKER_PTF_MRVL_TERALYNX) \
             $(DOCKER_SYNCD_MRVL_TERALYNX_RPC)

# Inject mrvl-teralynx sai into syncd
$(SYNCD)_DEPENDS += $(MRVL_TERALYNX_HSAI) $(MRVL_TERALYNX_LIBSAI) $(MRVL_TERALYNX_SHELL) $(LIBOR_TOOLS)
$(SYNCD)_UNINSTALLS += $(MRVL_TERALYNX_HSAI)

ifeq ($(ENABLE_SYNCD_RPC),y)
$(SYNCD)_DEPENDS := $(filter-out $(LIBTHRIFT_DEV),$($(SYNCD)_DEPENDS))
$(SYNCD)_DEPENDS += $(LIBSAITHRIFT_DEV)
endif

# Runtime dependency on mrvl-teralynx sai is set only for syncd
$(SYNCD)_RDEPENDS += $(MRVL_TERALYNX_HSAI)
