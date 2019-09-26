import RW from "./dff2glr.js"
import {glr2gltf} from "./glr.js"

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

  controls = new THREE.OrbitControls(camera, renderer.domElement)
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

function renderModel(Module, dffBuffer, txdBuffer) {
  Module._rw_Engine_init()
  Module._gta_attachPlugins()
  Module._rw_Engine_open()
  Module._rw_Engine_start(0)
  Module._rw_Texture_setLoadTextures(0)

  let stream = Module._rw_StreamMemory_new()
  let data = Module._malloc(txdBuffer.byteLength)
  Module.HEAP8.set(new Int8Array(txdBuffer), data)
  Module._rw_StreamMemory_open(stream, data, txdBuffer.byteLength)

  let header = Module._rw_ChunkHeaderInfo_new()
  Module._rw_readChunkHeaderInfo(stream, header)
  const ID_TEXDICTIONARY = 22
  console.assert(Module._rw_ChunkHeaderInfo_type(header) === ID_TEXDICTIONARY,
    'header.type == ID_TEXDICTIONARY')

  let txd = Module._rw_TexDictionary_streamRead(stream)
  console.assert(txd, 'txd')
  Module._rw_TexDictionary_setCurrent(txd)

  Module._rw_StreamMemory_close(stream)
  Module._free(data)

  data = Module._malloc(dffBuffer.byteLength)
  Module.HEAP8.set(new Int8Array(dffBuffer), data)
  Module._rw_StreamMemory_open(stream, data, dffBuffer.byteLength)

  Module._rw_currentUVAnimDictionary_set(0)
  Module._rw_readChunkHeaderInfo(stream, header)
  const ID_UVANIMDICT = 43
  if (Module._rw_ChunkHeaderInfo_type(header) === ID_UVANIMDICT) {
    let dict = Module._rw_UVAnimDictionary_streamRead(stream)
    Module._rw_currentUVAnimDictionary_set(dict)
    Module._rw_readChunkHeaderInfo(stream, header)
  }

  const ID_CLUMP = 16
  if (Module._rw_ChunkHeaderInfo_type(header) === ID_CLUMP) {
    let clump = Module._rw_Clump_streamRead(stream)
    console.assert(clump, 'clump')

    let out = Module.UTF8ToString(Module._printModelC(clump))
    Module._rw_Clump_destroy(clump)

    let gltf = glr2gltf(out)
    var loader = new THREE.GLTFLoader()
    loader.load('data:application/json,' + encodeURIComponent(JSON.stringify(gltf)), setup)
  }

  Module._rw_ChunkHeaderInfo_delete(header)
  Module._rw_StreamMemory_close(stream)
  Module._free(data)
  Module._rw_StreamMemory_delete(stream)
}

RW().then(Module => {
  init()
  const urlParams = new URLSearchParams(window.location.search)
  fetch('/data/models/gta3.dir').then(response => response.blob()).then(blob => blob.arrayBuffer()).then(dir => {
    try {
      let assets = Module.loadDIR(Module.to_bytes(dir))
      let assetDFF = Module.findEntry(assets, urlParams.get('dff') + '.dff')
      let assetTXD = Module.findEntry(assets, urlParams.get('txd') + '.txd')
      fetchAsset(assetDFF).then(response => response.blob()).then(blob => blob.arrayBuffer()).then(dff => {
        fetchAsset(assetTXD).then(response => response.blob()).then(blob => blob.arrayBuffer()).then(txd => {
          renderModel(Module, dff, txd)
        })
      })
    } catch (e) {
      console.error(e)
    }
  })
})
