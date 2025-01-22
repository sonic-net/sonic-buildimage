.ONESHELL:
SHELL = /bin/bash
.SHELLFLAGS += -e

MAIN_TARGET = libpam-tacplus_$(PAM_TACPLUS_VERSION)_$(CONFIGURED_ARCH).deb
DERIVED_TARGETS = libtac2_$(PAM_TACPLUS_VERSION)_$(CONFIGURED_ARCH).deb \
		  libtac-dev_$(PAM_TACPLUS_VERSION)_$(CONFIGURED_ARCH).deb

$(addprefix $(DEST)/, $(MAIN_TARGET)): $(DEST)/% :
	# Obtain pam_tacplus
	rm -rf ./pam_tacplus
	git clone https://github.com/jeroennijhof/pam_tacplus.git
	pushd ./pam_tacplus
	git checkout -f v1.4.1

	# Apply patch
	git apply ../0001-Don-t-init-declarations-in-a-for-loop.patch
	git apply ../0002-Fix-libtac2-bin-install-directory-error.patch
	git apply ../0003-Obfuscate-key-before-printing-to-syslog.patch
	git apply ../0004-management-vrf-support.patch
	git apply ../0005-pam-Modify-parsing-of-IP-address-and-port-number-to-.patch
	git apply ../0006-Add-support-for-source-ip-address.patch
	git apply ../0007-Fix-memory-leak-when-parse-configuration.patch
	git apply ../0008-Extract-tacacs-support-functions-into-library.patch
	git apply ../0009-Add-setting-flag-for-authorization-and-accounting.patch
	git apply ../0010-handle-bad-password-set-by-sshd.patch

ifeq ($(CROSS_BUILD_ENVIRON), y)
	dpkg-buildpackage -rfakeroot -b -us -uc -a$(CONFIGURED_ARCH) -Pcross,nocheck -j$(SONIC_CONFIG_MAKE_JOBS) --admindir $(SONIC_DPKG_ADMINDIR)
else
	dpkg-buildpackage -rfakeroot -b -us -uc -j$(SONIC_CONFIG_MAKE_JOBS) --admindir $(SONIC_DPKG_ADMINDIR)
endif
	popd

	mv $(DERIVED_TARGETS) $* $(DEST)/

$(addprefix $(DEST)/, $(DERIVED_TARGETS)): $(DEST)/% : $(DEST)/$(MAIN_TARGET)
