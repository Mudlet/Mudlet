/***************************************************************************
 *   Copyright (C) 2026 by Mudlet Developers                               *
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

#include "GroupedTest.h"
#include "MudletApp.h"

#include <QRegularExpression>
#include <QtTest/QtTest>

// The name an app that embeds Mudlet gives game servers in place of desktop
// Mudlet's (MudletApp::setClientName()). TelnetClientNameTest shows it reaching
// each protocol and TMxpVersionTagTest MXP's <VERSION>.
class MudletAppClientNameTest : public QObject
{
    Q_OBJECT

private slots:
    void cleanup() { MudletApp::setClientName(QString()); }

    void test_unsetItIsDesktopMudletsName() { QCOMPARE(MudletApp::clientName(), qsl("Mudlet")); }

    void test_aSetNameIsKeptAsGiven()
    {
        QVERIFY(MudletApp::setClientName(qsl("Mudlet-Android")));
        QCOMPARE(MudletApp::clientName(), qsl("Mudlet-Android"));
    }

    void test_anEmptyNameForgetsIt()
    {
        QVERIFY(MudletApp::setClientName(qsl("Mudlet-Android")));
        QVERIFY(MudletApp::setClientName(QString()));
        QCOMPARE(MudletApp::clientName(), qsl("Mudlet"));
    }

    void test_namesRfc1010AllowsAreTaken_data()
    {
        QTest::addColumn<QString>("name");
        QTest::newRow("one letter") << qsl("M");
        QTest::newRow("mixed case") << qsl("Mudlet-Android");
        QTest::newRow("slash and digits") << qsl("MUDLET/2-0");
        QTest::newRow("forty characters") << QString(40, QLatin1Char('A'));
    }

    void test_namesRfc1010AllowsAreTaken()
    {
        QFETCH(QString, name);
        QVERIFY(MudletApp::setClientName(name));
        QCOMPARE(MudletApp::clientName(), name);
    }

    void test_anyOtherNameIsRefusedAndTheLastOneKept_data()
    {
        QTest::addColumn<QString>("name");
        QTest::newRow("a space") << qsl("MUDLET TEST");
        QTest::newRow("a quote") << qsl("MUDLET\"TEST");
        QTest::newRow("ends in a hyphen") << qsl("MUDLET-");
        QTest::newRow("starts with a hyphen") << qsl("-MUDLET");
        QTest::newRow("starts with a digit") << qsl("2MUDLET");
        QTest::newRow("forty-one characters") << QString(41, QLatin1Char('A'));
        QTest::newRow("not ASCII") << qsl("MUDLÉT");
        QTest::newRow("a control character") << qsl("MUDLET\tTEST");
        QTest::newRow("a trailing line feed") << qsl("MUDLET\n");
    }

    void test_anyOtherNameIsRefusedAndTheLastOneKept()
    {
        QFETCH(QString, name);
        QVERIFY(MudletApp::setClientName(qsl("Mudlet-Test")));
        QTest::ignoreMessage(QtWarningMsg, QRegularExpression(qsl("refused")));
        QVERIFY(!MudletApp::setClientName(name));
        QCOMPARE(MudletApp::clientName(), qsl("Mudlet-Test"));
    }
};

MUDLET_GROUPED_TEST_GUILESS_MAIN(MudletAppClientNameTest)
#include "MudletAppClientNameTest.moc"
