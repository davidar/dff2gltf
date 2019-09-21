#include "io.h"
#include "lodepng.h"
#include "txd.h"

int main(int argc, char **argv) {
    std::string fname(argv[1]);
    std::vector<char> data;
    readfile(fname, data);
    for (auto texture : loadTXD(data)) {
        lodepng::save_file(texture.png, texture.name + ".png");

        FILE* meta = fopen((texture.name + ".txt").c_str(), "w");
        fprintf(meta, "%s", texture.transparent ? "BLEND" : "OPAQUE");
        fclose(meta);

        meta = fopen((texture.name + ".json").c_str(), "w");
        fprintf(meta, "{\"minFilter\": %d, \"magFilter\": %d, ",
                texture.minFilter, texture.magFilter);
        fprintf(meta, "\"wrapS\": %d, \"wrapT\": %d}\n",
                texture.wrapS, texture.wrapT);
        fclose(meta);
    }
    return 0;
}
