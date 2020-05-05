
#include <QTest>
#include "TMxpSendTagHandler.h"
#include "TMxpStubClient.h"
#include <TMxpTagParser.h>
#include <TMxpTagProcessor.h>


class TMxpSendTagHandlerTest : public QObject {
Q_OBJECT

private:

private slots:

    void testStaticText()
    {
        // <SEND "tell Zugg " PROMPT>Zugg</SEND>
        TMxpStubContext ctx;
        TMxpStubClient stub;

        TMxpTagParser parser;
        MxpStartTag* startTag = parser.parseStartTag("<SEND \"tell Zugg \" PROMPT>");
        MxpEndTag* endTag = parser.parseEndTag("</SEND>");

        TMxpSendTagHandler sendTagHandler;
        TMxpTagHandler& tagHandler = sendTagHandler;
        tagHandler.handleTag(ctx, stub, startTag);
        tagHandler.handleContent("Zugg");
        tagHandler.handleTag(ctx, stub, endTag);

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

        TMxpTagParser parser;
        MxpStartTag* startTag = parser.parseStartTag("<SEND href=\"&text;\" PROMPT>");
        MxpEndTag* endTag = parser.parseEndTag("</SEND>");

        TMxpSendTagHandler sendTagHandler;
        TMxpTagHandler& tagHandler = sendTagHandler;
        tagHandler.handleTag(ctx, stub, startTag);
        tagHandler.handleContent("north");
        tagHandler.handleTag(ctx, stub, endTag);

        QCOMPARE(stub.mHrefs.size(), 1);
        QCOMPARE(stub.mHrefs[0], "printCmdLine([[north]])");

        QCOMPARE(stub.mHints.size(), 1);
        QCOMPARE(stub.mHints[0], "north");
    }

    void testSendHrefTextEntity() {
        // Example from Age of Elements
        QString input = "<send 'push &text;' HINT='push button'>button</send>";

        TMxpTagParser parser;
        TMxpTagProcessor processor;
        TMxpStubClient stub;


        auto nodes = parser.parseToMxpNodeList(input, false);
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

        TMxpTagParser parser;
        MxpStartTag* startTag = parser.parseStartTag("<SEND href=\"say I am &charName;\">");
        MxpEndTag* endTag = parser.parseEndTag("</SEND>");

        TMxpSendTagHandler sendTagHandler;
        TMxpTagHandler& tagHandler = sendTagHandler;
        tagHandler.handleTag(ctx, stub, startTag);
        tagHandler.handleContent("TAG CONTENT");
        tagHandler.handleTag(ctx, stub, endTag);

        QCOMPARE(stub.mHrefs.size(), 1);
        QCOMPARE(stub.mHrefs[0], "send([[say I am Gandalf]])");

        QCOMPARE(stub.mHints.size(), 1);
        QCOMPARE(stub.mHints[0], "say I am Gandalf");
    }

};

#include "TMxpSendTagHandlerTest.moc"
QTEST_MAIN(TMxpSendTagHandlerTest)

