describe("tests the functionality of the STT module", function()
  describe("Tests the functionality of STT.listModels", function()
    local realIsAvailable, realListModels

    before_each(function()
      realIsAvailable = stt.isAvailable
      realListModels = stt.listModels
    end)

    after_each(function()
      stt.isAvailable = realIsAvailable
      stt.listModels = realListModels
    end)

    it("should list models on disk even when the Vosk library is unavailable", function()
      stt.isAvailable = function() return false end
      stt.listModels = function()
        return {{name = "vosk-model-small-en-us-0.15", path = "/models/vosk-model-small-en-us-0.15"}}
      end

      local models = STT.listModels()
      assert.equals(1, #models)
      assert.equals("vosk-model-small-en-us-0.15", models[1].name)
    end)

    it("should list models on disk when the Vosk library is available", function()
      stt.isAvailable = function() return true end
      stt.listModels = function()
        return {{name = "vosk-model-en-us-0.22", path = "/models/vosk-model-en-us-0.22"}}
      end

      local models = STT.listModels()
      assert.equals(1, #models)
      assert.equals("vosk-model-en-us-0.22", models[1].name)
    end)

    it("should return an empty table when the stt.listModels binding is missing", function()
      stt.isAvailable = function() return false end
      stt.listModels = nil

      assert.equals(0, #STT.listModels())
    end)
  end)

  describe("Tests the functionality of STT.UI._isModelInstalled", function()
    local realIsAvailable, realListModels

    before_each(function()
      realIsAvailable = stt.isAvailable
      realListModels = stt.listModels
    end)

    after_each(function()
      stt.isAvailable = realIsAvailable
      stt.listModels = realListModels
    end)

    it("should detect an installed model when the Vosk library is unavailable", function()
      stt.isAvailable = function() return false end
      stt.listModels = function()
        return {{name = "vosk-model-small-en-us-0.15", path = "/models/vosk-model-small-en-us-0.15"}}
      end

      assert.is_true(STT.UI._isModelInstalled("vosk-model-small-en-us-0.15"))
    end)

    it("should not report a model that is not on disk", function()
      stt.isAvailable = function() return false end
      stt.listModels = function() return {} end

      assert.is_false(STT.UI._isModelInstalled("vosk-model-small-en-us-0.15"))
    end)
  end)
end)
