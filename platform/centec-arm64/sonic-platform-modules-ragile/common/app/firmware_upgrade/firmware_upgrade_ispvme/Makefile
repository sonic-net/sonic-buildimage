include $(top_srcdir)/Rules.mk

OBJ = firmware_app_ispvme.o debug_ispvme.o hardware.o ispvm_ui.o ivm_core.o dfd_fpga_pkt.o dfd_fpga_upg.o dfd_fpga_debug.o
LIB += $(BUILD_CFALGS) $(BUILD_LDFLAGS) -lpthread -lreadline -lncurses 
ifdef ENABLE_GCOV
ifeq ($(ENABLE_GCOV), y)
LIB += -lgcov
endif
endif # ENABLE_GCOV
APP = firmware_upgrade
BUILD_DIR = tmp
ELF_FILE = $(BUILD_DIR)/$(APP)
MAP_FILE = $(BUILD_DIR)/$(APP).map.sym
INCLUDE = -Iinclude

.PHONY: build
build:make-dir $(addprefix $(BUILD_DIR)/,$(OBJ))
	$(CC) -o $(ELF_FILE) $(addprefix $(BUILD_DIR)/,$(OBJ)) $(LINKFLAGS) $(LIB)
	$(NM) $(ELF_FILE) | grep -v '\(compiled\)\|\(\.o$$\)\|\( [aUw] \)\|\(\.\.ng$$\)\|\(LASH[RL]DI\)' \
		| sort > $(MAP_FILE)
	@if [ ! -d $(common_out_put_dir) ]; then mkdir -p $(common_out_put_dir) ;fi
	cp -p $(ELF_FILE) $(common_out_put_dir)

# build
.PHONY: make-dir
make-dir:
	@mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/%.o:%.c
	$(CC) -c $(CFLAGS) $(INCLUDE) $< -o $@

.PHONY: install
install:
	echo "firmware_upgrade install success."
	cp -p $(ELF_FILE) $(common_out_put_dir)

.PHONY: clean
clean:
	rm -rf $(BUILD_DIR)
