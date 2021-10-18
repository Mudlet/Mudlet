-- for appveyor
mingw_base_dir = os.getenv("MINGW_BASE_DIR")
if mingw_base_dir then
  package.path = package.path .. ";"..mingw_base_dir.."/share/lua/5.1/?.lua"
end

local argparse = require "argparse"
local lunajson = require "lunajson"

-- don't load all of LuaGlobal, as that requires yajl installed
local github_workspace = os.getenv("GITHUB_WORKSPACE")
if github_workspace then
  -- the script struggles to load the load files relatively in CI
  loadfile(github_workspace.. "/src/mudlet-lua/lua/StringUtils.lua")()
  loadfile(github_workspace.."/src/mudlet-lua/lua/TableUtils.lua")()
else
  loadfile("../src/mudlet-lua/lua/StringUtils.lua")()
  loadfile("../src/mudlet-lua/lua/TableUtils.lua")()
end

local parser = argparse("generate-ptb-changelog.lua", "Generate a changelog from the HEAD until the most recent published commit.")
parser:option("-r --releasefile", "Downloaded DBLSQD release feed file")
local args = parser:parse()

local MAX_COMMITS_PER_CHANGELOG = 100

-- Basic algorithm is as follows:
--   retrieve last X commit hashes from current branch
--   retrieve list of releases and their hashes
--   go through the list collect hashes not present in releases
--   then get the changelog for the range of hashes
--   then sort the changelog into categories
--   html-ize and print the results.

-- credit: https://stackoverflow.com/a/326715/72944
function os.capture(cmd, raw)
  local f = assert(io.popen(cmd, 'r'))
  local s = assert(f:read('*a'))
  f:close()
  if raw then return s end
  s = string.gsub(s, '^%s+', '')
  s = string.gsub(s, '%s+$', '')
  s = string.gsub(s, '[\n\r]+', ' ')
  return s
end

function read_file(path)
    local file = io.open(path, "rb")
    if not file then return nil end
    local content = file:read "*a"
    file:close()
    return content
end

function extract_released_sha1s(input)
  local decoded = lunajson.decode(input)

  local t = {}
  for _, release in ipairs(decoded.releases) do
    t[#t+1] = release.version:match(".+%-(.-)$")
  end

  return t
end

function extract_historical_sha1s()
  local history, command
  if github_workspace then
    command = string.format("git log --pretty=%%H -n %d %s", MAX_COMMITS_PER_CHANGELOG, os.getenv("GITHUB_SHA"))
    history = string.split(os.capture(command))
  else
    command = "git log --pretty=%H -n "..MAX_COMMITS_PER_CHANGELOG
    history = string.split(os.capture(command))
  end

  local t = {}
  for _, sha1 in ipairs(history) do
    t[#t+1] = sha1:match("^(........)")
  end

  return t
end

function get_releases(location)
  return read_file(location)
end

-- returns a list of commits that have been added since the last release
function scan_commits(historical_commits, released_commits)
  local commits_added_since = {}

  local released_commits_length = #released_commits[1]
  for i, commit in ipairs(historical_commits) do historical_commits[i] = string.cut(commit, released_commits_length) end

  for i, v in ipairs(historical_commits) do
    if table.contains(released_commits, v) then
      commits_added_since[#commits_added_since + 1] = v
      return commits_added_since
    end
    commits_added_since[#commits_added_since + 1] = v
  end

  print("(hit the "..MAX_COMMITS_PER_CHANGELOG.." commit limit - couldn't find the latest published PTB release)")
  return {}
end

function get_changelog(commit1, commit2)
  return os.capture(string.format("git log --pretty=%%s %s..%s", commit1, commit2), true):trim()
end

function escape_for_html(text)
  local escapes = {
    ["&"] = "&amp;",
    ["<"] = "&lt;",
    [">"] = "&gt;",
    ['"'] = "&quot;",
    ["'"] = "&#39;"
  }

  return text:gsub(".", escapes)
end

function convert_to_html(text)
  local t = {}
  text = text.."\n"
  for s in string.gmatch(text, "(.-)\n") do
    s = escape_for_html(s)
    s = s:gsub("%(#(.-)%)", [[<a href="https://github.com/Mudlet/Mudlet/pull/%1">(#%1)</a>]])
		t[#t+1] = string.format("<p>%s</p>", s)
  end

  return table.concat(t, "\n")
end

function create_category_pattern(category)
  if not category then return end
  local nocase = category:genNocasePattern()
  local res = f[[^{nocase}%w*:*%s*(.*)]]
  return res
end

function print_sorted_changelog(changelog)
  local chgtbl = changelog:split("\n")
  local add, improve, fix, infra, other = {}, {}, {}, {}, {}
  local prefix = "- "
  for _,line in ipairs(chgtbl) do
    local trimmedLine
    local addPattern = create_category_pattern("add")
    local improvePattern = create_category_pattern("improve")
    local fixPattern = create_category_pattern("fix")
    local infraPattern = create_category_pattern("infra")
    local autoPattern = "^%(autocommit%) (.*)"
    if line:match(addPattern) then
      trimmedLine = prefix .. line:match(addPattern)
      add[#add+1] = trimmedLine
    elseif line:match(improvePattern) then
      trimmedLine = prefix .. line:match(improvePattern)
      improve[#improve+1] = trimmedLine
    elseif line:match(fixPattern) then
      trimmedLine = prefix .. line:match(fixPattern)
      fix[#fix+1] = trimmedLine
    elseif line:match(infraPattern) then
      trimmedLine = prefix .. line:match(infraPattern)
      infra[#infra+1] = trimmedLine
    elseif line:match(autoPattern) then --"(autocommit)" to catch bot PRs which may not start with "infra"
      trimmedLine = prefix .. line:match(autoPattern)
      infra[#infra+1] = trimmedLine
    else
      other[#other+1] = prefix .. line
    end
  end
  local addLines = lines_to_html(table.concat(add, "\n"))
  addLines = addLines and f"<h3>Added:</h3>\n{addLines}\n" or ""
  local improveLines = lines_to_html(table.concat(improve, "\n"))
  improveLines = improveLines and f"<h3>Improved:</h3>\n{improveLines}\n" or ""
  local fixLines = lines_to_html(table.concat(fix, "\n"))
  fixLines = fixLines and f"<h3>Fixed:</h3>\n{fixLines}\n" or ""
  local infraLines = lines_to_html(table.concat(infra, "\n"))
  infraLines = infraLines and f"<h3>Infrastructure:</h3>\n{infraLines}\n" or ""
  local otherLines = lines_to_html(table.concat(other, "\n"))
  otherLines = otherLines and f"<h3>Other:</h3>\n{otherLines}\n" or ""
  local final_changelog = f"{addLines}{improveLines}{fixLines}{infraLines}{otherLines}"
  print(final_changelog)
end

function lines_to_html(lines)
  if lines == "" then
    return nil
  end
  return convert_to_html(lines)
end

local historical_commits = extract_historical_sha1s()
local released_commits = extract_released_sha1s(get_releases(args.releasefile))
local unpublished_commits = scan_commits(historical_commits, released_commits)

if table.is_empty(unpublished_commits) then print("(changelog couldn't be generated)") os.exit() end

local changelog = get_changelog(unpublished_commits[#unpublished_commits], unpublished_commits[1])

print_sorted_changelog(changelog)