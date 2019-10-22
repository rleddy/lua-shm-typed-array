.PHONY : all

all : lshmtester lshmtester2

lshmtester: lshmtester.o
	$(CXX) $^ -o $@  -L./.. -lluashm

lshmtester2: lshmtester2.o
	$(CXX) $^ -o $@  -L./.. -lluashm

lshmtester.o,lshmtester2.o : CXXFLAGS+=-std=c++17
