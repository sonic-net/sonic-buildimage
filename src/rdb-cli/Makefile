.ONESHELL:
SHELL = /bin/bash
.SHELLFLAGS += -e

MAIN_TARGET = rdb-cli

$(addprefix $(DEST)/, $(MAIN_TARGET)): $(DEST)/% :
	rm -rf ./librdb
	git clone https://github.com/redis/librdb.git
	pushd ./librdb/
	git checkout 2fdfc0c2bc914d643fe3f86e6715aeb843d8966e
	git submodule update --init --recursive	
	make -j$(SONIC_CONFIG_MAKE_JOBS)
	mv bin/rdb-cli $(DEST)/
