.ONESHELL:
SHELL = /bin/bash
.SHELLFLAGS += -e

MAIN_TARGET = libevent_$(LIBEVENT_VERSION)_$(CONFIGURED_ARCH).deb

$(addprefix $(DEST)/, $(MAIN_TARGET)): $(DEST)/% :
	# Obtaining the libevent
	rm -rf ./libevent-$(LIBEVENT_VERSION)-stable
	wget -N "https://github.com/libevent/libevent/releases/download/release-2.1.8-stable/libevent-2.1.8-stable.tar.gz"
	tar -xzf libevent-$(LIBEVENT_VERSION)-stable.tar.gz
	rm -rf libevent-$(LIBEVENT_VERSION)-stable.tar.gz 
	cp debian libevent-$(LIBEVENT_VERSION)-stable -r

	pushd ./libevent-$(LIBEVENT_VERSION)-stable
	dpkg-buildpackage -rfakeroot -b -us -uc  -j$(SONIC_CONFIG_MAKE_JOBS)
	popd
	mv $(MAIN_TARGET) $(DEST)/

#$(addprefix $(DEST)/, $(DERIVED_TARGETS)): $(DEST)/% : $(DEST)/$(MAIN_TARGET)
