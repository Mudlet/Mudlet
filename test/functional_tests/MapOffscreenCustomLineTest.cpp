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
 * A custom exit line is a list of arbitrary points, so it can start at a room
 * hundreds of units off screen and still cross the viewport. T2DMap now asks
 * TAreaGridIndex which rooms are on screen instead of walking the whole Z
 * level, and a query by room position can never return that room - so TArea
 * keeps a second per-Z index of the rooms that have custom lines and
 * paintRoomExits() adds those on top.
 *
 * The map benchmark cannot cover this: its map has no custom lines at all, and
 * a viewport cull that drops off-screen lines looks perfect on it. Neither can
 * a Lua spec, since the only evidence is pixels and nothing in the Lua API
 * reads back what the mapper drew.
 *
 * Run with: ctest -R MapOffscreenCustomLineTest -V
 */

#include <QFileInfo>
#include <QPixmap>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest/QtTest>

#include "Host.h"
#include "MudletInstanceCoordinator.h"
#include "PortableModeTestHelper.h"
#include "ProfileTestHelper.h"
#include "T2DMap.h"
#include "TArea.h"
#include "TMap.h"
#include "TRoom.h"
#include "TRoomDB.h"
#include "TelnetServerStub.h"
#include "ctelnet.h"
#include "dlgMapper.h"
#include "mudlet.h"

#include "GroupedTest.h"

class MapOffscreenCustomLineTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    TelnetServerStub* mpServer = nullptr;
    Host* mpHost = nullptr;
    const QString mProfileName = qsl("MapOffscreenCustomLine-Test");
    const QString mLocalhost = qsl("localhost");
    QString mPort;

    // The widget is wider than it is tall so that the two axes of the viewport
    // rectangle cannot be confused with one another.
    static constexpr int kWidgetWidth = 600;
    static constexpr int kWidgetHeight = 400;
    // Map units across the shorter widget dimension, so the viewport spans
    // y in [-10, 10] and x in [-15, 15] around the centre.
    static constexpr double kZoom = 20.0;
    // Far enough out that no rounding of the viewport rectangle reaches it.
    static constexpr int kFarRoomY = 400;
    static constexpr int kGenerousBound = 160;
    static constexpr int kFarRoomId = 100;
    static constexpr int kPlayerRoomId = 5;

    static QColor lineColour() { return QColor(0, 255, 128); }

    void deleteProfileDirectory() const
    {
        QDir dir(mudlet::getMudletPath(enums::profileHomePath, mProfileName));
        if (dir.exists()) {
            dir.removeRecursively();
        }
    }

    TMap* map() const { return mpHost->mpMap.data(); }

    bool addRoomAt(const int id, const int areaId, const int x, const int y) const { return map()->addRoom(id) && map()->setRoomArea(id, areaId) && map()->setRoomCoordinates(id, x, y, 0); }

    // The line is drawn with an antialiased cosmetic pen, so almost none of its
    // pixels hold the pen colour exactly - they hold it blended with the black
    // behind it, which scales all three channels together. Counting the
    // channel ORDER instead accepts every one of those blends and still
    // excludes the rest of the frame, whose reds and greys have r >= g.
    static int countCustomLinePixels(const QImage& image)
    {
        int count = 0;
        for (int y = 0; y < image.height(); ++y) {
            for (int x = 0; x < image.width(); ++x) {
                const QColor pixel = image.pixelColor(x, y);
                if (pixel.green() > 60 && pixel.green() > pixel.blue() + 20 && pixel.blue() > pixel.red() + 20) {
                    ++count;
                }
            }
        }
        return count;
    }

private slots:
    void initTestCase()
    {
        if (portableMarkerPresent()) {
            QSKIP("portable.txt present - it takes precedence over XDG_CONFIG_HOME, so the config dir cannot be redirected");
        }

        // A config root of this process's own, so this never reads or writes
        // the developer's ~/.config/mudlet. Since #9712 the opt-in that makes
        // setupConfig() adopt a directory is $XDG_CONFIG_HOME/mudlet/profiles,
        // not the mudlet directory alone.
        QVERIFY(mConfigDir.isValid());
        QVERIFY(QDir().mkpath(qsl("%1/mudlet/profiles").arg(mConfigDir.path())));
        mSavedXdg = qgetenv("XDG_CONFIG_HOME");
        qputenv("XDG_CONFIG_HOME", mConfigDir.path().toUtf8());

        mpServer = new TelnetServerStub(qApp);
        mpServer->start(mLocalhost, 0);
        mPort = QString::number(mpServer->serverPort());

        mudlet::start();
        mudlet::self()->setupConfig();
        QCOMPARE(mudlet::getMudletPath(enums::mainPath), qsl("%1/mudlet").arg(mConfigDir.path()));
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);
        mudlet::self()->mSkipDefaultPackageInstall = true;
        deleteProfileDirectory();

        mpHost = TestProfile::create(mProfileName, mLocalhost, mPort);
        QVERIFY(mpHost);
        QSignalSpy connected(&(mpHost->mTelnet), &cTelnet::signal_connected);
        QVERIFY2(connected.wait(3000), "could not connect to the telnet stub");
    }

    void cleanupTestCase()
    {
        mpHost = nullptr;
        delete mpServer;
        mpServer = nullptr;
        if (mudlet::self()) {
            deleteProfileDirectory();
            delete mudlet::self();
        }
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
    }

    // A room far outside the viewport, whose custom line runs into it, still
    // gets that line drawn.
    void test_offscreenRoomsCustomLineStillReachesTheViewport()
    {
        TMap* pMap = map();
        TRoomDB* pRoomDB = pMap->mpRoomDB.get();
        pMap->mapClear();

        const int areaId = pRoomDB->addArea(qsl("Custom Line Area"));
        QVERIFY(areaId > 0);

        // A 3x3 block around the origin for the viewport to hold, with the
        // player in the middle one.
        int nextId = 1;
        for (int y = -1; y <= 1; ++y) {
            for (int x = -1; x <= 1; ++x) {
                QVERIFY(addRoomAt(nextId++, areaId, x, y));
            }
        }
        QVERIFY(addRoomAt(kFarRoomId, areaId, 0, kFarRoomY));

        TRoom* pFarRoom = pRoomDB->getRoom(kFarRoomId);
        QVERIFY(pFarRoom);
        // Southwards from the far room to the origin in steps, so most of the
        // line is off the top of the widget and only its end is inside.
        QList<QPointF> linePoints;
        for (int y = kFarRoomY; y >= 0; y -= 20) {
            linePoints << QPointF(0.0, static_cast<qreal>(y));
        }
        QCOMPARE(linePoints.constLast(), QPointF(0.0, 0.0));
        pFarRoom->customLines[qsl("s")] = linePoints;
        pFarRoom->customLinesColor[qsl("s")] = lineColour();
        // QMap::value() default-constructs a Qt::PenStyle as Qt::NoPen, so a
        // line without an explicit style draws nothing at all.
        pFarRoom->customLinesStyle[qsl("s")] = Qt::SolidLine;
        pFarRoom->customLinesArrow[qsl("s")] = false;
        pFarRoom->calcRoomDimensions();

        TArea* pArea = pRoomDB->getArea(areaId);
        QVERIFY(pArea);
        QVERIFY2(!pArea->gridMode, "grid mode takes a different paint path, which draws no custom lines");
        QVERIFY2(pArea->getCustomLineRoomsForZ(0).contains(kFarRoomId), "calcRoomDimensions() did not put the room into its area's custom-line index");
        // The premise of the test: the viewport query cannot reach this room.
        // The rectangle the renderer builds at this zoom is 32 by 22 cells, so
        // asking for one ten times that in each direction and still not being
        // given the room leaves no room for argument about rounding.
        QVERIFY2(!pArea->getGridIndex().roomsInViewport(0, -kGenerousBound, kGenerousBound, -kGenerousBound, kGenerousBound).contains(kFarRoomId),
                 "the far room is close enough to be returned by a viewport query, so this would pass with or without the custom-line index");

        pMap->mRoomIdHash[pMap->mProfileName] = kPlayerRoomId;
        pMap->mNewMove = false;
        pMap->setDefaultAreaShown(false);

        mpHost->showHideOrCreateMapper(false);
        QVERIFY(pMap->mpMapper);
        T2DMap* p2dMap = pMap->mpMapper->mp2dMap;
        QVERIFY(p2dMap);
        p2dMap->init();
        p2dMap->resize(kWidgetWidth, kWidgetHeight);
        // paintEvent() re-centres on the player and quietly draws somewhere
        // else unless the player's room, mRoomID and mShiftMode all agree.
        p2dMap->mRoomID = kPlayerRoomId;
        p2dMap->mShiftMode = true;
        p2dMap->mPick = false;
        p2dMap->mAreaID = areaId;
        p2dMap->mMapCenterZ = 0;
        p2dMap->mMapCenterX = 0;
        p2dMap->mMapCenterY = 0;
        pArea->set2DMapZoom(kZoom);

        QPixmap target(kWidgetWidth, kWidgetHeight);
        target.fill(Qt::black);
        p2dMap->render(&target, QPoint(), QRegion(), QWidget::DrawWindowBackground);
        const QImage frame = target.toImage();

        QCOMPARE(p2dMap->getAreaId(), areaId);
        // The visible part of the line runs from the top edge to the middle of
        // the widget, so it is hundreds of pixels long; a handful would mean
        // something else in the frame happened to be greenish.
        const int linePixels = countCustomLinePixels(frame);
        if (linePixels <= 100) {
            // A frame is far quicker to read than a pixel count when working
            // out whether the line went missing or the whole map did.
            const QString framePath = qsl("%1/MapOffscreenCustomLineTest-frame.png").arg(QDir::tempPath());
            frame.save(framePath);
            QFAIL(qPrintable(qsl("the custom line of the room at y=%1 never reached the viewport - only %2 pixels of its colour were drawn. The frame is at %3")
                                     .arg(QString::number(kFarRoomY), QString::number(linePixels), framePath)));
        }
    }

    // The index has to follow the room, or the line goes missing the first time
    // anything about the map changes.
    void test_customLineIndexFollowsTheRoom()
    {
        TMap* pMap = map();
        TRoomDB* pRoomDB = pMap->mpRoomDB.get();
        pMap->mapClear();

        const int areaId = pRoomDB->addArea(qsl("Index Area"));
        QVERIFY(areaId > 0);
        const int otherAreaId = pRoomDB->addArea(qsl("Other Index Area"));
        QVERIFY(otherAreaId > 0);
        QVERIFY(addRoomAt(1, areaId, 0, 0));
        QVERIFY(addRoomAt(2, areaId, 1, 0));

        TArea* pArea = pRoomDB->getArea(areaId);
        QVERIFY(pArea);
        QVERIFY2(pArea->getCustomLineRoomsForZ(0).isEmpty(), "a room without custom lines was indexed as having them");

        TRoom* pRoom = pRoomDB->getRoom(1);
        QVERIFY(pRoom);
        pRoom->customLines[qsl("n")] = QList<QPointF>{QPointF(0.0, 3.0)};
        pRoom->calcRoomDimensions();
        QVERIFY(pArea->getCustomLineRoomsForZ(0).contains(1));

        // A move to another Z level takes the room's entry with it, and does
        // not take its neighbour's non-entry with it.
        QVERIFY(pMap->setRoomCoordinates(1, 0, 0, 4));
        QVERIFY(pMap->setRoomCoordinates(2, 1, 0, 4));
        QVERIFY2(!pArea->getCustomLineRoomsForZ(0).contains(1), "the room stayed indexed on the Z level it left");
        QVERIFY(pArea->getCustomLineRoomsForZ(4).contains(1));
        QVERIFY2(!pArea->getCustomLineRoomsForZ(4).contains(2), "a room without custom lines was carried into the index by the move");

        // calcSpan() rebuilds every index the area holds from the rooms
        // themselves, so it has to arrive at the same answer.
        pArea->calcSpan();
        QCOMPARE(pArea->getCustomLineRoomsForZ(4), QSet<int>{1});

        QVERIFY(pMap->setRoomArea(1, otherAreaId));
        QVERIFY2(!pArea->getCustomLineRoomsForZ(4).contains(1), "the room stayed indexed in the area it left");
        TArea* pOtherArea = pRoomDB->getArea(otherAreaId);
        QVERIFY(pOtherArea);
        QVERIFY(pOtherArea->getCustomLineRoomsForZ(4).contains(1));

        QVERIFY(pRoomDB->removeRoom(1));
        QVERIFY2(!pOtherArea->getCustomLineRoomsForZ(4).contains(1), "a deleted room stayed in the custom-line index");
    }
};

#include "MapOffscreenCustomLineTest.moc"
MUDLET_GROUPED_TEST_MAIN(MapOffscreenCustomLineTest)
