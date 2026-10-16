GO ?= /usr/local/go/bin/go
ifeq ($(wildcard $(GO)),)
GO := go
endif

BUILD_DIR := build/bin
# gribigo patches, in the order they apply.
PATCHES := patches/gribigo-vrf-hook.patch patches/gribigo-rib-add-o1.patch
VENDOR_STAMP := vendor/.patched

# Static and pure Go: the binary runs in a container with no Go toolchain or
# swsscommon, and talks to orchagent over its ZeroMQ socket directly.
export CGO_ENABLED := 0
export GOFLAGS += -mod=vendor

# Private, writable caches: the image build runs Go packages in parallel under
# one $HOME, and sonic-restapi's clean removes $HOME/go outright, which fails
# on (and races with) Go's default read-only module cache. A dot directory,
# so ./... patterns do not descend into the cached modules.
export GOMODCACHE := $(CURDIR)/.gocache/mod
export GOCACHE := $(CURDIR)/.gocache/build

.PHONY: all check install clean

all: $(BUILD_DIR)/gribid

# Patches are applied to vendor/ rather than to the module cache, so every
# build and test runs the patched dependencies.
$(VENDOR_STAMP): go.mod go.sum $(PATCHES)
	rm -rf vendor
	GOFLAGS=-modcacherw $(GO) mod vendor
	for p in $(PATCHES); do patch -d vendor/github.com/openconfig/gribigo -p1 < $$p || exit 1; done
	touch $@

$(BUILD_DIR)/gribid: $(VENDOR_STAMP) $(shell find . -name '*.go' -not -path './vendor/*' -not -path './.gocache/*')
	$(GO) build -trimpath -o $@ ./cmd/gribid

check: $(VENDOR_STAMP)
	CGO_ENABLED=1 $(GO) test -race ./...

install: $(BUILD_DIR)/gribid
	install -D $(BUILD_DIR)/gribid $(DESTDIR)/usr/sbin/gribid

clean:
	rm -rf build vendor .gocache
