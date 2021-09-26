
#include "TMxpTagParser.h"
#include <MxpTag.h>
#include <QtTest/QtTest>

class TMxpTagParserTest : public QObject {
    Q_OBJECT

private:
private slots:

    void initTestCase()
    {
    }

    QSharedPointer<MxpNode> parseNode(const QString& tagText) const
    {
        auto nodes = TMxpTagParser::parseToMxpNodeList(tagText);
        return nodes.size() > 0 ? nodes.first() : nullptr;
    }

    void testMateriaMagicaScriptedActionEndTag()
    {
        auto node = parseNode("<play hangman/scripted_action>");
        QVERIFY(node->isEndTag());
        QCOMPARE(node->asEndTag()->getName(), "scripted_action");
    }

    void testMateriaMagicaScriptedActionTag()
    {
        TMxpTagParser parser;
        QString tagText = R"(<scripted_action desc="hangman start">play hangman<play hangman/scripted_action>)";

        auto list = parser.parseToMxpNodeList(tagText, false);
        QCOMPARE(list.size(), 3);

        QVERIFY(list[0]->isStartTag());
        QCOMPARE(list[0]->asStartTag()->getName(), "scripted_action");

        QVERIFY(!list[1]->isTag());
        QCOMPARE(list[1]->asText()->getContent(), "play hangman");

        QVERIFY(list[2]->isEndTag());
        QCOMPARE(list[2]->asEndTag()->getName(), "scripted_action");
    }

    void testSimpleEndTag()
    {
        TMxpTagParser parser;
        QString tagText = R"(<tag_name desc="blabla">tag content</tag_name>)";

        auto list = parser.parseToMxpNodeList(tagText, true);
        QCOMPARE(list.size(), 2);

        QVERIFY(list[0]->isStartTag());
        QVERIFY(list[1]->isEndTag());
        QCOMPARE(list[0]->asStartTag()->getName(), "tag_name");

        QVERIFY(list[1]->isEndTag());
        QCOMPARE(list[1]->asEndTag()->getName(), "tag_name");
    }

    void testSimpleTagQuotedSlash()
    {
        TMxpTagParser parser;
        QString tagText = R"(<tag_name desc="a/b">)";

        auto list = parser.parseToMxpNodeList(tagText, true);
        QCOMPARE(list.size(), 1);

        QVERIFY(list[0]->isStartTag());
        QCOMPARE(list[0]->asStartTag()->getName(), "tag_name");
        QCOMPARE(list[0]->asStartTag()->getAttributeValue("desc"), "a/b");
    }

    void testParseUrl()
    {
        QString tagText = R"(<A HREF="https://www.google.com/search?q=mudlet">)";
        auto node = parseNode(tagText);
        QVERIFY(node);

        QVERIFY(node->isStartTag());
        QCOMPARE(node->asStartTag()->getName(), "A");
        QCOMPARE(node->asStartTag()->getAttributeValue("HREF"), "https://www.google.com/search?q=mudlet");
    }

    void testParseWithEqualSymbol()
    {
        QString tagText = R"(<SEND HREF="1+1=2">)";
        auto node = parseNode(tagText);

        QVERIFY(node);
        QVERIFY(node->isStartTag());
        QCOMPARE(node->asStartTag()->getName(), "SEND");
        QCOMPARE(node->asStartTag()->getAttributeValue("HREF"), "1+1=2");
    }

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
        auto list = parser.parseToMxpNodeList(tagLine);

        QCOMPARE(list.size(), 1);
        QVERIFY(list[0]->isStartTag());

        MxpStartTag* tag = list[0]->asStartTag();

        QCOMPARE(tag->getName(), "!EL");
        QCOMPARE(tag->getAttributesCount(), 3);

        QCOMPARE(tag->getAttribute(0).getName(), "get");
        QCOMPARE(tag->getAttribute(0).getValue(), "");

        QCOMPARE(tag->getAttribute(1).getName(),
                 "<send href='examine &#34;&name;&#34;|get &#34;&name;&#34;' hint='Right mouse click to act on this item|Get &desc;|Examine &desc;|Look in &desc;' expire=get>");
        QCOMPARE(tag->getAttributeValue("ATT"), "name desc");

        auto sendTagNodeList = parser.parseToMxpNodeList(tag->getAttribute(1).getName());
        MxpStartTag* sendTag = sendTagNodeList[0]->asStartTag();
        QCOMPARE(sendTag->getName(), "send");
    }

    void testMxpTagParser()
    {
        TMxpTagParser parser;

        auto list = parser.parseToMxpNodeList("<!EL RExit FLAG=RoomExit>");
        QCOMPARE(list.size(), 1);
        QVERIFY(list[0]->isStartTag());

        MxpStartTag* tag = list[0]->asStartTag();

        QCOMPARE(tag->getName(), "!EL");

        QVERIFY(tag->hasAttribute("RExit"));
        QCOMPARE(tag->getAttributeValue("RExit"), "");
        QCOMPARE(tag->getAttribute(0).getName(), "RExit");
        QCOMPARE(tag->getAttribute(0).getValue(), "");

        QVERIFY(tag->hasAttribute("FLAG"));
        QCOMPARE(tag->getAttributeValue("FLAG"), "RoomExit");
    }

    void testAttrDefinition()
    {
        QString tagLine = R"(<!ATTLIST boldtext 'color=red background=white flags'>)";
        TMxpTagParser parser;
        auto list = parser.parseToMxpNodeList(tagLine);

        QCOMPARE(list.size(), 1);
        QVERIFY(list[0]->isStartTag());

        MxpStartTag* tag = list[0]->asStartTag();
        QCOMPARE(tag->getName(), "!ATTLIST");
        QCOMPARE(tag->getAttribute(0).getName(), "boldtext");
        QCOMPARE(tag->getAttribute(1).getName(), "color=red background=white flags");
    }

    void testSimpleElementDefinition()
    {
        QString tagLine = "<!EL RExit FLAG=RoomExit>";
        auto node = parseNode(tagLine);

        QVERIFY(node->isStartTag());
        QCOMPARE(node->asStartTag()->getName(), "!EL");
        QVERIFY(node->asStartTag()->hasAttribute("RExit"));
        QCOMPARE(node->asStartTag()->getAttributeValue("FLAG"), "RoomExit");
    }

    void testElementDefinitionWithExtraSpaces()
    {
        QString tagLine = "<!EL RExit   FLAG=RoomExit>";
        auto node = parseNode(tagLine);

        QVERIFY(node->isStartTag());
        QCOMPARE(node->asStartTag()->getName(), "!EL");
        QVERIFY(node->asStartTag()->hasAttribute("RExit"));
        QCOMPARE(node->asStartTag()->getAttributeValue("FLAG"), "RoomExit");
    }

    void testElementDefinitionQuotedAttributeSpaces()
    {
        QString tagLine = "<!EL sHp FLAG='Set Hp'>";
        auto node = parseNode(tagLine);

        QVERIFY(node->isStartTag());
        QCOMPARE(node->asStartTag()->getName(), "!EL");
        QVERIFY(node->asStartTag()->hasAttribute("sHp"));
        QCOMPARE(node->asStartTag()->getAttributeValue("FLAG"), "Set Hp");
    }

    void testAccessAttrCaseInsensitive()
    {
        TMxpTagParser parser;
        QString tagLine = "<!EL sHp FLAG='Set Hp'>";

        auto node = parseNode(tagLine);

        QVERIFY(node->isStartTag());
        MxpStartTag* tag = node->asStartTag();

        QVERIFY(tag->hasAttribute("sHp"));
        QVERIFY(tag->hasAttribute("shp"));
        QVERIFY(tag->hasAttribute("shP"));
        QCOMPARE(tag->getAttributeValue("flag"), "Set Hp");
    }

    void testElementDefinitionQuotesInQuotes()
    {
        QString tagLine = "<!EL x FLAG='Quote \" ex'>";
        auto node = parseNode(tagLine);

        QVERIFY(node->isStartTag());
        QCOMPARE(node->asStartTag()->getName(), "!EL");
        QVERIFY(node->asStartTag()->hasAttribute("x"));
        QCOMPARE(node->asStartTag()->getAttributeValue("FLAG"), "Quote \" ex");
    }

    void testCompleteElement()
    {
        TMxpTagParser parser;

        QString tagLine = R"(<FRAME Name="Map" Left="-20c" Top="0" Width="20c" Height="20c">)";
        auto node = parseNode(tagLine);

        QVERIFY(node->isStartTag());
        MxpStartTag* tag = node->asStartTag();

        QCOMPARE(tag->getName(), "FRAME");
        QCOMPARE(tag->getAttributeValue("Name"), "Map");
        QCOMPARE(tag->getAttributeValue("Left"), "-20c");
        QCOMPARE(tag->getAttributeValue("Top"), "0");
        QCOMPARE(tag->getAttributeValue("Width"), "20c");
        QCOMPARE(tag->getAttributeValue("Height"), "20c");
    }

    void testEndTag()
    {
        auto node = parseNode("</V>");
        QVERIFY(node->isEndTag());

        MxpEndTag* endTag = node->asEndTag();
        QCOMPARE(endTag->getName(), "V");
        QCOMPARE(endTag->asStartTag(), nullptr);
    }

    void testParseEndTag()
    {
        auto node = parseNode("</V>");
        QVERIFY(node);
        QVERIFY(node->isEndTag());

        MxpEndTag* tag = node->asEndTag();

        QCOMPARE(tag->getName(), "V");
        QVERIFY(!tag->asStartTag());
    }

    void testStartTagClosed()
    {
        auto node = parseNode("<RNum 212 />");

        QVERIFY(node);
        QVERIFY(node->isStartTag());

        MxpStartTag* tag = node->asStartTag();

        QVERIFY(tag->isEmpty());
        QCOMPARE(tag->getName(), "RNum");
        QCOMPARE(tag->getAttribute(0).getName(), "212");
        QCOMPARE(tag->getAttributesCount(), 1);
    }

    void testQuotedAttrValue()
    {
        auto node = parseNode("<color fore='red' back=\"blue\">");

        QVERIFY(node);
        QVERIFY(node->isStartTag());

        MxpStartTag* tag = node->asStartTag();

        QVERIFY(tag->isStartTag());
        QCOMPARE(tag->getName(), "color");
        QCOMPARE(tag->asStartTag()->getAttributesCount(), 2);
        QCOMPARE(tag->asStartTag()->getAttribute("fore").getValue(), "red");
        QCOMPARE(tag->asStartTag()->getAttribute("back").getValue(), "blue");
    }

    void cleanupTestCase() {}
};

#include "TMxpTagParserTest.moc"
QTEST_MAIN(TMxpTagParserTest)
