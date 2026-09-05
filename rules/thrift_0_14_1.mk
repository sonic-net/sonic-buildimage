# thrift package

THRIFT_0_14_1_VERSION = 0.14.1
THRIFT_0_14_1_VERSION_FULL = $(THRIFT_0_14_1_VERSION)

LIBTHRIFT_0_14_1 = libthrift0_$(THRIFT_0_14_1_VERSION)_$(CONFIGURED_ARCH).deb
$(LIBTHRIFT_0_14_1)_SRC_PATH = $(SRC_PATH)/thrift_0_14_1
SONIC_MAKE_DEBS += $(LIBTHRIFT_0_14_1)

LIBTHRIFT_0_14_1_DEV = libthrift-dev_$(THRIFT_0_14_1_VERSION)_$(CONFIGURED_ARCH).deb
$(eval $(call add_derived_package,$(LIBTHRIFT_0_14_1),$(LIBTHRIFT_0_14_1_DEV)))

PYTHON3_THRIFT_0_14_1 = python3-thrift_$(THRIFT_0_14_1_VERSION)_$(CONFIGURED_ARCH).deb
$(eval $(call add_derived_package,$(LIBTHRIFT_0_14_1),$(PYTHON3_THRIFT_0_14_1)))

PYTHON_THRIFT_0_14_1 = python-thrift_$(THRIFT_0_14_1_VERSION)_$(CONFIGURED_ARCH).deb
$(eval $(call add_derived_package,$(LIBTHRIFT_0_14_1),$(PYTHON_THRIFT_0_14_1)))

THRIFT_0_14_1_COMPILER = thrift-compiler_$(THRIFT_0_14_1_VERSION)_$(CONFIGURED_ARCH).deb
$(eval $(call add_derived_package,$(LIBTHRIFT_0_14_1),$(THRIFT_0_14_1_COMPILER)))
