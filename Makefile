CC = clang++
CXX = clang++
CXXFLAGS = -Iglm -std=c++14 -Werror
EMFLAGS = -s DISABLE_EXCEPTION_CATCHING=0 -s DEMANGLE_SUPPORT=1
export PATH := $(PWD):$(PATH)

all: img2files txd2png dff2glr
txd2png: txd2png.o txd.o lodepng.o base64.o
dff2glr: dff2glr.o dff.o Clump.o txd.o lodepng.o base64.o

js: txd.js dff2glr.js

txd.js: txd.cc common.cc lodepng.cpp base64.cpp
	em++ $(CXXFLAGS) $(EMFLAGS) $^ -o $@ --bind

dff2glr.js: dff2glr.cc common.cc dff.cc Clump.cpp txd.cc lodepng.cpp base64.cpp
	em++ $(CXXFLAGS) $(EMFLAGS) $^ -o $@ --bind

data:
	mkdir -p data/models
	cp $(GTA3)/models/gta3.dir $(GTA3)/models/gta3.img $(GTA3)/models/generic.txd data/models

export GTA3 = $(HOME)/.steam/steam/steamapps/common/Grand\ Theft\ Auto\ 3

IPL = \
	$(GTA3)/data/maps/comnbtm/comNbtm \
	$(GTA3)/data/maps/comntop/comNtop \
	$(GTA3)/data/maps/comse/comSE \
	$(GTA3)/data/maps/comsw/comSW \
	$(GTA3)/data/maps/industne/industNE \
	$(GTA3)/data/maps/industnw/industNW \
	$(GTA3)/data/maps/industse/industSE \
	$(GTA3)/data/maps/industsw/industSW \
	$(GTA3)/data/maps/landne/landne \
	$(GTA3)/data/maps/landsw/landsw

img: img2files
	mkdir -p img
	cd img && ../img2files $(GTA3)/models/gta3 && cd ..
	cp $(GTA3)/models/*.txd $(GTA3)/models/*.TXD img

ipl: ipl2glr.js dff2glr
	mkdir -p ipl
	cd ipl && for f in $(IPL); do echo "$$f"; ../ipl2glr.js "$$f.ipl" "`dirname "$$f"`/`basename "$$f" | tr A-Z a-z`.ide"; done
	cd ipl && ../ipl2glr.js $(GTA3)/data/maps/overview.ipl && ../ipl2glr.js $(GTA3)/data/maps/props.IPL
	touch ipl

ipl/%.gltf: ipl glr2gltf.js
	cd ipl && ../glr2gltf.js < $*.glr > $*.gltf

maps: ipl/comnbtm.gltf ipl/comntop.gltf ipl/comse.gltf ipl/comsw.gltf \
	ipl/industne.gltf ipl/industnw.gltf ipl/industse.gltf ipl/industsw.gltf \
	ipl/landne.gltf ipl/landsw.gltf ipl/overview.gltf ipl/props.gltf
