
--- @module luarocks.path_cmd
-- Driver for the `luarocks path` command.
local path_cmd = {}

local util = require("luarocks.util")
local cfg = require("luarocks.core.cfg")
local fs = require("luarocks.fs")

function path_cmd.add_to_parser(parser)
   local cmd = parser:command("path", [[
Returns the package path currently configured for this installation
of LuaRocks, formatted as shell commands to update LUA_PATH and LUA_CPATH. 

On Unix systems, you may run: 
  eval `luarocks path`
And on Windows: 
  luarocks path > "%temp%\_lrp.bat" && call "%temp%\_lrp.bat" && del "%temp%\_lrp.bat"]],
  util.see_also())
      :summary("Return the currently configured package path.")

   cmd:flag("--no-bin", "Do not export the PATH variable.")
   cmd:flag("--append", "Appends the paths to the existing paths. Default is "..
      "to prefix the LR paths to the existing paths.")
   cmd:flag("--lr-path", "Exports the Lua path (not formatted as shell command).")
   cmd:flag("--lr-cpath", "Exports the Lua cpath (not formatted as shell command).")
   cmd:flag("--lr-bin", "Exports the system path (not formatted as shell command).")
   cmd:flag("--bin"):hidden(true)
end

--- Driver function for "path" command.
-- @return boolean This function always succeeds.
function path_cmd.command(args)
   local lr_path, lr_cpath, lr_bin = cfg.package_paths(args.tree)
   local path_sep = cfg.export_path_separator

   if args.lr_path then
      util.printout(util.cleanup_path(lr_path, ';', cfg.lua_version))
      return true
   elseif args.lr_cpath then
      util.printout(util.cleanup_path(lr_cpath, ';', cfg.lua_version))
      return true
   elseif args.lr_bin then
      util.printout(util.cleanup_path(lr_bin, path_sep))
      return true
   end
   
   if args.append then
      lr_path = package.path .. ";" .. lr_path
      lr_cpath = package.cpath .. ";" .. lr_cpath
      lr_bin = os.getenv("PATH") .. path_sep .. lr_bin
   else
      lr_path =  lr_path.. ";" .. package.path
      lr_cpath = lr_cpath .. ";" .. package.cpath
      lr_bin = lr_bin .. path_sep .. os.getenv("PATH")
   end
   
   local lpath_var, lcpath_var = util.lua_path_variables()

   util.printout(fs.export_cmd(lpath_var, util.cleanup_path(lr_path, ';', cfg.lua_version)))
   util.printout(fs.export_cmd(lcpath_var, util.cleanup_path(lr_cpath, ';', cfg.lua_version)))
   if not args.no_bin then
      util.printout(fs.export_cmd("PATH", util.cleanup_path(lr_bin, path_sep)))
   end
   return true
end

return path_cmd
