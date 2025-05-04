include $(top_srcdir)/Rules.mk

#OBJ = firmware_app.o debug.o hardware.o ispvm_ui.o ivm_core.o crc32.o
PWD = $(shell pwd)
SRC :=
SRC += $(shell find $(PWD) -name '*.c')

OBJ := $(SRC:%.c=%.o)
LIB += $(BUILD_CFALGS) $(BUILD_LDFLAGS) -lpthread -lreadline -lncurses
INCLUDE = -Iinclude
INCLUDE+= -Wall
APP = firmware_upgrade
ELF_FILE = $(APP)
MAP_FILE = $(APP).map.sym

.PHONY: build
build:$(OBJ)
	$(CC) $^ -o $(ELF_FILE) $(LINKFLAGS) $(LIB)
	$(NM) $(ELF_FILE) | grep -v '\(compiled\)\|\(\.o$$\)\|\( [aUw] \)\|\(\.\.ng$$\)\|\(LASH[RL]DI\)' \
		| sort > $(MAP_FILE)
	cp -p $(ELF_FILE) $(common_out_put_dir)

%.o:%.c
	$(CC) -c $(CFLAGS) $(INCLUDE) $< -o $@

.PHONY: install
install:
	echo "firmware_upgrade install success."
	cp -p $(ELF_FILE) $(common_out_put_dir)

.PHONY: clean
clean:
	rm -rf $(BUILD_DIR)
