#include <MxpTag.h>
#include <QtTest/QtTest>
#include "TMxpTagParser.h"

class TMxpTagParserTest : public QObject {
Q_OBJECT

private:

private slots:

    void initTestCase()
    {}


    void testParseWithQuotes()
    {
        TMxpTagParser parser;
        QList<QSharedPointer<MxpNode>> list = parser.parseToMxpNodeList("  <!EN quot '\"'>", true);

        QCOMPARE(list.size(), 1);
        QCOMPARE(list[0]->asStartTag()->getName(), "!EN");
        QCOMPARE(list[0]->asStartTag()->getAttrName(0), "quot");
        QCOMPARE(list[0]->asStartTag()->getAttrName(1), "\"");
    }

    void testParseToNodes()
    {
        TMxpTagParser parser;
        QList<QSharedPointer<MxpNode>> list = parser.parseToMxpNodeList("<COLOR fore=red><B>");

        QCOMPARE(list.size(), 2);
        QVERIFY(list[0].get()->isTag());
        QCOMPARE(list[0]->asStartTag()->getName(), "COLOR");
        QCOMPARE(list[0]->asStartTag()->getAttributeValue("FORE"), "red");
        QCOMPARE(list[1]->asStartTag()->getName(), "B");
    }

    void testParseToNodesWithText()
    {
        TMxpTagParser parser;
        QList<QSharedPointer<MxpNode>> list = parser.parseToMxpNodeList("<SEND>north</SEND>");

        QCOMPARE(list.size(), 3);
        QVERIFY(list[0].get()->isTag());
        QCOMPARE(list[0]->asStartTag()->getName(), "SEND");

        QVERIFY(!list[1]->isTag());
        QCOMPARE(list[1]->asText()->getContent(), "north");
        QCOMPARE(list[2]->asEndTag()->getName(), "SEND");
    }

    void testParseToNodesIgnoreText()
    {
        TMxpTagParser parser;
        QList<QSharedPointer<MxpNode>> list = parser.parseToMxpNodeList("  <COLOR fore=red>asafsdf<B>asdf", true);

        QCOMPARE(list.size(), 2);
        QVERIFY(list[0].get()->isTag());
        QCOMPARE(list[0]->asStartTag()->getName(), "COLOR");
        QCOMPARE(list[0]->asStartTag()->getAttributeValue("FORE"), "red");
        QCOMPARE(list[1]->asStartTag()->getName(), "B");
    }

    void testParseToNodesWithClosedAndEndTag()
    {
        TMxpTagParser parser;
        QList<QSharedPointer<MxpNode>> list = parser.parseToMxpNodeList("<RNum 2 /><SEND>text</ SEND>");

        QCOMPARE(list.size(), 4);
        QVERIFY(list[0].get()->isTag());
        QVERIFY(list[0].get()->asStartTag()->isEmpty());
        QCOMPARE(list[0].get()->asStartTag()->getAttrName(0), "2");

        QVERIFY(list[3].get()->isEndTag());
        QCOMPARE(list[3].get()->asEndTag()->getName(), "SEND");

    }

    void testParseToNodesWithTextAndSpaces()
    {
        TMxpTagParser parser;
        QList<QSharedPointer<MxpNode>> list = parser.parseToMxpNodeList("<SEND href=\"&text;\">  go north...  </SEND>");

        QCOMPARE(list.size(), 3);
        QVERIFY(list[0].get()->isTag());
        QCOMPARE(list[0]->asStartTag()->getName(), "SEND");
        QCOMPARE(list[0]->asStartTag()->getAttributeValue("href"), "&text;");

        QVERIFY(!list[1]->isTag());
        QCOMPARE(list[1]->asText()->getContent(), "  go north...  ");
        QCOMPARE(list[2]->asEndTag()->getName(), "SEND");
    }

    void testComplexElementDefinitionToTag()
    {
        QString tagLine =
                "<!EL get \"<send href='examine &#34;&name;&#34;|get &#34;&name;&#34;' hint='Right mouse click to act on this item|Get &desc;|Examine &desc;|Look in &desc;' expire=get>\" ATT='name desc'>";
        TMxpTagParser parser;

        MxpStartTag tag = *parser.parseStartTag(tagLine);

        QCOMPARE(tag.getName(), "!EL");
        QCOMPARE(tag.getAttributesCount(), 3);

        QCOMPARE(tag.getAttribute(0).getName(), "get");
        QCOMPARE(tag.getAttribute(0).getValue(), "");

        QCOMPARE(tag.getAttribute(1).getName(),
                 "<send href='examine &#34;&name;&#34;|get &#34;&name;&#34;' hint='Right mouse click to act on this item|Get &desc;|Examine &desc;|Look in &desc;' expire=get>");
        QCOMPARE(tag.getAttributeValue("ATT"), "name desc");

        MxpStartTag sendTag = *parser.parseStartTag(tag.getAttribute(1).getName());
        QCOMPARE(sendTag.getName(), "send");
    }

    void testMxpTagParser()
    {
        TMxpTagParser parser;
        MxpStartTag tag = *parser.parseStartTag("<!EL RExit FLAG=RoomExit>");

        QCOMPARE(tag.getName(), "!EL");

        QVERIFY(tag.hasAttribute("RExit"));
        QCOMPARE(tag.getAttributeValue("RExit"), "");
        QCOMPARE(tag.getAttribute(0).getName(), "RExit");
        QCOMPARE(tag.getAttribute(0).getValue(), "");

        QVERIFY(tag.hasAttribute("FLAG"));
        QCOMPARE(tag.getAttributeValue("FLAG"), "RoomExit");
    }

    void testComplexElementDefinitionToList()
    {
        QString tagLine =
                "!EL get \"<send href='examine &#34;&name;&#34;|get &#34;&name;&#34;' hint='Right mouse click to act on this item|Get &desc;|Examine &desc;|Look in &desc;' expire=get>\" ATT='name desc'";

        QStringList list = TMxpTagParser::parseToList(tagLine);
        QCOMPARE(list.size(), 4);
        QCOMPARE(list[0], "!EL");
        QCOMPARE(list[1], "get");
        QCOMPARE(list[2],
                 "<send href='examine &#34;&name;&#34;|get &#34;&name;&#34;' hint='Right mouse click to act on this item|Get &desc;|Examine &desc;|Look in &desc;' expire=get>");
        QCOMPARE(list[3], "ATT='name desc'");
    }

    void testAttrDefinition()
    {
        QString tagLine = R"(<!ATTLIST boldtext 'color=red background=white flags'>)";
        TMxpTagParser::parseToList(tagLine);

    }

    void testSimpleElementDefition()
    {
        TMxpTagParser parser;

        QString tagLine = "!EL RExit FLAG=RoomExit";
        QStringList list = TMxpTagParser::parseToList(tagLine);
        QCOMPARE(list.size(), 3);
        QCOMPARE(list[0], "!EL");
        QCOMPARE(list[1], "RExit");
        QCOMPARE(list[2], "FLAG=RoomExit");
    }

    void testSimpleQuotedElementDefition()
    {
        TMxpTagParser parser;

        QString tagLine = "!EL RExit 'FLAG=RoomExit'";
        QStringList list = TMxpTagParser::parseToList(tagLine);
        QCOMPARE(list[0], "!EL");
        QCOMPARE(list[1], "RExit");
        QCOMPARE(list[2], "FLAG=RoomExit");
    }

    void testElementDefitionWithExtraSpaces()
    {
        TMxpTagParser parser;

        QString tagLine = "!EL RExit   FLAG=RoomExit";
        QStringList list = TMxpTagParser::parseToList(tagLine);
        QCOMPARE(list[0], "!EL");
        QCOMPARE(list[1], "RExit");
        QCOMPARE(list[2], "FLAG=RoomExit");
    }

    void testDoubleQuotedElementDefition()
    {
        TMxpTagParser parser;

        QString tagLine = R"(!EL RExit "FLAG=RoomExit")";
        QStringList list = TMxpTagParser::parseToList(tagLine);
        QCOMPARE(list[0], "!EL");
        QCOMPARE(list[1], "RExit");
        QCOMPARE(list[2], "FLAG=RoomExit");
    }

    void testElementDefitionQuotedAttributeSpaces()
    {
        TMxpTagParser parser;

        QString tagLine = "!EL sHp FLAG='Set Hp'";
        QStringList list = TMxpTagParser::parseToList(tagLine);
        QCOMPARE(list[0], "!EL");
        QCOMPARE(list[1], "sHp");
        QCOMPARE(list[2], "FLAG='Set Hp'");
    }

    void testAccessAttrCaseInsensitive()
    {
        TMxpTagParser parser;
        QString tagLine = "<!EL sHp FLAG='Set Hp'>";

        MxpStartTag tag = *parser.parseStartTag(tagLine);

        QVERIFY(tag.hasAttribute("sHp"));
        QVERIFY(tag.hasAttribute("shp"));
        QVERIFY(tag.hasAttribute("shP"));

    }

    void testElementDefitionQuotesInQuotes()
    {
        TMxpTagParser parser;

        QString tagLine = "!EL x FLAG='Quote \" ex'";
        QStringList list = TMxpTagParser::parseToList(tagLine);
        QCOMPARE(list[0], "!EL");
        QCOMPARE(list[1], "x");
        QCOMPARE(list[2], "FLAG='Quote \" ex'");
    }

    void testElementComplet1()
    {
        TMxpTagParser parser;

        QString tagLine = R"(FRAME Name="Map" Left="-20c" Top="0" Width="20c" Height="20c")";
        QStringList list = TMxpTagParser::parseToList(tagLine);

        QCOMPARE(list[0], "FRAME");
        QCOMPARE(list[1], "Name=\"Map\"");
        QCOMPARE(list[2], "Left=\"-20c\"");
        QCOMPARE(list[3], "Top=\"0\"");
        QCOMPARE(list[4], "Width=\"20c\"");
        QCOMPARE(list[5], "Height=\"20c\"");
    }

    void testEndTag()
    {
        TMxpTagParser parser;

        MxpEndTag* endTag = parser.parseEndTag("</V>");
        QCOMPARE(endTag->getName(), "V");
        QVERIFY(endTag->isEndTag());
        QCOMPARE(endTag->asStartTag(), nullptr);
    }

    void testParseEndTag()
    {
        TMxpTagParser parser;

        MxpTag* tag = parser.parseTag("</V>");
        QCOMPARE(tag->getName(), "V");
        QVERIFY(tag->isEndTag());
        QVERIFY(tag->asEndTag());
        QVERIFY(!tag->asStartTag());
    }

    void testStartTagClosed()
    {
        TMxpTagParser parser;
        MxpTag* tag = parser.parseTag("<RNum 212 />");

        QVERIFY(tag->isStartTag());
        QVERIFY(tag->asStartTag()->isEmpty());
        QCOMPARE(tag->getName(), "RNum");
        QCOMPARE(tag->asStartTag()->getAttribute(0).getName(), "212");
        QCOMPARE(tag->asStartTag()->getAttributesCount(), 1);
    }

    void cleanupTestCase()
    {}
};

#include "TMxpTagParserTest.moc"
QTEST_MAIN(TMxpTagParserTest)
