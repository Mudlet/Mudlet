/***************************************************************************
 *   Copyright (C) 2008-2009 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2013-2014, 2017-2018 by Stephen Lyons                   *
 *                                               - slysven@virginmedia.com *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/


// Debugging value to display ALL licences in the dialog
// #define DEBUG_SHOWALL


#include "dlgAboutDialog.h"


#include "pre_guard.h"
#include <QPainter>
#include <QStringBuilder>
#include <QTextLayout>
#include "post_guard.h"


dlgAboutDialog::dlgAboutDialog(QWidget* parent) : QDialog(parent)
{
    setupUi(this);

    // Copied from main():

    QImage splashImage(QStringLiteral(":/Mudlet_splashscreen_main.png"));

    { // Brace code using painter to ensure it is freed at right time...
        QPainter painter(&splashImage);

        unsigned fontSize = 16;
        QString sourceVersionText = QString("Version: " APP_VERSION APP_BUILD);

        bool isWithinSpace = false;
        while (!isWithinSpace) {
            QFont font(QStringLiteral("DejaVu Serif"), fontSize, QFont::Bold | QFont::Serif | QFont::PreferMatch | QFont::PreferAntialias);
            QTextLayout versionTextLayout(sourceVersionText, font, painter.device());
            versionTextLayout.beginLayout();
            // Start work in this text item
            QTextLine versionTextline = versionTextLayout.createLine();
            // First draw (one line from) the text we have put in on the layout to
            // see how wide it is..., assuming actually that it will only take one
            // line of text
            versionTextline.setLineWidth(280);
            //Splashscreen bitmap is (now) 320x360 - hopefully entire line will all fit into 280
            versionTextline.setPosition(QPointF(0, 0));
            // Only pretend, so we can see how much space it will take
            QTextLine dummy = versionTextLayout.createLine();
            if (!dummy.isValid()) {
                // No second line so have got all text in first so can do it
                isWithinSpace = true;
                qreal versionTextWidth = versionTextline.naturalTextWidth();
                // This is the ACTUAL width of the created text
                versionTextline.setPosition(QPointF((320 - versionTextWidth) / 2.0, 270));
                // And now we can place it centred horizontally
                versionTextLayout.endLayout();
                // end the layout process and paint it out
                painter.setPen(QColor(176, 64, 0, 255)); // #b04000
                versionTextLayout.draw(&painter, QPointF(0, 0));
            } else {
                // Too big - text has spilled over onto a second line - so try again
                fontSize--;
                versionTextLayout.clearLayout();
                versionTextLayout.endLayout();
            }
        }

        // Repeat for other text, but we know it will fit at given size
        // PLACEMARKER: Date-stamp needing annual update
        QString sourceCopyrightText = QStringLiteral("©️ Mudlet makers 2008-2018");
        QFont font(QStringLiteral("DejaVu Serif"), 16, QFont::Bold | QFont::Serif | QFont::PreferMatch | QFont::PreferAntialias);
        QTextLayout copyrightTextLayout(sourceCopyrightText, font, painter.device());
        copyrightTextLayout.beginLayout();
        QTextLine copyrightTextline = copyrightTextLayout.createLine();
        copyrightTextline.setLineWidth(280);
        copyrightTextline.setPosition(QPointF(1, 1));
        qreal copyrightTextWidth = copyrightTextline.naturalTextWidth();
        copyrightTextline.setPosition(QPointF((320 - copyrightTextWidth) / 2.0, 340));
        copyrightTextLayout.endLayout();
        painter.setPen(QColor(112, 16, 0, 255)); // #701000
        copyrightTextLayout.draw(&painter, QPointF(0, 0));
    }

    mudletTitleLabel->setPixmap(QPixmap::fromImage(splashImage));
    // clang-format off

    /*
     * Have moved the texts in from the dialog definitions - as it makes it
     * easier to do the translations required for I18n work (Qt Linguist is
     * "borked" for HTML content particularly in dialogues) - what follows
     * particularly for the third tab (3rd party licences) actually tries to
     * format the text in a uniform and modular fashion.  Whilst it is not
     * the most efficient way of putting together a set of large QStrings
     * some of which will shortly be made translable, it is intended to make
     * it easier to add and remove sections according to the build settings
     * and for boiler-plate licences to be reused mulitple times if necessary.
     */

    // Define a uniform header for all tabs:
    QString htmlHead(
                QStringLiteral("<head><style type=\"text/css\">"
                               "h1 { font-family: \"DejaVu Serif\"; text-align: center; }"
                               "h2 { font-family: \"DejaVu Serif\"; text-align: center; }"
                               "h3 { font-family: \"DejaVu Serif\"; text-align: center; white-space: pre-wrap; }"
                               "h4 { font-family: \"DejaVu Serif\"; white-space: pre-wrap; }"
                               "p { font-family: \"DejaVu Serif\" }"
                               "tt { font-family: \"Monospace\"; white-space: pre-wrap; }"
                               "</style></head>"));

    // TAB 1 - "About Mudlet"
    // I have found the hard way that the lupdate utility to extract translatable
    // strings from the code does not like C++11 syle raw string literals, see:
    // https://bugreports.qt.io/browse/QTBUG-42736
    QString aboutMudletHeader(
                tr("<tr><td><span style=\"color:#bc8942;\"><b>Homepage</b></span></td><td><a href=\"http://www.mudlet.org/\">www.mudlet.org</a></td></tr>\n"
                   "<tr><td><span style=\"color:#bc8942;\"><b>Forums</b></span></td><td><a href=\"http://forums.mudlet.org/\">forums.mudlet.org</a></td></tr>\n"
                   "<tr><td><span style=\"color:#bc8942;\"><b>Documentation</b></span></td><td><a href=\"http://wiki.mudlet.org/w/Main_Page\">wiki.mudlet.org/w/Main_Page</a></td></tr>\n"
                   "<tr><td><span style=\"color:#bc8942;\"><b>Discord</b></span></td><td><a href=\"https://discord.gg/kuYvMQ9\">discord.gg</a></td></tr>\n"
                   "<tr><td><span style=\"color:#40b040;\"><b>Source code</b></span></td><td><a href=\"https://github.com/Mudlet/Mudlet\">github.com/Mudlet/Mudlet</a></td></tr>\n"
                   "<tr><td><span style=\"color:#40b040;\"><b>Gitter</b></span></td><td><a href=\"https://gitter.im/Mudlet/Mudlet/\">gitter.im</a></td></tr>\n"
                   "<tr><td><span style=\"color:#40b040;\"><b>Features/bugs</b></span></td><td><a href=\"https://github.com/Mudlet/Mudlet/issues\">github.com/Mudlet/Mudlet/issues</a></td></tr>"));

    QString aboutMudletBody(
                tr("<p align=\"center\"><big><b>Original author: <span style=\"color:#bc8942;\">Heiko Köhn</span></b> (<b><span style=\"color:#0000ff;\">KoehnHeiko@googlemail.com</span></b>)</big></p>\n"
                   "<p align=\"center\"><big><b>Credits:</b></big></p>"
                   "<p><span style=\"color:#bc8942;\"><big><b>Vadim Peretokin</b></big></span> (<span style=\"color:#40b040;\">vadi2</span> <span style=\"color:#0000ff;\">vadim.peretokin@mudlet.org</span>) GUI design and initial feature planning. He is responsible for the project homepage and the user manual. Maintainer of the Windows, macOS, Ubuntu and generic Linux installers. Maintains the Mudlet wiki, Lua API, and handles project management, public relations &amp; user help. With the project from the very beginning and is an official spokesman of the project. Since the retirement of Heiko, he has become the head of the Mudlet project.</p>\n"
                   "<p><span style=\"color:#bc8942;\"><big><b>Stephen Lyons</b></big></span> (<span style=\"color:#40b040;\">SlySven</span> <span style=\"color:#0000ff;\">slysven@virginmedia.com</span>) after joining in 2013, has been poking various bits of the C++ code and GUI with a pointy stick; subsequently trying to patch over some of the holes made/found.  Most recently he has been working on I18n and L10n for Mudlet 4.0.0 so if you are playing Mudlet in a language other than American English you will be seeing the results of him getting fed up with the spelling differences between what was being used and the British English his brain wanted to see.</p>\n"
                   "<p><span style=\"color:#bc8942;\"><big><b>Damian Monogue</b></big></span> (<span style=\"color:#40b040;\">demonnic</span> <span style=\"color:#0000ff;\">demonnic@gmail.com</span>) former maintainer of the early Windows and Apple OSX packages. He also administers our server and helps the project in many ways.</p>\n"
                   "<p><span style=\"color:#bc8942;\"><big><b>Florian Scheel</b></big></span> (<span style=\"color:#40b040;\">keneanung</span> <span style=\"color:#0000ff;\">keneanung@googlemail.com</span>) contributed many improvements to Mudlet's db: interface, event system, and has been around the project for a very long while assisting users.</p>\n"
                   "<p><span style=\"color:#bc8942;\"><big><b>Ahmed Charles</b></big></span> (<span style=\"color:#40b040;\">ahmedcharles</span> <span style=\"color:#0000ff;\">acharles@outlook.com</span>) contributions to the Travis integration, CMake and Visual C++ build, a lot of code quality and memory management improvements.</p>\n"
                   "<p><span style=\"color:#bc8942;\"><big><b>Chris Mitchell</b></big></span> (<span style=\"color:#40b040;\">Chris7</span> <span style=\"color:#0000ff;\">chrismudlet@gmail.com</span>) has developed a shared module system that allows script packages to be shared among profiles, a UI for viewing Lua variables, improvements in the mapper and all around.</p>\n"
                   "<p><span style=\"color:#bc8942;\"><b>Ben Carlsen</b></span> (<span style=\"color:#0000ff;\">arkholt@gmail.com</span>) has developed the first version of our Mac OSX installer. He is the former maintainer of the Mac version of Mudlet.</p>\n"
                   "<p><span style=\"color:#bc8942;\"><b>Ben Smith</b></span> () joined in December 2009 though he's been around much longer. Contributed to the Lua API and is the former maintainer of the Lua API.</p>\n"
                   "<p><span style=\"color:#bc8942;\"><b>Blaine von Roeder</b></span> () joined in December 2009. He has contributed to the Lua API, submitted small bugfix patches and has helped with release management of 1.0.5.</p>\n"
                   "<p><span style=\"color:#bc8942;\"><b>Bruno Bigras</b></span> (<span style=\"color:#0000ff;\">bruno@burnbox.net</span>) developed the original cmake build script and he has committed a number of patches.</p>\n"
                   "<p><span style=\"color:#bc8942;\"><b>Carter Dewey</b></span> (<span style=\"color:#0000ff;\">eldarerathis@gmail.com</span>) contributions to the Lua API.</p>\n"
                   "<p><span style=\"color:#bc8942;\"><b>Erik Pettis</b></span> (<span style=\"color:#40b040;\">Oneymus</span>) developed the Vyzor GUI Manager for Mudlet.</p>\n"
                   "<p><span style=\"color:#bc8942;\"><b>\"ItsTheFae\"</b></span> (<span style=\"color:#40b040;\">Kae</span>) someone who has worked wonders in rejuventating our Website in 2017 but who prefers a little anonymity - if you are a <i>SpamBot</i> you will not get onto our Fora now. They have also made some useful C++ core code contributions and we look forward to future reviews on and work in that area.</p>\n"
                   "<p><span style=\"color:#bc8942;\"><b>Ian Adkins</b></span> (<span style=\"color:#40b040;\">dicene</span> <span style=\"color:#0000ff;\">ieadkins@gmail.com</span>) joining us 2017 they have given us some useful C++ and Lua contributions.</p>\n"
                   "<p><span style=\"color:#bc8942;\"><b>James Younquist</b></span> (<span style=\"color:#0000ff;\">daemacles@yahoo.com</span>) contributed the Geyser layout manager for Mudlet in March 2010. It is written in Lua and aims at simplifying user GUI scripting.</p>\n"
                   "<p><span style=\"color:#bc8942;\"><b>John Dahlström</b></span> (<span style=\"color:#0000ff;\">email@johndahlstrom.se</span>) helped develop and debug the Lua API. </p>\n"
                   "<p><span style=\"color:#bc8942;\"><b>Karsten Bock</span> (<span style=\"color:#40b040;\">Beliaar</span>) contributed several improvements and new features for Geyser.</p>\n"
                   "<p><span style=\"color:#bc8942;\"><b>Leigh Stillard</b></span> (<span style=\"color:#0000ff;\">leigh.stillard@gmail.com</span>) is the original author of our windows installer.</p>\n"
                   "<p><span style=\"color:#bc8942;\"><b>Maksym Grinenko</b></span> (<span style=\"color:#0000ff;\">maksym.grinenko@gmail.com</span>) worked on the manual, forum help and helps with GUI design and documentation.</p>\n"
                   "<p><span style=\"color:#bc8942;\"><b>Stephen Hansen</b></span> (<span style=\"color:#5500ff;\">me+mudlet@ixokai.io</span>) has developed a database Lua API that allows for far easier use of databases and one of the original OSX installers.</p>\n"
                   "<p><span style=\"color:#bc8942;\"><b>Thorsten Wilms</b></span> (<span style=\"color:#0000ff;\">t_w_@freenet.de</span>) has designed our beautiful logo, our splash screen, the about dialog, our website, several icons and badges. Visit his homepage at <a href=\"http://thorwil.wordpress.com/\">thorwil.wordpress.com</a>.</p>\n"
                   "<p>Others too, have make their mark on different aspects of the Mudlet project and if they have not been mentioned here it is by no means intentional! For past contributors you may see them mentioned in the <b><a href=\"https://launchpad.net/~mudlet-makers/+members#active\">Mudlet Makers</a></b> list (on our former bug-tracking site), or for on-going contributors they may well be included in the <b><a href=\"https://github.com/Mudlet/Mudlet/graphs/contributors\">Contributors</a></b> list on GitHub.</p>\n"
                   "<br>\n"
                   "<p>Many icons are taken from the <span style=\"color:#bc8942;\"><b><u>KDE4 oxygen icon theme</u></b></span> at <a href=\"https://web.archive.org/web/20130921230632/http://www.oxygen-icons.org/\">www.oxygen-icons.org <sup>{wayback machine archive}</sup></a> or <a href=\"http://www.kde.org\">www.kde.org</a>.  Most of the rest are from Thorsten Wilms, or from Stephen Lyons combining bits of Thorsten's work with the other sources.</p>\n"
                   "<p>Special thanks to <span style=\"color:#bc8942;\"><b>Brett Duzevich</b></span> and <span style=\"color:#bc8942;\"><b>Ronny Ho</b></span>. They have contributed many good ideas and thus helped improve the scripting framework substantially.</p>\n"
                   "<p>Thanks to <span style=\"color:#bc8942;\"><b>Tomas Mecir</b></span> (<span style=\"color:#0000ff;\">kmuddy@kmuddy.com</span>) who brought us all together and inspired us with his KMuddy project. Mudlet is using some of the telnet code he wrote for his KMuddy project (<a href=\"https://cgit.kde.org/kmuddy.git/\">cgit.kde.org/kmuddy.git/</a>).</p>\n"
                   "<p>Special thanks to <span style=\"color:#bc8942;\"><b>Nick Gammon</b></span> (<a href=\"http://www.gammon.com.au/mushclient/mushclient.htm\">www.gammon.com.au/mushclient/mushclient.htm</a>) for giving us some valued pieces of advice.</p>"));

    textBrowser_mudlet->setHtml(
                QStringLiteral("<html>%1<body><table border=\"0\" style=\"margin-top:36px; margin-bottom:36px; margin-left:36px; margin-right:36px;\" width=\"100%\" cellspacing=\"2\" cellpadding=\"0\">\n"
                               "%2</table>\n"
                               "%3</body></html>")
                .arg(htmlHead, aboutMudletHeader, aboutMudletBody));

    // TAB 2 - "License"
    // Only the introductory text at the top is to be translated - the Licence
    // itself MUST NOT be translated as only the English Language version is
    // legally definitive - any translations are NOT so:
    // CHECKME: I THOUGHT WE USED "GPL 2 OR LATER" - BUT WE ARE NOT SAYING THAT HERE!!!
    QString headerText(tr("<p>Mudlet was originally written by Heiko Köhn, KoehnHeiko@googlemail.com.</p>\n"
                          "<p>Mudlet is released under the GPL license version 2, which is reproduced below:</p>",
                          "For non-english language versions please append a translation of the following "
                          "to explain why the GPL is NOT reproduced in the relevent language: 'but only "
                          "the English form is considered the official version of the license, so the "
                          "following is reproduced in that language:' to replace 'which is reproduced below:'..."));

    QString gplText(
                QStringLiteral("<h1>GNU GENERAL PUBLIC LICENSE</h1>"
                               "<h2>Version 2, June 1991</h2>"
                               "<h3>Copyright © 1989, 1991 Free Software Foundation, Inc.\n"
                               "59 Temple Place, Suite 330, Boston, MA  02111-1307 USA</h3>"
                               "<p>Everyone is permitted to copy and distribute verbatim copies of this "
                               "license document, but changing it is not allowed.</p>"
                               "<h4>Preamble</h4>"
                               "<p>The licenses for most software are designed to take away your freedom to "
                               "share and change it.  By contrast, the GNU General Public License is intended "
                               "to guarantee your freedom to share and change free software--to make sure the "
                               "software is free for all its users.  This General Public License applies to "
                               "most of the Free Software Foundation's software and to any other program whose "
                               "authors commit to using it.  (Some other Free Software Foundation software is "
                               "covered by the GNU Library General Public License instead.)  You can apply it "
                               "to your programs, too.</p>"
                               "<p>When we speak of free software, we are referring to freedom, not price.  "
                               "Our General Public Licenses are designed to make sure that you have the "
                               "freedom to distribute copies of free software (and charge for this service if "
                               "you wish), that you receive source code or can get it if you want it, that you "
                               "can change the software or use pieces of it in new free programs; and that you "
                               "know you can do these things.</p>"
                               "<p>To protect your rights, we need to make restrictions that forbid anyone to "
                               "deny you these rights or to ask you to surrender the rights. These "
                               "restrictions translate to certain responsibilities for you if you distribute "
                               "copies of the software, or if you modify it.</p>"
                               "<p>For example, if you distribute copies of such a program, whether gratis or "
                               "for a fee, you must give the recipients all the rights that you have.  You "
                               "must make sure that they, too, receive or can get the source code.  And you "
                               "must show them these terms so they know their rights.</p>"
                               "<p>We protect your rights with two steps: (1) copyright the software, and (2) "
                               "offer you this license which gives you legal permission to copy, distribute "
                               "and/or modify the software.</p>"
                               "<p>Also, for each author's protection and ours, we want to make certain that "
                               "everyone understands that there is no warranty for this free software.  If the "
                               "software is modified by someone else and passed on, we want its recipients to "
                               "know that what they have is not the original, so that any problems introduced "
                               "by others will not reflect on the original authors' reputations.</p>"
                               "<p>Finally, any free program is threatened constantly by software patents.  "
                               "We wish to avoid the danger that redistributors of a free program will "
                               "individually obtain patent licenses, in effect making the program proprietary.  "
                               "To prevent this, we have made it clear that any patent must be licensed for "
                               "everyone's free use or not licensed at all.</p>"
                               "<p>The precise terms and conditions for copying, distribution and modification "
                               "follow.</p>"
                               "<h2>GNU GENERAL PUBLIC LICENSE</h2>"
                               "<h3>TERMS AND CONDITIONS FOR COPYING, DISTRIBUTION AND MODIFICATION</h3>"
                               "<p>0. This License applies to any program or other work which contains a notice "
                               "placed by the copyright holder saying it may be distributed under the terms of "
                               "this General Public License.  The &quot;Program&quot;, below, refers to any "
                               "such program or work, and a &quot;work based on the Program&quot; means either "
                               "the Program or any derivative work under copyright law: that is to say, a work "
                               "containing the Program or a portion of it, either verbatim or with "
                               "modifications and/or translated into another language.  (Hereinafter, "
                               "translation is included without limitation in the term "
                               "&quot;modification&quot;.)  Each licensee is addressed as &quot;you&quot;.</p>"
                               "<p>Activities other than copying, distribution and modification are not covered "
                               "by this License; they are outside its scope.  The act of running the Program is "
                               "not restricted, and the output from the Program is covered only if its contents "
                               "constitute a work based on the Program (independent of having been made by "
                               "running the Program). Whether that is true depends on what the Program "
                               "does.</p>"
                               "<p>1. You may copy and distribute verbatim copies of the Program's source code "
                               "as you receive it, in any medium, provided that you conspicuously and "
                               "appropriately publish on each copy an appropriate copyright notice and "
                               "disclaimer of warranty; keep intact all the notices that refer to this License "
                               "and to the absence of any warranty; and give any other recipients of the "
                               "Program a copy of this License along with the Program.</p>"
                               "<p>You may charge a fee for the physical act of transferring a copy, and you "
                               "may at your option offer warranty protection in exchange for a fee.</p>"
                               "<p>2. You may modify your copy or copies of the Program or any portion of it, "
                               "thus forming a work based on the Program, and copy and distribute such "
                               "modifications or work under the terms of Section 1 above, provided that you "
                               "also meet all of these conditions:</p>"
                               "<p>a) You must cause the modified files to carry prominent notices stating that "
                               "you changed the files and the date of any change.</p>"
                               "<p>b) You must cause any work that you distribute or publish, that in whole or "
                               "in part contains or is derived from the Program or any part thereof, to be "
                               "licensed as a whole at no charge to all third parties under the terms of this "
                               "License.</p>"
                               "<p>c) If the modified program normally reads commands interactively when run, "
                               "you must cause it, when started running for such interactive use in the most "
                               "ordinary way, to print or display an announcement including an appropriate "
                               "copyright notice and a notice that there is no warranty (or else, saying that "
                               "you provide a warranty) and that users may redistribute the program under these "
                               "conditions, and telling the user how to view a copy of this License.  "
                               "(Exception: if the Program itself is interactive but does not normally print "
                               "such an announcement, your work based on the Program is not required to print "
                               "an announcement.)</p>"
                               "<p>These requirements apply to the modified work as a whole.  If identifiable "
                               "sections of that work are not derived from the Program, and can be reasonably "
                               "considered independent and separate works in themselves, then this License, and "
                               "its terms, do not apply to those sections when you distribute them as separate "
                               "works.  But when you distribute the same sections as part of a whole which is a "
                               "work based on the Program, the distribution of the whole must be on the terms "
                               "of this License, whose permissions for other licensees extend to the entire "
                               "whole, and thus to each and every part regardless of who wrote it.</p>"
                               "<p>Thus, it is not the intent of this section to claim rights or contest your "
                               "rights to work written entirely by you; rather, the intent is to exercise the "
                               "right to control the distribution of derivative or collective works based on "
                               "the Program.</p>"
                               "<p>In addition, mere aggregation of another work not based on the Program with "
                               "the Program (or with a work based on the Program) on a volume of a storage or "
                               "distribution medium does not bring the other work under the scope of this "
                               "License.</p>"
                               "<p>3. You may copy and distribute the Program (or a work based on it, under "
                               "Section 2) in object code or executable form under the terms of Sections 1 and "
                               "2 above provided that you also do one of the following:</p>"
                               "<p>a) Accompany it with the complete corresponding machine-readable source "
                               "code, which must be distributed under the terms of Sections 1 and 2 above on a "
                               "medium customarily used for software interchange; or,</p>"
                               "<p>b) Accompany it with a written offer, valid for at least three years, to "
                               "give any third party, for a charge no more than your cost of physically "
                               "performing source distribution, a complete machine-readable copy of the "
                               "corresponding source code, to be distributed under the terms of Sections 1 and "
                               "2 above on a medium customarily used for software interchange; or,</p>"
                               "<p>c) Accompany it with the information you received as to the offer to "
                               "distribute corresponding source code.  (This alternative is allowed only for "
                               "noncommercial distribution and only if you received the program in object code "
                               "or executable form with such an offer, in accord with Subsection b above.)</p>"
                               "<p>The source code for a work means the preferred form of the work for making "
                               "modifications to it.  For an executable work, complete source code means all "
                               "the source code for all modules it contains, plus any associated interface "
                               "definition files, plus the scripts used to control compilation and installation "
                               "of the executable.  However, as a special exception, the source code "
                               "distributed need not include anything that is normally distributed (in either "
                               "source or binary form) with the major components (compiler, kernel, and so on) "
                               "of the operating system on which the executable runs, unless that component "
                               "itself accompanies the executable.</p>"
                               "<p>If distribution of executable or object code is made by offering access to "
                               "copy from a designated place, then offering equivalent access to copy the "
                               "source code from the same place counts as distribution of the source code, even "
                               "though third parties are not compelled to copy the source along with the object "
                               "code.</p>"
                               "<p>4. You may not copy, modify, sublicense, or distribute the Program except as "
                               "expressly provided under this License.  Any attempt otherwise to copy, modify, "
                               "sublicense or distribute the Program is void, and will automatically terminate "
                               "your rights under this License. However, parties who have received copies, or "
                               "rights, from you under this License will not have their licenses terminated so "
                               "long as such parties remain in full compliance.</p>"
                               "<p>5. You are not required to accept this License, since you have not signed "
                               "it.  However, nothing else grants you permission to modify or distribute the "
                               "Program or its derivative works.  These actions are prohibited by law if you do "
                               "not accept this License.  Therefore, by modifying or distributing the Program "
                               "(or any work based on the Program), you indicate your acceptance of this "
                               "License to do so, and all its terms and conditions for copying, distributing or "
                               "modifying the Program or works based on it.</p>"
                               "<p>6. Each time you redistribute the Program (or any work based on the "
                               "Program), the recipient automatically receives a license from the original "
                               "licensor to copy, distribute or modify the Program subject to these terms and "
                               "conditions.  You may not impose any further restrictions on the recipients' "
                               "exercise of the rights granted herein. You are not responsible for enforcing "
                               "compliance by third parties to this License.</p>"
                               "<p>7. If, as a consequence of a court judgment or allegation of patent "
                               "infringement or for any other reason (not limited to patent issues), conditions "
                               "are imposed on you (whether by court order, agreement or otherwise) that "
                               "contradict the conditions of this License, they do not excuse you from the "
                               "conditions of this License.  If you cannot distribute so as to satisfy "
                               "simultaneously your obligations under this License and any other pertinent "
                               "obligations, then as a consequence you may not distribute the Program at all.  "
                               "For example, if a patent license would not permit royalty-free redistribution "
                               "of the Program by all those who receive copies directly or indirectly through "
                               "you, then the only way you could satisfy both it and this License would be to "
                               "refrain entirely from distribution of the Program.</p>"
                               "<p>If any portion of this section is held invalid or unenforceable under any "
                               "particular circumstance, the balance of the section is intended to apply and "
                               "the section as a whole is intended to apply in other circumstances.</p>"
                               "<p>It is not the purpose of this section to induce you to infringe any patents "
                               "or other property right claims or to contest validity of any such claims; this "
                               "section has the sole purpose of protecting the integrity of the free software "
                               "distribution system, which is implemented by public license practices.  Many "
                               "people have made generous contributions to the wide range of software "
                               "distributed through that system in reliance on consistent application of that "
                               "system; it is up to the author/donor to decide if he or she is willing to "
                               "distribute software through any other system and a licensee cannot impose that "
                               "choice.</p>"
                               "<p>This section is intended to make thoroughly clear what is believed to be a "
                               "consequence of the rest of this License.</p>"
                               "<p>8. If the distribution and/or use of the Program is restricted in certain "
                               "countries either by patents or by copyrighted interfaces, the original "
                               "copyright holder who places the Program under this License may add an explicit "
                               "geographical distribution limitation excluding those countries, so that "
                               "distribution is permitted only in or among countries not thus excluded.  In "
                               "such case, this License incorporates the limitation as if written in the body "
                               "of this License.</p>"
                               "<p>9. The Free Software Foundation may publish revised and/or new versions of "
                               "the General Public License from time to time.  Such new versions will be "
                               "similar in spirit to the present version, but may differ in detail to address "
                               "new problems or concerns.</p>"
                               "<p>Each version is given a distinguishing version number.  If the Program "
                               "specifies a version number of this License which applies to it and &quot;any "
                               "later version&quot;, you have the option of following the terms and conditions "
                               "either of that version or of any later version published by the Free Software "
                               "Foundation.  If the Program does not specify a version number of this License, "
                               "you may choose any version ever published by the Free Software Foundation.</p>"
                               "<p>10. If you wish to incorporate parts of the Program into other free programs "
                               "whose distribution conditions are different, write to the author to ask for "
                               "permission.  For software which is copyrighted by the Free Software Foundation, "
                               "write to the Free Software Foundation; we sometimes make exceptions for this.  "
                               "Our decision will be guided by the two goals of preserving the free status of "
                               "all derivatives of our free software and of promoting the sharing and reuse of "
                               "software generally.</p>"
                               "<h3>NO WARRANTY</h3>"
                               "<p>11. BECAUSE THE PROGRAM IS LICENSED FREE OF CHARGE, THERE IS NO WARRANTY "
                               "FOR THE PROGRAM, TO THE EXTENT PERMITTED BY APPLICABLE LAW.  EXCEPT WHEN "
                               "OTHERWISE STATED IN WRITING THE COPYRIGHT HOLDERS AND/OR OTHER PARTIES "
                               "PROVIDE THE PROGRAM &quot;AS IS&quot; WITHOUT WARRANTY OF ANY KIND, EITHER "
                               "EXPRESSED OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES "
                               "OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.  THE ENTIRE RISK AS "
                               "TO THE QUALITY AND PERFORMANCE OF THE PROGRAM IS WITH YOU.  SHOULD THE "
                               "PROGRAM PROVE DEFECTIVE, YOU ASSUME THE COST OF ALL NECESSARY SERVICING, "
                               "REPAIR OR CORRECTION.</p>"
                               "<p>12. IN NO EVENT UNLESS REQUIRED BY APPLICABLE LAW OR AGREED TO IN WRITING "
                               "WILL ANY COPYRIGHT HOLDER, OR ANY OTHER PARTY WHO MAY MODIFY AND/OR "
                               "REDISTRIBUTE THE PROGRAM AS PERMITTED ABOVE, BE LIABLE TO YOU FOR DAMAGES, "
                               "INCLUDING ANY GENERAL, SPECIAL, INCIDENTAL OR CONSEQUENTIAL DAMAGES ARISING "
                               "OUT OF THE USE OR INABILITY TO USE THE PROGRAM (INCLUDING BUT NOT LIMITED TO "
                               "LOSS OF DATA OR DATA BEING RENDERED INACCURATE OR LOSSES SUSTAINED BY YOU OR "
                               "THIRD PARTIES OR A FAILURE OF THE PROGRAM TO OPERATE WITH ANY OTHER "
                               "PROGRAMS), EVEN IF SUCH HOLDER OR OTHER PARTY HAS BEEN ADVISED OF THE "
                               "POSSIBILITY OF SUCH DAMAGES.</p>"
                               "<h3>END OF TERMS AND CONDITIONS</h3>"
                               "<h3>How to Apply These Terms to Your New Programs</h3>"
                               "<p>If you develop a new program, and you want it to be of the greatest "
                               "possible use to the public, the best way to achieve this is to make it free "
                               "software which everyone can redistribute and change under these terms.</p>"
                               "<p>To do so, attach the following notices to the program.  It is safest to "
                               "attach them to the start of each source file to most effectively convey the "
                               "exclusion of warranty; and each file should have at least the "
                               "&quot;copyright&quot; line and a pointer to where the full notice is found.</p>"
                               "<tt>one line to give the program's name and a brief idea of what it does.\n"
                               "Copyright (C) yyyy  name of author\n"
                               "\n"
                               "This program is free software; you can redistribute it and/or\n"
                               "modify it under the terms of the GNU General Public License\n"
                               "as published by the Free Software Foundation; either version 2\n"
                               "of the License, or (at your option) any later version.\n"
                               "\n"
                               "This program is distributed in the hope that it will be useful,\n"
                               "but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
                               "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
                               "GNU General Public License for more details.\n"
                               "\n"
                               "You should have received a copy of the GNU General Public License\n"
                               "along with this program; if not, write to the Free Software\n"
                               "Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA</tt>"
                               "<P>Also add information on how to contact you by electronic and paper mail.</p>"
                               "<p>If the program is interactive, make it output a short notice like this when "
                               "it starts in an interactive mode:</p>"
                               "<tt>Gnomovision version 69, Copyright (C) year name of author\n"
                               "Gnomovision comes with ABSOLUTELY NO WARRANTY; for details\n"
                               "type 'show w'.  This is free software, and you are welcome\n"
                               "to redistribute it under certain conditions; type 'show c'\n"
                               "for details.</tt>"
                               "<p>The hypothetical commands <tt>'show w'</tt> and <tt>'show c'</tt> should "
                               "show the appropriate parts of the General Public License.  Of course, the "
                               "commands you use may be called something other than <tt>'show w'</tt> and "
                               "<tt>'show c'</tt>; they could even be mouse-clicks or menu items--whatever "
                               "suits your program.</p>"
                               "<p>You should also get your employer (if you work as a programmer) or your "
                               "school, if any, to sign a &quot;copyright disclaimer&quot; for the program, if "
                               "necessary. Here is a sample; alter the names:</p>"
                               "<tt>Yoyodyne, Inc., hereby disclaims all copyright\n"
                               "interest in the program 'Gnomovision'\n"
                               "(which makes passes at compilers) written\n"
                               "by James Hacker.\n"
                               "\n"
                               "signature of Ty Coon, 1 April 1989\n"
                               "Ty Coon, President of Vice</tt>"
                               "<p>This General Public License does not permit incorporating your program into "
                               "proprietary programs. If your program is a subroutine library, you may "
                               "consider it more useful to permit linking proprietary applications with the "
                               "library. If this is what you want to do, use the "
                               "<a href=\"https://www.gnu.org/licenses/lgpl.html\">GNU Lesser General Public "
                               "License</a> instead of this License.</p>"));

    //    textBrowser_license->setFont(QFont(QStringLiteral("Bitstream Vera Serif"), 14));
    textBrowser_license->setHtml(
                QStringLiteral("<html>%1<body>%2<hr>%3</body></html>")
                .arg(htmlHead, headerText, gplText));

    // TAB 3 - Third party items
    // Only the introductory text at the top and interspersed between items are
    // to be translated - the Licences themselves MUST NOT be translated:
    QString thirdPartiesHeader(
                tr("<p align=\"center\"><b>Mudlet</b> is built upon the shoulders of other projects in the FOSS world; "
                   "as well as using many GPL components we also make use of some third-party software "
                   "with other licenses:</p>"));

    // This one needs something about the name of the original copyright holder
    // and possible contributors as it includes a %1 placeholder in the text
    // to represent something that varies between different products using it:
    // There are a second & third %2/%3 placeholder that contains:
    // %2 either "COPYRIGHT HOLDERS AND CONTRIBUTORS" or "AUTHOR"
    // %3 either "COPYRIGHT HOLDERS OR CONTRIBUTORS" or "AUTHOR"
    // depending on the particular situation:
    QString BSD3Clause_Body(
                QStringLiteral("<h4>The [3-Clause] BSD Licence</h4>"
                               "<p>Redistribution and use in source and binary forms, with or without "
                               "modification, are permitted provided that the following conditions are met:"
                               "<ul><li>Redistributions of source code must retain the above copyright notice, "
                               "this list of conditions and the following disclaimer.</li>"
                               "<li>Redistributions in binary form must reproduce the above copyright notice, "
                               "this list of conditions and the following disclaimer in the documentation "
                               "and/or other materials provided with the distribution.</li>"
                               "<li>%1 be used to "
                               "endorse or promote products derived from this software without specific prior "
                               "written permission.</li></ul></p>"
                               "<p>THIS SOFTWARE IS PROVIDED BY THE %2 &quot;AS "
                               "IS&quot; AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, "
                               "THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE "
                               "ARE DISCLAIMED. IN NO EVENT SHALL THE %3 BE "
                               "LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR "
                               "CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF "
                               "SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS "
                               "INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN "
                               "CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING "
                               "IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE "
                               "POSSIBILITY OF SUCH DAMAGE.</p>"));

    // There are a first & second %1/%2 placeholder that contains:
    // %1 either "COPYRIGHT HOLDERS AND CONTRIBUTORS" or "AUTHOR"
    // %2 either "COPYRIGHT HOLDERS OR CONTRIBUTORS" or "AUTHOR"
    // depending on the particular situation:
    QString BSD2Clause_Body(
                QStringLiteral("<h4>The [2-Clause] BSD Licence</h4>"
                               "<p>Redistribution and use in source and binary forms, with or without "
                               "modification, are permitted provided that the following conditions are met:</p>"
                               "<ol><li>Redistributions of source code must retain the above copyright notice, "
                               "this list of conditions and the following disclaimer.</li>"
                               "<li>Redistributions in binary form must reproduce the above copyright notice, "
                               "this list of conditions and the following disclaimer in the documentation "
                               "and/or other materials provided with the distribution.</li></ol></p>"
                               "<p>THIS SOFTWARE IS PROVIDED BY THE %1 &quot;AS "
                               "IS&quot; AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, "
                               "THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE "
                               "ARE DISCLAIMED. IN NO EVENT SHALL THE %2 BE LIABLE "
                               "FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL "
                               "DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR "
                               "SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER "
                               "CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, "
                               "OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE "
                               "OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.</p>"));

#if defined(INCLUDE_UPDATER) || defined(DEBUG_SHOWALL)
    QString APACHE2_Body(
                QStringLiteral("<h4>Apache Licence</h4>"
                               "<p>Licensed under the Apache License, Version 2.0 (the &quot;License&quot;); "
                               "you may not use this file except in compliance with the License. You may obtain "
                               "a copy of the License at:</p>"
                               "<p><a href=\"http://www.apache.org/licenses/LICENSE-2.0\">http://www.apache.org/licenses/LICENSE-2.0</a></p>"
                               "<p>Unless required by applicable law or agreed to in writing, software "
                               "distributed under the License is distributed on an &quot;AS IS&quot; BASIS, "
                               "WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the "
                               "License for the specific language governing permissions and limitations under "
                               "the License.</p>"));
#endif

    QString MIT_Body(
                QStringLiteral("<h4>The MIT License</h4>"
                               "<p>Permission is hereby granted, free of charge, to any person obtaining a copy "
                               "of this software and associated documentation files (the &quot;Software&quot;), "
                               "to deal in the Software without restriction, including without limitation the "
                               "rights to use, copy, modify, merge, publish, distribute, sublicense, and/or "
                               "sell copies of the Software, and to permit persons to whom the Software is "
                               "furnished to do so, subject to the following conditions:</p>"
                               "<p>The above copyright notice and this permission notice shall be included in "
                               "all copies or substantial portions of the Software.</p>"
                               "<p>THE SOFTWARE IS PROVIDED &quot;AS IS&quot;, WITHOUT WARRANTY OF ANY KIND, "
                               "EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF "
                               "MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO "
                               "EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES "
                               "OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, "
                               "ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER "
                               "DEALINGS IN THE SOFTWARE.</p>"));

#if defined(INCLUDE_FONTS) || defined(DEBUG_SHOWALL)
    QString UbuntuFontText(
                QStringLiteral("<h3>UBUNTU FONT LICENCE Version 1.0</h3>"
                               "<p>PREAMBLE</p>"
                               "<p>This licence allows the licensed fonts to be used, studied, modified and "
                               "redistributed freely. The fonts, including any derivative works, can be "
                               "bundled, embedded, and redistributed provided the terms of this licence are "
                               "met. The fonts and derivatives, however, cannot be released under any other "
                               "licence. The requirement for fonts to remain under this licence does not "
                               "require any document created using the fonts or their derivatives to be "
                               "published under this licence, as long as the primary purpose of the document is "
                               "not to be a vehicle for the distribution of the fonts.</p>"
                               "<p>DEFINITIONS</p>"
                               "<p>&quot;Font Software&quot; refers to the set of files released by the "
                               "Copyright Holder(s) under this licence and clearly marked as such. This may "
                               "include source files, build scripts and documentation.</p>"
                               "<p>&quot;Original Version&quot; refers to the collection of Font Software "
                               "components as received under this licence.</p>"
                               "<p>&quot;Modified Version&quot; refers to any derivative made by adding to, "
                               "deleting, or substituting -- in part or in whole -- any of the components of "
                               "the Original Version, by changing formats or by porting the Font Software to a "
                               "new environment.</p>"
                               "<p>&quot;Copyright Holder(s)&quot; refers to all individuals and companies who "
                               "have a copyright ownership of the Font Software.</p>"
                               "<p>&quot;Substantially Changed&quot; refers to Modified Versions which can be "
                               "easily identified as dissimilar to the Font Software by users of the Font "
                               "Software comparing the Original Version with the Modified Version.</p>"
                               "<p>To &quot;Propagate&quot; a work means to do anything with it that, without "
                               "permission, would make you directly or secondarily liable for infringement "
                               "under applicable copyright law, except executing it on a computer or modifying "
                               "a private copy. Propagation includes copying, distribution (with or without "
                               "modification and with or without charging a redistribution fee), making "
                               "available to the public, and in some countries other activities as well.</p>"
                               "<p>PERMISSION &amp; CONDITIONS</p>"
                               "<p>This licence does not grant any rights under trademark law and all such "
                               "rights are reserved.</p>"
                               "<p>Permission is hereby granted, free of charge, to any person obtaining a copy "
                               "of the Font Software, to propagate the Font Software, subject to the below "
                               "conditions:"
                               "<ol style=\"1\"><li>Each copy of the Font Software must contain the above "
                               "copyright notice and this licence. These can be included either as stand-alone "
                               "text files, human-readable headers or in the appropriate machine-readable "
                               "metadata fields within text or binary files as long as those fields can be "
                               "easily viewed by the user.</li>"
                               "<li>The font name complies with the following:"
                               "<ol type=\"a\"><li>The Original Version must retain its name, unmodified.</li>"
                               "<li>Modified Versions which are Substantially Changed must be renamed to avoid "
                               "use of the name of the Original Version or similar names entirely.</li>"
                               "<li>Modified Versions which are not Substantially Changed must be renamed to "
                               "both (i) retain the name of the Original Version and (ii) add additional naming "
                               "elements to distinguish the Modified Version from the Original Version. The "
                               "name of such Modified Versions must be the name of the Original Version, with "
                               "&quot;derivative X&quot; where X represents the name of the new work, appended "
                               "to that name.</li></ol></li>"
                               "<li>The name(s) of the Copyright Holder(s) and any contributor to the Font "
                               "Software shall not be used to promote, endorse or advertise any Modified "
                               "Version, except (i) as required by this licence, (ii) to acknowledge the "
                               "contribution(s) of the Copyright Holder(s) or (iii) with their explicit written "
                               "permission.</li>"
                               "<li>The Font Software, modified or unmodified, in part or in whole, must be "
                               "distributed entirely under this licence, and must not be distributed under any "
                               "other licence. The requirement for fonts to remain under this licence does not "
                               "affect any document created using the Font Software, except any version of the "
                               "Font Software extracted from a document created using the Font Software may "
                               "only be distributed under this licence.</p>"
                               "<p>TERMINATION</li></ol>"
                               "<p>This licence becomes null and void if any of the above conditions are not "
                               "met.</p>"
                               "<p>DISCLAIMER</p>"
                               "<p>THE FONT SOFTWARE IS PROVIDED &quot;AS IS&quot;, WITHOUT WARRANTY OF ANY "
                               "KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO ANY WARRANTIES OF "
                               "MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT OF "
                               "COPYRIGHT, PATENT, TRADEMARK, OR OTHER RIGHT. IN NO EVENT SHALL THE COPYRIGHT "
                               "HOLDER BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, INCLUDING ANY "
                               "GENERAL, SPECIAL, INDIRECT, INCIDENTAL, OR CONSEQUENTIAL DAMAGES, WHETHER IN AN "
                               "ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF THE USE OR "
                               "INABILITY TO USE THE FONT SOFTWARE OR FROM OTHER DEALINGS IN THE FONT "
                               "SOFTWARE.</p>"));
#endif

    QString communiHeader(tr("<h2><u>Communi IRC Library</u></h2>"
                             "<h3>Copyright © 2008-2016 The Communi Project</h3>"));

    QString communiKonverstionSuppliment(tr("<p>Parts of <tt>irctextformat.cpp</t> code come from Konversation and are copyrighted to:<br>"
             "Copyright © 2002 Dario Abatianni &lt;eisfuchs@tigress.com&gt;<br>"
             "Copyright © 2004 Peter Simonsson &lt;psn@linux.se&gt;<br>"
             "Copyright © 2006-2008 Eike Hein &lt;hein@kde.org&gt;<br>"
             "Copyright © 2004-2009 Eli Mackenzie &lt;argonel@gmail.com&gt;</p>"));

    QString luaHeader(tr("<h2><u>lua - Lua 5.1</u></h2>"
                         "<h3>Copyright © 1994–2017 Lua.org, PUC-Rio.</h3>"));

    QString luaYajlHeader(tr("<h2><u>lua_yajl - Lua 5.1 interface to yajl</u></h2>"
                             "<h3>Author: Brian Maher &lt;maherb at brimworks dot com&gt;<br>"
                             "Copyright © 2009 Brian Maher</h3>"));

#if defined(Q_OS_MACOS) || defined(DEBUG_SHOWALL)
    QString luaZipHeader(tr("<h2><u>LuaZip - Reading files inside zip files</u></h2>"
                            "<h3>Author: Danilo Tuler<br>"
                            "Copyright © 2003-2007 Kepler Project</h3>"));
#endif

    QString edbeeHeader(tr("<h2><u>edbee - multi-feature editor widget</u></h2>"
                           "<h3>Copyright © 2012-2014 by Reliable Bits Software by Blommers IT</h3>"));

    QString edbeeSuppliment(tr("The <b>edbee-lib</b> widget itself incorporates other components with licences that must be noted as well, they are:"));

    QString OnigmoHeader(tr("<h2><u>Onigmo (Oniguruma-mod) LICENSE</u></h2>"
                            "<h3>Copyright © 2002-2009 K.Kosako &lt;sndgk393 AT ybb DOT ne DOT jp&gt;<br>"
                            "Copyright © 2011-2014 K.Takata &lt;kentkt AT csc DOT jp&gt;<br>"
                            "All rights reserved.</h3>"));

    QString OnigurumaHeader(tr("<h2><u>Oniguruma LICENSE</u></h2>"
                               "<h3>Copyright © 2002-2009 K.Kosako &lt;sndgk393 AT ybb DOT ne DOT jp&gt;<br>"
                               "All rights reserved.</h3>"));

    QString RubyHeader(tr("<h2><u>Ruby BSDL</u></h2>"
                          "<h3>Copyright © 1993-2013 Yukihiro Matsumoto.<br>"
                          "All rights reserved.</h3>"));

    QString QsLogHeader(tr("<h2><u>Qt-Components, QsLog</u></h2>"
                           "<h3>(<span style=\"color:red\"><u>https://bitbucket.org/razvapetru/qt-components [broken link]</u></span></a>"
                           "<small><a href=\"https://web.archive.org/web/20131220072148/https://bitbucket.org/razvanpetru/qt-components\"> {&quot;Wayback Machine&quot; archived version}</a></small>)<br>"
                           "Copyright © 2013, Razvan Petru<br>"
                           "All rights reserved.</h3>"));

#if defined(INCLUDE_UPDATER) || defined(DEBUG_SHOWALL)
    QString DblsqdHeader(tr("<h2><u>dblsqd</u></h2>"
                            "<h3>Copyright © 2017 Philipp Medien</h3>"));
#if defined(Q_OS_MACOS) || defined(DEBUG_SHOWALL)
    QString SparkleHeader(tr("<h2><u>Sparkle - macOS updater</u></h2>"
                             "<h3>Copyright © 2006-2013 Andy Matuschak.<br>"
                             "Copyright © 2009-2013 Elgato Systems GmbH.<br>"
                             "Copyright © 2011-2014 Kornel Lesiński.<br>"
                             "Copyright © 2015-2017 Mayur Pawashe.<br>"
                             "Copyright © 2014 C.W. Betts.<br>"
                             "Copyright © 2014 Petroules Corporation.<br>"
                             "Copyright © 2014 Big Nerd Ranch.<br>"
                             "All rights reserved.</h3>"));

    QString Sparkle3rdPartyHeader(tr("<h4>bspatch.c and bsdiff.c, from bsdiff 4.3 <a href=\"http://www.daemonology.net/bsdiff/\">http://www.daemonology.net/bsdiff</a>:</h4>"
                                     "<h3>Copyright © 2003-2005 Colin Percival.</h3>"
                                     "<h4>sais.c and sais.c, from sais-lite (2010/08/07) <a href=\"https://sites.google.com/site/yuta256/sais\">https://sites.google.com/site/yuta256/sais</a>:</h4>"
                                     "<h3>Copyright © 2008-2010 Yuta Mori.</h3>"
                                     "<h4>SUDSAVerifier.m:</h4>"
                                     "<h3>Copyright © 2011 Mark Hamlin.<br>"
                                     "All rights reserved.</h3>"));

    QString SparkleGlueHeader(tr("<h2><u>sparkle-glue</u></h2>"
                                 "<h3>Copyright © 2008 Remko Troncon<br>"
                                 "Copyright © 2017 Vadim Peretokin</h3>"));
#endif // defined(Q_OS_MACOS)
#endif // defined(INCLUDE_UPDATER)

    // Now start to assemble the fragments above:
    QStringList license_3rdParty_texts;
    license_3rdParty_texts.append(QStringLiteral("<html>%1<body>%2<hr>")
                                  .arg(htmlHead,                       //  1 - Html Header
                                       thirdPartiesHeader));           //  2 - Introductory header - translatable

    license_3rdParty_texts.append(QStringLiteral("%3%4%5<hr>")
                                  .arg(communiHeader,                  //  3 - Communi (IRC) header - translatable
                                       BSD3Clause_Body                 //  4 - Communi (IRC) body BSD3 ("COPYRIGHT HOLDERS AND/OR CONTRIBUTORS") - not translatable
                                       .arg(QStringLiteral("Neither the name of the Communi Project nor the names of its contributors may"),
                                            QStringLiteral("COPYRIGHT HOLDERS AND CONTRIBUTORS"),
                                            QStringLiteral("COPYRIGHT HOLDERS OR CONTRIBUTORS")),
                                       communiKonverstionSuppliment)); //  5 - Communi supplimentary about Konversation - translatable

    license_3rdParty_texts.append(QStringLiteral("%6%7<hr>%8%9<hr>")
                                  .arg(luaHeader,                      //  6 - lua header - translatable
                                       MIT_Body,                       //  7 - lua body MIT - not translatable
                                       luaYajlHeader,                  //  8 - lua_yajl header - translatable
                                       MIT_Body));                     //  9 - lua_yajl body MIT - not translatable
#if defined(Q_OS_MACOS) || defined(DEBUG_SHOWALL)
    license_3rdParty_texts.append(QStringLiteral("%10%11<hr>")
                                  .arg(luaZipHeader,                   // 10 - macOS luazip header - translatable
                                       MIT_Body));                     // 11 - macOS luazip body MIT - not translatable
#endif

    license_3rdParty_texts.append(QStringLiteral("%12%13")
                                  .arg(edbeeHeader,                    // 12 - edbee header - translatable
                                       MIT_Body));                     // 13 - edbee body MIT - not translatable

    license_3rdParty_texts.append(QStringLiteral("<hr width=\"50%\">%14"
                                                 "%15%16<hr width=\"33%\">"
                                                 "%17%18<hr width=\"33%\">"
                                                 "%19%20<hr width=\"33%\">"
                                                 "%21%22")
                                  .arg(edbeeSuppliment,                // 14 - edbee other components:
                                       OnigmoHeader,                   // 15 - Onigmo (Oniguruma-mod) header - translatable
                                       BSD2Clause_Body                 // 16 - Onigmo (Oniguruma-mod) body BSD2 ("AUTHOR AND/OR CONTRIBUTORS") - not translatable
                                       .arg(QStringLiteral("AUTHOR AND CONTRIBUTORS"),
                                            QStringLiteral("AUTHOR OR CONTRIBUTORS")),
                                       OnigurumaHeader,                // 17 - Oniguruma header - translatable
                                       BSD2Clause_Body                 // 18 - Oniguruma body BSD2 ("COPYRIGHT HOLDERS AND/OR CONTRIBUTORS") - not translatable
                                       .arg(QStringLiteral("COPYRIGHT HOLDERS AND CONTRIBUTORS"),
                                            QStringLiteral("COPYRIGHT HOLDERS OR CONTRIBUTORS")),
                                       RubyHeader,                     // 19 - Ruby Header - translatable
                                       BSD2Clause_Body                 // 20 - Ruby body BSD2 ("AUTHOR AND/OR CONTRIBUTORS") - not translatable
                                       .arg(QStringLiteral("AUTHOR AND CONTRIBUTORS"),
                                            QStringLiteral("AUTHOR OR CONTRIBUTORS")),
                                       QsLogHeader,                    // 21 - QsLog header - translatable
                                       BSD3Clause_Body                 // 22 - QsLog body BSD3 ("The name of the contributors may not" / "COPYRIGHT HOLDERS AND/OR CONTRIBUTORS") - not translatable
                                       .arg(QStringLiteral("The name of the contributors may not"),
                                            QStringLiteral("COPYRIGHT HOLDERS AND CONTRIBUTORS"),
                                            QStringLiteral("COPYRIGHT HOLDERS OR CONTRIBUTORS"))));

#if defined(INCLUDE_UPDATER) || defined(DEBUG_SHOWALL)
    license_3rdParty_texts.append(QStringLiteral("<hr>%23%24")
                                  .arg(DblsqdHeader,                   // 23 - dblsqd Header - translatable
                                       APACHE2_Body));                 // 24 - dblsqd body APACHE2 - not translatable
#if defined(Q_OS_MACOS) || defined(DEBUG_SHOWALL)
    license_3rdParty_texts.append(QStringLiteral("<hr width=\"50%\">%25%26<hr width=\"33%\">%27%28<hr width=\"33%\">%29%30")
                                  .arg(SparkleHeader,                  // 25 - Sparkle header - translatable
                                       MIT_Body,                       // 26 - Sparkle body MIT - not translatable
                                       Sparkle3rdPartyHeader,          // 27 - Sparkle 3rd Party headers - translatable
                                       BSD2Clause_Body                 // 28 - Sparkle 3rd Party body BSD2 ("AUTHOR") - not translatable
                                       .arg(QLatin1String("AUTHOR"),
                                            QLatin1String("AUTHOR")),
                                       SparkleGlueHeader,              // 29 - Sparkle glue header - translatable
                                       BSD2Clause_Body                 // 30 - Sparkle glue body BSD2 ("COPYRIGHT HOLDERS AND/OR CONTRIBUTORS") - not translatable
                                       .arg(QLatin1String("AUTHOR AND CONTRIBUTORS"),
                                            QLatin1String("AUTHOR OR CONTRIBUTORS"))));
#endif // defined(Q_OS_MACOS))
#endif // defined(INCLUDE_UPDATER)

#if defined(INCLUDE_FONTS) || defined(DEBUG_SHOWALL)
    license_3rdParty_texts.append(QStringLiteral("<hr>%31")
                                  .arg(UbuntuFontText));               // 31 - Ubuntu Font Text - not translatable
#endif
    license_3rdParty_texts.append(QStringLiteral("</body></html>"));

    textBrowser_license_3rdparty->setHtml(license_3rdParty_texts.join(QString()));

    // clang-format on
}
