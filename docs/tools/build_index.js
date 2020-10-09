#!/usr/bin/env node

const lunr = require('lunr')
const xml2js = require('xml2js')
const fs = require('fs')

if (process.argv.length < 4) {
  console.log(`Usage: ${__filename} <output> <files...>`)
  process.exit(1)
}
const outfile = process.argv[2]
const files = process.argv.slice(3)

let builder = new lunr.Builder()
builder.pipeline.add(
  lunr.trimmer,
  lunr.stopWordFilter,
  lunr.stemmer
)

builder.ref('path')
builder.field('title')
builder.field('body')

function extractText(obj) {
  if (typeof obj == 'string') return obj
  let result = ''
  for (let o of Array.isArray(obj) ? obj : Object.values(obj)) {
    result += extractText(o) + '\n'
  }
  return result
}

let titles = {}
for (let file of files) {
  let contents = fs.readFileSync(file)
  let xml;
  xml2js.parseString(contents, {async: false}, (err, result) => {
    if (err) { console.error(file, err); process.exit(1) }
    xml = result
  })
  // sdk/script/fn/Explode.xml => script/fn/Explode.html
  let doc = {path: file.replace(/^.*sdk(-de)?\//, '').slice(0, -3) + 'html'}
  if ('doc' in xml) {
    doc.title = xml.doc.title.toString()
    doc.body = extractText(xml.doc)
  } else if ('funcs' in xml) {
    let fn = 'func' in xml.funcs ? xml.funcs.func[0] : xml.funcs.const[0]
    doc.title = fn.title.toString()
    doc.body = fn.desc.toString()
  }
  builder.add(doc)
  titles[doc.path] = doc.title
}

let index = builder.build()
fs.writeFileSync(outfile, JSON.stringify({index, titles}))
