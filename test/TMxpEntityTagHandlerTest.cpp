
#include <QTest>
#include "TMxpEntityTagHandler.h"
#include "TMxpStubClient.h"
#include <TMxpTagParser.h>
#include <TMxpTagProcessor.h>
#include <TMxpProcessor.h>

class TMxpEntityTagHandlerTest : public QObject {
Q_OBJECT

private:

private slots:
    QSharedPointer<MxpNode> parseNode(const QString& tagText) const
    {
        auto nodes = TMxpTagParser::parseToMxpNodeList(tagText);
        return nodes.size() > 0 ? nodes.first() : nullptr;
    }

    void processInput(TMxpProcessor &processor, std::string &input) {
        for (char &ch : input) {
            processor.processMxpInput(ch);
        }
    }

    void testPublish()
    {
        TMxpStubClient stub;
        TMxpTagProcessor processor;

        auto tag = parseNode("<!EN ob \"street lamp\" publish>");
        processor.handleNode(processor, stub, tag.get());

        QCOMPARE(processor.getEntityResolver().getResolution("&ob;"), "street lamp");

        QCOMPARE(stub.mPublishedEntityName, "&ob;");
        QCOMPARE(stub.mPublishedEntityValue, "street lamp");
    }

    void testPrivate()
    {
        TMxpStubClient stub;
        TMxpTagProcessor processor;

        auto tag = parseNode("<!EN ob \"street lamp\" private>");
        processor.handleNode(processor, stub, tag.get());

        QCOMPARE(processor.getEntityResolver().getResolution("&ob;"), "street lamp");

        QCOMPARE(stub.mPublishedEntityName, "");
        QCOMPARE(stub.mPublishedEntityValue, "");
    }

    void testAddRemoveStart()
    {
        TMxpStubClient stub;
        TMxpTagProcessor processor;

        processor.getEntityResolver().registerEntity("&entity;", "v1");
        QCOMPARE(processor.getEntityResolver().getResolution("&entity;"), "v1");

        processor.handleNode(processor, stub, parseNode("<!EN entity \"v2\" add>").get());
        QCOMPARE(processor.getEntityResolver().getResolution("&entity;"), "v1|v2");

        processor.handleNode(processor, stub, parseNode("<!EN entity \"v1\" remove>").get());
        QCOMPARE(processor.getEntityResolver().getResolution("&entity;"), "v2");

        processor.handleNode(processor, stub, parseNode("<!EN entity \"v2\" remove>").get());
        QCOMPARE(processor.getEntityResolver().getResolution("&entity;"), "");
    }

    void testAddRemoveEnd()
    {
        TMxpStubClient stub;
        TMxpTagProcessor processor;

        processor.getEntityResolver().registerEntity("&entity;", "v1");
        QCOMPARE(processor.getEntityResolver().getResolution("&entity;"), "v1");

        processor.handleNode(processor, stub, parseNode("<!EN entity \"v2\" add>").get());
        QCOMPARE(processor.getEntityResolver().getResolution("&entity;"), "v1|v2");

        processor.handleNode(processor, stub, parseNode("<!EN entity \"v2\" remove>").get());
        QCOMPARE(processor.getEntityResolver().getResolution("&entity;"), "v1");
    }

    void testAddRemovePartOfItemValue()
    {
        TMxpStubClient stub;
        TMxpTagProcessor processor;

        processor.getEntityResolver().registerEntity("&entity;", "my text example");
        QCOMPARE(processor.getEntityResolver().getResolution("&entity;"), "my text example");

        processor.handleNode(processor, stub, parseNode("<!EN entity \"my value\" add>").get());
        QCOMPARE(processor.getEntityResolver().getResolution("&entity;"), "my text example|my value");

        processor.handleNode(processor, stub, parseNode("<!EN entity \"my text\" remove>").get());
        QCOMPARE(processor.getEntityResolver().getResolution("&entity;"), "my text example|my value");

        processor.handleNode(processor, stub, parseNode("<!EN entity \"example\" remove>").get());
        QCOMPARE(processor.getEntityResolver().getResolution("&entity;"), "my text example|my value");

        processor.handleNode(processor, stub, parseNode("<!EN entity \"value\" remove>").get());
        QCOMPARE(processor.getEntityResolver().getResolution("&entity;"), "my text example|my value");

        processor.handleNode(processor, stub, parseNode("<!EN entity \"my\" remove>").get());
        QCOMPARE(processor.getEntityResolver().getResolution("&entity;"), "my text example|my value");
    }

    void testInterpolation()
    {
        TMxpStubClient stub;
        TMxpProcessor processor(&stub);

        std::string input = "<!EN ob \"street lamp\" private><send href=\"kill &ob;\">kill</send>";
        processInput(processor, input);

        QCOMPARE(stub.mHrefs.size(), 1);
        QCOMPARE(stub.mHrefs[0], "send([[kill street lamp]])");

        QCOMPARE(stub.mHints.size(), 1);
        QCOMPARE(stub.mHints[0], "kill street lamp");
    }

    void testDelete() {
        TMxpStubClient stub;
        TMxpTagProcessor processor;

        processor.getEntityResolver().registerEntity("&entity;", "v1");
        QCOMPARE(processor.getEntityResolver().getResolution("&entity;"), "v1");

        processor.handleNode(processor, stub, parseNode("<!EN entity delete>").get());
        QCOMPARE(processor.getEntityResolver().getResolution("&entity;"), "&entity;");
    }
};

#include "TMxpEntityTagHandlerTest.moc"
QTEST_MAIN(TMxpEntityTagHandlerTest)

