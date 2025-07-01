$(LIBSAIREDIS)_DEB_BUILD_PROFILES += syncd vs

SYNCD_VS = syncd-vs_1.0.0_$(CONFIGURED_ARCH).deb
$(SYNCD_VS)_RDEPENDS += $(LIBSAIREDIS) $(LIBSAIMETADATA) $(LIBSAIVS)

ifeq ($(BLDENV),bookworm)
# dash-sai only support sonic-vs.img.gz. it don't support docker-sonic-vs.gz
ifeq ($(findstring docker-sonic-vs, $(SONIC_BUILD_TARGET)), )
  $(LIBSAIREDIS)_DEB_BUILD_PROFILES += dashsai
  $(LIBSAIREDIS)_DEPENDS += $(DASH_SAI)
  $(SYNCD_VS)_RDEPENDS += $(DASH_SAI)
endif
else
  $(warning DASH_SAI cannot support this build environment $(BLDENV))
endif


$(eval $(call add_derived_package,$(LIBSAIREDIS),$(SYNCD_VS)))

SYNCD_VS_DBG = syncd-vs-dbgsym_1.0.0_$(CONFIGURED_ARCH).deb
$(SYNCD_VS_DBG)_DEPENDS += $(SYNCD_VS)
$(SYNCD_VS_DBG)_RDEPENDS += $(SYNCD_VS)
$(eval $(call add_derived_package,$(LIBSAIREDIS),$(SYNCD_VS_DBG)))
