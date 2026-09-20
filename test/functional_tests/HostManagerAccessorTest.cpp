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
//
// The last test covers the other end of that lifetime. The header promises the
// accessor answers "until its members are torn down", and mHostPool is the
// member that matters: every loaded profile's ~Host() runs as it goes, and
// several repointed call sites reach HostManager::self() from there.

#include <QtTest/QtTest>

#include "GroupedTest.h"
#include "Host.h"
#include "HostManager.h"
#include "MudletInstanceCoordinator.h"
#include "PortableModeTestHelper.h"
#include "mudlet.h"

#include <QDir>
#include <QRegularExpression>
#include <QTemporaryDir>

#include <memory>

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

    // Last, because it builds the application object - and with it the one
    // HostManager the rest of the process would otherwise have to work around.
    void theProfilesGoAwayWhileTheAccessorStillAnswers()
    {
        if (portableMarkerPresent()) {
            QSKIP("portable.txt present - it takes precedence over XDG_CONFIG_HOME, so the config dir cannot be redirected");
        }
        QTemporaryDir configDir;
        QVERIFY(configDir.isValid());
        QVERIFY(QDir().mkpath(qsl("%1/mudlet/profiles").arg(configDir.path())));
        const QByteArray savedXdg = qgetenv("XDG_CONFIG_HOME");
        qputenv("XDG_CONFIG_HOME", configDir.path().toUtf8());

        mudlet::start();
        mudlet::self()->setupConfig();
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>(qsl("MudletInstanceCoordinator")));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);

        const QString profileName = qsl("HostManagerAccessor-Test");
        QVERIFY(HostManager::self());
        QVERIFY2(HostManager::self()->addHost(profileName, QString(), QString(), QString()), "failed to put a profile in the pool");
        Host* pHost = HostManager::self()->getHost(profileName);
        QVERIFY(pHost);

        // ~QObject() runs after ~Host()'s own body, so this is the latest point
        // in a profile's teardown - if the accessor still answers here it
        // answered for the whole of it.
        bool profileWasDestroyed = false;
        bool accessorAnsweredDuringTeardown = false;
        connect(pHost, &QObject::destroyed, qApp, [&profileWasDestroyed, &accessorAnsweredDuringTeardown]() {
            profileWasDestroyed = true;
            accessorAnsweredDuringTeardown = (HostManager::self() != nullptr);
        });

        // ~mudlet() nulls its own accessor at the end of its body and the
        // HostManager member is destroyed after that, so this is also the point
        // where mudlet::self() has already gone
        delete mudlet::self();

        savedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", savedXdg);

        QVERIFY2(profileWasDestroyed, "the profile outlived the manager that held it, so this says nothing about the order");
        QVERIFY2(accessorAnsweredDuringTeardown, "the accessor was cleared before the profile pool it describes was emptied");
    }
};

#include "HostManagerAccessorTest.moc"
MUDLET_GROUPED_TEST_MAIN(HostManagerAccessorTest)
