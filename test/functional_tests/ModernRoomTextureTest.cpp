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

/*
 * The modern 3D view keeps at most a fixed number of room textures (symbols,
 * ID digits, markers, names), and making room for another must not evict one
 * this frame has already drawn with.
 *
 * Why not a spec: no Lua call reports which textures a frame used. It needs a
 * GL context, which the offscreen platform ctest runs on has not, so there it
 * is listed as not run; run it on a display:
 *   QT_QPA_PLATFORM=xcb MUDLET_TEST_MODE=1 ./functional_map_tests ModernRoomTextureTest
 */

#include <QFileInfo>
#include <QTemporaryDir>
#include <QtTest/QtTest>

#include "Host.h"
#include "HostManager.h"
#include "MudletApp.h"
#include "MudletInstanceCoordinator.h"
#include "TMap.h"
#include "TRoom.h"
#include "TRoomDB.h"
#include "modern_glwidget.h"
#include "mudlet.h"

#include "GroupedTest.h"

class ModernRoomTextureTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    Host* mpHost = nullptr;
    const QString mProfileName = qsl("ModernRoomTexture-Test");

    static bool portableMarkerPresent()
    {
        return QFileInfo::exists(qsl("%1/portable.txt").arg(QCoreApplication::applicationDirPath())) || QFileInfo::exists(qsl("%1/.config/mudlet/portable.txt").arg(QDir::homePath()));
    }

    void deleteProfileDirectory() const
    {
        QDir dir(MudletApp::getMudletPath(enums::profileHomePath, mProfileName));
        if (dir.exists()) {
            dir.removeRecursively();
        }
    }

private slots:
    void initTestCase()
    {
        if (portableMarkerPresent()) {
            QSKIP("portable.txt present - it takes precedence over XDG_CONFIG_HOME, so the config dir cannot be redirected");
        }

        QVERIFY(mConfigDir.isValid());
        QVERIFY(QDir().mkpath(qsl("%1/mudlet/profiles").arg(mConfigDir.path())));
        mSavedXdg = qgetenv("XDG_CONFIG_HOME");
        qputenv("XDG_CONFIG_HOME", mConfigDir.path().toUtf8());

        mudlet::start();
        mudlet::self()->setupConfig();
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);
        deleteProfileDirectory();

        auto* hostManager = HostManager::self();
        QVERIFY2(hostManager->addHost(mProfileName, qsl("23"), QString(), QString()), "failed to create the test Host");
        mpHost = hostManager->getHost(mProfileName);
        QVERIFY(mpHost);
    }

    void cleanupTestCase()
    {
        mpHost = nullptr;
        if (mudlet::self()) {
            deleteProfileDirectory();
            delete mudlet::self();
        }
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
    }

    // More differently named rooms than the view has textures for, so names have to make room
    void test_roomIdDigitsKeepTheirTexturesThroughAFrameOverBudget()
    {
        TMap* pMap = mpHost->mpMap.data();
        pMap->mapClear();
        const int areaId = pMap->mpRoomDB->addArea(qsl("Crowded Area"));
        QVERIFY(areaId > 0);
        for (int id = 1; id <= 600; ++id) {
            QVERIFY(pMap->addRoom(id) && pMap->setRoomArea(id, areaId) && pMap->setRoomCoordinates(id, id % 25, id / 25, 0));
            pMap->mpRoomDB->getRoom(id)->name = qsl("Room number %1").arg(id);
        }
        mpHost->mShowRoomID = true;
        pMap->setRoomNamesShown(true);
        pMap->mRoomIdHash[pMap->mProfileName] = 312;

        ModernGLWidget widget(pMap, mpHost);
        widget.resize(400, 300);
        widget.grabFramebuffer();
        if (!widget.isValid() || !widget.failureMessage().isEmpty()) {
            QSKIP("no OpenGL context on this platform");
        }

        QStringList digitKeys;
        for (const QString& key : std::as_const(widget.mRoomTexturesInUse)) {
            if (key.startsWith(qsl("digit|"))) {
                digitKeys.append(key);
            }
        }
        QVERIFY2(widget.mRoomTexturesInUse.size() > 20, "the frame did not ask for the rooms' names");
        // Black and white, as the rooms' colors need, ten digits each
        QCOMPARE(digitKeys.size(), 20);
        for (const QString& key : std::as_const(digitKeys)) {
            QVERIFY2(widget.mLabelTextureCache.imageTexture(key), qPrintable(qsl("%1 was evicted in the frame that drew with it").arg(key)));
        }
    }
};

#include "ModernRoomTextureTest.moc"
MUDLET_GROUPED_TEST_MAIN(ModernRoomTextureTest)
