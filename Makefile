CC = clang++
CXX = clang++
CXXFLAGS = -Iglm -std=c++14 -Werror
export PATH := $(PWD):$(PATH)

all: img2files txd2png dff2glr
txd2png: txd2png.o txd.o lodepng.o
dff2glr: dff2glr.o dff.o Clump.o txd.o lodepng.o base64.o

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

ipl: ipl2glr.js dff2glr
	mkdir -p ipl buf
	ln -svf ../buf ipl
	cd ipl && for f in $(IPL); do echo "$$f"; ../ipl2glr.js "$$f.ipl" "`dirname "$$f"`/`basename "$$f" | tr A-Z a-z`.ide"; done
	cd ipl && ../ipl2glr.js $(GTA3)/data/maps/overview.ipl && ../ipl2glr.js $(GTA3)/data/maps/props.IPL
	touch ipl

ipl/%.gltf: ipl glr2gltf.js
	cd ipl && ../glr2gltf.js < $*.glr > $*.gltf

maps: ipl/comnbtm.gltf ipl/comntop.gltf ipl/comse.gltf ipl/comsw.gltf \
	ipl/industne.gltf ipl/industnw.gltf ipl/industse.gltf ipl/industsw.gltf \
	ipl/landne.gltf ipl/landsw.gltf ipl/overview.gltf ipl/props.gltf
