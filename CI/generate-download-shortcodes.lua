local http_request = require "http.request"

local function trim(s)
  return (s:gsub("^%s*(.-)%s*$", "%1"))
end

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

function find_latest_release()
  local tagname = trim(os.capture(string.format("git describe --tags $(git rev-list --tags --max-count=1)"), true))
  assert(tagname, "error: couldn't find any Git tags in the repository at all")
  return tagname:match("Mudlet%-(.+)")
end

function get_stats_for(url)
  local tempfile = os.tmpname()
  file = io.open(tempfile, "w")
  assert(file, "Couldn't create a temporary file for writing")

  local headers, stream = assert(http_request.new_from_uri(url):go())
  local body = assert(stream:save_body_to_file(file))
  if headers:get ":status" ~= "200" then
      error(body)
  end

  local sizebytes = file:seek("end")
  local sizemb = string.format("%.f", sizebytes * 10^-6)

  local shasum = os.capture("sha256sum "..tempfile):match("^(.-) ")

  file:close()
  os.remove(tempfile)

  return sizemb, shasum
end


local downloadpattern = {
  linux = "https://www.mudlet.org/wp-content/files/Mudlet-$version-linux-x64.AppImage.tar",
  win = "https://www.mudlet.org/wp-content/files/Mudlet-$version-windows-installer.exe",
  macos = "https://www.mudlet.org/wp-content/files/Mudlet-$version.dmg",
  source = "https://www.mudlet.org/wp-content/files/Mudlet-$version.tar.xz"
}

local data = {
  linux = { url = "", sizemb = "", shasum = "" },
  -- same for the rest
  win = {},
  macos = {},
  source = {}
}

local latest_version = find_latest_release()

for os, pattern in pairs(downloadpattern) do
  local url = pattern:gsub("$version", latest_version)
  local sizemb, shasum = get_stats_for(url)
  print(os, sizemb.."MB", shasum)
  data[os] = { url = url, sizemb = sizemb, shasum = shasum }
end

-- repetitive but safer to generate exactly the code wanted
print("Shortcode for the download button:\n\n")
print(string.format([=[[sc name="dlbutton2020" version="%s" win_url="%s" win_size="%s MB" win_sha256="%s" mac_url="%s" mac_size="%s MB" mac_sha256="%s" nix_url="%s" nix_size="%s MB" nix_sha256="%s" src_url="%s" src_size="%s MB" src_sha256="%s" btn_text="Download Mudlet" note_base="Download" note_win="for Windows" note_nix="for Linux" note_mac="for Mac OS" note_src="Source Code"][/sc]]=], latest_version,
    data.win.url, data.win.sizemb, data.win.shasum,
    data.macos.url, data.macos.sizemb, data.macos.shasum,
    data.linux.url, data.linux.sizemb, data.linux.shasum,
    data.source.url, data.source.sizemb, data.source.shasum
))