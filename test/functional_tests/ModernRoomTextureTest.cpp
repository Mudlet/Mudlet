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

#include "GeometryManager.h"
#include "Host.h"
#include "HostManager.h"
#include "MudletApp.h"
#include "MudletInstanceCoordinator.h"
#include "RenderCommand.h"
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

    // A room with a symbol under one of the level above, which has a border and an exit; and beside it on the
    // viewed level, a room whose environment color is wholly see-through
    int addLayeredArea() const
    {
        TMap* pMap = mpHost->mpMap.data();
        pMap->mapClear();
        const int areaId = pMap->mpRoomDB->addArea(qsl("Layered Area"));
        const QList<QVector3D> places{{0, 0, 0}, {2, 0, 0}, {0, 0, 1}, {2, 0, 1}};
        for (int id = 1; id <= places.size(); ++id) {
            const QVector3D& place = places.at(id - 1);
            if (!(pMap->addRoom(id) && pMap->setRoomArea(id, areaId) && pMap->setRoomCoordinates(id, place.x(), place.y(), place.z()))) {
                return 0;
            }
        }
        pMap->mpRoomDB->getRoom(1)->mSymbol = qsl("W");
        constexpr int clearEnvironment = 500;
        pMap->mCustomEnvColors[clearEnvironment] = QColor(0, 0, 0, 0);
        pMap->mpRoomDB->getRoom(2)->environment = clearEnvironment;
        pMap->mpRoomDB->getRoom(3)->mBorderColor = QColor(Qt::red);
        pMap->mpRoomDB->getRoom(3)->mBorderThickness = 4;
        if (!(pMap->setExit(3, 4, DIR_EAST) && pMap->setExit(4, 3, DIR_WEST))) {
            return 0;
        }
        mpHost->mShowRoomID = false;
        pMap->setRoomNamesShown(false);
        pMap->mRoomIdHash[pMap->mProfileName] = 1;
        return areaId;
    }

    // The commands one frame queues for the exits and rooms of the layered area, seen at a slant
    static void queueLayeredFrame(ModernGLWidget& widget, const int areaId)
    {
        GLuint lastTexture = 0;
        widget.mLabelTextureCache.useTextureStandIn(
                [&lastTexture](const QImage&) {
                    return ++lastTexture;
                },
                [](GLuint) {});
        widget.setViewCenter(areaId, 0, 0, 0);
        widget.resize(800, 600);
        widget.mCameraController.setViewportSize(widget.width(), widget.height());
        widget.setCameraPosition(1.0f, 40.0f, CameraController::scmNorthUpAzimuth);
        widget.updateMatrices();
        widget.mRenderCommandQueue.clear();
        widget.renderConnections();
        widget.renderRooms();
    }

    static bool seeThrough(const RenderCommand* command)
    {
        if (const auto* cubes = dynamic_cast<const RenderInstancedCubesCommand*>(command)) {
            return std::any_of(cubes->instances().cbegin(), cubes->instances().cend(), [](const CubeInstanceData& cube) {
                return cube.color[3] < 1.0f;
            });
        }
        if (const auto* triangles = dynamic_cast<const RenderTrianglesCommand*>(command)) {
            for (qsizetype i = 3; i < triangles->colors().size(); i += 4) {
                if (triangles->colors().at(i) < 1.0f) {
                    return true;
                }
            }
        }
        return false;
    }

    // The room with a border above the symbol, rather than the exit beside it
    static bool theBorderedRoomAbove(const RenderInstancedCubesCommand* cubes)
    {
        return std::any_of(cubes->instances().cbegin(), cubes->instances().cend(), [](const CubeInstanceData& cube) {
            const QVector4D center = cube.transform.column(3);
            return qFuzzyIsNull(center.x()) && qFuzzyIsNull(center.y()) && center.z() > 0.5f;
        });
    }

    // Drawn into the depth buffer, it would hide what is behind it that is drawn after it
    void test_nothingSeeThroughIsDrawnIntoTheDepthBuffer()
    {
        const int areaId = addLayeredArea();
        QVERIFY(areaId > 0);
        ModernGLWidget widget(mpHost->mpMap.data(), mpHost);
        queueLayeredFrame(widget, areaId);

        bool depthWrite = true;
        bool colorWrite = true;
        int seeThroughDraws = 0;
        for (const auto& command : widget.mRenderCommandQueue.commands()) {
            if (const auto* state = dynamic_cast<const GLStateCommand*>(command.get())) {
                depthWrite = state->stateType() == GLStateCommand::ENABLE_DEPTH_WRITE ? true : state->stateType() == GLStateCommand::DISABLE_DEPTH_WRITE ? false : depthWrite;
                colorWrite = state->stateType() == GLStateCommand::ENABLE_COLOR_WRITE ? true : state->stateType() == GLStateCommand::DISABLE_COLOR_WRITE ? false : colorWrite;
                continue;
            }
            if (seeThrough(command.get())) {
                ++seeThroughDraws;
                QVERIFY2(!(depthWrite && colorWrite), qPrintable(qsl("a see-through %1 is drawn into the depth buffer").arg(QLatin1String(command->getCommandName()))));
            }
        }
        QVERIFY2(seeThroughDraws > 0, "nothing see-through was drawn");
    }

    // From the lowest level up, each level with what is on it, so a symbol below shows only through the room above
    void test_eachLevelIsDrawnWithWhatIsOnItBeforeTheLevelAbove()
    {
        const int areaId = addLayeredArea();
        QVERIFY(areaId > 0);
        ModernGLWidget widget(mpHost->mpMap.data(), mpHost);
        queueLayeredFrame(widget, areaId);

        bool depthWrite = true;
        bool colorWrite = true;
        qsizetype index = 0;
        qsizetype lastLowerDecal = -1;
        qsizetype upperDepthOnly = -1;
        qsizetype upperColored = -1;
        for (const auto& command : widget.mRenderCommandQueue.commands()) {
            if (const auto* state = dynamic_cast<const GLStateCommand*>(command.get())) {
                depthWrite = state->stateType() == GLStateCommand::ENABLE_DEPTH_WRITE ? true : state->stateType() == GLStateCommand::DISABLE_DEPTH_WRITE ? false : depthWrite;
                colorWrite = state->stateType() == GLStateCommand::ENABLE_COLOR_WRITE ? true : state->stateType() == GLStateCommand::DISABLE_COLOR_WRITE ? false : colorWrite;
            } else if (const auto* decal = dynamic_cast<const RenderTexturedTrianglesCommand*>(command.get())) {
                const QVector<float>& vertices = decal->geometry().vertices;
                bool lower = !vertices.isEmpty();
                for (qsizetype i = 2; i < vertices.size(); i += 3) {
                    lower = lower && vertices.at(i) < 0.5f;
                }
                if (lower) {
                    lastLowerDecal = index;
                }
            } else if (const auto* cubes = dynamic_cast<const RenderInstancedCubesCommand*>(command.get()); cubes && seeThrough(cubes) && theBorderedRoomAbove(cubes)) {
                if (depthWrite && !colorWrite && upperDepthOnly < 0) {
                    upperDepthOnly = index;
                } else if (colorWrite && upperColored < 0) {
                    upperColored = index;
                }
            }
            ++index;
        }
        QVERIFY2(lastLowerDecal >= 0, "nothing was drawn on the viewed level's rooms");
        QVERIFY2(upperColored >= 0, "the level above was not drawn");
        QVERIFY2(lastLowerDecal < upperColored, "something on the viewed level's rooms is drawn over the level above");
        // Each pixel then takes the color of the nearest face of the see-through blocks only, not of those behind it too
        QVERIFY2(upperDepthOnly >= 0 && upperDepthOnly < upperColored, "the level above's see-through blocks are not first drawn into the depth buffer alone");
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
