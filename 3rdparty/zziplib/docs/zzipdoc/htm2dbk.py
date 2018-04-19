#! /usr/bin/env python

"""
this file converts simple html text into a docbook xml variant. 
The mapping of markups and links is far from perfect. But all we
want is the docbook-to-pdf converter and similar technology being
present in the world of docbook-to-anything converters. """

from datetime import date
import match
import sys

m = match.Match

class htm2dbk_conversion_base:
    regexlist = [
        m()("</[hH]2>(.*)", "m") >> "</title>\n<subtitle>\\1</subtitle>",
        m()("<[hH]2>") >> "<sect1 id=\"--filename--\"><title>",
        m()("<[Pp]([> ])","m") >> "<para\\1",
        m()("</[Pp]>") >> "</para>",
        m()("<(pre|PRE)>") >> "<screen>",
        m()("</(pre|PRE)>") >> "</screen>",
        m()("<[hH]3>") >> "<sect2><title>",
        m()("</[hH]3>((?:.(?!<sect2>))*.?)", "s") >> "</title>\\1</sect2>",
        m()("<!doctype [^<>]*>","s") >> "",
        m()("<!DOCTYPE [^<>]*>","s") >> "",
        m()("(<\w+\b[^<>]*\swidth=)(\d+\%)","s") >> "\\1\"\\2\"",
        m()("(<\w+\b[^<>]*\s\w+=)(\d+)","s") >> "\\1\"\\2\"",
        m()("&&") >> "\&amp\;\&amp\;",
        m()("\$\<") >> "\$\&lt\;",
        m()("&(\w+[\),])") >> "\&amp\;\\1",
        m()("(</?)span(\s[^<>]*)?>","s") >> "\\1phrase\\2>",
        m()("(</?)small(\s[^<>]*)?>","s") >> "\\1note\\2>",
        m()("(</?)(b|em|i)>")>> "\\1emphasis>",
        m()("(</?)(li)>") >> "\\1listitem>",
        m()("(</?)(ul)>") >> "\\1itemizedlist>",
        m()("(</?)(ol)>") >> "\\1orderedlist>",
        m()("(</?)(dl)>") >> "\\1variablelist>",
        m()("<dt\b([^<>]*)>","s") >> "<varlistentry\\1><term>",
        m()("</dt\b([^<>]*)>","s") >> "</term>",
        m()("<dd\b([^<>]*)>","s") >> "<listitem\\1>",
        m()("</dd\b([^<>]*)>","s") >> "</listitem></varlistentry>",
        m()("<table\b([^<>]*)>","s")
        >> "<informaltable\\1><tgroup cols=\"2\"><tbody>",
        m()("</table\b([^<>]*)>","s") >> "</tbody></tgroup></informaltable>",
        m()("(</?)tr(\s[^<>]*)?>","s") >> "\\1row\\2>",
        m()("(</?)td(\s[^<>]*)?>","s") >> "\\1entry\\2>",
        m()("<informaltable\b[^<>]*>\s*<tgroup\b[^<>]*>\s*<tbody>"+
          "\s*<row\b[^<>]*>\s*<entry\b[^<>]*>\s*<informaltable\b","s")
        >> "<informaltable",
        m()("</informaltable>\s*</entry>\s*</row>"+
          "\s*</tbody>\s*</tgroup>\s*</informaltable>", "s")
        >> "</informaltable>",
        m()("(<informaltable[^<>]*\swidth=\"100\%\")","s") >> "\\1 pgwide=\"1\"",
        m()("(<tbody>\s*<row[^<>]*>\s*<entry[^<>]*\s)(width=\"50\%\")","s")
        >> "<colspec colwidth=\"1*\" /><colspec colwidth=\"1*\" />\n\\1\\2",
        m()("<nobr>([\'\`]*)<tt>") >> "<cmdsynopsis>\\1",
        m()("</tt>([\'\`]*)</nobr>") >> "\\1</cmdsynopsis>",
        m()("<nobr><(?:tt|code)>([\`\"\'])") >> "<cmdsynopsis>\\1",
        m()("<(?:tt|code)><nobr>([\`\"\'])") >> "<cmdsynopsis>\\1",
        m()("([\`\"\'])</(?:tt|code)></nobr>") >> "\\1</cmdsynopsis>",
        m()("([\`\"\'])</nobr></(?:tt|code)>") >> "\\1</cmdsynopsis>",
        m()("(</?)tt>") >> "\\1constant>",
        m()("(</?)code>") >> "\\1literal>",
        m()(">([^<>]+)<br>","s") >> "><highlights>\\1</highlights>",
        m()("<br>") >> "<br />",
        #        m()("<date>") >> "<sect1info><date>",
        #        m()("</date>") >> "</date></sect1info>",
        m()("<reference>") >> "<reference id=\"reference\">" >> 1,
        m()("<a\s+href=\"((?:http|ftp|mailto):[^<>]+)\"\s*>((?:.(?!</a>))*.)</a>"
          ,"s") >> "<ulink url=\"\\1\">\\2</ulink>",
        m()("<a\s+href=\"zziplib.html\#([\w_]+)\"\s*>((?:.(?!</a>))*.)</a>","s")
        >> "<link linkend=\"$1\">$2</link>",
        m()("<a\s+href=\"(zziplib.html)\"\s*>((?:.(?!</a>))*.)</a>","s")
        >> "<link linkend=\"reference\">$2</link>",
        m()("<a\s+href=\"([\w-]+[.]html)\"\s*>((?:.(?!</a>))*.)</a>","s")
        >> "<link linkend=\"\\1\">\\2</link>",
        m()("<a\s+href=\"([\w-]+[.](?:h|c|am|txt))\"\s*>((?:.(?!</a>))*.)</a>"
          ,"s") >> "<ulink url=\"file:\\1\">\\2</ulink>",
        m()("<a\s+href=\"([A-Z0-9]+[.][A-Z0-9]+)\"\s*>((?:.(?!</a>))*.)</a>","s")
        >> "<ulink url=\"file:\\1\">\\2</ulink>"
        # m()("(</?)subtitle>") >> "\\1para>"
        # $_ .= "</sect1>" if /<sect1[> ]/
        ]
    regexlist2 = [
        m()(r"<br\s*/?>") >> "",
        m()(r"(</?)em>") >> r"\1emphasis>",
        m()(r"<code>") >> "<userinput>",
        m()(r"</code>") >> "</userinput>",
        m()(r"<link>") >> "<function>",
        m()(r"</link>") >> "</function>",
        m()(r"(?s)\s*</screen>") >> "</screen>",
        # m()(r"<ul>") >> "</para><programlisting>\n",
        # m()(r"</ul>") >> "</programlisting><para>",
        m()(r"<ul>") >> "<itemizedlist>",
        m()(r"</ul>") >> "</itemizedlist>",
        # m()(r"<li>") >> "",
        # m()(r"</li>") >> ""
        m()(r"<li>") >> "<listitem><para>",
        m()(r"</li>") >> "</para></listitem>\n",
        ]
class htm2dbk_conversion(htm2dbk_conversion_base):
    def __init__(self):
        self.version = "" # str(date.today)
        self.filename = "."
    def convert(self,text): # $text
        txt = text.replace("<!--VERSION-->", self.version)
        for conv in self.regexlist:
            txt &= conv
        return txt.replace("--filename--", self.filename)
    def convert2(self,text): # $text
        txt = text.replace("<!--VERSION-->", self.version)
        for conv in self.regexlist:
            txt &= conv
        return txt

class htm2dbk_document(htm2dbk_conversion):
    """ create document, add(text) and get the value() """
    doctype = (
        '<!DOCTYPE book PUBLIC "-//OASIS//DTD'+
        ' DocBook XML V4.1.2//EN"'+"\n"+
        '       "http://www.oasis-open.org/docbook/xml/4.1.2/docbookx.dtd">'+
        "\n")
    book_start = '<book><chapter><title>Documentation</title>'+"\n"
    book_end_chapters = '</chapter>'+"\n"
    book_end = '</book>'+"\n"
    def __init__(self):
        htm2dbk_conversion.__init__(self)
        self.text = self.doctype + self.book_start
    def add(self,text):
        if self.text & m()("<reference"):
            self.text += self.book_end_chapters ; self.book_end_chapters = ""
        self.text += self.convert(text).replace(
            "<br />","") & (
            m()("<link>([^<>]*)</link>") >> "<function>\\1</function>") & (
            m()("(?s)(<refentryinfo>\s*)<sect1info>" +
                "(<date>[^<>]*</date>)</sect1info>") >> "\\1\\2")
    def value(self):
        return self.text + self.book_end_chapters + self.book_end

def htm2dbk_files(args):
    doc = htm2dbk_document()
    for filename in args:
        try:
            f = open(filename, "r")
            doc.filename = filename
            doc.add(f.read())
            f.close()
        except IOError, e:
            print >> sys.stderr, "can not open "+filename
    return doc.value()

def html2docbook(text):
    """ the C comment may contain html markup - simulate with docbook tags """
    return htm2dbk_conversion().convert2(text)

if __name__ == "__main__":
    print htm2dbk_files(sys.argv[1:])
