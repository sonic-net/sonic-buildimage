include ../Rules.mk

OBJ = fw_upgrade.o fw_upgrade_debug.o

LIB += $(BUILD_CFALGS) $(BUILD_LDFLAGS) -lpthread
ifdef ENABLE_GCOV
ifeq ($(ENABLE_GCOV), y)
LIB += -fprofile-arcs
endif
endif # ENABLE_GCOV

APP = fw_upgrade
BUILD_DIR = tmp
ELF_FILE = $(BUILD_DIR)/$(APP)
MAP_FILE = $(BUILD_DIR)/$(APP).map.sym
INCLUDE = -Iinclude
CFLAGS+=-Wall -W -g

.PHONY: build
build:make-dir $(addprefix $(BUILD_DIR)/,$(OBJ))
	$(CC) -o $(ELF_FILE) $(addprefix $(BUILD_DIR)/,$(OBJ)) $(LINKFLAGS) $(LIB)

	cp -p $(ELF_FILE) $(common_out_put_dir)

.PHONY: make-dir
make-dir:
	@mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/%.o:%.c
	$(CC) -c $(CFLAGS) $(INCLUDE) $< -o $@

.PHONY: install
install:
	echo "fw_upgrade install success."
	cp -p $(ELF_FILE) $(common_out_put_dir)

.PHONY: clean
clean:
	rm -rf $(BUILD_DIR)
