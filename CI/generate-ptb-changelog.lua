local argparse = require "argparse"
local http_request = require "http.request"
local lunajson = require "lunajson"
local pretty = require"pl.pretty"

-- don't load all of LuaGlobal, as that requires yajl installed
loadfile("../src/mudlet-lua/lua/StringUtils.lua")()
loadfile("../src/mudlet-lua/lua/TableUtils.lua")()

local parser = argparse("generate-ptb-changelog.lua", "Generate a changelog from the HEAD until the most recent published commit.")
parser:option("-o --os", "OS: win/linux/mac")
parser:option("-a --architecture", "archite")
local args = parser:parse()

local MAX_COMMITS_PER_CHANGELOG = 100

-- Basic algorithm is as follows:
--   retrieve last X commit hashes from current branch
--   retrieve list of releases and their hashes
--   go through the list collect hashes not present in releases
--   then get the changelog for the range of hashes
--   html-ize and return the results.

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

function extract_released_sha1s(input)
  local decoded = lunajson.decode(input)

  local t = {}
  for _, release in ipairs(decoded.releases) do
    t[#t+1] = release.version:match(".+%-(.-)$")
  end

  return t
end

function extract_historical_sha1s()
  local history = string.split(os.capture("git log --pretty=%H -n "..MAX_COMMITS_PER_CHANGELOG))

  local t = {}
  for _, sha1 in ipairs(history) do
    t[#t+1] = sha1:match("^(........)")
  end

  return t
end

function get_releases(os, architecture)
  assert(os, "get_releases: os must be provided")
  assert(architecture, "get_releases: architecture must be provided")

  local headers, stream = assert(http_request.new_from_uri(string.format("https://feeds.dblsqd.com/MKMMR7HNSP65PquQQbiDIw/public-test-build/%s/%s", os, architecture)):go())
  local body = assert(stream:get_body_as_string())
  local status = headers:get ":status"
  if status ~= "200" then
    error(string.format("%s/%s release feed failed to retrieve: %s %s", os, architecture, status, tostring(body)))
  end
  return body
end

-- returns a list of commits that have been added since the last release
function scan_commits(historical_commits, released_commits)
  local commits_added_since = {}

  for i, v in ipairs(historical_commits) do
    if table.contains(released_commits, v) then
      return commits_added_since
    end
    commits_added_since[#commits_added_since + 1] = v
  end

  print("Hit the "..MAX_COMMITS_PER_CHANGELOG.." commit limit - no releases found in history that have been published. Is this the right branch?")
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

local historical_commits = extract_historical_sha1s()
local released_commits = extract_released_sha1s(get_releases(args.os, args.architecture))
local unpublished_commits = scan_commits(historical_commits, released_commits)

if table.is_empty(unpublished_commits) then os.exit(1) end

local changelog = get_changelog(unpublished_commits[#unpublished_commits], unpublished_commits[1])

print(convert_to_html(changelog))
