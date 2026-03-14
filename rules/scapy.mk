# scapy python3 wheel

SCAPY = scapy-2.4.5-py2.py3-none-any.whl
$(SCAPY)_SRC_PATH = $(SRC_PATH)/scapy
$(SCAPY)_PYTHON_VERSION = 3
$(SCAPY)_TEST = n
SONIC_PYTHON_WHEELS += $(SCAPY)
