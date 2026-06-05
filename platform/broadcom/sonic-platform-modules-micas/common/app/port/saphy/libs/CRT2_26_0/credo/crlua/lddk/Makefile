CC ?= gcc
CFLAGS = -fPIC -ansi -std=c99 -Wall -Werror -Wno-unused-result -I../ -I../../include -DCREDO_DEBUG_API

# set if making a production build
ifdef DEBUG
	CFLAGS +=  -g
else
	CFLAGS += -O2
endif

LIBS = cfye secy sab driver164

# build all examples
all: $(LIBS)

$(LIBS): % : l%.c
	mkdir -p $(IMG_ROOT)/lualibs/ddk
	$(CC) -shared $^ $(CFLAGS) -o $(IMG_ROOT)/lualibs/ddk/_$@.so

clean:
	rm -f -o $(IMG_ROOT)/lualibs/ddk/*.so
