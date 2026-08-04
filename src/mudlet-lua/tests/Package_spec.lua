-- Specs for the package and module lifecycle Lua APIs.
--
-- Every spec here drives the real API against the fixture kit committed in
-- fixtures/packages/ - nothing is mocked. Alongside each contract (what a call
-- returns, and what it returns when it is misused) the effect is checked too:
-- the package's files actually land under getMudletHomeDir(), its aliases and
-- scripts actually exist, its script body actually ran, and getPackages() /
-- getModules() actually list it.
--
-- Everything these specs install is uninstalled again in a finally() block, and
-- the last spec in the file asserts that nothing was left behind: the self-test
-- profile is reused between runs, so a leak here would break the next run.

local fixtureDirectory = debug.getinfo(1, "S").source:match("^@(.*)[/\\]") .. "/fixtures/packages"

-- Where the module fixtures are copied to before being installed - see
-- installFixtureModule() for why they cannot be installed from the repository.
local scratchDirectory = getMudletHomeDir() .. "/busted-package-fixtures"

local minimalPackage = "mudlet-spec-minimal"
local resourcesPackage = "mudlet-spec-resources"
local moduleName = "mudlet-spec-module"

local function contains(haystack, needle)
  return type(haystack) == "string" and haystack:find(needle, 1, true) ~= nil
end

-- Asserts that calling fn raises a Lua error whose message contains needle.
-- Matching a message substring (rather than merely "did it error?") ensures the
-- function is actually registered and reached its own argument validation: an
-- unregistered/nil function would raise a different "attempt to call" error.
local function assertArgError(fn, needle)
  local ok, err = pcall(fn)
  assert.is_false(ok)
  assert.is_true(contains(err, needle), tostring(err))
end

-- waitForEvent() spins the Qt event loop, so waiting for an event nothing ever
-- raises is how a spec gives Mudlet's queued work a chance to run: the install
-- events are raised from a zero-timer, and so is the profile save that
-- uninstallPackage() schedules.
local function pumpEventLoop(milliseconds)
  waitForEvent("mudletPackageSpecIdleEvent", milliseconds)
end

local function waitUntil(condition, timeoutMilliseconds)
  local waited = 0
  while waited < timeoutMilliseconds do
    if condition() then
      return true
    end
    pumpEventLoop(50)
    waited = waited + 50
  end
  return condition() and true or false
end

local function listContains(list, name)
  for _, entry in ipairs(list) do
    if entry == name then
      return true
    end
  end
  return false
end

local function packageInstalled(name)
  return listContains(getPackages(), name)
end

local function moduleInstalled(name)
  return listContains(getModules(), name)
end

local function fileExists(path)
  return lfs.attributes(path, "mode") ~= nil
end

local function copyFile(from, to)
  local source = io.open(from, "rb")
  assert.is_not_nil(source, "could not read the fixture " .. from)
  local contents = source:read("*a")
  source:close()
  local destination = io.open(to, "wb")
  assert.is_not_nil(destination, "could not write to " .. to)
  destination:write(contents)
  destination:close()
end

-- Every install and uninstall here starts an asynchronous profile save, and
-- while one is running the package API stops doing what it is told: an install
-- is postponed and answered with a bare true (see the pending spec at the end
-- of this file), an uninstall is refused, and a module reload is dropped. Lua
-- cannot ask whether a save is running, so each of the helpers below asks again
-- until what it wanted has actually happened.
local function installConfirmed(install, path, isInstalled, what)
  for attempt = 1, 5 do
    local ok, err = install(path)
    assert.is_true(ok == true, tostring(err))
    -- an install that is carried out is carried out there and then, so if it is
    -- not listed by the time the call returns it was postponed and dropped
    if isInstalled() then
      return
    end
    pumpEventLoop(150 * attempt)
  end
  assert.is_true(false, "could not install " .. what)
end

-- The same postponement answers a bad install path with true as well, so a spec
-- about the refusal waits for the save to pass and asks again.
local function refusedInstall(install, path)
  for attempt = 1, 5 do
    local ok, err = install(path)
    if ok == nil then
      return err
    end
    pumpEventLoop(150 * attempt)
  end
  assert.is_true(false, "the install was postponed instead of being answered")
end

-- reloadModule() is postponed the same way and then quietly dropped, so ask
-- until the reload is observable.
local function reloadModuleUntil(name, reloaded)
  for attempt = 1, 5 do
    reloadModule(name)
    if waitUntil(reloaded, 300) then
      return
    end
    pumpEventLoop(150 * attempt)
  end
  assert.is_true(false, "the module was never reloaded")
end

local function removeFixturePackage(name)
  if not packageInstalled(name) then
    return
  end
  -- uninstallPackage() refuses while a profile save is in progress, and the
  -- installs here start one, so keep asking until it takes.
  assert.is_true(waitUntil(function() return uninstallPackage(name) == true end, 5000),
                 "could not uninstall the fixture package " .. name)
  -- Let the profile save that uninstallPackage() queues run now, rather than
  -- during Mudlet's shutdown.
  pumpEventLoop(200)
end

local function installFixturePackage(name)
  installConfirmed(installPackage, fixtureDirectory .. "/" .. name .. ".mpackage",
                   function() return packageInstalled(name) end, "the fixture package " .. name)
end

local function withFixturePackage(name)
  finally(function() removeFixturePackage(name) end)
  installFixturePackage(name)
end

local function removeFixtureModule(name)
  if moduleInstalled(name) then
    assert.is_true(waitUntil(function() return uninstallModule(name) == true end, 5000),
                   "could not uninstall the fixture module " .. name)
    pumpEventLoop(200)
  end
  os.remove(scratchDirectory .. "/" .. name .. ".mpackage")
  lfs.rmdir(scratchDirectory)
end

-- A module is installed from a copy inside the profile, never from the
-- repository: with sync enabled a profile save rewrites the module's own
-- .mpackage in place, which would corrupt the committed fixture.
local function installFixtureModule(name)
  lfs.mkdir(scratchDirectory)
  local path = scratchDirectory .. "/" .. name .. ".mpackage"
  copyFile(fixtureDirectory .. "/" .. name .. ".mpackage", path)
  installConfirmed(installModule, path, function() return moduleInstalled(name) end, "the fixture module " .. name)
  return path
end

-- The clean-up is registered before the install so a fixture that only got
-- half-way in still leaves nothing behind.
local function withFixtureModule(name)
  finally(function() removeFixtureModule(name) end)
  return installFixtureModule(name)
end

-- Collects every occurrence of an event for the duration of one spec. The
-- uninstall events are raised inside uninstallPackage() itself, before a
-- waitForEvent() could be armed, so a pre-armed handler is what sees them.
local function collect(eventName, into)
  local handler = registerAnonymousEventHandler(eventName, function(_, ...)
    into[#into + 1] = {...}
  end)
  finally(function() killAnonymousEventHandler(handler) end)
end

describe("Tests the functionality of installPackage", function()
  it("raises a Lua error when called with no arguments", function()
    -- the Lua wrapper that lets installPackage() take a URL indexes its
    -- argument before the C++ side gets to report a "bad argument #1", so the
    -- call fails less clearly than its siblings do
    assert.has_error(function() installPackage() end)
  end)

  it("returns nil+msg when given an empty path", function()
    local err = refusedInstall(installPackage, "")
    assert.is_true(contains(err, "no package file was actually given"), tostring(err))
  end)

  it("returns nil+msg for a file that is not there", function()
    local err = refusedInstall(installPackage, fixtureDirectory .. "/mudlet-spec-there-is-no-such-package.mpackage")
    assert.is_true(contains(err, "could not open file"), tostring(err))
  end)

  it("returns nil+msg for a file that is not a zip archive", function()
    local err = refusedInstall(installPackage, fixtureDirectory .. "/mudlet-spec-notazip.mpackage")
    assert.is_true(contains(err, "could not unzip package"), tostring(err))
    assert.is_false(packageInstalled("mudlet-spec-notazip"))
    -- The failed unpacking still creates the destination folder; drop it so the
    -- profile is left exactly as it was found.
    lfs.rmdir(getMudletHomeDir() .. "/mudlet-spec-notazip")
  end)

  it("unpacks the package into the profile and runs its contents", function()
    local runsBefore = mudletSpecMinimalRuns or 0
    withFixturePackage(minimalPackage)

    local packageDirectory = getMudletHomeDir() .. "/" .. minimalPackage
    assert.is_true(fileExists(packageDirectory), "the package folder was not created")
    assert.is_true(fileExists(packageDirectory .. "/config.lua"))
    assert.is_true(fileExists(packageDirectory .. "/" .. minimalPackage .. ".xml"))
    assert.equals(1, exists(minimalPackage .. " alias", "alias"))
    assert.equals(1, exists("mudletSpecMinimalScript", "script"))
    assert.is_true(mudletSpecMinimalRuns > runsBefore, "the package's script did not run")
  end)

  it("refuses to install a package that is already installed", function()
    withFixturePackage(minimalPackage)

    local err = refusedInstall(installPackage, fixtureDirectory .. "/" .. minimalPackage .. ".mpackage")
    assert.is_true(contains(err, "package " .. minimalPackage .. " is already installed"), tostring(err))
  end)

  it("unpacks a folder of resources that ships with a package", function()
    withFixturePackage(resourcesPackage)

    local packageDirectory = getMudletHomeDir() .. "/" .. resourcesPackage
    assert.is_true(fileExists(packageDirectory .. "/resources/spec-note.txt"))
    assert.is_true(fileExists(packageDirectory .. "/resources/nested/spec-nested.txt"))
    local handle = io.open(packageDirectory .. "/resources/spec-note.txt", "rb")
    assert.is_not_nil(handle)
    local contents = handle:read("*a")
    handle:close()
    assert.is_true(contains(contents, "mudlet-spec-resources fixture resource"))
  end)

  it("names a package after its file when the archive has no config.lua", function()
    withFixturePackage("mudlet-spec-noconfig")

    assert.equals(1, exists("mudlet-spec-noconfig alias", "alias"))
    assert.same({}, getPackageInfo("mudlet-spec-noconfig"))
  end)

  it("installs a package from a plain XML file", function()
    local path = fixtureDirectory .. "/sources/mudlet-spec-xmlonly/mudlet-spec-xmlonly.xml"
    installConfirmed(installPackage, path, function() return packageInstalled("mudlet-spec-xmlonly") end,
                     "the XML fixture package")
    finally(function() removeFixturePackage("mudlet-spec-xmlonly") end)

    assert.equals(1, exists("mudlet-spec-xmlonly alias", "alias"))
    -- nothing is unpacked for a bare XML: the file stays where it is
    assert.is_false(fileExists(getMudletHomeDir() .. "/mudlet-spec-xmlonly"))
    assert.is_true(fileExists(path), "the package XML must not be moved out of the fixtures")
  end)

  it("raises sysInstall and sysInstallPackage once the install is complete", function()
    local generic, detailed = {}, {}
    collect("sysInstall", generic)
    collect("sysInstallPackage", detailed)

    withFixturePackage(minimalPackage)
    -- both events are raised from a zero-timer after the install returned
    assert.is_true(waitUntil(function() return #detailed > 0 end, 2000), "sysInstallPackage was never raised")

    assert.equals(1, #generic)
    assert.equals(minimalPackage, generic[1][1])
    assert.equals(1, #detailed)
    assert.equals(minimalPackage, detailed[1][1])
    assert.is_true(contains(detailed[1][2], minimalPackage .. ".mpackage"), tostring(detailed[1][2]))
  end)
end)

describe("Tests the functionality of uninstallPackage", function()
  it("raises a Lua error when called with no arguments", function()
    assertArgError(function() uninstallPackage() end, "uninstallPackage: bad argument #1 type")
  end)

  it("returns nil and no message for a package that is not installed", function()
    local ok, err = uninstallPackage("mudlet-spec-never-installed")
    assert.is_nil(ok)
    assert.is_nil(err)
  end)

  it("removes the package, its items and its folder", function()
    withFixturePackage(minimalPackage)
    local packageDirectory = getMudletHomeDir() .. "/" .. minimalPackage
    assert.is_true(fileExists(packageDirectory))

    removeFixturePackage(minimalPackage)

    assert.is_false(packageInstalled(minimalPackage))
    assert.equals(0, exists(minimalPackage .. " alias", "alias"))
    assert.equals(0, exists("mudletSpecMinimalScript", "script"))
    assert.is_false(fileExists(packageDirectory), "the package folder was left behind")
    assert.same({}, getPackageInfo(minimalPackage))
  end)

  it("raises sysUninstall and sysUninstallPackage", function()
    withFixturePackage(minimalPackage)
    local generic, detailed = {}, {}
    collect("sysUninstall", generic)
    collect("sysUninstallPackage", detailed)

    removeFixturePackage(minimalPackage)

    assert.equals(1, #generic)
    assert.equals(minimalPackage, generic[1][1])
    assert.equals(1, #detailed)
    assert.equals(minimalPackage, detailed[1][1])
  end)
end)

describe("Tests the functionality of getPackages", function()
  it("returns a table of the installed packages", function()
    local packages = getPackages()
    assert.is_table(packages)
    -- run-tests is the package running these specs, so it is always installed
    assert.is_true(listContains(packages, "run-tests"))
  end)

  it("lists a package while it is installed and not before or after", function()
    assert.is_false(packageInstalled(minimalPackage))
    withFixturePackage(minimalPackage)
    assert.is_true(packageInstalled(minimalPackage))
    removeFixturePackage(minimalPackage)
    assert.is_false(packageInstalled(minimalPackage))
  end)
end)

describe("Tests the functionality of getPackageInfo", function()
  it("raises a Lua error when the package name is not a string", function()
    assertArgError(function() getPackageInfo({}) end, "getPackageInfo: bad argument #1 type")
  end)

  it("raises a Lua error when the requested field is not a string", function()
    assertArgError(function() getPackageInfo(minimalPackage, {}) end, "getPackageInfo: bad argument #2 type")
  end)

  it("returns everything the package's config.lua declared", function()
    withFixturePackage(minimalPackage)

    assert.same({
      mpackage = minimalPackage,
      author = "Mudlet test suite",
      title = "Minimal fixture package for Package_spec.lua",
      version = "1.0",
      description = "One alias and one script, just enough to prove a package installed.",
    }, getPackageInfo(minimalPackage))
  end)

  it("returns a single field when one is named", function()
    withFixturePackage(resourcesPackage)

    assert.equals("2.5", getPackageInfo(resourcesPackage, "version"))
    assert.equals("Mudlet test suite", getPackageInfo(resourcesPackage, "author"))
  end)

  it("returns an empty string for a field the package does not have", function()
    withFixturePackage(minimalPackage)

    assert.equals("", getPackageInfo(minimalPackage, "no-such-field"))
  end)

  it("returns an empty table for a package that is not installed", function()
    assert.same({}, getPackageInfo("mudlet-spec-never-installed"))
  end)
end)

describe("Tests the functionality of setPackageInfo", function()
  it("raises a Lua error when called with no arguments", function()
    assertArgError(function() setPackageInfo() end, "setPackageInfo: bad argument #1 type")
  end)

  it("raises a Lua error when the value is missing", function()
    assertArgError(function() setPackageInfo(minimalPackage, "version") end, "setPackageInfo: bad argument #3 type")
  end)

  it("round-trips a value through getPackageInfo", function()
    withFixturePackage(minimalPackage)

    assert.is_true(setPackageInfo(minimalPackage, "version", "9.9"))
    assert.equals("9.9", getPackageInfo(minimalPackage, "version"))
    assert.equals("9.9", getPackageInfo(minimalPackage).version)
  end)

  it("adds a field the package did not declare", function()
    withFixturePackage(minimalPackage)

    assert.is_true(setPackageInfo(minimalPackage, "spec-added", "yes"))
    assert.equals("yes", getPackageInfo(minimalPackage, "spec-added"))
  end)
end)

describe("Tests the functionality of installModule", function()
  it("raises a Lua error when called with no arguments", function()
    assertArgError(function() installModule() end, "installModule: bad argument #1 type")
  end)

  it("returns nil+msg for a file that is not there", function()
    local err = refusedInstall(installModule, fixtureDirectory .. "/mudlet-spec-there-is-no-such-module.mpackage")
    assert.is_true(contains(err, "could not open file"), tostring(err))
  end)

  it("installs the module, unpacks it and runs its contents", function()
    local runsBefore = mudletSpecModuleRuns or 0
    withFixtureModule(moduleName)

    assert.is_true(moduleInstalled(moduleName))
    -- a module is not a package: it must not turn up in getPackages()
    assert.is_false(packageInstalled(moduleName))
    assert.is_true(fileExists(getMudletHomeDir() .. "/" .. moduleName))
    assert.equals(1, exists(moduleName .. " alias", "alias"))
    assert.is_true(mudletSpecModuleRuns > runsBefore, "the module's script did not run")
  end)

  it("refuses to install a module that is already installed", function()
    local path = withFixtureModule(moduleName)

    local err = refusedInstall(installModule, path)
    assert.is_true(contains(err, "module " .. moduleName .. " is already installed"), tostring(err))
  end)

  it("raises sysInstall and sysLuaInstallModule", function()
    local generic, detailed = {}, {}
    collect("sysInstall", generic)
    collect("sysLuaInstallModule", detailed)

    withFixtureModule(moduleName)
    assert.is_true(waitUntil(function() return #detailed > 0 end, 2000), "sysLuaInstallModule was never raised")

    assert.equals(1, #generic)
    assert.equals(moduleName, generic[1][1])
    assert.equals(1, #detailed)
    assert.equals(moduleName, detailed[1][1])
    assert.is_true(contains(detailed[1][2], moduleName .. ".mpackage"), tostring(detailed[1][2]))
  end)
end)

describe("Tests the functionality of uninstallModule", function()
  it("raises a Lua error when called with no arguments", function()
    assertArgError(function() uninstallModule() end, "uninstallModule: bad argument #1 type")
  end)

  it("returns false for a module that is not installed", function()
    assert.is_false(uninstallModule("mudlet-spec-never-installed"))
  end)

  it("removes the module, its items and its folder", function()
    withFixtureModule(moduleName)
    local moduleDirectory = getMudletHomeDir() .. "/" .. moduleName
    assert.is_true(fileExists(moduleDirectory))

    removeFixtureModule(moduleName)

    assert.is_false(moduleInstalled(moduleName))
    assert.equals(0, exists(moduleName .. " alias", "alias"))
    assert.is_false(fileExists(moduleDirectory), "the module folder was left behind")
    assert.same({}, getModuleInfo(moduleName))
  end)

  it("raises sysUninstall and sysLuaUninstallModule", function()
    withFixtureModule(moduleName)
    local generic, detailed = {}, {}
    collect("sysUninstall", generic)
    collect("sysLuaUninstallModule", detailed)

    removeFixtureModule(moduleName)

    assert.equals(1, #generic)
    assert.equals(moduleName, generic[1][1])
    assert.equals(1, #detailed)
    assert.equals(moduleName, detailed[1][1])
  end)
end)

describe("Tests the functionality of getModules", function()
  it("returns a table", function()
    assert.is_table(getModules())
  end)

  it("lists a module while it is installed and not before or after", function()
    assert.is_false(moduleInstalled(moduleName))
    withFixtureModule(moduleName)
    assert.is_true(moduleInstalled(moduleName))
    removeFixtureModule(moduleName)
    assert.is_false(moduleInstalled(moduleName))
  end)
end)

describe("Tests the functionality of getModuleInfo", function()
  it("raises a Lua error when the module name is not a string", function()
    assertArgError(function() getModuleInfo({}) end, "getModuleInfo: bad argument #1 type")
  end)

  it("returns everything the module's config.lua declared", function()
    withFixtureModule(moduleName)

    assert.same({
      mpackage = moduleName,
      author = "Mudlet test suite",
      title = "Module fixture for Package_spec.lua",
      version = "3.1",
      description = "Counts how often its script has been compiled, so a reload is observable.",
    }, getModuleInfo(moduleName))
  end)

  it("returns a single field when one is named", function()
    withFixtureModule(moduleName)

    assert.equals("3.1", getModuleInfo(moduleName, "version"))
    assert.equals("", getModuleInfo(moduleName, "no-such-field"))
  end)

  it("returns an empty table for a module that is not installed", function()
    assert.same({}, getModuleInfo("mudlet-spec-never-installed"))
  end)
end)

describe("Tests the functionality of setModuleInfo", function()
  it("raises a Lua error when the value is missing", function()
    assertArgError(function() setModuleInfo(moduleName, "version") end, "setModuleInfo: bad argument #3 type")
  end)

  it("round-trips a value through getModuleInfo", function()
    withFixtureModule(moduleName)

    assert.is_true(setModuleInfo(moduleName, "version", "8.8"))
    assert.equals("8.8", getModuleInfo(moduleName, "version"))
    assert.equals("8.8", getModuleInfo(moduleName).version)
  end)
end)

describe("Tests the functionality of getModulePath", function()
  it("returns the file the module was installed from", function()
    local path = withFixtureModule(moduleName)

    assert.equals(path, getModulePath(moduleName))
  end)
end)

describe("Tests the functionality of setModulePriority", function()
  it("raises a Lua error when the priority is missing", function()
    assertArgError(function() setModulePriority(moduleName) end, "setModulePriority: bad argument #2 type")
  end)

  it("returns nil+msg for a module that is not installed", function()
    local ok, err = setModulePriority("mudlet-spec-never-installed", 3)
    assert.is_nil(ok)
    assert.is_true(contains(err, "module doesn't exist"), tostring(err))
  end)

  it("returns no values and is read back by getModulePriority", function()
    withFixtureModule(moduleName)

    assert.equals(0, select('#', setModulePriority(moduleName, 7)))
    assert.equals(7, getModulePriority(moduleName))
    setModulePriority(moduleName, -2)
    assert.equals(-2, getModulePriority(moduleName))
  end)
end)

describe("Tests the functionality of getModulePriority", function()
  it("reports the default priority of a freshly installed module", function()
    -- BUG: a module nobody has called setModulePriority() on has no entry in
    -- the priority map, so getModulePriority() answers nil and "module doesn't
    -- exist" for a module that plainly does exist. The module manager shows
    -- such a module with priority 0, which is what this should return. Left
    -- pending rather than pinning the wrong answer as the contract.
    pending("getModulePriority() reports an installed module as non-existent until a priority is set")
    withFixtureModule(moduleName)

    assert.equals(0, getModulePriority(moduleName))
  end)
end)

describe("Tests the functionality of enableModuleSync", function()
  it("raises a Lua error when called with no arguments", function()
    assertArgError(function() enableModuleSync() end, "enableModuleSync: bad argument #1 type")
  end)

  it("returns nil+msg for an empty module name", function()
    local ok, err = enableModuleSync("")
    assert.is_nil(ok)
    assert.is_true(contains(err, "module name cannot be an empty string"), tostring(err))
  end)

  it("returns nil+msg for a module that is not installed", function()
    local ok, err = enableModuleSync("mudlet-spec-never-installed")
    assert.is_nil(ok)
    assert.is_true(contains(err, "not found"), tostring(err))
  end)

  it("turns syncing on for an installed module", function()
    withFixtureModule(moduleName)
    assert.is_false(getModuleSync(moduleName))

    assert.is_true(enableModuleSync(moduleName))
    assert.is_true(getModuleSync(moduleName))
    -- leave the module unsynced: a profile save rewrites a synced module's own
    -- .mpackage, and the fixture copy is thrown away right after this
    assert.is_true(disableModuleSync(moduleName))
  end)
end)

describe("Tests the functionality of disableModuleSync", function()
  it("returns nil+msg for a module that is not installed", function()
    local ok, err = disableModuleSync("mudlet-spec-never-installed")
    assert.is_nil(ok)
    assert.is_true(contains(err, "not found"), tostring(err))
  end)

  it("turns syncing back off", function()
    withFixtureModule(moduleName)
    assert.is_true(enableModuleSync(moduleName))

    assert.is_true(disableModuleSync(moduleName))
    assert.is_false(getModuleSync(moduleName))
  end)
end)

describe("Tests the functionality of getModuleSync", function()
  it("raises a Lua error when called with no arguments", function()
    assertArgError(function() getModuleSync() end, "getModuleSync: bad argument #1 type")
  end)

  it("returns nil+msg for a module that is not installed", function()
    local ok, err = getModuleSync("mudlet-spec-never-installed")
    assert.is_nil(ok)
    assert.is_true(contains(err, "not found"), tostring(err))
  end)

  it("is false for a module that was just installed", function()
    withFixtureModule(moduleName)

    assert.is_false(getModuleSync(moduleName))
  end)
end)

describe("Tests the functionality of reloadModule", function()
  it("raises a Lua error when called with no arguments", function()
    assertArgError(function() reloadModule() end, "reloadModule: bad argument #1 type")
  end)

  it("returns no values and does nothing for a module that is not installed", function()
    assert.equals(0, select('#', reloadModule("mudlet-spec-never-installed")))
    assert.is_false(moduleInstalled("mudlet-spec-never-installed"))
  end)

  it("runs the module's scripts again", function()
    withFixtureModule(moduleName)
    local runsBefore = mudletSpecModuleRuns

    reloadModuleUntil(moduleName, function() return mudletSpecModuleRuns > runsBefore end)

    assert.is_true(moduleInstalled(moduleName))
    assert.equals(1, exists(moduleName .. " alias", "alias"))
  end)

  it("re-reads the module's info from its config.lua", function()
    withFixtureModule(moduleName)
    setModuleInfo(moduleName, "title", "changed by the spec")
    assert.equals("changed by the spec", getModuleInfo(moduleName, "title"))

    reloadModuleUntil(moduleName, function()
      return getModuleInfo(moduleName, "title") == "Module fixture for Package_spec.lua"
    end)
  end)

  it("keeps the module's priority", function()
    withFixtureModule(moduleName)
    setModulePriority(moduleName, 4)
    local runsBefore = mudletSpecModuleRuns

    reloadModuleUntil(moduleName, function() return mudletSpecModuleRuns > runsBefore end)

    assert.equals(4, getModulePriority(moduleName))
  end)
end)

describe("Tests a package that uninstalls itself", function()
  -- Regression #9557: a package whose event handler uninstalls its own package
  -- used to free the TScript objects that Host::raiseEvent() was still
  -- iterating over. Package auto-updaters do exactly this.
  it("survives a package uninstalling itself from its own event handler", function()
    finally(function()
      removeFixturePackage("mudlet-spec-selfuninstall")
      mudletSpecSelfUninstallHandler = nil
      mudletSpecSelfUninstallSecondHandler = nil
      mudletSpecSelfUninstallRan = nil
      mudletSpecSelfUninstallSecondRan = nil
    end)
    installFixturePackage("mudlet-spec-selfuninstall")
    assert.equals(1, exists("mudletSpecSelfUninstallHandler", "script"))

    -- the handler's uninstallPackage() declines while the save the install
    -- started is still running, so raise until the package is really gone
    assert.is_true(waitUntil(function()
      mudletSpecSelfUninstallRan = nil
      mudletSpecSelfUninstallSecondRan = nil
      raiseEvent("mudletSpecSelfUninstall")
      return not packageInstalled("mudlet-spec-selfuninstall")
    end, 5000), "the package did not uninstall itself")

    assert.is_true(mudletSpecSelfUninstallRan, "the package's own handler did not run")
    -- the package's second handler for this event is the one the pre-fix code
    -- would have called through a freed script; whether it is reached at all
    -- depends on where in the dispatch the uninstall landed, so what is checked
    -- here is that both scripts are gone afterwards and nothing crashed
    assert.equals(0, exists("mudletSpecSelfUninstallHandler", "script"))
    assert.equals(0, exists("mudletSpecSelfUninstallSecondHandler", "script"))
    -- raising the event again must not reach the removed scripts
    raiseEvent("mudletSpecSelfUninstall")
    pumpEventLoop(100)
    assert.is_false(packageInstalled("mudlet-spec-selfuninstall"))
  end)
end)

describe("Tests installing a package while the profile is being saved", function()
  it("installs a package that is asked for while an earlier install is still saving", function()
    -- BUG: installing a package starts an asynchronous profile save, and an
    -- install that arrives during one is postponed until profileSaveFinished().
    -- That signal is only emitted while the profile writer is being retired, so
    -- an install asked for after the writers are gone but before the save has
    -- finished is never carried out - and installPackage() has already answered
    -- true, so a script has no way to notice. Left pending rather than pinning
    -- a silently dropped install as correct.
    pending("installPackage() answers true but drops the install when a save is in progress")
    finally(function()
      removeFixturePackage(minimalPackage)
      removeFixturePackage("mudlet-spec-noconfig")
    end)
    installFixturePackage(minimalPackage)

    assert.is_true(installPackage(fixtureDirectory .. "/mudlet-spec-noconfig.mpackage"))
    assert.is_true(waitUntil(function() return packageInstalled("mudlet-spec-noconfig") end, 5000))
  end)
end)

describe("Tests installing an archive with nothing in it for Mudlet", function()
  it("refuses an archive that holds neither a config.lua nor a package XML", function()
    -- BUG: such an archive is unpacked into the profile and answered with true,
    -- but nothing is registered: it is missing from getPackages(), so
    -- uninstallPackage() will not take it and the unpacked folder stays in the
    -- profile for good. Left pending rather than pinning a success that
    -- installs nothing and cannot be undone.
    pending("installPackage() answers true for an archive with no package in it, and leaves it unremovable")
    finally(function() removeFixturePackage("mudlet-spec-emptyarchive") end)

    local err = refusedInstall(installPackage, fixtureDirectory .. "/mudlet-spec-emptyarchive.mpackage")
    assert.is_string(err)
    assert.is_false(fileExists(getMudletHomeDir() .. "/mudlet-spec-emptyarchive"))
  end)
end)

describe("The package specs clean up after themselves", function()
  it("leaves no fixture package, module or folder behind", function()
    for _, name in ipairs(getPackages()) do
      assert.is_false(name:find("mudlet%-spec%-") ~= nil, "left the package " .. name .. " installed")
    end
    for _, name in ipairs(getModules()) do
      assert.is_false(name:find("mudlet%-spec%-") ~= nil, "left the module " .. name .. " installed")
    end
    for entry in lfs.dir(getMudletHomeDir()) do
      assert.is_false(entry:find("mudlet%-spec%-") ~= nil, "left the folder " .. entry .. " behind")
    end
    assert.is_false(fileExists(scratchDirectory), "left the fixture scratch folder behind")

    -- A synced module is copied into the shared module backup folder by a
    -- profile save; drop any copy of the fixture module that made it there.
    local configurationDirectory = getMudletHomeDir():match("^(.*)[/\\]profiles[/\\]")
    local backups = configurationDirectory and (configurationDirectory .. "/moduleBackups")
    if backups and fileExists(backups) then
      for entry in lfs.dir(backups) do
        if entry:find("mudlet%-spec%-") then
          os.remove(backups .. "/" .. entry)
        end
      end
    end

    -- Let the profile save that the last uninstall queued run while the profile
    -- is still up: it dereferences the profile when it fires, and Mudlet may be
    -- shutting down by the time it would otherwise get its turn.
    pumpEventLoop(1500)
  end)
end)
