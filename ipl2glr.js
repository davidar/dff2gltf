#!/usr/bin/env node
// https://gtamods.com/wiki/Item_Placement

const fs = require('fs')
const path = require('path')

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

let lod = new Set()

if (process.argv.length > 3) {
  read(process.argv[3], (section, line) => {
    if (section === 'objs') {
      const row = line.split(', ')
      const id = row[0]
      const model = row[1]
      const txd = row[2]
      const dist = (row.length > 5) ? row[4] : row[3]
      const flags = row[row.length-1]
      if (dist >= 300) lod.add(id)
    }
  })
}

const fname = process.argv[2]
const ipl = fs.readFileSync(fname).toString().split('\n')
const name = path.basename(fname.toLowerCase(), '.ipl')

let nodes = []
read(fname, (section, line) => {
  if (section === 'inst') {
    const [id, model, posX, posY, posZ, scaleX, scaleY, scaleZ, rotX, rotY, rotZ, rotW] = line.split(', ')
    const pos = [Number(posX), Number(posY), Number(posZ)]
    const scale = [Number(scaleX), Number(scaleY), Number(scaleZ)]
    const rot = [Number(rotX), Number(rotY), Number(rotZ), -Number(rotW)]
    if (lod.has(id)) return
    nodes.push({ $ref: 'glr/' + model + '.glr', name: model + '.' + id,
      translation: pos, rotation: rot, scale: scale })
  }
})

let model = {
  asset: { generator: 'dff2gltf', version: '2.0' },
  scene: { nodes: [{
    name: name, children: nodes, rotation: [0.5,0.5,0.5,-0.5]
  }] }
}

fs.writeFileSync(name + '.glr', JSON.stringify(model, null, 2))
