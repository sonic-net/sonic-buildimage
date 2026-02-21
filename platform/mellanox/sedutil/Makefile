#
# SPDX-FileCopyrightText: NVIDIA CORPORATION & AFFILIATES
# Copyright (c) 2025-2026 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
# SPDX-License-Identifier: Apache-2.0
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

.ONESHELL:
SHELL = /bin/bash
.SHELLFLAGS += -ex

MAIN_TARGET = sedutil_$(SEDUTIL_VERSION)_$(CONFIGURED_ARCH).deb

$(addprefix $(DEST)/, $(MAIN_TARGET)): $(DEST)/% :
	# Download pre-built sedutil binary from GitHub release
	wget $(SEDUTIL_GITHUB_URL)/releases/download/$(SEDUTIL_VERSION)/sedutil-cli-$(SEDUTIL_VERSION).zip
	unzip sedutil-cli-$(SEDUTIL_VERSION).zip

	# Create debian package structure
	mkdir -p debian-pkg/DEBIAN debian-pkg/usr/local/bin
	cp sedutil-cli debian-pkg/usr/local/bin/
	chmod 755 debian-pkg/usr/local/bin/sedutil-cli

	# Create control file
	echo "Package: sedutil" > debian-pkg/DEBIAN/control
	echo "Version: $(SEDUTIL_VERSION)" >> debian-pkg/DEBIAN/control
	echo "Architecture: $(CONFIGURED_ARCH)" >> debian-pkg/DEBIAN/control
	echo "Maintainer: SONiC Team" >> debian-pkg/DEBIAN/control
	echo "Description: SED management utility" >> debian-pkg/DEBIAN/control

	# Build the Debian package
	dpkg-deb --build debian-pkg $*
	mv $* $(DEST)/

	# Cleanup
	rm -rf debian-pkg sedutil-cli*
