package = "fifo"
version = "0.2-0"

description= {
	summary = "A lua library/'class' that implements a FIFO";
	homepage = "https://github.com/daurnimator/fifo.lua";
	license = "MIT/X11";
}

dependencies = {
	"lua";
}

source = {
	url = "https://github.com/daurnimator/fifo.lua/archive/0.2.zip";
	dir = "fifo.lua-0.2";
}

build = {
	type = "builtin";
	modules = {
		["fifo"] = "fifo.lua";
	};
}
