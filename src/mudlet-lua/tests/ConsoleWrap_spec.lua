-- How a console breaks the lines it is handed into the lines a player sees.
--
-- Every line Mudlet shows has been through this, but the only thing asserted
-- about it so far is that a long line becomes more than one. These specs pin
-- down where the break falls, what the two indents do to it, and when the
-- wrapping is applied - all read back out of the buffer, which is what the
-- console draws from.
--
-- A miniconsole is used throughout: it wraps with the same code as the main
-- console but its width is ours to set, while the main console's belongs to
-- the profile and is recomputed whenever the window is resized.

local win = "consoleWrapSpecWindow"

-- getLineCount() gives the index of the last line, which is the empty one the
-- buffer is left sitting on, so it doubles as the count of the finished lines
-- above it.
local function lines()
  return getLines(win, 0, getLineCount(win))
end

local function wrapAt(width, indent, hangingIndent)
  clearWindow(win)
  setWindowWrap(win, width)
  setWindowWrapIndent(win, indent or 0)
  setWindowWrapHangingIndent(win, hangingIndent or 0)
end

local function wrapped(width, text, indent, hangingIndent)
  wrapAt(width, indent, hangingIndent)
  echo(win, text .. "\n")
  return lines()
end

describe("Tests how a console wraps the lines it is given", function()
  setup(function()
    createMiniConsole(win, 0, 0, 400, 300)
  end)

  teardown(function()
    deleteMiniConsole(win)
  end)

  describe("Tests where a wrapped line breaks", function()
    it("breaks a long line at the last space that fits the width", function()
      assert.are.same({"the quick brown fox ", "jumps over the lazy ", "dog"},
                      wrapped(20, "the quick brown fox jumps over the lazy dog"))
    end)

    it("ends a broken line with the space it broke after", function()
      local out = wrapped(10, "abcde fghij")
      assert.are.same({"abcde ", "fghij"}, out)
    end)

    it("leaves a line that exactly fills the width alone", function()
      assert.are.same({"abcdefghij"}, wrapped(10, "abcdefghij"))
    end)

    it("breaks a line that is one column too wide", function()
      assert.are.same({"abcdefghij", "k"}, wrapped(10, "abcdefghijk"))
    end)

    it("breaks a word longer than the whole width at the width", function()
      assert.are.same({"abcdefghij", "klmnopqrst"}, wrapped(10, "abcdefghijklmnopqrst"))
    end)

    it("counts an east asian character as two columns", function()
      -- ten columns of a script drawn double width is five characters
      assert.are.same({"日本語のテ", "キストです"}, wrapped(10, "日本語のテキストです"))
    end)

    it("drops the spaces a break lands inside", function()
      -- the break falls in the middle of the run of six spaces: the four that
      -- fit finish the line and the two that would have started the next are
      -- dropped rather than indenting it
      assert.are.same({"aaaa    ", "bbbb"}, wrapped(8, "aaaa      bbbb"))
    end)

    it("puts a single character on each line when the width is one", function()
      assert.are.same({"a", "b", "c"}, wrapped(1, "abc"))
    end)

    it("wraps each half of a line that has a newline in the middle", function()
      assert.are.same({"one", "two three four five ", "six seven"},
                      wrapped(20, "one\ntwo three four five six seven"))
    end)
  end)

  describe("Tests the indents a wrapped line is given", function()
    it("indents the first line of a wrapped line by the wrap indent", function()
      local out = wrapped(20, "the quick brown fox jumps over the lazy dog", 2, 0)
      assert.are.equal("  the quick brown ", out[1])
      assert.are.equal("fox jumps over the ", out[2])
    end)

    it("indents the lines after the first by the hanging indent", function()
      assert.are.same({"the quick brown fox ", "    jumps over the ", "    lazy dog"},
                      wrapped(20, "the quick brown fox jumps over the lazy dog", 0, 4))
    end)

    it("applies both indents when the two differ", function()
      assert.are.same({"  the quick brown ", "    fox jumps over ", "    the lazy dog"},
                      wrapped(20, "the quick brown fox jumps over the lazy dog", 2, 4))
    end)

    it("takes the indent out of the room the text is given", function()
      -- three columns of indent leave seven for text, not the full ten
      local out = wrapped(10, "aaaa bbbb cccc dddd", 3, 0)
      assert.are.equal("   aaaa ", out[1])
      assert.are.equal("bbbb cccc ", out[2])
    end)

    it("ignores a wrap indent as wide as the window", function()
      -- an indent with no room left for text would wrap a line into single
      -- characters, so it is dropped instead
      assert.are.same(wrapped(10, "aaaa bbbb cccc dddd", 0, 0),
                      wrapped(10, "aaaa bbbb cccc dddd", 10, 0))
    end)

    it("ignores a hanging indent as wide as the window", function()
      assert.are.same(wrapped(10, "aaaa bbbb cccc dddd", 0, 0),
                      wrapped(10, "aaaa bbbb cccc dddd", 0, 10))
    end)
  end)

  describe("Tests when a line is wrapped", function()
    it("wraps text as it arrives and leaves what is already there alone", function()
      wrapAt(20)
      echo(win, "aaaa bbbb cccc dddd eeee\n")
      setWindowWrap(win, 40)
      echo(win, "the second line is quite long as well\n")
      assert.are.same({"aaaa bbbb cccc dddd ", "eeee", "the second line is quite long as well"},
                      lines())
    end)

    it("rewraps the line wrapLine() names to the width now set", function()
      wrapAt(20)
      echo(win, "aaaa bbbb cccc dddd eeee\n")
      setWindowWrap(win, 10)
      wrapLine(win, 0)
      assert.are.same({"aaaa bbbb ", "cccc dddd ", "eeee"}, lines())
    end)

    it("wraps a line that was added to before its newline arrived", function()
      wrapAt(20)
      echo(win, "keeps going")
      assert.are.equal(0, getLineCount(win))
      echo(win, " and going and going and going\n")
      assert.are.same({"keeps going and ", "going and going and ", "going"}, lines())
    end)

    it("carries the colour of the text over the break", function()
      wrapAt(20)
      cecho(win, "<red>aaaaaaaaaa bbbbbbbbbb ccccc\n")
      moveCursor(win, 0, 1)
      selectSection(win, 0, 5)
      local format = getTextFormat(win)
      deselect(win)
      assert.are.same({255, 0, 0}, format.foreground)
    end)

    it("gives a continued line no timestamp of its own", function()
      wrapAt(20)
      -- getTimestamp() refuses line zero, so the wrapped line goes second
      echo(win, "first\n")
      echo(win, "aaaaaaaaaa bbbbbbbbbb cccc\n")
      assert.is_not.matches("^%-+%s*$", getTimestamp(win, 1))
      assert.matches("^%-+%s*$", getTimestamp(win, 2))
    end)
  end)
end)
