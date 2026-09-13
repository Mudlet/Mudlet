----------------------------------------------------------------------------------
--- Mudlet DB
----------------------------------------------------------------------------------

if package.loaded["luasql.sqlite3"] then
  luasql = require "luasql.sqlite3"
end


db = {}
db.__autocommit = {}
db.__schema = {}
db.__conn = {}

db.debug_sql = false



-- NOT LUADOC
-- Converts the type of a lua object to the equivalent type in SQL
function db:_sql_type(value)
  local t = type(value)

  if t == "number" then
    return "REAL"
  elseif t == "nil" then
    return "NULL"
  elseif t == "table" and value._timestamp ~= nil then
    return "INTEGER"
  elseif t == "table" and value._isNull ~= nil then
    return "NULL"
  else
    return "TEXT"
  end
end



-- NOT LUADOC
-- Converts a data value in Lua to its SQL equivalent; notably it will also escape single-quotes to
-- prevent inadvertent SQL injection.
-- called when generating the schema
function db:_sql_convert(value)
  local t = db:_sql_type(value)

  if value == nil then
    return "NULL"
  elseif t == "TEXT" and type(value) == "string" then
    return '"' .. value:gsub("'", "''") .. '"'
  elseif t == "NULL" then
    return "NULL"
  elseif t == "INTEGER" then
    -- With db.Timestamp's, a value of false should be interpreted as nil.
    if value._timestamp == false then
      return "NULL"
    end
    return tostring(value._timestamp)
  else
    return tostring(value)
  end
end



-- NOT LUADOC
-- Given a sheet name and the details of an index, this function will return a unique index name to
-- add to the database. The purpose of this is to create unique index names as indexes are tested
-- for existence on each call of db:create and not only on creation. That way new indexes can be
-- added after initial creation.
function db:_index_name(tbl_name, params)
  local t = type(params)

  if t == "string" then
    return "idx_" .. tbl_name .. "_c_" .. params
  elseif assert(t == "table", "Indexes must be either a string or a table.") then
    local parts = { "idx", tbl_name, "c" }
    for _, v in pairs(params) do
      parts[#parts + 1] = v
    end
    return table.concat(parts, "_")
  end
end



-- NOT LUADOC
-- This function returns true if all of the columns referenced in index_columns also exist within
-- the sheet_columns table array. The purpose of this is to raise an error if someone tries to index
-- a column which doesn't currently exist in the schema.
function db:_index_valid(sheet_columns, index_columns)
  if type(index_columns) == "string" then
    if sheet_columns[index_columns] ~= nil then
      return true
    else
      return false
    end
  else
    for _, v in ipairs(index_columns) do
      if sheet_columns[v] == nil then
        db:echo_sql("\n--> Bad index " .. v)
        return false
      end
    end
  end
  return true
end



-- NOT LUADOC
-- The column_spec is either a string or an indexed table. This function returns either "column" or
-- "column1", "column2" for use in the column specification of INSERT.
function db:_sql_columns(value)
  local col_chunks = {}
  local colstr = ''
  local t = type(value)

  if t == "table" then
    for _, v in ipairs(value) do
      assert(type(v) == "string", "Column names must be strings, not " .. type(v) .. ".")
      -- see https://www.sqlite.org/syntaxdiagrams.html#ordering-term: a sort
      -- direction belongs to the column in front of it, so one that leads the
      -- list can only be a column of that name
      if col_chunks[1] and (v:lower() == "desc" or v:lower() == "asc") then
        col_chunks[#col_chunks] = col_chunks[#col_chunks] .. " " .. v
      else
        col_chunks[#col_chunks + 1] = '"' .. v:lower() .. '"'
      end
    end

    colstr = table.concat(col_chunks, ',')
  elseif assert(t == "string",
  "Must specify either a table array or string for index, not " .. type(value)) then
    colstr = '"' .. value:lower() .. '"'
  end
  return colstr
end



-- NOT LUADOC
-- This serves as a very similar function to db:_sql_columns, quoting column names properly but for
-- uses outside of INSERTs.
function db:_sql_fields(values)
  local sql_fields = {}

  for k, v in pairs(values) do
    sql_fields[#sql_fields + 1] = '"' .. k .. '"'
  end

  return "(" .. table.concat(sql_fields, ",") .. ")"
end



-- NOT LUADOC
-- This quotes values to be passed into an INSERT or UPDATE operation in a SQL list. Meaning, it turns
-- {x="this", y="that", z=1} into ('this', 'that', 1).
-- It is intelligent with data-types; strings are automatically quoted (with internal single quotes
-- escaped), nil turned into NULL, timestamps converted to integers, and such.
function db:_sql_values(values)
  local sql_values = {}

  for k, v in pairs(values) do
    local t = type(v)
    local s = ""

    if t == "string" then
      s = "'" .. v:gsub("'", "''") .. "'"
    elseif t == "nil" then
      s = "NULL"
    elseif t == "table" and v._timestamp ~= nil then
      if not v._timestamp then
        s = "NULL"
      elseif v._timestamp == "CURRENT_TIMESTAMP" then
        s = "datetime('now')"
      else
        s = "datetime('" .. v._timestamp .. "', 'unixepoch')"
      end
    elseif t == "table" and v._isNull then
      s = "NULL"
    else
      s = tostring(v)
    end

    sql_values[#sql_values + 1] = s
  end

  return "(" .. table.concat(sql_values, ",") .. ")"
end



--- <b><u>TODO</u></b> db:safe_name(name)
--   On a filesystem level, names are restricted to being alphanumeric only. So, "my_database" becomes
--   "mydatabase", and "../../../../etc/passwd" becomes "etcpasswd". This prevents any possible
--   security issues with database names.
function db:safe_name(name)
  name = name:gsub("[^%ad]", "")
  name = name:lower()
  return name
end


function db:_isActiveDBName(db_name)
  db_name = db:safe_name(db_name)

  return (
    db.__conn[db_name]
    and db.__conn[db_name] ~= 'SQLite3 connection (closed)'
    and io.exists(getMudletHomeDir() .. "/Database_" .. db_name .. ".db")
  )
end


-- NOT LUADOC
-- db:close takes the connection away but leaves db.__schema behind, so every sheet,
-- field and database handle a script is still holding keeps working right up to the
-- moment it needs the connection, and then indexes a nil. Nothing routes through a
-- single choke point, so each site that reaches into db.__conn has to notice for
-- itself; the callers of this function are the authoritative list of them.
--
-- Every one of those sites raised before these guards existed, and all but one still
-- does. THE RULE IS THAT NAMING THE CAUSE MUST NOT MAKE A LOUD FAILURE QUIET, not
-- that every site should answer the same way:
--
--   * db:add is the one that returns, because it alone already answers nil plus a
--     printed message when its INSERT fails, and packages do check that result.
--   * db:fetch_sql looks like it should match db:add, and must not. Its SQL-failure
--     path returns a BARE nil - no message, no print - and a query that matched
--     nothing returns an empty table, not nil. Handing back nil for a closed
--     database would make the fetch-then-add upsert that db:merge_unique itself
--     writes silently take the insert branch and loop forever.
--   * db.Database:_commit and _rollback answer false plus a message, the contract
--     #10068 gave them to match db:close. They are not a third opinion, just an
--     older one.
--   * everything else raises, which is what it did before.
--
-- error() rather than assert() throughout: assert would build the message on every
-- healthy call, and these sit on trigger-driven write paths.
--
-- expected is a parameter because the sites do not agree on what they take: a sheet at
-- most of them, a field at db:set and db:aggregate, a database name at db:_migrate, the
-- handle itself at the db.Database methods. No one phrase is true at all of them, and a
-- phrase that named several would have to deny a field reference is a field reference.
local A_SHEET = "a sheet, as in mydb.sheetname"
local A_FIELD = "a field, as in mydb.sheetname.fieldname"
local A_DATABASE = "a database handle, as in what db:create returned"

local function db_no_connection_message(action, db_name, expected)
  -- something that is not what this site takes carries no name to look up, so it lands
  -- on the same missing connection by a different route: saying "closed" there would
  -- name a cause that is not the one the caller has to fix
  if not db_name then
    return "can not "..action.." the database: expected "..expected.."."
  end

  -- likewise a name that was never created at all, where hunting for a db:close call
  -- that does not exist is the wrong place to send someone
  if not db.__schema[db_name] then
    return "can not "..action.." "..db_name..": no database by that name has been created."
  end

  return "can not "..action.." "..db_name.." because the database is closed."
end


local VALIDATION_OPTIONS = {
  "ABORT",
  "FAIL",
  "IGNORE",
  "REPLACE",
  "ROLLBACK"
}

---@param validations string
---@return boolean is_valid
---@return string msg
function db:_validate_validations(validations)
  if type(validations) ~= "string" then
    return false, "_validations must be a string. Received "..type(validations)
  elseif table.contains(VALIDATION_OPTIONS, validations) then
    return true, ""
  end

  return false, '_validations must be one of: {"ABORT", "FAIL", "IGNORE", "REPLACE", "ROLLBACK"}.  Received: '..validations
end


---@param unique_constraints string|table
---@return boolean is_valid
---@return string msg
function db:_validate_unique_contraints(unique_constraints)
  local is_valid, msg = true, ""

  local type_of = type(unique_constraints)
  local is_string = type_of == "string"
  local is_table = type_of == "table"

  if is_string then
    -- pass
  elseif is_table then
    local msgs = {}
    for _, unique_constraint in ipairs(unique_constraints) do
      type_of = type(unique_constraint)
      is_string = type_of == "string"
      is_table = type_of == "table"
      if is_string then
        -- pass
      elseif is_table then
        for _, value in ipairs(unique_constraint) do
          type_of = type(value)
          if type_of ~= "string" then
            is_valid = false
            table.insert(msgs, "Multi-column definitions for _unique must be a list of strings, for example: _unique = { {'foo', 'bar'} }.  Received "..type_of..".")
          end
        end
      else
        is_valid = false
        table.insert(msgs, "Members of _unique must be a string or table. Received ".. type_of..".")
      end
    end

    msg = table.concat(msgs, "\n")
  else
    is_valid = false
    msg = "_unique must be a string or a table.  Received "..type_of.."."
  end

  return is_valid, msg
end


--- Checks an _index sheet option, which takes the same shapes _unique does: a
--- single column name, a list of column names, or a list holding a list of
--- column names for a compound index.
---@param index string|table
---@return boolean is_valid
---@return string msg
function db:_validate_index(index)
  local type_of = type(index)

  if type_of == "string" then
    return true, ""
  elseif type_of ~= "table" then
    return false, "_index must be a string or a table.  Received "..type_of.."."
  end

  local msgs = {}
  for _, index_entry in ipairs(index) do
    type_of = type(index_entry)
    if type_of == "table" then
      for _, column_name in ipairs(index_entry) do
        if type(column_name) ~= "string" then
          table.insert(msgs, "Multi-column definitions for _index must be a list of strings, for example: _index = { {'foo', 'bar'} }.  Received "..type(column_name)..".")
        end
      end
    elseif type_of ~= "string" then
      table.insert(msgs, "Members of _index must be a string or table. Received "..type_of..".")
    end
  end

  return msgs[1] == nil, table.concat(msgs, "\n")
end


-- NOT LUADOC
-- Lua's reserved words, so db:create can tell a column name that may be written
-- as a bare key in the {name = ""} sheet form from one that has to be bracketed
local lua_reserved_words = {
  ["and"] = true, ["break"] = true, ["do"] = true, ["else"] = true, ["elseif"] = true,
  ["end"] = true, ["false"] = true, ["for"] = true, ["function"] = true, ["if"] = true,
  ["in"] = true, ["local"] = true, ["nil"] = true, ["not"] = true, ["or"] = true,
  ["repeat"] = true, ["return"] = true, ["then"] = true, ["true"] = true,
  ["until"] = true, ["while"] = true,
}


--- Creates and/or modifies an existing database. This function is safe to define at a top-level of a Mudlet
--- script: in fact it is recommended you run this function at a top-level without any kind of guards.
--- If the named database does not exist it will create it. If the database does exist then it will add
--- any columns or indexes which didn't exist before to that database. If the database already has all the
--- specified columns and indexes, it will do nothing. <br/><br/>
---
--- Because it is meant to run unguarded, a fault in the schema is only fatal when it has to be,
--- and where that line falls depends on what the faulty part holds. Something derived can be left
--- out and reported: an _index naming a column the sheet does not declare is skipped rather than
--- refused, because every row stays reachable without the index. A column is not derived. A sheet
--- built without one takes writes to it silently, since db:add reports the rejected INSERT the
--- same way and returns nil, which callers do not check, so a script can record nothing for weeks
--- with no sign of it. A fault that would cost the sheet a column therefore stays fatal, and its
--- message names the key so the fix is obvious. A value whose shape no sheet can be built from is
--- refused as well, whatever it holds: a malformed _index, _unique or _violations leaves nothing to
--- skip and nothing to build from. <br/><br/>
---
--- Note when weighing that up that printError is a quiet channel: it reaches the editor's Errors
--- tab, which is hidden until the user opens it, and the main console only when they have turned
--- on echoing Lua errors. <br/><br/>
---
--- The database will be called Database_<sanitized database name>.db and will be stored in the
--- Mudlet configuration directory. <br/><br/>
---
--- Database 'tables' are called 'sheets' consistently throughout this documentation, to avoid confusion
--- with Lua tables. <br/><br/>
---
--- The schema table must be a Lua table array containing table dictionaries that define the structure and
--- layout of each sheet. <br/><br/>
---
--- For sheets with unique indexes, you may specify a _violations key to indicate how the db layer handle
--- cases where the unique index is violated. The options you may use are:
---   <pre>
---   FAIL - the default. A hard error is thrown, cancelling the script.
---   IGNORE - The command that would add a record that violates uniqueness just fails silently.
---   REPLACE - The old record which matched the unique index is dropped, and the new one is added to replace it.
---   </pre>
---
--- @usage Example below will create a database with two sheets; the first is kills and is used to track every successful kill,
---   with both where and when the kill happened. It has one index, a compound index tracking the combination of name and area.
---   The second sheet has two indexes, but one is unique: it isn't possible to add two items to the enemies sheet with the same name.
---   <pre>
---   local mydb = db:create("combat_log",
---     {
---       kills = {
---         name = "",
---         area = "",
---         killed = db:Timestamp("CURRENT_TIMESTAMP"),
---         _index = {{"name", "area"}}
---       },
---       enemies = {
---         name = "",
---         city = "",
---         reason = "",
---         enemied = db:Timestamp("CURRENT_TIMESTAMP"),
---         _index = { "city" },
---         _unique = { "name" },
---         _violations = "IGNORE"
---       }
---     }
---   )
---   </pre>
---   Note that you have to use double {{ }} if you have composite index/unique constrain.
---   A single column may be given on its own instead of in a list, so _index = "city"
---   and _unique = "name" mean the same as the two lines above.
---   A sheet may also be given as a plain list of its column names, which then all
---   hold text and default to "". The sheet options are keys rather than list members,
---   so they work there too: enemies = {"name", "city", _index = "city"}
---   An _index entry naming a column the sheet does not have cannot be created, so
---   that entry is skipped and a warning is printed, while everything else the
---   sheet asks for is made as usual. The indexes the sheet already has are left
---   alone in that case, since what is left of _index is no longer the whole set
---   it asked for.
---   A _unique entry naming a column the sheet does not have is skipped and warned
---   about the same way, but that one costs more than an index would: a sheet made
---   from scratch is built without the uniqueness it asked for, so db:add takes rows
---   the constraint would have refused. A sheet that already enforces that uniqueness
---   keeps it and is warned about again, since taking a rule off a typo is not
---   something a later, correct db:create can undo: its constraints are then left
---   exactly as they are, a constraint it has newly asked for and a change to
---   _violations among them, until the entry is spelled right. A db:create that
---   loses the sheet nothing is applied as usual, and one that also drops a column
---   does lose the rule, since removing a column is a rebuild in its own right.
---   Naming _row_id in a _unique entry written as a list is warned about but kept,
---   since a sheet's _row_id is unique on its own and the constraint is therefore
---   one that can never refuse a row.
function db:create(db_name, sheets, force)
  if not db.__env or db.__env == 'SQLite3 environment (closed)' then
    db.__env = luasql.sqlite3()
  end

  local is_valid, msgs, warnings = true, {}, {}
  local schema = {}
  db_name = db:safe_name(db_name)


  -- We need to separate the actual column configuration from the meta-configuration of the desired
  -- sheet. {sheet={"column"}} verses {sheet={"column"}, _index={"column"}}. In the former we are
  -- creating a database with a single field; in the latter we are also adding an index on that
  -- field. The db package reserves any key that begins with an underscore to be special and syntax
  -- for its own use.
  for sheet_name, sheet in pairs(sheets) do
    local columns = {}
    local options = {}
    local has_skipped_index = false
    local has_skipped_unique = false

    -- the sheet was provided in {"column1", "column2"} format
    if sheet[1] ~= nil then
      -- The list holds the column names, which are text defaulting to "". A key
      -- is a sheet option when it starts with an underscore and an error
      -- otherwise: sweeping keys in with the column names would make a column
      -- out of an index definition. Numeric keys are checked against #sheet so
      -- that a stray [7] in a two-item list is not taken for a column name.
      -- These stay hard errors rather than being skipped the way an unusable
      -- _index entry is: a sheet built without one of its columns takes every
      -- write to that column silently, since db:add reports the rejected INSERT
      -- to the same place and returns nil, which callers do not check.
      local column_count = #sheet

      -- both working forms, spelled with the names this sheet did give plus the
      -- one being complained about, so the fix can be read straight off the
      -- message. Leaving that name out would advise a sheet without the column
      -- the user was declaring, which is the state this refusal exists to stop.
      local function keyed_entry(name)
        if name:match("^[%a_][%w_]*$") and not lua_reserved_words[name] then
          return name..' = ""'
        end
        -- anything else is not a bare key, so the form the message tells the
        -- user to write would not parse
        return "["..string.format("%q", name)..'] = ""'
      end

      local function both_forms(extra_name)
        local listed, keyed = {}, {}
        for i = 1, column_count do
          if type(sheet[i]) == "string" then
            listed[#listed + 1] = string.format("%q", sheet[i])
            keyed[#keyed + 1] = keyed_entry(sheet[i])
          end
        end
        if extra_name then
          listed[#listed + 1] = string.format("%q", extra_name)
          keyed[#keyed + 1] = keyed_entry(extra_name)
        end
        if listed[1] == nil then
          return '{"name", "city"}', '{name = "", city = ""}'
        end
        return "{"..table.concat(listed, ", ").."}", "{"..table.concat(keyed, ", ").."}"
      end

      for key, value in pairs(sheet) do
        if type(key) == "number" and key % 1 == 0 and key >= 1 and key <= column_count then
          if type(value) == "string" then
            columns[value] = ""
          else
            -- no example here: the two forms would have to be spelled without
            -- the name this entry was meant to give, which reads as advice to
            -- drop the column rather than to name it
            is_valid = false
            table.insert(msgs, "db:create - "..sheet_name.." - column name #"..key..
              " is a "..type(value)..", but a sheet's column names have to be strings.")
          end
        elseif type(key) == "string" and string.starts(key, "_") then
          options[key] = value
        elseif type(key) == "number" then
          -- past the end, at or below zero, or fractional: none of them is a
          -- position in the list, so none of them names a column
          is_valid = false
          table.insert(msgs, "db:create - "..sheet_name.." - ["..tostring(key)..
            "] is not a position in this "..column_count.." item list, so it names no column. "..
            "A sheet given as a list runs from 1 with no gaps.")
        else
          -- only a string key names a column the example can offer to declare
          local list_form, keyed_form = both_forms(type(key) == "string" and key or nil)
          is_valid = false
          table.insert(msgs, "db:create - "..sheet_name.." - "..string.format("%q", tostring(key))..
            " is a key, but a sheet given as a list takes its column names as list members. Write "..
            list_form..", or use the "..keyed_form.." form to give a column a default.")
        end
      end

    -- sheet provided in {"column1" = default} format
    else
      for key, value in pairs(sheet) do

        if string.starts(key, "_") then
          options[key] = value
        else
          columns[key] = value
        end
      end
    end

    if options._violations then
      local is_validations_valid, msg = db:_validate_validations(options._violations)
      if is_validations_valid == false then
        is_valid = false
        table.insert(msgs, "db:create - "..sheet_name.." - "..msg)
      end
    else
      options._violations = "FAIL"
    end

    if options._unique then
      local is_unique_valid, msg = db:_validate_unique_contraints(options._unique)
      if is_unique_valid == false then
        is_valid = false
        table.insert(msgs, "db:create - "..sheet_name.." - "..msg)
      end

      -- the readers of _unique want the list: the loop below walks it with ipairs,
      -- and db:merge_unique measures it with #, which on a string gives the length
      -- of the column name rather than one constraint and raises. db.__schema is
      -- only written here, so normalising once covers both
      if type(options._unique) == "string" then
        options._unique = { options._unique }
      end

      if type(options._unique) == "table" then
        -- the two forms reach the database by different routes, so what counts as a
        -- column the sheet has differs too: a compound entry is written into the
        -- CREATE TABLE for sqlite to resolve, which matches names case-insensitively
        -- and knows the _row_id every sheet is given, while a single column name is
        -- attached by an exact-match lookup in db:_build_create_table_sql
        local resolvable = { _row_id = true }
        for column_name in pairs(columns) do
          -- the keyed form takes every key that is not an option for a column name,
          -- numbers included, and sqlite resolves UNIQUE("2") against a column
          -- called 2 the same as it resolves any other name
          resolvable[tostring(column_name):lower()] = true
        end

        local wanted = {}

        for _, unique_entry in ipairs(options._unique) do
          local compound = type(unique_entry) == "table"
          local unique_columns = compound and unique_entry or {unique_entry}
          local unknown_column
          local names_row_id

          for _, column_name in ipairs(unique_columns) do
            if type(column_name) == "string" then
              local known
              if compound then
                known = resolvable[column_name:lower()] ~= nil
                names_row_id = names_row_id or column_name:lower() == "_row_id"
              else
                known = columns[column_name] ~= nil
              end

              if not known and not unknown_column then
                unknown_column = column_name
              end
            end
          end

          -- a UNIQUE naming a column sqlite cannot resolve is refused, and takes the
          -- whole CREATE TABLE with it, leaving no sheet at all. Dropping the
          -- constraint keeps the sheet, at the price of db:add then taking the
          -- duplicates it was meant to refuse; the single-column form never reached
          -- sqlite to be refused, so on a new sheet it only adds the message. On a
          -- sheet that exists either form can cost a rule the table enforces today,
          -- which is what has_skipped_unique carries to db:_migrate
          if compound and #unique_entry == 0 then
            has_skipped_unique = true
            table.insert(warnings, "db:create - "..sheet_name.." - _unique has an entry with no "..
              "column names in it: that constraint is skipped.")
          elseif not unknown_column then
            -- sqlite takes an entry naming _row_id, and the constraint can then never
            -- refuse a row, since a sheet's key is unique on its own. It is kept
            -- anyway: dropping it would change the sheet's SQL for no gain, and the
            -- has_skipped_unique route out of the rebuild is not free either, since a
            -- sheet with one would then never have its constraints reconciled again
            if names_row_id then
              table.insert(warnings, "db:create - "..sheet_name.." - _unique names \"_row_id\" in an "..
                "entry written as a list, and a sheet's _row_id is unique already, so that constraint "..
                "can never refuse a row: drop it, or name the columns you meant.")
            end

            wanted[#wanted + 1] = unique_entry
          elseif unknown_column == "_row_id" then
            has_skipped_unique = true
            table.insert(warnings, "db:create - "..sheet_name.." - _unique names \"_row_id\", the key "..
              "every sheet is given, which is unique already: that constraint is skipped. Naming it "..
              "in a compound entry makes one that can never refuse a row either.")
          else
            has_skipped_unique = true
            table.insert(warnings, "db:create - "..sheet_name.." - _unique names \""..unknown_column..
              "\", which is not one of the sheet's columns: that constraint is skipped.")
          end
        end

        -- nil rather than an empty list, so db:merge_unique can still tell a sheet
        -- with no unique index from one with several by which of its asserts fires
        options._unique = #wanted > 0 and wanted or nil
      end
    end

    -- A falsy _index means the sheet wants no indexes, the same as _unique and
    -- _violations above treat theirs
    if options._index then
      local is_index_valid, msg = db:_validate_index(options._index)
      if is_index_valid == false then
        is_valid = false
        table.insert(msgs, "db:create - "..sheet_name.." - "..msg)
      end

      -- A single column name is as good an _index as a list of them, but the
      -- readers of _index only handle the list: db:_drop_orphaned_indexes walks
      -- it with ipairs and db:_migrate_indexes ignores anything that is not a
      -- table. db.__schema is only written here, so normalising once covers
      -- both of them.
      if type(options._index) == "string" then
        options._index = { options._index }
      end

      -- An index on a column this sheet does not declare is dropped from the
      -- wanted set and reported rather than carried: db:_migrate_indexes cannot
      -- make the index a typo asks for, and leaving the entry in place has
      -- db:_drop_orphaned_indexes take the typo for part of the wanted set. It
      -- is a warning rather than an error because the rest of the sheet is
      -- sound and its data is reachable without the index, so a script that has
      -- lived with the stray name goes on working. What is left of _index is no
      -- longer everything the sheet asked for, though, so the sheet is marked
      -- for db:_drop_orphaned_indexes to leave the indexes it has alone. The
      -- shapes _validate_index refused above are left to it to report.
      if type(options._index) == "table" then
        local wanted = {}

        for _, index_entry in ipairs(options._index) do
          local index_columns = type(index_entry) == "table" and index_entry or {index_entry}
          local unknown_column

          for _, column_name in ipairs(index_columns) do
            if not unknown_column and type(column_name) == "string" and columns[column_name] == nil then
              unknown_column = column_name
            end
          end

          -- one unknown column costs the whole entry: a compound index on the
          -- rest of its columns is not the one that was asked for
          if not unknown_column then
            wanted[#wanted + 1] = index_entry
          else
            has_skipped_index = true

            if unknown_column == "_row_id" then
              table.insert(warnings, "db:create - "..sheet_name.." - _index names \"_row_id\", which is the "..
                "key every sheet is given rather than one of its own columns: that index is skipped.")
            elseif unknown_column:lower() == "asc" or unknown_column:lower() == "desc" then
              -- db:_sql_columns would build the ordering term, but
              -- db:_index_valid refuses it, so the index was never made
              table.insert(warnings, "db:create - "..sheet_name.." - _index names \""..unknown_column..
                "\", and an index takes column names only, not a sort direction: that index is skipped.")
            else
              table.insert(warnings, "db:create - "..sheet_name.." - _index names \""..unknown_column..
                "\", which is not one of the sheet's columns: that index is skipped.")
            end
          end
        end

        options._index = wanted
      end
    end

    schema[sheet_name] = {
      columns = columns,
      options = options,
      has_skipped_index = has_skipped_index,
      has_skipped_unique = has_skipped_unique,
    }
  end

  assert(is_valid, table.concat(msgs, "\n"))

  -- after the assert: what is wrong with the schema comes before what was left
  -- out of it, and a sheet that never got made has nothing to warn about
  for _, warning in ipairs(warnings) do
    printError(warning, true, false)
  end

  if not db:_isActiveDBName(db_name) then
    -- the driver answers nil plus a reason for a file it can not open, which a
    -- read-only profile directory or a full disk both produce: without this the
    -- setautocommit below is the nil index instead
    local conn, err = db.__env:connect(getMudletHomeDir() .. "/Database_" .. db_name .. ".db")
    if not conn then
      error("db:create could not open the database file for "..db_name..": "..tostring(err), 2)
    end

    db.__conn[db_name] = conn
    conn:setautocommit(false)
    db.__autocommit[db_name] = true
  end

  db.__schema[db_name] = schema

  for sheet_name, _ in pairs(sheets) do
    db:_migrate(db_name, sheet_name, force)
  end
  return db:get_database(db_name)
end


local function normalize_sql(sql)
  return (
    sql:lower()
    :gsub("\n", " ")
    :gsub("\r", " ")
    :gsub("%s+", " ")
    :gsub("^%s*(.-)%s*$", "%1")
  )
end


-- NOT LUADOC
-- Extracts UNIQUE constraints from a CREATE TABLE statement.
-- This includes both column-level constraints (e.g., "col1" TEXT UNIQUE ON CONFLICT REPLACE)
-- and table-level constraints (e.g., UNIQUE("col1", "col2") ON CONFLICT FAIL), each of
-- which may come without its ON CONFLICT clause (e.g., "col1" TEXT UNIQUE).
-- This allows us to detect when constraint definitions have changed without being affected by
-- column additions/removals.
function db:_extract_table_constraints(sql)
  if not sql or sql == "" then
    return ""
  end

  -- Normalize whitespace and case for consistent comparison
  local normalized = normalize_sql(sql)

  -- Extract the part between the parentheses of CREATE TABLE
  local content = normalized:match("create%s+table%s+[%w_\"]+%s*%((.+)%)")
  if not content then
    return ""
  end

  local constraints = {}

  -- A column name and a default value are both quoted, and either can hold the
  -- word, so the search runs over a copy with the quoted parts blanked out.
  -- Same-length blanks keep every offset lined up with the content itself.
  local function blank(quoted)
    return (" "):rep(#quoted)
  end
  local searchable = content:gsub('"[^"]*"', blank)
  searchable = searchable:gsub("'[^']*'", blank)

  -- Each UNIQUE is picked up with the column list it may carry, then with the
  -- ON CONFLICT clause it may carry. Both parts are optional: SQLite defaults
  -- the conflict resolution to ABORT, so a sheet whose table was not written by
  -- this module can hold a bare UNIQUE, and a bare one has to be seen or a
  -- change in uniqueness compares equal to no uniqueness at all.
  local position = 1
  while true do
    local start, stop = searchable:find("unique", position, true)
    if not start then
      break
    end
    position = stop + 1

    -- and a column called unique_id is not one either
    local before = start > 1 and searchable:sub(start - 1, start - 1) or " "
    local after = searchable:sub(stop + 1, stop + 1)
    if not before:match("[%w_]") and not after:match("[%w_]") then
      local constraint = "unique"

      local columns_start, columns_stop = content:find("^%s*%([^)]+%)", position)
      if columns_start then
        constraint = constraint .. content:sub(columns_start, columns_stop)
        position = columns_stop + 1
      end

      local conflict_start, conflict_stop = content:find("^%s+on%s+conflict%s+%w+", position)
      if conflict_start then
        constraint = constraint .. content:sub(conflict_start, conflict_stop)
        position = conflict_stop + 1
      end

      table.insert(constraints, constraint)
    end
  end

  -- Sort for consistent comparison
  table.sort(constraints)

  return table.concat(constraints, "|")
end



-- The pieces a CREATE TABLE body, or a UNIQUE column list, is written in: split
-- on the commas outside any brackets, so a compound UNIQUE stays in one piece.
-- The commas are counted off a copy with the quoted parts blanked out, which is
-- what keeps a comma inside a name or a default value from being one of them,
-- and both copies are returned since the caller reads names off the text and
-- searches the blanked one.
local function split_on_commas(text, searchable)
  local pieces = {}
  local depth, piece_start = 0, 1

  local function add(stop_at)
    pieces[#pieces + 1] = { text = text:sub(piece_start, stop_at), scan = searchable:sub(piece_start, stop_at) }
  end

  for position = 1, #searchable do
    local char = searchable:sub(position, position)
    if char == "(" then
      depth = depth + 1
    elseif char == ")" then
      depth = depth - 1
    elseif char == "," and depth == 0 then
      add(position - 1)
      piece_start = position + 1
    end
  end
  add(#text)

  return pieces
end


-- The column name a piece of a CREATE TABLE opens with. This module quotes every
-- name it writes, but a sheet whose table was not written by it need not: reading
-- quoted names only reduces each of that table's UNIQUEs to the same empty name,
-- and a sheet whose rules all look alike is one that can be told it is losing a
-- rule it is not. A name written in some other way sqlite takes, [name] among
-- them, does come back empty, which no expected CREATE TABLE ever holds, so that
-- sheet is left as it is rather than losing a rule to a misreading.
local function leading_column_name(text)
  local trimmed = text:match("^%s*(.-)%s*$")

  return trimmed:match('^"([^"]*)"') or trimmed:match("^([%w_$]+)") or ""
end


-- The columns each UNIQUE in a CREATE TABLE covers, sorted and counted. Sorted
-- because sqlite enforces the same rule whichever order they are written in, and
-- without the ON CONFLICT clause because that is a _violations change, which
-- costs no uniqueness. db:_extract_table_constraints is no use for this: it drops
-- the column name of a column-level UNIQUE, so a rule moving from one column to
-- another looks to it like no change at all.
local function unique_targets(sql)
  local counts = {}
  local content = normalize_sql(sql or ""):match("^create table [^(]+%((.+)%)$")
  if not content then
    return counts
  end

  -- a column name and a default value are both quoted, and either can hold a
  -- comma, a bracket or the word itself, so the scan runs over a copy with the
  -- quoted parts blanked out. Same-length blanks keep the offsets lined up
  local function blank(quoted)
    return (" "):rep(#quoted)
  end
  local searchable = content:gsub('"[^"]*"', blank):gsub("'[^']*'", blank)

  for _, part in ipairs(split_on_commas(content, searchable)) do
    local text, scan = part.text, part.scan

    local start, stop = scan:find("unique", 1, true)
    while start and (scan:sub(start - 1, start - 1):match("[%w_]") or scan:sub(stop + 1, stop + 1):match("[%w_]")) do
      -- a column called unique_id is not one
      start, stop = scan:find("unique", stop + 1, true)
    end

    if start then
      -- a table-level UNIQUE names its columns in the brackets that follow, a
      -- column-level one covers the column it is written on, which is the name
      -- the part opens with
      local columns = {}
      local list_start, list_stop = scan:find("^%s*%b()", stop + 1)

      if list_start then
        local list_text = text:sub(list_start, list_stop):match("^%s*%((.*)%)$")
        local list_scan = scan:sub(list_start, list_stop):match("^%s*%((.*)%)$")
        for _, entry in ipairs(split_on_commas(list_text, list_scan)) do
          columns[#columns + 1] = leading_column_name(entry.text)
        end
      else
        columns[1] = leading_column_name(text)
      end

      table.sort(columns)
      -- "\0" rather than a comma, which a column name can hold: joining on one
      -- makes a column called a,b compare equal to a UNIQUE over a and b
      local target = table.concat(columns, "\0")
      counts[target] = (counts[target] or 0) + 1
    end
  end

  return counts
end


-- Whether rebuilding a table to match the schema would leave it without a
-- uniqueness rule it carries today.
local function drops_a_unique(expected_sql, actual_sql)
  local expected = unique_targets(expected_sql)
  for target, count in pairs(unique_targets(actual_sql)) do
    if (expected[target] or 0) < count then
      return true
    end
  end

  return false
end


local function count_rows(conn, s_name)
  local count_cursor, count_err = conn:execute("SELECT COUNT(*) as cnt FROM " .. s_name);
  if count_cursor == nil then
    return nil, count_err
  end

  local count = count_cursor:fetch({}, "a").cnt;
  count_cursor:close();

  return count, nil;
end


-- NOT LUADOC
-- The migrate function is meant to upgrade an existing database live, to maintain a consistent
-- and correct set of sheets and fields, along with their indexes. It should be safe to run
-- at any time, and must not cause any data loss. It simply adds to what is there: in perticular
-- it is not capable of removing indexes, columns, or sheets after they have been defined.
function db:_migrate(db_name, s_name, force)
  local conn = db.__conn[db_name]
  -- db:create is the only caller outside the specs and it has a live connection by
  -- the time it gets here, so this only fires for a script calling db:_migrate
  -- directly. Note the first argument is a database name, not a sheet
  if not conn then error(db_no_connection_message("migrate", db_name, "a database name"), 2) end

  local schema = db.__schema[db_name][s_name]

  local current_columns = {}

  -- The PRAGMA table_info command is a query which returns all of the columns currently
  -- defined in the specified table. The purpose of this section is to see if any new columns
  -- have been added.
  db:echo_sql("PRAGMA table_info('" .. s_name .. "')")
  local cur = conn:execute("PRAGMA table_info('" .. s_name .. "')") -- currently broken - LuaSQL bug, needs to be upgraded for new sqlite API

  if type(cur) == "userdata" then
    local row = cur:fetch({}, "a")
    if row then
      while row do
        current_columns[row.name] = row.type
        row = cur:fetch({}, "a")
      end
    else
      ---------------  GETS ALL COLUMNS FROM SHEET IF IT EXISTS
      db:echo_sql("SELECT * FROM " .. s_name)
      local get_sheet_cur = conn:execute("SELECT * FROM " .. s_name)  -- select the sheet
      if get_sheet_cur and get_sheet_cur ~= 0 then
        local colnames = get_sheet_cur:getcolnames()

        if colnames then
          -- add each column from row to current_columns table
          for _, v in ipairs(colnames) do
            current_columns[v] = ""
          end
        end
        get_sheet_cur:close()
      end
    end
  end

  if type(cur) == "userdata" then
    cur:close()
  end

  -- The SQL definition of a column is:
  --    "column_name" column_type NULL
  -- The db module does not presently support columns that are required. Everything is optional,
  -- everything may be NULL / nil.
  -- If you specify a column's type, you also specify its default value.
  if table.is_empty(current_columns) then
    -- At this point, we know that the specified table does not exist in the database and so we
    -- should create it.

    -- Every sheet has an implicit _row_id column. It is not presently (and likely never will be)
    -- supported to define the primary key of any sheet.
    local sql = db:_build_create_table_sql(schema, s_name)
    db:echo_sql(sql)
    conn:execute(sql)

  else
    -- At this point we know that the sheet already exists, but we are concerned if the current
    -- definition includes columns which may be added.

    -- Check if the table-level constraints have changed (e.g., _violations option changed)
    -- by comparing only the UNIQUE constraint definitions, not the column list
    local expected_sql = db:_build_create_table_sql(schema, s_name)
    local get_actual_sql = "SELECT sql FROM sqlite_master " ..
                           "WHERE type = 'table' AND name = '" .. s_name .. "'"
    db:echo_sql(get_actual_sql)
    local sql_cur, sql_err = conn:execute(get_actual_sql)
    local table_constraints_changed = false
    local would_drop_a_unique = false
    local unique_lost_to_column_removal = false

    if sql_cur and type(sql_cur) ~= "number" then
      local sql_row = sql_cur:fetch({}, "a")
      sql_cur:close()

      if sql_row and sql_row.sql then
        local actual_sql = sql_row.sql
        local expected_constraints = db:_extract_table_constraints(expected_sql)
        local actual_constraints = db:_extract_table_constraints(actual_sql)

        if expected_constraints ~= actual_constraints then
          table_constraints_changed = true
          -- what is left of _unique after db:create skipped an entry is not the set
          -- the sheet asked for, so a rebuild that costs uniqueness costs it off a
          -- typo - and the create that spells the column right again cannot put the
          -- rule back over the duplicates the meantime let in. Everything else the
          -- rebuild would do, adding a constraint or changing _violations among it,
          -- is still applied: freezing those too loses the sheet changes it asked
          -- for and says nothing about them
          if schema.has_skipped_unique and drops_a_unique(expected_sql, actual_sql) then
            -- removing a column is a rebuild in its own right, further down and from
            -- the same pruned _unique, so holding this one back would only move where
            -- the rule is lost - and leaving the column instead is worse, since
            -- db.Sheet's __index raises for a column the schema has no entry for
            local removes_a_column = false
            for column_name in pairs(current_columns) do
              -- against nil rather than for truth: a column declared with a default
              -- of false is one the sheet has, and reading it as one that is going
              -- costs the sheet the very rule this guard is here to keep
              if column_name ~= "_row_id" and schema.columns[column_name] == nil then
                removes_a_column = true
                break
              end
            end

            would_drop_a_unique = not removes_a_column

            if would_drop_a_unique then
              printError("db:create - "..s_name.." - the uniqueness this sheet already enforces is "..
                "kept rather than rebuilt to match what is left of _unique: spell the skipped "..
                "entries right and it is applied then.", true, false)
            else
              -- said once the rebuild is past the guard that can still halt it, so
              -- this reports what happened rather than what was about to
              unique_lost_to_column_removal = true
            end
          end
        end
      end
    end

    -- If the table-level constraints have changed, we need to recreate the table
    if table_constraints_changed and not would_drop_a_unique then
      -- Commit any pending transaction before table recreation
      db:echo_sql("COMMIT")
      conn:commit()

      -- Check if we're deleting columns that contain data (unless force flag is set)
      local redundant_columns = {}
      for k, _ in pairs(current_columns) do
        -- against nil rather than for truth, for the reason the guard above gives:
        -- a column defaulting to false is declared, so it is not redundant
        if schema.columns[k] == nil and k ~= "_row_id" then
          redundant_columns[#redundant_columns + 1] = k
        end
      end

      if #redundant_columns > 0 and not force then
        -- Check if any of the redundant columns contain non-null data
        local not_blank = {}
        for _, col in ipairs(redundant_columns) do
          local check_sql = string.format('SELECT COUNT(*) AS cnt FROM %s WHERE "%s" IS NOT NULL', s_name, col)
          db:echo_sql(check_sql)
          local check_cur, check_err = conn:execute(check_sql)
          assert(check_cur, check_err)

          if type(check_cur) ~= "number" then
            local check_row = check_cur:fetch({}, "a")
            check_cur:close()

            if check_row and check_row.cnt and tonumber(check_row.cnt) > 0 then
              not_blank[#not_blank + 1] = col
            end
          end
        end

        assert(not not_blank[1] or force,
               "db:_migrate halted due to data present in undefined columns: " .. table.concat(not_blank, ", ") ..
               "\nuse force option to drop anyway.")
      end

      if unique_lost_to_column_removal then
        printError("db:create - "..s_name.." - removing a column rebuilds this sheet from what is "..
          "left of _unique, so the uniqueness it enforces today is lost whichever column that rule "..
          "sits on: spell the skipped entries right before removing a column.", true, false)
      end

      -- Build the list of columns to preserve (only columns that exist in both current and new schema)
      local fields = { "_row_id" }
      for k, _ in pairs(schema.columns) do
        if current_columns[k] then
          fields[#fields + 1] = string.format('"%s"', k)
        end
      end
      local fields_sql = table.concat(fields, ", ")

      -- Get the current CREATE TABLE statement to use for the backup
      local get_create = "SELECT sql FROM sqlite_master " ..
                        "WHERE type = 'table' AND name = '" .. s_name .. "'"
      db:echo_sql(get_create)
      local create_cur, create_err = conn:execute(get_create)
      assert(create_cur, create_err)

      if type(create_cur) ~= "number" then
        local row = create_cur:fetch({}, "a")
        create_cur:close()

        -- Ensure we got a result
        if not row or not row.sql then
          error("Unable to fetch CREATE TABLE statement for table: " .. s_name)
        end

        local original_count, og_count_err = count_rows(conn, s_name);
        assert(original_count, og_count_err);

        -- Create temporary backup table, recreate main table, copy data
        local create_tmp = row.sql:gsub(s_name, s_name .. "_bak")
        create_tmp = create_tmp:gsub("TABLE", "TEMPORARY TABLE")

        local sql_chunks = {}
        sql_chunks[#sql_chunks + 1] = create_tmp .. ";"
        sql_chunks[#sql_chunks + 1] = "INSERT INTO " .. s_name .. "_bak SELECT * FROM " .. s_name .. ";"
        sql_chunks[#sql_chunks + 1] = "DROP TABLE " .. s_name .. ";"

        local new_create_sql = db:_build_create_table_sql(schema, s_name)

        sql_chunks[#sql_chunks + 1] = new_create_sql .. ";"
        sql_chunks[#sql_chunks + 1] = string.format("INSERT INTO %s SELECT %s FROM %s_bak;", s_name, fields_sql, s_name)
        sql_chunks[#sql_chunks + 1] = "DROP TABLE " .. s_name .. "_bak;"

        for i, sql in ipairs(sql_chunks) do
          db:echo_sql(sql)
          local ret, str = conn:execute(sql)

          if not ret then
            conn:rollback()
            error("Migration failed at chunk " .. i .. ": " .. tostring(str))
          end
        end

        local migrated_count, migrated_count_err = count_rows(conn, s_name)
        if migrated_count_err then
          conn:rollback()
          error(migrated_count_err);
        end

        if (original_count ~= migrated_count and not force) then
            conn:rollback()
            error(
               "db:_migrate halted for ".. s_name .." during constraint migrations due to data loss."
               .."\n\t".. (original_count - migrated_count) .." rows would be lost with new constraints."
               .."\nUse force option to migrate anyway."
            )
        end

        -- Commit the migration transaction
        db:echo_sql("COMMIT")
        conn:commit()

        -- After recreating the table with new constraints, add any new columns that didn't exist before
        for k, v in pairs(schema.columns) do
          if not current_columns[k] then
            local sql_add = 'ALTER TABLE %s ADD COLUMN "%s" %s NULL DEFAULT %s'
            local t = db:_sql_type(v)
            local def = db:_sql_convert(v)
            local sql = sql_add:format(s_name, k, t, def)
            db:echo_sql(sql)
            conn:execute(sql)
            -- Update current_columns to reflect the newly added column
            current_columns[k] = ""
          end
        end
      end
    else
      -- No table definition change to apply - either there was none, or applying it
      -- would have cost the sheet a uniqueness rule it enforces today
      local missing = {}

    for k, v in pairs(schema.columns) do

      -- Here we test it a given column exists in the sheet already, and if not, we add that
      -- column.
      if not current_columns[k] then
        missing[#missing + 1] = { name = k, default = v }
      end
    end

    if #missing > 0 and
    table.size(current_columns) + #missing == table.size(schema.columns) + 1
    -- We have changes and when we did those changes, we have exactly
    -- the number of columns we need. The "+1" is for the _row_id
    -- which is not in the schema.
    then
      local sql_add = 'ALTER TABLE %s ADD COLUMN "%s" %s NULL DEFAULT %s'
      for _, v in ipairs(missing) do
        local t = db:_sql_type(v.default)
        local def = db:_sql_convert(v.default)
        local sql = sql_add:format(s_name, v.name, t, def)
        db:echo_sql(sql)
        conn:execute(sql)
      end
    elseif
    #missing + table.size(current_columns) > table.size(schema.columns) + 1
    -- if we add all missing columns and we have more columns than we want
    -- then there are currently some columns we don't want anymore.
    then
      --find the redundant columns
      local redundant_columns = {}
      for k, _ in pairs(current_columns) do
        if k ~= "_row_id" and not schema.columns[k] then
          table.insert(redundant_columns, k)
        end
      end

      --check if any of the redundant columns are non-empty
      local max_redundant = {}
      for i, v in ipairs(redundant_columns) do
        max_redundant[i] = string.format([[max(%s) AS %s]], v, v)
      end
      local sql_check_blank = [[SELECT %s from %s;]]
      local sql = sql_check_blank:format(table.concat(max_redundant, ", "), s_name)
      db:echo_sql(sql)
      local blank_cur = conn:execute(sql)
      local blank_results = blank_cur:fetch({}, "a")
      blank_cur:close()
      local not_blank = {}
      for k, _ in pairs(blank_results) do
        table.insert(not_blank, k)
      end

      -- don't drop non-empty columns unless force flag is provided
      assert(not not_blank[1] or force, "db:_migrate halted due to data present in undefined columns: "..table.concat(not_blank, ","))

      local get_create = "SELECT sql FROM sqlite_master " ..
      "WHERE type = 'table' AND " ..
      "name = '" .. s_name .. "'"
      local ret_str
      db:echo_sql(get_create)
      cur, ret_str = conn:execute(get_create)
      assert(cur, ret_str)
      if type(cur) ~= "number" then
        local row = cur:fetch({}, "a");
        cur:close()

        -- Ensure we got a result
        if not row or not row.sql then
          error("Unable to fetch CREATE TABLE statement for table: " .. s_name)
        end

        local create_tmp = row.sql:gsub(s_name, s_name .. "_bak")
        local sql_chunks = {}
        local fields = { "_row_id" }
        local sql

        create_tmp = create_tmp:gsub("TABLE", "TEMPORARY TABLE")

        for k, _ in pairs(schema.columns) do
          fields[#fields + 1] = string.format('"%s"', k)
        end
        local fields_sql = table.concat(fields, ", ")

        sql_chunks[#sql_chunks + 1] = create_tmp .. ";"
        sql_chunks[#sql_chunks + 1] = "INSERT INTO " .. s_name .. "_bak " ..
        "SELECT * FROM " .. s_name .. ";"
        sql_chunks[#sql_chunks + 1] = "DROP TABLE " .. s_name .. ";"
        sql_chunks[#sql_chunks + 1] = db:_build_create_table_sql(schema,
        s_name) .. ";"
        sql_chunks[#sql_chunks + 1] = string.format(
        "INSERT INTO %s SELECT %s FROM %s_bak;", s_name, fields_sql,
        s_name)
        sql_chunks[#sql_chunks + 1] = "DROP TABLE " .. s_name .. "_bak;"

        for _, sql in ipairs(sql_chunks) do
          db:echo_sql(sql)
          local ret, str = conn:execute(sql)
          assert(ret, str)
        end
      end
    end
    end -- end of else block for table_constraints_changed check
  end

  -- On every invocation of db:create we run the code that creates indexes, as that code will
  -- do nothing if the specific indexes already exist. This is enforced by the db:_index_name
  -- function creating a unique index.

  -- make up current_columns, as pragma_info currently does not populate it, due to luasql bug
  for key, value in pairs(schema.columns) do
    current_columns[key] = db:_sql_type(value)
  end

  local res_drops, msg = db:_drop_orphaned_indexes(conn, s_name, schema)
  assert(res_drops, msg)
  db:_migrate_indexes(conn, s_name, schema, current_columns)
  db:echo_sql("COMMIT")
  conn:commit()
  conn:execute("VACUUM")
end

function db:_build_create_table_sql(schema, s_name)
  local sql_column = '"%s" %s NULL'
  local sql_column_default = sql_column .. ' DEFAULT %s'

  local on_conflict = "ON CONFLICT "..(schema.options._violations or "FAIL")

  local sql_chunks = { '"_row_id" INTEGER PRIMARY KEY AUTOINCREMENT' }

  local unique_column_constraints = {}
  local unique_table_constraints = {}

  -- Validations were already performed in db:create, so the only thing
  -- we need to do here is filter our unique constraints to the appropirate
  -- tables.
  --
  -- Into unique_column_constraints when the a column is unique on its own
  -- and into unique_table_constraints when columns are grouped together.
  if schema.options and schema.options._unique then
    if type(schema.options._unique) == "string" then
      table.insert(unique_column_constraints, schema.options._unique)
    elseif type(schema.options._unique) == "table" then
      for i, unique_constraint in ipairs(schema.options._unique) do
        if type(unique_constraint) == "string" then
          table.insert(unique_column_constraints, unique_constraint)
        elseif type(unique_constraint) == "table" then
          table.insert(
            unique_table_constraints,
            'UNIQUE("'..table.concat(unique_constraint, '", "')..'") '..on_conflict
          )
        end
      end
    end
  end

  -- We iterate over every defined column, and add a line which creates it.
  for col_name, col_schema in pairs(schema.columns) do
    local sql = ""
    if col_schema == nil then
      sql = sql_column:format(col_name, db:_sql_type(col_schema))
    else
      sql = sql_column_default:format(col_name, db:_sql_type(col_schema), db:_sql_convert(col_schema))
    end
    if table.contains(unique_column_constraints, col_name) then
      sql = sql .. " UNIQUE "..on_conflict
    end
    sql_chunks[#sql_chunks + 1] = sql
  end

  -- Add in the unique constraints
  for _, unique_table_constraint in ipairs(unique_table_constraints) do
    sql_chunks[#sql_chunks + 1] = unique_table_constraint
  end

  return "CREATE TABLE " .. s_name.. " ("..table.concat(sql_chunks, ", ")..")"
end


-- Conditionally drops orphaned indexes.
function db:_drop_orphaned_indexes(conn, s_name, schema)
  local cur, err;

  -- db:create dropped an _index entry naming a column the sheet does not have,
  -- so what is left of _index is not the whole set the sheet asked for and
  -- pruning against it would take the indexes the typo never mentioned with it.
  -- Nothing is dropped this time round; the create that spells the column right
  -- prunes as usual.
  if schema.has_skipped_index then
    return true, nil
  end

  local sql = ([[
    SELECT
      name,
      tbl_name,
      sql
    FROM sqlite_master a
    WHERE
        type = 'index'
        AND tbl_name = '%s'
        AND a.sql IS NOT NULL;
  ]]):format(s_name):trim():gsub("%s+", " ")

  local sql_drop_index = "DROP INDEX IF EXISTS %s;"

  db:echo_sql(sql)
  cur, err = conn:execute(sql)
  if cur == nil then
    return nil, err
  end


  local row = cur:fetch({}, "a")
  local drops = {}

  -- No indexes should exist for the sheet, should remove all indexes for the table.
  if not schema.options._index then
    while row do
      table.insert(
        drops,
        sql_drop_index:format(row.name)
      )
      row = cur:fetch({}, "a")
    end
    cur:close()
    for _, drop in ipairs(drops) do
      db:echo_sql(drop)
      cur, err = conn:execute(drop)
      if cur == nil then
        return nil, err;
      end
    end
    return true, nil
  end


  local index_strs = {}

  for i, index in ipairs(schema.options._index) do
    if type(index) == "table" then
      local t = {}
      for j, col_name in ipairs(index) do
        t[j] = col_name:lower()
      end
      table.sort(t);
      index_strs[i] = table.concat(t, ',')
    else
      index_strs[i] = index:lower()
    end
  end

  local cols         = {}
  local index_match  = ""
  local is_dangling  = true;
  local cols_str     = ""
  local unique_match = ""

  while row do
    cols = {}
    sql = normalize_sql(row.sql)
    index_match  = sql:match("create%s+index%s+idx[%w_]+%s+on%s+[%w_]+%s+%((.+)%)")
    unique_match = sql:match("create%s+unique%s+index%s+idx[%w_]+%s+on%s+[%w_]+%s+%((.+)%)")

    -- matched unique index.  Mudlet doesn't make these anymore, so it should be removed.
    if unique_match then
        table.insert(
          drops,
          sql_drop_index:format(row.name)
        )

    elseif index_match then
      cols = {}
      for col in index_match:gmatch('"(%w+)"') do
        table.insert(cols, col)
      end
      table.sort(cols)
      cols_str = table.concat(cols, ',')

      is_dangling = true

      for _, index_str in ipairs(index_strs) do
        if cols_str == index_str then
          is_dangling = false
          break
        end
      end

      if is_dangling then
        table.insert(
          drops,
          sql_drop_index:format(row.name)
        )
      end

    end

    row = cur:fetch({}, "a")
  end
  cur:close()

  for _, drop in ipairs(drops) do
    db:echo_sql(drop)
    cur, err = conn:execute(drop)
    if cur == nil then
      return nil, err;
    end
  end

  return true, nil
end


-- NOT LUADOC
-- Creates any indexes which do not yet exist in the given database.
function db:_migrate_indexes(conn, s_name, schema, current_columns)
  local sql_create_index = "CREATE INDEX IF NOT EXISTS %s ON %s (%s);"
  local sql = ""

  if (type(schema.options._index) == "table") then
    for _, value in pairs(schema.options._index) do
      -- If an index references a column which does not presently exist within the schema
      -- this will fail.
      if db:_index_valid(current_columns, value) then
        --assert(db:_index_valid(current_columns, value),
        --      "In sheet "..s_name.." an index field is specified that does not exist.")
        sql = sql_create_index:format(
          db:_index_name(s_name, value),
          s_name,
          db:_sql_columns(value)
        )
        db:echo_sql(sql)
        conn:execute(sql)
      end

    end
  end
end


--- Adds one or more new rows to the specified sheet. If any of these rows would violate a UNIQUE index,
--- a lua error will be thrown and execution will cancel. As such it is advisable that if you use a UNIQUE
--- index, you test those values before you attempt to insert a new row. <br/><br/>
---
--- Each table is a series of key-value pairs to set the values of the sheet, but if any keys do not exist
--- then they will be set to nil or the default value. As you can see, all fields are optional.
---
--- @usage Adding one record.
---   <pre>
---   db:add(mydb.enemies, {name="Bob Smith", city="San Francisco"})
---   </pre>
--- @usage Adding multiple records.
---   <pre>
---   db:add(mydb.enemies,
---     {name="John Smith", city="San Francisco"},
---     {name="Jane Smith", city="San Francisco"},
---     {name="Richard Clark"}
---   )
---   </pre>
--- @return boolean result true when the rows went in, nil when the insert failed. A
---   multi-row call stops at the first row the database refuses, so earlier rows of
---   that same call are left in the transaction uncommitted rather than rolled back.
--- @return string message Why the insert failed, absent when it did not.
function db:add(sheet, ...)
  local db_name = sheet._db_name
  local s_name = sheet._sht_name
  assert(s_name, "First argument to db:add must be a proper Sheet object.")

  local conn = db.__conn[db_name]
  if not conn then
    -- printed as well as returned, the way the SQL failure below is: the callers
    -- of db:add that check the result are outnumbered by the ones that do not
    local msg = db_no_connection_message("add to", db_name, A_SHEET)
    printError(msg, true, false)
    return nil, msg
  end

  local sql_insert = "INSERT INTO %s %s VALUES %s"

  for _, t in ipairs({ ... }) do
    if t._row_id then
      -- You are not permitted to change a _row_id
      t._row_id = nil
    end

    local sql = sql_insert:format(
      s_name,
      db:_sql_fields(t),
      db:_sql_values(t)
    )
    db:echo_sql(sql)

    local result, msg = conn:execute(sql)
    if result == nil then
      printError(msg, true, false)
      return nil, msg
    end
  end
  if db.__autocommit[db_name] then
    conn:commit()
  end
  return true
end



--- Execute SQL select query against database. This only useful for some very specific cases. <br/>
--- Use db:fetch if possible instead - this function should not be normally used!
---
--- @release post Mudlet 1.1.1 (<b><u>TODO update before release</u></b>)
---
--- @usage Following will select all distinct area from my kills DB.
---   <pre>
---   db:fetch_sql(mydb.kills, "SELECT distinct area FROM kills")
---   </pre>
---
--- @return table results The matching rows, an empty table when the query matched nothing,
---   or nil when the SQL could not run. Raises when the database is closed.
--- @see db:fetch
function db:fetch_sql(sheet, sql)
  local db_name = sheet._db_name
  local conn = db.__conn[db_name]
  -- raises rather than answering nil, which db:merge_unique below shows the reason for:
  -- "local results = db:fetch(...)  if results and results[1] then update else add end"
  -- reads a nil as "not there yet" and inserts, so a closed database would silently
  -- take the insert branch every time round instead of stopping
  if not conn then error(db_no_connection_message("fetch from", db_name, A_SHEET), 2) end

  db:echo_sql(sql)
  local cur = conn:execute(sql)

  -- if we had a syntax error in our SQL, cur will be nil
  if cur and cur ~= 0 then
    local results = {}
    local columns = cur:getcolnames()
    local row = cur:fetch({}, "a")

    while row do
      results[#results + 1] = db:_coerce_sheet(sheet, row, columns)
      row = cur:fetch({}, "a")
    end
    cur:close()
    return results
  else
    return nil
  end
end



--- Returns a table array containing a table for each matching row in the specified sheet. All arguments
--- but sheet are optional. If query is nil, the entire contents of the sheet will be returned. <br/><br/>
---
--- Query is a string which should be built by calling the various db: expression functions, such as db:eq,
--- db:AND, and such. You may pass a SQL WHERE clause here if you wish, but doing so is very dangerous.
--- If you don't know SQL well, its best to build the expression.<br/><br/>
---
--- Query may also be a table array of such expressions, if so they will be AND'd together implicitly.<br/><br/>
---
--- The results that are returned are not in any guaranteed order, though they are usually the same order
--- as the records were inserted. If you want to rely on the order in any way, you must pass a value to the
--- order_by field. This must be a table array listing the fields you want to sort by.
--- It can be { mydb.kills.area }, or { mydb.kills.area, mydb.kills.name } <br/><br/>
---
--- The results are returned in ascending (smallest to largest) order; to reverse this pass true into the final field.
---
--- @usage The first will fetch all of your enemies, sorted first by the city they reside in and then by their name.
---   <pre>
---   db:fetch(mydb.enemies, nil, {mydb.enemies.city, mydb.enemies.name})
---   </pre>
--- @usage The second will fetch only the enemies which are in San Francisco.
---   <pre>
---   db:fetch(mydb.enemies, db:eq(mydb.enemies.city, "San Francisco"))
---   </pre>
--- @usage The third will fetch all the things you've killed in Undervault which have Drow in their name.
---   <pre>
---   db:fetch(mydb.kills,
---      {
---         db:eq(mydb.kills.area, "Undervault"),
---         db:like(mydb.kills.name, "%Drow%")
---      }
---   )
---   </pre>
---
--- @return table results The matching rows, an empty table when the query matched nothing,
---   or nil when the SQL could not run. Raises when the database is closed.
--- @see db:fetch_sql
function db:fetch(sheet, query, order_by, descending)
  local s_name = sheet._sht_name
  -- a table with no sheet name is not a sheet whatever else it carries, so its
  -- _db_name is no more to be trusted than the rest of it
  local db_name = s_name and sheet._db_name or nil

  -- db:fetch_sql's own argument guard is too late for this one: the concatenation
  -- below reaches a nil sheet name first and dies there
  if not s_name then error(db_no_connection_message("fetch from", db_name, A_SHEET), 2) end

  local sql = "SELECT * FROM " .. s_name

  if query then
    if type(query) == "table" and not query._isExp then
      sql = sql .. " WHERE " .. db:AND(unpack(query))
    else
      sql = sql .. " WHERE " .. tostring(query)
    end
  end

  if order_by then
    local o = {}
    for _, v in ipairs(order_by) do
      assert(v.name, "You must pass field instances (as obtained from yourdb.yoursheet.yourfield) to sort.")
      o[#o + 1] = v.name

      if descending then
        o[#o + 1] = "DESC"
      end
    end

    sql = sql .. " ORDER BY " .. db:_sql_columns(o)
  end

  -- the closed database is checked here rather than left to db:fetch_sql, whose own
  -- guard would raise without naming a line: the return below is a tail call, so this
  -- frame is already gone when its error(msg, 2) goes looking for level 2
  if not db.__conn[db_name] then error(db_no_connection_message("fetch from", db_name, A_SHEET), 2) end

  return db:fetch_sql(sheet, sql)
end



--- Returns the result of calling the specified aggregate function on the field and its sheet. <br/><br/>
---
--- The supported aggregate functions are:
---   <pre>
---   COUNT - Returns the total number of records that are in the sheet or match the query.
---   AVG   - Returns the average of all the numbers in the specified field.
---   MAX   - Returns the highest number in the specified field.
---   MIN   - Returns the lowest number in the specified field.
---   TOTAL - Returns the value of adding all the contents of the specified field.
---   </pre>
---
--- @param query optional
---
--- @usage Example:
---   <pre>
---   local mydb = db:get_database("my database")
---   echo(db:aggregate(mydb.enemies.name, "count"))
---   </pre>
function db:aggregate(field, fn, query, distinct)
  local db_name = field.database
  local s_name = field.sheet
  assert(type(field) == "table", "Field must be a field reference.")
  assert(field.name, "Field must be a real field reference.")

  -- the connection check goes last: a bad field reference is the more useful complaint
  local conn = db.__conn[db_name]
  if not conn then error(db_no_connection_message("aggregate over", db_name, A_FIELD), 2) end

  local sql_chunks = { "SELECT", fn, "(", distinct and "DISTINCT" or "", field.name, ")", "AS", fn, "FROM", s_name }

  if query then
    sql_chunks[#sql_chunks + 1] = "WHERE"
    if type(query) == "table" and not query._isExp then
      sql_chunks[#sql_chunks + 1] = db:AND(unpack(query))
    else
      sql_chunks[#sql_chunks + 1] = tostring(query)
    end
  end

  local sql = table.concat(sql_chunks, " ")

  db:echo_sql(sql)
  local cur = conn:execute(sql)

  if cur ~= 0 then
    local row = cur:fetch({}, "a")
    local count = row[fn]
    cur:close()

    -- give back the correct data type. see http://www.sqlite.org/lang_aggfunc.html
    if (fn:upper() ~= "MIN" and fn:upper() ~= "MAX") or field.type == "number" then
      return tonumber(count)
    end
    if field.type == "string" then
      return count
    end
    -- Only datetime left
    local utc_epoch = datetime:parse(count, nil, true)
    local locale_diff = datetime:calculate_UTCdiff(utc_epoch)
    return db:Timestamp(utc_epoch + locale_diff)
  else
    return 0
  end
end



--- Deletes rows from the specified sheet. The argument for query tries to be intelligent: <br/>
--- * if it is a simple number, it deletes a specific row by _row_id <br/>
--- * if it is a table that contains a _row_id (e.g., a table returned by db:get) it deletes just that record. <br/>
--- * Otherwise, it deletes every record which matches the query pattern which is specified as with db:get. <br/>
--- * If the query is simply true, then it will truncate the entire contents of the sheet. <br/>
---
--- @usage When passed an actual result table that was obtained from db:fetch, it will delete the record for that table.
---   <pre>
---   enemies = db:fetch(mydb.enemies)
---   db:delete(mydb.enemies, enemies[1])
---   </pre>
--- @usage When passed a number, will delete the record for that _row_id. This example shows getting the row id from a table.
---   <pre>
---   enemies = db:fetch(mydb.enemies)
---   db:delete(mydb.enemies, enemies[1]._row_id)
---   </pre>
--- @usage As above, but this example just passes in the row id directly.
---   <pre>
---   db:delete(mydb.enemies, 5)
---   </pre>
--- @usage Here, we will delete anything which matches the same kind of query as db:fetch uses - namely,
---   anyone who is in the city of San Francisco.
---   <pre>
---   db:delete(mydb.enemies, db:eq(mydb.enemies.city, "San Francisco"))
---   </pre>
--- @usage And finally, we will delete the entire contents of the enemies table.
---   <pre>
---   db:delete(mydb.enemies, true)
---   </pre>
function db:delete(sheet, query)
  local db_name = sheet._db_name
  local s_name = sheet._sht_name

  assert(query, "must pass a query argument to db:delete()")

  local conn = db.__conn[db_name]
  if not conn then error(db_no_connection_message("delete from", db_name, A_SHEET), 2) end

  if type(query) == "number" then
    query = "_row_id = " .. tostring(query)
  elseif type(query) == "table" and not query._isExp then
    assert(query._row_id, "Passed a non-result table to db:delete, need a _row_id field to continue.")
    query = "_row_id = " .. tostring(query._row_id)
  end

  local sql = "DELETE FROM " .. s_name

  if query ~= true then
    sql = sql .. " WHERE " .. tostring(query)
  end

  db:echo_sql(sql)
  assert(conn:execute(sql))
  if db.__autocommit[db_name] then
    conn:commit()
  end
end



--- Merges the specified table array into the sheet, modifying any existing rows and adding any that don't exist.
---
--- This function is a convenience utility that allows you to quickly modify a sheet, changing
--- existing rows and add new ones as appropriate. It ONLY works on sheets which have a unique
--- index, and only when that unique index is only on a single field. For more complex situations
--- you'll have to do the logic yourself.
---
--- The table array may contain tables that were either returned previously by db:fetch, or new tables
--- that you've constructed with the correct fields, or any mix of both. Each table must have a value
--- for the unique key that has been set on this sheet.
---
--- @usage For example, consider this database:
---   <pre>
---   local mydb = db:create("peopledb",
---     {
---       friends = {
---         name = "",
---         race = "",
---         level = 0,
---         city = "",
---         _index = { "city" },
---         _unique = { "name" }
---       }
---     }
---   )
---   </pre>
---
---   Here you have a database with one sheet, which contains your friends, their race, level,
---   and what city they live in. Let's say you want to fetch everyone who lives in San Francisco, you could do:
---   <pre>
---   local results = db:fetch(mydb.friends, db:eq(mydb.friends.city, "San Francisco"))
---   </pre>
---
---   The tables in results are static, any changes to them are not saved back to the database.
---   But after a major radioactive cataclysm rendered everyone in San Francisco a mutant,
---   you could make changes to the tables as so:
---   <pre>
---   for _, friend in ipairs(results) do
---     friend.race = "Mutant"
---   end
---   </pre>
---
---   If you are also now aware of a new arrival in San Francisco, you could add them to that existing table array:
---   <pre>
---   results[#results+1] = {name="Bobette", race="Mutant", city="San Francisco"}
---   </pre>
---
---   And commit all of these changes back to the database at once with:
---   <pre>
---   db:merge_unique(mydb.friends, results)
---   </pre>
---
---   The db:merge_unique function will change the 'city' values for all the people who we previously fetched, but then add a new record as well.
function db:merge_unique(sheet, tables)
  assert(type(tables) == "table", "db:merge_unique: missing the required table of data to merge")

  local db_name = sheet._db_name
  local s_name = sheet._sht_name

  -- ahead of the schema lookup below, which indexes db.__schema[nil] for a table that
  -- is not a sheet and dies there before this can say so
  if not db.__conn[db_name] then error(db_no_connection_message("merge into", db_name, A_SHEET), 2) end

  local unique_options = db.__schema[db_name][s_name].options._unique
  assert(unique_options, "db:merge_unique only works on a sheet with a unique index.")
  assert(#unique_options == 1, "db:merge_unique only works on a sheet with a single unique index.")

  local unique_index = unique_options[1]
  local unique_key = ""
  if type(unique_index) == "table" then
    assert(#unique_index == 1, "db:merge_unique currently only supports sheets with a single unique index with a single column.")
    unique_key = unique_index[1]
  else
    unique_key = unique_index
  end

  db:echo_sql(":: Unique index = " .. unique_key)

  local conn = db.__conn[db_name]
  local mydb = db:get_database(db_name)
  mydb:_begin()

  for _, tbl in ipairs(tables) do
    assert(tbl[unique_key], "attempting to db:merge_unique with a table that does not have the unique key.")

    local results = db:fetch(sheet, db:eq(sheet[unique_key], tbl[unique_key]))
    if results and results[1] then
      local t = results[1]
      for k, v in pairs(tbl) do
        t[k] = v
      end

      db:update(sheet, t)
    else
      db:add(sheet, tbl)
    end
  end

  mydb:_commit()
  mydb:_end()
end



--- This function updates a row in the specified sheet, but only accepts a row which has been previously
--- obtained by db:fetch. Its primary purpose is that if you do a db:fetch, then change the value of a field
--- or tow, you can save back that table.
---
--- @usage This obtains a database reference, and queries the friends sheet for someone named Bob. As this
---   returns a table array containing only one item, it assigns that one item to the local variable named bob.
---   We then change the notes on Bob, and pass it into db:update() to save the changes back.
---   <pre>
---   local mydb = db:get_database("my database")
---   local bob = db:fetch(mydb.friends, db:eq(mydb.friends.name, "Bob"))[1]
---   bob.notes = "He's a really awesome guy."
---   db:update(mydb.friends, bob)
---   </pre>
function db:update(sheet, tbl)
  assert(tbl._row_id, "Can only update a table with a _row_id")
  assert(not table.is_empty(tbl), "An empty table was passed to db:update")

  local db_name = sheet._db_name
  local s_name = sheet._sht_name

  local conn = db.__conn[db_name]
  if not conn then error(db_no_connection_message("update", db_name, A_SHEET), 2) end

  local sql_chunks = { "UPDATE", s_name, "SET" }

  local set_chunks = {}
  local set_block = [["%s" = %s]]

  for k, v in pairs(db.__schema[db_name][s_name]['columns']) do
    if tbl[k] then
      local field = sheet[k]
      set_chunks[#set_chunks + 1] = set_block:format(k, db:_coerce(field, tbl[k]))
    end
  end

  sql_chunks[#sql_chunks + 1] = table.concat(set_chunks, ",")
  sql_chunks[#sql_chunks + 1] = "WHERE _row_id = " .. tbl._row_id

  local sql = table.concat(sql_chunks, " ")
  db:echo_sql(sql)
  assert(conn:execute(sql))
  if db.__autocommit[db_name] then
    conn:commit()
  end
end



--- The db:set function allows you to set a certain field to a certain value across an entire sheet.
--- Meaning, you can change all of the last_read fields in the sheet to a certain value, or possibly only
--- the last_read fields which are in a certain city. The query argument can be any value which is appropriate
--- for db:fetch, even nil which will change the value for the specified column for EVERY row in the sheet.
---
--- For example, consider a situation in which you are tracking how many times you find a certain
--- type of egg during Easter. You start by setting up your database and adding an Eggs sheet, and
--- then adding a record for each type of egg.
---   <pre>
---   local mydb = db:create("egg database", {eggs = {color = "", last_found = db.Timestamp(false), found = 0}})
---   db:add(mydb.eggs,
---     {color = "Red"},
---     {color = "Blue"},
---     {color = "Green"},
---     {color = "Yellow"},
---     {color = "Black"}
---   )
---   </pre>
---
--- Now, you have three columns. One is a string, one a timestamp (that ends up as nil in the database),
--- and one is a number. <br/><br/>
---
--- You can then set up a trigger to capture from the mud the string, "You pick up a (.*) egg!", and you
--- end up arranging to store the value of that expression in a variable called "myegg". <br/><br/>
---
--- To increment how many we found, we will do this:
---   <pre>
---   myegg = "Red" -- We will pretend a trigger set this.
---   db:set(mydb.eggs.found, db:exp("found + 1"), db:eq(mydb.eggs.color, myegg))
---   db:set(mydb.eggs.last_found, db.Timestamp("CURRENT_TIMESTAMP"), db:eq(mydb.eggs.color, myegg))
---   </pre>
---
--- This will go out and set two fields in the Red egg sheet; the first is the found field, which will
--- increment the value of that field (using the special db:exp function). The second will update the
--- last_found field with the current time. <br/><br/>
---
--- Once this contest is over, you may wish to reset this data but keep the database around.
--- To do that, you may use a more broad use of db:set as such:
---   <pre>
---   db:set(mydb.eggs.found, 0)
---   db:set(mydb.eggs.last_found, nil)
---   </pre>
function db:set(field, value, query)
  local db_name = field.database
  local s_name = field.sheet

  local conn = db.__conn[db_name]
  if not conn then error(db_no_connection_message("set a field in", db_name, A_FIELD), 2) end

  local sql_update = [[UPDATE %s SET "%s" = %s]]
  if query then
    sql_update = sql_update .. [[ WHERE %s]]
  end

  local sql = sql_update:format(
    s_name,
    field.name,
    db:_coerce(field, value),
    tostring(query)
  )

  db:echo_sql(sql)
  assert(conn:execute(sql))
  if db.__autocommit[db_name] then
    conn:commit()
  end
end



--- This is a debugging function, which echos any SQL commands if db.debug_sql is true.
--- You should not call this function directly from Mudlet.
---
--- @usage Set following lua variable to enable SQL echos.
---   <pre>
---   db.debug_sql=true
---   </pre>
function db:echo_sql(sql)
  if db.debug_sql then
    print(sql)
  end
end



-- NOT LUADOC
-- After a table so retrieved from the database, this function coerces values to
-- their proper types. Specifically, numbers and datetimes become the proper
-- types.
function db:_coerce_sheet(sheet, tbl, columns)
  if tbl then
    columns = columns or table.keys(tbl)
    tbl._row_id = tonumber(tbl._row_id)

    for _, k in pairs(columns) do
      if k ~= "_row_id" then
        local field = sheet[k]
        if field.type == "number" then
          tbl[k] = tonumber(tbl[k]) or tbl[k]
        elseif field.type == "datetime" then
          if (tbl[k] == nil) then
            tbl[k] = db:Timestamp(nil)
          else
            -- the value, tbl[k], is a UTC timestamp
            local utc_epoch = datetime:parse(tbl[k], nil, true)
            local locale_diff = datetime:calculate_UTCdiff(utc_epoch)
            tbl[k] = db:Timestamp(utc_epoch + locale_diff)
          end
        end
      end
    end
    return tbl
  end
end



-- NOT LUADOC
-- The function converts a Lua value into its SQL representation, depending on the
-- type of the specified field. Strings will be single-quoted (and single-quotes
-- within will be properly escaped), numbers will be rendered properly, and such.
function db:_coerce(field, value)
  if type(value) == "table" and value._isExp then
    return value._expression
  elseif type(value) == "table" and value._isNull then
    return "NULL"
  elseif field.type == "number" then
    return tonumber(value) or ("'" .. value .. "'")
  elseif field.type == "datetime" then
    if value._timestamp == false then
      return "NULL"
    elseif value._timestamp == "CURRENT_TIMESTAMP" then
      return "datetime('now')"
    else
      return "datetime('" .. value._timestamp .. "', 'unixepoch')" or "'" .. value .. "'"
    end
  else
    return "'" .. tostring(value):gsub("'", "''") .. "'"
  end
end



--- Returns a database expression to test if the field in the sheet is equal to the value.
---
--- @see db:fetch
function db:eq(field, value, case_insensitive)
  local fieldname = field.name
  -- escape column names as per https://www.sqlite.org/lang_expr.html
  fieldname = '"' .. fieldname:gsub("'", "''") .. '"'
  if case_insensitive then
    local v = db:_coerce(field, value):lower()
    return "lower(" .. fieldname .. ") == " .. v
  else
    local v = db:_coerce(field, value)
    return fieldname .. " == " .. v
  end
end



--- Returns a database expression to test if the field in the sheet is NOT equal to the value.
---
--- @see db:fetch
function db:not_eq(field, value, case_insensitive)
  if case_insensitive then
    local v = db:_coerce(field, value):lower()
    return "lower(" .. field.name .. ") != " .. v
  else
    local v = db:_coerce(field, value)
    return field.name .. " != " .. v
  end
end



--- Returns a database expression to test if the field in the sheet is less than the value.
---
--- @see db:fetch
function db:lt(field, value)
  local v = db:_coerce(field, value)
  return field.name .. " < " .. v
end



--- Returns a database expression to test if the field in the sheet is less than or equal to the value.
---
--- @see db:fetch
function db:lte(field, value)
  local v = db:_coerce(field, value)
  return field.name .. " <= " .. v
end



--- Returns a database expression to test if the field in the sheet is greater than to the value.
---
--- @see db:fetch
function db:gt(field, value)
  local v = db:_coerce(field, value)
  return field.name .. " > " .. v
end



--- Returns a database expression to test if the field in the sheet is greater than or equal to the value.
---
--- @see db:fetch
function db:gte(field, value)
  local v = db:_coerce(field, value)
  return field.name .. " >= " .. v
end



--- Returns a database expression to test if the field in the sheet is nil.
---
--- @see db:fetch
function db:is_nil(field)
  return field.name .. " IS NULL"
end



--- Returns a database expression to test if the field in the sheet is not nil.
---
--- @see db:fetch
function db:is_not_nil(field)
  return field.name .. " IS NOT NULL"
end



--- Returns a database expression to test if the field in the sheet matches the specified pattern. <br/><br/>
---
--- LIKE patterns are not case-sensitive, and allow two wild cards. The first is an underscore which matches
--- any single one character. The second is a percent symbol which matches zero or more of any character.
---   <pre>
---   LIKE with "_" is therefore the same as the "." regular expression.
---   LIKE with "%" is therefore the same as ".*" regular expression.
---   </pre>
---
--- @see db:not_like
--- @see db:fetch
function db:like(field, value)
  local v = db:_coerce(field, value)
  return field.name .. " LIKE " .. v
end



--- Returns a database expression to test if the field in the sheet does not match the specified pattern.
---
--- LIKE patterns are not case-sensitive, and allow two wild cards. The first is an underscore which matches
--- any single one character. The second is a percent symbol which matches zero or more of any character.
---   <pre>
---   LIKE with "_" is therefore the same as the "." regular expression.
---   LIKE with "%" is therefore the same as ".*" regular expression.
---   </pre>
---
--- @see db:like
--- @see db:fetch
function db:not_like(field, value)
  local v = db:_coerce(field, value)
  return field.name .. " NOT LIKE " .. v
end



--- Returns a database expression to test if the field in the sheet is a value between lower_bound and upper_bound.
--- This only really makes sense for numbers and Timestamps.
---
--- @see db:not_between
--- @see db:fetch
function db:between(field, left_bound, right_bound)
  local x = db:_coerce(field, left_bound)
  local y = db:_coerce(field, right_bound)
  return field.name .. " BETWEEN " .. x .. " AND " .. y
end



--- Returns a database expression to test if the field in the sheet is NOT a value between lower_bound and upper_bound.
--- This only really makes sense for numbers and Timestamps.
---
--- @see db:between
--- @see db:fetch
function db:not_between(field, left_bound, right_bound)
  local x = db:_coerce(field, left_bound)
  local y = db:_coerce(field, right_bound)
  return field.name .. " NOT BETWEEN " .. x .. " AND " .. y
end



--- Returns a database expression to test if the field in the sheet is one of the values in the table array. <br/><br/>
---
--- First, note the trailing underscore carefully! It is required.
---
--- @usage The following example illustrates the use of <b>in_</b>:
---   This will obtain all of your kills which happened in the Undervault, Hell or Purgatory. Every db:in_ expression
---   can be written as a db:OR, but that quite often gets very complex.
---   <pre>
---   local mydb = db:get_database("my database")
---   local areas = {"Undervault", "Hell", "Purgatory"}
---   db:fetch(mydb.kills, db:in_(mydb.kills.area, areas))
---   </pre>
---
--- @see db:fetch
function db:in_(field, tbl)
  local parts = {}
  for _, v in ipairs(tbl) do
    parts[#parts + 1] = db:_coerce(field, v)
  end

  return field.name .. " IN (" .. table.concat(parts, ",") .. ")"
end



--- Returns a database expression to test if the field in the sheet is not one of the values in the table array.
---
--- @see db:in_
--- @see db:fetch
function db:not_in(field, tbl)
  local parts = {}
  for _, v in ipairs(tbl) do
    parts[#parts + 1] = db:_coerce(field, v)
  end

  return field.name .. " NOT IN (" .. table.concat(parts, ",") .. ")"
end



-- NOT LUADOC
-- The metatable for db:exp values. It renders as the raw expression text whenever
-- concatenated or stringified, so WHERE-position use (db:fetch, db:AND, db:OR, ...)
-- is unchanged, while db:_coerce recognises the _isExp marker and passes the raw
-- expression through (letting db:exp be used as a db:set value, not just in WHERE).
db.__Expression = {
  __tostring = function(self)
    return self._expression
  end,
  __concat = function(a, b)
    return tostring(a) .. tostring(b)
  end,
}

--- Returns the string as-is to the database. <br/><br/>
---
--- Use this function with caution, but it is very useful in some circumstances. One of the most
--- common of such is incrementing an existing field in a db:set() operation, as so:
---   <pre>
---   db:set(mydb.enemies, db:exp("kills + 1"), db:eq(mydb.enemies.name, "Ixokai"))
---   </pre>
---
--- This will increment the value of the kills field for the row identified by the name Ixokai. <br/><br/>
---
--- But there are other uses, as the underlining database layer provides many functions you can call
--- to do certain things. If you want to get a list of all your enemies who have a name longer then
--- 10 characters, you may do:
---   <pre>
---   db:fetch(mydb.enemies, db:exp("length(name) > 10"))
---   </pre>
---
--- Again, take special care with this, as you are doing SQL syntax directly and the library can't
--- help you get things right.
---
--- @see db:fetch
function db:exp(text)
  return setmetatable({ _expression = text, _isExp = true }, db.__Expression)
end



--- Returns a compound database expression that combines all of the simple expressions passed into it.
--- These expressions should be generated with other db: functions such as db:eq, db:like, db:lt and the like. <br/><br/>
---
--- This compound expression will only find items in the sheet if all sub-expressions match.
---
--- @see db:fetch
function db:AND(...)
  local parts = {}

  for _, expression in ipairs({ ... }) do
    parts[#parts + 1] = "(" .. expression .. ")"
  end

  return "(" .. table.concat(parts, " AND ") .. ")"
end



--- Returns a compound database expression that combines both of the simple expressions passed into it.
--- These expressions should be generated with other db: functions such as db:eq, db:like, db:lt and the like. <br/><br/>
---
--- This compound expression will find any item that matches either the first or the second sub-expression.
---
--- @see db:fetch
function db:OR(left, right)
  -- coerce to strings so db:exp sentinels work here as well as plain expressions
  left = tostring(left)
  right = tostring(right)

  if not string.starts(left, "(") then
    left = "(" .. left .. ")"
  end

  if not string.starts(right, "(") then
    right = "(" .. right .. ")"
  end

  return left .. " OR " .. right
end


--- Closes all databases.
--- @return boolean result Returns true if all databases closed successfully and false otherwise 
--- @return string message
function db:_closeAll()
  if db.__env == nil then
    return false, "database environment is nil, did you forget to call db:create?"
  end

  local result, msgs = true, {}
  for db_name, conn in pairs(db.__conn) do
    if not conn:close() then
      result = false
      table.insert(msgs, "database object for "..db_name.." is already closed.")
    end
  end

  db.__conn = {}
  db.__env:close()
  db.__env = nil

  return result, table.concat(msgs, "\n")
end


--- Closes the named database or all databases if no name is provided.
--- @param db_name string|nil The name of the database to close.
--- @return boolean result Returns true in case of success and false otherwise.
--- @return string message Why the database failed to close.
function db:close(db_name)
  if db.__env == nil then
    return false, "database environment is nil, did you forget to call db:create?"
  end

  if db_name == nil then
    return db:_closeAll()
  end

  assert(
    type(db_name) == "string",
    "expected db_name to be string or nil but recieved "..type(db_name).."."
  )

  db_name = db:safe_name(db_name)
  if not db:_isActiveDBName(db_name) then
    return false, "can not close "..db_name.." because it does not exist.  Did you forget to call db:create?"
  end

  if db.__conn[db_name]:close() then
    db.__conn[db_name] = nil

    return true, ""
  else

    return false, "database object is already closed."
  end


end



-- Timestamp support
db.__Timestamp = {}



db.__TimestampMT = {
  __index = db.__Timestamp
}


function db.__Timestamp:as_string(format)
  if not format then
    format = "%m-%d-%Y %H:%M:%S"
  end

  -- given how we have an as_string function, having to wrap it in tostring() is a bit silly. So in this case, return nil as a string
  if type(self._timestamp) ~= "number" then
    return "nil", "db.Timestamp:as_string: timestamp seems to be invalid and isn't a number"
  end

  return os.date(format, self._timestamp)
end

function db.__Timestamp:as_table()
  if type(self._timestamp) ~= "number" then
    return nil, "db.Timestamp:as_table: timestamp seems to be invalid and isn't a number"
  end

  return os.date("*t", self._timestamp)
end

function db.__Timestamp:as_number()
  if type(self._timestamp) ~= "number" then
    return nil, "db.Timestamp:as_number: timestamp seems to be invalid and isn't a number"
  end

  return self._timestamp
end

function db.__Timestamp:set(timestamp)
  assert(type(timestamp) == "number", "db.Timestamp:set: timestamp needs to be a number")

  self._timestamp = timestamp
end


--- <b><u>TODO</u></b>
function db:Timestamp(ts, fmt)
  local dt = {}

  if ts == nil then
      dt._timestamp = false
  elseif ts == "CURRENT_TIMESTAMP" then
      dt._timestamp = "CURRENT_TIMESTAMP"
  else
    local t = type(ts)
    if t == "table" then
      dt._timestamp = os.time(ts)
    elseif t == "number" then
      dt._timestamp = ts
    elseif t == "string" then
      dt._timestamp = datetime:parse(ts, fmt, true)
    else
      error("Invalid value passed to db.Timestamp()")
    end
  end
  return setmetatable(dt, db.__TimestampMT)
end

function db:Null()
  return {_isNull = true}
end


db.Field = {}
db.__FieldMT = {
  __index = db.Field
}



db.Sheet = {}
db.__SheetMT = {
  __index = function(t, k)
    local v = rawget(db.Sheet, k)
    if v then
      return v
    end

    local db_name = rawget(t, "_db_name")
    local sht_name = rawget(t, "_sht_name")
    local f_name = k

    local errormsg = "Attempt to access field '%s' which does not exist (in sheet '%s' within database '%s')"

    local field = db.__schema[db_name][sht_name]['columns'][f_name]
    local field_type = ""
    local rt
    if assert(field ~= nil, errormsg:format(k, sht_name, db_name)) then
      field_type = type(field)
      if field_type == "table" and field._timestamp ~= nil then
        field_type = "datetime"
      end

      rt = setmetatable({ database = db_name, sheet = sht_name, type = field_type, name = f_name }, db.__FieldMT)
      rawset(t, k, rt)
      return rt
    end

  end
}



db.Database = {}
db.__DatabaseMT = {
  __index = function(t, k)
    local v = rawget(t, k)
    if v then
      return v
    end

    v = rawget(db.Database, k)
    if v then
      return v
    end

    local db_name = rawget(t, "_db_name")
    local rt
    if assert(db.__schema[db_name][k], "Attempt to access sheet '" .. k .. "'in db '" .. db_name .. "' that does not exist.") then
      rt = setmetatable({ _db_name = db_name, _sht_name = k }, db.__SheetMT)
      rawset(t, k, rt)
      return rt
    end
  end
}



--- Holds back every write on this database until the next commit.
--- @return boolean result Returns true in case of success and false otherwise.
--- @return string message Why the transaction was not opened, empty when it was.
function db.Database:_begin()
  -- without this the flag would be set on a database with nothing behind it, and the
  -- reopen would quietly reset it, turning the caller's transaction into autocommit
  if not db.__conn[self._db_name] then
    return false, db_no_connection_message("begin a transaction on", self._db_name, A_DATABASE)
  end

  db.__autocommit[self._db_name] = false
  return true, ""
end



--- Commits the work done on this database since the last commit.
--- @return boolean result Returns true in case of success and false otherwise.
--- @return string message Why the work was not committed, empty when it was.
function db.Database:_commit()
  local conn = db.__conn[self._db_name]
  if not conn then
    return false, db_no_connection_message("commit", self._db_name, A_DATABASE)
  end

  -- db:create turns the driver's own autocommit off, so nothing lands until a
  -- commit goes through and a refused one loses the work without saying so
  local committed, err = conn:commit()
  if not committed then
    return false, "can not commit "..self._db_name..": "..tostring(err)
  end

  return true, ""
end



--- Discards the work done on this database since the last commit.
--- @return boolean result Returns true in case of success and false otherwise.
--- @return string message Why the work was not rolled back, empty when it was.
function db.Database:_rollback()
  local conn = db.__conn[self._db_name]
  if not conn then
    return false, db_no_connection_message("roll back", self._db_name, A_DATABASE)
  end

  local rolled_back, err = conn:rollback()
  if not rolled_back then
    return false, "can not roll back "..self._db_name..": "..tostring(err)
  end

  return true, ""
end



--- Lets writes on this database commit on their own again.
--- @return boolean result Returns true in case of success and false otherwise.
--- @return string message Why the transaction was not ended, empty when it was.
function db.Database:_end()
  if not db.__conn[self._db_name] then
    return false, db_no_connection_message("end a transaction on", self._db_name, A_DATABASE)
  end

  db.__autocommit[self._db_name] = true
  return true, ""
end



function db.Database:_drop(s_name)
  local conn = db.__conn[self._db_name]
  if not conn then error(db_no_connection_message("drop a sheet from", self._db_name, A_DATABASE), 2) end

  local schema = db.__schema[self._db_name][s_name]

  -- db:create normalises _index and _unique to a list before they reach
  -- db.__schema, which is written nowhere else: the string form only arrives here
  -- from a schema built by hand
  local index_groups = { schema.options._index, schema.options._unique }
  for _, group in pairs(index_groups) do
    if type(group) == "string" then
      group = { group }
    end
    for _, value in pairs(group) do
      conn:execute("DROP INDEX IF EXISTS " .. db:_index_name(s_name, value))
    end
  end

  conn:execute("DROP TABLE IF EXISTS " .. s_name)
  conn:commit()
end



--- Returns a reference of an already existing database. This instance can be used to get references
--- to the sheets (and from there, fields) that are defined within the database. You use these
--- references to construct queries. <br/><br/>
---
--- These references do not contain any actual data, they only point to parts of the database structure.
---
--- @usage If a database has a sheet named enemies, you can obtain a reference to that sheet by simply doing:
---   <pre>
---   local mydb = db:get_database("my database")
---   local enemies_ref = mydb.enemies
---   local name_ref = mydb.enemies.name
---   </pre>
function db:get_database(db_name)
  db_name = db:safe_name(db_name)
  assert(db.__schema[db_name], "Attempt to access database that does not exist.")

  local db_inst = { _db_name = db_name }
  return setmetatable(db_inst, db.__DatabaseMT)
end

--- Queries for database content matching the given example. Different fields of
--- the example are AND connected.
--- </br>
--- Field values should be strings and can contain the following values:
--- <ul>
---   <li>literal strings to search for
---   <li>comparison terms prepended with &lt;, &gt;, &gt;=, &lt;=, !=, &lt;&gt;
---       for number and date comparisons
---   <li>ranges with :: between lower and upper bound
---   <li>different single values combined by || as OR
---   <li>strings containing % for a single and _ for multiple wildcard
---       characters
--- </ul>
--- <br/>
--- @param database Reference to the database that should be queried.
--- @param example  Query prototype that should be searched for.
--- @usage This example shows, how to use this function:
---   <pre>
---      mydb = db:create("mydb",
---        {
---          sheet = {
---            name = "", id = 0, city = "",
---            _index = { "name" },
---            _unique = { "id" },
---            _violations = "FAIL"
---          }
---        })
---      test_data = {
---        {name="Ixokai", city="Magnagora", id=1},
---        {name="Vadi", city="New Celest", id=2},
---        {name="Heiko", city="Hallifax", id=3},
---        {name="Keneanung", city="Hashan", id=4},
---        {name="Carmain", city="Mhaldor", id=5},
---        {name="Ixokai", city="Hallifax", id=6},
---      }
---      db:add(mydb.sheet, unpack(test_data))
---      res = db:query_by_example(mydb.sheet, { name = "Ixokai"})
---      display(res)
---      --[[
---      Prints
---      {
---        {
---          id = 1,
---          name = "Ixokai",
---          city = "Magnagora"
---        },
---        {
---          id = 6,
---          name = "Ixokai",
---          city = "Hallifax"
---        }
---      }
---      --]]
---   </pre>
---
--- @usage This example shows, how to combine two fields:
---   <pre>
---      mydb = db:create("mydb",
---        {
---          sheet = {
---            name = "", id = 0, city = "",
---            _index = { "name" },
---            _unique = { "id" },
---            _violations = "FAIL"
---          }
---        })
---      test_data = {
---        {name="Ixokai", city="Magnagora", id=1},
---        {name="Vadi", city="New Celest", id=2},
---        {name="Heiko", city="Hallifax", id=3},
---        {name="Keneanung", city="Hashan", id=4},
---        {name="Carmain", city="Mhaldor", id=5},
---      }
---      db:add(mydb.sheet, unpack(test_data))
---      res = db:query_by_example(mydb.sheet, { name = "Ixokai", id = "1"})
---      display(res)
---      --[[
---      Prints
---      {
---        id = 1,
---        name = "Ixokai",
---        city = "Magnagora"
---      }
---      --]]
---   </pre>
function db:query_by_example(database, example)

  if table.is_empty(example) then
    return nil
  end

  local topLevel = {}
  local find = string.find
  local match = string.match

  for key, value in pairs(example) do

    value = string.trim(value)

    local op, exp = match(value, "^%s*([<>=!]*)%s*(.*)$")

    if op == "<" then
      topLevel[#topLevel + 1] = db:lt(database[key], exp)
    elseif op == ">" then
      topLevel[#topLevel + 1] = db:gt(database[key], exp)
    elseif op == ">=" then
      topLevel[#topLevel + 1] = db:gte(database[key], exp)
    elseif op == "<=" then
      topLevel[#topLevel + 1] = db:lte(database[key], exp)
    elseif op == "!=" or op == "<>" then
      if find(exp, "__NULL__", 1, true) then
        topLevel[#topLevel + 1] = db:is_not_nil(database[key])
      else
        topLevel[#topLevel + 1] = db:not_eq(database[key], exp)
      end
    else
      if find(value, "%s*||%s*") then
        topLevel[#topLevel + 1] = db:in_(database[key], string.split(value,
        "%s*||%s*"))
      elseif find(value, "__NULL__", 1, true) then
        topLevel[#topLevel + 1] = db:is_nil(database[key])
      elseif find(value, "_", 1, true) or find(value, "%", 1, true) then
        topLevel[#topLevel + 1] = db:like(database[key], value)
      elseif find(value, "::", 1, true) then
        topLevel[#topLevel + 1] = db:between(database[key], match(value, "^(.-)::(.+)$"))
      else
        topLevel[#topLevel + 1] = db:eq(database[key], value)
      end
    end

  end

  return db:AND(unpack(topLevel))
end
