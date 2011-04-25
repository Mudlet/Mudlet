--------------------------------------
--                                  --
-- The Geyser Layout Manager by guy --
--                                  --
-- TiledImage by beliar             --
--------------------------------------

--- Represents an image that is tiled over a window
-- @class table
-- @name Geyser.TiledImage
-- @field tile_image The path to the image that is to be tiled
-- @field tile_width The width of a single tile
-- @field tile_height The height of a single tile
-- @field tile_alignment Wheter the image should be tiled horizontally or vertically
-- @field start_tile_image Path to an image that should be used as the first tile.
--                      Leave empty if none should be used
-- @field start_tile_width Width if the first tile
-- @field start_tile_height Height of the first tile
-- @field end_tile_image Path to an image that should be used as the last tile.
--                      Leave empty if none should be used
-- @field end_tile_width Width if the last tile
-- @field end_tile_height Height of the last tile

Geyser.TileAlignment = {horizontal=0, vertical=1}
Geyser.TiledImage = Geyser.Window:new({
                name = "TiledImageClass",
                tile_image = "",
                tile_width = 1,
                tile_height = 1,
                tile_alignment = Geyser.TileAlignment.horizontal,
                start_tile_image = "",
                start_tile_width = 1,
                start_tile_height = 1,
                end_tile_image = "",
                end_tile_width = 1,
                end_tile_height = 1
        })

-- Save a reference to our parent constructor
Geyser.TiledImage.parent = Geyser.Window
       
--- Responsible for placing/moving/resizing this window to the correct place/size.
-- Called on window resize events.
function Geyser.TiledImage:reposition ()
        self.parent:reposition()
        self:create_tiles()
end

--- Create the tiles
function Geyser.TiledImage:create_tiles()
        local startPos = 0;
        local width = self:get_width()
        local height = self:get_height()
        local tile_size = 0
        local i = 1
        local use_start_tile = not (self.start_tile_image == "" or self.start_tile_image == nil)
        local use_end_tile = not (self.end_tile_image == "" or self.end_tile_image == nil)
        local size = 0
        local size_add = 0
        for _, image in ipairs(self.images) do
                image:hide()
        end
        self.images = {}
        if use_start_tile then
                local tile = Geyser.Label:new({
                                                                        name=self.name .. "_start_tile",
                                                                        x="0px", y="0px",
                                                                        width=self.start_tile_width, height=self.start_tile_height,
                                                                        },
                                                                        self)
                tile:setBackgroundImage(self.start_tile_image)
                table.insert(self.images, tile)
                if self.tile_alignment == Geyser.TileAlignment.horizontal then
                        startPos = startPos + self.start_tile_width
                        width = width - self.start_tile_width
                else
                        startPos = startPos + self.start_tile_height
                        height = height - self.start_tile_height
                end
        end
       
        if use_end_tile then
                if self.tile_alignment == Geyser.TileAlignment.horizontal then
                        width = width - self.end_tile_width
                else
                        height = height - self.end_tile_height
                end
        end
       
        if self.tile_alignment == Geyser.TileAlignment.horizontal then
                size = width
                size_add = self.tile_width
        else
                size = height
                size_add = self.tile_height
        end
        while startPos + size_add < size do
                local con = {}
                if self.tile_alignment == Geyser.TileAlignment.horizontal then
                        con = {
                                x = startPos, y="0px",
                                width=self.tile_width, height=self.tile_height,
                                name = self.name .. "_tile_" .. i
                                }
                else
                        con = {
                                x = "0px", y=startPos,
                                width=self.tile_width, height=self.tile_height,
                                name = self.name .. "_tile_" .. i
                                }
                end
                local tile = Geyser.Label:new(con, self)
                tile:setBackgroundImage(self.tile_image)
                table.insert(self.images, tile)
                startPos = startPos + size_add
                i = i + 1
        end
       
        local con = {}
        if self.tile_alignment == Geyser.TileAlignment.horizontal then
                local tile_width = width - startPos
                con = {
                        name = self.name .. "_tile_" .. i,
                        x = startPos, y="0px",
                        width=tile_width, height=self.tile_height,
                        }
                startPos = startPos + tile_width
        else
                local tile_height = height - startPos
                con = {
                        x = "0px", y=startPos,
                        width=self.tile_width, height=tile_height,
                        name = self.name .. "_tile_" .. i
                        }
                startPos = startPos + tile_height
        end
        local tile = Geyser.Label:new(con, self)
        tile:setBackgroundImage(self.tile_image)
        table.insert(self.images, tile)

        if use_end_tile then
                local con = {}
                if self.tile_alignment == Geyser.TileAlignment.horizontal then
                        con = {
                                        name=self.name .. "_end_tile",
                                        x=startPos, y="0px",
                                        width=self.end_tile_width, height=self.end_tile_height,
                                        }
                else
                        con = {
                                        name=self.name .. "_end_tile",
                                        x="0px", y=startPos,
                                        width=self.end_tile_width, height=self.end_tile_height,
                                        }
                end
                local tile = Geyser.Label:new(con,self)
                tile:setBackgroundImage(self.end_tile_image)
                table.insert(self.images, tile)
        end
end

function Geyser.TiledImage:new (cons, container)
        -- Initiate and set Window specific things
        cons = cons or {}
        cons.type = cons.type or "tiled_image"

        -- Call parent's constructor
        local me = self.parent:new(cons, container)
        me.images = {}
        -- Set the metatable.
        setmetatable(me, self)
        self.__index = self
        --print(" New in " .. self.name .. " : " .. me.name)
        me:create_tiles()
        return me
end