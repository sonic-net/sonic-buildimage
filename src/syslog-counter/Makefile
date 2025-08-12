.ONESHELL:
SHELL = /bin/bash
.SHELLFLAGS += -e

MAIN_TARGET = syslog-counter_$(SYSLOG_COUNTER_VERSION)_$(CONFIGURED_ARCH).deb

$(addprefix $(DEST)/, $(MAIN_TARGET)): $(DEST)/% :
	# build debian package with cargo deb
	cargo install cargo-deb
    # sonic install rust and cargo with rustup, so can't build debian package with dpkg-buildpackage
	cargo test
	cargo deb
	
	mv ./target/debian/$(MAIN_TARGET) $(DEST)/
