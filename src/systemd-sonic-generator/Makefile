CC=gcc
CFLAGS += -std=gnu99 -D_GNU_SOURCE

CXX=g++
CXXFLAGS += -std=c++14 -D_GNU_SOURCE
LDFLAGS += -lstdc++ -lpthread -lboost_filesystem -lboost_system -lgtest

BINARY = systemd-sonic-generator

$(BINARY): systemd-sonic-generator.cpp
	rm -f ./systemd-sonic-generator

	$(CXX) $(CXXFLAGS) -o $@ $^

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

systemd-sonic-generator.o: systemd-sonic-generator.cpp
	$(CXX) $(CXXFLAGS) -D_SSG_UNITTEST -o $@ -c $^

all: $(BINARY) test

clean:
	rm -f ./systemd-sonic-generator
	rm -f ./systemd-sonic-generator.o
	rm -f ./ssg_test
