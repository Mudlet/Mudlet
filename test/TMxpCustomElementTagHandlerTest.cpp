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

#include <QTest>
#include <TMxpSendTagHandler.h>
#include <TMxpElementDefinitionHandler.h>
#include <TMxpCustomElementTagHandler.h>
#include <TMxpFormattingTagsHandler.h>
#include <TMxpColorTagHandler.h>
#include <TMxpTagParser.h>
#include <TMxpTagProcessor.h>

#include "TMxpStubClient.h"

/*
 * One can certainly argue if this should be in a separate module, but
 * I want to include only the handlers needed by this file into the module
 * for a sensible link time and executable size
 */

class TMxpStubHandlerContext : public TMxpStubContext {
    TMxpSendTagHandler sendTagHandler;
    TMxpFormattingTagsHandler formattingTagsHandler;
    TMxpColorTagHandler colorTagHandler;

public:
    virtual TMxpTagHandlerResult handleTag(TMxpContext& ctx, TMxpClient& client, MxpTag* tag)
    {
        TMxpTagHandler *tagHandler;

        if (sendTagHandler.supports(ctx, client, tag)) {
            tagHandler = &sendTagHandler;
        } else if (formattingTagsHandler.supports(ctx, client, tag)) {
            tagHandler = &formattingTagsHandler;
        } else if (colorTagHandler.supports(ctx, client, tag)) {
            tagHandler = &colorTagHandler;
        } else {
            qDebug() << QString("unhandled Tag: [%1%2]").arg(tag->isEndTag() ? "/" : "").arg(tag->getName());
            return MXP_TAG_HANDLED;
        }
        qDebug() << QString("handleTag([%1%2])").arg(tag->isEndTag() ? "/" : "").arg(tag->getName());
        return tag->isStartTag() ? tagHandler->handleStartTag(ctx, client, tag->asStartTag()) : tagHandler->handleEndTag(ctx, client, tag->asEndTag());
    }
};

class TMxpCustomElementTagHandlerTest : public QObject {
Q_OBJECT

private:

private slots:

    QSharedPointer<MxpNode> parseNode(const QString& tagText) const
    {
        auto nodes = TMxpTagParser::parseToMxpNodeList(tagText);
        return nodes.size() > 0 ? nodes.first() : nullptr;
    }

    void testCustomItemCmd() {
        // Complex Example: Aldebaran defines a profile with definitions like this:
        // <!EL ITI '<SEND "examine &ID;|drop &ID;" HINT="&CMDH;examine|drop">' ATT='ID'>
        // Then uses <ITI inv##N>a rock</ITI> for brevity all over the mud (here plain ITem in Inventory)
        // Note the use of positional parameter ID (first arg)
        // Mudlet, up to version 4.10, made the hints ALL UPPER in this context

        TMxpStubHandlerContext ctx;
        TMxpStubClient stub;

        ctx.getEntityResolver().registerEntity("&CMDH;", "Right-click for options|");

        auto defTag = parseNode(R"(<!EL ITI '<SEND "examine &ID;|drop &ID;" HINT="&CMDH;examine|drop">' ATT='ID'>)");

        TMxpElementDefinitionHandler definitionHandler;
        definitionHandler.handleTag(ctx, stub, defTag->asStartTag());

        auto startTag = parseNode(R"(<ITI inv##10>)");
        auto endTag = parseNode("</ITI>");

        TMxpCustomElementTagHandler customElementTagHandler;
        TMxpTagHandler& tagHandler = customElementTagHandler;

        tagHandler.handleTag(ctx, stub, startTag->asStartTag());
        tagHandler.handleContent("a rock");
        tagHandler.handleTag(ctx, stub, endTag->asEndTag());

        QCOMPARE(stub.mHrefs.size(), 2);
        QCOMPARE(stub.mHrefs[0], "send([[examine inv##10]])");
        QCOMPARE(stub.mHrefs[1], "send([[drop inv##10]])");

        QCOMPARE(stub.mHints.size(), 3);
        // previous Mudlet made these ALL UPPER:
        QCOMPARE(stub.mHints[0], "Right-click for options");
        QCOMPARE(stub.mHints[1], "examine");
        QCOMPARE(stub.mHints[2], "drop");
    }

    void testCustomElementDynamicEntity() {
        // Complex Example: Aldebaran has a global custom element MAP used in maps printed for
        // the player to click onto it and then follow the map.
        // However, it uses the id of the map currently looked at in an entity redefined for
        // each map you look at.
        // <!EL ITI '<SEND "examine &ID;|drop &ID;" HINT="&CMDH;examine|drop">' ATT='ID'>
        // Then uses <ITI inv##N>a rock</ITI> for brevity all over the mud (here plain ITem in Inventory)
        // Note the use of positional parameter ID (first arg)
        // Mudlet, up to version 4.10, made the hints ALL UPPER in this context

        TMxpStubHandlerContext ctx;
        TMxpStubClient stub;

        ctx.getEntityResolver().registerEntity("&MID;", "map of the newbie jungle");

        auto defTag = parseNode(R"(<!EL MAP '<SEND "follow &MID; to &ID;" HINT="go here">' ATT='ID'>)");

        TMxpElementDefinitionHandler definitionHandler;
        definitionHandler.handleTag(ctx, stub, defTag->asStartTag());

        auto startTag = parseNode(R"(<MAP P8x7>)");
        auto endTag = parseNode("</MAP>");

        TMxpCustomElementTagHandler customElementTagHandler;
        TMxpTagHandler& tagHandler = customElementTagHandler;

        tagHandler.handleTag(ctx, stub, startTag->asStartTag());
        tagHandler.handleContent("*");
        tagHandler.handleTag(ctx, stub, endTag->asEndTag());

        QCOMPARE(stub.mHrefs.size(), 1);
        QCOMPARE(stub.mHrefs[0], "send([[follow map of the newbie jungle to P8x7]])");

        QCOMPARE(stub.mHints.size(), 1);
        QCOMPARE(stub.mHints[0], "go here");

        // Now player looks at another map and gets an MXP button to go to another place.
        // Note the MAP element is NOT redefined, only the entity.
        ctx.getEntityResolver().registerEntity("&MID;", "map of the south forest");
        startTag = parseNode(R"(<MAP P42>)");
        endTag = parseNode("</MAP>");

        tagHandler.handleTag(ctx, stub, startTag->asStartTag());
        tagHandler.handleContent("*");
        tagHandler.handleTag(ctx, stub, endTag->asEndTag());

        QCOMPARE(stub.mHrefs.size(), 1);
        QCOMPARE(stub.mHrefs[0], "send([[follow map of the south forest to P42]])");

        QCOMPARE(stub.mHints.size(), 1);
        QCOMPARE(stub.mHints[0], "go here");
    }

    void testCustomElementDefaultAttributes() {
        // This example literally taken from the MXP definition at https://www.zuggsoft.com/zmud/mxp.htm#ELEMENT
        //
        // <!ELEMENT boldtext '<COLOR &col;><B>' ATT='col=red'>
        //
        // Then you could use it on the MUD like this:
        //
        // <boldtext>This is bold red</boldtext>
        // <boldtext col=blue>This is bold blue text</boldtext>
        // <boldtext blue>This is also bold blue text</boldtext>

        TMxpStubHandlerContext ctx;
        TMxpStubClient stub;

        auto defTag = parseNode(R"(<!ELEMENT boldtext '<COLOR &col;><B>' ATT='col=red'>)");

        TMxpElementDefinitionHandler definitionHandler;
        definitionHandler.handleTag(ctx, stub, defTag->asStartTag());

        auto startTag = parseNode(R"(<boldtext>)");
        auto endTag = parseNode("</boldtext>");

        TMxpCustomElementTagHandler customElementTagHandler;
        TMxpTagHandler& tagHandler = customElementTagHandler;

        tagHandler.handleTag(ctx, stub, startTag->asStartTag());
        tagHandler.handleContent("This is bold red");
        // is it?
        QCOMPARE(stub.isBold(), true);
        QCOMPARE(stub.fgColor, "red");
        tagHandler.handleTag(ctx, stub, endTag->asEndTag());

        // back to defaults:
        QCOMPARE(stub.isBold(), false);
        QCOMPARE(stub.fgColor, "");

        startTag = parseNode(R"(<boldtext COL=blue>)");
        tagHandler.handleTag(ctx, stub, startTag->asStartTag());
        tagHandler.handleContent("This is bold blue text");
        // is it?
        QCOMPARE(stub.isBold(), true);
        QCOMPARE(stub.fgColor, "blue");
        tagHandler.handleTag(ctx, stub, endTag->asEndTag());

        // back to defaults:
        QCOMPARE(stub.isBold(), false);
        QCOMPARE(stub.fgColor, "");

        startTag = parseNode(R"(<boldtext blue>)");
        tagHandler.handleTag(ctx, stub, startTag->asStartTag());
        tagHandler.handleContent("This is also bold blue text");
        // is it?

        QCOMPARE(stub.isBold(), true);
        QCOMPARE(stub.fgColor, "blue");
        tagHandler.handleTag(ctx, stub, endTag->asEndTag());

        // back to defaults:
        QCOMPARE(stub.isBold(), false);
        QCOMPARE(stub.fgColor, "");
    }

    void testCustomElementPlayer() {
        // Real life example from Aldebaran: (there are more sensible ones with EXPIRE which Mudlet does not yet support)
        //
        // <!EL WH '<SEND "whisper &NAME; |finger &NAME; |tell &NAME; " HINT="whisper &NAME;|finger &NAME;|tell &NAME;" PROMPT>' ATT='NAME=someone'>
        //
        // Used like <WH playerid>Player</WH> says: Hello!
        // However, if player is invisible, playerid is empty, like <WH >Someone</WH> says: Hello!
        //
        // Upper and lower case are mixed in the entity name to make this a more severe test case

        TMxpStubHandlerContext ctx;
        TMxpStubClient stub;

        auto defTag = parseNode(R"(<!EL WH '<SEND "whisper &Name; |finger &NAme; |tell &namE; " HINT="whisper &name;|finger &NAME;|tell &NAME;" PROMPT>' ATT='NAme=someone'>)");

        TMxpElementDefinitionHandler definitionHandler;
        definitionHandler.handleTag(ctx, stub, defTag->asStartTag());

        auto startTag = parseNode(R"(<WH playerid>)");
        auto endTag = parseNode("</WH>");

        TMxpCustomElementTagHandler customElementTagHandler;
        TMxpTagHandler& tagHandler = customElementTagHandler;

        tagHandler.handleTag(ctx, stub, startTag->asStartTag());
        tagHandler.handleContent("Player");
        tagHandler.handleTag(ctx, stub, endTag->asEndTag());

        QCOMPARE(stub.mHrefs.size(), 3);
        QCOMPARE(stub.mHrefs[0], "printCmdLine([[whisper playerid ]])");
        QCOMPARE(stub.mHrefs[1], "printCmdLine([[finger playerid ]])");
        QCOMPARE(stub.mHrefs[2], "printCmdLine([[tell playerid ]])");

        QCOMPARE(stub.mHints.size(), 4);
        // default mouse over text for send menu is implementation dependent, thus not checked.
        QCOMPARE(stub.mHints[1], "whisper playerid");
        QCOMPARE(stub.mHints[2], "finger playerid");
        QCOMPARE(stub.mHints[3], "tell playerid");

        // Now w/o a NAME parameter given:
        startTag = parseNode(R"(<WH>)");
        tagHandler.handleTag(ctx, stub, startTag->asStartTag());
        tagHandler.handleContent("Invisible SuperAdmin");
        tagHandler.handleTag(ctx, stub, endTag->asEndTag());

        QCOMPARE(stub.mHrefs.size(), 3);
        QCOMPARE(stub.mHrefs[0], "printCmdLine([[whisper someone ]])");
        QCOMPARE(stub.mHrefs[1], "printCmdLine([[finger someone ]])");
        QCOMPARE(stub.mHrefs[2], "printCmdLine([[tell someone ]])");

        QCOMPARE(stub.mHints.size(), 4);
        // default mouse over text for send menu is implementation dependent, thus not checked.
        QCOMPARE(stub.mHints[1], "whisper someone");
        QCOMPARE(stub.mHints[2], "finger someone");
        QCOMPARE(stub.mHints[3], "tell someone");
    }
};

#include "TMxpCustomElementTagHandlerTest.moc"
QTEST_MAIN(TMxpCustomElementTagHandlerTest)

