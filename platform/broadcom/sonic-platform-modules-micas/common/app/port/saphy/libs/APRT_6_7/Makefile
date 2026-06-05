#
# $Id$
# $Copyright: (c) 2020 Broadcom.
# Broadcom Proprietary and Confidential. All rights reserved.$
#
export CC=gcc 

LOCALDIR = libs/phymod
ifndef PHYMOD_EPIL
PHYMOD_EPIL = $(CURDIR)
endif

#PHYMOD_EPIL_CPPFLAGS = -DBCM_PLP_BASE_T_PHY -DBCM_PLP_LEGACY_PHY_SUPPORT -DBCM_ESW_SUPPORT -DPTRS_ARE_64BITS
#PHYMOD_EPIL_CPPFLAGS = -DBCM_PLP_LEGACY_PHY_SUPPORT -DBCM_ESW_SUPPORT -DPTRS_ARE_64BITS

#LEGACY_DRIVER_INCLUDE_PATH += -I$(PHYMOD_EPIL)/phymod/include/phymod/chip/legacy_include -I$(PHYMOD_EPIL)/phymod/include/phymod/chip/legacy_include/auxiliary -I$(PHYMOD_EPIL)/phymod/include/phymod
#PHYMOD_EPIL_INCLUDE_PATH += $(LEGACY_DRIVER_INCLUDE_PATH)

include ${PHYMOD_EPIL}/make/config.mk

ifneq (,$(findstring -DUSE_EPDM_COMMON_DEFINES,$(CPPFLAGS)))
ifeq ($(EPDM_HOME), )
$(error EPDM_HOME not defined)
else
PHYMOD_EPIL_INCLUDE_PATH += -I$(EPDM_HOME)/
endif
endif

# Core library
VPATH = phymod/core phymod/diag
VSRCS = $(wildcard phymod/core/*.c) $(wildcard phymod/diag/*.c)

#interface layer support
VPATH += bcm_pm_if
VSRCS += $(wildcard bcm_pm_if/*.c)              
PHYMOD_EPIL_INCLUDE_PATH += -I bcm_pm_if
#MACSEC Support
ifneq (,$(findstring -DINCLUDE_PLP_MACSEC,$(CPPFLAGS)))

ifeq ($(MACSECHOME), )
MACSECHOME =.
endif
VPATH += $(MACSECHOME)/bcm_plp_sec
VSRCS += $(wildcard $(MACSECHOME)/bcm_plp_sec/*.c)
endif
#MACDIAG Support
ifneq (,$(findstring DBCM_PLP_MAC_SUPPORT,$(CPPFLAGS)))
$(info Compiling with PLP MAC SUPPORT)
else
#removing bcm_plp_mac_api.c if compile FLAG is not there
VSRCS := $(filter-out bcm_pm_if/bcm_plp_mac_api.c,$(VSRCS))
endif

# TimeSync IEEE1588 support
ifneq (,$(findstring DBCM_PLP_TIMESYNC_SUPPORT,$(CPPFLAGS)))
VPATH += phymod/common/ieee1588
VSRCS += $(wildcard phymod/common/ieee1588/*.c)
PHYMOD_EPIL_INCLUDE_PATH += -I$(PHYMOD_EPIL)/phymod/common/ieee1588
endif

# PHY util
VPATH += phymod/util 
VSRCS += $(wildcard phymod/util/*.c) 

#IMACSEC Support
ifneq (,$(findstring -DINCLUDE_PLP_IMACSEC,$(CPPFLAGS)))
VPATH  += bcm_plp_base_t_sec
VSRCS += $(wildcard bcm_plp_base_t_sec/*.c)
VPATH += imacsec/Kit/EIP160/src imacsec/Kit/EIP201/src imacsec/Kit/List/src imacsec/Kit/Log/src imacsec/Kit/SABuilder_MACsec/src
VPATH += imacsec/Integration/Adapter_EIP160/src imacsec/Integration/Adapter_EIP160/src/common imacsec/Integration/DriverFramework/src

VSRCS += imacsec/Integration/Adapter_EIP160/src/adapter_xtsecy.c \
         imacsec/Integration/DriverFramework/src/hwpal_device_umdevxs.c \
         imacsec/Integration/Adapter_EIP160/src/common/adapter_sleep.c \
         imacsec/Integration/Adapter_EIP160/src/common/adapter_driver165_init.c imacsec/Integration/Adapter_EIP164/src/common/adapter_interrupts.c \
         imacsec/Integration/Adapter_EIP160/src/common/adapter_init.c
VSRCS += imacsec/Kit/EIP160/src/eip160_device.c imacsec/Kit/EIP160/src/eip160_secy.c \
         imacsec/Kit/EIP201/src/eip201.c imacsec/Kit/List/src/list.c imacsec/Kit/Log/src/log.c \
         imacsec/Kit/SABuilder_MACsec/src/sa_builder_macsec.c imacsec/Kit/SABuilder_MACsec/src/sa_builder_eip165.c

ifneq (,$(findstring -DMACSEC_BASET_V2_2,$(CPPFLAGS)))
VSRCS += imacsec/Integration/Adapter_EIP160/src/adapter_init_support.c \
         imacsec/Integration/Adapter_EIP160/src/adapter_init_warmboot.c \
         imacsec/Integration/Adapter_EIP160/src/adapter_xtsecy_warmboot.c \
         imacsec/Integration/Adapter_EIP160/src/adapter_warmboot_support.c \
         imacsec/Integration/Adapter_EIP160/src/adapter_warmboot.c \
         imacsec/Integration/Adapter_EIP160/src/adapter_xtsecy_ext.c \
         imacsec/Integration/DriverFramework/src/device_generic.c
endif


# Needed for DDK compilation
PHYMOD_EPIL_INCLUDE_PATH += -I imacsec/Integration/Adapter_EIP160/src -I imacsec/Integration/Adapter_EIP160/incl \
         -I imacsec/Integration/Adapter_EIP160/src/common -I imacsec/Kit/EIP160/src -I imacsec/Kit/EIP160/incl \
         -I imacsec/Kit/Log/incl -I imacsec/Kit/List/src -I imacsec/Kit/EIP201/src -I imacsec/Kit/EIP201/incl -I imacsec/Kit/DriverFramework/CLib_Abstraction_API/incl \
         -I  bcm_plp_base_t_sec -I imacsec/Kit/DriverFramework/Basic_Defs_API/incl -I imacsec/Kit/DriverFramework/Device_API/incl \
         -I imacsec/include -I imacsec/Kit/List/incl -I imacsec/Kit/EIP217/src -I imacsec/Integration/DriverFramework/src -I imacsec/Kit/SABuilder_MACsec/src -I imacsec/Kit/SABuilder_MACsec/incl

endif

# Madura chip support
VPATH +=  phymod/chip/madura  phymod/chip/madura/tier2  phymod/chip/madura/tier1  phymod/chip/madura/tier1/madura_serdes/falcon2_madura_src
VSRCS += $(wildcard  phymod/chip/madura/*.c)
VSRCS += $(wildcard  phymod/chip/madura/tier2/*.c)
VSRCS += $(wildcard  phymod/chip/madura/tier1/*.c)
VSRCS += $(wildcard  phymod/chip/madura/tier1/madura_serdes/falcon2_madura_src/falcon2_madura_functions.c)
VSRCS += $(wildcard  phymod/chip/madura/tier1/madura_serdes/falcon2_madura_src/falcon2_madura_dependencies.c)

VPATH += phymod/chip/dino phymod/chip/dino/tier2 phymod/chip/dino/tier1 phymod/chip/dino/tier1/dino_serdes/falcon2_dino_src phymod/chip/dino/tier1/dino_serdes/merlin_dino_src
VSRCS += $(wildcard phymod/chip/dino/*.c)
VSRCS += $(wildcard phymod/chip/dino/tier2/*.c)
VSRCS += $(wildcard phymod/chip/dino/tier1/*.c)
VSRCS += $(wildcard phymod/chip/dino/tier1/dino_serdes/falcon2_dino_src/falcon2_dino_functions.c)
VSRCS += $(wildcard phymod/chip/dino/tier1/dino_serdes/falcon2_dino_src/falcon2_dino_dependencies.c)
VSRCS += $(wildcard phymod/chip/dino/tier1/dino_serdes/merlin_dino_src/merlin_dino_functions.c)
VSRCS += $(wildcard phymod/chip/dino/tier1/dino_serdes/merlin_dino_src/merlin_dino_dependencies.c)


# Miura support
VPATH += phymod/chip/miura phymod/chip/miura/tier2 phymod/chip/miura/tier1 phymod/chip/miura/miura_pm phymod/chip/miura/miura_pm/pms
VSRCS += $(wildcard phymod/chip/miura/*.c)
VSRCS += $(wildcard phymod/chip/miura/tier2/*.c)
VSRCS += $(wildcard phymod/chip/miura/tier1/*.c)
VSRCS += $(wildcard phymod/chip/miura/miura_pm/portmod_dispatch.c)
VSRCS += $(wildcard phymod/chip/miura/miura_pm/pms/portmod_chain.c)
VSRCS += $(wildcard phymod/chip/miura/miura_pm/pms/xlmac.c)
VSRCS += $(wildcard phymod/chip/miura/miura_pm/pms/pm4x10.c)
VSRCS += $(wildcard phymod/chip/miura/miura_pm/pms/pm4x10_dispatch.c)
PHYMOD_EPIL_INCLUDE_PATH += -I. -I phymod/chip/miura -I phymod/chip/miura/miura_pm -I phymod/chip/miura/miura_pm/pms

#PMD 
PHYMOD_EPIL_INCLUDE_PATH += -I phymod/chip/miura/miura_pm/pcs_pmd
VPATH += phymod/chip/miura/miura_pm/pcs_pmd/eagle_miura phymod/chip/miura/miura_pm/pcs_pmd/eagle_miura/tier2 phymod/chip/miura/miura_pm/pcs_pmd/eagle_miura/tier1
VSRCS += $(wildcard phymod/chip/miura/miura_pm/pcs_pmd/eagle_miura/*.c)
VSRCS += $(wildcard phymod/chip/miura/miura_pm/pcs_pmd/eagle_miura/tier2/*.c)
VSRCS += $(wildcard phymod/chip/miura/miura_pm/pcs_pmd/eagle_miura/tier1/*.c)
PHYMOD_EPIL_INCLUDE_PATH += -I phymod/chip/miura/miura_pm/pcs_pmd/eagle_miura
#PMD 
VPATH += phymod/chip/miura/miura_pm/pcs_pmd/merlin_miura phymod/chip/miura/miura_pm/pcs_pmd/merlin_miura/tier2 phymod/chip/miura/miura_pm/pcs_pmd/merlin_miura/tier1
VSRCS += $(wildcard phymod/chip/miura/miura_pm/pcs_pmd/merlin_miura/*.c)
VSRCS += $(wildcard phymod/chip/miura/miura_pm/pcs_pmd/merlin_miura/tier2/*.c)
VSRCS += $(wildcard phymod/chip/miura/miura_pm/pcs_pmd/merlin_miura/tier1/*.c)
PHYMOD_EPIL_INCLUDE_PATH += -I phymod/chip/miura/miura_pm/pcs_pmd/merlin_miura
#
#tsce support
VPATH += phymod/chip/miura/miura_pm/pcs_pmd/tsce_miura phymod/chip/miura/miura_pm/pcs_pmd/tsce_miura/tier2 phymod/chip/miura/miura_pm/pcs_pmd/tsce_miura/tier1
VSRCS += $(wildcard phymod/chip/miura/miura_pm/pcs_pmd/tsce_miura/*.c)
VSRCS += $(wildcard phymod/chip/miura/miura_pm/pcs_pmd/tsce_miura/tier2/*.c)
VSRCS += $(wildcard phymod/chip/miura/miura_pm/pcs_pmd/tsce_miura/tier1/*.c)
PHYMOD_EPIL_INCLUDE_PATH += -I. -I phymod/chip/miura/miura_pm/pcs_pmd/tsce_miura

# Evora support
VPATH += phymod/chip/evora phymod/chip/evora/tier2 phymod/chip/evora/tier1 phymod/chip/evora/evora_pm phymod/chip/evora/evora_pm/pms
VSRCS += $(wildcard phymod/chip/evora/*.c)
VSRCS += $(wildcard phymod/chip/evora/tier2/*.c)
VSRCS += $(wildcard phymod/chip/evora/tier1/*.c)
#VSRCS += $(wildcard phymod/chip/evora/evora_pm/pms/portmod_phy_control.c)
VSRCS += $(wildcard phymod/chip/evora/evora_pm/pms/portmod_chain.c)
VSRCS += $(wildcard phymod/chip/evora/evora_pm/portmod_dispatch.c)
VSRCS += $(wildcard phymod/chip/evora/evora_pm/pms/clmac.c)
VSRCS += $(wildcard phymod/chip/evora/evora_pm/pms/pm4x25.c)
VSRCS += $(wildcard phymod/chip/evora/evora_pm/pms/pm4x25_dispatch.c)

#PMD 
VPATH += phymod/chip/evora/evora_pm/pcs_pmd/falcon_evora phymod/chip/evora/evora_pm/pcs_pmd/falcon_evora/tier2 phymod/chip/evora/evora_pm/pcs_pmd/falcon_evora/tier1
VSRCS += $(wildcard phymod/chip/evora/evora_pm/pcs_pmd/falcon_evora/*.c)
VSRCS += $(wildcard phymod/chip/evora/evora_pm/pcs_pmd/falcon_evora/tier2/*.c)
VSRCS += $(wildcard phymod/chip/evora/evora_pm/pcs_pmd/falcon_evora/tier1/*.c)

#tscf support
VPATH += phymod/chip/evora/evora_pm/pcs_pmd/tscf_evora phymod/chip/evora/evora_pm/pcs_pmd/tscf_evora/tier2 phymod/chip/evora/evora_pm/pcs_pmd/tscf_evora/tier1
VSRCS += $(wildcard phymod/chip/evora/evora_pm/pcs_pmd/tscf_evora/*.c)
VSRCS += $(wildcard phymod/chip/evora/evora_pm/pcs_pmd/tscf_evora/tier2/*.c)
VSRCS += $(wildcard phymod/chip/evora/evora_pm/pcs_pmd/tscf_evora/tier1/*.c)

# MACSEC support
ifneq (,$(findstring -DINCLUDE_PLP_MACSEC,$(CPPFLAGS)))
VPATH += $(MACSECHOME)/ddk/Kit/EIP163/src $(MACSECHOME)/ddk/Kit/EIP164/src $(MACSECHOME)/ddk/Kit/EIP201/src $(MACSECHOME)/ddk/Kit/List/src $(MACSECHOME)/ddk/Kit/Log/src $(MACSECHOME)/ddk/Kit/SABuilder_MACsec/src
VPATH += $(MACSECHOME)/ddk/Integration/Adapter_EIP164/src $(MACSECHOME)/ddk/Integration/Adapter_EIP164/src/common $(MACSECHOME)/ddk/Integration/Adapter_EIP163/src $(MACSECHOME)/ddk/Integration/DriverFramework/src

VSRCS += $(MACSECHOME)/ddk/Integration/Adapter_EIP163/src/adapter_cfye.c \
         $(MACSECHOME)/ddk/Integration/Adapter_EIP164/src/adapter_secy.c $(MACSECHOME)/ddk/Integration/DriverFramework/src/hwpal_device_umdevxs.c \
         $(MACSECHOME)/ddk/Integration/Adapter_EIP164/src/common/adapter_sleep.c \
         $(MACSECHOME)/ddk/Integration/Adapter_EIP164/src/common/adapter_driver164_init.c $(MACSECHOME)/ddk/Integration/Adapter_EIP164/src/common/adapter_interrupts.c \
         $(MACSECHOME)/ddk/Integration/Adapter_EIP164/src/common/adapter_init.c \
         $(MACSECHOME)/ddk/Integration/Adapter_EIP164/src/common/adapter_warmboot.c

APRT := $(if $(findstring -DMACSEC_V2_1,$(CPPFLAGS)),YES)
APRT2 := $(if $(findstring -DMACSEC_V3_2,$(CPPFLAGS)),YES)
ifeq ($(or $(APRT),$(APRT2)),YES)
VSRCS += $(MACSECHOME)/ddk/Integration/Adapter_EIP163/src/adapter_cfye_ext.c \
         $(MACSECHOME)/ddk/Integration/Adapter_EIP163/src/adapter_cfye_rule.c \
         $(MACSECHOME)/ddk/Integration/Adapter_EIP163/src/adapter_cfye_stats.c \
         $(MACSECHOME)/ddk/Integration/Adapter_EIP163/src/adapter_cfye_support.c \
         $(MACSECHOME)/ddk/Integration/Adapter_EIP163/src/adapter_cfye_vport.c \
         $(MACSECHOME)/ddk/Integration/Adapter_EIP163/src/adapter_cfye_warmboot.c \
         $(MACSECHOME)/ddk/Integration/Adapter_EIP164/src/common/adapter_init_support.c \
         $(MACSECHOME)/ddk/Integration/Adapter_EIP164/src/common/adapter_init_warmboot.c \
         $(MACSECHOME)/ddk/Integration/Adapter_EIP164/src/common/adapter_secy_ext.c \
         $(MACSECHOME)/ddk/Integration/Adapter_EIP164/src/common/adapter_secy_sa.c \
         $(MACSECHOME)/ddk/Integration/Adapter_EIP164/src/common/adapter_secy_stats.c \
         $(MACSECHOME)/ddk/Integration/Adapter_EIP164/src/common/adapter_secy_warmboot.c \
         $(MACSECHOME)/ddk/Integration/Adapter_EIP164/src/common/adapter_secy_support.c \
         $(MACSECHOME)/ddk/Integration/Adapter_EIP164/src/common/adapter_warmboot_support.c \
         $(MACSECHOME)/ddk/Integration/DriverFramework/src/device_generic.c
endif

VSRCS += $(MACSECHOME)/ddk/Kit/EIP163/src/eip163.c $(MACSECHOME)/ddk/Kit/EIP164/src/eip164_rules.c $(MACSECHOME)/ddk/Kit/EIP164/src/eip164_device.c $(MACSECHOME)/ddk/Kit/EIP164/src/eip164_secy.c \
         $(MACSECHOME)/ddk/Kit/EIP201/src/eip201.c $(MACSECHOME)/ddk/Kit/SABuilder_MACsec/src/sa_builder_macsec.c $(MACSECHOME)/ddk/Kit/List/src/list.c $(MACSECHOME)/ddk/Kit/Log/src/log.c \
         $(MACSECHOME)/ddk/Kit/SABuilder_MACsec/src/sa_builder_eip164.c

# Needed for DDK compilation
PHYMOD_EPIL_INCLUDE_PATH += -I $(MACSECHOME)/ddk/Integration/Adapter_EIP164/src -I $(MACSECHOME)/ddk/Integration/Adapter_EIP164/incl -I $(MACSECHOME)/ddk/Integration/Adapter_EIP163/src \
         -I $(MACSECHOME)/ddk/Integration/Adapter_EIP163/incl -I $(MACSECHOME)/ddk/Integration/Adapter_EIP164/src/common -I $(MACSECHOME)/ddk/Kit/EIP163/src -I $(MACSECHOME)/ddk/Kit/EIP163/incl \
         -I $(MACSECHOME)/ddk/Kit/EIP164/src -I $(MACSECHOME)/ddk/Kit/EIP164/incl -I $(MACSECHOME)/ddk/Kit/Log/incl -I $(MACSECHOME)/ddk/Kit/List/src -I $(MACSECHOME)/ddk/Kit/EIP201/src \
         -I $(MACSECHOME)/ddk/Kit/EIP201/incl -I $(MACSECHOME)/ddk/Kit/DriverFramework/CLib_Abstraction_API/incl \
         -I $(MACSECHOME)/ddk/Kit/DriverFramework/Basic_Defs_API/incl -I $(MACSECHOME)/ddk/Kit/DriverFramework/Device_API/incl -I bcm_pm_if -I $(MACSECHOME)/bcm_plp_sec \
         -I $(MACSECHOME)/ddk/include -I $(MACSECHOME)/ddk/Kit/List/incl -I $(MACSECHOME)/ddk/Kit/EIP217/src -I $(MACSECHOME)/ddk/Integration/DriverFramework/src -I phymod/include/phymod \
         -I  phymod/chip/evora/tier1 -I $(MACSECHOME)/ddk/Kit/SABuilder_MACsec/incl -I $(MACSECHOME)/ddk/Kit/SABuilder_MACsec/src

# Needed for DDK compilation
PHYMOD_EPIL_INCLUDE_PATH += -I $(MACSECHOME)/ddk/Integration/Adapter_EIP164/src -I $(MACSECHOME)/ddk/Integration/Adapter_EIP164/incl \
         -I $(MACSECHOME)/ddk/Integration/Adapter_EIP163/src -I $(MACSECHOME)/ddk/Integration/Adapter_EIP163/incl \
         -I $(MACSECHOME)/ddk/Integration/Adapter_EIP164/src/common -I $(MACSECHOME)/ddk/Kit/EIP163/src -I $(MACSECHOME)/ddk/Kit/EIP163/incl  -I $(MACSECHOME)/ddk/Kit/EIP164/src -I $(MACSECHOME)/ddk/Kit/EIP164/incl \
         -I $(MACSECHOME)/ddk/Kit/Log/incl -I $(MACSECHOME)/ddk/Kit/List/src -I $(MACSECHOME)/ddk/Kit/EIP201/src -I $(MACSECHOME)/ddk/Kit/EIP201/incl -I $(MACSECHOME)/ddk/Kit/DriverFramework/CLib_Abstraction_API/incl \
         -I $(MACSECHOME)/ddk/Kit/DriverFramework/Basic_Defs_API/incl -I $(MACSECHOME)/ddk/Kit/DriverFramework/Device_API/incl -I bcm_pm_if -I $(MACSECHOME)/bcm_plp_sec \
         -I $(MACSECHOME)/ddk/include -I $(MACSECHOME)/ddk/Kit/List/incl -I $(MACSECHOME)/ddk/Kit/EIP217/src -I $(MACSECHOME)/ddk/Integration/DriverFramework/src -I phymod/include/phymod \
         -I  phymod/chip/miura/tier1 -I $(MACSECHOME)/ddk/Kit/SABuilder_MACsec/incl -I $(MACSECHOME)/ddk/Kit/SABuilder_MACsec/src
endif
# Quadra28 chip support
# APERTA support
PHYMOD_EPIL_INCLUDE_PATH += -I phymod/chip/aperta/aperta_pm/
PHYMOD_EPIL_INCLUDE_PATH += -I phymod/chip/aperta/
# Needed for DDK compilation
PHYMOD_EPIL_INCLUDE_PATH += -I phymod/chip/aperta/tier1

VPATH += phymod/chip/aperta phymod/chip/aperta/tier2 phymod/chip/aperta/tier1 phymod/chip/aperta/aperta_pm phymod/chip/aperta/aperta_pm/pms
VSRCS += $(wildcard phymod/chip/aperta/*.c)
VSRCS += $(wildcard phymod/chip/aperta/tier2/*.c)
VSRCS += $(wildcard phymod/chip/aperta/tier1/*.c)
VSRCS += $(wildcard phymod/chip/aperta/aperta_pm/portmod_dispatch.c)
VSRCS += $(wildcard phymod/chip/aperta/aperta_pm/portmod_enums.c)
#VSRCS += $(wildcard phymod/chip/aperta/aperta_pm/portmod_stat.c)
VSRCS += $(wildcard phymod/chip/aperta/aperta_pm/pms/cdmac.c)
VSRCS += $(wildcard phymod/chip/aperta/aperta_pm/pms/pm8x50.c)
VSRCS += $(wildcard phymod/chip/aperta/aperta_pm/pms/pm8x50_dispatch.c)

#PMD 
PHYMOD_EPIL_INCLUDE_PATH += -I phymod/chip/aperta/aperta_pm/pcs_pmd/include
PHYMOD_EPIL_INCLUDE_PATH += -I phymod/chip/aperta/aperta_pm/pcs_pmd
VPATH += phymod/chip/aperta/aperta_pm/pcs_pmd/blackhawk phymod/chip/aperta/aperta_pm/pcs_pmd/blackhawk/tier2 phymod/chip/aperta/aperta_pm/pcs_pmd/blackhawk/tier1
VSRCS += $(wildcard phymod/chip/aperta/aperta_pm/pcs_pmd/blackhawk/*.c)
VSRCS += $(wildcard phymod/chip/aperta/aperta_pm/pcs_pmd/blackhawk/tier2/*.c)
VSRCS += $(wildcard phymod/chip/aperta/aperta_pm/pcs_pmd/blackhawk/tier1/*.c)

#tscf support
VPATH += phymod/chip/aperta/aperta_pm/pcs_pmd/aperta_tscbh phymod/chip/aperta/aperta_pm/pcs_pmd/aperta_tscbh/tier2 phymod/chip/aperta/aperta_pm/pcs_pmd/aperta_tscbh/tier1
VSRCS += $(wildcard phymod/chip/aperta/aperta_pm/pcs_pmd/aperta_tscbh/*.c)
VSRCS += $(wildcard phymod/chip/aperta/aperta_pm/pcs_pmd/aperta_tscbh/tier2/*.c)
VSRCS += $(wildcard phymod/chip/aperta/aperta_pm/pcs_pmd/aperta_tscbh/tier1/*.c)


# Quadra28 chip support
VPATH += phymod/chip/quadra28 phymod/chip/quadra28/tier2 phymod/chip/quadra28/tier1 phymod/chip/quadra28/tier1/quadra28_serdes
VSRCS += $(wildcard phymod/chip/quadra28/*.c)
VSRCS += $(wildcard phymod/chip/quadra28/tier2/*.c)
VSRCS += $(wildcard phymod/chip/quadra28/tier1/*.c)
VSRCS += $(wildcard phymod/chip/quadra28/tier1/quadra28_serdes/merlin_quadra28_functions.c)
VSRCS += $(wildcard phymod/chip/quadra28/tier1/quadra28_serdes/merlin_quadra28_dependencies.c)
PHYMOD_EPIL_INCLUDE_PATH += -I$(PHYMOD_EPIL)/phymod/chip/quadra28/tier1
# Gallardo28 chip support
ifneq ($(LEGACY_DRIVER_INCLUDE_PATH), )
VPATH += phymod/chip/gallardo28 phymod/chip/gallardo28/tier2 phymod/chip/gallardo28/tier1
VSRCS += $(wildcard phymod/chip/gallardo28/*.c)
VSRCS += $(wildcard phymod/chip/gallardo28/tier2/*.c)
VSRCS += $(wildcard phymod/chip/gallardo28/tier1/*.c)
endif
# Koi chip support
VPATH += phymod/chip/koi phymod/chip/koi/tier2 phymod/chip/koi/tier1 phymod/chip/koi/tier1/legacy
VSRCS += $(wildcard phymod/chip/koi/*.c)
VSRCS += $(wildcard phymod/chip/koi/tier2/*.c)
VSRCS += $(wildcard phymod/chip/koi/tier1/*.c)
VSRCS += $(wildcard phymod/chip/koi/tier1/legacy/*.c)

# Orca chip support
VPATH += phymod/chip/orca phymod/chip/orca/tier2 phymod/chip/orca/tier1 phymod/chip/orca/tier1/serdes/eagle_orca phymod/chip/orca/tier1/legacy \
         phymod/chip/orca/tier1/fwapi/utils phymod/chip/orca/tier1/fwapi/src
VSRCS += $(wildcard phymod/chip/orca/*.c)
VSRCS += $(wildcard phymod/chip/orca/tier2/*.c)
VSRCS += $(wildcard phymod/chip/orca/tier1/*.c)
VSRCS += $(wildcard phymod/chip/orca/tier1/fwapi/src/*.c)
VSRCS += $(wildcard phymod/chip/orca/tier1/fwapi/utils/*.c)
VSRCS += $(wildcard phymod/chip/orca/tier1/serdes/eagle_orca/eagle_phy_1_dependencies.c)
VSRCS += $(wildcard phymod/chip/orca/tier1/serdes/eagle_orca/eagle_phy_1_functions.c)
VSRCS += $(wildcard phymod/chip/orca/tier1/legacy/*.c)

# Shortfin chip support
VPATH += phymod/chip/shortfin phymod/chip/shortfin/tier2  phymod/chip/shortfin/tier1 \
         phymod/chip/shortfin/tier1/fwapi/utils phymod/chip/shortfin/tier1/fwapi/src
##         phymod/chip/shortfin/tier1/serdes/eagle_orca
VSRCS += $(wildcard phymod/chip/shortfin/*.c)
VSRCS += $(wildcard phymod/chip/shortfin/tier2/*.c)
VSRCS += $(wildcard phymod/chip/shortfin/tier1/*.c)
VSRCS += $(wildcard phymod/chip/shortfin/tier1/fwapi/src/*.c)
## VSRCS += $(wildcard phymod/chip/shortfin/tier1/fwapi/utils/*.c)
## VSRCS += $(wildcard phymod/chip/shortfin/tier1/serdes/eagle_orca/eagle_phy_1_functions.c)
## VSRCS += $(wildcard phymod/chip/shortfin/tier1/serdes/eagle_orca/eagle_phy_1_dependencies.c)

# Seahawks chip support
VPATH += phymod/chip/seahawks phymod/chip/seahawks/tier2 phymod/chip/seahawks/tier1 phymod/chip/seahawks/tier1/legacy
VSRCS += $(wildcard phymod/chip/seahawks/*.c)
VSRCS += $(wildcard phymod/chip/seahawks/tier2/*.c)
VSRCS += $(wildcard phymod/chip/seahawks/tier1/*.c)
VSRCS += $(wildcard phymod/chip/seahawks/tier1/legacy/*.c)

#Estoque chip support
VPATH += phymod/chip/estoque phymod/chip/estoque/tier2 phymod/chip/estoque/tier1 phymod/chip/estoque/tier1/blackhawk_estoque phymod/chip/estoque/tier1/blackhawk_estoque/src
VSRCS += $(wildcard phymod/chip/estoque/*.c)
VSRCS += $(wildcard phymod/chip/estoque/tier2/*.c)
VSRCS += $(wildcard phymod/chip/estoque/tier1/*.c)
VSRCS += $(wildcard phymod/chip/estoque/tier1/blackhawk_estoque/src/*.c)
PHYMOD_EPIL_INCLUDE_PATH += -I phymod/chip/estoque/tier1/blackhawk_estoque/inc -I phymod/chip/estoque/tier1

#Millenio chip support
VPATH += phymod/chip/millenio phymod/chip/millenio/tier2 phymod/chip/millenio/tier1 phymod/chip/millenio/tier1/capi
VSRCS += $(wildcard phymod/chip/millenio/*.c)
VSRCS += $(wildcard phymod/chip/millenio/tier2/*.c)
VSRCS += $(wildcard phymod/chip/millenio/tier1/*.c)
VSRCS += $(wildcard phymod/chip/millenio/tier1/capi/*.c)
PHYMOD_EPIL_INCLUDE_PATH += -I phymod/chip/millenio/tier1

#Barchetta chip support
VPATH += phymod/chip/barchetta phymod/chip/barchetta/tier2 phymod/chip/barchetta/tier1 phymod/chip/barchetta/tier1/blackhawk_barchetta phymod/chip/barchetta/tier1/blackhawk_barchetta/src
VSRCS += $(wildcard phymod/chip/barchetta/*.c)
VSRCS += $(wildcard phymod/chip/barchetta/tier2/*.c)
VSRCS += $(wildcard phymod/chip/barchetta/tier1/*.c)
VSRCS += $(wildcard phymod/chip/barchetta/tier1/blackhawk_barchetta/src/*.c)
PHYMOD_EPIL_INCLUDE_PATH += -I phymod/chip/barchetta/tier1/blackhawk_barchetta/include -I phymod/chip/barchetta/tier1

#Barchetta2 chip support
VPATH += phymod/chip/barchetta2 phymod/chip/barchetta2/tier2 phymod/chip/barchetta2/tier1 phymod/chip/barchetta2/tier1/capi/images
VPATH += phymod/chip/barchetta2/tier1/capi/public/common/src phymod/chip/barchetta2/tier1/capi/public/src
VPATH += phymod/chip/barchetta2/tier1/capi/public/line_card/src
VPATH += phymod/chip/barchetta2/tier1/capi/public/chal/serdes_api/src
VPATH += phymod/chip/barchetta2/tier1/capi/public/chal/serdes/src
VPATH += phymod/chip/barchetta2/tier1/capi/public/chal/cw/src
VSRCS += $(wildcard phymod/chip/barchetta2/*.c)
VSRCS += $(wildcard phymod/chip/barchetta2/tier2/*.c)
VSRCS += $(wildcard phymod/chip/barchetta2/tier1/*.c)
VSRCS += $(wildcard phymod/chip/barchetta2/tier1/capi/public/common/src/*.c)
VSRCS += $(wildcard phymod/chip/barchetta2/tier1/capi/public/src/*.c)
VSRCS += $(wildcard phymod/chip/barchetta2/tier1/capi/public/line_card/src/*.c)
VSRCS += $(wildcard phymod/chip/barchetta2/tier1/capi/public/chal/serdes_api/src/*.c)
VSRCS += $(wildcard phymod/chip/barchetta2/tier1/capi/public/chal/serdes/src/*.c)
VSRCS += $(wildcard phymod/chip/barchetta2/tier1/capi/public/chal/cw/src/*.c)
PHYMOD_EPIL_INCLUDE_PATH += -I phymod/chip/barchetta2/tier1 -I phymod/chip/barchetta2/tier1/capi/public/common/inc
PHYMOD_EPIL_INCLUDE_PATH += -I phymod/chip/barchetta2/tier1/capi/images -I phymod/chip/barchetta2/tier1/capi/public/inc
PHYMOD_EPIL_INCLUDE_PATH += -I phymod/chip/barchetta2/tier1/capi/public/chal/cw/inc
PHYMOD_EPIL_INCLUDE_PATH += -I phymod/chip/barchetta2/tier1/capi/public/line_card/inc
PHYMOD_EPIL_INCLUDE_PATH += -I phymod/chip/barchetta2/tier1/capi/public/chal/serdes/inc
PHYMOD_EPIL_INCLUDE_PATH += -I phymod/chip/barchetta2/tier1/capi/public/chal/serdes_api/include
PHYMOD_EPIL_INCLUDE_PATH += -I phymod/chip/barchetta2/tier1/capi/public/chal/serdes_api/include/common
PHYMOD_EPIL_INCLUDE_PATH += -I phymod/chip/barchetta2/tier1/capi/public/chal/serdes_api/include/public

# Broncos chip support
VPATH += phymod/chip/broncos phymod/chip/broncos/tier2 phymod/chip/broncos/tier1 phymod/chip/broncos/tier1/legacy
VSRCS += $(wildcard phymod/chip/broncos/*.c)
VSRCS += $(wildcard phymod/chip/broncos/tier2/*.c)
VSRCS += $(wildcard phymod/chip/broncos/tier1/*.c)
VSRCS += $(wildcard phymod/chip/broncos/tier1/legacy/*.c)

# Ravens chip support
VPATH += phymod/chip/ravens phymod/chip/ravens/tier2 phymod/chip/ravens/tier1 phymod/chip/ravens/tier1/legacy
VSRCS += $(wildcard phymod/chip/ravens/*.c)
VSRCS += $(wildcard phymod/chip/ravens/tier2/*.c)
VSRCS += $(wildcard phymod/chip/ravens/tier1/*.c)
VSRCS += $(wildcard phymod/chip/ravens/tier1/legacy/*.c)

# Giants chip support
VPATH += phymod/chip/giants phymod/chip/giants/tier2 phymod/chip/giants/tier1 phymod/chip/giants/tier1/legacy
VSRCS += $(wildcard phymod/chip/giants/*.c)
VSRCS += $(wildcard phymod/chip/giants/tier2/*.c)
VSRCS += $(wildcard phymod/chip/giants/tier1/*.c)
VSRCS += $(wildcard phymod/chip/giants/tier1/legacy/*.c)

# Blackfin chip support
VPATH += phymod/chip/blackfin phymod/chip/blackfin/tier2  phymod/chip/blackfin/tier1 \
         phymod/chip/blackfin/tier1/fwapi/utils phymod/chip/blackfin/tier1/fwapi/src
##         phymod/chip/blackfin/tier1/serdes/eagle_orca
VSRCS += $(wildcard phymod/chip/blackfin/*.c)
VSRCS += $(wildcard phymod/chip/blackfin/tier2/*.c)
VSRCS += $(wildcard phymod/chip/blackfin/tier1/*.c)
VSRCS += $(wildcard phymod/chip/blackfin/tier1/fwapi/src/*.c)

# Kauai chip support
VPATH += phymod/chip/kauai phymod/chip/kauai/tier2  phymod/chip/kauai/tier1 \
         phymod/chip/kauai/tier1/fwapi/utils phymod/chip/kauai/tier1/fwapi/src
VSRCS += $(wildcard phymod/chip/kauai/*.c)
VSRCS += $(wildcard phymod/chip/kauai/tier2/*.c)
VSRCS += $(wildcard phymod/chip/kauai/tier1/*.c)
VSRCS += $(wildcard phymod/chip/kauai/tier1/fwapi/src/*.c)

# Lanai chip support
VPATH += phymod/chip/lanai phymod/chip/lanai/tier2  phymod/chip/lanai/tier1 \
         phymod/chip/lanai/tier1/fwapi/utils phymod/chip/lanai/tier1/fwapi/src
VSRCS += $(wildcard phymod/chip/lanai/*.c)
VSRCS += $(wildcard phymod/chip/lanai/tier2/*.c)
VSRCS += $(wildcard phymod/chip/lanai/tier1/*.c)
VSRCS += $(wildcard phymod/chip/lanai/tier1/fwapi/src/*.c)

# Broadfin/Longfin/Whitetip Common XLMAC support
VPATH += phymod/common/xlmac
VSRCS += $(wildcard phymod/common/xlmac/*.c)
PHYMOD_EPIL_INCLUDE_PATH += -I$(PHYMOD_EPIL)/phymod/common/xlmac

# Broadfin chip support
VPATH += phymod/chip/broadfin phymod/chip/broadfin/tier2  phymod/chip/broadfin/tier1 \
         phymod/chip/broadfin/tier1/fwapi/utils phymod/chip/broadfin/tier1/fwapi/src
VSRCS += $(wildcard phymod/chip/broadfin/*.c)
VSRCS += $(wildcard phymod/chip/broadfin/tier2/*.c)
VSRCS += $(wildcard phymod/chip/broadfin/tier1/*.c)
VSRCS += $(wildcard phymod/chip/broadfin/tier1/fwapi/src/*.c)
PHYMOD_EPIL_INCLUDE_PATH += -I$(PHYMOD_EPIL)/phymod/chip/broadfin/tier1/fwapi/inc

# Whitetip chip support
VPATH += phymod/chip/whitetip phymod/chip/whitetip/tier2  phymod/chip/whitetip/tier1 \
         phymod/chip/whitetip/tier1/fwapi/utils phymod/chip/whitetip/tier1/fwapi/src
VSRCS += $(wildcard phymod/chip/whitetip/*.c)
VSRCS += $(wildcard phymod/chip/whitetip/tier2/*.c)
VSRCS += $(wildcard phymod/chip/whitetip/tier1/*.c)
VSRCS += $(wildcard phymod/chip/whitetip/tier1/fwapi/src/*.c)
PHYMOD_EPIL_INCLUDE_PATH += -I$(PHYMOD_EPIL)/phymod/chip/whitetip/tier1/fwapi/inc

# Longfin chip support
VPATH += phymod/chip/longfin phymod/chip/longfin/tier2  phymod/chip/longfin/tier1 \
         phymod/chip/longfin/tier1/fwapi/utils phymod/chip/longfin/tier1/fwapi/src
VSRCS += $(wildcard phymod/chip/longfin/*.c)
VSRCS += $(wildcard phymod/chip/longfin/tier2/*.c)
VSRCS += $(wildcard phymod/chip/longfin/tier1/*.c)
VSRCS += $(wildcard phymod/chip/longfin/tier1/fwapi/src/*.c)
PHYMOD_EPIL_INCLUDE_PATH += -I$(PHYMOD_EPIL)/phymod/chip/longfin/tier1/fwapi/inc

# Monza chip support
VPATH += phymod/chip/monza phymod/chip/monza/tier2 phymod/chip/monza/tier1 phymod/chip/monza/tier1/legacy
VSRCS += $(wildcard phymod/chip/monza/*.c)
VSRCS += $(wildcard phymod/chip/monza/tier2/*.c)
VSRCS += $(wildcard phymod/chip/monza/tier1/*.c)

# Rams chip support
VPATH += phymod/chip/rams phymod/chip/rams/tier2 phymod/chip/rams/tier1 phymod/chip/rams/tier1/legacy
VSRCS += $(wildcard phymod/chip/rams/*.c)
VSRCS += $(wildcard phymod/chip/rams/tier2/*.c)
VSRCS += $(wildcard phymod/chip/rams/tier1/*.c)
VSRCS += $(wildcard phymod/chip/rams/tier1/legacy/*.c)

# Cowboys chip support
VPATH += phymod/chip/cowboys phymod/chip/cowboys/tier2 phymod/chip/cowboys/tier1 phymod/chip/cowboys/tier1/legacy
VSRCS += $(wildcard phymod/chip/cowboys/*.c)
VSRCS += $(wildcard phymod/chip/cowboys/tier2/*.c)
VSRCS += $(wildcard phymod/chip/cowboys/tier1/*.c)
VSRCS += $(wildcard phymod/chip/cowboys/tier1/legacy/*.c)
#Agera chip support
VPATH += phymod/chip/agera phymod/chip/agera/tier2 phymod/chip/agera/tier1 phymod/chip/agera/tier1/osprey7_agera phymod/chip/agera/tier1/osprey7_agera/src
VSRCS += $(wildcard phymod/chip/agera/*.c)
VSRCS += $(wildcard phymod/chip/agera/tier2/*.c)
VSRCS += $(wildcard phymod/chip/agera/tier1/*.c)
VSRCS += $(wildcard phymod/chip/agera/tier1/osprey7_agera/src/*.c)
PHYMOD_EPIL_INCLUDE_PATH += -I phymod/chip/agera/tier1/osprey7_agera/include -I phymod/chip/agera/tier1
#Ageralite chip support
VPATH += phymod/chip/ageralite phymod/chip/ageralite/tier2 phymod/chip/ageralite/tier1 phymod/chip/ageralite/tier1/ospreylite7_ageralite phymod/chip/ageralite/tier1/ospreylite7_ageralite/src
VSRCS += $(wildcard phymod/chip/ageralite/*.c)
VSRCS += $(wildcard phymod/chip/ageralite/tier2/*.c)
VSRCS += $(wildcard phymod/chip/ageralite/tier1/*.c)
VSRCS += $(wildcard phymod/chip/ageralite/tier1/ospreylite7_ageralite/src/*.c)
PHYMOD_EPIL_INCLUDE_PATH += -I phymod/chip/ageralite/tier1/ospreylite7_ageralite/include -I phymod/chip/ageralite/tier1

#Agera2 chip support
VPATH += phymod/chip/agera2 phymod/chip/agera2/tier2 phymod/chip/agera2/tier1 phymod/chip/agera2/tier1/peregrine5_agera2 phymod/chip/agera2/tier1/peregrine5_agera2/src
VPATH += phymod/chip/agera2/tier1/fec_diag/chal/CW/src phymod/chip/agera2/tier1/fec_diag/public/src
VSRCS += $(wildcard phymod/chip/agera2/*.c)
VSRCS += $(wildcard phymod/chip/agera2/tier2/*.c)
VSRCS += $(wildcard phymod/chip/agera2/tier1/*.c)
VSRCS += $(wildcard phymod/chip/agera2/tier1/peregrine5_agera2/src/*.c)
VSRCS += $(wildcard phymod/chip/agera2/tier1/fec_diag/chal/CW/src/*.c)
VSRCS += $(wildcard phymod/chip/agera2/tier1/fec_diag/public/src/*.c)
PHYMOD_EPIL_INCLUDE_PATH += -I phymod/chip/agera2/tier1/peregrine5_agera2/include -I phymod/chip/agera2/tier1
PHYMOD_EPIL_INCLUDE_PATH += -I phymod/chip/agera2/tier1/fec_diag/common/inc
PHYMOD_EPIL_INCLUDE_PATH += -I phymod/chip/agera2/tier1/fec_diag/public/inc
PHYMOD_EPIL_INCLUDE_PATH += -I phymod/chip/agera2/tier1/fec_diag/chal/CW/inc

# APERTA2 support
PHYMOD_EPIL_INCLUDE_PATH += -I phymod/chip/aperta2/aperta2_pm/
PHYMOD_EPIL_INCLUDE_PATH += -I phymod/chip/aperta2/
PHYMOD_EPIL_INCLUDE_PATH += -I phymod/chip/aperta2/tier1
PHYMOD_EPIL_INCLUDE_PATH += -I phymod/chip/aperta2/aperta2_pm/pcs_pmd/tscp/tier1

VPATH += phymod/chip/aperta2 phymod/chip/aperta2/tier2 phymod/chip/aperta2/tier1 phymod/chip/aperta2/aperta2_pm phymod/chip/aperta2/aperta2_pm/pms
VSRCS += $(wildcard phymod/chip/aperta2/*.c)
VSRCS += $(wildcard phymod/chip/aperta2/tier2/*.c)
VSRCS += $(wildcard phymod/chip/aperta2/tier1/*.c)
VSRCS += $(wildcard phymod/chip/aperta2/aperta2_pm/pms/dcccport.c)
VSRCS += $(wildcard phymod/chip/aperta2/aperta2_pm/portmod_dispatch.c)
VSRCS += $(wildcard phymod/chip/aperta2/aperta2_pm/portmod_enums.c)
VSRCS += $(wildcard phymod/chip/aperta2/aperta2_pm/pms/pm8x100_gen2.c)
VSRCS += $(wildcard phymod/chip/aperta2/aperta2_pm/pms/pm8x100_gen2_dispatch.c)

#PMD 
PHYMOD_EPIL_INCLUDE_PATH += -I phymod/chip/aperta2/aperta2_pm/pcs_pmd/include
PHYMOD_EPIL_INCLUDE_PATH += -I phymod/chip/aperta2/aperta2_pm/pcs_pmd
PHYMOD_EPIL_INCLUDE_PATH += -I phymod/chip/aperta2/aperta2_pm/pcs_pmd/peregrine5_pc/tier1
VPATH += phymod/chip/aperta2/aperta2_pm/pcs_pmd/peregrine5_pc phymod/chip/aperta2/aperta2_pm/pcs_pmd/peregrine5_pc/tier2 phymod/chip/aperta2/aperta2_pm/pcs_pmd/peregrine5_pc/tier1
VSRCS += $(wildcard phymod/chip/aperta2/aperta2_pm/pcs_pmd/peregrine5_pc/*.c)
VSRCS += $(wildcard phymod/chip/aperta2/aperta2_pm/pcs_pmd/peregrine5_pc/tier2/*.c)
VSRCS += $(wildcard phymod/chip/aperta2/aperta2_pm/pcs_pmd/peregrine5_pc/tier1/*.c)

#tscp support
VPATH += phymod/chip/aperta2/aperta2_pm/pcs_pmd/tscp phymod/chip/aperta2/aperta2_pm/pcs_pmd/tscp/tier2 phymod/chip/aperta2/aperta2_pm/pcs_pmd/tscp/tier1
VSRCS += $(wildcard phymod/chip/aperta2/aperta2_pm/pcs_pmd/tscp/*.c)
VSRCS += $(wildcard phymod/chip/aperta2/aperta2_pm/pcs_pmd/tscp/tier2/*.c)
VSRCS += $(wildcard phymod/chip/aperta2/aperta2_pm/pcs_pmd/tscp/tier1/*.c)

LSRCS = $(notdir $(VSRCS))
LOBJS = $(addsuffix .o, $(basename $(LSRCS)))
BOBJS = $(addprefix $(BLDDIR)/,$(LOBJS))

LIBNAME = libaprt

ifeq ($(MAKECMDGOALS),)
include ${PHYMOD_EPIL}/make/lib.mk
else ifeq ($(MAKECMDGOALS),clean)
include ${PHYMOD_EPIL}/make/lib.mk
endif
