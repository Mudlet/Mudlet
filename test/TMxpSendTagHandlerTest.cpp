/***************************************************************************
 *   Copyright (C) 2020 by Gustavo Sousa - gustavocms@gmail.com            *
 *   Copyright (C) 2021 by Florian Scheel - keneanung@gmail.com            *
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

// Aldebaran / Entity in Send Test by Michael Weller, 12/2020, michael.weller@t-online.de


#include <QTest>
#include <TMxpSendTagHandler.h>
#include <TMxpTagParser.h>
#include <TMxpTagProcessor.h>
#include <TMxpProcessor.h>

#include "TMxpStubClient.h"


class TMxpSendTagHandlerTest : public QObject {
Q_OBJECT

private:

private slots:
    QSharedPointer<MxpNode> parseNode(const QString& tagText) const
    {
        auto nodes = TMxpTagParser::parseToMxpNodeList(tagText);
        return nodes.size() > 0 ? nodes.first() : nullptr;
    }

    void testSendHrefUTF8FromMxpProcessor()
    {
        // issue #4368
        TMxpStubClient stub;
        TMxpProcessor processor(&stub);

        std::string input = "<SEND href=\"áéíóúñ\" >test link: áéíóúñ</SEND>";
        for (char &ch : input) {
          processor.processMxpInput(ch);
        }

        QCOMPARE(stub.mHrefs.size(), 1);
        QCOMPARE(stub.mHrefs[0], "send([[áéíóúñ]])");

        QCOMPARE(stub.mHints.size(), 1);
        QCOMPARE(stub.mHints[0], "áéíóúñ");

    }

    void testSendHrefUTF8()
    {
        // issue #4368
        QString input = "<SEND href=\"áéíóúñ\" >test link: áéíóúñ</SEND>";

        TMxpTagProcessor processor;
        TMxpStubClient stub;

        auto nodes = TMxpTagParser::parseToMxpNodeList(input, false);
        QCOMPARE(nodes.size(), 3);
        for (const auto &node : nodes) {
          processor.handleNode(processor, stub, node.get());
        }

        QCOMPARE(stub.mHrefs.size(), 1);
        QCOMPARE(stub.mHrefs[0], "send([[áéíóúñ]])");

        QCOMPARE(stub.mHints.size(), 1);
        QCOMPARE(stub.mHints[0], "áéíóúñ");
    }

    void testStaticText()
    {
        // <SEND "tell Zugg " PROMPT>Zugg</SEND>
        TMxpStubContext ctx;
        TMxpStubClient stub;

        auto startTag = parseNode("<SEND \"tell Zugg \" PROMPT>");
        auto endTag = parseNode("</SEND>");

        TMxpSendTagHandler sendTagHandler;
        TMxpTagHandler& tagHandler = sendTagHandler;
        tagHandler.handleTag(ctx, stub, startTag->asStartTag());
        tagHandler.handleContent("Zugg");
        tagHandler.handleTag(ctx, stub, endTag->asEndTag());

        QCOMPARE(stub.mHrefs.size(), 1);
        QCOMPARE(stub.mHrefs[0], "printCmdLine([[tell Zugg ]])");

        QCOMPARE(stub.mHints.size(), 1);
        QCOMPARE(stub.mHints[0], "tell Zugg ");
    }

    void testSimpleSend()
    {
        // <SEND>north</SEND>
        TMxpStubContext ctx;
        TMxpStubClient stub;

        MxpStartTag startTag("SEND");
        MxpEndTag endTag("SEND");

        TMxpSendTagHandler sendTagHandler;
        TMxpTagHandler& tagHandler = sendTagHandler;

        tagHandler.handleTag(ctx, stub, &startTag);
        tagHandler.handleContent("north");
        tagHandler.handleTag(ctx, stub, &endTag);

        QCOMPARE(stub.mHrefs.size(), 1);
        QCOMPARE(stub.mHrefs[0], "send([[north]])");

        QCOMPARE(stub.mHints.size(), 1);
        QCOMPARE(stub.mHints[0], "north");
    }

    void testSendPrompt()
    {
        // <SEND href="&text;" PROMPT>north</SEND>
        TMxpStubContext ctx;
        TMxpStubClient stub;

        auto startTag = parseNode("<SEND href=\"&text;\" PROMPT>");
        auto endTag = parseNode("</SEND>");

        TMxpSendTagHandler sendTagHandler;
        TMxpTagHandler& tagHandler = sendTagHandler;
        tagHandler.handleTag(ctx, stub, startTag->asStartTag());
        tagHandler.handleContent("north");
        tagHandler.handleTag(ctx, stub, endTag->asEndTag());

        QCOMPARE(stub.mHrefs.size(), 1);
        QCOMPARE(stub.mHrefs[0], "printCmdLine([[north]])");

        QCOMPARE(stub.mHints.size(), 1);
        QCOMPARE(stub.mHints[0], "north");
    }

    void testSendHrefTextEntity() {
        // Example from Age of Elements
        QString input = "<send 'push &text;' HINT='push button'>button</send>";

        TMxpTagProcessor processor;
        TMxpStubClient stub;


        auto nodes = TMxpTagParser::parseToMxpNodeList(input, false);
        for (const auto& node : nodes) {
            processor.handleNode(processor, stub, node.get());
        }

        QCOMPARE(stub.mHrefs.size(), 1);
        QCOMPARE(stub.mHrefs[0], "send([[push button]])");

        QCOMPARE(stub.mHints.size(), 1);
        QCOMPARE(stub.mHints[0], "push button");
    }

    void testResolvingEntity()
    {
        TMxpStubContext ctx;
        TMxpStubClient stub;

        ctx.getEntityResolver().registerEntity("&charName;", "Gandalf");

        auto startTag = parseNode("<SEND href=\"say I am &charName;\">");
        auto endTag = parseNode("</SEND>");

        TMxpSendTagHandler sendTagHandler;
        TMxpTagHandler& tagHandler = sendTagHandler;
        tagHandler.handleTag(ctx, stub, startTag->asStartTag());
        tagHandler.handleContent("TAG CONTENT");
        tagHandler.handleTag(ctx, stub, endTag->asEndTag());

        QCOMPARE(stub.mHrefs.size(), 1);
        QCOMPARE(stub.mHrefs[0], "send([[say I am Gandalf]])");

        QCOMPARE(stub.mHints.size(), 1);
        QCOMPARE(stub.mHints[0], "say I am Gandalf");
    }

    void testSendHrefHintMismatch() {
        // Example from starmourn on WARES command from NPCs
        // <SEND HREF="PROBE SUSPENDERS30901|BUY SUSPENDERS30901" hint="Click to see command menu">30901</SEND>
        TMxpStubContext ctx;
        TMxpStubClient stub;

        auto startTag = parseNode(R"(<SEND HREF="PROBE SUSPENDERS30901|BUY SUSPENDERS30901" hint="Click to see command menu">)");
        auto endTag = parseNode("</SEND>");

        TMxpSendTagHandler sendTagHandler;
        TMxpTagHandler& tagHandler = sendTagHandler;
        tagHandler.handleTag(ctx, stub, startTag->asStartTag());
        tagHandler.handleContent("3091");
        tagHandler.handleTag(ctx, stub, endTag->asEndTag());

        QCOMPARE(stub.mHrefs.size(), 2);
        QCOMPARE(stub.mHrefs[0], "send([[PROBE SUSPENDERS30901]])");
        QCOMPARE(stub.mHrefs[1], "send([[BUY SUSPENDERS30901]])");

        QCOMPARE(stub.mHints.size(), 3);
        QCOMPARE(stub.mHints[0], "Click to see command menu");
        QCOMPARE(stub.mHints[1], "PROBE SUSPENDERS30901");
        QCOMPARE(stub.mHints[2], "BUY SUSPENDERS30901");
    }

    void testSendHrefHintTooFew() {
        // For Send Menus, if there is more than 1 hint but not enough for per command hints,
        // fill them up with HREF texts
        // <SEND HREF="say a|say b|say c" hint="Talk about A|Talk about B">telling nonsense</SEND>
        TMxpStubContext ctx;
        TMxpStubClient stub;

        auto startTag = parseNode(R"(<SEND HREF="say a|say b|say c" hint="Talk about A|Talk about B">)");
        auto endTag = parseNode("</SEND>");

        TMxpSendTagHandler sendTagHandler;
        TMxpTagHandler& tagHandler = sendTagHandler;
        tagHandler.handleTag(ctx, stub, startTag->asStartTag());
        tagHandler.handleContent("telling nonsense");
        tagHandler.handleTag(ctx, stub, endTag->asEndTag());

        QCOMPARE(stub.mHrefs.size(), 3);
        QCOMPARE(stub.mHrefs[0], "send([[say a]])");
        QCOMPARE(stub.mHrefs[1], "send([[say b]])");
        QCOMPARE(stub.mHrefs[2], "send([[say c]])");

        QCOMPARE(stub.mHints.size(), 4);
        // MouseOver for SEND Menu is MXP implementation dependent and thus not literally checked:
        // QCOMPARE(stub.mHints[0], "Click to see command menu");
        QCOMPARE(stub.mHints[1], "Talk about A");
        QCOMPARE(stub.mHints[2], "Talk about B");
        QCOMPARE(stub.mHints[3], "say c");
    }


    void testSendHrefHintTooMany() {
        // If there are too many hints for HREFS and SEND MENU Mouse over: Ignore them
        // for historical reasons in the source, the first hints are ignored (this originates from a time were the send menu mouse over could
        // not be set)
        // <SEND HREF="say a|say b|say c" hint="Right-click for options|Talk about A|Talk about B|Talk about C|Talk about D">telling nonsense</SEND>
        TMxpStubContext ctx;
        TMxpStubClient stub;

        auto startTag = parseNode(R"(<SEND HREF="say a|say b|say c" hint="Right-click for options|Talk about A|Talk about B|Talk about C|Talk about D">)");
        auto endTag = parseNode("</SEND>");

        TMxpSendTagHandler sendTagHandler;
        TMxpTagHandler& tagHandler = sendTagHandler;
        tagHandler.handleTag(ctx, stub, startTag->asStartTag());
        tagHandler.handleContent("telling nonsense");
        tagHandler.handleTag(ctx, stub, endTag->asEndTag());

        QCOMPARE(stub.mHrefs.size(), 3);
        QCOMPARE(stub.mHrefs[0], "send([[say a]])");
        QCOMPARE(stub.mHrefs[1], "send([[say b]])");
        QCOMPARE(stub.mHrefs[2], "send([[say c]])");

        QCOMPARE(stub.mHints.size(), 4);
        // MouseOver for SEND Menu is MXP implementation dependent and thus not literally checked:
        QCOMPARE(stub.mHints[0], "Talk about A");
        QCOMPARE(stub.mHints[1], "Talk about B");
        QCOMPARE(stub.mHints[2], "Talk about C");
        QCOMPARE(stub.mHints[3], "Talk about D");
    }

    void testSendCustomMenuHint() {
        // Example: Everywhere in Aldebaran
        // <SEND HREF="wield item##2|drop item##2" hint="wield|drop">sword</SEND>
        TMxpStubContext ctx;
        TMxpStubClient stub;

        auto startTag = parseNode(R"(<SEND HREF="wield item##2|drop item##2" hint="wield|drop">)");
        auto endTag = parseNode("</SEND>");

        TMxpSendTagHandler sendTagHandler;
        TMxpTagHandler& tagHandler = sendTagHandler;
        tagHandler.handleTag(ctx, stub, startTag->asStartTag());
        tagHandler.handleContent("sword");
        tagHandler.handleTag(ctx, stub, endTag->asEndTag());

        QCOMPARE(stub.mHrefs.size(), 2);
        QCOMPARE(stub.mHrefs[0], "send([[wield item##2]])");
        QCOMPARE(stub.mHrefs[1], "send([[drop item##2]])");

        QCOMPARE(stub.mHints.size(), 3);
        // MouseOver for SEND Menu is MXP implementation dependent and thus not literally checked:
        // QCOMPARE(stub.mHints[0], "Click to see command menu");
        QCOMPARE(stub.mHints[1], "wield");
        QCOMPARE(stub.mHints[2], "drop");
    }

    void testSendCustomMenuHintMouseOver() {
        // Example: Everywhere in Aldebaran
        // <SEND HREF="wield item##2|drop item##2" hint="Right-click for options|wield|drop">sword</SEND>
        TMxpStubContext ctx;
        TMxpStubClient stub;

        auto startTag = parseNode(R"(<SEND HREF="wield item##2|drop item##2" hint="Right-click for options|wield|drop">)");
        auto endTag = parseNode("</SEND>");

        TMxpSendTagHandler sendTagHandler;
        TMxpTagHandler& tagHandler = sendTagHandler;
        tagHandler.handleTag(ctx, stub, startTag->asStartTag());
        tagHandler.handleContent("sword");
        tagHandler.handleTag(ctx, stub, endTag->asEndTag());

        QCOMPARE(stub.mHrefs.size(), 2);
        QCOMPARE(stub.mHrefs[0], "send([[wield item##2]])");
        QCOMPARE(stub.mHrefs[1], "send([[drop item##2]])");

        QCOMPARE(stub.mHints.size(), 3);
        // MouseOver is now explicitly set:
        QCOMPARE(stub.mHints[0], "Right-click for options");
        QCOMPARE(stub.mHints[1], "wield");
        QCOMPARE(stub.mHints[2], "drop");
    }

    void testSendEntityWithPipeOptMouseOver() {
        // Example: Aldebaran: Prompt Menu containing aliases, Send Menu Mouse Over Hint provided for clients that
        // support and need it (mushclient) (&HD is empty for clients that cannot deal with it (z/cmud))
        // <SEND HREF="&CMDS;who|inventory" hint="&HD;&CMDH;Who is online|Your inventory">Aldebaran&gt; </SEND>
        TMxpStubContext ctx;
        TMxpStubClient stub;

        ctx.getEntityResolver().registerEntity("&CMDS;", "gc|");
        ctx.getEntityResolver().registerEntity("&CMDH;", "get all from corpse|");
        ctx.getEntityResolver().registerEntity("&HD;", "Right-click for options|");

        auto startTag = parseNode(R"(<SEND HREF="&CMDS;who|inventory" hint="&HD;&CMDH;Who is online?|show inventory">)");
        auto endTag = parseNode("</SEND>");

        TMxpSendTagHandler sendTagHandler;
        TMxpTagHandler& tagHandler = sendTagHandler;
        tagHandler.handleTag(ctx, stub, startTag->asStartTag());
        tagHandler.handleContent("Aldebaran&gt; ");
        tagHandler.handleTag(ctx, stub, endTag->asEndTag());

        QCOMPARE(stub.mHrefs.size(), 3);
        QCOMPARE(stub.mHrefs[0], "send([[gc]])");
        QCOMPARE(stub.mHrefs[1], "send([[who]])");
        QCOMPARE(stub.mHrefs[2], "send([[inventory]])");

        QCOMPARE(stub.mHints.size(), 4);
        // MouseOver is now explicitly set:
        QCOMPARE(stub.mHints[0], "Right-click for options");
        QCOMPARE(stub.mHints[1], "get all from corpse");
        QCOMPARE(stub.mHints[2], "Who is online?");
        QCOMPARE(stub.mHints[3], "show inventory");
    }
};

#include "TMxpSendTagHandlerTest.moc"
QTEST_MAIN(TMxpSendTagHandlerTest)

