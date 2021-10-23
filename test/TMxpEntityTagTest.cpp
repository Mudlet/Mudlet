/***************************************************************************
 *   Copyright (C) 2020 by Gustavo Sousa - gustavocms@gmail.com            *
 *   Copyright (C) 2021 by Florian Scheel - keneanung@gmail.com            *
 *   Copyright (C) 2020 by Michael Weller - michael.weller@t-online.de     *
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

#include <MxpTag.h>
#include <QTest>
#include <TMxpEntityTagHandler.h>
#include <TMxpStubClient.h>
#include <TMxpTagParser.h>
#include <TMxpTagProcessor.h>
#include <TEntityResolver.h>

class TMxpEntityTagTest : public QObject {
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

    void testEntityTagNew()
    {
        // set a new entity:

        TMxpStubContext ctx;
        TMxpStubClient stub;

        auto entityTag = parseNode("<!EN CDIR 'filename'>");

        TMxpEntityTagHandler entityTagHandler;
        TMxpTagHandler& tagHandler = entityTagHandler;
        tagHandler.handleTag(ctx, stub, entityTag->asStartTag());

        QCOMPARE(ctx.getEntityResolver().interpolate("cd &cdir;"), "cd filename");
    }

    void testEntityTagRedefine()
    {
        // redefine existing entity:

        TMxpStubContext ctx;
        TMxpStubClient stub;

        auto entityTag = parseNode("<!EN Atom 'org value'>");

        TMxpEntityTagHandler entityTagHandler;
        TMxpTagHandler& tagHandler = entityTagHandler;
        tagHandler.handleTag(ctx, stub, entityTag->asStartTag());

        QCOMPARE(ctx.getEntityResolver().interpolate("cd &ATOM;"), "cd org value");

        entityTag = parseNode("<!entity atom 'new value'>");
        tagHandler.handleTag(ctx, stub, entityTag->asStartTag());

        QCOMPARE(ctx.getEntityResolver().interpolate("cd &ATOM;"), "cd new value");
    }

    void testEntityTagEmpty()
    {
        // set an entity to empty string: Note that the direct call
        // to parseStartTag does not handle '' as in <!EN item ''> well.
        // In the real mudlet code '' is resolved to empty string in advance

        TMxpStubContext ctx;
        TMxpStubClient stub;

        auto entityTag = parseNode("<!en item torch>");

        TMxpEntityTagHandler entityTagHandler;
        TMxpTagHandler& tagHandler = entityTagHandler;
        tagHandler.handleTag(ctx, stub, entityTag->asStartTag());

        QCOMPARE(ctx.getEntityResolver().interpolate("examine &Item;"), "examine torch");

        entityTag = parseNode("<!Entity item>");
        tagHandler.handleTag(ctx, stub, entityTag->asStartTag());

        QCOMPARE(ctx.getEntityResolver().interpolate("examine &Item;"), "examine ");
    }

    void testEntityTagDelete()
    {
        // remove an existing entity:
        TMxpStubContext ctx;
        TMxpStubClient stub;

        auto entityTag = parseNode("<!EN weapon sword>");

        TMxpEntityTagHandler entityTagHandler;
        TMxpTagHandler& tagHandler = entityTagHandler;
        tagHandler.handleTag(ctx, stub, entityTag->asStartTag());

        QCOMPARE(ctx.getEntityResolver().interpolate("wield &weapon;"), "wield sword");

        entityTag = parseNode("<!Entity Weapon Delete>");
        tagHandler.handleTag(ctx, stub, entityTag->asStartTag());

        QCOMPARE(ctx.getEntityResolver().interpolate("wield &weapon;"), "wield &weapon;");
    }

    void cleanupTestCase()
    {}
};

#include "TMxpEntityTagTest.moc"
QTEST_MAIN(TMxpEntityTagTest)
