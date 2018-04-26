#! /bin/sh
# this is the sh/sed variant of the mksite script. It is largely
# derived from snippets that I was using to finish doc pages for 
# website publishing. For the mksite project the functionaliy has
# been expanded of course. Still this one does only use simple unix
# commands like sed, date, and test. And it still works. :-)=)
#                                               http://zziplib.sf.net/mksite/
#   THE MKSITE.SH (ZLIB/LIBPNG) LICENSE
#       Copyright (c) 2004 Guido U. Draheim <guidod@gmx.de>
#   This software is provided 'as-is', without any express or implied warranty
#       In no event will the authors be held liable for any damages arising
#       from the use of this software.
#   Permission is granted to anyone to use this software for any purpose, 
#       including commercial applications, and to alter it and redistribute it 
#       freely, subject to the following restrictions:
#    1. The origin of this software must not be misrepresented; you must not
#       claim that you wrote the original software. If you use this software 
#       in a product, an acknowledgment in the product documentation would be 
#       appreciated but is not required.
#    2. Altered source versions must be plainly marked as such, and must not
#       be misrepresented as being the original software.
#    3. This notice may not be removed or altered from any source distribution.
# $Id: mksite.sh,v 1.5 2006-09-22 00:33:22 guidod Exp $

# Zsh is not Bourne compatible without the following: (seen in autobook)
if test -n "$ZSH_VERSION"; then
  emulate sh
  NULLCMD=:
fi

# initialize some defaults
test ".$SITEFILE" = "." && test -f "site.htm"  && SITEFILE="site.htm"
test ".$SITEFILE" = "." && test -f "site.html" && SITEFILE="site.html"
test ".$SITEFILE" = "." && SITEFILE="site.htm"
MK="-mksite"     # note the "-" at the start
SED="sed"
CAT="cat"        # "sed -e n" would be okay too
GREP="grep"
DATE_NOW="date"  # should be available on all posix systems
DATE_R="date -r" # gnu date has it / solaris date not
STAT_R="stat"    # gnu linux
LS_L="ls -l"     # linux uses one less char than solaris

DATA="~~"     # extension for meta data files
HEAD="~head~" # extension for head sed script
BODY="~body~" # extension for body sed script
FOOT="~foot~" # append to body text (non sed)

NULL="/dev/null"                             # to divert stdout/stderr
CATNULL="$CAT $NULL"                         # to create 0-byte files
SED_LONGSCRIPT="$SED -f"

Q='q class='
QX='/q'
LOWER="abcdefghijklmnopqrstuvwxyz"
UPPER="ABCDEFGHIJKLMNOPQRSTUVWXYZ"
az="$LOWER"                                  # some old sed tools can not
AZ="$UPPER"                                  # use char-ranges in the 
NN="0123456789"                              # match expressions so that
AA="_$NN$AZ$az"                              # we use their unrolled
AX="$AA.+-"                                  # definition here
AP="|"                                       # (pipe symbol in char-range)
AK="["                                       # (open range in char-range)

LANG="C" ; LANGUAGE="C" ; LC_COLLATE="C"     # these are needed for proper
export LANG LANGUAGE LC_COLLATE              # lowercasing as some collate
                                             # treat A-Z to include a-z

HTMLTAGS=" a p h1 h2 h3 h4 h5 h6 dl dd dt ul ol li pre code table tr td th"
HTMLTAGS=" $HTMLTAGS b u i s q em strong strike cite big small sup sub tt"
HTMLTAGS=" $HTMLTAGS thead tbody center hr br nobr wbr"
HTMLTAGS=" $HTMLTAGS span div img adress blockquote"
HTMLTAGS2=" html head body title meta http-equiv style link"

# ==========================================================================
if "${SHELL-/bin/sh}" -c 'foo () { exit 0; }; foo' 2>$NULL ; then : ; else
echo "!! sorry, this shell '$SHELL' does not support shell functions" ; exit 1
fi

error ()
{
    echo "ERROR:" "$@" 1>&2
}

warn ()
{
    echo "WARN:" "$@" 1>&2
}

note ()
{
    echo "NOTE:" "$@" 1>&2
}

hint=":"

init ()
{
    if test -d DEBUG
	then hint="note"
    fi
    if test "$SED" = "sed" ; then
	if gsed --version 2>$NULL | $GREP "GNU sed" >$NULL ; then
	    SED="gsed"
	    $hint "using 'gsed' as SED"
	fi
    fi
    if $SED --version 2>$NULL | $GREP "GNU sed" >$NULL ; then
	az="a-z"                                # but if we have GNU sed
	AZ="A-Z"                                # then we assume there are
	NN="0-9"                                # char-ranges available
	AA="_$NN$AZ$az"                         # that makes the resulting
	AX="$AA.+-"                             # script more readable
	$hint "found GNU sed - good"
    elif uname -s | $GREP HP-UX >$NULL ; then
	SED_LONGSCRIPT="sed_longscript"         # due to 100 sed lines limit
	$hint "weird sed - hpux sed has a limit of 100 lines" \
	    "- using sed_longscript mode"
    fi
    if echo "TEST" | sed -e "s%[:[]*TEST%OK%" | grep OK 2>&1 > $NULL
	 then :
    elif echo "TEST" | sed -e "s%[:\\[]*TEST%OK%" | grep OK 2>&1 > $NULL
	then  AK="\\[" ; $hint "AK=\\["
    else AK="" ; warn "buggy sed - disabled [ in char-ranges / fileref-tests"
    fi
    if echo "TEST" | sed -e "s%[:|]*TEST%OK%" | grep OK 2>&1 > $NULL
	 then :
    elif echo "TEST" | sed -e "s%[:\\|]*TEST%OK%" | grep OK 2>&1 > $NULL
	then  AP="\\[" ; $hint "AP=\\|"
    else AP="" ; warn "buggy sed - disabled | in char-ranges / fileref-tests"
    fi	
}

init "NOW!!!"

sed_debug ()
{
    $note "sed" "$@" >&2
    sed "$@"
}

# ==========================================================================
# reading options from the command line                            GETOPT
opt_variables="files"
opt_fileseparator="?"
opt_files=""
opt_main_file=""
opt_formatter="$0"
opt=""
for arg in "$@"        # this variant should allow to embed spaces in $arg
do if test ".$opt" != "." ; then
      eval "export opt_$opt='$arg'"
      opt=""
   else
      case "$arg" in
      -*=*) 
         opt=`echo "$arg" | $SED -e "s/-*\\([$AA][$AA-]*\\).*/\\1/" -e y/-/_/`
         if test ".$opt" = "." ; then
            error "invalid option $arg"
         else
            arg=`echo "$arg" | $SED -e "s/^[^=]*=//"`
            eval "export opt_$opt='$arg'"
	    opt_variables="$opt_variables $opt"
         fi
         opt="" ;;
      -*?-*) : an option with an argument --main-file=x or --main-file x
         opt=`echo "$arg" | $SED -e "s/-*\\([$AA][$AA-]*\\).*/\\1/" -e y/-/_/`
         if test ".$opt" = "." ; then
            error "invalid option $arg"
            opt=""
         else :
            # keep the option for next round
         fi ;;
      -*)   : a simple option --filelist or --debug or --verbose
         opt=`echo "$arg" | $SED -e "s/^-*\\([$AA][$AA-]*\\).*/\\1/" -e y/-/_/`
         if test ".$opt" = "." ; then
            error "invalid option $arg"
         else
            arg=`echo "$arg" | $SED -e "s/^[^=]*=//"`
            eval "export opt_$opt=' '"
         fi
         opt="" ;;
      *) $hint "<$arg>"
	 if test ".$opt_main_file" = "." ; then opt_main_file="$arg" ; else
         test ".$opt_files" != "." && opt_files="$opt_files$opt_fileseparator"
         opt_files="$opt_files$arg" ; fi
         opt="" ;;
      esac
   fi
done ; if test ".$opt" != "." ; then
      eval "export opt_$opt='$arg'"
      opt=""
fi
### env | grep ^opt

test ".$opt_main_file" != "." && test -f "$opt_main_file" && \
SITEFILE="$opt_main_file"
test ".$opt_site_file" != "." && test -f "$opt_site_file" && \
SITEFILE="$opt_site_file"
test "$opt_debug" && \
hint="note"

if test ".$opt_help" != "." ; then
    F="$SITEFILE"
    echo "$0 [sitefile]";
    echo "  default sitefile = $F";
    echo "options:";
    echo " --filelist : show list of target files as ectracted from $F"
    echo " --src-dir xx : if source files are not where mksite is executed"
    echo " --tmp-dir xx : use temp instead of local directory"
    echo " --tmp : use automatic temp directory in ${TEMP-/tmp}/mksite.*"
    exit;
    echo " internal:"
    echo "--fileseparator=x : for building the internal filelist (default '?')"
    echo "--files xx : for list of additional files to be processed"
    echo "--main-file xx : for the main sitefile to take file list from"
fi

if test ".$SITEFILE" = "." ; then
   error "no SITEFILE found (default would be 'site.htm')"
   exit 1
else
   $hint "sitefile:" `ls -s $SITEFILE`
fi

tmp="." ; if test ".$opt_tmp_dir" != "." ; then tmp="$opt_tmp_dir" ; fi
if test ".$opt_tmp_dir" = "." && test ".$opt_tmp" != "." ; then
tmp="${TEMP-/tmp}/mksite.$$" ; fi

# we use external files to store mappings - kind of relational tables
MK_TAGS="$tmp/$MK.tags.tmp.sed"
MK_VARS="$tmp/$MK.vars.tmp.sed"
MK_SPAN="$tmp/$MK.span.tmp.sed"
MK_META="$tmp/$MK.meta.tmp.htm"
MK_METT="$tmp/$MK.mett.tmp.htm"
MK_TEST="$tmp/$MK.test.tmp.htm"
MK_FAST="$tmp/$MK.fast.tmp.sed"
MK_GETS="$tmp/$MK.gets.tmp.sed"
MK_PUTS="$tmp/$MK.puts.tmp.sed"
MK_SITE="$tmp/$MK.site.tmp.sed"
MK_SECT1="$tmp/$MK.sect1.tmp.sed"
MK_SECT2="$tmp/$MK.sect2.tmp.sed"
MK_SECT3="$tmp/$MK.sect3.tmp.sed"
MK_STYLE="$tmp/$MK.style.tmp.sed"
MK_DATA="$tmp/$MK.$DATA.tmp.htm"

# ========================================================================
# ========================================================================
# ========================================================================
#                                                             MAGIC VARS
#                                                            IN $SITEFILE
printerfriendly=""
sectionlayout="list"
sitemaplayout="list"
attribvars=" "         # <x ref="${varname:=default}">
updatevars=" "         # <!--$varname:=-->default
expandvars=" "         # <!--$varname-->
commentvars=" "        # $updatevars && $expandsvars
sectiontab=" "         # highlight ^<td class=...>...href="$section"
currenttab=" "         # highlight ^<br>..<a href="$topic">
headsection="no"
tailsection="no"
sectioninfo="no"       # using <h2> title <h2> = info text
emailfooter="no"

if $GREP "<!--multi-->"               $SITEFILE >$NULL ; then
echo \
"WARNING: do not use <!--multi-->, change to <!--mksite:multi--> " "$SITEFILE"
echo \
"warning: or <!--mksite:multisectionlayout--> <!--mksite:multisitemaplayout-->"
sectionlayout="multi"
sitemaplayout="multi"
fi
if $GREP "<!--mksite:multi-->"               $SITEFILE >$NULL ; then
sectionlayout="multi"
sitemaplayout="multi"
fi
if $GREP "<!--mksite:multilayout-->"         $SITEFILE >$NULL ; then
sectionlayout="multi"
sitemaplayout="multi"
fi

mksite_magic_option ()
{
    # $1 is word/option to check for
    INP="$2" ; test ".$INP" = "." && INP="$SITEFILE"
    $SED \
      -e "s/\\(<!--mksite:\\)\\($1\\)-->/\\1\\2: -->/g" \
      -e "s/\\(<!--mksite:\\)\\([$AA][$AA]*\\)\\($1\\)-->/\\1\\3:\\2-->/g" \
      -e "/<!--mksite:$1:/!d" \
      -e "s/.*<!--mksite:$1:\\([^<>]*\\)-->.*/\\1/" \
      -e "s/.*<!--mksite:$1:\\([^-]*\\)-->.*/\\1/" \
      -e "/<!--mksite:$1:/d" -e q $INP # $++
}

x=`mksite_magic_option sectionlayout` ; case "$x" in
       "list"|"multi") sectionlayout="$x" ;; esac
x=`mksite_magic_option sitemaplayout` ; case "$x" in
       "list"|"multi") sitemaplayout="$x" ;; esac
x=`mksite_magic_option attribvars` ; case "$x" in
      " "|"no"|"warn") attribvars="$x" ;; esac
x=`mksite_magic_option updatevars` ; case "$x" in
      " "|"no"|"warn") updatevars="$x" ;; esac
x=`mksite_magic_option expandvars` ; case "$x" in
      " "|"no"|"warn") expandvars="$x" ;; esac
x=`mksite_magic_option commentvars` ; case "$x" in
      " "|"no"|"warn") commentvars="$x" ;; esac
x=`mksite_magic_option printerfriendly` ; case "$x" in
        " "|".*"|"-*") printerfriendly="$x" ;; esac
x=`mksite_magic_option sectiontab` ; case "$x" in
      " "|"no"|"warn") sectiontab="$x" ;; esac
x=`mksite_magic_option currenttab` ; case "$x" in
      " "|"no"|"warn") currenttab="$x" ;; esac
x=`mksite_magic_option sectioninfo` ; case "$x" in
      " "|"no"|"[=:-]") sectioninfo="$x" ;; esac
x=`mksite_magic_option emailfooter` 
   test ".$x" != "." && emailfooter="$x"

test ".$opt_print" != "." && printerfriendly="$opt_print"
test ".$commentvars"  = ".no" && updatevars="no"   # duplicated into
test ".$commentvars"  = ".no" && expandvars="no"   # info2vars_sed ()


$hint "'$sectionlayout'sectionlayout '$sitemaplayout'sitemaplayout"
$hint "'$attribvars'attribvars '$updatevars'updatevars"
$hint "'$expandvars'expandvars '$commentvars'commentvars "
$hint "'$currenttab'currenttab '$sectiontab'sectiontab"
$hint "'$headsection'headsection '$tailsection'tailsection"

if ($STAT_R "$SITEFILE" >$NULL) 2>$NULL ; then : ; else STAT_R=":" ; fi
# ==========================================================================
# init a few global variables
#                                                                  0. INIT

mkpathdir () {
    if test -n "$1"  && test ! -d "$1" ; then
       echo "!! mkdir '$1'" ; mkdir "$1"
       test ! -d "$1" || mkdir -p "$1"
    fi
}

mkpathfile () {
    dirname=`echo "$1" | $SED -e "s:/[^/][^/]*\$::"`
    if test ".$1" != ".$dirname" && test ".$dirname" != "." ;
    then mkpathdir "$dirname"; fi
}

mknewfile () {
    mkpathfile "$1"
    $CATNULL > "$1" 
}

tmp_dir_was_created="no"
if test ! -d "$tmp" ; then mkpathdir "$tmp" ; tmp_dir_was_created="yes" ; fi

# $MK_TAGS - originally, we would use a lambda execution on each 
# uppercased html tag to replace <P> with <p class="P">. Here we just
# walk over all the known html tags and make an sed script that does
# the very same conversion. There would be a chance to convert a single
# tag via "h;y;x" or something we do want to convert all the tags on
# a single line of course.
mknewfile "$MK_TAGS"
for M in `echo $HTMLTAGS`
do P=`echo "$M" | $SED -e "y/$LOWER/$UPPER/"`
  echo "s|<$P>|<$M class=\"$P\">|g"         >> "$MK_TAGS"
  echo "s|<$P |<$M class=\"$P\" |g"         >> "$MK_TAGS"
  echo "s|</$P>|</$M>|g"                    >> "$MK_TAGS"
done
  echo "s|<>|\\&nbsp\\;|g"                  >> "$MK_TAGS"
  echo "s|<->|<WBR />|g"                    >> "$MK_TAGS"
  echo "s|<c>|<code>|g"                     >> "$MK_TAGS"
  echo "s|</c>|</code>|g"                   >> "$MK_TAGS"
  echo "s|<section>||g"                     >> "$MK_TAGS"
  echo "s|</section>||g"                    >> "$MK_TAGS"
  echo "s|<\\(a [^<>]*\\) />|<\\1></a>|g"   >> "$MK_TAGS"
  _ulink_="<a href=\"\\1\" remap=\"url\">\\1</a>"
  echo "s|<a>\\([$az$AZ][$az$AZ]*://[^<>]*\\)</a>|$_ulink_|g" >> "$MK_TAGS"
# also make sure that some non-html entries are cleaned away that
# we are generally using to inject meta information. We want to see
# that meta ino in the *.htm browser view during editing but they
# shall not get present in the final html page for publishing.
DC_VARS="contributor date source language coverage identifier"
DC_VARS="$DC_VARS rights relation creator subject description"
DC_VARS="$DC_VARS publisher DCMIType"
_EQUIVS="refresh expires content-type cache-control"
_EQUIVS="$_EQUIVS redirect charset" # mapped to refresh / content-type
_EQUIVS="$_EQUIVS content-language content-script-type content-style-type"
for P in $DC_VARS $_EQUIVS ; do # dublin core embedded
   echo "s|<$P>[^<>]*</$P>||g"              >> "$MK_TAGS"
done
   test ".$opt_keepsect" = "." && \
   echo "s|<a sect=\"[$AZ$NN]\"|<a|g"       >> "$MK_TAGS"
   echo "s|<!--[$AX]*[?]-->||g"             >> "$MK_TAGS"
   echo "s|<!--\\\$[$AX]*[?]:-->||g"        >> "$MK_TAGS"
   echo "s|<!--\\\$[$AX]*:[?=]-->||g"       >> "$MK_TAGS"
   echo "s|\\(<[^<>]*\\)\\\${[$AX]*:[?=]\\([^<{}>]*\\)}\\([^<>]*>\\)|\\1\\2\\3|g"        >>$MK_TAGS

# see overview at www.metatab.de - http-equivs are
# <refresh>5; url=target</reresh>   or <redirect>target</redirect>
# <content-type>text/html; charset=koi8-r</content-type> iso-8859-1/UTF-8
# <content-language>de</content-language>             <charset>UTF-8</charset>
# <content-script-type>text/javascript</content-script-type> /jscript/vbscript
# <content-style-type>text/css</content-style-type>
# <cache-control>no-cache</cache-control>

trimm ()
{
    echo "$1" | $SED -e "s:^ *::" -e "s: *\$::";
}
trimmm ()
{
    echo "$1" | $SED -e "s:^ *::" -e "s: *\$::" -e "s:[	 ][	 ]*: :g";
}

timezone ()
{
    # +%z is an extension while +%Z is supposed to be posix
    _timezone=`$DATE_NOW +%z`
    case "$_timezone" in
	*+*) echo "$_timezone" ;;
	*-*) echo "$_timezone" ;;
	*) $DATE_NOW +%Z
    esac
}
timetoday () 
{
    $DATE_NOW +%Y-%m-%d
}
timetodays ()
{
    $DATE_NOW +%Y-%m%d
}
    
# ======================================================================
#                                                                FUNCS

sed_longscript ()
{
    # hpux sed has a limit of 100 entries per sed script !
    $SED             -e "100q" "$1" > "$1~1~"
    $SED -e "1,100d" -e "200q" "$1" > "$1~2~"
    $SED -e "1,200d" -e "300q" "$1" > "$1~3~"
    $SED -e "1,300d" -e "400q" "$1" > "$1~4~"
    $SED -e "1,400d" -e "500q" "$1" > "$1~5~"
    $SED -e "1,500d" -e "600q" "$1" > "$1~6~"
    $SED -e "1,600d" -e "700q" "$1" > "$1~7~"
    $SED -e "1,700d" -e "800q" "$1" > "$1~8~"
    $SED -e "1,800d" -e "900q" "$1" > "$1~9~"
    $SED -f "$1~1~"  -f "$1~2~" -f "$1~3~" -f "$1~4~" -f "$1~5~" \
         -f "$1~6~"  -f "$1~7~" -f "$1~8~" -f "$1~9~" "$2"
}

sed_escape_key () 
{
    $SED -e "s|\\.|\\\\&|g" -e "s|\\[|\\\\&|g" -e "s|\\]|\\\\&|g" "$@"
}

sed_slash_key ()      # helper to escape chars special in /anchor/ regex
{                     # currently escaping "/" "[" "]" "."
    echo "$1" | sed_escape_key -e "s|/|\\\\&|g"
}
sed_piped_key ()      # helper to escape chars special in s|anchor|| regex
{                     # currently escaping "|" "[" "]" "."
    echo "$1" | sed_escape_key -e "s/|/\\\\&/g"
}

back_path ()          # helper to get the series of "../" for a given path
{
    echo "$1" | $SED -e "/\\//!d" -e "s|/[^/]*\$|/|" -e "s|[^/]*/|../|g"
}

dir_name ()
{
    echo "$1" | $SED -e "s:/[^/][^/]*\$::"
}

piped_value="s/|/\\\\|/g"
amp_value="s|&|\\\\&|g"
info2vars_sed ()          # generate <!--$vars--> substition sed addon script
{
  INP="$1" ; test ".$INP" = "." && INP="$tmp/$F.$DATA"
  V8=" *\\([^ ][^ ]*\\) \\(.*\\)<$QX>"
  V9=" *DC[.]\\([^ ][^ ]*\\) \\(.*\\)<$QX>"
  N8=" *\\([^ ][^ ]*\\) \\([$NN].*\\)<$QX>"
  N9=" *DC[.]\\([^ ][^ ]*\\) \\([$NN].*\\)<$QX>"
  V0="\\\\([<]*\\\\)\\\\\\\$"
  V1="\\\\([^<>]*\\\\)\\\\\\\$"
  V2="\\\\([^{<>}]*\\\\)"
  V3="\\\\([^<>]*\\\\)"
  SS="<""<>"">" # spacer so value="2004" does not make for s|\(...\)|\12004|
  test ".$commentvars"  = ".no" && updatevars="no"   # duplicated from
  test ".$commentvars"  = ".no" && expandvars="no"   # option handling
  test ".$expandvars" != ".no" && \
  $SED -e "/^=....=formatter /d" -e "$piped_value" \
      -e "/^<$Q'name'>/s,<$Q'name'>$V9,s|<!--$V0\\1[?]-->|- \\2|," \
      -e "/^<$Q'Name'>/s,<$Q'Name'>$V9,s|<!--$V0\\1[?]-->|(\\2)|," \
      -e "/^<$Q'name'>/s,<$Q'name'>$V8,s|<!--$V0\\1[?]-->|- \\2|," \
      -e "/^<$Q'Name'>/s,<$Q'Name'>$V8,s|<!--$V0\\1[?]-->|(\\2)|," \
      -e "/^<$Q/d" -e "/^<!/d" -e "$amp_value"  $INP # $++
  test ".$expandvars" != ".no" && \
  $SED -e "/^=....=formatter /d" -e "$piped_value" \
      -e "/^<$Q'text'>/s,<$Q'text'>$V9,s|<!--$V1\\1-->|\\\\1$SS\\2|," \
      -e "/^<$Q'Text'>/s,<$Q'Text'>$V9,s|<!--$V1\\1-->|\\\\1$SS\\2|," \
      -e "/^<$Q'name'>/s,<$Q'name'>$V9,s|<!--$V1\\1[?]-->|\\\\1$SS\\2|," \
      -e "/^<$Q'Name'>/s,<$Q'Name'>$V9,s|<!--$V1\\1[?]-->|\\\\1$SS\\2|," \
      -e "/^<$Q'text'>/s,<$Q'text'>$V8,s|<!--$V1\\1-->|\\\\1$SS\\2|," \
      -e "/^<$Q'Text'>/s,<$Q'Text'>$V8,s|<!--$V1\\1-->|\\\\1$SS\\2|," \
      -e "/^<$Q'name'>/s,<$Q'name'>$V8,s|<!--$V1\\1[?]-->|\\\\1$SS\\2|," \
      -e "/^<$Q'Name'>/s,<$Q'Name'>$V8,s|<!--$V1\\1[?]-->|\\\\1$SS\\2|," \
      -e "/^<$Q/d" -e "/^<!/d" -e "$amp_value"  $INP # $++
  test ".$updatevars" != ".no" && \
  $SED -e "/^=....=formatter /d" -e "$piped_value" \
      -e "/^<$Q'name'>/s,<$Q'name'>$V9,s|<!--$V0\\1:[?]-->[^<>]*|- \\2|," \
      -e "/^<$Q'Name'>/s,<$Q'Name'>$V9,s|<!--$V0\\1:[?]-->[^<>]*|(\\2)|," \
      -e "/^<$Q'name'>/s,<$Q'name'>$V8,s|<!--$V0\\1:[?]-->[^<>]*|- \\2|," \
      -e "/^<$Q'Name'>/s,<$Q'Name'>$V8,s|<!--$V0\\1:[?]-->[^<>]*|(\\2)|," \
      -e "/^<$Q/d"  -e "/^<!/d" -e "$amp_value"  $INP # $++
  test ".$updatevars" != ".no" && \
  $SED -e "/^=....=formatter /d"  -e "$piped_value" \
      -e "/^<$Q'text'>/s,<$Q'text'>$V9,s|<!--$V1\\1:[=]-->[^<>]*|\\\\1$SS\\2|," \
      -e "/^<$Q'Text'>/s,<$Q'Text'>$V9,s|<!--$V1\\1:[=]-->[^<>]*|\\\\1$SS\\2|," \
      -e "/^<$Q'name'>/s,<$Q'name'>$V9,s|<!--$V1\\1:[?]-->[^<>]*|\\\\1$SS\\2|," \
      -e "/^<$Q'Name'>/s,<$Q'Name'>$V9,s|<!--$V1\\1:[?]-->[^<>]*|\\\\1$SS\\2|," \
      -e "/^<$Q'text'>/s,<$Q'text'>$V8,s|<!--$V1\\1:[=]-->[^<>]*|\\\\1$SS\\2|," \
      -e "/^<$Q'Text'>/s,<$Q'Text'>$V8,s|<!--$V1\\1:[=]-->[^<>]*|\\\\1$SS\\2|," \
      -e "/^<$Q'name'>/s,<$Q'name'>$V8,s|<!--$V1\\1:[?]-->[^<>]*|\\\\1$SS\\2|," \
      -e "/^<$Q'Name'>/s,<$Q'Name'>$V8,s|<!--$V1\\1:[?]-->[^<>]*|\\\\1$SS\\2|," \
      -e "/^<$Q/d" -e "/^<!/d" -e "$amp_value"  $INP # $++
  test ".$attribvars" != ".no" && \
  $SED -e "/^=....=formatter /d" -e "$piped_value" \
      -e "/^<$Q'text'>/s,<$Q'text'>$V9,s|<$V1{\\1:[=]$V2}$V3>|<\\\\1$SS\\2\\\\3>|," \
      -e "/^<$Q'Text'>/s,<$Q'Text'>$V9,s|<$V1{\\1:[=]$V2}$V3>|<\\\\1$SS\\2\\\\3>|," \
      -e "/^<$Q'name'>/s,<$Q'name'>$V9,s|<$V1{\\1:[?]$V2}$V3>|<\\\\1$SS\\2\\\\3>|," \
      -e "/^<$Q'Name'>/s,<$Q'Name'>$V9,s|<$V1{\\1:[?]$V2}$V3>|<\\\\1$SS\\2\\\\3>|," \
      -e "/^<$Q'text'>/s,<$Q'text'>$V8,s|<$V1{\\1:[=]$V2}$V3>|<\\\\1$SS\\2\\\\3>|," \
      -e "/^<$Q'Text'>/s,<$Q'Text'>$V8,s|<$V1{\\1:[=]$V2}$V3>|<\\\\1$SS\\2\\\\3>|," \
      -e "/^<$Q'name'>/s,<$Q'name'>$V8,s|<$V1{\\1:[?]$V2}$V3>|<\\\\1$SS\\2\\\\3>|," \
      -e "/^<$Q'Name'>/s,<$Q'Name'>$V8,s|<$V1{\\1:[?]$V2}$V3>|<\\\\1$SS\\2\\\\3>|," \
      -e "/^<$Q/d" -e "/^<!/d" -e "$amp_value"  $INP # $++
  # if value="2004" then generated sed might be "\\12004" which is bad
  # instead we generate an edited value of "\\1$SS$value" and cut out
  # the spacer now after expanding the variable values:
  echo "s|$SS||g" # $++
}

info2meta_sed ()         # generate <meta name..> text portion
{
  # http://www.metatab.de/meta_tags/DC_type.htm
  INP="$1" ; test ".$INP" = "." && INP="$tmp/$F.$DATA"
  V6=" *HTTP[.]\\([^ ][^ ]*\\) \\(.*\\)<$QX>"
  V7=" *DC[.]\\([^ ][^ ]*\\) \\(.*\\)<$QX>"
  V8=" *\\([^ ][^ ]*\\) \\(.*\\)<$QX>"
  DATA_META_TYPE_SCHEME="name=\"DC.type\" content=\"\\2\" scheme=\"\\1\""
  DATA_META_DCMI="name=\"\\1\" content=\"\\2\" scheme=\"DCMIType\""
  DATA_META_NAME_TZ="name=\"\\1\" content=\"\\2 `timezone`\"" 
  DATA_META_NAME="name=\"\\1\" content=\"\\2\""
  DATA_META_HTTP="http-equiv=\"\\1\" content=\"\\2\""
  $SED -e "/=....=today /d" \
  -e "/<$Q'meta'>HTTP[.]/s,<$Q'meta'>$V6, <meta $DATA_META_HTTP />," \
  -e "/<$Q'meta'>DC[.]DCMIType /s,<$Q'meta'>$V7, <meta $DATA_META_TYPE_SCHEME />," \
  -e "/<$Q'meta'>DC[.]type Collection$/s,<$Q'meta'>$V8, <meta $DATA_META_DCMI />," \
  -e "/<$Q'meta'>DC[.]type Dataset$/s,<$Q'meta'>$V8, <meta $DATA_META_DCMI />," \
  -e "/<$Q'meta'>DC[.]type Event$/s,<$Q'meta'>$V8, <meta $DATA_META_DCMI />," \
  -e "/<$Q'meta'>DC[.]type Image$/s,<$Q'meta'>$V8, <meta $DATA_META_DCMI />," \
  -e "/<$Q'meta'>DC[.]type Service$/s,<$Q'meta'>$V8, <meta $DATA_META_DCMI />," \
  -e "/<$Q'meta'>DC[.]type Software$/s,<$Q'meta'>$V8, <meta $DATA_META_DCMI />," \
  -e "/<$Q'meta'>DC[.]type Sound$/s,<$Q'meta'>$V8, <meta $DATA_META_DCMI />," \
  -e "/<$Q'meta'>DC[.]type Text$/s,<$Q'meta'>$V8, <meta $DATA_META_DCMI />," \
  -e "/<$Q'meta'>DC[.]date[.].*[+]/s,<$Q'meta'>$V8, <meta $DATA_META_NAME />," \
  -e "/<$Q'meta'>DC[.]date[.].*[:]/s,<$Q'meta'>$V8, <meta $DATA_META_NAME_TZ />," \
  -e "/<$Q'meta'>/s,<$Q'meta'>$V8, <meta $DATA_META_NAME />," \
  -e "/<meta name=\"[^\"]*\" content=\"\" /d" \
  -e "/<meta http-equiv=\"[^\"]*\" content=\"\" /d" \
  -e "/^<$Q/d" -e "/^<!/d" $INP # $++
}

info_get_entry () # get the first <!--vars--> value known so far
{
  TXT="$1" ; test ".$TXT" = "." && TXT="sect"
  INP="$2" ; test ".$INP" = "." && INP="$tmp/$F.$DATA"
  $SED -e "/<$Q'text'>$TXT /!d" \
       -e "s|<$Q'text'>$TXT ||" -e "s|<$QX>||" -e "q" $INP # $++
}

info1grep () # test for a <!--vars--> substition to be already present
{
  TXT="$1" ; test ".$TXT" = "." && TXT="sect"
  INP="$2" ; test ".$INP" = "." && INP="$tmp/$F.$DATA"
  $GREP "^<$Q'text'>$TXT " $INP >$NULL
  return $?
}

dx_init()
{
    mkpathdir "$tmp"
    dx_meta formatter `basename $opt_formatter` > "$tmp/$F.$DATA"
    for opt in $opt_variables ; do case "$opt" in # commandline --def=value
      *_*) op_=`echo "$opt" | sed -e "y/_/-/"`    # makes for <!--$def-->
           dx_meta "$op_" `eval echo "\\\$opt_$opt"` ;; 
      *)   dx_text "$opt" `eval echo "\\\$opt_$opt"` ;;
    esac ; done
}

dx_line ()
{
    echo "<$Q$1>$2 "`trimmm "$3"`"<$QX>" >> "$tmp/$F.$DATA"
}

DX_line ()
{
    dx_val_=`echo "$3" | sed -e "s/<[^<>]*>//g"`
    dx_line "$1" "$2" "$dx_val_"
}

dx_text ()
{
    dx_line "'text'" "$1" "$2"
}

DX_text ()   # add a <!--vars--> substition includings format variants
{
  N=`trimm "$1"` ; T=`trimm "$2"`
  if test ".$N" != "." ; then
    if test ".$T" != "." ; then
      text=`echo "$T" | $SED -e "y/$UPPER/$LOWER/" -e "s/<[^<>]*>//g"`
      dx_line "'text'" "$N" "$T"
      dx_line "'name'" "$N" "$text"
      varname=`echo "$N" | $SED -e 's/.*[.]//'`    # cut out front part
      if test ".$N" != ".$varname" ; then 
      text=`echo "$varname $T" | $SED -e "y/$UPPER/$LOWER/" -e "s/<[^<>]*>//g"`
      dx_line "'Text'" "$varname" "$T"
      dx_line "'Name'" "$varname" "$text"
      fi
    fi
  fi
}

dx_meta ()
{
    DX_line "'meta'" "$1" "$2"
}

DX_meta ()  # add simple meta entry and its <!--vars--> subsitution
{
    DX_line "'meta'" "$1" "$2"
    DX_text "$1" "$2"
}

DC_meta ()   # add new DC.meta entry plus two <!--vars--> substitutions
{
    DX_line "'meta'" "DC.$1" "$2"
    DX_text "DC.$1" "$2"
    DX_text "$1" "$2"
}

HTTP_meta ()   # add new HTTP.meta entry plus two <!--vars--> substitutions
{
    DX_line "'meta'" "HTTP.$1" "$2"
    DX_text "HTTP.$1" "$2"
    DX_text "$1" "$2"
}

DC_VARS_Of () # check DC vars as listed in $DC_VARS global and generate DC_meta
{             # the results will be added to .meta.tmp and .vars.tmp later
   FILENAME="$1" ; test ".$FILENAME" = "." && FILENAME="$SOURCEFILE"   
   for M in $DC_VARS title ; do
      # scan for a <markup> of this name
      part=`$SED -e "/<$M>/!d" -e "s|.*<$M>||" -e "s|</$M>.*||" -e q $FILENAME`
      part=`trimm "$part"`
      text=`echo  "$part" | $SED -e "s|^[$AA]*:||"`
      text=`trimm "$text"`
      test ".$text" = "." && continue
      # <mark:part> will be <meta name="mark.part">
      if test ".$text" != ".$part" ; then
         N=`echo "$part" | $SED -e "s/:.*//"`
         DC_meta "$M.$N" "$text"
      elif test ".$M" = ".date" ; then
         DC_meta "$M.issued" "$text" # "<date>" -> "<date>issued:"
      else
         DC_meta "$M" "$text"
      fi
   done
}

HTTP_VARS_Of () # check HTTP-EQUIVs as listed in $_EQUIV global then
{               # generate meta tags that are http-equiv= instead of name=
   FILENAME="$1" ; test ".$FILENAME" = "." && FILENAME="$SOURCEFILE"   
   for M in $_EQUIVS ; do
      # scan for a <markup> of this name
      part=`$SED -e "/<$M>/!d" -e "s|.*<$M>||" -e "s|</$M>.*||" -e q $FILENAME`
      part=`trimm "$part"`
      text=`echo  "$part" | $SED -e "s|^[$AA]*:||"`
      text=`trimm "$text"`
      test ".$text" = "." && continue
      if test ".$M" = ".redirect" ; then
         HTTP_meta "refresh" "5; url=$text" ; DX_text "$M" "$text"
      elif test ".$M" = ".charset" ; then
         HTTP_meta "content-type" "text/html; charset=$text" 
      else
         HTTP_meta "$M" "$text"
      fi
   done
}

DC_isFormatOf ()       # make sure there is this DC.relation.isFormatOf tag
{                      # choose argument for a fallback (usually $SOURCEFILE)
   NAME="$1" ; test ".$NAME" = "." && NAME="$SOURCEFILE"   
   info1grep DC.relation.isFormatOf || DC_meta relation.isFormatOf "$NAME"
}

DC_publisher ()        # make sure there is this DC.publisher meta tag
{                      # choose argument for a fallback (often $USER)
   NAME="$1" ; test ".$NAME" = "." && NAME="$USER"
   info1grep DC.publisher || DC_meta publisher "$NAME"
}

DC_modified ()         # make sure there is a DC.date.modified meta tag
{                      # maybe choose from filesystem dates if possible
   ZZ="$1" # target file
   if info1grep DC.date.modified ; then :
   else
      _42_chars="........................................."
      cut_42_55="s/^$_42_chars\\(.............\\).*/\\1/" # i.e.`cut -b 42-55`
      text=`$STAT_R $ZZ 2>$NULL | $SED -e '/odify:/!d' -e 's|.*fy:||' -e q`
      text=`echo "$text" | $SED -e "s/:..[.][$NN]*//"`
      text=`trimm "$text"`
      test ".$text" = "." && \
      text=`$DATE_R "$ZZ" +%Y-%m-%d 2>$NULL`   # GNU sed
      test ".$text" = "." && 
      text=`$LS_L "$ZZ" | $SED -e "$cut_42_55" -e "s/^ *//g" -e "q"`
      text=`echo "$text" | $SED -e "s/[$NN]*:.*//"` # cut way seconds
      DC_meta date.modified `trimm "$text"`
   fi
}

DC_date ()             # make sure there is this DC.date meta tag
{                      # choose from one of the available DC.date.* specials
   ZZ="$1" # source file
   if info1grep DC.date 
   then DX_text issue "dated `info_get_entry DC.date`"
        DX_text updated     "`info_get_entry DC.date`"
   else text=""
      for kind in available issued modified created ; do
        text=`info_get_entry DC.date.$kind` 
      # test ".$text" != "." && echo "$kind = date = $text ($ZZ)"
        test ".$text" != "." && break
      done
      if test ".$text" = "." ; then
        M="date"
        part=`$SED -e "/<$M>/!d" -e "s|.*<$M>||" -e "s|</$M>.*||" -e q $ZZ`
	part=`trimm "$part"`
        text=`echo "$part" | $SED -e "s|^[$AA]*:||"`
	text=`trimm "$text"`
      fi
      if test ".$text" = "." ; then 
        M="!--date:*=*--" # takeover updateable variable...
        part=`$SED -e "/<$M>/!d" -e "s|.*<$M>||" -e "s|</.*||" -e q $ZZ`
	part=`trimm "$part"`
        text=`echo "$part" | $SED -e "s|^[$AA]*:||" -e "s|\\&.*||"`
	text=`trimm "$text"`
      fi
      text=`echo "$text" | $SED -e "s/[$NN]*:.*//"` # cut way seconds
      DX_text updated "$text"
      text1=`echo "$text" | $SED -e "s|^.* *updated ||"`
      if test ".$text" != ".$text1" ; then
        kind="modified" ; text=`echo "$text1" | $SED -e "s|,.*||"`
      fi
      text1=`echo "$text" | $SED -e "s|^.* *modified ||"`
      if test ".$text" != ".$text1" ; then
        kind="modified" ; text=`echo "$text1" | $SED -e "s|,.*||"`
      fi
      text1=`echo "$text" | $SED -e "s|^.* *created ||"`
      if test ".$text" != ".$text1" ; then
        kind="created" ; text=`echo "$text1" | $SED -e "s|,.*||"`
      fi
      text=`echo "$text" | $SED -e "s/[$NN]*:.*//"` # cut way seconds
      DC_meta date `trimm "$text"`
      DX_text issue `trimm "$kind $text"`
   fi
}

DC_title ()
{
   # choose a title for the document, either an explicit title-tag
   # or one of the section headers in the document or fallback to filename
   ZZ="$1" # target file
   if info1grep DC.title ; then :
   else
      for M in TITLE title H1 h1 H2 h2 H3 H3 H4 H4 H5 h5 H6 h6 ; do
        text=`$SED -e "/<$M>/!d" -e "s|.*<$M>||" -e "s|</$M>.*||" -e q $ZZ`
	text=`trimm "$text"` ; test ".$text" != "." && break
        MM="$M [^<>]*"
        text=`$SED -e "/<$MM>/!d" -e "s|.*<$MM>||" -e "s|</$M>.*||" -e q $ZZ`
	text=`trimm "$text"` ; test ".$text" != "." && break
      done
      if test ".text" = "." ; then
	text=`basename $ZZ .html`
        text=`basename $text .htm | $SED -e 'y/_/ /' -e "s/\\$/ info/"`
      fi
      term=`echo "$text" | $SED -e 's/.*[(]//' -e 's/[)].*//'`
      text=`echo "$text" | $SED -e 's/[(][^()]*[)]//'`
      if test ".$term" = "." || test ".$term" = ".$text" ; then
         DC_meta title "$text"
      else
         DC_meta title "$term - $text"
      fi
   fi
}    

site_get_section () # return parent section page of given page
{
   _F_=`sed_slash_key "$1"`
   $SED -e "/^<$Q'sect'>$_F_ /!d" \
        -e "s|^<$Q'sect'>$_F_ ||" -e "s|<$QX>||" \
        -e q "$MK_DATA" # $++
}

DC_section () # not really a DC relation (shall we use isPartOf ?) 
{             # each document should know its section father
   sectn=`site_get_section "$F"`
   if test ".$sectn" != "." ; then
      DC_meta relation.section "$sectn"
   fi
}

info_get_entry_section()
{
    info_get_entry DC.relation.section # $++
}    

site_get_selected ()  # return section of given page
{
   _F_=`sed_slash_key "$1"`
   $SED -e "/<$Q'use.'>$_F_ /!d" \
        -e "s|<$Q'use.'>[^ ]* ||" -e "s|<$QX>||" \
        -e q "$MK_DATA" # $++
}

DC_selected () # not really a DC title (shall we use alternative ?)
{
   # each document might want to highlight the currently selected item
   short=`site_get_selected "$F"`
   if test ".$short" != "." ; then
      DC_meta title.selected "$short"
   fi
}

info_get_entry_selected ()
{
    info_get_entry DC.title.selected # $++
}

site_get_rootsections () # return all sections from root of nav tree
{
   $SED -e "/^<$Q'use1'>/!d" \
        -e "s|^<$Q'use.'>\\([^ ]*\\) .*|\\1|" "$MK_DATA" # $++
}

site_get_sectionpages () # return all children pages in the given section
{
   _F_=`sed_slash_key "$1"`
   $SED -e "/^<$Q'sect'>[^ ]* $_F_<[^<>]*>\$/!d" \
        -e "s|^<$Q'sect'>||" -e "s|<$QX>||" \
        -e "s/ .*//" "$MK_DATA" # $++
}

site_get_subpages () # return all page children of given page
{
   _F_=`sed_slash_key "$1"`
   $SED -e "/^<$Q'node'>[^ ]* $_F_<[^<>]*>\$/!d" \
        -e "s|^<$Q'node'>||" -e "s|<$QX>||" \
        -e "s/ .*//" "$MK_DATA"
   # $++
}

site_get_parentpage () # return parent page for given page (".." for sections)
{
   _F_=`sed_slash_key "$1"`
   $SED -e "/^<$Q'node'>$_F_ /!d" \
        -e "s|^<$Q'node'>[^ ]* ||" -e "s|<$QX>||" \
        -e "q" "$MK_DATA"  # $++
}

DX_alternative ()        # detect wether page asks for alternative style
{                        # which is generally a shortpage variant
    x=`mksite_magic_option alternative $1 | sed -e "s/^ *//" -e "s/ .*//"`
    if test ".$x" != "." ; then
      DX_text alternative "$x"
    fi
}

info2head_sed ()      # append alternative handling script to $HEAD
{
    have=`info_get_entry alternative`
    if test ".$have" != "." ; then
       echo "/<!--mksite:alternative:$have .*-->/{" # $++
       echo "s/<!--mksite:alternative:$have\\( .*\\)-->/\\1/" # $++
       echo "q" # $++ 
       echo "}" # $++
    fi
}
info2body_sed ()      # append alternative handling script to $BODY
{
    have=`info_get_entry alternative`
    if test ".$have" != "." ; then
       echo "s/<!--mksite:alternative:$have\\( .*\\)-->/\\1/" # $++
    fi
}

bodymaker_for_sectioninfo ()
{
    test ".$sectioninfo" = ".no" && return
    _x_="<!--mksite:sectioninfo::-->"
    _q_="\\([^<>]*[$AX][^<>]*\\)"
    test ".$sectioninfo" != ". " && _q_="[ ][ ]*$sectioninfo\\([ ]\\)" 
    echo "s|\\(^<[hH][$NN][ >].*</[hH][$NN]>\\)$_q_|\\1$_x_\\2|"       # $++
    echo "/$_x_/s|^|<table width=\"100%\"><tr valign=\"bottom\"><td>|" # $++
    echo "/$_x_/s|</[hH][$NN]>|&</td><td align=\"right\"><i>|"         # $++
    echo "/$_x_/s|\$|</i></td></tr></table>|"                          # $++
    echo "s|$_x_||"                                                    # $++
}

fast_href ()  # args "$FILETOREFERENCE" "$FROMCURRENTFILE:$F"
{   # prints path to $FILETOREFERENCE href-clickable in $FROMCURRENTFILE
    # if no subdirectoy then output is the same as input $FILETOREFERENCE
    R="$2" ; test ".$R" = "." && R="$F"
    S=`back_path "$R"`
    if test ".$S" = "." 
    then echo "$1" # $++
    else _1_=`echo "$1" | \
         $SED -e "/^ *\$/d" -e "/^\\//d" -e "/^[.][.]/d" -e "/^[$AA]*:/d" `
         if test ".$_1_" = "." # don't move any in the pattern above
         then echo "$1"   # $++
         else echo "$S$1" # $++  prefixed with backpath
    fi fi
}

make_back_path () # "$FILE"
{
    R="$1" ; test ".$R" = "." && R="$F"
    S=`back_path "$R"`
    if test ".$S" != "." ; then
       echo "s|\\(<[^<>]* href=\\\"\\)\\([$AA][^<>:]*\\\"[^<>]*>\\)|\\1$S\\2|g"
       echo "s|\\(<[^<>]* src=\\\"\\)\\([$AA][^<>:]*\\\"[^<>]*>\\)|\\1$S\\2|g"
    fi
}

# ============================================================== SITE MAP DATA
# each entry needs atleast a list-title, a long-title, and a list-date
# these are the basic information to be printed in the sitemap file
# where it is bound the hierarchy of sect/subsect of the entries.

site_map_list_title() # $file $text
{
    echo "<$Q'list'>$1 $2<$QX>" >> "$MK_DATA"
}
info_map_list_title() # $file $text
{
    echo "<$Q'list'>$2<$QX>" >> "$tmp/$1.$DATA"
}
site_map_long_title() # $file $text
{
    echo "<$Q'long'>$1 $2<$QX>" >> "$MK_DATA"
}
info_map_long_title() # $file $text
{
    echo "<$Q'long'>$2<$QX>" >> "$tmp/$1.$DATA"
}
site_map_list_date() # $file $text
{
    echo "<$Q'date'>$1 $2<$QX>" >> "$MK_DATA"
}
info_map_list_date() # $file $text
{
    echo "<$Q'date'>$2<$QX>" >> "$tmp/$1.$DATA"
}

siteinfo2sitemap ()  # generate <name><page><date> addon sed scriptlet
{                    # the resulting script will act on each item/line
                     # containing <!--"filename"--> and expand any following
                     # reference of <!--name--> or <!--date--> or <!--long-->
  INP="$1" ; test ".$INP" = "." && INP="$MK_DATA"
  _list_="s|\\\\(<!--\"\\1\"-->.*\\\\)<name href=[^<>]*>.*</name>|\\\\1<name href=\"\\1\">\\2</name>|"
  _date_="s|\\\\(<!--\"\\1\"-->.*\\\\)<date>.*</date>|\\\\1<date>\\2</date>|"
  _long_="s|\\\\(<!--\"\\1\"-->.*\\\\)<long>.*</long>|\\\\1<long>\\2</long>|"
  $SED -e "s:&:\\\\&:g" \
       -e "s:<$Q'list'>\\([^ ]*\\) \\(.*\\)<$QX>:$_list_:" \
       -e "s:<$Q'date'>\\([^ ]*\\) \\(.*\\)<$QX>:$_date_:" \
       -e "s:<$Q'long'>\\([^ ]*\\) \\(.*\\)<$QX>:$_long_:" \
       -e "/^s|/!d" $INP # $++
}

make_multisitemap ()
{  # each category gets its own column along with the usual entries
   INPUTS="$1" ; test ".$INPUTS" = "." && INPUTS="$MK_DATA"
   siteinfo2sitemap > "$MK_SITE" # have <name><long><date> addon-sed
  _form_="<!--\"\\2\"--><!--use\\1--><long>\\3</long><!--end\\1-->"
  _form_="$_form_<br><name href=\"\\2\">\\3</name><date>......</date>"
  _tiny_="small><small><small" ; _tinyX_="small></small></small "
  _tabb_="<br><$_tiny_> </$_tinyX_>" ; _bigg_="<big> </big>"
  echo "<table width=\"100%\"><tr><td> " # $++
  $SED -e "/^<$Q'[Uu]se.'>/!d" \
       -e "/>[$AZ$az][$AZ$az][$AZ$az][$AZ$az]*:/d" \
       -e "s|^<$Q'[Uu]se\\(.\\)'>\\([^ ]*\\) \\(.*\\)<$QX>|$_form_|" \
       -f "$MK_SITE" -e "/<name/!d" \
       -e "s|<!--use1-->|</td><td valign=\"top\"><b>|" \
       -e "s|<!--end1-->|</b>|"  \
       -e "s|<!--use2-->|<br>|"  \
       -e "s|<!--use.-->|<br>|" -e "s/<!--[^<>]*-->/ /g" \
       -e "s|<name |<$_tiny_><a |" -e "s|</name>||" \
       -e "s|<date>|<small style=\"date\">|" \
       -e "s|</date>|</small></a><br></$_tinyX_>|" \
       -e "s|<long>|<!--long-->|" -e "s|</long>|<!--/long-->|" \
       $INPUTS              # $++
   echo "</td><tr></table>" # $++
}

make_listsitemap ()
{   # traditional - the body contains a list with date and title extras
   INPUTS="$1" ; test ".$INPUTS" = "." && INPUTS="$MK_DATA"
   siteinfo2sitemap > "$MK_SITE" # have <name><long><date> addon-sed
   _form_="<!--\"\\2\"--><!--use\\1--><name href=\"\\2\">\\3</name>"
   _form_="$_form_<date>......</date><long>\\3</long>"
   _tabb_="<td>\\&nbsp\\;</td>" 
   echo "<table cellspacing=\"0\" cellpadding=\"0\">" # $++
   $SED -e "/^<$Q'[Uu]se.'>/!d" \
        -e "/>[$AZ$az][$AZ$az][$AZ$az][$AZ$az]*:/d" \
        -e "s|^<$Q'[Uu]se\\(.\\)'>\\([^ ]*\\) \\(.*\\)<$QX>|$_form_|" \
        -f "$MK_SITE" -e "/<name/!d" \
        -e "s|<!--use\\(1\\)-->|<tr class=\"listsitemap\\1\"><td>*</td>|" \
        -e "s|<!--use\\(2\\)-->|<tr class=\"listsitemap\\1\"><td>-</td>|" \
        -e "s|<!--use\\(.\\)-->|<tr class=\"listsitemap\\1\"><td> </td>|" \
        -e  "/<tr.class=\"listsitemap3\">/s|<name [^<>]*>|&- |" \
        -e "s|<!--[^<>]*-->| |g" \
	-e "s|<name href=\"name:sitemap:|<name href=\"|" \
        -e "s|<name |<td><a |" -e "s|</name>|</a></td>$_tabb_|" \
        -e "s|<date>|<td><small style=\"date\">|" \
        -e "s|</date>|</small></td>$_tabb_|" \
        -e "s|<long>|<td><em><!--long-->|" \
        -e "s|</long>|<!--/long--></em></td></tr>|" \
        "$INPUTS"             # $++
   for xx in `grep "^<$Q'use.'>name:sitemap:" $INPUTS` ; do
       xx=`echo $xx | sed -e "s/^<$Q'use.'>name:sitemap://" -e "s|<$QX>||"` 
       if test -f "$xx" ; then
	   grep "<tr.class=\"listsitemap[$NN]\">" $xx # $++
       fi
   done
   echo "</table>"          # $++
}

_xi_include_=`echo \
    "<xi:include xmlns:xi=\"http://www.w3.org/2001/XInclude\" parse=\"xml\""`
make_xmlsitemap ()
{   # traditional - the body contains a list with date and title extras
   INPUTS="$1" ; test ".$INPUTS" = "." && INPUTS="$MK_DATA"
   siteinfo2sitemap > "$MK_SITE" # have <name><long><date> addon-sed
   _form_="<!--\"\\2\"--><name href=\"\\2\">\\3</name>"
   _sitefile_=`sed_slash_key "$SITEFILE"`
   $SED -e "/^<$Q'[Uu]se.'>/!d" \
        -e "/>[$AZ$az][$AZ$az][$AZ$az][$AZ$az]*:/d" \
        -e "s|^<$Q'[Uu]se\\(.\\)'>\\([^ ]*\\) \\(.*\\)<$QX>|$_form_|" \
        -f "$MK_SITE" -e "/<name/!d" \
        -e "/${_sitefile_}/d" \
        -e "/${_sitefile_}l/d" \
        -e "s|\\(href=\"[^<>]*\\)\\.html\\(\"\\)|\\1.xml\\2|g" \
        -e "s|.*<name|$_xi_include_\\n   |" \
        -e "s|>.*</name>| />|" \
        "$INPUTS"            # $++
}

print_extension ()
{
    ARG="$1" ; test ".$ARG" = "." && ARG="$opt_print"
    case "$ARG" in
      -*|.*) echo "$ARG" ;;   # $++
      *)     echo ".print" ;; # $++
    esac
}

from_sourcefile ()
{
    if test -f "$1"
    then echo "$1"
    elif test -f "$opt_srcdir/$1"
    then echo    "$opt_srcdir/$1"
    else echo "$1"
    fi
}
    
html_sourcefile ()  # generally just cut away the trailing "l" (ell)
{                   # making "page.html" argument into "page.htm" return
                    # (as a new addtion the source may be in ".dbk" xml)
    _SRCFILE_=`echo "$1" | $SED -e "s/l\\$//"`
    _XMLFILE_=`echo "$1" | $SED -e "s/\\.html/.dbk/"`
    if test -f "$_SRCFILE_" 
    then echo  "$_SRCFILE_" # $++
    elif test -f "$_XMLFILE_" 
    then echo    "$_XMLFILE_" # $++
    elif test -f "$opt_src_dir/$_SRCFILE_" 
    then echo    "$opt_src_dir/$_SRCFILE_" # $++
    elif test -f "$opt_src_dir/$_XMLFILE_" 
    then echo    "$opt_src_dir/$_XMLFILE_" # $++
    else echo ".//$_SRCFILE_" # $++ (not found?)
    fi
}
html_printerfile_sourcefile () 
{                   
    if test ".$printerfriendly" = "."
    then 
    echo "$1" | sed -e "s/l\$//" # $++
    else 
    _ext_=`print_extension "$printerfriendly"`
    _ext_=`sed_slash_key "$_ext_"`
    echo "$1" | sed -e "s/l\$//" -e "s/$_ext_\\([.][$AA]*\\)\$/\\1/" # $++
    fi
}

fast_html_printerfile () {
    x=`html_printerfile "$1"` ; basename "$x" # $++
#   x=`html_printerfile "$1"` ; fast_href "$x" $2 # $++
}

html_printerfile () # generate the printerfile for a given normal output
{
    _ext_=`print_extension "$printerfriendly" | sed -e "s/&/\\\\&/"`
    echo "$1" | sed -e "s/\\([.][$AA]*\\)\$/$_ext_\\1/" # $++
}

make_printerfile_fast () # generate s/file.html/file.print.html/ for hrefs
{                        # we do that only for the $FILELIST
   ALLPAGES="$1" ; # ="$FILELIST"
   for p in $ALLPAGES ; do
       a=`sed_slash_key "$p"`
       b=`html_printerfile "$p"`
       if test "$b" != "$p" ; then
         b=`html_printerfile "$p" | sed -e "s:&:\\\\&:g" -e "s:/:\\\\\\/:g"`
         echo "s/<a href=\"$a\">/<a href=\"$b\">/" # $++
         echo "s/<a href=\"$a\" /<a href=\"$b\" /" # $++
       fi
   done
}

echo_printsitefile_style ()
{
   _bold_="text-decoration : none ; font-weight : bold ; "
   echo "   <style>"                                          # $+++
   echo "     a:link    { $_bold_ color : #000060 ; }"        # $+++
   echo "     a:visited { $_bold_ color : #000040 ; }"        # $+++
   echo "     body      { background-color : white ; }"       # $+++
   echo "   </style>"                                         # $+++
}

make_printsitefile_head() # $sitefile
{
   echo_printsitefile_style > "$MK_STYLE"
   $SED -e "/<title>/p" -e "/<title>/d" \
        -e "/<head>/p"   -e "/<head>/d" \
        -e "/<\/head>/p"  -e "/<\/head>/d" \
        -e "/<body>/p"   -e "/<body>/d" \
        -e "/^.*<link [^<>]*rel=\"shortcut icon\"[^<>]*>.*\$/p" \
        -e "d" $SITEFILE | $SED -e "/<head>/r $MK_STYLE" # $+++
}


# ------------------------------------------------------------------------
# The printsitefile is a long text containing html href markups where
# each of the href lines in the file is being prefixed with the section
# relation. During a secondary call the printsitefile can grepp'ed for
# those lines that match a given output fast-file. The result is a
# navigation header with 1...3 lines matching the nesting level

# these alt-texts will be only visible in with a text-mode browser:
printsitefile_square="width=\"8\" height=\"8\" border=\"0\""
printsitefile_img_1="<img alt=\"|go text:\" $printsitefile_square />"
printsitefile_img_2="<img alt=\"||topics:\" $printsitefile_square />"
printsitefile_img_3="<img alt=\"|||pages:\" $printsitefile_square />"
_SECT="mksite:sect:"

echo_current_line () # $sect $extra
{
    echo "<!--$_SECT\"$1\"-->$2" # $++
}
make_current_entry () # $sect $file      ## requires $MK_SITE
{
  S="$1" ; R="$2"
  SSS=`sed_slash_key "$S"`  
  sep=" - " ; _left_=" [ " ; _right_=" ] "
  echo_current_line "$S" "<!--\"$R\"--><name href=\"$R\">$R</name>$sep" \
       | $SED -f "$MK_SITE" \
        -e "s|<!--[^<>]*--><name |<a |" -e "s|</name>|</a>|" \
        -e "/<a href=\"$SSS\"/s/<a href/$_left_&/" \
        -e "/<a href=\"$SSS\"/s/<\\/a>/&$_right_/"  # $+++
}
echo_subpage_line () # $sect $extra
{
    echo "<!--$_SECT*:\"$1\"-->$2" # $++
}

make_subpage_entry ()
{
  S="$1" ; R="$2"
  RR=`sed_slash_key "$R"`  
  sep=" - " ;
  echo_subpage_line "$S" "<!--\"$R\"--><name href=\"$R\">$R</name>$sep" \
       | $SED -f "$MK_SITE" \
        -e "s|<!--[^<>]*--><name |<a |" -e "s|</name>|</a>|" # $+++
}

make_printsitefile ()
{
   # building the printsitefile looks big but its really a loop over sects
   INPUTS="$1" ; test ".$INPUTS" = "." && INPUTS="$MK_DATA"
   siteinfo2sitemap > "$MK_SITE" # have <name><long><date> addon-sed
   if test -d DEBUG &&  test -f "$MK_SITE" 
       then FFFF=`echo "$F" | sed -e "s,/,:,g"`
       cp "$MK_DATA" "DEBUG/$FFFF.SITE.tmp.sed"
   fi

   make_printsitefile_head $SITEFILE # $++
   sep=" - "
   _sect1="<a href=\"#.\" title=\"section\">$printsitefile_img_1</a> ||$sep"
   _sect2="<a href=\"#.\" title=\"topics\">$printsitefile_img_2</a> ||$sep"
   _sect3="<a href=\"#.\" title=\"pages\">$printsitefile_img_3</a> ||$sep"
   site_get_rootsections > "$MK_SECT1"
   # round one - for each root section print a current menu
   for r in `cat "$MK_SECT1"` ; do
       echo_current_line "$r" "<!--mksite:sect1:A--><br>$_sect1" # $++
       for s in `cat "$MK_SECT1"` ; do 
	   make_current_entry "$r" "$s" # $++
       done
       echo_current_line "$r" "<!--mksite:sect1:Z-->" # $++
   done # "$r"

   # round two - for each subsection print a current and subpage menu
   for r in `cat "$MK_SECT1"` ; do
   site_get_subpages "$r"     > "$MK_SECT2"
   for s in `cat "$MK_SECT2"` ; do test "$r" = "$s" && continue
       echo_current_line  "$s" "<!--mksite:sect2:A--><br>$_sect2" # $++
       for t in `cat "$MK_SECT2"` ; do test "$r" = "$t" && continue
	   make_current_entry "$s" "$t" # $++
       done # "$t"
       echo_current_line  "$s" "<!--mksite:sect2:Z-->" # $++
   done # "$s"
       _have_children_="0"
       for t in `cat "$MK_SECT2"` ; do test "$r" = "$t" && continue
	   test "$_have_children_" = "0" && _have_children_="1" && \
       echo_subpage_line  "$r" "<!--mksite:sect2:A--><br>$_sect2" # $++
	   make_subpage_entry "$r" "$t" # $++
       done # "$t"
           test "$_have_children_" = "1" && \
       echo_subpage_line  "$r" "<!--mksite:sect2:Z-->" # $++
   done # "$r"

   # round three - for each subsubsection print a current and subpage menu
   for r in `cat "$MK_SECT1"` ; do
   site_get_subpages "$r"     > "$MK_SECT2"
   for s in `cat "$MK_SECT2"` ; do test "$r" = "$s" && continue
   site_get_subpages "$s"     > "$MK_SECT3"
   for t in `cat "$MK_SECT3"` ; do test "$s" = "$t" && continue
       echo_current_line  "$t" "<!--mksite:sect3:A--><br>$_sect3" # $++
       for u in `cat "$MK_SECT3"` ; do test "$s" = "$u" && continue
	   make_current_entry "$t" "$u" # $++
       done # "$u"
       echo_current_line  "$t" "<!--mksite:sect3:Z-->" # $++
   done # "$t"
       _have_children_="0"
       for u in `cat "$MK_SECT3"` ; do test "$u" = "$s" && continue
	   test "$_have_children_" = "0" && _have_children_="1" && \
       echo_subpage_line  "$s" "<!--mksite:sect3:A--><br>$_sect3" # $++
	   make_subpage_entry "$s" "$u" # $++
       done # "$u"
           test "$_have_children_" = "1" && \
       echo_subpage_line  "$s" "<!--mksite:sect3:Z-->" # $++
   done # "$s"
   done # "$r"
   echo "<a name=\".\"></a>" # $++
   echo "</body></html>"    # $++
}

# create a selector that can grep a printsitefile for the matching entries
select_in_printsitefile () # arg = "page" : return to stdout >> $P.$HEAD
{
   _selected_="$1" ; test ".$_selected_" = "." && _selected_="$F"
   _section_=`sed_slash_key "$_selected_"`
   echo "s/^<!--$_SECT\"$_section_\"-->//"        # sect3
   echo "s/^<!--$_SECT[*]:\"$_section_\"-->//"    # children
   _selected_=`site_get_parentpage "$_selected_"` 
   _section_=`sed_slash_key "$_selected_"`
   echo "s/^<!--$_SECT\"$_section_\"-->//"        # sect2
   _selected_=`site_get_parentpage "$_selected_"` 
   _section_=`sed_slash_key "$_selected_"`
   echo "s/^<!--$_SECT\"$_section_\"-->//"        # sect1
   echo "/^<!--$_SECT\"[^\"]*\"-->/d"     
   echo "/^<!--$_SECT[*]:\"[^\"]*\"-->/d" 
   echo "s/^<!--mksite:sect[$NN]:[$AZ]-->//"
}

body_for_emailfooter ()
{
    test ".$emailfooter" = ".no" && return
    _email_=`echo "$emailfooter" | sed -e "s|[?].*||"`
    _dated_=`info_get_entry updated`
    echo "<hr><table border=\"0\" width=\"100%\"><tr><td>"
    echo "<a href=\"mailto:$emailfooter\">$_email_</a>"
    echo "</td><td align=\"right\">"
    echo "$_dated_</td></tr></table>"
}

# =================================================================== CSS
# There was another project to support sitemap build from xml files.
# The source format was using .dbk+xml with embedded references to .css
# files for visual preview in a browser. An docbook xml file with semantic
# outlines is far better suited for quality documentation than any html
# source. It happens that the xml/css support in browsers is still not
# very portable - especially embedded css style blocks are a nightmare.
# Instead we (a) grab all non-html xml markup tags (b) grab all referenced
# css stylesheets (c) cut out css defs from [b] that are known by [a] and
# (d) append those to the <style> tag in the output html file as well as
# (e) reformatting the defs as well as markups from tags to tag classes.
# Input dbk/htm
#  <?xml-stylesheet type="text/css" href="html.css" ?>         <!-- dbk/xml -->
#  <link rel="stylesheet" type="text/css" href="sdocbook.css" /> <!-- xhtml -->
#  <article><para>
#  Using some <command>exe</command>
#  </para></article>
# Input css:
#  article { .. ; display : block }
#  para { .. ; display : block }
#  command { .. ; display : inline }
# Output html:
#  <html><style type="text/css">
#  div .article { .. }
#  div .para { .. }
#  span .command { .. }
#  </style>
#  <div class="article"><div class="para>
#  Using some <span class="command">exe</span>
#  </div></div>

css_sourcefile ()
{
    if test -f "$1" ; then echo "$1"
    elif test -f "$opt_src_dir/$1" ; then echo "$opt_src_dir/$1"
    elif echo "$1" | grep "^/" > $NULL ; then echo "$1"
    else echo "./$1"
    fi
}

css_xmltags () # $SOURCEFILE
{
   X=`echo $SOURCEFILE | sed -e "y:/:~:"`
   S="$SOURCEFILE"
   cat "$S" | $SED -e "s|>[^<>]*<|><|g" -e "s|^[^<>]*<|<|" \
                   -e "s|>[^<>]*\$|>|"  -e "s|<|\\n|g" \
            | $SED -e "/^\\//d" -e "/^ *\$/d" -e "/>/!d" -e "s|>.*||" \
            | sort | uniq > "$tmp/$MK.$X.xmltags.tmp.txt"
}

css_xmlstyles () # $SOURCEFILE
{
   X=`echo $SOURCEFILE | sed -e "y:/:~:"`
   S="$SOURCEFILE"
   cat "$S" "$SITEFILE" \
       | sed \
       -e "s|<link  *rel=['\"]*stylesheet|<?xml-stylesheet |" \
       -e "/<.xml-stylesheet/!d" -e "/href/!N" -e "/href/!N" \
       -e "s|^.*<.xml-stylesheet||" -e 's|^.*href="||' -e 's|".*||' \
       | sort | uniq > "$tmp/$MK.$X.xmlstylesheets.tmp.txt"
}

css_xmlstyles_sed () # $SOURCEFILE
{
   X=`echo $SOURCEFILE | sed -e "y:/:~:"`
   S="$tmp/$MK.$X.xmltags.tmp.txt"
   R="$tmp/$MK.$X.xmltags.tmp.sed"
   rm -f "$R"
   {
      for x in 1 2 3 4 5 6 7 8 9 ; do echo "/}/d" ; echo "/{/!N" ; done
      echo "s|\\r||g"
      $SED "/^[$AZ$az$NN]/!d" "$S" | { while read xmltag ; do
	 xmltag=`echo "$xmltag" | sed -e "s/ .*//"`
         _xmltag=`sed_slash_key "$xmltag"`
         if echo " title section " | grep " $xmltag " > $NULL ; then
	    test "$xmltag" = "section" && continue;
            echo "/^ *$_xmltag *[,\\n{]/bfound" >> "$R"
            echo "/[,\\n] *$_xmltag *[,\\n{]/bfound" >> "$R"
            $SED "/^[$AZ$az$NN]/!d" "$S" | { while read xmlparent ; do
	       xmlparent=`echo "$xmlparent" | sed -e "s/ .*//"`
               _xmlparent=`sed_slash_key "$xmlparent"`
               echo "/^ *$_xmlparent  *$_xmltag *[,\\n{]/bfound"
               echo "/[ ,\\n] *$_xmlparent  *$_xmltag *[,\\n{]/bfound"
            done }
         else
            echo "/^ *$_xmltag *[ ,\\n{]/bfound"
            echo "/[ ,\\n] *$_xmltag *[ ,\\n{]/bfound"
         fi
      done }
      echo "d" ; echo ":found" 
      for x in 1 2 3 4 5 6 7 8 9 ; do echo "/}/!N" ; done
      $SED "/^[$AZ$az$NN]/!d" "$S" | { while read xmltag ; do
	 xmltag=`echo "$xmltag" | sed -e "s/ .*//"`
         if echo " $HTMLTAGS $HTMLTAGS2" | grep " $xmltag " > $NULL ; then
           continue # keep html tags
         fi
         echo "s|^\\( *\\)\\($xmltag *[ ,\\n{]\\)|\\1.\\2|g"
         echo "s|\\([ ,\\n] *\\)\\($xmltag *[ ,\\n{]\\)|\\1.\\2|g"
      done }          
   } > "$R"
}

css_xmltags_css () # $SOURCEFILE
{
   X=`echo $SOURCEFILE | sed -e "y:/:~:"`
   S="$tmp/$MK.$X.xmltags.tmp.sed"
   R="$tmp/$MK.$X.xmltags.tmp.css"
   {
      cat "$tmp/$MK.$X.xmlstylesheets.tmp.txt" | { while read xmlstylesheet ; do
         stylesheet=`css_sourcefile "$xmlstylesheet"`
         if test -f "$stylesheet" ; then
            echo "/* $xmlstylesheet */"
            cat "$stylesheet" | $SED -f "$S"
         else
            error "$xmlstylesheet : ERROR, no such stylesheet"
         fi
      done }
   } > "$R"
}

css_xmlmapping_sed () # $SOURCEFILE
{
   X=`echo $SOURCEFILE | sed -e "y:/:~:"`
   S="$tmp/$MK.$X.xmltags.tmp.txt"
   R="$tmp/$MK.$X.xmlmapping.tmp.sed"
   rm -f "$R"
   {
      for x in 1 2 3 4 5 6 7 8 9 ; do echo "/}/d" ; echo "/{/!N" ; done
      echo "s|\\r||g"
      $SED "/^[$AZ$az$NN]/!d" "$S" | { while read xmltag ; do
         xmltag=`echo "$xmltag" | sed -e "s/ .*//"`
	 xmltag=`sed_slash_key "$xmltag"`
         echo "/^ *\\.$xmltag *[ ,\\n{]/bfound"
         echo "/[ ,\\n] *\\.$xmltag *[,\\n{]/bfound"
      done }
      echo "d" ; echo ":found" 
      for x in 1 2 3 4 5 6 7 8 9 ; do echo "/}/!N" ; done
      echo "s/^/>>/"
      echo "/[\\n ]display *: *list-item/s|^.*>>|li>>|"
      echo "/[\\n ]display *: *table-caption/s|^.*>>|caption>>|"
      echo "/[\\n ]display *: *table-cell/s|^.*>>|td>>|"
      echo "/[\\n ]display *: *table-row/s|^.*>>|tr>>|"
      echo "/[\\n ]display *: *table/s|^.*>>|table>>|"
      echo "/[\\n ]display *: *block/s|^.*>>|div>>|"
      echo "/[\\n ]display *: *inline/s|^.*>>|span>>|"
      echo "/[\\n ]display *: *none/s|^.*>>|small>>|"
      echo "/^div>>.*[\\n ]list-style-type *: *disc/s|^.*>>|ul>>|"
      echo "/^div>>.*[\\n ]list-style-type *: *decimal/s|^.*>>|ol>>|"
      echo "/^span>>.*[\\n ]font-family *: *monospace/s|^.*>>|tt>>|"
      echo "/^span>>.*[\\n ]font-style *: *italic/s|^.*>>|em>>|"
      echo "/^span>>.*[\\n ]font-weight *: *bold/s|^.*>>|b>>|"
      echo "/^div>>.*[\\n ]white-space *: *pre/s|^.*>>|pre>>|"
      echo "/^div>>.*[\\n ]margin-left *: *[$NN]/s|^.*>>|blockquote>>|"
      $SED "/^[$AZ$az$NN]/!d" "$S" | { while read xmltag ; do
         xmltag=`echo "$xmltag" | sed -e "s/ .*//"`
         echo "s|^\\(.*\\)>> *\\.$xmltag *[ ,\\n{].*|\\1 .$xmltag|"
         echo "s|^\\(.*\\)>>.*[ ,\\n] *\\.$xmltag *[ ,\\n{].*|\\1 .$xmltag|"
      done }
      echo "s/^div \\.para\$/p .para/"
      echo "s/^span \\.ulink\$/a .ulink/"
   } > "$R"
}

css_xmlmapping () # $SOURCEFILE
{
   X=`echo $SOURCEFILE | sed -e "y:/:~:"`
   cat     "$tmp/$MK.$X.xmltags.tmp.css" | \
   $SED -f "$tmp/$MK.$X.xmlmapping.tmp.sed" \
         > "$tmp/$MK.$X.xmlmapping.tmp.txt"
}

css_scan() # $SOURCEFILE
{
    css_xmltags
    css_xmlstyles
    css_xmlstyles_sed
    css_xmltags_css
    css_xmlmapping_sed
    css_xmlmapping
}

tags2span_sed() # $SOURCEFILE > $++
{
   X=`echo $SOURCEFILE | sed -e "y:/:~:"`
   S="$tmp/$MK.$X.xmltags.tmp.txt"
   R="$tmp/$MK.$X.xmltags.tmp.css"
   echo "s|<[?]xml-stylesheet[^<>]*[?]>||"
   echo "s|<link  *rel=['\"]*stylesheet[^<>]*>||"
   echo "s|<section[^<>]*>||g"
   echo "s|</section>||g" 
   $SED "/^[$AZ$az$NN]/!d" "$S" | { while read xmltag ; do 
      # note "xmltag=$xmltag"
      xmltag=`echo "$xmltag" | sed -e "s/ .*//"`
      if echo " $HTMLTAGS $HTMLTAGS2" | grep " $xmltag " > $NULL ; then
        continue # keep html tags
      fi
      _xmltag=`sed_slash_key "$xmltag"`
      _span_=`$SED -e "/ \\.$_xmltag\$/!d" -e "s/ .*//" -e q \
                  < "$tmp/$MK.$X.xmlmapping.tmp.txt"`
      test ".$_span_" = "." && _span_="span"
      _xmltag=`sed_piped_key "$xmltag"`
      echo "s|<$xmltag\\([\\n\\t ][^<>]*\\)url=|<$_span_ class=\"$xmltag\"\\1href=|g"
      echo "s|<$xmltag\\([\\n\\t >]\\)|<$_span_ class=\"$xmltag\"\\1|g"
      echo "s|</$xmltag\\([\\n\\t >]\\)|</$_span_\\1|g"
   done }   
   cat "$tmp/$MK.$X.xmlstylesheets.tmp.txt" | { while read xmlstylesheet ; do
      if test -f "$xmlstylesheet" ; then
         R="[^<>]*href=['"'"'"]$xmlstylesheet['"'"'"][^<>]*"
         echo "s|<[?]xml-stylesheet$R>||"
         echo "s|<link[^<>]* rel=['"'"'"]*stylesheet['"'"'" ]$R>||"
      fi
   done }
}

tags2meta_sed() # $SOURCEFILE > $++
{
   X=`echo $SOURCEFILE | sed -e "y:/:~:"`
   S="$tmp/$MK.$X.xmlstylesheets.tmp.txt"
   R="$tmp/$MK.$X.xmltags.tmp.css"
   cat "$tmp/$MK.$X.xmlstylesheets.tmp.txt" | { while read xmlstylesheet ; do
      if test -f "$xmlstylesheet" ; then
         echo " <style type=\"text/css\"><!--"
         $SED -e "s/^/  /" < "$R"
         echo " --></style>"
         break
      fi
   done }
}

# ==========================================================================
# xml/docbook support is taking an dbk input file converting any html    DBK
# syntax into pure docbook tagging. Each file is being given a docbook
# doctype so that an xml/docbook viewer can render it correctly - that
# is needed atleast since docbook files do not embed stylesheet infos.
# Most of the processing is related to remap html markup and some other
# shortcut markup into correct docbook markup. The result is NOT checked
# for being well-formed or even matching the docbook schema DTD at all.

scan_xml_rootnode ()
{
  rootnode=`cat "$SOURCEFILE" | \
     $SED -e "/<[$AZ$az$NN]/!d" -e "s/<\\([$AZ$az$NN]*\\).*/\\1/" -e q`  
  echo "<$Q'root'>$F $rootnode<$QX>" >> "$MK_DATA"
}

get_xml_rootnode ()
{
  _file_=`sed_slash_key "$F"`
  $SED -e "/^<$Q'root'>$_file_ /!d" \
       -e "s|.* ||" -e "s|<.*||" -e q "$MK_DATA" # +
}

xml_sourcefile ()  
{
    _XMLFILE_=`echo "$1" | $SED -e "s/\\.xml\\$/.dbk/"`
    _SRCFILE_=`echo "$1" | $SED -e "s/\\.xml\\$/.htm/"`
    test "$1" = "$_XMLFILE_" && _XMLFILE_="///"
    test "$1" = "$_SRCFILE_" && _SRCFILE_="///"
    if test -f "$_XMLFILE_" 
    then echo    "$_XMLFILE_" # $++
    elif test -f "$_SRCFILE_" 
    then echo  "$_SRCFILE_" # $++
    elif test -f "$opt_src_dir/$_XMLFILE_" 
    then echo    "$opt_src_dir/$_XMLFILE_" # $++
    elif test -f "$opt_src_dir/$_SRCFILE_" 
    then echo    "$opt_src_dir/$_SRCFILE_" # $++
    else echo ".//$_XMLFILE_" # $++ (not found?)
    fi
}

scan_xmlfile()
{
   SOURCEFILE=`xml_sourcefile "$F"`
   $hint "'$SOURCEFILE': scanning xml -> '$F'" 
   scan_xml_rootnode
   rootnode=`get_xml_rootnode | sed -e "/^h[$NN]/s|\$| <?section?>|"`
   $hint "'$SOURCEFILE': rootnode ('$rootnode')" 
}

make_xmlfile()
{
   SOURCEFILE=`xml_sourcefile "$F"`
   X=`echo $SOURCEFILE | sed -e "y:/:~:"`
   article=`get_xml_rootnode`
   test ".$article" = "." && article="article"
   echo '<!DOCTYPE '$article' PUBLIC "-//OASIS//DTD DocBook XML V4.4//EN"' \
        > "$F"
   echo  '    "http://www.oasis-open.org/docbook/xml/4.4/docbookx.dtd">' \
       >> "$F"
   cat "$tmp/$MK.$X.xmlstylesheets.tmp.txt" | { while read stylesheet ; do
       echo "<?xml-stylesheet type=\"text/css\" href=\"$stylesheet\" ?>" \
           >> "$F"
   done }
   __secinfo="\\1<sectioninfo>\\2</sectioninfo>"
   cat "$SOURCEFILE" | $SED \
	-e "s!<>!\&nbsp\;!g" \
	-e "s!\\(&\\)\\(&\\)!\\1amp;\\2amp;!g" \
	-e "s!\\(<[^<>]*\\)\\(width\\)\\(=\\)\\([$NN]*\%*\\)!\\1\\2\\3\"\\4\"!g" \
	-e "s!\\(<[^<>]*\\)\\(cellpadding\\)\\(=\\)\\([$NN]*\%*\\)!\\1\\2\\3\"\\4\"!g" \
	-e "s!\\(<[^<>]*\\)\\(border\\)\\(=\\)\\([$NN]*\%*\\)!\\1\\2\\3\"\\4\"!g" \
	-e "s!<[?]xml-stylesheet[^<>]*>!!" \
	-e "s!<link[^<>]* rel=[\'\"]*stylesheet[^<>]*>!!" \
	-e "s!<[hH][$NN]!<title!g" \
	-e "s!</[hH][$NN]!</title!g" \
	-e "s!\\(</title> *\\)\\([^<>]*[$AZ$az$NN][^<>\r\n]*\\)\$!\\1<sub>\\2</sub>!" \
	-e "s!\\(</title>.*\\)<sub>!\\1<subtitle>!g" \
	-e "s!\\(</title>.*\\)</sub>!\\1</subtitle>!g" \
	-e "s!\\(<section>[^<>]*\\)\\(<date>.*</date>[^<>]*\\)\$!\\1<sectioninfo>\\2</sectioninfo>!g" \
        -e "s!<em>!<emphasis>!g" \
        -e "s!</em>!</emphasis>!g" \
        -e "s!<i>!<emphasis>!g" \
        -e "s!</i>!</emphasis>!g" \
        -e "s!<b>!<emphasis role=\"bold\">!g" \
        -e "s!</b>!</emphasis>!g" \
        -e "s!<u>!<emphasis role=\"underline\">!g" \
        -e "s!</u>!</emphasis>!g" \
        -e "s!<big>!<emphasis role=\"strong\">!g" \
        -e "s!</big>!</emphasis>!g" \
        -e "s!<\\(strike\\)>!<emphasis role=\"strikethrough\">!g" \
        -e "s!<\\(s\\)>!<emphasis role=\"strikethrough\">!g" \
        -e "s!</\\(strike\\)>!</emphasis>!g" \
        -e "s!</\\(s\\)>!</emphasis>!g" \
        -e "s!<center>!<blockquote><para>!g" \
        -e "s!</center>!</para></blockquote>!g" \
        -e "s!<p align=\\(\"[$AZ$az$NN]*\"\\)>!<para role=\\1>!g" \
        -e "s!<[pP]>!<para>!g" \
        -e "s!</[pP]>!</para>!g" \
        -e "s!<\\(pre\\)>!<screen>!g" \
        -e "s!<\\(PRE\\)>!<screen>!g" \
        -e "s!</\\(pre\\)>!</screen>!g" \
        -e "s!</\\(PRE\\)>!</screen>!g" \
        -e "s!<a\\( [^<>]*\\)name=\\([^<>]*\\)/>!<anchor \\1id=\\2/>!g" \
        -e "s!<a\\( [^<>]*\\)name=\\([^<>]*\\)>!<anchor \\1id=\\2/>!g" \
        -e "s!<a\\( [^<>]*\\)href=!<ulink\\1url=!g" \
        -e "s!</a>!</ulink>!g" \
	-e "s! remap=\"url\">[^<>]*</ulink>! />!g" \
	-e "s!<\\(/*\\)span\\([ 	][^<>]*\\)>!<\\1phrase\\2>!g" \
	-e "s!<\\(/*\\)span>!<\\1phrase>!g" \
	-e "s!<small\\([ 	][^<>]*\\)>!<phrase role=\"small\"\\1>!g" \
	-e "s!<small>!<phrase role=\"small\">!g" \
	-e "s!</small>!</phrase>!g" \
	-e "s!<\\(/*\\)\\(sup\\)>!<\\1superscript>!g" \
	-e "s!<\\(/*\\)\\(sub\\)>!<\\1subscript>!g" \
	-e "s!\\(<\\)\\(li\\)\\(><\\)!\\1listitem\\3!g" \
	-e "s!\\(></\\)\\(li\\)\\(>\\)!\\1listitem\\3!g" \
	-e "s!\\(<\\)\\(li\\)\\(>\\)!\\1listitem\\3<para>!g" \
	-e "s!\\(</\\)\\(li\\)\\(>\\)!</para>\\1listitem\\3!g" \
	-e "s!\\(</*\\)\\(ul\\)>!\\1itemizedlist>!g" \
	-e "s!\\(</*\\)\\(ol\\)>!\\1orderedlist>!g" \
	-e "s!\\(</*\\)\\(dl\\)>!\\1variablelist>!g" \
	-e "s!<\\(/*\\)DT>!<\\1dt>!g" \
	-e "s!<\\(/*\\)DD>!<\\1dd>!g" \
	-e "s!<\\(/*\\)DL>!<\\1dl>!g" \
	-e "s!<BLOCKQUOTE>!<blockquote><para>!g" \
	-e "s!</BLOCKQUOTE>!</para></blockquote>!g" \
	-e "s!<\\(/*\\)dl>!<\\1variablelist>!g" \
	-e "s!<dt\\( [^<>]*\\)>!<varlistentry\\1><term>!g" \
	-e "s!<dt>!<varlistentry><term>!g" \
	-e "s!</dt>!</term>!g" \
	-e "s!<dd\\( [^<>]*\\)><!<listitem\\1><!g" \
	-e "s!<dd><!<listitem><!g" \
	-e "s!></dd>!></listitem></varlistentry>!g" \
	-e "s!<dd\\( [^<>]*\\)>!<listitem\\1><para>!g" \
	-e "s!<dd>!<listitem><para>!g" \
	-e "s!</dd>!</para></listitem></varlistentry>!g" \
	-e "s!<table[^<>]*><tr><td>\\(<table[^<>]*>\\)!\\1!" \
	-e "s!\\(</table>\\)</td></tr></table>!\\1!" \
	-e "s!<table\\( [^<>]*\\)>!<informaltable\\1><tgroup cols=\"2\"><tbody>!g" \
	-e "s!<table>!<informaltable><tgroup cols=\"2\"><tbody>!g" \
	-e "s!</table>!</tbody></tgroup></informaltable>!g" \
	-e "s!\\(</*\\)tr\\([ 	][^<>]*\\)>!\\1row\\2>!g" \
	-e "s!\\(</*\\)tr>!\\1row>!g" \
	-e "s!\\(</*\\)td\\([ 	][^<>]*\\)>!\\1entry\\2>!g" \
	-e "s!\\(</*\\)td>!\\1entry>!g" \
	-e "s!\\(<informaltable[^<>]*[ 	]width=\"100\%\"\\)!\\1 pgwide=\"1\"!g" \
	-e "s!\\(<tgroup[<>]*[ 	]cols=\"2\">\\)\\(<tbody>\\)!\\1<colspec colwidth=\"1*\" /><colspec colwidth=\"1*\" />\\2!g" \
	-e "s!\\(<entry[^<>]*[ 	]\\)width=\\(\"[$NN]*\%*\"\\)!\\1remap=\\2!g" \
	-e "s!<nobr>\\([\'\`]*\\)<tt>!<cmdsynopsis><command>\\1!g" \
	-e "s!</tt>\\([\'\`]*\\)</nobr>!\\1</command></cmdsynopsis>!g" \
	-e "s!<nobr><\\(code\\)>\\([\`\"\']\\)!<cmdsynopsis><command>\\2!g" \
	-e "s!<\\(code\\)><nobr>\\([\`\"\']\\)!<cmdsynopsis><command>\\2!g" \
	-e "s!\\([\`\"\']\\)</\\(code\\)></nobr>!\\1</command></cmdsynopsis>!g" \
	-e "s!\\([\`\"\']\\)</nobr></\\(code\\)>!\\1</command></cmdsynopsis>!g" \
	-e "s!<nobr><\\(tt\\)>\\([\`\"\']\\)!<cmdsynopsis><command>\\2!g" \
	-e "s!<\\(tt\\)><nobr>\\([\`\"\']\\)!<cmdsynopsis><command>\\2!g" \
	-e "s!\\([\`\"\']\\)</\\(tt\\)></nobr>!\\1</command></cmdsynopsis>!g" \
	-e "s!\\([\`\"\']\\)</nobr></\\(tt\\)>!\\1</command></cmdsynopsis>!g" \
	-e "s!\\(</*\\)tt>!\\1constant>!g" \
	-e "s!\\(</*\\)code>!\\1literal>!g" \
	-e "s!<br>!<br />!g" \
	-e "s!<br */>!<screen role=\"linebreak\">\n</screen>!g" \
       >> "$F"
   echo "'$SOURCEFILE': " `ls -s $SOURCEFILE` ">>" `ls -s $F`
}

make_xmlmaster ()
{
   SOURCEFILE=`xml_sourcefile "$F"`
   X=`echo $SOURCEFILE | sed -e "y:/:~:"`
   article="section" # book? chapter?
   echo '<!DOCTYPE' $article 'PUBLIC "-//OASIS//DTD DocBook XML V4.4//EN"' >$F
   echo '    "http://www.oasis-open.org/docbook/xml/4.4/docbookx.dtd">' >>$F
   cat "$tmp/$MK.$X.xmlstylesheets.tmp.txt" | { while read stylesheet ; do
       echo "<?xml-stylesheet type=\"text/css\" href=\"$stylesheet\" ?>" \
           >> "$F"
   done }
   echo "<section><title>Documentation</title>" >>$F
   make_xmlsitemap >> $F
   echo "</section>" >> $F
   echo "'$SOURCEFILE': " `ls -s $SOURCEFILE` ">*>" `ls -s $F`
}

# ==========================================================================
#  
#  During processing we will create a series of intermediate files that
#  store relations. They all have the same format being
#   =relationtype=key value
#  where key is usually s filename or an anchor. For mere convenience
#  we assume that the source html text does not have lines that start
#  off with =xxxx= (btw, ye remember perl section notation...). Of course
#  any other format would be usuable as well.
#

# we scan the SITEFILE for href references to be converted
# - in the new variant we use a ".gets.tmp" sed script that            SECTS
# marks all interesting lines so they can be checked later
# with an sed anchor of sect="[$NN]" (or sect="[$AZ]")
S="\\&nbsp\\;"
# S="[&]nbsp[;]"

# HR and EM style markups must exist in input - BR sometimes left out 
# these routines in(ter)ject hardspace before, between, after markups
# note that "<br>" is sometimes used with HR - it must exist in input
echo_HR_EM_PP ()
{
    echo "s%^\\($1$2$3*<a\\) \\(href=\\)%\\1 $4 \\2%"
    echo "s%^\\(<>$1$2$3*<a\\) \\(href=\\)%\\1 $4 \\2%"
    echo "s%^\\($S$1$2$3*<a\\) \\(href=\\)%\\1 $4 \\2%"
    echo "s%^\\($1<>$2$3*<a\\) \\(href=\\)%\\1 $4 \\2%"
    echo "s%^\\($1$S$2$3*<a\\) \\(href=\\)%\\1 $4 \\2%"
    echo "s%^\\($1$2<>$3*<a\\) \\(href=\\)%\\1 $4 \\2%"
    echo "s%^\\($1$2$S$3*<a\\) \\(href=\\)%\\1 $4 \\2%"
}

echo_br_EM_PP ()
{
    echo_HR_EM_PP  "$1" "$2" "$3" "$4"
    echo "s%^\\($2$3*<a\\) \\(href=\\)%\\1 $4 \\2%"
    echo "s%^\\(<>$2$3*<a\\) \\(href=\\)%\\1 $4 \\2%"
    echo "s%^\\($S$2$3*<a\\) \\(href=\\)%\\1 $4 \\2%"
    echo "s%^\\($2<>$3*<a\\) \\(href=\\)%\\1 $4 \\2%"
    echo "s%^\\($2$S$3*<a\\) \\(href=\\)%\\1 $4 \\2%"
    echo "s%^\\($2$3*<><a\\) \\(href=\\)%\\1 $4 \\2%"
    echo "s%^\\($2$3*$S<a\\) \\(href=\\)%\\1 $4 \\2%"
}    

echo_HR_PP ()
{
    echo "s%^\\($1<a\\) \\(href=\\)%\\1 $3 \\2%"
    echo "s%^\\($1$2*<a\\) \\(href=\\)%\\1 $3 \\2%"
    echo "s%^\\(<>$1$2*<a\\) \\(href=\\)%\\1 $3 \\2%"
    echo "s%^\\($S$1$2*<a\\) \\(href=\\)%\\1 $3 \\2%"
    echo "s%^\\($1<>$2*<a\\) \\(href=\\)%\\1 $3 \\2%"
    echo "s%^\\($1$S$2*<a\\) \\(href=\\)%\\1 $3 \\2%"
}
echo_br_PP ()
{
    echo_HR_PP "$1" "$2" "$3"
    echo "s%^\\($2*<a\\) \\(href=\\)%\\1 $3 \\2%"
    echo "s%^\\(<>$2*<a\\) \\(href=\\)%\\1 $3 \\2%"
    echo "s%^\\($S$2*<a\\) \\(href=\\)%\\1 $3 \\2%"
}
echo_sp_PP ()
{
    echo "s%^\\(<>$1*<a\\) \\(href=\\)%\\1 $2 \\2%"
    echo "s%^\\($S$1*<a\\) \\(href=\\)%\\1 $2 \\2%"
    echo "s%^\\(<><>$1*<a\\) \\(href=\\)%\\1 $2 \\2%"
    echo "s%^\\($S$S$1*<a\\) \\(href=\\)%\\1 $2 \\2%"
    echo "s%^\\(<>$1<>*<a\\) \\(href=\\)%\\1 $2 \\2%"
    echo "s%^\\($S$1$S*<a\\) \\(href=\\)%\\1 $2 \\2%"
    echo "s%^\\($1<><>*<a\\) \\(href=\\)%\\1 $2 \\2%"
    echo "s%^\\($1$S$S*<a\\) \\(href=\\)%\\1 $2 \\2%"
    echo "s%^\\($1<>*<a\\) \\(href=\\)%\\1 $2 \\2%"
    echo "s%^\\($1$S*<a\\) \\(href=\\)%\\1 $2 \\2%"
}

echo_sp_SP ()
{
    echo "s%^\\($1<a\\) \\(href=\\)%\\1 $2 \\2%"
    echo "s%^\\(<>$1<a\\) \\(href=\\)%\\1 $2 \\2%"
    echo "s%^\\($S$1<a\\) \\(href=\\)%\\1 $2 \\2%"
    echo "s%^\\(<><>$1<a\\) \\(href=\\)%\\1 $2 \\2%"
    echo "s%^\\($S$S$1<a\\) \\(href=\\)%\\1 $2 \\2%"
    echo "s%^\\(<>$1<><a\\) \\(href=\\)%\\1 $2 \\2%"
    echo "s%^\\($S$1$S<a\\) \\(href=\\)%\\1 $2 \\2%"
    echo "s%^\\($1<><><a\\) \\(href=\\)%\\1 $2 \\2%"
    echo "s%^\\($1$S$S<a\\) \\(href=\\)%\\1 $2 \\2%"
    echo "s%^\\($1<><a\\) \\(href=\\)%\\1 $2 \\2%"
    echo "s%^\\($1$S<a\\) \\(href=\\)%\\1 $2 \\2%"
}

echo_sp_sp ()
{
    echo "s%^\\($1<a\\) \\(name=\\)%\\1 $2 \\2%"
    echo "s%^\\(<>$1<a\\) \\(name=\\)%\\1 $2 \\2%"
    echo "s%^\\($S$1<a\\) \\(name=\\)%\\1 $2 \\2%"
    echo "s%^\\(<><>$1<a\\) \\(name=\\)%\\1 $2 \\2%"
    echo "s%^\\($S$S$1<a\\) \\(name=\\)%\\1 $2 \\2%"
    echo "s%^\\(<>$1<><a\\) \\(name=\\)%\\1 $2 \\2%"
    echo "s%^\\($S$1$S<a\\) \\(name=\\)%\\1 $2 \\2%"
    echo "s%^\\($1<><><a\\) \\(name=\\)%\\1 $2 \\2%"
    echo "s%^\\($1$S$S<a\\) \\(name=\\)%\\1 $2 \\2%"
    echo "s%^\\($1<><a\\) \\(name=\\)%\\1 $2 \\2%"
    echo "s%^\\($1$S<a\\) \\(name=\\)%\\1 $2 \\2%"
}

make_sitemap_init()
{
    # build a list of detectors that map site.htm entries to a section table
    # note that the resulting .gets.tmp / .puts.tmp are real sed-script
    h1="[-$AP$AK]"
    b1="[*=]"
    b2="[-$AP$AK]"
    b3="[:/]"
    q3="[:/,$AK]"
    echo_HR_PP    "<hr>"            "$h1"    "sect=\"1\""      > "$MK_GETS"
    echo_HR_EM_PP "<hr>" "<em>"     "$h1"    "sect=\"1\""     >> "$MK_GETS"
    echo_HR_EM_PP "<hr>" "<strong>" "$h1"    "sect=\"1\""     >> "$MK_GETS"
    echo_HR_PP    "<br>"            "$b1$b1" "sect=\"1\""     >> "$MK_GETS"
    echo_HR_PP    "<br>"            "$b2$b2" "sect=\"2\""     >> "$MK_GETS"
    echo_HR_PP    "<br>"            "$b3$b3" "sect=\"3\""     >> "$MK_GETS"
    echo_br_PP    "<br>"            "$b2$b2" "sect=\"2\""     >> "$MK_GETS"
    echo_br_PP    "<br>"            "$b3$b3" "sect=\"3\""     >> "$MK_GETS"
    echo_br_EM_PP "<br>" "<small>"  "$q3"    "sect=\"3\""     >> "$MK_GETS"
    echo_br_EM_PP "<br>" "<em>"     "$q3"    "sect=\"3\""     >> "$MK_GETS"
    echo_br_EM_PP "<br>" "<u>"      "$q3"    "sect=\"3\""     >> "$MK_GETS"
    echo_HR_PP    "<br>"            "$q3"    "sect=\"3\""     >> "$MK_GETS"
    echo_br_PP    "<u>"             "$b2"    "sect=\"2\""     >> "$MK_GETS"
    echo_sp_PP                      "$q3"    "sect=\"3\""     >> "$MK_GETS"
    echo_sp_SP                      ""       "sect=\"2\""     >> "$MK_GETS"
    echo_sp_sp                      "$q3"    "sect=\"9\""     >> "$MK_GETS"
    echo_sp_sp    "<br>"                     "sect=\"9\""     >> "$MK_GETS"
    $SED -e "s/\\(>\\)\\(\\[\\)/\\1 *\\2/" "$MK_GETS" > "$MK_PUTS"
    # the .puts.tmp variant is used to <b><a href=..></b> some hrefs which
    # shall not be used otherwise for being generated - this is nice for
    # some quicklinks somewhere. The difference: a whitspace "<hr> <a...>"
    echo "" > "$MK_DATA" # fresh start
}

_uses_="<$Q'use\\1'>\\2 \\3<$QX>" 
_name_="<$Q'use\\1'>name:\\2 \\3<$QX>" ; 

make_sitemap_list()
{
    _sitefile_="$1" ; test ".$_sitefile_" = "." && _sitefile_="$SITEFILE"
    # scan sitefile for references pages - store as "=use+=href+ anchortext"
    $SED -f "$MK_GETS"           -e "/<a sect=\"[$NN]\"/!d" \
	-e "s|.*<a sect=\"\\([^\"]*\\)\" href=\"\\([^\"]*\\)\"[^<>]*>\\(.*\\)</a>.*|$_uses_|" \
	-e "s|.*<a sect=\"\\([^\"]*\\)\" name=\"\\([^\"]*\\)\"[^<>]*>\\(.*\\)</a>.*|$_name_|" \
	-e "s|.*<a sect=\"\\([^\"]*\\)\" name=\"\\([^\"]*\\)\"[^<>]*>\\(.*\\)|$_name_|" \
	-e "/^<$Q/!d" -e "/^<!/d" \
           "$_sitefile_" >> "$MK_DATA"
}

_Uses_="<$Q'Use\\1'>\\2 \\3<$QX>" 
_Name_="<$Q'Use\\1'>name:\\2 \\3<$QX>" ; 

make_subsitemap_list()
{
    _sitefile_="$1" ; test ".$_sitefile_" = "." && _sitefile_="$SITEFILE"
    # scan sitefile for references pages - store as "=use+=href+ anchortext"
    $SED -f "$MK_GETS"           -e "/<a sect=\"[$NN]\"/!d" \
	-e "s|.*<a sect=\"\\([^\"]*\\)\" href=\"\\([^\"]*\\)\"[^<>]*>\\(.*\\)</a>.*|$_Uses_|" \
	-e "s|.*<a sect=\"\\([^\"]*\\)\" name=\"\\([^\"]*\\)\"[^<>]*>\\(.*\\)</a>.*|$_Name_|" \
	-e "s|.*<a sect=\"\\([^\"]*\\)\" name=\"\\([^\"]*\\)\"[^<>]*>\\(.*\\)|$_Name_|" \
	-e "/^<$Q/!d" -e "/^<!/d" \
        -e "s|>\\([^:./][^:./]*[./]\\)|>$2\\1|" \
           "$_sitefile_" >> "$MK_DATA"
}

make_sitemap_sect() 
{
    # scan used pages and store prime section group relation 'sect' and 'node'
    # (A) each "use1" creates "'sect'>href+ href1" for all following non-"use1"
    # (B) each "use1" creates "'node'>href2 href1" for all following "use2"
    $SED -e "/^<$Q'use.'>/!d" \
         -e "/^<$Q'use1'>/{" \
           -e "h" -e "s|^<$Q'use1'>\\([^ ]*\\) .*|\\1|" \
           -e "x" -e "}" \
         -e "s|^<$Q'use.'>\\([^ ]*\\) .*|<$Q'sect'>\\1|" \
         -e G -e "s|\\n| |" -e "s|\$|<$QX>|" "$MK_DATA" >> "$MK_DATA"
    $SED -e "/^<$Q'use.'>/!d" \
         -e "/^<$Q'use1'>/{" \
           -e "h" -e "s|^<$Q'use1'>\\([^ ]*\\) .*|\\1|" \
           -e "x" -e "}" \
         -e "/^<$Q'use[13456789]'>/d" \
         -e "s|<$Q'use.'>\\([^ ]*\\) .*|<$Q'node'>\\1|" \
         -e G -e "s|\\n| |" -e "s|\$|<$QX>|" "$MK_DATA" >> "$MK_DATA"
}

make_sitemap_page()
{
    # scan used pages and store secondary group relation 'page' and 'node'
    # the parenting 'node' for use3 is usually a use2 (or use1 if none there)
    $SED -e "/^<$Q'use.'>/!d" \
         -e "/^<$Q'use1'>/{" \
            -e "h" -e "s|^<$Q'use1'>\\([^ ]*\\) .*|\\1|" \
            -e "x" -e "}" \
         -e "/^<$Q'use2'>/{" \
            -e "h" -e "s|^<$Q'use2'>\\([^ ]*\\) .*|\\1|" \
            -e "x" -e "}" \
         -e "/^<$Q'use1'>/d" \
         -e "s|^<$Q'use.'>\\([^ ]*\\) .*|<$Q'page'>\\1<$QX>|" \
         -e G -e "s|\\n| |" "$MK_DATA" >> "$MK_DATA"
    $SED -e "/^<$Q'use.'>/!d" \
         -e "/^<$Q'use1'>/{" \
            -e "h" -e "s|^<$Q'use1'>\\([^ ]*\\) .*|\\1|" \
            -e "x" -e "}" \
         -e "/^<$Q'use2'>/{" \
            -e "h" -e "s|^<$Q'use2'>\\([^ ]*\\) .*|\\1|" \
            -e "x" -e "}" \
         -e "/^<$Q'use[12456789]'>/d" \
         -e "s|^<$Q'use.'>\\([^ ]*\\) .*|<$Q'node'>\\1<$QX>|" \
         -e G -e "s|\\n| |" "$MK_DATA" >> "$MK_DATA"
    # and for the root sections we register ".." as the parenting group
    $SED -e "/^<$Q'use1'>/!d" \
         -e "s|^<$Q'use.'>\\([^ ]*\\) .*|<$Q'node'>\\1 ..<$QX>|"  "$MK_DATA" >> "$MK_DATA"
}

echo_site_filelist()
{
    $SED -e "/^<$Q'use.'>/!d" \
         -e "s|^<$Q'use.'>||" -e "s| .*||" "$MK_DATA"
}

# ==========================================================================
# originally this was a one-pass compiler but the more information
# we were scanning out the more slower the system ran - since we
# were rescanning files for things like section information. Now
# we scan the files first for global information.
#                                                                    1.PASS

scan_sitefile () # $F
{
 SOURCEFILE=`html_sourcefile "$F"`
 $hint "'$SOURCEFILE': scanning -> sitefile"
 if test "$SOURCEFILE" != "$F" ; then
   dx_init "$F"
   dx_text today "`timetoday`"
   short=`echo "$F" | $SED -e "s:.*/::" -e "s:[.].*::"` # basename for all exts
   short="$short ~"
   DC_meta title "$short"
   DC_meta date.available "`timetoday`"
   DC_meta subject sitemap
   DC_meta DCMIType Collection
   DC_VARS_Of "$SOURCEFILE"  ; HTTP_VARS_Of "$SOURCEFILE"
   DC_modified "$SOURCEFILE" ; DC_date "$SOURCEFILE"
   DC_section "$F"
   DX_text date.formatted `timetoday`
   test ".$printerfriendly" != "." && \
   DX_text "printerfriendly" `fast_html_printerfile "$F"`
   test ".$USER" != "." && DC_publisher "$USER"
   echo "'$SOURCEFILE': $short (sitemap)"
   site_map_list_title "$F" "$short"
   site_map_long_title "$F" "generated sitemap index"
   site_map_list_date  "$F" "`timetoday`"
 fi
}

scan_htmlfile() # "$F"
{
 SOURCEFILE=`html_sourcefile "$F"`                                    # SCAN :
 $hint "'$SOURCEFILE': scanning -> $F"                                # HTML :
 if test "$SOURCEFILE" != "$F" ; then :
 if test -f "$SOURCEFILE" ; then
   dx_init "$F"
   dx_text today "`timetoday`"
   dx_text todays "`timetodays`"
   DC_VARS_Of "$SOURCEFILE" ; HTTP_VARS_Of "$SOURCEFILE"
   DC_title "$SOURCEFILE"
   DC_isFormatOf "$SOURCEFILE" 
   DC_modified "$SOURCEFILE" ; DC_date "$SOURCEFILE" ; DC_date "$SITEFILE"
   DC_section "$F" ;  DC_selected "$F" ;  DX_alternative "$SOURCEFILE"
   test ".$USER" != "." && DC_publisher "$USER"
   DX_text date.formatted "`timetoday`"
   test ".$printerfriendly" != "." && \
   DX_text "printerfriendly" `fast_html_printerfile "$F"`
   sectn=`info_get_entry DC.relation.section`
   short=`info_get_entry DC.title.selected`
   site_map_list_title "$F" "$short"
   info_map_list_title "$F" "$short"
   title=`info_get_entry DC.title`
   site_map_long_title "$F" "$title"
   info_map_long_title "$F" "$title"
   edate=`info_get_entry DC.date`
   issue=`info_get_entry issue`
   site_map_list_date "$F" "$edate"
   info_map_list_date "$F" "$edate"
   css_scan
   echo "'$SOURCEFILE':  '$title' ('$short') @ '$issue' ('$sectn')"
 else
   echo "'$SOURCEFILE': does not exist"
   site_map_list_title "$F" "$F"
   site_map_long_title "$F" "$F (no source)"
 fi ; else
   echo "<$F> - skipped"
 fi
}

scan_subsitemap_long ()
{
    grep "<a href=\"[^\"]*\">" "$1" | { 
	while read _line_ ; do
	    _href_=`echo "$_line_" | $SED -e "s|.*<a href=\"\\([^\"]*\\)\">.*|\\1|"`
	    _date_=`echo "$_line_" | $SED -e "s|.*<small style=\"date\">\\([^<>]*\\)</small>.*|\\1|" -e "/<a href=\"[^\"]*\">/d"`
	    _long_=`echo "$_line_" | $SED -e "s|.*<!--long-->\\([^<>]*\\)<!--/long-->.*|\\1|" -e "/<a href=\"[^\"]*\">/d"`
	    if test ".$_href_" != "." && test ".$_date_" != "." ; then
		site_map_list_date "$2$_href_" "$_date_"
	    fi
	    if test ".$_href_" != "." && test ".$_long_" != "." ; then
		site_map_long_title "$2$_href_" "$_long_"
	    fi
	done
    }
}

scan_namespec ()
{
    # nothing so far
    case "$1" in
	name:sitemap:*)
	    short=`echo "$F" | $SED -e "s:.*/::" -e "s:[.].*::"` 
	    short=`echo "$short ~" | $SED -e "s/name:sitemap://"` 
	    site_map_list_title "$F" "$short"
	    site_map_long_title "$F" "external sitemap index"
	    site_map_list_date  "$F" "`timetoday`"
	    echo "'$F' external sitemap index$n" 
	    ;;
	name:*.htm|name:*.html)
	    FF=`echo "$1" | $SED -e "s|name:||"`
	    FFF=`echo "$FF" | $SED -e "s|/[^/]*\$|/|"` # dirname
	    case "$FFF" in */*) : ;; *) FFF="" ;; esac
	    make_subsitemap_list "$FF" "$FFF"
	    scan_subsitemap_long "$FF" "$FFF"
	    ;;
    esac
}
scan_httpspec ()
{
    # nothing so far
    return;
}

skip_namespec ()
{
    # nothing so far
    return;
}
skip_httpspec ()
{
    # nothing so far
    return;
}

# ==========================================================================
# and now generate the output pages
#                                                                   2.PASS

head_sed_sitemap() # $filename $section
{
   FF=`sed_piped_key "$1"`
   SECTION=`sed_slash_key "$2"`
   SECTS="sect=\"[$NN$AZ]\"" ; SECTN="sect=\"[$NN]\"" # lines with hrefs
   echo "s|\\(<a $SECTS href=\"$FF\">.*</a>\\)|<b>\\1</b>|"          # $++
   test ".$sectiontab" != ".no" && \
   echo "/ href=\"$SECTION\"/s|^<td class=\"[^\"]*\"|<td |"    # $++
}

head_sed_listsection() # $filename $section
{
   # traditional.... the sitefile is the full navigation bar
   FF=`sed_piped_key "$1"`
   SECTION=`sed_slash_key "$2"`
   SECTS="sect=\"[$NN$AZ]\"" ; SECTN="sect=\"[$NN]\"" # lines with hrefs
   echo "s|\\(<a $SECTS href=\"$FF\">.*</a>\\)|<b>\\1</b>|"          # $++
   test ".$sectiontab" != ".no" && \
   echo "/ href=\"$SECTION\"/s|^<td class=\"[^\"]*\"|<td |"    # $++
}

head_sed_multisection() # $filename $section
{
   # sitefile navigation bar is split into sections
   FF=`sed_piped_key "$1"`
   SECTION=`sed_slash_key "$2"`
   SECTS="sect=\"[$NN$AZ]\"" ; SECTN="sect=\"[$NN]\"" # lines with hrefs
   # grep all pages with a class='sect' relation to current $SECTION and
   # build foreach an sed line "s|$SECTS\(<a href=$F>\)|<!--sectX-->\1|"
   # after that all the (still) numeric SECTNs are deactivated / killed.
   for section in $SECTION $headsection $tailsection ; do
       test ".$section" = ".no" && continue
   $SED -e "/^<$Q'sect'>[^ ]* $section/!d" \
        -e "s|<$Q'sect'>||" -e "s| .*||" \
        -e "s/.*/s|<a $SECTS \\\\(href=\"&\"\\\\)|<a sect=\"X\" \\\\1|/" \
        "$MK_DATA"  # $++
   $SED -e "/^<$Q'sect'>name:[^ ]* $section/!d" \
        -e "s|<$Q'sect'>name:||" -e "s| .*||" \
        -e "s/.*/s|<a $SECTS \\\\(name=\"&\"\\\\)|<a sect=\"X\" \\\\1|/" \
        "$MK_DATA"  # $++
   done
   echo "s|.*<a \\($SECTN href=[^<>]*\\)>.*|<!-- \\1 -->|"  # $++
   echo "s|.*<a \\($SECTN name=[^<>]*\\)>.*|<!-- \\1 -->|"  # $++
   echo "s|\\(<a $SECTS href=\"$FF\">\\)|<b>\\1</b>|"          # $++
   test ".$sectiontab" != ".no" && \
   echo "/ href=\"$SECTION\"/s|^<td class=\"[^\"]*\"|<td |"    # $++
}

make_sitefile () # "$F"
{
 SOURCEFILE=`html_sourcefile "$F"`
 if test "$SOURCEFILE" != "$F" ; then
 if test -f "$SOURCEFILE" ; then 
   # remember that in this case "${SITEFILE}l" = "$F" = "${SOURCEFILE}l"
   info2vars_sed > $MK_VARS           # have <!--title--> vars substituted
   info2meta_sed > $MK_META           # add <meta name="DC.title"> values
   F_HEAD="$tmp/$F.$HEAD" ; F_FOOT="$tmp/$F.$FOOT"
   $CAT "$MK_PUTS"                                    > "$F_HEAD"
   head_sed_sitemap "$F" "`info_get_entry_section`"  >> "$F_HEAD"
   echo "/<head>/r $MK_META"                         >> "$F_HEAD"
   $CAT "$MK_VARS" "$MK_TAGS"                        >> "$F_HEAD"
   echo "/<\\/body>/d"                               >> "$F_HEAD"
   case "$sitemaplayout" in
   multi) make_multisitemap > "$F_FOOT" ;;       # here we use ~foot~ to
   *)     make_listsitemap  > "$F_FOOT" ;;       # hold the main text
   esac

   mkpathfile "$F"
   $SED_LONGSCRIPT "$F_HEAD"               "$SITEFILE"  > $F   # ~head~
   $CAT            "$F_FOOT"                           >> $F   # ~body~
   $SED -e "/<\\/body>/!d" -f "$MK_VARS"   "$SITEFILE" >> $F   #</body>
   echo "'$SOURCEFILE': " `ls -s $SOURCEFILE` ">->" `ls -s $F` "(sitemap)"
 else
   echo "'$SOURCEFILE': does not exist"
 fi fi
}

make_htmlfile() # "$F"
{
 SOURCEFILE=`html_sourcefile "$F"`                      #     2.PASS
 if test "$SOURCEFILE" != "$F" ; then
 if test -f "$SOURCEFILE" ; then
   if grep '<meta name="formatter"' "$SOURCEFILE" > $NULL ; then
     echo "'$SOURCEFILE': SKIP, this sourcefile looks like a formatted file"
     echo "'$SOURCEFILE':  (may be a sourcefile in place of a targetfile?)"
     return
   fi
   info2vars_sed > $MK_VARS           # have <!--$title--> vars substituted
   info2meta_sed > $MK_META           # add <meta name="DC.title"> values
   tags2span_sed > $MK_SPAN           # extern text/css -> intern css classes
   tags2meta_sed >>$MK_META           # extern text/css -> intern css classes
   F_HEAD="$tmp/$F.$HEAD" ; F_BODY="$tmp/$F.$BODY" ; F_FOOT="$tmp/$F.$FOOT"
   $CAT "$MK_PUTS"                        > "$F_HEAD"
   case "$sectionlayout" in
   multi) head_sed_multisection "$F" "`info_get_entry_section`" >> "$F_HEAD" ;;
       *) head_sed_listsection  "$F" "`info_get_entry_section`" >> "$F_HEAD" ;;
   esac
      $CAT "$MK_VARS" "$MK_TAGS" "$MK_SPAN" >> "$F_HEAD" #tag and vars
      echo "/<\\/body>/d"                   >> "$F_HEAD" #cut lastline
      echo "/<head>/r $MK_META"             >> "$F_HEAD" #add metatags
      echo "/<title>/d"                      > "$F_BODY" #not that line
      $CAT "$MK_VARS" "$MK_TAGS" "$MK_SPAN" >> "$F_BODY" #tag and vars
      bodymaker_for_sectioninfo             >> "$F_BODY" #if sectioninfo
      info2body_sed                         >> "$F_BODY" #cut early
      info2head_sed                         >> "$F_HEAD"
      make_back_path "$F"                   >> "$F_HEAD"
      test ".$emailfooter" != ".no" && \
      body_for_emailfooter                   > "$F_FOOT"

      mkpathfile "$F"
      $SED_LONGSCRIPT "$F_HEAD" $SITEFILE                > $F # ~head~
      $SED_LONGSCRIPT "$F_BODY" $SOURCEFILE             >> $F # ~body~
      test -f "$F_FOOT" && $CAT "$F_FOOT"               >> $F # ~foot~
      $SED -e "/<\\/body>/!d" -f "$MK_VARS" "$SITEFILE" >> $F #</body>
   echo "'$SOURCEFILE': " `ls -s $SOURCEFILE` "->" `ls -s $F`
 else # test -f $SOURDEFILE
   echo "'$SOURCEFILE': does not exist"
 fi ; else
   echo "<$F> - skipped"
 fi
}

make_printerfriendly () # "$F"
{                                                                 # PRINTER
  printsitefile="0"                                               # FRIENDLY
  P=`html_printerfile "$F"`
  P_HEAD="$tmp/$P.$HEAD"
  P_BODY="$tmp/$P.$BODY"
  case "$F" in
  ${SITEFILE}|${SITEFILE}l)
          printsitefile=">=>" ; BODY_TXT="$tmp/$F.$FOOT"  ;;
  *.html) printsitefile="=>" ;  BODY_TXT="$SOURCEFILE" ;;
  esac
  if grep '<meta name="formatter"' "$BODY_TXT" > $NULL ; then return; fi
  if test ".$printsitefile" != ".0" && test -f "$SOURCEFILE" ; then
      make_printerfile_fast "$FILELIST" > ./$MK_FAST
      $CAT "$MK_VARS" "$MK_TAGS" "$MK_FAST" > "$P_HEAD"
      $SED -e "/DC.relation.isFormatOf/s|content=\"[^\"]*\"|content=\"$F\"|" \
           "$MK_META" > "$MK_METT"
      echo "/<head>/r $MK_METT"                       >> "$P_HEAD" # meta
      echo "/<\\/body>/d"                             >> "$P_HEAD"
      select_in_printsitefile "$F"                    >> "$P_HEAD"
      _ext_=`print_extension "$printerfriendly"`                   # head-
      # line_=`sed_slash_key "$printsitefile_img_2"`               # back-
      echo "/||topics:/s| href=\"[#][.]\"| href=\"$F\"|" >> "$P_HEAD"
      echo "/|||pages:/s| href=\"[#][.]\"| href=\"$F\"|" >> "$P_HEAD"
      make_back_path "$F"                             >> "$P_HEAD"
      $CAT "$MK_VARS" "$MK_TAGS" "$MK_FAST"            > "$P_BODY"
      make_back_path "$F"                             >> "$P_BODY"

      mkpathfile "$P"
      $SED_LONGSCRIPT "$P_HEAD"              $PRINTSITEFILE  > $P # ~head~
      $SED_LONGSCRIPT "$P_BODY"                   $BODY_TXT >> $P # ~body~
      $SED -e "/<\\/body>/!d" -f $MK_VARS $PRINTSITEFILE >> $P #</body>
   echo "'$SOURCEFILE': " `ls -s $SOURCEFILE` "$printsitefile" `ls -s $P`
   fi 
}


# ========================================================================
# ========================================================================
# ========================================================================

# ========================================================================
#                                                          #### 0. INIT
make_sitemap_init
make_sitemap_list
make_sitemap_sect
make_sitemap_page

if test -d DEBUG &&  test -f "$MK_DATA" 
then FFFF=`echo "$F" | sed -e "s,/,:,g"`
    cp "$MK_DATA" "DEBUG/$FFFF.DATA.tmp.htm"
fi

FILELIST=`echo_site_filelist`
if test ".$opt_filelist" != "." || test ".$opt_list" = ".file"; then
   for F in $FILELIST; do echo "$F" ; done ; exit # --filelist
fi
if test ".$opt_files" != "." ; then FILELIST="$opt_files" ; fi # --files
if test ".$FILELIST" = "."; then warn "nothing to do (no --filelist)"  ; fi
if test ".$FILELIST" = ".SITEFILE" ; then warn "only '$SITEFILE'?!" ; fi

for F in $FILELIST ; do case "$F" in                       #### 1. PASS
name:*)                   scan_namespec "$F" ;;
http:*|https:*|ftp:*|mailto:*|telnet:*|news:*|gopher:*|wais:*) 
                          scan_httpspec "$F" ;;
${SITEFILE}|${SITEFILE}l) scan_sitefile "$F" ;;   # ........... SCAN SITE
*@*.de) 
   echo "!! -> '$F' (skipping malformed mailto:-link)"
   ;;
../*) 
   echo "!! -> '$F' (skipping topdir build)"
   ;;
# */*.html) 
#    echo "!! -> '$F' (skipping subdir build)"
#    ;;
# */*/*/|*/*/|*/|*/index.htm|*/index.html) 
#    echo "!! -> '$F' (skipping subdir index.html)"
#    ;;
*.html) scan_htmlfile "$F"                         # ........... SCAN HTML
   if test ".$opt_xml" != "." ; then
        F=`echo "$F" | sed -e "s/\\.html$/.xml/"`
        scan_xmlfile "$F"
   fi ;;
*.xml)  scan_xmlfile "$F" ;;
*/) echo "'$F' : directory - skipped"
   site_map_list_title "$F" "`sed_slash_key $F`"
   site_map_long_title "$F" "(directory)"
   ;;
*) echo "?? -> '$F'"
   ;;
esac done

if test ".$printerfriendly" != "." ; then           # .......... PRINT VERSION
  _ext_=`print_extension "$printerfriendly" | sed -e "s/&/\\\\&/"`
  PRINTSITEFILE=`echo "$SITEFILE" | sed -e "s/\\.[$AA]*\$/$_ext_&/"`
  echo "NOTE: going to create printer-friendly sitefile $PRINTSITEFILE"
  make_printsitefile > "$PRINTSITEFILE"
fi

for F in $FILELIST ; do case "$F" in                        #### 2. PASS
name:*)                    skip_namespec "$F" ;; 
http:*|https:*|ftp:*|mailto:*|telnet:*|news:*|gopher:*|wais:*)              
                           skip_httpspec "$F" ;;
${SITEFILE}|${SITEFILE}l)  make_sitefile "$F"           # ........ SITE FILE
    if test ".$printerfriendly" != "." ; then make_printerfriendly "$F" ; fi 
    if test ".$opt_xml" != "." ; then _old_F_="$F"
         F=`echo "$F" | sed -e "s/\\.html$/.xml/"`
         make_xmlmaster "$F"      ;F="$_old_F_"
    fi ;;
*@*.de) 
   echo "!! -> '$F' (skipping malformed mailto:-link)"
   ;;
../*) 
    echo "!! -> '$F' (skipping topdir build)"
    ;;
# */*.html) 
#   echo "!! -> '$F' (skipping subdir build)"
#   ;;
# */*/*/|*/*/|*/|*/index.htm|*/index.html) 
#   echo "!! -> '$F' (skipping subdir index.html)"
#   ;;
*.html)  make_htmlfile "$F"                  # .................. HTML FILES
    test ".$printerfriendly" != "." && make_printerfriendly "$F"
    if test ".$opt_xml" != "." ; then _old_F_="$F"
         F=`echo "$F" | sed -e "s/\\.html$/.xml/"`
         make_xmlfile "$F"      ;F="$_old_F_"
    fi ;;
*.xml)   make_xmlfile "$F" ;;
*/) echo "'$F' : directory - skipped"
    ;;
*)  echo "?? -> '$F'"
    ;;
esac
# .............. debug ....................
   if test -d DEBUG && test -f "./$F" ; then
      FFFF=`echo "$F" | sed -e "s,/,:,g"`
      test -f  "$tmp/$F.$DATA" && cp "$tmp/$F.$DATA" DEBUG/$FFFF.data.tmp.htm
      test -f  "$tmp/$F.$HEAD" && cp "$tmp/$F.$HEAD" DEBUG/$FFFF.head.tmp.sed
      test -f  "$tmp/$F.$BODY" && cp "$tmp/$F.$BODY" DEBUG/$FFFF.body.tmp.sed
      test -f  "$tmp/$F.$FOOT" && cp "$tmp/$F.$FOOT" DEBUG/$FFFF.foot.tmp.sed
      for P in tags vars span meta page date list html sect \
               data head body foot fast          xmlmapping \
               gets puts site mett sect1 sect2 sect3 style ; do
      test -f $tmp/$MK.$P.tmp.htm && cp $tmp/$MK.$P.tmp.htm DEBUG/$FFFF.$P.tmp.htm
      test -f $tmp/$MK.$P.tmp.sed && cp $tmp/$MK.$P.tmp.sed DEBUG/$FFFF.$P.tmp.sed
      done
   fi
done

if test ".$opt_keeptmpfiles" = "." ; then
    for i in $tmp/$MK.*.tmp.htm $tmp/$MK.*.tmp.sed \
             $tmp/$MK.*.tmp.css $tmp/$MK.*.tmp.txt
    do test -f "$i" && rm "$i"
    done
fi
if test ".$tmp_dir_was_created" != ".no" ; then rm $tmp/* ; rmdir $tmp ; fi
exit 0
