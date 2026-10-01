.ONESHELL:
SHELL = /bin/bash
.SHELLFLAGS += -e

MAIN_TARGET = $(ZSTD_DECOMPRESS)

$(addprefix $(DEST)/, $(MAIN_TARGET)): $(DEST)/% :
	rm -rf ./zstd
	git clone --depth 1 --branch v$(ZSTD_VERSION) https://github.com/facebook/zstd.git
	pushd ./zstd

	CC=musl-gcc \
	MOREFLAGS="-static -no-pie" \
	CFLAGS="-O2 -s -ffunction-sections -fdata-sections -flto" \
	LDFLAGS="-Wl,--gc-sections -flto" \
	make -C programs zstd-decompress

	cp programs/zstd-decompress $(DEST)/$(MAIN_TARGET)
