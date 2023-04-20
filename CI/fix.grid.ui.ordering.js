const fs = require("fs").promises;
const convert = require("xml-js");

const ATTRIBUTE_KEY = "_attributes";
const GRID_LAYOUT_CLASS = "QGridLayout";

const uiDir = `${__dirname}/../src/ui`;

function getCellOrder(node, attrKey) {
  let attrs = node[attrKey];
  let row = parseInt(attrs.row);
  let col = parseInt(attrs.column);
  return row * 1000 + col;
}

function visitNode(node) {
  if (typeof node !== "object") {
    return;
  }

  const subject = Array.isArray(node) ? node : Object.values(node);
  subject.forEach(item => visitNode(item))

  if (node[ATTRIBUTE_KEY]?.class === GRID_LAYOUT_CLASS && Array.isArray(node.item)) {
    node.item.sort(
      (a, b) => getCellOrder(a, ATTRIBUTE_KEY) - getCellOrder(b, ATTRIBUTE_KEY)
    );
  }
}

function visitUiFile(file) {
  const filePath = `${uiDir}/${file}`
  fs.readFile(filePath).then(contents => {
    var uiDefinition = convert.xml2js(contents, { compact: true });
    visitNode(uiDefinition)
    var result = convert.js2xml(uiDefinition, { compact: true, spaces: 1})
    fs.writeFile(filePath, result + "\n")
  })  
}


fs.readdir( uiDir ).then(files => files.forEach(visitUiFile))
