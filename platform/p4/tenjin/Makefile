SHELL = /bin/bash
.ONESHELL:

TENJIN_VERSION = 1.1.1
TENJIN_VERSION_FULL = $(TENJIN_VERSION)-1

MAIN_TARGET = python-tenjin_$(TENJIN_VERSION_FULL)_all.deb

$(addprefix $(DEST)/, $(MAIN_TARGET)): $(DEST)/% :
	rm -rf Tenjin-$(TENJIN_VERSION)

	wget -nc https://pypi.python.org/packages/source/T/Tenjin/Tenjin-$(TENJIN_VERSION).tar.gz
	tar xzf Tenjin-$(TENJIN_VERSION).tar.gz

	pushd Tenjin-$(TENJIN_VERSION)
	python setup.py --command-packages=stdeb.command bdist_deb
	popd

	mv Tenjin-$(TENJIN_VERSION)/deb_dist/$* $(DEST)/
