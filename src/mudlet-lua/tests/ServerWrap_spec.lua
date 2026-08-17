-- Host::mUndoServerWrap: rejoining of lines that the game server hard-wrapped
-- itself, so that the buffer holds whole logical lines. The join only runs on
-- server data, which feedTelnet() delivers - that needs the profile offline,
-- see the tests README.

describe("Tests undoing the game's own line wrapping", function()

  -- 70 characters, inside the join band for a wrap column of 80. That leaves 10
  -- columns free, so the fit check alone will not part it from a continuation
  -- opening with more than one character - unlike the lines built by
  -- heldLine(), which have to survive a one character continuation:
  local segment1 = string.rep("x", 64) .. " alpha"
  local segment2 = "beta tail."

  local savedWrap, savedUndo, savedWidth, mark

  local function feed(data)
    local ok, msg = feedTelnet(data)
    assert.is_true(ok, "start the suite with --offline, see the tests README - feedTelnet said: " .. tostring(msg))
  end

  -- setConfig answers nil rather than raising when a key it does not know or a
  -- width outside 20 to 500 is asked for, which would leave the join running at
  -- a setting the test never chose
  local function configure(key, value)
    assert.is_true(setConfig(key, value), "setConfig(" .. key .. ") was refused")
  end

  -- a line held back for a continuation is committed by a flush timer
  -- csmServerWrapFlushDelayMs (300ms) after the game goes quiet, so pump until
  -- that has fallen due and the buffer has stopped growing rather than for a
  -- fixed time a loaded machine could outrun
  local function settle()
    local previous, waited = -1, 0
    while waited < 3000 do
      assert.is_true(pumpEvents(100), "pumpEvents needs MUDLET_TEST_MODE set, see the tests README")
      waited = waited + 100
      local lastLine = getLastLineNumber("main")
      if lastLine == previous and waited >= 500 then
        return
      end
      previous = lastLine
    end
  end

  local function feedAndSettle(data)
    feed(data)
    settle()
  end

  -- whole buffer lines, compared exactly: a segment that was joined onto its
  -- neighbour must not still count as standing on its own. Only the lines this
  -- test wrote are looked at, so an earlier test's identical text cannot answer
  -- for one this one never produced.
  local function bufferHasLine(text)
    local last = getLastLineNumber("main")
    for _, line in ipairs(getLines("main", mark, last + 1)) do
      if line == text then
        return true
      end
    end
    return false
  end

  -- Only a line inside the join band - the wrap column less TBuffer's
  -- csmServerWrapSlack - is ever held back for a continuation. Lines that have
  -- to be held are padded to a fixed width inside it and checked, because one
  -- that drifted out would never be held, leaving the assertions that expect a
  -- line to stand on its own passing without the code under test having run. It
  -- also has to leave too little room for the continuation to have opened on
  -- it, even where that continuation opens with a single character, or the pair
  -- reads as two separate lines rather than as a wrap:
  local heldLineLength = 76

  local function heldLine(text)
    return text .. " " .. string.rep("x", heldLineLength - #text - 1)
  end

  local function verifyHeldLines(lines)
    for _, line in ipairs(lines) do
      assert.are.equal(heldLineLength, #line,
        "test line is " .. #line .. " characters, not the " .. heldLineLength .. " that put it inside the join band")
    end
  end

  -- Same guard for the cases that use text from a real game verbatim at that
  -- game's own wrap column instead of padding to a fixed length. The band floor
  -- repeats csmServerWrapSlack, which is private to TBuffer:
  local function verifyInJoinBand(lines, width)
    local floor = width - 15
    for _, line in ipairs(lines) do
      assert.is_true(#line >= floor and #line <= width,
        "test line is " .. #line .. " characters, outside the join band of " .. floor .. " to " .. width)
    end
  end

  setup(function()
    savedWrap = getWindowWrap("main")
    savedUndo = getConfig("undoServerWrap")
    savedWidth = getConfig("undoServerWrapWidth")
    -- keep Mudlet's own display wrap out of the way so that logical lines can
    -- be compared with buffer lines verbatim
    setWindowWrap("main", 500)
  end)

  teardown(function()
    setConfig("undoServerWrap", savedUndo)
    setConfig("undoServerWrapWidth", savedWidth)
    setWindowWrap("main", savedWrap)
  end)

  before_each(function()
    configure("undoServerWrap", true)
    configure("undoServerWrapWidth", 80)
    mark = getLastLineNumber("main")
  end)

  after_each(function()
    -- a line still held back would be committed into the next test's window. A
    -- blank line is not a wrap continuation, so it commits held text on the
    -- spot and needs no waiting for the flush timer.
    feed("\r\n")
    configure("undoServerWrap", false)
  end)

  it("leaves a wrapped line and its continuation apart by default", function()
    -- savedUndo is read before the before_each above ever turns the option on,
    -- so it is the setting a profile comes up with
    assert.is_false(savedUndo, "undoing the game's own wrapping must be off unless a profile asks for it")
    configure("undoServerWrap", false)
    feedAndSettle(segment1 .. "\r\n" .. segment2 .. "\r\n")

    assert.is_true(bufferHasLine(segment1), "wrapped segment was not committed as its own line with the option off")
    assert.is_true(bufferHasLine(segment2), "continuation was not committed as its own line with the option off")
    assert.is_false(bufferHasLine(segment1 .. " " .. segment2), "lines were joined although the option is off")
  end)

  it("joins a wrapped line onto its continuation", function()
    feedAndSettle(segment1 .. "\r\n" .. segment2 .. "\r\n")

    assert.is_true(bufferHasLine(segment1 .. " " .. segment2), "wrapped segment and its continuation were not joined into one logical line")
  end)

  it("does not swallow a prompt into the full-width line above it", function()
    -- the prompt is terminated by IAC GA rather than a newline
    feedAndSettle(segment1 .. "\r\nHP:100> <T_IAC><T_GA>")

    assert.is_true(bufferHasLine(segment1), "full-width final line was not committed on its own when followed by a prompt")
    assert.is_true(bufferHasLine("HP:100> "), "prompt was not committed on its own")
  end)

  it("flushes a lone full-width line once the game goes quiet", function()
    -- nothing follows, so the held line has to be committed by the flush timer
    feed(segment1 .. "\r\n")
    assert.is_false(bufferHasLine(segment1), "the full-width line was not held back for a continuation")

    settle()
    assert.is_true(bufferHasLine(segment1), "held full-width line was not flushed after the game went quiet")
  end)

  it("ends a paragraph at a blank line", function()
    feedAndSettle(segment1 .. "\r\n\r\n" .. segment2 .. "\r\n")

    assert.is_true(bufferHasLine(segment1), "full-width line before a blank line was not committed on its own")
    assert.is_true(bufferHasLine(segment2), "line after a blank line was not committed on its own")
    assert.is_false(bufferHasLine(segment1 .. " " .. segment2), "lines were joined across a blank line")
  end)

  it("leaves art and indented lines alone", function()
    -- a full-width divider, a full-width prose line followed by an indented
    -- line (menu/centered art), and only then real wrapped prose - only the
    -- last pair may be joined
    local divider = string.rep("-", 70)
    local indented = "   [1] Enter the game"
    feedAndSettle(divider .. "\r\n" .. segment1 .. "\r\n" .. indented .. "\r\n" .. segment1 .. "\r\n" .. segment2 .. "\r\n")

    assert.is_true(bufferHasLine(divider), "full-width divider was not committed on its own")
    assert.is_true(bufferHasLine(indented), "indented line was joined although it cannot be a wrap continuation")
    assert.is_true(bufferHasLine(segment1), "full-width line before an indented line was not committed on its own")
    assert.is_true(bufferHasLine(segment1 .. " " .. segment2), "genuine wrapped prose was no longer joined")
  end)

  it("joins both styles of kept break space into a single space", function()
    -- some games keep the space they broke the line at - either at the end of
    -- the wrapped line or at the start of the continuation. Either way the
    -- rejoined line carries exactly one space:
    feedAndSettle(segment1 .. " \r\nbeta trailing.\r\n")
    assert.is_true(bufferHasLine(segment1 .. " beta trailing."), "trailing-space wrap style was not joined into one line")

    feedAndSettle(segment1 .. "\r\n gamma leading.\r\n")
    assert.is_true(bufferHasLine(segment1 .. " gamma leading."), "leading-space wrap style was not joined into one line")
  end)

  it("leaves a padded line alone", function()
    -- a line space-padded out to the wrap column is a table row or a colour
    -- fill, not a wrapped segment - word wrap never produces a run of trailing
    -- spaces
    local padded = "2 - visit the game" .. string.rep(" ", 60)
    feedAndSettle(padded .. "\r\n" .. segment2 .. "\r\n")

    assert.is_true(bufferHasLine(padded), "padded line was not committed on its own")
    assert.is_true(bufferHasLine(segment2), "line after a padded line was not committed on its own")
  end)

  it("joins a wrap that lands on a sentence gap", function()
    -- games that put two spaces after a full stop keep both when the wrap point
    -- lands right after a sentence - neither a held line nor a continuation
    -- ending in ".  " is padding:
    local sentenceGap = string.rep("x", 62) .. " alpha.  "
    feedAndSettle(sentenceGap .. "\r\n" .. segment2 .. "\r\n")
    assert.is_true(bufferHasLine(sentenceGap .. segment2), "line ending in a sentence gap was mistaken for padding and not joined")

    feedAndSettle(segment1 .. "\r\nbeta done.  \r\n")
    assert.is_true(bufferHasLine(segment1 .. " beta done.  "), "continuation ending in a sentence gap was not joined onto the held line")

    -- three or more trailing spaces are still padding, sentence or not
    local sentencePadded = string.rep("x", 62) .. " alpha." .. string.rep(" ", 5)
    feedAndSettle(sentencePadded .. "\r\n" .. segment2 .. "\r\n")
    assert.is_true(bufferHasLine(sentencePadded), "sentence-final line padded with several spaces was not committed on its own")
  end)

  it("leaves list entries alone", function()
    -- only the marker tells these entries from a wrapped paragraph; the last
    -- carries the single leading space a game may indent an index by
    local entry1 = heldLine("[581] Stat Fury - a viking only stat that grants bonuses")
    local entry2 = heldLine("(3) Viking Default: what you get if you do not customise")
    local entry3 = heldLine("1. Viking Specializations lists the class specialisations")
    local entry4 = " [1366] Vikings: a barbaric fighter class."
    verifyHeldLines({entry1, entry2, entry3})
    feedAndSettle(entry1 .. "\r\n" .. entry2 .. "\r\n" .. entry3 .. "\r\n" .. entry4 .. "\r\n")

    assert.is_true(bufferHasLine(entry1), "bracketed list entry was joined onto the entry below it")
    assert.is_true(bufferHasLine(entry2), "parenthesised list entry was joined onto the entry below it")
    assert.is_true(bufferHasLine(entry3), "numbered list entry was joined onto the entry below it")
    assert.is_true(bufferHasLine(entry4), "indented list entry was joined onto the entry above it")
  end)

  it("still joins a list entry that wrapped", function()
    -- the marker is looked for on the continuation only, so an entry too long
    -- for one line still wraps like any other prose
    local entry = heldLine("[1364] Viking Default: if you chose not to customise your")
    verifyHeldLines({entry})
    feedAndSettle(entry .. "\r\nviking this is what is included.\r\n")

    assert.is_true(bufferHasLine(entry .. " viking this is what is included."), "a wrapped list entry was no longer joined back together")
  end)

  it("does not mistake prose for a list", function()
    -- every one of these continuations opens with something a list marker could
    -- be mistaken for
    local dash = heldLine("The Grand Bazaar sells everything you could want in this")
    local aside = heldLine("You gain a large amount of experience for your daring")
    local price = heldLine("The merchant paid for the whole shipment in advance, all")
    local reference = heldLine("More detail about the viking class can be found over")
    verifyHeldLines({dash, aside, price, reference})

    feedAndSettle(dash .. "\r\n- weapons, armour and rope - at a very fair price.\r\n")
    assert.is_true(bufferHasLine(dash .. " - weapons, armour and rope - at a very fair price."), "a spaced dash opening a continuation was mistaken for a bullet")

    feedAndSettle(aside .. "\r\n(2500) and the whole town cheers for you.\r\n")
    assert.is_true(bufferHasLine(aside .. " (2500) and the whole town cheers for you."), "a parenthesised number too long to be a label was mistaken for one")

    feedAndSettle(price .. "\r\n1364. gold was a fair price for it.\r\n")
    assert.is_true(bufferHasLine(price .. " 1364. gold was a fair price for it."), "a number too long to be a list label was mistaken for one")

    feedAndSettle(reference .. "\r\n(see help vikings) for the full list.\r\n")
    assert.is_true(bufferHasLine(reference .. " (see help vikings) for the full list."), "a parenthesised phrase carrying no number was mistaken for a list marker")
  end)

  it("does not join two complete sentences", function()
    configure("undoServerWrapWidth", 78)
    -- two lines of MorgenGrauen's appraisal, each a whole sentence. The width is
    -- that game's, so that the first line lands inside the join band and is
    -- held; it stops 13 columns short, and "Die" would have fitted after it, so
    -- the game broke the line rather than its wrap:
    local verdict = "Der Logiker haelt mindestens fuenf Mal so viel aus, wie ein Hund!"
    local armour = "Die Ruestung des Logikers ist besser als Deine."
    verifyInJoinBand({verdict}, 78)
    feedAndSettle(verdict .. "\r\n" .. armour .. "\r\n")

    assert.is_true(bufferHasLine(verdict), "sentence with room to spare left on it was not committed on its own")
    assert.is_true(bufferHasLine(armour), "sentence following one that had room for its first word was not committed on its own")
    assert.is_false(bufferHasLine(verdict .. " " .. armour), "two whole sentences the game sent separately were joined into one line")
  end)

  it("still joins hand-wrapped prose", function()
    configure("undoServerWrapWidth", 78)
    -- two room descriptions as MorgenGrauen sends them. Lines laid out by hand
    -- stop several columns short of the wrap column, so a continuation's first
    -- word can look like one that would have fitted: "sich" would have ended 5
    -- columns clear of the column, and only csmServerWrapFitTolerance keeps that
    -- paragraph joined:
    local hall = {"Der grosse Raum mit seiner niedrigen Decke und den grob geschnittenen",
                  "Querbalken hat Platz fuer sehr viele Personen. Knarrende Dielen erzaehlen",
                  "ueber Heldentaten laengst vergessener Abenteurer, beruehmter als mancher",
                  "Weise unserer Zeit."}
    local visitors = {"Abenteurer, aber auch andere Bewohner dieser Welt kommen hierher, um",
                      "sich zu informieren, ihre Erfahrungen auszutauschen oder sich in den",
                      "verschiedensten Wissenschaften zu verbessern."}
    verifyInJoinBand({hall[1], hall[2], hall[3], visitors[1], visitors[2]}, 78)

    feedAndSettle(table.concat(hall, "\r\n") .. "\r\n")
    assert.is_true(bufferHasLine(table.concat(hall, " ")), "a hand-wrapped paragraph was no longer joined back into one line")

    feedAndSettle(table.concat(visitors, "\r\n") .. "\r\n")
    assert.is_true(bufferHasLine(table.concat(visitors, " ")), "a paragraph broken well short of the wrap column was no longer joined")
  end)

  it("turns the join on where the continuation's first word would have ended", function()
    configure("undoServerWrapWidth", 78)
    -- one character of held line either side of the decision: with 66 the
    -- continuation's "one" would have ended on column 70, a full
    -- csmServerWrapFitTolerance clear of the column, and with 67 it would have
    -- ended one column later than the tolerance allows:
    local roomToSpare = string.rep("x", 60) .. " alpha"
    local noRoom = string.rep("x", 61) .. " alpha"
    local continuation = "one more word."
    verifyInJoinBand({roomToSpare, noRoom}, 78)

    feedAndSettle(roomToSpare .. "\r\n" .. continuation .. "\r\n")
    assert.is_true(bufferHasLine(roomToSpare), "a line the continuation's first word would have fitted on was not committed on its own")
    assert.is_false(bufferHasLine(roomToSpare .. " " .. continuation), "a line with room to spare for the continuation's first word was joined anyway")

    feedAndSettle(noRoom .. "\r\n" .. continuation .. "\r\n")
    assert.is_true(bufferHasLine(noRoom .. " " .. continuation), "a line one character too long for the continuation's first word was not joined")
  end)

  it("counts kept break spaces towards the fit", function()
    configure("undoServerWrapWidth", 78)
    -- the word would have gone where the join puts it, so the spaces the game
    -- left at the break count towards it. Both of these hold 66 characters of
    -- text - one short of joining on their own - and only join because a second
    -- space sits between them and the continuation:
    local sentenceGap = string.rep("x", 59) .. " alpha.  "
    local spacedBothSides = string.rep("x", 60) .. " alpha "
    local continuation = "one more word."
    verifyInJoinBand({sentenceGap, spacedBothSides}, 78)

    feedAndSettle(sentenceGap .. "\r\n" .. continuation .. "\r\n")
    assert.is_true(bufferHasLine(sentenceGap .. continuation), "the two spaces a game leaves after a sentence were not counted towards where the next word would have gone")

    feedAndSettle(spacedBothSides .. "\r\n " .. continuation .. "\r\n")
    assert.is_true(bufferHasLine(spacedBothSides .. " " .. continuation), "a break space kept on both lines was not counted towards where the next word would have gone")
  end)

  it("measures a joined paragraph by the game line it ends on", function()
    configure("undoServerWrapWidth", 78)
    -- once a paragraph has been joined the held text is longer than the wrap
    -- column, so the game line it ends on is what the next word has to be
    -- measured against - "Die" would have fitted on the appraisal but not on the
    -- whole two lines held by then:
    local opening = "Der Logiker mustert Dich abschaetzend und schnaubt dann veraechtlich"
    local verdict = "Der Logiker haelt mindestens fuenf Mal so viel aus, wie ein Hund!"
    local armour = "Die Ruestung des Logikers ist besser als Deine."
    verifyInJoinBand({opening, verdict}, 78)
    feedAndSettle(opening .. "\r\n" .. verdict .. "\r\n" .. armour .. "\r\n")

    assert.is_true(bufferHasLine(opening .. " " .. verdict), "the wrapped opening was not joined onto the line that continues it")
    assert.is_true(bufferHasLine(armour), "a whole sentence after a joined paragraph was not committed on its own")
    assert.is_false(bufferHasLine(opening .. " " .. verdict .. " " .. armour), "a whole sentence was glued onto a paragraph its first word would have fitted the last line of")
  end)
end)
