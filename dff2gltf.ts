import * as THREE from "three";
import { TypedArray } from "three";
import { GLTFLoader } from "three/examples/jsm/loaders/GLTFLoader";
import { OrbitControls } from "three/examples/jsm/controls/OrbitControls";
import * as base64js from "base64-js";

import * as rw from "./rwbind";
import { glr2gltf } from "./glr";

var container, controls
var camera, scene, renderer
var hemispheric, ambient, directional

function init() {
  container = document.createElement('div')
  document.body.appendChild(container)
  camera = new THREE.PerspectiveCamera(75, window.innerWidth / window.innerHeight, 0.1, 1000)
  scene = new THREE.Scene()

  renderer = new THREE.WebGLRenderer({ antialias: true })
  renderer.setPixelRatio(window.devicePixelRatio)
  renderer.setSize(window.innerWidth/2, window.innerHeight/2, false)
  renderer.gammaOutput = true
  renderer.setClearColor(0x222222)
  container.appendChild(renderer.domElement)

  hemispheric = new THREE.HemisphereLight(0xffffff, 0x222222, 1.2)
  scene.add(hemispheric)

  ambient = new THREE.AmbientLight(0x404040)
  scene.add(ambient)

  directional = new THREE.DirectionalLight(0x404040)
  directional.position.set(0.5, 0, 0.866)
  scene.add(directional)

  controls = new OrbitControls(camera, renderer.domElement)
  controls.screenSpacePanning = true
  controls.update()

  window.addEventListener('resize', onWindowResize, false)
  renderer.domElement.onmousemove = renderer.domElement.onkeydown = renderer.domElement.onwheel = animate
  animate()
}

function setup(gltf) {
  const box = new THREE.Box3().setFromObject(gltf.scene)
  const size = box.getSize(new THREE.Vector3()).length()
  const center = box.getCenter(new THREE.Vector3())

  gltf.scene.position.x += (gltf.scene.position.x - center.x)
  gltf.scene.position.y += (gltf.scene.position.y - center.y)
  gltf.scene.position.z += (gltf.scene.position.z - center.z)
  controls.maxDistance = size * 10
  camera.near = size / 100
  camera.far = size * 100
  camera.updateProjectionMatrix()

  camera.position.copy(center)
  camera.position.x += size / 2.0
  camera.position.y += size / 5.0
  camera.position.z += size / 2.0
  camera.lookAt(center)

  scene.add(gltf.scene)
  animate()
}

function onWindowResize() {
  camera.aspect = window.innerWidth / window.innerHeight
  camera.updateProjectionMatrix()
  renderer.setSize(window.innerWidth/2, window.innerHeight/2, false)
}

function animate() {
  renderer.render(scene, camera)
}

function UTF8ToString(array: Uint8Array) {
  let length = 0; while (length < array.length && array[length]) length++;
  return new TextDecoder().decode(array.subarray(0, length));
}

interface Asset {
  offset: number;
  size: number;
}

interface Assets {
  [name: string]: Asset;
}

function loadDIR(buf: ArrayBuffer) {
  let assets = {} as Assets;
  let view = new DataView(buf);
  for (let i = 0; i < buf.byteLength; i += 32) {
    let offset = view.getUint32(i + 0, true);
    let size   = view.getUint32(i + 4, true);
    let name = UTF8ToString(new Uint8Array(buf, i + 8, 24));
    assets[name.toLowerCase()] = { offset, size };
  }
  return assets;
}

function fetchAsset(asset: Asset) {
  let range = "bytes=" + (2048 * asset.offset) + "-" + (2048 * (asset.offset + asset.size));
  return fetch("/data/models/gta3.img", { headers: { Range: range }});
}

function loadTXD(buf: ArrayBuffer) {
  let stream = new rw.StreamMemory(buf);
  let header = new rw.ChunkHeaderInfo(stream);
  console.assert(header.type === rw.PluginID.ID_TEXDICTIONARY);
  let txd = new rw.TexDictionary(stream);
  txd.setCurrent();
  header.delete();
  stream.delete();
}

function bufferURI(array: TypedArray) {
  let bytes = new Uint8Array(array.buffer, array.byteOffset, array.byteLength);
  return "data:application/gltf-buffer;base64," + base64js.fromByteArray(bytes);
}

const GL = WebGLRenderingContext;
const addressConvMap = [null, GL.REPEAT, GL.MIRRORED_REPEAT, GL.CLAMP, GL.CLAMP_TO_BORDER];

function glrAtomic(atomic: rw.Atomic, named) {
  let geom = atomic.geometry;
  let name = atomic.frame.name.toLowerCase();

  let positions = geom.morphTarget(0).vertices;
  let attributes: {POSITION: any, NORMAL?: any, TEXCOORD_0?: any, COLOR_0?: any} = {
    POSITION: {
      bufferView: {
        buffer: { name: name + ".positions" },
        target: GL.ARRAY_BUFFER,
        byteLength: positions.byteLength
      },
      type: "VEC3",
      componentType: GL.FLOAT,
      count: geom.numVertices,
      min: [-9, -9, -9],
      max: [9, 9, 9]
    }
  };
  named[name + ".positions"] = {
    byteLength: positions.byteLength,
    uri: bufferURI(positions)
  };

  let normals = geom.morphTarget(0).normals;
  if (normals) {
    attributes.NORMAL = {
      bufferView: {
        buffer: { name: name + ".normals" },
        target: GL.ARRAY_BUFFER,
        byteLength: normals.byteLength
      },
      type: "VEC3",
      componentType: GL.FLOAT,
      count: geom.numVertices
    };
    named[name + ".normals"] = {
      byteLength: normals.byteLength,
      uri: bufferURI(normals)
    };
  }

  if (geom.numTexCoordSets) {
    let texCoords = geom.texCoords(0);
    attributes.TEXCOORD_0 = {
      bufferView: {
        buffer: { name: name + ".texCoords" },
        target: GL.ARRAY_BUFFER,
        byteLength: texCoords.byteLength
      },
      type: "VEC2",
      componentType: GL.FLOAT,
      count: geom.numVertices
    };
    named[name + ".texCoords"] = {
      byteLength: texCoords.byteLength,
      uri: bufferURI(texCoords)
    }
  }

  let colors = geom.colors;
  if (colors) {
    attributes.COLOR_0 = {
      bufferView: {
        buffer: { name: name + ".colors" },
        target: GL.ARRAY_BUFFER,
        byteLength: colors.byteLength
      },
      type: "VEC4",
      componentType: GL.UNSIGNED_BYTE,
      count: geom.numVertices,
      normalized: true
    };
    named[name + ".colors"] = {
      byteLength: colors.byteLength,
      uri: bufferURI(colors)
    }
  }

  let primitives = [];
  let h = geom.meshHeader;
  for (let i = 0; i < h.numMeshes; i++) {
    let m = h.mesh(i);
    let indices = m.indices;
    let mat = m.material;
    let col = mat.color;

    let pbrMetallicRoughness: {metallicFactor: number,
                               baseColorFactor: number[],
                               baseColorTexture?: any} = {
      metallicFactor: 0,
      baseColorFactor: [col[0]/0xff, col[1]/0xff, col[2]/0xff, col[3]/0xff]
    };
    if (mat.texture) {
      let img = mat.texture.raster.toImage();
      img.unindex();

      if (img.depth < 24) {
        console.warn("ignoring 16-bit texture", mat.texture.name, "for atomic", name);
      } else {
        let c = document.createElement("canvas");
        c.width = img.width;
        c.height = img.height;

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

        pbrMetallicRoughness.baseColorTexture = {
          index: {
            source: {
              name: mat.texture.name,
              uri: c.toDataURL()
            },
            sampler: {
              minFilter: GL.LINEAR_MIPMAP_LINEAR,
              magFilter: (mat.texture.filter % 2) ? GL.NEAREST : GL.LINEAR,
              wrapS: addressConvMap[mat.texture.addressU],
              wrapT: addressConvMap[mat.texture.addressV]
            }
          }
        }
      }

      img.delete();
    }

    let alphaMode = "OPAQUE";
    if (mat.color[3] < 0xff || (mat.texture &&
        rw.Raster.formatHasAlpha(mat.texture.raster.format))) {
      alphaMode = "BLEND";
    }

    primitives.push({
      mode: h.tristrip ? GL.TRIANGLE_STRIP : GL.TRIANGLES,
      attributes,
      indices: {
        bufferView: {
          buffer: { name: name + ".indices_" + i },
          target: GL.ELEMENT_ARRAY_BUFFER,
          byteLength: indices.byteLength
        },
        type: "SCALAR",
        componentType: GL.UNSIGNED_SHORT,
        count: m.numIndices
      },
      material: {
        pbrMetallicRoughness,
        alphaMode,
        doubleSided: true
      }
    })

    named[name + ".indices_" + i] = {
      byteLength: indices.byteLength,
      uri: bufferURI(indices)
    }
  }

  return { name, mesh: { primitives } };
}

function glrClump(model: rw.Clump) {
  let nodes = [];
  let named = {};
  for (let lnk = model.atomics.begin; !lnk.is(model.atomics.end); lnk = lnk.next) {
    let atomic = rw.Atomic.fromClump(lnk);
    nodes.push(glrAtomic(atomic, named));
  }
  return {
    asset: { generator: "dff2gltf", version: "2.0" },
    scene: { nodes: [{
      name: model.frame.name.toLowerCase(),
      children: nodes,
      rotation: [-0.5,0.5,0.5,0.5]
    }]},
    named
  };
}

let assets = null;

async function dff2glr(dff: string, txd: string) {
  let assetDFF = assets[dff + ".dff"];
  let fetchTXD = (txd === "generic")
    ? fetch("/data/models/generic.txd")
    : fetchAsset(assets[txd + ".txd"]);
  loadTXD(await fetchTXD.then(response => response.arrayBuffer()));
  let buf = await fetchAsset(assetDFF).then(response => response.arrayBuffer());

  let stream = new rw.StreamMemory(buf);
  let header = new rw.ChunkHeaderInfo(stream);

  rw.UVAnimDictionary.current = null;
  if (header.type === rw.PluginID.ID_UVANIMDICT) {
    let dict = rw.UVAnimDictionary.streamRead(stream);
    rw.UVAnimDictionary.current = dict;
    header.delete();
    header = new rw.ChunkHeaderInfo(stream);
  }

  let out = null;
  if (header.type === rw.PluginID.ID_CLUMP) {
    let clump = rw.Clump.streamRead(stream);
    out = JSON.stringify(glrClump(clump));
    clump.delete();
  }

  header.delete();
  stream.delete();
  return out;
}

async function ipl2gltf(fname: string, ide?: string) {
  const name = fname.toLowerCase().match(/([^\/]*).ipl$/)[1];
  let txd = {};
  let glr = {};
  let nodes = [];
  let named = {};

  async function read(uri: string, cb: (section: string, line: string) => Promise<void>) {
    const text = await fetch(uri).then(response => response.text());
    const lines = text.split("\n");
    let section = null;
    for (const s of lines) {
      const line = s.trim().toLowerCase();
      if (line === "" || line[0] === "#") continue;
      if (section === null) {
        section = line;
      } else if (line === "end") {
        section = null;
      } else {
        await cb(section, line);
      }
    }
  }

  async function parseItem(section: string, line: string) {
    if (section === "inst") {
      const [id, model, posX, posY, posZ, scaleX, scaleY, scaleZ, rotX, rotY, rotZ, rotW] = line.split(", ");
      if (model.startsWith("lod")) return;
      if (!glr[model]) {
        console.log(model, txd[model]);
        glr[model] = await dff2glr(model, txd[model]);
      }
      let obj = JSON.parse(glr[model]);
      named = Object.assign(named, obj.named);
      let node = obj.scene.nodes[0];
      if (node.children && node.children.length > 1) {
        for (const child of node.children) {
          if (child.name.endsWith("_l0")) { // breakable objects
            node = child;
            break;
          }
        }
      }
      node.name = model + "." + id;
      node.translation = [Number(posX), Number(posY), Number(posZ)];
      node.rotation = [Number(rotX), Number(rotY), Number(rotZ), -Number(rotW)];
      node.scale = [Number(scaleX), Number(scaleY), Number(scaleZ)];
      nodes.push(node);
    }
  }

  let ides = [fname.match(/.*\/maps\//) + "gta3.IDE"];
  if (ide) ides.push(ide);
  for (const ide of ides) {
    await read(ide, async function (section, line) {
      if (section === "objs" || section === "tobj") {
        const row = line.split(", ");
        const id = row[0];
        const model = row[1];
        txd[model] = row[2];
        const dist = (row.length > 5) ? row[4] : row[3];
        const flags = (section === "tobj") ? row[row.length - 3] : row[row.length - 1];
      }
    })
  }

  await read(fname, parseItem);

  let model = {
    asset: { generator: "dff2gltf", version: "2.0" },
    scene: {
      nodes: [{
        name: name, children: nodes, rotation: [0.5, 0.5, 0.5, -0.5]
      }]
    },
    named: named
  };

  return glr2gltf(JSON.stringify(model, null, 2));
}

async function main() {
  init();
  await rw.init({
    gtaPlugins: true,
    loadTextures: false,
    locateFile: function (path, prefix) {
      console.log("locateFile", path, prefix);
      return prefix + "/wasm/" + path;
    }
  });

  let dir = await fetch("/data/models/gta3.dir").then(response => response.arrayBuffer());
  assets = loadDIR(dir);

  let gltf = null;
  const urlParams = new URLSearchParams(window.location.search);
  if (urlParams.get("dff")) {
    let dff = urlParams.get("dff").toLowerCase();
    let txd = urlParams.get("txd").toLowerCase();
    let out = await dff2glr(dff, txd);
    gltf = glr2gltf(out);
  } else if (urlParams.get("ipl")) {
    let ipl = "/data/data/maps/" + urlParams.get("ipl").toLowerCase() + ".ipl";
    let ide = urlParams.get("ide");
    if (ide) ide = "/data/data/maps/" + ide.toLowerCase() + ".ide";
    gltf = await ipl2gltf(ipl, ide);
  }
  let loader = new GLTFLoader();
  loader.parse(JSON.stringify(gltf), null, setup);
}

main().catch(console.error);
