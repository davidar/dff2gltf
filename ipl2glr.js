#!/usr/bin/env node
// https://gtamods.com/wiki/Item_Placement

const fs = require('fs')
const path = require('path')
const child_process = require('child_process')

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

let img = {}

for (const fname of fs.readdirSync('img')) {
  img[fname.toLowerCase()] = 'img/' + fname
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
read(fname, (section, line) => {
  if (section === 'inst') {
    const [id, model, posX, posY, posZ, scaleX, scaleY, scaleZ, rotX, rotY, rotZ, rotW] = line.split(', ')
    if (model.startsWith('lod')) return
    if (!glr[model]) {
      const dff = img[model + '.dff']
      const tex = img[txd[model] + '.txd']
      console.log(model, txd[model])
      glr[model] = child_process.execFileSync('dff2glr', [dff, tex])
    }
    let node = JSON.parse(glr[model]).scene.nodes[0]
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
})

let model = {
  asset: { generator: 'dff2gltf', version: '2.0' },
  scene: { nodes: [{
    name: name, children: nodes, rotation: [0.5,0.5,0.5,-0.5]
  }] }
}

fs.writeFileSync(name + '.glr', JSON.stringify(model, null, 2))
