from match import Match

class FunctionPrototype:
    """ takes a single function prototype line (cut from some source file)
    and parses it into the relevant portions 'prespec', 'namespec' and
    'callspec'. Additionally we present 'name' from the namespec that is
    usually used as the filename stem for a manual page """
    def __init__(self, functionheader = None):
        self.functionheader = functionheader
        self.prespec = None
        self.namespec = None
        self.callspec = None
        self.name = None
    def get_functionheader(self):
        return self.functionheader
    def get_prototype(self):
        if self.functionheader is None:
            return None
        return self.functionheader.get_prototype()
    def get_filename(self):
        if self.functionheader is None:
            return None
        return self.functionheader.get_filename()
    def parse(self, functionheader = None):
        if functionheader is not None:
            self.functionheader = functionheader
        if self.functionheader is None:
            return False
        found = Match()
        prototype = self.get_prototype()
        if prototype & found(r"(?s)^(.*[^.])"
                             r"\b(\w[\w.]*\w)\b"
                             r"(\s*\(.*)$"):
            self.prespec = found.group(1).lstrip()
            self.namespec = found.group(2)
            self.callspec = found.group(3).lstrip()
            self.name = self.namespec.strip()
            return True
        return False
    def _assert_parsed(self):
        if self.name is None:
            return self.parse()
        return True
    def get_prespec(self):
        if not self._assert_parsed(): return None
        return self.prespec
    def get_namespec(self):
        if not self._assert_parsed(): return None
        return self.namespec
    def get_callspec(self):
        if not self._assert_parsed(): return None
        return self.callspec
    def get_name(self):
        if not self._assert_parsed(): return None
        return self.name
    def xml_text(self):
        if not self.namespec: return self.namespec
        return ("<fu:protospec><fu:prespec>"+self.prespec+"</fu:prespec>"+
                "<fu:namespec>"+self.namespec+"</fu:namespec>"+
                "<fu:callspec>"+self.callspec+"</fu:callspec></fu:protospec>")
