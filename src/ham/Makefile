.ONESHELL:
.SHELLFLAGS += -e

TOPDIR   := $(abspath .)
INSTALL  := /usr/bin/install
SUB-DIRS := $(dir $(wildcard $(addsuffix /Makefile,$(shell find * -maxdepth 0 -type d | sort ))))


.PHONY: all
all:
	@for dir in ${SUB-DIRS}; do \
       make -C $${dir}; \
    done


.PHONY: install
install:
	rsync --archive --verbose --no-owner --no-group $(TOPDIR)/hamd/etc $(DESTDIR)
	rsync --archive --verbose --no-owner --no-group $(TOPDIR)/libham/usr $(DESTDIR)

    # Create destination directories
	$(INSTALL) -d $(DESTDIR)/usr/sbin/
	$(INSTALL) -d $(DESTDIR)/usr/bin/
	$(INSTALL) -d $(DESTDIR)/lib/x86_64-linux-gnu/       # NSS libraries MUST be under /lib and NOT /usr/lib
	$(INSTALL) -d $(DESTDIR)/usr/lib/x86_64-linux-gnu/

    # Copy files over to newly created directories
	$(INSTALL) -D $(TOPDIR)/hamd/hamd                  $(DESTDIR)/usr/sbin/.
	$(INSTALL) -D $(TOPDIR)/hamctl/hamctl              $(DESTDIR)/usr/bin/.
	$(INSTALL) -D $(TOPDIR)/libham/libham.so           $(DESTDIR)/usr/lib/x86_64-linux-gnu/.
	$(INSTALL) -D $(TOPDIR)/libnss_ham/libnss_ham.so.2 $(DESTDIR)/lib/x86_64-linux-gnu/.
	$(INSTALL) -D $(TOPDIR)/libnss_sac/libnss_sac.so.2 $(DESTDIR)/lib/x86_64-linux-gnu/.


.PHONY: clean
clean:
	@for dir in ${SUB-DIRS}; do \
       make -C $${dir} clean; \
    done

.PHONY: cleanall
cleanall:
	git clean -fdx

