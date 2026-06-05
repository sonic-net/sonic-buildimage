#
# $Copyright:.$
#

ifeq ($(PHYMOD_CHIPS),)
MAKE_PHYMOD_CHIPS += TSCP
endif
ifneq ($(filter TSCP,$(MAKE_PHYMOD_CHIPS)),)
VPATH += chip/tscp chip/tscp/tier2 chip/tscp/tier1
VSRCS += $(wildcard chip/tscp/*.c)
VSRCS += $(wildcard chip/tscp/tier2/*.c)
VSRCS += $(wildcard chip/tscp/tier1/*.c)
endif
