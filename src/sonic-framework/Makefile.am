.PHONY: compile_protobufs
AUTOMAKE_OPTIONS = subdir-objects
INCLUDES = -I ../sonic-swss-common/common
BUILT_SOURCES = rebootbackend_protobuf_compilation rebootbackend_dbus_compilation

compile_protobufs: rebootbackend_protobuf_compilation rebootbackend_dbus_compilation

rebootbackend_protobuf_compilation: 
	/usr/bin/protoc --cpp_out=rebootbackend \
		--proto_path=gnoi \
		gnoi/types/types.proto \
		gnoi/common/common.proto \
		gnoi/system/system.proto

rebootbackend_dbus_compilation:
	/usr/bin/dbusxx-xml2cpp rebootbackend/gnoi_reboot.xml \
		--proxy=rebootbackend/gnoi_reboot_dbus.h

SUBDIRS = rebootbackend
