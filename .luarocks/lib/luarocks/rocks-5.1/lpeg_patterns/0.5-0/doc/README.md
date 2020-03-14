A collection of [LPEG](http://www.inf.puc-rio.br/~roberto/lpeg/lpeg.html) patterns

## Use cases

  - Strict validation of user input
  - Searching free-form input


## Modules

### `core`

A small module implementing commonly used rules from [RFC-5234 appendix B.1](https://tools.ietf.org/html/rfc5234#appendix-B.1)

  - `ALPHA` (pattern)
  - `BIT` (pattern)
  - `CHAR` (pattern)
  - `CR` (pattern)
  - `CRLF` (pattern)
  - `CTL` (pattern)
  - `DIGIT` (pattern)
  - `DQUOTE` (pattern)
  - `HEXDIG` (pattern)
  - `HTAB` (pattern)
  - `LF` (pattern)
  - `LWSP` (pattern)
  - `OCTET` (pattern)
  - `SP` (pattern)
  - `VCHAR` (pattern)
  - `WSP` (pattern)


### `IPv4`

  - `IPv4address` (pattern): parses an IPv4 address in dotted decimal notation. on success, returns addresses as an IPv4 object
  - `IPv4_methods` (table):
      - `unpack` (function): the IPv4 address as a series of 4 8 bit numbers
      - `binary` (function): the IPv4 address as a 4 byte binary string
  - `IPv4_mt` (table): metatable given to IPv4 objects
      - `__index` (table): `IPv4_methods`
      - `__tostring` (function): returns the IPv4 address in dotted decimal notation

IPv4 "dotted decimal notation" in this document refers to "strict" form (see [RFC-6943 section 3.1.1](https://tools.ietf.org/html/rfc6943#section-3.1.1)) unless otherwise noted.


### `IPv6`

  - `IPv6address` (pattern): parses an IPv6 address
  - `IPv6addrz` (pattern): parses an IPv6 address with optional "ZoneID" (see [RFC-6874](https://tools.ietf.org/html/rfc6874))
  - `IPv6_methods` (table): methods available on IPv6 objects
      - `unpack` (function): the IPv6 address as a series of 8 16bit numbers, optionally followed by zoneid
      - `binary` (function): the IPv6 address as a 16 byte binary string
      - `setzoneid` (function): set the zoneid of this IPv6 address
  - `IPv6_mt` (table): metatable given to IPv6 objects
      - `__tostring` (function): will return the IPv6 address as a valid IPv6 string


### `uri`

Parses URIs as described in [RFC-3986](https://tools.ietf.org/html/rfc3986).

  - `uri` (pattern): on success, returns a table with fields: (similar to [luasocket](http://w3.impa.br/~diego/software/luasocket/url.html#parse))
      - `scheme`
      - `userinfo`
      - `host`
      - `port`
      - `path`
      - `query`
      - `fragment`
  - `absolute_uri` (pattern): similar to `uri`, but does not permit fragments
  - `uri_reference` (pattern): similar to `uri`, but permits relative URIs
  - `relative_part` (pattern): matches a relative uri not including query and fragment; data is held in named group captures `"userinfo"`, `"host"`, `"port"`, `"path"`
  - `scheme` (pattern): matches the scheme portion of a URI
  - `userinfo` (pattern): matches the userinfo portion of a URI
  - `host` (pattern): matches the host portion of a URI
  - `IP_literal` (pattern): matches an IP based host portion of a URI. Capture is an [IPv4](#IPv4), [IPv6](#IPv6) or IPvFuture object
  - `port` (pattern): matches the port portion of a URI
  - `authority` (pattern): matches the authority portion of a URI; data is held in named group captures of `"userinfo"`, `"host"`, `"port"`
  - `path` (pattern): matches the path portion of a URI. Captures `nil` for the empty path.
  - `segment` (pattern): matches a path segment (a piece of a path without a `/`)
  - `query` (pattern): matches the query portion of a URI
  - `fragment` (pattern): matches the fragment portion of a URI
  - `sane_uri` (pattern): a variant that shouldn't match things that people would not normally consider URIs.
    e.g. uris without a hostname
  - `sane_host` (pattern): a variant that shouldn't match things that people would not normally consider valid hosts.
  - `sane_authority` (pattern): a variant that shouldn't match things that people would not normally consider valid hosts.
  - `pct_encoded` (pattern): matches a percent encoded octet, produces a capture of the normalised form.
  - `sub_delims` (pattern): the set of subcomponent delimeters


### `email`

  - `mailbox` (pattern): the mailbox format: matches either `name_addr` or an addr-spec.
  - `name_addr` (pattern): the name and address format i.e. `Display Name<email@example.com>`
    Has captures of the local_part and the domain. Captures the display name in the named capture `"display"`
  - `email` (pattern): also known as an "addr-spec"; follows [RFC-5322 section 3.4.1](http://tools.ietf.org/html/rfc5322#section-3.4.1)
    Has captures of the local_part and the domain
    Be careful trying to reconstruct the email address from the captures; you may need escaping
  - `local_part` (pattern): the bit before the `@` in an email address
  - `domain` (pattern): the bit after the `@` in an email address
  - `email_nocfws` (pattern): a variant that doesn't allow for comments or folding whitespace
  - `local_part_nocfws` (pattern): the bit before the `@` in an email address; no comments or folding whitespace allowed.
  - `domain_nocfws` (pattern):  the bit after the `@` in an email address; no comments or folding whitespace allowed.


### `http`

These patterns should be considered to have non stable APIs.

#### [RFC 4918](https://tools.ietf.org/html/rfc4918)

  - `DAV` (pattern)
  - `Depth` (pattern)
  - `Destination` (pattern)
  - `If` (pattern)
  - `Lock_Token` (pattern)
  - `Overwrite` (pattern)
  - `TimeOut` (pattern)


#### [RFC 5023](https://tools.ietf.org/html/rfc5023)

  - `SLUG` (pattern)


#### [RFC 5323](https://tools.ietf.org/html/rfc5323)

  - `DASL` (pattern)


#### [RFC 5789](https://tools.ietf.org/html/rfc5789)

  - `Accept_Patch` (pattern)


#### [RFC 5988](https://tools.ietf.org/html/rfc5988)

  - `Link` (pattern)


#### [RFC 6265](https://tools.ietf.org/html/rfc6265)

  - `Set_Cookie` (pattern)
  - `Cookie` (pattern)


#### [RFC 6266](https://tools.ietf.org/html/rfc6266)

  - `Content_Disposition` (pattern)


#### [RFC 6454](https://tools.ietf.org/html/rfc6454)

  - `Origin` (pattern)


#### [RFC 6455](https://tools.ietf.org/html/rfc6455)

  - `Sec_WebSocket_Accept` (pattern)
  - `Sec_WebSocket_Key` (pattern)
  - `Sec_WebSocket_Extensions` (pattern)
  - `Sec_WebSocket_Protocol_Client` (pattern)
  - `Sec_WebSocket_Protocol_Server` (pattern)
  - `Sec_WebSocket_Version_Client` (pattern)
  - `Sec_WebSocket_Version_Server` (pattern)


#### [RFC 6638](https://tools.ietf.org/html/rfc6638)

  - `Schedule_Reply` (pattern)
  - `Schedule_Tag` (pattern)
  - `If_Schedule_Tag_Match` (pattern)


#### [RFC 6797](https://tools.ietf.org/html/rfc6797)

  - `Strict_Transport_Security` (pattern)


#### [RFC 7034](https://tools.ietf.org/html/rfc7034)

  - `X_Frame_Options` (pattern)


#### [RFC 7089](https://tools.ietf.org/html/rfc7089)

  - `Accept_Datetime` (pattern)
  - `Memento_Datetime` (pattern)


#### [RFC 7230](https://tools.ietf.org/html/rfc7230)

  - `request_line` (pattern)
  - `field_name` (pattern)
  - `field_value` (pattern)
  - `header_field` (pattern)
  - `OWS` (pattern)
  - `RWS` (pattern)
  - `BWS` (pattern)
  - `token` (pattern)
  - `qdtext` (pattern)
  - `quoted_string` (pattern)
  - `comment` (pattern)
  - `Content_Length` (pattern)
  - `Transfer_Encoding` (pattern)
  - `chunk_ext` (pattern)
  - `TE` (pattern)
  - `Trailer` (pattern)
  - `request_target` (pattern)
  - `Host` (pattern)
  - `Via` (pattern): captures are a list of tables with fields `.protocol`, `.by` and `.comment`
  - `Connection` (pattern)
  - `Upgrade` (pattern): captures are a list of strings containing *protocol* or *protocol/version*


#### [RFC 7231](https://tools.ietf.org/html/rfc7231)

  - `IMF_fixdate` (pattern)
  - `Content_Encoding` (pattern)
  - `Content_Type` (pattern)
  - `Content_Language` (pattern)
  - `Content_Location` (pattern)
  - `Expect` (pattern)
  - `Max_Forwards` (pattern)
  - `Accept` (pattern)
  - `Accept_Charset` (pattern)
  - `Accept_Encoding` (pattern)
  - `Accept_Language` (pattern)
  - `From` (pattern)
  - `Referer` (pattern)
  - `User_Agent` (pattern)
  - `Date` (pattern): capture is a table in the same format as used by [`os.time`](http://www.lua.org/manual/5.3/manual.html#pdf-os.time)
  - `Location` (pattern)
  - `Retry_After` (pattern): capture is either a table describing an absolute time in the same format as used by [`os.time`](http://www.lua.org/manual/5.3/manual.html#pdf-os.time), or a relative time as a number of seconds
  - `Vary` (pattern)
  - `Allow` (pattern)
  - `Server` (pattern)


#### [RFC 7232](https://tools.ietf.org/html/rfc7232)

  - `Last_Modified` (pattern): capture is a table in the same format as used by [`os.time`](http://www.lua.org/manual/5.3/manual.html#pdf-os.time)
  - `ETag` (pattern)
  - `If_Match` (pattern)
  - `If_None_Match` (pattern)
  - `If_Modified_Since` (pattern): capture is a table in the same format as used by [`os.time`](http://www.lua.org/manual/5.3/manual.html#pdf-os.time)
  - `If_Unmodified_Since` (pattern): capture is a table in the same format as used by [`os.time`](http://www.lua.org/manual/5.3/manual.html#pdf-os.time)


#### [RFC 7233](https://tools.ietf.org/html/rfc7233)

  - `Accept_Ranges` (pattern)
  - `Range` (pattern)
  - `If_Range` (pattern): capture is either an `entity_tag` or a table in the same format as used by [`os.time`](http://www.lua.org/manual/5.3/manual.html#pdf-os.time)
  - `Content_Range` (pattern)


#### [RFC 7234](https://tools.ietf.org/html/rfc7234)

  - `Age` (pattern)
  - `Cache_Control` (pattern): captures are grouped into key/value pairs (where a directive with no value has a value of `true`)
  - `Expires` (pattern): capture is a table in the same format as used by [`os.time`](http://www.lua.org/manual/5.3/manual.html#pdf-os.time)
  - `Pragma` (pattern)
  - `Warning` (pattern)


#### [RFC 7235](https://tools.ietf.org/html/rfc7235)

  - `WWW_Authenticate` (pattern)
  - `Authorization` (pattern)
  - `Proxy_Authenticate` (pattern)
  - `Proxy_Authorization` (pattern)


#### [RFC 7239](https://tools.ietf.org/html/rfc7239)

  - `Forwarded` (pattern)


#### [RFC 7469](https://tools.ietf.org/html/rfc7469)

  - `Public_Key_Pins` (pattern)
  - `Public_Key_Pins_Report_Only` (pattern)


#### [RFC 7486](https://tools.ietf.org/html/rfc7486)

  - `Hobareg` (pattern)


#### [RFC 7615](https://tools.ietf.org/html/rfc7615)

  - `Authentication_Info` (pattern)
  - `Proxy_Authentication_Info` (pattern)


#### [RFC 7639](https://tools.ietf.org/html/rfc7639)

  - `ALPN` (pattern)


#### [RFC 7809](https://tools.ietf.org/html/rfc7809)

  - `CalDAV_Timezones` (pattern)


#### [RFC 7838](https://tools.ietf.org/html/rfc7838)

  - `Alt_Svc` (pattern)
  - `Alt_Used` (pattern)


#### [Expect-CT Extension for HTTP](https://tools.ietf.org/html/draft-ietf-httpbis-expect-ct-06)

  - `Expect_CT` (pattern)


#### [Referrer-Policy header](https://www.w3.org/TR/referrer-policy/#referrer-policy-header)

  - `Referrer_Policy` (pattern)


### `phone`

  - `phone` (pattern): includes detailed checking for:
      - USA phone numbers using the [NANP](https://en.wikipedia.org/wiki/North_American_Numbering_Plan)


### `language`

Patterns for definitions from [RFC-4646 Section 2.1](https://tools.ietf.org/html/rfc4646#section-2.1)

  - `langtag` (pattern): Capture is a table with the language tag decomposed into components:
      - `language`
      - `extlang` (optional)
      - `script` (optional)
      - `region` (optional)
      - `variant` (optional): an array
      - `extension` (optional): a dictionary from singleton to value
      - `privateuse` (optional): an array
  - `privateuse` (pattern): captures an array
  - `Language_Tag` (pattern): captures the whole language tag
