#! /usr/local/bin/perl
# this file converts simple html text into a docbook xml variant. 
# The mapping of markups and links is far from perfect. But all we
# want is the docbook-to-pdf converter and similar technology being
# present in the world of docbook-to-anything converters.

use strict;

my %o;

my %file;
my $F;
my @order;

for $F (@ARGV)
{
    if ($F =~ /^(\w+)=(.*)/)
    {
        $o{$1} = $2;
    }else{
        open F, "<$F" or next;
        my $T = join ("",<F>); close F;
	$file{$F}{text} = $T;
	$file{$F}{orig} = $F;
	push @order, $F;
    }
}

$o{version} = `date` if not length $o{version};

for $F (keys %file)
{
    $_ = $file{$F}{text};
    s{<!--VERSION-->}{ $o{version} }gse;
    s{</h2>(.*)}{</title>\n<subtitle>$1</subtitle>}mg;
    s{<h2>}{<sect1 id=\"$F\"><title>}mg;
    s{<[Pp]([> ])}{<para$1}mg; s{</[Pp]>}{</para>}mg;
    s{<pre>}{<screen>}mg; s{</pre>}{</screen>}mg;
    s{<h3>}{<sect2><title>}mg;
    s{</h3>((?:.(?!<sect2>))*.?)}{</title>$1</sect2>}sg;
    s{<!doctype [^<>]*>}{}sg;
    s{<!DOCTYPE [^<>]*>}{}sg;
    s{(<\w+\b[^<>]*\swidth=)(\d+\%)}{$1\"$2\"}sg;
    s{(<\w+\b[^<>]*\s\w+=)(\d+)}{$1\"$2\"}sg;
    s{&&}{\&amp\;\&amp\;}sg;
    s{\$\<}{\$\&lt\;}sg;
    s{&(\w+[\),])}{\&amp\;$1}sg;
    s{(</?)span(\s[^<>]*)?>}{$1."phrase$2>"}sge;
    s{(</?)small(\s[^<>]*)?>}{$1."note$2>"}sge;
    s{(</?)(b|em|i)>}{$1."emphasis>"}sge;
    s{(</?)(li)>}{$1."listitem>"}sge;
    s{(</?)(ul)>}{$1."itemizedlist>"}sge;
    s{(</?)(ol)>}{$1."orderedlist>"}sge;
    s{(</?)(dl)>}{$1."variablelist>"}sge;
    s{<dt\b([^<>]*)>}{"<varlistentry$1><term>"}sge;
    s{</dt\b([^<>]*)>}{"</term>"}sge;
    s{<dd\b([^<>]*)>}{"<listitem$1>"}sge;
    s{</dd\b([^<>]*)>}{"</listitem></varlistentry>"}sge;
    s{<table\b([^<>]*)>}{"<informaltable$1><tgroup cols=\"2\"><tbody>"}sge;
    s{</table\b([^<>]*)>}{"</tbody></tgroup></informaltable>"}sge;
    s{(</?)tr(\s[^<>]*)?>}{$1."row$2>"}sge;
    s{(</?)td(\s[^<>]*)?>}{$1."entry$2>"}sge;
    s{<informaltable\b[^<>]*>\s*<tgroup\b[^<>]*>\s*<tbody>
	  \s*<row\b[^<>]*>\s*<entry\b[^<>]*>\s*<informaltable\b}
    {<informaltable}gsx;
    s{</informaltable>\s*</entry>\s*</row>
          \s*</tbody>\s*</tgroup>\s*</informaltable>}
    {</informaltable>}gsx;
    s{(<informaltable[^<>]*\swidth=\"100\%\")}{$1 pgwide=\"1\"}gs;
    s{(<tbody>\s*<row[^<>]*>\s*<entry[^<>]*\s)(width=\"50\%\")}
    {<colspec colwidth=\"1*\" /><colspec colwidth=\"1*\" />\n$1$2}gs;

    s{<nobr>([\'\`]*)<tt>}{<cmdsynopsis>$1}sg;
    s{</tt>([\'\`]*)</nobr>}{$2</cmdsynopsis>}sg;
    s{<nobr><(?:tt|code)>([\`\"\'])}{<cmdsynopsis>$1}sg;
    s{<(?:tt|code)><nobr>([\`\"\'])}{<cmdsynopsis>$1}sg;
    s{([\`\"\'])</(?:tt|code)></nobr>}{$1</cmdsynopsis>}sg;
    s{([\`\"\'])</nobr></(?:tt|code)>}{$1</cmdsynopsis>}sg;
    s{(</?)tt>}{$1."constant>"}sge;
    s{(</?)code>}{$1."literal>"}sge;
    s{>([^<>]+)<br>}{><highlights>$1</highlights>}sg;
    s{<br>}{<br />}sg;

    s{(</?)date>}{$1."sect1info>"}sge;
    s{<reference>}{<reference id=\"reference\">}s;

    s{<a\s+href=\"((?:http|ftp|mailto):[^<>]+)\"\s*>((?:.(?!</a>))*.)</a>}
      { "<ulink url=\"$1\">$2</ulink>" }sge;
    s{<a\s+href=\"zziplib.html\#([\w_]+)\"\s*>((?:.(?!</a>))*.)</a>}
    { "<link linkend=\"$1\">$2</link>" }sge;
    s{<a\s+href=\"(zziplib.html)\"\s*>((?:.(?!</a>))*.)</a>}
    { "<link linkend=\"reference\">$2</link>" }sge;
    s{<a\s+href=\"([\w-]+[.]html)\"\s*>((?:.(?!</a>))*.)</a>}
    { my $K = $1; chop $K;
      if (not exists $file{$K}) { print STDERR "bad link $1\n"; }
      "<link linkend=\"$K\">$2</link>" }sge;
    s{<a\s+href=\"([\w-]+[.](?:h|c|am|txt))\"\s*>((?:.(?!</a>))*.)</a>}
    { "<ulink url=\"file:$1\">$2</ulink>" }sge;
    s{<a\s+href=\"([A-Z0-9]+[.][A-Z0-9]+)\"\s*>((?:.(?!</a>))*.)</a>}
    { "<ulink url=\"file:$1\">$2</ulink>" }sge;

#   s{(</?)subtitle>}{$1."para>"}ge;
    
    $_ .= "</sect1>" if /<sect1[> ]/;
    $file{$F}{text} = $_;
}

my $n = "\n";

print '<!DOCTYPE reference PUBLIC "-//OASIS//DTD DocBook XML V4.1.2//EN"',$n;
print '       "http://www.oasis-open.org/docbook/xml/4.1.2/docbookx.dtd">',$n;
print '<book><chapter><title>Documentation</title>',$n;
for $F (@order)
{
    print "</chapter>" if $file{$F}{text} =~ /<reference /;
    print $file{$F}{text},$n,$n;
}
print '</book>',$n;
