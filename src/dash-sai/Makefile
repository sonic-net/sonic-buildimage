SHELL = /bin/bash
.ONESHELL:
.SHELLFLAGS += -xe

MAIN_TARGET = libsai_$(DASH_SAI_VERSION)_$(CONFIGURED_ARCH).deb
DERIVED_TARGETS = libsai-dev_$(DASH_SAI_VERSION)_$(CONFIGURED_ARCH).deb

CURRENT_DIR=$(shell pwd)
PIPELINE_DIR=$(CURRENT_DIR)/DASH/dash-pipeline
P4_MAIN=bmv2/dash_pipeline.p4
P4_OUTDIR=bmv2/dash_pipeline.bmv2


$(addprefix $(DEST)/, $(MAIN_TARGET)): $(DEST)/% :
# Remove any stale files
	rm -rf DASH

	git clone https://github.com/sonic-net/DASH.git DASH

	pushd DASH/dash-pipeline
	git checkout $(DASH_SAI_COMMIT)
	git submodule update --init

	mkdir -p $(P4_OUTDIR)
	mkdir -p SAI/lib
	sudo ln -sf $(PIPELINE_DIR)/bmv2 /bmv2

# Compile the P4 program
	p4c-bm2-ss -DTARGET_BMV2_V1MODEL $(P4_MAIN) -o $(P4_OUTDIR)/dash_pipeline.json --p4runtime-files $(P4_OUTDIR)/dash_pipeline_p4rt.json,$(P4_OUTDIR)/dash_pipeline_p4rt.txt --toJSON $(P4_OUTDIR)/dash_pipeline_ir.json

# Generate SAI files
	sudo ln -sf $(PIPELINE_DIR)/SAI /SAI
	pushd SAI
	make
	popd

# Compile sai meta
	sed -i.bak '/checkenumlock.sh/d' SAI/SAI/meta/Makefile
	sed -i.bak '/checkancestry.sh/d' SAI/SAI/meta/Makefile
	sed -i.bak '/checkstructs.sh/d' SAI/SAI/meta/Makefile
	sed -i.bak "/.\/saimetadatatest >\/dev\/null/d" SAI/SAI/meta/Makefile
	pushd SAI/SAI/meta/
	CFLAGS=-Wdangling-pointer=1 make all libsaimetadata.so
	popd

# Compile sai library
	pushd SAI/lib
	make
	popd

	pushd SAI/debian
	make
	mv $(DERIVED_TARGETS) $* $(DEST)/
	popd

	popd

$(addprefix $(DEST)/, $(DERIVED_TARGETS)): $(DEST)/% : $(DEST)/$(MAIN_TARGET)

