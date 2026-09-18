CFLAGS := -fPIC -O3 -g -Wall -Werror -std=gnu99
#CFLAGS := -fPIC -O3 -g -Wall -Werror -std=c99
CC ?= gcc
PROGRAM := fpga

SOURCE := $(wildcard *.c) $(wildcard driver/*/*.c)
OBJECT := $(SOURCE:.c=.o)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(PROGRAM): $(OBJECT)
	$(CC) -static -o $@ $^ $(CFLAGS) $(LDFLAGS)

clean:
	$(RM) $(PROGRAM) *.o driver/*/*.o

