CC = clang++
CXX = clang++
export PATH := $(PWD):$(PATH)

txd2png: txd2png.o txd.o lodepng.o
dff2glr: dff2glr.o Clump.o txd.o lodepng.o base64.o

GTA3 = $(HOME)/.steam/steam/steamapps/common/Grand\ Theft\ Auto\ 3

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

glr: txd dff2glr
	mkdir -p glr buf
	ln -svf ../txd ../buf glr
	cd glr && ls ../img/*.dff ../img/*.DFF | xargs -tn1 ../dff2glr.sh
	touch glr

ipl: ipl2glr.js dff2glr
	mkdir -p ipl
	ln -svf ../img ../buf ipl
	cd ipl && for f in $(IPL); do echo "$$f"; ../ipl2glr.js "$$f.ipl" "`dirname "$$f"`/`basename "$$f" | tr A-Z a-z`.ide"; done
	cd ipl && ../ipl2glr.js $(GTA3)/data/maps/overview.ipl && ../ipl2glr.js $(GTA3)/data/maps/props.IPL
	touch ipl

ipl/%.gltf: ipl
	cd ipl && ../glr2gltf.js < $*.glr > $*.gltf

maps: ipl/comnbtm.gltf ipl/comntop.gltf ipl/comse.gltf ipl/comsw.gltf \
	ipl/industne.gltf ipl/industnw.gltf ipl/industse.gltf ipl/industsw.gltf \
	ipl/landne.gltf ipl/landsw.gltf ipl/overview.gltf ipl/props.gltf

gta3.gltf: gta3.glr ipl glr2gltf.js
	./glr2gltf.js < gta3.glr > $@

gta3-overview.gltf: ipl glr2gltf.js
	./glr2gltf.js < ipl/overview.glr > $@

%.glb: %.gltf
	gltf-pipeline -i $< -o $@
