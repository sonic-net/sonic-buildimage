# sonic-firebreak: host package for the FIREBREAK daemon.

SONIC_FIREBREAK = sonic-firebreak_1.4_all.deb
$(SONIC_FIREBREAK)_SRC_PATH = $(SRC_PATH)/firebreak

SONIC_DPKG_DEBS += $(SONIC_FIREBREAK)
# slave.mk already waits on this list before it builds the installer.
SONIC_INSTALLER_EXTRA_DEBS += $(SONIC_FIREBREAK)

export SONIC_FIREBREAK
