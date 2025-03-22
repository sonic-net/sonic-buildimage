AUTOMAKE_OPTIONS = subdir-objects
INCLUDES = -I ../sonic-swss-common/common -Ibuild/gen -Ibuild/gen/github.com/openconfig/gnoi
BUILT_SOURCES = rebootbackend_protobuf_compilation rebootbackend_dbus_compilation

.NOTPARALLEL: compile_protobufs
compile_protobufs: rebootbackend_protobuf_compilation rebootbackend_dbus_compilation

rebootbackend_protobuf_compilation: 
	mkdir -p build/gen
	/usr/bin/protoc --cpp_out=build/gen \
		--proto_path=github.com/openconfig/gnoi=gnoi \
		gnoi/types/types.proto \
		gnoi/common/common.proto \
		gnoi/system/system.proto

rebootbackend_dbus_compilation: build/gen/librebootgnoi.la
	/usr/bin/dbusxx-xml2cpp rebootbackend/reboot.xml \
		--proxy=rebootbackend/reboot_dbus.h

lib_LTLIBRARIES = build/gen/librebootgnoi.la
build_gen_librebootgnoi_la_SOURCES = \
        build/gen/github.com/openconfig/gnoi/system/system.pb.cc \
        build/gen/github.com/openconfig/gnoi/types/types.pb.cc \
        build/gen/github.com/openconfig/gnoi/common/common.pb.cc

SUBDIRS = rebootbackend
