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

  -- How many of this test's buffer lines carry the given text anywhere in them.
  -- Comparing whole lines cannot see text that a join duplicated rather than
  -- moved: every line the game sent still matches on its own while a chat prefix
  -- also sits in the middle of the paragraph below, on more lines than were sent.
  local function bufferLinesContaining(text)
    local last = getLastLineNumber("main")
    local found = 0
    for _, line in ipairs(getLines("main", mark, last + 1)) do
      if line:find(text, 1, true) then
        found = found + 1
      end
    end
    return found
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
    -- be compared with buffer lines verbatim - a joined room description runs
    -- to just over 500 characters
    setWindowWrap("main", 1000)
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
    -- held; its text stops 13 columns short before the kept break space, and
    -- "Die" would have fitted after it, so the game broke the line rather than
    -- its wrap. That break space is this fixture's, not the game's - MorgenGrauen
    -- swallows the one it breaks at - and without it the sentence the line ends
    -- on is refused a hold outright, leaving the fit check untried:
    local verdict = "Der Logiker haelt mindestens fuenf Mal so viel aus, wie ein Hund! "
    local armour = "Die Ruestung des Logikers ist besser als Deine."
    verifyInJoinBand({verdict}, 78)
    feedAndSettle(verdict .. "\r\n" .. armour .. "\r\n")

    assert.is_true(bufferHasLine(verdict), "sentence with room to spare left on it was not committed on its own")
    assert.is_true(bufferHasLine(armour), "sentence following one that had room for its first word was not committed on its own")
    assert.is_false(bufferHasLine(verdict .. armour), "two whole sentences the game sent separately were joined into one line")
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
    -- measured against - "Die" would have fitted on the appraisal, whose text
    -- stops 13 columns short before its kept break space, but not on the whole
    -- two lines held by then. That break space is this fixture's, not the game's:
    -- without it the sentence the appraisal ends on is refused a hold. Its
    -- opening word differs from the line above's so that the two do not read as
    -- one message the game re-prefixed:
    local opening = "Dieser Logiker mustert Dich abschaetzend und schnaubt dann veraechtlich"
    local verdict = "Der Logiker haelt mindestens fuenf Mal so viel aus, wie ein Hund! "
    local armour = "Die Ruestung des Logikers ist besser als Deine."
    verifyInJoinBand({opening, verdict}, 78)
    feedAndSettle(opening .. "\r\n" .. verdict .. "\r\n" .. armour .. "\r\n")

    assert.is_true(bufferHasLine(opening .. " " .. verdict), "the wrapped opening was not joined onto the line that continues it")
    assert.is_true(bufferHasLine(armour), "a whole sentence after a joined paragraph was not committed on its own")
    assert.is_false(bufferHasLine(opening .. " " .. verdict .. armour), "a whole sentence was glued onto a paragraph its first word would have fitted the last line of")
  end)

  -- The cases below carry MorgenGrauen's output verbatim, as a player reported
  -- it: that game wraps at 78 columns and swallows the space it broke at, so a
  -- line it ended itself is indistinguishable from a wrapped one by length
  -- alone.

  it("does not join a room's exits onto its description", function()
    configure("undoServerWrapWidth", 78)
    -- the description's last line ends a sentence right at the wrap column, and
    -- the exits line that follows is a line of its own - joining it on broke the
    -- reporter's trigger anchored to the start of it:
    local description = {"Du befindest Dich auf einem Trampelpfad, der sich in Ost-West-Richtung durch",
                         "den Wald schlaengelt. Im Westen fuehrt er auf eine Kreuzung, nach Osten",
                         "scheint es tiefer in den Wald zu gehen. Im Sueden siehst Du eine Lichtung."}
    local exits = "Es gibt drei sichtbare Ausgaenge: osten, westen und sueden."
    local object = "Das Haus von Bambel."
    -- including the sentence-final line, so that its refusal is what leaves the
    -- exits line alone rather than it having fallen out of the band
    verifyInJoinBand(description, 78)
    feedAndSettle(table.concat(description, "\r\n") .. "\r\n" .. exits .. "\r\n" .. object .. "\r\n")

    assert.is_true(bufferHasLine(table.concat(description, " ")), "the wrapped room description was not joined back into one line")
    assert.is_true(bufferHasLine(exits), "the exits line was joined onto the room description")
    assert.is_true(bufferHasLine(object), "the object line was not committed on its own")
  end)

  it("does not join a room's contents onto its exits", function()
    configure("undoServerWrapWidth", 78)
    -- here it is the exits line itself that ends a sentence at the wrap column,
    -- and the object below it was joined onto it. The description ends a
    -- sentence at the column too, so it is committed in the two halves the game
    -- broke at a sentence boundary rather than as one paragraph:
    local description = {"Auf den lichten Aesten der Eichen und Birken sitzen viele grosse und kleine",
                         "Voegel. Sie singen ihre Lieder und scheinen sich ueber Dich zu amuesieren.",
                         "Scheinbar wollen sie Dir etwas mitteilen. Im Sueden hoerst Du ein sanftes",
                         "Plaetschern. Direkt vor Dir befindet sich ein Findlingsstein."}
    local ground = "Nach Sueden hin wird das Gelaende felsiger."
    local exits = "Es gibt vier sichtbare Ausgaenge: westen, sueden, suedosten und nordwesten."
    local house = "Das Haus von Flinx."
    local beam = "Ein Lichtstrahl."
    verifyInJoinBand({description[1], description[2], description[3], exits}, 78)
    feedAndSettle(table.concat(description, "\r\n") .. "\r\n" .. ground .. "\r\n" .. exits .. "\r\n" .. house .. "\r\n" .. beam .. "\r\n")

    assert.is_true(bufferHasLine(exits), "the exits line was held and the object below it joined onto it")
    assert.is_false(bufferHasLine(exits .. " " .. house), "the object line was joined onto the exits line")
    assert.is_true(bufferHasLine(house), "the object line was not committed on its own")
    assert.is_true(bufferHasLine(beam), "the second object line was not committed on its own")
    assert.is_true(bufferHasLine(description[1] .. " " .. description[2]), "the first two description lines were not joined")
    assert.is_true(bufferHasLine(description[3] .. " " .. description[4]), "the last two description lines were not joined")
    assert.is_true(bufferHasLine(ground), "the line below the description was not committed on its own")
  end)

  it("keeps a room's exits apart from its contents", function()
    -- the elven grove's exits line at the game's own width of 78 sits exactly on
    -- the edge of the fit check: "Ein" would have ended csmServerWrapFitTolerance
    -- clear of the column to the character, so that check alone parts the two and
    -- nothing here would be testing the sentence refusal. One column narrower
    -- puts it out of the fit check's reach, leaving the sentence the exits line
    -- ends on as the only thing that can keep the board below it off the end:
    configure("undoServerWrapWidth", 77)
    local exits = "Es gibt vier sichtbare Ausgaenge: oben, norden, westen und sueden."
    local board = "Ein Partybrett."
    verifyInJoinBand({exits}, 77)
    feedAndSettle(exits .. "\r\n" .. board .. "\r\n")

    assert.is_true(bufferHasLine(exits), "the exits line was not committed on its own")
    assert.is_true(bufferHasLine(board), "the object line was not committed on its own")
    assert.is_false(bufferHasLine(exits .. " " .. board), "an object was joined onto the exits line above it")
  end)

  it("does not join a tell that repeats its prefix on every line", function()
    configure("undoServerWrapWidth", 78)
    -- the game wraps a tell itself and re-prefixes every physical line of it.
    -- Those lines end mid-sentence at the wrap column, so nothing but the
    -- repeated prefix tells them from wrapped prose - joining them embedded the
    -- prefix mid-paragraph and swallowed the room description that followed:
    local tell = {"Anne teilt Dir mit: Willkommen in Moron, Gast7.",
                  "Anne teilt Dir mit: In dieser Stadt lauern viele Gefahren auf Dich. Wenn Du",
                  "Anne teilt Dir mit: Dich hier nicht auskennst, solltest Du sehr vorsichtig",
                  "Anne teilt Dir mit: sein. Am besten ist es, wenn Du mich zuerst besuchst. Ich",
                  "Anne teilt Dir mit: halte mich in der Bibliothek im Sueden von Moron auf."}
    local description = {"Du bist bis zu den ersten Haeusern der Stadt vorgedrungen. Rechts und links",
                         "neben Dir befinden sich die Ueberreste von mehreren steinernen Saeulen, die",
                         "moeglicherweise einmal ein Tor gebildet haben. Doch jetzt sind hier nur noch",
                         "Truemmer zu finden... Aber obwohl hier nur Ruinen stehen, wirst Du einfach das",
                         "Gefuehl nicht los, dass diese Stadt doch nicht so unbewohnt ist, wie sie bis",
                         "jetzt aussieht. Die Strasse fuehrt noch ein Stueck nach Westen in die Stadt",
                         "hinein, wo sie dann nach Sueden abknickt."}
    local exits = "Es gibt zwei sichtbare Ausgaenge: osten und westen."
    -- every line the game could have held; the tell's first is short enough that
    -- it never was a candidate
    verifyInJoinBand({tell[2], tell[3], tell[4], tell[5],
                      description[1], description[2], description[3], description[4], description[5], description[6]}, 78)
    feedAndSettle(table.concat(tell, "\r\n") .. "\r\n" .. table.concat(description, "\r\n") .. "\r\n" .. exits .. "\r\n")

    for index, line in ipairs(tell) do
      assert.is_true(bufferHasLine(line), "line " .. index .. " of the tell was not committed exactly as the game sent it")
    end
    assert.are.equal(#tell, bufferLinesContaining("Anne teilt Dir mit:"),
      "the tell's prefix ended up on a different number of lines than the game sent it on")
    assert.is_true(bufferHasLine(table.concat(description, " ")), "the room description below the tell was not joined into one line of its own")
    assert.is_true(bufferHasLine(exits), "the exits line was not committed on its own")
  end)

  it("still joins a sentence whose break space the game kept", function()
    configure("undoServerWrapWidth", 78)
    -- a kept break space is evidence the game broke the line there, so a segment
    -- ending a sentence is still held - both the single space and the two that
    -- games leaving a wide gap after a full stop keep:
    local oneSpace = string.rep("x", 62) .. " alpha. "
    local twoSpaces = string.rep("x", 61) .. " alpha.  "
    local continuation = "one more word."
    verifyInJoinBand({oneSpace, twoSpaces}, 78)

    feedAndSettle(oneSpace .. "\r\n" .. continuation .. "\r\n")
    assert.is_true(bufferHasLine(oneSpace .. continuation), "a sentence whose break space the game kept was not joined onto its continuation")

    feedAndSettle(twoSpaces .. "\r\n" .. continuation .. "\r\n")
    assert.is_true(bufferHasLine(twoSpaces .. continuation), "a sentence followed by the wide gap such a game leaves was not joined onto its continuation")
  end)

  it("joins a continuation that opens with the same single word", function()
    configure("undoServerWrapWidth", 78)
    -- only a repeated opening of two words or more marks a message the game
    -- re-prefixed; German prose opens line after line with the same pronoun or
    -- article, and those lines are wrapped like any other:
    local held = "Du bist bis zu den ersten Haeusern der Stadt vorgedrungen, hinter denen"
    local continuation = "Du weitere Gebaeude vermutest."
    verifyInJoinBand({held}, 78)
    feedAndSettle(held .. "\r\n" .. continuation .. "\r\n")

    assert.is_true(bufferHasLine(held .. " " .. continuation), "a continuation sharing one opening word with the line above was mistaken for a re-prefixed message")
  end)

  it("flushes a continuation that opens with the same two words", function()
    configure("undoServerWrapWidth", 78)
    -- the shortest prefix a game re-prefixes with is a name and a verb, and the
    -- two lines below share exactly that much: one word fewer has to join, one
    -- word more must not be needed. The held line is long enough that "Anne"
    -- would not have fitted on it, so the repeated opening is the only thing
    -- that can part the two:
    local held = "Anne sagt: Ich habe Dir etwas Wichtiges zu erzaehlen, mein lieber Gast"
    local continuation = "Anne sagt: Hoere gut zu, denn es ist wichtig."
    verifyInJoinBand({held}, 78)
    feedAndSettle(held .. "\r\n" .. continuation .. "\r\n")

    assert.is_true(bufferHasLine(held), "the first line of a re-prefixed message was not committed on its own")
    assert.is_true(bufferHasLine(continuation), "the second line of a re-prefixed message was not committed on its own")
    assert.is_false(bufferHasLine(held .. " " .. continuation), "two lines the game re-prefixed with the same two words were joined into one")
  end)

  it("joins the leading-space wrap style over a repeated first word", function()
    configure("undoServerWrapWidth", 78)
    -- a game that moves the break space to the start of the continuation makes
    -- every held segment open with one. That space begins no word, so these
    -- three lines share a single opening word - "der" - and are a paragraph, not
    -- a message repeated with its prefix. Both held lines are long enough that
    -- "der" would not have fitted on them:
    local first = "Vor Dir liegt ein weiter Platz, auf dem sich zahlreiche Haendler und"
    local second = " der Laerm der vielen Stimmen dringt von allen Seiten auf Dich ein, und"
    local third = " der Duft von Gewuerzen liegt in der Luft."
    verifyInJoinBand({first, second}, 78)
    feedAndSettle(first .. "\r\n" .. second .. "\r\n" .. third .. "\r\n")

    -- the continuations carry the break space themselves, so the join adds none
    assert.is_true(bufferHasLine(first .. second .. third), "a paragraph wrapped in the leading-space style was parted where its lines repeat one word")
  end)

  it("compares a continuation against the game line it follows, not the paragraph", function()
    configure("undoServerWrapWidth", 78)
    -- the opening kept for the comparison has to be the last game line's, taken
    -- before the paragraph above it is joined on. The last line here opens with
    -- the same two words as the FIRST - which is no longer what it follows - and
    -- its "Du" would not have fitted on the line it does follow, so a paragraph
    -- read as one message is the only way these three could come apart:
    local first = "Du siehst hier einen breiten Weg, der sich nach Norden hin zwischen"
    local second = "hohen Felsen hindurchzieht, und was Du dort in der weiten Ferne erkennst:"
    local third = "Du siehst nur Staub und Steine."
    verifyInJoinBand({first, second}, 78)
    feedAndSettle(first .. "\r\n" .. second .. "\r\n" .. third .. "\r\n")

    assert.is_true(bufferHasLine(first .. " " .. second .. " " .. third), "a continuation was compared against the opening of the paragraph rather than of the game line above it")
  end)

  it("holds a segment ending mid-sentence and refuses one ending on any sentence mark", function()
    configure("undoServerWrapWidth", 78)
    -- a comma is not the end of anything, so a line that wraps on one is held
    -- like any other - the marks that refuse a hold are only those that finish a
    -- sentence, not every mark a wrapped line may end on:
    local clause = "Er blickt Dich freundlich an, nickt Dir kurz zu und laechelt dabei,"
    local rest = "waehrend er Dir die Hand reicht."
    verifyInJoinBand({clause}, 78)
    feedAndSettle(clause .. "\r\n" .. rest .. "\r\n")
    assert.is_true(bufferHasLine(clause .. " " .. rest), "a line wrapped on a comma was refused a hold as though it had ended a sentence")

    -- and a sentence finished with something other than a full stop is refused
    -- just the same; "Der" would not have fitted on it, so nothing else parts
    -- these two
    local exclaimed = "Was fuer ein wunderschoener Tag, denk Dir nur, was noch alles kommt!"
    local follower = "Der Tag beginnt gut."
    verifyInJoinBand({exclaimed}, 78)
    feedAndSettle(exclaimed .. "\r\n" .. follower .. "\r\n")
    assert.is_true(bufferHasLine(exclaimed), "a line ending on an exclamation mark was not committed on its own")
    assert.is_true(bufferHasLine(follower), "the line after one ending on an exclamation mark was not committed on its own")
    assert.is_false(bufferHasLine(exclaimed .. " " .. follower), "a line the game ended on an exclamation mark had the next line joined onto it")
  end)
end)
