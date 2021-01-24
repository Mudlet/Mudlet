/***************************************************************************
 *   Copyright (C) 2020 by Gustavo Sousa - gustavocms@gmail.com            *
 *   Copyright (C) 2020 by Stephen Lyons - slysven@virginmedia.com         *
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
#include <TMxpTagParser.h>
#include <TMxpTagProcessor.h>
#include <TEntityResolver.h>

#include "TMxpStubClient.h"



class TMxpEntityTagTest : public QObject {
    Q_OBJECT

private:
private slots:

    void initTestCase()
    {}

    void testEntityTagNew()
    {
        // set a new entity:

        TMxpTagParser parser;
        TMxpStubContext ctx;
        TMxpStubClient stub;

        MxpStartTag* entityTag = parser.parseStartTag("<!EN CDIR 'filename'>");

        TMxpEntityTagHandler entityTagHandler;
        TMxpTagHandler& tagHandler = entityTagHandler;
        tagHandler.handleTag(ctx, stub, entityTag);

        QCOMPARE(ctx.getEntityResolver().interpolate("cd &cdir;"), "cd filename");
    }

    void testEntityTagRedefine()
    {
        // redefine existing entity:

        TMxpTagParser parser;
        TMxpStubContext ctx;
        TMxpStubClient stub;

        MxpStartTag* entityTag = parser.parseStartTag("<!EN Atom 'org value'>");

        TMxpEntityTagHandler entityTagHandler;
        TMxpTagHandler& tagHandler = entityTagHandler;
        tagHandler.handleTag(ctx, stub, entityTag);

        QCOMPARE(ctx.getEntityResolver().interpolate("cd &ATOM;"), "cd org value");

        entityTag = parser.parseStartTag("<!entity atom 'new value'>");
        tagHandler.handleTag(ctx, stub, entityTag);

        QCOMPARE(ctx.getEntityResolver().interpolate("cd &ATOM;"), "cd new value");
    }

    void testEntityTagEmpty()
    {
        // set an entity to empty string: Note that the direct call
        // to parseStartTag does not handle '' as in <!EN item ''> well.
        // In the real mudlet code '' is resolved to empty string in advance

        TMxpTagParser parser;
        TMxpStubContext ctx;
        TMxpStubClient stub;

        MxpStartTag* entityTag = parser.parseStartTag("<!en item torch>");

        TMxpEntityTagHandler entityTagHandler;
        TMxpTagHandler& tagHandler = entityTagHandler;
        tagHandler.handleTag(ctx, stub, entityTag);

        QCOMPARE(ctx.getEntityResolver().interpolate("examine &Item;"), "examine torch");

        entityTag = parser.parseStartTag("<!Entity item>");
        tagHandler.handleTag(ctx, stub, entityTag);

        QCOMPARE(ctx.getEntityResolver().interpolate("examine &Item;"), "examine ");
    }

    void testEntityTagDelete()
    {
        // remove an existing entity:

        TMxpTagParser parser;
        TMxpStubContext ctx;
        TMxpStubClient stub;

        MxpStartTag* entityTag = parser.parseStartTag("<!EN weapon sword>");

        TMxpEntityTagHandler entityTagHandler;
        TMxpTagHandler& tagHandler = entityTagHandler;
        tagHandler.handleTag(ctx, stub, entityTag);

        QCOMPARE(ctx.getEntityResolver().interpolate("wield &weapon;"), "wield sword");

        entityTag = parser.parseStartTag("<!Entity Weapon Delete>");
        tagHandler.handleTag(ctx, stub, entityTag);

        QCOMPARE(ctx.getEntityResolver().interpolate("wield &weapon;"), "wield &weapon;");
    }

    void cleanupTestCase()
    {}
};

#include "TMxpEntityTagTest.moc"
QTEST_MAIN(TMxpEntityTagTest)
