#! /usr/bin/perl
# this is the perl variant of the mksite script. It based directly on a
# copy of mksite.sh which is derived from snippets that I was using to 
# finish doc pages for website publishing. Using only sh/sed along with
# files has a great disadvantage: it is a very slow process atleast. The
# perl language in contrast has highly optimized string, replace, search
# functions as well as data structures to store intermediate values. As
# an advantage large parts of the syntax are similar to the sh/sed variant.
#
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
# $Id: mksite.pl,v 1.2 2006-09-22 00:33:22 guidod Exp $

use strict; use warnings; no warnings "uninitialized";
use File::Basename qw(basename);
use POSIX qw(strftime);

# initialize some defaults
my $SITEFILE="";
$SITEFILE="site.htm"  if not $SITEFILE and -f "site.htm";
$SITEFILE="site.html" if not $SITEFILE and -f "site.html";
$SITEFILE="site.htm"  if not $SITEFILE;
# my $MK="-mksite";     # note the "-" at the start
my $SED="sed";

my $DATA="~~";     # extension for meta data files
my $HEAD="~head~"; # extension for head sed script
my $BODY="~body~"; # extension for body sed script
my $FOOT="~foot~"; # append to body text (non sed)

my $SED_LONGSCRIPT="$SED -f";

my $az="a-z";                                     # for perl 
my $AZ="A-Z";                                     # we may assume there are
my $NN="0-9";                                     # char-ranges available
my $AA="_$NN$AZ$az";                              # that makes the resulting
my $AX="$AA.+-";                                  # script more readable

my $n = "\n";
my $Q = "q class";
my $QX = "/q";

# LANG="C" ; LANGUAGE="C" ; LC_COLLATE="C"     # these are needed for proper
# export LANG LANGUAGE LC_COLLATE              # lowercasing as some collate
                                               # treat A-Z to include a-z

my @HTMLTAGS = qw/a p h1 h2 h3 h4 h5 h6 dl dd dt ul ol li pre code 
    table tr td th b u i s q em strong strike cite big small sup sub tt
    thead tbody center hr br nobr wbr span div img adress blockquote/;
my @HTMLTAGS2 = qw/html head body title meta http-equiv style link/;

# ==========================================================================
my $hint="";

sub echo
{
    print join(" ",@_),$n;
}
sub error
{
    print STDERR "ERROR: ", join(" ",@_),$n;
}
sub warns
{
    print STDERR "WARN: ", join(" ",@_), $n;
}
sub hint
{
    print STDERR "NOTE: ", join(" ", @_), $n if $hint;
}
sub init
{
    $hint="1" if -d "DEBUG";
}

&init ("NOW!!!");

sub ls_s {
    my $x=`ls -s @_`;
    chomp($x);
    return $x;
}

# ==========================================================================
# reading options from the command line                            GETOPT
my %o = (); # to store option variables
$o{variables}="files";
$o{fileseparator}="?";
$o{files}="";
$o{main_file}="";
$o{formatter}="$0";
my $opt="";
for my $arg (@ARGV) {     # this variant should allow to embed spaces in $arg
    if ($opt) {
	$o{$opt}=$arg;
	$opt="";
    } else {
	$_=$arg;
	if (/^-.*=.*$/) {
	    $opt=$arg; $opt =~ s/-*([$AA][$AA-]*).*/$1/; $opt =~ y/-/_/;
	    if (not $opt) {
		error "invalid option $arg";
	    } else {
		$arg =~ s/^[^=]*=//;
		$o{$opt} = $arg;
		$o{variables} .= " ".$opt;
	    }
	    $opt="";;
	} elsif (/^-.*.-.*$/) {
	    $opt=$arg; $opt =~ s/-*([$AA][$AA-]*).*/$1/; $opt =~ y/-/_/;
	    if (not $opt) {
		error "invalid option $arg";
		$opt="";
	    } else {
		    # keep the option for next round
	    } ;;
	} elsif (/^-.*/) {
	    $opt=$arg; $opt =~ s/^-*([$AA][$AA-]*).*/$1/; $opt =~ y/-/_/;
	    if (not $opt) {
		error "invalid option $arg";
	    } else {
		$arg =~ s/^[^=]*=//;
		$o{$opt} = ' ';
	    }
	    $opt="" ;;
	} else {
	    hint "<$arg>";
	    if (not $o{main_file}) { $o{main_file} = $arg; } else {
	    $o{files} .= $o{fileseparator} if $o{files};
	    $o{files} .= $arg; };
	    $opt="" ;;
	};
    }
} ; if ($opt) {
	$o{$opt}=" ";
	$opt="";
    }

### env | grep ^opt

$SITEFILE=$o{main_file} if $o{main_file} and -f $o{main_file};
$SITEFILE=$o{site_file} if $o{site_file} and -f $o{site_file};
$hint="1" if $o{debug};

if ($o{help}) {
    $_=$SITEFILE;
    echo "$0 [sitefile]";
    echo "  default sitefile = $_  ($o{main_file}) ($o{files})";
    echo "options:";
    echo " --filelist : show list of target files as ectracted from $_";
    echo " --src-dir xx : if source files are not where mksite is executed";
    echo " --tmp-dir xx : use temp instead of local directory";
    echo " --tmp : use automatic temp directory in \$TEMP/mksite.*";
    exit;
    echo " internal:";
    echo "--fileseparator=x : for building the internal filelist (def. '?')";
    echo "--files xx : for list of additional files to be processed";
    echo "--main-file xx : for the main sitefile to take file list from";
}
	
if (not $SITEFILE) {
    error "no SITEFILE found (default would be 'site.htm')$n";
    exit 1;
} else {
    hint "sitefile: ", ls_s($SITEFILE);
}

# we use internal hashes to store mappings - kind of relational tables
my @MK_TAGS= (); # "./$MK.tags.tmp"
my @MK_VARS= (); # "./$MK.vars.tmp"
my @MK_SPAN= (); # "./$MK.span.tmp"
my @MK_META= (); # "./$MK.meta.tmp"
my @MK_METT= (); # "./$MK.mett.tmp"
my @MK_TEST= (); # "./$MK.test.tmp"
my @MK_FAST= (); # "./$MK.fast.tmp"
my @MK_GETS= (); # "./$MK.gets.tmp"
my @MK_PUTS= (); # "./$MK.puts.tmp"
my @MK_OLDS= (); # "./$MK.olds.tmp"
my @MK_SITE= (); # "./$MK.site.tmp"
my @MK_SECT1= (); # "./$MK.sect1.tmp"
my @MK_SECT2= (); # "./$MK.sect2.tmp"
my @MK_SECT3= (); # "./$MK.sect3.tmp"
my @MK_DATA= (); # "./$MK~~"
my %DATA= (); # used for $F.$PARTs

# ========================================================================
# ========================================================================
# ========================================================================
#                                                             MAGIC VARS
#                                                            IN $SITEFILE
my $printerfriendly="";
my $sectionlayout="list";
my $sitemaplayout="list";
my $attribvars=" ";         # <x ref="${varname:=default}">
my $updatevars=" ";         # <!--$varname:=-->default
my $expandvars=" ";         # <!--$varname-->
my $commentvars=" ";        # $updatevars && $expandsvars
my $sectiontab=" ";         # highlight ^<td class=...>...href="$section"
my $currenttab=" ";         # highlight ^<br>..<a href="$topic">
my $headsection="no";
my $tailsection="no";
my $sectioninfo="no";       # using <h2> title <h2> = info text
my $emailfooter="no";

for (source($SITEFILE)) {
    if (/<!--multi-->/) {
	warns("do not use <!--multi-->,"
	     ." change to <!--mksite:multi-->  $SITEFILE"
	     ."warning: or"
	     ." <!--mksite:multisectionlayout-->"
	     ." <!--mksite:multisitemaplayout-->");
	$sectionlayout="multi";
	$sitemaplayout="multi";
    }
    if (/<!--mksite:multi-->/) {
	$sectionlayout="multi";
	$sitemaplayout="multi";
    }
    if (/<!--mksite:multilayout-->/) {
	$sectionlayout="multi";
	$sitemaplayout="multi";
    }
}

sub mksite_magic_option
{
    # $1 is word/option to check for
    my ($U,$INP,$Z) = @_;
    $INP=$SITEFILE if not $INP;
    for (source($INP)) {
	s/(<!--mksite:)($U)-->/$1$2: -->/g;
	s/(<!--mksite:)(\w\w*)($U)-->/$1$3:$2-->/g;
	/<!--mksite:$U:/ or next;
	s/.*<!--mksite:$U:([^<>]*)-->.*/$1/;
	s/.*<!--mksite:$U:([^-]*)-->.*/$1/;
	/<!--mksite:$U:/ and next;
	chomp;
	return $_;
    }
    return "";
}

{
    my $x;
    $x=mksite_magic_option("sectionlayout"); if 
	($x =~ /^(list|multi)$/) { $sectionlayout="$x" ; }
    $x=mksite_magic_option("sitemaplayout"); if
	($x =~ /^(list|multi)$/) { $sitemaplayout="$x" ; }
    $x=mksite_magic_option("attribvars"); if 
	($x =~ /^( |no|warn)$/) { $attribvars="$x" ; }
    $x=mksite_magic_option("updatevars"); if
	($x =~ /^( |no|warn)$/) { $updatevars="$x" ; }
    $x=mksite_magic_option("expandvars"); if
	($x =~ /^( |no|warn)$/) { $expandvars="$x" ; }
    $x=mksite_magic_option("commentvars"); if
	($x =~ /^( |no|warn)$/) { $commentvars="$x" ; }
    $x=mksite_magic_option("printerfriendly"); if
	($x =~ /^( |[.].*|[-]-.*)$/) { $printerfriendly="$x" ; }
    $x=mksite_magic_option("sectiontab"); if
	($x =~ /^( |no|warn)$/) { $sectiontab="$x" ; }
    $x=mksite_magic_option("currenttab"); if
	($x =~ /^( |no|warn)$/) { $currenttab="$x" ; }
    $x=mksite_magic_option("sectioninfo"); if
	($x =~ /^( |no|[=:-])$/) { $sectioninfo="$x" ; }
    $x=mksite_magic_option("commentvars"); if
	($x =~ /^( |no|warn)$/) { $commentvars="$x" ; }
    $x=mksite_magic_option("emailfooter"); if
	($x) { $emailfooter="$x"; }
}

$printerfriendly=$o{print} if $o{print};
$updatevars="no" if $commentvars eq "no"; # duplicated into
$expandvars="no" if $commentvars eq "no"; # info2vars_sed

hint "'$sectionlayout\'sectionlayout '$sitemaplayout\'sitemaplayout";
hint "'$attribvars\'attribvars '$updatevars\'updatevars";
hint "'$expandvars\'expandvars '$commentvars\'commentvars";
hint "'$currenttab\'currenttab '$sectiontab\'sectiontab";
hint "'$headsection\'headsection '$tailsection\'tailsection";

# ==========================================================================
# init a few global variables
#                                                                  0. INIT

# $MK.tags.tmp - originally, we would use a lambda execution on each 
# uppercased html tag to replace <P> with <p class="P">. Here we just
# walk over all the known html tags and make an sed script that does
# the very same conversion. There would be a chance to convert a single
# tag via "h;y;x" or something we do want to convert all the tags on
# a single line of course.
@MK_TAGS=();
{ my ($M,$P); for $M (@HTMLTAGS) {
    $P=uc($M);
    push @MK_TAGS, "s|<$P>|<$M class=\\\"$P\\\">|g;";
    push @MK_TAGS, "s|<$P |<$M class=\\\"$P\\\" |g;";
    push @MK_TAGS, "s|</$P>|</$M>|g;";
}}
push @MK_TAGS, "s|<>|\\&nbsp\\;|g;";
push @MK_TAGS, "s|<->|<WBR />\\;|g;";
push @MK_TAGS, "s|<c>|<code>|g;";
push @MK_TAGS, "s|</c>|</code>|g;";
push @MK_TAGS, "s|<section>||g;";
push @MK_TAGS, "s|</section>||g;";
push @MK_TAGS, "s|<(a [^<>]*) />|<\$1></a>|g";
my $_ulink_="<a href=\"\$1\" remap=\"url\">\$1</a>";
push @MK_TAGS, "s|<a>\\s*(\\w+://[^<>]*)</a>|$_ulink_|g;";
# also make sure that some non-html entries are cleaned away that
# we are generally using to inject meta information. We want to see
# that meta ino in the *.htm browser view during editing but they
# shall not get present in the final html page for publishing.
my @DC_VARS = 
    ("contributor", "date", "source", "language", "coverage", "identifier",
     "rights", "relation", "creator", "subject", "description",
     "publisher", "DCMIType");
my @_EQUIVS =
    ("refresh", "expires", "content-type", "cache-control", 
     "redirect", "charset", # mapped to refresh / content-type
     "content-language", "content-script-type", "content-style-type");
{ my $P; for $P (@DC_VARS) { # dublin core embedded
    push @MK_TAGS, "s|<$P>[^<>]*</$P>||g;";
}}
{ my $P; for $P (@_EQUIVS) {
    push @MK_TAGS, "s|<$P>[^<>]*</$P>||g;";
}}
push @MK_TAGS, "s|<a sect=\\\"[$AZ$NN]\\\"|<a|g;" if not $o{keepsect};
push @MK_TAGS, "s|<!--[$AX]*[?]-->||g;";
push @MK_TAGS, "s|<!--\\\$[$AX]*[?]:-->||g;";
push @MK_TAGS, "s|<!--\\\$[$AX]*:[?=]-->||g;";
push @MK_TAGS, "s|(<[^<>]*)\\\${[$AX]*:[?=]([^<{}>]*)}([^<>]*>)|\$1\$2\$3|g;";

my $TRIMM=" -e 's:^ *::' -e 's: *\$::'";  # trimm away leading/trailing spaces
sub trimm
{
    my ($T,$Z) = @_;
    $T =~ s:\A\s*::s; $T =~ s:\s*\Z::s;
    return $T;
}
sub trimmm
{
    my ($T,$Z) = @_;
    $T =~ s:\A\s*::s; $T =~ s:\s*\Z::s; $T =~ s:\s+: :g;
    return $T;
}
sub timezone
{
    # +%z is an extension while +%Z is supposed to be posix
    my $tz;
    eval { $tz = strftime("%z", localtime()) };
    return $tz  if $tz =~ /[+]/;
    return $tz  if $tz =~ /[-]/;
    return strftime("%Z", localtime());
}

sub timetoday
{
    return strftime("%Y-%m-%d", localtime());
}
sub timetodays
{
    return strftime("%Y-%m%d", localtime());
}

sub esc
{
    my ($TXT,$XXX) = @_;
    $TXT =~ s|&|\\\\&|g;
    return $TXT;
}

my %SOURCE;
sub source # $file : @lines
{
    my ($FILE,$Z) = @_;
    if (exists $SOURCE{$FILE}) { return @{$SOURCE{$FILE}}; }
    my @TEXT = ();
    open FILE, "<$FILE" or die "could not open $FILE: $!";
    for my $line (<FILE>) {
	push @TEXT, $line;
    } close FILE;
    @{$SOURCE{$FILE}} = @TEXT;
    return @{$SOURCE{$FILE}};
}
sub savesource # $file \@lines
{
    my ($FILE,$LINES,$Z) = @_;
    @{$SOURCE{$FILE}} = @{$LINES};
}

my $F; # current file during loop <<<<<<<<<
my $i = 100;
sub savelist {
    if (-d "DEBUG") {
	my ($script,$ext,$Z) = @_;
	if (not $ext) { $ext = "_".$i; $i++; }
	my $X = "$F.$ext.tmp.PL"; $X =~ s|/|:|g;
	open X, ">DEBUG/$X" or die "could not open $X: $!";
	print X "#! /usr/bin/env perl",$n;
	print X "# ",$#_," $ext files ",localtime(),$n;
	my $TEXT = join("$n", @{$script});
	$TEXT =~ s|source\([^()]*\)|<>|;
	print X $TEXT,$n; close X;
    }
}

sub eval_MK_LIST # $str @list
{
    my $FILETYPE = $_[0]; shift @_;
    my $result = $_[0]; shift @_;
    my $extra = "";
    my $script = "\$_ = \$result; my \$Z;";
    $script .= join(";$n ", @_);
    $script .= "$n;\$result = \$_;$n";
    savelist([$script],$FILETYPE);
    eval $script;
    return $result.$extra;
}

sub eval_MK_FILE  {
    my $FILETYPE = $_[0]; shift @_;
    my $FILENAME = $_[0]; shift @_;
    my $result = "";
    my $script = "my \$FILE; my \$extra = ''; my \$Z; $n";
    $script.= "for (source('$FILENAME')) { $n";
    $script.= join(";$n  ", @_);
    $script.= "$n; \$result .= \$_; ";
    $script.= "$n if(\$extra){\$result.=\$extra;\$extra='';\$result.=\"\\n\"}";
    $script.= "$n} if(\$extra){\$result.=\$extra;}$n";
    savelist([$script],$FILETYPE);
    eval $script;
    return $result;
}
my $sed_add = "\$extra .= "; # "/r ";

sub foo { print "               '$F'$n"; }

# ======================================================================
#                                                                FUNCS

my $SOURCEFILE; # current file <<<<<<<<
my @FILELIST; # <<<<<<<

sub sed_slash_key  # helper to escape chars special in /anchor/ regex
{                     # currently escaping "/" "[" "]" "."
    my $R = $_[0]; $R =~ s|[\"./[-]|\\$&|g; $R =~ s|\]|\\\\$&|g;
    return $R;
}
sub sed_piped_key  # helper to escape chars special in s|anchor|| regex
{                     # currently escaping "|" "[" "]" "."
    my $R = $_[0]; $R =~ s/[\".|[-]/\\$&/g; $R =~ s/\]/\\\\$&/g;
    return $R;
}

sub back_path      # helper to get the series of "../" for a given path
{
    my ($R,$Z) = @_; if ($R !~ /\//) { return ""; }
    $R =~ s|/[^/]*$|/|; $R =~ s|[^/]*/|../|g;
    return $R;
}

sub dir_name
{
    my $R = $_[0]; $R =~ s:/[^/][^/]*\$::;
    return $R;
}

sub info2vars_sed      # generate <!--$vars--> substition sed addon script
{
    my ($INP,$Z) = @_;
    $INP = \@{$DATA{$F}} if not $INP;
    my @OUT = ();
    my $V8=" *([^ ][^ ]*) +(.*)<$QX>";
    my $V9=" *DC[.]([^ ][^ ]*) +(.*)<$QX>";
    my $N8=" *([^ ][^ ]*) ([$NN].*)<$QX>";
    my $N9=" *DC[.]([^ ][^ ]*) ([$NN].*)<$QX>";
    my $V0="([<]*)\\\$";
    my $V1="([^<>]*)\\\$";
    my $V2="([^{<>}]*)";
    my $V3="([^<>]*)";
    my $SS="<"."<>".">"; # spacer so value="2004" dont make for s|\(...\)|\12004|
    $Z="\$Z=";
    $updatevars = "no" if $commentvars  eq "no";   # duplicated from
    $expandvars = "no" if $commentvars  eq "no";   # option handling
    my @_INP = (); for (@{$INP}) { 
	my $x=$_; $x =~ s/(>[^<>]*)'([^<>]*<)/$1\\'$2/; push @_INP, $x; # OOOOPS
    }
    if ($expandvars ne "no") {
	for (@_INP) {
    if    (/^=....=formatter /) { next; } 
    elsif (/^<$Q='name'>$V9/){push @OUT, "\$Z='$2';s|<!--$V0$1\\?-->|- \$Z|;"}
    elsif (/^<$Q='Name'>$V9/){push @OUT, "\$Z='$2';s|<!--$V0$1\\?-->|(\$Z)|;"}
    elsif (/^<$Q='name'>$V8/){push @OUT, "\$Z='$2';s|<!--$V0$1\\?-->|- \$Z|;"}
    elsif (/^<$Q='Name'>$V8/){push @OUT, "\$Z='$2';s|<!--$V0$1\\?-->|(\$Z)|;"}
        } 
    }
    if ($expandvars ne "no") {
	for (@_INP) {
    if    (/^=....=formatter /) { next; } 
    elsif (/^<$Q='text'>$V9/){push @OUT, "\$Z='$2';s|<!--$V1$1-->|\$1$SS\$Z|;"}
    elsif (/^<$Q='Text'>$V9/){push @OUT, "\$Z='$2';s|<!--$V1$1-->|\$1$SS\$Z|;"}
    elsif (/^<$Q='name'>$V9/){push @OUT, "\$Z='$2';s|<!--$V1$1\\?-->|\$1$SS\$Z|;"}
    elsif (/^<$Q='Name'>$V9/){push @OUT, "\$Z='$2';s|<!--$V1$1\\?-->|\$1$SS\$Z|;"}
    elsif (/^<$Q='text'>$V8/){push @OUT, "\$Z='$2';s|<!--$V1$1-->|\$1$SS\$Z|;"}
    elsif (/^<$Q='Text'>$V8/){push @OUT, "\$Z='$2';s|<!--$V1$1-->|\$1$SS\$Z|;"}
    elsif (/^<$Q='name'>$V8/){push @OUT, "\$Z='$2';s|<!--$V1$1\\?-->|\$1$SS\$Z|;"}
    elsif (/^<$Q='Name'>$V8/){push @OUT, "\$Z='$2';s|<!--$V1$1\\?-->|\$1$SS\$Z|;"}
	}
    }
    if ($updatevars ne "no") {
	for (@_INP) {  my $H = "[^<>]*";
    if    (/^=....=formatter /) { next; }
    elsif (/^<$Q='name'>$V9/){push @OUT, "\$Z='$2';s|<!--$V0$1:\\?-->$H|- \$Z|;"}
    elsif (/^<$Q='Name'>$V9/){push @OUT, "\$Z='$2';s|<!--$V0$1:\\?-->$H|(\$Z)|;"}
    elsif (/^<$Q='name'>$V8/){push @OUT, "\$Z='$2';s|<!--$V0$1:\\?-->$H|- \$Z|;"}
    elsif (/^<$Q='Name'>$V8/){push @OUT, "\$Z='$2';s|<!--$V0$1:\\?-->$H|(\$Z)|;"}
	}
    }
    if ($updatevars ne "no") {
	for (@_INP) {  my $H = "[^<>]*";
    if    (/^=....=formatter /) { next; }
    elsif (/^<$Q='text'>$V9/){push @OUT,"\$Z='$2';s|<!--$V1$1:\\=-->$H|\$1$SS\$Z|;"}
    elsif (/^<$Q='Text'>$V9/){push @OUT,"\$Z='$2';s|<!--$V1$1:\\=-->$H|\$1$SS\$Z|;"}
    elsif (/^<$Q='name'>$V9/){push @OUT,"\$Z='$2';s|<!--$V1$1:\\?-->$H|\$1$SS\$Z|;"}
    elsif (/^<$Q='Name'>$V9/){push @OUT,"\$Z='$2';s|<!--$V1$1:\\?-->$H|\$1$SS\$Z|;"}
    elsif (/^<$Q='text'>$V8/){push @OUT,"\$Z='$2';s|<!--$V1$1:\\=-->$H|\$1$SS\$Z|;"}
    elsif (/^<$Q='Text'>$V8/){push @OUT,"\$Z='$2';s|<!--$V1$1:\\=-->$H|\$1$SS\$Z|;"}
    elsif (/^<$Q='name'>$V8/){push @OUT,"\$Z='$2';s|<!--$V1$1:\\?-->$H|\$1$SS\$Z|;"}
    elsif (/^<$Q='Name'>$V8/){push @OUT,"\$Z='$2';s|<!--$V1$1:\\?-->$H|\$1$SS\$Z|;"}
	}
    }
    if ($attribvars ne "no") {
	for (@_INP) {  my $H = "[^<>]*";
    if    (/^=....=formatter /) { next; }
    elsif (/^<$Q='text'>$V9/){push @OUT,"\$Z='$2';s|<$V1\{$1:[=]$V2}$V3>|<\$1$SS\$Z\$3>|;"}
    elsif (/^<$Q='Text'>$V9/){push @OUT,"\$Z='$2';s|<$V1\{$1:[=]$V2}$V3>|<\$1$SS\$Z\$3>|;"}
    elsif (/^<$Q='name'>$V9/){push @OUT,"\$Z='$2';s|<$V1\{$1:[?]$V2}$V3>|<\$1$SS\$Z\$3>|;"}
    elsif (/^<$Q='Name'>$V9/){push @OUT,"\$Z='$2';s|<$V1\{$1:[?]$V2}$V3>|<\$1$SS\$Z\$3>|;"}
    elsif (/^<$Q='text'>$V8/){push @OUT,"\$Z='$2';s|<$V1\{$1:[=]$V2}$V3>|<\$1$SS\$Z\$3>|;"}
    elsif (/^<$Q='Text'>$V8/){push @OUT,"\$Z='$2';s|<$V1\{$1:[=]$V2}$V3>|<\$1$SS\$Z\$3>|;"}
    elsif (/^<$Q='name'>$V8/){push @OUT,"\$Z='$2';s|<$V1\{$1:[?]$V2}$V3>|<\$1$SS\$Z\$3>|;"}
    elsif (/^<$Q='Name'>$V8/){push @OUT,"\$Z='$2';s|<$V1\{$1:[?]$V2}$V3>|<\$1$SS\$Z\$3>|;"} 
	}
        for (split / /, $o{variables}) {
	    {push @OUT,"\$Z='$o{$_}';s|<$V1\{$_:[?]$V2}$V3>|<\$1$SS\$Z\$3>|;"} 
	}
    }
    # if value="2004" then generated sed might be "\\12004" which is bad
    # instead we generate an edited value of "\\1$SS$value" and cut out
    # the spacer now after expanding the variable values:
    push @OUT, "s|$SS||g;";
    return @OUT;
	
}

sub info2meta_sed     # generate <meta name..> text portion
{
    my ($INP,$XXX) = @_;
    $INP = \@{$DATA{$F}} if not $INP;
    my @OUT = ();
    # http://www.metatab.de/meta_tags/DC_type.htm
    my $V6=" *HTTP[.]([^ ]+) (.*)<$QX>";
    my $V7=" *DC[.]([^ ]+) (.*)<$QX>";
    my $V8=" *([^ ]+) (.*)<$QX>" ;
    sub __TYPE_SCHEME { "name=\"DC.type\" content=\"$2\" scheme=\"$1\"" };
    sub __DCMI { "name=\"$1\" content=\"$2\" scheme=\"DCMIType\"" };
    sub __NAME { "name=\"$1\" content=\"$2\"" };
    sub __NAME_TZ { "name=\"$1\" content=\"$2 ".&timezone()."\"" };
    sub __HTTP { "http-equiv=\"$1\" content=\"$2\"" };
    for (@$INP) {
	if (/=....=today /) { next; }
	if (/<$Q='meta'>HTTP[.]/ && /<$Q='meta'>$V6/) {
	    push @OUT, " <meta ${\(__HTTP)} />" if $2; next; }
	if (/<$Q='meta'>DC[.]DCMIType / && /<$Q='meta'>$V7/) {
	    push @OUT, " <meta ${\(__TYPE_SCHEME)} />" if $2; next; }
	if (/<$Q='meta'>DC[.]type Collection$/ && /<$Q='meta'>$V8/) {
	    push @OUT, " <meta ${\(__DCMI)} />" if $2; next; }
	if (/<$Q='meta'>DC[.]type Dataset$/ && /<$Q='meta'>$V8/) {
	    push @OUT, " <meta ${\(__DCMI)} />" if $2; next; }
	if (/<$Q='meta'>DC[.]type Event$/ && /<$Q='meta'>$V8/) {
	    push @OUT, " <meta ${\(__DCMI)} />" if $2; next; }
	if (/<$Q='meta'>DC[.]type Image$/ && /<$Q='meta'>$V8/) {
	    push @OUT, " <meta ${\(__DCMI)} />" if $2; next; }
	if (/<$Q='meta'>DC[.]type Service$/ && /<$Q='meta'>$V8/) {
	    push @OUT, " <meta ${\(__DCMI)} />" if $2; next; }
	if (/<$Q='meta'>DC[.]type Software$/ && /<$Q='meta'>$V8/) {
	    push @OUT, " <meta ${\(__DCMI)} />" if $2; next; }
	if (/<$Q='meta'>DC[.]type Sound$/ && /<$Q='meta'>$V8/) {
	    push @OUT, " <meta ${\(__DCMI)} />" if $2; next; }
	if (/<$Q='meta'>DC[.]type Text$/ && /<$Q='meta'>$V8/) {
	    push @OUT, " <meta ${\(__DCMI)} />" if $2; next; }
	if (/<$Q='meta'>DC[.]date[.].*[+]/ && /<$Q='meta'>$V8/) {
	    push @OUT, " <meta ${\(__NAME)} />" if $2; next; }
	if (/<$Q='meta'>DC[.]date[.].*[:]/ && /<$Q='meta'>$V8/) {
	    push @OUT, " <meta ${\(__NAME_TZ)} />" if $2; next; }
	if (/<$Q='meta'>/ && /<$Q='meta'>$V8/) {
	    push @OUT, " <meta ${\(__NAME)} />" if $2; next; }
    }
    return @OUT;
}

sub info_get_entry # get the first <!--vars--> value known so far
{
    my ($TXT,$INP,$XXX) = @_;
    $TXT = "sect" if not $TXT;
    $INP = \@{$DATA{$F}} if not $INP;
    for (grep {/<$Q='text'>$TXT /} @$INP) {
	my $info = $_;
	$info =~ s|<$Q='text'>$TXT ||; $info =~ s|<$QX>||;
	chomp($info); chomp($info); return $info;
    }
}

sub info1grep # test for a <!--vars--> substition to be already present
{
    my ($TXT,$INP,$XXX) = @_;
    $TXT = "sect" if not $TXT;
    $INP = \@{$DATA{$F}} if not $INP;
    return scalar(grep {/^<$Q='text'>$TXT /} @$INP); # returning the count
}

sub dx_init
{
    @{$DATA{$F}} = ();
    &dx_meta ("formatter", basename($o{formatter}));
    for (split / /, $o{variables}) {        # commandline --def=value
	if (/_/) { my $u=$_; $u =~ y/_/-/;  # makes for <!--$def--> override
                   &dx_meta ($u, $o{$_}); 
	} else {   &dx_text ($_, $o{$_}); }
    }
}

sub dx_line
{
    my ($U,$V,$W,$Z) = @_; chomp($U); chomp($V);
    push @{$DATA{$F}}, "<$Q=$U>".$V." ".trimmm($W)."<$QX>";
}

sub DX_line
{
    my ($U,$V,$W,$Z) = @_; $W =~ s/<[^<>]*>//g;
    &dx_line ($U,$V,$W);
}

sub dx_text
{
    my ($U,$V,$Z) = @_;
    &dx_line ("'text'",$U,$V);
}

sub DX_text   # add a <!--vars--> substition includings format variants
{
    my ($N, $T,$XXX) = @_;
    $N = trimm($N); $T = trimm($T); 
    if ($N) {
	if ($T) {
	    my $text=lc("$T"); $text =~ s/<[^<>]*>//g;
	    &dx_line ("'text'",$N,$T);
	    &dx_line ("'name'",$N,$text);
	    my $varname=$N; $varname =~ s/.*[.]//;  # cut out front part
	    if ($N ne $varname and $varname) {
		$text=lc("$varname $T"); $text =~ s/<[^<>]*>//g;
		&dx_line ("'Text'",$varname,$T);
		&dx_line ("'Name'",$varname,$text);
	    }
	}
    }
}

sub dx_meta
{
    my ($U,$V,$Z) = @_;
    &DX_line ("'meta'",$U,$V);
}

sub DX_meta  # add simple meta entry and its <!--vars--> subsitution
{
    my ($U,$V,$Z) = @_;
    &DX_line ("'meta'",$U,$V);
    &DX_text ("$U", $V);
}

sub DC_meta   # add new DC.meta entry plus two <!--vars--> substitutions
{
    my ($U,$V,$Z) = @_;
    &DX_line ("'meta'","DC.$U",$V);
    &DX_text ("DC.$U", $V);
    &DX_text ("$U", $V);
}

sub HTTP_meta   # add new HTTP.meta entry plus two <!--vars--> substitutions
{
    my ($U,$V,$Z) = @_;
    &DX_line ("'meta'","HTTP.$U",$V);
    &DX_text ("HTTP.$U", $V);
    &DX_text ("$U", $V);
}

sub DC_VARS_Of # check DC vars as listed in $DC_VARS global/generate DC_meta
{                 # the results will be added to .meta.tmp and .vars.tmp later
    my ($FILENAME,$Z)= @_; 
    $FILENAME=$SOURCEFILE if not $FILENAME;
    for my $M (@DC_VARS, "title") {
	# scan for a <markup> of this name                  FIXME
	my ($part,$text);
	for (source($FILENAME)) {
	    /<$M>/ or next; s|.*<$M>||; s|</$M>.*||;
	    $part = trimm($_); last;
	}
	$text=$part;  $text =~ s|^\w*:||; $text = trimm($text);
	next if not $text;
	# <mark:part> will be <meta name="mark.part"> 
	if ($text ne $part) {
	    my $N=$part; $N =~ s/:.*//;
	    &DC_meta ("$M.$N", $text);
	} elsif ($M eq "date") {
	    &DC_meta ("$M.issued", $text); # "<date>" -> "<date>issued:"
	} else {
	    &DC_meta ("$M", $text);
	}
    }
}

sub HTTP_VARS_Of  # check HTTP-EQUIVs as listed in $_EQUIV global then
{                 # generate meta tags that are http-equiv= instead of name=
    my ($FILENAME,$Z)= @_; 
    $FILENAME=$SOURCEFILE if not $FILENAME;
    for my $M (@_EQUIVS) {
	# scan for a <markup> of this name                  FIXME
	my ($part,$text);
	for (source($FILENAME)) {
	    /<$M>/ or next; s|.*<$M>||; s|</$M>.*||;
	    $part = trimm($_); last;
	}
	$text=$part;  $text =~ s|^\w*:||; $text = trimm($text);
	next if not $text;
	if ($M eq "redirect") {
	    &HTTP_meta ("refresh", "5; url=$text"); &DX_text ("$M", $text);
	} elsif ($M eq "charset") {
	    &HTTP_meta ("content-type", "text/html; charset=$text");
	} else {
	    &HTTP_meta ("$M", $text);
	}
    }
}

sub DC_isFormatOf   # make sure there is this DC.relation.isFormatOf tag
{                      # choose argument for a fallback (usually $SOURCEFILE)
    my ($NAME,$Z) = @_;
    $NAME=$SOURCEFILE if not $NAME;
    if (not &info1grep ("DC.relation.isFormatOf")) {
	&DC_meta ("relation.isFormatOf", "$NAME");
    }
}

sub DC_publisher    # make sure there is this DC.publisher meta tag
{                      # choose argument for a fallback (often $USER)
    my ($NAME,$Z) = @_;
    $NAME=$ENV{"USER"} if not $NAME;
    if (not &info1grep ("DC.publisher")) {
	&DC_meta ("publisher", "$NAME");
    }
}

sub DC_modified     # make sure there is a DC.date.modified meta tag
{                      # maybe choose from filesystem dates if possible
    my ($ZZ,$Z) = @_; # target file
    if (not &info1grep ("DC.date.modified")) {
	my @stats = stat($ZZ);
	my $text =  strftime("%Y-%m-%d", localtime($stats[9]));
	&DC_meta ("date.modified", $text);
    }
}

sub DC_date         # make sure there is this DC.date meta tag
{                      # choose from one of the available DC.date.* specials
    my ($ZZ,$Z) = @_; # source file
    if (&info1grep ("DC.date")) {
	&DX_text ("issue", "dated ".&info_get_entry("DC.date"));
        &DX_text ("updated", &info_get_entry("DC.date"));
    } else { 
	my $text=""; my $kind;
	for $kind (qw/available issued modified created/) {
	    $text=&info_get_entry("DC.date.$kind");
	    # test ".$text" != "." && echo "$kind = date = $text ($ZZ)"
	    last if $text;
	}
	if (not $text) {
	    my $part; my $M="date";
	    for (source($ZZ)) {
		/<$M>/ or next; s|.*<$M>||; s|</$M>.*||;
		$part=trimm($_); last;
	    }
	    $text=$part; $text =~ s|^[$AA]*:||;
	    $text = &trimm ($text);
	}
	if (not $text) {
	    my $part; my $M="!--date:*=*--"; # takeover updateable variable...
	    for (source($ZZ)) {
		/<$M>/ or next; s|.*<$M>||; s|</.*||;
		$part=trimm($_); last;
	    }
	    $text=$part; $text =~ s|^[$AA]*:||; $text =~ s|\&.*||;
	    $text = &trimm ($text);
	}
	$text =~ s/[$NN]*:.*//; # cut way seconds
	&DX_text ("updated", $text);
	my $text1=$text; $text1 =~ s|^.* *updated ||;
	if ($text ne $text1) {
	    $kind="modified" ; $text=$text1; $text =~ s|,.*||;
	}
	$text1=$text; $text1 =~ s|^.* *modified ||;
	if ($text ne $text1) {
	    $kind="modified" ; $text=$text1; $text =~ s|,.*||;
	}
	$text1=$text; $text1 =~ s|^.* *created ||;
	if ($text ne $text1) {
	    $kind="created" ; $text=$text1; $text =~ s|,.*||;
	}
	&DC_meta ("date", "$text");
	&DX_text ("issue", "$kind $text");
    }
}

sub DC_title
{
    # choose a title for the document, either an explicit title-tag
    # or one of the section headers in the document or fallback to filename
    my ($ZZ,$Z) = @_; # target file
    my ($term, $text);
    if (not &info1grep ("DC.title")) { 
	for my $M (qw/TITLE title H1 h1 H2 h2 H3 H3 H4 H4 H5 h5 H6 h6/) {
	    for (source($ZZ)) {
		/<$M>/ or next; s|.*<$M>||; s|</$M>.*||;
		$text = trimm($_); last;
	    }
	    last if $text;
	    for (source($ZZ)) {
		/<$M [^<>]*>/ or next; s|.*<$M [^<>]*>||; s|</$M>.*||;
		$text = trimm($_); last;
	    }
	    last if $text;
	}
	if (not $text) {
	    $text=basename($ZZ,".html"); 
	    $text=basename($text,".htm"); $text =~ y/_/ /; $text =~ s/$/ info/;
	    $text=~ s/\n/      /g;
	}
	$term=$text; $term =~ s/.*[\(]//; $term =~ s/[\)].*//;
	$text =~ s/[\(][^\(\)]*[\)]//;
	if (not $term or $term eq $text) {
	    &DC_meta ("title", "$text");
	} else {
	    &DC_meta ("title", "$term - $text");
	}
    }
}    

sub site_get_section # return parent section page of given page
{
    my $_F_ = &sed_slash_key(@_);
    for my $x (grep {/<$Q='sect'>$_F_ /} @MK_DATA) {
	my $info = $x; $info =~ s|<$Q='sect'>[^ ]* ||; $info =~ s|<$QX>||;
	return $info;
    }
}

sub DC_section # not really a DC relation (shall we use isPartOf ?) 
{                 # each document should know its section father
    my $sectn = &site_get_section($F);
    if ($sectn) {
	&DC_meta ("relation.section", $sectn);
    }
}

sub info_get_entry_section
{
    return &info_get_entry("DC.relation.section");
}    

sub site_get_selected  # return section of given page
{
    my $_F_ = &sed_slash_key(@_);
    for my $x (grep {/<$Q='[u]se.'>$_F_ /} @MK_DATA) {
	my $info = $x; 
	$info =~ s/<$Q='[u]se.'>[^ ]* //; $info =~ s|<$QX>||;
	return $info;
    }
}

sub DC_selected # not really a DC title (shall we use alternative ?)
{
    # each document might want to highlight the currently selected item
    my $short=&site_get_selected($F);
    if ($short) {
	&DC_meta ("title.selected", $short);
    }
}

sub info_get_entry_selected
{
    return &info_get_entry("DC.title.selected");
}

sub site_get_rootsections # return all sections from root of nav tree
{
    my @OUT;
    for (grep {/<$Q='[u]se1'>/} @MK_DATA) { 
	my $x = $_;
	$x =~ s/<$Q='[u]se.'>([^ ]*) .*/$1/;
	push @OUT, $x;
    }
    return @OUT;
}

sub site_get_sectionpages # return all children pages in the given section
{
    my $_F_=&sed_slash_key(@_);
    my @OUT = ();
    for (grep {/^<$Q='sect'>[^ ]* $_F_$/} @MK_DATA) {
	my $x = $_;
	$x =~ s/^<$Q='sect'>//; $x =~ s/ .*//; $x =~ s|<$QX>||;
	push @OUT, $x;
    }
    return @OUT;
}

sub site_get_subpages # return all page children of given page
{
    my $_F_=&sed_slash_key(@_);
    my @OUT = ();
    for (grep {/^<$Q='node'>[^ ]* $_F_<[^<>]*>$/} @MK_DATA) {
	my $x = $_; 
	$x =~ s/^<$Q='node'>//; $x =~ s/ .*//; $x =~ s|<$QX>||;
	push @OUT, $x;
    }
    return @OUT;
}

sub site_get_parentpage # ret parent page for given page (".." for sections)
{
    my $_F_=&sed_slash_key(@_);
    for (grep {/^<$Q='node'>$_F_ /} @MK_DATA) {
	my $x = $_;
	$x =~ s/^<$Q='node'>[^ ]* //; $x =~ s|<$QX>||;
	return $x;
    } 
}

sub DX_alternative    # detect wether page asks for alternative style
{                        # which is generally a shortpage variant 
    my ($U,$Z) = @_;
    my $x=&mksite_magic_option("alternative",$U);
    $x =~ s/^ *//; $x =~s/ .*//;
    if ($x) {
	&DX_text ("alternative", $x);
    }
}

sub info2head_sed  # append alternative handling script to $HEAD
{
    my @OUT = ();
    my $have=&info_get_entry("alternative");
    if ($have) {
	push @OUT, "/<!--mksite:alternative:$have .*-->/ && do {";
	push @OUT, "s/<!--mksite:alternative:$have( .*)-->/\$1/";
	push @OUT, "$sed_add \$_; last; };";
    }
    return @OUT;
}
sub info2body_sed  # append alternative handling script to $BODY
{
    my @OUT = ();
    my $have=&info_get_entry("alternative");
    if ($have) {
	push @OUT, "s/<!--mksite:alternative:$have( .*)-->/\$1/";
    }
    return @OUT;
}

sub bodymaker_for_sectioninfo
{
    if ($sectioninfo eq "no") { return ""; }
    my $_x_="<!--mksite:sectioninfo::-->";
    my $_q_="([^<>]*[$AX][^<>]*)";
    $_q_="[ ][ ]*$sectioninfo([ ])" if $sectioninfo ne " ";
    my @OUT = ();
    push @OUT, "s|(^<[hH][$NN][ >].*</[hH][$NN]>)$_q_|\$1$_x_\$2|";
    push @OUT, "/$_x_/ and s|^|<table width=\"100%\"><tr valign=\"bottom\"><td>|";
    push @OUT, "/$_x_/ and s|</[hH][$NN]>|&</td><td align=\"right\"><i>|";
    push @OUT, "/$_x_/ and s|\$|</i></td></tr></table>|";
    push @OUT, "s|$_x_||";
    return @OUT;
}

sub fast_href  # args "$FILETOREFERENCE" "$FROMCURRENTFILE:$F"
{   # prints path to $FILETOREFERENCE href-clickable in $FROMCURRENTFILE
    # if no subdirectoy then output is the same as input $FILETOREFERENCE
    my ($T,$R,$Z) = @_;
    my $S=&back_path ($R);
    if (not $S) {
	return $T;
    } else {
	my $t=$T;
	$t =~ s/^ *$//; $t =~ s/^\/.*//; 
	$t =~ s/^[.][.].*//; $t =~ s/^\w*:.*//;
	if (not $t) { # don't move any in the pattern above
	    return $T;
	} else {
	    return "$S$T";   # prefixed with backpath
	}
    }
}

sub make_back_path # "$FILE"
{
    my ($R,$Z) = @_;
    my $S=&back_path ($R);
    my @OUT = ();
    return @OUT if $S !~ /^\.\./;
    push @OUT, "s|(<[^<>]*\\shref=\\\")(\\w[^<>:]*\\\"[^<>]*>)|\$1$S\$2|g;";
    push @OUT, "s|(<[^<>]*\\ssrc=\\\")(\\w[^<>:]*\\\"[^<>]*>)|\$1$S\$2|g;";
    return @OUT;
}

# ============================================================== SITE MAP DATA
# each entry needs atleast a list-title, a long-title, and a list-date
# these are the basic information to be printed in the sitemap file
# where it is bound the hierarchy of sect/subsect of the entries.

sub site_map_list_title # $file $text
{
    my ($U,$V,$Z) = @_; chomp($U);
    push @MK_DATA, "<$Q='list'>$U ".trimm($V)."<$QX>";
}
sub info_map_list_title # $file $text
{
    my ($U,$V,$Z) = @_; chomp($U);
    push @{$DATA{$U}}, "<$Q='list'>".trimm($V)."<$QX>";
}
sub site_map_long_title # $file $text
{
    my ($U,$V,$Z) = @_; chomp($U);
    push @MK_DATA, "<$Q='long'>$U ".trimm($V)."<$QX>";
}
sub info_map_long_title # $file $text
{
    my ($U,$V,$Z) = @_; chomp($U);
    push @{$DATA{$U}}, "<$Q='long'>".trimm($V)."<$QX>";
}
sub site_map_list_date # $file $text
{
    my ($U,$V,$Z) = @_; chomp($U);
    push @MK_DATA, "<$Q='date'>$U ".trimm($V)."<$QX>";
}
sub info_map_list_date # $file $text
{
    my ($U,$V,$Z) = @_; chomp($U);
    push @{$DATA{$U}}, "<$Q='date'>".trimm($V)."<$QX>";
}

sub site_get_list_title
{
    my ($U,$V,$Z) = @_;
    for (@MK_DATA) { if (m|^<$Q='list'>$U (.*)<$QX>|) { return $1; } } return "";
}
sub site_get_long_title
{
    my ($U,$V,$Z) = @_;
    for (@MK_DATA) { if (m|^<$Q='long'>$U (.*)<$QX>|) { return $1; } } return "";
}
sub site_get_list_date
{
    my ($U,$V,$Z) = @_;
    for (@MK_DATA) { if (m|^<$Q='date'>$U (.*)<$QX>|) { return $1; } } return "";
}

sub siteinfo2sitemap# generate <name><page><date> addon sed scriptlet
{                      # the resulting script will act on each item/line
                       # containing <!--"filename"--> and expand any following
                       # reference of <!--name--> or <!--date--> or <!--long-->
    my ($INP,$Z) = @_ ; $INP= \@MK_DATA if not $INP;
    my @OUT = ();
    my $_list_=
	sub{"s|(<!--\\\"$1\\\"-->.*)<name [^<>]*>.*</name>|\$1<name href=\\\"$1\\\">$2</name>|"};
    my $_date_=
	sub{"s|(<!--\\\"$1\\\"-->.*)<date>.*</date>|\$1<date>$2</date>|"};
    my $_long_=
	sub{"s|(<!--\\\"$1\\\"-->.*)<long>.*</long>|\$1<long>$2</long>|"};
    
    for (@$INP) {
	my $info = $_;
	$info =~ s:<$Q='list'>([^ ]*) (.*)<$QX>:&$_list_:e;
	$info =~ s:<$Q='date'>([^ ]*) (.*)<$QX>:&$_date_:e;
	$info =~ s:<$Q='long'>([^ ]*) (.*)<$QX>:&$_long_:e;
	$info =~ /^s\|/ || next;
	push @OUT, $info;
    }
    return @OUT;
}

sub make_multisitemap
{  # each category gets its own column along with the usual entries
    my ($INPUTS,$Z)= @_ ; $INPUTS=\@MK_DATA if not $INPUTS;
    @MK_SITE = &siteinfo2sitemap(); # have <name><long><date> addon-sed
    my @OUT = (); 
    my $_form_= sub{"<!--\"$2\"--><!--use$1--><long>$3</long><!--end$1-->"
			."<br><name href=\"$2\">$3</name><date>......</date>" };
    my $_tiny_="small><small><small" ; my $_tinyX_="small></small></small ";
    my $_tabb_="<br><$_tiny_> </$_tinyX_>" ; my $_bigg_="<big> </big>";
    push @OUT, "<table width=\"100%\"><tr><td> ".$n;
    for (grep {/<$Q='[Uu]se.'>/} @$INPUTS) {
	my $x = $_;
	$x =~ />\w\w\w\w*:/ and next; # name: http: ftp: mailto: ...
	$x =~ s|<$Q='[Uu]se(.)'>([^ ]*) (.*)<$QX>|&$_form_|e;
	$x = &eval_MK_LIST("multisitemap", $x, @MK_SITE); 
	$x =~ /<name/ or next;
	$x =~ s|<!--[u]se1-->|</td><td valign=\"top\"><b>|;
	$x =~ s|<!--[e]nd1-->|</b>|;
	$x =~ s|<!--[u]se2-->|<br>|;
	$x =~ s|<!--[u]se.-->|<br>|; $x =~ s/<!--[^<>]*-->/ /g;
	$x =~ s|<name |<$_tiny_><a |; $x =~ s|</name>||;
	$x =~ s|<date>|<small style="date">|; 
	$x =~ s|</date>|</small></a><br></$_tinyX_>|;
	$x =~ s|<long>|<!--long-->|; 
	$x =~ s|</long>|<!--/long-->|;
	chomp $x;
	push @OUT, $x.$n;
    }
    push @OUT, "</td><tr></table>".$n;
    return @OUT;
}

sub make_listsitemap
{   # traditional - the body contains a list with date and title extras
    my ($INPUTS,$Z)= @_ ; $INPUTS=\@MK_DATA if not $INPUTS;
    @MK_SITE = &siteinfo2sitemap(); # have <name><long><date> addon-sed
    my @OUT = (); 
    my $_form_=sub{
	"<!--\"$2\"--><!--use$1--><name href=\"$2\">$3</name><date>......</date><long>$3</long>"};
    my $_tabb_="<td>\&nbsp\;</td>";
    push @OUT, "<table cellspacing=\"0\" cellpadding=\"0\">".$n;
    my $xx;
    for $xx (grep {/<$Q='[Uu]se.'>/} @$INPUTS) {
	my $x = "".$xx;
	$x =~ />\w\w\w\w*:/ and next;
	$x =~ s|<$Q='[Uu]se(.)'>([^ ]*) (.*)<$QX>|&$_form_|e;
	$x = &eval_MK_LIST("listsitemap", $x, @MK_SITE); 
	$x =~ /<name/ or next;
        $x =~ s|<!--[u]se(1)-->|<tr class=\"listsitemap$1\"><td>*</td>|;
        $x =~ s|<!--[u]se(2)-->|<tr class=\"listsitemap$1\"><td>-</td>|;
        $x =~ s|<!--[u]se(.)-->|<tr class=\"listsitemap$1\"><td> </td>|; 
        $x =~  /<tr.class=\"listsitemap3\">/ and $x =~ s|(<name [^<>]*>)|$1- |;
	$x =~ s|<!--[^<>]*-->| |g;
	$x =~ s|<name href=\"name:sitemap:|<name href=\"|;
        $x =~ s|<name |<td><a |;     $x =~ s|</name>|</a></td>$_tabb_|;
        $x =~ s|<date>|<td><small style="date">|; 
	$x =~ s|</date>|</small></td>$_tabb_|;
        $x =~ s|<long>|<td><em><!--long-->|;    
	$x =~ s|</long>|<!--/long--></em></td></tr>|;
        push @OUT, $x.$n; 
    }
    for $xx (grep {/<$Q='[u]se.'>/} @$INPUTS) {
	my $x = $xx; 
	$x =~ s/<$Q='[u]se.'>name:sitemap://; $x =~ s|<$QX>||; $x =~ s:\s*::gs; 
	if (-f $x) { 
	    for (grep {/<tr.class=\"listsitemap\d\">/} source($x)) {
		push @OUT, $_;
	    }
	}
    }
    push @OUT, "</table>".$n;
    return @OUT;
}

my $_xi_include_=
    "<xi:include xmlns:xi=\"http://www.w3.org/2001/XInclude\" parse=\"xml\"";

sub make_xmlsitemap
{   # traditional - the body contains a list with date and title extras
    my ($INPUTS,$Z)= @_ ; $INPUTS=\@MK_DATA if not $INPUTS;
    @MK_SITE = &siteinfo2sitemap(); # have <name><long><date> addon-sed
    my @OUT = (); 
    my $_form_=sub{"<!--\"$2\"--><name href=\"$2\">$3</name>"};
    my $xx;
    for $xx (grep {/<$Q='[Uu]se.'>/} @$INPUTS) {
	my $x = "".$xx;
	$x =~ />\w\w\w\w*:/ and next;
	$x =~ s|<$Q='[Uu]se(.)'>([^ ]*) (.*)<$QX>|&$_form_|e;
	$x = &eval_MK_LIST("listsitemap", $x, @MK_SITE); 
	$x =~ /<name/ or next;
	$x =~ m|href="${SITEFILE}"| and next;
	$x =~ m|href="${SITEFILE}l"| and next;
	$x =~ s|(href="[^<>]*)\.html(")|$1.xml$2|g;
	$x =~ s|.*<name|$_xi_include_$n   |;
	$x =~ s|>.*</name>| />|;
        push @OUT, $x.$n; 
    }
    return @OUT;
}

sub print_extension
{
    my ($ARG,$Z)= @_ ; $ARG=$o{print} if not $ARG;
    if ($ARG =~ /^([.-])/) {
	return $ARG;
    } else {
	return ".print";
    }
}

sub from_sourcefile
{
    my ($U,$Z) = @_;
    if (-f $U) {
	return $U;
    } elsif (-f "$o{src_dir}/$U") {
	return "$o{src_dir}/$U";
    } else {
	return $U;
    }
}
   
sub html_sourcefile  # generally just cut away the trailing "l" (ell)
{                    # making "page.html" argument into "page.htm" return
    my ($U,$Z) = @_;
    my $_SRCFILE_=$U; $_SRCFILE_ =~ s/l$//;
    my $_XMLFILE_=$U; $_XMLFILE_ =~ s/\.html$/.dbk/;
    if (-f $_SRCFILE_) { 
	return $_SRCFILE_;
    } elsif (-f $_XMLFILE_) { 
	return $_XMLFILE_;
    } elsif (-f "$o{src_dir}/$_SRCFILE_") {
	return "$o{src_dir}/$_SRCFILE_";
    } elsif (-f "$o{src_dir}/$_XMLFILE_") {
	return "$o{src_dir}/$_XMLFILE_";
    } else {
	return ".//$_SRCFILE_";
    }
}
sub html_printerfile_sourcefile 
{                   
    my ($U,$Z) = @_;
    if (not $printerfriendly) {
	$U =~ s/l\$//; return $U;
    } else {
	my $_ext_=&sed_slash_key(&print_extension($printerfriendly));
	$U =~ s/l\$//; $U =~ s/$_ext_([.][\w]*)$/$1/; return $U;
    }
}

sub fast_html_printerfile {
    my ($U,$V,$Z) = @_;
    my $x=&html_printerfile($U) ; return basename($x);
#   my $x=&html_printerfile($U) ; return &fast_href($x,$V);
}

sub html_printerfile # generate the printerfile for a given normal output
{
    my ($U,$Z) = @_;
    my $_ext_=&esc(&print_extension($printerfriendly));
    $U =~ s/([.][\w]*)$/$_ext_$1/; return $U; # index.html -> index.print.html
}

sub make_printerfile_fast # generate s/file.html/file.print.html/ for hrefs
{                        # we do that only for the $FILELIST
    my ($U,$Z) = @_;
    my $ALLPAGES=$U;
    my @OUT = ();
    for my $p (@$ALLPAGES) {
	my $a=&sed_slash_key($p);
	my $b=&html_printerfile($p);
	if ($b ne $p) {
	    $b =~ s:/:\\/:g;
	    push @OUT, 
	    "s/<a href=\\\"$a\\\"([^<>])*>/<a href=\\\"$b\\\"\$1>/;";
	}
    }
    return @OUT;
}

sub echo_printsitefile_style
{
    my $_bold_="text-decoration : none ; font-weight : bold ; ";
    return "   <style>"
	."$n     a:link    { $_bold_ color : #000060 ; }"
	."$n     a:visited { $_bold_ color : #000040 ; }"
	."$n     body      { background-color : white ; }"
	."$n   </style>"
	."$n";
}

sub make_printsitefile_head # $sitefile
{
    my $MK_STYLE = &echo_printsitefile_style();
    my @OUT = ();
    for (source($SITEFILE)) {
	if (/<head>/) {  push @OUT, $_; 
			 push @OUT, $MK_STYLE; next; }
	if (/<title>/) { push @OUT, $_; next; }
        if (/<\/head>/) { push @OUT, $_; next; }
	if (/<body>/) { push @OUT, $_; next; }
	if (/<link [^<>]*rel=\"shortcut icon\"[^<>]*>/) {
	    push @OUT, $_; next;
	}
    }
    return @OUT;
}

# ------------------------------------------------------------------------
# The printsitefile is a long text containing html href markups where
# each of the href lines in the file is being prefixed with the section
# relation. During a secondary call the printsitefile can grepp'ed for
# those lines that match a given output fast-file. The result is a
# navigation header with 1...3 lines matching the nesting level

# these alt-texts will be only visible in with a text-mode browser:
my $printsitefile_square="width=\"8\" height=\"8\" border=\"0\"";
my $printsitefile_img_1="<img alt=\"|go text:\" $printsitefile_square />";
my $printsitefile_img_2="<img alt=\"||topics:\" $printsitefile_square />";
my $printsitefile_img_3="<img alt=\"|||pages:\" $printsitefile_square />";
my $_SECT="mksite:sect:";

sub echo_current_line # $sect $extra
{
    # add the prefix which is used by select_in_printsitefile to cut out things
    my ($N,$M,$Z) = @_;
    return "<!--$_SECT\"$N\"-->$M";
}
sub make_current_entry # $sect $file      ## requires $MK_SITE
{
    my ($S,$R,$Z) = @_;
    my $RR=&sed_slash_key($R); 
    my $sep=" - " ; my $_left_=" [ " ; my $_right_=" ] ";
    my $name = site_get_list_title($R);
    $_ = &echo_current_line ("$S", "<a href=\"$R\">$name</a>$sep");
    if ($R eq $S) {
	s/<a href/$_left_$&/;
	s/<\/a>/$&$_right_/;
    }
    return $_;
}
sub echo_subpage_line # $sect $extra
{
    my ($N,$M,$Z) = @_;
    return "<!--$_SECT*:\"$N\"-->$M";
}

sub make_subpage_entry
{
    my ($S,$R,$Z) = @_;
    my $RR=&sed_slash_key($R);
    my $sep=" - " ;
    my $name = site_get_list_title($R);
    $_ = &echo_subpage_line ("$S", "<a href=\"$R\">$name</a>$sep");
    return $_;
}

sub make_printsitefile
{
   # building the printsitefile looks big but its really a loop over sects
    my ($INPUTS,$Z) = @_; $INPUTS=\@MK_DATA if not $INPUTS;
    @MK_SITE = &siteinfo2sitemap(); # have <name><long><date> addon-sed
    savelist(\@MK_SITE,"SITE");

    my @OUT = &make_printsitefile_head ($SITEFILE);
    my $sep=" - " ;
    my $_sect1=
	"<a href=\"#.\" title=\"section\">$printsitefile_img_1</a> ||$sep";
    my $_sect2=
	"<a href=\"#.\" title=\"topics\">$printsitefile_img_2</a> ||$sep";
    my $_sect3=
	"<a href=\"#.\" title=\"pages\">$printsitefile_img_3</a> ||$sep";

    my $_SECT1="mksite:sect1";
    my $_SECT2="mksite:sect2";
    my $_SECT3="mksite:sect3";

    @MK_SECT1 = &site_get_rootsections();
    # round one - for each root section print a current menu
    for my $r (@MK_SECT1) {
	push @OUT, &echo_current_line ("$r", "<!--$_SECT1:A--><br>$_sect1");
	for my $s (@MK_SECT1) {
	    push @OUT, &make_current_entry ("$r", "$s");
	}
	push @OUT, &echo_current_line ("$r", "<!--$_SECT1:Z-->");
    }

    # round two - for each subsection print a current and subpage menu
    for my $r (@MK_SECT1) {
    @MK_SECT2 = &site_get_subpages ("$r");
    for my $s (@MK_SECT2) {
	push @OUT, &echo_current_line ("$s", "<!--$_SECT2:A--><br>$_sect2");
	for my $t (@MK_SECT2) {
	    push @OUT, &make_current_entry ("$s", "$t");
	} # "$t"
	push @OUT, &echo_current_line  ("$s", "<!--$_SECT2:Z-->");
    } # "$s"
	my $_have_children_="";
	for my $t (@MK_SECT2) {
	    if (not $_have_children_) {
	push @OUT, &echo_subpage_line ("$r", "<!--$_SECT2:A--><br>$_sect2"); }
	    $_have_children_ .= "1";
	    push @OUT, &make_subpage_entry ("$r", "$t");
	}
	    if ($_have_children_) {
	push @OUT, &echo_subpage_line ("$r", "<!--$_SECT2:Z-->"); }
    } # "$r"

    # round three - for each subsubsection print a current and subpage menu
    for my $r (@MK_SECT1) {
    @MK_SECT2 = &site_get_subpages ("$r");
    for my $s (@MK_SECT2) {
    @MK_SECT3 = &site_get_subpages ("$s");
    for my $t (@MK_SECT3) {
	push @OUT, &echo_current_line ("$t", "<!--$_SECT3:A--><br>$_sect3");
	for my $u (@MK_SECT3) {
	    push @OUT, &make_current_entry ("$t", "$u");
	} # "$t"
	push @OUT, &echo_current_line  ("$t", "<!--$_SECT3:Z-->");
    } # "$t"
	my $_have_children_="";
	for my $u (@MK_SECT3) {
	    if (not $_have_children_) {
	push @OUT, &echo_subpage_line ("$s", "<!--$_SECT3:A--><br>$_sect3"); }
	    $_have_children_ .= "1";
	    push @OUT, &make_subpage_entry ("$s", "$u");
	}
	    if ($_have_children_) {
	push @OUT, &echo_subpage_line ("$s", "<!--$_SECT3:Z-->"); }
    } # "$s"
    } # "$r"
    push @OUT, "<a name=\".\"></a>";
    push @OUT, "</body></html>";
    savelist(\@OUT,"FORM");
    return @OUT;
}

# create a selector that can grep a printsitefile for the matching entries
sub select_in_printsitefile # arg = "page" : return to stdout >> $P.$HEAD
{
    my ($N,$Z) = @_;
    my $_selected_="$N" ; $_selected_="$F" if not $_selected_;
    my $_section_=&sed_slash_key($_selected_);
    my @OUT = ();
    push @OUT, "s/^<!--$_SECT\\\"$_section_\\\"-->//;";        # sect3
    push @OUT, "s/^<!--$_SECT\[*\]:\\\"$_section_\\\"-->//;";    # children
    $_selected_=&site_get_parentpage($_selected_);
    $_section_=&sed_slash_key($_selected_);
    push @OUT, "s/^<!--$_SECT\\\"$_section_\\\"-->//;";        # sect2
    $_selected_=&site_get_parentpage($_selected_);
    $_section_=&sed_slash_key($_selected_);
    push @OUT, "s/^<!--$_SECT\\\"$_section_\\\"-->//;";        # sect1
    push @OUT, "/^<!--$_SECT\\\"[^\\\"]*\\\"-->/ and next;";
    push @OUT, "/^<!--$_SECT\[*\]:\\\"[^\\\"]*\\\"-->/ and next;";
    push @OUT, "s/^<!--mksite:sect[$NN]:[$AZ]-->//;";
    return @OUT;
}

sub body_for_emailfooter
{
    return "" if $emailfooter eq "no";
    my $_email_=$emailfooter; $_email_ =~ s|[?].*||;
    my $_dated_=&info_get_entry("updated");
    return "<hr><table border=\"0\" width=\"100%\"><tr><td>"
	."$n"."<a href=\"mailto:$emailfooter\">$_email_</a>"
	."$n"."</td><td align=\"right\">"
	."$n"."$_dated_</td></tr></table>"
	."$n";
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

sub css_sourcefile
{
    my ($X,$XXX) = @_;
    return $X if -f $X;
    return "$o{src_dir}/$X" if -f "$o{src_dir}/$X";
    return "$X" if "$X" =~ m:^/:;
    return "./$X";
}

my %XMLTAGS = ();
sub css_xmltags # $SOURCEFILE
{
    my $X=$SOURCEFILE;
    my %R = ();
    my $line;
    foreach $line (source($SOURCEFILE)) {
	$line =~ s|>[^<>]*<|><|g;
	$line =~ s|^[^<>]*<|<|;
	$line =~ s|>[^<>]*\$|>|;
	my $item;
	foreach $item (split /</, $line) {
	    $item =~ m:^/: and next;
	    $item =~ m:^\s*$: and next;
	    $item !~ m|>| and next;
	    $item =~ s|>.*||;
	    chomp $item;
	    $R{$item} = "";
	}
    }
    @{$XMLTAGS{$X}} = keys %R;
}

my %XMLSTYLESHEETS = ();
sub css_xmlstyles # $SOURCEFILE
{
    my $X=$SOURCEFILE;
    my %R = ();
    my $text = "";
    my $line = "";
    foreach $line (source($SOURCEFILE)) {
	$text .= $line;
	$text =~ s|<link  *rel=[\'\"]*stylesheet|<?xml-stylesheet |;
	if ($text !~ m/<.xml-stylesheet/) { $text = ""; next; }
	if ($text !~ m/href=/) { next; }
	$text =~ s|^.*<.xml-stylesheet||;
	$text =~ s|^.*href=[\"\']||; $text =~ s|[\"\'].*||s;
	chomp $text;
	$R{$text} = "";
    }
    foreach $line (source($SITEFILE)) {
	$text .= $line;
	$text =~ s|<link  *rel=[\'\"]*stylesheet|<?xml-stylesheet |;
	if ($text !~ m/<.xml-stylesheet/) { $text = ""; next; }
	if ($text !~ m/href=/) { next; }
	$text =~ s|^.*<.xml-stylesheet||;
	$text =~ s|^.*href=[\"\']||; $text =~ s|[\"\'].*||s;
	chomp $text;
	$R{$text} = "";
    }
    @{$XMLSTYLESHEETS{$X}} = keys %R;
}

my %XMLTAGSCSS = ();
sub css_xmltags_css # $SOURCEFILE
{
    my $X=$SOURCEFILE;
    my @S = $XMLTAGS{$X};
    my @R = ();
    my $xmlstylesheet;
    foreach $xmlstylesheet (@{$XMLSTYLESHEETS{$X}}) {
	my $stylesheet = css_sourcefile($xmlstylesheet);
	if (-f $stylesheet) {
	    push @R, "/* $xmlstylesheet */";
	    my $text = "";
	    my $line = "";
	    my $STYLESHEET = $stylesheet;
	    open STYLESHEET, "<$STYLESHEET" or next;
	    foreach $line (<STYLESHEET>)
	    {
		$text .= $line;
		if ($text =~ /^[^\{]*\}/s) { $text = ""; next; }
		if ($text !~ /^[^\{]*\{.*\}/s) { next; }
		$text =~ s|\r||g;
		my $xmltag; my $found = 0;
		foreach $xmltag (grep /^\w/, @{$XMLTAGS{$X}}) {
		    $xmltag =~ s| .*||;
		    if (grep {$_ eq $xmltag} qw/title section/) {
			next if $xmltag eq "section";
			$found++ if $text =~ 
			    /\b$xmltag\s*(?:,[^{},]*)*\s*\{/s;
			my $xmlparent;
			foreach $xmlparent (@{$XMLTAGS{$X}}) {
			    $xmlparent =~ s| .*||;
			    /^\w/ or next;
			    $found++ if $text =~ 
				/\b$xmlparent\s+$xmltag\s*(?:,[^{},]*)*\s*\{/s;
			}
		    } else {
			$found++ if $text =~ 
			    /\b$xmltag\s*(?:,[^\{\},]*)*\{/s;
		    }
		    last if $found;
		}
		if (not $found) { $text = ""; next; }
		foreach $xmltag (grep /^\w/, @{$XMLTAGS{$X}}) { 
		    $xmltag =~ s| .*||;
		    if (grep {$_ eq $xmltag} @HTMLTAGS) { next; }
		    if (grep {$_ eq $xmltag} @HTMLTAGS2) { next; }
		    $text =~ s/(\b$xmltag\s*(?:,[^{},]*)*\s*\{)/.$1/gs;
		}
		chomp $text;
		push @R, $text; $text = ""; next;
	    }	    
	} else {
	    warn "$xmlstylesheet : ERROR, no such stylesheet $xmlstylesheet";
	}
    }
    @{$XMLTAGSCSS{$X}} = @R;
}

my %XMLMAPPING = ();
sub css_xmlmapping # $SOURCEFILE
{
    my $X=$SOURCEFILE;
    my %R = ();
    foreach (@{$XMLTAGSCSS{$X}}) {
	my $span = "";
	$span="li"      if /\bdisplay\s*:\s*list-item\b/;
	$span="caption" if /\bdisplay\s*:\s*table-caption\b/;
	$span="td"      if /\bdisplay\s*:\s*table-cell\b/;
	$span="tr"      if /\bdisplay\s*:\s*table-row\b/;
	$span="table"   if /\bdisplay\s*:\s*table\b/;
	$span="div"     if /\bdisplay\s*:\s*block\b/;
	$span="span"    if /\bdisplay\s*:\s*inline\b/;
	$span="small"    if /\bdisplay\s*:\s*none\b/;
	$span="ul"  if /\blist-style-type\s*:\s*disc\b/    and $span eq "div";
	$span="ol"  if /\blist-style-type\s*:\s*decimal\b/ and $span eq "div";
	$span="tt"  if /\bfont-family\s*:\s*monospace\b/   and $span eq "span";
	$span="em"  if /\bfont-style\s*:\s*italic\b/       and $span eq "span";
	$span="b"   if /\bfont-weight\s*:\s*bold\b/        and $span eq "span";
	$span="pre" if /\bwhite-space\s*:\s*pre\b/         and $span eq "div";
	my $xmltag;
	for $xmltag (grep /^\w/, @{$XMLTAGS{$X}}) { 
	    $xmltag =~ s| .*||;
	    if (/\.$xmltag\b/s) { 
		$R{$xmltag} = $span;
		$R{$xmltag} = "p" if $xmltag eq "para" and $span eq "div";
		$R{$xmltag} = "a" if $xmltag eq "ulink" and $span eq "span";
	    }
	}
    }
    %{$XMLMAPPING{$X}} = %R;
}

sub css_scan # $SOURCEFILE
{
    css_xmltags ();
    css_xmlstyles ();
    css_xmltags_css ();
    css_xmlmapping ();
}

sub tags2span_sed # $SOURCEFILE > $++
{
    my $X=$SOURCEFILE;
    my $xmltag;
    my @R = ();
    push @R, "s|<[?]xml-stylesheet[^<>]*[?]>||";
    push @R, "s|<link  *rel=['\"]*stylesheet[^<>]*>||";
    push @R, "s|<section[^<>]*>||g;";
    push @R, "s|</section[^<>]*>||g;";
    for $xmltag (grep /^\w/, @{$XMLTAGS{$X}}) { 
	$xmltag =~ s| .*||;
	if (grep {$_ eq $xmltag} @HTMLTAGS) { next; }
	if (grep {$_ eq $xmltag} @HTMLTAGS2) { next; }
	my $span = $XMLMAPPING{$X}{$xmltag};
	$span = "span" if $span eq "";
	push @R, "s|<$xmltag([\\n\\t ][^<>]*)url=|<$span class=\"$xmltag\"\$1href=|g;";
	push @R, "s|<$xmltag([\\n\\t >])|<$span class=\"$xmltag\"\$1|g;";
	push @R, "s|</$xmltag([\\n\\t >])|</$span\$1|g;";
    }
    my $xmlstylesheet;
    foreach $xmlstylesheet (@{$XMLSTYLESHEETS{$X}}) {
	my $H="[^<>]*href=[\'\"]${xmlstylesheet}[\'\"][^<>]*";
	push @R, "s|<[?]xml-stylesheet$H>||;";
	push @R, "s|<link[^<>]* rel=['\"]*stylesheet['\"]$H>||;";
    }
    return @R;
}

sub tags2meta_sed # $SOURCEFILE > $++
{
    my @R = ();
    push @R, " <style type=\"text/css\"><!--";
    push @R, map {s/(^|\n)/$1  /g;$_} @{$XMLTAGSCSS{$SOURCEFILE}};
    push @R, " --></style>";
    @R = () if $#R < 3;
    return @R;
}

# ==========================================================================
# xml/docbook support is taking an dbk input file converting any html    DBK
# syntax into pure docbook tagging. Each file is being given a docbook
# doctype so that an xml/docbook viewer can render it correctly - that
# is needed atleast since docbook files do not embed stylesheet infos.
# Most of the processing is related to remap html markup and some other
# shortcut markup into correct docbook markup. The result is NOT checked
# for being well-formed or even matching the docbook schema DTD at all.

sub scan_xml_rootnode
{
    my ($INF,$XXX) = @_;
    $INF = \@{$DATA{$F}} if not $INF;
    for my $entry (source($SOURCEFILE)) {
	my $line = $entry; next if $line !~ /<\w/;
	$line =~ s/<(\w*).*/$1/s;
	# print ":",$line,$n;
	push @{$INF}, "<!root $F>$line";
	return;
    }
}

sub get_xml_rootnode
{
    my ($INF,$XXX) = @_;
    $INF = \@{$DATA{$F}} if not $INF;
    my $_file_ = sed_slash_key($F);
    foreach my $entry (grep /^<!root $_file_>/, @{$INF}) {
	my $line=$entry; $line =~ s|.*>||; 
	return $line;
    }
}

sub xml_sourcefile
{
    my ($X,$XXX) = @_;
    my $XMLFILE=$X; $XMLFILE =~ s/\.xml$/.dbk/;
    my $SRCFILE=$X; $SRCFILE =~ s/\.xml$/.htm/;
    $XMLFILE="///" if $X eq $XMLFILE;
    $SRCFILE="///" if $X eq $SRCFILE;
    return $XMLFILE if -f $XMLFILE;
    return $SRCFILE if -f $SRCFILE;
    return "$o{src_dir}/$XMLFILE" if -f "$o{src_dir}/$XMLFILE";
    return "$o{src_dir}/$SRCFILE" if -f "$o{src_dir}/$SRCFILE";
    return ".//$XMLFILE"; # $++ (not found?)
}

sub scan_xmlfile
{
    $SOURCEFILE= &xml_sourcefile($F);
    hint "'$SOURCEFILE': scanning xml -> '$F'";
    scan_xml_rootnode();
    my $rootnode=&get_xml_rootnode(); $rootnode =~ s|^(h\d.*$)|$1 <?section?>|;
    hint "'$SOURCEFILE': rootnode ('$rootnode')";
}

sub make_xmlfile
{
    $SOURCEFILE= &xml_sourcefile($F);
    my $X=$SOURCEFILE;
    my $article= &get_xml_rootnode();
    $article="article" if $article eq "";
    my $text = "";
    $text .= '<!DOCTYPE '.$article.
	' PUBLIC "-//OASIS//DTD DocBook XML V4.4//EN"'.$n;
    $text .= '    "http://www.oasis-open.org/docbook/xml/4.4/docbookx.dtd">'
	.$n;
    for my $stylesheet (@{$XMLSTYLESHEETS{$X}}) {
	$text .= "<?xml-stylesheet type=\"text/css\" href=\"$stylesheet\"   ?>"
	    .$n;
    }
    for (source($SOURCEFILE)) {	
	s!<>!\&nbsp\;!g;
	s!(&)(&)!${1}amp;${2}amp;!g;
	s!(<[^<>]*)(width)(=)(\d+\%*)!$1$2$3\"$4\"!g;
	s!(<[^<>]*)(cellpadding)(=)(\d+\%*)!$1$2$3\"$4\"!g;
	s!(<[^<>]*)(border)(=)(\d+\%*)!$1$2$3\"$4\"!g;
	s!<[?]xml-stylesheet[^<>]*>!!;
	s!<link[^<>]* rel=[\'\"]*stylesheet[^<>]*>!!;
	s!<[hH]\d!<title!g;
	s!</[hH]\d!</title!g;
	s!(</title> *)([^<>]*\w[^<>\r\n]*)$!$1<sub>$2</sub>!;
	s!(</title>.*)<sub>!$1<subtitle>!g;
	s!(</title>.*)</sub>!$1</subtitle>!g;
	s!(<section>[^<>]*)(<date>.*</date>[^<>\n]*)$!$1<sectioninfo>$2</sectioninfo>!gx;
        s!<em>!<emphasis>!g;
        s!</em>!</emphasis>!g;
        s!<i>!<emphasis>!g;
        s!</i>!</emphasis>!g;
        s!<b>!<emphasis role=\"bold\">!g;
        s!</b>!</emphasis>!g;
        s!<u>!<emphasis role=\"underline\">!g;
        s!</u>!</emphasis>!g;
        s!<big>!<emphasis role=\"strong\">!g;
        s!</big>!</emphasis>!g;
        s!<(s|strike)>!<emphasis role=\"strikethrough\">!g;
        s!</(s|strike)>!</emphasis>!g;
        s!<center>!<blockquote><para>!g; 
        s!</center>!</para></blockquote>!g; 
        s!<p align=(\"\w*\")>!<para role=${1}>!g; 
        s!<[pP]>!<para>!g; 
        s!</[pP]>!</para>!g; 
        s!<(pre|PRE)>!<screen>!g; 
        s!</(pre|PRE)>!</screen>!g; 
        s!<a( [^<>]*)name=([^<>]*)/>!<anchor ${1}id=${2}/>!g;
        s!<a( [^<>]*)name=([^<>]*)>!<anchor ${1}id=${2}/>!g;
        s!<a( [^<>]*)href=!<ulink${1}url=!g;
        s!</a>!</ulink>!g;
	s! remap=\"url\">[^<>]*</ulink>! />!g;
	s!<(/?)span(\s[^<>]*)?>!<${1}phrase${2}>!g;
	s!<small(\s[^<>]*)?>!<phrase role=\"small\"${1}>!g;
	s!</small(\s[^<>]*)?>!</phrase${1}>!g;
	s!<(/?)(sup)>!<${1}superscript>!g;
	s!<(/?)(sub)>!<${1}subscript>!g;
	s!(<)(li)(><)!${1}listitem${3}!g;
	s!(></)(li)(>)!${1}listitem${3}!g;
	s!(<)(li)(>)!${1}listitem${3}<para>!g;
	s!(</)(li)(>)!</para>${1}listitem${3}!g;
	s!(</?)(ul)>!${1}itemizedlist>!g;
	s!(</?)(ol)>!${1}orderedlist>!g;
	s!(</?)(dl)>!${1}variablelist>!g;
	s!<(/?)DT>!<${1}dt>!g;
	s!<(/?)DD>!<${1}dd>!g;
	s!<(/?)DL>!<${1}dl>!g;
	s!<BLOCKQUOTE>!<blockquote><para>!g;
	s!</BLOCKQUOTE>!</para></blockquote>!g;
	s!<(/?)dl>!<${1}variablelist>!g;	
	s!<dt\b([^<>]*)>!<varlistentry${1}><term>!g;
	s!</dt\b([^<>]*)>!</term>!g;
	s!<dd\b([^<>]*)><!<listitem${1}><!g;
	s!></dd\b([^<>]*)>!></listitem></varlistentry>!g;
	s!<dd\b([^<>]*)>!<listitem${1}><para>!g;
	s!</dd\b([^<>]*)>!</para></listitem></varlistentry>!g;
	s!<table[^<>]*><tr><td>(<table[^<>]*>)!$1!;
	s!(</table>)</td></tr></table>!$1!;
	s!<table\b([^<>]*)>!<informaltable${1}><tgroup cols=\"2\"><tbody>!g;
	s!</table\b([^<>]*)>!</tbody></tgroup></informaltable>!g;
	s!(</?)tr(\s[^<>]*)?>!${1}row${2}>!g;
	s!(</?)td(\s[^<>]*)?>!${1}entry${2}>!g;
	s!(<informaltable[^<>]*\swidth=\"100\%\")!$1 pgwide=\"1\"!g;
	s!(<tgroup[<>]*\scols=\"2\">)(<tbody>)
	    !$1<colspec colwidth=\"1*\" /><colspec colwidth=\"1*\" />$2!gx;
	s!(<entry[^<>]*\s)width=(\"\d*\%*\")!${1}remap=${2}!g;
	s!<nobr>([\'\`]*)<tt>!<cmdsynopsis><command>$1!g;
	s!</tt>([\'\`]*)</nobr>!$1</command></cmdsynopsis>!g;
	s!<nobr><(tt|code)>([\`\"\'])!<cmdsynopsis><command>$2!g;
	s!<(tt|code)><nobr>([\`\"\'])!<cmdsynopsis><command>$2!g;
	s!([\`\"\'])</(tt|code)></nobr>!$1</command></cmdsynopsis>!g;
	s!([\`\"\'])</nobr></(tt|code)>!$1</command></cmdsynopsis>!g;
	s!(</?)tt>!${1}constant>!g;
	s!(</?)code>!${1}literal>!g;
	s!<br>!<br />!g;
	s!<br */>!<screen role=\"linebreak\">\n</screen>!g;
	$text .= $_; 
    }
    open F, ">$F" or die "could not write $F: $!"; print F $text; close F;
    echo "'$SOURCEFILE': ",&ls_s($SOURCEFILE)," >> ",&ls_s($F);
}

sub make_xmlmaster
{
    $SOURCEFILE= &xml_sourcefile($F);
    my $X=$SOURCEFILE;
    my $article="section"; # book? chapter?
    my $text = "";
    $text .= '<!DOCTYPE '.$article.
	' PUBLIC "-//OASIS//DTD DocBook XML V4.4//EN"'.$n;
    $text .= '    "http://www.oasis-open.org/docbook/xml/4.4/docbookx.dtd">'
	.$n;
    for my $stylesheet (@{$XMLSTYLESHEETS{$X}}) {
	$text .= "<?xml-stylesheet type=\"text/css\" href=\"$stylesheet\"   ?>"
	    .$n;
    }
    # $text .= "<section><sectioninfo><date/><authorblurb/></sectioninfo>...";
    $text .= "<section><title>Documentation</title>$n";
    for (make_xmlsitemap()) {
	$text .= $_; 
    }
    $text .= "</section>$n";
    open F, ">$F" or die "could not write $F: $!"; print F $text; close F;
    echo "'$SOURCEFILE': ",&ls_s($SOURCEFILE)," >*> ",&ls_s($F);
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
my $S="\\&nbsp\\;";
# S="[&]nbsp[;]"

# HR and EM style markups must exist in input - BR sometimes left out 
# these routines in(ter)ject hardspace before, between, after markups
# note that "<br>" is sometimes used with HR - it must exist in input
sub echo_HR_EM_PP
{
    my ($U,$V,$W,$X,$Z) = @_;
    my @list = (
		"s%^($U$V$W*<a) (href=)%\$1 $X \$2%;",
		"s%^(<>$U$V$W*<a) (href=)%\$1 $X \$2%;",
		"s%^($S$U$V$W*<a) (href=)%\$1 $X \$2%;",
		"s%^($U<>$V$W*<a) (href=)%\$1 $X \$2%;",
		"s%^($U$S$V$W*<a) (href=)%\$1 $X \$2%;",
		"s%^($U$V<>$W*<a) (href=)%\$1 $X \$2%;",
		"s%^($U$V$S$W*<a) (href=)%\$1 $X \$2%;" );
    return @list;
}

sub echo_br_EM_PP
{
    my ($U,$V,$W,$X,$Z) = @_;
    my @list = &echo_HR_EM_PP  ("$U", "$V", "$W", "$X");
    my @listt = (
		 "s%^($V$W*<a) (href=)%\$1 $X \$2%;",
		 "s%^(<>$V$W*<a) (href=)%\$1 $X \$2%;",
		 "s%^($S$V$W*<a) (href=)%\$1 $X \$2%;",
		 "s%^($V<>$W*<a) (href=)%\$1 $X \$2%;",
		 "s%^($V$S$W*<a) (href=)%\$1 $X \$2%;",
		 "s%^($V$W*<><a) (href=)%\$1 $X \$2%;",
		 "s%^($V$W*$S<a) (href=)%\$1 $X \$2%;" );
    push @list, @listt;
    return @list;
}    

sub echo_HR_PP
{
    my ($U,$V,$W,$Z) = @_;
    my @list = (
		"s%^($U<a) (href=)%\$1 $W \$2%;",
		"s%^($U$V*<a) (href=)%\$1 $W \$2%;",
		"s%^(<>$U$V*<a) (href=)%\$1 $W \$2%;",
		"s%^($S$U$V*<a) (href=)%\$1 $W \$2%;",
		"s%^($U<>$V*<a) (href=)%\$1 $W \$2%;",
		"s%^($U$S$V*<a) (href=)%\$1 $W \$2%;" );
    return @list;
}
sub echo_br_PP
{
    my ($U,$V,$W,$Z) = @_;
    my @list = &echo_HR_PP ("$U", "$V", "$W");
    my @listt = (
		 "s%^($V*<a) (href=)%\$1 $W \$2%;",
		 "s%^(<>$V*<a) (href=)%\$1 $W \$2%;",
		 "s%^($S$V*<a) (href=)%\$1 $W \$2%;" );
    push @list, @listt;
    return @list;
}
sub echo_sp_PP
{
    my ($U,$V,$Z) = @_;
    my @list = (
		"s%^(<>$U*<a) (href=)%\$1 $V \$2%;",
		"s%^($S$U*<a) (href=)%\$1 $V \$2%;",
		"s%^(<><>$U*<a) (href=)%\$1 $V \$2%;",
		"s%^($S$S$U*<a) (href=)%\$1 $V \$2%;",
		"s%^(<>$U<>*<a) (href=)%\$1 $V \$2%;",
		"s%^($S$U$S*<a) (href=)%\$1 $V \$2%;",
		"s%^($U<><>*<a) (href=)%\$1 $V \$2%;",
		"s%^($U$S$S*<a) (href=)%\$1 $V \$2%;",
		"s%^($U<>*<a) (href=)%\$1 $V \$2%;",
		"s%^($U$S*<a) (href=)%\$1 $V \$2%;" );
    return @list;
}
sub echo_sp_SP
{
    my ($U,$V,$Z) = @_;
    my @list = (
		"s%^($U<a) (href=)%\$1 $V \$2%;",
		"s%^(<>$U<a) (href=)%\$1 $V \$2%;",
		"s%^($S$U<a) (href=)%\$1 $V \$2%;",
		"s%^(<><>$U<a) (href=)%\$1 $V \$2%;",
		"s%^($S$S$U<a) (href=)%\$1 $V \$2%;",
		"s%^(<>$U<><a) (href=)%\$1 $V \$2%;",
		"s%^($S$U$S<a) (href=)%\$1 $V \$2%;",
		"s%^($U<><><a) (href=)%\$1 $V \$2%;",
		"s%^($U$S$S<a) (href=)%\$1 $V \$2%;",
		"s%^($U<><a) (href=)%\$1 $V \$2%;",
		"s%^($U$S<a) (href=)%\$1 $V \$2%;" );
    return @list;
}
sub echo_sp_sp
{
    my ($U,$V,$Z) = @_;
    my @list = (
		"s%^($U<a) (name=)%\$1 $V \$2%;",
		"s%^(<>$U<a) (name=)%\$1 $V \$2%;",
		"s%^($S$U<a) (name=)%\$1 $V \$2%;",
		"s%^(<><>$U<a) (name=)%\$1 $V \$2%;",
		"s%^($S$S$U<a) (name=)%\$1 $V \$2%;",
		"s%^(<>$U<><a) (name=)%\$1 $V \$2%;",
		"s%^($S$U$S<a) (name=)%\$1 $V \$2%;",
		"s%^($U<><><a) (name=)%\$1 $V \$2%;",
		"s%^($U$S$S<a) (name=)%\$1 $V \$2%;",
		"s%^($U<><a) (name=)%\$1 $V \$2%;",
		"s%^($U$S<a) (name=)%\$1 $V \$2%;" );
    return @list;
}

sub make_sitemap_init
{
    # build a list of detectors that map site.htm entries to a section table
    # note that the resulting .gets.tmp / .puts.tmp are real sed-script
    my $h1="[-|[]";
    my $b1="[*=]";
    my $b2="[-|[]";
    my $b3="[\\/:]";
    my $q3="[\\/:,[]";
    @MK_GETS = ();
    push @MK_GETS, &echo_HR_PP   ("<hr>",            "$h1",    "sect=\\\"1\\\"");
    push @MK_GETS, &echo_HR_EM_PP("<hr>","<em>",     "$h1",    "sect=\\\"1\\\"");
    push @MK_GETS, &echo_HR_EM_PP("<hr>","<strong>", "$h1",    "sect=\\\"1\\\"");
    push @MK_GETS, &echo_HR_PP   ("<br>",          , "$b1$b1", "sect=\\\"1\\\"");
    push @MK_GETS, &echo_HR_PP   ("<br>",          , "$b2$b2", "sect=\\\"2\\\"");
    push @MK_GETS, &echo_HR_PP   ("<br>",          , "$b3$b3", "sect=\\\"3\\\"");
    push @MK_GETS, &echo_br_PP   ("<br>",          , "$b2$b2", "sect=\\\"2\\\"");
    push @MK_GETS, &echo_br_PP   ("<br>",          , "$b3$b3", "sect=\\\"3\\\"");
    push @MK_GETS, &echo_br_EM_PP("<br>","<small>" , "$q3"   , "sect=\\\"3\\\"");
    push @MK_GETS, &echo_br_EM_PP("<br>","<em>"    , "$q3"   , "sect=\\\"3\\\"");
    push @MK_GETS, &echo_br_EM_PP("<br>","<u>"     , "$q3"   , "sect=\\\"3\\\"");
    push @MK_GETS, &echo_HR_PP   ("<br>",          , "$q3"   , "sect=\\\"3\\\"");
    push @MK_GETS, &echo_br_PP   ("<u>",           , "$b2"   , "sect=\\\"2\\\"");
    push @MK_GETS, &echo_sp_PP   (                   "$q3"   , "sect=\\\"3\\\"");
    push @MK_GETS, &echo_sp_SP   (                   ""      , "sect=\\\"2\\\"");
    push @MK_GETS, &echo_sp_sp   (                   "$q3"   , "sect=\\\"9\\\"");
    push @MK_GETS, &echo_sp_sp   ("<br>",                      "sect=\\\"9\\\"");
    @MK_PUTS = map { my $x=$_; $x =~ s/(>)(\[)/$1 *$2/; $x } @MK_GETS;
    # the .puts.tmp variant is used to <b><a href=..></b> some hrefs which
    # shall not be used otherwise for being generated - this is nice for
    # some quicklinks somewhere. The difference: a whitspace "<hr> <a...>"
}

my $_uses_= sub{"<$Q='use$1'>$2 $3<$QX>" }; 
my $_name_= sub{"<$Q='use$1'>name:$2 $3<$QX>" }; 

sub make_sitemap_list
{
    my ($V,$Z) = @_; $V = $SITEFILE if not $V;
    # scan sitefile for references pages - store as "=use+=href+ anchortext"
    for (source($V)) {
	my $x = $_;
	local $_ = &eval_MK_LIST("sitemap_list", $x, @MK_GETS);
	/<a sect=\"[$NN]\"/ or next;
	chomp;
	s{.*<a sect=\"([^\"]*)\" href=\"([^\"]*)\"[^<>]*>(.*)</a>.*}{&$_uses_}e;
	s{.*<a sect=\"([^\"]*)\" name=\"([^\"]*)\"[^<>]*>(.*)</a>.*}{&$_name_}e;
	s{.*<a sect=\"([^\"]*)\" name=\"([^\"]*)\"[^<>]*>(.*)}{&$_name_}e;
	/^<$Q=/ or next;
	/^<!/ and next; 
	push @MK_DATA, $_;
    }
}

my $_Uses_= sub{"<$Q='Use$1'>$2 $3<$QX>" }; 
my $_Name_= sub{"<$Q='Use$1'>name:$2 $3<$QX>" }; 

sub make_subsitemap_list # file-to-scan
{
    my ($V,$W,$Z) = @_; $V = $SITEFILE if not $V;
    # scan sitefile for references pages - store as "=use+=href+ anchortext"
    for (source($V)) {
	my $x = $_; 
	local $_ = &eval_MK_LIST("subsitemap_list", $x, @MK_GETS);
	/<a sect=\"[$NN]\"/ or next;
	chomp;
	s{.*<a sect=\"([^\"]*)\" href=\"([^\"]*)\"[^<>]*>(.*)</a>.*}{&$_Uses_}e;
	s{.*<a sect=\"([^\"]*)\" name=\"([^\"]*)\"[^<>]*>(.*)</a>.*}{&$_Name_}e;
	s{.*<a sect=\"([^\"]*)\" name=\"([^\"]*)\"[^<>]*>(.*)}{&$_Name_}e;
	/^<$Q=/ or next;
	/^<!/ and next;
	s|>([^:./][^:./]*[./])|>$W$1|;
	push @MK_DATA, $_;
    }
}

sub make_sitemap_sect
{
    # scan used pages and store prime section group relation =sect= and =node=
    # (A) each "use1" creates "=sect=href+ href1" for all following non-"use1"
    # (B) each "use1" creates "=node=href2 href1" for all following "use2"
    my $sect = "";
    for (grep {/<$Q='[u]se.'>/} @MK_DATA) {
	if (/<$Q='[u]se1'>([^ ]*) .*/) { $sect = $1; }
	my $x = $_; # chomp $x; 
	$x =~ s|<$Q='[u]se.'>([^ ]*) .*|<$Q='sect'>$1 $sect<$QX>|; 
	push @MK_DATA, $x;
    }
    for (grep {/<$Q='[u]se.'>/} @MK_DATA) {
	if (/<$Q='[u]se1'>([^ ]*) .*/) { $sect = $1; }
	/<$Q='[u]se[13456789]'>/ and next;
	my $x = $_; # chomp $x; 
	$x =~ s|<$Q='[u]se.'>([^ ]*) .*|<$Q='node'>$1 $sect<$QX>|;
	push @MK_DATA, $x;
    }
}

sub make_sitemap_page
{
    # scan used pages and store secondary group relation =page= and =node=
    # the parenting =node= for use3 is usually a use2 (or use1 if none there)
    my $sect = "";
    for (grep {/<$Q='[u]se.'>/} @MK_DATA) {
	if (/<$Q='[u]se1'>([^ ]*) .*/) { $sect = $1; }
	if (/<$Q='[u]se2'>([^ ]*) .*/) { $sect = $1; }
	/<$Q='[u]se[1]'>/ and next;
	my $x = $_; 
	$x =~ s|<$Q='[u]se.'>([^ ]*) .*|<$Q='page'>$1<$QX>|; chomp $x;
	push @MK_DATA, "$x $sect";
    }
    for (grep {/<$Q='[u]se.'>/} @MK_DATA) {
	if (/<$Q='[u]se1'>([^ ]*) .*/) { $sect = $1; }
	if (/<$Q='[u]se2'>([^ ]*) .*/) { $sect = $1; }
	/<$Q='[u]se[12456789]'>/ and next; 
	my $x = $_; 
	$x =~ s/<$Q='[u]se.'>([^ ]*) .*/<$Q='node'>$1<$QX>/; chomp $x;
	push @MK_DATA, "$x $sect"; ## print "(",$_,")","$x $sect", $n;
    }
    # and for the root sections we register ".." as the parenting group
    for (grep {/<$Q='[u]se1'>/} @MK_DATA) {
	my $x = $_; $x = trimm($x); 
	$x =~ s/<$Q='[u]se.'>([^ ]*) .*/<$Q='node'>$1 ..<$QX>/; chomp $x;
	push @MK_DATA, $x;
    }
}
sub echo_site_filelist
{
    my @OUT = ();
    for (grep {/<$Q='[u]se.'>/} @MK_DATA) {
	my $x = $_; $x =~ s/<$Q='[u]se.'>//; $x =~ s/ .*[\n]*//; 
	push @OUT, $x;
    }
    return @OUT;
}

# ==========================================================================
# originally this was a one-pass compiler but the more information
# we were scanning out the more slower the system ran - since we
# were rescanning files for things like section information. Now
# we scan the files first for global information.
#                                                                    1.PASS

sub scan_sitefile # $F
{
    $SOURCEFILE=&html_sourcefile($F);
    hint "'$SOURCEFILE': scanning -> sitefile";
    if ($SOURCEFILE ne $F) {
	dx_init "$F";
	dx_text ("today", &timetoday());
	my $short=$F; 
	$short =~ s:.*/::; $short =~ s:[.].*::; # basename for all exts
	$short .=" ~";
	DC_meta ("title", "$short");
	DC_meta ("date.available", &timetoday());
	DC_meta ("subject", "sitemap");
	DC_meta ("DCMIType", "Collection");
	DC_VARS_Of ($SOURCEFILE) ; HTTP_VARS_Of ($SOURCEFILE) ;
	DC_modified ($SOURCEFILE) ; DC_date ($SOURCEFILE);
	DC_section ($F);
	DX_text ("date.formatted", &timetoday());
	if ($printerfriendly) {
	    DX_text ("printerfriendly", fast_html_printerfile($F)); }
	if ($ENV{USER}) { DC_publisher ($ENV{USER}); }
	echo "'$SOURCEFILE': $short (sitemap)";
	site_map_list_title ($F, "$short");
	site_map_long_title ($F, "generated sitemap index");
	site_map_list_date  ($F, &timetoday());
    }
}

sub scan_htmlfile # "$F"
{
    my ($FF,$Z) = @_;
    $SOURCEFILE=&html_sourcefile($F);                                  # SCAN :
    hint "'$SOURCEFILE': scanning -> $F";                              # HTML :
    if ($SOURCEFILE ne $F) {
    if ( -f $SOURCEFILE) {
	dx_init "$F";
	dx_text ("today", &timetoday());
	dx_text ("todays", &timetodays());
	DC_VARS_Of ($SOURCEFILE); HTTP_VARS_Of ($SOURCEFILE);
	DC_title ($SOURCEFILE);
	DC_isFormatOf ($SOURCEFILE);
	DC_modified ($SOURCEFILE);
	DC_date ($SOURCEFILE); DC_date ($SITEFILE);
	DC_section ($F);  DC_selected ($F);  DX_alternative ($SOURCEFILE);
	if ($ENV{USER}) { DC_publisher ($ENV{USER}); }
	DX_text ("date.formatted", &timetoday());
	if ($printerfriendly) {
	    DX_text ("printerfriendly", fast_html_printerfile($F)); }
	my $sectn=&info_get_entry("DC.relation.section");
	my $short=&info_get_entry("DC.title.selected");
	&site_map_list_title ($F, "$short");
	&info_map_list_title ($F, "$short");
	my $title=&info_get_entry("DC.title");
	&site_map_long_title ($F, "$title");
	&info_map_long_title ($F, "$title");
	my $edate=&info_get_entry("DC.date");
	my $issue=&info_get_entry("issue");
	&site_map_list_date ($F, "$edate");
	&info_map_list_date ($F, "$edate");
        css_scan();
	echo "'$SOURCEFILE':  '$title' ('$short') @ '$issue' ('$sectn')";
    }else {
	echo "'$SOURCEFILE': does not exist";
	site_map_list_title ($F, "$F");
	site_map_long_title ($F, "$F (no source)");
    } 
    } else {
	echo "<$F> - skipped - ($SOURCEFILE)";
    }
}

sub scan_subsitemap_long
{
    my ($V,$W,$ZZZ) = @_;
    for (source($V)) {
	my $x = $_;
	if ($x =~ m|<a href="([^\"]*)">.*<small style="date">([^<>]*)</small>|) {
	    &site_map_list_date($W.$1,$2);
	}
	if ($x =~ m|<a href="([^\"]*)">.*<!--long-->([^<>]*)<!--/long-->|) {
	    &site_map_long_title($W.$1,$2);
	}
    }
}

sub scan_namespec 
{
    # nothing so far
    # my ($F,$ZZZ) = @_;
    if ($F =~ /^name:sitemap:/) {
	my $short=$F; 
	$short =~ s:.*/::; $short =~ s:[.].*::; # basename for all exts
	$short =~ s/name:sitemap://;
	$short .=" ~";
	site_map_list_title ($F, "$short");
	site_map_long_title ($F, "external sitemap index");
	site_map_list_date  ($F, &timetoday());
	echo "'$F' external sitemap index";
    }
    elsif ($F =~ /^name:(.*\.html*)$/) { # assuming it is a subsitefile
	my $FF=$1;
	my $FFF=$FF; $FFF =~ s:/[^/]*$:/:; # dirname
	$FFF="" if $FFF !~ m:/:;
	make_subsitemap_list($FF, $FFF);
	scan_subsitemap_long($FF, $FFF);
    }
}
sub scan_httpspec
{
    # nothing so far
}

sub skip_namespec 
{
    # nothing so far
}
sub skip_httpspec
{
    # nothing so far
}

# ==========================================================================
# and now generate the output pages
#                                                                   2.PASS

sub head_sed_sitemap # $filename $section
{
    my ($U,$V,$Z) = @_;
    my $FF=&sed_piped_key($U);
    my $SECTION=&sed_slash_key($V);
    my $SECTS="sect=\"[$NN$AZ]\"" ; 
    my $SECTN="sect=\"[$NN]\""; # lines with hrefs
    my @OUT = ();
    push @OUT, "s|(<a $SECTS href=\\\"$FF\\\">.*</a>)|<b>\$1</b>|;";
    push @OUT, "/ href=\\\"$SECTION\\\"/ "
	."and s|^<td class=\\\"[^\\\"]*\\\"|<td |;" if $sectiontab ne "no";
    return @OUT;
}

sub head_sed_listsection # $filename $section
{
   # traditional.... the sitefile is the full navigation bar
    my ($U,$V,$Z) = @_;
    my $FF=&sed_piped_key($U);
    my $SECTION=&sed_slash_key($V);
    my $SECTS="sect=\"[$NN$AZ]\"" ; 
    my $SECTN="sect=\"[$NN]\""; # lines with hrefs
    my @OUT = ();
    push @OUT, "s|(<a $SECTS href=\\\"$FF\\\">.*</a>)|<b>\$1</b>|;";
    push @OUT, "/ href=\\\"$SECTION\\\"/ "
	."and s|^<td class=\\\"[^\\\"]*\\\"|<td |;" if $sectiontab ne "no";
    return @OUT;
}

sub head_sed_multisection # $filename $section
{
    # sitefile navigation bar is split into sections
    my ($U,$V,$Z) = @_;
    my $FF=&sed_piped_key($U);
    my $SECTION=&sed_slash_key($V);
    my $SECTS="sect=\"[$NN$AZ]\"" ; 
    my $SECTN="sect=\"[$NN]\""; # lines with hrefs
    my @OUT = ();
    # grep all pages with a =sect= relation to current $SECTION and
    # build foreach an sed line "s|<a $SECTS (href=$F)>|<a sect="X" $1>|"
    # after that all the (still) numeric SECTNs are deactivated / killed.
    for my $section ($SECTION, $headsection, $tailsection) {
	next if $section eq "no";
	for (grep {/^<$Q='sect'>[^ ]* $section/} @MK_DATA) {
	    my $x = $_;
	    $x =~ s|<$Q='sect'>||; $x =~ s| .*||; # $filename
	    $x =~ s/(.*)/s|<a $SECTS \(href=\\\"$1\\\"\)|<a sect=\\\"X\\\" \$1|/;
	    push @OUT, $x.";";
	}
	for (grep {/^<$Q='sect'>name:[^ ]* $section/} @MK_DATA) {
	    my $x = $_;
	    $x =~ s|<$Q='sect'>name:||; $x =~ s| .*||; # $filename
	    $x =~ s/(.*)/s|<a $SECTS \(name=\\\"$1\\\"\)|<a sect=\\\"X\\\" \$1|/;
	    push @OUT, $x.";";
	}
    }
    push @OUT, "s|.*<a ($SECTN href=[^<>]*)>.*|<!-- \$1 -->|;";
    push @OUT, "s|.*<a ($SECTN name=[^<>]*)>.*|<!-- \$1 -->|;";
    push @OUT, "s|(<a $SECTS href=\\\"$FF\\\">.*</a>)|<b>\$1</b>|;";
    push @OUT, "/ href=\\\"$SECTION\\\"/ "
	."and s|^<td class=\\\"[^\\\"]*\\\"|<td |;" if $sectiontab ne "no";
    return @OUT;
}

sub make_sitefile # "$F"
{
    $SOURCEFILE=&html_sourcefile($F); 
 if ($SOURCEFILE ne $F) { 
 if (-f $SOURCEFILE) {
   # remember that in this case "${SITEFILE}l" = "$F" = "${SOURCEFILE}l"
   @MK_VARS = &info2vars_sed();           # have <!--title--> vars substituted
   @MK_META = &info2meta_sed();           # add <meta name="DC.title"> values
   my @F_HEAD = (); my @F_FOOT = ();
   push @F_HEAD, @MK_PUTS;
   push @F_HEAD, &head_sed_sitemap ($F, &info_get_entry_section());
   push @F_HEAD, "/<head>/ and $sed_add join(\"\\n\", \@MK_META);";
   push @F_HEAD, @MK_VARS; push @F_HEAD, @MK_TAGS; 
   push @F_HEAD, "/<\\/body>/ and next;";                #cut lastline
   if ( $sitemaplayout eq "multi") {
       push @F_FOOT,  &make_multisitemap();        # here we use ~foot~ to
   } else {
       push @F_FOOT,  &make_listsitemap();         # hold the main text
   }

   my $html = ""; # 
   $html .= &eval_MK_FILE("SITE", $SITEFILE, @F_HEAD);
   $html .= join("", @F_FOOT);
   for (source($SITEFILE)) {
       /<\/body>/ or next;
       $html .= &eval_MK_LIST("sitefile", $_, @MK_VARS);
   }
  open F, ">$F"; print F $html; close F;
  echo "'$SOURCEFILE': ",ls_s($SOURCEFILE)," >-> ",ls_s($F);
   savesource("$F.~head~", \@F_HEAD);
   savesource("$F.~foot~", \@F_FOOT);
} else {
    echo "'$SOURCEFILE': does not exist";
} }
}

sub make_htmlfile # "$F"
{
    $SOURCEFILE=&html_sourcefile($F);                      #     2.PASS
 if ("$SOURCEFILE" ne "$F") {
 if (-f "$SOURCEFILE") {
    if (grep {/<meta name="formatter"/} source($SOURCEFILE)) {
      echo "'$SOURCEFILE': SKIP, this sourcefile looks like a formatted file";
      echo "'$SOURCEFILE':  (may be a sourcefile in place of a targetfile?)";
    return; }
    @MK_VARS = &info2vars_sed();           # have <!--title--> vars substituted
    @MK_META = &info2meta_sed();           # add <meta name="DC.title"> values
    @MK_SPAN = &tags2span_sed();       # extern text/css -> intern css classes
    push @MK_META, &tags2meta_sed();       # extern text/css -> intern css classes
    my @F_HEAD = (); my @F_BODY = (); my $F_FOOT = "";
    push @F_HEAD, @MK_PUTS;
    if ( $sectionlayout eq "multi") {
	push @F_HEAD, &head_sed_multisection ($F, &info_get_entry_section());
    } else {
	push @F_HEAD, &head_sed_listsection ($F, &info_get_entry_section());
    }
    push @F_HEAD, @MK_VARS; push @F_HEAD, @MK_TAGS; push @F_HEAD, @MK_SPAN;
    push @F_HEAD, "/<\\/body>/ and next;";                #cut lastline
    push @F_HEAD, "/<head>/ and $sed_add join(\"\\n\",\@MK_META);"; #add metatags
    push @F_BODY, "/<title>/ and next;";                  #not that line
    push @F_BODY, @MK_VARS; push @F_BODY, @MK_TAGS; push @F_BODY, @MK_SPAN;
    push @F_BODY, &bodymaker_for_sectioninfo();             #if sectioninfo
    push @F_BODY, &info2body_sed();                         #cut early
    push @F_HEAD, &info2head_sed();
    push @F_HEAD, &make_back_path($F); 
    if ($emailfooter ne "no") {
	$F_FOOT = &body_for_emailfooter();
    }
    my $html = "";
    $html .= eval_MK_FILE("head", $SITEFILE, @F_HEAD);
    $html .= eval_MK_FILE("body", $SOURCEFILE, @F_BODY);
    $html .= $F_FOOT;
    for (source($SITEFILE)) {
	/<\/body>/ or next;
	$_ = &eval_MK_LIST("htmlfile", $_, @MK_VARS);
	$html .= $_;
    }
    open F, ">$F" or die "could not write $F: $!"; print F $html; close F;
    echo "'$SOURCEFILE': ",&ls_s($SOURCEFILE)," -> ",&ls_s($F);
    savesource("$F.~head~", \@F_HEAD);
    savesource("$F.~body~", \@F_BODY);
 } else {
     echo "'$SOURCEFILE': does not exist";
 }} else {
     echo "<$F> - skipped";
 }
}

my $PRINTSITEFILE;
sub make_printerfriendly # "$F"
{                                                                 # PRINTER
    my $printsitefile="0";                                        # FRIENDLY
    my $BODY_TXT; my $BODY_SED;
    my $P=&html_printerfile ($F);
    my @P_HEAD = (); my @P_BODY = ();
    if ("$F" =~ /^(${SITEFILE}|${SITEFILE}l)$/) {
	$printsitefile=">=>" ; $BODY_TXT="$F.~foot~" ; 
    } elsif ("$F" =~ /^(.*[.]html)$/) {
	$printsitefile="=>" ;  $BODY_TXT="$SOURCEFILE";
    }
    if (grep {/<meta name="formatter"/} source($BODY_TXT)) { return; }
    if ($printsitefile ne "0" and -f $SOURCEFILE) {       my $x;
      @MK_FAST = &make_printerfile_fast (\@FILELIST);
      push @P_HEAD, @MK_VARS; push @P_HEAD, @MK_TAGS; push @P_HEAD, @MK_FAST;
      @MK_METT = map { $x = $_; $x =~
      /DC.relation.isFormatOf/ and $x =~ s|content=\"[^\"]*\"|content=\"$F\"| ;
	  $x } @MK_META;
      push @P_HEAD, "/<head>/ and $sed_add join(\"\\n\", \@MK_METT);";
      push @P_HEAD, "/<\\/body>/ and next;";
      push @P_HEAD, &select_in_printsitefile ("$F");
      my $_ext_=&print_extension($printerfriendly);
#     my $line_=&sed_slash_key($printsitefile_img_2);
      push @P_HEAD, "/\\|\\|topics:/"
	  ." and s| href=\\\"\\#\\.\\\"| href=\\\"$F\\\"|;";
      push @P_HEAD, "/\\|\\|\\|pages:/"
	  ." and s| href=\\\"\\#\\.\\\"| href=\\\"$F\\\"|;";
      push @P_HEAD, &make_back_path("$F");
      push @P_BODY, @MK_VARS; push @P_BODY, @MK_TAGS; push @P_BODY, @MK_FAST;
      push @P_BODY, &make_back_path("$F");             
      my $html = "";
      $html .= eval_MK_FILE("p_head", $PRINTSITEFILE, @P_HEAD);
      $html .= eval_MK_FILE("p_body", $BODY_TXT, @P_BODY);
      for (source($PRINTSITEFILE)) {
	  /<\/body>/ or next;
	  $_ = &eval_MK_LIST("printerfriendly", $_, @MK_VARS);
	  $html .= $_;
      }
      open P, ">$P" or die "could not write $P: $!"; print P $html; close P;
      echo "'$SOURCEFILE': ",ls_s($SOURCEFILE)," $printsitefile ",ls_s($P);
  }
}


# ========================================================================
# ========================================================================
# ========================================================================
# ========================================================================
#                                                          #### 0. INIT
$F=$SITEFILE;
&make_sitemap_init();
&make_sitemap_list($SITEFILE);
&make_sitemap_sect();
&make_sitemap_page();
savelist(\@MK_DATA, "DATA");

@FILELIST=&echo_site_filelist();
if ($o{filelist} or $o{list} eq "file" or $o{list} eq "files") {
    for (@FILELIST) { echo $_;  } exit; # --filelist
}
if ($o{files}) { @FILELIST=split(/ /, $o{files}); } # --files
if ($#FILELIST < 0) { warns "nothing to do (no --filelist)"; }
if ($#FILELIST == 0 and 
    $FILELIST[0] eq $SITEFILE) { warns "only '$SITEFILE'?!"; }

for (@FILELIST) {                                    #### 1. PASS
    $F = $_;
    if (/^(name:.*)$/) { 
	&scan_namespec ("$F"); 
    } elsif (/^(http:|https:|ftp:|mailto:|telnet:|news:|gopher:|wais:)/) { 
	&scan_httpspec ("$F"); 
    } elsif (/^(${SITEFILE}|${SITEFILE}l)$/) {
	&scan_sitefile ("$F") ;;                      # ........... SCAN SITE
    } elsif (/^(.*\@.*\.de)$/) { 
	echo "!! -> '$F' (skipping malformed mailto:-link)";
    } elsif (/^(\.\.\/.*)$/) { 
	echo "!! -> '$F' (skipping topdir build)";
# */*.html) 
#    make_back_path  # try for later subdir build
#    echo "!! -> '$F' (skipping subdir build)"
#    ;;
# */*/*/|*/*/|*/|*/index.htm|*/index.html) 
#    echo "!! -> '$F' (skipping subdir index.html)"
#    ;;
    } elsif (/^(.*\.html)$/) {
	&scan_htmlfile ("$F");                        # ........... SCAN HTML
	if ($o{xml}) {
	    $F =~ s/\.html$/.xml/;
	    &scan_xmlfile ("$F");
	}
    } elsif (/^(.*\.xml)$/) {
	&scan_xmlfile ("$F") ;;
    } elsif (/^(.*\/)$/) {
	echo "'$F' : directory - skipped";
	&site_map_list_title ("$F", &sed_slash_key($F));
	&site_map_long_title ("$F", "(directory)");
    } else {
	echo "?? -> '$F'";
    }
}

if ($printerfriendly) {                            # .......... PRINT VERSION
    my $_ext_=esc(&print_extension($printerfriendly));
    $PRINTSITEFILE=$SITEFILE; $PRINTSITEFILE =~ s/(\.\w*)$/$_ext_$1/;
    $F=$PRINTSITEFILE;
    my @TEXT = &make_printsitefile();
    echo "NOTE: going to create printer-friendly sitefile '$PRINTSITEFILE'"
	." $F._$i";
    savelist(\@TEXT, "TEXT");
    my @LINES = map { chomp; $_."$n" } @TEXT;
    savesource($PRINTSITEFILE, \@LINES);
    if (1) {
	if (open PRINTSITEFILE, ">$PRINTSITEFILE") {
	    print PRINTSITEFILE join("", @LINES); close PRINTSITEFILE;
	}
    }
}

for (@FILELIST) {                                          #### 2. PASS
  $F = $_;
  if (/^(name:.*)$/) { 
      &skip_namespec ("$F") ;;
  } elsif (/^(http:|https:|ftp:|mailto:|telnet:|news:|gopher:|wais:)/) { 
      &skip_httpspec ("$F") ;;
  } elsif (/^(${SITEFILE}|${SITEFILE}l)$/) {
      &make_sitefile ("$F") ;;                         # ........ SITE FILE
      &make_printerfriendly ("$F") if ($printerfriendly);
      if ($o{xml}) {
	  $F =~ s/\.html$/.xml/;
	  &make_xmlmaster ("$F");
      }
  } elsif (/^(.*\@.*\.de)$/) { 
      echo "!! -> '$F' (skipping malformed mailto:-link)";
  } elsif (/^(\.\.\/.*)$/) {
      echo "!! -> '$F' (skipping topdir build)";
# */*.html) 
#   echo "!! -> '$F' (skipping subdir build)"
#   ;;
# */*/*/|*/*/|*/|*/index.htm|*/index.html) 
#   echo "!! -> '$F' (skipping subdir index.html)"
#   ;;
  } elsif (/^(.*\.html)$/) {
      &make_htmlfile ("$F") ;                # .................. HTML FILES
      &make_printerfriendly ("$F") if ($printerfriendly);
      if ($o{xml}) {
	  $F =~ s/\.html$/.xml/;
	  &make_xmlfile ("$F");
      }
  } elsif (/^(.*\.xml)$/) {
      &make_xmlfile ("$F") ;;
  } elsif (/^(.*\/)$/) {
      echo "'$F' : directory - skipped";
  } else {
      echo "?? -> '$F'";
  }

# .............. debug ....................
  if (-d "DEBUG" and -f $F)  {
      my $INP = \@{$DATA{$F}};
      my $FFFF = $F; $FFFF =~ s,/,:,g;
      if (open FFFF, ">DEBUG/$FFFF.data.tmp.ht") {
	  for (@{$INP}) { print FFFF $_,$n; } close FFFF;
      } 
      if (open FFFF, ">DEBUG/$FFFF.tags.tmp.pl") {
	  print FFFF "# /usr/bin/env perl -p",$n;
	  for (@MK_TAGS) { print FFFF $_,$n; } close FFFF;
      } 
      if (open FFFF, ">DEBUG/$FFFF.vars.tmp.pl") {
	  print FFFF "# /usr/bin/env perl -p",$n;
	  for (@MK_VARS) { print FFFF $_,$n; } close FFFF;
      } 
      if (open FFFF, ">DEBUG/$FFFF.span.tmp.pl") {
	  print FFFF "# /usr/bin/env perl -p",$n;
	  for (@MK_SPAN) { print FFFF $_,$n; } close FFFF;
      } 
      if (open FFFF, ">DEBUG/$FFFF.meta.tmp.ht") {
	  for (@MK_META) { print FFFF $_,$n; } close FFFF;
      } 
      if (open FFFF, ">DEBUG/$FFFF.gets.tmp.ht") {
	  for (@MK_GETS) { print FFFF $_,$n; } close FFFF;
      } 
      if (open FFFF, ">DEBUG/$FFFF.puts.tmp.ht") {
	  for (@MK_PUTS) { print FFFF $_,$n; } close FFFF;
      } 
      if (open FFFF, ">DEBUG/$FFFF.fast.tmp.ht") {
	  for (@MK_FAST) { print FFFF $_,$n; } close FFFF;
      } 
  }
} # done

## rm ./$MK.*.tmp.* if not $o{keeptmpfiles}
exit 0
