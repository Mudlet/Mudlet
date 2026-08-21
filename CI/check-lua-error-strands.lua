#!/usr/bin/env lua
--[[
Finds heap-owning C++ objects left alive across a lua_error() raise.

lua_error() longjmps in Lua 5.1, so it unwinds the C stack without running a
single C++ destructor. Any QString, QByteArray, QStringList, std::string or
TMediaData still in scope at the raise leaks its buffer. PRs #9602 and #9605
removed the LSan suppression hiding that class and cleared all 377 sites then
present, but the sweep was a one-off: nothing re-runs it, so a file added
afterwards reintroduces the bug silently. LeakSanitizer only catches it if a
spec happens to exercise the raising path, which for a freshly added Lua
function is usually never.

Only functions taking a lua_State* are considered: those are the only frames
a longjmp can unwind. Closing the call graph over every function name instead
produces thousands of false hits.

Usage: lua CI/check-lua-error-strands.lua [file.cpp ...]
       (no arguments scans src/*.cpp)
Exits 1 when anything is found.
]]

-- Helpers that raise on the caller's behalf, plus the raw raisers. The
-- check*Arg() family is deliberately absent: those return a bool and are the
-- non-raising counterparts callers are meant to use when they hold anything.
local RAISERS = {
  "getVerifiedString", "getVerifiedInt", "getVerifiedBool", "getVerifiedFloat",
  "getVerifiedDouble", "getVerifiedStringOrInteger",
  "parseCommandOrFunction", "parseHintsTable", "parseCommandsOrFunctionsTable",
  "lua_error", "luaL_error", "luaL_argerror", "luaL_typerror",
  "luaL_checkstring", "luaL_checklstring", "luaL_checknumber", "luaL_checkinteger",
  "luaL_checkstack", "luaL_checktype", "luaL_checkany", "luaL_checkudata",
  "luaL_optstring", "luaL_optlstring", "luaL_optnumber", "luaL_optinteger",
}

-- A raiser paired with the non-raising checker that makes its raise dead: a
-- caller that validates with check*Arg() first and returns on failure leaves
-- the paired raiser's own check unable to fail. Both calls have to name the
-- same argument position for the pairing to hold.
local PAIRED_CHECKER = {
  parseCommandOrFunction = "checkCommandOrFunctionArg",
  parseHintsTable = "checkHintsTable",
  parseCommandsOrFunctionsTable = "checkCommandsOrFunctionsTable",
  getVerifiedString = "checkStringArg",
  getVerifiedInt = "checkIntArg",
  getVerifiedBool = "checkBoolArg",
  getVerifiedFloat = "checkNumberArg",
  getVerifiedDouble = "checkNumberArg",
  getVerifiedStringOrInteger = "checkStringOrIntegerArg",
}

-- Types whose default-constructed state is free but which own heap memory as
-- soon as they hold anything.
local OWNING_TYPES = {
  "QString", "QStringList", "QByteArray", "std::string", "TMediaData",
}

-- Initialisers that hand back heap-owning data, so an `auto` local holding one
-- is tracked even though its type is never spelled out.
local OWNING_INITIALISERS = {
  -- getVerifiedInt/Float/Double/Bool hand back scalars, so only the two that
  -- return a QString belong here
  "getVerifiedString%s*%(", "getVerifiedStringOrInteger%s*%(",
  "lua_tostring%s*%(", "QString::fromUtf8%s*%(",
  "QString%s*[{(]", "QStringList%s*[{(]", "QByteArray%s*[{(]",
  "%.toUtf8%s*%(", "%.arg%s*%(", "%.split%s*%(", "%.join%s*%(",
}

-- Calls that fill a container declared empty, turning a free local into an
-- owning one partway through the function.
local FILLING_OPERATIONS = {
  "%s*<<%s*", "%s*%+=%s*", "%.append%s*%(", "%.push_back%s*%(",
  "%.insert%s*%(", "%.prepend%s*%(", "%.replace%s*%(",
}

-- Expressions that produce an owning temporary when passed straight into a
-- raising call - the sub-case a declaration scan cannot see, because there is
-- no named local to find.
local OWNING_TEMPORARIES = {
  "%.toUtf8%s*%(", "%.toLatin1%s*%(", "%.toLocal8Bit%s*%(",
  "%.toStdString%s*%(", "%.arg%s*%(", "%.toLower%s*%(", "%.toUpper%s*%(",
  "%.trimmed%s*%(", "%.simplified%s*%(", "%.join%s*%(", "%.split%s*%(",
}

-- Whether an initialiser can be trusted not to have allocated. QStringLiteral
-- data is in the binary, and a default-constructed or empty Qt container is a
-- shared singleton, so none of them own a buffer to strand.
local function initialiserIsFree(init)
  if not init or init:match("^%s*$") then
    return true                                   -- default-constructed
  end
  init = init:gsub("^%s*[=({]%s*", ""):gsub("%s*[;)}]%s*$", "")
  if init:match("^%s*$") then return true end
  return init:match('^qsl%s*%(')
      or init:match('^QStringLiteral%s*%(')
      or init:match('^""$')
      or init:match('^QString%s*%(%s*%)$')
      or init:match('^QByteArray%s*%(%s*%)$')
      or init:match('^QStringList%s*%(%s*%)$')
      or false
end

-- A line with its string and character literals blanked and its trailing
-- comment cut. Brace depth is what scopes every tracked variable, so a "{" in
-- a message or a URL inside a // comment would otherwise leak a scope and
-- silence every later raise in the function.
local function codeOnly(line, inBlockComment)
  local out, k, n = {}, 1, #line
  while k <= n do
    local c = line:sub(k, k)
    if inBlockComment then
      if c == "*" and line:sub(k + 1, k + 1) == "/" then
        inBlockComment = false
        k = k + 1
      end
      out[#out + 1] = " "
      k = k + 1
    elseif c == "/" and line:sub(k + 1, k + 1) == "/" then
      break
    elseif c == "/" and line:sub(k + 1, k + 1) == "*" then
      inBlockComment = true
      k = k + 2
    -- a digit separator (1'000) is not a character literal
    elseif c == "'" and line:sub(k - 1, k - 1):match("[%w_]") then
      out[#out + 1] = c
      k = k + 1
    elseif c == '"' or c == "'" then
      -- R"(...)" raw strings end at )" rather than at the next quote
      local raw = c == '"' and line:sub(k - 1, k - 1) == "R"
      out[#out + 1] = c
      k = k + 1
      while k <= n do
        local d = line:sub(k, k)
        if raw then
          if d == ")" and line:sub(k + 1, k + 1) == '"' then
            out[#out + 1] = '")'
            k = k + 2
            break
          end
        elseif d == "\\" then
          k = k + 1
        elseif d == c then
          out[#out + 1] = c
          k = k + 1
          break
        end
        out[#out + 1] = " "
        k = k + 1
      end
    else
      out[#out + 1] = c
      k = k + 1
    end
  end
  return table.concat(out), inBlockComment
end

-- The argument list of a call to `name`, parenthesis-balanced. Returning the
-- span rather than the rest of the line is what stops a temporary built from a
-- raiser's *result* - getVerifiedString(...).trimmed() - being read as one
-- built inside its arguments.
local function argsOfCall(line, name)
  local s, e = line:find(name, 1, true)
  while s do
    local before = s > 1 and line:sub(s - 1, s - 1) or " "
    if not before:match("[%w_]") and line:sub(e + 1, e + 1) == "(" then
      local depth, k = 0, e + 1
      while k <= #line do
        local c = line:sub(k, k)
        if c == "(" then depth = depth + 1
        elseif c == ")" then
          depth = depth - 1
          if depth == 0 then return line:sub(e + 2, k - 1) end
        end
        k = k + 1
      end
      return line:sub(e + 2)   -- arguments continue on the next line
    end
    s, e = line:find(name, e + 1, true)
  end
  return nil
end

local function findRaiser(line)
  for _, name in ipairs(RAISERS) do
    local args = argsOfCall(line, name)
    if args then return name, args end
  end
  return nil, nil
end

local function owningTemporaryIn(line)
  for _, pat in ipairs(OWNING_TEMPORARIES) do
    if line:match(pat) then
      return (pat:gsub("%%%.", "."):gsub("%%s%*", ""):gsub("%%%(", "()"))
    end
  end
  return nil
end

-- The argument position a checker or raiser is talking about: the third field,
-- after the lua_State and the function name. Getting this wrong is not a near
-- miss - every call in a file shares __func__ as its second field, so reading
-- that one instead makes the pairing test below always succeed, and one
-- unrelated check*Arg() line then silences the whole function.
local function positionArg(args)
  local fields, depth, current = {}, 0, {}
  for k = 1, #args do
    local c = args:sub(k, k)
    if c == "(" or c == "<" then depth = depth + 1 end
    if c == ")" or c == ">" then depth = depth - 1 end
    if c == "," and depth == 0 then
      fields[#fields + 1] = table.concat(current)
      current = {}
    else
      current[#current + 1] = c
    end
  end
  fields[#fields + 1] = table.concat(current)
  return (fields[3] or ""):gsub("^%s*", ""):gsub("%s*$", "")
end

local findings = {}
local functionsScanned = 0

local function scanFile(path)
  local fh = io.open(path, "r")
  if not fh then
    io.stderr:write(("could not read %s\n"):format(path))
    os.exit(2)
  end
  local lines = {}
  for line in fh:lines() do lines[#lines + 1] = line end
  fh:close()

  local i, n = 1, #lines
  while i <= n do
    local line = lines[i]
    -- A definition's parameter list can wrap, so gather lines until the
    -- parentheses balance before looking for the lua_State*. ColumnLimit is
    -- 200 and clang-format keeps these on one line today, but a scanner that
    -- goes quiet on a shape it cannot parse is worse than one that is noisy.
    local signature, signatureEnd = line, i
    if line:match("^[%w_]") and line:match("%(") then
      local open = select(2, line:gsub("%(", "")) - select(2, line:gsub("%)", ""))
      local k = i
      while open > 0 and k < n and k - i < 6 do
        k = k + 1
        signature = signature .. " " .. lines[k]
        open = open + select(2, lines[k]:gsub("%(", "")) - select(2, lines[k]:gsub("%)", ""))
      end
      signatureEnd = k
    end
    if signature:match("lua_State%s*%*") and signature:match("%(") and not line:match("^%s*//")
       and not signature:match("%)%s*;%s*$") and not line:match("^%s*%*") then
      local depth, bodyStart = 0, nil
      for j = signatureEnd, math.min(signatureEnd + 4, n) do
        if lines[j]:match("{") then bodyStart = j; break end
      end
      if bodyStart then
        functionsScanned = functionsScanned + 1
        depth = 0
        local live = {}   -- varname -> {line=, type=}
        local free = {}   -- declared empty, so owning nothing until something fills it
        local scopes = {} -- depth -> list of varnames declared at that depth
        local emptyGuards = {} -- depth -> varname the enclosing if() proved empty
        local inBlockComment = false
        local j = bodyStart
        repeat
          local body = lines[j]
          local code
          code, inBlockComment = codeOnly(body, inBlockComment)

          -- A raiser's arguments can wrap. Join forward while the parentheses
          -- are unbalanced so a temporary on a continuation line is still seen.
          local raiseText = code
          if select(2, code:gsub("%(", "")) > select(2, code:gsub("%)", "")) then
            local open = select(2, code:gsub("%(", "")) - select(2, code:gsub("%)", ""))
            local k = j
            while open > 0 and k < n and k - j < 4 do
              k = k + 1
              local more = codeOnly(lines[k], false)
              raiseText = raiseText .. " " .. more
              open = open + select(2, more:gsub("%(", "")) - select(2, more:gsub("%)", ""))
            end
          end

          local raiser, raiserArgs = findRaiser(raiseText)
          if raiser and raiserArgs and PAIRED_CHECKER[raiser] then
            local checker = PAIRED_CHECKER[raiser]
            local wanted = positionArg(raiserArgs)
            for back = bodyStart, j - 1 do
              local checkerArgs = argsOfCall(codeOnly(lines[back], false), checker)
              if checkerArgs and wanted ~= "" and positionArg(checkerArgs) == wanted then
                raiser = nil
                break
              end
            end
          end
          if raiser and j > bodyStart then
            for name, info in pairs(live) do
              -- Inside `if (x.isEmpty()) { ... }` the variable really is the
              -- shared empty buffer. Outside it - and the idiom here is to
              -- return from that block - x is proven NON-empty, so the guard
              -- has to be scoped to the block rather than to nearby lines.
              local guarded = false
              for _, guardedName in pairs(emptyGuards) do
                if guardedName == name then guarded = true end
              end
              if not guarded then
              findings[#findings + 1] = {
                file = path, line = j, kind = "local",
                detail = ("%s %s (declared line %d) is live across %s()")
                         :format(info.type, name, info.line, raiser),
              }
              end
            end
            local temp = raiserArgs and owningTemporaryIn(raiserArgs)
            if temp then
              findings[#findings + 1] = {
                file = path, line = j, kind = "temporary",
                detail = ("%s builds an owning temporary inside %s()'s arguments")
                         :format(temp, raiser),
              }
            end
          end

          local emptyGuarded = code:match("if%s*%(%s*([%w_]+)%s*%.%s*isEmpty%s*%(%s*%)%s*%)")
          if emptyGuarded and code:match("{%s*$") then
            emptyGuards[depth + 1] = emptyGuarded
          end

          -- a container declared empty starts owning as soon as something fills it
          for name, info in pairs(free) do
            for _, op in ipairs(FILLING_OPERATIONS) do
              if code:match("%f[%w]" .. name .. "%f[%W]" .. op) then
                live[name] = {line = j, type = info.type}
                scopes[info.depth] = scopes[info.depth] or {}
                table.insert(scopes[info.depth], name)
                free[name] = nil
                break
              end
            end
          end

          local autoName, autoInit = code:match("^%s*[a-zA-Z:%s]-%f[%w]auto%f[%W]%s*[%*&]?%s*([%w_]+)%s*=%s*(.*)$")
          if autoName then
            for _, pat in ipairs(OWNING_INITIALISERS) do
              if autoInit:match(pat) then
                live[autoName] = {line = j, type = "auto"}
                scopes[depth] = scopes[depth] or {}
                table.insert(scopes[depth], autoName)
                break
              end
            end
          end

          for _, ty in ipairs(OWNING_TYPES) do
            local pat = "^%s*[a-zA-Z:<>,%s]-%f[%w]" .. ty:gsub("%p", "%%%0") .. "%f[%W]%s+([%w_]+)%s*(.*)$"
            local varname, rest = code:match(pat)
            if varname and not code:match("%f[%w]static%f[%W]")
               and not code:match("%f[%w]return%f[%W]") and varname ~= "const" then
              if initialiserIsFree(rest) then
                free[varname] = {line = j, type = ty, depth = depth}
              else
                live[varname] = {line = j, type = ty}
                scopes[depth] = scopes[depth] or {}
                table.insert(scopes[depth], varname)
              end
              break
            end
          end

          -- Brace tracking, so a variable dies with its block. Braces are
          -- walked in text order: "} else if (...) {" closes the previous
          -- branch's scope before opening the next one, and counting all the
          -- opens first would keep that branch's locals alive into it.
          for k = 1, #code do
            local c = code:sub(k, k)
            if c == "{" then
              depth = depth + 1
            elseif c == "}" then
              if scopes[depth] then
                for _, v in ipairs(scopes[depth]) do live[v] = nil; free[v] = nil end
                scopes[depth] = nil
              end
              emptyGuards[depth] = nil
              depth = depth - 1
            end
          end
          j = j + 1
        until depth <= 0 or j > n
        i = j - 1
      end
    end
    i = i + 1
  end
end

local targets = {...}
if #targets == 0 then
  -- find, not a glob: src/ has subdirectories (src/updater, src/crash_reporter)
  -- and the workflow's paths filter watches src/**, so a glob would let a file
  -- trigger the job and then never be read. Headers are scanned too, for the
  -- same reason - they are watched, and can carry inline bodies.
  local pipe = io.popen("find src \\( -name '*.cpp' -o -name '*.h' \\) 2>/dev/null")
  for f in pipe:lines() do targets[#targets + 1] = f end
  pipe:close()
end

-- Finding nothing to scan reads exactly like a clean tree, so say so instead.
-- Run from the wrong directory, this is the whole difference between a check
-- and a green tick that means nothing.
if #targets == 0 then
  io.stderr:write("no C++ sources to scan - run this from the repository root\n")
  os.exit(2)
end

for _, f in ipairs(targets) do scanFile(f) end

table.sort(findings, function(a, b)
  if a.file ~= b.file then return a.file < b.file end
  return a.line < b.line
end)

for _, f in ipairs(findings) do
  print(("%s:%d: %s"):format(f.file, f.line, f.detail))
end

if #findings > 0 then
  print("")
  print(("%d heap object(s) stranded across a lua_error() raise."):format(#findings))
  print("lua_error() longjmps past C++ destructors, so each one leaks.")
  print("Validate with the non-raising checkStringArg()/checkIntArg() family before")
  print("building the object, or pass a plain literal instead of a QString temporary.")
  os.exit(1)
end
print(("No heap objects stranded across a lua_error() raise (%d files, %d functions scanned).")
      :format(#targets, functionsScanned))
