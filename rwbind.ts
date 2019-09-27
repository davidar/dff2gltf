import Module from "./rw";

export let M = null; // TODO no export

export class NullPointerException {}

export class CObject {
  private p: number;
  constructor(ptr: number) {
    if (ptr) {
      this.p = ptr;
    } else {
      throw new NullPointerException();
    }
  }
  get ptr() {
    if (this.p) {
      return this.p;
    } else {
      throw new NullPointerException();
    }
  }
  is(other: CObject) {
    return this.ptr === other.ptr;
  }
  delete() {
    delete this.p;
  }
}

export enum PluginID {
  // Core
  ID_NAOBJECT      = 0x00,
  ID_STRUCT        = 0x01,
  ID_STRING        = 0x02,
  ID_EXTENSION     = 0x03,
  ID_CAMERA        = 0x05,
  ID_TEXTURE       = 0x06,
  ID_MATERIAL      = 0x07,
  ID_MATLIST       = 0x08,
  ID_WORLD         = 0x0B,
  ID_MATRIX        = 0x0D,
  ID_FRAMELIST     = 0x0E,
  ID_GEOMETRY      = 0x0F,
  ID_CLUMP         = 0x10,
  ID_LIGHT         = 0x12,
  ID_ATOMIC        = 0x14,
  ID_TEXTURENATIVE = 0x15,
  ID_TEXDICTIONARY = 0x16,
  ID_IMAGE         = 0x18,
  ID_GEOMETRYLIST  = 0x1A,
  ID_ANIMANIMATION = 0x1B,
  ID_RIGHTTORENDER = 0x1F,
  ID_UVANIMDICT    = 0x2B,

  // Toolkit
  ID_SKYMIPMAP     = 0x110,
  ID_SKIN          = 0x116,
  ID_HANIM         = 0x11E,
  ID_USERDATA      = 0x11F,
  ID_MATFX         = 0x120,
  ID_PDS           = 0x131,
  ID_ADC           = 0x134,
  ID_UVANIMATION   = 0x135,

  // World
  ID_MESH          = 0x50E,
  ID_NATIVEDATA    = 0x510,
  ID_VERTEXFMT     = 0x511,

  // custom native raster
  ID_RASTERGL      = 0xA02,
  ID_RASTERPS2     = 0xA04,
  ID_RASTERXBOX    = 0xA05,
  ID_RASTERD3D8    = 0xA08,
  ID_RASTERD3D9    = 0xA09,
  ID_RASTERWDGL    = 0xA0B,
  ID_RASTERGL3     = 0xA0C,

  // anything driver/device related (only as allocation tag)
  ID_DRIVER        = 0xB00
}

export class Stream extends CObject {}

export class StreamMemory extends Stream {
  private data: number;
  constructor(buf: ArrayBuffer) {
    super(M._rw_StreamMemory_new());
    const size = buf.byteLength;
    this.data = M._malloc(size);
    M.HEAPU8.set(new Uint8Array(buf), this.data);
    M._rw_StreamMemory_open(this.ptr, this.data, size);
  }
  delete() {
    M._rw_StreamMemory_close(this.ptr);
    M._free(this.data);
    this.data = null;
    M._rw_StreamMemory_delete(this.ptr);
    super.delete();
  }
}

export class ChunkHeaderInfo extends CObject {
  constructor(stream: Stream) {
    super(M._rw_ChunkHeaderInfo_new());
    if (!M._rw_readChunkHeaderInfo(stream.ptr, this.ptr)) {
      throw new Error("rw::readChunkHeaderInfo failed");
    }
  }
  get type(): PluginID {
    return M._rw_ChunkHeaderInfo_type(this.ptr);
  }
  delete() {
    M._rw_ChunkHeaderInfo_delete(this.ptr);
    super.delete();
  }
}

export class TexDictionary extends CObject {
  constructor(stream: Stream) {
    super(M._rw_TexDictionary_streamRead(stream.ptr));
  }
  setCurrent() {
    M._rw_TexDictionary_setCurrent(this.ptr);
  }
  get textures() {
    return new LinkList(M._rw_TexDictionary_textures(this.ptr));
  }
  delete() {
    M._rw_TexDictionary_delete(this.ptr);
    super.delete();
  }
}

export class LinkList extends CObject {
  get begin() {
    return this.end.next;
  }
  get end() {
    return new LLLink(M._rw_LinkList_end(this.ptr));
  }
}

export class LLLink extends CObject {
  get next() {
    return new LLLink(M._rw_LLLink_next(this.ptr));
  }
}

export class Texture extends CObject {
  static fromDict(lnk: LLLink) {
    return new Texture(M._rw_Texture_fromDict(lnk.ptr));
  }
  get name(): string {
    return M.UTF8ToString(M._rw_Texture_name(this.ptr));
  }
  get raster() {
    return new Raster(M._rw_Texture_raster(this.ptr));
  }
}

export class Raster extends CObject {
  toImage() {
    return new Image(M._rw_Raster_toImage(this.ptr));
  }
}

export class Image extends CObject {
  get bpp(): number {
    return M._rw_Image_bpp(this.ptr);
  }
  get depth(): number {
    return M._rw_Image_depth(this.ptr);
  }
  get width(): number {
    return M._rw_Image_width(this.ptr);
  }
  get height(): number {
    return M._rw_Image_height(this.ptr);
  }
  pixels() {
    return new Uint8Array(M.HEAPU8.buffer,
      M._rw_Image_pixels(this.ptr), this.bpp * this.width * this.height);
  }
  hasAlpha() {
    return M._rw_Image_hasAlpha(this.ptr) !== 0;
  }
  unindex() {
    M._rw_Image_unindex(this.ptr);
  }
  delete() {
    M._rw_Image_destroy(this.ptr);
    super.delete();
  }
}

export class UVAnimDictionary extends CObject {
  static streamRead(stream: Stream) {
    return new UVAnimDictionary(M._rw_UVAnimDictionary_streamRead(stream.ptr));
  }
  static set current(d: UVAnimDictionary) {
    let p = (d === null) ? 0 : d.ptr;
    M._rw_currentUVAnimDictionary_set(p);
  }
}

export class Clump extends CObject {
  static streamRead(stream: Stream) {
    return new Clump(M._rw_Clump_streamRead(stream.ptr));
  }
  delete() {
    M._rw_Clump_destroy(this.ptr);
    super.delete();
  }
}

interface InitOpt {
  locateFile?(path: string, prefix: string): string;
  loadTextures?: boolean;
  gtaPlugins?: boolean;
}

export function init(opt: InitOpt) {
  return new Promise(resolve => Module(opt).then(module => {
    M = module;
    M._rw_Engine_init();
    if (opt.gtaPlugins) M._gta_attachPlugins();
    M._rw_Engine_open();
    M._rw_Engine_start(0);
    if (opt.loadTextures) M._rw_Texture_setLoadTextures(opt.loadTextures ? 1 : 0);
    resolve();
  }));
}