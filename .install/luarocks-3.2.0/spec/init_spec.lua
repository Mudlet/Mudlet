local test_env = require("spec.util.test_env")
local run = test_env.run
local testing_paths = test_env.testing_paths
local copy_dir = test_env.copy_dir
local is_win = test_env.TEST_TARGET_OS == "windows"
local write_file = test_env.write_file
local lfs = require("lfs")

test_env.unload_luarocks()

describe("Luarocks init test #integration", function()

   setup(function()
      test_env.setup_specs()
   end)

   it("LuaRocks init with no arguments", function()
      test_env.run_in_tmp(function(tmpdir)
         local myproject = tmpdir .. "/myproject"
         lfs.mkdir(myproject)
         lfs.chdir(myproject)
         
         assert(run.luarocks("init"))
         if is_win then
            assert.truthy(lfs.attributes(myproject .. "/lua.bat"))
            assert.truthy(lfs.attributes(myproject .. "/luarocks.bat"))
         else
            assert.truthy(lfs.attributes(myproject .. "/lua"))
            assert.truthy(lfs.attributes(myproject .. "/luarocks"))
         end
         assert.truthy(lfs.attributes(myproject .. "/lua_modules"))
         assert.truthy(lfs.attributes(myproject .. "/.luarocks"))
         assert.truthy(lfs.attributes(myproject .. "/.luarocks/config-" .. test_env.lua_version .. ".lua"))
         assert.truthy(lfs.attributes(myproject .. "/.gitignore"))
         assert.truthy(lfs.attributes(myproject .. "/myproject-dev-1.rockspec"))
      end, finally)
   end)
   
   it("LuaRocks init with given arguments", function()
      test_env.run_in_tmp(function(tmpdir)
         local myproject = tmpdir .. "/myproject"
         lfs.mkdir(myproject)
         lfs.chdir(myproject)
         
         assert(run.luarocks("init customname 1.0"))
         assert.truthy(lfs.attributes(myproject .. "/customname-1.0-1.rockspec"))
      end, finally)
   end)
   
   it("LuaRocks init with --lua-versions", function()
      test_env.run_in_tmp(function(tmpdir)
         local myproject = tmpdir .. "/myproject"
         lfs.mkdir(myproject)
         lfs.chdir(myproject)

         assert(run.luarocks("init --lua-versions=5.1,5.2,5.3"))
         local rockspec_name = myproject .. "/myproject-dev-1.rockspec"
         assert.truthy(lfs.attributes(rockspec_name))
         local fd = assert(io.open(rockspec_name, "rb"))
         local data = fd:read("*a")
         fd:close()
         assert.truthy(data:find("lua >= 5.1, < 5.4", 1, true))
      end, finally)
   end)

   it("LuaRocks init in a git repo", function()
      test_env.run_in_tmp(function(tmpdir)
         local myproject = tmpdir .. "/myproject"
         copy_dir(testing_paths.fixtures_dir .. "/git_repo", myproject)
         lfs.chdir(myproject)
         
         assert(run.luarocks("init"))
         local fd = assert(io.open(myproject .. "/myproject-dev-1.rockspec", "r"))
         local content = assert(fd:read("*a"))
         assert.truthy(content:find("summary = \"Test repo\""))
         assert.truthy(content:find("detailed = .+Test repo.+"))
         assert.truthy(content:find("license = \"MIT\""))
         
         fd = assert(io.open(myproject .. "/.gitignore", "r"))
         content = assert(fd:read("*a"))
         assert.truthy(content:find("/foo"))
         assert.truthy(content:find("/lua"))
         assert.truthy(content:find("/lua_modules"))
      end, finally)
   end)

   it("LuaRocks init does not autodetect config or dependencies as modules of the package", function()
      test_env.run_in_tmp(function(tmpdir)
         local myproject = tmpdir .. "/myproject"
         lfs.mkdir(myproject)
         lfs.chdir(myproject)

         assert(run.luarocks("init"))
         assert.truthy(lfs.attributes(myproject .. "/.luarocks/config-" .. test_env.lua_version .. ".lua"))
         local rockspec_filename = myproject .. "/myproject-dev-1.rockspec"
         assert.truthy(lfs.attributes(rockspec_filename))

         -- install a package locally
         write_file("my_dependency-1.0-1.rockspec", [[
            package = "my_dependency"
            version = "1.0-1"
            source = {
               url = "file://]] .. tmpdir:gsub("\\", "/") .. [[/my_dependency.lua"
            }
            build = {
               type = "builtin",
               modules = {
                  my_dependency = "my_dependency.lua"
               }
            }
         ]], finally)
         write_file(tmpdir .. "/my_dependency.lua", "return {}", finally)

         assert.truthy(run.luarocks("build my_dependency-1.0-1.rockspec"))
         assert.truthy(lfs.attributes(myproject .. "/lua_modules/share/lua/" .. test_env.lua_version .."/my_dependency.lua"))

         os.remove(rockspec_filename)
         os.remove("my_dependency-1.0-1.rockspec")
         
         -- re-run init
         assert(run.luarocks("init"))

         -- file is recreated
         assert.truthy(lfs.attributes(rockspec_filename))
         
         local fd = assert(io.open(rockspec_filename, "rb"))
         local rockspec = assert(fd:read("*a"))
         fd:close()
         
         assert.no.match("my_dependency", rockspec, 1, true)
         assert.no.match("config", rockspec, 1, true)

      end, finally)
   end)
end)
