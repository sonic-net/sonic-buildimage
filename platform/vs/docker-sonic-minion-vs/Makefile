ONESHELL:
SHELL = /bin/bash
.SHELLFLAGS += -e

MAIN_TARGET = sonic-minion-vs-root.tar.gz

$(addprefix $(DEST)/, $(MAIN_TARGET)): $(DEST)/% :
	sudo tar -C $(DEST)/../../../fsroot-vs -zc --exclude='*.sock' --exclude='*.a' --exclude='dev/*' --exclude='boot/*' --exclude='usr/lib/modules/*' -f $(MAIN_TARGET) .
	mv $(MAIN_TARGET) $(DEST)/



