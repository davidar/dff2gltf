#!/usr/bin/env node

const fs = require('fs')
const {glr2gltf} = require('./glr.js')

let contents = fs.readFileSync('/dev/stdin').toString()
if (!contents) process.exit()

let model = glr2gltf(contents)
console.log(JSON.stringify(model, null, 2))
