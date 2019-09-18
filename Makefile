CC = clang++
CXX = clang++
DEBUG_FLAGS = -g -fsanitize=address
CXXFLAGS = $(shell pkg-config --cflags Magick++) $(DEBUG_FLAGS)
LDFLAGS = $(shell pkg-config --libs Magick++) $(DEBUG_FLAGS)

dff2glr: dff2glr.o Clump.o

GTA3 = $(HOME)/.steam/steam/steamapps/common/Grand\ Theft\ Auto\ 3

img: img2files
	mkdir -p img
	cd img && ../img2files $(GTA3)/models/gta3 && cd ..

txd: img txd2png
	mkdir -p txd
	cd txd && ls ../img/*.txd ../img/*.TXD | xargs -tn1 ../txd2png && cd ..
	touch txd

gltf: txd dff2glr
	mkdir -p gltf
	ln -svf ../txd gltf
	cd gltf && ls ../img/*.dff ../img/*.DFF | xargs -tn1 ../dff2gltf && cd ..

glr: txd dff2glr
	mkdir -p glr
	ln -svf ../txd glr
	cd glr && ls ../img/*.dff ../img/*.DFF | xargs -tn1 ../dff2glr.sh && cd ..
