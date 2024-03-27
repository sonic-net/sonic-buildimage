CC=gcc
CFLAGS += -std=gnu99 -D_GNU_SOURCE

CXX=g++
CXXFLAGS += -std=c++11 -D_GNU_SOURCE
LDFLAGS += -lpthread -lboost_filesystem -lboost_system -lgtest -ljson-c

BINARY = systemd-sonic-generator

$(BINARY): systemd-sonic-generator.c
	rm -f ./systemd-sonic-generator

	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

install: $(BINARY)
	mkdir -p $(DESTDIR)
	mkdir -p $(DESTDIR)/lib
	mkdir -p $(DESTDIR)/lib/systemd
	mkdir -p $(DESTDIR)/lib/systemd/system-generators
	cp ./systemd-sonic-generator $(DESTDIR)/lib/systemd/system-generators

.PHONY: test
test: ssg_test
	./ssg_test

ssg_test: ssg-test.cc systemd-sonic-generator.o
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

systemd-sonic-generator.o: systemd-sonic-generator.c
	$(CC) $(CFLAGS) -D_SSG_UNITTEST -o $@ -c $^

clean:
	rm -f ./systemd-sonic-generator
	rm -f ./systemd-sonic-generator.o
	rm -f ./ssg_test
