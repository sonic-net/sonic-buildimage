# OR-Tools Package Makefile
# Handles download, build, and packaging

SHELL = /bin/bash
.ONESHELL:
.SHELLFLAGS += -e

OR_TOOLS_SRC_DIR = or-tools-$(OR_TOOLS_VERSION)
OR_TOOLS_TARBALL = v$(OR_TOOLS_VERSION).tar.gz
OR_TOOLS_URL = https://github.com/google/or-tools/archive/refs/tags/$(OR_TOOLS_TARBALL)

MAIN_TARGET = libortools_$(OR_TOOLS_VERSION_FULL)_$(CONFIGURED_ARCH).deb

$(addprefix $(DEST)/, $(MAIN_TARGET)): $(DEST)/% :
	@echo "=== Downloading OR-Tools source ==="
	rm -rf $(OR_TOOLS_SRC_DIR) $(OR_TOOLS_TARBALL)
	wget $(OR_TOOLS_URL)
	tar -xzf $(OR_TOOLS_TARBALL)
	rm -f $(OR_TOOLS_TARBALL)
	
	@echo "=== Building OR-Tools ==="
	pushd $(OR_TOOLS_SRC_DIR)
	cmake -S . -B build -DBUILD_DEPS=ON -DBUILD_SHARED_LIBS=ON -DBUILD_EXAMPLES=OFF \
		-DBUILD_TESTING=OFF -DBUILD_TOOLS=OFF -DCMAKE_BUILD_TYPE=Release \
		-DCMAKE_CXX_FLAGS="-Wno-deprecated-declarations -D_GLIBCXX_USE_CXX11_ABI=0" \
		-DCPACK_PACKAGE_NAME=libortools \
		-DCPACK_PACKAGE_VERSION=$(OR_TOOLS_VERSION) \
		-DCPACK_DEBIAN_PACKAGE_RELEASE=1 \
		-DCPACK_DEBIAN_PACKAGE_ARCHITECTURE=$(CONFIGURED_ARCH) \
		-DCPACK_DEBIAN_PACKAGE_SHLIBDEPS=ON \
		-DCPACK_DEBIAN_PACKAGE_MAINTAINER="Pavan Naregundi <pnaregundi@marvell.com>" \
		-DCPACK_PACKAGE_DESCRIPTION_SUMMARY="Google OR-Tools optimization library" \
		-DCPACK_PACKAGE_DESCRIPTION="Google OR-Tools optimization library" \
		-DCPACK_DEBIAN_FILE_NAME=DEB-DEFAULT
	cmake --build build --target ortools -j$(SONIC_CONFIG_MAKE_JOBS)
	
	pushd build

	@echo "=== Preparing release files (libraries only) ==="
	mkdir -p install_release/usr/lib
	cp -P lib/libortools* install_release/usr/lib/
	
	@echo "=== Creating debian package with CPack ==="
	cpack -G DEB \
		-D CPACK_INSTALL_CMAKE_PROJECTS="" \
		-D CPACK_INSTALLED_DIRECTORIES="`pwd`/install_release/usr;/usr" \
		-D CPACK_PACKAGING_INSTALL_PREFIX=""
	popd
	popd
	
	# Move the newly-built .deb package to the destination directory
	mv $(OR_TOOLS_SRC_DIR)/build/libortools_$(OR_TOOLS_VERSION)-1_$(CONFIGURED_ARCH).deb $(DEST)/$*
