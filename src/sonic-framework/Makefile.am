.PHONY: compile_protobufs
BUILT_SOURCES = rebootbackend_protobuf_compilation rebootbackend_dbus_compilation

# compile_protobufs target needed by:
# gpins-cicd/scripts/sonic-framework/build_and_test_in_docker.sh
compile_protobufs: rebootbackend_protobuf_compilation rebootbackend_dbus_compilation

rebootbackend_protobuf_compilation:
	/usr/bin/protoc --cpp_out=rebootbackend --proto_path=third_party/gnoi \
		third_party/gnoi/types/types.proto \
		third_party/gnoi/common/common.proto \
		third_party/gnoi/system/system.proto
	/usr/bin/protoc --experimental_allow_proto3_optional --cpp_out=. rebootbackend/container_stop.proto

rebootbackend_dbus_compilation:
	/usr/bin/dbusxx-xml2cpp rebootbackend/gnoi_reboot.xml \
		--proxy=rebootbackend/gnoi_reboot_dbus.h
	/usr/bin/dbusxx-xml2cpp rebootbackend/gnoi_container_shutdown.xml \
		--proxy=rebootbackend/gnoi_container_shutdown_dbus.h

SUBDIRS = stateverificationd rebootbackend
