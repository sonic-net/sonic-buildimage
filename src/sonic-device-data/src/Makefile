.ONESHELL:
SHELL = /bin/bash
.SHELLFLAGS += -e

test:
	# Execute Broadcom config file test
	pushd ../tests/
	for f in $$(find ../../../device -name "*.config.bcm"); do
		./config_checker $$f
	done
	for f in $$(find ../../../device -name media_settings.json); do
	    ./media_checker $$f
	done
	for f in $$(find ../../../device -name platform.json); do
		if [[ $$(dirname $$f) =~ '../../../device/virtual/x86_64-kvm_x86_64-r0/alpine_' ]]; then
		# Tests expect the platform.json to be in a certain format different
		# from what Alpine uses
			continue
		fi
	    ./platform_json_checker $$f
	done
	for f in $$(find ../../../device -name hwsku.json); do
	    ./hwsku_json_checker $$f
	done
	./platform_asic_checker ../../../platform ../../../device
	popd
