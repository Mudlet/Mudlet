from match import Match

class FunctionHeader:
    """ parsing the comment block that is usually presented before
    a function prototype - the prototype part is passed along
    for further parsing through => FunctionPrototype """
    def __init__(self, functionheaderlist, comment, prototype):
        self.parent = functionheaderlist
        self.comment = comment
        self.prototype = prototype
        self.firstline = None
        self.otherlines = None
        self.titleline = None
        self.alsolist = []
    def get_filename(self):
        return self.parent.get_filename()
    def parse_firstline(self):
        if not self.comment: return False
        x = self.comment.find("\n")
        if x > 0:
            self.firstline = self.comment[:x]
            self.otherlines = self.comment[x:]
        elif x == 0:
            self.firstline = "..."
            self.otherlines = self.comment[1:x]
        else:
            self.firstline = self.comment
            self.otherlines = ""
        return True
    def get_firstline(self):
        if self.firstline is None:
            if not self.parse_firstline(): return ""
        return self.firstline
    def get_otherlines(self):
        if self.firstline is None:
            if not self.parse_firstline(): return ""
        return self.otherlines
    def parse_titleline(self):
        """ split extra-notes from the firstline - keep only titleline """
        line = self.get_firstline()
        if line is None: return False
        self.titleline = line
        self.alsolist = []
        x = line.find("also:")
        if x > 0:
            self.titleline = line[:x]
            for also in line[x+5:].split(","):
                self.alsolist += [ also.strip() ]
        self._alsolist = self.alsolist
        return True
    def get_alsolist(self):
        """ gets the see-also notes from the firstline """
        if self.titleline is None:
            if not self.parse_titleline(): return None
        return self.alsolist
    def get_titleline(self):
        """ gets firstline with see-also notes removed """
        if self.titleline is None:
            if not self.parse_titleline(): return False
        return self.titleline
    def get_title(self):
        """ gets titleline unless that is a redirect """
        titleline = self.get_titleline()
        if titleline & Match(r"^\s*=>"): return ""
        if titleline & Match(r"^\s*<link>"): return ""
        return titleline
    def get_prototype(self):
        return self.prototype
    
class FunctionHeaderList:
    """ scan for comment blocks in the source file that are followed by
    something quite like a C definition (probably a function definition).
    Unpack the occurrences and fill self.comment and self.prototype. """
    def __init__(self, textfile = None):
        self.textfile = textfile # TextFile
        self.children = None     # src'style
    def parse(self, textfile = None):
        if textfile is not None:
            self.textfile = textfile
        if self.textfile is None:
            return False
        text = self.textfile.get_src_text()
        m = Match(r"(?s)\/\*[*]+(?=\s)"
                  r"((?:.(?!\*\/))*.)\*\/"
                  r"([^/\{\}\;\#]+)[\{\;]")
        self.children = []
        for found in m.finditer(text):
            child = FunctionHeader(self, found.group(1), found.group(2))
            self.children += [ child ]
        return len(self.children) > 0
    def get_filename(self):
        return self.textfile.get_filename()
    def get_children(self):
        if self.children is None:
            if not self.parse(): return []
        return self.children
