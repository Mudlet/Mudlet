#! /usr/bin/env python
# -*- coding: UTF-8 -*-
from match import Match

class HtmlDocument:
    """ binds some html content page with additional markup - in this
    base version it is just the header information while other variants
    might add navigation items around the content block elements """
    def __init__(self, o, filename = None):
        self.o = o
        self.filename = filename
        self.title = ""
        self.meta = []
        self.style = []
        self.text = []
        self.navi = None
    def meta(self, style):
        """ add some header meta entry """
        self.meta += [ meta ]
        return self
    def style(self, style):
        """ add a style block """
        self.style += [ style ]
        return self
    def add(self, text):
        """ add some content """
        self.text += [ text ]
        return self
    def get_title(self):
        if self.title: return self.title
        try:   return self.text[0].get_title()
        except Exception, e: pass
        return self.title
    def _html_meta(self, meta):
        """ accepts adapter objects with .html_meta() """
        try:   return meta.html_meta()
        except Exception, e: pass
        return str(meta)
    def _html_style(self, style):
        """ accepts adapter objects with .html_style() and .xml_style() """
        ee = None
        try:   return style.html_style()
        except Exception, e: ee = e; pass
        try:   return style.xml_style()
        except Exception, e: print "HtmlDocument/style", ee, e; pass
        try:   return str(style)
        except Exception, e: print "HtmlDocument/style", e; return ""
    def _html_text(self, html):
        """ accepts adapter objects with .html_text() and .xml_text() """
        ee = None
        try:   return html.html_text()
        except Exception, e: ee = e; pass
        try:   return html.xml_text()
        except Exception, e: print "HtmlDocument/text", ee, e; pass
        try:   return str(html)
        except Exception, e: print "HtmlDocument/text", e; return "&nbsp;"
    def navigation(self):
        if self.navi:
            return self.navi
        if self.o.body:
            try:
                fd = open(self.o.body, "r")
                self.navi = fd.read()
                fd.close()
                return self.navi
            except Exception, e:
                pass
        return None
    def html_header(self):
        navi = self.navigation()
        if not navi:
            T = "<html><head>"
            title = self.get_title()
            if title:
                T += "<title>"+title+"</title>"
            T += "\n"
            for style in self.style:
                T += self._html_style(style)
                T += "\n"
            return T+"</head><body>"
        else:
            title = self.get_title()
            return navi & (
                Match(r"<!--title-->") >> " - "+title) & (
                Match(r"<!--VERSION-->") >> self.o.version) & (
                Match(r"(?m).*</body></html>") >> "")
    def html_footer(self):
        navi = self.navigation()
        if not navi:
            return "</body></html>"
        else:
            return navi & (
                Match(r"(?m)(.*</body></html>)") >> "%&%&%&%\\1") & (
                Match(r"(?s).*%&%&%&%") >> "")
    def _filename(self, filename):
        if filename is not None:
            self.filename = filename
        filename = self.filename
        if not filename & Match(r"\.\w+$"):
            ext = self.o.html
            if not ext: ext = "html"
            filename += "."+ext
        return filename
    def save(self, filename = None):
        filename = self._filename(filename)
        print "writing '"+filename+"'"
        try:
            fd = open(filename, "w")
            print >>fd, self.html_header()
            for text in self.text:
                print >>fd, self._html_text(text)
            print >>fd, self.html_footer()
            fd.close()
            return True
        except IOError, e:
            print "could not open '"+filename+"'file", e
            return False
