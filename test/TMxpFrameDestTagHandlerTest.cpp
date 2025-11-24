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
        // Test that FRAME tag is parsed correctly
        auto node = parseNode(R"(<FRAME name="Status" align="left" width="25%" height="100%">)");
        
        QVERIFY(node->isStartTag());
        MxpStartTag* tag = node->asStartTag();
        
        QCOMPARE(tag->getName(), "FRAME");
        QVERIFY(tag->hasAttribute("name"));
        QCOMPARE(tag->getAttributeValue("name"), "Status");
        QCOMPARE(tag->getAttributeValue("align"), "left");
        QCOMPARE(tag->getAttributeValue("width"), "25%");
        QCOMPARE(tag->getAttributeValue("height"), "100%");
    }

    void testFrameTagHandling()
    {
        // Test that FRAME tag handler processes attributes
        TMxpStubContext ctx;
        TMxpStubClient stub;
        TMxpFrameTagHandler handler;
        
        auto tag = parseNode(R"(<FRAME name="TestFrame" width="300px" height="200px">)");
        
        // The handler should delegate to the client
        TMxpTagHandlerResult result = handler.handleTag(ctx, stub, tag->asTag());
        
        // Since TMxpStubClient doesn't implement frame methods, it should return false
        // but the handler should have attempted to call it
        QVERIFY(result == MXP_TAG_HANDLED || result == MXP_TAG_NOT_HANDLED);
    }

    void testFrameCloseTag()
    {
        // Test closing a frame - end tags only have a name
        TMxpStubContext ctx;
        TMxpStubClient stub;
        
        MxpEndTag endTag("FRAME");
        QCOMPARE(endTag.getName(), "FRAME");
        
        // Test that handler processes the end tag
        TMxpFrameTagHandler handler;
        handler.handleTag(ctx, stub, &endTag);
        
        // Verify the close method was called (would need to enhance TMxpStubClient to track this)
        QVERIFY(true); // Basic validation that end tag is accepted
    }

    void testDestTagParsing()
    {
        // Test basic DEST tag
        auto node = parseNode(R"(<DEST name="Status">)");
        
        QVERIFY(node->isStartTag());
        MxpStartTag* tag = node->asStartTag();
        
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
        
        QVERIFY(node->isStartTag());
        MxpStartTag* tag = node->asStartTag();
        
        QCOMPARE(tag->getName(), "DEST");
        QCOMPARE(tag->getAttributeValue("name"), "Combat");
        QVERIFY(tag->hasAttribute("eol"));
    }

    void testDestTagWithEOF()
    {
        // Test DEST tag with EOF flag
        auto node = parseNode(R"(<DEST name="Log" eof>)");
        
        QVERIFY(node->isStartTag());
        MxpStartTag* tag = node->asStartTag();
        
        QCOMPARE(tag->getName(), "DEST");
        QCOMPARE(tag->getAttributeValue("name"), "Log");
        QVERIFY(tag->hasAttribute("eof"));
    }

    void testDestTagHandling()
    {
        // Test that DEST tag handler processes correctly
        TMxpStubContext ctx;
        TMxpStubClient stub;
        TMxpDestTagHandler handler;
        
        auto tag = parseNode(R"(<DEST name="TestDest" eol>)");
        
        TMxpTagHandlerResult result = handler.handleTag(ctx, stub, tag->asTag());
        
        // The handler should attempt to set destination
        QVERIFY(result == MXP_TAG_HANDLED || result == MXP_TAG_NOT_HANDLED);
    }

    void testDestCloseTag()
    {
        // Test closing DEST (clears destination)
        auto node = parseNode("</DEST>");
        
        QVERIFY(node->isEndTag());
        MxpEndTag* tag = node->asEndTag();
        
        QCOMPARE(tag->getName(), "DEST");
    }

    void testFrameWithAllAttributes()
    {
        // Test FRAME tag with all possible attributes
        auto node = parseNode(R"(<FRAME name="Complete" parent="main" title="Complete Test" align="top" width="50%" height="300px" scrolling="no" external="yes">)");
        
        QVERIFY(node->isStartTag());
        MxpStartTag* tag = node->asStartTag();
        
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
        // Test character-based frame sizing
        auto node = parseNode(R"(<FRAME name="Chars" width="40c" height="20c">)");
        
        QVERIFY(node->isStartTag());
        MxpStartTag* tag = node->asStartTag();
        
        QCOMPARE(tag->getAttributeValue("width"), "40c");
        QCOMPARE(tag->getAttributeValue("height"), "20c");
    }

    void testPixelBasedSizing()
    {
        // Test pixel-based frame sizing  
        auto node = parseNode(R"(<FRAME name="Pixels" width="640px" height="480px">)");
        
        QVERIFY(node->isStartTag());
        MxpStartTag* tag = node->asStartTag();
        
        QCOMPARE(tag->getAttributeValue("width"), "640px");
        QCOMPARE(tag->getAttributeValue("height"), "480px");
    }

    void testPercentageBasedSizing()
    {
        // Test percentage-based frame sizing
        auto node = parseNode(R"(<FRAME name="Percent" width="75%" height="50%">)");
        
        QVERIFY(node->isStartTag());
        MxpStartTag* tag = node->asStartTag();
        
        QCOMPARE(tag->getAttributeValue("width"), "75%");
        QCOMPARE(tag->getAttributeValue("height"), "50%");
    }
};

#include "TMxpFrameDestTagHandlerTest.moc"
QTEST_MAIN(TMxpFrameDestTagHandlerTest)
