package = "cqueues"
version = "20190813.51-0"
source = {
	url = "https://github.com/wahern/cqueues/archive/rel-20190813.tar.gz";
	md5 = "24b6f642514dabc9fb7996d2595060fa";
	dir = "cqueues-rel-20190813";
}
description = {
	summary = "Continuation Queues: Embeddable asynchronous networking, threading, and notification framework for Lua on Unix.";
	homepage = "http://25thandclement.com/~william/projects/cqueues.html";
	license = "MIT/X11";
	maintainer = "Daurnimator <quae@daurnimator.com>";
}
supported_platforms = {
	"linux";
	"bsd";
	"solaris";
}
dependencies = {
	"lua == 5.1";
}
external_dependencies = {
	OPENSSL = {
		header = "openssl/ssl.h";
		library = "ssl";
	};
	CRYPTO = {
		header = "openssl/crypto.h";
		library = "crypto";
	};
}
build = {
	type = "make";
	makefile = "GNUmakefile";

	build_target = "all5.1";
	build_variables = {
		CPPFLAGS= '-I"$(OPENSSL_INCDIR)" -I"$(CRYPTO_INCDIR)"';
		CFLAGS = '$(CFLAGS)';
		LDFLAGS = '$(LIBFLAG) -L"$(OPENSSL_LIBDIR)" -L"$(CRYPTO_LIBDIR)"';
		bindir = "$(LUA_BINDIR)";
		includedir = "$(LUA_INCDIR)";
		libdir = "$(LUA_LIBDIR)";
	};

	install_target = "install5.1";
	install_variables = {
		prefix = "$(PREFIX)";
		lua51cpath = "$(LIBDIR)";
		lua51path = "$(LUADIR)";
		bindir = "$(BINDIR)";
	};
}
