# docker image for sonic-mgmt
DOCKER_SONIC_MGMT = docker-sonic-mgmt.gz
GHZ_VERSION = 0.121.0
GHZ_ARCHIVE = ghz-v$(GHZ_VERSION)-linux-x86_64.tar.gz
$(GHZ_ARCHIVE)_URL = https://github.com/bojand/ghz/releases/download/v$(GHZ_VERSION)/ghz-linux-x86_64.tar.gz
GNMI_PROTO_VERSION = 0.14.1
GNMI_PROTO = gnmi-v$(GNMI_PROTO_VERSION).proto
$(GNMI_PROTO)_URL = https://raw.githubusercontent.com/openconfig/gnmi/v$(GNMI_PROTO_VERSION)/proto/gnmi/gnmi.proto
GNMI_EXT_PROTO = gnmi_ext-v$(GNMI_PROTO_VERSION).proto
$(GNMI_EXT_PROTO)_URL = https://raw.githubusercontent.com/openconfig/gnmi/v$(GNMI_PROTO_VERSION)/proto/gnmi_ext/gnmi_ext.proto
GNMI_LICENSE = gnmi-v$(GNMI_PROTO_VERSION)-LICENSE
$(GNMI_LICENSE)_URL = https://raw.githubusercontent.com/openconfig/gnmi/v$(GNMI_PROTO_VERSION)/LICENSE
SONIC_ONLINE_FILES += $(GHZ_ARCHIVE) $(GNMI_PROTO) $(GNMI_EXT_PROTO) $(GNMI_LICENSE)

$(DOCKER_SONIC_MGMT)_PATH = $(DOCKERS_PATH)/docker-sonic-mgmt
$(DOCKER_SONIC_MGMT)_DEPENDS += $(SONIC_DEVICE_DATA)
$(DOCKER_SONIC_MGMT)_FILES += $(GITHUB_GET) $(GHZ_ARCHIVE) $(GNMI_PROTO) $(GNMI_EXT_PROTO) $(GNMI_LICENSE)
SONIC_DOCKER_IMAGES += $(DOCKER_SONIC_MGMT)
SONIC_BOOKWORM_DOCKERS += $(DOCKER_SONIC_MGMT)
