CXXFLAGS=-Wall -Werror -std=c++11 -ggdb -Iroot/include
PJLIBS=$(shell PKG_CONFIG_PATH="root/lib/pkgconfig/" pkg-config libpjproject --libs)

a.out: ue.cpp main.cpp pjsip_core.cpp
	g++ $(CXXFLAGS) main.cpp ue.cpp pjsip_core.cpp $(PJLIBS)

main2: main.cpp  libue.so
	g++ -o main2 $(CXXFLAGS) main.cpp -L . -lue

libue.so: ue.cpp ue_c_interface.cpp pjsip_core.cpp
	g++ -o libue.so -shared -fPIC $(CXXFLAGS) ue_c_interface.cpp ue.cpp pjsip_core.cpp $(PJLIBS)


libue.a: ue.cpp ue_c_interface.cpp pjsip_core.cpp
	g++ -c -fPIC $(CXXFLAGS) ue_c_interface.cpp ue.cpp pjsip_core.cpp $(PJLIBS)
	ar -M < script.ar
