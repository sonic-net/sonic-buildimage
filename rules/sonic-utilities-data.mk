# SONiC command line utilities data package 

SONIC_UTILITIES_DATA = sonic-utilities-data_1.0-1_all.deb
$(SONIC_UTILITIES_DATA)_SRC_PATH = $(SRC_PATH)/sonic-utilities/sonic-utilities-data
$(SONIC_UTILITIES_DATA)_WHEEL_DEPENDS = $(SONIC_UTILITIES_PY3)
SONIC_DPKG_DEBS += $(SONIC_UTILITIES_DATA)
