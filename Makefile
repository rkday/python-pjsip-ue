CXXFLAGS=-Wall -Werror -std=c++11 -ggdb -Iroot/include
PJLIBS=$(shell PKG_CONFIG_PATH="root/lib/pkgconfig/" pkg-config libpjproject --libs)

libue.a: ue.cpp ue_c_interface.cpp pjsip_core.cpp
	g++ -c -fPIC $(CXXFLAGS) $^ $(PJLIBS)
	ar -M < script.ar
