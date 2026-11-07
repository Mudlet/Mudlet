local function geometry(name)
  local x, y, width, height = getWindowGeometry(name)
  return {x = x, y = y, width = width, height = height}
end

describe("Tests functionality of Geyser.ListItem", function()
  local list, created

  local function track(object)
    created[#created + 1] = object
    return object
  end

  before_each(function()
    created = {}
    list = Geyser.List:new({name = "gliList", x = 0, y = 0, width = 100, height = 100})
  end)

  after_each(function()
    list:delete()
    for _, object in ipairs(created) do
      if not object.deleted then
        object:delete()
      end
    end
  end)

  describe("Geyser.ListItem:new/new2", function()
    it("is a label of its own type, a line tall and fixed", function()
      local item = Geyser.ListItem:new({name = "gliNew"}, list)
      assert.are.equal("listitem", item.type)
      assert.are.equal("label", windowType("gliNew"))
      assert.are.equal("1c", item.height)
      assert.are.equal(Geyser.Fixed, item.v_policy)
      assert.are.equal(Geyser.Label.setClickCallback, item.setClickCallback)
      assert.are.equal(list.content, item.container)
    end)

    it("works outside a list", function()
      local item = track(Geyser.ListItem:new({name = "gliAlone", x = 0, y = 0, width = 10}))
      assert.is_nil(item:getList())
      assert.is_nil(item:getIndex())
      assert.is_false(item:isSelected())
      local moved, err = item:moveTo(1)
      assert.is_nil(moved)
      assert.is_string(err)
      item:dismiss()
      assert.is_true(item.deleted)
    end)

    it("returns nil and why for a name it can't use", function()
      local console = track(Geyser.MiniConsole:new({name = "gliTaken", x = 0, y = 0, width = 10, height = 10}))
      stub(_G, "printError")
      local item, err = Geyser.ListItem:new({name = console.name}, list)
      assert.stub(printError).was.called()
      printError:revert()
      assert.is_nil(item)
      assert.is_string(err)
    end)

    it("new2 builds through add2", function()
      local item = Geyser.ListItem:new2({name = "gliNew2"}, list)
      assert.is_true(item.useAdd2)
      assert.are.equal(list.content, item.container)
    end)

    it("calls onInsert with the finished item", function()
      local seen, text
      local item = Geyser.ListItem:new({name = "gliInsert", message = "hi", onInsert = function(i)
        seen, text = i, i.message
      end}, list)
      assert.are.equal(item, seen)
      assert.are.equal("hi", text)
    end)

    it("builds rows from an item used as a prototype", function()
      local seen = {}
      local proto = Geyser.ListItem:new({name = "gliProto"}, list)
      proto:setOnInsert(function(i) seen[#seen + 1] = i.name end)
      local item = proto:new({name = "gliFromProto"}, list)
      assert.are.equal("label", windowType("gliFromProto"))
      assert.are.equal(list, item:getList())
      assert.are.same({"gliFromProto"}, seen)
    end)

    it("a row built from another has its own selection, stripe and life", function()
      local striped = track(Geyser.List:new({name = "gliProtoList", x = 0, y = 0, width = 100, height = 100,
        selectionMode = "single", stripes = {"background-color: red;", "background-color: blue;"}}))
      local proto = Geyser.ListItem:new({name = "gliProtoA", stylesheet = "color: white;"}, striped)
      local item = proto:new({name = "gliProtoB"}, striped)
      item.clickCallback()
      assert.is_true(item:isSelected())
      assert.is_false(proto:isSelected())
      assert.are.equal("background-color: blue;color: white;", getLabelStyleSheet("gliProtoB"))
      proto:delete()
      local orphan = proto:new({name = "gliProtoC"}, striped)
      orphan:delete()
      assert.is_true(orphan.deleted)
      assert.is_nil(striped:getItem("gliProtoC"))
      assert.is_nil(windowType("gliProtoC"))
    end)

    it("fits the list to a row that sizes itself", function()
      local item = list:addItem({name = "gliAuto", autoHeight = true, message = "one<br>two<br>three"})
      assert.is_true(item:get_height() > 10)
      assert.are.equal(item:get_height(), list.content:get_height())
    end)
  end)

  describe("dismiss", function()
    it("deletes right away with no onDismiss", function()
      local item = Geyser.ListItem:new({name = "gliNow"}, list)
      item:dismiss()
      assert.is_true(item.deleted)
      assert.is_nil(windowType("gliNow"))
      assert.are.equal(0, list:getItemCount())
    end)

    it("waits for onDismiss to call done, and asks only once", function()
      local calls, finish = 0, nil
      local item = Geyser.ListItem:new({name = "gliWait", onDismiss = function(i, done)
        calls, finish = calls + 1, done
        assert.are.equal("gliWait", i.name)
      end}, list)
      item:dismiss()
      item:dismiss()
      assert.are.equal(1, calls)
      assert.is_falsy(item.deleted)
      assert.are.equal("label", windowType("gliWait"))
      finish()
      assert.is_true(item.deleted)
      finish()
      assert.is_nil(windowType("gliWait"))
    end)

    it("setOnDismiss replaces it", function()
      local asked = false
      local item = Geyser.ListItem:new({name = "gliSet"}, list)
      item:setOnDismiss(function() asked = true end)
      item:dismiss()
      assert.is_true(asked)
      assert.is_falsy(item.deleted)
      item:delete()
    end)

    it("delete doesn't consult onDismiss, and is safe twice", function()
      local asked = false
      local item = Geyser.ListItem:new({name = "gliDelete", onDismiss = function() asked = true end}, list)
      item:delete()
      item:delete()
      assert.is_false(asked)
      assert.is_true(item.deleted)
    end)

    it("a parent going away takes it without consulting onDismiss", function()
      local asked = false
      local item = Geyser.ListItem:new({name = "gliParent", onDismiss = function() asked = true end}, list)
      list:clear()
      assert.is_false(asked)
      assert.is_true(item.deleted)
    end)
  end)

  describe("echoing", function()
    local item

    before_each(function()
      item = Geyser.ListItem:new({name = "gliEcho", fgColor = "white"}, list)
    end)

    it("echoes new text, and setters still show it again", function()
      item:echo("one")
      assert.are.equal("one", item.message)
      item:echo("one")
      assert.are.equal("one", item.message)
      item:setFgColor("red")
      assert.are.equal("red", item.fgColor)
      item:setBold(true)
      assert.is_true(item.formatTable.bold)
    end)

    it("shows text again that rawEcho replaced", function()
      item:echo("AAA")
      item:rawEcho("BBB")
      item:echo("AAA")
      assert.is_truthy(getLabelText("gliEcho"):find("AAA", 1, true))
    end)

    for _, flavour in ipairs({"cecho", "decho", "hecho"}) do
      it(flavour .. " shows new text and then plain echo replaces it", function()
        assert.is_true(item[flavour](item, "text"))
        assert.is_true(item[flavour](item, "text"))
        assert.are.equal(flavour, item.echoType)
        item:echo("text")
        assert.is_false(item.echoType)
      end)

      it(flavour .. " shows text again that rawEcho replaced", function()
        item[flavour](item, "AAA")
        item:rawEcho("BBB")
        item[flavour](item, "AAA")
        assert.is_truthy(getLabelText("gliEcho"):find("AAA", 1, true))
      end)
    end
  end)

  describe("its place in the list", function()
    it("knows its list and position, and can move", function()
      local a = list:addItem({name = "gliA", height = 10})
      local b = list:addItem({name = "gliB", height = 10})
      assert.are.equal(list, a:getList())
      assert.are.equal(2, b:getIndex())
      assert.is_true(b:moveTo(1))
      assert.are.equal(1, b:getIndex())
      assert.are.equal(10, geometry("gliA").y)
    end)

    it("a change of height refits the list", function()
      local a = list:addItem({name = "gliTall", height = 10})
      local b = list:addItem({name = "gliBelow", height = 10})
      a:resize(nil, 30)
      assert.are.equal(40, list.content:get_height())
      assert.are.equal(30, geometry("gliBelow").y)
      assert.are.equal(b, list:getItem(2))
    end)

    it("raising and lowering don't change its position", function()
      local a = list:addItem({name = "gliRaiseA", height = 10})
      local b = list:addItem({name = "gliRaiseB", height = 10})
      a:raise()
      assert.are.equal(1, a:getIndex())
      b:lower()
      assert.are.equal(2, b:getIndex())
      list:fit()
      assert.are.equal(10, geometry("gliRaiseB").y)
    end)
  end)
end)
