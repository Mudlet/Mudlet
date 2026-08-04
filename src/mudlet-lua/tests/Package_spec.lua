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

-- waitForEvent() is inert outside test mode, and without it these specs cannot
-- let the profile save finish - the uninstalls would fail and strand fixture
-- packages in the profile, so say so rather than make a mess of it.
if not os.getenv("MUDLET_TEST_MODE") then
  describe("Tests the package and module lifecycle", function()
    it("needs test mode", function()
      pending("the package specs need MUDLET_TEST_MODE (waitForEvent() does nothing without it)")
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

-- Mudlet stops responding part way through this file on macOS: every install
-- and uninstall starts a profile save, and on that platform the run wedges
-- somewhere in the middle of them, so the one-minute CI step for the Lua tests
-- is killed. Linux (including the AddressSanitizer build) and Windows run the
-- whole file fine. Until the save is fixed the specs that install something are
-- pending on macOS; the contract specs still run there.
local installsWedgeThisPlatform = getOS() == "mac"

local function requireWorkingInstalls()
  if installsWedgeThisPlatform then
    pending("installing a package wedges Mudlet on macOS - see the PR that added this file")
  end
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
    waitForProfileSaveToPass()
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
    pumpEventLoop(400 * attempt)
  end
  assert.is_true(false, "could not install " .. what)
end

-- The same postponement answers a bad install path with true as well, so a spec
-- about the refusal waits for the save to pass and asks again.
local function installUntilRefused(install, path)
  for attempt = 1, 3 do
    waitForProfileSaveToPass()
    local ok, err = install(path)
    if ok == nil then
      return err
    end
    pumpEventLoop(400 * attempt)
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
    pumpEventLoop(400 * attempt)
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
    waitForProfileSaveToPass()
    -- uninstallPackage() refuses while a profile save is in progress, and the
    -- installs here start one, so keep asking until it takes
    assert.is_true(waitUntil(function() return uninstallPackage(name) == true end, 5000),
                   "could not uninstall the fixture package " .. name)
    -- let the profile save that uninstallPackage() queues run now, rather than
    -- during Mudlet's shutdown
    pumpEventLoop(200)
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
    waitForProfileSaveToPass()
    assert.is_true(waitUntil(function() return uninstallModule(name) == true end, 5000),
                   "could not uninstall the fixture module " .. name)
    pumpEventLoop(200)
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
      if installsWedgeThisPlatform then
        return
      end
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
      if installsWedgeThisPlatform then
        return
      end
      for _, handler in ipairs(handlers) do
        killAnonymousEventHandler(handler)
      end
      removeFixturePackage(minimalPackage)
    end)

    it("unpacks the package into the profile and runs its contents", function()
      requireWorkingInstalls()
      local packageDirectory = getMudletHomeDir() .. "/" .. minimalPackage
      assert.is_true(fileExists(packageDirectory), "the package folder was not created")
      assert.is_true(fileExists(packageDirectory .. "/config.lua"))
      assert.is_true(fileExists(packageDirectory .. "/" .. minimalPackage .. ".xml"))
      assert.equals(1, exists(minimalPackage .. " alias", "alias"))
      assert.equals(1, exists("mudletSpecMinimalScript", "script"))
      assert.is_true(mudletSpecMinimalRuns > runsBefore, "the package's script did not run")
    end)

    it("raises sysInstall and sysInstallPackage once the install is complete", function()
      requireWorkingInstalls()
      assert.equals(1, #installEvents)
      assert.equals(minimalPackage, installEvents[1][1])
      assert.equals(1, #packageEvents)
      assert.equals(minimalPackage, packageEvents[1][1])
      assert.is_true(contains(packageEvents[1][2], minimalPackage .. ".mpackage"), tostring(packageEvents[1][2]))
    end)

    it("refuses to install a package that is already installed", function()
      requireWorkingInstalls()
      local err = installUntilRefused(installPackage, fixtureDirectory .. "/" .. minimalPackage .. ".mpackage")
      assert.is_true(contains(err, "package " .. minimalPackage .. " is already installed"), tostring(err))
    end)
  end)

  it("unpacks a folder of resources that ships with a package", function()
    requireWorkingInstalls()
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
    requireWorkingInstalls()
    withFixturePackage("mudlet-spec-noconfig")

    assert.equals(1, exists("mudlet-spec-noconfig alias", "alias"))
    assert.same({}, getPackageInfo("mudlet-spec-noconfig"))
  end)

  it("installs a package from a plain XML file", function()
    requireWorkingInstalls()
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
    requireWorkingInstalls()
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
end)

describe("Tests the package info accessors", function()
  setup(function()
    if not installsWedgeThisPlatform then
      installFixturePackage(minimalPackage)
    end
  end)
  teardown(function()
    if not installsWedgeThisPlatform then
      removeFixturePackage(minimalPackage)
    end
  end)

  describe("Tests the functionality of getPackageInfo", function()
    it("raises a Lua error when the package name is not a string", function()
      assertArgError(function() getPackageInfo({}) end, "getPackageInfo: bad argument #1 type")
    end)

    it("raises a Lua error when the requested field is not a string", function()
      requireWorkingInstalls()
      assertArgError(function() getPackageInfo(minimalPackage, {}) end, "getPackageInfo: bad argument #2 type")
    end)

    it("returns everything the package's config.lua declared", function()
      requireWorkingInstalls()
      assert.same({
        mpackage = minimalPackage,
        author = "Mudlet test suite",
        title = "Minimal fixture package for Package_spec.lua",
        version = "1.0",
        description = "One alias and one script, just enough to prove a package installed.",
      }, getPackageInfo(minimalPackage))
    end)

    it("returns a single field when one is named", function()
      requireWorkingInstalls()
      assert.equals("1.0", getPackageInfo(minimalPackage, "version"))
      assert.equals("Mudlet test suite", getPackageInfo(minimalPackage, "author"))
    end)

    it("returns an empty string for a field the package does not have", function()
      requireWorkingInstalls()
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
      requireWorkingInstalls()
      assertArgError(function() setPackageInfo(minimalPackage, "version") end, "setPackageInfo: bad argument #3 type")
    end)

    it("round-trips a value through getPackageInfo", function()
      requireWorkingInstalls()
      defer(function() setPackageInfo(minimalPackage, "version", "1.0") end)

      assert.is_true(setPackageInfo(minimalPackage, "version", "9.9"))
      assert.equals("9.9", getPackageInfo(minimalPackage, "version"))
      assert.equals("9.9", getPackageInfo(minimalPackage).version)
    end)

    it("adds a field the package did not declare", function()
      requireWorkingInstalls()
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
      if installsWedgeThisPlatform then
        return
      end
      runsBefore = mudletSpecModuleRuns or 0
      local genericHandler, detailedHandler
      installEvents, genericHandler = collectEvents("sysInstall")
      moduleEvents, detailedHandler = collectEvents("sysLuaInstallModule")
      handlers = {genericHandler, detailedHandler}
      modulePath = installFixtureModule(moduleName)
      waitUntil(function() return #moduleEvents > 0 end, 2000)
    end)

    teardown(function()
      if installsWedgeThisPlatform then
        return
      end
      for _, handler in ipairs(handlers) do
        killAnonymousEventHandler(handler)
      end
      removeFixtureModule(moduleName)
    end)

    it("installs the module, unpacks it and runs its contents", function()
      requireWorkingInstalls()
      assert.is_true(moduleInstalled(moduleName))
      -- a module is not a package: it must not turn up in getPackages()
      assert.is_false(packageInstalled(moduleName))
      assert.is_true(fileExists(getMudletHomeDir() .. "/" .. moduleName))
      assert.equals(1, exists(moduleName .. " alias", "alias"))
      assert.is_true(mudletSpecModuleRuns > runsBefore, "the module's script did not run")
    end)

    it("raises sysInstall and sysLuaInstallModule", function()
      requireWorkingInstalls()
      assert.equals(1, #installEvents)
      assert.equals(moduleName, installEvents[1][1])
      assert.equals(1, #moduleEvents)
      assert.equals(moduleName, moduleEvents[1][1])
      assert.is_true(contains(moduleEvents[1][2], moduleName .. ".mpackage"), tostring(moduleEvents[1][2]))
    end)

    it("refuses to install a module that is already installed", function()
      requireWorkingInstalls()
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
    requireWorkingInstalls()
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
    if not installsWedgeThisPlatform then
      modulePath = installFixtureModule(moduleName)
    end
  end)
  teardown(function()
    if not installsWedgeThisPlatform then
      removeFixtureModule(moduleName)
    end
  end)

  describe("Tests the functionality of getModuleInfo", function()
    it("raises a Lua error when the module name is not a string", function()
      assertArgError(function() getModuleInfo({}) end, "getModuleInfo: bad argument #1 type")
    end)

    it("returns everything the module's config.lua declared", function()
      requireWorkingInstalls()
      assert.same({
        mpackage = moduleName,
        author = "Mudlet test suite",
        title = "Module fixture for Package_spec.lua",
        version = "3.1",
        description = "Counts how often its script has been compiled, so a reload is observable.",
      }, getModuleInfo(moduleName))
    end)

    it("returns a single field when one is named", function()
      requireWorkingInstalls()
      assert.equals("3.1", getModuleInfo(moduleName, "version"))
      assert.equals("", getModuleInfo(moduleName, "no-such-field"))
    end)

    it("returns an empty table for a module that is not installed", function()
      assert.same({}, getModuleInfo("mudlet-spec-never-installed"))
    end)
  end)

  describe("Tests the functionality of setModuleInfo", function()
    it("raises a Lua error when the value is missing", function()
      requireWorkingInstalls()
      assertArgError(function() setModuleInfo(moduleName, "version") end, "setModuleInfo: bad argument #3 type")
    end)

    it("round-trips a value through getModuleInfo", function()
      requireWorkingInstalls()
      defer(function() setModuleInfo(moduleName, "version", "3.1") end)

      assert.is_true(setModuleInfo(moduleName, "version", "8.8"))
      assert.equals("8.8", getModuleInfo(moduleName, "version"))
      assert.equals("8.8", getModuleInfo(moduleName).version)
    end)
  end)

  describe("Tests the functionality of getModulePath", function()
    it("returns the file the module was installed from", function()
      requireWorkingInstalls()
      assert.equals(modulePath, getModulePath(moduleName))
    end)
  end)

  describe("Tests the functionality of getModulePriority", function()
    -- Runs before setModulePriority's specs on purpose: a priority outlives the
    -- module it was set on (Host::uninstallPackage() leaves mModulePriorities
    -- alone), so once one has been set for this module the default this spec is
    -- about can never be observed again.
    it("reports the default priority of a freshly installed module", function()
      -- BUG: a module nobody has called setModulePriority() on has no entry in
      -- the priority map, so getModulePriority() answers nil and "module
      -- doesn't exist" for a module that plainly does exist. The module manager
      -- reads the same map with operator[] and so shows 0, which is what this
      -- should return. Left pending rather than pinning the wrong answer.
      pending("getModulePriority() reports an installed module as non-existent until a priority is set")

      assert.equals(0, getModulePriority(moduleName))
    end)
  end)

  describe("Tests the functionality of setModulePriority", function()
    it("raises a Lua error when the priority is missing", function()
      requireWorkingInstalls()
      assertArgError(function() setModulePriority(moduleName) end, "setModulePriority: bad argument #2 type")
    end)

    it("returns nil+msg for a module that is not installed", function()
      local ok, err = setModulePriority("mudlet-spec-never-installed", 3)
      assert.is_nil(ok)
      assert.is_true(contains(err, "module doesn't exist"), tostring(err))
    end)

    it("returns no values and is read back by getModulePriority", function()
      requireWorkingInstalls()
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
      requireWorkingInstalls()
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
      requireWorkingInstalls()
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
      requireWorkingInstalls()
      assert.is_false(getModuleSync(moduleName))
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
      if not installsWedgeThisPlatform then
        installFixtureModule(moduleName)
      end
    end)
    teardown(function()
      if not installsWedgeThisPlatform then
        removeFixtureModule(moduleName)
      end
    end)

    it("runs the module's scripts again", function()
      requireWorkingInstalls()
      local runsBefore = mudletSpecModuleRuns

      reloadModuleUntil(moduleName, function() return mudletSpecModuleRuns > runsBefore end)

      assert.is_true(moduleInstalled(moduleName))
      assert.equals(1, exists(moduleName .. " alias", "alias"))
    end)

    it("re-reads the module's info from its config.lua", function()
      requireWorkingInstalls()
      setModuleInfo(moduleName, "title", "changed by the spec")
      assert.equals("changed by the spec", getModuleInfo(moduleName, "title"))

      reloadModuleUntil(moduleName, function()
        return getModuleInfo(moduleName, "title") == "Module fixture for Package_spec.lua"
      end)
    end)

    it("keeps the module's priority and sync setting", function()
      requireWorkingInstalls()
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

describe("Tests a package that uninstalls itself", function()
  -- Regression #9557: a package whose event handler uninstalls its own package
  -- used to free the TScript objects that Host::raiseEvent() was still
  -- iterating over. Package auto-updaters do exactly this.
  it("survives a package uninstalling itself from its own event handler", function()
    requireWorkingInstalls()
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
    -- BUG: such an archive is unpacked into the profile and answered with true,
    -- but nothing is registered: it is missing from getPackages(), so
    -- uninstallPackage() will not take it and the unpacked folder stays in the
    -- profile for good. Left pending rather than pinning a success that
    -- installs nothing and cannot be undone.
    pending("installPackage() answers true for an archive with no package in it, and leaves it unremovable")
    -- uninstallPackage() will not take a package it never registered, so the
    -- unpacked folder has to go by hand
    defer(function()
      os.remove(getMudletHomeDir() .. "/mudlet-spec-emptyarchive/readme.txt")
      lfs.rmdir(getMudletHomeDir() .. "/mudlet-spec-emptyarchive")
    end)

    local ok, err = installPackage(fixtureDirectory .. "/mudlet-spec-emptyarchive.mpackage")
    assert.is_nil(ok)
    assert.is_string(err)
    assert.is_false(fileExists(getMudletHomeDir() .. "/mudlet-spec-emptyarchive"))
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
    -- is still up: it dereferences the profile when it fires, and Mudlet may be
    -- shutting down by the time it would otherwise get its turn.
    pumpEventLoop(1500)
  end)
end)
