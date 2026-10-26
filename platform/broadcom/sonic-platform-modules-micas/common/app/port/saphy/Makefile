################################################################################
# Copyright(C) 2026 Micas Network. All rights reserved.
#
# Makefile for PHY Application (phy_app)
#
# Source files :
#   APP  : src/main.c, src/common.c, src/mdio.c
#   APERTA: src/aperta/aperta_phy_sdk.c, src/aperta/aperta_phy.c etc
#
# The aperta/ sources are first archived into a static library (libaperta.a),
# then linked together with the app objects and Broadcom SDK into phy_app.
#
# Outputs:
#   build/aperta/libaperta.a   - APERTA PHY SDK static library
#   phy_app                    - final executable
#
# All plp_aperta_* symbols are implemented in bcm_sdk_stubs.c.
# No external Broadcom SDK library required.
################################################################################

CC       := gcc
AR       := ar

# ----- Directories -----------------------------------------------------------
# NOTE: these must be defined BEFORE CFLAGS, because CFLAGS uses += (immediate
# expansion). If INC_DIR were defined after, -I$(INC_DIR) would expand to a
# bare "-I" and the compiler could not find the headers.
SRC_DIR  := src
INC_DIR  := include
BUILD_DIR:= build

CFLAGS   := -Wall -Werror -Wno-unused-variable -Wno-unused-parameter
CFLAGS   += -O2 -g
CFLAGS   += -I$(INC_DIR)
CFLAGS   += -I$(INC_DIR)/aperta
CFLAGS   += -D_GNU_SOURCE

# ----- APP source files (top-level, compiled to individual .o) ----------------
APP_SRCS     := $(SRC_DIR)/main.c    \
                $(SRC_DIR)/common.c  \
                $(SRC_DIR)/mdio.c

APP_OBJS     := $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(APP_SRCS))
APP_DEPFILES := $(APP_OBJS:.o=.d)

# ----- APERTA source files (archived into libaperta.a) ------------------------
# bcm_sdk_stubs.c provides the plp_aperta_* extern symbols previously linked
# from the Broadcom SDK .a library – no external BCM SDK dependency needed.
APERTA_SRCS     := $(SRC_DIR)/aperta/aperta_phy_sdk.c    \
                   $(SRC_DIR)/aperta/aperta_phy.c        \
                   $(SRC_DIR)/aperta/aperta_bcm_sdk_stubs.c \
                   $(SRC_DIR)/aperta/aperta_fw_dload.c   \
                   $(SRC_DIR)/aperta/aperta_ucode.c \
                   $(SRC_DIR)/aperta/aperta_tbl.c \
                   $(SRC_DIR)/aperta/aperta_pm_port.c

APERTA_OBJS     := $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(APERTA_SRCS))
APERTA_DEPFILES := $(APERTA_OBJS:.o=.d)
APERTA_LIB      := $(BUILD_DIR)/aperta/libaperta.a

# ----- Output ----------------------------------------------------------------
TARGET   := saphy

# ----- External libraries ----------------------------------------------------
# No external Broadcom SDK library needed – all plp_aperta_* symbols are
# implemented in bcm_sdk_stubs.c within the aperta library.
LDLIBS   := -lpthread -lm
LDFLAGS  :=

# ----- Default target --------------------------------------------------------
.PHONY: all
all: $(APERTA_LIB) $(TARGET)

# ----- Static library (aperta) -----------------------------------------------
# Archive the aperta objects into a static library. Rebuild the .a whenever any
# object file changes.
$(APERTA_LIB): $(APERTA_OBJS)
	@mkdir -p $(@D)
	$(AR) rcs $@ $^
	@echo "===== Library built: $@ ====="

# ----- Link (saphy) --------------------------------------------------------
# Link order: app objects, then aperta lib, then external libs.
$(TARGET): $(APP_OBJS) $(APERTA_LIB)
	$(CC) $(CFLAGS) $^ $(LDFLAGS) $(LDLIBS) -o $@
	@mkdir -p $(port_out_put_dir)
	cp -p ./saphy $(port_out_put_dir)
	@echo "===== Build complete: $@ ====="

# ----- Compile (single .c -> .o rule for all sources) ------------------------
# Auto-creates the target directory for each object file.
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

# ----- Include auto-generated dependency files --------------------------------
-include $(APP_DEPFILES)
-include $(APERTA_DEPFILES)

# ----- Utilities -------------------------------------------------------------
.PHONY: clean distclean info

clean:
	rm -rf $(BUILD_DIR)

distclean: clean
	rm -f $(TARGET)

info:
	@echo "CC        = $(CC)"
	@echo "AR        = $(AR)"
	@echo "CFLAGS    = $(CFLAGS)"
	@echo "LDFLAGS   = $(LDFLAGS)"
	@echo "LDLIBS    = $(LDLIBS)"
	@echo "APP_SRCS  = $(APP_SRCS)"
	@echo "APP_OBJS  = $(APP_OBJS)"
	@echo "APERTA_SRCS = $(APERTA_SRCS)"
	@echo "APERTA_OBJS = $(APERTA_OBJS)"
	@echo "APERTA_LIB  = $(APERTA_LIB)"
	@echo "TARGET    = $(TARGET)"
