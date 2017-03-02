describe("Tests the GUI utilities as far as possible without mudlet", function()

  setup(function()
    -- add in the location of our files
    package.path = "../lua/?.lua;"

    -- add in the location of Lua libraries on Ubuntu 12.04
    package.path = package.path ..
"/usr/local/share/lua/5.1/?.lua;/usr/local/share/lua/5.1/?/init.lua;/usr/local/lib/lua/5.1/?.lua;/usr/local/lib/lua/5.1/?/init.lua;/usr/share/lua/5.1/?.lua;/usr/share/lua/5.1/?/init.lua"

    rex    = require"rex_pcre"

    -- define some common Mudlet functions essential to operation

    -- load rrequired other functions
    require"StringUtils"

    -- load the functions
    require"GUIUtils"
  end)

  describe("Test the operation of the ansi2decho function", function()

    it("Should have loaded the function successfully", function()
      assert.truthy(ansi2decho)
    end)

    it("Should convert simple single ANSI sequences correctly", function()
      local sequences = {
        {"\27[0m", "<r>"},
        {"\27[30m", "<0,0,0:>"},
        {"\27[31m", "<128,0,0:>"},
        {"\27[32m", "<0,179,0:>"},
        {"\27[33m", "<128,128,0:>"},
        {"\27[34m", "<0,0,128:>"},
        {"\27[35m", "<128,0,128:>"},
        {"\27[36m", "<0,128,128:>"},
        {"\27[37m", "<192,192,192:>"},
        {"\27[40m", "<:0,0,0>"},
        {"\27[41m", "<:128,0,0>"},
        {"\27[42m", "<:0,179,0>"},
        {"\27[43m", "<:128,128,0>"},
        {"\27[44m", "<:0,0,128>"},
        {"\27[45m", "<:128,0,128>"},
        {"\27[46m", "<:0,128,128>"},
        {"\27[47m", "<:192,192,192>"},
        {"\27[90m", "<128,128,128:>"},
        {"\27[91m", "<255,0,0:>"},
        {"\27[92m", "<0,255,0:>"},
        {"\27[93m", "<255,255,0:>"},
        {"\27[94m", "<0,0,255:>"},
        {"\27[95m", "<255,0,255:>"},
        {"\27[96m", "<0,255,255:>"},
        {"\27[97m", "<255,255,255:>"},
        {"\27[100m", "<:128,128,128>"},
        {"\27[101m", "<:255,0,0>"},
        {"\27[102m", "<:0,255,0>"},
        {"\27[103m", "<:255,255,0>"},
        {"\27[104m", "<:0,0,255>"},
        {"\27[105m", "<:255,0,255>"},
        {"\27[106m", "<:0,255,255>"},
        {"\27[107m", "<:255,255,255>"},
      }
      for _, seq in ipairs(sequences) do
        local actualResult = ansi2decho(seq[1])
        assert.are.same(seq[2], actualResult)
      end
    end)

    it("Should combine tags correctly", function()
      local sequences = {
        {"\27[0;30m", "<r><0,0,0:>"},
        {"\27[1;30m", "<128,128,128:>"},
        {"\27[1;40m", "<:0,0,0>"},
        {"\27[31;42m", "<128,0,0:0,179,0>"},
        {"\27[30;0m", "<r>"},
        {"\27[0;1;30;40m", "<r><128,128,128:0,0,0>"},
      }
      for _, seq in ipairs(sequences) do
        local actualResult = ansi2decho(seq[1])
        assert.are.same(seq[2], actualResult)
      end
    end)

    it("Should leave normal text and other escape sequences alone", function()
      local sequences = {
        {"Hello World", "Hello World"},
        {"[Something in braces]", "[Something in braces]"},
        {"\27[4z<PROMPT>4876h, 3539m, 22200e, 21648w cexkdb-\27[4z</PROMPT>", "\27[4z<PROMPT>4876h, 3539m, 22200e, 21648w cexkdb-\27[4z</PROMPT>"},
      }
      for _, seq in ipairs(sequences) do
        local actualResult = ansi2decho(seq[1])
        assert.are.same(seq[2], actualResult)
      end
    end)

    it("Should convert xterm256 codes correctly", function()
      local sequences = {
        { "\27[38;2;120;134;94m", "<120,134,94:>"},
        { "\27[48;2;85;250;33m", "<:85,250,33>"},
        { "\27[38;2;120;134;94;48;2;85;250;33m", "<120,134,94:85,250,33>"},
        { "\27[38;5;4m", "<0,0,128:>"},
        { "\27[48;5;3m", "<:128,128,0>"},
        { "\27[38;5;4;48;5;3m", "<0,0,128:128,128,0>"},
        { "\27[38;5;10m", "<0,255,0:>"},
        { "\27[48;5;9m", "<:255,0,0>"},
        { "\27[38;5;10;48;5;9m", "<0,255,0:255,0,0>"},
        { "\27[38;5;159m", "<126,210,210:>"},
        { "\27[48;5;106m", "<:84,126,0>"},
        { "\27[38;5;159;48;5;106m", "<126,210,210:84,126,0>"},
        { "\27[38;5;240m", "<80,80,80:>"},
        { "\27[48;5;245m", "<:130,130,130>"},
        { "\27[38;5;240;48;5;245m", "<80,80,80:130,130,130>"},
      }
      for _, seq in ipairs(sequences) do
        local actualResult = ansi2decho(seq[1])
        assert.are.same(seq[2], actualResult)
      end
    end)

    it("Should convert some real life examples correctly", function()
      local sequences = {
        {"\27[4z<PROMPT>\27[0;32;40m4876h, \27[0;1;33;40m3539m, \27[0;1;31;40m22200e, \27[0;1;32;40m21648w \27[0;37;40mcexkdb-\27[4z</PROMPT>", "\27[4z<PROMPT><r><0,179,0:0,0,0>4876h, <r><255,255,0:0,0,0>3539m, <r><255,0,0:0,0,0>22200e, <r><0,255,0:0,0,0>21648w <r><192,192,192:0,0,0>cexkdb-\27[4z</PROMPT>"},
        {'\27[0;1;36;40mYou say in a baritone voice, "Test."\27[0;37;40m', '<r><0,255,255:0,0,0>You say in a baritone voice, "Test."<r><192,192,192:0,0,0>'},
        {'\27[38;5;179;48;5;230mYou say in a baritone voice, "Test."\27[0;37;40m', '<168,126,42:210,210,168>You say in a baritone voice, "Test."<r><192,192,192:0,0,0>'}
      }
      for _, seq in ipairs(sequences) do
        local actualResult = ansi2decho(seq[1])
        assert.are.same(seq[2], actualResult)
      end
    end)

  end)

end)
