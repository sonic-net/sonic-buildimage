# docker image for sonic-mgmt
DOCKER_SONIC_MGMT = docker-sonic-mgmt.gz
GHZ_VERSION = 0.121.0
GHZ_COMMIT = b7ca8f5fad40d3da97092a8886e4a4f636712069
GHZ_SOURCE = ghz-v$(GHZ_VERSION)-$(GHZ_COMMIT).tar.gz
$(GHZ_SOURCE)_URL = https://api.github.com/repos/bojand/ghz/tarball/$(GHZ_COMMIT)
GO_VERSION = 1.27.1
ifeq ($(CROSS_BUILD_ENVIRON),y)
GO_ARCHIVE_ARCH = amd64
else ifeq ($(CONFIGURED_ARCH),amd64)
GO_ARCHIVE_ARCH = amd64
else ifeq ($(CONFIGURED_ARCH),arm64)
GO_ARCHIVE_ARCH = arm64
else ifeq ($(CONFIGURED_ARCH),armhf)
GO_ARCHIVE_ARCH = armv6l
else
$(error Unsupported docker-sonic-mgmt architecture: $(CONFIGURED_ARCH))
endif
GO_TOOLCHAIN = go$(GO_VERSION).linux-$(GO_ARCHIVE_ARCH).tar.gz
$(GO_TOOLCHAIN)_URL = https://go.dev/dl/$(GO_TOOLCHAIN)
GNMI_PROTO_VERSION = 0.14.1
GNMI_PROTO = gnmi-v$(GNMI_PROTO_VERSION).proto
$(GNMI_PROTO)_URL = https://raw.githubusercontent.com/openconfig/gnmi/v$(GNMI_PROTO_VERSION)/proto/gnmi/gnmi.proto
GNMI_EXT_PROTO = gnmi_ext-v$(GNMI_PROTO_VERSION).proto
$(GNMI_EXT_PROTO)_URL = https://raw.githubusercontent.com/openconfig/gnmi/v$(GNMI_PROTO_VERSION)/proto/gnmi_ext/gnmi_ext.proto
GNMI_LICENSE = gnmi-v$(GNMI_PROTO_VERSION)-LICENSE
$(GNMI_LICENSE)_URL = https://raw.githubusercontent.com/openconfig/gnmi/v$(GNMI_PROTO_VERSION)/LICENSE
CONFIGOR_LICENSE_COMMIT = f7a0fc7c9fc697269b1749c1cc472d49baa8d33c
CONFIGOR_LICENSE = configor-$(CONFIGOR_LICENSE_COMMIT)-LICENSE
$(CONFIGOR_LICENSE)_URL = https://raw.githubusercontent.com/jinzhu/configor/$(CONFIGOR_LICENSE_COMMIT)/LICENSE
SONIC_ONLINE_FILES += $(GHZ_SOURCE) $(GO_TOOLCHAIN) \
                      $(GNMI_PROTO) $(GNMI_EXT_PROTO) $(GNMI_LICENSE) $(CONFIGOR_LICENSE)

$(DOCKER_SONIC_MGMT)_PATH = $(DOCKERS_PATH)/docker-sonic-mgmt
$(DOCKER_SONIC_MGMT)_DEPENDS += $(SONIC_DEVICE_DATA)
$(DOCKER_SONIC_MGMT)_FILES += $(GITHUB_GET) $(GHZ_SOURCE) $(GO_TOOLCHAIN) \
                              $(GNMI_PROTO) $(GNMI_EXT_PROTO) $(GNMI_LICENSE) $(CONFIGOR_LICENSE)
SONIC_DOCKER_IMAGES += $(DOCKER_SONIC_MGMT)
SONIC_BOOKWORM_DOCKERS += $(DOCKER_SONIC_MGMT)
