-----------------------------------------------------------------------------
-- General-purpose useful tools that were needed during development:
-----------------------------------------------------------------------------
if package.loaded["rex_pcre"] then rex = require"rex_pcre" end



-- Tests if a table is empty: this is useful in situations where you find
-- yourself wanting to do 'if my_table == {}' and such.
function table.is_empty(tbl)
   for k, v in pairs(tbl) do
      return false
   end
   return true
end

function string.starts(String,Start)
   return string.sub(String,1,string.len(Start))==Start
end

function string.ends(String,End)
   return End=='' or string.sub(String,-string.len(End))==End
end

-----------------------------------------------------------------------------
-- Some Date / Time parsing functions.
-----------------------------------------------------------------------------

datetime = {
   _directives = {
      ["%b"] = "(?P<abbrev_month_name>jan|feb|mar|apr|may|jun|jul|aug|sep|oct|nov|dec)",
      ["%B"] = "(?P<month_name>january|febuary|march|april|may|june|july|august|september|october|november|december)",
      ["%d"] = "(?P<day_of_month>\\d{2})",
      ["%H"] = "(?P<hour_24>\\d{2})",
      ["%I"] = "(?P<hour_12>\\d{2})",
      ["%m"] = "(?P<month>\\d{2})",
      ["%M"] = "(?P<minute>\\d{2})",
      ["%p"] = "(?P<ampm>am|pm)",
      ["%S"] = "(?P<second>\\d{2})",
      ["%y"] = "(?P<year_half>\\d{2})",
      ["%Y"] = "(?P<year_full>\\d{4})"
   },
   _pattern_cache = {},
   _month_names = {
      ["january"] = 1,
      ["febuary"] = 2,
      ["march"] = 3,
      ["april"] = 4,
      ["may"] = 5,
      ["june"] = 6,
      ["july"] = 7,
      ["august"] = 8,
      ["september"] = 9,
      ["october"] = 10,
      ["november"] = 11,
      ["december"] = 12
   },
   _abbrev_month_names = {
      ["jan"] = 1,
      ["feb"] = 2,
      ["mar"] = 3,
      ["apr"] = 4,
      ["may"] = 5,
      ["jun"] = 6,
      ["jul"]= 7,
      ["aug"]= 8,
      ["sep"] = 9,
      ["oct"] = 10,
      ["nov"] = 11,
      ["dec"] = 12
   }
}

-- datetime:_get_pattern(format)
--  The rex.match function does not return named patterns even if you use named capture
--  groups, but the r:tfind does -- but this only operates on compiled patterns. So,
--  we are caching the conversion of 'simple format' date patterns into a regex, and
--  then compiling them.
function datetime:_get_pattern(format)
   if not datetime._pattern_cache[format] then
      local fmt = rex.gsub(format, "(%[A-Za-z])", 
         function(m)
               return datetime._directives[m] or m
         end
         )
      
      datetime._pattern_cache[format] = rex.new(fmt, rex.flags().CASELESS)
   end

   return datetime._pattern_cache[format]
end

-- datetime:parse(source, format, as_epoch)
--   Parse the source for a date time value, according to the specified format. The
--   default format if not specified is: "^%Y-%m-%d %H:%M:%S$"
--   If as_epoch is true, the return value will be a single number. Otherwise, it will
--   be a time table. See: http://www.lua.org/pil/22.1.html
function datetime:parse(source, format, as_epoch)
   if not format then
      format = "^%Y-%m-%d %H:%M:%S$"
   end

   local fmt = datetime:_get_pattern(format)
   local m = {fmt:tfind(source)}
   
   if m then
      m = m[3]
      dt = {}
      
      if m.year_half then
         dt.year = tonumber("20"..m.year_half)
      elseif m.year_full then
         dt.year = tonumber(m.year_full)
      end

      if m.month then
         dt.month = tonumber(m.month)
      elseif m.month_name then
         dt.month = datetime._month_names[m.month_name:lower()]
      elseif m.abbrev_month_name then
         dt.month = datetime._abbrev_month_names[m.abbrev_month_name:lower()]
      end

      dt.day = m.day_of_month
      
      if m.hour_12 then
         assert(m.ampm, "You must use %p (AM|PM) with %I (12-hour time)")
         if m.ampm == "PM" then
            dt.hour = 12 + tonumber(m.hour_12)
         else
            dt.hour = tonumber(m.hour_12)
         end
      else
         dt.hour = tonumber(m.hour_24)
      end

      dt.min = tonumber(m.minute)
      dt.sec = tonumber(m.second)
      dt.isdst = false
      
      if as_epoch then
         return os.time(dt)
      else
         return dt
      end
   else
      return nil
   end
end

-----------------------------------------------------------------------------
-- The database wrapper library
-----------------------------------------------------------------------------

if package.loaded["luasql.sqlite3"] then require "luasql.sqlite3" end

db = {}
db.__autocommit = {}
db.__schema = {}
db.__conn = {}
   
db.debug_sql = false

-- db:_sql_type(v)
--   Converts the type of a lua object to the equivalent type in SQL 
function db:_sql_type(value)
   local t = type(value)

   if t == "number" then
      return "REAL"
   elseif t == "nil" then
      return "NULL"
   elseif t == "table" and value._timestamp ~= nil then
      return "INTEGER"
   else
      return "TEXT"
   end
end

-- db:_sql_convert(v)
--   Converts a data value in Lua to its SQL equivalent; notably it will also escape single-quotes to 
--   prevent inadvertant SQL injection.
function db:_sql_convert(value)
   local t = db:_sql_type(value)
   
   if value == nil then
      return "NULL"
   elseif t == "TEXT" then
      return '"'..value:gsub("'", "''")..'"'
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

-- db:_index_name(sheet_name, index_contents)
--   Given a sheet name and the details of an index, this function will return a unique index name to
--   add to the database. The purpose of this is to create unique index names as indexes are tested
--   for existance on each call of db:create and not only on creation. That way new indexes can be 
--   added after initial creation.
function db:_index_name(tbl_name, params)
   local t = type(params)
   
   if t == "string" then
      return "idx_" .. tbl_name .. "_c_" .. params
   elseif assert(t == "table", "Indexes must be either a string or a table.") then
      local parts = {"idx", tbl_name, "c"}
      for _, v in pairs(params) do
         parts[#parts+1] = v
      end
      return table.concat(parts, "_")
   end
end

-- db:_index_valid(sheet_columns, index_columns)
--   This function returns true if all of the columns referenced in index_columns also exist within
--   the sheet_columns table array. The purpose of this is to raise an error if someone tries to index
--   a column which doesn't currently exist in the schema.
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
            echo("\n--> Bad index "..v)
            return false
         end
      end
   end
   return true
end
-- db:_sql_columns(column_spec)
--   The column_spec is either a string or an indexed table. This function returns either "column" or
--   "column1", "column2" for use in the column specification of INSERT.
function db:_sql_columns(value)
   local colstr = ''
   local t = type(value)
   
   if t == "table" then
      col_chunks = {}
      for _, v in ipairs(value) do
         col_chunks[#col_chunks+1] = '"'..v:lower()..'"'
      end
      
      colstr = table.concat(col_chunks, ',') 
   elseif assert(t == "string", 
         "Must specify either a table array or string for index, not "..type(value)) then
      colstr = '"'..value:lower()..'"'
   end
   return colstr
end

-- db:_sql_fields(field_spec)
--   This serves as a very similar function to db:_sql_columns, quoting column names properly but for
--   uses outside of INSERTs.
function db:_sql_fields(values)
   local sql_fields = {}

   for k, v in pairs(values) do
      sql_fields[#sql_fields+1] = '"'..k..'"'
   end

   return   "("..table.concat(sql_fields, ",")..")"
end

-- db:_sql_values(value_spec)
--   This quotes values to be passed into an INSERT or UPDATE operation in a SQL list. Meaning, it turns
--   {x="this", y="that", z=1} into ('this', 'that', 1).
--   It is intelligent with data-types; strings are automatically quoted (with internal single quotes 
--   escaped), nil turned into NULL, timestamps converted to integers, and such.
function db:_sql_values(values)
   local sql_values = {}

   for k, v in pairs(values) do
      local t = type(v)
      local s = ""

      if t == "string" then
         s = "'"..v:gsub("'", "''").."'"
      elseif t == "nil" then
         s = "NULL"
      elseif t == "table" and t._timestamp ~= nil then
         if not t._timestamp then
            return "NULL"
         else
            s = tostring(t._timestamp)
         end
      else
         s = tostring(v)
      end
      
      sql_values[#sql_values+1] = s
   end

   return "("..table.concat(sql_values, ",")..")"
end

-- db:safe_name(name)
--   On a filesystem level, names are restricted to being alphanumeric only. So, "my_database" becomes
--   "mydatabase", and "../../../../etc/passwd" becomes "etcpasswd". This prevents any possible 
--   security issues with database names.
function db:safe_name(name)
   name = name:gsub("[^%ad]", "")
   name = name:lower()
   return name
end

-- db:create(db_name, ...)
--   Creates and/or modifies an existing database. This function is safe to define at a top-level of a
--   Mudlet script: in fact it is reccommended you run this function at a top-level without any kind
--   of guards. If the named database does not exist it will ccreate it. If the database does exist
--   then it will add any columns or indexes which didn't exist before to that database. If the
--   database already has all the specified columns and indexes, it will do nothing.
function db:create(db_name, sheets)
   if not db.__env then
      db.__env = luasql.sqlite3()
   end
   
   db_name = db:safe_name(db_name)
   
   if not db.__conn[db_name] then
      db.__conn[db_name] = db.__env:connect(getMudletHomeDir() .. "/Database_" .. db_name .. ".db")
      db.__conn[db_name]:setautocommit(false)
      db.__autocommit[db_name] = true
   end
   
   db.__schema[db_name] = {}
   
   -- We need to separate the actual column configuration from the meta-configuration of the desired
   -- sheet. {sheet={"column"}} verses {sheet={"column"}, _index={"column"}}. In the former we are
   -- creating a database with a single field; in the latter we are also adding an index on that
   -- field. The db package reserves any key that begins with an underscore to be special and syntax
   -- for its own use.
   for s_name, sht in pairs(sheets) do
      options = {}

      if sht[1] ~= nil then
         t = {}
         for k, v in pairs(sht) do
            t[v] = nil
         end
         sht = t
      else
         for k, v in pairs(sht) do
            if string.starts(k, "_") then
               options[k] = v
               sht[k] = nil
            end
         end
      end
      
      if not options._violations then
         options._violations = "FAIL"
      end
      
      db.__schema[db_name][s_name] = {columns=sht, options=options}
      db:_migrate(db_name, s_name)
   end
   return db:get_database(db_name)
end

-- db:_migrate(db_name, sheet_name)
--   The migrate function is meant to upgrade an existing database live, to maintain a consistant
--   and correct set of sheets and fields, along with their indexes. It should be safe to run 
--   at any time, and must not cause any data loss. It simply adds to what is there: in perticular
--   it is not capable of removing indexes, columns, or sheets after they have been defined.
function db:_migrate(db_name, s_name)
   local conn = db.__conn[db_name]
   local schema = db.__schema[db_name][s_name]
   
   local current_columns = {}
   
   -- The PRAGMA table_info command is a query which returns all of the columns currently 
   -- defined in the specified table. The purpose of this section is to see if any new columns
   -- have been added.
   local cur = conn:execute("PRAGMA table_info('"..s_name.."')")
   
   if cur ~= 0 then
      local row = cur:fetch({}, "a")
      
      while row do
         current_columns[row.name:lower()] = row.type
         row = cur:fetch({}, "a")
      end
      cur:close()
   end
   
   -- The SQL definition of a column is:
   --    "column_name" column_type NULL
   -- The db module does not presently support columns that are required. Everything is optional,
   -- everything may be NULL / nil.
   -- If you specify a column's type, you also specify its default value. 
   local sql_column = ', "%s" %s NULL'
   local sql_column_default = sql_column..' DEFAULT %s'
   
   if table.is_empty(current_columns) then
      -- At this point, we know that the specified table does not exist in the database and so we
      -- should create it.
      
      -- Every sheet has an implicit _row_id column. It is not presently (and likely never will be)
      -- supported to define the primary key of any sheet.
      local sql_chunks = {"CREATE TABLE ", s_name,  '("_row_id" INTEGER PRIMARY KEY AUTOINCREMENT'}
      
      -- We iterate over every defined column, and add a line which creates it.
      for key, value in pairs(schema.columns) do
         local sql = ""
         if value == nil then
            sql = sql_column:format(key:lower(), db:_sql_type(value))
         else
            sql = sql_column_default:format(key:lower(), db:_sql_type(value), db:_sql_convert(value))
         end
         sql_chunks[#sql_chunks+1] = sql
      end

      sql_chunks[#sql_chunks+1] = ")"
      
      local sql = table.concat(sql_chunks, "")
      db:echo_sql(sql)
      conn:execute(sql)

   else
      -- At this point we know that the sheet already exists, but we are concerned if the current 
      -- definition includes columns which may be added.
      
      local sql_chunks = {}
      local sql_add = 'ALTER TABLE %s ADD COLUMN "%s" %s NULL DEFAULT %s'
      
      for k, v in pairs(schema.columns) do
         k = k:lower()
         t = db:_sql_type(v)
         v = db:_sql_convert(v)
         
         -- Here we test it a given column exists in the sheet already, and if not, we add that
         -- column.
         if not current_columns[k] then
            local sql = sql_add:format(s_name, k, t, v)
            conn:execute(sql)
            db:echo_sql(sql)
         end
      end
   end
   
   -- On every invocation of db:create we run the code that creates indexes, as that code will
   -- do nothing if the specific indexes already exist. This is enforced by the db:_index_name
   -- function creating a unique index.
   --
   -- Note that in no situation will an existing index be deleted. 
   db:_migrate_indexes(conn, s_name, schema, current_columns)      
   db:echo_sql("COMMIT")
   conn:commit()
   conn:execute("VACUUM")
end

-- db:_migrate_indexes(connection, sheet_name, schema, existing_columns)
--   Creates any indexes which do not yet exist in the given database.
function db:_migrate_indexes(conn, s_name, schema, current_columns)      
   local sql_create_index = "CREATE %s IF NOT EXISTS %s ON %s (%s);"
   local opt = {_unique = "UNIQUE INDEX", _index = "INDEX"} -- , _check = "CHECK"}

   for option_type, options in pairs(schema.options) do
      if option_type == "_unique" or option_type == "_index" then
         for _, value in pairs(options) do
         
            -- If an index references a column which does not presently exist within the schema
            -- this will fail. 

            if db:_index_valid(current_columns, value) then
               --assert(db:_index_valid(current_columns, value), 
               --      "In sheet "..s_name.." an index field is specified that does not exist.")

               sql = sql_create_index:format(
                     opt[option_type], db:_index_name(s_name, value), s_name, db:_sql_columns(value)
               )
               db:echo_sql(sql)
               conn:execute(sql)
            end
         end
      end
   end
end

-- db:add(sheet, table1, ..., tableN)
--   Adds one or more new rows to the specified sheet. If any of these rows would violate a
--   UNIQUE index, a lua error will be thrown and execution will cancel. As such it is
--   advisable that if you use a UNIQUE index, you test those values before you attempt to
--   insert a new row.
--
--   Each table is a series of key-value pairs to set the values of the sheet, but if any
--   keys do nto exist then they will be set to nil or the default value.
function db:add(sheet, ...)
   local db_name = sheet._db_name
   local s_name = sheet._sht_name
   assert(s_name, "First argument to db:add must be a proper Sheet object.")

   local conn = db.__conn[db_name]
   local sql_insert = "INSERT OR %s INTO %s %s VALUES %s"
   
   for _, t in ipairs({...}) do
      if t._row_id then
         -- You are not permitted to change a _row_id
         t._row_id = nil
      end
      
      local sql = sql_insert:format(db.__schema[db_name][s_name].options._violations, s_name, db:_sql_fields(t), db:_sql_values(t))
      db:echo_sql(sql)
      assert(conn:execute(sql), "Failed to add item: this is probably a violation of a UNIQUE index or other constraint.")
   end
   if db.__autocommit[db_name] then
      conn:commit()
   end
end

-- db:fetch(sheet, query, order_by, descending)
--   Returns a table array of table dictionaries representing each row in the specified sheet.
--   If query is not specified, it returns EVERY row in the sheet. 
function db:fetch(sheet, query, order_by, descending)
   local db_name = sheet._db_name
   local s_name = sheet._sht_name

   local conn = db.__conn[db_name]
   local sql = "SELECT * FROM "..s_name
   
   if query then
      if type(query) == "table" then
         sql = sql.." WHERE "..db:AND(unpack(query))
      else
         sql = sql.." WHERE "..query
      end
   end
   
   if order_by then
      local o = {}
      for _, v in ipairs(order_by) do
         assert(v.name, "You must pass field instances (as obtained from yourdb.yoursheet.yourfield) to sort.")
         o[#o+1] = v.name
      end
      
      sql = sql.." ORDER BY "..db:_sql_columns(o)
      
      if descending then
         sql = sql.." DESC"
      end
   end
   
   db:echo_sql(sql)
   local cur = conn:execute(sql)

   if cur ~= 0 then
      local results = {}
      local row = cur:fetch({}, "a")

      while row do
         results[#results+1] = db:_coerce_sheet(sheet, row)
         row = cur:fetch({}, "a")
      end
      cur:close()
      return results
   else
      return nil
   end
end   

-- db:aggregate(field, function, query)
--   Runs an aggregate function over the specified field, in the specified sheet. Query is optional.

function db:aggregate(field, fn, query)
   local db_name = field.database
   local s_name = field.sheet
   local conn = db.__conn[db_name]
   
   assert(type(field) == "table", "Field must be a field reference.")
   assert(field.name, "Field must be a real field reference.")
   
   local sql_chunks = {"SELECT", fn, "(", field.name, ")", "AS", fn, "FROM", s_name}
   
   if query then
      if type(query) == "table" then
         sql_chunks[#sql_chunks+1] = db:AND(unpack(query))
      else
         sql_chunks[#sql_chunks+1] = query
      end
   end
   
   if order_by then
      local o = {}
      for _, v in ipairs(order_by) do
         assert(v.name, "You must pass field instances (as obtained from yourdb.yoursheet.yourfield) to sort.")
         o[#o+1] = v.name
      end
      
      sql_chunks[#sql_chunks+1] = "ORDER BY"
      sql_chunks[#sql_chunks+1] = db:_sql_columns(o)
      
      if descending then
         sql_chunks[#sql_chunks+1] = "DESC"
      end
   end
   
   local sql = table.concat(sql_chunks, " ")
   
   db:echo_sql(sql)
   local cur = conn:execute(sql)

   if cur ~= 0 then
      local row = cur:fetch({}, "a")
      local count = row[fn]
      cur:close()
      return count
   else
      return 0
   end
end   


-- db:delete(sheet, query)
--   Deletes rows from the specified sheet. The argument for query tries to be intelligent: 
--      - if it is a simple number, it deletes a specific row by _row_id
--      - if it is a table that contains a _row_id (e.g., a table returned by db:get) it 
--        deletes just that record.
--      - Otherwise, it deletes every record which matches the query pattern which is
--        specified as with db:get.
--      - If the query is simply true, then it will truncate the entire contents of the
--        sheet.
function db:delete(sheet, query)
   local db_name = sheet._db_name
   local s_name = sheet._sht_name
   
   local conn = db.__conn[db_name]
   
   assert(query, "must pass a query argument to db:delete()")
   if type(query) == "number" then
      query = "_row_id = "..tostring(query)
   elseif type(query) == "table" then
      assert(query._row_id, "Passed a non-result table to db:delete, need a _row_id field to continue.")
      query = "_row_id = "..tostring(query._row_id)
   end
   
   local sql = "DELETE FROM "..s_name
   
   if query ~= true then
      sql = sql.." WHERE "..query
   end
   
   db:echo_sql(sql)
   assert(conn:execute(sql))
   if db.__autocommit[db_name] then
      conn:commit()
   end
end


-- db:merge(sheet, tables)
function db:merge_unique(sheet, tables)
   local db_name = sheet._db_name
   local s_name = sheet._sht_name
   
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
   
   db:echo_sql(":: Unique index = "..unique_key)
   
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

-- db:update(sheet, table)
--   This function updates a row in the specified sheet, but only accepts a row which has
--   been previously obtained by db:get. Its primary purpose is that if you do a db:get,
--   then change the value of a field or tow, you can save back that table.
function db:update(sheet, tbl)
   assert(tbl._row_id, "Can only update a table with a _row_id")
   assert(not table.is_empty(tbl), "An empty table was passed to db:update")
   
   local db_name = sheet._db_name
   local s_name = sheet._sht_name
   
   local conn = db.__conn[db_name]
   
   local sql_chunks = {"UPDATE OR", db.__schema[db_name][s_name].options._violations, s_name, "SET"}
   
   local set_chunks = {}
   local set_block = [["%s" = %s]] 
   
   for k, v in pairs(db.__schema[db_name][s_name]['columns']) do
      if tbl[k] then
         local field = sheet[k]
         set_chunks[#set_chunks+1] = set_block:format(k, db:_coerce(field, tbl[k]))
      end
   end
   
   sql_chunks[#sql_chunks+1] = table.concat(set_chunks, ",")
   sql_chunks[#sql_chunks+1] = "WHERE _row_id = "..tbl._row_id
   
   local sql = table.concat(sql_chunks, " ")
   db:echo_sql(sql)
   assert(conn:execute(sql))
   if db.__autocommit[db_name] then
      conn:commit()
   end
end

-- db:set(field, value, query)
--   The db:set function allows you to set a certain field to a certain value across
--   an entire sheet. Meaning, you can change all of the last_read fields in the
--   sheet to a certain value, or possibly only the last_read fields which are in
--   a certain city. The query argument can be any value which is appropriate for
--   db:get.
function db:set(field, value, query)
   local db_name = sheet._db_name
   local s_name = sheet._sht_name

   local conn = db.__conn[db_name]

   local sql_update = [[UPDATE OR %s %s SET "%s" = %s]]
   if query then
       sql_update = sql_update .. [[ WHERE %s]]
   end
   
   local sql = sql_update:format(db.__schema[db_name][s_name].options._violations, s_name, field.name, db:_coerce(field, value), query)

   db:echo_sql(sql)
   assert(conn:execute(sql))
   if db.__autocommit[db_name] then
      conn:commit()
   end
end

-- db:echo_eql(sql)
--   This is a debugging function, which echos any SQL commands if db.debug_sql is
--   true
function db:echo_sql(sql)
   if db.debug_sql then
      echo("\n"..sql.."\n")
   end
end

-- db:_coerce_sheet(sheet, tbl)
--   After a table so retrieved from the database, this function coerces values to
--   their proper types. Specifically, numbers and datetimes become the proper 
--   types.
function db:_coerce_sheet(sheet, tbl)
   if tbl then
      tbl._row_id = tonumber(tbl._row_id)
      
      for k, v in pairs(tbl) do
         if k ~= "_row_id" then
            local field = sheet[k]
            if field.type == "number" then
               tbl[k] = tonumber(tbl[k]) or tbl[k]
            elseif field.type == "datetime" then
               tbl[k] = db:Timestamp(datetime:parse(tbl[k], nil, true))
            end
         end   
      end
      return tbl
   end
end

-- db:_coerce(field, value)
--   The function converts a Lua value into its SQL representation, depending on the
--   type of the specified field. Strings will be single-quoted (and single-quotes 
--   within will be properly escaped), numbers will be rendered properly, and such.
function db:_coerce(field, value)
   if field.type == "number" then
      return tonumber(value) or "'"..value.."'"
   elseif field.type == "datetime" then
      if value._timestamp == false then
         return "NULL"
      else
         return tonumber(value._timestamp) or "'"..value.."'"
      end
   else
      return "'"..tostring(value):gsub("'", "''").."'"
   end
end

-- SQL Expression Functions
--   The following functions form the basis of defining basic database expressions.
--   If a value is equal to a field, if a value is less then a field, and so
--   forth.

function db:eq(field, value, case_insensitive)
   if case_insensitive then
      local v = db:_coerce(field, value):lower()
      return "lower("..field.name..") == "..v      
   else
      local v = db:_coerce(field, value)
      return field.name.." == "..v
   end
end

function db:not_eq(field, value, case_insensitive)
   if case_insensitive then
      local v = db:_coerce(field, value):lower()
      return "lower("..field.name..") != "..v      
   else
      local v = db:_coerce(field, value)
      return field.name.." != "..v
   end
end

function db:lt(field, value)
   local v = db:_coerce(field, value)
   return field.name.." < "..v
end

function db:lte(field, value)
   local v = db:_coerce(field, value)
   return field.name.." <= "..v
end

function db:gt(field, value)
   local v = db:_coerce(field, value)
   return field.name.." > "..v
end

function db:gte(field, value)
   local v = db:_coerce(field, value)
   return field.name.." >= "..v
end

function db:is_nil(field)
   return field.name.." IS NULL"
end

function db:is_not_nil(field)
   return field.name.." IS NOT NULL"
end

function db:like(field, value)
   local v = db:_coerce(field, value)
   return field.name.." LIKE "..v
end

function db:not_like(field, value)
   local v = db:_coerce(field, value)
   return field.name.." NOT LIKE "..v
end

function db:between(field, left_bound, right_bound)
   local x = db:_coerce(field, left_bound)
   local y = db:_coerce(field, right_bound)
   return field.name.." BETWEEN "..x.." AND "..y
end

function db:not_between(field, left_bound, right_bound)
   local x = db:_coerce(field, left_bound)
   local y = db:_coerce(field, right_bound)
   return field.name.." NOT BETWEEN "..x.." AND "..y
end

function db:in_(field, tbl)
   local parts = {}
   for _, v in ipairs(tbl) do
      parts[#parts+1] = db:_coerce(field, v)
   end

   return field.name.." IN ("..table.concat(parts, ",")..")"
end

function db:not_in(field, tbl)
   local parts = {}
   for _, v in ipairs(tbl) do
      parts[#parts+1] = db:_coerce(field, v)
   end

   return field.name.." NOT IN ("..table.concat(parts, ",")..")"
end

function db:exp(text)
   return text
end

function db:AND(...)
   local parts = {}

   for _, expression in ipairs({...}) do
      parts[#parts+1] = "("..expression..")"
   end

   return "("..table.concat(parts, " AND ")..")"
end   

function db:OR(left, right)
   if not string.starts(left, "(") then
      left = "("..left..")"
   end

   if not string.starts(right, "(") then
      right = "("..right..")"
   end

   return left.." OR "..right
end

function db:close()
   for _, c in pairs(db.__conn) do
      c:close()
   end
   db.__env:close()
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
   
   return os.date(format, self._timestamp)
end

function db.__Timestamp:as_table()
   return os.date("*t", self._timestamp)
end

function db.__Timestamp:as_number()
   return self._timestamp
end

function db:Timestamp(ts, fmt)
   local dt = {}
   if type(ts) == "table" then
      dt._timestamp = os.time(ts)
   elseif type(ts) == "number" then
      dt._timestamp = ts
   elseif type(ts) == "string" and 
           assert(ts == "CURRENT_TIMESTAMP", "The only strings supported by db.DateTime:new is CURRENT_TIMESTAMP") then
      dt._timestamp = "CURRENT_TIMESTAMP"
   elseif ts == nil then
      dt._timestamp = false
   else
      assert(nil, "Invalid value passed to db.Timestamp()")
   end
   return setmetatable(dt, db.__TimestampMT)
end

-- function db.Timestamp:new(ts, fmt)
--    local dt = {}
--    if type(ts) == "table" then
--       dt._timestamp = os.time(ts)
--    elseif type(ts) == "number" then
--       dt._timestamp = ts
--    elseif assert(ts == "CURRENT_TIMESTAMP", "The only strings supported by db.DateTime:new is CURRENT_TIMESTAMP") then
--       dt._timestamp = "CURRENT_TIMESTAMP"
--    end
--    return setmetatable(dt, db.__TimestampMT)
-- end

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
      local f_name = k:lower()

      local errormsg = "Attempt to access field %s in sheet %s in database %s that does not exist."
      
      local field = db.__schema[db_name][sht_name]['columns'][f_name]
      if assert(field, errormsg:format(k, sht_name, db_name)) then
         type_ = type(field)
         if type_ == "table" and field._timestamp then
            type_ = "datetime"
         end
         
         rt = setmetatable({database=db_name, sheet=sht_name, type=type_, name=f_name}, db.__FieldMT)
         rawset(t,k,rt)
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

      local v = rawget(db.Database, k)
      if v then
         return v
      end

      local db_name = rawget(t, "_db_name")
      if assert(db.__schema[db_name][k:lower()], "Attempt to access sheet '"..k:lower().."'in db '"..db_name.."' that does not exist.") then
         rt = setmetatable({_db_name = db_name, _sht_name = k:lower()}, db.__SheetMT)
         rawset(t,k,rt)
         return rt
      end
   end
}

function db.Database:_begin()
   db.__autocommit[self._db_name] = false
end

function db.Database:_commit()
   local conn = db.__conn[self._db_name]
   conn:commit()
end

function db.Database:_rollback()
   local conn = db.__conn[self._db_name]
   conn:rollback()
end

function db.Database:_end()
   db.__autocommit[self._db_name] = true
end

function db.Database._drop(s_name)
   local conn = db.__conn[self._db_name]
   local schema = db.__schema[self._db_name]
   
   if schema.options._index then
      for _, value in schema.options._index do
         conn:execute("DROP INDEX IF EXISTS " .. db:_index_name(s_name, value))
      end
   end

   if schema.options._unique then
      for _, value in schema.options._unique do
         conn:execute("DROP INDEX IF EXISTS " .. db:_index_name(s_name, value))
      end
   end
   
   conn:execute("DROP TABLE IF EXISTS "..s_name)
   conn:commit()
end

function db:get_database(db_name)
   db_name = db:safe_name(db_name)
   assert(db.__schema[db_name], "Attempt to access database that does not exist.")

   db_inst = {_db_name = db_name}
   return setmetatable(db_inst, db.__DatabaseMT)
end
