SRCS = $(wildcard *.c)
OBJS = $(SRCS:.c=.o)
TARGET = dfd_debug

INC_DIR = ../../public/include/
CFLAGS += -I$(INC_DIR)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS) $(LDFLAGS)

install:
	install -d $(DESTDIR)/usr/bin
	install -m 0755 $(TARGET) $(DESTDIR)/usr/bin

clean:
	rm -f $(TARGET) *.o
