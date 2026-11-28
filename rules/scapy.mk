# scapy python3 wheel

# Match the pinned source without deriving a version from shallow checkout tags.
export SCAPY_VERSION = 2.6.1.dev0
SCAPY = scapy-$(SCAPY_VERSION)-py3-none-any.whl
$(SCAPY)_SRC_PATH = $(SRC_PATH)/scapy
$(SCAPY)_PYTHON_VERSION = 3
$(SCAPY)_TEST = n
SONIC_PYTHON_WHEELS += $(SCAPY)
