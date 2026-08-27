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
-- The download, HTTP and MMCP families are the exception: their infrastructure
-- can be stood up locally, so their real effects are checked against the
-- fixture server in CI/http-fixture-server.py (ephemeral port in
-- MUDLET_TEST_HTTP_PORT) and the scripted chat peer in CI/mmcp-peer.py
-- (handover directory in MUDLET_TEST_MMCP_DIR). Both skip cleanly when absent
-- so the suite still passes without them. Nothing here mocks a real API
-- function.

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
  -- which only happens inside waitForEvent() and pumpEvents(), so arming the
  -- wait after issuing the request cannot miss the reply.
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

-- The MMCP specs above check the no-peer contracts. These drive the real
-- protocol against the scripted peer in CI/mmcp-peer.py: it accepts the call
-- mmcp.call() places, records the bytes Mudlet sends and sends chat traffic
-- back when a spec asks it to. Nothing is mocked - each assertion is either a
-- byte the peer received, an event Mudlet raised in response to real socket
-- traffic, or a value read back through the mmcp API.
--
-- The peer's port is ephemeral (MMCP's default 4050 would collide between CI
-- jobs and parallel worktrees) and is handed over through the directory named
-- by MUDLET_TEST_MMCP_DIR. Without a peer these specs skip, unless
-- MUDLET_TEST_REQUIRE_MMCP_PEER is set, which is how CI turns a fixture that
-- failed to start into a failure rather than a green skip. Linux and macOS
-- start one; the Windows job does not, so the block pends there.
--
-- The specs share one connection and run in the order they are declared, after
-- the no-peer contracts above them. Anything that shuffles the suite would
-- need them made independent first.
describe("MMCP effects against a scripted chat peer", function()
  -- Both of these are what CI/mmcp-peer.py calls itself
  local PEER_NAME = "BustedPeer"
  local PEER_VERSION = "Mudlet 0.0.0-busted-peer"
  local CHAT_NAME = "MudletBustedTester"
  local mmcpDir = os.getenv("MUDLET_TEST_MMCP_DIR")
  local peerRequired = os.getenv("MUDLET_TEST_REQUIRE_MMCP_PEER")
  local commandCounter = 0
  local originalChatName

  local function readFile(path)
    local handle = io.open(path, "r")
    if not handle then
      return nil
    end
    local contents = handle:read("*a")
    handle:close()
    return contents
  end

  -- The peer writes its port only once it is accepting, so a readable port file
  -- means the fixture is up.
  local function peerPort()
    if not mmcpDir then
      return nil
    end
    local raw = readFile(mmcpDir .. "/port")
    return raw and tonumber(raw:match("%d+"))
  end

  -- Returns true when the caller should stop because the fixture cannot be
  -- talked to and skipping is allowed.
  local function peerUnavailable()
    local reason
    if not peerPort() then
      reason = "MMCP peer fixture not running (run CI/mmcp-peer.py with MUDLET_TEST_MMCP_DIR set)"
    elseif type(yajl) ~= "table" then
      -- yajl is loaded as an optional module, and both channels to the peer are
      -- JSON, so say so rather than dying on a nil index further down.
      reason = "the yajl Lua module is unavailable, so the peer's JSON channels cannot be used"
    else
      return false
    end
    if peerRequired then
      assert.is_true(false, "MUDLET_TEST_REQUIRE_MMCP_PEER is set but " .. reason .. " (MUDLET_TEST_MMCP_DIR=" .. tostring(mmcpDir) .. ")")
    end
    pending(reason)
    return true
  end

  local function pump(ms)
    pumpEvents(ms)
  end

  local function waitUntil(predicate, timeoutMs)
    local step = 20
    for _ = 1, math.ceil((timeoutMs or 2000) / step) do
      if predicate() then
        return true
      end
      pump(step)
    end
    return predicate()
  end

  local function capture()
    local raw = readFile(mmcpDir .. "/capture.json")
    if not raw or raw == "" then
      return nil
    end
    local ok, decoded = pcall(yajl.to_value, raw)
    if not ok then
      return nil
    end
    return decoded
  end

  -- How far the peer's history has got, so a spec can disregard what earlier
  -- specs left behind and look only at what its own action produced. An
  -- unreadable capture would silently widen that to the whole history, so it
  -- fails here instead.
  local function captureSeq()
    local decoded = capture()
    assert.is_table(decoded)
    assert.is_number(decoded.seq)
    return decoded.seq
  end

  local function waitForPeerEvent(afterSeq, matches, timeoutMs)
    local found
    waitUntil(function()
      local decoded = capture()
      found = nil
      for _, event in ipairs(decoded and decoded.events or {}) do
        if event.seq > afterSeq and matches(event) then
          found = event
          break
        end
      end
      return found ~= nil
    end, timeoutMs)
    return found
  end

  -- Waits for a protocol command of this name to reach the peer and returns it,
  -- or nil if none arrived in time.
  local function waitForCommand(name, afterSeq, timeoutMs)
    return waitForPeerEvent(afterSeq, function(event)
      return event.type == "command" and event.name == name
    end, timeoutMs)
  end

  -- Instructs the peer. Written as "<n>.json.tmp" and renamed into place so the
  -- peer never picks up a half-written command.
  local function tellPeer(command)
    commandCounter = commandCounter + 1
    local path = mmcpDir .. "/commands/" .. commandCounter .. ".json"
    local handle = assert(io.open(path .. ".tmp", "w"))
    handle:write(yajl.to_string(command))
    handle:close()
    assert(os.rename(path .. ".tmp", path))
  end

  local function peerSends(code, text)
    tellPeer({action = "send", code = code, text = text})
  end

  -- The command channel is JSON, so bytes that are not valid UTF-8 - the 0xff
  -- terminator above all - have to travel as hex.
  local function peerSendsRaw(bytes)
    tellPeer({action = "send_hex", hex = (bytes:gsub(".", function(char)
      return string.format("%02x", char:byte())
    end))})
  end

  local function peerClient()
    local clients = mmcp.getClientList()
    if type(clients) ~= "table" then
      return nil
    end
    for _, client in ipairs(clients) do
      if client.name == PEER_NAME then
        return client
      end
    end
    return nil
  end

  -- Set once the peer has failed to answer a call. Every spec calls ensurePeer,
  -- so without this a peer that died mid-run would cost each of them the full
  -- handshake wait and blow the workflow's one-minute cap before busted could
  -- report anything.
  local peerNotAnswering

  -- Places a call to the fixture peer unless one is already up, and returns the
  -- peer's entry in mmcp.getClientList().
  local function ensurePeer()
    local client = peerClient()
    if client then
      return client
    end
    if peerNotAnswering then
      assert.is_true(false, peerNotAnswering)
    end
    -- A peer under some other name is one an earlier spec renamed and did not
    -- rename back; drop it so this call is not refused as a duplicate.
    local stale = mmcp.getClientList()
    if type(stale) == "table" then
      for _, entry in ipairs(stale) do
        mmcp.disconnect(entry.name)
      end
      waitUntil(function() return mmcp.getClientList() == nil end, 2000)
    end
    originalChatName = originalChatName or mmcp.chatName()
    -- A fixed name, so the bytes the peer records are predictable.
    mmcp.chatName(CHAT_NAME)
    assert.is_true(mmcp.call("127.0.0.1", peerPort()))
    -- A peer joins the client list only once it has accepted the call, and that
    -- is what raises sysMMCPPeerUpdateEvent.
    if waitForEvent("sysMMCPPeerUpdateEvent", 3000) ~= "sysMMCPPeerUpdateEvent" then
      peerNotAnswering = "the MMCP peer fixture never answered a call on port " .. tostring(peerPort())
      assert.is_true(false, peerNotAnswering)
    end
    client = peerClient()
    assert.is_table(client)
    return client
  end

  -- Runs action with a handler armed for eventName and returns the argument
  -- lists it saw. Events Mudlet raises inside an mmcp.* call are raised before
  -- that call returns, so they have to be watched for, not waited on.
  local function collectEvents(eventName, action)
    local seen = {}
    local handlerId = registerAnonymousEventHandler(eventName, function(_, ...)
      seen[#seen + 1] = {...}
    end)
    local ok, err = pcall(action)
    killAnonymousEventHandler(handlerId)
    if not ok then
      error(err, 0)
    end
    return seen
  end

  -- Every spec below leaves the peer's flags as it found them, so this does
  -- nothing on a passing run. It matters when one does fail: an assertion that
  -- stops a spec halfway through a toggle would otherwise leave the peer
  -- ignored or private and take the specs after it down as well.
  after_each(function()
    if not peerClient() then
      return
    end
    local flags = mmcp.getClientFlags(PEER_NAME)
    if type(flags) ~= "string" then
      return
    end
    if flags:sub(3, 3) ~= " " then mmcp.setPrivate(PEER_NAME) end
    if flags:sub(4, 4) ~= " " then mmcp.ignore(PEER_NAME) end
    if flags:sub(5, 5) ~= " " then mmcp.serve(PEER_NAME) end
    if flags:sub(7, 7) ~= " " then mmcp.allowSnoop(PEER_NAME) end
  end)

  describe("mmcp.call", function()
    it("completes the MudMaster handshake with the peer", function()
      if peerUnavailable() then return end
      ensurePeer()
      local decoded = capture()
      assert.is_table(decoded.caller)
      -- "CHAT:<name>\n<address><port left aligned in 5 columns>", asserted
      -- whole: the port's padding is part of the format a MudMaster peer reads
      -- back, and only the exact string keeps it honest.
      local port = tostring(peerPort())
      local padded = port .. string.rep(" ", math.max(0, 5 - #port))
      assert.equals("CHAT:" .. CHAT_NAME .. "\n127.0.0.1" .. padded, decoded.caller.raw)
    end)

    it("announces itself as Mudlet once the call is accepted", function()
      if peerUnavailable() then return end
      ensurePeer()
      local sent = waitForPeerEvent(0, function(event)
        return event.type == "command" and event.name == "Version"
      end, 2000)
      assert.is_table(sent)
      -- Peers switch behaviour on this string - a Mudlet peer only forwards side
      -- channel data to versions saying "Mudlet", and picks the snoop colour
      -- format from "MudMaster" - so the prefix is load-bearing, not cosmetic.
      assert.equals("Mudlet ", sent.text:sub(1, 7))
    end)

    it("lists the accepted peer with its address, port and version", function()
      if peerUnavailable() then return end
      local client = ensurePeer()
      assert.equals(1, client.id)
      assert.equals(PEER_NAME, client.name)
      assert.equals("127.0.0.1", client.host)
      assert.equals(peerPort(), client.port)
      -- The peer's version arrives just after its acceptance, which is what
      -- releases ensurePeer, so give it its own wait rather than assuming the
      -- two landed in the same read.
      assert.is_true(waitUntil(function()
        local entry = peerClient()
        return entry ~= nil and entry.version == PEER_VERSION
      end, 2000), tostring(peerClient() and peerClient().version))
    end)

    it("refuses to place a second call to a peer it is already talking to", function()
      if peerUnavailable() then return end
      ensurePeer()
      local before = capture().connections
      local ok, err = mmcp.call("127.0.0.1", peerPort())
      assert.is_nil(ok)
      assert.is_true(contains(err, "already connected to that client"))
      pump(200)
      assert.equals(before, capture().connections)
    end)

    it("leaves no client behind when nothing answers the port", function()
      if peerUnavailable() then return end
      ensurePeer()
      -- Port 1 on loopback refuses rather than listens. The call is placed
      -- (that much is asynchronous), but the client it creates has to be
      -- disposed of on the error rather than lingering in the session.
      assert.is_true(mmcp.call("127.0.0.1", 1))
      pump(500)
      assert.equals(1, #mmcp.getClientList())
      assert.is_table(peerClient())
    end)

    it("leaves no client behind when the peer refuses the call", function()
      if peerUnavailable() then return end
      if peerClient() then
        mmcp.disconnect(PEER_NAME)
        waitUntil(function() return peerClient() == nil end, 2000)
      end
      tellPeer({action = "accept", accept = false})
      waitUntil(function()
        local decoded = capture()
        return decoded ~= nil and decoded.accepting == false
      end, 1000)

      local mark = captureSeq()
      assert.is_true(mmcp.call("127.0.0.1", peerPort()))
      -- The peer answers "NO:<name>" and hangs up, so the call never reaches
      -- the connected state and nothing is added to the client list.
      assert.is_table(waitForPeerEvent(mark, function(event)
        return event.type == "handshake"
      end, 2000))
      pump(300)
      assert.is_nil(mmcp.getClientList())

      tellPeer({action = "accept", accept = true})
      waitUntil(function()
        local decoded = capture()
        return decoded ~= nil and decoded.accepting == true
      end, 1000)
    end)
  end)

  describe("outgoing chat", function()
    it("chatAll sends the message to the peer and echoes it locally", function()
      if peerUnavailable() then return end
      ensurePeer()
      local mark = captureSeq()
      local echoes = collectEvents("sysMMCPChatMessage", function()
        assert.is_true(mmcp.chatAll("hello everyone"))
      end)
      local sent = waitForCommand("TextEveryone", mark)
      assert.is_table(sent)
      assert.equals(CHAT_NAME .. " chats to everybody, 'hello everyone'\n", sent.text)
      -- One echo, attributed to "System" because it was addressed to no-one in
      -- particular.
      assert.equals(1, #echoes)
      assert.equals("System", echoes[1][1])
      assert.is_true(contains(echoes[1][2], "You chat to everybody, 'hello everyone'"), tostring(echoes[1][2]))
    end)

    it("chatTo sends a personal message to the named peer", function()
      if peerUnavailable() then return end
      ensurePeer()
      local mark = captureSeq()
      local echoes = collectEvents("sysMMCPChatMessage", function()
        assert.is_true(mmcp.chatTo(PEER_NAME, "just for you"))
      end)
      local sent = waitForCommand("TextPersonal", mark)
      assert.is_table(sent)
      assert.equals(CHAT_NAME .. " chats to you, 'just for you'\n", sent.text)
      -- Unlike chatAll's echo this one is attributed to the peer it was
      -- addressed to, not to "System", even though we are the ones speaking.
      assert.equals(1, #echoes)
      assert.equals(PEER_NAME, echoes[1][1])
      assert.is_true(contains(echoes[1][2], "You chat to " .. PEER_NAME .. ", 'just for you'"), tostring(echoes[1][2]))
    end)

    it("emoteAll sends an unquoted emote to everyone", function()
      if peerUnavailable() then return end
      ensurePeer()
      local mark = captureSeq()
      local echoes = collectEvents("sysMMCPChatMessage", function()
        assert.is_true(mmcp.emoteAll("waves at the room"))
      end)
      local sent = waitForCommand("TextEveryone", mark)
      assert.is_table(sent)
      assert.equals(CHAT_NAME .. " waves at the room\n", sent.text)
      assert.equals(1, #echoes)
      assert.equals("System", echoes[1][1])
      -- The profile default leaves emotes unprefixed, so the echo is the bare
      -- emote and not the "You emote to everyone: '...'" wording.
      assert.is_true(contains(echoes[1][2], CHAT_NAME .. " waves at the room"), tostring(echoes[1][2]))
      assert.is_false(contains(echoes[1][2], "You emote to everyone"), tostring(echoes[1][2]))
    end)
  end)

  describe("mmcp.setGroup and mmcp.chatGroup", function()
    it("reports an empty group and sends nothing until a peer is assigned", function()
      if peerUnavailable() then return end
      ensurePeer()
      local mark = captureSeq()
      local ok, err = mmcp.chatGroup("testers", "nobody there")
      assert.is_nil(ok)
      assert.is_true(contains(err, "nobody in group 'testers' now"))
      assert.is_nil(waitForCommand("TextGroup", mark, 500))
    end)

    it("reaches the peer once it has been assigned to the group", function()
      if peerUnavailable() then return end
      ensurePeer()
      assert.is_true(mmcp.setGroup(PEER_NAME, "testers"))
      local mark = captureSeq()
      assert.is_true(mmcp.chatGroup("testers", "group hello"))
      local sent = waitForCommand("TextGroup", mark)
      assert.is_table(sent)
      -- MudMaster's group field is a fixed 15 characters wide
      assert.equals("testers        ", sent.text:sub(1, 15))
      assert.is_true(contains(sent.text, " chats to the group, 'group hello'"), sent.text)
    end)

    it("reads back a group chat of its own making", function()
      if peerUnavailable() then return end
      ensurePeer()
      assert.is_true(mmcp.setGroup(PEER_NAME, "testers"))
      local mark = captureSeq()
      assert.is_true(mmcp.chatGroup("testers", "round trip"))
      local sent = waitForCommand("TextGroup", mark)
      assert.is_table(sent)
      -- Hand Mudlet's own bytes straight back: sender and parser have to agree
      -- about where the 15 character group field ends, or a Mudlet peer would
      -- render another Mudlet's group chat wrongly.
      local received = collectEvents("sysMMCPChatMessage", function()
        peerSendsRaw(string.char(6) .. sent.text .. string.char(255))
        pump(500)
      end)
      assert.equals(1, #received)
      assert.equals(PEER_NAME, received[1][1])
      assert.is_true(contains(received[1][2], "(testers)"), tostring(received[1][2]))
      assert.is_true(contains(received[1][2], "'round trip'"), tostring(received[1][2]))
    end)

    it("stops reaching the peer once it is removed from the group", function()
      if peerUnavailable() then return end
      ensurePeer()
      assert.is_true(mmcp.setGroup(PEER_NAME, "none"))
      local mark = captureSeq()
      local ok, err = mmcp.chatGroup("testers", "still there?")
      assert.is_nil(ok)
      assert.is_true(contains(err, "nobody in group 'testers' now"))
      assert.is_nil(waitForCommand("TextGroup", mark, 500))
    end)
  end)

  describe("per-peer flags", function()
    -- getClientFlags returns a fixed 8 character field: two spaces, then
    -- Private, Ignored, Served, Firewalled, the snoop state and a trailing
    -- space.
    it("are all clear while nothing has been toggled", function()
      if peerUnavailable() then return end
      ensurePeer()
      assert.equals("        ", mmcp.getClientFlags(PEER_NAME))
    end)

    it("setPrivate toggles the P flag", function()
      if peerUnavailable() then return end
      ensurePeer()
      assert.is_true(mmcp.setPrivate(PEER_NAME))
      assert.equals("  P     ", mmcp.getClientFlags(PEER_NAME))
      assert.is_true(mmcp.setPrivate(PEER_NAME))
      assert.equals("        ", mmcp.getClientFlags(PEER_NAME))
    end)

    it("ignore toggles the I flag", function()
      if peerUnavailable() then return end
      ensurePeer()
      assert.is_true(mmcp.ignore(PEER_NAME))
      assert.equals("   I    ", mmcp.getClientFlags(PEER_NAME))
      assert.is_true(mmcp.ignore(PEER_NAME))
      assert.equals("        ", mmcp.getClientFlags(PEER_NAME))
    end)

    it("serve toggles the S flag and tells the peer both times", function()
      if peerUnavailable() then return end
      ensurePeer()
      local mark = captureSeq()
      assert.is_true(mmcp.serve(PEER_NAME))
      assert.equals("    S   ", mmcp.getClientFlags(PEER_NAME))
      local told = waitForCommand("Message", mark)
      assert.is_table(told)
      assert.equals("<CHAT> You are now being served by " .. CHAT_NAME .. ".", told.text)

      mark = captureSeq()
      assert.is_true(mmcp.serve(PEER_NAME))
      assert.equals("        ", mmcp.getClientFlags(PEER_NAME))
      told = waitForCommand("Message", mark)
      assert.is_table(told)
      assert.equals("<CHAT> You are no longer being served by " .. CHAT_NAME .. ".", told.text)
    end)

    it("allowSnoop toggles the n flag and tells the peer both times", function()
      if peerUnavailable() then return end
      ensurePeer()
      local mark = captureSeq()
      assert.is_true(mmcp.allowSnoop(PEER_NAME))
      assert.equals("      n ", mmcp.getClientFlags(PEER_NAME))
      local told = waitForCommand("Message", mark)
      assert.is_table(told)
      assert.equals("<CHAT> You are now allowed to snoop " .. CHAT_NAME .. ".", told.text)

      mark = captureSeq()
      assert.is_true(mmcp.allowSnoop(PEER_NAME))
      assert.equals("        ", mmcp.getClientFlags(PEER_NAME))
      told = waitForCommand("Message", mark)
      assert.is_table(told)
      assert.equals("<CHAT> You are no longer allowed to snoop " .. CHAT_NAME .. ".", told.text)
    end)
  end)

  describe("incoming chat", function()
    it("raises sysMMCPChatMessage for a chat to everyone", function()
      if peerUnavailable() then return end
      ensurePeer()
      peerSends(4, PEER_NAME .. " chats to everybody, 'peer speaking'\n")
      local name, from, message = waitForEvent("sysMMCPChatMessage", 2000)
      assert.equals("sysMMCPChatMessage", name)
      assert.equals(PEER_NAME, from)
      assert.is_true(contains(message, PEER_NAME .. " chats to everybody, 'peer speaking'"), tostring(message))
    end)

    it("raises sysMMCPChatMessage for a personal chat", function()
      if peerUnavailable() then return end
      ensurePeer()
      peerSends(5, PEER_NAME .. " chats to you, 'just between us'\n")
      local _, from, message = waitForEvent("sysMMCPChatMessage", 2000)
      assert.equals(PEER_NAME, from)
      assert.is_true(contains(message, "chats to you, 'just between us'"), tostring(message))
    end)

    it("names the group an incoming group chat arrived on", function()
      if peerUnavailable() then return end
      ensurePeer()
      -- MudMaster's format: a 15 character group field, then the message.
      -- Mudlet's own sender adds a newline after that field, which the
      -- round-trip spec above covers.
      peerSends(6, "testers        " .. PEER_NAME .. " chats to the group, 'group inbound'")
      local _, from, message = waitForEvent("sysMMCPChatMessage", 2000)
      assert.equals(PEER_NAME, from)
      assert.is_true(contains(message, "(testers)"), tostring(message))
      assert.is_true(contains(message, "'group inbound'"), tostring(message))
    end)

    it("displays a plain protocol message from the peer", function()
      if peerUnavailable() then return end
      ensurePeer()
      peerSends(7, "<CHAT> the peer has something to say")
      local _, from, message = waitForEvent("sysMMCPChatMessage", 2000)
      assert.equals(PEER_NAME, from)
      assert.is_true(contains(message, "the peer has something to say"), tostring(message))
    end)

    it("waits for the rest of a command that arrives in two pieces", function()
      if peerUnavailable() then return end
      ensurePeer()
      -- Commands are only complete at their 0xff terminator, and TCP is free to
      -- deliver one in as many reads as it likes. Mudlet has to hold the first
      -- half rather than displaying a truncated line or dropping it.
      peerSendsRaw(string.char(4) .. PEER_NAME .. " chats to everybody, 'split ")
      pump(150)
      peerSendsRaw("message'\n" .. string.char(255))
      local _, from, message = waitForEvent("sysMMCPChatMessage", 2000)
      assert.equals(PEER_NAME, from)
      assert.is_true(contains(message, "'split message'"), tostring(message))
    end)

    it("handles two commands that arrive in a single write", function()
      if peerUnavailable() then return end
      ensurePeer()
      -- The parser walks the buffer command by command, so a write carrying
      -- two of them has to produce two messages rather than one or none.
      local received = collectEvents("sysMMCPChatMessage", function()
        peerSendsRaw(string.char(7) .. "<CHAT> first of two" .. string.char(255)
                     .. string.char(7) .. "<CHAT> second of two" .. string.char(255))
        pump(500)
      end)
      assert.equals(2, #received)
      assert.is_true(contains(received[1][2], "first of two"), tostring(received[1][2]))
      assert.is_true(contains(received[2][2], "second of two"), tostring(received[2][2]))
    end)

    it("skips a command it does not know without losing the next one", function()
      if peerUnavailable() then return end
      ensurePeer()
      -- An unknown command byte must be skipped up to its terminator; consuming
      -- the wrong number of bytes would swallow whatever followed it.
      local received = collectEvents("sysMMCPChatMessage", function()
        peerSendsRaw(string.char(99) .. "nonsense" .. string.char(255)
                     .. string.char(7) .. "<CHAT> after the unknown" .. string.char(255))
        pump(500)
      end)
      assert.equals(1, #received)
      assert.is_true(contains(received[1][2], "after the unknown"), tostring(received[1][2]))
    end)

    it("drops chat from an ignored peer and resumes when un-ignored", function()
      if peerUnavailable() then return end
      ensurePeer()
      assert.is_true(mmcp.ignore(PEER_NAME))
      peerSends(4, PEER_NAME .. " chats to everybody, 'ignored line'\n")
      assert.is_nil(waitForEvent("sysMMCPChatMessage", 500))

      assert.is_true(mmcp.ignore(PEER_NAME))
      peerSends(4, PEER_NAME .. " chats to everybody, 'heard line'\n")
      local _, _, message = waitForEvent("sysMMCPChatMessage", 2000)
      assert.is_true(contains(message, "'heard line'"), tostring(message))
    end)
  end)

  describe("connection lists a peer sends", function()
    -- A connection list makes Mudlet dial the addresses in it, so this is the
    -- one incoming command that has a peer reaching outside the session. The
    -- fixture's second port answers nothing and hangs up, which is enough to
    -- record that Mudlet dialled it.
    local function dialPort()
      return capture().dial_port
    end

    local function dialled(afterSeq, timeoutMs)
      return waitForPeerEvent(afterSeq, function(event)
        return event.type == "dialled"
      end, timeoutMs)
    end

    it("dials an address the peer hands over", function()
      if peerUnavailable() then return end
      ensurePeer()
      local mark = captureSeq()
      peerSends(3, "127.0.0.1," .. dialPort())
      assert.is_table(dialled(mark, 2000))
      -- Nothing answered, so no peer joined the session over it.
      pump(300)
      assert.is_table(peerClient())
      assert.equals(1, #mmcp.getClientList())
    end)

    it("dials nothing when the list has a host without a port", function()
      if peerUnavailable() then return end
      ensurePeer()
      local mark = captureSeq()
      -- An odd number of fields is rejected as badly formatted rather than
      -- being half-parsed into a connection attempt.
      peerSends(3, "127.0.0.1," .. dialPort() .. ",127.0.0.1")
      assert.is_nil(dialled(mark, 600))
    end)
  end)

  describe("mmcp.sendSideChannel", function()
    it("sends channel and message to the peer as one comma separated payload", function()
      if peerUnavailable() then return end
      ensurePeer()
      local mark = captureSeq()
      assert.is_true(mmcp.sendSideChannel("TestChannel", "payload here"))
      local sent = waitForCommand("SideChannel", mark)
      assert.is_table(sent)
      assert.equals("TestChannel,payload here", sent.text)
    end)

    it("raises sysMMCPSideChannelMessage for incoming side channel data", function()
      if peerUnavailable() then return end
      ensurePeer()
      peerSends(40, "TestChannel,inbound payload")
      local name, from, channel, message = waitForEvent("sysMMCPSideChannelMessage", 2000)
      assert.equals("sysMMCPSideChannelMessage", name)
      assert.equals(PEER_NAME, from)
      assert.equals("TestChannel", channel)
      assert.equals("inbound payload", message)
    end)
  end)

  describe("mmcp.snoop", function()
    it("asks the peer for a snoop feed", function()
      if peerUnavailable() then return end
      ensurePeer()
      local mark = captureSeq()
      assert.is_true(mmcp.snoop(PEER_NAME))
      local sent = waitForCommand("Snoop", mark)
      assert.is_table(sent)
      assert.equals("", sent.text)
      -- Asking a second time is what stops a snoop, since the command is a
      -- toggle at the far end. That is not asserted here: the local "am I
      -- snooping them" flag is never set (nothing calls setSnooped(true)), so
      -- MMCPServer::snoop's stop branch cannot be reached and asserting either
      -- way would freeze the defect in place.
    end)

    it("refuses a snoop from a peer that has not been allowed one", function()
      if peerUnavailable() then return end
      ensurePeer()
      local mark = captureSeq()
      -- An incoming Snoop with the n flag clear: the peer is told no, and never
      -- starts receiving what the game sends us.
      peerSends(30, "")
      local told = waitForCommand("Message", mark)
      assert.is_table(told)
      assert.equals("<CHAT> You do not have permission to snoop " .. CHAT_NAME .. ".", told.text)
      assert.equals("        ", mmcp.getClientFlags(PEER_NAME))
    end)

    it("starts and stops snooping for a peer that has been allowed one", function()
      if peerUnavailable() then return end
      ensurePeer()
      local mark = captureSeq()
      assert.is_true(mmcp.allowSnoop(PEER_NAME))
      -- Granting permission sends a Message of its own; let it land before
      -- marking, so what is waited for below cannot be that one.
      assert.is_table(waitForCommand("Message", mark))

      mark = captureSeq()
      peerSends(30, "")
      local told = waitForCommand("Message", mark)
      assert.is_table(told)
      assert.equals("<CHAT> You have begun snooping " .. CHAT_NAME .. ".", told.text)
      -- N, not n: the peer is snooping us now rather than merely permitted to.
      assert.equals("      N ", mmcp.getClientFlags(PEER_NAME))

      mark = captureSeq()
      peerSends(30, "")
      told = waitForCommand("Message", mark)
      assert.is_table(told)
      assert.equals("<CHAT> You have stopped snooping " .. CHAT_NAME .. ".", told.text)
      assert.equals("      n ", mmcp.getClientFlags(PEER_NAME))
      assert.is_true(mmcp.allowSnoop(PEER_NAME))
    end)

    it("raises sysMMCPIncomingSnoopMessage for snooped output", function()
      if peerUnavailable() then return end
      ensurePeer()
      peerSends(31, "You see a snooped line of game output")
      local name, from, message = waitForEvent("sysMMCPIncomingSnoopMessage", 2000)
      assert.equals("sysMMCPIncomingSnoopMessage", name)
      assert.equals(PEER_NAME, from)
      assert.is_true(contains(message, "a snooped line of game output"), tostring(message))
    end)

    it("keeps the colour of a snooped line", function()
      if peerUnavailable() then return end
      ensurePeer()
      -- Snoop data is where the other end's colour arrives, and Mudlet tracks
      -- it across lines, so the escape sequences have to survive into the event
      -- rather than being stripped or reordered away from their text.
      peerSendsRaw(string.char(31) .. "\27[1;32ma green snooped line\27[0m" .. string.char(255))
      local _, _, message = waitForEvent("sysMMCPIncomingSnoopMessage", 2000)
      assert.is_true(contains(message, "\27[1;32ma green snooped line"), tostring(message))
    end)
  end)

  describe("mmcp.ping", function()
    it("sends a timestamped ping the peer can answer", function()
      if peerUnavailable() then return end
      ensurePeer()
      local mark = captureSeq()
      assert.is_true(mmcp.ping(PEER_NAME))
      local sent = waitForCommand("PingRequest", mark)
      assert.is_table(sent)
      -- the payload is milliseconds since the epoch, which is what comes back
      assert.is_number(tonumber(sent.text))
    end)

    it("answers an incoming ping with the same payload", function()
      if peerUnavailable() then return end
      ensurePeer()
      local mark = captureSeq()
      peerSends(26, "1234567890123")
      local answered = waitForCommand("PingResponse", mark)
      assert.is_table(answered)
      assert.equals("1234567890123", answered.text)
    end)
  end)

  describe("mmcp.chatName", function()
    it("announces a new name to connected peers and reads it back", function()
      if peerUnavailable() then return end
      ensurePeer()
      local mark = captureSeq()
      assert.is_true(mmcp.chatName("RenamedTester"))
      local sent = waitForCommand("NameChange", mark)
      assert.is_table(sent)
      assert.equals("RenamedTester", sent.text)
      assert.equals("RenamedTester", mmcp.chatName())

      mark = captureSeq()
      assert.is_true(mmcp.chatName(CHAT_NAME))
      local restored = waitForCommand("NameChange", mark)
      assert.is_table(restored)
      assert.equals(CHAT_NAME, restored.text)
    end)

    it("does not announce a name that has not changed", function()
      if peerUnavailable() then return end
      ensurePeer()
      assert.is_true(mmcp.chatName(CHAT_NAME))
      local mark = captureSeq()
      assert.is_true(mmcp.chatName(CHAT_NAME))
      assert.is_nil(waitForCommand("NameChange", mark, 500))
    end)

    it("does not announce a rejected name", function()
      if peerUnavailable() then return end
      ensurePeer()
      local mark = captureSeq()
      local ok, err = mmcp.chatName("bad,name")
      assert.is_nil(ok)
      assert.is_true(contains(err, "comma"))
      assert.is_nil(waitForCommand("NameChange", mark, 500))
      assert.equals(CHAT_NAME, mmcp.chatName())
    end)

    it("follows the peer when it renames itself", function()
      if peerUnavailable() then return end
      ensurePeer()
      peerSends(1, "RenamedPeer")
      assert.is_true(waitUntil(function()
        local clients = mmcp.getClientList()
        return type(clients) == "table" and clients[1] ~= nil and clients[1].name == "RenamedPeer"
      end, 2000))
      -- and the new name is what addresses it from then on
      assert.is_true(mmcp.chatTo("RenamedPeer", "hello again"))

      peerSends(1, PEER_NAME)
      assert.is_true(waitUntil(function() return peerClient() ~= nil end, 2000))
    end)
  end)

  describe("mmcp.displayClientList", function()
    it("prints the connected peer with its address and port", function()
      if peerUnavailable() then return end
      ensurePeer()
      local printed = collectEvents("sysMMCPChatMessage", function()
        assert.is_true(mmcp.displayClientList())
      end)
      -- The whole table goes out as one message, attributed to nobody in
      -- particular.
      assert.equals(1, #printed)
      assert.equals("System", printed[1][1])
      local text = printed[1][2]
      assert.is_true(contains(text, PEER_NAME), text)
      assert.is_true(contains(text, "127.0.0.1"), text)
      assert.is_true(contains(text, tostring(peerPort())), text)
    end)
  end)

  describe("mmcp.accept and mmcp.deny", function()
    -- No peer needed: this is about what the mmcp table contains.
    it("are not reachable from Lua, so incoming calls cannot be covered", function()
      -- Mudlet's pending-call notice tells the user to run mmcp.accept(id) or
      -- mmcp.deny(id), but neither is in the mmcp table: their registration in
      -- TLuaInterpreter.cpp is commented out, along with setDoNotDisturb,
      -- startServer, stopServer, request and peek. Without startServer Mudlet
      -- cannot listen either, so no incoming call can be staged here at all.
      -- Left pending rather than asserted so the gap is not locked in place -
      -- but registering them has to be noticed, hence the failure below.
      if mmcp.accept ~= nil or mmcp.deny ~= nil then
        assert.is_true(false, "mmcp.accept/mmcp.deny are registered now - replace this spec with real accept and deny coverage")
      end
      pending("mmcp.accept/mmcp.deny are not registered in the Lua mmcp table")
    end)
  end)

  describe("disconnection", function()
    it("notices when the peer closes the connection", function()
      if peerUnavailable() then return end
      ensurePeer()
      tellPeer({action = "close"})
      assert.equals("sysMMCPPeerUpdateEvent", waitForEvent("sysMMCPPeerUpdateEvent", 2000))
      assert.is_nil(peerClient())
    end)

    it("mmcp.disconnect closes the connection from this end", function()
      if peerUnavailable() then return end
      ensurePeer()
      local mark = captureSeq()
      assert.is_true(mmcp.disconnect(PEER_NAME))
      assert.equals("sysMMCPPeerUpdateEvent", waitForEvent("sysMMCPPeerUpdateEvent", 2000))
      assert.is_nil(mmcp.getClientList())
      assert.is_table(waitForPeerEvent(mark, function(event)
        return event.type == "disconnect"
      end, 2000))
      assert.is_false(capture().connected)
    end)

  end)

  -- Restores whatever chat name the profile was carrying before these specs
  -- ran, so nothing that follows sees a name this file chose.
  teardown(function()
    if originalChatName then
      mmcp.chatName(originalChatName)
    end
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

describe("The IRC configuration functions round-trip through the profile", function()
  -- While a profile has no IRC dialog - none of these specs opens one - the
  -- getters read the profile's own configuration off disk, which is what the
  -- setters write to. So the round trip is testable with no IRC server and no
  -- connection anywhere in sight.
  --
  -- The profile's own IRC configuration is put back afterwards, because the
  -- self-test profile is reused between runs. One thing the restore cannot
  -- reach, which matters to a developer running the suite against a config root
  -- that is not a throwaway one: the last-used nick, which setIrcNick() also
  -- writes to a file shared by every profile (mudlet's data directory, not the
  -- profile's). Putting the profile's nick back writes that file again rather
  -- than restoring it. The password is left alone by every call below that does
  -- not pass one, and the specs that do pass one put the profile's own back
  -- through setConfig().
  local function restoreIrcConfiguration()
    local nick = getIrcNick()
    local hostName, port, secure = getIrcServer()
    local channels = getIrcChannels()
    finally(function()
      setIrcNick(nick)
      setIrcServer(hostName, port, secure)
      setIrcChannels(channels)
    end)
  end

  -- setIrcServer takes a password but no getter of its own reads one back, so
  -- the specs for it go through the configuration option the same file is
  -- behind: getConfig("ircPassword") and setConfig("ircPassword", ...).
  local function restoreIrcConfigurationWithPassword()
    local nick = getIrcNick()
    local hostName, port, secure = getIrcServer()
    local channels = getIrcChannels()
    local password = getConfig("ircPassword")
    finally(function()
      setIrcNick(nick)
      setIrcServer(hostName, port, secure)
      setIrcChannels(channels)
      setConfig("ircPassword", password)
    end)
  end

  describe("getIrcNick, getIrcServer and getIrcChannels", function()
    it("report a nick, a server and a channel list without an IRC client", function()
      -- with nothing configured each getter falls back to a built-in default
      -- rather than to nil, which is what makes them safe to read before
      -- anything has been set
      local nick = getIrcNick()
      assert.is_string(nick)
      assert.is_true(#nick > 0)

      local hostName, port, secure = getIrcServer()
      assert.is_string(hostName)
      assert.is_true(#hostName > 0)
      assert.is_number(port)
      assert.is_true(port >= 1 and port <= 65535, tostring(port))
      assert.is_boolean(secure)

      local channels = getIrcChannels()
      assert.is_table(channels)
      assert.is_true(#channels > 0)
      for _, channel in ipairs(channels) do
        assert.is_string(channel)
      end
    end)
  end)

  describe("setIrcNick", function()
    it("raises a Lua error when the nick is missing or not a string", function()
      assertArgError(function() setIrcNick() end, "setIrcNick: bad argument #1 type (nick as string expected")
      assertArgError(function() setIrcNick({}) end, "setIrcNick: bad argument #1 type (nick as string expected, got table!)")
    end)

    it("returns nil and a message for an empty nick, leaving the stored one alone", function()
      restoreIrcConfiguration()
      assert.is_true(setIrcNick("BustedKeptNick"))

      local ok, err = setIrcNick("")
      assert.is_nil(ok)
      assert.is_true(contains(err, "nick must not be empty"), tostring(err))
      assert.equals("BustedKeptNick", getIrcNick())
    end)

    it("stores the nick where getIrcNick reads it back", function()
      restoreIrcConfiguration()
      assert.is_true(setIrcNick("BustedNickOne"))
      assert.equals("BustedNickOne", getIrcNick())

      assert.is_true(setIrcNick("BustedNickTwo"))
      assert.equals("BustedNickTwo", getIrcNick())
    end)
  end)

  describe("setIrcServer", function()
    it("raises a Lua error when the hostname or an optional argument is wrongly typed", function()
      assertArgError(function() setIrcServer() end, "setIrcServer: bad argument #1 type (hostname as string expected")
      assertArgError(function() setIrcServer({}) end, "setIrcServer: bad argument #1 type (hostname as string expected, got table!)")
      assertArgError(function() setIrcServer("irc.busted.invalid", {}) end, "port number")
      assertArgError(function() setIrcServer("irc.busted.invalid", 6667, "yes") end, "secure")
      assertArgError(function() setIrcServer("irc.busted.invalid", 6667, false, {}) end, "server password")
    end)

    it("returns nil and a message for an empty hostname or an out-of-range port", function()
      restoreIrcConfiguration()
      assert.is_true(setIrcServer("irc.busted-kept.invalid", 6690))

      local ok, err = setIrcServer("")
      assert.is_nil(ok)
      assert.is_true(contains(err, "hostname must not be empty"), tostring(err))

      ok, err = setIrcServer("irc.busted.invalid", 70000)
      assert.is_nil(ok)
      assert.is_true(contains(err, "invalid port number 70000"), tostring(err))

      ok, err = setIrcServer("irc.busted.invalid", 0)
      assert.is_nil(ok)
      assert.is_true(contains(err, "invalid port number 0"), tostring(err))

      -- a refused call stored nothing
      local hostName, port = getIrcServer()
      assert.equals("irc.busted-kept.invalid", hostName)
      assert.equals(6690, port)
    end)

    it("stores the hostname, port and secure flag where getIrcServer reads them back", function()
      restoreIrcConfiguration()
      -- it reports success as true plus a nil second value
      local ok, extra = setIrcServer("irc.busted-one.invalid", 6697, true)
      assert.is_true(ok)
      assert.is_nil(extra)

      local hostName, port, secure = getIrcServer()
      assert.equals("irc.busted-one.invalid", hostName)
      assert.equals(6697, port)
      assert.is_true(secure)

      -- the secure flag is stored, not merely defaulted: turn it back off
      assert.is_true(setIrcServer("irc.busted-two.invalid", 6668, false))
      hostName, port, secure = getIrcServer()
      assert.equals("irc.busted-two.invalid", hostName)
      assert.equals(6668, port)
      assert.is_false(secure)
    end)

    it("leaves the stored password alone when it is not passed", function()
      -- #9786: every call rewrote the password, so a script that changed the
      -- host or the port destroyed a credential it was never given.
      restoreIrcConfigurationWithPassword()

      assert.is_true(setIrcServer("irc.busted-password.invalid", 6667, false, "BustedSecret"))
      assert.equals("BustedSecret", getConfig("ircPassword"))

      -- the same server on another port, with the password argument left off
      assert.is_true(setIrcServer("irc.busted-password.invalid", 6668))
      assert.equals(6668, select(2, getIrcServer()))
      assert.equals("BustedSecret", getConfig("ircPassword"))

      -- and it is kept across a change of server too, because an argument that
      -- was not passed says nothing about the credential
      assert.is_true(setIrcServer("irc.busted-other.invalid", 6667))
      assert.equals("irc.busted-other.invalid", (getIrcServer()))
      assert.equals("BustedSecret", getConfig("ircPassword"))

      -- an empty string is how a script asks for the password to go
      assert.is_true(setIrcServer("irc.busted-other.invalid", 6667, false, ""))
      assert.equals("", getConfig("ircPassword"))
    end)

    it("falls back to port 6667 and an insecure connection when only a hostname is given", function()
      restoreIrcConfiguration()
      assert.is_true(setIrcServer("irc.busted-secure.invalid", 6697, true))

      assert.is_true(setIrcServer("irc.busted-default.invalid"))
      local hostName, port, secure = getIrcServer()
      assert.equals("irc.busted-default.invalid", hostName)
      assert.equals(6667, port)
      assert.is_false(secure)
    end)

    it("takes an explicit nil for any optional argument, as leaving it off does", function()
      -- #9787: a script forwarding optional variables passes nil for the ones
      -- it has no value for, and only the port accepted that
      restoreIrcConfigurationWithPassword()

      assert.is_true(setIrcServer("irc.busted-nil.invalid", 6697, true, "BustedNilSecret"))
      assert.equals("BustedNilSecret", getConfig("ircPassword"))

      assert.is_true(setIrcServer("irc.busted-nil.invalid", nil, nil, nil))
      local storedHost, storedPort, storedSecure = getIrcServer()
      assert.equals("irc.busted-nil.invalid", storedHost)
      -- a nil optional is the default, the same as leaving it off
      assert.equals(6667, storedPort)
      assert.is_false(storedSecure)
      -- except for the password, which a nil argument keeps as it is
      assert.equals("BustedNilSecret", getConfig("ircPassword"))
    end)
  end)

  describe("setIrcChannels", function()
    it("raises a Lua error when the channels are not a table", function()
      assertArgError(function() setIrcChannels("#mudlet") end, "setIrcChannels: bad argument #1 type (channels as table expected, got string!)")
      assertArgError(function() setIrcChannels() end, "setIrcChannels: bad argument #1 type (channels as table expected, got no value!)")
    end)

    it("returns nil and a message when no entry is a usable channel name", function()
      restoreIrcConfiguration()
      assert.is_true(setIrcChannels({"#busted-kept"}))

      local ok, err = setIrcChannels({})
      assert.is_nil(ok)
      assert.is_true(contains(err, "no (valid) channel names provided"), tostring(err))

      -- a channel name has to start with #, & or +, and only strings are read
      ok, err = setIrcChannels({"mudlet", 42, ""})
      assert.is_nil(ok)
      assert.is_true(contains(err, "no (valid) channel names provided"), tostring(err))
      assert.same({"#busted-kept"}, getIrcChannels())
    end)

    it("stores the channel list where getIrcChannels reads it back", function()
      restoreIrcConfiguration()
      assert.is_true(setIrcChannels({"#busted-one", "&busted-two", "+busted-three"}))
      assert.same({"#busted-one", "&busted-two", "+busted-three"}, getIrcChannels())
    end)

    it("keeps the usable channel names out of a mixed list and drops the rest", function()
      restoreIrcConfiguration()
      assert.is_true(setIrcChannels({"#busted-good", "busted-bad", "&busted-also-good"}))
      assert.same({"#busted-good", "&busted-also-good"}, getIrcChannels())
    end)

    it("drops a channel name carrying a space or a comma rather than storing two", function()
      -- #9789: the stored list is space-joined and the JOIN command is
      -- comma-joined, so a name holding either came back as two channels. An
      -- IRC channel name can hold neither, which puts them with the other
      -- unusable names above: dropped, and refused outright when nothing is
      -- left.
      restoreIrcConfiguration()
      assert.is_true(setIrcChannels({"#busted-spaced one", "#busted-plain"}))
      assert.same({"#busted-plain"}, getIrcChannels())

      assert.is_true(setIrcChannels({"#busted-a,#busted-b", "#busted-tabbed\tname", "#busted-plain-two"}))
      assert.same({"#busted-plain-two"}, getIrcChannels())

      local ok, err = setIrcChannels({"#busted only spaced"})
      assert.is_nil(ok)
      assert.is_true(contains(err, "no (valid) channel names provided"), tostring(err))
      assert.same({"#busted-plain-two"}, getIrcChannels())
    end)
  end)

  describe("getIrcConnectedHost and restartIrc without a client", function()
    -- Both of these read whether the profile has an IRC dialog, and nothing in
    -- the suite creates one - see the openIRC spec below for why. Should
    -- something start doing so, these are where it shows up first.
    --
    -- Which is also why only the failure half of getIrcConnectedHost's pair is
    -- checked here, arity included: the other half needs a client connected far
    -- enough for the server's RPL_YOURHOST, which is where #9788 was - it pushed
    -- true and the host name but returned only one of them.
    it("getIrcConnectedHost returns false and says there is no client", function()
      local ok, err = getIrcConnectedHost()
      assert.is_false(ok)
      assert.equals("no client active", err)
      assert.equals(2, select("#", getIrcConnectedHost()))
    end)

    it("restartIrc returns false", function()
      -- there is no client to restart, and it says so by returning false
      -- rather than by opening one
      assert.is_false(restartIrc(), "something in this run opened an IRC client")
    end)
  end)

  describe("sendIrc", function()
    -- Both arguments are checked before the IRC dialog would be created, so
    -- these calls open no client. A well-formed sendIrc() does create one,
    -- which is why there is no spec here for the delivery path.
    it("raises a Lua error when the target or the message is missing or wrongly typed", function()
      assertArgError(function() sendIrc() end, "sendIrc: bad argument #1 type (target as string expected")
      assertArgError(function() sendIrc("#mudlet") end, "sendIrc: bad argument #2 type (message as string expected")
      assertArgError(function() sendIrc({}, "hello") end, "sendIrc: bad argument #1 type (target as string expected, got table!)")
      assertArgError(function() sendIrc("#mudlet", {}) end, "sendIrc: bad argument #2 type (message as string expected, got table!)")
    end)
  end)

  describe("openIRC", function()
    it("opens the IRC client window", function()
      pending("openIRC creates the profile's IRC dialog and nothing in the Lua API closes it again. "
        .. "From then on the getters answer out of the copy the dialog read when it was constructed - "
        .. "a setIrcNick() while it is open is not seen by getIrcNick() until restartIrc() - so the "
        .. "round trips above would stop working for the rest of the run, and the dialog dials the "
        .. "configured server and raises a window over the specs that follow")
    end)
  end)
end)

describe("getNetworkLatency", function()
  it("reports zero on a profile whose game socket has never been timed", function()
    -- The latency is measured between a command going out and the game's reply
    -- being read, and nothing in the suite connects the game socket - so the
    -- untouched value is what this reads, which is also what pins it to the
    -- right member. A meaningful reading needs a game server.
    local latency = getNetworkLatency()
    assert.is_number(latency)
    assert.equals(0, latency)
  end)
end)
