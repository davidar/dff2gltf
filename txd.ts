import * as rw from "./rwbind";

async function main() {
  await rw.init({
    loadTextures: false,
    locateFile: function (path, prefix) {
      console.log("locateFile", path, prefix);
      return prefix + "/wasm/" + path;
    }
  });

  let response = await fetch("/data/models/generic.txd");
  let stream = new rw.StreamMemory(await response.arrayBuffer());
  let header = new rw.ChunkHeaderInfo(stream);
  console.assert(header.type === rw.PluginID.ID_TEXDICTIONARY);
  header.delete();

  let txd = new rw.TexDictionary(stream);
  txd.setCurrent();

  stream.delete();

  for (let lnk = txd.textures.begin; !lnk.is(txd.textures.end); lnk = lnk.next) {
    let tex = rw.Texture.fromDict(lnk);
    let img = tex.raster.toImage();
    img.unindex();

    if (img.depth < 24) {
      console.warn("ignoring 16-bit texture", tex.name);
    } else {
      let c = document.createElement("canvas");
      c.width = img.width;
      c.height = img.height;
      c.title = tex.name;

      let ctx = c.getContext("2d");
      let buf = ctx.createImageData(img.width, img.height);
      let pixels = img.pixels;
      if (img.hasAlpha()) {
        buf.data.set(pixels);
      } else {
        for (let i = 0, j = 0; i < buf.data.length;) {
          buf.data[i++] = pixels[j++];
          buf.data[i++] = pixels[j++];
          buf.data[i++] = pixels[j++];
          buf.data[i++] = 0xff;
        }
      }
      ctx.putImageData(buf, 0, 0);
      document.body.appendChild(c);
    }

    img.delete();
  }
}

main().catch(console.error);
