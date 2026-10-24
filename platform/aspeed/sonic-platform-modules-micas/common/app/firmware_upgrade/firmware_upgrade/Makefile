INCLUDE_DIRS := $(abspath ./include) $(abspath ../include)
CFLAGS += -fPIC $(foreach dir, $(INCLUDE_DIRS), -I$(dir))

LDFLAGS += -lpthread
SRCS := $(shell find . -name '*.c')
OBJS := $(SRCS:.c=.o)
TARGET = firmware_upgrade

all: $(TARGET) 

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS) $(LDFLAGS)

install:
	install -d $(DESTDIR)/usr/bin
	install -m 0755 $(TARGET) $(DESTDIR)/usr/bin/
	ln -sf /usr/bin/$(TARGET) $(DESTDIR)/usr/bin/fw_up_bios
clean:
	rm -f $(TARGET) *.o

