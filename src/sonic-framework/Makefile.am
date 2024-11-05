.PHONY: compile_protobufs
AUTOMAKE_OPTIONS = subdir-objects
INCLUDES = -I ../sonic-swss-common/common
BUILT_SOURCES = rebootbackend_protobuf_compilation rebootbackend_dbus_compilation

compile_protobufs: rebootbackend_protobuf_compilation rebootbackend_dbus_compilation

update:
	git submodule init
	git submodule update

rebootbackend_protobuf_compilation: update
	sed -i 's/import \"github.com\/openconfig\/gnoi\//import \"/' gnoi/common/common.proto
	sed -i 's/import \"github.com\/openconfig\/gnoi\//import \"/' gnoi/system/system.proto
	/usr/bin/protoc -I=gnoi:github.com/openconfig/gnoi=gnoi --proto_path=gnoi:github.com/openconfig/gnoi=gnoi \
		--cpp_out=rebootbackend \
		gnoi/types/types.proto 

	/usr/bin/protoc -I=gnoi:github.com/openconfig/gnoi=gnoi --proto_path=gnoi:github.com/openconfig/gnoi=gnoi \
		--cpp_out=rebootbackend \
		gnoi/common/common.proto 

	/usr/bin/protoc -I=gnoi:github.com/openconfig/gnoi=gnoi --proto_path=gnoi:github.com/openconfig/gnoi=gnoi \
		--cpp_out=rebootbackend \
		gnoi/system/system.proto

rebootbackend_dbus_compilation:
	/usr/bin/dbusxx-xml2cpp rebootbackend/gnoi_reboot.xml \
		--proxy=rebootbackend/gnoi_reboot_dbus.h

SUBDIRS = rebootbackend
