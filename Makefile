test/arf: ARF.h ARF.cpp test/arf.cpp
	c++ -g -I. -o $@ ARF.cpp test/arf.cpp
