$(LIBSAIREDIS)_DEB_BUILD_PROFILES += syncd vs

SYNCD_VS = syncd-vs_1.0.0_amd64.deb
$(SYNCD_VS)_RDEPENDS += $(LIBSAIREDIS) $(LIBSAIMETADATA) $(LIBSAIVS)
$(eval $(call add_derived_package,$(LIBSAIREDIS),$(SYNCD_VS)))

SYNCD_VS_DBGSYM = syncd-vs-dbgsym_1.0.0_amd64.deb
$(SYNCD_VS_DBGSYM)_DEPENDS += $(SYNCD_VS)
$(SYNCD_VS_DBGSYM)_RDEPENDS += $(SYNCD_VS)
$(eval $(call add_derived_package,$(LIBSAIREDIS),$(SYNCD_VS_DBGSYM)))
