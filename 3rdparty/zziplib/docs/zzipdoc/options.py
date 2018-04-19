#! /usr/bin/python
# -*- coding: utf-8 -*-
# @creator (C) 2003 Guido U. Draheim
# @license http://creativecommons.org/licenses/by-nc-sa/2.0/de/

from match import Match

# use as o.optionname to check for commandline options.
class Options:
    var = {}
    def __getattr__(self, name):
        if not self.var.has_key(name): return None
        return self.var[name]
    def __setattr__(self, name, value):
        self.var[name] = value
    def scan(self, optionstring): # option-name or None
        x = Match()
        if optionstring & x(r"^--?(\w+)=(.*)"):
            self.var[x[1]] = x[2] ;  return x[1]
        if optionstring & x(r"^--?no-(\w+)$"):
            self.var[x[1]] = "" ; return x[1]
        if optionstring & x(r"^--?(\w+)$"):
            self.var[x[1]] = "*"; return x[1]
        return None
#end Options

if False:
    o = Options()
    o.help = """
    scans for options
    """
