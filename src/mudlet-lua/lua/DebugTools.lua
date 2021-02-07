----------------------------------------------------------------------------------
--- Mudlet Debug Tools
----------------------------------------------------------------------------------



-- Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#showCaptureGroups
function showCaptureGroups()
  for k, v in pairs( matches ) do
    selectCaptureGroup( tonumber(k) )
    setFgColor( math.random(0, 255), math.random(0, 255), math.random(0, 255) )
    setBgColor( math.random(0, 255), math.random(0, 255), math.random(0, 255) )
  end
end


-- Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#showMultimatches
function showMultimatches()
  echo("\n-------------------------------------------------------");
  echo("\nThe table multimatches[n][m] contains:");
  echo("\n-------------------------------------------------------");
  for k, v in ipairs(multimatches) do
    echo("\nregex " .. k .. " captured: (multimatches[" .. k .. "][1-n])");
    for k2, v2 in pairs(v) do
      echo("\n          key=" .. k2 .. " value=" .. v2);
    end
  end
  echo("\n-------------------------------------------------------\n");
end


-- No documentation available in wiki - internal function
--- get the Lua keywords as a set-like table.
-- So <code>res["and"]</code> etc would be <code>true</code>.
-- @return a table
local function get_keywords()
  if not lua_keyword then
    lua_keyword = {
      ["and"] = true, ["break"] = true, ["do"] = true,
      ["else"] = true, ["elseif"] = true, ["end"] = true,
      ["false"] = true, ["for"] = true, ["function"] = true,
      ["if"] = true, ["in"] = true, ["local"] = true, ["nil"] = true,
      ["not"] = true, ["or"] = true, ["repeat"] = true,
      ["return"] = true, ["then"] = true, ["true"] = true,
      ["until"] = true, ["while"] = true
    }
  end
  return lua_keyword
end

-- No documentation available in wiki - internal function
local function quote_if_necessary (v)
  if not v then
    return ''
  else
    if v:find ' ' then
      v = '"' .. v .. '"'
    end
  end
  return v
end

local keywords

-- No documentation available in wiki - internal function
local function is_identifier (s)
  return type(s) == 'string' and s:find('^[%a_][%w_]*$') and not keywords[s]
end

-- No documentation available in wiki - internal function
local function quote (s)
  if type(s) == 'table' then
    return prettywrite(s, '')
  else
    return ('%q'):format(tostring(s))
  end
end

-- No documentation available in wiki - internal function
local function index (numkey, key)
  if not numkey then
    key = quote(key)
  end
  return '[' .. key .. ']'
end

-- Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#prettywrite
local append = table.insert
function prettywrite (tbl, space, not_clever)
  if type(tbl) ~= 'table' then
    if type(tbl) == "string" then
      return string.format("\"%s\"\n", tostring(tbl))
    else
      return string.format("%s\n", tostring(tbl))
    end
  end

  -- return a nice {} instead of {\n} on blanks
  if not next(tbl) then
    return '{}'
  end

  if not keywords then
    keywords = get_keywords()
  end
  local set = ' = '
  if space == '' then
    set = '='
  end
  space = space or '  '
  local lines = {}
  local line = ''
  local tables = {}


  local function put(s)
    if #s > 0 then
      line = line .. s
    end
  end

  local function putln (s)
    if #line > 0 then
      line = line .. s
      append(lines, line)
      line = ''
    else
      append(lines, s)
    end
  end

  local function eat_last_comma ()
    local n, lastch = #lines
    local lastch = lines[n]:sub(-1, -1)
    if lastch == ',' then
      lines[n] = lines[n]:sub(1, -2)
    end
  end


  local writeit
  writeit = function(t, oldindent, indent)
    local tp = type(t)
    if tp ~= 'string' and tp ~= 'table' then
      putln(quote_if_necessary(tostring(t)) .. ',')
    elseif tp == 'string' then
      if t:find('\n') then
        putln('[[\n' .. t .. ']],')
      else
        putln(quote(t) .. ',')
      end
    elseif tp == 'table' then
      if tables[t] then
        putln('<cycle>,')
        return
      end
      tables[t] = true
      local newindent = indent .. space
      putln('{')
      local used = {}
      if not not_clever then
        for i, val in ipairs(t) do
          put(indent)
          writeit(val, indent, newindent)
          used[i] = true
        end
      end
      for key, val in pairs(t) do
        local numkey = type(key) == 'number'
        if not_clever then
          key = tostring(key)
          put(indent .. index(numkey, key) .. set)
          writeit(val, indent, newindent)
        else
          if not numkey or not used[key] then
            -- non-array indices
            if numkey or not is_identifier(key) then
              key = index(numkey, key)
            end
            put(indent .. key .. set)
            writeit(val, indent, newindent)
          end
        end
      end
      tables[t] = nil
      eat_last_comma()
      putln(oldindent .. '},')
    else
      putln(tostring(t) .. ',')
    end
  end
  writeit(tbl, '', space)
  eat_last_comma()
  return table.concat(lines, #space > 0 and '\n' or '')
end

-- Documentation: https://wiki.mudlet.org/index.php?title=Manual:Lua_Functions#display
function display(...)
  local arg = {...}
  arg.n = table.maxn(arg)
  if arg.n > 1 then
    for i = 1, arg.n do
      display(arg[i])
    end
  else
    echo((prettywrite(arg[1], '  ') or 'nil') .. '\n')
  end
end
