# OR-Tools Package Makefile
# Handles download, build, and packaging

OR_TOOLS_SRC_DIR = or-tools-$(OR_TOOLS_VERSION)
OR_TOOLS_TARBALL = v$(OR_TOOLS_VERSION).tar.gz
OR_TOOLS_URL = https://github.com/google/or-tools/archive/refs/tags/$(OR_TOOLS_TARBALL)

# Download source if not present
$(OR_TOOLS_SRC_DIR)/CMakeLists.txt:
	@echo "=== Downloading OR-Tools source ==="
	rm -rf $(OR_TOOLS_SRC_DIR) $(OR_TOOLS_TARBALL)
	wget $(OR_TOOLS_URL)
	tar -xzf $(OR_TOOLS_TARBALL)
	rm -f $(OR_TOOLS_TARBALL)

# Build OR-Tools libraries
$(OR_TOOLS_SRC_DIR)/build/lib/libortools.so: $(OR_TOOLS_SRC_DIR)/CMakeLists.txt
	@echo "=== Building OR-Tools ==="
	cd $(OR_TOOLS_SRC_DIR) && \
	mkdir -p build && \
	cd build && \
	cmake .. -DBUILD_DEPS=ON -DBUILD_SHARED_LIBS=ON -DBUILD_EXAMPLES=OFF \
		-DBUILD_TESTING=OFF -DBUILD_TOOLS=OFF -DCMAKE_BUILD_TYPE=Release \
		-DCMAKE_CXX_FLAGS="-Wno-deprecated-declarations -D_GLIBCXX_USE_CXX11_ABI=0" && \
	make -j$(shell nproc) ortools

# Phony target for debian/rules to build the library
library: $(OR_TOOLS_SRC_DIR)/build/lib/libortools.so

# Clean target
clean:
	rm -rf $(OR_TOOLS_SRC_DIR) $(OR_TOOLS_TARBALL)
	rm -rf debian/.debhelper debian/tmp debian/*-stamp debian/libortools
	rm -f debian/files debian/libortools.substvars
	dh_clean || true

.PHONY: all library clean
