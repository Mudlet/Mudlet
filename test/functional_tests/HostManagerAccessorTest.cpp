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

// HostManager::self() is dereferenced unguarded all over the tree, so what a
// second instance does to it matters: the application holds one as a value
// member, but the class is publicly constructible and a test exercising the
// host pool on its own is the obvious next thing to write.

#include <QtTest/QtTest>

#include "GroupedTest.h"
#include "HostManager.h"

#include <QRegularExpression>

class HostManagerAccessorTest : public QObject
{
    Q_OBJECT

private slots:
    void theAccessorFollowsTheOneManagersLifetime()
    {
        QVERIFY2(!HostManager::self(), "nothing in this process should have built a HostManager yet");
        {
            HostManager manager;
            QCOMPARE(HostManager::self(), &manager);
        }
        QVERIFY(!HostManager::self());
    }

    void aSecondManagerNeitherStealsTheAccessorNorClearsIt()
    {
        HostManager first;
        QCOMPARE(HostManager::self(), &first);
        {
            QTest::ignoreMessage(QtWarningMsg, QRegularExpression(qsl("a HostManager already exists")));
            HostManager second;
            QCOMPARE(HostManager::self(), &first);
        }
        QCOMPARE(HostManager::self(), &first);
    }
};

#include "HostManagerAccessorTest.moc"
MUDLET_GROUPED_TEST_MAIN(HostManagerAccessorTest)
