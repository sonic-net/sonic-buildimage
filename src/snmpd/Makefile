SHELL = /bin/bash
.ONESHELL:
.SHELLFLAGS += -e

MAIN_TARGET = libsnmp-base_$(SNMPD_VERSION_FULL)_all.deb
ifneq (,$(findstring 5.9,$(SNMPD_VERSION)))
DERIVED_TARGETS = snmptrapd_$(SNMPD_VERSION_FULL)_$(CONFIGURED_ARCH).deb \
		  snmp_$(SNMPD_VERSION_FULL)_$(CONFIGURED_ARCH).deb \
		  snmpd_$(SNMPD_VERSION_FULL)_$(CONFIGURED_ARCH).deb \
		  snmp-dbgsym_$(SNMPD_VERSION_FULL)_$(CONFIGURED_ARCH).deb \
		  snmpd-dbgsym_$(SNMPD_VERSION_FULL)_$(CONFIGURED_ARCH).deb \
		  libsnmp40_$(SNMPD_VERSION_FULL)_$(CONFIGURED_ARCH).deb \
		  libsnmp40-dbgsym_$(SNMPD_VERSION_FULL)_$(CONFIGURED_ARCH).deb \
		  libsnmp-dev_$(SNMPD_VERSION_FULL)_$(CONFIGURED_ARCH).deb \
		  libsnmp-perl_$(SNMPD_VERSION_FULL)_$(CONFIGURED_ARCH).deb \
		  tkmib_$(SNMPD_VERSION_FULL)_all.deb
else
DERIVED_TARGETS = snmptrapd_$(SNMPD_VERSION_FULL)_$(CONFIGURED_ARCH).deb \
		  snmp_$(SNMPD_VERSION_FULL)_$(CONFIGURED_ARCH).deb \
		  snmpd_$(SNMPD_VERSION_FULL)_$(CONFIGURED_ARCH).deb \
		  snmp-dbgsym_$(SNMPD_VERSION_FULL)_$(CONFIGURED_ARCH).deb \
		  snmpd-dbgsym_$(SNMPD_VERSION_FULL)_$(CONFIGURED_ARCH).deb \
		  libsnmp30_$(SNMPD_VERSION_FULL)_$(CONFIGURED_ARCH).deb \
		  libsnmp30-dbg_$(SNMPD_VERSION_FULL)_$(CONFIGURED_ARCH).deb \
		  libsnmp-dev_$(SNMPD_VERSION_FULL)_$(CONFIGURED_ARCH).deb \
		  libsnmp-perl_$(SNMPD_VERSION_FULL)_$(CONFIGURED_ARCH).deb \
		  tkmib_$(SNMPD_VERSION_FULL)_all.deb
endif

$(addprefix $(DEST)/, $(MAIN_TARGET)): $(DEST)/% :
	rm -rf net-snmp-$(SNMPD_VERSION)

	# download debian net-snmp
	dget -u http://archive.debian.org/debian/pool/main/n/net-snmp/net-snmp_$(SNMPD_VERSION_FULL).dsc

	pushd net-snmp-$(SNMPD_VERSION)
	git init
	git add -f *
	git commit -m "unmodified snmpd source"

	# Apply patches
	stg init
	stg import -s ../patch-$(SNMPD_VERSION)/series

	dpkg-buildpackage -rfakeroot -b -d -us -uc -j1 --admindir $(SONIC_DPKG_ADMINDIR)
	popd

	mv $(DERIVED_TARGETS) $* $(DEST)/

$(addprefix $(DEST)/, $(DERIVED_TARGETS)): $(DEST)/% : $(DEST)/$(MAIN_TARGET)
