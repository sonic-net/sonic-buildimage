$(LIBSAIREDIS)_DEB_BUILD_PROFILES += syncd vs

SYNCD_VS = syncd-vs_1.0.0_$(CONFIGURED_ARCH).deb
$(SYNCD_VS)_RDEPENDS += $(LIBSAIREDIS) $(LIBSAIMETADATA) $(LIBSAIVS)

# No DASH SAI on the LX2160ARDB: SAI-VS emulates a switch ASIC here, not a
# DPU. DASH pulls in p4lang (pi/bmv2/p4c), whose gRPC build dependencies the
# sonic-slave image only installs on amd64.

$(eval $(call add_derived_package,$(LIBSAIREDIS),$(SYNCD_VS)))

SYNCD_VS_DBG = syncd-vs-dbgsym_1.0.0_$(CONFIGURED_ARCH).deb
$(SYNCD_VS_DBG)_DEPENDS += $(SYNCD_VS)
$(SYNCD_VS_DBG)_RDEPENDS += $(SYNCD_VS)
$(eval $(call add_derived_package,$(LIBSAIREDIS),$(SYNCD_VS_DBG)))
