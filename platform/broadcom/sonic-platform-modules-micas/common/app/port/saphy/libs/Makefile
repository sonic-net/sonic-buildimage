SHELL = /bin/bash
.ONESHELL:
.SHELLFLAGS += -e

EPDM_PATH=./epdm_4_4_0
EPIL_PATH=./APT2_2_2
EPIL2_PATH=./BARC_3_6
EPIL3_PATH=./APRT_6_7
CREDO_PATH=./CRT2_26_0

EPDM_TARGET=libepdm.a
EPIL_TARGET=libapt2.a
EPIL2_TARGET=libbarc.a
EPIL3_TARGET=libaprt.a
CREDO_TARGET=libbald_eagle.a libblackhawk.a libcredo.a libcrlua.a libscreaming_eagle.a
ALL_LIB=$(EPDM_TARGET) $(EPIL_TARGET) $(EPIL2_TARGET) $(EPIL3_TARGET) $(CREDO_TARGET)
all:
	pushd $(EPDM_PATH);\
	make;\
    for lib in $(EPDM_TARGET); do \
        cp ./$$lib ../; \
    done; \
	popd; \
	pushd $(EPIL_PATH);\
	make;\
    for lib in $(EPIL_TARGET); do \
        cp ./build/$$lib ../; \
    done; \
	popd; \
	pushd $(EPIL2_PATH);\
	make;\
    for lib in $(EPIL2_TARGET); do \
        cp ./build/$$lib ../; \
    done; \
	popd; \
	pushd $(EPIL3_PATH);\
	make;\
    for lib in $(EPIL3_TARGET); do \
        cp ./build/$$lib ../; \
    done; \
	popd; \
	pushd $(CREDO_PATH);\
	make all;\
    for lib in $(CREDO_TARGET); do \
        cp ./build/$$lib ../; \
    done; \
	popd;
clean:
	pushd $(EPDM_PATH);\
	make clean;\
	popd; \
	pushd $(EPIL_PATH);\
	make clean;\
	popd;
	pushd $(EPIL2_PATH);\
	make clean;\
	popd;
	pushd $(EPIL3_PATH);\
	make clean;\
	popd;
	pushd $(CREDO_PATH);\
	make clean;\
	popd;\
	rm $(ALL_LIB)