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
 * The modern 3D view draws what the 2D map's mapper settings ask for: the grid,
 * round rooms and the upper and lower levels' colors, and it leaves out room IDs
 * that would be unreadable or hidden behind another, as it does names.
 *
 * Why not a spec: no Lua call reports what the 3D view drew. The ID pass is run
 * without GL, with stand-in textures, so ctest's offscreen platform runs it; the
 * frames are read back from GL, which is skipped there, and run on a display:
 *   QT_QPA_PLATFORM=xcb MUDLET_TEST_MODE=1 ./functional_map_tests ModernMapperRenderTest
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
#include "TRoomDB.h"
#include "modern_glwidget.h"
#include "mudlet.h"

#include "GroupedTest.h"

class ModernMapperRenderTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    Host* mpHost = nullptr;
    const QString mProfileName = qsl("ModernMapperRender-Test");
    static constexpr int kPlayerRoomId = 1;
    static constexpr int kEastRoomId = 2;
    static constexpr int kLowerRoomId = 3;
    // A room's height in the view, whose rooms are a level apart and flattened eightfold
    static constexpr float kRoomHeight = 2.0f / 4.0f / 8.0f;

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

    TMap* map() const { return mpHost->mpMap.data(); }

    bool addRoomAt(const int id, const int areaId, const int x, const int y, const int z) const
    {
        return map()->addRoom(id) && map()->setRoomArea(id, areaId) && map()->setRoomCoordinates(id, x, y, z);
    }

    // The player's room and one east of it on level 0, and one on the level below, north of them
    int addArea() const
    {
        map()->mapClear();
        const int areaId = map()->mpRoomDB->addArea(qsl("Render Area"));
        if (!(addRoomAt(kPlayerRoomId, areaId, 0, 0, 0) && addRoomAt(kEastRoomId, areaId, 2, 0, 0) && addRoomAt(kLowerRoomId, areaId, 0, 2, -1))) {
            return 0;
        }
        map()->mRoomIdHash[map()->mProfileName] = kPlayerRoomId;
        return areaId;
    }

    // Probed first, so that where there is no GL no widget ever tries for a context
    static bool glAvailable()
    {
        QOpenGLContext probe;
        return probe.create();
    }

    // Looking straight down on the player's room from a fixed distance
    static void lookDown(ModernGLWidget& widget)
    {
        widget.resize(600, 600);
        widget.grabFramebuffer();
        widget.mCameraController.setTopView();
        // About a hundred pixels to a room's spacing
        widget.setCameraDistance(0.6f);
    }

    // The color of the frame where this point of the map shows
    static QColor colorAt(ModernGLWidget& widget, const QVector3D& point)
    {
        const QImage frame = widget.grabFramebuffer();
        const std::optional<QPointF> pixel = widget.mCameraController.pixelForPoint(point);
        if (!pixel || !frame.rect().contains(pixel->toPoint())) {
            return QColor();
        }
        return frame.pixelColor(pixel->toPoint());
    }

    static int difference(const QColor& a, const QColor& b) { return qAbs(a.red() - b.red()) + qAbs(a.green() - b.green()) + qAbs(a.blue() - b.blue()); }

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
        mpHost->mBgColor_2 = QColor(Qt::black);
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

    // As on the 2D map, the grid's lines run along whole coordinates, here between the two rooms
    void test_theGridIsDrawnWhenShown()
    {
        if (!glAvailable()) {
            QSKIP("no OpenGL context on this platform");
        }
        QVERIFY(addArea() > 0);
        ModernGLWidget widget(map(), mpHost);
        lookDown(widget);
        if (!widget.isValid()) {
            QSKIP("no OpenGL context on this platform");
        }
        QVERIFY2(widget.failureMessage().isEmpty(), qPrintable(widget.failureMessage()));
        mpHost->mMapGridColor = QColor(Qt::white);
        mpHost->mMapGridLineSize = 5.0;
        const QVector3D onALine(1.0f, 1.5f, -kRoomHeight / 2.0f);

        mpHost->mMapperShowGrid = false;
        const QColor hidden = colorAt(widget, onALine);
        mpHost->mMapperShowGrid = true;
        const QColor shown = colorAt(widget, onALine);
        mpHost->mMapperShowGrid = false;
        QVERIFY(hidden.isValid() && shown.isValid());
        QVERIFY2(shown.lightness() - hidden.lightness() > 100, qPrintable(qsl("the grid line is %1 shown and %2 hidden").arg(shown.name(), hidden.name())));
    }

    // Round rooms are cylinders, so a square room's corner is background on a round one
    void test_roundRoomsLeaveTheirCornersEmpty()
    {
        if (!glAvailable()) {
            QSKIP("no OpenGL context on this platform");
        }
        QVERIFY(addArea() > 0);
        ModernGLWidget widget(map(), mpHost);
        lookDown(widget);
        if (!widget.isValid()) {
            QSKIP("no OpenGL context on this platform");
        }
        const QVector3D corner(2.22f, 0.22f, kRoomHeight / 2.0f);
        const QVector3D empty(2.4f, 0.4f, kRoomHeight / 2.0f);

        mpHost->mBubbleMode = false;
        const QColor square = colorAt(widget, corner);
        const QColor squareEmpty = colorAt(widget, empty);
        mpHost->mBubbleMode = true;
        const QColor round = colorAt(widget, corner);
        const QColor middle = colorAt(widget, QVector3D(2.0f, 0.0f, kRoomHeight / 2.0f));
        mpHost->mBubbleMode = false;
        QVERIFY(square.isValid() && squareEmpty.isValid() && round.isValid() && middle.isValid());
        QVERIFY2(difference(square, squareEmpty) > 60, qPrintable(qsl("a square room's corner is %1, beside it %2").arg(square.name(), squareEmpty.name())));
        QVERIFY2(difference(round, squareEmpty) < 30, qPrintable(qsl("a round room's corner is %1, beside it %2").arg(round.name(), squareEmpty.name())));
        QVERIFY2(difference(middle, squareEmpty) > 60, qPrintable(qsl("a round room's middle is %1, beside it %2").arg(middle.name(), squareEmpty.name())));
    }

    // With the upper and lower levels shown, as on the 2D map, the level below takes on the lower level color
    void test_otherLevelsTakeOnTheUpperAndLowerLevelColors()
    {
        if (!glAvailable()) {
            QSKIP("no OpenGL context on this platform");
        }
        QVERIFY(addArea() > 0);
        ModernGLWidget widget(map(), mpHost);
        lookDown(widget);
        if (!widget.isValid()) {
            QSKIP("no OpenGL context on this platform");
        }
        const QColor savedLower = mpHost->mLowerLevelColor;
        const bool savedShown = mudlet::self()->drawUpperLowerLevels();
        mpHost->mLowerLevelColor = QColor(Qt::red);
        const QVector3D lowerRoom(0.0f, 2.0f, -1.0f + kRoomHeight / 2.0f);

        mudlet::self()->setDrawUpperLowerLevels(false);
        const QColor plain = colorAt(widget, lowerRoom);
        mudlet::self()->setDrawUpperLowerLevels(true);
        const QColor colored = colorAt(widget, lowerRoom);
        mudlet::self()->setDrawUpperLowerLevels(savedShown);
        mpHost->mLowerLevelColor = savedLower;
        QVERIFY(plain.isValid() && colored.isValid());
        const int plainRedness = plain.red() - plain.green();
        const int coloredRedness = colored.red() - colored.green();
        QVERIFY2(coloredRedness - plainRedness > 30, qPrintable(qsl("the lower room is %1 with the levels' colors and %2 without").arg(colored.name(), plain.name())));
    }

    // As names are: where IDs overlap one is kept, the viewed level's as only it has IDs on the 2D map,
    // and none so small as to be unreadable
    void test_roomIdsAreDeclutteredWithoutGL()
    {
        map()->mapClear();
        const int areaId = map()->mpRoomDB->addArea(qsl("Stacked Area"));
        QVERIFY(addRoomAt(kPlayerRoomId, areaId, 0, 0, 0) && addRoomAt(kLowerRoomId, areaId, 0, 0, 1));
        map()->mRoomIdHash[map()->mProfileName] = kPlayerRoomId;
        mpHost->mShowRoomID = true;
        ModernGLWidget widget(map(), mpHost);
        GLuint lastTexture = 0;
        widget.mLabelTextureCache.useTextureStandIn(
                [&lastTexture](const QImage&) {
                    return ++lastTexture;
                },
                [](GLuint) {});
        widget.setViewCenter(areaId, 0, 0, 0);
        widget.resize(600, 600);
        widget.mCameraController.setViewportSize(widget.width(), widget.height());
        widget.frameRooms(3.0);
        // Straight down, so the room above hides the viewed one and is nearer
        widget.mCameraController.setTopView();
        widget.updateMatrices();
        widget.renderRooms();
        QCOMPARE(widget.mRoomIdsDrawn, QSet<int>{kPlayerRoomId});

        widget.mCameraController.setScale(CameraController::scmMaxDistance);
        widget.updateMatrices();
        widget.renderRooms();
        mpHost->mShowRoomID = false;
        QVERIFY2(widget.mRoomIdsDrawn.isEmpty(), "IDs too small to read were drawn");
    }
};

#include "ModernMapperRenderTest.moc"
MUDLET_GROUPED_TEST_MAIN(ModernMapperRenderTest)
