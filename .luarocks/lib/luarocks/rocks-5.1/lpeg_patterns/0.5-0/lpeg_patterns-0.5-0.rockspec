package = "lpeg_patterns"
version = "0.5-0"

description= {
	summary = "a collection of LPEG patterns";
	license = "MIT";
}

dependencies = {
	"lua";
	"lpeg";
}

source = {
	url = "https://github.com/daurnimator/lpeg_patterns/archive/v0.5.zip";
	dir = "lpeg_patterns-0.5";
}

build = {
	type = "builtin";
	modules = {
		["lpeg_patterns.util"] = "lpeg_patterns/util.lua";
		["lpeg_patterns.core"] = "lpeg_patterns/core.lua";
		["lpeg_patterns.IPv4"] = "lpeg_patterns/IPv4.lua";
		["lpeg_patterns.IPv6"] = "lpeg_patterns/IPv6.lua";
		["lpeg_patterns.uri"] = "lpeg_patterns/uri.lua";
		["lpeg_patterns.email"] = "lpeg_patterns/email.lua";
		["lpeg_patterns.http"] = "lpeg_patterns/http.lua";
		["lpeg_patterns.phone"] = "lpeg_patterns/phone.lua";
		["lpeg_patterns.language"] = "lpeg_patterns/language.lua";
	};
}
