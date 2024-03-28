# grpc package
# A newer GRPC has been released in bookworm, So we only need to build it
# in the bullseye environment.
ifeq ($(BLDENV),bullseye)

    GRPC_VERSION = 1.30.2
    GRPC_VERSION_FULL = $(GRPC_VERSION)-3

    export GRPC_VERSION
    export GRPC_VERSION_FULL

    GRPC = libgrpc10_$(GRPC_VERSION_FULL)_$(CONFIGURED_ARCH).deb
    $(GRPC)_DEPENDS = $(PROTOBUF) $(PROTOC32) $(PROTOBUF_COMPILER) $(PROTOBUF_DEV) $(PROTOC_DEV) $(RUBY_PROTOBUF)
    $(GRPC)_SRC_PATH = $(SRC_PATH)/grpc
    SONIC_MAKE_DEBS += $(GRPC)

    GRPC_DEV = libgrpc-dev_$(GRPC_VERSION_FULL)_$(CONFIGURED_ARCH).deb
    $(GRPC_DEV)_DEPENDS = $(GRPC)
    $(eval $(call add_derived_package,$(GRPC),$(GRPC_DEV)))

    GRPC_CPP = libgrpc++1_$(GRPC_VERSION_FULL)_$(CONFIGURED_ARCH).deb
    $(GRPC_CPP)_DEPENDS = $(GRPC) $(PROTOC32)
    $(eval $(call add_derived_package,$(GRPC),$(GRPC_CPP)))

    GRPC_CPP_DEV = libgrpc++-dev_$(GRPC_VERSION_FULL)_$(CONFIGURED_ARCH).deb
    $(GRPC_CPP_DEV)_DEPENDS = $(GRPC_CPP) $(GRPC_DEV)
    $(eval $(call add_derived_package,$(GRPC),$(GRPC_CPP_DEV)))

    PYTHON3_GRPC = python3-grpcio_$(GRPC_VERSION_FULL)_$(CONFIGURED_ARCH).deb
    $(PYTHON3_GRPC)_DEPENDS = $(GRPC_DEV) $(GRPC) $(GRPC_DEV) $(GRPC_CPP) $(GRPC_CPP_DEV) $(PYTHON3_PROTOBUF)
    $(PYTHON3_GRPC)_RDEPENDS = $(GRPC) $(GRPC_CPP) $(PYTHON3_PROTOBUF)
    $(eval $(call add_derived_package,$(GRPC),$(PYTHON3_GRPC)))

    GRPC_COMPILER = protobuf-compiler-grpc_$(GRPC_VERSION_FULL)_$(CONFIGURED_ARCH).deb
    $(GRPC_COMPILER)_DEPENDS = $(PROTOBUF) $(PROTOC32) $(PROTOBUF_COMPILER) 
    $(GRPC_COMPILER)_RDEPENDS = $(PROTOBUF) $(PROTOC32) $(PROTOBUF_COMPILER)
    $(eval $(call add_derived_package,$(GRPC),$(GRPC_COMPILER)))

endif
