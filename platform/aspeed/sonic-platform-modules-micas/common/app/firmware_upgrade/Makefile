SUBDIRS = firmware_upgrade

all:
	@for dir in $(SUBDIRS); do \
		$(MAKE) -C $$dir || exit 1; \
	done

install:
	@for dir in $(SUBDIRS); do \
		$(MAKE) -C $$dir install DESTDIR=$(DESTDIR) || exit 1; \
	done

clean:
	@for dir in $(SUBDIRS); do \
		$(MAKE) -C $$dir clean || exit 1; \
	done
