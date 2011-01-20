--------------------------------------
--                                  --
-- The Geyser Layout Manager by guy --
--                                  --
--------------------------------------

-- TESTS FOR GEYSER --

--- Test labels.  Creates 101 Labels of varying hue and transparency.
function Geyser.testLabels()
   labelTestContainer = Geyser.Container:new({x=0,y=0,width="100%",height="100%",name="labelTestContainer"})

   for i=0,90,10 do
      for j=0,90,10 do
         local myName = "TestLabel_" .. i .. "_" .. j
         local l = Geyser.Label:new({
                               name = myName,
                               x = i .. "%", y = j .. "%",
                               width = "8%", height = "8%",
                               message = myName,
                               fgColor = "white"},
                               labelTestContainer)
         l:setColor(i,j,100, i*j/8100 * 128 + 127)
      end
   end
   Geyser.Label:new({
               name = "Clickable",
               fontSize = 22,
               message = [[<p style="font-size:22pt"><center>Click me</center></p>]],
               x = "53%", y = "53%",
               width = "-40px", height = "-3c",
               backgroundAlpha = 240,
               callback = "echo",
               args = "You Clicked Me!!\n"},
               labelTestContainer)
end



--- Test gauges.  Creates 100 gauges of varying fullness.
function Geyser.testGauges()
   gaugeTestContainer = Geyser.Container:new({x=0,y=0,width="100%",height="100%",name="gaugeTestContainer"})

   for i=0,90,10 do
   for j=0,90,10 do
      local myName = "TestGauge_" .. i .. "_" .. j
      local g = Geyser.Gauge:new({name = myName, x = i .. "%", y = j .. "%", width = "9%", height = "9%",
                                  message = myName, fgColor = "red",},
                                 gaugeTestContainer)
      g:setValue(1.0 * i)
      --g:setText(myName)
   end
   end
end

--- Used by Geyser.demo1(). Sorry about the namespace polution,
-- but callbacks can't use functions contained in a table.  Anyway,
-- it just echoes some status info to the two miniconsoles.
function demoCallback1 (value)
	local g1 = geyserDemoContainer.windowList.myGauge
	local g2 = geyserDemoContainer.windowList.myGoofyGauge
	local c1 = geyserDemoContainer.windowList["console 1"]
	local c2 = geyserDemoContainer.windowList["console 2"]
	g1:setValue((g1.value - value) % 100)
	g2:setValue((g2.value - 2*value) % 100)
	local v1 = tostring(g1.value)
	local v2 = tostring(g2.value)
	c1:decho(string.format("<90,90,%d> You are at %d \n", v1, v1))
	c2:decho(string.format("<90,%d,%d> You are at %d \n", 255-v2, v2, v2))
end

local myDemoState = 0

--- Used by Geyser.demo1(). It moves the location of the
-- container window.  Look how easy it is to shift around all those
-- gui elements!
function demoCallback2 ()
   if myDemoState == 0 then
		geyserDemoContainer:resize(nil, "50%")
		geyserDemoContainer:move("-90c")
		myDemoState = 1
	else
		geyserDemoContainer:resize(nil, "100%")
		geyserDemoContainer:move("-42c")
		myDemoState = 0
	end
end		


--- This demonstrates some of the neat features in Geyser.
function Geyser.demo1()
   ----------------------------------------
   -- 1. Create a container on the left side of the screen wide
   -- enough for 40 characters

   geyserDemoContainer = Geyser.Container:new(
      { x = "-42c", y = "0px",
        width = "40c", height = "100%",
        name = "myContainer"})


   ----------------------------------------
   -- 2. Add a miniconsole to the top of the container,
   -- full width, with 20 lines of text

   local console1 = Geyser.MiniConsole:new(
      { x = "0px", y = "0px",
        width = "100%", height = "20c",
        name = "console 1"},
      geyserDemoContainer) -- add console1 to container directly

   console1:setColor(80,40,20,255)
   console1:echo("Go ahead - resize the Mudlet window\n")


   ----------------------------------------
   -- 3. Add two gauges and a clickable label just below the
   -- miniconsole

   local g1 = Geyser.Gauge:new(
      { x = "0px", y = console1.height,
        width = "60%", height = "4c",
        orientation = "batty", color = "violet",
        name = "myGauge"})
   g1:setValue(77)

   geyserDemoContainer:add(g1) -- add gauge after creation

   local g2 = Geyser.Gauge:new(
      { x = "10%", y = console1.height,
        width = "50%", height = "4c",
        orientation = "goofy",
        name = "myGoofyGauge"}, geyserDemoContainer) -- add gauge directly
   g2:setValue(55)
   g2:setColor("cyan")


   local label1 = Geyser.Label:new(
      { x = g1.width, y = console1.height,
        width = "40%", height = "4c",
        name = "label with callback",
        callback = "demoCallback1",
        args=5, fontSize = 8, fgColor = "white",
        message = "Click me"})

   geyserDemoContainer:add(label1) -- you know the drill

   ----------------------------------------
   -- 4. Add another miniconsole below the gauge that extends
   -- to the bottom of the screen and wraps at 40 characters and
   -- another clickable label.
echo("adfa\n")
   local console2 = Geyser.MiniConsole:new({
        x = "0px", y = "24c",
        width = "100%", height = "-0px",
        name = "console 2",
        wrapAt = 40,
        color = "<10,20,40>",},
        geyserDemoContainer) -- add console2 to geyserDemoContainer directly

   console2:echo("Vampire Gauges!\n")

   local label2 = Geyser.Label:new(
      { x = g1.width, y = "60%",
        width = "40%", height = "4c",
        name = "label with another callback and a long name!",
        callback = "demoCallback2",
        message = [[<center>and me</center>]]})

   geyserDemoContainer:add(label2) -- same here


   ----------------------------------------
   -- 5. hide all windows just created

   geyserDemoContainer:hide()


   ----------------------------------------
   -- 6. now show them again

   geyserDemoContainer:show()
end
