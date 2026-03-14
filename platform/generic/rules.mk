include $(PLATFORM_PATH)/aboot-image.mk
include $(PLATFORM_PATH)/onie-image.mk

SONIC_ALL += $(DOCKER_DATABASE) \
         $(DOCKER_SNMP) \
         $(DOCKER_LLDP) \
         $(DOCKER_PLATFORM_MONITOR) \
         $(DOCKER_DHCP_RELAY) \
         $(DOCKER_PTF) \
         $(SONIC_GENERIC_ONIE_IMAGE)
