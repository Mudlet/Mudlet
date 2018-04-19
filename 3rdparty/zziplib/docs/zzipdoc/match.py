#! /usr/bin/python
# -*- coding: utf-8 -*-
# @creator (C) 2003 Guido U. Draheim
# @license http://creativecommons.org/licenses/by-nc-sa/2.0/de/

import re

# ---------------------------------------------------------- Regex Match()
# beware, stupid python interprets backslashes in replace-parts only partially!
class MatchReplace:
    """ A MatchReplace is a mix of a Python Pattern and a Replace-Template """
    def __init__(self, matching, template, count = 0, flags = None):
        """ setup a substition from regex 'matching' into 'template',
            the replacement count default of 0 will replace all occurrences.
            The first argument may be a Match object or it is a string that
            will be turned into one by using Match(matching, flags). """
        self.template = template
        MatchReplace.__call__(self, matching, template, count, flags)
    def __call__(self, matching, template = None, count = 0, flags = None):
        """ other than __init__ the template may be left off to be unchanged"""
        if isinstance(count, basestring): # count/flags swapped over?
            flags = count; count = 0
        if isinstance(matching, Match):
            self.matching = matching
        else:
            self.matching = Match()(matching, flags) ## python 2.4.2 bug
        if template is not None:
            self.template = template
        self.count = count
    def __and__(self, string):
        """ z = MatchReplace('foo', 'bar') & 'foo'; assert z = 'bar' """
        text, self.matching.replaced = \
              self.matching.regex.subn(self.template, string, self.count)
        return text
    def __rand__(self, string):
        """ z = 'foo' & Match('foo') >> 'bar'; assert z = 'bar' """
        text, self.matching.replaced = \
              self.matching.regex.subn(self.template, string, self.count)
        return text
    def __iand__(self, string):
        """ x = 'foo' ; x &= Match('foo') >> 'bar'; assert x == 'bar' """
        string, self.matching.replaced = \
                self.matching.regex.subn(self.template, string, self.count)
        return string
    def __rshift__(self, count):
        " shorthand to set the replacement count: Match('foo') >> 'bar' >> 1 "
        self.count = count ; return self
    def __rlshift__(self, count):
        self.count = count ; return self

class Match(str):
    """ A Match is actually a mix of a Python Pattern and MatchObject """
    def __init__(self, pattern = None, flags = None):
        """ flags is a string: 'i' for case-insensitive etc.; it is just
        short for a regex prefix: Match('foo','i') == Match('(?i)foo') """
        Match.__call__(self, pattern, flags)
    def __call__(self, pattern, flags = None):
        assert isinstance(pattern, str) or pattern is None
        assert isinstance(flags, str) or flags is None
        str.__init__(self, pattern)
        self.replaced = 0 # set by subn() inside MatchReplace
        self.found = None # set by search() to a MatchObject
        self.pattern = pattern
        if pattern is not None:
            if flags:
                self.regex = re.compile("(?"+flags+")"+self.pattern)
            else:
                self.regex = re.compile(self.pattern)
        return self
    def __truth__(self):
        return self.found is not None
    def __and__(self, string):
        self.found = self.regex.search(string)
        return self.__truth__()
    def __rand__(self, string):
        self.found = self.regex.search(string)
        return self.__truth__()
    def __rshift__(self, template):
        return MatchReplace(self, template)
    def __rlshift__(self, template):
        return MatchReplace(self, template)
    def __getitem__(self, index):
        return self.group(index)
    def group(self, index):
        assert self.found is not None
        return self.found.group(index)
    def finditer(self, string):
        return self.regex.finditer(string)

if __name__ == "__main__":
    # matching:
    if "foo" & Match("oo"):
        print "oo"
    x = Match()
    if "foo" & x("(o+)"):
        print x[1]
    # replacing:
    y = "fooboo" & Match("oo") >> "ee"
    print y
    r = Match("oo") >> "ee"
    print "fooboo" & r
    s = MatchReplace("oo", "ee")
    print "fooboo" & s
