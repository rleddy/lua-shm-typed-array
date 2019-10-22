libluashm.dylib: lua_shm.o
	$(CXX) -shared $^ -o $@ -Wl

lua_shm.o : CXXFLAGS+=-fPIC -std=c++17
