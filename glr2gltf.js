#!/usr/bin/env node

const fs = require('fs')

const schemas = {
  "scene": {
    "nodes": ["node"]
  },
  "node": {
    "camera": "",
    "children": ["node"],
    "skin": "",
    "mesh": ""
  },
  "skin": {
    "inverseBindMatrices": "accessor",
    "skeleton": "node",
    "joints": ["node"]
  },
  "mesh": {
    "primitives": [{
      "attributes": {
        "*": "accessor"
      },
      "indices": "accessor",
      "material": "",
      "targets": {
        "*": "accessor"
      }
    }]
  },
  "accessor": {
    "bufferView": ""
  },
  "material": {
    "pbrMetallicRoughness": {
      "baseColorTexture": {
        "index": "texture"
      },
      "metallicRoughnessTexture": {
        "index": "texture"
      }
    },
    "emissiveTexture": {
      "index": "texture"
    }
  },
  "bufferView": {
    "buffer": ""
  },
  "texture": {
    "sampler": "",
    "source": "image"
  },
  "image": {
    "bufferView": ""
  },
  "animation": {
    "channels": [{
      "sampler": ""
    }],
    "samplers": [{
      "input": "accessor",
      "output": "accessor"
    }]
  }
}

let contents = fs.readFileSync('/dev/stdin').toString()
if (!contents) process.exit()

let model = JSON.parse(contents)
let named = model.named
delete model.named

function plural(word) {
  if (word === 'mesh') return word + 'es'
  return word + 's'
}

function push_unique(arr, elt) {
    for (let i = 0; i < arr.length; i++) {
      if (arr[i] === elt) return i
    }
    return arr.push(elt) - 1
}

function lift(obj, type) {
  if (type === 'node' && obj.hasOwnProperty('$ref')) {
    console.error('Loading', obj['$ref'])
    const ext = JSON.parse(fs.readFileSync(obj['$ref']).toString())
    delete obj['$ref']
    if (ext.scene.nodes.length == 1) {
      obj = Object.assign(ext.scene.nodes[0], obj)
    } else {
      if (!obj.name) obj.name = ext.scene.name
      obj.children = ext.scene.nodes
    }
  }
  if (typeof type === 'string' || type instanceof String) {
    const types = plural(type)
    if (!model.hasOwnProperty(types)) model[types] = []
    if (obj.hasOwnProperty('children')) {
      if (obj.children.length == 0) {
        delete obj.children
      } else if (obj.children.length == 1) {
        let child = obj.children[0]
        delete obj.children
        obj = Object.assign(child, obj)
      }
    }
    if (schemas.hasOwnProperty(type)) lift(obj, schemas[type])
    if (type === 'node') {
      return model[types].push(JSON.stringify(obj)) - 1
    }
    return push_unique(model[types], JSON.stringify(obj))
  } else if (Array.isArray(type)) {
    for (let i = 0; i < obj.length; i++) {
      obj[i] = lift(obj[i], type[0])
    }
    return obj
  } else {
    for (key in obj) {
      if (type.hasOwnProperty(key)) {
        let childType = type[key]
        if (childType === '') childType = key
        obj[key] = lift(obj[key], childType)
      } else if (type.hasOwnProperty('*')) {
        let childType = type['*']
        obj[key] = lift(obj[key], childType)
      }
    }
    return obj
  }
}

model.scene = lift(model.scene, 'scene')
for (key in model) {
  if (Array.isArray(model[key])) {
    model[key] = model[key].map(s => {
      let obj = JSON.parse(s)
      if (named[obj.name]) obj = Object.assign(obj, named[obj.name])
      return obj
    })
  }
}
console.log(JSON.stringify(model, null, 2))
