# redis package
# TODO: docker-sonic-p4 depends on redis-tools in Jessie.
#       Remove this file and src/redis after that resolved.
ifneq ($(BLDENV),buster)

    REDIS_VERSION = 5.0.3-3~bpo9+2

    REDIS_TOOLS = redis-tools_$(REDIS_VERSION)_$(CONFIGURED_ARCH).deb
    $(REDIS_TOOLS)_SRC_PATH = $(SRC_PATH)/redis
    SONIC_MAKE_DEBS += $(REDIS_TOOLS)

    REDIS_TOOLS_DBG = redis-tools-dbgsym_$(REDIS_VERSION)_$(CONFIGURED_ARCH).deb
    $(eval $(call add_derived_package,$(REDIS_TOOLS),$(REDIS_TOOLS_DBG)))

    REDIS_SERVER = redis-server_$(REDIS_VERSION)_$(CONFIGURED_ARCH).deb
    $(eval $(call add_derived_package,$(REDIS_TOOLS),$(REDIS_SERVER)))

    REDIS_SENTINEL = redis-sentinel_$(REDIS_VERSION)_$(CONFIGURED_ARCH).deb
    $(REDIS_SENTINEL)_DEPENDS += $(REDIS_SERVER)
    $(REDIS_SENTINEL)_RDEPENDS += $(REDIS_SERVER)
    $(eval $(call add_derived_package,$(REDIS_TOOLS),$(REDIS_SENTINEL)))

    # The .c, .cpp, .h & .hpp files under src/{$DBG_SRC_ARCHIVE list}
    # are archived into debug one image to facilitate debugging.
    #
    DBG_SRC_ARCHIVE += redis

endif
