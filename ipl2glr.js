#!/usr/bin/env node

const fs = require('fs')
const path = require('path')

const fname = process.argv[2]
const ipl = fs.readFileSync(fname).toString().split('\n')
const name = path.basename(fname.toLowerCase(), '.ipl')
let inst = false
let nodes = []

for (const s of ipl) {
  const line = s.trim().toLowerCase()
  if (line === 'inst') {
    inst = true
  } else if (line === 'end') {
    inst = false
  } else if (inst) {
    const [id, model, posX, posY, posZ, scaleX, scaleY, scaleZ, rotX, rotY, rotZ, rotW] = line.split(', ')
    const pos = [Number(posX), Number(posY), Number(posZ)]
    const scale = [Number(scaleX), Number(scaleY), Number(scaleZ)]
    const rot = [Number(rotX), Number(rotY), Number(rotZ), Number(rotW)]
    nodes.push({ $ref: model + '.glr', name: model + '.' + id,
      translation: pos, rotation: rot, scale: scale })
  }
}

let model = {
  asset: { generator: 'dff2gltf', version: '2.0' },
  scene: { name, nodes }
}

fs.writeFileSync(name + '.glr', JSON.stringify(model, null, 2))
