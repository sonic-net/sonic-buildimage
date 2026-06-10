# rules/systemd-ukify.mk

SYSTEMD_UKIFY_VERSION = 259-1
SYSTEMD_UKIFY = systemd-ukify_$(SYSTEMD_UKIFY_VERSION)_all.deb

$(SYSTEMD_UKIFY)_SRC_PATH = $(SRC_PATH)/systemd-ukify

SONIC_MAKE_DEBS += $(SYSTEMD_UKIFY)
