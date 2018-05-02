from match import Match
import string

class dbk2htm_conversion:
    mapping = { "<screen>" : "<pre>", "</screen>" : "</pre>",
                "<para>" : "<p>", "</para>" : "</p>" ,
                "<function>" : "<link>", "</function>" : "</link>" }
    def __init__(self):
        pass
    def section2html(self, text):
        for str in self.mapping:
            text = string.replace(text, str, self.mapping[str])
        return text
    def paramdef2html(self, text):
        s = Match()
        txt = text & s(r"\s+<paramdef>") >> r"\n<nobr>"
        txt &= s(r"<paramdef>") >> r"<nobr>"
        txt &= s(r"</paramdef>") >> r"</nobr>"
        txt &= s(r"<parameters>") >> r"\n <code>"
        txt &= s(r"</parameters>") >> r"</code>\n"
        return txt

def section2html(text):
    return dbk2htm_conversion().section2html(text)
def paramdef2html(text):
    return dbk2htm_conversion().paramdef2html(text)
