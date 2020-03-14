package = "binaryheap"
version = "0.4-1"

source = {
   url = "https://github.com/Tieske/binaryheap.lua/archive/version_0v4.tar.gz",
   dir = "binaryheap.lua-version_0v4"
}

description = {
   summary = "Binary heap implementation in pure Lua",
   detailed = [[
      Binary heaps are an efficient sorting algorithm. This module
      implements a plain binary heap (without reverse lookup) and a
      'unique' binary heap (with unique payloads and reverse lookup).
   ]],
   license = "MIT/X11",
   homepage = "https://github.com/Tieske/binaryheap.lua"
}
dependencies = {
   "lua >= 5.1",
}

build = {
   type = "builtin",
   modules = { binaryheap = "src/binaryheap.lua" }
}
