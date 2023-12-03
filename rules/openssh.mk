# openssh package

OPENSSH_VERSION = 9.2p1-2+deb12u1

export OPENSSH_VERSION

OPENSSH_SERVER = openssh-server_$(OPENSSH_VERSION)_$(CONFIGURED_ARCH).deb
$(OPENSSH_SERVER)_SRC_PATH = $(SRC_PATH)/openssh
$(OPENSSH_SERVER)_DEPENDS +=  $(LIBNL3_DEV) $(LIBNL_ROUTE3_DEV)
SONIC_MAKE_DEBS += $(OPENSSH_SERVER)

OPENSSH_CLIENT = openssh-client_$(OPENSSH_VERSION)_$(CONFIGURED_ARCH).deb
$(eval $(call add_derived_package,$(OPENSSH_SERVER),$(OPENSSH_CLIENT)))

OPENSSH_SFTP_SERVER = openssh-sftp-server_$(OPENSSH_VERSION)_$(CONFIGURED_ARCH).deb
$(eval $(call add_derived_package,$(OPENSSH_SERVER),$(OPENSSH_SFTP_SERVER)))

# The .c, .cpp, .h & .hpp files under src/{$DBG_SRC_ARCHIVE list}
# are archived into debug one image to facilitate debugging.
#
DBG_SRC_ARCHIVE += openssh
