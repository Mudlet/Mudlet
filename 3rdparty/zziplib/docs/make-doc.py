#! /usr/bin/python
# -*- coding: UTF-8 -*-
import sys
import re
import string
import commands
import warnings

errors = 0
def warn(msg, error=None):
    global errors
    errors += 1
    if error is None:
        warnings.warn("-- "+str(errors)+" --\n  "+msg, RuntimeWarning, 2)
    else:
        warnings.warn("-- "+str(errors)+" --\n  "+msg+
                      "\n  error was "+str(error), RuntimeWarning, 2)
#fu

# beware, stupid python interprets backslashes in repl only partially!
def s(string, pattern, repl, count=0):
    return re.sub(pattern, repl, string, count)
def m(string, pattern):
    return re.match(pattern, string)
def sorted_keys(dict):
    keys = dict.keys()
    keys.sort()
    return keys

# we make up a few formatter routines to help in the processing:
def html2docbook(text):
    """ the C comment may contain html markup - simulate with docbook tags """
    return (
        s(s(s(s(s(s(s(s(s(s(s(text,
                              r"<br\s*/?>",""),
                            r"(</?)em>",r"\1emphasis>"),
                          r"<code>","<userinput>"),
                        r"</code>","</userinput>"),
                      r"<link>","<function>"),
                    r"</link>","</function>"),
                  r"(?s)\s*</screen>","</screen>"),
#               r"<ul>","</para><itemizedlist>"),
#             r"</ul>","</itemizedlist><para>"),
#           r"<li>","<listitem><para>"),
#         r"</li>","</para></listitem>\n"),
                r"<ul>","</para><programlisting>\n"),
              r"</ul>","</programlisting><para>"),
            r"<li>",""),
          r"</li>",""))
def paramdef2html(text):
    return s(s(s(s(s(text,
                     r"\s+<paramdef>", r"\n<nobr>"),
                   r"<paramdef>",r"<nobr>"),
                 r"</paramdef>",r"</nobr>"),
               r"<parameters>",r"\n <code>"),
             r"</parameters>",r"</code>\n")
def section2html(text):
    mapping = { "<screen>" : "<pre>", "</screen>" : "</pre>",
                "<para>" : "<p>", "</para>" : "</p>" ,
                "<function>" : "<link>", "</function>" : "</link>" }
    for str in mapping:
        text = string.replace(text, str, mapping[str])
    return text
def html(text):
    return section2html(paramdef2html(text))
def cdata1(text):
    return string.replace(text, "&",  "&amp;")
def cdata31(text):
    return string.replace(string.replace(text, "<","&lt;"), ">","&gt;")
def cdata3(text):
    return cdata31(cdata1(text))
def cdata43(text):
    return string.replace(text,"\"", "&quot;")
def cdata41(text):
    return cdata43(cdata31(text))
def cdata4(text):
    return cdata43(cdata3(text))
def markup_as_screen41 (text):
    """ used for non-star lines in comment blocks """
    return " <screen> " + s(cdata41(text), r"(?m)^", r" ") +" </screen> "

def file_comment2section(text):
    """ convert a C comment into a series of <para> and <screen> parts """
    return ("<para>\n"+
            s(s(s(s(s(s(s(text,
                          r"(?s){<([\w\.\-]+\@[\w\.\-]+\w\w)>",
                          r"&lt;\1&gt;"),
                        r"(?mx) ^\s?\s?\s? ([^\*\s]+ .*) $",
                        lambda x : markup_as_screen41 (x.group(1))),
                      r"(?mx) ^\s*[*]\s* $", r" \n</para><para>\n"),
                    r"(?mx) ^\s?\s?\s?\* (.*) $", r" \1 "),
                  r"(?sx) </screen>(\s*)<screen> ", r"\1"),
                r"(?sx) <([^<>\;]+\@[^<>\;]+)> ", r"<email>\1</email>"),
              r"(?sx) \&lt\;([^<>\&\;]+\@[^<>\&\;]+)\&gt\; ",
              r"<email>\1</email>") + "\n</para>")
def func_comment2section(text):
    """ convert a C comment into a series of <para> and <screen> parts
        and sanitize a few markups already present in the comment text
    """
    return ("<para>\n"+
            s(s(s(s(s(s(s(s(s(s(s(text,
                                  r"<c>",r"<code>"),   r"</c>", r"</code>"),
                              r"(?mx) ^\s?\s?\s? ([^\*\s]+.*)",
                              lambda x: markup_as_screen41 (x.group(1))),
                            r"(?mx) ^\s?\s?\s?\* (.*) $", r" <br /> \1"),
                          r"(?mx) ^\s*<br\s*\/>\s* $", r"\n</para><para>\n"),
                        r"<<",r"&lt;"),   r">>",r"&gt;"),
                    r"(?sx) (</?para>\s*)<br\s*\/?>",r"\1"),
                  r"(?sx) (</?para>\s*)<br\s*\/?>",r"\1"),
                r"(?sx) (<br\s*\/?>\s*)<br\s*\/?>",r"\1"),
              r"(?sx) <\/screen>(\s*)<screen>",r"\1") + "\n</para>")
def markup_link_syntax(text):
    """ markup the link-syntax ` => somewhere ` in the text block """
    return (
        s(s(s(s(text,
                r"(?mx) (^|\s)\=\>\"([^\"]*)\"", r"\1<link>\2</link>"),
              r"(?mx) (^|\s)\=\>\'([^\"]*)\'", r"\1<link>\2</link>"),
            r"(?mx) (^|\s)\=\>\s(\w[\w.]*\w)\b", r"\1<link>\2</link>"),
          r"(?mx) (^|\s)\=\>\s([^\s\,\.\!\?\:\;\<\>\&\'\=\-]+)",
          r"\1<link>\2</link>"))
def this_function_link(text, name):
    return s(text, r"(?sx) (T|t)his \s (function|procedure) ", lambda x
             : "<function>"+x.group(1)+"he "+name+" "+x.group(2)+"</function>")

# -----------------------------------------------------------------------
class Options:
    var = {}
    def __getattr__(self, name):
        if not self.var.has_key(name): return None
        return self.var[name]
    def __setattr__(self, name, value):
        self.var[name] = value
#end

o = Options()
o.verbose = 0

o.version = s( commands.getoutput(
    """ grep -i "^version *:" *.spec 2>/dev/null |
        sed -e "s/[Vv]ersion *: *//" """),  r"\s*",r"")
o.package = s(commands.getoutput(
    """ grep -i "^name *:" *.spec 2>/dev/null |
        sed -e "s/[Nn]ame *: *//" """),     r"\s*",r"")

if not len(o.version):
    o.version = commands.getoutput(""" date +%Y.%m.%d """)
if not len(o.package):
    o.package = "_project"

o.suffix = "-doc3"
o.mainheader = o.package+".h"

class File:
    def __init__(self, filename):
        self.name = filename
        self.mainheader = o.mainheader
        self.authors = ""
        self.copyright = ""
    def __getattr__(self, name):
        """ defend against program to break on uninited members """
        if self.__dict__.has_key(name): return self.__dict__[name]
        warn("no such member: "+name); return None
    def set_author(self, text):
        if self.authors:
            self.authors += "\n"
        self.authors += text
        return text
    def set_copyright(self, text):
        self.copyright = text
        return text

class InputFiles:
    """ for each set of input files we can create an object
        it does correspond with a single html-output page and
        a single docbook <reference> master page to be output
    """
    def __init__(self):
        # the id will tell us in which order
        # we did meet each function definition
        self.id = 1000
        self.files = [] # file_list
        self.funcs = [] # func_list: of hidden class FuncDeclaration
        self.file = None # current file
    def new_File(self, name):
        self.file = File(name)
        self.files.append(self.file)
        return self.file
    def next_id(self):
        id = self.id ; self.id += 1
        return id
    def add_function_declaration(self, comment, prototype):
        class FuncDeclaration:    # note that both decl.comment and
            pass                  # decl.prototype are in cdata1 format
        func = FuncDeclaration()
        func.file = self.file
        func.comment = s(comment, # need to take out email-style markups
                         r"<([\w\.\-]+\@[\w\.\-]+\w\w)>", r"&lt;\1&gt;")
        func.prototype = prototype
        func.id = all.next_id()
        self.funcs.append(func)
        # print id
        return prototype

def scan_options (options, list):
    def encode(text):
        return s(s(text, r"¬",  r"&#AC;"), r"\*/",r"¬")
    def decode(text):
        return s(text, r"¬", r"*/")

    for name in options:
        found = m(name, r"^(\w+)=(.*)")
        if found:
            o.var[found.group(1)] = found.group(2)
            continue
        #else
        try:
            input = open(name, "r")
        except IOError, error:
            warn(#...... (scan_options) ...............
                "can not open input file: "+name, error)
            continue
        text = input.read() ; input.close()
        text = encode (cdata1 (text))

        file = list.new_File(name)
        
        # cut per-function comment block
        text = s(text, r"(?x) [/][*][*](?=\s) ([^¬]+) ¬ ([^\{\}\;\#]+) [\{\;]",
                 lambda x : list.add_function_declaration(
            decode(x.group(1)), decode(x.group(2))))

        # cut per-file comment block
        found = m(text, r"(?sx)  [/][*]+(?=\s) ([^¬]+) ¬ "
                  r"(?:\s*\#define\s*\S+)*"
                  r"(\s*\#include\s*<[^<>]*>(?:\s*//[^\n]*)?)")
        if found:
            file.comment = decode(found.group(1))
            file.include = cdata31(found.group(2))
        else:
            file.comment = None
            file.include = None
            found = m(text, r"(?sx)  ^ [/][*]+(?=\s) ([^¬]+) ¬ ")
            if found:
                file.comment = decode(found.group(1))
        #fi
        # throw away the rest - further processing on memorized strings only

    return None

all = InputFiles()
scan_options (sys.argv[1:], all)

if not o.docbookfile:
    o.docbookfile = o.package+o.suffix+".docbook"
if not o.libhtmlfile:
    o.libhtmlfile = o.package+o.suffix+".html"
if not o.dumpdocfile:
    o.dumpdocfile = o.package+o.suffix+".dxml"

# ...........................................................................
# check out information in the file.comment section

def all_files_comment2section(list):
    for file in list:
        if file.comment is None: continue
        file.section = file_comment2section(file.comment)
    
        file.section = s(
            file.section, r"(?sx) \b[Aa]uthor\s*:(.*</email>) ", lambda x
            : "<author>" + file.set_author(x.group(1)) + "</author>")
        file.section = s(
            file.section, r"(?sx) \b[Cc]opyright\s*:([^<>]*)</para> ",lambda x
            : "<copyright>" + file.set_copyright(x.group(1)) + "</copyright>")
        # if "file" in file.name: print >> sys.stderr, file.comment # 2.3
    #od
all_files_comment2section(all.files)

# -----------------------------------------------------------------------

class Function:
    " <prespec>void* </><namespec>hello</><namespec> (int) const</callspec> "
    def __init__(self):
        self.prespec  = ""
        self.namespec = ""
        self.callspec = ""
        self.name = ""
#    def set(self, **defines):
#        name = defines.keys()[0]
#        self.__dict__[name] = defines[name]
#        return defines[name]
#    def cut(self, **defines):
#        name = defines.keys()[0]
#        self.__dict__[name] += defines[name]
#        return ""
    def __getattr__(self, name):
        """ defend against program exit on members being not inited """
        if self.__dict__.has_key(name): return self.__dict__[name]
        warn("no such member: "+name); return None
    def dict(self):
        return self.__dict__
    def dict_sorted_keys(self):
        keys = self.__dict__.keys()
        keys.sort()
        return keys
    def parse(self, prototype):
        found = m(prototype, r"(?sx) ^(.*[^.]) \b(\w[\w.]*\w)\b (\s*\(.*) $ ")
        if found:
            self.prespec = found.group(1).lstrip()
            self.namespec = found.group(2)
            self.callspec = found.group(3).lstrip()
            self.name = self.namespec.strip()
            return self.name
        return None

# pass 1 of per-func strings ...............................................
# (a) cut prototype into prespec/namespec/callspec
# (b) cut out first line of comment as headline information
# (c) sanitize rest of comment block into proper docbook formatted .body
# 
# do this while copying strings from all.funcs to function_list
# and remember the original order in name_list

def markup_callspec(text):
    return (
        s(s(s(s(s(text,
                  r"(?sx) ^([^\(\)]*)\(", r"\1<parameters>(<paramdef>",1),
                r"(?sx) \)([^\(\)]*)$", r"</paramdef>)</parameters>\1",1),
              r"(?sx) , ", r"</paramdef>,<paramdef>"),
            r"(?sx) <paramdef>(\s+) ", r"\1<paramdef>"),
          r"(?sx) (\s+)</paramdef>", r"</paramdef>\1"))

def parse_all_functions(func_list): # list of FunctionDeclarations
    """ parse all FunctionDeclarations and create a list of Functions """
    list = []
    for func in all.funcs:
        function = Function()
        if not function.parse (func.prototype): continue

        list.append(function)

        function.body = markup_link_syntax(func.comment)
        if "\n" not in function.body: # single-line comment is the head
            function.head = function.body
            function.body = ""
        else: # cut comment in first-line and only keep the rest as descr body
            function.head = s(function.body,  r"(?sx) ^([^\n]*\n).*",r"\1",1)
            function.body = s(function.body,  r"(?sx)  ^[^\n]*\n",   r"",  1)
        #fi
        if m(function.head, r"(?sx) ^\s*$ "): # empty head line, autofill here
            function.head = s("("+func.file.name+")", r"[.][.][/]", r"")

        function.body = func_comment2section(function.body)
        function.src = func # keep a back reference

        # add extra docbook markups to callspec in $fn-hash
        function.callspec = markup_callspec (function.callspec)
    #od
    return list
function_list = parse_all_functions(all.funcs)

def examine_head_anchors(func_list):
    """ .into tells later steps which func-name is the leader of a man 
        page and that this func should add its descriptions over there. """
    for function in func_list:
        function.into = None
        function.seealso = None
        
        found = m(function.head, r"(?sx) ^ \s* <link>(\w[\w.]*\w)<\/link>")
        # if found and found.group(1) in func_list.names:
        if found and found.group(1):
            function.into = found.group(1)

        def set_seealso(f, value):
            f.seealso = value
            return value
        function.head = s(function.head, r"(.*)also:(.*)", lambda x
                          : set_seealso(function, x.group(2)) and x.group(1))
        if function.seealso and None:
            print "function[",function.name,"].seealso=",function.seealso
examine_head_anchors(function_list)

# =============================================================== HTML =====

def find_by_name(func_list, name):
    for func in func_list:
        if func.name == name:
            return func
    #od
    return None
#fu

class HtmlFunction:
    def __init__(self, func):
        self.src = func.src
        self.into = func.into
        self.name = func.name
        self.toc_line = paramdef2html(
            "  <td valign=\"top\"><code>"+func.prespec+"</code></td>\n"+
            "  <td valign=\"top\">&nbsp;&nbsp;</td>\n"+
            "  <td valign=\"top\"><a href=\"#"+func.name+"\">\n"+
            "                       <code>"+func.namespec+"</code>"+
            "  </a></td>\n"+
            "  <td valign=\"top\">&nbsp;&nbsp;</td>\n"+
            "  <td valign=\"top\">"+func.callspec+"</td>\n")
        self.synopsis = paramdef2html(
            "  <code>"+func.prespec+"</code>\n"+
            "  <br /><b><code>"+func.namespec+"</code></b>\n"+
            "   &nbsp; <code>"+func.callspec+"</code>\n")
        self.anchor = "<a name=\""+func.name+"\" />"
        self.section = "<para><em> &nbsp;"+func.head+"\n"+ \
                       "\n</em></para>"+section2html(func.body)
#class

class HtmlFunctionFamily(HtmlFunction):
    def __init__(page, func):
        HtmlFunction.__init__(page, func)
        page.toc_line_list = [ page.toc_line ]
        # page.html_txt     = page.synopsis
        page.synopsis_list = [ page.synopsis ]
        page.anchor_list   = [ page.anchor ]
        page.section_list  = [ this_function_link(page.section, func.name) ]

def ensure_name(text, name):
    adds = "<small><code>"+name+"</code></small> -"
    match = r"(?sx) .*>[^<>]*\b" + name + r"\b[^<>]*<.*"
    found = m(text, match)
    if found: return text
    found = m(text, r".*<p(ara)?>.*")
    if found: return s(text, r"(<p(ara)?>)", r"\1"+adds, 1)
    return adds+text

def combined_html_pages(func_list):
    """ and now add descriptions of non-leader entries (html-mode) """
    combined = {}
    
    for func in func_list: # assemble leader pages
        if func.into is not None: continue
        combined[func.name] =  HtmlFunctionFamily(func)

    for func in func_list: 
        if func.into is None: continue
        if func.into not in combined :
            warn(#......... (combine_html_pages) ..............
                "function '"+func.name+"'s into => '"+func.into+
                "\n: no such target function: "+func.into)
            combined[func.name] = HtmlFunctionFamily(func)
            continue
        #fi
        page = HtmlFunction(func)
        into = combined[func.into]
        into.toc_line_list.append( page.toc_line )
        into.anchor_list.append( page.anchor )
        into.synopsis_list.append( page.synopsis )
        into.section_list.append(
            s(ensure_name(this_function_link(section2html( func.body ),
                                             func.name), func.name),
              r"(?sx) (</?para>\s*) <br\s*\/>", r"\1"))
    return combined.values()
html_pages = combined_html_pages(function_list)

def html_resolve_links_on_page(text, list):
    """ link ref-names of a page with its endpoint on the same html page"""
    def html_link (name , extra):
        """ make <link>s to <href> of correct target or make it <code> """
        if find_by_name(list, name) is None:
            return "<code>"+name+extra+"</code>"
        else:
            return "<a href=\"#"+name+"\"><code>"+name+extra+"</code></a>"
    #fu html_link
    return s(s(text, r"(?sx) <link>(\w+)([^<>]*)<\/link> ",
               lambda x : html_link(x.group(1),x.group(2))),
             r"(?sx) \-\> ", r"<small>-&gt;</small>") # just sanitize..
#fu html_resolve_links

class HtmlPage:
    def __init__(self):
        self.toc = ""
        self.txt = ""
        self.package = o.package
        self.version = o.version
    def page_text(self):
        """ render .toc and .txt parts into proper <html> page """
        T = ""
        T += "<html><head>"
        T += "<title>"+self.package+"autodoc documentation </title>"
        T += "</head>\n<body>\n"
        T += "\n<h1>"+self.package+" <small><small><i>- "+self.version
        T += "</i></small></small></h1>"
        T += "\n<table border=0 cellspacing=2 cellpadding=0>"
        T +=  self.toc
        T += "\n</table>"
        T += "\n<h3>Documentation</h3>\n\n<dl>"
        T += html_resolve_links_on_page(self.txt, function_list)
        T += "\n</dl>\n</body></html>\n"
        return T
    def add_page_map(self, list):
        """ generate the index-block at the start of the onepage-html file """
        keys = list.keys()
        keys.sort()
        for name in keys:
            self.toc += "<tr valign=\"top\">\n"+ \
                        "\n</tr><tr valign=\"top\">\n".join(
                list[name].toc_line_list)+"</tr>\n"
            self.txt += "\n<dt>"+" ".join(list[name].anchor_list)
            self.txt += "\n"+"\n<br />".join(list[name].synopsis_list)+"<dt>"
            self.txt += "\n<dd>\n"+"\n".join(list[name].section_list)
            self.txt += ("\n<p align=\"right\">"+
                         "<small>("+list[name].src.file.name+")</small>"+
                         "</p></dd>")
    def add_page_list(self, functions):
        """ generate the index-block at the start of the onepage-html file """
        mapp = {}
        for func in functions:
            mapp[func.name] = func
        #od
        self.add_page_map(mapp)
#end

html = HtmlPage()
# html.add_function_dict(Fn)
# html.add_function_list(Fn.sort.values())
html.add_page_list(html_pages)

# and finally print the html-formatted output
try:
    F = open(o.libhtmlfile, "w")
except IOError, error:
    warn(# ............. open(o.libhtmlfile, "w") ..............
        "can not open html output file: "+o.libhtmlfile, error)
else:
    print >> F, html.page_text()
    F.close()
#fi

# ========================================================== DOCBOOK =====
# let's go for the pure docbook, a reference type master for all man pages

class RefPage:
    def __init__(self, func):
        """ initialize the fields needed for a man page entry - the fields are
           named after the docbook-markup that encloses (!!) the text we store
           the entries like X.refhint = "hello" will be printed therefore as
           <refhint>hello</refhint>. Names with underscores are only used as
           temporaries but they are memorized, perhaps for later usage. """
        self.refhint = "\n<!--========= "+func.name+" (3) ===========-->\n"
        self.refentry = None
        self.refentry_date = o.version.strip()        # //refentryinfo/date
        self.refentry_productname = o.package.strip() # //refentryinfo/prod*
        self.refentry_title = None                    # //refentryinfo/title
        self.refentryinfo = None                      # override
        self.manvolnum = "3"                         # //refmeta/manvolnum
        self.refentrytitle = None                    # //refmeta/refentrytitle
        self.refmeta = None                          # override
        self.refpurpose = None                       # //refnamediv/refpurpose
        self.refname = None                          # //refnamediv/refname
        self.refname_list = []
        self.refnamediv = None                       # override
        self.mainheader = func.src.file.mainheader
        self.includes = func.src.file.include
        self.funcsynopsisinfo = ""       # //funcsynopsisdiv/funcsynopsisinfo
        self.funcsynopsis = None         # //funcsynopsisdiv/funcsynopsis
        self.funcsynopsis_list = []
        self.description = None
        self.description_list = []
        # optional sections
        self.authors_list = []           # //sect1[authors]/listitem
        self.authors = None              # override
        self.copyright = None
        self.copyright_list = []
        self.seealso = None
        self.seealso_list = []
        if  func.seealso:
            self.seealso_list.append(func.seealso)
        # func.func references
        self.func = func
        self.file_authors = None
        if  func.src.file.authors:
            self.file_authors = func.src.file.authors
        self.file_copyright = None
        if  func.src.file.copyright:
            self.file_copyright = func.src.file.copyright
    #fu
    def refentryinfo_text(page):
        """ the manvol formatter wants to render a footer line and header line
            on each manpage and such info is set in <refentryinfo> """
        if page.refentryinfo:
            return page.refentryinfo
        if page.refentry_date and \
           page.refentry_productname and \
           page.refentry_title: return (
            "\n <date>"+page.refentry_date+"</date>"+ 
            "\n <productname>"+page.refentry_productname+"</productname>"+
            "\n <title>"+page.refentry_title+"</title>")
        if page.refentry_date and \
           page.refentry_productname: return (
            "\n <date>"+page.refentry_date+"</date>"+ 
            "\n <productname>"+page.refentry_productname+"</productname>")
        return ""
    def refmeta_text(page):
        """ the manvol formatter needs to know the filename of the manpage to
            be made up and these parts are set in <refmeta> actually """
        if page.refmeta:
            return page.refmeta
        if page.manvolnum and page.refentrytitle:
            return (
                "\n <refentrytitle>"+page.refentrytitle+"</refentrytitle>"+
                "\n <manvolnum>"+page.manvolnum+"</manvolnum>")
        if page.manvolnum and page.func.name:
            return (
                "\n <refentrytitle>"+page.func.name+"</refentrytitle>"+
                "\n <manvolnum>"+page.manvolnum+"</manvolnum>")
        return ""
    def refnamediv_text(page):
        """ the manvol formatter prints a header line with a <refpurpose> line
            and <refname>'d functions that are described later. For each of
            the <refname>s listed here, a mangpage is generated, and for each
            of the <refname>!=<refentrytitle> then a symlink is created """
        if page.refnamediv:
            return page.refnamediv
        if page.refpurpose and page.refname:
            return ("\n <refname>"+page.refname+'</refname>'+
                    "\n <refpurpose>"+page.refpurpose+" </refpurpose>")
        if page.refpurpose and page.refname_list:
            T = ""
            for refname in page.refname_list:
                T += "\n <refname>"+refname+'</refname>'
            T += "\n <refpurpose>"+page.refpurpose+" </refpurpose>"
            return T
        return ""
    def funcsynopsisdiv_text(page):
        """ refsynopsisdiv shall be between the manvol mangemaent information
            and the reference page description blocks """
        T=""
        if page.funcsynopsis:
            T += "\n<funcsynopsis>"
            if page.funcsynopsisinfo:
                T += "\n<funcsynopsisinfo>"+    page.funcsynopsisinfo + \
                     "\n</funcsynopsisinfo>\n"
            T += page.funcsynopsis + \
                 "\n</funcsynopsis>\n"
        if page.funcsynopsis_list:
            T += "\n<funcsynopsis>"
            if page.funcsynopsisinfo:
                T += "\n<funcsynopsisinfo>"+    page.funcsynopsisinfo + \
                     "\n</funcsynopsisinfo>\n"
            for funcsynopsis in page.funcsynopsis_list:
                T += funcsynopsis
            T += "\n</funcsynopsis>\n"
        #fi
        return T
    def description_text(page):
        """ the description section on a manpage is the main part. Here
            it is generated from the per-function comment area. """
        if page.description:
            return page.description
        if page.description_list:
            T = ""
            for description in page.description_list:
                if not description: continue
                T += description
            if T: return T
        return ""
    def authors_text(page):
        """ part of the footer sections on a manpage and a description of
            original authors. We prever an itimizedlist to let the manvol
            show a nice vertical aligment of authors of this ref item """
        if page.authors:
            return page.authors
        if page.authors_list:
            T = "<itemizedlist>"
            previous=""
            for authors in page.authors_list:
                if not authors: continue
                if previous == authors: continue
                T += "\n <listitem><para>"+authors+"</para></listitem>"
                previous = authors
            T += "</itemizedlist>"
            return T
        if page.authors:
            return page.authors
        return ""
    def copyright_text(page):
        """ the copyright section is almost last on a manpage and purely
            optional. We list the part of the per-file copyright info """
        if page.copyright:
            return page.copyright
        """ we only return the first valid instead of merging them """
        if page.copyright_list:
            T = ""
            for copyright in page.copyright_list:
                if not copyright: continue
                return copyright # !!!
        return ""
    def seealso_text(page):
        """ the last section on a manpage is called 'SEE ALSO' usally and
            contains a comma-separated list of references. Some manpage
            viewers can parse these and convert them into hyperlinks """
        if page.seealso:
            return page.seealso
        if page.seealso_list:
            T = ""
            for seealso in page.seealso_list:
                if not seealso: continue
                if T: T += ", "
                T += seealso
            if T: return T
        return ""
    def refentry_text(page, id=None):
        """ combine fields into a proper docbook refentry """
        if id is None:
            id = page.refentry
        if id:
            T = '<refentry id="'+id+'">'
        else:
            T = '<refentry>' # this is an error
           
        if page.refentryinfo_text():
            T += "\n<refentryinfo>"+       page.refentryinfo_text()+ \
                 "\n</refentryinfo>\n"
        if page.refmeta_text():
            T += "\n<refmeta>"+            page.refmeta_text() + \
                 "\n</refmeta>\n" 
        if page.refnamediv_text():
            T += "\n<refnamediv>"+         page.refnamediv_text() + \
                 "\n</refnamediv>\n"
        if page.funcsynopsisdiv_text():     
            T += "\n<refsynopsisdiv>\n"+   page.funcsynopsisdiv_text()+ \
                 "\n</refsynopsisdiv>\n"
        if page.description_text():
            T += "\n<refsect1><title>Description</title> " + \
                 page.description_text() + "\n</refsect1>"
        if page.authors_text():
            T += "\n<refsect1><title>Author</title> " + \
                 page.authors_text() + "\n</refsect1>"
        if page.copyright_text():
            T += "\n<refsect1><title>Copyright</title> " + \
                 page.copyright_text() + "\n</refsect1>\n"
        if page.seealso_text():
            T += "\n<refsect1><title>See Also</title><para> " + \
                 page.seealso_text() + "\n</para></refsect1>\n"

        T +=  "\n</refentry>\n"
        return T
    #fu
#end

# -----------------------------------------------------------------------
class FunctionRefPage(RefPage):
    def reinit(page):
        """ here we parse the input function for its values """
        if page.func.into:
            page.refhint = "\n              <!-- see "+page.func.into+" -->\n"
        #fi
        page.refentry = page.func.name               # //refentry@id
        page.refentry_title = page.func.name.strip() # //refentryinfo/title
        page.refentrytitle = page.func.name          # //refmeta/refentrytitle
        if page.includes:
            page.funcsynopsisinfo += "\n"+page.includes
        if not page.funcsynopsisinfo:
            page.funcsynopsisinfo="\n"+' #include &lt;'+page.mainheader+'&gt;'
        page.refpurpose = page.func.head
        page.refname = page.func.name

        def funcsynopsis_of(func):
            return (
                "\n <funcprototype>\n <funcdef>"+func.prespec+
                " <function>"+func.name+"</function></funcdef>"+
                "\n"+s(s(s(func.callspec,
                           r"<parameters>\s*\(",r" "),
                         r"\)\s*</parameters>",r" "),
                       r"</paramdef>\s*,\s*",r"</paramdef>\n ")+
                " </funcprototype>")
        page.funcsynopsis = funcsynopsis_of(page.func)

        page.description = (
            html2docbook(this_function_link(page.func.body, page.func.name)))

        if page.file_authors:
            def add_authors(page, ename, email):
                page.authors_list.append( ename+' '+email )
                return ename+email
            s(page.file_authors,
              r"(?sx) \s* ([^<>]*) (<email>[^<>]*</email>) ", lambda x
              : add_authors(page, x.group(1), x.group(2)))
        #fi

        if page.file_copyright:
            page.copyright = "<screen>\n"+page.file_copyright+"</screen>\n"
        #fi
        return page
    def __init__(page,func):
        RefPage.__init__(page, func)
        FunctionRefPage.reinit(page)
    
def refpage_list_from_function_list(funclist):
    list = []
    mapp = {}
    for func in funclist:
        mapp[func.name] = func
    #od
    for func in funclist:
        page = FunctionRefPage(func)
        if func.into and func.into not in mapp:
            warn (# ............ (refpage_list_from_function_list) .......
                "page '"+page.func.name+"' has no target => "+
                "'"+page.func.into+"'"
                "\n: going to reset .into of Function '"+page.func.name+"'")
            func.into = None
        #fi
        list.append(FunctionRefPage(func))
    return list
#fu
    
# ordered list of pages
refpage_list = refpage_list_from_function_list(function_list)

class FunctionFamilyRefPage(RefPage):
    def __init__(self, page):
        RefPage.__init__(self, page.func)
        self.seealso_list = [] # reset
        self.refhint_list = []
    def refhint_list_text(page):
        T = ""
        for hint in page.refhint_list:
            T += hint
        return T
    def refentry_text(page):
        return page.refhint_list_text() + "\n" + \
               RefPage.refentry_text(page)
    pass

def docbook_pages_recombine(pagelist):
    """ take a list of RefPages and create a new list where sections are
        recombined in a way that their description is listed on the same
        page and the manvol formatter creates symlinks to the combined
        function description page - use the attribute 'into' to guide the
        processing here as each of these will be removed from the output
        list. If no into-pages are there then the returned list should
        render to the very same output text like the input list would do """

    list = []
    combined = {}
    for orig in pagelist:
        if orig.func.into: continue
        page = FunctionFamilyRefPage(orig)
        combined[orig.func.name] = page ; list.append(page)

        page.refentry = orig.refentry              # //refentry@id
        page.refentry_title = orig.refentrytitle   # //refentryinfo/title
        page.refentrytitle = orig.refentrytitle    # //refmeta/refentrytitle
        page.includes = orig.includes
        page.funcsynopsisinfo = orig.funcsynopsisinfo
        page.refpurpose = orig.refpurpose
        if orig.refhint:
            page.refhint_list.append( orig.refhint )
        if orig.refname:
            page.refname_list.append( orig.refname )
        elif orig.refname_list:
            page.refname_list.extend( orig.refname_list )
        if orig.funcsynopsis:
            page.funcsynopsis_list.append( orig.funcsynopsis )
        elif orig.refname_list:
            page.funcsynopsis_list.extend( orig.funcsynopsis_list )
        if orig.description:
            page.description_list.append( orig.description )
        elif orig.refname_list:
            page.description_list.extend( orig.description_list )
        if orig.seealso:
            page.seealso_list.append( orig.seealso )
        elif orig.seealso_list:
            page.seealso_list.extend( orig.seealso_list )
        if orig.authors:
            page.authors_list.append( orig.authors )
        elif orig.authors_list:
            page.authors_list.extend( orig.authors_list )
        if orig.copyright:
            page.copyright_list.append( orig.copyright )
        elif orig.refname_list:
            page.copyright_list.extend( orig.copyright_list )
    #od
    for orig in pagelist:
        if not orig.func.into: continue
        if orig.func.into not in combined:
            warn("page for '"+orig.func.name+
                 "' has no target => '"+orig.func.into+"'")
            page = FunctionFamilyRefPage(orig)
        else:
            page = combined[orig.func.into]

        if orig.refname:
            page.refname_list.append( orig.refname )
        elif orig.refname_list:
            page.refname_list.extend( orig.refname_list )
        if orig.funcsynopsis:
            page.funcsynopsis_list.append( orig.funcsynopsis )
        elif orig.refname_list:
            page.funcsynopsis_list.extend( orig.funcsynopsis_list )
        if orig.description:
            page.description_list.append( orig.description )
        elif orig.refname_list:
            page.description_list.extend( orig.description_list )
        if orig.seealso:
            page.seealso_list.append( orig.seealso )
        elif orig.seealso_list:
            page.seealso_list.extend( orig.seealso_list )
        if orig.authors:
            page.authors_list.append( orig.authors )
        elif orig.authors_list:
            page.authors_list.extend( orig.authors_list )
        if orig.copyright:
            page.copyright_list.append( orig.copyright )
        elif orig.refname_list:
            page.copyright_list.extend( orig.copyright_list )
    #od
    return list
#fu

combined_pages = docbook_pages_recombine(pagelist = refpage_list)

# -----------------------------------------------------------------------

class HeaderRefPage(RefPage):
    pass

def docbook_refpages_perheader(page_list): # headerlist
    " creating the per-header manpage - a combination of function man pages "
    header = {}
    for page in page_list:
        assert not page.func.into
        file = page.func.src.file.mainheader # short for the mainheader index
        if file not in header:
            header[file] = HeaderRefPage(page.func)
            header[file].id = s(file, r"[^\w\.]","-")
            header[file].refentry = header[file].id
            header[file].refentryinfo = None
            header[file].refentry_date = page.refentry_date
            header[file].refentry_productname = (
                "the library "+page.refentry_productname)
            header[file].manvolnum = page.manvolnum
            header[file].refentrytitle = file
            header[file].funcsynopsis = ""
        if 1: # or += or if not header[file].refnamediv:
            header[file].refpurpose = " library "
            header[file].refname = header[file].id

        if not header[file].funcsynopsisinfo and page.funcsynopsisinfo:
            header[file].funcsynopsisinfo  = page.funcsynopsisinfo
        if page.funcsynopsis:
            header[file].funcsynopsis  += "\n"+page.funcsynopsis
        if not header[file].copyright and page.copyright:
            header[file].copyright = page.copyright
        if not header[file].authors and page.authors:
            header[file].authors = page.authors
        if not header[file].authors and page.authors_list:
            header[file].authors_list = page.authors_list
        if not header[file].description:
            found = m(commands.getoutput("cat "+o.package+".spec"),
                      r"(?s)\%description\b([^\%]*)\%")
            if found:
                header[file].description = found.group(1)
            elif not header[file].description:
                header[file].description = "<para>" + (
                    page.refentry_productname + " library") + "</para>";
            #fi
        #fi
    #od
    return header#list
#fu

def leaders(pagelist):
    list = []
    for page in pagelist:
        if page.func.into : continue
        list.append(page)
    return list
header_refpages = docbook_refpages_perheader(leaders(refpage_list))

# -----------------------------------------------------------------------
# printing the docbook file is a two-phase process - we spit out the
# leader pages first - later we add more pages with _refstart pointing
# to the leader page, so that xmlto will add the functions there. Only the
# leader page contains some extra info needed for troff page processing.

doctype = '<!DOCTYPE reference PUBLIC "-//OASIS//DTD DocBook XML V4.1.2//EN"'
doctype += "\n     "
doctype += '"http://www.oasis-open.org/docbook/xml/4.1.2/docbookx.dtd">'+"\n"

try:
    F = open(o.docbookfile,"w")
except IOError, error:
    warn("can not open docbook output file: "+o.docbookfile, error)
else:
    print >> F, doctype, '<reference><title>Manual Pages</title>'

    for page in combined_pages:
        print >> F, page.refentry_text()
    #od

    for page in header_refpages.values():
        if not page.refentry: continue
        print >> F, "\n<!-- _______ "+page.id+" _______ -->",
        print >> F, page.refentry_text()
    #od

    print >> F, "\n",'</reference>',"\n"
    F.close()
#fi

# _____________________________________________________________________
try:
    F = open( o.dumpdocfile, "w")
except IOError, error:
    warn ("can not open"+o.dumpdocfile,error)
else:
    for func in function_list:
        name = func.name
        print >> F, "<fn id=\""+name+"\">"+"<!-- FOR \""+name+"\" -->\n"
        for H in sorted_keys(func.dict()):
            print >> F, "<"+H+" name=\""+name+"\">",
            print >> F, str(func.dict()[H]),
            print >> F, "</"+H+">"
        #od
        print >> F, "</fn><!-- END \""+name+"\" -->\n\n";
    #od
    F.close();
#fi

if errors: sys.exit(errors)
