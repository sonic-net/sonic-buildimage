$(LIBSAIREDIS)_DEB_BUILD_PROFILES += syncd vs

SYNCD_VS = syncd-vs_1.0.0_$(CONFIGURED_ARCH).deb
$(SYNCD_VS)_RDEPENDS += $(LIBSAIREDIS) $(LIBSAIMETADATA) $(LIBSAIVS)
$(eval $(call add_derived_package,$(LIBSAIREDIS),$(SYNCD_VS)))

SYNCD_VS_DBG = syncd-vs-dbgsym_1.0.0_$(CONFIGURED_ARCH).deb
$(SYNCD_VS_DBG)_DEPENDS += $(SYNCD_VS)
$(SYNCD_VS_DBG)_RDEPENDS += $(SYNCD_VS)
$(eval $(call add_derived_package,$(LIBSAIREDIS),$(SYNCD_VS_DBG)))
