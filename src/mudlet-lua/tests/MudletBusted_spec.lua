describe("Mudlet Busted sanity check", function()
    it("runs a simple assert", function()
        assert.truthy("Not nil or false.")
      end)
    it("checks that it has access to mudlet environment functions", function()
        assert.truthy(string.find(getMudletHomeDir(), "[mM]udlet"))
      end)
  end)

