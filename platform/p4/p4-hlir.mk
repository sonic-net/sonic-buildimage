# p4 bmv package

P4_HLIR_V1_1 = python-p4-hlir-v1-1_1.1.7-1_all.deb
$(P4_HLIR_V1_1)_SRC_PATH = $(PLATFORM_PATH)/p4-hlir/p4-hlir-v1.1
SONIC_PYTHON_STDEB_DEBS += $(P4_HLIR_V1_1)

P4_HLIR = python-p4-hlir_0.9.36-1_all.deb
$(P4_HLIR)_SRC_PATH = $(PLATFORM_PATH)/p4-hlir/p4-hlir
SONIC_PYTHON_STDEB_DEBS += $(P4_HLIR)
