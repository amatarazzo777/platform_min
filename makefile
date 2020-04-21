CC=clang-9
#CC=g++
CFLAGS=-std=c++17 -Os `Magick++-config --cppflags --cxxflags`
INCLUDES=-I/projects/guidom `pkg-config --cflags freetype2 fontconfig` -fexceptions

LFLAGS=`pkg-config --libs freetype2 xcb-image fontconfig` `Magick++-config --ldflags --libs`

debug: CFLAGS += -g
debug: vis.out

release: LFLAGS += -s
release: vis.out

all: vis.out

vis.out: main.o uxdevice.o
	$(CC) -o vis.out main.o uxdevice.o -lstdc++ -lm -lX11-xcb -lX11 -lxcb-keysyms $(LFLAGS) 
	
main.o: main.cpp uxdevice.hpp
	$(CC) $(CFLAGS) $(INCLUDES) -c main.cpp -o main.o

uxdevice.o: uxdevice.cpp uxdevice.hpp
	$(CC) $(CFLAGS) $(INCLUDES) -c uxdevice.cpp -o uxdevice.o

clean:
	rm *.o *.out

