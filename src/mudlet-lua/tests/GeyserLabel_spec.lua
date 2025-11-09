describe("Tests functionality of Geyser.Label", function()
  describe('Tests that decho/hecho/cecho preserve font size', function()
    local label
    local globalEchoSpy

    before_each(function()
      -- Spy on the global echo function to inspect what HTML is generated
      globalEchoSpy = spy.on(_G, "echo")

      -- Create a label with a specific font size
      label = Geyser.Label:new({
        name = "testLabel",
        x = 0, y = 0,
        width = 100, height = 100,
      })

      -- Set font size to 50
      label:setFontSize(50)
    end)

    after_each(function()
      _G.echo:revert()
      if label then
        label:hide()
      end
    end)

    it('preserves font size when using echo()', function()
      label:echo("test message")

      -- Verify echo was called
      assert.spy(globalEchoSpy).was.called()

      -- Get the HTML that was passed to echo
      local callArgs = globalEchoSpy.calls[#globalEchoSpy.calls]
      local html = callArgs.vals[2] -- second argument is the message

      -- Verify the HTML contains font-size: 50pt
      assert.is_truthy(html:find("font%-size: 50pt"))
    end)

    it('preserves font size when using decho()', function()
      label:decho("<255,0,0>red text")

      -- Verify echo was called (decho ultimately calls echo)
      assert.spy(globalEchoSpy).was.called()

      -- Get the HTML that was passed to echo
      local callArgs = globalEchoSpy.calls[#globalEchoSpy.calls]
      local html = callArgs.vals[2]

      -- Verify the HTML contains font-size: 50pt
      assert.is_truthy(html:find("font%-size: 50pt"), "decho did not preserve font size")
    end)

    it('preserves font size when using hecho()', function()
      label:hecho("|cff0000red text")

      -- Verify echo was called (hecho ultimately calls echo)
      assert.spy(globalEchoSpy).was.called()

      -- Get the HTML that was passed to echo
      local callArgs = globalEchoSpy.calls[#globalEchoSpy.calls]
      local html = callArgs.vals[2]

      -- Verify the HTML contains font-size: 50pt
      assert.is_truthy(html:find("font%-size: 50pt"), "hecho did not preserve font size")
    end)

    it('preserves font size when using cecho()', function()
      label:cecho("<red>red text")

      -- Verify echo was called (cecho ultimately calls echo)
      assert.spy(globalEchoSpy).was.called()

      -- Get the HTML that was passed to echo
      local callArgs = globalEchoSpy.calls[#globalEchoSpy.calls]
      local html = callArgs.vals[2]

      -- Verify the HTML contains font-size: 50pt
      assert.is_truthy(html:find("font%-size: 50pt"), "cecho did not preserve font size")
    end)

    it('preserves bold formatting when using decho()', function()
      label:setBold(true)
      label:decho("<255,0,0>bold red text")

      local callArgs = globalEchoSpy.calls[#globalEchoSpy.calls]
      local html = callArgs.vals[2]

      -- Verify the HTML contains both font-size and bold tags
      assert.is_truthy(html:find("font%-size: 50pt"))
      assert.is_truthy(html:find("<b>"))
    end)

    it('preserves alignment when using hecho()', function()
      label:setAlignment("center")
      label:hecho("|cff0000centered text")

      local callArgs = globalEchoSpy.calls[#globalEchoSpy.calls]
      local html = callArgs.vals[2]

      -- Verify the HTML contains alignment
      assert.is_truthy(html:find('align="center"'))
      assert.is_truthy(html:find("font%-size: 50pt"))
    end)
  end)
end)
