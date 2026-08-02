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

#include "dlgConnectionProfiles.h"
#include "utils.h"

#include <QtTest/QtTest>

/*
 * Tests for the profile name character validation used by the connection
 * dialog. Guards the character-set half of the fix for profile folders
 * duplicated outside of Mudlet (e.g. a file manager appending " (2)" to a
 * copied folder) which used to be rejected, greying out the Connect/Offline
 * buttons. The on-disk exemption half is covered by
 * test/functional_tests/ProfileFolderNameTest.cpp.
 */
class ProfileNameValidationTest : public QObject
{
    Q_OBJECT

private slots:
    void acceptableNames_data()
    {
        QTest::addColumn<QString>("name");

        QTest::newRow("plain") << qsl("Achaea");
        QTest::newRow("leading digit") << qsl("3Scapes");
        QTest::newRow("default new name") << qsl("new profile name");
        QTest::newRow("parenthesised copy suffix") << qsl("test (2)");
        QTest::newRow("parenthesised word") << qsl("StickMUD (backup)");
        QTest::newRow("windows copy suffix") << qsl("test - Copy");
        QTest::newRow("all punctuation") << qsl("a.b_c-d#e&f (g)");
        QTest::newRow("empty") << QString();
    }

    void acceptableNames()
    {
        QFETCH(QString, name);
        QVERIFY(dlgConnectionProfiles::firstInvalidProfileNameChar(name).isNull());
    }

    void rejectedNames_data()
    {
        QTest::addColumn<QString>("name");
        QTest::addColumn<QChar>("badChar");

        QTest::newRow("path separator") << qsl("test/2") << QChar('/');
        QTest::newRow("windows path separator") << qsl("test\\2") << QChar('\\');
        QTest::newRow("first of several invalid") << qsl("a/b:c") << QChar('/');
        QTest::newRow("windows drive colon") << qsl("test:2") << QChar(':');
        QTest::newRow("double quote") << qsl("test\"2") << QChar('"');
        QTest::newRow("tab") << qsl("test\t2") << QChar('\t');
        QTest::newRow("non-ascii") << qsl("café") << QChar(0x00e9);
    }

    void rejectedNames()
    {
        QFETCH(QString, name);
        QFETCH(QChar, badChar);
        QCOMPARE(dlgConnectionProfiles::firstInvalidProfileNameChar(name), badChar);
    }
};

QTEST_GUILESS_MAIN(ProfileNameValidationTest)
#include "ProfileNameValidationTest.moc"
