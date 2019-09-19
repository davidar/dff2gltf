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

gltf: txd dff2glr
	mkdir -p gltf
	ln -svf ../txd gltf
	cd gltf && ls ../img/*.dff ../img/*.DFF | xargs -tn1 ../dff2gltf && cd ..

glr: txd dff2glr
	mkdir -p glr
	ln -svf ../txd glr
	cd glr && ls ../img/*.dff ../img/*.DFF | xargs -tn1 ../dff2glr.sh
	cd glr && for f in $(IPL); do echo "$$f"; ../ipl2glr.js "$$f"; done

gta3.gltf: gta3.glr glr
	cd glr && ../glr2gltf.js < ../gta3.glr > ../gta3.gltf

gta3.glb: gta3.gltf
	cd glr && cp -f ../gta3.gltf . && gltf-pipeline -i gta3.gltf -o ../gta3.glb && rm -f gta3.gltf
