CC = clang++
CXX = clang++
DEBUG_FLAGS = -g -fsanitize=address
CXXFLAGS = $(shell pkg-config --cflags Magick++) $(DEBUG_FLAGS)
LDFLAGS = $(shell pkg-config --libs Magick++) $(DEBUG_FLAGS)

dff2glr: dff2glr.o Clump.o

GTA3 = $(HOME)/.steam/steam/steamapps/common/Grand\ Theft\ Auto\ 3

IPL = \
	$(GTA3)/data/maps/comnbtm/comNbtm.ipl \
	$(GTA3)/data/maps/comntop/comNtop.ipl \
	$(GTA3)/data/maps/comse/comSE.ipl \
	$(GTA3)/data/maps/comsw/comSW.ipl \
	$(GTA3)/data/maps/industne/industNE.ipl \
	$(GTA3)/data/maps/industnw/industNW.ipl \
	$(GTA3)/data/maps/industse/industSE.ipl \
	$(GTA3)/data/maps/industsw/industSW.ipl \
	$(GTA3)/data/maps/landne/landne.ipl \
	$(GTA3)/data/maps/landsw/landsw.ipl \
	$(GTA3)/data/maps/overview.ipl \
	$(GTA3)/data/maps/props.IPL

img: img2files
	mkdir -p img
	cd img && ../img2files $(GTA3)/models/gta3 && cd ..

txd: img txd2png
	mkdir -p txd
	cd txd && ls ../img/*.txd ../img/*.TXD | xargs -tn1 ../txd2png && cd ..
	touch txd

glr: txd dff2glr
	mkdir -p glr buf
	ln -svf ../txd ../buf glr
	cd glr && ls ../img/*.dff ../img/*.DFF | xargs -tn1 ../dff2glr.sh
	touch glr

ipl: glr ipl2glr.js
	mkdir -p ipl
	ln -svf ../txd ../buf ../glr ipl
	cd ipl && for f in $(IPL); do echo "$$f"; ../ipl2glr.js "$$f"; done
	touch ipl

ipl/%.gltf: ipl
	cd ipl && ../glr2gltf.js < $*.glr > $*.gltf

gta3.gltf: gta3.glr ipl glr2gltf.js
	./glr2gltf.js < gta3.glr > gta3.gltf

%.glb: %.gltf
	gltf-pipeline -i $< -o $@
