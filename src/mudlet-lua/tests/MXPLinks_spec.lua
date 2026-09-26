-- A link a game makes with <SEND> or <A> is kept as a line of Lua that runs
-- when the user clicks it, with the command or address the game gave written
-- into it as a string. Whatever the game puts there - in an attribute or as the
-- text the tag wraps - has to stay inside that string, or a game could run any
-- Lua it liked in the user's profile.
--
-- Each action is run here with nothing but a recorder in reach, so a payload
-- that did break out could only record the names it reached for.

describe("Tests the Lua an MXP link runs", function()
  local probeElement = "mxpLinksSpecProbe"

  local function feed(data)
    feedTriggers(data .. "\n")
  end

  -- the actions of the most recent link, as a plain element that follows the
  -- tags reports them
  local function actionsAfter(data)
    feed(data)
    if type(mxp) == "table" then
      mxp[probeElement:lower()] = nil
    end
    feed(("<%s>probe</%s>"):format(probeElement, probeElement))
    return mxp and mxp[probeElement:lower()] and mxp[probeElement:lower()].actions
  end

  -- every global the action reached for, and what it called each one with
  local function run(action)
    local chunk, problem = loadstring(action)
    assert.is_function(chunk, ("%s\n%s"):format(tostring(problem), action))
    local calls = {}
    setfenv(chunk, setmetatable({}, {
      __index = function(_, name)
        calls[#calls + 1] = {name}
        local call = calls[#calls]
        return function(...)
          call[2] = {...}
        end
      end
    }))
    local ran, err = pcall(chunk)
    assert.is_true(ran, ("%s\n%s"):format(tostring(err), action))
    return calls
  end

  -- each action runs exactly the one call expected of it, in order
  local function assertRunsEach(actions, expected)
    assert.is_table(actions)
    assert.are.equal(#expected, #actions, table.concat(actions, "\n----\n"))
    for i, call in ipairs(expected) do
      assert.are.same({{call[1], {call[2]}}}, run(actions[i]))
    end
  end

  local function assertRunsOnly(actions, name, argument)
    assertRunsEach(actions, {{name, argument}})
  end

  -- text written to end a [[ ]] string, or one with any level of = signs, or
  -- to merge with the closing bracket, or to open a nested one
  local payloads = {
    [===[x]]..os.exit()..[[]===],
    [===[x]=]..os.exit()..[=[]===],
    [===[x]==]..os.exit()..[==[]===],
    [===[x")..os.exit()..("]===],
    [===[x]]]===],
    [===[x]]===],
    [===[x[[y]===],
    [===[x[=[y]=]z]===],
  }

  -- an attribute quoted with whichever quote the payload leaves free
  local function attribute(name, value)
    local quote = value:find('"', 1, true) and "'" or '"'
    return ("%s=%s%s%s"):format(name, quote, value, quote)
  end

  setup(function()
    setConfig("specialForceMXPProcessorOn", true)
    feed(("<!ELEMENT %s FLAG='%s'>"):format(probeElement, probeElement))
    -- elements a game defines put its values into the same links
    feed([[<!ELEMENT mxpLinksShop '<SEND href="buy &name;|sell &name;" hint="menu|buy|sell">' ATT='name'>]])
    feed([[<!ELEMENT mxpLinksTell '<SEND href="tell &who; " PROMPT>' ATT='who'>]])
    feed([[<!ELEMENT mxpLinksItem '<SEND href="buy &text;">'>]])
    feed([[<!ELEMENT mxpLinksWiki '<A href="https://wiki.example/&text;">'>]])
  end)

  teardown(function()
    setConfig("specialForceMXPProcessorOn", false)
  end)

  for _, payload in ipairs(payloads) do
    describe(("given %q"):format(payload), function()
      it("sends a SEND href as it was given", function()
        assertRunsOnly(actionsAfter(("<SEND %s>go</SEND>"):format(attribute("href", payload))), "send", payload)
      end)

      it("sends the text a bare SEND wraps as it was given", function()
        assertRunsOnly(actionsAfter(("<SEND>%s</SEND>"):format(payload)), "send", payload)
      end)

      it("fills &text; in a SEND href with the text as it was given", function()
        assertRunsOnly(actionsAfter(("<SEND href=\"say &text;\">%s</SEND>"):format(payload)), "send", "say " .. payload)
      end)

      it("puts a PROMPT SEND's text on the command line as it was given", function()
        assertRunsOnly(actionsAfter(("<SEND href=\"tell &text;\" PROMPT>%s</SEND>"):format(payload)), "printCmdLine", "tell " .. payload)
      end)

      it("hands the mxp.send event the same command the link runs", function()
        if type(mxp) == "table" then
          mxp.send = nil
        end
        feed(("<SEND>%s</SEND>"):format(payload))
        assertRunsOnly(mxp and mxp.send and mxp.send.actions, "send", payload)
      end)

      it("fills &text; into every command of a menu as it was given", function()
        assertRunsEach(actionsAfter(("<SEND href=\"look &text;|get &text;\" hint=\"menu|look|get\">%s</SEND>"):format(payload)), {
          {"send", "look " .. payload},
          {"send", "get " .. payload},
        })
      end)

      it("hands the mxp.send event the same menu the link runs", function()
        if type(mxp) == "table" then
          mxp.send = nil
        end
        feed(("<SEND href=\"look &text;|get &text;\" hint=\"menu|look|get\">%s</SEND>"):format(payload))
        assertRunsEach(mxp and mxp.send and mxp.send.actions, {
          {"send", "look " .. payload},
          {"send", "get " .. payload},
        })
      end)

      it("puts an element's attribute into every command of its menu as it was given", function()
        assertRunsEach(actionsAfter(("<mxpLinksShop %s>item</mxpLinksShop>"):format(attribute("name", payload))), {
          {"send", "buy " .. payload},
          {"send", "sell " .. payload},
        })
      end)

      it("puts an element's positional attribute on the command line as it was given", function()
        local quote = payload:find("'", 1, true) and '"' or "'"
        assertRunsOnly(actionsAfter(("<mxpLinksTell %s%s%s>who</mxpLinksTell>"):format(quote, payload, quote)), "printCmdLine", "tell " .. payload .. " ")
      end)

      it("fills &text; in an element's SEND with its text as it was given", function()
        assertRunsOnly(actionsAfter(("<mxpLinksItem>%s</mxpLinksItem>"):format(payload)), "send", "buy " .. payload)
      end)

      it("fills &text; in an element's A with its text as it was given", function()
        assertRunsOnly(actionsAfter(("<mxpLinksWiki>%s</mxpLinksWiki>"):format(payload)), "openUrl", "https://wiki.example/" .. payload)
      end)

      it("opens an A href as it was given", function()
        assertRunsOnly(actionsAfter(("<A %s>go</A>"):format(attribute("href", payload))), "openUrl", payload)
      end)

      it("opens the text a bare A wraps as it was given", function()
        assertRunsOnly(actionsAfter(("<A>%s</A>"):format(payload)), "openUrl", payload)
      end)
    end)
  end

  it("keeps every command of a plain menu, in order", function()
    assertRunsEach(actionsAfter([[<SEND href="who|look|say hi there" hint="Menu|W|L|S">x</SEND>]]), {
      {"send", "who"},
      {"send", "look"},
      {"send", "say hi there"},
    })
  end)

  -- a bare <A> takes its address from the text it wraps; none of that text may
  -- carry over into the links that come after it
  describe("Tests the address a bare A takes from its text", function()
    it("opens only its own text when another bare A came before it", function()
      feed("<A>https://first.example/</A>")
      assertRunsOnly(actionsAfter("<A>https://second.example/</A>"), "openUrl", "https://second.example/")
    end)

    it("fills &text; in an A href with only the text that A wraps", function()
      feed("<A>https://first.example/</A>")
      assertRunsOnly(actionsAfter([[<A href="https://wiki.example/&text;">Page</A>]]), "openUrl", "https://wiki.example/Page")
    end)

    it("leaves an A href with no &text; as it was after a bare A", function()
      feed("<A>https://first.example/</A>")
      assertRunsOnly(actionsAfter([[<A href="https://plain.example/">words</A>]]), "openUrl", "https://plain.example/")
    end)
  end)
end)
