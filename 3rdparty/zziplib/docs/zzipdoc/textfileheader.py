from match import Match

class TextFileHeader:
    """ scan for a comment block at the source file start and fill the
    inner text into self.comment - additionally scan for the first
    #include statement and put the includename into self.mainheader
    (TextFileHeader re-exports all => TextFile methods for processing)"""
    def __init__(self, textfile = None):
        self.textfile = textfile # TextFile
        self.comment = ""    # src'style
        self.mainheader = ""     # src'style
    def parse(self, textfile = None):
        if textfile is not None:
            self.textfile = textfile
        if self.textfile is None:
            return False
        x = Match()
        text = self.textfile.get_src_text()
        if not text:
            print "nonexistant file:", self.textfile.get_filename()
            return False
        if text & x(r"(?s)[/][*]+(\s(?:.(?!\*\/))*.)\*\/"
                    r"(?:\s*\#(?:define|ifdef|endif)[ ]*\S*[ ]*\S*)*"
                    r"(\s*\#include\s*<[^<>]*>(?:\s*//[^\n]*)?)"):
            self.comment = x[1]
            self.mainheader = x[2].strip()
        elif text & x(r"(?s)[/][*]+(\s(?:.(?!\*\/))*.)\*\/"):
            self.comment = x[1]
        elif text & x(r"(?s)(?:\s*\#(?:define|ifdef|endif)[ ]*\S*[ ]*\S*)*"
                      r"(\s*\#include\s*<[^<>]*>(?:\s*//[^\n]*)?)"):
            self.mainheader = x[1].strip()
        return True
    def src_mainheader(self):
        return self.mainheader
    def src_filecomment(self):
        return self.comment
    # re-export textfile functions - allows textfileheader to be used instead
    def get_filename(self):
        return self.textfile.get_filename()
    def get_src_text(self):
        return self.textfile.get_src_text()
    def get_xml_text(self):
        return self.textfile.get_src_text()
    def line_src__text(self, offset):
        return self.textfile.line_src_text(offset)
    def line_xml__text(self, offset):
        return self.textfile.line_xml_text(offset)
