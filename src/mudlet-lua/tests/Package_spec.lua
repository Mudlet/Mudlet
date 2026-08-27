-- Specs for the package and module lifecycle Lua APIs.
--
-- Every spec here drives the real API against the fixture kit committed in
-- fixtures/packages/ - nothing is mocked. Alongside each contract (what a call
-- returns, and what it returns when it is misused) the effect is checked too:
-- the package's files actually land under getMudletHomeDir(), its aliases and
-- scripts actually exist, its script body actually ran, and getPackages() /
-- getModules() actually list it.
--
-- Installing or uninstalling anything costs a full profile save, so the specs
-- that only read share one installed fixture through setup()/teardown() rather
-- than each installing their own - a file that saved the profile fifty times
-- took longer than the whole rest of the suite.
--
-- Everything these specs install is uninstalled again when the spec (or its
-- block) ends, and the last spec in the file asserts that nothing was left
-- behind: the self-test profile is reused between runs, so a leak here would
-- break the next run.

-- pumpEvents() is inert outside test mode, and without it the profile save
-- never finishes: the uninstalls fail and strand fixture packages in the
-- profile, so say so rather than make that mess.
if not os.getenv("MUDLET_TEST_MODE") then
  describe("Tests the package and module lifecycle", function()
    it("needs test mode", function()
      pending("the package specs need MUDLET_TEST_MODE (pumpEvents() does nothing without it)")
    end)
  end)
  return
end

local specDirectory = debug.getinfo(1, "S").source:match("^@(.*)[/\\]")
assert(specDirectory, "Package_spec.lua has to be run from a file so that it can find its fixtures")
local fixtureDirectory = specDirectory .. "/fixtures/packages"

-- Where the module fixtures are copied to before being installed - see
-- installFixtureModule() for why they cannot be installed from the repository.
local scratchDirectory = getMudletHomeDir() .. "/busted-package-fixtures"

local minimalPackage = "mudlet-spec-minimal"
local resourcesPackage = "mudlet-spec-resources"
local moduleName = "mudlet-spec-module"

-- busted keeps only the last function handed to finally(), so everything that
-- has to be undone at the end of a spec goes through one registration here -
-- otherwise a spec that cleans up both a fixture and an event handler silently
-- loses one of them.
local cleanups
local function defer(cleanup)
  if not cleanups then
    cleanups = {}
    finally(function()
      local queued = cleanups
      cleanups = nil
      -- one clean-up giving up (an uninstall that never took, say) must not
      -- strand the rest, so run them all and report the first failure after
      local firstFailure
      for index = #queued, 1, -1 do
        local ok, err = pcall(queued[index])
        if not ok and not firstFailure then
          firstFailure = err
        end
      end
      if firstFailure then
        error(firstFailure, 0)
      end
    end)
  end
  cleanups[#cleanups + 1] = cleanup
end

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

-- The install events, and the profile save uninstallPackage() schedules, are
-- all raised from a zero-timer, so none of them happen unless a spec pumps.
local function waitUntil(condition, timeoutMilliseconds)
  local waited = 0
  while waited < timeoutMilliseconds do
    if condition() then
      return true
    end
    pumpEvents(50)
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

-- Everything the main console gained since it was at line `mark`, joined up.
-- The console wraps long lines and a wrap swallows the space it broke at, so
-- the announcements below are matched with all whitespace removed.
local function textFrom(mark)
  return table.concat(getLines("main", mark, getLastLineNumber("main") + 1), "")
end

local function containsWrapped(haystack, needle)
  return contains((tostring(haystack):gsub("%s+", "")), (needle:gsub("%s+", "")))
end

-- A file: URL for a local path, in the three-slash form that keeps a Windows
-- drive letter from being read as the host name. The checkout these fixtures
-- live in can sit anywhere, so the characters that would otherwise end the path
-- early - a space, a fragment, a query, a half-written escape - are encoded.
local function fileUrl(path)
  local normalised = path:gsub("\\", "/"):gsub("[%%#%?%s]", function(character) return string.format("%%%02X", character:byte()) end)
  if normalised:sub(1, 1) ~= "/" then
    normalised = "/" .. normalised
  end
  return "file://" .. normalised
end

local function copyFile(from, to)
  local source = io.open(from, "rb")
  assert.is_not_nil(source, "could not read the fixture " .. from)
  local contents = source:read("*a")
  source:close()
  local destination = io.open(to, "wb")
  assert.is_not_nil(destination, "could not write to " .. to)
  assert.is_not_nil(destination:write(contents), "could not write to " .. to)
  destination:close()
end

-- Every install and uninstall here starts an asynchronous profile save, and
-- while one is running the package API stops doing what it is told: an install
-- is postponed and answered with a bare true (see the pending spec at the end
-- of this file), an uninstall is refused, and a module reload is dropped. Lua
-- cannot ask whether a save is running, so each of the helpers below asks again
-- until what it wanted has actually happened. They wait longer between tries
-- than they need to on a fast machine on purpose: each postponed call queues
-- another attempt for whenever the save does finish, and a pile of those all
-- arriving at once starts a pile of saves.
-- Lua has no direct way to ask whether a profile save is running, but
-- installPackage() gives it away: while one is in progress it postpones
-- whatever it was asked to do and answers true, even for an empty path it would
-- otherwise refuse outright. Waiting for the refusal to come back is what keeps
-- the installs below from being postponed - a postponed install is carried out
-- later, and can put a package back after a spec has taken it away again.
local function waitForProfileSaveToPass()
  return waitUntil(function() return installPackage("") == nil end, 5000)
end

local function installUntilConfirmed(install, path, isInstalled, what)
  for attempt = 1, 3 do
    if isInstalled() then
      return
    end
    assert.is_true(waitForProfileSaveToPass(), "a profile save was still running after 5s, so this install would be postponed")
    local ok, err = install(path)
    -- a postponed install can still be carried out while the pump below runs
    -- the event loop, so a repeat may legitimately come back "already installed"
    if ok ~= true and not contains(err, "already installed") then
      assert.is_true(false, tostring(err))
    end
    -- an install that is carried out is carried out there and then, so if it is
    -- not listed by the time the call returns it was postponed
    if isInstalled() then
      return
    end
    pumpEvents(400 * attempt)
  end
  assert.is_true(false, "could not install " .. what)
end

-- The same postponement answers a bad install path with true as well, so a spec
-- about the refusal waits for the save to pass and asks again.
local function installUntilRefused(install, path)
  for attempt = 1, 3 do
    assert.is_true(waitForProfileSaveToPass(), "a profile save was still running after 5s, so this install would be postponed")
    local ok, err = install(path)
    if ok == nil then
      return err
    end
    pumpEvents(400 * attempt)
  end
  assert.is_true(false, "the install was postponed instead of being answered")
end

-- reloadModule() is postponed the same way and then quietly dropped, so ask
-- until the reload is observable.
local function reloadModuleUntil(name, reloaded)
  for attempt = 1, 3 do
    reloadModule(name)
    if waitUntil(reloaded, 300) then
      return
    end
    pumpEvents(400 * attempt)
  end
  assert.is_true(false, "the module was never reloaded")
end

-- Uninstalls and then waits, twice over if it has to: an install this file
-- postponed earlier can be carried out while the wait runs the event loop, and
-- would otherwise reinstall the package behind the spec's back.
local function removeFixturePackage(name)
  for _ = 1, 3 do
    if not packageInstalled(name) then
      return
    end
    assert.is_true(waitForProfileSaveToPass(), "a profile save was still running after 5s, so this uninstall would be refused")
    -- uninstallPackage() refuses while a profile save is in progress, and the
    -- installs here start one, so keep asking until it takes
    assert.is_true(waitUntil(function() return uninstallPackage(name) == true end, 5000),
                   "could not uninstall the fixture package " .. name)
    -- let the profile save that uninstallPackage() queues run now, rather than
    -- during Mudlet's shutdown
    pumpEvents(200)
  end
  assert.is_false(packageInstalled(name), "the fixture package " .. name .. " reinstalled itself")
end

local function installFixturePackage(name)
  installUntilConfirmed(installPackage, fixtureDirectory .. "/" .. name .. ".mpackage",
                        function() return packageInstalled(name) end, "the fixture package " .. name)
end

local function withFixturePackage(name)
  defer(function() removeFixturePackage(name) end)
  installFixturePackage(name)
end

local function removeFixtureModule(name)
  for _ = 1, 3 do
    if not moduleInstalled(name) then
      break
    end
    assert.is_true(waitForProfileSaveToPass(), "a profile save was still running after 5s, so this uninstall would be refused")
    assert.is_true(waitUntil(function() return uninstallModule(name) == true end, 5000),
                   "could not uninstall the fixture module " .. name)
    pumpEvents(200)
  end
  assert.is_false(moduleInstalled(name), "the fixture module " .. name .. " reinstalled itself")
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
  installUntilConfirmed(installModule, path, function() return moduleInstalled(name) end, "the fixture module " .. name)
  return path
end

-- The clean-up is registered before the install so a fixture that only got
-- half-way in still leaves nothing behind.
local function withFixtureModule(name)
  defer(function() removeFixtureModule(name) end)
  return installFixtureModule(name)
end

-- Collects every occurrence of an event until stopCollecting() is called. The
-- uninstall events are raised inside uninstallPackage() itself, before a
-- waitForEvent() could be armed, so a pre-armed handler is what sees them.
-- Returns the list the events land in and the handler id to kill.
local function collectEvents(eventName)
  local events = {}
  local handler = registerAnonymousEventHandler(eventName, function(_, ...)
    events[#events + 1] = {...}
  end)
  return events, handler
end

-- The same, for a spec that can register its own clean-up.
local function collectEventsForSpec(eventName)
  local events, handler = collectEvents(eventName)
  defer(function() killAnonymousEventHandler(handler) end)
  return events
end

describe("Tests the functionality of installPackage", function()
  it("raises a Lua error when called with no arguments", function()
    -- the Lua wrapper that lets installPackage() take a URL indexes its
    -- argument before the C++ side gets to report a "bad argument #1", so the
    -- call fails less clearly than its siblings do
    assert.has_error(function() installPackage() end)
  end)

  it("returns nil+msg when given an empty path", function()
    local err = installUntilRefused(installPackage, "")
    assert.is_true(contains(err, "no package file was actually given"), tostring(err))
  end)

  it("returns nil+msg for a file that is not there", function()
    local err = installUntilRefused(installPackage, fixtureDirectory .. "/mudlet-spec-there-is-no-such-package.mpackage")
    assert.is_true(contains(err, "could not open file"), tostring(err))
  end)

  it("returns nil+msg for a file that is not a zip archive", function()
    -- the failed unpacking still creates the destination folder; drop it so the
    -- profile is left exactly as it was found
    defer(function() lfs.rmdir(getMudletHomeDir() .. "/mudlet-spec-notazip") end)

    local err = installUntilRefused(installPackage, fixtureDirectory .. "/mudlet-spec-notazip.mpackage")
    assert.is_true(contains(err, "could not unzip package"), tostring(err))
    assert.is_false(packageInstalled("mudlet-spec-notazip"))
  end)

  describe("with the fixture package installed", function()
    local runsBefore, installEvents, packageEvents, handlers

    setup(function()
      runsBefore = mudletSpecMinimalRuns or 0
      local genericHandler, detailedHandler
      installEvents, genericHandler = collectEvents("sysInstall")
      packageEvents, detailedHandler = collectEvents("sysInstallPackage")
      handlers = {genericHandler, detailedHandler}
      installFixturePackage(minimalPackage)
      -- the install events are raised from a zero-timer once the install is done
      waitUntil(function() return #packageEvents > 0 end, 2000)
    end)

    teardown(function()
      for _, handler in ipairs(handlers) do
        killAnonymousEventHandler(handler)
      end
      removeFixturePackage(minimalPackage)
    end)

    it("unpacks the package into the profile and runs its contents", function()
      local packageDirectory = getMudletHomeDir() .. "/" .. minimalPackage
      assert.is_true(fileExists(packageDirectory), "the package folder was not created")
      assert.is_true(fileExists(packageDirectory .. "/config.lua"))
      assert.is_true(fileExists(packageDirectory .. "/" .. minimalPackage .. ".xml"))
      assert.equals(1, exists(minimalPackage .. " alias", "alias"))
      assert.equals(1, exists("mudletSpecMinimalScript", "script"))
      assert.is_true(mudletSpecMinimalRuns > runsBefore, "the package's script did not run")
    end)

    it("raises sysInstall and sysInstallPackage once the install is complete", function()
      assert.equals(1, #installEvents)
      assert.equals(minimalPackage, installEvents[1][1])
      assert.equals(1, #packageEvents)
      assert.equals(minimalPackage, packageEvents[1][1])
      assert.is_true(contains(packageEvents[1][2], minimalPackage .. ".mpackage"), tostring(packageEvents[1][2]))
    end)

    it("refuses to install a package that is already installed", function()
      local err = installUntilRefused(installPackage, fixtureDirectory .. "/" .. minimalPackage .. ".mpackage")
      assert.is_true(contains(err, "package " .. minimalPackage .. " is already installed"), tostring(err))
    end)
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
    -- the resources package declares its own version, unlike the minimal one
    assert.equals("2.5", getPackageInfo(resourcesPackage, "version"))
  end)

  it("names a package after its file when the archive has no config.lua", function()
    withFixturePackage("mudlet-spec-noconfig")

    assert.equals(1, exists("mudlet-spec-noconfig alias", "alias"))
    assert.same({}, getPackageInfo("mudlet-spec-noconfig"))
  end)

  it("installs a package from a plain XML file", function()
    local path = fixtureDirectory .. "/sources/mudlet-spec-xmlonly/mudlet-spec-xmlonly.xml"
    defer(function() removeFixturePackage("mudlet-spec-xmlonly") end)
    installUntilConfirmed(installPackage, path, function() return packageInstalled("mudlet-spec-xmlonly") end,
                          "the XML fixture package")

    assert.equals(1, exists("mudlet-spec-xmlonly alias", "alias"))
    -- nothing is unpacked for a bare XML: the file stays where it is
    assert.is_false(fileExists(getMudletHomeDir() .. "/mudlet-spec-xmlonly"))
    assert.is_true(fileExists(path), "the package XML must not be moved out of the fixtures")
  end)
end)

describe("Tests the functionality of uninstallPackage", function()
  it("raises a Lua error when called with no arguments", function()
    assertArgError(function() uninstallPackage() end, "uninstallPackage: bad argument #1 type")
  end)

  -- uninstallPackage() answers nil with no message where uninstallModule()
  -- answers false: two conventions for the same case, pinned as they are
  -- because packages published today read one or the other.
  it("returns nil and no message for a package that is not installed", function()
    local ok, err = uninstallPackage("mudlet-spec-never-installed")
    assert.is_nil(ok)
    assert.is_nil(err)
  end)

  it("removes the package, its items and its folder, and raises the uninstall events", function()
    withFixturePackage(minimalPackage)
    local packageDirectory = getMudletHomeDir() .. "/" .. minimalPackage
    assert.is_true(fileExists(packageDirectory))
    assert.is_true(packageInstalled(minimalPackage))
    local generic = collectEventsForSpec("sysUninstall")
    local detailed = collectEventsForSpec("sysUninstallPackage")

    removeFixturePackage(minimalPackage)

    assert.is_false(packageInstalled(minimalPackage))
    assert.equals(0, exists(minimalPackage .. " alias", "alias"))
    assert.equals(0, exists("mudletSpecMinimalScript", "script"))
    assert.is_false(fileExists(packageDirectory), "the package folder was left behind")
    assert.same({}, getPackageInfo(minimalPackage))
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
    assert.is_false(listContains(packages, "mudlet-spec-never-installed"))
  end)

  -- Mudlet skips mpkg for a profile built under MUDLET_TEST_MODE, because mpkg
  -- downloads the package listing on load and, whenever the repository is ahead of
  -- the bundled copy, removes and reinstalls itself - which clears the editor's tree
  -- widgets and writes to the main console partway through whatever is running. The
  -- C++ side proves the package is left off the install list; this proves a profile
  -- really does come up without it.
  it("has no mpkg in a test profile, which would upgrade itself mid-run", function()
    assert.is_false(packageInstalled("mpkg"),
      "mpkg is installed in this profile. A profile created before this check existed keeps it - " ..
      "remove it, or let a fresh self-test profile be made.")
  end)
end)

describe("Tests the package info accessors", function()
  setup(function()
    installFixturePackage(minimalPackage)
  end)
  teardown(function()
    removeFixturePackage(minimalPackage)
  end)

  describe("Tests the functionality of getPackageInfo", function()
    it("raises a Lua error when the package name is not a string", function()
      assertArgError(function() getPackageInfo({}) end, "getPackageInfo: bad argument #1 type")
    end)

    it("raises a Lua error when the requested field is not a string", function()
      assertArgError(function() getPackageInfo(minimalPackage, {}) end, "getPackageInfo: bad argument #2 type")
    end)

    it("returns everything the package's config.lua declared", function()
      assert.same({
        mpackage = minimalPackage,
        author = "Mudlet test suite",
        title = "Minimal fixture package for Package_spec.lua",
        version = "1.0",
        description = "One alias and one script, just enough to prove a package installed.",
      }, getPackageInfo(minimalPackage))
    end)

    it("returns a single field when one is named", function()
      assert.equals("1.0", getPackageInfo(minimalPackage, "version"))
      assert.equals("Mudlet test suite", getPackageInfo(minimalPackage, "author"))
    end)

    it("returns an empty string for a field the package does not have", function()
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
      defer(function() setPackageInfo(minimalPackage, "version", "1.0") end)

      assert.is_true(setPackageInfo(minimalPackage, "version", "9.9"))
      assert.equals("9.9", getPackageInfo(minimalPackage, "version"))
      assert.equals("9.9", getPackageInfo(minimalPackage).version)
    end)

    it("adds a field the package did not declare", function()
      assert.is_true(setPackageInfo(minimalPackage, "spec-added", "yes"))
      assert.equals("yes", getPackageInfo(minimalPackage, "spec-added"))
    end)
  end)
end)

describe("Tests the functionality of installModule", function()
  it("raises a Lua error when called with no arguments", function()
    assertArgError(function() installModule() end, "installModule: bad argument #1 type")
  end)

  it("returns nil+msg for a file that is not there", function()
    local err = installUntilRefused(installModule, fixtureDirectory .. "/mudlet-spec-there-is-no-such-module.mpackage")
    assert.is_true(contains(err, "could not open file"), tostring(err))
  end)

  describe("with the fixture module installed", function()
    local runsBefore, installEvents, moduleEvents, handlers, modulePath

    setup(function()
      runsBefore = mudletSpecModuleRuns or 0
      local genericHandler, detailedHandler
      installEvents, genericHandler = collectEvents("sysInstall")
      moduleEvents, detailedHandler = collectEvents("sysLuaInstallModule")
      handlers = {genericHandler, detailedHandler}
      modulePath = installFixtureModule(moduleName)
      waitUntil(function() return #moduleEvents > 0 end, 2000)
    end)

    teardown(function()
      for _, handler in ipairs(handlers) do
        killAnonymousEventHandler(handler)
      end
      removeFixtureModule(moduleName)
    end)

    it("installs the module, unpacks it and runs its contents", function()
      assert.is_true(moduleInstalled(moduleName))
      -- a module is not a package: it must not turn up in getPackages()
      assert.is_false(packageInstalled(moduleName))
      assert.is_true(fileExists(getMudletHomeDir() .. "/" .. moduleName))
      assert.equals(1, exists(moduleName .. " alias", "alias"))
      assert.is_true(mudletSpecModuleRuns > runsBefore, "the module's script did not run")
    end)

    it("raises sysInstall and sysLuaInstallModule", function()
      assert.equals(1, #installEvents)
      assert.equals(moduleName, installEvents[1][1])
      assert.equals(1, #moduleEvents)
      assert.equals(moduleName, moduleEvents[1][1])
      assert.is_true(contains(moduleEvents[1][2], moduleName .. ".mpackage"), tostring(moduleEvents[1][2]))
    end)

    it("refuses to install a module that is already installed", function()
      local err = installUntilRefused(installModule, modulePath)
      assert.is_true(contains(err, "module " .. moduleName .. " is already installed"), tostring(err))
    end)
  end)
end)

describe("Tests the functionality of uninstallModule", function()
  it("raises a Lua error when called with no arguments", function()
    assertArgError(function() uninstallModule() end, "uninstallModule: bad argument #1 type")
  end)

  it("returns false for a module that is not installed", function()
    assert.is_false(uninstallModule("mudlet-spec-never-installed"))
  end)

  it("removes the module, its items and its folder, and raises the uninstall events", function()
    withFixtureModule(moduleName)
    local moduleDirectory = getMudletHomeDir() .. "/" .. moduleName
    assert.is_true(fileExists(moduleDirectory))
    local generic = collectEventsForSpec("sysUninstall")
    local detailed = collectEventsForSpec("sysLuaUninstallModule")

    removeFixtureModule(moduleName)

    assert.is_false(moduleInstalled(moduleName))
    assert.equals(0, exists(moduleName .. " alias", "alias"))
    assert.is_false(fileExists(moduleDirectory), "the module folder was left behind")
    assert.same({}, getModuleInfo(moduleName))
    assert.equals(1, #generic)
    assert.equals(moduleName, generic[1][1])
    assert.equals(1, #detailed)
    assert.equals(moduleName, detailed[1][1])
  end)
end)

describe("Tests the functionality of getModules", function()
  it("returns a table of the installed modules", function()
    local modules = getModules()
    assert.is_table(modules)
    assert.is_false(listContains(modules, "mudlet-spec-never-installed"))
  end)
end)

describe("Tests the module accessors", function()
  local modulePath

  setup(function()
    modulePath = installFixtureModule(moduleName)
  end)
  teardown(function()
    removeFixtureModule(moduleName)
  end)

  describe("Tests the functionality of getModuleInfo", function()
    it("raises a Lua error when the module name is not a string", function()
      assertArgError(function() getModuleInfo({}) end, "getModuleInfo: bad argument #1 type")
    end)

    it("returns everything the module's config.lua declared", function()
      assert.same({
        mpackage = moduleName,
        author = "Mudlet test suite",
        title = "Module fixture for Package_spec.lua",
        version = "3.1",
        description = "Counts how often its script has been compiled, so a reload is observable.",
      }, getModuleInfo(moduleName))
    end)

    it("returns a single field when one is named", function()
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
      defer(function() setModuleInfo(moduleName, "version", "3.1") end)

      assert.is_true(setModuleInfo(moduleName, "version", "8.8"))
      assert.equals("8.8", getModuleInfo(moduleName, "version"))
      assert.equals("8.8", getModuleInfo(moduleName).version)
    end)
  end)

  describe("Tests the functionality of getModulePath", function()
    it("returns the file the module was installed from", function()
      assert.equals(modulePath, getModulePath(moduleName))
    end)
  end)

  describe("Tests the functionality of getModulePriority", function()
    -- Runs before setModulePriority's specs on purpose: a priority outlives the
    -- module it was set on (Host::uninstallPackage() leaves mModulePriorities
    -- alone), so once one has been set for this module the default this spec is
    -- about can never be observed again.
    it("reports the default priority of a freshly installed module", function()
      -- Installing a module seeds no priority for it, so this is the default the
      -- module manager displays and the profile exporter writes out, rather than
      -- the "module doesn't exist" that reading the priority map as an existence
      -- check used to answer here.
      assert.equals(0, getModulePriority(moduleName))
    end)

    it("returns nil+msg for a module that is not installed", function()
      local ok, err = getModulePriority("mudlet-spec-never-installed")
      assert.is_nil(ok)
      assert.is_true(contains(err, "module doesn't exist"), tostring(err))
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
      assert.equals(0, select('#', setModulePriority(moduleName, 7)))
      assert.equals(7, getModulePriority(moduleName))
      setModulePriority(moduleName, -2)
      assert.equals(-2, getModulePriority(moduleName))
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
      -- leave the module unsynced: a profile save rewrites a synced module's
      -- own .mpackage, and the fixture copy is thrown away when this block ends
      defer(function() disableModuleSync(moduleName) end)
      assert.is_false(getModuleSync(moduleName))

      assert.is_true(enableModuleSync(moduleName))
      assert.is_true(getModuleSync(moduleName))
    end)
  end)

  describe("Tests the functionality of disableModuleSync", function()
    it("returns nil+msg for a module that is not installed", function()
      local ok, err = disableModuleSync("mudlet-spec-never-installed")
      assert.is_nil(ok)
      assert.is_true(contains(err, "not found"), tostring(err))
    end)

    it("turns syncing back off", function()
      defer(function() disableModuleSync(moduleName) end)
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

    it("is false for a module that nobody has turned syncing on for", function()
      assert.is_false(getModuleSync(moduleName))
    end)
  end)

  -- A synced module is the only thing that makes a profile save do any module
  -- work at all: with none installed the save's module list comes out empty and
  -- the background write returns at its first line. The write itself and
  -- everything it touches - the module documents, the backup, the archive
  -- rewrite - therefore go unseen by the sanitizers this suite runs under
  -- unless a spec puts a synced module in the profile first.
  describe("Tests saving a profile that has a module to write", function()
    it("writes the synced module out again", function()
      -- rewriting this module's own .mpackage is only safe because
      -- installFixtureModule() installed a scratch copy, not the committed one
      defer(function() disableModuleSync(moduleName) end)
      assert.is_true(enableModuleSync(moduleName))
      assert.is_true(waitForProfileSaveToPass(), "a profile save was still running")

      -- taking the unpacked XML away is what makes "the save wrote the module
      -- out" a plain yes or no rather than a guess about file timestamps
      local moduleXml = getMudletHomeDir() .. "/" .. moduleName .. "/" .. moduleName .. ".xml"
      os.remove(moduleXml)
      assert.is_nil(lfs.attributes(moduleXml), "the module's unpacked XML could not be cleared")

      assert.is_true(saveProfile())
      assert.is_true(waitUntil(function() return lfs.attributes(moduleXml) ~= nil end, 10000), "the profile save never wrote the synced module out")
      assert.is_true(waitForProfileSaveToPass(), "a profile save was still running")

      -- a file that merely exists could be an empty or half-written one
      local written = io.open(moduleXml, "rb")
      assert.is_not_nil(written, "the module's XML could not be read back")
      local contents = written:read("*a")
      written:close()
      assert.is_true(contains(contents, "<MudletPackage"), "the module's XML was written without a package in it")
    end)
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

  describe("with the fixture module installed", function()
    setup(function()
      installFixtureModule(moduleName)
    end)
    teardown(function()
      removeFixtureModule(moduleName)
    end)

    it("runs the module's scripts again", function()
      local runsBefore = mudletSpecModuleRuns

      reloadModuleUntil(moduleName, function() return mudletSpecModuleRuns > runsBefore end)

      assert.is_true(moduleInstalled(moduleName))
      assert.equals(1, exists(moduleName .. " alias", "alias"))
    end)

    it("re-reads the module's info from its config.lua", function()
      setModuleInfo(moduleName, "title", "changed by the spec")
      assert.equals("changed by the spec", getModuleInfo(moduleName, "title"))

      reloadModuleUntil(moduleName, function()
        return getModuleInfo(moduleName, "title") == "Module fixture for Package_spec.lua"
      end)
    end)

    it("keeps the module's priority and sync setting", function()
      defer(function() disableModuleSync(moduleName) end)
      setModulePriority(moduleName, 4)
      assert.is_true(enableModuleSync(moduleName))
      local runsBefore = mudletSpecModuleRuns

      reloadModuleUntil(moduleName, function() return mudletSpecModuleRuns > runsBefore end)

      assert.equals(4, getModulePriority(moduleName))
      assert.is_true(getModuleSync(moduleName))
    end)
  end)
end)

-- Runs once the block above has uninstalled the module it shares, which is what
-- this is about - it installs nothing of its own.
describe("Tests reading the priority of a module that has been uninstalled", function()
  it("stops answering for it, even though the priority it was given is remembered", function()
    assert.is_false(moduleInstalled(moduleName), "the module accessor specs left their module installed")
    -- Uninstalling leaves the module's entry in the priority map behind, so
    -- reading that map is not a way to tell whether the module is there.
    local ok, err = getModulePriority(moduleName)
    assert.is_nil(ok)
    assert.is_true(contains(err, "module doesn't exist"), tostring(err))
  end)
end)

describe("Tests a package that uninstalls itself", function()
  -- Regression #9557: a package whose event handler uninstalls its own package
  -- used to free the TScript objects that Host::raiseEvent() was still
  -- iterating over. Package auto-updaters do exactly this.
  it("survives a package uninstalling itself from its own event handler", function()
    defer(function()
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
    pumpEvents(100)
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
    defer(function()
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
    -- Nothing in such an archive registers the package, so answering true would
    -- leave a name that getPackages() does not list, that uninstallPackage()
    -- refuses, and a folder in the profile that only a file manager can take
    -- away. If the refusal ever regresses, this puts the folder back by hand.
    defer(function()
      if fileExists(getMudletHomeDir() .. "/mudlet-spec-emptyarchive") then
        os.remove(getMudletHomeDir() .. "/mudlet-spec-emptyarchive/readme.txt")
        lfs.rmdir(getMudletHomeDir() .. "/mudlet-spec-emptyarchive")
      end
    end)

    -- an install asked for while a save is running is postponed and answered
    -- with a bare true, which would read here as the refusal not happening
    assert.is_true(waitForProfileSaveToPass(), "a profile save was still running")

    local ok, err = installPackage(fixtureDirectory .. "/mudlet-spec-emptyarchive.mpackage")
    assert.is_nil(ok)
    -- the message matters: "could not unzip package" here would mean the fixture
    -- has rotted and the spec is passing for the wrong reason
    assert.is_true(contains(err, "no package found in"), tostring(err))
    assert.is_false(packageInstalled("mudlet-spec-emptyarchive"))
    assert.is_false(fileExists(getMudletHomeDir() .. "/mudlet-spec-emptyarchive"))
  end)
end)

describe("Tests the functionality of verbosePackageInstall", function()
  it("installs the package and says so on the main console", function()
    defer(function() removeFixturePackage(minimalPackage) end)
    local path = fixtureDirectory .. "/" .. minimalPackage .. ".mpackage"
    -- an install asked for while a save is running is postponed, and would be
    -- announced as a success without anything being installed
    assert.is_true(waitForProfileSaveToPass(), "a profile save was still running")
    local mark = getLastLineNumber("main")

    verbosePackageInstall(path)

    assert.is_true(packageInstalled(minimalPackage), "the package was not installed")
    assert.is_true(containsWrapped(textFrom(mark), "Package '" .. path .. "' installed successfully."), textFrom(mark))
  end)

  it("says why an install failed", function()
    -- a path that is not there fails without installing anything, so this spec
    -- costs none of the profile saves an install-then-reinstall would
    local path = fixtureDirectory .. "/mudlet-spec-there-is-no-such-package.mpackage"
    assert.is_true(waitForProfileSaveToPass(), "a profile save was still running")
    local mark = getLastLineNumber("main")

    verbosePackageInstall(path)

    local text = textFrom(mark)
    assert.is_true(containsWrapped(text, "Installing '" .. path .. "' failed:"), text)
    assert.is_true(containsWrapped(text, "could not open file"), text)
    assert.is_false(packageInstalled("mudlet-spec-there-is-no-such-package"))
  end)

  it("names the file, not the whole path, when the install fails", function()
    -- the announcement is trimmed on both branches, and only the success one is
    -- reached from installPackageFromUrl's spec
    local name = "mudlet-spec-there-is-no-such-package.mpackage"
    -- an install asked for while a save is running is postponed and answered
    -- with a bare true, so the failure under test would be announced a success
    assert.is_true(waitForProfileSaveToPass(), "a profile save was still running")
    local mark = getLastLineNumber("main")

    verbosePackageInstall(getMudletHomeDir() .. "/" .. name)

    local text = textFrom(mark)
    assert.is_true(containsWrapped(text, "Installing '" .. name .. "' failed:"), text)
    -- the reason installPackage() gives does name the whole path, so only the
    -- announcement's own name is checked for having been trimmed
    assert.is_false(containsWrapped(text, "Installing '" .. getMudletHomeDir()), text)
  end)
end)

describe("Tests the functionality of verboseModuleInstall", function()
  -- A module is installed from a copy inside the profile for the same reason
  -- installFixtureModule() does it: a save rewrites a synced module's own
  -- .mpackage, which must not be the committed fixture.
  local function stageModule()
    lfs.mkdir(scratchDirectory)
    local path = scratchDirectory .. "/" .. moduleName .. ".mpackage"
    copyFile(fixtureDirectory .. "/" .. moduleName .. ".mpackage", path)
    return path
  end

  it("installs the module and says so on the main console", function()
    defer(function() removeFixtureModule(moduleName) end)
    local path = stageModule()
    assert.is_true(waitForProfileSaveToPass(), "a profile save was still running")
    local mark = getLastLineNumber("main")

    verboseModuleInstall(path)

    assert.is_true(moduleInstalled(moduleName), "the module was not installed")
    assert.is_true(containsWrapped(textFrom(mark), "Module '" .. path .. "' installed successfully."), textFrom(mark))
  end)

  it("says why an install failed", function()
    local path = fixtureDirectory .. "/mudlet-spec-there-is-no-such-module.mpackage"
    assert.is_true(waitForProfileSaveToPass(), "a profile save was still running")
    local mark = getLastLineNumber("main")

    verboseModuleInstall(path)

    local text = textFrom(mark)
    -- the module and package failures are announced in the same words, so it is
    -- the spec above, not this one, that tells the two functions apart
    assert.is_true(containsWrapped(text, "Installing '" .. path .. "' failed:"), text)
    assert.is_true(containsWrapped(text, "could not open file"), text)
    assert.is_false(moduleInstalled("mudlet-spec-there-is-no-such-module"))
  end)
end)

describe("Tests the functionality of installPackageFromUrl", function()
  local downloadedName = minimalPackage .. ".mpackage"

  it("downloads the package, installs it and tidies the download away", function()
    defer(function()
      removeFixturePackage(minimalPackage)
      os.remove(getMudletHomeDir() .. "/" .. downloadedName)
    end)
    assert.is_true(waitForProfileSaveToPass(), "a profile save was still running")
    local mark = getLastLineNumber("main")
    -- a file: URL keeps this off the network while still going through
    -- downloadFile() and the sysDownloadDone handler the function registers
    local url = fileUrl(fixtureDirectory .. "/" .. downloadedName)

    installPackageFromUrl(downloadedName, url)

    local event, installedName = waitForEvent("sysInstallPackage", 10000)
    assert.equals("sysInstallPackage", event)
    assert.equals(minimalPackage, installedName)
    assert.is_true(packageInstalled(minimalPackage))
    local text = textFrom(mark)
    assert.is_true(containsWrapped(text, "Downloading package from " .. url), text)
    assert.is_true(containsWrapped(text, "installed successfully."), text)
    assert.is_false(fileExists(getMudletHomeDir() .. "/" .. downloadedName), "the downloaded copy was left in the profile")
  end)

  it("names the file, not the whole path, in the announcement", function()
    -- this only bites while the profile name holds a Lua pattern magic
    -- character ("Mudlet self-test" holds a "-"): a pattern-based strip finds
    -- nothing to match there and announces the whole path
    defer(function()
      removeFixturePackage(minimalPackage)
      os.remove(getMudletHomeDir() .. "/" .. downloadedName)
    end)
    assert.is_true(waitForProfileSaveToPass(), "a profile save was still running")
    local mark = getLastLineNumber("main")

    installPackageFromUrl(downloadedName, fileUrl(fixtureDirectory .. "/" .. downloadedName))

    waitForEvent("sysInstallPackage", 10000)
    assert.is_true(containsWrapped(textFrom(mark), "Package '" .. downloadedName .. "' installed successfully."), textFrom(mark))
  end)

  it("reports a download that failed and installs nothing", function()
    local missingName = "mudlet-spec-never-downloadable.mpackage"
    defer(function() os.remove(getMudletHomeDir() .. "/" .. missingName) end)
    local mark = getLastLineNumber("main")

    installPackageFromUrl(missingName, fileUrl(fixtureDirectory .. "/" .. missingName))

    local event = waitForEvent("sysDownloadError", 10000)
    assert.equals("sysDownloadError", event)
    pumpEvents(200)
    assert.is_false(packageInstalled("mudlet-spec-never-downloadable"))
    local text = textFrom(mark)
    -- the warning only means something paired with the download it reports on
    assert.is_true(containsWrapped(text, "Downloading package from"), text)
    assert.is_true(containsWrapped(text, "[ WARN ]"), text)
  end)
end)

describe("Tests the functionality of packageDrop", function()
  it("hands a dropped package file to the installer", function()
    -- The file is one that is not there: what this is about is that dropping
    -- reaches the installer with the path that was dropped, and installing for
    -- real costs two profile saves that verbosePackageInstall's own spec has
    -- already paid for.
    local path = fixtureDirectory .. "/mudlet-spec-there-is-no-such-drop.mpackage"
    assert.is_true(waitForProfileSaveToPass(), "a profile save was still running")
    local mark = getLastLineNumber("main")

    -- raised rather than called so that the handler registration in Other.lua
    -- is what is being tested as well
    raiseEvent("sysDropEvent", path, "mpackage", 10, 10, "main")

    local text = textFrom(mark)
    assert.is_true(containsWrapped(text, "Installing '" .. path .. "' failed:"), text)
    assert.is_false(packageInstalled("mudlet-spec-there-is-no-such-drop"))
  end)

  it("hands on every kind of file Mudlet installs", function()
    -- same trick as above, so that narrowing the list of suffixes Mudlet
    -- accepts cannot go unnoticed
    assert.is_true(waitForProfileSaveToPass(), "a profile save was still running")

    for _, suffix in ipairs({"xml", "zip", "trigger"}) do
      local path = fixtureDirectory .. "/mudlet-spec-there-is-no-such-drop." .. suffix
      local mark = getLastLineNumber("main")

      packageDrop("sysDropEvent", path, suffix)

      local text = textFrom(mark)
      assert.is_true(containsWrapped(text, "Installing '" .. path .. "' failed:"), suffix .. ": " .. text)
    end
  end)

  it("ignores a file whose type Mudlet does not install", function()
    -- an install that arrives while a save is running is postponed and would
    -- land after this spec rather than in it
    assert.is_true(waitForProfileSaveToPass(), "a profile save was still running")
    local mark = getLastLineNumber("main")

    assert.equals(0, select('#', packageDrop("sysDropEvent", fixtureDirectory .. "/" .. minimalPackage .. ".mpackage", "exe")))

    assert.is_false(packageInstalled(minimalPackage))
    -- an install that was attempted says so either way round, so neither
    -- announcement having been made is what proves the drop was turned away
    local text = textFrom(mark)
    assert.is_false(containsWrapped(text, "installed successfully."), text)
    assert.is_false(containsWrapped(text, "failed:"), text)
  end)
end)

describe("Tests the functionality of packageUrlDrop", function()
  -- installPackageFromUrl() announces the download while the call is still on
  -- the stack, so that line on the console separates a drop that was passed on
  -- from one that was turned away without any waiting. Nothing listens on the
  -- port below, so the download a passed-on drop starts cannot leave the
  -- machine.
  local droppedUrl = "http://127.0.0.1:1/mudlet-spec-dropped.mpackage"

  local function announcedADownload(mark)
    return containsWrapped(textFrom(mark), "Downloading package from")
  end

  it("hands a dropped package URL to the downloader", function()
    defer(function() os.remove(getMudletHomeDir() .. "/mudlet-spec-dropped.mpackage") end)
    local mark = getLastLineNumber("main")

    packageUrlDrop("sysDropUrlEvent", droppedUrl, "http")

    assert.is_true(containsWrapped(textFrom(mark), "Downloading package from " .. droppedUrl), textFrom(mark))
    -- let the refused connection be reported here rather than in a later spec
    waitForEvent("sysDownloadError", 5000)
    assert.is_false(packageInstalled("mudlet-spec-dropped"))
  end)

  it("ignores a URL whose scheme it does not handle", function()
    -- the scheme is a separate argument from the URL, so the URL is one the
    -- spec above proved would otherwise be downloaded
    local mark = getLastLineNumber("main")

    assert.equals(0, select('#', packageUrlDrop("sysDropUrlEvent", droppedUrl, "ftp")))

    assert.is_false(announcedADownload(mark))
  end)

  it("does not download a URL that is not a package file", function()
    -- no save to wait for: a URL with the wrong suffix is handed to the plain
    -- installer, which gives up on opening it as a file before installing
    -- anything
    local mark = getLastLineNumber("main")

    packageUrlDrop("sysDropUrlEvent", "http://127.0.0.1:1/mudlet-spec-not-a-package.txt", "http")

    assert.is_false(announcedADownload(mark))
  end)
end)

describe("The package specs clean up after themselves", function()
  it("leaves no fixture package, module or folder behind", function()
    for _, name in ipairs(getPackages()) do
      assert.is_nil(name:find("mudlet%-spec%-"), "left the package " .. name .. " installed")
    end
    for _, name in ipairs(getModules()) do
      assert.is_nil(name:find("mudlet%-spec%-"), "left the module " .. name .. " installed")
    end
    for entry in lfs.dir(getMudletHomeDir()) do
      assert.is_nil(entry:find("mudlet%-spec%-"), "left the folder " .. entry .. " behind")
    end
    assert.is_false(fileExists(scratchDirectory), "left the fixture scratch folder behind")

    -- A profile save that catches the sync spec's module while syncing is on
    -- copies it into the shared module backup folder. That is Mudlet working as
    -- intended rather than a leak to fail the run over, but the copy is this
    -- file's to take away again.
    local configurationDirectory = getMudletHomeDir():match("^(.*)[/\\]profiles[/\\]")
    assert.is_string(configurationDirectory, "could not work out the configuration folder from " .. getMudletHomeDir())
    local backups = configurationDirectory .. "/moduleBackups"
    if fileExists(backups) then
      for entry in lfs.dir(backups) do
        if entry:find("mudlet%-spec%-") then
          os.remove(backups .. "/" .. entry)
        end
      end
    end

    -- Let the profile save that the last uninstall queued run while the profile
    -- is still up, rather than leaving it to be stopped by the profile close.
    pumpEvents(1500)
  end)
end)
