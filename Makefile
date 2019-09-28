CC = clang++
CXX = clang++
DEBUG_FLAGS = -g #-fsanitize=address
CXXFLAGS = -Iglm -Ilibrw -Ilibrwgta/src -std=c++14 -fno-exceptions $(DEBUG_FLAGS)
LDFLAGS = $(DEBUG_FLAGS)
EMFLAGS = -s ALLOW_MEMORY_GROWTH=1 -s MODULARIZE=1
export PATH := $(PWD):$(PATH)

LIBRW_PLATFORM = linux-amd64-null
LIBRW = librw/lib/$(LIBRW_PLATFORM)/Release/librw.a
LIBRWGTA = librwgta/lib/$(LIBRW_PLATFORM)/Release/librwgta.a
LIBRW_SOURCES = $(shell ls librw/src/*.cpp librw/src/*/*.cpp librwgta/src/*.cpp)

all: img2files txd2png dff2glr

$(LIBRW): $(LIBRW_SOURCES)
	cd librw && premake5 gmake && $(MAKE) -C build config=release_$(LIBRW_PLATFORM)
$(LIBRWGTA): $(LIBRW_SOURCES)
	cd librwgta && LIBRW=../librw premake5 gmake && $(MAKE) -C build config=release_$(LIBRW_PLATFORM) librwgta

img2files: img2files.o dir.o
txd2png: txd2png.o lodepng.o base64.o $(LIBRW)
dff2glr: dff2glr.o dir.o lodepng.o base64.o $(LIBRWGTA) $(LIBRW)

rw.js: librw-bindings.cc $(LIBRW_SOURCES) dff2glr.cc lodepng.cpp base64.cpp
	em++ $(CXXFLAGS) $(EMFLAGS) $^ -o $@ --bind

data:
	mkdir -p data/models
	cp $(GTA3)/models/gta3.dir $(GTA3)/models/gta3.img $(GTA3)/models/generic.txd data/models
	cp -rv $(GTA3)/data data/

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

txd: img txd2png
	mkdir -p txd
	cd txd && ls ../img/*.txd ../img/*.TXD | xargs -tn1 ../txd2png && cd ..
	touch txd

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
