# platform-cisco repo sources under $(PLATFORM_SRC_PATH)
PLATFORM_SRC_PATH := $(PLATFORM_PATH)/src
export PLATFORM_SRC_PATH

include $(PLATFORM_SRC_PATH)/rules.mk

include $(PLATFORM_PATH)/docker-platform-monitor.mk
include $(PLATFORM_PATH)/docker-syncd-cisco.mk
include $(PLATFORM_PATH)/docker-syncd-cisco-rpc.mk
include $(PLATFORM_PATH)/docker-gbsyncd-cisco.mk
