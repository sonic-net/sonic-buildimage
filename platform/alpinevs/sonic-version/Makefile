.ONESHELL:
SHELL = /bin/bash
.SHELLFLAGS += -e

MAIN_TARGET = sonic_version.yml

$(addprefix $(DEST)/, $(MAIN_TARGET)): $(DEST)/% :
	./build_sonic_version.sh $(MAIN_TARGET)

	mv $(MAIN_TARGET) $(DEST)/
