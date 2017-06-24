require 'busted.runner'()

describe("Tests busted setup works", function()
  setup(function()
    
    function ineedthis() return "." end

  end)

  describe("Tests that DB creation and deletion works", function()
    it("Should have ineedthis defined", function()
      ineedthis()
    end)


    teardown(function()
      print("teardown worked")
    end)
  end)

end)
