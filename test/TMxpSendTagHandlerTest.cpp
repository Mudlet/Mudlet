
#include <QTest>
#include "TMxpSendTagHandler.h"
#include "TMxpStubClient.h"
#include <TMxpTagParser.h>
#include <TMxpTagProcessor.h>
#include <TMxpProcessor.h>

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
          processor.processMxpInput(ch, true);
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

    void testResolvingEntityWithPipe()
    {
        TMxpStubContext ctx;
        TMxpStubClient stub;

        ctx.getEntityResolver().registerEntity("&frontHint;", "");
        ctx.getEntityResolver().registerEntity("&frontHref;", "");

        ctx.getEntityResolver().registerEntity("&backHints;", "");
        ctx.getEntityResolver().registerEntity("&backHrefs;", "");

        TMxpSendTagHandler sendTagHandler;
        TMxpTagHandler& tagHandler = sendTagHandler;

        // First check the SEND TAG with empty entities
        auto startTag = parseNode("<SEND href=\"&frontHref;look|say hello&backHrefs;\" hint=\"&frontHint;LOOK AROUND|SAY HELLO&backHints;\">");
        auto endTag = parseNode("</SEND>");

        tagHandler.handleTag(ctx, stub, startTag->asStartTag());
        tagHandler.handleContent("TAG CONTENT");
        tagHandler.handleTag(ctx, stub, endTag->asEndTag());

        QCOMPARE(stub.mHrefs.size(), 2);
        QCOMPARE(stub.mHrefs[0], "send([[look]])");
        QCOMPARE(stub.mHrefs[1], "send([[say hello]])");

        QCOMPARE(stub.mHints.size(), 2);
        QCOMPARE(stub.mHints[0], "LOOK AROUND");
        QCOMPARE(stub.mHints[1], "SAY HELLO");

        // Now add top menu entries

        ctx.getEntityResolver().registerEntity("&frontHint;", "WHO IS ONLINE?|");
        ctx.getEntityResolver().registerEntity("&frontHref;", "who|");

        tagHandler.handleTag(ctx, stub, startTag->asStartTag());
        tagHandler.handleContent("TAG CONTENT");
        tagHandler.handleTag(ctx, stub, endTag->asEndTag());

        QCOMPARE(stub.mHrefs.size(), 3);
        QCOMPARE(stub.mHrefs[0], "send([[who]])");
        QCOMPARE(stub.mHrefs[1], "send([[look]])");
        QCOMPARE(stub.mHrefs[2], "send([[say hello]])");

        QCOMPARE(stub.mHints.size(), 3);
        QCOMPARE(stub.mHints[0], "WHO IS ONLINE?");
        QCOMPARE(stub.mHints[1], "LOOK AROUND");
        QCOMPARE(stub.mHints[2], "SAY HELLO");

        // Finally add something to the end of the menu

        ctx.getEntityResolver().registerEntity("&backHints;", "|KNOCK AT THE DOOR|BREAK THE DOOR");
        ctx.getEntityResolver().registerEntity("&backHrefs;", "|knock at door|break door");

        tagHandler.handleTag(ctx, stub, startTag->asStartTag());
        tagHandler.handleContent("TAG CONTENT");
        tagHandler.handleTag(ctx, stub, endTag->asEndTag());

        QCOMPARE(stub.mHrefs.size(), 5);
        QCOMPARE(stub.mHrefs[0], "send([[who]])");
        QCOMPARE(stub.mHrefs[1], "send([[look]])");
        QCOMPARE(stub.mHrefs[2], "send([[say hello]])");
        QCOMPARE(stub.mHrefs[3], "send([[knock at door]])");
        QCOMPARE(stub.mHrefs[4], "send([[break door]])");

        QCOMPARE(stub.mHints.size(), 5);
        QCOMPARE(stub.mHints[0], "WHO IS ONLINE?");
        QCOMPARE(stub.mHints[1], "LOOK AROUND");
        QCOMPARE(stub.mHints[2], "SAY HELLO");
        QCOMPARE(stub.mHints[3], "KNOCK AT THE DOOR");
        QCOMPARE(stub.mHints[4], "BREAK THE DOOR");
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

        QCOMPARE(stub.mHints.size(), 2);
        QCOMPARE(stub.mHints[0], "PROBE SUSPENDERS30901");
        QCOMPARE(stub.mHints[1], "BUY SUSPENDERS30901");
    }

};

#include "TMxpSendTagHandlerTest.moc"
QTEST_MAIN(TMxpSendTagHandlerTest)

