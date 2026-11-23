/***************************************************************************
 *   Copyright (C) 2026 by Vadim Peretokin - vadim.peretokin@mudlet.org    *
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

/*
 * Where the event names the script editor offers come from: the hand-kept list
 * of the ones Mudlet raises, and the reading of a piece of Lua for the ones it
 * names itself.
 *
 * Run with: ctest -R EventNamesTest -V
 */

#include <QSet>
#include <QtTest/QtTest>

#include "EventNames.h"
#include "utils.h"

#include "GroupedTest.h"

class EventNamesTest : public QObject
{
    Q_OBJECT

private:
    // A floor well under today's 89 names, so adding an event never fails a run
    static constexpr int scmLeastSystemEvents = 70;

private slots:
    void test_theNamesInAPieceOfLuaAreRead()
    {
        const QString code = qsl("raiseEvent(\"a\", 1)\n"
                                 "raiseEvent('b')\n"
                                 "raiseGlobalEvent(\"g\")\n"
                                 "registerAnonymousEventHandler(\"c\", myHandler)\n"
                                 "registerNamedEventHandler(\"u\", \"h\", \"d\", myHandler)\n"
                                 "raiseEvent(someVariable)\n"
                                 "raiseEvent(\"a\", 2)\n");

        const QStringList found = eventNames::namesInCode(code);

        QCOMPARE(found, QStringList({qsl("a"), qsl("b"), qsl("g"), qsl("c"), qsl("d")}));
        QVERIFY2(!found.contains(qsl("someVariable")), "a name the code works out at run time was read as a literal");
    }

    // An unclosed quote stops at its line end, so it costs that call rather
    // than the rest of the file
    void test_aQuoteOnlyEndsItsOwnKind()
    {
        QCOMPARE(eventNames::namesInCode(qsl("raiseEvent(\"it's here\")")), QStringList{qsl("it's here")});
        QCOMPARE(eventNames::namesInCode(qsl("raiseEvent(\"unclosed\nraiseEvent(\"after\")")), QStringList{qsl("after")});
    }

    void test_theGamesNamesAreKnownByTheirShape()
    {
        QVERIFY2(eventNames::fromGame(qsl("gmcp.Char.Vitals")), "a GMCP key was not read as the game's");
        QVERIFY2(eventNames::fromGame(qsl("msdp.ROOM")), "an MSDP variable was not read as the game's");
        QVERIFY2(eventNames::fromGame(qsl("mssp")), "the MSSP table's own name was not read as the game's");
        QVERIFY2(eventNames::fromGame(qsl("mxp.entity")), "an MXP tag was not read as the game's");
        QVERIFY2(!eventNames::fromGame(qsl("sysLoadEvent")), "an event Mudlet raises was read as the game's");
        QVERIFY2(!eventNames::fromGame(qsl("gmcpish")), "a name that only starts with a protocol's letters was read as the game's");
        QVERIFY2(!eventNames::fromGame(qsl("MyEvent")), "a script's own name was read as the game's");
        QVERIFY2(!eventNames::fromGame(QString()), "a name with nothing in it was read as the game's");
    }

    void test_theSystemEventsAreListedOnceEach()
    {
        const QStringList system = eventNames::systemEvents();

        QVERIFY2(system.contains(qsl("sysConnectionEvent")), "the list is missing an event Mudlet raises from C++");
        QVERIFY2(system.contains(qsl("sysSpeedwalkStarted")), "the list is missing an event Mudlet raises from its own Lua");
        QCOMPARE(QSet<QString>(system.constBegin(), system.constEnd()).size(), system.size());
        // A list that lost most of its entries still passes everything above
        QVERIFY2(system.size() >= scmLeastSystemEvents,
                 qPrintable(qsl("the list offers %1 names, against the %2 it is kept at - it has lost entries rather than gained them").arg(system.size()).arg(scmLeastSystemEvents)));

        for (const QString& name : system) {
            QVERIFY2(!name.isEmpty(), "the list carries an entry with nothing in it");
            QVERIFY2(!name.trimmed().isEmpty() && name.trimmed() == name, qPrintable(qsl("\"%1\" is padded with whitespace, which is not the name anything is raised under").arg(name)));
            // Not every Mudlet event starts with sys (the map, speech and channel
            // 102 ones), so the check is that none is the game's
            QVERIFY2(!eventNames::fromGame(name), qPrintable(qsl("\"%1\" is named after a protocol table, so it is an event the game sends rather than one Mudlet raises").arg(name)));
        }

        for (int step = 1; step < system.size(); ++step) {
            QVERIFY2(system.at(step - 1).compare(system.at(step), Qt::CaseInsensitive) < 0,
                     qPrintable(qsl("\"%1\" is listed before \"%2\", so the list is not in the order it is offered in").arg(system.at(step - 1), system.at(step))));
        }
    }
};

#include "EventNamesTest.moc"
MUDLET_GROUPED_TEST_MAIN(EventNamesTest)
