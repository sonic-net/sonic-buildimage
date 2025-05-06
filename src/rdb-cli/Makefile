.ONESHELL:
SHELL = /bin/bash
.SHELLFLAGS += -e

MAIN_TARGET = rdb-cli

$(addprefix $(DEST)/, $(MAIN_TARGET)): $(DEST)/% :
	rm -rf ./librdb
	git clone https://github.com/redis/librdb.git
	pushd ./librdb/
	git checkout tags/v1.0.0
	git submodule update --init --recursive	
	# Set WARNS=... to work around https://github.com/redis/librdb/issues/55
	make -j$(SONIC_CONFIG_MAKE_JOBS) WARNS='-Wall -Wextra -pedantic -flto=auto'
	mv bin/rdb-cli $(DEST)/
