describe("Tests the properties a map gives its areas", function()
  local continent, zone, other
  local names = {"FieldSpecContinent", "FieldSpecZone", "FieldSpecOther"}
  local savedFields

  local fields = {
    levelRange = {name = "Level range", type = "range", inherit = true},
    noMagic = {type = "yesno", inherit = true},
    notes = {type = "text"},
    population = {type = "number"},
    terrain = {type = "choice", choices = {"forest", "desert", "swamp"}},
  }

  setup(function()
    savedFields = getMapAreaFields()
    continent = addAreaName(names[1])
    zone = addAreaName(names[2])
    other = addAreaName(names[3])
    assert.is_true(setAreaParent(zone, continent))
  end)

  teardown(function()
    setMapAreaFields(savedFields or {})
    for _, name in ipairs(names) do
      deleteArea(name)
    end
  end)

  before_each(function()
    assert.is_true(setMapAreaFields(fields))
    for _, id in ipairs({continent, zone, other}) do
      for key in pairs(fields) do
        setAreaFieldValue(id, key, nil)
      end
    end
  end)

  describe("setMapAreaFields and getMapAreaFields", function()
    it("reads the fields back, with the key as the name where none is given", function()
      local read = getMapAreaFields()
      assert.are.same({name = "Level range", type = "range", inherit = true}, read.levelRange)
      assert.are.same({name = "noMagic", type = "yesno", inherit = true}, read.noMagic)
      assert.are.same({name = "notes", type = "text", inherit = false}, read.notes)
      assert.are.same({name = "terrain", type = "choice", inherit = false, choices = {"forest", "desert", "swamp"}}, read.terrain)
    end)

    it("replaces all of the fields, and an empty table removes them", function()
      assert.is_true(setMapAreaFields({notes = {type = "text"}}))
      assert.are.same({"notes"}, table.keys(getMapAreaFields()))
      assert.is_true(setMapAreaFields({}))
      assert.are.same({}, getMapAreaFields())
    end)

    it("keeps the field definitions in the map's user data", function()
      assert.is_string(getMapUserData("system.areaFields"))
    end)

    it("refuses a definition it cannot use, and keeps the fields it had", function()
      local refusals = {
        {bad = {type = "colour"}},
        {bad = {}},
        {bad = "text"},
        {["system.bad"] = {type = "text"}},
        {bad = {type = "choice"}},
        {bad = {type = "choice", choices = {}}},
        {bad = {type = "choice", choices = {"a", "a"}}},
        {bad = {type = "choice", choices = {"a", 2}}},
        {bad = {type = "text", choices = {"a"}}},
        {bad = {type = "text", name = 5}},
        {bad = {type = "text", inherit = "yes"}},
      }
      for _, definition in ipairs(refusals) do
        local ok, message = setMapAreaFields(definition)
        assert.is_nil(ok)
        assert.is_string(message)
      end
      assert.are.same(fields.terrain.choices, getMapAreaFields().terrain.choices)
    end)

    it("raises an error for fields that are not a table", function()
      assert.has_error(function() setMapAreaFields("notes") end)
    end)
  end)

  describe("setAreaFieldValue and getAreaFieldValue", function()
    it("keeps each type's value and gives it back as that type, with the area it is from", function()
      assert.is_true(setAreaFieldValue(zone, "notes", "Orcs at night"))
      assert.is_true(setAreaFieldValue(zone, "population", 1200))
      assert.is_true(setAreaFieldValue(zone, "levelRange", {10, 20}))
      assert.is_true(setAreaFieldValue(zone, "noMagic", false))
      assert.is_true(setAreaFieldValue(zone, "terrain", "swamp"))

      assert.are.same({"Orcs at night", zone}, {getAreaFieldValue(zone, "notes")})
      assert.are.same({1200, zone}, {getAreaFieldValue(zone, "population")})
      assert.are.same({{10, 20}, zone}, {getAreaFieldValue(zone, "levelRange")})
      assert.are.same({false, zone}, {getAreaFieldValue(zone, "noMagic")})
      assert.are.same({"swamp", zone}, {getAreaFieldValue(zone, "terrain")})
    end)

    it("keeps the values as text in the area's user data, where a script's own values are read too", function()
      setAreaFieldValue(zone, "levelRange", {10, 20})
      assert.are.equal("10-20", getAreaUserData(zone, "levelRange"))
      setAreaUserData(other, "levelRange", "-5-5")
      setAreaUserData(other, "noMagic", "yes")
      assert.are.same({-5, 5}, (getAreaFieldValue(other, "levelRange")))
      assert.is_true((getAreaFieldValue(other, "noMagic")))
    end)

    it("reads back exactly the numbers it keeps, the largest whole ones and the smallest ranges included", function()
      for _, number in ipairs({9007199254740991, -9007199254740991, 0.1, 1 / 3, 1e300, 12.5}) do
        assert.is_true(setAreaFieldValue(zone, "population", number))
        assert.are.equal(number, (getAreaFieldValue(zone, "population")))
      end
      setAreaFieldValue(zone, "population", 9007199254740991)
      assert.are.equal("9007199254740991", getAreaUserData(zone, "population"))
      setAreaFieldValue(zone, "population", 0.1)
      assert.are.equal("0.1", getAreaUserData(zone, "population"))

      for _, range in ipairs({{1e-8, 1e-7}, {-1e-8, 1e300}, {-5, 5}, {0.1, 0.2}}) do
        assert.is_true(setAreaFieldValue(zone, "levelRange", range))
        assert.are.same(range, (getAreaFieldValue(zone, "levelRange")))
      end
    end)

    it("refuses a value that does not fit the field's type", function()
      local refusals = {
        {"notes", 5},
        {"population", "many"},
        {"levelRange", {20, 10}},
        {"levelRange", {10}},
        {"levelRange", "10-20"},
        {"noMagic", "true"},
        {"terrain", "mountains"},
      }
      for _, refusal in ipairs(refusals) do
        local ok, message = setAreaFieldValue(zone, refusal[1], refusal[2])
        assert.is_nil(ok, refusal[1])
        assert.is_string(message)
      end
      assert.is_nil(getAreaFieldValue(zone, "notes"))
    end)

    it("removes a value given nil", function()
      setAreaFieldValue(zone, "notes", "gone soon")
      assert.is_true(setAreaFieldValue(zone, "notes", nil))
      assert.is_nil(getAreaFieldValue(zone, "notes"))
      assert.is_nil(getAreaUserData(zone, "notes"))
    end)

    it("takes an inherited field's value from the nearest parent area that has one", function()
      setAreaFieldValue(continent, "noMagic", true)
      setAreaFieldValue(continent, "levelRange", {1, 50})
      setAreaFieldValue(zone, "levelRange", {10, 20})

      assert.are.same({true, continent}, {getAreaFieldValue(zone, "noMagic")})
      assert.are.same({{10, 20}, zone}, {getAreaFieldValue(zone, "levelRange")})
    end)

    it("does not take a field's value from a parent area unless the field is inherited", function()
      setAreaFieldValue(continent, "notes", "the continent's own")
      assert.is_nil(getAreaFieldValue(zone, "notes"))
    end)

    it("returns nil and a message for text in user data that does not fit the type", function()
      setAreaUserData(zone, "population", "lots")
      local value, message = getAreaFieldValue(zone, "population")
      assert.is_nil(value)
      assert.is_truthy(message:find("lots", 1, true))
    end)

    it("returns nil and a message for a field the map does not have, or an area that does not exist", function()
      assert.is_nil(getAreaFieldValue(zone, "unknownField"))
      assert.is_nil(setAreaFieldValue(zone, "unknownField", "x"))
      assert.is_nil(getAreaFieldValue(990000004, "notes"))
      assert.is_nil(setAreaFieldValue(990000004, "notes", "x"))
    end)
  end)
end)
