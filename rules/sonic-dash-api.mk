# libdashsai package

LIB_SONIC_DASH_API_VERSION = 1.0.0

LIB_SONIC_DASH_API = libdashapi_$(LIB_SONIC_DASH_API_VERSION)_$(CONFIGURED_ARCH).deb
$(LIB_SONIC_DASH_API)_SRC_PATH = $(SRC_PATH)/sonic-dash-api

$(LIB_SONIC_DASH_API)_DEPENDS += $(PROTOBUF) $(PROTOBUF_LITE) $(PROTOBUF_DEV) $(PROTOBUF_COMPILER)
$(LIB_SONIC_DASH_API)_RDEPENDS += $(PROTOBUF) $(PROTOBUF_LITE) $(PYTHON3_PROTOBUF)

SONIC_DPKG_DEBS += $(LIB_SONIC_DASH_API)

LIB_SONIC_DASH_API_DBG = libdashapi-dbgsym_$(LIB_SONIC_DASH_API_VERSION)_$(CONFIGURED_ARCH).deb
$(eval $(call add_derived_package,$(LIB_SONIC_DASH_API),$(LIB_SONIC_DASH_API_DBG)))

# The .c, .cpp, .h & .hpp files under src/{$DBG_SRC_ARCHIVE list}
# are archived into debug one image to facilitate debugging.
#
DBG_SRC_ARCHIVE += sonic-dash-api

