-- Specs for the networking, MMCP and Discord APIs. The media contracts that
-- used to be here now live in Media_spec.lua.
--
-- Most of these functions depend on live infrastructure for their real effect
-- (a connected game server, connected MMCP peers, a running Discord client),
-- and for those what is verified here is the part that is fully deterministic
-- offline: argument validation, and the nil+message / hard-error shapes each
-- function returns when its precondition (a connection, a peer, an enabled
-- protocol, an available API) is not met.
--
-- The download and HTTP families are the exception: their infrastructure can be
-- stood up locally, so their real effects are checked against the fixture
-- server in CI/http-fixture-server.py, whose ephemeral port arrives in
-- MUDLET_TEST_HTTP_PORT. They skip cleanly when it is absent so the suite still
-- passes without a server. Nothing here mocks a real API function.

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
  assert.is_true(contains(err, needle))
end

describe("Networking send functions honour their disconnected/offline contracts", function()
  -- Force a non-connected telnet state so the connection guards fire
  -- deterministically regardless of what the self-test profile's socket is
  -- doing. disconnect() only closes the socket; it issues no traffic.
  before_each(function()
    disconnect()
  end)

  describe("sendMSDP", function()
    it("raises a Lua error when called with no arguments", function()
      assert.has_error(function() sendMSDP() end)
    end)

    it("raises a Lua error when a value argument is not a string", function()
      assert.has_error(function() sendMSDP("HEALTH", {}) end)
    end)

    it("returns nil and a message while disconnected", function()
      local ok, err = sendMSDP("HEALTH")
      assert.is_nil(ok)
      assert.is_true(contains(err, "not connected to game server"))
    end)
  end)

  describe("sendATCP", function()
    it("names the offending value's real type when the message is not a string", function()
      -- Regression #9543: the type-name placeholder must be expanded, not printed
      -- as a literal "%1". lua_pushfstring only understands C-style "%s".
      local ok, err = pcall(function() sendATCP({}) end)
      assert.is_false(ok)
      assert.is_true(contains(err, "sendATCP: bad argument #1 type (message as string expected, got table!)"), tostring(err))
      assert.is_false(contains(err, "%1"), tostring(err))
    end)

    it("names the real type when the optional second argument is not a string", function()
      local ok, err = pcall(function() sendATCP("Char.Login", {}) end)
      assert.is_false(ok)
      assert.is_true(contains(err, "sendATCP: bad argument #2 type (what as string is optional, got table!)"), tostring(err))
      assert.is_false(contains(err, "%1"), tostring(err))
    end)

    it("returns nil and a message while disconnected", function()
      local ok, err = sendATCP("Char.Login")
      assert.is_nil(ok)
      assert.is_true(contains(err, "not connected to game server"))
    end)
  end)

  describe("sendTelnetChannel102", function()
    it("raises a Lua error when the payload is not a string", function()
      assert.has_error(function() sendTelnetChannel102({}) end)
    end)

    it("returns nil when the payload is not exactly two bytes", function()
      local ok, err = sendTelnetChannel102("x")
      assert.is_nil(ok)
      assert.is_true(contains(err, "invalid message of length 1"))
    end)

    it("returns nil when subchannel 102 has not been enabled by the server", function()
      local ok, err = sendTelnetChannel102("ab")
      assert.is_nil(ok)
      assert.is_true(contains(err, "102 subchannel support has not been enabled"))
    end)
  end)

  describe("sendSocket", function()
    it("raises a Lua error when the data is not a string", function()
      assert.has_error(function() sendSocket({}) end)
    end)

    it("returns nil and a message when the socket cannot accept the data", function()
      local ok, err = sendSocket("noop")
      assert.is_nil(ok)
      assert.is_true(contains(err, "unable to send"))
    end)
  end)
end)

describe("connectToServer validates its arguments without connecting", function()
  it("raises a Lua error when the url is missing", function()
    assert.has_error(function() connectToServer() end)
  end)

  it("rejects an out-of-range port and returns nil plus a message", function()
    local ok, err = connectToServer("example.invalid", 70000)
    assert.is_nil(ok)
    assert.is_true(contains(err, "invalid port number"))
  end)

  it("rejects a port below 1", function()
    local ok, err = connectToServer("example.invalid", 0)
    assert.is_nil(ok)
    assert.is_true(contains(err, "invalid port number"))
  end)
end)

describe("getConnectionInfo returns a host/port/connected triple", function()
  it("returns a string, a number and a boolean", function()
    local host, port, connected = getConnectionInfo()
    assert.is_string(host)
    assert.is_number(port)
    assert.is_boolean(connected)
  end)
end)

describe("HTTP and download functions validate arguments before issuing a request", function()
  -- Every case below returns (hard error, or nil+message) strictly before the
  -- network call: either the url is invalid (so the request is refused locally)
  -- or a valid-looking url is never contacted because a header/argument error is
  -- raised first.
  describe("downloadFile", function()
    it("raises a Lua error when the local filename is missing", function()
      assertArgError(function() downloadFile() end, "downloadFile: bad argument")
    end)

    it("raises a Lua error when the url is missing", function()
      assertArgError(function() downloadFile("/tmp/mudlet-contract-test") end, "downloadFile: bad argument")
    end)

    it("returns nil for an invalid url without downloading", function()
      local ok, err = downloadFile("/tmp/mudlet-contract-test", "")
      assert.is_nil(ok)
      assert.is_true(contains(err, "url is invalid"))
    end)
  end)

  describe("getHTTP", function()
    it("raises a Lua error when the url is missing", function()
      assertArgError(function() getHTTP() end, "getHTTP: bad argument")
    end)

    it("returns nil for an invalid url without issuing a request", function()
      local ok, err = getHTTP("")
      assert.is_nil(ok)
      assert.is_true(contains(err, "url is invalid"))
    end)

    it("raises a Lua error when headers is not a table", function()
      assertArgError(function() getHTTP("http://localhost/", 5) end,
        "getHTTP: bad argument #2 type (headers as a table expected, got number!)")
    end)

    it("raises a Lua error when a header value is not a string", function()
      assertArgError(function() getHTTP("http://localhost/", {["X-Test"] = 5}) end, "getHTTP: bad argument")
    end)
  end)

  describe("deleteHTTP", function()
    it("raises a Lua error when the url is missing", function()
      assertArgError(function() deleteHTTP() end, "deleteHTTP: bad argument")
    end)

    it("returns nil for an invalid url without issuing a request", function()
      local ok, err = deleteHTTP("")
      assert.is_nil(ok)
      assert.is_true(contains(err, "url is invalid"))
    end)

    it("raises a Lua error when headers is not a table", function()
      assertArgError(function() deleteHTTP("http://localhost/", 5) end,
        "deleteHTTP: bad argument #2 type (headers as a table expected, got number!)")
    end)

    it("raises a Lua error when a header value is not a string", function()
      assertArgError(function() deleteHTTP("http://localhost/", {["X-Test"] = 5}) end, "deleteHTTP: bad argument")
    end)
  end)

  describe("postHTTP", function()
    it("raises a Lua error when the data argument is missing", function()
      assertArgError(function() postHTTP() end, "postHTTP: bad argument")
    end)

    it("raises a Lua error when the url is missing", function()
      assertArgError(function() postHTTP("payload") end, "postHTTP: bad argument")
    end)

    it("returns nil for an invalid url without issuing a request", function()
      local ok, err = postHTTP("payload", "")
      assert.is_nil(ok)
      assert.is_true(contains(err, "url is invalid"))
    end)

    it("raises a Lua error when headers is not a table", function()
      assertArgError(function() postHTTP("payload", "http://localhost/", 5) end,
        "postHTTP: bad argument #3 type (headers as a table expected, got number!)")
    end)
  end)

  describe("putHTTP", function()
    it("raises a Lua error when the data argument is missing", function()
      assertArgError(function() putHTTP() end, "putHTTP: bad argument")
    end)

    it("raises a Lua error when the url is missing", function()
      assertArgError(function() putHTTP("payload") end, "putHTTP: bad argument")
    end)

    it("returns nil for an invalid url without issuing a request", function()
      local ok, err = putHTTP("payload", "")
      assert.is_nil(ok)
      assert.is_true(contains(err, "url is invalid"))
    end)

    it("raises a Lua error when headers is not a table", function()
      assertArgError(function() putHTTP("payload", "http://localhost/", 5) end,
        "putHTTP: bad argument #3 type (headers as a table expected, got number!)")
    end)
  end)

  describe("the optional file argument", function()
    it("returns nil and a message when the file cannot be read", function()
      local ok, err = postHTTP("payload", "http://localhost/", {}, getMudletHomeDir() .. "/busted-no-such-upload.txt")
      assert.is_nil(ok)
      assert.is_true(contains(err, "couldn't open"), tostring(err))
      assert.is_true(contains(err, "busted-no-such-upload.txt"), tostring(err))
    end)
  end)

  describe("customHTTP", function()
    it("raises a Lua error when the method is missing", function()
      assertArgError(function() customHTTP() end, "customHTTP: bad argument")
    end)

    it("raises a Lua error when the data argument is missing", function()
      assertArgError(function() customHTTP("REPORT") end, "customHTTP: bad argument")
    end)

    it("raises a Lua error when the url is missing", function()
      assertArgError(function() customHTTP("REPORT", "payload") end, "customHTTP: bad argument")
    end)

    it("reports the real type of a non-table headers argument", function()
      -- Regression #9544: performHttpRequest must read the type of the offending
      -- slot (pos + 3), not a hardcoded slot 3, so the headers error names the
      -- number that was actually passed rather than the url's type.
      local ok, err = pcall(function() customHTTP("REPORT", "payload", "http://localhost/", 5) end)
      assert.is_false(ok)
      assert.is_true(contains(err, "customHTTP: bad argument #4 type (headers as a table expected, got number!)"), tostring(err))
    end)

    it("reports the real type of a non-string file argument", function()
      -- Regression #9544: the file error must read pos + 4, not a hardcoded slot 4,
      -- so it names the boolean that was passed and not the headers table's type.
      -- A boolean is used rather than a number because lua_isstring also accepts
      -- numbers, so only a genuinely non-string value reaches the type error.
      local ok, err = pcall(function() customHTTP("REPORT", "payload", "http://localhost/", {}, true) end)
      assert.is_false(ok)
      assert.is_true(contains(err, "customHTTP: bad argument #5 type (file to send as string location expected, got boolean!)"), tostring(err))
    end)
  end)
end)

describe("Downloads and HTTP verbs against the local fixture server", function()
  -- CI starts CI/http-fixture-server.py before the suite and passes its
  -- ephemeral port in MUDLET_TEST_HTTP_PORT. The server serves
  -- CI/http-fixtures/ and answers any verb below /echo by reporting the
  -- method, headers and body it received, which is how these specs prove what
  -- Mudlet actually put on the wire. Every response carries the
  -- X-Mudlet-Fixture header so the response table each event delivers can be
  -- checked too.
  --
  -- The requests are asynchronous: nothing is sent until the event loop runs,
  -- which only happens inside waitForEvent(), so arming the wait after issuing
  -- the request cannot miss the reply.
  local httpPort = os.getenv("MUDLET_TEST_HTTP_PORT")
  -- the contents of CI/http-fixtures/fixture.txt
  local fixtureBody = "Mudlet self-test HTTP fixture.\n"

  local function fixtureUrl(path)
    return "http://127.0.0.1:" .. httpPort .. path
  end

  -- Returns true when the caller must stop because there is no server to talk
  -- to. A developer's local run without the fixture server still passes; CI
  -- sets MUDLET_TEST_REQUIRE_HTTP_FIXTURE so that a workflow which stops
  -- handing the port over fails instead of quietly skipping the whole family.
  local requireFixture = os.getenv("MUDLET_TEST_REQUIRE_HTTP_FIXTURE")

  local function noFixtureServer()
    if httpPort then
      return false
    end
    if requireFixture then
      assert.is_true(false, "MUDLET_TEST_REQUIRE_HTTP_FIXTURE is set but MUDLET_TEST_HTTP_PORT is not - the fixture server did not reach the specs")
    end
    pending("MUDLET_TEST_HTTP_PORT is not set (fixture HTTP server not running)")
    return true
  end

  local function readFile(path)
    local handle = io.open(path, "rb")
    if not handle then
      return nil
    end
    local body = handle:read("*a")
    handle:close()
    return body
  end

  local function writeFile(path, body)
    local handle = io.open(path, "wb")
    handle:write(body)
    handle:close()
  end

  -- Qt normalises the header names it hands back (they arrive lower-cased from
  -- Qt 6.7 on, as sent before that), so look the header up without relying on
  -- its case.
  local function headerValue(response, name)
    for key, value in pairs(response.headers) do
      if key:lower() == name:lower() then
        return value
      end
    end
    return nil
  end

  -- Proves the response reached Lua as a table carrying the header and the
  -- cookie that the fixture server sets on every response, rather than as some
  -- other truthy value.
  local function assertFixtureResponse(response)
    assert.is_table(response)
    assert.is_table(response.headers)
    assert.is_table(response.cookies)
    assert.equals("1", headerValue(response, "X-Mudlet-Fixture"))
    assert.equals("1", response.cookies["mudlet-fixture"])
  end

  describe("downloadFile", function()
    it("writes the fixture to disk and reports it in sysDownloadDone", function()
      if noFixtureServer() then
        return
      end
      local target = getMudletHomeDir() .. "/busted-download-done.txt"
      os.remove(target)
      finally(function() os.remove(target) end)
      local queued, actualUrl = downloadFile(target, fixtureUrl("/fixture.txt"))
      assert.is_true(queued)
      assert.equals(fixtureUrl("/fixture.txt"), actualUrl)

      local event, localFile, bytesWritten, response = waitForEvent("sysDownloadDone", 2000)
      assert.equals("sysDownloadDone", event)
      assert.equals(target, localFile)
      assert.equals(#fixtureBody, bytesWritten)
      assert.equals(fixtureBody, readFile(target))
      assertFixtureResponse(response)
    end)

    it("reports the download's progress while it runs", function()
      if noFixtureServer() then
        return
      end
      local target = getMudletHomeDir() .. "/busted-download-progress.txt"
      os.remove(target)
      -- Collected through a handler rather than a wait of its own: how many
      -- progress events Qt emits for a 31 byte body is not fixed, but the last
      -- one must account for the whole body.
      local progress = {}
      local handler = registerAnonymousEventHandler("sysDownloadFileProgress", function(_, url, downloaded, total)
        progress[#progress + 1] = {url = url, downloaded = downloaded, total = total}
      end)
      finally(function()
        killAnonymousEventHandler(handler)
        os.remove(target)
      end)

      assert.is_true(downloadFile(target, fixtureUrl("/fixture.txt")))
      assert.equals("sysDownloadDone", (waitForEvent("sysDownloadDone", 2000)))

      assert.is_true(#progress > 0)
      local last = progress[#progress]
      assert.equals(fixtureUrl("/fixture.txt"), last.url)
      assert.equals(#fixtureBody, last.downloaded)
      assert.equals(#fixtureBody, last.total)
    end)

    it("raises sysDownloadError and writes no file when the url 404s", function()
      if noFixtureServer() then
        return
      end
      local target = getMudletHomeDir() .. "/busted-download-missing.txt"
      os.remove(target)
      finally(function() os.remove(target) end)
      assert.is_true(downloadFile(target, fixtureUrl("/no-such-fixture.txt")))

      local event, message, localFile, url, response = waitForEvent("sysDownloadError", 2000)
      assert.equals("sysDownloadError", event)
      assert.is_string(message)
      assert.equals(target, localFile)
      assert.equals(fixtureUrl("/no-such-fixture.txt"), url)
      assertFixtureResponse(response)
      assert.is_nil(readFile(target))
    end)

    it("raises sysDownloadError naming the local file when it cannot be written", function()
      if noFixtureServer() then
        return
      end
      -- the directory does not exist, so QSaveFile cannot open the target: this
      -- path reports a local reason as its fourth argument where the network
      -- error path reports the url
      local target = getMudletHomeDir() .. "/busted-no-such-directory/download.txt"
      assert.is_true(downloadFile(target, fixtureUrl("/fixture.txt")))

      local event, message, localFile, reason = waitForEvent("sysDownloadError", 2000)
      assert.equals("sysDownloadError", event)
      assert.equals("Couldn't save to the destination file", message)
      assert.equals(target, localFile)
      assert.equals("Couldn't open the destination file for writing (permission errors?)", reason)
    end)
  end)

  describe("getHTTP", function()
    it("delivers the fixture's body in sysGetHttpDone", function()
      if noFixtureServer() then
        return
      end
      local queued = getHTTP(fixtureUrl("/fixture.txt"))
      assert.is_true(queued)

      local event, url, body, response = waitForEvent("sysGetHttpDone", 2000)
      assert.equals("sysGetHttpDone", event)
      assert.equals(fixtureUrl("/fixture.txt"), url)
      assert.equals(fixtureBody, body)
      assertFixtureResponse(response)
    end)

    it("sends the custom headers it was given", function()
      if noFixtureServer() then
        return
      end
      assert.is_true(getHTTP(fixtureUrl("/echo"), {["X-Mudlet-Test"] = "get-header"}))

      local event, _, body = waitForEvent("sysGetHttpDone", 2000)
      assert.equals("sysGetHttpDone", event)
      assert.is_true(contains(body, "method=GET"))
      assert.is_true(contains(body, "header:x-mudlet-test=get-header"), body)
      -- setNetworkRequestDefaults() puts Mudlet's own user agent on the request
      assert.is_true(contains(body, "header:user-agent=Mozilla/5.0 (Mudlet/"), body)
    end)

    it("raises sysGetHttpError for a url that 404s", function()
      if noFixtureServer() then
        return
      end
      assert.is_true(getHTTP(fixtureUrl("/no-such-fixture.txt")))

      local event, message, url, response = waitForEvent("sysGetHttpError", 2000)
      assert.equals("sysGetHttpError", event)
      assert.is_string(message)
      assert.equals(fixtureUrl("/no-such-fixture.txt"), url)
      assertFixtureResponse(response)
    end)
  end)

  describe("postHTTP", function()
    it("sends its data and headers, and reports the reply in sysPostHttpDone", function()
      if noFixtureServer() then
        return
      end
      local queued = postHTTP("posted=payload", fixtureUrl("/echo"), {["X-Mudlet-Test"] = "post-header"})
      assert.is_true(queued)

      local event, url, body, response = waitForEvent("sysPostHttpDone", 2000)
      assert.equals("sysPostHttpDone", event)
      assert.equals(fixtureUrl("/echo"), url)
      assert.is_true(contains(body, "method=POST"), body)
      assert.is_true(contains(body, "header:x-mudlet-test=post-header"), body)
      assert.is_true(contains(body, "body=posted=payload"), body)
      assertFixtureResponse(response)
    end)

    it("sends a file's contents in place of the data argument", function()
      if noFixtureServer() then
        return
      end
      local upload = getMudletHomeDir() .. "/busted-http-upload.txt"
      writeFile(upload, "contents from the uploaded file")
      finally(function() os.remove(upload) end)

      assert.is_true(postHTTP("data that must be ignored", fixtureUrl("/echo"), {}, upload))

      local event, _, body = waitForEvent("sysPostHttpDone", 2000)
      assert.equals("sysPostHttpDone", event)
      assert.is_true(contains(body, "body=contents from the uploaded file"), body)
      assert.is_false(contains(body, "data that must be ignored"))
    end)

    it("accepts a nil data argument when a file is supplied", function()
      if noFixtureServer() then
        return
      end
      local upload = getMudletHomeDir() .. "/busted-http-upload-only.txt"
      writeFile(upload, "file body with no data argument")
      finally(function() os.remove(upload) end)

      assert.is_true(postHTTP(nil, fixtureUrl("/echo"), {}, upload))

      local event, _, body = waitForEvent("sysPostHttpDone", 2000)
      assert.equals("sysPostHttpDone", event)
      assert.is_true(contains(body, "body=file body with no data argument"), body)
    end)

    it("raises sysPostHttpError when the endpoint refuses the verb", function()
      if noFixtureServer() then
        return
      end
      -- Only /echo accepts a POST; the static fixture path answers 404.
      assert.is_true(postHTTP("payload", fixtureUrl("/fixture.txt")))

      local event, message, url, response = waitForEvent("sysPostHttpError", 2000)
      assert.equals("sysPostHttpError", event)
      assert.is_string(message)
      assert.equals(fixtureUrl("/fixture.txt"), url)
      assertFixtureResponse(response)
    end)
  end)

  describe("putHTTP", function()
    it("sends its data with the PUT verb and reports sysPutHttpDone", function()
      if noFixtureServer() then
        return
      end
      local queued = putHTTP("put=payload", fixtureUrl("/echo"), {["X-Mudlet-Test"] = "put-header"})
      assert.is_true(queued)

      local event, url, body, response = waitForEvent("sysPutHttpDone", 2000)
      assert.equals("sysPutHttpDone", event)
      assert.equals(fixtureUrl("/echo"), url)
      assert.is_true(contains(body, "method=PUT"), body)
      assert.is_true(contains(body, "header:x-mudlet-test=put-header"), body)
      assert.is_true(contains(body, "body=put=payload"), body)
      assertFixtureResponse(response)
    end)

    it("raises sysPutHttpError when the endpoint refuses the verb", function()
      if noFixtureServer() then
        return
      end
      assert.is_true(putHTTP("payload", fixtureUrl("/fixture.txt")))

      local event, message, url = waitForEvent("sysPutHttpError", 2000)
      assert.equals("sysPutHttpError", event)
      assert.is_string(message)
      assert.equals(fixtureUrl("/fixture.txt"), url)
    end)
  end)

  describe("deleteHTTP", function()
    it("sends the DELETE verb and reports sysDeleteHttpDone", function()
      if noFixtureServer() then
        return
      end
      local queued = deleteHTTP(fixtureUrl("/echo"), {["X-Mudlet-Test"] = "delete-header"})
      assert.is_true(queued)

      local event, url, body, response = waitForEvent("sysDeleteHttpDone", 2000)
      assert.equals("sysDeleteHttpDone", event)
      assert.equals(fixtureUrl("/echo"), url)
      assert.is_true(contains(body, "method=DELETE"), body)
      assert.is_true(contains(body, "header:x-mudlet-test=delete-header"), body)
      assertFixtureResponse(response)
    end)

    it("raises sysDeleteHttpError when the endpoint refuses the verb", function()
      if noFixtureServer() then
        return
      end
      assert.is_true(deleteHTTP(fixtureUrl("/fixture.txt")))

      local event, message, url = waitForEvent("sysDeleteHttpError", 2000)
      assert.equals("sysDeleteHttpError", event)
      assert.is_string(message)
      assert.equals(fixtureUrl("/fixture.txt"), url)
    end)
  end)

  describe("customHTTP", function()
    it("sends the verb it was given and echoes it back in sysCustomHttpDone", function()
      if noFixtureServer() then
        return
      end
      local queued = customHTTP("REPORT", "custom=payload", fixtureUrl("/echo"), {["X-Mudlet-Test"] = "custom-header"})
      assert.is_true(queued)

      local event, url, body, method, response = waitForEvent("sysCustomHttpDone", 2000)
      assert.equals("sysCustomHttpDone", event)
      assert.equals(fixtureUrl("/echo"), url)
      assert.equals("REPORT", method)
      assert.is_true(contains(body, "method=REPORT"), body)
      assert.is_true(contains(body, "header:x-mudlet-test=custom-header"), body)
      assert.is_true(contains(body, "body=custom=payload"), body)
      assertFixtureResponse(response)
    end)

    it("raises sysCustomHttpError naming the verb when the endpoint refuses it", function()
      if noFixtureServer() then
        return
      end
      assert.is_true(customHTTP("REPORT", "payload", fixtureUrl("/fixture.txt")))

      local event, message, url, method = waitForEvent("sysCustomHttpError", 2000)
      assert.equals("sysCustomHttpError", event)
      assert.is_string(message)
      assert.equals(fixtureUrl("/fixture.txt"), url)
      assert.equals("REPORT", method)
    end)
  end)
end)

describe("openUrl validates its argument without launching anything", function()
  it("raises a Lua error when the url is missing", function()
    assertArgError(function() openUrl() end, "openUrl: bad argument")
  end)

  it("raises a Lua error when the url is not a string", function()
    assertArgError(function() openUrl({}) end, "openUrl: bad argument")
  end)
end)

describe("MMCP chat commands report the absence of a session", function()
  -- With no connected chat peers, every registered command reports its
  -- no-session state. initMMCPServer() runs lazily inside these calls; it
  -- constructs the server object but never calls listen(), so no socket is
  -- opened (only mmcpStartServer would, and it is not registered into the Lua
  -- mmcp table).
  local NO_CLIENTS = "no connected clients"
  local NO_SUCH = "no client by that name or id"

  it("chatAll returns nil with no peers", function()
    local ok, err = mmcp.chatAll("hi")
    assert.is_nil(ok)
    assert.is_true(contains(err, NO_CLIENTS))
  end)

  it("emoteAll returns nil with no peers", function()
    local ok, err = mmcp.emoteAll("waves")
    assert.is_nil(ok)
    assert.is_true(contains(err, NO_CLIENTS))
  end)

  it("chatGroup returns nil with no peers", function()
    local ok, err = mmcp.chatGroup("friends", "hi")
    assert.is_nil(ok)
    assert.is_true(contains(err, NO_CLIENTS))
  end)

  it("getClientFlags returns nil with no peers", function()
    local ok, err = mmcp.getClientFlags("someone")
    assert.is_nil(ok)
    assert.is_true(contains(err, NO_CLIENTS))
  end)

  it("sendSideChannel returns nil with no peers", function()
    local ok, err = mmcp.sendSideChannel("Chan", "msg")
    assert.is_nil(ok)
    assert.is_true(contains(err, NO_CLIENTS))
  end)

  it("chatTo returns nil for an unknown target", function()
    local ok, err = mmcp.chatTo("nobody", "hi")
    assert.is_nil(ok)
    assert.is_true(contains(err, NO_SUCH))
  end)

  it("ping returns nil for an unknown target", function()
    local ok, err = mmcp.ping("nobody")
    assert.is_nil(ok)
    assert.is_true(contains(err, NO_SUCH))
  end)

  it("setPrivate returns nil for an unknown target", function()
    local ok, err = mmcp.setPrivate("nobody")
    assert.is_nil(ok)
    assert.is_true(contains(err, NO_SUCH))
  end)

  it("serve returns nil for an unknown target", function()
    local ok, err = mmcp.serve("nobody")
    assert.is_nil(ok)
    assert.is_true(contains(err, NO_SUCH))
  end)

  it("snoop returns nil for an unknown target", function()
    local ok, err = mmcp.snoop("nobody")
    assert.is_nil(ok)
    assert.is_true(contains(err, NO_SUCH))
  end)

  it("allowSnoop returns nil for an unknown target", function()
    local ok, err = mmcp.allowSnoop("nobody")
    assert.is_nil(ok)
    assert.is_true(contains(err, NO_SUCH))
  end)

  it("setGroup returns nil for an unknown target", function()
    local ok, err = mmcp.setGroup("nobody", "team")
    assert.is_nil(ok)
    assert.is_true(contains(err, NO_SUCH))
  end)

  it("disconnect returns nil for an unknown target", function()
    local ok, err = mmcp.disconnect("nobody")
    assert.is_nil(ok)
    assert.is_true(contains(err, NO_SUCH))
  end)

  it("ignore returns nil for an unknown target", function()
    local ok, err = mmcp.ignore("nobody")
    assert.is_nil(ok)
    assert.is_true(contains(err, NO_SUCH))
  end)

  it("getClientList returns nil when there are no peers", function()
    assert.is_nil(mmcp.getClientList())
  end)

  it("chatTo requires a target argument", function()
    assert.has_error(function() mmcp.chatTo() end)
  end)

  it("chatAll requires a message argument", function()
    assert.has_error(function() mmcp.chatAll() end)
  end)

  describe("mmcp.call", function()
    it("raises a Lua error when the host is missing", function()
      assert.has_error(function() mmcp.call() end)
    end)

    it("rejects an out-of-range port without connecting", function()
      local ok, err = mmcp.call("127.0.0.1", 70000)
      assert.is_nil(ok)
      assert.is_true(contains(err, "invalid port number"))
    end)
  end)

  describe("mmcp.chatName", function()
    it("returns the current chat name as a string", function()
      assert.is_string(mmcp.chatName())
    end)

    it("rejects names containing a tilde or comma", function()
      local ok, err = mmcp.chatName("bad~name")
      assert.is_nil(ok)
      assert.is_true(contains(err, "tilde"))
    end)
  end)
end)

describe("Discord Lua API availability contract", function()
  -- Every rich-presence function is gated on the Discord API being available
  -- (the discord-rpc library loaded and Discord enabled for this profile). In
  -- CI the library is not on the load path, so each gated function is denied
  -- with the same stable reason. On a machine where Discord is live these
  -- pend instead of mutating real presence data.
  local gatedFunctions = {
    "usingMudletsDiscordID", "getDiscordDetail", "getDiscordLargeIcon",
    "getDiscordLargeIconText", "getDiscordParty", "getDiscordSmallIcon",
    "getDiscordSmallIconText", "getDiscordState", "getDiscordTimeStamps",
    "resetDiscordData", "setDiscordApplicationID", "setDiscordDetail",
    "setDiscordElapsedStartTime", "setDiscordGame", "setDiscordLargeIcon",
    "setDiscordLargeIconText", "setDiscordParty", "setDiscordRemainingEndTime",
    "setDiscordSmallIcon", "setDiscordSmallIconText", "setDiscordState",
  }

  -- Probe with a read-access getter. When it returns nil+message the API is
  -- denied and that message is the shared denial reason; otherwise the API is
  -- usable in this environment and the contract tests pend.
  local function discordDenial()
    local ok, msg = getDiscordState()
    if ok == nil and type(msg) == "string" then
      return msg
    end
    return nil
  end

  it("the denial reason refers to Discord", function()
    local denial = discordDenial()
    if not denial then
      pending("Discord API is enabled in this environment")
      return
    end
    assert.is_true(contains(denial, "Discord"))
  end)

  for _, fnName in ipairs(gatedFunctions) do
    it(fnName .. " returns the shared denial while the API is unavailable", function()
      local denial = discordDenial()
      if not denial then
        pending("Discord API is enabled in this environment")
        return
      end
      -- Called with no arguments: the availability gate is checked before any
      -- argument, so nothing is read or mutated on the denied path.
      local ok, msg = _G[fnName]()
      assert.is_nil(ok)
      assert.equals(denial, msg)
    end)
  end

  describe("setDiscordGameUrl (intentionally ungated)", function()
    -- setDiscordGameUrl changes the profile's invite button, not rich
    -- presence, so it has no availability gate. Only its argument type is a
    -- deterministic offline contract; the success path is left to effect tests
    -- as it mutates profile state.
    it("raises a Lua error when the url argument is not a string", function()
      assertArgError(function() setDiscordGameUrl({}) end, "setDiscordGameUrl: bad argument")
    end)
  end)
end)
