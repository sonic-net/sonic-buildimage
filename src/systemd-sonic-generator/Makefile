CC=gcc

CXX=g++
CXXFLAGS += -std=c++20 -D_GNU_SOURCE -I ./
LDFLAGS += -l stdc++ -lpthread -lboost_filesystem -lboost_system -lgtest -ljson-c

BINARY = systemd-sonic-generator

$(BINARY): systemd-sonic-generator.cpp
	rm -f ./systemd-sonic-generator

	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

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
	$(CXX) $(CXXFLAGS) -ggdb -o $@ $^ $(LDFLAGS)

systemd-sonic-generator.o: systemd-sonic-generator.cpp
	$(CXX) $(CXXFLAGS) -ggdb -D_SSG_UNITTEST -o $@ -c $^

all: $(BINARY) test

clean:
	rm -f ./systemd-sonic-generator
	rm -f ./systemd-sonic-generator.o
	rm -f ./ssg_test
