
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

        QCOMPARE(stub.mHints.size(), 2);
        QCOMPARE(stub.mHints[0], "PROBE SUSPENDERS30901");
        QCOMPARE(stub.mHints[1], "BUY SUSPENDERS30901");
    }

};

#include "TMxpSendTagHandlerTest.moc"
QTEST_MAIN(TMxpSendTagHandlerTest)

