# $Id$
# $Copyright: (c) 2020 Broadcom.
# Broadcom Proprietary and Confidential. All rights reserved.$
#

# This reference Makefile is intented to compile epdm with driver source. User can optimize it 

CC=gcc -fPIC
VSRCS += $(wildcard ./*.c)
LSRCS = $(notdir $(VSRCS))
LOBJS = $(addsuffix .o, $(basename $(LSRCS)))
BOBJS = $(addprefix $(BLDDIR)/,$(LOBJS))
BLDDIR = $(PWD)
LOCALDIR = $(PWD)
LIBDIR = $(PWD)
LIBNAME = libepdm
LIBSUFFIX = a
CFLAGS = -g -Wall -Werror -Wno-uninitialized -DSERDES_API_FLOATING_POINT
##CFLAGS += -O2 -m32 -fstack-protector-strong -Wformat -Werror=format-security
#CFLAGS += -fPIC

# Uncomment the chip family as needed.  
# For example, to enable dino support uncomment the line with CFLAGS += -DPLP_DINO_SUPPORT
#CFLAGS += -DPLP_DINO_SUPPORT
#CFLAGS += -DPLP_QUADRA28_SUPPORT
#CFLAGS += -DPLP_GALLARDO28_SUPPORT
#CFLAGS += -DPLP_ESTOQUE_SUPPORT
#CFLAGS += -DPLP_ESTOQUEJ_SUPPORT
CFLAGS += -DPLP_BARCHETTA_SUPPORT
#CFLAGS += -DPLP_BARCHETTA2_SUPPORT
#CFLAGS += -DPLP_MILLENIO_SUPPORT
#CFLAGS += -DPLP_MILLENIOB_SUPPORT
#CFLAGS += -DPLP_AGERA_SUPPORT
#CFLAGS += -DPLP_AGERALITE_SUPPORT
#CFLAGS += -DPLP_AGERA2_SUPPORT
#  *** MACSEC PHYs ***
#CFLAGS += -DPLP_EVORA_SUPPORT
#CFLAGS += -DPLP_MIURA_SUPPORT
#CFLAGS += -DPLP_EUROPA_SUPPORT
CFLAGS += -DPLP_APERTA_SUPPORT
CFLAGS += -DPLP_APERTA2_SUPPORT
#Uncomment the PLP_MACSEC_SUPPORT to support MacSec functionality
CFLAGS += -DPLP_MACSEC_SUPPORT
#  *** 1G PHYs  ***
#CFLAGS += -DPLP_SEAHAWKS_SUPPORT
#Uncomment DPLP_MACSEC_SUPPORT to enable macsec functionality for SEAHAWKS
#CFLAGS += -DPLP_BRONCOS_SUPPORT
#CFLAGS += -DPLP_RAVENS_SUPPORT
#CFLAGS += -DPLP_GIANTS_SUPPORT
#CFLAGS += -DPLP_RAMS_SUPPORT
#CFLAGS += -DPLP_COWBOYS_SUPPORT
#  *** 10G PHYs  ***
#CFLAGS += -DPLP_KOI_SUPPORT
#CFLAGS += -DPLP_ORCA_SUPPORT
#CFLAGS += -DPLP_SHORTFIN_SUPPORT
#CFLAGS += -DPLP_BLACKFIN_SUPPORT
#CFLAGS += -DPLP_LONGFIN_SUPPORT
#CFLAGS += -DPLP_BROADFIN_SUPPORT
#CFLAGS += -DPLP_WHITETIP_SUPPORT
#CFLAGS += -DPLP_KAUAI_SUPPORT
#CFLAGS += -DPLP_LANAI_SUPPORT
#Uncomment DPLP_MACSEC_SUPPORT to enable macsec functionality for Longfin/Broadfin/Whitetip
#  *** 1G/10G PHYs  ***
#CFLAGS += -DBCM_PLP_BASE_T_PHY

# HSIP_MAC_PHY and BASET_MAC_PHY need should be set for macsec supported PHYs.
ifneq (,$(findstring PLP_APERTA_SUPPORT,$(CFLAGS)))
FLAGS += HSIP_MAC_PHY
endif
ifneq (,$(findstring PLP_EVORA_SUPPORT,$(CFLAGS)))
FLAGS += HSIP_MAC_PHY
endif
ifneq (,$(findstring PLP_EUROPA_SUPPORT,$(CFLAGS)))
FLAGS += HSIP_MAC_PHY
endif
ifneq (,$(findstring PLP_MIURA_SUPPORT,$(CFLAGS)))
FLAGS += HSIP_MAC_PHY
endif
ifneq (,$(findstring PLP_APERTA2_SUPPORT,$(CFLAGS)))
FLAGS += HSIP_MAC_PHY
endif

ifneq (,$(findstring PLP_SEAHAWKS_SUPPORT,$(CFLAGS)))
FLAGS += BASET_MAC_PHY
endif
ifneq (,$(findstring PLP_LONGFIN_SUPPORT,$(CFLAGS)))
FLAGS += BASET_MAC_PHY
endif
ifneq (,$(findstring PLP_BROADFIN_SUPPORT,$(CFLAGS)))
FLAGS += BASET_MAC_PHY
endif
ifneq (,$(findstring PLP_WHITETIP_SUPPORT,$(CFLAGS)))
FLAGS += BASET_MAC_PHY
endif

# For MACSEC PHYs this section sets the BASET_HSIP_MACSEC_SUPPORT flag automatically if BaseT and HSIP macsec phys are enabled.
ifneq (,$(findstring HSIP_MAC_PHY,$(FLAGS)))
	ifneq (,$(findstring BASET_MAC_PHY,$(FLAGS)))
	CFLAGS += -DBASET_HSIP_MACSEC_SUPPORT
	endif
endif

CPPFLAGS = -I $(PWD) 
ECHO = echo

$(BLDDIR)/%.o: %.c 
	@$(ECHO) 'Compiling $(LOCALDIR)/$<'
	$(CC) $< $(CPPFLAGS) $(CFLAGS) -c -g  -o $@

$(LIBDIR)/$(LIBNAME).$(LIBSUFFIX): $(BOBJS) 
	@$(ECHO) 'Building library $(LIBNAME)...'
	$(Q)$(AR) $(ARFLAGS) $@ $(BOBJS)

BLIBNAME = $(LIBDIR)/$(LIBNAME).$(LIBSUFFIX)


all:: $(BLIBNAME)

clean::
	@$(ECHO) Cleaning objects for $(notdir $(BLIBNAME))
	$(RM) $(BOBJS) $(BLIBNAME)
