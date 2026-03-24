-- Tests for OSC 8 hyperlink context menu title feature
-- Verifies that OSC 8 links with JSON title configs are correctly processed
-- without crashes and that the link text appears in the buffer.
--

describe("Tests OSC 8 hyperlink title feature", function()
	local ESC = "\27"
	local OSC8_START = ESC .. "]8;;"
	local OSC8_END = ESC .. "]8;;" .. ESC .. "\\"
	local ST = ESC .. "\\"

	-- Helper: wrap text in an OSC 8 hyperlink with optional ANSI color
	local function osc8(url, text, ansiColor)
		ansiColor = ansiColor or "36"
		return OSC8_START .. url .. ST
			.. ESC .. "[" .. ansiColor .. "m" .. text .. ESC .. "[0m"
			.. OSC8_END
	end

	-- Helper: append JSON config as query param
	local function withConfig(baseUrl, config)
		return baseUrl .. "?config=" .. config
	end

	describe("title parsing via feedTriggers", function()
		it("should process a simple string title without crashing", function()
			local config =
			'{"title":"Lamb and Barley Stew","menu":[{"View Details":"send:look stew"},{"Buy":"send:buy stew"},{"Taste":"send:taste stew"}]}'
			local line = osc8(withConfig("send:look stew", config), "[Lamb and Barley Stew]", "33") .. "\n"

			local ok, err = pcall(feedTriggers, line)
			assert.is_true(ok, "Simple string title should not crash: " .. tostring(err))
		end)

		it("should process a styled title object with bold and custom color", function()
			local config =
			'{"title":{"text":"Magic Shop - Potions","style":{"color":"#ffd700","bold":true}},"menu":[{"Buy Health Potion":"send:buy health potion"},{"Buy Mana Potion":"send:buy mana potion"},{"Browse All":"send:list potions"}]}'
			local line = osc8(withConfig("send:list potions", config), "[Magic Shop]", "35") .. "\n"

			local ok, err = pcall(feedTriggers, line)
			assert.is_true(ok, "Styled title object should not crash: " .. tostring(err))
		end)

		it("should process a title with italic style and background color", function()
			local config =
			'{"title":{"text":"Sir Galahad the Brave","style":{"color":"#ffffff","bg":"#333333","italic":true}},"menu":[{"Talk":"send:talk galahad"},{"Attack":"send:attack galahad"},{"Inspect":"send:look galahad"}]}'
			local line = osc8(withConfig("send:look galahad", config), "[Sir Galahad]", "32") .. "\n"

			local ok, err = pcall(feedTriggers, line)
			assert.is_true(ok, "Italic title with background color should not crash: " .. tostring(err))
		end)

		it("should process compact syntax with 'ti' shorthand for title", function()
			local config =
			'{"ti":"Rusty Sword","m":[{"Equip":"send:wield sword"},{"Drop":"send:drop sword"},{"Examine":"send:examine sword"}]}'
			local line = osc8(withConfig("send:examine sword", config), "[Rusty Sword]", "31") .. "\n"

			local ok, err = pcall(feedTriggers, line)
			assert.is_true(ok, "Compact syntax with 'ti' shorthand should not crash: " .. tostring(err))
		end)

		it("should process a menu without title (no header expected)", function()
			local config = '{"menu":[{"North":"send:north"},{"South":"send:south"},{"East":"send:east"}]}'
			local line = osc8(withConfig("send:north", config), "[Exits]", "34") .. "\n"

			local ok, err = pcall(feedTriggers, line)
			assert.is_true(ok, "Menu without title should not crash: " .. tostring(err))
		end)

		it("should process a title without menu (no popup expected)", function()
			local config = '{"title":"Lonely Title"}'
			local line = osc8(withConfig("send:test", config), "[No Menu Link]", "37") .. "\n"

			local ok, err = pcall(feedTriggers, line)
			assert.is_true(ok, "Title without menu should not crash: " .. tostring(err))
		end)
	end)

	describe("link text appears correctly in buffer", function()
		it("should display link text for simple string title config", function()
			local config =
			'{"title":"Lamb and Barley Stew","menu":[{"View Details":"send:look stew"}]}'
			local line = osc8(withConfig("send:look stew", config), "[Lamb and Barley Stew]", "33") .. "\n"

			feedTriggers(line)

			local found = selectString("[Lamb and Barley Stew]", 1)
			assert.is_true(found ~= -1, "Link text should be found in the buffer")
		end)

		it("should display link text for styled title config", function()
			local config =
			'{"title":{"text":"Magic Shop - Potions","style":{"color":"#ffd700","bold":true}},"menu":[{"Buy Health Potion":"send:buy health potion"}]}'
			local line = osc8(withConfig("send:list potions", config), "[Magic Shop]", "35") .. "\n"

			feedTriggers(line)

			local found = selectString("[Magic Shop]", 1)
			assert.is_true(found ~= -1, "Styled title link text should be found in the buffer")
		end)

		it("should display link text for compact syntax config", function()
			local config =
			'{"ti":"Rusty Sword","m":[{"Equip":"send:wield sword"}]}'
			local line = osc8(withConfig("send:examine sword", config), "[Rusty Sword]", "31") .. "\n"

			feedTriggers(line)

			local found = selectString("[Rusty Sword]", 1)
			assert.is_true(found ~= -1, "Compact syntax link text should be found in the buffer")
		end)

		it("should display link text for menu-only config (no title)", function()
			local config = '{"menu":[{"North":"send:north"},{"South":"send:south"}]}'
			local line = osc8(withConfig("send:north", config), "[Exits]", "34") .. "\n"

			feedTriggers(line)

			local found = selectString("[Exits]", 1)
			assert.is_true(found ~= -1, "Menu-only link text should be found in the buffer")
		end)

		it("should display link text for title-only config (no menu)", function()
			local config = '{"title":"Lonely Title"}'
			local line = osc8(withConfig("send:test", config), "[No Menu Link]", "37") .. "\n"

			feedTriggers(line)

			local found = selectString("[No Menu Link]", 1)
			assert.is_true(found ~= -1, "Title-only link text should be found in the buffer")
		end)
	end)

	describe("title with various style properties", function()
		it("should handle title with all text decoration properties", function()
			local config =
			'{"title":{"text":"Decorated Title","style":{"color":"#ff0000","bold":true,"italic":true,"underline":true,"strikethrough":true}},"menu":[{"Action":"send:action"}]}'
			local line = osc8(withConfig("send:action", config), "[Decorated]", "36") .. "\n"

			local ok, err = pcall(feedTriggers, line)
			assert.is_true(ok, "Title with all decorations should not crash: " .. tostring(err))
		end)

		it("should handle title with underline style variants", function()
			local variants = {
				{ name = "wavy",   style = '{"color":"#00ff00","underline":"wavy"}' },
				{ name = "dotted", style = '{"color":"#00ff00","underline":"dotted"}' },
				{ name = "dashed", style = '{"color":"#00ff00","underline":"dashed"}' },
			}

			for _, variant in ipairs(variants) do
				local config = '{"title":{"text":"' ..
				variant.name .. ' underline","style":' .. variant.style .. '},"menu":[{"Test":"send:test"}]}'
				local line = osc8(withConfig("send:test", config), "[" .. variant.name .. "]", "36") .. "\n"

				local ok, err = pcall(feedTriggers, line)
				assert.is_true(ok, variant.name .. " underline title should not crash: " .. tostring(err))
			end
		end)

		it("should handle title with background-color CSS property name", function()
			local config =
			'{"title":{"text":"CSS BG Title","style":{"color":"#ffffff","background-color":"#660000"}},"menu":[{"Action":"send:action"}]}'
			local line = osc8(withConfig("send:action", config), "[CSS BG]", "36") .. "\n"

			local ok, err = pcall(feedTriggers, line)
			assert.is_true(ok, "Title with background-color CSS property should not crash: " .. tostring(err))
		end)

		it("should handle title with text-decoration-color", function()
			local config =
			'{"title":{"text":"Color Decoration","style":{"color":"#ffffff","underline":true,"text-decoration-color":"#ff00ff"}},"menu":[{"Action":"send:action"}]}'
			local line = osc8(withConfig("send:action", config), "[Deco Color]", "36") .. "\n"

			local ok, err = pcall(feedTriggers, line)
			assert.is_true(ok, "Title with text-decoration-color should not crash: " .. tostring(err))
		end)
	end)

	describe("title edge cases", function()
		it("should handle empty title string gracefully", function()
			local config = '{"title":"","menu":[{"Action":"send:action"}]}'
			local line = osc8(withConfig("send:action", config), "[Empty Title]", "36") .. "\n"

			local ok, err = pcall(feedTriggers, line)
			assert.is_true(ok, "Empty title string should not crash: " .. tostring(err))
		end)

		it("should handle title object with empty text", function()
			local config = '{"title":{"text":"","style":{"color":"#ff0000"}},"menu":[{"Action":"send:action"}]}'
			local line = osc8(withConfig("send:action", config), "[Empty Obj Title]", "36") .. "\n"

			local ok, err = pcall(feedTriggers, line)
			assert.is_true(ok, "Title object with empty text should not crash: " .. tostring(err))
		end)

		it("should handle title object without text key", function()
			local config = '{"title":{"style":{"color":"#ff0000"}},"menu":[{"Action":"send:action"}]}'
			local line = osc8(withConfig("send:action", config), "[No Text Key]", "36") .. "\n"

			local ok, err = pcall(feedTriggers, line)
			assert.is_true(ok, "Title object without text key should not crash: " .. tostring(err))
		end)

		it("should handle title object without style key", function()
			local config = '{"title":{"text":"Style-less Title"},"menu":[{"Action":"send:action"}]}'
			local line = osc8(withConfig("send:action", config), "[No Style]", "36") .. "\n"

			local ok, err = pcall(feedTriggers, line)
			assert.is_true(ok, "Title object without style key should not crash: " .. tostring(err))
		end)

		it("should handle title with unicode characters", function()
			local config = '{"title":"Potion du Guerrier","menu":[{"Drink":"send:drink potion"}]}'
			local line = osc8(withConfig("send:drink potion", config), "[Potion]", "36") .. "\n"

			local ok, err = pcall(feedTriggers, line)
			assert.is_true(ok, "Title with unicode should not crash: " .. tostring(err))
		end)

		it("should handle title with special characters in text", function()
			local config = '{"title":"Item <Rare> [+5] & More!","menu":[{"Use":"send:use item"}]}'
			local line = osc8(withConfig("send:use item", config), "[Special Chars]", "36") .. "\n"

			local ok, err = pcall(feedTriggers, line)
			assert.is_true(ok, "Title with special characters should not crash: " .. tostring(err))
		end)

		it("should handle title alongside other config properties", function()
			local config =
			'{"title":"Full Config","tooltip":"A helpful tooltip","style":{"color":"#00ffff"},"menu":[{"Action 1":"send:action1"},{"Action 2":"send:action2"}]}'
			local line = osc8(withConfig("send:action1", config), "[Full Config]", "36") .. "\n"

			local ok, err = pcall(feedTriggers, line)
			assert.is_true(ok, "Title alongside other config properties should not crash: " .. tostring(err))
		end)

		it("should handle numeric title value gracefully (invalid type)", function()
			local config = '{"title":42,"menu":[{"Action":"send:action"}]}'
			local line = osc8(withConfig("send:action", config), "[Num Title]", "36") .. "\n"

			local ok, err = pcall(feedTriggers, line)
			assert.is_true(ok, "Numeric title value should not crash: " .. tostring(err))
		end)

		it("should handle boolean title value gracefully (invalid type)", function()
			local config = '{"title":true,"menu":[{"Action":"send:action"}]}'
			local line = osc8(withConfig("send:action", config), "[Bool Title]", "36") .. "\n"

			local ok, err = pcall(feedTriggers, line)
			assert.is_true(ok, "Boolean title value should not crash: " .. tostring(err))
		end)

		it("should handle array title value gracefully (invalid type)", function()
			local config = '{"title":["a","b"],"menu":[{"Action":"send:action"}]}'
			local line = osc8(withConfig("send:action", config), "[Array Title]", "36") .. "\n"

			local ok, err = pcall(feedTriggers, line)
			assert.is_true(ok, "Array title value should not crash: " .. tostring(err))
		end)
	end)
end)
