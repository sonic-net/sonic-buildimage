# sonic-rest-api package

RESTAPI = sonic-rest-api_1.0.1_$(CONFIGURED_ARCH).deb
$(RESTAPI)_SRC_PATH = $(SRC_PATH)/sonic-restapi
$(RESTAPI)_DEPENDS += $(LIBNL3_DEV) $(LIBNL_GENL3_DEV) \
                            $(LIBNL_ROUTE3_DEV) $(LIBSWSSCOMMON_DEV) $(LIBSWSSCOMMON)
$(RESTAPI)_RDEPENDS += $(LIBNL3) $(LIBNL_GENL3) \
                             $(LIBNL_ROUTE3) $(LIBSWSSCOMMON)
SONIC_DPKG_DEBS += $(RESTAPI)
