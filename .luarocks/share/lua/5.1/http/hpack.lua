-- This module implements HPACK - Header Compression for HTTP/2
-- Reference documentation: https://http2.github.io/http2-spec/compression.html

local schar = string.char
local spack = string.pack or require "compat53.string".pack -- luacheck: ignore 143
local sunpack = string.unpack or require "compat53.string".unpack -- luacheck: ignore 143
local band = require "http.bit".band
local bor = require "http.bit".bor
local new_headers = require "http.headers".new
local unpack = table.unpack or unpack -- luacheck: ignore 113 143
local h2_errors = require "http.h2_error".errors

-- Section 5.1
local function encode_integer(i, prefix_len, mask)
	assert(i >= 0 and i % 1 == 0)
	assert(prefix_len >= 0 and prefix_len <= 8 and prefix_len % 1 == 0)
	assert(mask >= 0 and mask <= 256 and mask % 1 == 0)
	if i < 2^prefix_len then
		return schar(bor(mask, i))
	else
		local prefix_mask = 2^prefix_len-1
		local chars = {
			bor(prefix_mask, mask);
		}
		local j = 2
		i = i - prefix_mask
		while i >= 128 do
			chars[j] = i % 128 + 128
			j = j + 1
			i = math.floor(i / 128)
		end
		chars[j] = i
		return schar(unpack(chars, 1, j))
	end
end

local function decode_integer(str, prefix_len, pos)
	pos = pos or 1
	local prefix_mask = 2^prefix_len-1
	if pos > #str then return end
	local I = band(prefix_mask, str:byte(pos, pos))
	if I == prefix_mask then
		local M = 0
		repeat
			pos = pos + 1
			if pos > #str then return end
			local B = str:byte(pos, pos)
			I = I + band(B, 127) * 2^M
			M = M + 7
		until band(B, 128) ~= 128
	end
	return I, pos+1
end

local huffman_decode, huffman_encode
do
	local huffman_codes = {
		[  0] = "1111111111000";
		[  1] = "11111111111111111011000";
		[  2] = "1111111111111111111111100010";
		[  3] = "1111111111111111111111100011";
		[  4] = "1111111111111111111111100100";
		[  5] = "1111111111111111111111100101";
		[  6] = "1111111111111111111111100110";
		[  7] = "1111111111111111111111100111";
		[  8] = "1111111111111111111111101000";
		[  9] = "111111111111111111101010";
		[ 10] = "111111111111111111111111111100";
		[ 11] = "1111111111111111111111101001";
		[ 12] = "1111111111111111111111101010";
		[ 13] = "111111111111111111111111111101";
		[ 14] = "1111111111111111111111101011";
		[ 15] = "1111111111111111111111101100";
		[ 16] = "1111111111111111111111101101";
		[ 17] = "1111111111111111111111101110";
		[ 18] = "1111111111111111111111101111";
		[ 19] = "1111111111111111111111110000";
		[ 20] = "1111111111111111111111110001";
		[ 21] = "1111111111111111111111110010";
		[ 22] = "111111111111111111111111111110";
		[ 23] = "1111111111111111111111110011";
		[ 24] = "1111111111111111111111110100";
		[ 25] = "1111111111111111111111110101";
		[ 26] = "1111111111111111111111110110";
		[ 27] = "1111111111111111111111110111";
		[ 28] = "1111111111111111111111111000";
		[ 29] = "1111111111111111111111111001";
		[ 30] = "1111111111111111111111111010";
		[ 31] = "1111111111111111111111111011";
		[ 32] = "010100";
		[ 33] = "1111111000";
		[ 34] = "1111111001";
		[ 35] = "111111111010";
		[ 36] = "1111111111001";
		[ 37] = "010101";
		[ 38] = "11111000";
		[ 39] = "11111111010";
		[ 40] = "1111111010";
		[ 41] = "1111111011";
		[ 42] = "11111001";
		[ 43] = "11111111011";
		[ 44] = "11111010";
		[ 45] = "010110";
		[ 46] = "010111";
		[ 47] = "011000";
		[ 48] = "00000";
		[ 49] = "00001";
		[ 50] = "00010";
		[ 51] = "011001";
		[ 52] = "011010";
		[ 53] = "011011";
		[ 54] = "011100";
		[ 55] = "011101";
		[ 56] = "011110";
		[ 57] = "011111";
		[ 58] = "1011100";
		[ 59] = "11111011";
		[ 60] = "111111111111100";
		[ 61] = "100000";
		[ 62] = "111111111011";
		[ 63] = "1111111100";
		[ 64] = "1111111111010";
		[ 65] = "100001";
		[ 66] = "1011101";
		[ 67] = "1011110";
		[ 68] = "1011111";
		[ 69] = "1100000";
		[ 70] = "1100001";
		[ 71] = "1100010";
		[ 72] = "1100011";
		[ 73] = "1100100";
		[ 74] = "1100101";
		[ 75] = "1100110";
		[ 76] = "1100111";
		[ 77] = "1101000";
		[ 78] = "1101001";
		[ 79] = "1101010";
		[ 80] = "1101011";
		[ 81] = "1101100";
		[ 82] = "1101101";
		[ 83] = "1101110";
		[ 84] = "1101111";
		[ 85] = "1110000";
		[ 86] = "1110001";
		[ 87] = "1110010";
		[ 88] = "11111100";
		[ 89] = "1110011";
		[ 90] = "11111101";
		[ 91] = "1111111111011";
		[ 92] = "1111111111111110000";
		[ 93] = "1111111111100";
		[ 94] = "11111111111100";
		[ 95] = "100010";
		[ 96] = "111111111111101";
		[ 97] = "00011";
		[ 98] = "100011";
		[ 99] = "00100";
		[100] = "100100";
		[101] = "00101";
		[102] = "100101";
		[103] = "100110";
		[104] = "100111";
		[105] = "00110";
		[106] = "1110100";
		[107] = "1110101";
		[108] = "101000";
		[109] = "101001";
		[110] = "101010";
		[111] = "00111";
		[112] = "101011";
		[113] = "1110110";
		[114] = "101100";
		[115] = "01000";
		[116] = "01001";
		[117] = "101101";
		[118] = "1110111";
		[119] = "1111000";
		[120] = "1111001";
		[121] = "1111010";
		[122] = "1111011";
		[123] = "111111111111110";
		[124] = "11111111100";
		[125] = "11111111111101";
		[126] = "1111111111101";
		[127] = "1111111111111111111111111100";
		[128] = "11111111111111100110";
		[129] = "1111111111111111010010";
		[130] = "11111111111111100111";
		[131] = "11111111111111101000";
		[132] = "1111111111111111010011";
		[133] = "1111111111111111010100";
		[134] = "1111111111111111010101";
		[135] = "11111111111111111011001";
		[136] = "1111111111111111010110";
		[137] = "11111111111111111011010";
		[138] = "11111111111111111011011";
		[139] = "11111111111111111011100";
		[140] = "11111111111111111011101";
		[141] = "11111111111111111011110";
		[142] = "111111111111111111101011";
		[143] = "11111111111111111011111";
		[144] = "111111111111111111101100";
		[145] = "111111111111111111101101";
		[146] = "1111111111111111010111";
		[147] = "11111111111111111100000";
		[148] = "111111111111111111101110";
		[149] = "11111111111111111100001";
		[150] = "11111111111111111100010";
		[151] = "11111111111111111100011";
		[152] = "11111111111111111100100";
		[153] = "111111111111111011100";
		[154] = "1111111111111111011000";
		[155] = "11111111111111111100101";
		[156] = "1111111111111111011001";
		[157] = "11111111111111111100110";
		[158] = "11111111111111111100111";
		[159] = "111111111111111111101111";
		[160] = "1111111111111111011010";
		[161] = "111111111111111011101";
		[162] = "11111111111111101001";
		[163] = "1111111111111111011011";
		[164] = "1111111111111111011100";
		[165] = "11111111111111111101000";
		[166] = "11111111111111111101001";
		[167] = "111111111111111011110";
		[168] = "11111111111111111101010";
		[169] = "1111111111111111011101";
		[170] = "1111111111111111011110";
		[171] = "111111111111111111110000";
		[172] = "111111111111111011111";
		[173] = "1111111111111111011111";
		[174] = "11111111111111111101011";
		[175] = "11111111111111111101100";
		[176] = "111111111111111100000";
		[177] = "111111111111111100001";
		[178] = "1111111111111111100000";
		[179] = "111111111111111100010";
		[180] = "11111111111111111101101";
		[181] = "1111111111111111100001";
		[182] = "11111111111111111101110";
		[183] = "11111111111111111101111";
		[184] = "11111111111111101010";
		[185] = "1111111111111111100010";
		[186] = "1111111111111111100011";
		[187] = "1111111111111111100100";
		[188] = "11111111111111111110000";
		[189] = "1111111111111111100101";
		[190] = "1111111111111111100110";
		[191] = "11111111111111111110001";
		[192] = "11111111111111111111100000";
		[193] = "11111111111111111111100001";
		[194] = "11111111111111101011";
		[195] = "1111111111111110001";
		[196] = "1111111111111111100111";
		[197] = "11111111111111111110010";
		[198] = "1111111111111111101000";
		[199] = "1111111111111111111101100";
		[200] = "11111111111111111111100010";
		[201] = "11111111111111111111100011";
		[202] = "11111111111111111111100100";
		[203] = "111111111111111111111011110";
		[204] = "111111111111111111111011111";
		[205] = "11111111111111111111100101";
		[206] = "111111111111111111110001";
		[207] = "1111111111111111111101101";
		[208] = "1111111111111110010";
		[209] = "111111111111111100011";
		[210] = "11111111111111111111100110";
		[211] = "111111111111111111111100000";
		[212] = "111111111111111111111100001";
		[213] = "11111111111111111111100111";
		[214] = "111111111111111111111100010";
		[215] = "111111111111111111110010";
		[216] = "111111111111111100100";
		[217] = "111111111111111100101";
		[218] = "11111111111111111111101000";
		[219] = "11111111111111111111101001";
		[220] = "1111111111111111111111111101";
		[221] = "111111111111111111111100011";
		[222] = "111111111111111111111100100";
		[223] = "111111111111111111111100101";
		[224] = "11111111111111101100";
		[225] = "111111111111111111110011";
		[226] = "11111111111111101101";
		[227] = "111111111111111100110";
		[228] = "1111111111111111101001";
		[229] = "111111111111111100111";
		[230] = "111111111111111101000";
		[231] = "11111111111111111110011";
		[232] = "1111111111111111101010";
		[233] = "1111111111111111101011";
		[234] = "1111111111111111111101110";
		[235] = "1111111111111111111101111";
		[236] = "111111111111111111110100";
		[237] = "111111111111111111110101";
		[238] = "11111111111111111111101010";
		[239] = "11111111111111111110100";
		[240] = "11111111111111111111101011";
		[241] = "111111111111111111111100110";
		[242] = "11111111111111111111101100";
		[243] = "11111111111111111111101101";
		[244] = "111111111111111111111100111";
		[245] = "111111111111111111111101000";
		[246] = "111111111111111111111101001";
		[247] = "111111111111111111111101010";
		[248] = "111111111111111111111101011";
		[249] = "1111111111111111111111111110";
		[250] = "111111111111111111111101100";
		[251] = "111111111111111111111101101";
		[252] = "111111111111111111111101110";
		[253] = "111111111111111111111101111";
		[254] = "111111111111111111111110000";
		[255] = "11111111111111111111101110";
		EOS   = "111111111111111111111111111111";
	}
	local function bit_string_to_byte(bitstring)
		return string.char(tonumber(bitstring, 2))
	end
	huffman_encode = function(s)
		-- [TODO]: optimize
		local t = { s:byte(1, -1) }
		for i=1, #s do
			t[i] = huffman_codes[t[i]]
		end
		local bitstring = table.concat(t)
		-- round up to next octet
		bitstring = bitstring .. ("1"):rep(7 - (#bitstring - 1) % 8)
		local bytes = bitstring:gsub("........", bit_string_to_byte)
		return bytes
	end
	-- Build tree for huffman decoder
	local huffman_tree = {}
	for k, v in pairs(huffman_codes) do
		local prev_node
		local node = huffman_tree
		local lr
		for j=1, #v do
			lr = v:sub(j, j)
			prev_node = node
			node = prev_node[lr]
			if node == nil then
				node = {}
				prev_node[lr] = node
			end
		end
		prev_node[lr] = k
	end
	local byte_to_bitstring = {}
	for i=0, 255 do
		local val = ""
		for j=7, 0, -1 do
			val = val .. (band(i, 2^j) ~= 0 and "1" or "0")
		end
		byte_to_bitstring[string.char(i)] = val
	end
	local EOS_length = #huffman_codes.EOS
	huffman_decode = function(s)
		local bitstring = s:gsub(".", byte_to_bitstring)
		local node = huffman_tree
		local output = {}
		for c in bitstring:gmatch(".") do
			node = node[c]
			local nt = type(node)
			if nt == "number" then
				table.insert(output, node)
				node = huffman_tree
			elseif node == "EOS" then
				-- 5.2: A Huffman encoded string literal containing the EOS symbol MUST be treated as a decoding error.
				return nil, h2_errors.COMPRESSION_ERROR:new_traceback("invalid huffman code (EOS)")
			elseif nt ~= "table" then
				return nil, h2_errors.COMPRESSION_ERROR:new_traceback("invalid huffman code")
			end
		end
		--[[ Ensure that any left over bits are all one.
		Section 5.2: A padding not corresponding to the most significant bits
		of the code for the EOS symbol MUST be treated as a decoding error]]
		if node ~= huffman_tree then
			-- We check this by continuing through on the '1' branch and ensure that we end up at EOS
			local n_padding = EOS_length
			while type(node) == "table" do
				node = node["1"]
				n_padding = n_padding - 1
			end
			if node ~= "EOS" then
				return nil, h2_errors.COMPRESSION_ERROR:new_traceback("invalid huffman padding: expected most significant bits to match EOS")
			end
			-- Section 5.2: A padding strictly longer than 7 bits MUST be treated as a decoding error
			if n_padding < 0 or n_padding >= 8 then
				return nil, h2_errors.COMPRESSION_ERROR:new_traceback("invalid huffman padding: too much padding")
			end
		end

		return string.char(unpack(output))
	end
end

--[[
Section 5.2, String Literal Representation

Huffman is a tristate.
  - true: always use huffman encoding
  - false: never use huffman encoding
  - nil: don't care
]]
local function encode_string(s, huffman)
	-- For now we default to huffman off
	-- In future: encode with huffman, if the string is shorter, use it.
	if huffman then
		s = huffman_encode(s)
		return encode_integer(#s, 7, 0x80) .. s
	else
		return encode_integer(#s, 7, 0) .. s
	end
end

local function decode_string(str, pos)
	pos = pos or 1
	if pos > #str then return end
	local first_byte = str:byte(pos, pos)
	local huffman = band(first_byte, 0x80) ~= 0
	local len
	len, pos = decode_integer(str, 7, pos)
	if len == nil then return end
	local newpos = pos + len
	if newpos > #str+1 then return end
	local val = str:sub(pos, newpos-1)
	if huffman then
		local err
		val, err = huffman_decode(val)
		if not val then
			return nil, err
		end
	end
	return val, newpos
end

local function compound_key(name, value)
	return spack("s4s4", name, value)
end
local function uncompound_key(key)
	return sunpack("s4s4", key)
end
-- Section 4.1
local function dynamic_table_entry_size(k)
	return 32 - 8 + #k -- 8 is number of bytes of overhead introduced by compound_key
end
local static_names_to_index = {}
local static_pairs = {}
local max_static_index
do
	-- We prefer earlier indexes as examples in spec are like that
	local function p(i, name, value)
		if not static_names_to_index[name] then
			static_names_to_index[name] = i
		end
		local k = compound_key(name, value or "")
		static_pairs[k] = i
		static_pairs[i] = k
	end
	p( 1, ":authority")
	p( 2, ":method", "GET")
	p( 3, ":method", "POST")
	p( 4, ":path", "/")
	p( 5, ":path", "/index.html")
	p( 6, ":scheme", "http")
	p( 7, ":scheme", "https")
	p( 8, ":status", "200")
	p( 9, ":status", "204")
	p(10, ":status", "206")
	p(11, ":status", "304")
	p(12, ":status", "400")
	p(13, ":status", "404")
	p(14, ":status", "500")
	p(15, "accept-charset")
	p(16, "accept-encoding", "gzip, deflate")
	p(17, "accept-language")
	p(18, "accept-ranges")
	p(19, "accept")
	p(20, "access-control-allow-origin")
	p(21, "age")
	p(22, "allow")
	p(23, "authorization")
	p(24, "cache-control")
	p(25, "content-disposition")
	p(26, "content-encoding")
	p(27, "content-language")
	p(28, "content-length")
	p(29, "content-location")
	p(30, "content-range")
	p(31, "content-type")
	p(32, "cookie")
	p(33, "date")
	p(34, "etag")
	p(35, "expect")
	p(36, "expires")
	p(37, "from")
	p(38, "host")
	p(39, "if-match")
	p(40, "if-modified-since")
	p(41, "if-none-match")
	p(42, "if-range")
	p(43, "if-unmodified-since")
	p(44, "last-modified")
	p(45, "link")
	p(46, "location")
	p(47, "max-forwards")
	p(48, "proxy-authenticate")
	p(49, "proxy-authorization")
	p(50, "range")
	p(51, "referer")
	p(52, "refresh")
	p(53, "retry-after")
	p(54, "server")
	p(55, "set-cookie")
	p(56, "strict-transport-security")
	p(57, "transfer-encoding")
	p(58, "user-agent")
	p(59, "vary")
	p(60, "via")
	p(61, "www-authenticate")
	max_static_index = 61
end

-- Section 6.1
local function encode_indexed_header(index)
	assert(index > 0)
	return encode_integer(index, 7, 0x80)
end

-- Section 6.2.1
local function encode_literal_header_indexed(index, value, huffman)
	return encode_integer(index, 6, 0x40) .. encode_string(value, huffman)
end

local function encode_literal_header_indexed_new(name, value, huffman)
	return "\64" .. encode_string(name, huffman) .. encode_string(value, huffman)
end

-- Section 6.2.2
local function encode_literal_header_none(index, value, huffman)
	return encode_integer(index, 4, 0) .. encode_string(value, huffman)
end

local function encode_literal_header_none_new(name, value, huffman)
	return "\0" .. encode_string(name, huffman) .. encode_string(value, huffman)
end

-- Section 6.2.3
local function encode_literal_header_never(index, value, huffman)
	return encode_integer(index, 4, 0x10) .. encode_string(value, huffman)
end

local function encode_literal_header_never_new(name, value, huffman)
	return "\16" .. encode_string(name, huffman) .. encode_string(value, huffman)
end

-- Section 6.3
local function encode_max_size(n)
	return encode_integer(n, 5, 0x20)
end

--[[
"class" to represent an encoding/decoding context
This object encapulates a dynamic table

The FIFO implementation uses an ever growing head/tail;
with the exception that when empty, the indexes are reset.

This requires indexes to be corrected, as in the specification
the 'newest' item is always just after the static section.
]]

local methods = {}
local mt = {
	__name = "http.hpack";
	__index = methods;
}

local function new(SETTINGS_HEADER_TABLE_SIZE)
	local self = {
		dynamic_names_to_indexes = {};
		dynamic_pairs = {};
		dynamic_index_head = 1;
		dynamic_index_tail = 0;
		dynamic_current_size = 0;
		dynamic_max = nil; -- filled in below
		total_max = SETTINGS_HEADER_TABLE_SIZE or 0;
		data = {};
	}
	self.dynamic_max = self.total_max;
	return setmetatable(self, mt)
end

function methods:append_data(val)
	table.insert(self.data, val)
	return self
end

function methods:render_data()
	return table.concat(self.data)
end

function methods:clear_data()
	self.data = {}
	return true
end

-- Returns a boolean indicating if an entry was successfully removed
function methods:evict_from_dynamic_table()
	local old_head = self.dynamic_index_head
	if old_head > self.dynamic_index_tail then return false end
	local pair = self.dynamic_pairs[old_head]
	if self.dynamic_pairs[pair] == old_head then -- don't want to evict a duplicate entry (2.3.2)
		self.dynamic_pairs[pair] = nil
	end
	self.dynamic_pairs[old_head] = nil
	local name = self.dynamic_names_to_indexes[old_head]
	if name ~= nil then
		if self.dynamic_names_to_indexes[name] == old_head then
			self.dynamic_names_to_indexes[name] = nil
		end
		self.dynamic_names_to_indexes[old_head] = nil
	end
	local old_entry_size = dynamic_table_entry_size(pair)
	self.dynamic_current_size = self.dynamic_current_size - old_entry_size
	if self.dynamic_current_size == 0 then
		-- [Premature Optimisation]: reset to head at 1 and tail at 0
		self.dynamic_index_head = 1
		self.dynamic_index_tail = 0
	else
		self.dynamic_index_head = old_head + 1
	end
	return true
end

-- Returns a string in the format of the examples in the spec
function methods:dynamic_table_tostring()
	local r = {}
	local size = 0
	for i=self.dynamic_index_tail, self.dynamic_index_head, -1 do
		local pair = self.dynamic_pairs[i]
		local name, value = uncompound_key(pair)
		local entry_size = dynamic_table_entry_size(pair)
		local j = self.dynamic_index_tail - i + 1
		local line = string.format("[%3i] (s =%4d) %s: %s", j, entry_size, name, value)
		line = line:gsub(("."):rep(68), "%0\\\n                 ") -- Wrap lines
		size = size + entry_size
		table.insert(r, line)
	end
	table.insert(r, string.format("      Table size:%4d", size))
	return table.concat(r, "\n")
end

function methods:set_max_dynamic_table_size(SETTINGS_HEADER_TABLE_SIZE)
	self.total_max = SETTINGS_HEADER_TABLE_SIZE
	return true
end

function methods:encode_max_size(val)
	self:append_data(encode_max_size(val))
	return true
end

-- Section 4.3
function methods:resize_dynamic_table(new_size)
	assert(new_size >= 0)
	if new_size > self.total_max then
		return nil, h2_errors.COMPRESSION_ERROR:new_traceback("Dynamic Table size update new maximum size MUST be lower than or equal to the limit")
	end
	while new_size < self.dynamic_current_size do
		assert(self:evict_from_dynamic_table())
	end
	self.dynamic_max = new_size
	return true
end

function methods:add_to_dynamic_table(name, value, k) -- luacheck: ignore 212
	-- Early exit if we can't fit into dynamic table
	if self.dynamic_max == 0 then
		return true
	end
	local new_entry_size = dynamic_table_entry_size(k)
	-- Evict old entries until we can fit, Section 4.4
	while self.dynamic_current_size + new_entry_size > self.dynamic_max do
		if not self:evict_from_dynamic_table() then
			--[[It is not an error to attempt to add an entry that is larger than the maximum size;
			an attempt to add an entry larger than the maximum size causes the table to be emptied
			of all existing entries, and results in an empty table.]]
			return true
		end
	end
	-- Increment current index
	local index = self.dynamic_index_tail + 1
	self.dynamic_index_tail = index
	-- Add to dynamic table
	self.dynamic_pairs[k] = index
	self.dynamic_pairs[index] = k
	-- [Premature Optimisation]: Don't both putting it in dynamic table if it's in static table
	if static_names_to_index[name] == nil then
		self.dynamic_names_to_indexes[index] = name
		self.dynamic_names_to_indexes[name] = index -- This intentionally overwrites to keep up to date
	end
	self.dynamic_current_size = self.dynamic_current_size + new_entry_size
	return true
end

function methods:dynamic_table_id_to_index(id)
	return max_static_index + self.dynamic_index_tail - id + 1
end
methods.dynamic_index_to_table_id = methods.dynamic_table_id_to_index

function methods:lookup_pair_index(k)
	local pair_static_index = static_pairs[k]
	if pair_static_index ~= nil then
		return pair_static_index
	end
	local pair_dynamic_id = self.dynamic_pairs[k]
	if pair_dynamic_id then
		return self:dynamic_table_id_to_index(pair_dynamic_id)
	end
	return nil
end

function methods:lookup_name_index(name)
	local name_static_index = static_names_to_index[name]
	if name_static_index then
		return name_static_index
	end
	local name_dynamic_id = self.dynamic_names_to_indexes[name]
	if name_dynamic_id then
		return self:dynamic_table_id_to_index(name_dynamic_id)
	end
	return nil
end

function methods:lookup_index(index)
	if index <= max_static_index then
		local k = static_pairs[index]
		if k then
			return uncompound_key(k)
		end
	else -- Dynamic?
		local id = self:dynamic_index_to_table_id(index)
		local k = self.dynamic_pairs[id]
		if k then
			return uncompound_key(k)
		end
	end
	return
end

function methods:add_header_indexed(name, value, huffman)
	local k = compound_key(name, value)
	local pair_index = self:lookup_pair_index(k)
	if pair_index then
		local data = encode_indexed_header(pair_index)
		return self:append_data(data)
	end
	local name_index = self:lookup_name_index(name)
	if name_index then
		local data = encode_literal_header_indexed(name_index, value, huffman)
		self:add_to_dynamic_table(name, value, k)
		return self:append_data(data)
	end
	-- Never before seen name
	local data = encode_literal_header_indexed_new(name, value, huffman)
	self:add_to_dynamic_table(name, value, k)
	return self:append_data(data)
end

function methods:add_header_never_indexed(name, value, huffman)
	local name_index = self:lookup_name_index(name)
	if name_index then
		local data = encode_literal_header_never(name_index, value, huffman)
		return self:append_data(data)
	end
	-- Never before seen name
	local data = encode_literal_header_never_new(name, value, huffman)
	return self:append_data(data)
end

function methods:encode_headers(headers)
	for name, value, never_index in headers:each() do
		if never_index then
			self:add_header_never_indexed(name, value)
		else
			self:add_header_indexed(name, value)
		end
	end
	return true
end

local function decode_header_helper(self, payload, prefix_len, pos)
	local index, name, value
	index, pos = decode_integer(payload, prefix_len, pos)
	if index == nil then
		return index, pos
	end
	if index == 0 then
		name, pos = decode_string(payload, pos)
		if name == nil then
			return name, pos
		end
		value, pos = decode_string(payload, pos)
		if value == nil then
			return value, pos
		end
	else
		name = self:lookup_index(index)
		if name == nil then
			return nil, h2_errors.COMPRESSION_ERROR:new_traceback(string.format("index %d not found in table", index))
		end
		value, pos = decode_string(payload, pos)
		if value == nil then
			return value, pos
		end
	end
	return name, value, pos
end
function methods:decode_headers(payload, header_list, pos)
	header_list = header_list or new_headers()
	pos = pos or 1
	while pos <= #payload do
		local first_byte = payload:byte(pos, pos)
		if band(first_byte, 0x80) ~= 0 then -- Section 6.1
			-- indexed header
			local index, newpos = decode_integer(payload, 7, pos)
			if index == nil then break end
			pos = newpos
			local name, value = self:lookup_index(index)
			if name == nil then
				return nil, h2_errors.COMPRESSION_ERROR:new_traceback(string.format("index %d not found in table", index))
			end
			header_list:append(name, value, false)
		elseif band(first_byte, 0x40) ~= 0 then -- Section 6.2.1
			local name, value, newpos = decode_header_helper(self, payload, 6, pos)
			if name == nil then
				if value == nil then
					break -- EOF
				end
				return nil, value
			end
			pos = newpos
			self:add_to_dynamic_table(name, value, compound_key(name, value))
			header_list:append(name, value, false)
		elseif band(first_byte, 0x20) ~= 0 then -- Section 6.3
			--[[ Section 4.2
			This dynamic table size update MUST occur at the beginning of the
			first header block following the change to the dynamic table size.
			In HTTP/2, this follows a settings acknowledgment.]]
			if header_list:len() > 0 then
				return nil, h2_errors.COMPRESSION_ERROR:new_traceback("dynamic table size update MUST occur at the beginning of a header block")
			end
			local size, newpos = decode_integer(payload, 5, pos)
			if size == nil then break end
			pos = newpos
			local ok, err = self:resize_dynamic_table(size)
			if not ok then
				return nil, err
			end
		else -- Section 6.2.2 and 6.2.3
			local never_index = band(first_byte, 0x10) ~= 0
			local name, value, newpos = decode_header_helper(self, payload, 4, pos)
			if name == nil then
				if value == nil then
					break -- EOF
				end
				return nil, value
			end
			pos = newpos
			header_list:append(name, value, never_index)
		end
	end
	return header_list, pos
end

return {
	new = new;
	methods = methods;
	mt = mt;

	encode_integer = encode_integer;
	decode_integer = decode_integer;
	encode_string = encode_string;
	decode_string = decode_string;
	encode_indexed_header = encode_indexed_header;
	encode_literal_header_indexed = encode_literal_header_indexed;
	encode_literal_header_indexed_new = encode_literal_header_indexed_new;
	encode_literal_header_none = encode_literal_header_none;
	encode_literal_header_none_new = encode_literal_header_none_new;
	encode_literal_header_never = encode_literal_header_never;
	encode_literal_header_never_new = encode_literal_header_never_new;
	encode_max_size = encode_max_size;
}
