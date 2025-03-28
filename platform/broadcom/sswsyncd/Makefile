CXX := g++
CXXFLAGS := -std=c++11 -O2
LDFLAGS := -lpthread

all: bcmcmd dsserve

bcmcmd: bcmcmd.cpp
	$(CXX) $(CXXFLAGS) -o sswsyncd-bcmcmd.bin bcmcmd.cpp $(LDFLAGS)

dsserve: dsserve.cpp
	$(CXX) $(CXXFLAGS) -o sswsyncd-dsserve.bin dsserve.cpp $(LDFLAGS)

clean:
	rm -f sswsyncd-bcmcmd.bin sswsyncd-dsserve.bin
