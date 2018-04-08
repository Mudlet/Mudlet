
use strict "vars";

my $x;
my $F;
my @regs;
my %file;
my %func;

my %o = ( verbose => 0 );

$o{version} = 
    `grep -i "^version *:" *.spec | sed -e "s/[Vv]ersion *: *//"`;
$o{package} = 
    `grep -i "^name *:" *.spec | sed -e "s/[Nn]ame *: *//"`;
$o{version} =~ s{\s*}{}gs;
$o{package} =~ s{\s*}{}gs;

$o{version} = `date +%Y.%m.%d` if not length $o{version};
$o{package} = "_project" if not length $o{package};

$o{suffix} = "-doc1";
$o{mainheader} = "$o{package}.h";

my %fn;
my $id = 1000;

for $F (@ARGV)
{
    if ($F =~ /^(\w+)=(.*)/)
    {
	$o{$1} = $2;
    }else{
	open F, "<$F" or next;
	my $T = join ("",<F>); close F;
    
	$T =~ s/\&/\&amp\;/sg;
	$T =~ s/¬/\&#AC\;/sg;
	$T =~ s/\*\//¬/sg;
    
	# cut per-function comment block
	while ( $T =~
		s{ [/][*][*](?=\s) ([^¬]+) ¬ ([^\{\}\;\#]+) [\{\;] }
		{ per_function_comment_and_declaration($F," ".$1,$2) }gsex
		) {}

	# cut per-file comment block
	if ( $T =~ m{ ^ [/][*]+(?=\s) ([^¬]+) ¬ 
			  (\s*\#include\s*<[^<>]*>(?:\s*/[/*][^\n]*)?) }sx)
	{
	    $file{$F}{comment} = $1;
	    $file{$F}{include} = $2;
	    $file{$F}{comment} =~ s/¬/\*\//sg;
	    $file{$F}{include} =~ s/¬/\*\//sg;
	    $file{$F}{include} =~ s{[/][*]}{//}s;
	    $file{$F}{include} =~ s{[*][/]}{\n}s;
	    $file{$F}{include} =~ s{<}{\&lt\;}sg;
	    $file{$F}{include} =~ s{>}{\&gt\;}sg;
 	}
	elsif ( $T =~ m{ ^ [/][*]+(?=\s) ([^¬]+) ¬ }sx)
	{
	    $file{$F}{comment} = $1;
	    $file{$F}{comment} =~ s/¬/\*\//sg;
 	}

	# throw away the rest - further processing on memorized strings only
    }
}

$o{outputfilestem}= "$o{package}$o{suffix}" if not length $o{outputfilestem};
$o{docbookfile}= "$o{outputfilestem}.docbook" if not length $o{docbookfile};
$o{libhtmlfile}= "$o{outputfilestem}.html"    if not length $o{libhtmlfile};
$o{dumpdocfile}= "$o{outputfilestem}.dxml"    if not length $o{dumpdocfile};

sub per_function_comment_and_declaration
{
    my ($filename, $comment, $prototype) = @_;
 
    $prototype =~ s{¬}{*/}sg;
    $comment =~ s{¬}{*/}sg;
    $comment =~ s{<([\w\.\-]+\@[\w\.\-]+\w\w)>}{&lt;$1&gt;}sg;
    $func{$id}{filename} = $filename;
    $func{$id}{comment}  = $comment;
    $func{$id}{prototype} = $prototype;
    $id ++;
    return $prototype;
}
# -----------------------------------------------------------------------
sub pre { # used for non-star lines in comment blocks
    my $T = $_[0]; $T =~ s/\&/\&amp\;/g;
    $T =~ s/\</\&lt\;/g; $T =~ s/\>/\&gt\;/g; $T =~ s/\"/\&quot\;/g;
    $T =~ s/^/\ /gm; #  $T =~ s/^/\| /gm;
    return " <pre> $T </pre> ";
}

# per-file comment block handling
my $name;
for $name (keys %file)
{
    $file{$name}{comment} =~ s{<([\w\.\-]+\@[\w\.\-]+\w\w)>}{&lt;$1&gt;}sg;
    $file{$name}{comment} =~ s{ ^\s?\s?\s? ([^\*\s]+ .*) $}{&pre($1)}mgex;
    $file{$name}{comment} =~ s{ ^\s*[*]\s* $}{ <p> }gmx;
    $file{$name}{comment} =~ s{ ^\s?\s?\s?\* (.*) $}{ $1 }gmx;
    $file{$name}{comment} =~ s{ </pre>(\s*)<pre> }{$1}gsx;
    $file{$name}{comment} =~ s{ <([^<>\;]+\@[^<>\;]+)> }{<email>$1</email>}gsx;
    $file{$name}{comment} =~ s{ \&lt\;([^<>\&\;]+\@[^<>\&\;]+)\&gt\; }
				{<email>$1</email>}gsx;
    $file{$name}{comment} .= "<p>";

    $file{$name}{comment} =~ s{ \b[Aa]uthor\s*:(.*<\/email>) }
    {
	$file{$name}{author} = "$1";
	"<author>"."$1"."</author>"
    }sex;

    $file{$name}{comment} =~ s{ \b[Cc]opyright[\s:]([^<>]*)<p> }
    {
	$file{$name}{copyright} = "$1";
	"<copyright>"."$1"."</copyright>"
    }sex;
#   if ($name =~ /file/) {
#       print STDERR $file{$name}{comment},"\n";
#   }
    if ($file{$name}{include} =~ m{//\s*(\w+)[.][.][.]\s*})
    {
	if (length $o{$1}) {
	    $file{$name}{include} = "#include "
		.$o{$1}."\n";
	    $file{$name}{include} =~ s{<}{\&lt\;}sg;
	    $file{$name}{include} =~ s{>}{\&gt\;}sg;
	}
    }
}

# -----------------------------------------------------------------------

# pass 1 of per-func strings:
# (a) cut prototype into prespec/namespec/callspec
# (b) sanitize comment-block into proper docbook format
# do this while copying strings from $func{$name} to $fn{name} strstrhash
my @namelist;
for $x (sort keys %func)
{
    my $name = $func{$x}{prototype};
    $name =~ s/^.*[^.]\b(\w[\w.]*\w)\b\s*\(.*$/$1/s;
    push @namelist, $name; # may be you want to omit some funcs from output?

    $func{$x}{prototype} =~ m{ ^(.*[^.]) \b(\w[\w.]*\w)\b (\s*\(.*) $ }sx; 
    $fn{$name}{prespec} = $1; 
    $fn{$name}{namespec} = $2; 
    $fn{$name}{callspec} = $3;

    $fn{$name}{comment} = $func{$x}{comment};
    $fn{$name}{comment} =~ s/(^|\s)\=\>\"([^\"]*)\"/$1<link>$2<\/link>/gmx;
    $fn{$name}{comment} =~ s/(^|\s)\=\>\'([^\"]*)\'/$1<link>$2<\/link>/gmx;
    $fn{$name}{comment} =~ s/(^|\s)\=\>\s(\w[\w.]*\w)\b/$1<link>$2<\/link>/gmx;
    $fn{$name}{comment} =~ 
	s/(^|\s)\=\>\s([^\s\,\.\!\?\:\;\<\>\&\'\=\-]+)/$1<link>$2<\/link>/gmx;

    # cut comment in first-line (describe) and only keep the rest in comment
    $fn{$name}{describe} = $fn{$name}{comment};
    $fn{$name}{describe} =~ s{^([^\n]*\n).*}{$1}gs;
    $fn{$name}{comment} =~ s{^[^\n]*\n}{}gs;
    if ($fn{$name}{describe} =~ /^\s*$/s)
    {
	$fn{$name}{describe} = "(".$func{$x}{filename}.")";
	$fn{$name}{describe} =~ s,[.][.][/],,g;
    }

    $fn{$name}{comment} =~ s/ ^\s?\s?\s? ([^\*\s]+ .*) $/&pre($1)/mgex;
    $fn{$name}{comment} =~ s/ ^\s?\s?\s?\* (.*) $/ <br \/> $1 /gmx;
    $fn{$name}{comment} =~ s/ ^\s*<br\s*\/>\s* $/ <p> /gmx;
    $fn{$name}{comment} =~ s{<<}{&lt;}sg;  
    $fn{$name}{comment} =~ s{>>}{&gt;}sg;
    $fn{$name}{comment} =~ s/ (<p>\s*)<br\s*\/?>/$1/gsx;
    $fn{$name}{comment} =~ s/ (<p>\s*)<br\s*\/?>/$1/gsx;
    $fn{$name}{comment} =~ s/ (<br\s*\/?>\s*)<br\s*\/?>/$1/gsx;
    $fn{$name}{comment} =~ s/<c>/<code>/gs;
    $fn{$name}{comment} =~ s/<\/c>/<\/code>/gs;
    $fn{$name}{comment} =~ s/<\/pre>(\s*)<pre>/$1/gs;
    
    $fn{$name}{filename} = $func{$x}{filename};
    $fn{$name}{callspec} =~ s{^ \s*}{}gsx;
    $fn{$name}{prespec}  =~ s{^ \s*}{}gsx;
    $fn{$id} = $x;
}

# add extra docbook markups to callspec in $fn-hash
for $name (@namelist) # <paramdef>
{
    $fn{$name}{callspec} =~ s:^([^\(\)]*)\(:$1<parameters>\(<paramdef>:s;
    $fn{$name}{callspec} =~ s:\)([^\(\)]*)$:</paramdef>\)</parameters>$1:s;
    $fn{$name}{callspec} =~ s:,:</paramdef>,<paramdef>:gs;
    $fn{$name}{callspec} =~ s:<paramdef>(\s+):$1<paramdef>:gs;
    $fn{$name}{callspec} =~ s:(\s+)</paramdef>:</paramdef>$1:gs;
}

# html-formatting of callspec strings
for $name (@namelist)
{
    $fn{$name}{declcode} =
        "<td valign=\"top\"><code>".$fn{$name}{prespec}."<\/code><\/td>"
	."<td valign=\"top\">&nbsp;&nbsp;</td>"
	."<td valign=\"top\"><a href=\"#$name\">"
	."\n <code>".$fn{$name}{namespec}."<\/code>\n"
	."<\/a><\/td>"
	."<td valign=\"top\">&nbsp;&nbsp;</td>"
	."<td valign=\"top\">".$fn{$name}{callspec}."<\/td>";

    $fn{$name}{implcode} =
        "<code>".$fn{$name}{prespec}."<\/code>"
	."\n <br \/><b><code>".$fn{$name}{namespec}."<\/code><\/b>"
	."\n &nbsp; <code>".$fn{$name}{callspec}."<\/code>";

    $fn{$name}{declcode} =~ s{\s+<paramdef>}{\n<nobr>}gs;
    $fn{$name}{implcode} =~ s{\s+<paramdef>}{\n<nobr>}gs;
    $fn{$name}{declcode} =~ s{<paramdef>}{<nobr>}gs;
    $fn{$name}{implcode} =~ s{<paramdef>}{<nobr>}gs;
    $fn{$name}{declcode} =~ s{</paramdef>}{</nobr>}gs;
    $fn{$name}{implcode} =~ s{</paramdef>}{</nobr>}gs;
    $fn{$name}{declcode} =~ s{<parameters>}{\n <code>}gs;
    $fn{$name}{implcode} =~ s{<parameters>}{\n <code>}gs;
    $fn{$name}{declcode} =~ s{</parameters>}{</code>\n}gs;
    $fn{$name}{implcode} =~ s{</parameters>}{</code>\n}gs;
}

# whether each function should get its own page or combined with others
my $combinedstyle = 1;

for $name (@namelist)
{
    if ($fn{$name}{describe} =~ /^ \s* <link>(\w[\w.]*\w)<\/link> /sx)
    {
	if ($combinedstyle and exists $fn{$1})
	{
	    # $into tells later steps which func-name is the leader of a man 
	    # page and that this func should add its descriptions over there.
	    $fn{$name}{into} = $1;
	}
    }

    if ($fn{$name}{describe} =~ s/(.*)also:(.*)/$1/)
    {
	$fn{$name}{_seealso} = $2;
    }

    # and prepare items for being filled in $combinedstyle (html-mode)
    # which includes adding descriptions of the leader functions firsthand
    $fn{$name}{_anchors} = "<a name=\"$name\" />";
    $fn{$name}{_impcode} = "<code>".$fn{$name}{implcode}."</code>";
    $fn{$name}{_comment} = "<p> &nbsp;".$fn{$name}{describe}."\n";
    $fn{$name}{_comment} .= "<p>".$fn{$name}{comment};
}

for $name (@namelist) # and add descriptions of non-leader entries (html-mode)
{
    next if not exists $fn{$name}{into}; # skip leader pages
    my $into = $fn{$name}{into};
    $fn{$into}{_anchors} .= "<a name=\"$name\" />";
    $fn{$into}{_impcode} .= "<br />\n";
    $fn{$into}{_impcode} .= "<code>".$fn{$name}{implcode}."</code>";
    my $text = $fn{$name}{comment};
    $text =~ s/ (T|t)his \s (function|procedure) /$1."he ".$name." ".$2/gsex;
    $fn{$name}{_comment} .= "<p>".$text;
}

my $htmlTOC = "";
my $htmlTXT = "";

# generate the index-block at the start of the onepage-html file
for $name (@namelist)
{
    $fn{$name}{_comment} =~ s/ (<p>\s*)<br\s*\/>/$1/gsx;

    $htmlTOC .= "<tr valign=\"top\">\n".$fn{$name}{declcode}."</tr>";
    next if $combinedstyle and exists $fn{$name}{into};

    $htmlTXT .=  "\n<dt>".$fn{$name}{_anchors}.$fn{$name}{_impcode}."<dt>";
    $htmlTXT .= "\n<dd>".$fn{$name}{_comment};
    $htmlTXT .= "\n<p align=\"right\"><small>("
	.$fn{$name}{filename}.")</small></p></dd>";
}

# link ref-names in this page with its endpoints on the same html page
$htmlTXT =~ s/ <link>(\w+)([^<>]*)<\/link> / &a_name($1,$2) /gsex;
sub a_name 
{ 
    my ($n,$m) = @_; 
    if (exists $fn{$n}) { return "<a href=\"#$n\"><code>$n$m</code></a>"; }
    else { return "<code>$n$m</code>"; }
}
$htmlTXT =~ s/ \-\> /<small>-\&gt\;<\/small>/gsx; # just sanitize

# and finally print the html-formatted output
open F, ">$o{libhtmlfile}" or die "could not open '$o{libhtmlfile}': $!";
print F "<html><head><title> $o{package} autodoc documentation </title>";
print F "</head>\n<body>\n";
print F "\n<h1>",$o{package}," <small><small><i>-", $o{version};
print F "</i></small></small></h1>";
print F "\n<table border=0 cellspacing=2 cellpadding=0>";
print F $htmlTOC;
print F "\n</table>";
print F "\n<h3>Documentation</h3>\n";
print F "\n<dl>";
print F $htmlTXT;
print F "\n</dl>";
print F "\n</body></html>\n";
close F;

# =========================================================================== #
# let's go for the pure docbook, a reference type master file for all man pages
my @headerlist; # leader function pages - a file will be created for each of th
my @mergedlist; # non-leader function that end up in one of those in headerlist

for $name (@namelist)
{
    push @headerlist, $name if not exists $fn{$name}{into};
    push @mergedlist, $name if exists $fn{$name}{into};

    # and initialize the fields need for a man page entry - the fields are
    # named after the docbook-markup that encloses (!!) the text we store
    # in the strstrhash - therefore, {}{_refhint} = "hello" will be printed
    # as <refhint>hello</refhint>. Names with scores at the end are only used
    # as temporaries but they are memorized - perhaps they are useful later.

    $fn{$name}{_refhint} = 
		"\n<!--========= ".$name." (3) ===========-->\n";
    $fn{$name}{_refstart} = "";
    $fn{$name}{_date_} = $o{version};
    $fn{$name}{_date_} =~ s{\s*}{}gs;
    $fn{$name}{_refentryinfo} 
    = "\n <date>".$fn{$name}{_date_}."</date>";
    $fn{$name}{_productname_} = $o{package};
    $fn{$name}{_productname_} =~ s{\s*}{}gs;
    $fn{$name}{_refentryinfo} 
    .= "\n <productname>".$fn{$name}{_productname_}."</productname>";
#    if (exists $file{ $fn{$name}{filename} }{author})
#    {
#	$H = $file{ $fn{$name}{filename} }{author};
#	$H =~ s{ \s* ([^<>]*) (<email>[^<>]*</email>) }{
#	    $fn{$name}{_refentryinfo} .= "\n <author>".$1.$2."</author>";
#	    "" }gmex;
#    }
    $fn{$name}{_refmeta} = "";
    $fn{$name}{_refnamediv} = "";
    $fn{$name}{_mainheader} = $o{mainheader};
    $fn{$name}{_includes} = $file{ $fn{$name}{filename} }{include};
    $fn{$name}{_manvolnum} = "3";
    $fn{$name}{_funcsynopsisinfo} = "";
    $fn{$name}{_funcsynopsis} = "";
    $fn{$name}{_description} = "";
    $fn{$name}{_refends} = "";
}

push @headerlist, @mergedlist; # aaahmm...

# let's walk all (!!) entries...
for $name (@headerlist)
{
    # $into is the target-manpage to add descriptions to. Initially it does
    # reference the name of the function itself - but it overridden in the
    # next line when we see an {into} mark. The self/into state is registered
    # in two vars: $into is an index into %fn-strstrhash to be used instead of
    # the $name-runvar and $me just a boolean value to conditionally add texts
    my $into = $name; my $me = 1;

    if (exists $fn{$name}{into})
    {
	$into = $fn{$name}{into}; $me = 0;
	$fn{$name}{_refhint} =
	    "\n              <!-- see ".$fn{$name}{mergeinto}." -->\n";
    }

    $fn{$into}{_refstart} .= '<refentry id="'.$name.'">' if $me;
    $fn{$into}{_refends}  .= "\n</refentry>\n" if $me;

    $fn{$name}{_title_} = $name; 
    $fn{$name}{_title_} =~ s{\s*}{}gs;
    $fn{$name}{_refentryinfo} 
    .= "\n <title>".$fn{$name}{_title_}."</title>" if $me;
    $fn{$into}{_refmeta} 
    .= "\n <manvolnum>".$fn{$name}{_manvolnum}."</manvolnum>" if $me;
    $fn{$into}{_refmeta} 
    .= "\n <refentrytitle>".$name."</refentrytitle>" if $me;

    $fn{$name}{_funcsynopsisinfo} 
    = "\n".' #include &lt;'.$fn{$into}{_mainheader}.'&gt;' if $me;
    $fn{$name}{_funcsynopsisinfo} 
    = "\n".$fn{$into}{_includes} if $me and length $fn{$into}{_includes};
    $fn{$name}{_funcsynopsisinfo} 
    .= " // ".$o{synopsis} if $me and length $o{synopsis};

    $fn{$into}{_refnamediv} .= "\n ".
	"<refpurpose>".$fn{$name}{describe}." </refpurpose>" if $me;
    $fn{$into}{_refnamediv} .= "\n".' <refname>'.$name.'</refname>';

    # add to {}{_funcsynopsis}...
    $fn{$into}{_funcsynopsis} .= "\n <funcprototype>\n <funcdef>";
    $fn{$into}{_funcsynopsis} .= $fn{$name}{prespec}
    ." <function>".$name."</function></funcdef>";
    $fn{$name}{_callspec_} = $fn{$name}{callspec};
    $fn{$name}{_callspec_} =~ s{<parameters>\s*\(}{ }gs;
    $fn{$name}{_callspec_} =~ s{\)\s*</parameters>}{ }gs;
    $fn{$name}{_callspec_} =~ s{</paramdef>\s*,\s*}{</paramdef>\n }gs;
    $fn{$into}{_funcsynopsis} 
    .= "\n".$fn{$name}{_callspec_}." </funcprototype>";

    # add to {}{_description}...
    $fn{$name}{_comment_} = "<para>\n".$fn{$name}{comment}."\n</para>";
    $fn{$name}{_comment_} =~ s{ (T|t)his \s (function|procedure) }
    { $1."he <function>".$name."</function> ".$2 }gsex;
    $fn{$name}{_comment_} =~ s{<p>}{"\n</para><para>\n"}gsex;
    $fn{$name}{_comment_} =~ s{<br\s*/?>}{}gs;
    $fn{$name}{_comment_} =~ s{(</?)em>}{$1emphasis>}gs;
    $fn{$name}{_comment_} =~ s{<code>}{<userinput>}gs;
    $fn{$name}{_comment_} =~ s{</code>}{</userinput>}gs;
    $fn{$name}{_comment_} =~ s{<link>}{<function>}gs;
    $fn{$name}{_comment_} =~ s{</link>}{</function>}gs;
    $fn{$name}{_comment_} =~ s{<pre>}{<screen>}gs;   # only xmlto .8 and
    $fn{$name}{_comment_} =~ s{</pre>}{</screen>}gs; # higher !!
#    $fn{$name}{_comment_} =~ s{<ul>}{</para><itemizedlist>}gs;
#    $fn{$name}{_comment_} =~ s{</ul>}{</itemizedlist><para>}gs;
#    $fn{$name}{_comment_} =~ s{<li>}{<listitem><para>}gs;
#    $fn{$name}{_comment_} =~ s{</li>}{</para></listitem>\n}gs;
    $fn{$name}{_comment_} =~ s{<ul>}{</para><programlisting>\n}gs;
    $fn{$name}{_comment_} =~ s{</ul>}{</programlisting><para>}gs;
    $fn{$name}{_comment_} =~ s{<li>}{}gs;
    $fn{$name}{_comment_} =~ s{</li>}{}gs;
    $fn{$into}{_description} .= $fn{$name}{_comment_}; 

    if (length $fn{$name}{_seealso} and not $me)
    {
	$fn{$into}{_seealso} .= ", " if length $fn{$into}{_seealso};
	$fn{$into}{_seealso} .= $fn{$name}{_seealso};
    }

    if (exists $file{ $fn{$name}{filename} }{author})
    {
	my $authors = $file{ $fn{$name}{filename} }{author};
	$fn{$into}{_authors} = "<itemizedlist>";
	$authors =~ s{ \s* ([^<>]*) (<email>[^<>]*</email>) }{
	    $fn{$into}{_authors} 
	    .= "\n <listitem><para>".$1." ".$2."</para></listitem>";
	    "" }gmex;
	$fn{$into}{_authors} .= "</itemizedlist>";
    }

    if (exists $file{ $fn{$name}{filename} }{copyright})
    {
	$fn{$into}{_copyright} 
	= "<screen>\n".$file{ $fn{$name}{filename} }{copyright}."</screen>\n";
    }
}

# printing the docbook file is a two-phase process - we spit out the
# leader pages first - later we add more pages with _refstart pointing
# to the lader page, so that xmlto will add the functions there. Only the
# leader page contains some extra info needed for troff page processing.

my %header;

open F, ">$o{docbookfile}" or die "could not open $o{docbookfile}: $!";
print F '<!DOCTYPE reference PUBLIC "-//OASIS//DTD DocBook XML V4.1.2//EN"';
print F "\n",'   "http://www.oasis-open.org/docbook/xml/4.1.2/docbookx.dtd">';
print F "\n\n",'<reference><title>Manual Pages</title>',"\n";
for $name (@namelist)
{
    print F $fn{$name}{_refhint};
    next if exists $fn{$name}{into};
    print F $fn{$name}{_refstart};
    print F "\n<refentryinfo>",               $fn{$name}{_refentryinfo}
    ,       "\n</refentryinfo>\n"   if length $fn{$name}{_refentryinfo};
    print F "\n<refmeta>",                    $fn{$name}{_refmeta}
    ,       "\n</refmeta>\n"        if length $fn{$name}{_refmeta};
    print F "\n<refnamediv>",                 $fn{$name}{_refnamediv}
    ,       "\n</refnamediv>\n"     if length $fn{$name}{_refnamediv};

    print F "\n<refsynopsisdiv>"    if length $fn{$name}{_funcsynopsis};
    print F "\n<funcsynopsisinfo>",           $fn{$name}{_funcsynopsisinfo}
    ,       "\n</funcsynopsisinfo>" if length $fn{$name}{_funcsynopsisinfo};
    print F "\n<funcsynopsis>",               $fn{$name}{_funcsynopsis}
    ,       "\n</funcsynopsis>"     if length $fn{$name}{_funcsynopsis};
    print F "\n</refsynopsisdiv>"   if length $fn{$name}{_funcsynopsis};
    print F "\n<refsect1><title>Description</title>", $fn{$name}{_description}
    ,       "\n</refsect1>"                 if length $fn{$name}{_description};
    print F "\n<refsect1><title>Author</title>",      $fn{$name}{_authors}
    ,       "\n</refsect1>"                 if length $fn{$name}{_authors};
    print F "\n<refsect1><title>Copyright</title>",   $fn{$name}{_copyright}
    ,       "\n</refsect1>"                 if length $fn{$name}{_copyright};
    print F "\n<refsect1><title>See Also</title>",    $fn{$name}{_seealso}
    ,       "\n</refsect1>"                 if length $fn{$name}{_seealso};
	
    print F $fn{$name}{_refends};

    # ------------------------------------------------------------------
    # creating the per-header manpage - a combination of function man pages

    my $H = $fn{$name}{_mainheader}; # a shorthand for the mainheader index
    my $me = 0; $me = 1 if not exists $header{$H};
    my $HH = $H; $HH =~ s/[^\w\.]/-/g;
    $header{$H}{_refstart} = "\n<refentry id=\"".$HH."\">" if $me;
    $header{$H}{_refends} = "\n</refentry>\n" if $me;
    $header{$H}{_refentryinfo} = $fn{$name}{_refentryinfo} if $me;
    $header{$H}{_refentryinfo} 
    =~ s/(<title>)([^<>]*)(<\/title>)/$1 the library $3/s if $me;
    $header{$H}{_refmeta} 
    = "\n   <manvolnum>".$fn{$name}{_manvolnum}."</manvolnum>\n"
    . "\n   <refentrytitle>".$fn{$name}{_mainheader}."</refentrytitle>" if $me;
    $header{$H}{_refnamediv} = "\n   <refpurpose> library </refpurpose>";
    $header{$H}{_refnamediv} .= "\n   <refname>".$HH."</refname>";

    $header{$H}{_refsynopsisinfo} 
    = $fn{$name}{_refsynopsisinfo} if exists $fn{$name}{_refsynopsisinfo};
    $header{$H}{_funcsynopsis} 
    .= "\n".$fn{$name}{_funcsynopsis} if exists $fn{$name}{_funcsynopsis};
#    $header{$H}{_funcsynopsisdiv} .= "\n<funcsynopsis>"
#	.$fn{$name}{_funcsynopsis}."</funcsynopsis>"
#	if exists  $fn{$name}{_funcsynopsis};
    $header{$H}{_copyright} 
    = $fn{$name}{_copyright} if exists $fn{$name}{_copyright} and $me;
    $header{$H}{_authors} 
    = $fn{$name}{_authors}   if exists $fn{$name}{_authors} and $me;
    if ($me)
    {
	my $T = `cat $o{package}.spec`;
	if ($T =~ /\%description\b([^\%]*)\%/s)
	{
	    $header{$H}{_description} = $1;
	}elsif (not length $header{$H}{_description})
	{
	    $header{$H}{_description} = "$o{package} library";
	}
    }
}

my $H;
for $H (keys %header) # second pass
{
    next if not length $header{$H}{_refstart};
    print F "\n<!-- _______ ",$H," _______ -->";
    print F $header{$H}{_refstart};
    print F "\n<refentryinfo>",               $header{$H}{_refentryinfo}
    ,       "\n</refentryinfo>\n"   if length $header{$H}{_refentryinfo};
    print F "\n<refmeta>",                    $header{$H}{_refmeta}
    ,       "\n</refmeta>\n"        if length $header{$H}{_refmeta};
    print F "\n<refnamediv>",                 $header{$H}{_refnamediv}
    ,       "\n</refnamediv>\n"     if length $header{$H}{_refnamediv};

    print F "\n<refsynopsisdiv>"    if length $header{$H}{_funcsynopsis};
    print F "\n<funcsynopsisinfo>",           $header{$H}{_funcsynopsisinfo}
    ,       "\n</funcsynopsisinfo>" if length $header{$H}{_funcsynopsisinfo};
    print F "\n<funcsynopsis>",               $header{$H}{_funcsynopsis}
    ,       "\n</funcsynopsis>"     if length $header{$H}{_funcsynopsis};
    print F  "\n</refsynopsisdiv>"  if length $header{$H}{_funcsynopsis};

    print F "\n<refsect1><title>Description</title>", $header{$H}{_description}
    ,       "\n</refsect1>"                if length $header{$H}{_description};
    print F "\n<refsect1><title>Author</title>",      $header{$H}{_authors}
    ,       "\n</refsect1>"                 if length $header{$H}{_authors};
    print F "\n<refsect1><title>Copyright</title>",   $header{$H}{_copyright}
    ,       "\n</refsect1>"                 if length $header{$H}{_copyright};
	
    print F $header{$H}{_refends};
}
print F "\n",'</reference>',"\n";
close (F);

# _____________________________________________________________________
open F, ">$o{dumpdocfile}" or die "could not open $o{dumpdocfile}: $!";

for $name (sort keys %fn)
{
    print F "<fn id=\"$name\"><!-- FOR \"$name\" -->\n";
    for $H (sort keys %{$fn{$name}})
    {
	print F "<$H name=\"$name\">",$fn{$name}{$H},"</$H>\n";
    }
    print F "</fn><!-- END \"$name\" -->\n\n";
}
close F;
