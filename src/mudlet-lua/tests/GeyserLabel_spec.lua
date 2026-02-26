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

  describe("Tests SVG transform functions", function()
    local testLabel = "testSvgLabel"

    setup(function()
      createLabel(testLabel, 0, 0, 100, 100, 1)
    end)

    teardown(function()
      hideWindow(testLabel)
    end)

    describe("Tests setSvgTint", function()
      it("should accept RGB values", function()
        local result = setSvgTint(testLabel, 255, 0, 0)
        assert.is_true(result)
      end)

      it("should accept color string", function()
        local result = setSvgTint(testLabel, "#ff0000")
        assert.is_true(result)
      end)

      it("should return nil for invalid color string", function()
        local result, err = setSvgTint(testLabel, "notacolor")
        assert.is_nil(result)
        assert.is_string(err)
      end)

      it("should return nil for out-of-range RGB", function()
        local result, err = setSvgTint(testLabel, 256, 0, 0)
        assert.is_nil(result)
        assert.is_string(err)
      end)

      it("should return nil for non-existent label", function()
        local result, err = setSvgTint("noSuchLabel", 255, 0, 0)
        assert.is_nil(result)
        assert.is_string(err)
      end)
    end)

    describe("Tests resetSvgTint", function()
      it("should succeed on existing label", function()
        local result = resetSvgTint(testLabel)
        assert.is_true(result)
      end)

      it("should return nil for non-existent label", function()
        local result, err = resetSvgTint("noSuchLabel")
        assert.is_nil(result)
        assert.is_string(err)
      end)
    end)

    describe("Tests setSvgRotation", function()
      it("should accept positive angle", function()
        local result = setSvgRotation(testLabel, 45)
        assert.is_true(result)
      end)

      it("should accept negative angle", function()
        local result = setSvgRotation(testLabel, -90)
        assert.is_true(result)
      end)

      it("should return nil for non-existent label", function()
        local result, err = setSvgRotation("noSuchLabel", 45)
        assert.is_nil(result)
        assert.is_string(err)
      end)
    end)

    describe("Tests resetSvgRotation", function()
      it("should succeed on existing label", function()
        local result = resetSvgRotation(testLabel)
        assert.is_true(result)
      end)

      it("should return nil for non-existent label", function()
        local result, err = resetSvgRotation("noSuchLabel")
        assert.is_nil(result)
        assert.is_string(err)
      end)
    end)

    describe("Tests setSvgShear", function()
      it("should accept shear values", function()
        local result = setSvgShear(testLabel, 0.3, 0.1)
        assert.is_true(result)
      end)

      it("should return nil for non-existent label", function()
        local result, err = setSvgShear("noSuchLabel", 0.3, 0.1)
        assert.is_nil(result)
        assert.is_string(err)
      end)
    end)

    describe("Tests resetSvgShear", function()
      it("should succeed on existing label", function()
        local result = resetSvgShear(testLabel)
        assert.is_true(result)
      end)

      it("should return nil for non-existent label", function()
        local result, err = resetSvgShear("noSuchLabel")
        assert.is_nil(result)
        assert.is_string(err)
      end)
    end)

    describe("Tests resetSvgTransform", function()
      it("should succeed on existing label", function()
        local result = resetSvgTransform(testLabel)
        assert.is_true(result)
      end)

      it("should return nil for non-existent label", function()
        local result, err = resetSvgTransform("noSuchLabel")
        assert.is_nil(result)
        assert.is_string(err)
      end)
    end)
  end)
end)
