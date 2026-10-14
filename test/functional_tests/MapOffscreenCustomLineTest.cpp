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
#include <QFutureWatcher>
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

    // Antialiased plain exit lines are a grey blended with black, so all three channels stay level
    static bool plainLineColoured(const QColor& pixel)
    {
        const int brightest = std::max({pixel.red(), pixel.green(), pixel.blue()});
        return brightest > 40 && brightest - std::min({pixel.red(), pixel.green(), pixel.blue()}) <= 12;
    }

    static bool drawnOn(const QColor& pixel) { return pixel != QColor(Qt::black); }

    // A locked door is drawn in ExitAppearance::doorColor(3), a dark red; so is a room with no environment, so
    // the door tests paint their rooms with paintRoomsBlue().
    static bool doorColoured(const QColor& pixel) { return pixel.red() > 60 && pixel.green() * 3 < pixel.red() && pixel.blue() * 3 < pixel.red(); }

    void paintRoomsBlue(const QList<int>& roomIds) const
    {
        constexpr int blueEnvironment = 300;
        map()->mCustomEnvColors[blueEnvironment] = QColor(Qt::blue);
        for (const int roomId : roomIds) {
            map()->mpRoomDB->getRoom(roomId)->environment = blueEnvironment;
        }
    }

    // Pixels matching wanted in the image exportAreaToImage() writes for the area, or -1 if there is none.
    int countInExport(T2DMap* p2dMap, const int areaId, const QString& name, bool (*wanted)(const QColor&)) const
    {
        const QString filePath = qsl("%1/%2.png").arg(mConfigDir.path(), name);
        const auto [exported, message] = p2dMap->exportAreaToImage(areaId, filePath, 0, 1.0);
        if (!exported) {
            qWarning().noquote() << "exportAreaToImage() failed:" << message;
            return -1;
        }
        QImage image;
        // The file is written by a QtConcurrent task, and the widget must outlive it
        if (!QTest::qWaitFor(
                    [&]() {
                        return image.load(filePath);
                    },
                    10000)
            || !QTest::qWaitFor(
                    [&]() {
                        return p2dMap->findChildren<QFutureWatcherBase*>(QString(), Qt::FindDirectChildrenOnly).isEmpty();
                    },
                    10000)) {
            qWarning().noquote() << "the exported image" << filePath << "could not be read back";
            return -1;
        }
        int count = 0;
        for (int y = 0; y < image.height(); ++y) {
            for (int x = 0; x < image.width(); ++x) {
                if (wanted(image.pixelColor(x, y))) {
                    ++count;
                }
            }
        }
        return count;
    }

    // A rendered frame with the mapping it was drawn with, which can change from one frame to the next.
    struct Frame
    {
        QImage image;
        double rx = 0.0;
        double ry = 0.0;
        double pixelsPerUnit = 0.0;

        QPoint at(const QPointF& mapPoint) const { return QPoint(qRound(rx + mapPoint.x() * pixelsPerUnit), qRound(ry - mapPoint.y() * pixelsPerUnit)); }

        // Pixels matching wanted within radius of the stretch [fromT, toT] of the line from 'from' to 'to'.
        int countAlong(const QPointF& from, const QPointF& to, const double fromT, const double toT, const int radius, bool (*wanted)(const QColor&)) const
        {
            QSet<QPoint> counted;
            const int steps = std::max(1, qRound(QLineF(at(from), at(to)).length() * (toT - fromT)));
            for (int step = 0; step <= steps; ++step) {
                const QPoint centre = at(from + (to - from) * (fromT + (toT - fromT) * step / steps));
                for (int dy = -radius; dy <= radius; ++dy) {
                    for (int dx = -radius; dx <= radius; ++dx) {
                        const QPoint pixel = centre + QPoint(dx, dy);
                        if (image.rect().contains(pixel) && wanted(image.pixelColor(pixel))) {
                            counted.insert(pixel);
                        }
                    }
                }
            }
            return static_cast<int>(counted.size());
        }

        QString savedFor(const QString& name) const
        {
            const QString framePath = qsl("%1/MapOffscreenCustomLineTest-%2.png").arg(QDir::tempPath(), name);
            return image.save(framePath) ? qsl("The frame is at %1").arg(framePath) : qsl("The frame could not be saved to %1").arg(framePath);
        }
    };

    // The mapper showing areaId at kZoom, centred on the origin with the player there.
    T2DMap* showAreaAtTheOrigin(const int areaId) const
    {
        TMap* pMap = map();
        TArea* pArea = pMap->mpRoomDB->getArea(areaId);
        if (!pArea) {
            qWarning() << "showAreaAtTheOrigin(): there is no area" << areaId;
            return nullptr;
        }
        pMap->mRoomIdHash[pMap->mProfileName] = kPlayerRoomId;
        pMap->mNewMove = false;
        pMap->setDefaultAreaShown(false);
        mpHost->showHideOrCreateMapper(false);
        if (!pMap->mapper() || !pMap->mapper()->mp2dMap) {
            qWarning() << "showAreaAtTheOrigin(): the profile has no 2D mapper";
            return nullptr;
        }
        T2DMap* p2dMap = pMap->mapper()->mp2dMap;
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
        return p2dMap;
    }

    static Frame renderMapFrame(T2DMap* p2dMap)
    {
        QPixmap target(kWidgetWidth, kWidgetHeight);
        target.fill(Qt::black);
        p2dMap->render(&target, QPoint(), QRegion(), QWidget::DrawWindowBackground);
        return Frame{target.toImage(), static_cast<double>(p2dMap->mRX), static_cast<double>(p2dMap->mRY), static_cast<double>(p2dMap->mRoomWidth)};
    }

    static void setCustomLine(TRoom* pRoom, const QString& exitKey, const QList<QPointF>& points, const Qt::PenStyle style = Qt::SolidLine)
    {
        pRoom->customLines[exitKey] = points;
        pRoom->customLinesColor[exitKey] = lineColour();
        // QMap::value() default-constructs a Qt::PenStyle as Qt::NoPen, so a
        // line without an explicit style draws nothing at all.
        pRoom->customLinesStyle[exitKey] = style;
        pRoom->customLinesArrow[exitKey] = false;
        pRoom->calcRoomDimensions();
    }

    static void removeCustomLine(TRoom* pRoom, const QString& exitKey)
    {
        pRoom->customLines.remove(exitKey);
        pRoom->calcRoomDimensions();
    }

    // Two rooms linked both ways, from (x, y) through direction to the room two cells along it.
    bool addTwoWayPair(const int fromId, const int toId, const int areaId, const QPoint& from, const int direction, const int reverse) const
    {
        const QVector3D step = TMap::scmUnitVectors.value(direction);
        if (!addRoomAt(fromId, areaId, from.x(), from.y()) || !addRoomAt(toId, areaId, from.x() + 2 * qRound(step.x()), from.y() - 2 * qRound(step.y()))) {
            return false;
        }
        return map()->mpRoomDB->getRoom(fromId)->setExit(toId, direction) && map()->mpRoomDB->getRoom(toId)->setExit(fromId, reverse);
    }

    static QPointF positionOf(const TRoom* pRoom) { return QPointF(pRoom->x(), pRoom->y()); }

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

    // Which room of a pair paints last follows hash order, seeded per process, so many pairs keep a lucky pass unlikely.
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
                QVERIFY(addTwoWayPair(westId, eastId, areaId, QPoint(x, y), DIR_EAST, DIR_WEST));
                setCustomLine(pRoomDB->getRoom(westId), qsl("e"), {QPointF(x + 2, y)});
                westEnds << QPoint(x, y);
            }
        }

        T2DMap* p2dMap = showAreaAtTheOrigin(areaId);
        QVERIFY(p2dMap);
        const Frame frame = renderMapFrame(p2dMap);
        QCOMPARE(p2dMap->getAreaId(), areaId);
        QVERIFY(frame.pixelsPerUnit > 10.0);

        // A pair the plain line won shows next to none of the custom colour.
        QList<int> counts;
        for (const QPoint& westEnd : std::as_const(westEnds)) {
            counts << frame.countAlong(westEnd, westEnd + QPointF(2, 0), 0.2, 0.8, 3, customLineColoured);
        }
        const int most = *std::max_element(counts.cbegin(), counts.cend());
        QVERIFY2(most > 10, "no custom line was drawn at all");
        const int overdrawn = static_cast<int>(std::count_if(counts.cbegin(), counts.cend(), [most](const int count) {
            return count * 2 < most;
        }));
        if (overdrawn) {
            QFAIL(qPrintable(qsl("%1 of %2 custom lines were drawn over by the plain exit back. %3").arg(overdrawn).arg(westEnds.size()).arg(frame.savedFor(qsl("overdraw")))));
        }
    }

    // A custom line drawn dotted shows any plain line in its gaps, whichever room paints last. All eight
    // directions, from a plain end with and without a custom line of its own on another exit.
    void test_noPlainLineShowsUnderAStraightCustomLineBackInAnyDirection()
    {
        TMap* pMap = map();
        TRoomDB* pRoomDB = pMap->mpRoomDB.get();
        pMap->mapClear();

        const int areaId = pRoomDB->addArea(qsl("Directions Area"));
        QVERIFY(areaId > 0);
        QVERIFY(addRoomAt(kPlayerRoomId, areaId, 0, 0));

        const QList<int> directions{DIR_NORTH, DIR_NORTHEAST, DIR_EAST, DIR_SOUTHEAST, DIR_SOUTH, DIR_SOUTHWEST, DIR_WEST, DIR_NORTHWEST};
        struct Pair
        {
            int plainId;
            int customId;
            QString description;
        };
        QList<Pair> pairs;
        int nextId = 1000;
        int cell = 0;
        for (const bool plainEndHasACustomLine : {false, true}) {
            for (const int direction : directions) {
                const int reverse = TMap::scmReverseDirections.value(direction);
                // Cells three wide in rows y = 4 and below, clear of the area name overlay and of the player at the origin.
                const QPoint cellCorner(-14 + 3 * (cell % 8) + (cell % 8 >= 4 ? 3 : 0), 2 - 6 * (cell / 8));
                ++cell;
                const QVector3D step = TMap::scmUnitVectors.value(direction);
                const QPoint plainAt(cellCorner.x() + (step.x() < 0 ? 2 : 0), cellCorner.y() + (step.y() > 0 ? 2 : 0));
                const int plainId = nextId++;
                const int customId = nextId++;
                QVERIFY(addTwoWayPair(plainId, customId, areaId, plainAt, direction, reverse));
                TRoom* pPlain = pRoomDB->getRoom(plainId);
                setCustomLine(pRoomDB->getRoom(customId), TRoom::dirCodeToShortString(reverse), {positionOf(pPlain)}, Qt::DotLine);
                if (plainEndHasACustomLine) {
                    setCustomLine(pPlain, qsl("up"), {positionOf(pPlain) + QPointF(-0.3 * step.y(), -0.3 * step.x())});
                }
                pairs << Pair{plainId, customId, qsl("%1 exit%2").arg(TRoom::dirCodeToShortString(direction), plainEndHasACustomLine ? qsl(", plain end with a custom line of its own") : QString())};
            }
        }

        T2DMap* p2dMap = showAreaAtTheOrigin(areaId);
        QVERIFY(p2dMap);
        const Frame frame = renderMapFrame(p2dMap);
        QCOMPARE(p2dMap->getAreaId(), areaId);
        QVERIFY(frame.pixelsPerUnit > 10.0);

        QStringList showingPlainLine;
        for (const Pair& pair : std::as_const(pairs)) {
            const QPointF plainAt = positionOf(pRoomDB->getRoom(pair.plainId));
            const QPointF customAt = positionOf(pRoomDB->getRoom(pair.customId));
            QVERIFY2(frame.countAlong(customAt, plainAt, 0.3, 0.7, 1, customLineColoured) > 5,
                     qPrintable(qsl("the custom line for the %1 was not drawn. %2").arg(pair.description, frame.savedFor(qsl("directions")))));
            if (frame.countAlong(customAt, plainAt, 0.3, 0.7, 1, plainLineColoured) > 2) {
                showingPlainLine << pair.description;
            }
        }
        if (!showingPlainLine.isEmpty()) {
            QFAIL(qPrintable(qsl("a plain line was drawn under or over the custom line back for: %1. %2").arg(showingPlainLine.join(qsl("; ")), frame.savedFor(qsl("directions")))));
        }
    }

    // Two exits between the same two rooms draw the same plain line, so a custom line for one
    // of them has to stand in for both, from either end.
    void test_aSecondExitBetweenTheSameRoomsDoesNotShowUnderACustomLine()
    {
        TMap* pMap = map();
        TRoomDB* pRoomDB = pMap->mpRoomDB.get();
        pMap->mapClear();

        const int areaId = pRoomDB->addArea(qsl("Sibling Exits Area"));
        QVERIFY(areaId > 0);
        QVERIFY(addRoomAt(kPlayerRoomId, areaId, 0, 0));
        QList<QPair<int, int>> pairs;
        int nextId = 1000;
        for (int x = -14; x <= 10; x += 4) {
            const int westId = nextId++;
            const int eastId = nextId++;
            QVERIFY(addTwoWayPair(westId, eastId, areaId, QPoint(x, -4), DIR_EAST, DIR_WEST));
            QVERIFY(pRoomDB->getRoom(westId)->setExit(eastId, DIR_NORTHEAST));
            QVERIFY(pRoomDB->getRoom(eastId)->setExit(westId, DIR_SOUTHWEST));
            setCustomLine(pRoomDB->getRoom(eastId), qsl("w"), {QPointF(x, -4)}, Qt::DotLine);
            pairs << qMakePair(westId, eastId);
        }

        T2DMap* p2dMap = showAreaAtTheOrigin(areaId);
        QVERIFY(p2dMap);
        const Frame frame = renderMapFrame(p2dMap);
        QCOMPARE(p2dMap->getAreaId(), areaId);
        QVERIFY(frame.pixelsPerUnit > 10.0);
        for (const auto& [westId, eastId] : std::as_const(pairs)) {
            const QPointF westAt = positionOf(pRoomDB->getRoom(westId));
            const QPointF eastAt = positionOf(pRoomDB->getRoom(eastId));
            const int plainPixels = frame.countAlong(westAt, eastAt, 0.3, 0.7, 1, plainLineColoured);
            QVERIFY2(
                    plainPixels <= 2,
                    qPrintable(
                            qsl("rooms %1 and %2 drew a plain line along the custom line between them (%3 pixels). %4").arg(westId).arg(eastId).arg(plainPixels).arg(frame.savedFor(qsl("siblings")))));
        }
    }

    // Only a custom line this view paints stands in for the plain exit back: not one in another area
    // or on another level, nor one with no points.
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
        for (const Pair& pair : pairs) {
            QVERIFY(addTwoWayPair(pair.plainId, pair.customId, areaId, QPoint(plainX, pair.y), DIR_EAST, DIR_WEST));
        }
        QVERIFY(pMap->setRoomArea(pairs[0].customId, otherAreaId));
        QVERIFY(pMap->setRoomCoordinates(pairs[1].customId, plainX + 2, pairs[1].y, 1));

        T2DMap* p2dMap = showAreaAtTheOrigin(areaId);
        QVERIFY(p2dMap);
        for (const Pair& pair : pairs) {
            removeCustomLine(pRoomDB->getRoom(pair.customId), qsl("w"));
        }
        const Frame withoutLines = renderMapFrame(p2dMap);
        for (const Pair& pair : pairs) {
            setCustomLine(pRoomDB->getRoom(pair.customId), qsl("w"), (pair.customId == pairs[2].customId) ? QList<QPointF>{} : QList<QPointF>{QPointF(plainX, pair.y)});
        }
        const Frame withLines = renderMapFrame(p2dMap);
        QCOMPARE(p2dMap->getAreaId(), areaId);
        QVERIFY(withLines.pixelsPerUnit > 10.0);

        for (const Pair& pair : pairs) {
            const QPointF plainAt(plainX, pair.y);
            // Counted, not compared: where both ends draw the plain line its antialiased edges are darker
            const int drawnWithout = withoutLines.countAlong(plainAt, plainAt + QPointF(2, 0), 0.3, 0.5, 3, drawnOn);
            const int drawnWith = withLines.countAlong(plainAt, plainAt + QPointF(2, 0), 0.3, 0.5, 3, drawnOn);
            QVERIFY2(drawnWithout > 0, qPrintable(qsl("room %1 drew no exit east even with no custom line back").arg(pair.plainId)));
            QVERIFY2(drawnWith * 2 >= drawnWithout,
                     qPrintable(qsl("room %1 lost its exit east to a custom line this view does not paint (%2 pixels drawn, %3 without it)").arg(pair.plainId).arg(drawnWith).arg(drawnWithout)));
        }
    }

    // A custom line back stands in for the plain line, not for the door on it, which only this end draws.
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
        const QPoint doorAt(-8, 3);
        QVERIFY(addTwoWayPair(doorRoomId, customRoomId, areaId, doorAt, DIR_EAST, DIR_WEST));
        TRoom* pDoorRoom = pRoomDB->getRoom(doorRoomId);
        TRoom* pCustomRoom = pRoomDB->getRoom(customRoomId);
        QVERIFY(pDoorRoom->setDoor(qsl("e"), 3));
        paintRoomsBlue({kPlayerRoomId, doorRoomId, customRoomId});

        T2DMap* p2dMap = showAreaAtTheOrigin(areaId);
        QVERIFY(p2dMap);

        removeCustomLine(pCustomRoom, qsl("w"));
        const Frame withoutLine = renderMapFrame(p2dMap);
        setCustomLine(pCustomRoom, qsl("w"), {QPointF(doorAt)});
        const Frame withLine = renderMapFrame(p2dMap);
        QCOMPARE(p2dMap->getAreaId(), areaId);
        QVERIFY(withLine.pixelsPerUnit > 10.0);

        // The door sits on the near half of the exit.
        const int reach = qRound(0.6 * withLine.pixelsPerUnit);
        const int doorWithout = withoutLine.countAlong(doorAt, doorAt + QPointF(2, 0), 0.25, 0.6, reach, doorColoured);
        const int doorWith = withLine.countAlong(doorAt, doorAt + QPointF(2, 0), 0.25, 0.6, reach, doorColoured);
        QVERIFY2(doorWithout > 10, qPrintable(qsl("room %1 drew no door east even with no custom line back (%2 pixels)").arg(doorRoomId).arg(doorWithout)));
        if (doorWith * 2 < doorWithout) {
            QFAIL(qPrintable(qsl("room %1 lost its locked door east to the custom line back (%2 pixels drawn, %3 without it). %4")
                                     .arg(doorRoomId)
                                     .arg(doorWith)
                                     .arg(doorWithout)
                                     .arg(withLine.savedFor(qsl("door")))));
        }
    }

    // A custom line back that bends away does not cover the plain line, so that line and its door stay;
    // a door left on a stretch of map with no line under it reads as a door to nowhere.
    void test_aCustomLineBackThatBendsAwayLeavesThePlainExitAndItsDoor()
    {
        TMap* pMap = map();
        TRoomDB* pRoomDB = pMap->mpRoomDB.get();
        pMap->mapClear();

        const int areaId = pRoomDB->addArea(qsl("Bent Line Area"));
        QVERIFY(areaId > 0);
        QVERIFY(addRoomAt(kPlayerRoomId, areaId, 0, 0));
        constexpr int doorRoomId = 3201;
        constexpr int customRoomId = 3202;
        const QPoint doorAt(-8, 1);
        QVERIFY(addTwoWayPair(doorRoomId, customRoomId, areaId, doorAt, DIR_EAST, DIR_WEST));
        QVERIFY(pRoomDB->getRoom(doorRoomId)->setDoor(qsl("e"), 3));
        paintRoomsBlue({kPlayerRoomId, doorRoomId, customRoomId});
        setCustomLine(pRoomDB->getRoom(customRoomId), qsl("w"), {QPointF(doorAt) + QPointF(2, -3), QPointF(doorAt) + QPointF(0, -3), QPointF(doorAt)});

        T2DMap* p2dMap = showAreaAtTheOrigin(areaId);
        QVERIFY(p2dMap);
        const Frame frame = renderMapFrame(p2dMap);
        QCOMPARE(p2dMap->getAreaId(), areaId);
        QVERIFY(frame.pixelsPerUnit > 10.0);

        const int reach = qRound(0.6 * frame.pixelsPerUnit);
        QVERIFY2(frame.countAlong(doorAt, doorAt + QPointF(2, 0), 0.25, 0.6, reach, doorColoured) > 10,
                 qPrintable(qsl("room %1 drew no door east. %2").arg(doorRoomId).arg(frame.savedFor(qsl("bent")))));
        const int plainPixels = frame.countAlong(doorAt, doorAt + QPointF(2, 0), 0.6, 0.75, 1, plainLineColoured);
        QVERIFY2(plainPixels > 5,
                 qPrintable(qsl("the exit between rooms %1 and %2 has no straight line, though the custom line back bends away from it (%3 pixels). %4")
                                    .arg(doorRoomId)
                                    .arg(customRoomId)
                                    .arg(plainPixels)
                                    .arg(frame.savedFor(qsl("bent")))));
    }

    // A straight custom line between rooms off a compass bearing leaves its first half-room segment off the plain
    // line, which then only runs beside it.
    void test_aStraightCustomLineOffACompassBearingHasNoPlainLineBesideIt()
    {
        TMap* pMap = map();
        TRoomDB* pRoomDB = pMap->mpRoomDB.get();
        pMap->mapClear();

        const int areaId = pRoomDB->addArea(qsl("Off Bearing Area"));
        QVERIFY(areaId > 0);
        QVERIFY(addRoomAt(kPlayerRoomId, areaId, 0, 0));
        struct Pair
        {
            int customId;
            int plainId;
            QPoint from;
            QPoint to;
            int reverse;
        };
        QList<Pair> pairs;
        int nextId = 3501;
        for (int y = -9; y <= 1; y += 5) {
            for (int x = -14; x <= 6; x += 5) {
                const bool eastward = ((x + y) / 5) % 2 == 0;
                const Pair pair{nextId, nextId + 1, QPoint(x, y), QPoint(x + (eastward ? 3 : 2), y + 1), eastward ? DIR_WEST : DIR_SOUTHWEST};
                nextId += 2;
                QVERIFY(addRoomAt(pair.customId, areaId, pair.from.x(), pair.from.y()));
                QVERIFY(addRoomAt(pair.plainId, areaId, pair.to.x(), pair.to.y()));
                QVERIFY(pRoomDB->getRoom(pair.customId)->setExit(pair.plainId, eastward ? DIR_EAST : DIR_NORTHEAST));
                setCustomLine(pRoomDB->getRoom(pair.customId), eastward ? qsl("e") : qsl("ne"), {QPointF(pair.to)});
                pairs << pair;
            }
        }

        T2DMap* p2dMap = showAreaAtTheOrigin(areaId);
        QVERIFY(p2dMap);
        const Frame withoutReturn = renderMapFrame(p2dMap);
        for (const Pair& pair : std::as_const(pairs)) {
            QVERIFY(pRoomDB->getRoom(pair.plainId)->setExit(pair.customId, pair.reverse));
        }
        const Frame withReturn = renderMapFrame(p2dMap);
        QCOMPARE(p2dMap->getAreaId(), areaId);
        QVERIFY(withReturn.pixelsPerUnit > 10.0);

        for (const Pair& pair : std::as_const(pairs)) {
            const int plainWithout = withoutReturn.countAlong(pair.from, pair.to, 0.2, 0.8, 2, plainLineColoured);
            const int plainWith = withReturn.countAlong(pair.from, pair.to, 0.2, 0.8, 2, plainLineColoured);
            QVERIFY2(plainWith - plainWithout <= 3,
                     qPrintable(qsl("room %1 drew its plain exit back beside room %2's straight custom line (%3 plain line pixels, %4 without that exit). %5")
                                        .arg(pair.plainId)
                                        .arg(pair.customId)
                                        .arg(plainWith)
                                        .arg(plainWithout)
                                        .arg(withReturn.savedFor(qsl("offbearing")))));
        }
    }

    // A custom line back that stops short of this room leaves the rest of the way to the plain line,
    // also when it stops off the widget and another custom line of that room crosses the widget.
    void test_aCustomLineBackThatStopsShortLeavesThePlainExit()
    {
        TMap* pMap = map();
        TRoomDB* pRoomDB = pMap->mpRoomDB.get();
        pMap->mapClear();

        const int areaId = pRoomDB->addArea(qsl("Short Line Area"));
        QVERIFY(areaId > 0);
        QVERIFY(addRoomAt(kPlayerRoomId, areaId, 0, 0));
        QVERIFY(addRoomAt(3301, areaId, -12, 2));
        QVERIFY(addRoomAt(3302, areaId, -2, 2));
        QVERIFY(pRoomDB->getRoom(3301)->setExit(3302, DIR_EAST));
        QVERIFY(pRoomDB->getRoom(3302)->setExit(3301, DIR_WEST));
        setCustomLine(pRoomDB->getRoom(3302), qsl("w"), {QPointF(-3, 2)});
        // The other end is right of the widget, its line back too; its line north loops into the widget.
        QVERIFY(addRoomAt(3303, areaId, 8, -6));
        QVERIFY(addRoomAt(3304, areaId, 20, -6));
        QVERIFY(addRoomAt(3305, areaId, 20, -4));
        QVERIFY(pRoomDB->getRoom(3303)->setExit(3304, DIR_EAST));
        QVERIFY(pRoomDB->getRoom(3304)->setExit(3303, DIR_WEST));
        QVERIFY(pRoomDB->getRoom(3304)->setExit(3305, DIR_NORTH));
        setCustomLine(pRoomDB->getRoom(3304), qsl("w"), {QPointF(18, -6)});
        setCustomLine(pRoomDB->getRoom(3304), qsl("n"), {QPointF(20, -5), QPointF(10, -5), QPointF(10, -4), QPointF(20, -4)});

        T2DMap* p2dMap = showAreaAtTheOrigin(areaId);
        QVERIFY(p2dMap);
        const Frame frame = renderMapFrame(p2dMap);
        QCOMPARE(p2dMap->getAreaId(), areaId);
        QVERIFY(frame.pixelsPerUnit > 10.0);
        QVERIFY2(frame.at(QPointF(18, -6)).x() > kWidgetWidth, "the line back from room 3304 reaches the widget");

        const int nearPixels = frame.countAlong(QPointF(-12, 2), QPointF(-2, 2), 0.1, 0.8, 1, plainLineColoured);
        const int offWidgetPixels = frame.countAlong(QPointF(8, -6), QPointF(13, -6), 0.2, 1.0, 1, plainLineColoured);
        QVERIFY2(nearPixels > 20, qPrintable(qsl("room 3301 drew no exit east, though the custom line back stops short of it (%1 pixels). %2").arg(nearPixels).arg(frame.savedFor(qsl("short")))));
        QVERIFY2(offWidgetPixels > 20,
                 qPrintable(qsl("room 3303 drew no exit east to a room whose line back is off the widget (%1 pixels). %2").arg(offWidgetPixels).arg(frame.savedFor(qsl("short")))));
    }

    // A custom line back that ends off the widget, short of a room on screen, leaves that room its plain exit.
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
        const QPointF plainAt(8, 4);
        QVERIFY(addRoomAt(plainRoomId, areaId, 8, 4));
        QVERIFY(addRoomAt(customRoomId, areaId, 20, 4));
        TRoom* pPlainRoom = pRoomDB->getRoom(plainRoomId);
        TRoom* pCustomRoom = pRoomDB->getRoom(customRoomId);
        QVERIFY(pPlainRoom->setExit(customRoomId, DIR_EAST));
        QVERIFY(pCustomRoom->setExit(plainRoomId, DIR_WEST));

        T2DMap* p2dMap = showAreaAtTheOrigin(areaId);
        QVERIFY(p2dMap);

        removeCustomLine(pCustomRoom, qsl("w"));
        const Frame withoutLine = renderMapFrame(p2dMap);
        setCustomLine(pCustomRoom, qsl("w"), {QPointF(18, 4)});
        const Frame withLine = renderMapFrame(p2dMap);
        QCOMPARE(p2dMap->getAreaId(), areaId);
        QVERIFY(withLine.pixelsPerUnit > 10.0);
        QVERIFY2(withLine.at(QPointF(18, 4)).x() > kWidgetWidth, "the custom line back ends on the widget");
        QVERIFY2(withLine.at(plainAt + QPointF(5, 0)).x() < kWidgetWidth, "the stretch of plain exit this counts is not on the widget");

        const int drawnWithout = withoutLine.countAlong(plainAt + QPointF(1, 0), plainAt + QPointF(5, 0), 0.0, 1.0, 3, drawnOn);
        const int drawnWith = withLine.countAlong(plainAt + QPointF(1, 0), plainAt + QPointF(5, 0), 0.0, 1.0, 3, drawnOn);
        QVERIFY2(drawnWithout > 0, qPrintable(qsl("room %1 drew no exit east even with no custom line back").arg(plainRoomId)));
        if (drawnWith * 2 < drawnWithout) {
            QFAIL(qPrintable(qsl("room %1 lost its exit east to a custom line drawn nowhere on the widget (%2 pixels drawn, %3 without it). %4")
                                     .arg(plainRoomId)
                                     .arg(drawnWith)
                                     .arg(drawnWithout)
                                     .arg(withLine.savedFor(qsl("offwidget")))));
        }
    }

    // A one-way exit keeps its arrowed line, even where the other room has a custom line straight back,
    // and a custom line straight back stands in only for the exit it is drawn for.
    void test_aOneWayExitKeepsItsLineWhereTheOtherRoomsCustomLineLeadsElsewhere()
    {
        TMap* pMap = map();
        TRoomDB* pRoomDB = pMap->mpRoomDB.get();
        pMap->mapClear();

        const int areaId = pRoomDB->addArea(qsl("One Way Area"));
        QVERIFY(areaId > 0);
        QVERIFY(addRoomAt(kPlayerRoomId, areaId, 0, 0));
        QVERIFY(addRoomAt(3401, areaId, -8, 2));
        QVERIFY(addRoomAt(3402, areaId, -4, 2));
        QVERIFY(addRoomAt(3403, areaId, -12, 2));
        TRoom* pOtherEnd = pRoomDB->getRoom(3402);
        QVERIFY(pRoomDB->getRoom(3401)->setExit(3402, DIR_EAST));
        QVERIFY(pOtherEnd->setExit(3403, DIR_WEST));
        // One way north-east, while the line back is for an exit that does lead to room 3411
        QVERIFY(addRoomAt(3411, areaId, -8, -2));
        QVERIFY(addRoomAt(3412, areaId, -4, -2));
        QVERIFY(pRoomDB->getRoom(3411)->setExit(3412, DIR_NORTHEAST));
        QVERIFY(pRoomDB->getRoom(3412)->setExit(3411, DIR_WEST));
        // Two way north-east and south-west, while the line back is for an exit to room 3423
        QVERIFY(addRoomAt(3421, areaId, -8, -6));
        QVERIFY(addRoomAt(3422, areaId, -4, -6));
        QVERIFY(addRoomAt(3423, areaId, -4, -9));
        QVERIFY(pRoomDB->getRoom(3421)->setExit(3422, DIR_NORTHEAST));
        QVERIFY(pRoomDB->getRoom(3422)->setExit(3421, DIR_SOUTHWEST));
        QVERIFY(pRoomDB->getRoom(3422)->setExit(3423, DIR_WEST));

        T2DMap* p2dMap = showAreaAtTheOrigin(areaId);
        QVERIFY(p2dMap);
        removeCustomLine(pOtherEnd, qsl("w"));
        const Frame withoutLine = renderMapFrame(p2dMap);
        // Running straight back to room 3401, though the exit it is for leads to room 3403; drawn with no
        // pen, so all the frame can show is room 3401's own line
        setCustomLine(pOtherEnd, qsl("w"), {QPointF(-8, 2)}, Qt::NoPen);
        setCustomLine(pRoomDB->getRoom(3412), qsl("w"), {QPointF(-8, -2)}, Qt::NoPen);
        setCustomLine(pRoomDB->getRoom(3422), qsl("w"), {QPointF(-8, -6)}, Qt::NoPen);
        const Frame withLine = renderMapFrame(p2dMap);
        QCOMPARE(p2dMap->getAreaId(), areaId);
        QVERIFY(withLine.pixelsPerUnit > 10.0);

        const int drawnWithout = withoutLine.countAlong(QPointF(-8, 2), QPointF(-4, 2), 0.15, 0.85, 3, plainLineColoured);
        const int drawnWith = withLine.countAlong(QPointF(-8, 2), QPointF(-4, 2), 0.15, 0.85, 3, plainLineColoured);
        QVERIFY2(drawnWithout > 10, "room 3401 drew no one-way exit east even with no custom line on room 3402");
        QVERIFY2(drawnWith * 2 >= drawnWithout,
                 qPrintable(qsl("room 3401 lost its one-way exit east to room 3402's custom line to room 3403 (%1 pixels drawn, %2 without it). %3")
                                    .arg(drawnWith)
                                    .arg(drawnWithout)
                                    .arg(withLine.savedFor(qsl("oneway")))));
        const int oneWayPixels = withLine.countAlong(QPointF(-8, -2), QPointF(-4, -2), 0.15, 0.85, 3, plainLineColoured);
        QVERIFY2(oneWayPixels > 10,
                 qPrintable(qsl("room 3411 lost its one-way exit north-east to room 3412's custom line straight back (%1 pixels). %2").arg(oneWayPixels).arg(withLine.savedFor(qsl("oneway")))));
        const int otherExitPixels = withLine.countAlong(QPointF(-8, -6), QPointF(-4, -6), 0.15, 0.85, 3, plainLineColoured);
        QVERIFY2(otherExitPixels > 10,
                 qPrintable(qsl("rooms 3421 and 3422 lost their exit to a custom line drawn for room 3422's exit to room 3423 (%1 pixels). %2")
                                    .arg(otherExitPixels)
                                    .arg(withLine.savedFor(qsl("oneway")))));
    }

    // exportAreaToImage() draws exits with its own copy of the mapper's code, so it needs the same care.
    void test_anExportedAreaImageShowsNoPlainLineUnderACustomLine()
    {
        TMap* pMap = map();
        TRoomDB* pRoomDB = pMap->mpRoomDB.get();
        pMap->mapClear();

        const int areaId = pRoomDB->addArea(qsl("Export Area"));
        QVERIFY(areaId > 0);
        QVERIFY(addRoomAt(kPlayerRoomId, areaId, 0, 0));
        QList<QPair<int, int>> pairs;
        int nextId = 1000;
        for (int y = -6; y <= 6; y += 3) {
            for (int x = -12; x <= 8; x += 5) {
                const int westId = nextId++;
                const int eastId = nextId++;
                QVERIFY(addTwoWayPair(westId, eastId, areaId, QPoint(x, y), DIR_EAST, DIR_WEST));
                setCustomLine(pRoomDB->getRoom(westId), qsl("e"), {QPointF(x + 2, y)}, Qt::DotLine);
                pairs << qMakePair(westId, eastId);
            }
        }
        T2DMap* p2dMap = showAreaAtTheOrigin(areaId);
        QVERIFY(p2dMap);

        for (const auto& [westId, eastId] : std::as_const(pairs)) {
            pRoomDB->getRoom(eastId)->setExit(-1, DIR_WEST);
        }
        const int withoutReturnExits = countInExport(p2dMap, areaId, qsl("export-one-way"), plainLineColoured);
        QVERIFY(withoutReturnExits >= 0);
        for (const auto& [westId, eastId] : std::as_const(pairs)) {
            QVERIFY(pRoomDB->getRoom(eastId)->setExit(westId, DIR_WEST));
        }
        const int withReturnExits = countInExport(p2dMap, areaId, qsl("export-two-way"), plainLineColoured);
        QVERIFY(withReturnExits >= 0);
        QVERIFY2(withReturnExits - withoutReturnExits <= pairs.size(),
                 qPrintable(qsl("the exported image drew plain return exits along the custom lines (%1 plain line pixels with them, %2 without). The image is at %3/export-two-way.png")
                                    .arg(withReturnExits)
                                    .arg(withoutReturnExits)
                                    .arg(mConfigDir.path())));
    }

    // The exported image leaves out a second plain exit under a room's own straight custom line, and keeps an exit to a
    // room on another level, whose custom line back it does not draw.
    void test_anExportedAreaImageLeavesOutOnlyThePlainLinesACustomLineDraws()
    {
        TMap* pMap = map();
        TRoomDB* pRoomDB = pMap->mpRoomDB.get();
        pMap->mapClear();

        const int areaId = pRoomDB->addArea(qsl("Export Lines Area"));
        QVERIFY(areaId > 0);
        QVERIFY(addRoomAt(kPlayerRoomId, areaId, 0, 0));
        QVERIFY(addTwoWayPair(3601, 3602, areaId, QPoint(-6, 0), DIR_EAST, DIR_WEST));
        // Drawn with no pen, so all the image can show along it is plain lines
        setCustomLine(pRoomDB->getRoom(3601), qsl("e"), {QPointF(-4, 0)}, Qt::NoPen);
        QVERIFY(addRoomAt(3611, areaId, 3, 0));
        QVERIFY(addRoomAt(3612, areaId, 5, 0));
        QVERIFY(pMap->setRoomCoordinates(3612, 5, 0, 1));
        QVERIFY(pRoomDB->getRoom(3611)->setExit(3612, DIR_EAST));
        QVERIFY(pRoomDB->getRoom(3612)->setExit(3611, DIR_WEST));
        T2DMap* p2dMap = showAreaAtTheOrigin(areaId);
        QVERIFY(p2dMap);

        const int withoutLines = countInExport(p2dMap, areaId, qsl("export-lines-without"), plainLineColoured);
        QVERIFY(withoutLines >= 0);
        QVERIFY(pRoomDB->getRoom(3601)->setExit(3602, DIR_NORTHEAST));
        QVERIFY(pRoomDB->getRoom(3602)->setExit(3601, DIR_SOUTHWEST));
        setCustomLine(pRoomDB->getRoom(3612), qsl("w"), {QPointF(3, 0)}, Qt::NoPen);
        const int withLines = countInExport(p2dMap, areaId, qsl("export-lines-with"), plainLineColoured);
        QVERIFY(withLines >= 0);
        // Under the second exit's line, or with the exit up a level gone, the two differ by about three hundred
        QVERIFY2(qAbs(withLines - withoutLines) <= 20,
                 qPrintable(qsl("the exported image drew a second plain exit under a custom line, or lost the exit to a room on another level (%1 plain line pixels, %2 "
                                "before). The image is at %3/export-lines-with.png")
                                    .arg(withLines)
                                    .arg(withoutLines)
                                    .arg(mConfigDir.path())));
    }

    // A door on an exit the exported image draws as a custom line is as wide as on a plain line.
    void test_anExportedDoorKeepsItsWidthWhereACustomLineDrawsTheExit()
    {
        TMap* pMap = map();
        TRoomDB* pRoomDB = pMap->mpRoomDB.get();
        pMap->mapClear();

        const int areaId = pRoomDB->addArea(qsl("Export Door Area"));
        QVERIFY(areaId > 0);
        QVERIFY(addRoomAt(kPlayerRoomId, areaId, 0, 0));
        QVERIFY(addTwoWayPair(3701, 3702, areaId, QPoint(-6, 0), DIR_EAST, DIR_WEST));
        QVERIFY(pRoomDB->getRoom(3701)->setDoor(qsl("e"), 3));
        paintRoomsBlue({kPlayerRoomId, 3701, 3702});
        T2DMap* p2dMap = showAreaAtTheOrigin(areaId);
        QVERIFY(p2dMap);

        const int doorOnPlainLine = countInExport(p2dMap, areaId, qsl("export-door-plain"), doorColoured);
        setCustomLine(pRoomDB->getRoom(3702), qsl("w"), {QPointF(-6, 0)});
        const int doorOnCustomLine = countInExport(p2dMap, areaId, qsl("export-door-custom"), doorColoured);
        QVERIFY2(doorOnPlainLine > 20, qPrintable(qsl("the exported image drew no door on room 3701's exit east (%1 pixels)").arg(doorOnPlainLine)));
        QVERIFY2(doorOnCustomLine * 4 >= doorOnPlainLine * 3,
                 qPrintable(qsl("the exported door is thinner where a custom line draws the exit (%1 pixels, %2 on a plain line). The image is at %3/export-door-custom.png")
                                    .arg(doorOnCustomLine)
                                    .arg(doorOnPlainLine)
                                    .arg(mConfigDir.path())));
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
