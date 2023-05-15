#
# Copyright (c) 2016-2022 NVIDIA CORPORATION & AFFILIATES.
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
KCFG_LIST = $(TEMP_HW_MGMT_DIR)/kconfig
HWMGMT_NONUP_LIST = $(BUILD_WORKDIR)/$($(MLNX_HW_MANAGEMENT)_SRC_PATH)/hwmgmt_nonup_patches
HWMGMT_USER_OUTFILE = $(BUILD_WORKDIR)/integrate-mlnx-hw-mgmt_user.out
TMPFILE_OUT := $(shell mktemp)
SB_COM_MSG := $(shell mktemp -t sb_commit_msg_file_XXXXX.log)
SLK_COM_MSG := $(shell mktemp -t slk_commit_msg_file_XXXXX.log)
SB_HEAD = $(shell git rev-parse --short HEAD)
SLK_HEAD = $(shell cd src/sonic-linux-kernel; git rev-parse --short HEAD)

integrate-mlnx-hw-mgmt:
	$(FLUSH_LOG)
	rm -rf $(TEMP_HW_MGMT_DIR) $(TMPFILE_OUT)
	mkdir -p $(PTCH_DIR) $(NON_UP_PTCH_DIR)
	touch $(PTCH_LIST) $(KCFG_LIST)

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
							--config_inclusion $(KCFG_LIST) \
							--build_root $(BUILD_WORKDIR) \
							--kernel_version $(KERNEL_VERSION) \
							--hw_mgmt_ver ${MLNX_HW_MANAGEMENT_VERSION}  $(LOG_SIMPLE)

	$(BUILD_WORKDIR)/$($(MLNX_HW_MANAGEMENT)_SRC_PATH)/hw-mgmt/recipes-kernel/linux/deploy_kernel_patches.py \
							--dst_accepted_folder $(PTCH_DIR) \
							--dst_candidate_folder $(NON_UP_PTCH_DIR) \
							--series_file $(PTCH_LIST) \
							--config_file $(KCFG_LIST) \
							--kernel_version $(KERNEL_VERSION) \
							--os_type sonic $(LOG_SIMPLE)

	# Post-processing
	integration-scripts/hwmgmt_kernel_patches.py post \
							--patches $(PTCH_DIR) \
							--non_up_patches $(NON_UP_PTCH_DIR) \
							--kernel_version $(KERNEL_VERSION) \
							--hw_mgmt_ver ${MLNX_HW_MANAGEMENT_VERSION} \
							--config_inclusion $(KCFG_LIST) \
							--series $(PTCH_LIST) \
							--current_non_up_patches $(HWMGMT_NONUP_LIST) \
							--build_root $(BUILD_WORKDIR) \
							--sb_msg $(SB_COM_MSG) \
							--slk_msg $(SLK_COM_MSG) $(LOG_SIMPLE)
	
	# Commit the changes in linux kernel and and log the diff
	pushd $(BUILD_WORKDIR)/src/sonic-linux-kernel
	git add -- patch/

	echo -en "\n###-> series file changes in sonic-linux-kernel <-###\n" >> ${HWMGMT_USER_OUTFILE}
	git diff --no-color --staged -- patch/series >> ${HWMGMT_USER_OUTFILE}

	echo -en "\n###-> kconfig-inclusions file changes in sonic-linux-kernel <-###\n" >> ${HWMGMT_USER_OUTFILE}
	git diff --no-color --staged -- patch/kconfig-inclusions >> ${HWMGMT_USER_OUTFILE}

	echo -en "\n###-> kconfig-exclusions file changes in sonic-linux-kernel <-###\n" >> ${HWMGMT_USER_OUTFILE}
	git diff --no-color --staged -- patch/kconfig-exclusions >> ${HWMGMT_USER_OUTFILE}

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

	echo -en '\n###-> Non Upstream series.patch changes <-###\n' >> ${HWMGMT_USER_OUTFILE}
	git diff --no-color --staged -- $(PLATFORM_PATH)/non-upstream-patches/series.patch >> ${HWMGMT_USER_OUTFILE}

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

SONIC_PHONY_TARGETS += integrate-mlnx-hw-mgmt
