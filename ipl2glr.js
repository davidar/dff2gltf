#!/usr/bin/env node
// https://gtamods.com/wiki/Item_Placement

const fs = require('fs')
const path = require('path')
const Module = require('./dff2glr.js')

Module.onRuntimeInitialized = main

const GTA3 = process.env.GTA3.replace(/\\/g,'')

function readAsset(asset) {
  let offset = 2048 * asset.offset
  let size = 2048 * asset.size
  let f = fs.openSync(GTA3 + '/models/gta3.img', 'r')
  let buf = Buffer.alloc(size)
  fs.readSync(f, buf, 0, size, offset)
  fs.closeSync(f)
  return buf
}

function dff2glr(dff, txd) {
  let dir = fs.readFileSync(GTA3 + '/models/gta3.dir')
  let assets = Module.loadDIR(Module.to_bytes(dir))

  let assetDFF = readAsset(Module.findEntry(assets, dff + '.dff'))
  let model    = Module.loadDFF(Module.to_bytes(assetDFF))

  let assetTXD = (txd === 'generic')
               ? fs.readFileSync(GTA3 + '/models/generic.txd')
               : readAsset(Module.findEntry(assets, txd + '.txd'))
  let textures = Module.loadTXD(Module.to_bytes(assetTXD))

  return Module.printModel(model, textures)
}

function read(fname, cb) {
  const lines = fs.readFileSync(fname).toString().split('\n')
  let section = null
  for (const s of lines) {
    const line = s.trim().toLowerCase()
    if (line === '' || line[0] === '#') continue
    if (section === null) {
      section = line
    } else if (line === 'end') {
      section = null
    } else {
      cb(section, line)
    }
  }
}

let txd = {}

const fname = process.argv[2]
const name = path.basename(fname.toLowerCase(), '.ipl')

let ides = [fname.match(/.*\/maps\//) + 'gta3.IDE']
if (process.argv.length > 3) {
  ides.push(process.argv[3])
}
for (const ide of ides) {
  read(ide, (section, line) => {
    if (section === 'objs' || section === 'tobj') {
      const row = line.split(', ')
      const id = row[0]
      const model = row[1]
      txd[model] = row[2]
      const dist = (row.length > 5) ? row[4] : row[3]
      const flags = (section === 'tobj') ? row[row.length-3] : row[row.length-1]
    }
  })
}

let glr = {}
let nodes = []
let named = {}

function parseItem(section, line) {
  if (section === 'inst') {
    const [id, model, posX, posY, posZ, scaleX, scaleY, scaleZ, rotX, rotY, rotZ, rotW] = line.split(', ')
    if (model.startsWith('lod')) return
    if (!glr[model]) {
      console.log(model, txd[model])
      glr[model] = dff2glr(model, txd[model])
    }
    let obj = JSON.parse(glr[model])
    named = Object.assign(named, obj.named)
    let node = obj.scene.nodes[0]
    if (node.children && node.children.length > 1) {
      for (const child of node.children) {
        if (child.name.endsWith('_l0')) { // breakable objects
          node = child
          break
        }
      }
    }
    node.name = model + '.' + id,
    node.translation = [Number(posX), Number(posY), Number(posZ)]
    node.rotation = [Number(rotX), Number(rotY), Number(rotZ), -Number(rotW)]
    node.scale = [Number(scaleX), Number(scaleY), Number(scaleZ)]
    nodes.push(node)
  }
}

function main() {
  read(fname, parseItem)

  let model = {
    asset: { generator: 'dff2gltf', version: '2.0' },
    scene: { nodes: [{
      name: name, children: nodes, rotation: [0.5,0.5,0.5,-0.5]
    }] },
    named: named
  }

  fs.writeFileSync(name + '.glr', JSON.stringify(model, null, 2))
}
