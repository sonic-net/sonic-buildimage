#
# $Copyright:.$
#

ifeq ($(PHYMOD_CHIPS),)
MAKE_PHYMOD_CHIPS += PEREGRINE5_PC
endif
ifneq ($(filter PEREGRINE5_PC,$(MAKE_PHYMOD_CHIPS)),)
VPATH += chip/peregrine5_pc chip/peregrine5_pc/tier2 chip/peregrine5_pc/tier1
VSRCS += $(wildcard chip/peregrine5_pc/*.c)
VSRCS += $(wildcard chip/peregrine5_pc/tier2/*.c)
VSRCS += $(wildcard chip/peregrine5_pc/tier1/*.c)
endif
