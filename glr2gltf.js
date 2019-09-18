#!/usr/bin/env node

const fs = require('fs');

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

function plural(word) {
  if (word === 'mesh') return word + 'es'
  return word + 's'
}

function push_unique(arr, elt) {
    for (let i = 0; i < arr.length; i++) {
      if (JSON.stringify(arr[i]) === JSON.stringify(elt)) {
        return i
      }
    }
    return arr.push(elt) - 1
}

function lift(obj, type) {
  if (typeof type === 'string' || type instanceof String) {
    const types = plural(type)
    if (!model.hasOwnProperty(types)) model[types] = []
    if (schemas.hasOwnProperty(type)) lift(obj, schemas[type])
    return push_unique(model[types], obj)
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
console.log(JSON.stringify(model, null, 2))
