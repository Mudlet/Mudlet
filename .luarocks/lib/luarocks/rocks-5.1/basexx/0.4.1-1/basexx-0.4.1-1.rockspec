package = "basexx"
version = "0.4.1-1"

description = {
   summary  = "A base2, base16, base32, base64 and base85 library for Lua",
   detailed = "A Lua library which provides base2(bitfield), base16(hex), base32(crockford/rfc), base64(rfc/url), base85(z85) decoding and encoding.",
   license  = "MIT",
   homepage = "https://github.com/aiq/basexx"
}

source = {
   url = "https://github.com/aiq/basexx/archive/v0.4.1.tar.gz",
   md5 = "85fda02f7068183ced02d88696972e81",
   dir = "basexx-0.4.1"
}

dependencies = {
   "lua >= 5.1"
}

build = {
   type = 'builtin',
   modules = {
      basexx = "lib/basexx.lua"
   },
   copy_directories = { "test" }
}
