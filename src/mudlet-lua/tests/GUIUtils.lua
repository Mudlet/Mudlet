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

  end)

end)
