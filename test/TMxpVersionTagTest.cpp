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
#include <TMxpVersionTagHandler.h>
#include <TMxpTagParser.h>
#include <TMxpTagProcessor.h>
#include <TEntityResolver.h>

#include "TMxpStubClient.h"



class TMxpVersionTagTest : public QObject {
    Q_OBJECT

private:
private slots:

    void initTestCase()
    {}

    QSharedPointer<MxpNode> parseNode(const QString& tagText) const
    {
        auto nodes = TMxpTagParser::parseToMxpNodeList(tagText);
        return nodes.size() > 0 ? nodes.first() : nullptr;
    }

    void testVersionTag()
    {
        // Vanilla use of version

        TMxpStubContext ctx;
        TMxpStubClient stub;

        auto versionTag = parseNode("<Version>");

        TMxpVersionTagHandler versionTagHandler;
        TMxpTagHandler& tagHandler = versionTagHandler;
        tagHandler.handleTag(ctx, stub, versionTag->asStartTag());

        QCOMPARE(stub.sentToServer, "\n\u001B[1z<VERSION MXP=1.0 CLIENT=Mudlet VERSION=Stub-1.0>\n");
    }

     void testVersionStyle()
    {
        // Set a Style (Version of MXP Template of mud)

        TMxpStubContext ctx;
        TMxpStubClient stub;

        // style is generally just a string, but compared to other implementations we must at least allow for full 9 digit
        // (which always fit into a 32bit int)
        auto versionTag = parseNode("<Version 987654321>");

        TMxpVersionTagHandler versionTagHandler;
        TMxpTagHandler& tagHandler = versionTagHandler;
        tagHandler.handleTag(ctx, stub, versionTag->asStartTag());

        // NO return value when setting style.
        // This is a grey area.. but Z/CMUD do it this way and the MXP definition seems to imply this interpretation:
        // The client caches this version information and returns it when requested by a plain <VERSION> request.
        QCOMPARE(stub.sentToServer, "");

        // From now on, return it with version
        versionTag = parseNode("<VERSION>");
        tagHandler.handleTag(ctx, stub, versionTag->asStartTag());
        QCOMPARE(stub.sentToServer, "\n\u001B[1z<VERSION MXP=1.0 CLIENT=Mudlet VERSION=Stub-1.0 STYLE=987654321>\n");
    }

    void cleanupTestCase()
    {}
};

#include "TMxpVersionTagTest.moc"
QTEST_MAIN(TMxpVersionTagTest)
