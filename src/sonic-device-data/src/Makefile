.ONESHELL:
SHELL = /bin/bash
.SHELLFLAGS += -e

test:
	python3 -B ../tests/test_vs_hwsku_order.py
	# Execute Broadcom config file test
	pushd ../tests/
	for f in $$(find ../../../device -name "*.config.bcm"); do
		./config_checker $$f
	done
	for f in $$(find ../../../device -name media_settings.json); do
	    ./media_checker $$f
	done
	for f in $$(find ../../../device -name platform.json); do
	    ./platform_json_checker $$f
	done
	for f in $$(find ../../../device -name hwsku.json); do
	    ./hwsku_json_checker $$f
	done
	./platform_asic_checker ../../../platform ../../../device
	popd
