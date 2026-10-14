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

#include <QComboBox>
#include <QDialog>
#include <QFileInfo>
#include <QPixmap>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTimer>
#include <QtTest/QtTest>

#include <algorithm>

#include "Host.h"
#include "MudletInstanceCoordinator.h"
#include "MudletApp.h"
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

using namespace std::chrono_literals;

class MapOffscreenCustomLineTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    TelnetServerStub* mpServer = nullptr;
    Host* mpHost = nullptr;
    // A member rather than a local because the timer that writes it is still
    // queued if the test function returns early
    bool mDialogAnswered = false;
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

    static bool customLineColoured(const QColor& pixel) { return pixel.green() > 60 && pixel.green() > pixel.blue() + 20 && pixel.blue() > pixel.red() + 20; }

    void deleteProfileDirectory() const
    {
        QDir dir(MudletApp::getMudletPath(enums::profileHomePath, mProfileName));
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
                if (customLineColoured(image.pixelColor(x, y))) {
                    ++count;
                }
            }
        }
        return count;
    }

    // The mapper showing areaId at kZoom, centred on the origin with the player there.
    T2DMap* showAreaAtTheOrigin(const int areaId) const
    {
        TMap* pMap = map();
        pMap->mRoomIdHash[pMap->mProfileName] = kPlayerRoomId;
        pMap->mNewMove = false;
        pMap->setDefaultAreaShown(false);
        mpHost->showHideOrCreateMapper(false);
        if (!pMap->mpMapper || !pMap->mpMapper->mp2dMap) {
            return nullptr;
        }
        T2DMap* p2dMap = pMap->mpMapper->mp2dMap;
        p2dMap->init();
        p2dMap->resize(kWidgetWidth, kWidgetHeight);
        p2dMap->mRoomID = kPlayerRoomId;
        p2dMap->mShiftMode = true;
        p2dMap->mPick = false;
        p2dMap->mAreaID = areaId;
        p2dMap->mMapCenterZ = 0;
        p2dMap->mMapCenterX = 0;
        p2dMap->mMapCenterY = 0;
        TArea* pArea = pMap->mpRoomDB->getArea(areaId);
        if (!pArea) {
            return nullptr;
        }
        pArea->set2DMapZoom(kZoom);
        return p2dMap;
    }

    static QImage renderMapFrame(T2DMap* p2dMap)
    {
        QPixmap target(kWidgetWidth, kWidgetHeight);
        target.fill(Qt::black);
        p2dMap->render(&target, QPoint(), QRegion(), QWidget::DrawWindowBackground);
        return target.toImage();
    }

    static void setCustomLine(TRoom* pRoom, const QString& exitKey, const QList<QPointF>& points)
    {
        pRoom->customLines[exitKey] = points;
        pRoom->customLinesColor[exitKey] = lineColour();
        pRoom->customLinesStyle[exitKey] = Qt::SolidLine;
        pRoom->customLinesArrow[exitKey] = false;
        pRoom->calcRoomDimensions();
    }

    static void removeCustomLine(TRoom* pRoom, const QString& exitKey)
    {
        pRoom->customLines.remove(exitKey);
        pRoom->calcRoomDimensions();
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
        QCOMPARE(MudletApp::getMudletPath(enums::mainPath), qsl("%1/mudlet").arg(mConfigDir.path()));
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);
        mudlet::self()->mSkipDefaultPackageInstall = true;
        deleteProfileDirectory();

        mpHost = TestProfile::create(mProfileName, mLocalhost, mPort);
        QVERIFY(mpHost);
        QSignalSpy connected(&(mpHost->mTelnet), &cTelnet::signal_connected);
        QVERIFY2(connected.wait(3s), "could not connect to the telnet stub");
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
        // The rectangle the renderer builds at this zoom is 33 by 23 cells, so
        // asking for one ten times that in each direction and still not being
        // given the room leaves no room for argument about rounding.
        QVERIFY2(!pArea->getGridIndex().roomsInViewport(0, -kGenerousBound, kGenerousBound, -kGenerousBound, kGenerousBound).contains(kFarRoomId),
                 "the far room is close enough to be returned by a viewport query, so this would pass with or without the custom-line index");

        pMap->mRoomIdHash[pMap->mProfileName] = kPlayerRoomId;
        pMap->mNewMove = false;
        pMap->setDefaultAreaShown(false);

        mpHost->showHideOrCreateMapper(false);
        QVERIFY(pMap->mapper());
        T2DMap* p2dMap = pMap->mapper()->mp2dMap;
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

    // A custom line on one end of a two-way exit stands for both directions, so
    // the plain line from the other end must not be drawn over it. Which of
    // the two rooms paints last follows hash order, which Qt seeds per process,
    // so one pair could pass by luck where seventy cannot.
    void test_aPlainReturnExitDoesNotOverdrawACustomLine()
    {
        TMap* pMap = map();
        TRoomDB* pRoomDB = pMap->mpRoomDB.get();
        pMap->mapClear();

        const int areaId = pRoomDB->addArea(qsl("Overdraw Area"));
        QVERIFY(areaId > 0);
        QVERIFY(addRoomAt(kPlayerRoomId, areaId, 0, 0));

        QList<QPoint> westEnds;
        int nextId = 1000;
        for (int y = -9; y <= 9; y += 2) {
            for (int x = -14; x <= 10; x += 4) {
                const int westId = nextId++;
                const int eastId = nextId++;
                QVERIFY(addRoomAt(westId, areaId, x, y));
                QVERIFY(addRoomAt(eastId, areaId, x + 2, y));
                TRoom* pWest = pRoomDB->getRoom(westId);
                TRoom* pEast = pRoomDB->getRoom(eastId);
                QVERIFY(pWest && pEast);
                pWest->setEast(eastId);
                pEast->setWest(westId);
                pWest->customLines[qsl("e")] = QList<QPointF>{QPointF(x + 2, y)};
                pWest->customLinesColor[qsl("e")] = lineColour();
                pWest->customLinesStyle[qsl("e")] = Qt::SolidLine;
                pWest->customLinesArrow[qsl("e")] = false;
                pWest->calcRoomDimensions();
                westEnds << QPoint(x, y);
            }
        }

        pMap->mRoomIdHash[pMap->mProfileName] = kPlayerRoomId;
        pMap->mNewMove = false;
        pMap->setDefaultAreaShown(false);

        mpHost->showHideOrCreateMapper(false);
        QVERIFY(pMap->mpMapper);
        T2DMap* p2dMap = pMap->mpMapper->mp2dMap;
        QVERIFY(p2dMap);
        p2dMap->init();
        p2dMap->resize(kWidgetWidth, kWidgetHeight);
        p2dMap->mRoomID = kPlayerRoomId;
        p2dMap->mShiftMode = true;
        p2dMap->mPick = false;
        p2dMap->mAreaID = areaId;
        p2dMap->mMapCenterZ = 0;
        p2dMap->mMapCenterX = 0;
        p2dMap->mMapCenterY = 0;
        TArea* pArea = pRoomDB->getArea(areaId);
        QVERIFY(pArea);
        pArea->set2DMapZoom(kZoom);

        QPixmap target(kWidgetWidth, kWidgetHeight);
        target.fill(Qt::black);
        p2dMap->render(&target, QPoint(), QRegion(), QWidget::DrawWindowBackground);
        const QImage frame = target.toImage();
        QCOMPARE(p2dMap->getAreaId(), areaId);

        const double pixelsPerUnit = static_cast<double>(p2dMap->mRoomWidth);
        QVERIFY(pixelsPerUnit > 10.0);
        const auto colouredPixelsBetween = [&](const double fromX, const double toX, const int mapY) {
            int count = 0;
            const int screenY = qRound(p2dMap->mRY - mapY * pixelsPerUnit);
            for (int screenX = qRound(p2dMap->mRX + fromX * pixelsPerUnit); screenX <= qRound(p2dMap->mRX + toX * pixelsPerUnit); ++screenX) {
                for (int dy = -3; dy <= 3; ++dy) {
                    if (customLineColoured(frame.pixelColor(screenX, screenY + dy))) {
                        ++count;
                    }
                }
            }
            return count;
        };

        // Both ends draw a two-way exit's plain line the whole way across, so a
        // pair the plain line won shows next to none of the custom colour.
        QList<int> counts;
        for (const QPoint& westEnd : std::as_const(westEnds)) {
            counts << colouredPixelsBetween(westEnd.x() + 0.4, westEnd.x() + 1.6, westEnd.y());
        }
        const int most = *std::max_element(counts.cbegin(), counts.cend());
        QVERIFY2(most > 10, "no custom line was drawn at all");
        const int overdrawn = static_cast<int>(std::count_if(counts.cbegin(), counts.cend(), [most](const int count) {
            return count * 2 < most;
        }));
        if (overdrawn) {
            const QString framePath = qsl("%1/MapOffscreenCustomLineTest-overdraw.png").arg(QDir::tempPath());
            frame.save(framePath);
            QFAIL(qPrintable(qsl("%1 of %2 custom lines were drawn over by the plain exit back. The frame is at %3").arg(overdrawn).arg(westEnds.size()).arg(framePath)));
        }
    }

    // Only a custom line this view paints stands in for the plain exit back:
    // one from a room in another area or on another level is not painted
    // here, nor is one with no points, and an area exit's arrow is also its
    // speed-walk click target.
    void test_aPlainExitStaysWhereTheCustomLineBackIsNotPainted()
    {
        TMap* pMap = map();
        TRoomDB* pRoomDB = pMap->mpRoomDB.get();
        pMap->mapClear();

        const int areaId = pRoomDB->addArea(qsl("Shown Area"));
        const int otherAreaId = pRoomDB->addArea(qsl("Other Area"));
        QVERIFY(areaId > 0 && otherAreaId > 0);
        QVERIFY(addRoomAt(kPlayerRoomId, areaId, 0, 0));

        struct Pair
        {
            int plainId;
            int customId;
            int y;
        };
        const QList<Pair> pairs{{2001, 2002, 3}, {2003, 2004, 0}, {2005, 2006, -3}};
        constexpr int plainX = -8;
        QVERIFY(addRoomAt(pairs[0].plainId, areaId, plainX, pairs[0].y));
        QVERIFY(addRoomAt(pairs[0].customId, otherAreaId, plainX + 2, pairs[0].y));
        QVERIFY(addRoomAt(pairs[1].plainId, areaId, plainX, pairs[1].y));
        QVERIFY(addRoomAt(pairs[1].customId, areaId, plainX + 2, pairs[1].y));
        QVERIFY(pMap->setRoomCoordinates(pairs[1].customId, plainX + 2, pairs[1].y, 1));
        QVERIFY(addRoomAt(pairs[2].plainId, areaId, plainX, pairs[2].y));
        QVERIFY(addRoomAt(pairs[2].customId, areaId, plainX + 2, pairs[2].y));
        for (const Pair& pair : pairs) {
            TRoom* pPlain = pRoomDB->getRoom(pair.plainId);
            TRoom* pCustom = pRoomDB->getRoom(pair.customId);
            QVERIFY(pPlain && pCustom);
            pPlain->setEast(pair.customId);
            pCustom->setWest(pair.plainId);
        }
        const auto setCustomLinesBack = [&](const bool present) {
            for (const Pair& pair : pairs) {
                TRoom* pCustom = pRoomDB->getRoom(pair.customId);
                if (present) {
                    pCustom->customLines[qsl("w")] = (pair.customId == pairs[2].customId) ? QList<QPointF>{} : QList<QPointF>{QPointF(plainX, pair.y)};
                    pCustom->customLinesColor[qsl("w")] = lineColour();
                    pCustom->customLinesStyle[qsl("w")] = Qt::SolidLine;
                    pCustom->customLinesArrow[qsl("w")] = false;
                } else {
                    pCustom->customLines.remove(qsl("w"));
                }
                pCustom->calcRoomDimensions();
            }
        };

        pMap->mRoomIdHash[pMap->mProfileName] = kPlayerRoomId;
        pMap->mNewMove = false;
        pMap->setDefaultAreaShown(false);

        mpHost->showHideOrCreateMapper(false);
        QVERIFY(pMap->mpMapper);
        T2DMap* p2dMap = pMap->mpMapper->mp2dMap;
        QVERIFY(p2dMap);
        p2dMap->init();
        p2dMap->resize(kWidgetWidth, kWidgetHeight);
        p2dMap->mRoomID = kPlayerRoomId;
        p2dMap->mShiftMode = true;
        p2dMap->mPick = false;
        p2dMap->mAreaID = areaId;
        p2dMap->mMapCenterZ = 0;
        p2dMap->mMapCenterX = 0;
        p2dMap->mMapCenterY = 0;
        TArea* pArea = pRoomDB->getArea(areaId);
        QVERIFY(pArea);
        pArea->set2DMapZoom(kZoom);

        const auto renderFrame = [&]() {
            QPixmap target(kWidgetWidth, kWidgetHeight);
            target.fill(Qt::black);
            p2dMap->render(&target, QPoint(), QRegion(), QWidget::DrawWindowBackground);
            return target.toImage();
        };
        setCustomLinesBack(false);
        const QImage withoutLines = renderFrame();
        setCustomLinesBack(true);
        const QImage withLines = renderFrame();
        QCOMPARE(p2dMap->getAreaId(), areaId);

        const double pixelsPerUnit = static_cast<double>(p2dMap->mRoomWidth);
        QVERIFY(pixelsPerUnit > 10.0);
        for (const Pair& pair : pairs) {
            const int screenY = qRound(p2dMap->mRY - pair.y * pixelsPerUnit);
            // Counted, not compared: where both ends draw the plain line its antialiased edges are darker
            int drawnWithout = 0;
            int drawnWith = 0;
            for (int screenX = qRound(p2dMap->mRX + (plainX + 0.6) * pixelsPerUnit); screenX <= qRound(p2dMap->mRX + (plainX + 1.0) * pixelsPerUnit); ++screenX) {
                for (int dy = -3; dy <= 3; ++dy) {
                    if (withoutLines.pixelColor(screenX, screenY + dy) != QColor(Qt::black)) {
                        ++drawnWithout;
                    }
                    if (withLines.pixelColor(screenX, screenY + dy) != QColor(Qt::black)) {
                        ++drawnWith;
                    }
                }
            }
            QVERIFY2(drawnWithout > 0, qPrintable(qsl("room %1 drew no exit east even with no custom line back").arg(pair.plainId)));
            QVERIFY2(drawnWith * 2 >= drawnWithout,
                     qPrintable(qsl("room %1 lost its exit east to a custom line this view does not paint (%2 pixels drawn, %3 without it)").arg(pair.plainId).arg(drawnWith).arg(drawnWithout)));
        }
    }

    // The custom line back stands in for the plain line, not for the door on
    // it: that line draws only its own room's door, so this end's door must
    // still be drawn by this end.
    void test_aDoorStaysWhereTheCustomLineBackReplacesThePlainExit()
    {
        TMap* pMap = map();
        TRoomDB* pRoomDB = pMap->mpRoomDB.get();
        pMap->mapClear();

        const int areaId = pRoomDB->addArea(qsl("Door Area"));
        QVERIFY(areaId > 0);
        QVERIFY(addRoomAt(kPlayerRoomId, areaId, 0, 0));
        constexpr int doorRoomId = 3001;
        constexpr int customRoomId = 3002;
        constexpr int doorX = -8;
        constexpr int rowY = 3;
        QVERIFY(addRoomAt(doorRoomId, areaId, doorX, rowY));
        QVERIFY(addRoomAt(customRoomId, areaId, doorX + 2, rowY));
        TRoom* pDoorRoom = pRoomDB->getRoom(doorRoomId);
        TRoom* pCustomRoom = pRoomDB->getRoom(customRoomId);
        QVERIFY(pDoorRoom && pCustomRoom);
        pDoorRoom->setEast(customRoomId);
        pCustomRoom->setWest(doorRoomId);
        QVERIFY(pDoorRoom->setDoor(qsl("e"), 3));

        T2DMap* p2dMap = showAreaAtTheOrigin(areaId);
        QVERIFY(p2dMap);
        // Nothing else in the frame is magenta, nor is the custom line.
        p2dMap->mLockedDoorColor = QColor(255, 0, 255);
        const auto doorColoured = [](const QColor& pixel) {
            return pixel.red() > 60 && pixel.blue() > 60 && qAbs(pixel.red() - pixel.blue()) < 40 && pixel.green() * 2 < pixel.red();
        };

        removeCustomLine(pCustomRoom, qsl("w"));
        const QImage withoutLine = renderMapFrame(p2dMap);
        setCustomLine(pCustomRoom, qsl("w"), {QPointF(doorX, rowY)});
        const QImage withLine = renderMapFrame(p2dMap);
        QCOMPARE(p2dMap->getAreaId(), areaId);

        const double pixelsPerUnit = static_cast<double>(p2dMap->mRoomWidth);
        QVERIFY(pixelsPerUnit > 10.0);
        // The door sits most of the way along the near half of the exit, clear of both room squares.
        const int screenY = qRound(p2dMap->mRY - rowY * pixelsPerUnit);
        const int reach = qRound(0.6 * pixelsPerUnit);
        int doorWithout = 0;
        int doorWith = 0;
        for (int screenX = qRound(p2dMap->mRX + (doorX + 0.5) * pixelsPerUnit); screenX <= qRound(p2dMap->mRX + (doorX + 1.2) * pixelsPerUnit); ++screenX) {
            for (int dy = -reach; dy <= reach; ++dy) {
                if (doorColoured(withoutLine.pixelColor(screenX, screenY + dy))) {
                    ++doorWithout;
                }
                if (doorColoured(withLine.pixelColor(screenX, screenY + dy))) {
                    ++doorWith;
                }
            }
        }
        QVERIFY2(doorWithout > 10, qPrintable(qsl("room %1 drew no door east even with no custom line back (%2 pixels)").arg(doorRoomId).arg(doorWithout)));
        if (doorWith * 2 < doorWithout) {
            const QString framePath = qsl("%1/MapOffscreenCustomLineTest-door.png").arg(QDir::tempPath());
            withLine.save(framePath);
            QFAIL(qPrintable(qsl("room %1 lost its locked door east to the custom line back (%2 pixels drawn, %3 without it). The frame is at %4")
                                     .arg(doorRoomId)
                                     .arg(doorWith)
                                     .arg(doorWithout)
                                     .arg(framePath)));
        }
    }

    // The room at the other end is culled when its custom line never comes
    // near the widget, so that line is not painted and cannot stand in for
    // the plain exit from a room that is on screen.
    void test_aPlainExitStaysWhereTheCustomLineBackIsOffTheWidget()
    {
        TMap* pMap = map();
        TRoomDB* pRoomDB = pMap->mpRoomDB.get();
        pMap->mapClear();

        const int areaId = pRoomDB->addArea(qsl("Off Widget Area"));
        QVERIFY(areaId > 0);
        QVERIFY(addRoomAt(kPlayerRoomId, areaId, 0, 0));
        constexpr int plainRoomId = 3101;
        constexpr int customRoomId = 3102;
        constexpr int plainX = 8;
        constexpr int customX = 20;
        constexpr int lineEndX = 18;
        constexpr int rowY = 4;
        QVERIFY(addRoomAt(plainRoomId, areaId, plainX, rowY));
        QVERIFY(addRoomAt(customRoomId, areaId, customX, rowY));
        TRoom* pPlainRoom = pRoomDB->getRoom(plainRoomId);
        TRoom* pCustomRoom = pRoomDB->getRoom(customRoomId);
        QVERIFY(pPlainRoom && pCustomRoom);
        pPlainRoom->setEast(customRoomId);
        pCustomRoom->setWest(plainRoomId);

        T2DMap* p2dMap = showAreaAtTheOrigin(areaId);
        QVERIFY(p2dMap);

        removeCustomLine(pCustomRoom, qsl("w"));
        const QImage withoutLine = renderMapFrame(p2dMap);
        setCustomLine(pCustomRoom, qsl("w"), {QPointF(lineEndX, rowY)});
        const QImage withLine = renderMapFrame(p2dMap);
        QCOMPARE(p2dMap->getAreaId(), areaId);

        const double pixelsPerUnit = static_cast<double>(p2dMap->mRoomWidth);
        QVERIFY(pixelsPerUnit > 10.0);
        // The premise: the whole custom line, and so its room, is right of the widget.
        QVERIFY2(p2dMap->mRX + lineEndX * pixelsPerUnit > kWidgetWidth, "the custom line back reaches the widget, so it would rightly stand in for the plain exit");
        QVERIFY2(p2dMap->mRX + (plainX + 5) * pixelsPerUnit < kWidgetWidth, "the stretch of plain exit this counts is not on the widget");

        const int screenY = qRound(p2dMap->mRY - rowY * pixelsPerUnit);
        int drawnWithout = 0;
        int drawnWith = 0;
        for (int screenX = qRound(p2dMap->mRX + (plainX + 1) * pixelsPerUnit); screenX <= qRound(p2dMap->mRX + (plainX + 5) * pixelsPerUnit); ++screenX) {
            for (int dy = -3; dy <= 3; ++dy) {
                if (withoutLine.pixelColor(screenX, screenY + dy) != QColor(Qt::black)) {
                    ++drawnWithout;
                }
                if (withLine.pixelColor(screenX, screenY + dy) != QColor(Qt::black)) {
                    ++drawnWith;
                }
            }
        }
        QVERIFY2(drawnWithout > 0, qPrintable(qsl("room %1 drew no exit east even with no custom line back").arg(plainRoomId)));
        if (drawnWith * 2 < drawnWithout) {
            const QString framePath = qsl("%1/MapOffscreenCustomLineTest-offwidget.png").arg(QDir::tempPath());
            withLine.save(framePath);
            QFAIL(qPrintable(qsl("room %1 lost its exit east to a custom line drawn nowhere on the widget (%2 pixels drawn, %3 without it). The frame is at %4")
                                     .arg(plainRoomId)
                                     .arg(drawnWith)
                                     .arg(drawnWithout)
                                     .arg(framePath)));
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

    // The index is read once per frame, so a room left in it after its last
    // custom line has gone costs a room lookup and a cull test on every frame
    // for the rest of the session - a script that clears lines across a lot of
    // rooms pays that for all of them.
    void test_customLineIndexDropsARoomThatLostItsLastLine()
    {
        TMap* pMap = map();
        TRoomDB* pRoomDB = pMap->mpRoomDB.get();
        pMap->mapClear();

        const int areaId = pRoomDB->addArea(qsl("Losing Lines Area"));
        QVERIFY(areaId > 0);
        QVERIFY(addRoomAt(1, areaId, 0, 0));

        TArea* pArea = pRoomDB->getArea(areaId);
        QVERIFY(pArea);
        TRoom* pRoom = pRoomDB->getRoom(1);
        QVERIFY(pRoom);

        pRoom->customLines[qsl("n")] = QList<QPointF>{QPointF(0.0, 3.0)};
        pRoom->customLines[qsl("s")] = QList<QPointF>{QPointF(0.0, -3.0)};
        pRoom->calcRoomDimensions();
        QVERIFY(pArea->getCustomLineRoomsForZ(0).contains(1));

        // Dropping one line and recomputing is what removeCustomLine() does;
        // the room still owes the renderer the other line's pixels.
        pRoom->customLines.remove(qsl("n"));
        pRoom->calcRoomDimensions();
        QVERIFY2(pArea->getCustomLineRoomsForZ(0).contains(1), "a room that still has a custom line was dropped from the index");

        pRoom->customLines.remove(qsl("s"));
        pRoom->calcRoomDimensions();
        QVERIFY2(!pArea->getCustomLineRoomsForZ(0).contains(1), "a room that lost its last custom line stayed in the index");
    }

    // The custom exit line properties dialog carried WA_DeleteOnClose and was
    // run with exec(), so the combo boxes read out of it once exec() returned
    // had already been freed. Reading them from an accepted() lambda instead
    // means the slot now hands control back with the dialog still up, which is
    // what this pins - and the mapper is no longer frozen behind it (#6754)
    void test_theCustomLinePropertiesDialogDoesNotBlockTheRestOfMudlet()
    {
        TMap* pMap = map();
        TRoomDB* pRoomDB = pMap->mpRoomDB.get();
        pMap->mapClear();

        const int areaId = pRoomDB->addArea(qsl("Line Properties Area"));
        QVERIFY(areaId > 0);
        QVERIFY(addRoomAt(1, areaId, 0, 0));
        QVERIFY(addRoomAt(2, areaId, 0, 1));

        TRoom* pRoom = pRoomDB->getRoom(1);
        QVERIFY(pRoom);
        pRoom->setNorth(2);
        pRoom->customLines[qsl("n")] = QList<QPointF>{QPointF(0.0, 1.0)};
        pRoom->customLinesColor[qsl("n")] = lineColour();
        pRoom->customLinesStyle[qsl("n")] = Qt::SolidLine;
        pRoom->customLinesArrow[qsl("n")] = false;
        pRoom->calcRoomDimensions();

        mpHost->showHideOrCreateMapper(false);
        QVERIFY(pMap->mapper());
        T2DMap* p2dMap = pMap->mapper()->mp2dMap;
        QVERIFY(p2dMap);
        // What clicking a custom line leaves behind for the context menu
        p2dMap->mCustomLineSelectedRoom = 1;
        p2dMap->mCustomLineSelectedExit = qsl("n");

        // Armed before the call, because a dialog holding its own event loop
        // would give the test no other moment to reach it. Nothing pumps the
        // event loop between here and the slot returning, so this can only
        // have run by then if the slot ran a loop itself.
        mDialogAnswered = false;
        QTimer::singleShot(0ms, p2dMap, [this, p2dMap]() {
            auto* pDialog = p2dMap->findChild<QDialog*>();
            QVERIFY(pDialog);
            auto* pLineStyle = pDialog->findChild<QComboBox*>(qsl("lineStyle"));
            QVERIFY(pLineStyle);
            pLineStyle->setCurrentIndex(pLineStyle->findData(static_cast<int>(Qt::DashLine)));
            pDialog->accept();
            mDialogAnswered = true;
        });

        p2dMap->slot_customLineProperties();
        QVERIFY2(!mDialogAnswered, "the slot did not hand control back until the dialog had been answered, so everything else in Mudlet was frozen behind it");
        auto* pDialog = p2dMap->findChild<QDialog*>();
        QVERIFY2(pDialog, "the dialog was not left open for the mapper to go on running behind");
        QVERIFY2(pDialog->isVisible(), "the dialog was never put on screen, so there was nothing there for the user to answer");

        QTRY_VERIFY2(mDialogAnswered, "the custom line properties dialog never appeared");
        QTRY_COMPARE(pRoom->customLinesStyle.value(qsl("n")), Qt::DashLine);

        // The dialog closes itself, but the deletion that follows is posted
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        QVERIFY2(!p2dMap->findChild<QDialog*>(), "the dialog outlived its own closing");
    }
};

#include "MapOffscreenCustomLineTest.moc"
MUDLET_GROUPED_TEST_MAIN(MapOffscreenCustomLineTest)
