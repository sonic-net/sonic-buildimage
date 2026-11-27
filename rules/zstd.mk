# zstd static decompressor binary

ZSTD_VERSION = 1.5.7
ZSTD_DECOMPRESS = zstd-decompress

export ZSTD_DECOMPRESS
export ZSTD_VERSION

$(ZSTD_DECOMPRESS)_SRC_PATH = $(SRC_PATH)/zstd
SONIC_MAKE_FILES += $(ZSTD_DECOMPRESS)


