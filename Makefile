CC = clang++
CXX = clang++
CXXFLAGS = $(shell pkg-config --cflags Magick++)
LDFLAGS = $(shell pkg-config --libs Magick++)

dff2glr: dff2glr.o Clump.o

GTA3 = $(HOME)/.steam/steam/steamapps/common/Grand\ Theft\ Auto\ 3

img: img2files
	mkdir -p img
	cd img && ../img2files $(GTA3)/models/gta3 && cd ..
