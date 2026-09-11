CFLAGS += -fPIC -I../include
LDFLAGS += -L../src/esmi_oob/ -lapml -lpthread -lrt -lm

SRCS = apml_tool.c mi300_tool.c apml_nda_tool.c
OBJS = $(SRCS:.c=.o)
TARGET = apml_tool

SRCS2 = apml_cpuid_tool.c
OBJS2 = $(SRCS2:.c=.o)
TARGET2 = apml_cpuid_tool

SRCS3 = apml_dimm_temp.c
OBJS3 = $(SRCS3:.c=.o)
TARGET3 = apml_dimm_temp

all: $(TARGET) $(TARGET2) $(TARGET3)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS) $(LDFLAGS)

$(TARGET2): $(OBJS2)
	$(CC) $(CFLAGS) -o $(TARGET2) $(OBJS2) $(LDFLAGS)

$(TARGET3): $(OBJS3)
	$(CC) $(CFLAGS) -o $(TARGET3) $(OBJS3) $(LDFLAGS)

install:
	install -d $(DESTDIR)/usr/bin
	install -m 0755 $(TARGET) $(DESTDIR)/usr/bin/
	install -m 0755 $(TARGET2) $(DESTDIR)/usr/bin/
	install -m 0755 $(TARGET3) $(DESTDIR)/usr/bin/

clean:
	rm -f $(TARGET) $(TARGET2) $(TARGET3) *.o
