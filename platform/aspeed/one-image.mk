# ONIE installer for Aspeed BMC platform with U-Boot

SONIC_ONE_IMAGE = sonic-aspeed-arm64.bin
$(SONIC_ONE_IMAGE)_ARCH = arm64
$(SONIC_ONE_IMAGE)_MACHINE = aspeed
$(SONIC_ONE_IMAGE)_PLATFORM = aspeed
$(SONIC_ONE_IMAGE)_IMAGE_TYPE = onie

# Aspeed BMC is a minimal embedded platform - only include essential components
DISABLED_PACKAGES_LOCAL = $(DOCKER_DHCP_RELAY) $(DOCKER_SFLOW) $(DOCKER_MGMT_FRAMEWORK) \
                          $(DOCKER_NAT) $(DOCKER_TEAMD) $(DOCKER_ROUTER_ADVERTISER) \
                          $(DOCKER_MUX) $(DOCKER_MACSEC) \
                          $(DOCKER_EVENTD) $(DOCKER_DASH_HA) $(DOCKER_STP) \
                          $(DOCKER_RESTAPI)

# Aspeed BMC does not ship eventd / snmp / radv / restapi as features. Disable
# their INCLUDE_* flags so the rendered init_cfg.json FEATURE table does not
# advertise them as enabled, which would otherwise cause "show feature status"
# and tools/tests that trust it to try to operate on systemd units that do not
# exist on BMC.
DISABLED_FEATURE_FLAGS = INCLUDE_SYSTEM_EVENTD \
                         INCLUDE_SNMP \
                         INCLUDE_ROUTER_ADVERTISER \
                         INCLUDE_RESTAPI

$(info [aspeed] Filtering out packages: $(DISABLED_PACKAGES_LOCAL))
$(info [aspeed] Disabling feature flags: $(DISABLED_FEATURE_FLAGS))

SONIC_PACKAGES_LOCAL := $(filter-out $(DISABLED_PACKAGES_LOCAL), $(SONIC_PACKAGES_LOCAL))
$(foreach feature, $(DISABLED_FEATURE_FLAGS), $(eval override $(feature)=n))

$(SONIC_ONE_IMAGE)_INSTALLS += $(SYSTEMD_SONIC_GENERATOR)
$(SONIC_ONE_IMAGE)_INSTALLS += $(ASPEED_PLATFORM_SERVICES)
$(SONIC_ONE_IMAGE)_LAZY_INSTALLS += $(ASPEED_EVB_AST2700_PLATFORM_MODULE)
$(SONIC_ONE_IMAGE)_LAZY_INSTALLS += $(NEXTHOP_COMMON_PLATFORM_MODULE)
$(SONIC_ONE_IMAGE)_LAZY_INSTALLS += $(ASPEED_NEXTHOP_B27_PLATFORM_MODULE)
$(SONIC_ONE_IMAGE)_LAZY_INSTALLS += $(ASPEED_NVIDIA_AST2700_BMC_PLATFORM_MODULE)
$(SONIC_ONE_IMAGE)_LAZY_INSTALLS += $(ARISTA_PLATFORM_MODULE_ALL)
$(SONIC_ONE_IMAGE)_LAZY_INSTALLS += $(NOKIA_BMC_H6_128_PLATFORM_MODULE)

# NVIDIA hw-management-bmc
$(SONIC_ONE_IMAGE)_LAZY_INSTALLS += $(MLNX_HW_MANAGEMENT_BMC)

$(SONIC_ONE_IMAGE)_DOCKERS = $(DOCKER_DATABASE) $(DOCKER_GNMI) $(DOCKER_PLATFORM_MONITOR) $(DOCKER_LLDP) $(DOCKER_TELEMETRY) $(DOCKER_SYSMGR)
ifeq ($(INCLUDE_REDFISH), y)
$(SONIC_ONE_IMAGE)_DOCKERS += $(DOCKER_SONIC_REDFISH)
endif
SONIC_INSTALLERS += $(SONIC_ONE_IMAGE)
