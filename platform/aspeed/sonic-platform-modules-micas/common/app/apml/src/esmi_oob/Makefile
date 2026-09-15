CFLAGS += -fPIC -I../../include
LDFLAGS += -shared
SRCS = $(wildcard *.c)
OBJS = $(SRCS:.c=.o)

TARGET = libapml.so.1.0
SONAME_LINK = libapml.so.1
SONAME_LINK_SHORT = libapml.so

all: $(TARGET) $(SONAME_LINK) $(SONAME_LINK_SHORT)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LDFLAGS) -Wl,-soname,$(SONAME_LINK)

$(SONAME_LINK): $(TARGET)
	ln -sf $(TARGET) $@

$(SONAME_LINK_SHORT): $(SONAME_LINK)
	ln -sf $(SONAME_LINK) $@

install:
	install -d $(DESTDIR)/usr/lib
	install -m 0644 $(TARGET) $(DESTDIR)/usr/lib/
	ln -sf $(TARGET) $(DESTDIR)/usr/lib/$(SONAME_LINK)
	ln -sf $(SONAME_LINK) $(DESTDIR)/usr/lib/$(SONAME_LINK_SHORT)

clean:
	rm -f $(TARGET) $(SONAME_LINK) $(SONAME_LINK_SHORT) *.o
