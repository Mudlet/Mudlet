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
 * Why not a spec: no Lua call reports which textures a frame used. The room
 * pass is run without GL, with stand-in textures, so ctest's offscreen platform
 * runs it; the GL frame is skipped there, and runs on a display:
 *   QT_QPA_PLATFORM=xcb MUDLET_TEST_MODE=1 ./functional_map_tests ModernRoomTextureTest
 */

#include <QFileInfo>
#include <QOpenGLContext>
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
    int addCrowdedArea() const
    {
        TMap* pMap = mpHost->mpMap.data();
        pMap->mapClear();
        const int areaId = pMap->mpRoomDB->addArea(qsl("Crowded Area"));
        for (int id = 1; id <= 600; ++id) {
            if (!(pMap->addRoom(id) && pMap->setRoomArea(id, areaId) && pMap->setRoomCoordinates(id, id % 25, id / 25, 0))) {
                return 0;
            }
            // Short, so that every name fits its room and none is decluttered away
            pMap->mpRoomDB->getRoom(id)->name = qsl("R%1").arg(id);
        }
        mpHost->mShowRoomID = true;
        pMap->setRoomNamesShown(true);
        pMap->mRoomIdHash[pMap->mProfileName] = 312;
        return areaId;
    }

    static void verifyDigitsKept(const ModernGLWidget& widget)
    {
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

    // The room pass itself, with textures that need no GL context, so this runs wherever ctest does
    void test_roomIdDigitsKeepTheirTexturesWithoutGL()
    {
        const int areaId = addCrowdedArea();
        QVERIFY(areaId > 0);
        ModernGLWidget widget(mpHost->mpMap.data(), mpHost);
        GLuint lastTexture = 0;
        widget.mLabelTextureCache.useTextureStandIn(
                [&lastTexture](const QImage&) {
                    return ++lastTexture;
                },
                [](GLuint) {});
        widget.setViewCenter(areaId, 12, 12, 0);
        // Never shown, so nothing sizes it: big enough for every name to be legible, with the whole area in view
        widget.resize(1200, 1200);
        widget.mCameraController.setViewportSize(widget.width(), widget.height());
        widget.frameRooms(26.0);
        // As paintGL() does first: names are placed through the camera's matrices
        widget.updateMatrices();

        // The second frame starts with a full cache, so it shows whether the first left the digits usable
        for (int frame = 1; frame <= 2; ++frame) {
            widget.renderRooms();
            verifyDigitsKept(widget);
            if (QTest::currentTestFailed()) {
                qWarning().nospace() << "in frame " << frame;
                return;
            }
        }
        QVERIFY2(lastTexture >= 512, "the view never reached its texture budget");
    }

    void test_roomIdDigitsKeepTheirTexturesInAGLFrame()
    {
        // Probed first, so that where there is no GL no widget ever tries for a context
        if (QOpenGLContext probe; !probe.create()) {
            QSKIP("no OpenGL context on this platform");
        }
        const int areaId = addCrowdedArea();
        QVERIFY(areaId > 0);
        ModernGLWidget widget(mpHost->mpMap.data(), mpHost);
        widget.resize(400, 300);
        widget.grabFramebuffer();
        if (!widget.isValid()) {
            QSKIP("no OpenGL context on this platform");
        }
        // A context that will not draw, such as one whose shaders do not compile, is a failure
        QVERIFY2(widget.failureMessage().isEmpty(), qPrintable(widget.failureMessage()));
        verifyDigitsKept(widget);
    }
};

#include "ModernRoomTextureTest.moc"
MUDLET_GROUPED_TEST_MAIN(ModernRoomTextureTest)
