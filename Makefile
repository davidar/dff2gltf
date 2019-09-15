CXX = clang++
CXXFLAGS = $(shell pkg-config --cflags Magick++)
LDFLAGS = $(shell pkg-config --libs Magick++)
