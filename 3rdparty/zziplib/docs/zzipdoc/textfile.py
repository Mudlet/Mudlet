
def _src_to_xml(text):
    return text.replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt")

class TextFile:
    def __init__(self, filename = None):
        self.filename = filename
        self.src_text = None
        self.xml_text = None
    def parse(self, filename = None):
        if filename is not None:
            self.filename = filename
        if self.filename is None:
            return False
        try:
            fd = open(self.filename, "r")
            self.src_text = fd.read()
            fd.close()
            return True
        except IOError, e:
            pass
        return False
    def assert_src_text(self):
        if self.src_text: return True
        return self.parse()
    def assert_xml_text(self):
        if self.xml_text: return True
        if not self.assert_src_text(): return False
        self.xml_text = _src_to_xml(self.src_text)
    def get_src_text(self):
        self.assert_src_text()
        return self.src_text
    def get_xml_text(self):
        self.assert_xml_text()
        return self.xml_text
    def get_filename(self):
        return self.filename
    def line_xml_text(self, offset):
        self._line(self.xml_text, offset)
    def line_src_text(self, offset):
        self._line(self.src_text, offset)
    def _line(self, text, offset):
        line = 1
        for x in xrange(0,offset):
            if x == "\n":
                line += 1
        return line
            

