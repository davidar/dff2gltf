CC = clang++
CXX = clang++
CXXFLAGS = $(shell pkg-config --cflags Magick++)
LDFLAGS = $(shell pkg-config --libs Magick++)

dff2gltf: dff2gltf.o Clump.o
