#! /usr/bin/env python
# -*- coding: UTF-8 -*-
from match import Match

class DocbookDocument:
    """ binds some xml content page with additional markup - in this
    variant we set the rootnode container to 'reference' and the DTD
    to the Docbook 4.1.2 version. Modify as you like."""
    has_title_child = [ "book", "chapter", "section", "reference" ]
    docbook_dtd = (
        ' PUBLIC "-//OASIS//DTD DocBook XML V4.1.2//EN"'+"\n"+
        '       "http://www.oasis-open.org/docbook/xml/4.1.2/docbookx.dtd"')
    def __init__(self, o, filename = None):
        self.o = o
        self.rootnode = "reference"
        self.filename = filename
        self.title = ""
        self.text = []
    def add(self, text):
        """ add some content """
        self.text += [ text ]
        return self
    def get_title(self):
        if self.title: return title
        try:   return self.text[0].get_title()
        except Exception, e: pass
        return self.title
    def _xml_doctype(self, rootnode):
        return "<!DOCTYPE "+rootnode+self.docbook_dtd+">"
    def _xml_text(self, xml):
        """ accepts adapter objects with .xml_text() """
        try:   return xml.xml_text()
        except Exception, e: print "DocbookDocument/text", e; pass
        return str(xml)
    def _fetch_rootnode(self, text):
        fetch = Match(r"^[^<>]*<(\w+)\b")
        if text & fetch: return fetch[1]
        return self.rootnode
    def _filename(self, filename):
        if filename is not None:
            self.filename = filename
        filename = self.filename
        if not filename & Match(r"\.\w+$"):
            ext = self.o.docbook
            if not ext: ext = "docbook"
            filename += "."+ext
        return filename
    def save(self, filename = None):
        filename = self._filename(filename)
        print "writing '"+filename+"'"
        if len(self.text) > 1:
            self.save_all(filename)
        else:
            self.save_text(filename, self.text[0])
    def save_text(self, filename, text):
        try:
            fd = open(filename, "w")
            xml_text = self._xml_text(text)
            rootnode = self._fetch_rootnode(xml_text)
            doctype = self._xml_doctype(rootnode)
            print >>fd, doctype
            print >>fd, xml_text
            fd.close()
            return True
        except IOError, e:
            print "could not open '"+filename+"'file", e
            return False
    def save_all(self, filename):
        assert len(self.text) > 1
        try:
            fd = open(filename, "w")
            xml_text = self._xml_text(self.text[0])
            rootnode = self._fetch_rootnode(xml_text)
            if rootnode == self.rootnode:
                rootnode = "book"
            else:
                rootnode = self.rootnode
            doctype = self._xml_doctype(rootnode)
            print >>fd, doctype
            title = self.get_title()
            if title and self.rootnode in self.has_title_child:
                print >>fd, "<"+self.rootnode+'><title>'+title+'</title>'
            elif title:
                print >>fd, "<"+self.rootnode+' id="'+title+'">'
            else:
                print >>fd, "<"+self.rootnode+'>'
            for text in self.text:
                text = self._xml_text(text)
                print >>fd, text
            print >>fd, "</"+self.rootnode+">"
            fd.close()
            return True
        except IOError, e:
            print "could not open '"+filename+"'file", e
            return False
