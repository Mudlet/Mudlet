  const fs = require('fs');
  const jsonabc = require('jsonabc');
  const data = fs.readFileSync('src/lua-function-list.json');
  const parseddata = JSON.parse(data);
  const sorted = jsonabc.sortObj(parseddata);
  fs.writeFileSync('src/lua-function-list.json', JSON.stringify(sorted));
