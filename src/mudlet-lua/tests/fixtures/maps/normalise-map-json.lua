-- What build-cross-platform-map.lua and Mapper_spec.lua put a saveJsonMap() export through before writing or comparing
-- it. QFile::Text writes CRLF on Windows; QFont::toString() gains fields across Qt versions, and CI runs desktop on
-- 6.11.3 and the browser on 6.9.0; a label's PNG comes out a few pixel bytes apart in the browser build, so only
-- whether it has one is kept
return function(json)
  return (json:gsub("\r\n", "\n")
              :gsub('("mapSymbolFontDetails": "' .. ('[^,"]*,'):rep(15) .. '[^,"]*)[^"]*"', '%1"')
              :gsub('("image": %[)([^%]]*)', function(open, lines)
                return open .. (lines:find('"') and '"<png>"' or '')
              end))
end
