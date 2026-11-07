-- A list is a container, so it has no widget of its own: what can be seen of it
-- is its scroll box, the rows in that, and the geometry they were given.
local function geometry(name)
  local x, y, width, height = getWindowGeometry(name)
  return {x = x, y = y, width = width, height = height}
end

describe("Tests functionality of Geyser.List", function()
  local created

  local function track(object)
    created[#created + 1] = object
    return object
  end

  local function alive(object)
    if not object or not object.container or not object.container.windowList then
      return false
    end
    return object.container.windowList[object.name] == object
  end

  local function newList(name, cons)
    cons = cons or {}
    cons.name, cons.x, cons.y = name, 0, 0
    cons.width, cons.height = cons.width or 100, cons.height or 100
    return track(Geyser.List:new(cons))
  end

  local function row(list, name, height, extra)
    local cons = extra or {}
    cons.name, cons.height = name, height or 10
    return list:addItem(cons)
  end

  local function names(list)
    local result = {}
    for i, item in ipairs(list:getItems()) do
      result[i] = item.name
    end
    return result
  end

  before_each(function()
    created = {}
  end)

  after_each(function()
    for _, object in ipairs(created) do
      if alive(object) then
        object:delete()
      end
    end
    created = {}
  end)

  describe("Geyser.List:new/new2", function()
    it("defaults the type and builds its scroll box", function()
      local list = newList("glNew")
      assert.are.equal("list", list.type)
      assert.are.equal("scrollbox", windowType("glNew.ScrollBox"))
      assert.are.equal(0, list:getItemCount())
      assert.is_nil(getWindowGeometry("glNew"))
    end)

    it("makes up a name when not given one", function()
      local list = track(Geyser.List:new({x = 0, y = 0, width = 10, height = 10}))
      assert.is_string(list.name)
      assert.are.equal("scrollbox", windowType(list.name .. ".ScrollBox"))
    end)

    it("reports a name it can't use rather than raising, and leaves nothing behind", function()
      local label = track(Geyser.Label:new({name = "glTaken.ScrollBox", x = 0, y = 0, width = 10, height = 10}))
      stub(_G, "printError")
      local list, err = Geyser.List:new({name = "glTaken", x = 0, y = 0, width = 10, height = 10})
      assert.stub(printError).was.called()
      printError:revert()
      assert.is_nil(list)
      assert.is_string(err)
      assert.is_nil(Geyser.windowList.glTaken)
      assert.are.equal("label", windowType(label.name))
    end)

    it("passes on its container refusing the name rather than raising", function()
      local refuse = stub(Geyser.Container, "new").returns(nil, "name taken")
      local ok, list, err = pcall(Geyser.List.new, Geyser.List, {name = "glRefused", x = 0, y = 0, width = 10, height = 10})
      refuse:revert()
      assert.is_true(ok, list)
      assert.is_nil(list)
      assert.are.equal("name taken", err)
    end)

    it("new2 still puts rows in the content", function()
      local list = track(Geyser.List:new2({name = "glNew2", x = 0, y = 0, width = 100, height = 100}))
      assert.is_true(list.useAdd2)
      assert.are.equal(Geyser.List.add, list.add)
      local item = row(list, "glNew2Row")
      assert.are.equal(list.content, item.container)
    end)

    it("starts hidden when told to", function()
      local list = newList("glHidden", {hidden = true})
      local item = row(list, "glHiddenRow")
      assert.is_true(list.hidden)
      assert.is_true(item.auto_hidden)
    end)
  end)

  describe("rows", function()
    it("stacks rows and fits the content to them", function()
      local list = newList("glStack")
      local one, two = row(list, "glStack1", 10), row(list, "glStack2", 20)
      assert.are.equal(30, list.content:get_height())
      assert.are.equal(0, geometry("glStack1").y)
      assert.are.equal(10, geometry("glStack2").y)
      one:delete()
      assert.are.equal(20, list.content:get_height())
      assert.are.equal(0, geometry("glStack2").y)
      two:delete()
      assert.are.equal(1, list.content:get_height())
    end)

    it("makes any window a row of fixed height", function()
      local list = newList("glPlain")
      local label = Geyser.Label:new({name = "glPlainRow", height = 15}, list)
      assert.are.equal(list.content, label.container)
      assert.are.equal(Geyser.Fixed, label.v_policy)
      assert.are.equal(15, list.content:get_height())
    end)

    it("addItem makes a list item, insertItem at a position", function()
      local list = newList("glInsert")
      row(list, "a")
      row(list, "c")
      local item = list:insertItem({name = "b", height = 10}, 2)
      assert.are.equal("listitem", item.type)
      assert.are.same({"a", "b", "c"}, names(list))
    end)

    it("insertItem reports a name it can't use", function()
      local list = newList("glInsertTaken")
      local console = track(Geyser.MiniConsole:new({name = "glTakenRow", x = 0, y = 0, width = 10, height = 10}))
      stub(_G, "printError")
      local item, err = list:insertItem({name = console.name}, 1)
      assert.stub(printError).was.called()
      printError:revert()
      assert.is_nil(item)
      assert.is_string(err)
      assert.are.equal(0, list:getItemCount())
    end)

    it("looks rows up by position and name", function()
      local list = newList("glLookup")
      local a, b = row(list, "a"), row(list, "b")
      assert.are.equal(a, list:getItem(1))
      assert.are.equal(b, list:getItem("b"))
      assert.is_nil(list:getItem(3))
      assert.is_nil(list:getItem("nope"))
      assert.are.equal(2, list:getItemCount())
      assert.are.equal(2, list:indexOf(b))
      assert.are.equal(1, list:indexOf("a"))
      assert.is_nil(list:indexOf("nope"))
    end)

    it("getItems is a copy", function()
      local list = newList("glCopy")
      row(list, "a")
      row(list, "b")
      for _, item in ipairs(list:getItems()) do
        item:delete()
      end
      assert.are.equal(0, list:getItemCount())
    end)

    it("moves rows, stopping at either end", function()
      local list = newList("glMove")
      local a = row(list, "a")
      row(list, "b")
      row(list, "c")
      assert.is_true(list:moveItem(a, 2))
      assert.are.same({"b", "a", "c"}, names(list))
      assert.are.equal(10, geometry("a").y)
      list:moveItem("a", 99)
      assert.are.same({"b", "c", "a"}, names(list))
      list:moveItem("a", -5)
      assert.are.same({"a", "b", "c"}, names(list))
      local moved, err = list:moveItem("nope", 1)
      assert.is_nil(moved)
      assert.is_string(err)
    end)

    it("clear deletes every row without consulting onDismiss", function()
      local list = newList("glClear")
      local asked = false
      row(list, "a", 10, {onDismiss = function() asked = true end})
      row(list, "b")
      list:clear()
      assert.is_false(asked)
      assert.are.equal(0, list:getItemCount())
      assert.are.equal(1, list.content:get_height())
      assert.is_nil(windowType("a"))
    end)

    it("dismissAll lets onDismiss have its say, and deletes what can't be dismissed", function()
      local list = newList("glDismissAll")
      local dones = {}
      row(list, "a", 10, {onDismiss = function(_, done) dones[#dones + 1] = done end})
      row(list, "b")
      Geyser.Label:new({name = "c", height = 10}, list)
      list:dismissAll()
      assert.are.same({"a"}, names(list))
      dones[1]()
      assert.are.equal(0, list:getItemCount())
    end)

    it("leaves the list when moved to another one", function()
      local from, to = newList("glFrom"), newList("glTo")
      local item = row(from, "glTraveller", 10)
      item:changeContainer(to)
      assert.are.equal(to, item:getList())
      assert.are.equal(0, from:getItemCount())
      assert.are.equal(1, from.content:get_height())
      assert.are.equal(10, to.content:get_height())
    end)

    it("moves a row's label into the other list's scroll box", function()
      local from = newList("glFromBox")
      local to = newList("glToBox", {x = 200, y = 150})
      local item = row(from, "glBoxTraveller", 10)
      item:changeContainer(to)
      assert.are.equal("glToBox.ScrollBox", item.windowname)
      assert.are.same({x = 0, y = 0, width = item:get_width(), height = 10}, geometry("glBoxTraveller"))
    end)

    it("takes everything with it when deleted", function()
      local list = newList("glDelete")
      row(list, "glDeleteRow")
      list:delete()
      assert.is_nil(windowType("glDelete.ScrollBox"))
      assert.is_nil(windowType("glDelete.ScrollSpacer"))
      assert.is_nil(windowType("glDeleteRow"))
      assert.is_nil(Geyser.windowList.glDelete)
    end)
  end)

  describe("sorting", function()
    local function reverse(a, b)
      return a.name > b.name
    end

    it("sorts by name when given nothing", function()
      local list = newList("glSortName")
      row(list, "b")
      row(list, "a")
      assert.are.same({"b", "a"}, names(list))
      list:sort()
      assert.are.same({"a", "b"}, names(list))
      assert.are.equal(0, geometry("a").y)
    end)

    it("a comparator handed to sort is used once", function()
      local list = newList("glSortOnce")
      row(list, "a")
      row(list, "b")
      list:sort(reverse)
      assert.are.same({"b", "a"}, names(list))
      row(list, "c")
      assert.are.same({"b", "a", "c"}, names(list))
      assert.is_nil(list.sortBy)
    end)

    it("sortBy keeps the list in order as rows come in", function()
      local list = newList("glSortBy", {sortBy = reverse})
      row(list, "a")
      row(list, "c")
      row(list, "b")
      assert.are.same({"c", "b", "a"}, names(list))
      assert.are.equal(20, geometry("a").y)
    end)

    it("sortBy can see an item's constraints", function()
      local list = newList("glSortCons", {sortBy = function(a, b) return a.rank < b.rank end})
      row(list, "a", 10, {rank = 2})
      row(list, "b", 10, {rank = 1})
      assert.are.same({"b", "a"}, names(list))
    end)

    it("a sorted list refuses to move rows, and insertItem follows the sort", function()
      local list = newList("glSorted", {sortBy = reverse})
      local a = row(list, "a")
      row(list, "c")
      local moved, err = list:moveItem(a, 1)
      assert.is_nil(moved)
      assert.is_string(err)
      list:insertItem({name = "b", height = 10}, 1)
      assert.are.same({"c", "b", "a"}, names(list))
    end)

    it("setSortBy sorts now, and nil leaves the rows as they are", function()
      local list = newList("glSetSort")
      row(list, "a")
      row(list, "b")
      list:setSortBy(reverse)
      assert.are.same({"b", "a"}, names(list))
      list:setSortBy(nil)
      row(list, "c")
      assert.are.same({"b", "a", "c"}, names(list))
      assert.is_true(list:moveItem("c", 1))
    end)
  end)

  describe("updates", function()
    it("holds the layout back until end_update", function()
      local list = newList("glDefer", {sortBy = function(a, b) return a.name < b.name end})
      local fits = 0
      local fit = list.fit
      list.fit = function(...)
        fits = fits + 1
        return fit(...)
      end
      list:begin_update()
      row(list, "b", 10)
      row(list, "a", 10)
      assert.are.equal(1, list.content:get_height())
      list:end_update()
      assert.are.equal(20, list.content:get_height())
      assert.are.same({"a", "b"}, names(list))
      assert.are.equal(0, geometry("a").y)
      assert.are.equal(10, geometry("b").y)
      assert.are.equal(1, fits)
      list.fit = nil
    end)

    it("catches up on rows deleted meanwhile", function()
      local list = newList("glDeferDel")
      local a = row(list, "a", 10)
      row(list, "b", 10)
      list:begin_update()
      a:delete()
      assert.are.equal(20, list.content:get_height())
      list:end_update()
      assert.are.equal(10, list.content:get_height())
      assert.are.equal(0, geometry("b").y)
    end)

    it("clear doesn't end an update it didn't start", function()
      local list = newList("glDeferClear")
      row(list, "a", 10)
      list:begin_update()
      list:clear()
      assert.are.equal(10, list.content:get_height())
      list:end_update()
      assert.are.equal(1, list.content:get_height())
    end)
    it("places the rows once for each row added", function()
      local list = newList("glOnePass")
      local first = row(list, "a", 10)
      row(list, "b", 10)
      local placed = 0
      local set = first.set_constraints
      first.set_constraints = function(...)
        placed = placed + 1
        return set(...)
      end
      row(list, "c", 10)
      assert.are.equal(1, placed)
      assert.are.equal(20, geometry("c").y)
    end)

    it("dismissAll lays the list out once, after the rows are gone", function()
      local list = newList("glDismissOnce")
      for i = 1, 5 do
        row(list, "glDismissOnce" .. i, 10)
      end
      local organized = 0
      local organize = list.content.organize
      list.content.organize = function(...)
        organized = organized + 1
        return organize(...)
      end
      list:dismissAll()
      assert.are.equal(0, list:getItemCount())
      assert.are.equal(1, list.content:get_height())
      assert.are.equal(1, organized)
    end)

    it("dismissAll doesn't end an update it didn't start", function()
      local list = newList("glDismissDefer")
      row(list, "a", 10)
      list:begin_update()
      list:dismissAll()
      assert.are.equal(10, list.content:get_height())
      list:end_update()
      assert.are.equal(1, list.content:get_height())
    end)
  end)

  describe("the scrollbar's room", function()
    it("only gives up width while the content overflows", function()
      local list = newList("glBar", {height = 40})
      local a = row(list, "a", 30)
      assert.are.equal(96, list.content:get_width())
      local b = row(list, "b", 30)
      assert.are.equal(80, list.content:get_width())
      assert.are.equal(80, a:get_width())
      b:delete()
      assert.are.equal(96, list.content:get_width())
      assert.are.equal(96, a:get_width())
    end)

    it("counts the frame when deciding whether content overflows", function()
      local list = newList("glBarFrame", {height = 40})
      local a = row(list, "a", 36)
      assert.are.equal(96, list.content:get_width())
      a:resize(nil, 38)
      assert.are.equal(80, list.content:get_width())
    end)

    it("follows the list being resized", function()
      local list = newList("glBarResize", {height = 40})
      row(list, "a", 60)
      assert.are.equal(80, list.content:get_width())
      list:resize(nil, 100)
      assert.are.equal(96, list.content:get_width())
      list:resize(nil, 40)
      assert.are.equal(80, list.content:get_width())
    end)

    it("takes its widths from constraints and setters, zero included", function()
      local list = newList("glBarWidths", {height = 40, frameWidth = 0, scrollbarWidth = 10})
      assert.are.equal(100, list.content:get_width())
      row(list, "a", 60)
      assert.are.equal(90, list.content:get_width())
      list:setScrollbarWidth(0)
      assert.are.equal(100, list.content:get_width())
      list:setScrollbarWidth(14)
      assert.are.equal(86, list.content:get_width())
      list:resize(nil, 100)
      list:setFrameWidth(8)
      assert.are.equal(92, list.content:get_width())
    end)
  end)

  describe("stripes", function()
    local A, B = "background-color: red;", "background-color: blue;"

    local function sheets(list)
      local result = {}
      for i, item in ipairs(list:getItems()) do
        result[i] = item.stylesheet
      end
      return result
    end

    it("rows take them in turn, and any number of them", function()
      local list = newList("glStripes", {stripes = {A, B}})
      row(list, "a")
      row(list, "b")
      row(list, "c")
      assert.are.same({A, B, A}, sheets(list))
      list:setStripes({A, B, "color: white;"})
      assert.are.same({A, B, "color: white;"}, sheets(list))
    end)

    it("stay in order as rows go, move and sort", function()
      local list = newList("glStripeMoves", {stripes = {A, B}})
      local a = row(list, "a")
      row(list, "b")
      row(list, "c")
      a:delete()
      assert.are.same({A, B}, sheets(list))
      assert.are.equal(A, list:getItem("b").stylesheet)
      list:moveItem("c", 1)
      assert.are.equal(A, list:getItem("c").stylesheet)
      assert.are.equal(B, list:getItem("b").stylesheet)
      list:sort()
      assert.are.equal(A, list:getItem("b").stylesheet)
      list:insertItem({name = "z", height = 10}, 1)
      assert.are.equal(A, list:getItem("z").stylesheet)
      assert.are.equal(B, list:getItem("b").stylesheet)
    end)

    it("catch up after an update", function()
      local list = newList("glStripeDefer", {stripes = {A, B}})
      local a = row(list, "a")
      row(list, "b")
      list:begin_update()
      a:delete()
      list:end_update()
      assert.are.same({A}, sheets(list))
    end)

    it("go under an item's own stylesheet, from its constraints or set later", function()
      local list = newList("glStripeOwn", {stripes = {A}})
      local a = row(list, "a", 10, {stylesheet = "color: white;"})
      assert.are.equal(A .. "color: white;", a.stylesheet)
      a:setStyleSheet("color: black;")
      assert.are.equal(A .. "color: black;", a.stylesheet)
      a:setStyleSheet("")
      assert.are.equal(A, a.stylesheet)
    end)

    it("get a semicolon if they need one", function()
      local list = newList("glStripeSemi", {stripes = {"color: red"}})
      local a = row(list, "a", 10, {stylesheet = "margin: 0;"})
      assert.are.equal("color: red;margin: 0;", a.stylesheet)
    end)

    it("can be taken away, leaving the item's own", function()
      local list = newList("glStripeOff", {stripes = {A}})
      local a = row(list, "a", 10, {stylesheet = "color: white;"})
      list:setStripes(nil)
      assert.are.equal("color: white;", a.stylesheet)
      row(list, "b")
      assert.are.equal("", list:getItem("b").stylesheet or "")
    end)

    it("go in a rule of their own when the item's stylesheet has rules", function()
      local list = newList("glStripeRules", {stripes = {A}})
      local own = "QLabel{color: white;} QLabel:hover{color: green;}"
      row(list, "glStripeRulesRow", 10, {stylesheet = own})
      assert.are.equal("QLabel{ " .. A .. " } " .. own, getLabelStyleSheet("glStripeRulesRow"))
    end)

    it("put the item's own in a rule when they have rules", function()
      local stripe = "QLabel{background-color: red;}"
      local list = newList("glStripeRuled", {stripes = {stripe}})
      row(list, "glStripeRuledRow", 10, {stylesheet = "color: white;"})
      assert.are.equal(stripe .. " QLabel{ color: white; }", getLabelStyleSheet("glStripeRuledRow"))
    end)

    it("stay behind when a row moves to a list without them", function()
      local from = newList("glStripeFrom", {stripes = {A}})
      local to = newList("glStripeTo")
      local a = row(from, "a", 10, {stylesheet = "color: white;"})
      a:changeContainer(to)
      assert.are.equal("color: white;", a.stylesheet)
    end)

    it("leave other windows alone", function()
      local list = newList("glStripeOther", {stripes = {A}})
      local label = Geyser.Label:new({name = "glStripeLabel", height = 10, stylesheet = "color: white;"}, list)
      row(list, "a")
      assert.are.equal("color: white;", label.stylesheet)
    end)
  end)

  describe("selection", function()
    local function selected(list)
      local result = {}
      for i, item in ipairs(list:getSelected()) do
        result[i] = item.name
      end
      return result
    end

    it("refuses selection by default, and leaves clicks alone", function()
      local list = newList("glSelNone")
      local a = row(list, "a")
      local ok, err = list:select(a)
      assert.is_nil(ok)
      assert.is_string(err)
      assert.is_false(list:isSelected(a))
      assert.is_nil(a.clickCallback)
    end)

    it("single mode keeps one, and clicking selects", function()
      local list = newList("glSelOne", {selectionMode = "single"})
      local a, b = row(list, "a"), row(list, "b")
      assert.is_function(a.clickCallback)
      a.clickCallback()
      assert.are.same({"a"}, selected(list))
      a.clickCallback()
      assert.are.same({"a"}, selected(list))
      b.clickCallback()
      assert.are.same({"b"}, selected(list))
    end)

    it("multi mode keeps many, in list order, and clicking toggles", function()
      local list = newList("glSelMany", {selectionMode = "multi"})
      local a, b = row(list, "a"), row(list, "b")
      b.clickCallback()
      a.clickCallback()
      assert.are.same({"a", "b"}, selected(list))
      a.clickCallback()
      assert.are.same({"b"}, selected(list))
      assert.is_true(b:isSelected())
    end)

    it("tells onSelectionChange what changed, and only that", function()
      local calls = {}
      local list = newList("glSelTell", {selectionMode = "single", onSelectionChange = function(l, item, on)
        calls[#calls + 1] = {l.name, item.name, on}
      end})
      local a, b = row(list, "a"), row(list, "b")
      list:select(a)
      list:select(a)
      list:select(b)
      list:deselect(b)
      list:deselect(b)
      assert.are.same({
        {"glSelTell", "a", true},
        {"glSelTell", "a", false},
        {"glSelTell", "b", true},
        {"glSelTell", "b", false},
      }, calls)
    end)

    it("doesn't select a row that onSelectionChange took away", function()
      local calls = {}
      local list
      list = newList("glSelTaken", {selectionMode = "single", onSelectionChange = function(_, item, on)
        calls[#calls + 1] = {item and item.name, on}
        if item and item.name == "a" and not on then
          list:getItem("b"):delete()
        end
      end})
      local a = row(list, "a")
      row(list, "b")
      list:select(a)
      local ok, err = list:select("b")
      assert.is_nil(ok)
      assert.is_string(err)
      assert.are.same({}, selected(list))
      assert.are.same({}, list.selected)
      assert.are.same({{"a", true}, {"a", false}}, calls)
    end)

    it("keeps single mode to one row when onSelectionChange selects another", function()
      local list
      list = newList("glSelSwap", {selectionMode = "single", onSelectionChange = function(_, item, on)
        if item.name == "a" and not on then
          list:select("c")
        end
      end})
      local a = row(list, "a")
      row(list, "b")
      row(list, "c")
      list:select(a)
      local ok, err = list:select("b")
      assert.is_nil(ok)
      assert.is_string(err)
      assert.are.same({"c"}, selected(list))
    end)

    it("leaves an item's own click callback alone", function()
      local list = newList("glSelOwn", {selectionMode = "single"})
      local own = function() end
      local a = row(list, "a", 10, {clickCallback = own})
      assert.are.equal(own, a.clickCallback)
    end)

    it("refuses a row from elsewhere", function()
      local list, other = newList("glSelHere", {selectionMode = "single"}), newList("glSelThere")
      local ok, err = list:select(row(other, "a"))
      assert.is_nil(ok)
      assert.is_string(err)
    end)

    it("forgets a row that goes", function()
      local list = newList("glSelGone", {selectionMode = "multi"})
      local a = row(list, "a")
      list:select(a)
      a:delete()
      assert.are.same({}, selected(list))
      assert.are.same({}, list.selected)
    end)

    it("clearSelection and toggle", function()
      local list = newList("glSelClear", {selectionMode = "multi"})
      local a, b = row(list, "a"), row(list, "b")
      assert.is_true(list:toggle(a))
      assert.is_false(list:toggle(a))
      list:select(a)
      list:select(b)
      list:clearSelection()
      assert.are.same({}, selected(list))
    end)

    it("setSelectionMode connects rows and trims the selection", function()
      local list = newList("glSelMode")
      local a, b = row(list, "a"), row(list, "b")
      assert.is_true(list:setSelectionMode("multi"))
      assert.is_function(a.clickCallback)
      list:select(a)
      list:select(b)
      list:setSelectionMode("single")
      assert.are.same({"a"}, selected(list))
      list:setSelectionMode("none")
      assert.are.same({}, selected(list))
      a.clickCallback()
      assert.are.same({}, selected(list))
      local ok, err = list:setSelectionMode("some")
      assert.is_nil(ok)
      assert.is_string(err)
    end)
  end)
end)
