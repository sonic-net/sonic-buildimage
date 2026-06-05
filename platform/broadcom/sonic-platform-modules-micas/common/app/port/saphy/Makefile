SHELL = /bin/bash
CC = gcc

BUILD_DIR = $(CURDIR)/build

OUTPUT_DIR = $(CURDIR)/output

BLDDIR = $(BUILD_DIR)
LOCALDIR = $(BUILD_DIR)
LIBDIR = $(BUILD_DIR)

VSRCS += $(wildcard ./src/*.c)
LSRCS = $(notdir $(VSRCS))
LOBJS = $(addsuffix .o,$(basename $(LSRCS)))
BOBJS = $(addprefix $(BLDDIR)/,$(LOBJS))

DEBUG = 

CFLAGS=$(CFLAG)
CFLAGS+= -Wall -Wno-unused -fPIC

INCLUDES = -I ./include -I ./include/credo -I ./libs/epdm_4_4_0
LIB_PATH = ./libs

VLIBS = $(wildcard ./libs/*.a)
LLIBS = $(notdir $(VLIBS))
LLOBJS = $(addsuffix .o,$(basename $(LLIBS)))
LBOBJS += $(addprefix $(BLDDIR)/,$(LLOBJS))

TARGET_LIB = $(OUTPUT_DIR)/libsaphy.a
TARGET_BIN = $(OUTPUT_DIR)/saphy
TARGET_SO = $(OUTPUT_DIR)/libsaphy.so
TARGET_SOBIN = $(OUTPUT_DIR)/saphy_so

DEPENDENCY_LIB = ./libs/libscreaming_eagle.a ./libs/libblackhawk.a ./libs/libcrlua.a ./libs/libcredo.a ./libs/libepdm.a ./libs/libapt2.a ./libs/libbarc.a ./libs/libaprt.a 

LIB_SRC = $(LIB_PATH)/

$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

$(OUTPUT_DIR):
	@mkdir -p $(OUTPUT_DIR)

# .a
$(TARGET_LIB): $(BOBJS) | $(OUTPUT_DIR)
	@echo "Build libsaphy.a"
	ar -rcs $(TARGET_LIB) $^

# .bin
$(TARGET_BIN): $(BOBJS) dependency_libs | $(OUTPUT_DIR)
	@echo "Build saphy"
	$(CC) -o $@ $(BOBJS) -L $(LIB_PATH) -Wl,--whole-archive $(DEPENDENCY_LIB) -Wl,--no-whole-archive -lstdc++ -lpthread -ldl -lm -lreadline -lncurses

# .o
$(BOBJS): $(BLDDIR)/%.o: ./src/%.c ./include/sap_common.h ./include/sap_api.h ./include/sap_debug.h ./include/sap_mdio.h 
	@mkdir -p $(BUILD_DIR)
	$(CC) $(INCLUDES) $(DEBUG) -c $(CFLAGS) $< -o $@ -DPLP_BARCHETTA_SUPPORT -DPLP_APERTA2_SUPPORT -DPLP_MACSEC_SUPPORT -DAPERTA2_FRC_SERBOOT_LOW -DCREDO_DEBUG_API -DPLP_APERTA_SUPPORT

# .so
$(TARGET_SO): $(VSRCS) dependency_libs | $(BUILD_DIR) $(OUTPUT_DIR)
	$(CC) $(VSRCS) $(INCLUDES) $(DEBUG) $(CFLAGS) -shared -o $(TARGET_SO) -DPLP_BARCHETTA_SUPPORT -DPLP_APERTA2_SUPPORT -DPLP_MACSEC_SUPPORT -DPLP_APERTA_SUPPORT -L $(LIB_PATH) -Wl,--whole-archive $(DEPENDENCY_LIB) -Wl,--no-whole-archive -lstdc++ -lpthread -ldl -lm -lpthread -lrt

# .so.bin
$(TARGET_SOBIN): ./src/sap_debug.c | $(BUILD_DIR) $(OUTPUT_DIR)
	$(CC) ./src/sap_debug.c $(INCLUDES) $(DEBUG) $(CFLAGS) -L $(OUTPUT_DIR) -lsaphy -o $@

./build/sapcmd.o:./src/sapcmd/sapcmd.c ./include/sap_socket.h | $(BUILD_DIR)
	$(CC) -c $< -o $@ -I ./include

all: install

sapcmd_bin:./build/sapcmd.o ./build/sap_socket.o | $(OUTPUT_DIR)
	echo "enter sapcmd build"
	$(CC) -o ./output/sapcmd ./build/sapcmd.o ./build/sap_socket.o -lpthread -lreadline

install: bin
	@mkdir -p $(port_out_put_dir)
	cp -p $(OUTPUT_DIR)/* $(port_out_put_dir)

dependency_libs:
	pushd $(LIB_SRC);\
	make all;\
	popd;

lib: $(TARGET_LIB) dependency_libs
	cp ./libs/*.a $(OUTPUT_DIR)

solib: $(TARGET_SO)

sobin: CFLAGS += -DSAPHY_BINARY
sobin: solib $(TARGET_SOBIN)
	
bin: CFLAGS += -DSAPHY_BINARY
bin: $(TARGET_BIN)
bin: sapcmd_bin

clean:
	@echo "Remove linked and compiled files......"
	-rm -rf $(BUILD_DIR) $(OUTPUT_DIR)
	pushd $(LIB_SRC);\
	make clean;\
	popd;

rebuild: clean all

.PHONY:all rebuild clean lib bin sapcmd_bin
