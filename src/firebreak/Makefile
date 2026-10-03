# Build sonic-firebreak_1.4_all.deb with dpkg-deb. No debhelper.
# Native package version (no Debian revision) matches debian/source/format.
PKG := sonic-firebreak
VER := 1.4
DEB := $(PKG)_$(VER)_all.deb
STAGE := debbuild

.PHONY: all deb clean

all: deb

deb:
	rm -rf $(STAGE)
	install -d $(STAGE)/DEBIAN \
		$(STAGE)/opt/firebreak \
		$(STAGE)/opt/firebreak/cli \
		$(STAGE)/etc/sonic/firebreak \
		$(STAGE)/lib/systemd/system
	cp -a firebreakd redis_proxy bpf config $(STAGE)/opt/firebreak/
	install -m 0644 cli/show_firebreak.py $(STAGE)/opt/firebreak/cli/show_firebreak.py
	install -m 0644 cli/show_firebreak_plugin.py $(STAGE)/opt/firebreak/cli/show_firebreak_plugin.py
	install -m 0644 cli/config_firebreak.py $(STAGE)/opt/firebreak/cli/config_firebreak.py
	install -m 0644 cli/config_firebreak_plugin.py $(STAGE)/opt/firebreak/cli/config_firebreak_plugin.py
	chmod 755 $(STAGE)/opt/firebreak/firebreakd/firebreakd.py \
		$(STAGE)/opt/firebreak/redis_proxy/redis_proxy.py \
		$(STAGE)/opt/firebreak/bpf/connect4_loader.py
	install -m 0644 config/*.json $(STAGE)/etc/sonic/firebreak/
	install -m 0644 firebreak.service $(STAGE)/lib/systemd/system/firebreak.service
	install -m 0644 debian/control.binary $(STAGE)/DEBIAN/control
	install -m 0644 debian/conffiles $(STAGE)/DEBIAN/conffiles
	install -m 0755 debian/postinst $(STAGE)/DEBIAN/postinst
	install -m 0755 debian/prerm $(STAGE)/DEBIAN/prerm
	if dpkg-deb --help 2>/dev/null | grep -q root-owner-group; then \
		dpkg-deb --root-owner-group --build $(STAGE) $(DEB); \
	else \
		dpkg-deb --build $(STAGE) $(DEB); \
	fi
	rm -rf $(STAGE)

clean:
	rm -rf $(STAGE) $(DEB)
