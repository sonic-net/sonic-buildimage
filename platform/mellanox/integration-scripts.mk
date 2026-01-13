#
# Copyright (c) 2016-2023 NVIDIA CORPORATION & AFFILIATES.
# Apache-2.0
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
# Mellanox Integration Scripts

# override this for other branches
BRANCH_SONIC = master
# set this flag to y to create a branch instead of commit
CREATE_BRANCH = n

TEMP_HW_MGMT_DIR = /tmp/hw_mgmt
PTCH_DIR = $(TEMP_HW_MGMT_DIR)/patch_dir/
NON_UP_PTCH_DIR = $(TEMP_HW_MGMT_DIR)/non_up_patch_dir/
PTCH_LIST  = $(TEMP_HW_MGMT_DIR)/series
HWMGMT_NONUP_LIST = $(BUILD_WORKDIR)/$($(MLNX_HW_MANAGEMENT)_SRC_PATH)/hwmgmt_nonup_patches
HWMGMT_USER_OUTFILE = $(BUILD_WORKDIR)/integrate-mlnx-hw-mgmt_user.out
SDK_USER_OUTFILE = $(BUILD_WORKDIR)/integrate-mlnx-sdk_user.out
TMPFILE_OUT := $(shell mktemp)
SDK_TMPDIR := $(shell mktemp -d)
SB_COM_MSG := $(shell mktemp -t sb_commit_msg_file_XXXXX.log)
SLK_COM_MSG := $(shell mktemp -t slk_commit_msg_file_XXXXX.log)
SB_HEAD = $(shell git rev-parse --short HEAD)
SLK_HEAD = $(shell cd src/sonic-linux-kernel; git rev-parse --short HEAD)

# kconfig related variables
KCFG_BASE_TMPDIR = $(TEMP_HW_MGMT_DIR)/linux_kconfig/
KCFG_BASE = $(KCFG_BASE_TMPDIR)/amd64.config
KCFG_LIST = $(TEMP_HW_MGMT_DIR)/kconfig_amd64
KCFG_DOWN_LIST = $(TEMP_HW_MGMT_DIR)/kconfig_downstream_amd64
KCFG_BASE_ARM = $(KCFG_BASE_TMPDIR)/arm64.config
KCFG_LIST_ARM = $(TEMP_HW_MGMT_DIR)/kconfig_arm64
KCFG_DOWN_LIST_ARM = $(TEMP_HW_MGMT_DIR)/kconfig_downstream_arm64


integrate-mlnx-hw-mgmt:
	$(FLUSH_LOG)
	rm -rf $(TEMP_HW_MGMT_DIR) $(TMPFILE_OUT)
	mkdir -p $(PTCH_DIR) $(NON_UP_PTCH_DIR) $(KCFG_BASE_TMPDIR)
	touch $(PTCH_LIST) $(KCFG_LIST) $(KCFG_DOWN_LIST) $(KCFG_LIST_ARM) $(KCFG_DOWN_LIST_ARM)

	# Fetch the vanilla .config files
	pushd $(KCFG_BASE_TMPDIR) $(LOG_SIMPLE)
	rm -rf linux/; mkdir linux
	# Note: gregkh is the stable linux mirror
	git clone --depth 1 --branch v$(KERNEL_VERSION) https://github.com/gregkh/linux.git linux $(LOG_SIMPLE)

	pushd linux
	rm -rf .config; make ARCH=x86_64 defconfig; cp -f .config $(KCFG_BASE) $(LOG_SIMPLE)
	rm -rf .config; make ARCH=arm64 defconfig; cp -f .config $(KCFG_BASE_ARM) $(LOG_SIMPLE)
	popd
	popd $(LOG_SIMPLE)

	# clean up existing untracked files
	pushd $(BUILD_WORKDIR); git clean -f -- platform/mellanox/
ifeq ($(CREATE_BRANCH), y)
	git checkout -B "$(BRANCH_SONIC)_$(SB_HEAD)_integrate_$(MLNX_HW_MANAGEMENT_VERSION)" HEAD
	echo $(BRANCH_SONIC)_$(SB_HEAD)_integrate_$(MLNX_HW_MANAGEMENT_VERSION) branch created in sonic-buildimage
endif
	popd

	pushd $(BUILD_WORKDIR)/src/sonic-linux-kernel; git clean -f -- patch/
ifeq ($(CREATE_BRANCH), y)
	git checkout -B "$(BRANCH_SONIC)_$(SLK_HEAD)_integrate_$(MLNX_HW_MANAGEMENT_VERSION)" HEAD
	echo $(BRANCH_SONIC)_$(SLK_HEAD)_integrate_$(MLNX_HW_MANAGEMENT_VERSION) branch created in sonic-linux-kernel
endif
	popd

	echo "#### Integrate HW-MGMT $(MLNX_HW_MANAGEMENT_VERSION) Kernel Patches into SONiC" > ${HWMGMT_USER_OUTFILE}
	pushd $(BUILD_WORKDIR)/$(PLATFORM_PATH) $(LOG_SIMPLE)

	# Run tests
	pushd integration-scripts/tests; pytest-3 -v; popd

	# Checkout to the corresponding hw-mgmt version and update mk file
	pushd hw-management/hw-mgmt; git checkout V.${MLNX_HW_MANAGEMENT_VERSION}; popd
	sed -i "s/\(^MLNX_HW_MANAGEMENT_VERSION = \).*/\1${MLNX_HW_MANAGEMENT_VERSION}/g" hw-management.mk

	# Pre-processing before runing hw_mgmt script
	integration-scripts/hwmgmt_kernel_patches.py pre \
							--config_base_amd $(KCFG_BASE) \
							--config_base_arm $(KCFG_BASE_ARM) \
							--config_inc_amd $(KCFG_LIST) \
							--config_inc_arm $(KCFG_LIST_ARM) \
							--build_root $(BUILD_WORKDIR) \
							--kernel_version $(KERNEL_VERSION) \
							--hw_mgmt_ver ${MLNX_HW_MANAGEMENT_VERSION} $(LOG_SIMPLE)

	# Disable Writing KConfigs for arm64 platform
	# $(BUILD_WORKDIR)/$($(MLNX_HW_MANAGEMENT)_SRC_PATH)/hw-mgmt/recipes-kernel/linux/deploy_kernel_patches.py \
	# 						--dst_accepted_folder $(PTCH_DIR) \
	# 						--dst_candidate_folder $(NON_UP_PTCH_DIR) \
	# 						--series_file $(PTCH_LIST) \
	# 						--config_file $(KCFG_LIST_ARM) \
	# 						--config_file_downstream $(KCFG_DOWN_LIST_ARM) \
	# 						--kernel_version $(KERNEL_VERSION) \
	# 						--arch arm64 \
	# 						--os_type sonic $(LOG_SIMPLE)
	
	$(BUILD_WORKDIR)/$($(MLNX_HW_MANAGEMENT)_SRC_PATH)/hw-mgmt/recipes-kernel/linux/deploy_kernel_patches.py \
							--dst_accepted_folder $(PTCH_DIR) \
							--dst_candidate_folder $(NON_UP_PTCH_DIR) \
							--series_file $(PTCH_LIST) \
							--config_file $(KCFG_LIST) \
							--config_file_downstream $(KCFG_DOWN_LIST) \
							--kernel_version $(KERNEL_VERSION) \
							--arch amd64 \
							--os_type sonic $(LOG_SIMPLE)

	# Post-processing
	integration-scripts/hwmgmt_kernel_patches.py post \
							--patches $(PTCH_DIR) \
							--non_up_patches $(NON_UP_PTCH_DIR) \
							--kernel_version $(KERNEL_VERSION) \
							--hw_mgmt_ver ${MLNX_HW_MANAGEMENT_VERSION} \
							--config_base_amd $(KCFG_BASE) \
							--config_base_arm $(KCFG_BASE_ARM) \
							--config_inc_amd $(KCFG_LIST) \
							--config_inc_arm $(KCFG_LIST_ARM) \
							--config_inc_down_amd $(KCFG_DOWN_LIST) \
							--config_inc_down_arm $(KCFG_DOWN_LIST_ARM) \
							--series $(PTCH_LIST) \
							--current_non_up_patches $(HWMGMT_NONUP_LIST) \
							--build_root $(BUILD_WORKDIR) \
							--sb_msg $(SB_COM_MSG) \
							--slk_msg $(SLK_COM_MSG) $(LOG_SIMPLE)
	
	# Commit the changes in linux kernel and and log the diff
	pushd $(BUILD_WORKDIR)/src/sonic-linux-kernel
	git add -- patches-sonic/
	git add -- config.local/ 

	echo -en "\n###-> series file changes in sonic-linux-kernel <-###\n" >> ${HWMGMT_USER_OUTFILE}
	git diff --no-color --staged -- patches-sonic/series >> ${HWMGMT_USER_OUTFILE}

	echo -en "\n###-> Common config changes in sonic-linux-kernel <-###\n" >> ${HWMGMT_USER_OUTFILE}
	git diff --no-color --staged -- config.local/featureset-sonic/config >> ${HWMGMT_USER_OUTFILE}

	echo -en "\n###-> AMD64 config changes in sonic-linux-kernel <-###\n" >> ${HWMGMT_USER_OUTFILE}
	git diff --no-color --staged -- config.local/amd64/config.sonic >> ${HWMGMT_USER_OUTFILE}

	echo -en "\n###-> ARM64 config changes in sonic-linux-kernel <-###\n" >> ${HWMGMT_USER_OUTFILE}
	git diff --no-color --staged -- config.local/arm64/config.sonic-mellanox >> ${HWMGMT_USER_OUTFILE}

	echo -en '\n###-> Summary of files updated in sonic-linux-kernel <-###\n' >> ${HWMGMT_USER_OUTFILE}
	git diff --no-color --staged --stat --output=${TMPFILE_OUT}
	cat ${TMPFILE_OUT} | tee -a ${HWMGMT_USER_OUTFILE}

	git diff --staged --quiet || git commit -m "$$(cat $(SLK_COM_MSG))";
	popd

	# Commit the changes in buildimage and log the diff
	pushd $(BUILD_WORKDIR)
	git add -- $($(MLNX_HW_MANAGEMENT)_SRC_PATH)
	git add -- $(PLATFORM_PATH)/non-upstream-patches/
	git add -- $(PLATFORM_PATH)/hw-management.mk

	echo -en '\n###-> Non Upstream changes <-###\n' >> ${HWMGMT_USER_OUTFILE}
	git diff --no-color --staged -- $(PLATFORM_PATH)/non-upstream-patches/ >> ${HWMGMT_USER_OUTFILE}

	echo -en '\n###-> Non Upstream patch list file <-###\n' >> ${HWMGMT_USER_OUTFILE}
	git diff --no-color --staged -- $($(MLNX_HW_MANAGEMENT)_SRC_PATH)/hwmgmt_nonup_patches >> ${HWMGMT_USER_OUTFILE}

	echo -en '\n###-> hw-mgmt submodule update <-###\n' >> ${HWMGMT_USER_OUTFILE}
	git diff --no-color --staged -- $($(MLNX_HW_MANAGEMENT)_SRC_PATH)/hw-mgmt >> ${HWMGMT_USER_OUTFILE}

	echo -en '\n###-> hw-management make file version change <-###\n' >> ${HWMGMT_USER_OUTFILE}
	git diff --no-color --staged -- $(PLATFORM_PATH)/hw-management.mk >> ${HWMGMT_USER_OUTFILE}
	
	echo -en '\n###-> Summary of buildimage changes <-###\n' >> ${HWMGMT_USER_OUTFILE}
	git diff --no-color --staged --stat --output=${TMPFILE_OUT} -- $(PLATFORM_PATH)
	cat ${TMPFILE_OUT} | tee -a ${HWMGMT_USER_OUTFILE}

	git diff --staged --quiet || git commit -m "$$(cat $(SB_COM_MSG))";
	popd

	popd $(LOG_SIMPLE)

	rm -rf $(TEMP_HW_MGMT_DIR)

integrate-mlnx-sdk:
	$(FLUSH_LOG)
	rm -rf $(MLNX_SDK_VERSION).zip sx_kernel-$(MLNX_SDK_VERSION)-$(MLNX_SDK_ISSU_VERSION).tar.gz

ifeq ($(SDK_FROM_SRC),y)
	wget $(MLNX_SDK_SOURCE_BASE_URL)/sx_kernel-$(MLNX_SDK_VERSION)-$(MLNX_SDK_ISSU_VERSION).tar.gz $(LOG_SIMPLE)
	tar -xf sx_kernel-$(MLNX_SDK_VERSION)-$(MLNX_SDK_ISSU_VERSION).tar.gz --strip-components=1 -C $(SDK_TMPDIR) $(LOG_SIMPLE)
else
	# Download from upstream repository
	wget $(MLNX_SDK_DRIVERS_GITHUB_URL)/archive/refs/heads/$(MLNX_SDK_VERSION).zip $(LOG_SIMPLE)
	unzip $(MLNX_SDK_VERSION).zip -d $(SDK_TMPDIR) $(LOG_SIMPLE)
	mv $(SDK_TMPDIR)/Spectrum-SDK-Drivers-$(MLNX_SDK_VERSION)/* $(SDK_TMPDIR) $(LOG_SIMPLE)
endif

	pushd $(BUILD_WORKDIR)/src/sonic-linux-kernel; git clean -f -- patch/; git stash -- patch/
ifeq ($(CREATE_BRANCH), y)
	git checkout -B "$(BRANCH_SONIC)_$(SLK_HEAD)_integrate_$(MLNX_SDK_VERSION)" HEAD
	echo $(BRANCH_SONIC)_$(SLK_HEAD)_integrate_$(MLNX_SDK_VERSION) branch created in sonic-linux-kernel $(LOG_SIMPLE)
endif
	popd

	echo "#### Integrate SDK $(MLNX_SDK_VERSION) Kernel Patches into SONiC" > ${SDK_USER_OUTFILE}

	pushd $(BUILD_WORKDIR)/$(PLATFORM_PATH) $(LOG_SIMPLE)
	
    # Run tests
	pushd integration-scripts/tests; pytest-3 -v; popd

	integration-scripts/sdk_kernel_patches.py \
                            --sonic_kernel_ver $(KERNEL_VERSION) \
                            --patches $(SDK_TMPDIR) \
                            --slk_msg $(SLK_COM_MSG) \
                            --sdk_ver $(MLNX_SDK_VERSION) \
                            --build_root $(BUILD_WORKDIR) $(LOG_SIMPLE)

    # Commit the changes in linux kernel and and log the diff
	pushd $(BUILD_WORKDIR)/src/sonic-linux-kernel
	git add -- patches-sonic/

	echo -en "\n###-> series file changes in sonic-linux-kernel <-###\n" >> ${SDK_USER_OUTFILE}
	git diff --no-color --staged -- patches-sonic/series >> ${SDK_USER_OUTFILE}

	echo -en "\n###-> summary of files updated in sonic-linux-kernel <-###\n" >> ${SDK_USER_OUTFILE}
	git diff --no-color --staged --stat >> ${SDK_USER_OUTFILE}

	git diff --staged --quiet || git commit -m "$$(cat $(SLK_COM_MSG))"
	popd

	popd $(LOG_SIMPLE)
 
SONIC_PHONY_TARGETS += integrate-mlnx-hw-mgmt integrate-mlnx-sdk
