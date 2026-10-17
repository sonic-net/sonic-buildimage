# The Rust pmon daemons: seven Debian packages out of one source build.
#
# Built from the same source tree as the Python daemons: debian/ at the root of
# sonic-platform-daemons drives cargo over the whole workspace, while each
# sonic-<daemon>/setup.py still drives its own wheel.  The two build systems do
# not overlap, and both sets ship -- which is what makes the supervisord switch
# in pmon_daemon_control.json a runtime choice rather than a rebuild.
#
# One source package rather than seven because they share crates/pmon-common
# and the generated facade; seven binary packages because a platform may want
# one daemon's Rust version and not another's.

SONIC_THERMALCTLD_RS = sonic-thermalctld-rs_1.0.0_$(CONFIGURED_ARCH).deb
$(SONIC_THERMALCTLD_RS)_SRC_PATH = $(SRC_PATH)/sonic-platform-daemons
# libswsscommon-dev for the STATE_DB bindings the daemons write through;
# python3-swsscommon is not needed, the Rust crate talks to the C++ library.
$(SONIC_THERMALCTLD_RS)_DEPENDS = $(LIBSWSSCOMMON_DEV) $(LIBSWSSCOMMON)
$(SONIC_THERMALCTLD_RS)_RDEPENDS = $(LIBSWSSCOMMON)
# The workspace takes platform-api, platform-provider and swss-common from git
# at the commits its Cargo.lock pins.  The image builds them from its own
# submodules instead, so the PyO3 bridge matches the sonic-platform-common
# wheel installed beside it and swss-common the libswsscommon it links:
# debian/rules passes this cargo config, whose [patch] entries point at them.
$(SONIC_THERMALCTLD_RS)_BUILD_ENV = SONIC_CARGO_CONFIG=$(CURDIR)/rules/sonic-pmon-daemons-rs.cargo.toml
SONIC_DPKG_DEBS += $(SONIC_THERMALCTLD_RS)

# The other six come out of the same build.  thermalctld is the main package
# only because something has to be; nothing else about it is special.
SONIC_PSUD_RS        = sonic-psud-rs_1.0.0_$(CONFIGURED_ARCH).deb
SONIC_SYSEEPROMD_RS  = sonic-syseepromd-rs_1.0.0_$(CONFIGURED_ARCH).deb
SONIC_PCIED_RS       = sonic-pcied-rs_1.0.0_$(CONFIGURED_ARCH).deb
SONIC_STORMOND_RS    = sonic-stormond-rs_1.0.0_$(CONFIGURED_ARCH).deb
SONIC_CHASSISD_RS    = sonic-chassisd-rs_1.0.0_$(CONFIGURED_ARCH).deb

$(eval $(call add_derived_package,$(SONIC_THERMALCTLD_RS),$(SONIC_PSUD_RS)))
$(eval $(call add_derived_package,$(SONIC_THERMALCTLD_RS),$(SONIC_SYSEEPROMD_RS)))
$(eval $(call add_derived_package,$(SONIC_THERMALCTLD_RS),$(SONIC_PCIED_RS)))
$(eval $(call add_derived_package,$(SONIC_THERMALCTLD_RS),$(SONIC_STORMOND_RS)))
$(eval $(call add_derived_package,$(SONIC_THERMALCTLD_RS),$(SONIC_CHASSISD_RS)))

# The debug symbols debhelper splits out of each binary.  Declared rather than
# left where dpkg-buildpackage dropped them: the main package's recipe moves
# itself and its derived packages into target/debs and nothing else, so an
# undeclared dbgsym stays in src/ and the -dbg image ships without symbols for
# these seven -- which is the one image where that matters.
SONIC_THERMALCTLD_RS_DBG = sonic-thermalctld-rs-dbgsym_1.0.0_$(CONFIGURED_ARCH).deb
$(SONIC_THERMALCTLD_RS_DBG)_DEPENDS  += $(SONIC_THERMALCTLD_RS)
$(SONIC_THERMALCTLD_RS_DBG)_RDEPENDS += $(SONIC_THERMALCTLD_RS)
$(eval $(call add_derived_package,$(SONIC_THERMALCTLD_RS),$(SONIC_THERMALCTLD_RS_DBG)))

SONIC_PSUD_RS_DBG = sonic-psud-rs-dbgsym_1.0.0_$(CONFIGURED_ARCH).deb
$(SONIC_PSUD_RS_DBG)_DEPENDS  += $(SONIC_PSUD_RS)
$(SONIC_PSUD_RS_DBG)_RDEPENDS += $(SONIC_PSUD_RS)
$(eval $(call add_derived_package,$(SONIC_THERMALCTLD_RS),$(SONIC_PSUD_RS_DBG)))


SONIC_SYSEEPROMD_RS_DBG = sonic-syseepromd-rs-dbgsym_1.0.0_$(CONFIGURED_ARCH).deb
$(SONIC_SYSEEPROMD_RS_DBG)_DEPENDS  += $(SONIC_SYSEEPROMD_RS)
$(SONIC_SYSEEPROMD_RS_DBG)_RDEPENDS += $(SONIC_SYSEEPROMD_RS)
$(eval $(call add_derived_package,$(SONIC_THERMALCTLD_RS),$(SONIC_SYSEEPROMD_RS_DBG)))

SONIC_PCIED_RS_DBG = sonic-pcied-rs-dbgsym_1.0.0_$(CONFIGURED_ARCH).deb
$(SONIC_PCIED_RS_DBG)_DEPENDS  += $(SONIC_PCIED_RS)
$(SONIC_PCIED_RS_DBG)_RDEPENDS += $(SONIC_PCIED_RS)
$(eval $(call add_derived_package,$(SONIC_THERMALCTLD_RS),$(SONIC_PCIED_RS_DBG)))

SONIC_STORMOND_RS_DBG = sonic-stormond-rs-dbgsym_1.0.0_$(CONFIGURED_ARCH).deb
$(SONIC_STORMOND_RS_DBG)_DEPENDS  += $(SONIC_STORMOND_RS)
$(SONIC_STORMOND_RS_DBG)_RDEPENDS += $(SONIC_STORMOND_RS)
$(eval $(call add_derived_package,$(SONIC_THERMALCTLD_RS),$(SONIC_STORMOND_RS_DBG)))

SONIC_CHASSISD_RS_DBG = sonic-chassisd-rs-dbgsym_1.0.0_$(CONFIGURED_ARCH).deb
$(SONIC_CHASSISD_RS_DBG)_DEPENDS  += $(SONIC_CHASSISD_RS)
$(SONIC_CHASSISD_RS_DBG)_RDEPENDS += $(SONIC_CHASSISD_RS)
$(eval $(call add_derived_package,$(SONIC_THERMALCTLD_RS),$(SONIC_CHASSISD_RS_DBG)))
