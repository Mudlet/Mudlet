/***************************************************************************
 *   Copyright (C) 2020 by Gustavo Sousa - gustavocms@gmail.com            *
 *   Copyright (C) 2020 by Stephen Lyons - slysven@virginmedia.com         *
 *   Copyright (C) 2023 by Michael Weller - michael.weller@t-online.de     *
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
#include "TMxpFormattingTagsHandler.h"
#include "TMxpStubClient.h"
#include <TMxpTagParser.h>
#include <TMxpTagProcessor.h>
#include <TMxpProcessor.h>




class TMxpFormattingTagsTest : public QObject {
    Q_OBJECT

private:
private slots:
    QSharedPointer<MxpNode> parseNode(const QString& tagText) const
    {
        auto nodes = TMxpTagParser::parseToMxpNodeList(tagText);
        return nodes.size() > 0 ? nodes.first() : nullptr;
    }

    void initTestCase()
    {}

    void testSimpleFormatting()
    {

        TMxpStubContext ctx;
        TMxpStubClient stub;      
        TMxpFormattingTagsHandler formattingTagsHandler;
        TMxpTagHandler& tagHandler = formattingTagsHandler;

        // Check general functionality:
        tagHandler.handleTag(ctx, stub, parseNode("<I>")->asTag());
        QVERIFY(stub.italic());
        tagHandler.handleTag(ctx, stub, parseNode("</I>")->asTag());
        QVERIFY(!stub.italic());

        tagHandler.handleTag(ctx, stub, parseNode("<B>")->asTag());
        QVERIFY(stub.bold());
        tagHandler.handleTag(ctx, stub, parseNode("</B>")->asTag());
        QVERIFY(!stub.bold());

        tagHandler.handleTag(ctx, stub, parseNode("<S>")->asTag());
        QVERIFY(stub.strikeOut());
        tagHandler.handleTag(ctx, stub, parseNode("</S>")->asTag());
        QVERIFY(!stub.strikeOut());

        tagHandler.handleTag(ctx, stub, parseNode("<U>")->asTag());
        QVERIFY(stub.underline());
        tagHandler.handleTag(ctx, stub, parseNode("</U>")->asTag());
        QVERIFY(!stub.underline());
    }

    void testNestedFormatting()
    {

        TMxpStubContext ctx;
        TMxpStubClient stub;
        TMxpFormattingTagsHandler formattingTagsHandler;
        TMxpTagHandler& tagHandler = formattingTagsHandler;

        // Now we mix and match, and also use some aliases:
        tagHandler.handleTag(ctx, stub, parseNode("<I>")->asTag());
        QVERIFY(stub.italic());
        tagHandler.handleTag(ctx, stub, parseNode("<STRONG>")->asTag());
        QVERIFY(stub.italic() && stub.bold());

        tagHandler.handleTag(ctx, stub, parseNode("<EM>")->asTag());
        tagHandler.handleTag(ctx, stub, parseNode("<S>")->asTag());
        QVERIFY(stub.italic() && stub.bold() && stub.strikeOut());

        tagHandler.handleTag(ctx, stub, parseNode("</S>")->asTag());
        QVERIFY(stub.italic() && stub.bold() && !stub.strikeOut());

        tagHandler.handleTag(ctx, stub, parseNode("<UNDERLINE>")->asTag());
        QVERIFY(stub.italic() && stub.bold() && stub.underline());

        tagHandler.handleTag(ctx, stub, parseNode("</UNDERLINE>")->asTag());
        QVERIFY(stub.italic() && stub.bold() && !stub.underline());

        // Closing EM (an alias for I) should still stay italics, as we opened two of them
        tagHandler.handleTag(ctx, stub, parseNode("</EM>")->asTag());
        QVERIFY(stub.italic() && stub.bold() && !stub.underline());

        tagHandler.handleTag(ctx, stub, parseNode("</STRONG>")->asTag());
        QVERIFY(stub.italic() && !stub.bold() && !stub.underline());

        tagHandler.handleTag(ctx, stub, parseNode("</I>")->asTag());
        QVERIFY(!stub.italic() && !stub.bold() && !stub.underline());

        // Behaviour when closing unopened tags, or closing out of order is undefined, but should be handled gracefully
        // and not crash (current implementation: tags can be closed out of order, if unopened they are ignored)
        tagHandler.handleTag(ctx, stub, parseNode("</BOLD>")->asTag());
        QVERIFY(!stub.italic() && !stub.bold() && !stub.underline() && !stub.strikeOut());

        tagHandler.handleTag(ctx, stub, parseNode("<BOLD>")->asTag());
        QVERIFY(!stub.italic() && stub.bold() && !stub.underline() && !stub.strikeOut());

        tagHandler.handleTag(ctx, stub, parseNode("</BOLD>")->asTag());
        QVERIFY(!stub.italic() && !stub.bold() && !stub.underline() && !stub.strikeOut());
    }

    void cleanupTestCase()
    {}
};

#include "TMxpFormattingTagsTest.moc"
QTEST_MAIN(TMxpFormattingTagsTest)
