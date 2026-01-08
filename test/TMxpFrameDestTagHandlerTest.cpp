/***************************************************************************
 *   Copyright (C) 2025 by Mike Conley - mike.conley@stickmud.com          *
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

#include <QTest>
#include "TMxpFrameTagHandler.h"
#include "TMxpDestTagHandler.h"
#include "TMxpStubClient.h"
#include <TMxpTagParser.h>
#include <TMxpTagProcessor.h>
#include <TMxpProcessor.h>

class TMxpFrameDestTagHandlerTest : public QObject {
    Q_OBJECT

private slots:
    static QSharedPointer<MxpNode> parseNode(const QString& tagText)
    {
        auto nodes = TMxpTagParser::parseToMxpNodeList(tagText);
        return !nodes.empty() ? nodes.first() : nullptr;
    }

    void testFrameTagParsing()
    {
        auto node = parseNode(R"(<FRAME name="Status" align="left" width="25%" height="100%">)");
        QVERIFY(node != nullptr);
        
        QVERIFY(node->isStartTag());
        MxpStartTag* tag = node->asStartTag();
        QVERIFY(tag != nullptr);
        
        QCOMPARE(tag->getName(), "FRAME");
        QVERIFY(tag->hasAttribute("name"));
        QCOMPARE(tag->getAttributeValue("name"), "Status");
        QCOMPARE(tag->getAttributeValue("align"), "left");
        QCOMPARE(tag->getAttributeValue("width"), "25%");
        QCOMPARE(tag->getAttributeValue("height"), "100%");
    }

    void testFrameTagHandling()
    {
        TMxpStubContext ctx;
        TMxpStubClient stub;
        TMxpFrameTagHandler handler;
        
        auto tag = parseNode(R"(<FRAME name="TestFrame" width="300px" height="200px">)");
        QVERIFY(tag != nullptr);
        QVERIFY(tag->asTag() != nullptr);
        
        TMxpTagHandlerResult result = handler.handleTag(ctx, stub, tag->asTag());
        
        QVERIFY(stub.createMxpFrameCalled);
        QCOMPARE(stub.lastCreatedFrameName, qsl("TestFrame"));
        QCOMPARE(stub.lastFrameAttributes.value(qsl("WIDTH")), qsl("300px"));
        QCOMPARE(stub.lastFrameAttributes.value(qsl("HEIGHT")), qsl("200px"));
        
        // Stub returns false, so handler returns NOT_HANDLED
        QCOMPARE(result, MXP_TAG_NOT_HANDLED);
    }

    void testFrameCloseTag()
    {
        TMxpStubContext ctx;
        TMxpStubClient stub;
        
        MxpEndTag endTag("FRAME");
        QCOMPARE(endTag.getName(), "FRAME");
        
        TMxpFrameTagHandler handler;
        TMxpTagHandlerResult result = handler.handleTag(ctx, stub, &endTag);
        
        // FRAME has no handleEndTag override
        QCOMPARE(result, MXP_TAG_NOT_HANDLED);
    }

    void testDestTagParsing()
    {
        auto node = parseNode(R"(<DEST name="Status">)");
        QVERIFY(node != nullptr);
        
        QVERIFY(node->isStartTag());
        MxpStartTag* tag = node->asStartTag();
        QVERIFY(tag != nullptr);
        
        QCOMPARE(tag->getName(), "DEST");
        QVERIFY(tag->hasAttribute("name"));
        QCOMPARE(tag->getAttributeValue("name"), "Status");
        QVERIFY(!tag->hasAttribute("eol"));
        QVERIFY(!tag->hasAttribute("eof"));
    }

    void testDestTagWithEOL()
    {
        // Test DEST tag with EOL flag
        auto node = parseNode(R"(<DEST name="Combat" eol>)");
        QVERIFY(node != nullptr);
        
        QVERIFY(node->isStartTag());
        MxpStartTag* tag = node->asStartTag();
        QVERIFY(tag != nullptr);
        
        QCOMPARE(tag->getName(), "DEST");
        QCOMPARE(tag->getAttributeValue("name"), "Combat");
        QVERIFY(tag->hasAttribute("eol"));
    }

    void testDestTagWithEOF()
    {
        auto node = parseNode(R"(<DEST name="Log" eof>)");
        QVERIFY(node != nullptr);
        
        QVERIFY(node->isStartTag());
        MxpStartTag* tag = node->asStartTag();
        QVERIFY(tag != nullptr);
        
        QCOMPARE(tag->getName(), "DEST");
        QCOMPARE(tag->getAttributeValue("name"), "Log");
        QVERIFY(tag->hasAttribute("eof"));
    }

    void testDestTagHandling()
    {
        TMxpStubContext ctx;
        TMxpStubClient stub;
        TMxpDestTagHandler handler;
        
        auto tag = parseNode(R"(<DEST name="TestDest" eol>)");
        QVERIFY(tag != nullptr);
        QVERIFY(tag->asTag() != nullptr);
        
        TMxpTagHandlerResult result = handler.handleTag(ctx, stub, tag->asTag());
        
        QVERIFY(stub.setMxpDestinationCalled);
        QCOMPARE(stub.lastDestinationName, qsl("TestDest"));
        QVERIFY(stub.lastDestinationEol);
        QVERIFY(!stub.lastDestinationEof);
        
        // Stub returns false, so handler returns NOT_HANDLED
        QCOMPARE(result, MXP_TAG_NOT_HANDLED);
    }

    void testDestCloseTag()
    {
        TMxpStubContext ctx;
        TMxpStubClient stub;
        TMxpDestTagHandler handler;
        
        auto node = parseNode("</DEST>");
        QVERIFY(node != nullptr);
        
        QVERIFY(node->isEndTag());
        MxpEndTag* tag = node->asEndTag();
        QVERIFY(tag != nullptr);
        
        QCOMPARE(tag->getName(), "DEST");
        
        TMxpTagHandlerResult result = handler.handleTag(ctx, stub, tag);
        
        QVERIFY(stub.clearMxpDestinationCalled);
        QCOMPARE(result, MXP_TAG_HANDLED);
    }

    void testFrameWithAllAttributes()
    {
        auto node = parseNode(R"(<FRAME name="Complete" parent="main" title="Complete Test" align="top" width="50%" height="300px" scrolling="no" external="yes">)");
        QVERIFY(node != nullptr);
        
        QVERIFY(node->isStartTag());
        MxpStartTag* tag = node->asStartTag();
        QVERIFY(tag != nullptr);
        
        QCOMPARE(tag->getName(), "FRAME");
        QCOMPARE(tag->getAttributeValue("name"), "Complete");
        QCOMPARE(tag->getAttributeValue("parent"), "main");
        QCOMPARE(tag->getAttributeValue("title"), "Complete Test");
        QCOMPARE(tag->getAttributeValue("align"), "top");
        QCOMPARE(tag->getAttributeValue("width"), "50%");
        QCOMPARE(tag->getAttributeValue("height"), "300px");
        QCOMPARE(tag->getAttributeValue("scrolling"), "no");
        QCOMPARE(tag->getAttributeValue("external"), "yes");
    }

    void testCharacterBasedSizing()
    {
        auto node = parseNode(R"(<FRAME name="Chars" width="40c" height="20c">)");
        QVERIFY(node != nullptr);
        
        QVERIFY(node->isStartTag());
        MxpStartTag* tag = node->asStartTag();
        QVERIFY(tag != nullptr);
        
        QCOMPARE(tag->getAttributeValue("width"), "40c");
        QCOMPARE(tag->getAttributeValue("height"), "20c");
    }

    void testPixelBasedSizing()
    {
        auto node = parseNode(R"(<FRAME name="Pixels" width="640px" height="480px">)");
        QVERIFY(node != nullptr);
        
        QVERIFY(node->isStartTag());
        MxpStartTag* tag = node->asStartTag();
        QVERIFY(tag != nullptr);
        
        QCOMPARE(tag->getAttributeValue("width"), "640px");
        QCOMPARE(tag->getAttributeValue("height"), "480px");
    }

    void testPercentageBasedSizing()
    {
        auto node = parseNode(R"(<FRAME name="Percent" width="75%" height="50%">)");
        QVERIFY(node != nullptr);
        
        QVERIFY(node->isStartTag());
        MxpStartTag* tag = node->asStartTag();
        QVERIFY(tag != nullptr);
        
        QCOMPARE(tag->getAttributeValue("width"), "75%");
        QCOMPARE(tag->getAttributeValue("height"), "50%");
    }
};

#include "TMxpFrameDestTagHandlerTest.moc"
QTEST_MAIN(TMxpFrameDestTagHandlerTest)
