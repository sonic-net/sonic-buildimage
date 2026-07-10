# include $(PLATFORM_PATH)/p4-softswitch.mk
include $(PLATFORM_PATH)/tenjin.mk
include $(PLATFORM_PATH)/p4-hlir.mk
include $(PLATFORM_PATH)/p4c-bm.mk
# include $(PLATFORM_PATH)/p4-sai-bm.mk
include $(PLATFORM_PATH)/p4-bmv.mk
include $(PLATFORM_PATH)/p4-switch.mk
include $(PLATFORM_PATH)/docker-sonic-p4.mk
include $(PLATFORM_PATH)/libsaithrift-dev.mk

SONIC_ALL += $(DOCKER_SONIC_P4)

$(SYNCD)_DEPENDS += $(P4_SWITCH)
ifeq ($(ENABLE_SYNCD_RPC),y)
$(SYNCD)_DEPENDS += $(LIBSAITHRIFT_DEV_P4)
endif
$(SYNCD)_RDEPENDS += $(P4_SWITCH)
