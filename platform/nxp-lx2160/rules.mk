# NXP LX2160A (Layerscape) as SONiC control-plane CPU.
# The switch ASIC is emulated with SAI-VS (sairedis vslib): front-panel ports
# are bound to the board's DPAA2 network interfaces through the hwsku's
# lanemap.ini, so traffic on those ports is real and only the ASIC is virtual.
include $(PLATFORM_PATH)/syncd-vs.mk
include $(PLATFORM_PATH)/docker-syncd-vs.mk
include $(PLATFORM_PATH)/platform-modules-nxp.mk
include $(PLATFORM_PATH)/one-image.mk

SONIC_ALL += $(SONIC_ONE_IMAGE)
