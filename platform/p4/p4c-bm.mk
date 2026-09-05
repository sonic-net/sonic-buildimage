# p4c bm package

P4C_BM = python-p4c-bm_1.0.0-5415c416-1_all.deb
$(P4C_BM)_SRC_PATH = $(PLATFORM_PATH)/p4c-bm/p4c-bm
$(P4C_BM)_DEPENDS += $(TENJIN) $(P4_HLIR) $(P4_HLIR_V1_1)
$(P4C_BM)_RDEPENDS += $(TENJIN) $(P4_HLIR) $(P4_HLIR_V1_1)
SONIC_PYTHON_STDEB_DEBS += $(P4C_BM)
