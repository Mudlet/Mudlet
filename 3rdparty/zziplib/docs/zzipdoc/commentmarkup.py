from match import Match

def markup_link_syntax(text):
    """ markup the link-syntax ` => somewhere ` in the text block """
    return (text
            & Match(r"(?m)(^|\s)\=\>\"([^\"]*)\"")
            >> r"\1<link>\2</link>"
            & Match(r"(?m)(^|\s)\=\>\'([^\']*)\'")
            >> r"\1<link>\2</link>"
            & Match(r"(?m)(^|\s)\=\>\s(\w[\w.]*\w\(\d+\))")
            >> r"\1<link>\2</link>"
            & Match(r"(?m)(^|\s)\=\>\s([^\s\,\.\!\?]+)")
            >> r"\1<link>\2</link>")

class CommentMarkup:
    """ using a structure having a '.comment' item - it does pick it up
    and enhances its text with new markups so that they can be represented
    in xml. Use self.xml_text() to get markup text (knows 'this function') """
    def __init__(self, header = None):
        self.header = header
        self.text = None     # xml'text
    def get_filename(self):
        if self.header is None:
            return None
        return self.header.get_filename()
    def parse(self, header = None):
        if header is not None:
            self.header = header
        if self.header is None:
            return False
        comment = self.header.comment
        try:
            comment = self.header.get_otherlines()
        except Exception, e:
            pass
        mode = ""
        text = ""
        for line in comment.split("\n"):
            check = Match()
            if line & check(r"^\s?\s?\s?[*]\s+[*]\s(.*)"):
                if mode != "ul":
                    if mode: text += "</"+mode+">"
                    mode = "ul" ; text += "<"+mode+">"
                line = check.group(1)
                text += "<li><p> "+self.markup_para_line(line)+" </p></li>\n"
            elif line & check(r"^\s?\s?\s?[*](.*)"):
                if mode != "para":
                    if mode: text += "</"+mode+">"
                    mode = "para" ; text += "<"+mode+">"
                line = check.group(1)
                if line.strip() == "":
                    text += "</para><para>"+"\n"
                else:
                    text += " "+self.markup_para_line(line)+"\n"
            else:
                if mode != "screen":
                    if mode: text += "</"+mode+">"
                    mode = "screen" ; text += "<"+mode+">"
                text += " "+self.markup_screen_line(line)+"\n"
        if mode: text += "</"+mode+">"+"\n"
        self.text = (text
                     & Match(r"(<para>)(\s*[R]eturns)") >>r"\1This function\2"
                     & Match(r"(?s)<para>\s*</para><para>") >> "<para>"
                     & Match(r"(?s)<screen>\s*</screen>") >> "")
        return True
    def markup_screen_line(self, line):
        return self.markup_line(line.replace("&","&amp;")
                                .replace("<","&lt;")
                                .replace(">","&gt;"))
    def markup_para_line(self, line):
        return markup_link_syntax(self.markup_line(line))
    def markup_line(self, line):
        return (line
                .replace("<c>","<code>")
                .replace("</c>","</code>"))
    def xml_text(self, functionname = None):
        if self.text is None:
            if not self.parse(): return None
        text = self.text
        if functionname is not None:
            def function(text): return "<function>"+text+"</function> function"
            text = (text
                    .replace("this function", "the "+function(functionname))
                    .replace("This function", "The "+function(functionname)))
        return text
