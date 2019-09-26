import RW from './rw.js'

function main(Module, buf) {
  Module._rw_Engine_init()
  Module._rw_Engine_open()
  Module._rw_Engine_start(0)
  Module._rw_Texture_setLoadTextures(0)

  let data = Module._malloc(buf.byteLength)
  let stream = Module._rw_StreamMemory_new()
  Module.HEAP8.set(new Int8Array(buf), data)
  Module._rw_StreamMemory_open(stream, data, buf.byteLength)

  let header = Module._rw_ChunkHeaderInfo_new()
  Module._rw_readChunkHeaderInfo(stream, header)
  const ID_TEXDICTIONARY = 22
  console.assert(Module._rw_ChunkHeaderInfo_type(header) === ID_TEXDICTIONARY,
    'header.type == ID_TEXDICTIONARY')
  Module._rw_ChunkHeaderInfo_delete(header)

  let txd = Module._rw_TexDictionary_streamRead(stream)
  console.assert(txd, 'txd')
  Module._rw_TexDictionary_setCurrent(txd)

  Module._rw_StreamMemory_close(stream)
  Module._rw_StreamMemory_delete(stream)
  Module._free(data)

  let textures = Module._rw_TexDictionary_textures(txd)
  for (let lnk = Module._rw_LinkList_begin(textures)
      ;  lnk !== Module._rw_LinkList_end(textures)
      ;    lnk = Module._rw_LLLink_next(lnk)) {
    let tex = Module._rw_Texture_fromDict(lnk)
    let raster = Module._rw_Texture_raster(tex)
    let name = Module.UTF8ToString(Module._rw_Texture_name(tex))
    let img = Module._rw_Raster_toImage(raster)
    Module._rw_Image_unindex(img)
    let depth = Module._rw_Image_depth(img)

    if (depth < 24) {
      console.warn('ignoring 16-bit texture', name)
    } else {
      let c = document.createElement('canvas')
      let pixels = Module._rw_Image_pixels(img)
      let width = Module._rw_Image_width(img)
      let height = Module._rw_Image_height(img)
      let hasAlpha = Module._rw_Image_hasAlpha(img)
      c.width = width
      c.height = height
      c.title = name

      let ctx = c.getContext('2d')
      let imgdata = ctx.createImageData(width, height)
      let pixelArray = new Uint8Array(Module.HEAP8.buffer,
        pixels, (depth / 8) * width * height)
      if (hasAlpha) {
        imgdata.data.set(pixelArray)
      } else {
        for (let i = 0, j = 0; i < 4 * width * height;) {
          imgdata.data[i++] = pixelArray[j++]
          imgdata.data[i++] = pixelArray[j++]
          imgdata.data[i++] = pixelArray[j++]
          imgdata.data[i++] = 0xff
        }
      }
      ctx.putImageData(imgdata, 0, 0)
      document.body.appendChild(c)
    }

    Module._rw_Image_destroy(img)
  }
}

RW().then(Module => {
  fetch('/data/models/generic.txd').then(response => response.blob()).then(blob => blob.arrayBuffer()).then(buf => {
    try {
      main(Module, buf)
    } catch(e) {
      console.error(e)
    }
  })
})
