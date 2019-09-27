import * as THREE from "three";
import { GLTFLoader } from "three/examples/jsm/loaders/GLTFLoader";
import { OrbitControls } from "three/examples/jsm/controls/OrbitControls";

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

function fetchAsset(asset) {
  let range = 'bytes=' + (2048 * asset.offset) + '-' + (2048 * (asset.offset + asset.size))
  return fetch('/data/models/gta3.img', { headers: { 'Range': range }})
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

async function renderModel(dffBuffer: ArrayBuffer, txdBuffer: ArrayBuffer, printModel) {
  loadTXD(txdBuffer);

  let stream = new rw.StreamMemory(dffBuffer);
  let header = new rw.ChunkHeaderInfo(stream);

  rw.UVAnimDictionary.current = null;
  if (header.type === rw.PluginID.ID_UVANIMDICT) {
    let dict = rw.UVAnimDictionary.streamRead(stream);
    rw.UVAnimDictionary.current = dict;
    header.delete();
    header = new rw.ChunkHeaderInfo(stream);
  }

  if (header.type === rw.PluginID.ID_CLUMP) {
    let clump = rw.Clump.streamRead(stream);
    let out = printModel(clump);
    clump.delete();

    let gltf = glr2gltf(out);
    var loader = new GLTFLoader();
    loader.parse(JSON.stringify(gltf), null, setup);
  }

  header.delete();
  stream.delete();
}

async function main() {
  init();
  const urlParams = new URLSearchParams(window.location.search);
  await rw.init({
    gtaPlugins: true,
    loadTextures: false,
    locateFile: function (path, prefix) {
      console.log("locateFile", path, prefix);
      return prefix + "/wasm/" + path;
    }
  });

  fetch("/data/models/gta3.dir").then(response => response.arrayBuffer()).then(dir => {
    try {
      let assets = rw.M.loadDIR(rw.M.to_bytes(dir));
      let assetDFF = rw.M.findEntry(assets, urlParams.get("dff") + ".dff");
      let assetTXD = rw.M.findEntry(assets, urlParams.get("txd") + ".txd");
      fetchAsset(assetDFF).then(response => response.arrayBuffer()).then(dff => {
        fetchAsset(assetTXD).then(response => response.arrayBuffer()).then(txd => {
          renderModel(dff, txd, o => rw.M.UTF8ToString(rw.M._printModelC(o.ptr)));
        });
      });
    } catch (e) {
      console.error(e);
    }
  })
}

main().catch(console.error);
