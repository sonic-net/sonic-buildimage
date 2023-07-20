include $(PLATFORM_PATH)/invm-sai.mk
include $(PLATFORM_PATH)/platform-modules-cel.mk
include $(PLATFORM_PATH)/platform-modules-supermicro.mk
#include $(PLATFORM_PATH)/platform-modules-delta.mk
include $(PLATFORM_PATH)/platform-modules-wistron.mk
#include $(PLATFORM_PATH)/platform-modules-netberg.mk
include $(PLATFORM_PATH)/docker-syncd-invm.mk
include $(PLATFORM_PATH)/docker-syncd-invm-rpc.mk
include $(PLATFORM_PATH)/one-image.mk
include $(PLATFORM_PATH)/libsaithrift-dev.mk
include $(PLATFORM_PATH)/python-saithrift.mk

SONIC_ALL += $(SONIC_INVM_ONE_IMAGE) \
             $(DOCKER_FPM) \
             $(DOCKER_PTF_INVM) \
             $(DOCKER_SYNCD_INVM_RPC)

# Inject invm sai into syncd
$(SYNCD)_DEPENDS += $(INVM_HSAI) $(INVM_LIBSAI) $(LIBSAITHRIFT_DEV_INVM) $(INVM_SHELL)
$(SYNCD)_UNINSTALLS += $(INVM_HSAI)

# Runtime dependency on invm sai is set only for syncd
$(SYNCD)_RDEPENDS += $(INVM_HSAI)
