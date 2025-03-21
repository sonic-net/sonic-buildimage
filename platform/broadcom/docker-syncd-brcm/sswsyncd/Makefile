CXX := g++
CXXFLAGS := -std=c++11 -O2
LDFLAGS := -lpthread

all: bcmcmd dsserve

bcmcmd: bcmcmd.cpp
	$(CXX) $(CXXFLAGS) -o bcmcmd bcmcmd.cpp $(LDFLAGS)

dsserve: dsserve.cpp
	$(CXX) $(CXXFLAGS) -o dsserve dsserve.cpp $(LDFLAGS)

clean:
	rm -f bcmcmd dsserve
