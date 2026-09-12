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
 * Zoomed far enough out, a non-grid room is a pixel or two across and T2DMap
 * stops drawing the things that are smaller than that - the border, the
 * symbol, the name, the vertical exit markers - in favour of writing the
 * environment colour straight into a scanline.
 *
 * The trap is the exits. A blanket "rooms are tiny, skip the exits" would
 * throw away the only marks left that carry information at that zoom: an exit
 * to a room the other side of the area, a custom line drawn across it, and the
 * area exit markers that are also the click targets for a speed walk. So the
 * decision is made one exit at a time, from the length that exit is actually
 * drawn at, against the size the two rooms it joins are drawn as. These tests
 * pin the surviving half of that rule, which is the half a mistake removes.
 *
 * The dropped half has no pixel signature to assert on: an exit whose two ends
 * are inside one room blob of each other runs underneath the two blobs, which
 * are painted over it, which is exactly why it is safe to drop. What can be
 * checked is that the rule does not reach any further than that, and
 * test_adjacentRoomExitsSurviveJustInsideTheTier() is that check.
 *
 * The rest is what the tier still has to do besides drawing. A double-click
 * on a room is answered by the room loop, so the reduced tier has to answer it
 * too, and answer it the same way - including about a room it is not drawing.
 *
 * Two tests here are a level down from all that, on the viewport query that
 * decides which rooms either tier is offered in the first place. One runs at
 * forty pixels a room, well above the threshold, because that is the zoom at
 * which where a room stops can be read off the frame at all; the other at a
 * zoom nothing in the UI can reach but a script can set, where the range of
 * room coordinates the query works in stops fitting in the int it returns.
 *
 * A Lua spec cannot reach any of this: the only evidence is pixels and the
 * mapper's own state, and nothing in the Lua API reads back what the mapper
 * drew or what a click on it resolved to.
 *
 * Run with: ctest -R MapLevelOfDetailTest -V
 */

#include <QPixmap>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest/QtTest>

#include "Host.h"
#include "MudletInstanceCoordinator.h"
#include "MudletPaths.h"
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

class MapLevelOfDetailTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    TelnetServerStub* mpServer = nullptr;
    Host* mpHost = nullptr;
    const QString mProfileName = qsl("MapLevelOfDetail-Test");
    const QString mLocalhost = qsl("localhost");
    QString mPort;

    // Wider than tall so the two axes of the viewport cannot be confused, and
    // so that a room is drawn kWidgetHeight / zoom pixels across.
    static constexpr int kWidgetWidth = 600;
    static constexpr int kWidgetHeight = 400;
    // The zoom that makes a room exactly cLodPixelThreshold pixels wide, and
    // one a hair further out, which is the first zoom in the reduced tier.
    static constexpr double kZoomAtThreshold = 100.0;
    static constexpr double kZoomJustInsideTier = 101.0;
    // A room three pixels across: still in the reduced tier, but far enough
    // above one pixel that an exit to the room next door is longer than the
    // blob and so has to survive.
    static constexpr double kZoomThreePixelRooms = 400.0 / 3.0;
    static constexpr double kZoomNearlyFourPixelRooms = 400.0 / 3.9;
    static constexpr double kZoomOnePixelRooms = 400.0;
    // Well clear of the threshold, so the full-detail room loop runs and a room
    // is big enough that where its rectangle stops can be read off the frame.
    static constexpr double kZoomFortyPixelRooms = 10.0;
    // Half a pixel per room, so a quarter of a pixel of room once the room size
    // fraction is applied, which rounds to none.
    static constexpr double kZoomHalfPixelRooms = 800.0;
    // A ten-millionth of a pixel per room. Nothing in the UI zooms out this
    // far, but setMapZoom() and TArea::set2DMapZoom() only reject a zoom that
    // is too small, so a script can ask for this one.
    static constexpr double kZoomFarBeyondAPixelPerRoom = 4.0e9;

    // Parked outside every viewport used here, so the player room's own
    // full-detail drawing and its highlight gradient cannot be mistaken for
    // anything a test is counting.
    static constexpr int kPlayerRoomId = 999;
    static constexpr int kPlayerRoomY = 1000;

    // The room a click is aimed at, and a hidden one sitting on the origin
    // that the same click must not reach.
    static constexpr int kVisibleClickTargetId = 2;
    static constexpr int kVisibleClickTargetX = 2;
    static constexpr int kHiddenClickTargetId = 5;

    // Environment ids above 255 miss every branch of the built-in palette, so
    // the colour is whatever mCustomEnvColors was given.
    static constexpr int kEnvRoom = 501;
    static constexpr int kEnvHidden = 502;
    static constexpr int kEnvAreaExitDestination = 503;
    static constexpr int kEnvDark = 504;
    // setCustomEnvColor() takes an alpha, so an environment colour that is
    // only partly opaque is something a script can ask for.
    static constexpr int kEnvTranslucent = 505;
    // An environment a map file remapped onto an id that carries no colour of
    // its own. Nothing rejects the mapping on the way in, so every tier has to
    // fall back to something, and to the same something.
    static constexpr int kEnvRemappedToNothing = 506;
    static constexpr int kEnvWithNoColourOfItsOwn = 4242;

    static QColor exitColour() { return QColor(0, 255, 128); }
    static QColor customLineColour() { return QColor(255, 0, 255); }
    static QColor roomColour() { return QColor(255, 128, 0); }
    static QColor hiddenRoomColour() { return QColor(0, 128, 255); }
    static QColor areaExitColour() { return QColor(0, 255, 255); }
    static QColor borderColour() { return QColor(0, 255, 0); }
    static QColor translucentRoomColour() { return QColor(255, 200, 100, 128); }

    // At these zooms an exit pen is a twentieth of a pixel wide and Qt draws it
    // at a matching fraction of its colour, so hardly any pixel in a frame
    // holds a colour above exactly. What a blend over the black background
    // does keep is the RATIO between the three channels, since it scales all
    // three by the same amount - so a mark is identified by rescaling the
    // pixel's brightest channel up to the colour's own and comparing from
    // there. The palette above is picked so no two of these ratios are close.
    static bool isScaledFrom(const QColor& pixel, const QColor& colour)
    {
        const int brightest = qMax(pixel.red(), qMax(pixel.green(), pixel.blue()));
        if (brightest < 8) {
            // Too dark to tell from the background, whatever its ratios say.
            return false;
        }
        const int colourBrightest = qMax(colour.red(), qMax(colour.green(), colour.blue()));
        const int tolerance = qMax(6, brightest / 5);
        const int channels[3][2] = {{pixel.red(), colour.red()}, {pixel.green(), colour.green()}, {pixel.blue(), colour.blue()}};
        for (const auto& channel : channels) {
            const int expected = qRound(static_cast<double>(channel[1]) * brightest / colourBrightest);
            if (qAbs(channel[0] - expected) > tolerance) {
                return false;
            }
        }
        return true;
    }

    static int countPixels(const QImage& image, const QColor& colour)
    {
        int count = 0;
        for (int y = 0; y < image.height(); ++y) {
            for (int x = 0; x < image.width(); ++x) {
                if (isScaledFrom(image.pixelColor(x, y), colour)) {
                    ++count;
                }
            }
        }
        return count;
    }

    // Everything the map put on the widget, whatever colour it ended up: the
    // background is filled black here, so anything else is a mark.
    static int countMarkedPixels(const QImage& image)
    {
        int count = 0;
        for (int y = 0; y < image.height(); ++y) {
            for (int x = 0; x < image.width(); ++x) {
                const QColor pixel = image.pixelColor(x, y);
                if (pixel.red() + pixel.green() + pixel.blue() > 8) {
                    ++count;
                }
            }
        }
        return count;
    }

    // The colour a room was actually drawn in, taken as the brightest pixel in
    // the frame. A room a few pixels across lands on fractional coordinates, so
    // its edge pixels carry a fraction of the colour however the fill was
    // composited and only the interior carries the whole of it.
    static QColor brightestPixel(const QImage& image)
    {
        QColor brightest(0, 0, 0);
        int brightestSum = -1;
        for (int y = 0; y < image.height(); ++y) {
            for (int x = 0; x < image.width(); ++x) {
                const QColor pixel = image.pixelColor(x, y);
                const int sum = pixel.red() + pixel.green() + pixel.blue();
                if (sum > brightestSum) {
                    brightestSum = sum;
                    brightest = pixel;
                }
            }
        }
        return brightest;
    }

    static bool coloursMatch(const QColor& lhs, const QColor& rhs, const int tolerance)
    {
        return qAbs(lhs.red() - rhs.red()) <= tolerance && qAbs(lhs.green() - rhs.green()) <= tolerance && qAbs(lhs.blue() - rhs.blue()) <= tolerance;
    }

    // A frame is far quicker to read than a pixel count when working out
    // whether one mark went missing or the whole map did.
    QString saveFrame(const QImage& frame, const QString& label) const
    {
        const QString framePath = qsl("%1/MapLevelOfDetailTest-%2.png").arg(QDir::tempPath(), label);
        frame.save(framePath);
        return framePath;
    }

    void deleteProfileDirectory() const
    {
        QDir dir(MudletPaths::getMudletPath(enums::profileHomePath, mProfileName));
        if (dir.exists()) {
            dir.removeRecursively();
        }
    }

    TMap* map() const { return mpHost->mpMap.data(); }

    bool addRoomAt(const int id, const int areaId, const int x, const int y, const int environment, const int z = 0) const
    {
        if (!map()->addRoom(id) || !map()->setRoomArea(id, areaId) || !map()->setRoomCoordinates(id, x, y, z)) {
            return false;
        }
        TRoom* pRoom = map()->mpRoomDB->getRoom(id);
        if (!pRoom) {
            return false;
        }
        pRoom->environment = environment;
        return true;
    }

    // The reduced tier only takes rooms from the area's exit index while that
    // offers fewer of them than a viewport query does, and falls back to the
    // loop over every room on the level otherwise. The fall-back draws the
    // same frame, so a test meaning to exercise the index has to say so or it
    // passes either way.
    static bool indexPathTaken(const T2DMap* p2dMap) { return p2dMap->mLodExitIndexRoomsHandedOver >= 0; }

    // Every test starts from an empty map with the player parked off screen.
    int freshArea(const QString& name) const
    {
        TMap* pMap = map();
        pMap->mapClear();
        pMap->mCustomEnvColors[kEnvRoom] = roomColour();
        pMap->mCustomEnvColors[kEnvHidden] = hiddenRoomColour();
        pMap->mCustomEnvColors[kEnvAreaExitDestination] = areaExitColour();
        pMap->mCustomEnvColors[kEnvDark] = QColor(0, 0, 0);
        pMap->mCustomEnvColors[kEnvTranslucent] = translucentRoomColour();

        const int areaId = pMap->mpRoomDB->addArea(name);
        if (areaId <= 0 || !addRoomAt(kPlayerRoomId, areaId, 0, kPlayerRoomY, kEnvRoom)) {
            return 0;
        }
        pMap->mRoomIdHash[pMap->mProfileName] = kPlayerRoomId;
        pMap->mNewMove = false;
        pMap->setDefaultAreaShown(false);
        return areaId;
    }

    // Antialiasing is off so that a pen thinner than a pixel lands as a solid
    // hairline of its own colour rather than a few per cent of it, which no
    // channel-order test could tell from the background.
    T2DMap* prepareWidget(const int areaId, const double zoom, const double roomSize, const double borderSize, const double lineSize = 10.0)
    {
        mpHost->mRoomSize = roomSize;
        mpHost->mRoomBorderSize = borderSize;
        mpHost->mLineSize = lineSize;
        mpHost->mMapperUseAntiAlias = false;
        mpHost->mMapperShowGrid = false;
        mpHost->mMapperShowRoomBorders = false;
        mpHost->mMapperCenterSmallAreas = false;
        mpHost->mBgColor_2 = QColor(0, 0, 0);
        mpHost->mFgColor_2 = exitColour();
        // Otherwise the info box paints a room's environment colour into the
        // top left corner of every frame.
        mpHost->mMapInfoContributors.clear();

        TMap* pMap = map();
        if (!pMap->mpMapper) {
            mpHost->showHideOrCreateMapper(false);
        }
        if (!pMap->mpMapper) {
            return nullptr;
        }
        T2DMap* p2dMap = pMap->mpMapper->mp2dMap;
        if (!p2dMap) {
            return nullptr;
        }
        p2dMap->init();
        p2dMap->resize(kWidgetWidth, kWidgetHeight);
        // paintEvent() re-centres on the player and quietly draws somewhere
        // else unless the player's room, mRoomID and mShiftMode all agree.
        p2dMap->mRoomID = kPlayerRoomId;
        p2dMap->mShiftMode = true;
        p2dMap->mPick = false;
        p2dMap->mAreaID = areaId;
        p2dMap->mMapCenterX = 0;
        p2dMap->mMapCenterY = 0;
        p2dMap->mMapCenterZ = 0;
        p2dMap->mMultiSelectionSet.clear();

        TArea* pArea = pMap->mpRoomDB->getArea(areaId);
        if (!pArea) {
            return nullptr;
        }
        pArea->set2DMapZoom(zoom);
        return p2dMap;
    }

    // Something was drawn hard against an edge of the widget, which is what a
    // level running off the side looks like from the outside.
    static bool touchesAnEdge(const QImage& frame)
    {
        const QRgb background = frame.pixel(0, 0);
        for (int x = 0; x < frame.width(); ++x) {
            if (frame.pixel(x, 0) != background || frame.pixel(x, frame.height() - 1) != background) {
                return true;
            }
        }
        for (int y = 0; y < frame.height(); ++y) {
            if (frame.pixel(0, y) != background || frame.pixel(frame.width() - 1, y) != background) {
                return true;
            }
        }
        return false;
    }

    static QImage renderFrame(T2DMap* p2dMap)
    {
        QPixmap target(kWidgetWidth, kWidgetHeight);
        target.fill(Qt::black);
        p2dMap->render(&target, QPoint(), QRegion(), QWidget::DrawWindowBackground);
        return target.toImage();
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
        QCOMPARE(MudletPaths::getMudletPath(enums::mainPath), qsl("%1/mudlet").arg(mConfigDir.path()));
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

    // The exit that carries the most information at this zoom is the one that
    // goes somewhere: it is hundreds of pixels long however small the rooms
    // get, and it is all that says the two ends are connected.
    void test_aLongExitSurvivesWhenARoomIsOnePixelWide()
    {
        TMap* pMap = map();
        const int areaId = freshArea(qsl("Long Exit Area"));
        QVERIFY(areaId > 0);
        QVERIFY(addRoomAt(1, areaId, 0, 0, kEnvRoom));
        QVERIFY(addRoomAt(2, areaId, 150, 0, kEnvRoom));
        QVERIFY(pMap->setExit(1, 2, DIR_EAST));
        QVERIFY(pMap->setExit(2, 1, DIR_WEST));

        T2DMap* p2dMap = prepareWidget(areaId, kZoomOnePixelRooms, 0.5, 10.0);
        QVERIFY(p2dMap);
        const QImage frame = renderFrame(p2dMap);

        QCOMPARE(p2dMap->getAreaId(), areaId);
        // Without this the test could pass by never entering the reduced tier
        // at all, which is the one way it must not pass.
        QVERIFY2(p2dMap->mRoomWidth < 4.0f, "the rooms were not drawn small enough to reach the reduced level of detail");
        QCOMPARE(p2dMap->mRoomWidth, 1.0f);

        const int exitPixels = countPixels(frame, exitColour());
        if (exitPixels <= 100) {
            QFAIL(qPrintable(qsl("the 150 unit long exit was not drawn - only %1 pixels of the exit colour are in the frame, saved at %2")
                                     .arg(QString::number(exitPixels), saveFrame(frame, qsl("long-exit")))));
        }
    }

    // The rule is per exit and measured against the size a room is drawn as,
    // so a room drawn just under four pixels keeps the exits to the rooms next
    // door: they are longer than the blob and stick out from under it. A
    // blanket "below four pixels, no exits" passes every other test here and
    // fails this one.
    void test_adjacentRoomExitsSurviveJustInsideTheTier()
    {
        TMap* pMap = map();
        const int areaId = freshArea(qsl("Adjacent Exit Area"));
        QVERIFY(areaId > 0);
        constexpr int chainLength = 20;
        for (int i = 0; i < chainLength; ++i) {
            QVERIFY(addRoomAt(i + 1, areaId, i - (chainLength / 2), 0, kEnvRoom));
        }
        for (int i = 1; i < chainLength; ++i) {
            QVERIFY(pMap->setExit(i, i + 1, DIR_EAST));
            QVERIFY(pMap->setExit(i + 1, i, DIR_WEST));
        }

        T2DMap* p2dMap = prepareWidget(areaId, kZoomNearlyFourPixelRooms, 0.5, 10.0);
        QVERIFY(p2dMap);
        const QImage frame = renderFrame(p2dMap);

        QVERIFY2(p2dMap->mRoomWidth < 4.0f, "the rooms were not drawn small enough to reach the reduced level of detail");
        QVERIFY2(p2dMap->mRoomWidth > 3.0f, "the rooms have to be drawn nearly four pixels across for their exits to be longer than the blob");

        const int exitPixels = countPixels(frame, exitColour());
        if (exitPixels < 15) {
            QFAIL(qPrintable(qsl("the exits between the %1 rooms of the chain were dropped even though each is longer than the blob that covers it - only %2 pixels of the exit colour are "
                                 "in the frame, saved at %3")
                                     .arg(QString::number(chainLength), QString::number(exitPixels), saveFrame(frame, qsl("adjacent-exits")))));
        }
    }

    // A custom line is a list of arbitrary points, so its length has nothing
    // to do with how big a room is drawn and it is never a candidate for the
    // reduced tier.
    void test_aLongCustomLineSurvivesWhenARoomIsOnePixelWide()
    {
        const int areaId = freshArea(qsl("Custom Line Area"));
        QVERIFY(areaId > 0);
        QVERIFY(addRoomAt(1, areaId, 0, 0, kEnvRoom));
        QVERIFY(addRoomAt(2, areaId, 150, 0, kEnvRoom));

        TRoom* pRoom = map()->mpRoomDB->getRoom(1);
        QVERIFY(pRoom);
        QList<QPointF> linePoints;
        for (int x = 0; x <= 150; x += 10) {
            linePoints << QPointF(static_cast<qreal>(x), 0.0);
        }
        pRoom->customLines[qsl("e")] = linePoints;
        pRoom->customLinesColor[qsl("e")] = customLineColour();
        // QMap::value() default-constructs a Qt::PenStyle as Qt::NoPen, so a
        // line without an explicit style draws nothing at all.
        pRoom->customLinesStyle[qsl("e")] = Qt::SolidLine;
        pRoom->customLinesArrow[qsl("e")] = false;
        pRoom->calcRoomDimensions();

        T2DMap* p2dMap = prepareWidget(areaId, kZoomOnePixelRooms, 0.5, 10.0);
        QVERIFY(p2dMap);
        const QImage frame = renderFrame(p2dMap);

        QVERIFY2(p2dMap->mRoomWidth < 4.0f, "the rooms were not drawn small enough to reach the reduced level of detail");

        const int linePixels = countPixels(frame, customLineColour());
        if (linePixels <= 100) {
            QFAIL(qPrintable(qsl("the 150 unit long custom line was not drawn - only %1 pixels of its colour are in the frame, saved at %2")
                                     .arg(QString::number(linePixels), saveFrame(frame, qsl("custom-line")))));
        }
    }

    // An area exit marker is a fixed size whatever the exit's length, and it
    // is the click target for a speed walk into that area, so it is kept even
    // when it joins two rooms that are a blob apart.
    void test_anAreaExitMarkerSurvivesWhenTheRoomsAreABlobApart()
    {
        TMap* pMap = map();
        const int areaId = freshArea(qsl("Area Exit Source"));
        QVERIFY(areaId > 0);
        const int otherAreaId = pMap->mpRoomDB->addArea(qsl("Area Exit Destination"));
        QVERIFY(otherAreaId > 0);
        QVERIFY(addRoomAt(1, areaId, 0, 0, kEnvRoom));
        QVERIFY(addRoomAt(2, otherAreaId, 1, 0, kEnvAreaExitDestination));
        QVERIFY(pMap->setExit(1, 2, DIR_EAST));

        // Rooms filling their whole cell, so the blob is as wide as a room is
        // drawn and the destination is exactly one blob away: an exit within
        // this area would be dropped at this size.
        // The exit pen is deliberately fat here. Its width is a fraction of the
        // room size, so at the default setting the marker's coloured line is a
        // third of a pixel wide, drawn as a third of its colour over the white
        // outline underneath it - which is a blend against something other
        // than the background and so the one thing the counting cannot see
        // through. Widening the pen puts the colour down solid instead.
        T2DMap* p2dMap = prepareWidget(areaId, kZoomNearlyFourPixelRooms, 1.0, 10.0, 1.0);
        QVERIFY(p2dMap);
        p2dMap->mLargeAreaExitArrows = true;
        const QImage frame = renderFrame(p2dMap);

        QVERIFY2(p2dMap->mRoomWidth < 4.0f, "the rooms were not drawn small enough to reach the reduced level of detail");

        const int markerPixels = countPixels(frame, areaExitColour());
        if (markerPixels < 3) {
            QFAIL(qPrintable(qsl("the area exit marker was dropped along with the short exits - only %1 pixels of the destination area's colour are in the frame, saved at %2")
                                     .arg(QString::number(markerPixels), saveFrame(frame, qsl("area-exit")))));
        }
    }

    // Drawing the marker is only half of it: it is also the click target for a
    // speed walk into the area it points at, and that hit test runs over every
    // marker in the frame rather than over anything belonging to one room. The
    // reduced tier draws no room in full except the player's, so hanging the
    // test off a room left every marker on screen dead whenever the player was
    // somewhere else - which, at a zoom that fits an area on screen, is most of
    // the time.
    void test_anAreaExitMarkerAnswersAClickWithThePlayerOffScreen_data()
    {
        QTest::addColumn<double>("zoom");

        // The full-detail row is the control: the marker has always answered
        // there, and it is the only thing that says the reduced-tier row is
        // measuring the tier rather than the way the click is delivered.
        QTest::newRow("full detail") << kZoomAtThreshold;
        QTest::newRow("reduced tier") << kZoomNearlyFourPixelRooms;
    }

    void test_anAreaExitMarkerAnswersAClickWithThePlayerOffScreen()
    {
        QFETCH(double, zoom);

        TMap* pMap = map();
        const int areaId = freshArea(qsl("Area Exit Click Source"));
        QVERIFY(areaId > 0);
        const int otherAreaId = pMap->mpRoomDB->addArea(qsl("Area Exit Click Destination"));
        QVERIFY(otherAreaId > 0);
        QVERIFY(addRoomAt(1, areaId, 0, 0, kEnvRoom));
        QVERIFY(addRoomAt(2, otherAreaId, 1, 0, kEnvAreaExitDestination));
        QVERIFY(pMap->setExit(1, 2, DIR_EAST));

        T2DMap* p2dMap = prepareWidget(areaId, zoom, 1.0, 10.0, 1.0);
        QVERIFY(p2dMap);
        // The first frame is what settles where the map is panned to, which is
        // what turns a room coordinate into a widget one.
        renderFrame(p2dMap);

        // paintRoomExits() puts an east exit's click point one room cell east
        // of the room the exit leaves.
        const QPoint markerPoint(qRound(p2dMap->mRX + static_cast<double>(p2dMap->mRoomWidth)), p2dMap->mRY);
        p2dMap->mTargetRoomId = 0;
        p2dMap->mPHighlight = markerPoint;
        p2dMap->mPick = true;
        p2dMap->mStartSpeedWalk = true;
        const QImage frame = renderFrame(p2dMap);

        if (p2dMap->mTargetRoomId != 2) {
            QFAIL(qPrintable(qsl("a double-click on the area exit marker at %1,%2 started no speed walk into the area it points at - the target room is %3, frame saved at %4")
                                     .arg(QString::number(markerPoint.x()), QString::number(markerPoint.y()), QString::number(p2dMap->mTargetRoomId), saveFrame(frame, qsl("area-exit-click")))));
        }
    }

    // A double-click on a room resolves into a speed walk from inside the room
    // loop, and the reduced tier does not run the full-detail loop that has
    // always done it - it answers the click from the grid index instead. That
    // block is the only thing standing between a double-click and nothing
    // happening at all below four pixels a room, and nothing else here clicks
    // on a room: the area exit marker test goes through the marker hit test,
    // which is a different function and runs whichever tier drew the frame.
    //
    // A hidden room is where the two ways of answering can disagree. drawRoom()
    // returns before its hit test for one, so at full detail a click where a
    // hidden room sits does nothing, while the grid index hands its id over
    // like any other room's - which would walk the player into a room the map
    // is not drawing.
    void test_aClickInTheReducedTierWalksIntoAVisibleRoomOnly_data()
    {
        QTest::addColumn<double>("zoom");
        QTest::addColumn<bool>("reducedTier");
        QTest::addColumn<int>("clickedRoomX");
        QTest::addColumn<int>("expectedTargetRoomId");

        // The full-detail rows are the control: they are what the reduced tier
        // has to agree with, and they say a row that goes green did so because
        // of the tier and not because the click never arrived.
        QTest::newRow("visible room, full detail") << kZoomAtThreshold << false << kVisibleClickTargetX << kVisibleClickTargetId;
        QTest::newRow("visible room, reduced tier") << kZoomThreePixelRooms << true << kVisibleClickTargetX << kVisibleClickTargetId;
        QTest::newRow("hidden room, full detail") << kZoomAtThreshold << false << 0 << 0;
        QTest::newRow("hidden room, reduced tier") << kZoomThreePixelRooms << true << 0 << 0;
    }

    void test_aClickInTheReducedTierWalksIntoAVisibleRoomOnly()
    {
        QFETCH(double, zoom);
        QFETCH(bool, reducedTier);
        QFETCH(int, clickedRoomX);
        QFETCH(int, expectedTargetRoomId);

        const int areaId = freshArea(qsl("Room Click Area"));
        QVERIFY(areaId > 0);
        QVERIFY(addRoomAt(1, areaId, -kVisibleClickTargetX, 0, kEnvRoom));
        QVERIFY(addRoomAt(kVisibleClickTargetId, areaId, kVisibleClickTargetX, 0, kEnvRoom));
        QVERIFY(addRoomAt(kHiddenClickTargetId, areaId, 0, 0, kEnvHidden));
        TRoom* pHiddenRoom = map()->mpRoomDB->getRoom(kHiddenClickTargetId);
        QVERIFY(pHiddenRoom);
        pHiddenRoom->hidden = true;

        T2DMap* p2dMap = prepareWidget(areaId, zoom, 1.0, 10.0);
        QVERIFY(p2dMap);
        // The first frame is what settles where the map is panned to, which is
        // what turns a room coordinate into a widget one.
        renderFrame(p2dMap);
        if (reducedTier) {
            QVERIFY2(p2dMap->mRoomWidth < 4.0f, "the rooms were not drawn small enough to reach the reduced level of detail");
        } else {
            QCOMPARE(p2dMap->mRoomWidth, 4.0f);
        }

        const QPoint clickPoint(qRound(p2dMap->mRX + clickedRoomX * static_cast<double>(p2dMap->mRoomWidth)), p2dMap->mRY);
        p2dMap->mTargetRoomId = 0;
        p2dMap->mPHighlight = clickPoint;
        p2dMap->mPick = true;
        p2dMap->mStartSpeedWalk = true;
        const QImage frame = renderFrame(p2dMap);

        if (p2dMap->mTargetRoomId == expectedTargetRoomId) {
            return;
        }
        if (expectedTargetRoomId) {
            QFAIL(qPrintable(qsl("a double-click on the room at %1,%2 started no speed walk into it - the target room is %3, frame saved at %4")
                                     .arg(QString::number(clickPoint.x()), QString::number(clickPoint.y()), QString::number(p2dMap->mTargetRoomId), saveFrame(frame, qsl("room-click")))));
        }
        QFAIL(qPrintable(qsl("a double-click at %1,%2, where a hidden room the map did not draw sits, started a speed walk into room %3. The frame is at %4")
                                 .arg(QString::number(clickPoint.x()), QString::number(clickPoint.y()), QString::number(p2dMap->mTargetRoomId), saveFrame(frame, qsl("room-click-hidden")))));
    }

    // What the reduced tier does draw: one blob of the environment colour per
    // room, and nothing at all for a hidden one.
    void test_roomsBecomeTheirEnvironmentColourAndHiddenOnesStayUndrawn()
    {
        const int areaId = freshArea(qsl("Blob Area"));
        QVERIFY(areaId > 0);
        QVERIFY(addRoomAt(1, areaId, -2, 0, kEnvRoom));
        QVERIFY(addRoomAt(2, areaId, 2, 0, kEnvRoom));
        QVERIFY(addRoomAt(3, areaId, 0, -2, kEnvRoom));
        QVERIFY(addRoomAt(4, areaId, 0, 2, kEnvRoom));
        QVERIFY(addRoomAt(5, areaId, 0, 0, kEnvHidden));
        TRoom* pHiddenRoom = map()->mpRoomDB->getRoom(5);
        QVERIFY(pHiddenRoom);
        pHiddenRoom->hidden = true;

        T2DMap* p2dMap = prepareWidget(areaId, kZoomThreePixelRooms, 1.0, 10.0);
        QVERIFY(p2dMap);
        const QImage frame = renderFrame(p2dMap);

        QVERIFY2(p2dMap->mRoomWidth < 4.0f, "the rooms were not drawn small enough to reach the reduced level of detail");
        QCOMPARE(p2dMap->mRoomWidth, 3.0f);

        // Four rooms, three pixels square each.
        const int roomPixels = countPixels(frame, roomColour());
        if (roomPixels < 24) {
            QFAIL(qPrintable(qsl("the four visible rooms were not drawn in their environment colour - only %1 pixels of it are in the frame, saved at %2")
                                     .arg(QString::number(roomPixels), saveFrame(frame, qsl("blobs")))));
        }
        const int hiddenPixels = countPixels(frame, hiddenRoomColour());
        if (hiddenPixels != 0) {
            QFAIL(qPrintable(
                    qsl("the hidden room was drawn - %1 pixels of its environment colour are in the frame, saved at %2").arg(QString::number(hiddenPixels), saveFrame(frame, qsl("blobs-hidden")))));
        }
    }

    // The reduced tiers write their colours straight into the scanlines of a
    // premultiplied image rather than handing them to a QPainter, so an
    // environment colour that is not fully opaque has to be premultiplied on
    // the way in. Left alone it composites at full strength and the room
    // brightens as the map crosses the threshold, which is the one zoom step
    // where nothing about a room is supposed to change but its size.
    //
    // There are three such scanline loops - one for a non-grid area, and two
    // for a grid one, which splits at a room narrower than a pixel - and each
    // arrives at its colour separately.
    void test_aTranslucentEnvironmentColourSurvivesTheThresholdUnchanged_data()
    {
        QTest::addColumn<bool>("gridMode");
        QTest::addColumn<double>("reducedTierZoom");

        QTest::newRow("non-grid") << false << kZoomThreePixelRooms;
        QTest::newRow("grid") << true << kZoomThreePixelRooms;
        QTest::newRow("grid, room narrower than a pixel") << true << kZoomHalfPixelRooms;
    }

    void test_aTranslucentEnvironmentColourSurvivesTheThresholdUnchanged()
    {
        QFETCH(bool, gridMode);
        QFETCH(double, reducedTierZoom);

        const int areaId = freshArea(qsl("Translucent Area"));
        QVERIFY(areaId > 0);
        QVERIFY(addRoomAt(1, areaId, 0, 0, kEnvTranslucent));
        TArea* pArea = map()->mpRoomDB->getArea(areaId);
        QVERIFY(pArea);
        pArea->gridMode = gridMode;

        T2DMap* p2dMap = prepareWidget(areaId, kZoomAtThreshold, 1.0, 10.0);
        QVERIFY(p2dMap);
        const QImage atThreshold = renderFrame(p2dMap);
        QCOMPARE(p2dMap->mRoomWidth, 4.0f);
        const QColor fullDetailColour = brightestPixel(atThreshold);

        p2dMap = prepareWidget(areaId, reducedTierZoom, 1.0, 10.0);
        QVERIFY(p2dMap);
        const QImage insideTier = renderFrame(p2dMap);
        QVERIFY2(p2dMap->mRoomWidth < 4.0f, "the second frame was not drawn inside the reduced level of detail");
        const QColor reducedTierColour = brightestPixel(insideTier);

        // The colour the QPainter fill of the full-detail path arrives at: the
        // environment colour scaled by its own alpha, over the black background.
        const QColor overBlack(qRound(translucentRoomColour().red() * translucentRoomColour().alphaF()),
                               qRound(translucentRoomColour().green() * translucentRoomColour().alphaF()),
                               qRound(translucentRoomColour().blue() * translucentRoomColour().alphaF()));
        if (!coloursMatch(fullDetailColour, overBlack, 4)) {
            QFAIL(qPrintable(qsl("the full-detail room was not drawn as its translucent environment colour over the background - expected %1, drew %2, frame saved at %3")
                                     .arg(overBlack.name(), fullDetailColour.name(), saveFrame(atThreshold, qsl("translucent-at-threshold")))));
        }
        if (!coloursMatch(reducedTierColour, fullDetailColour, 4)) {
            QFAIL(qPrintable(
                    qsl("the room changed colour on crossing into the reduced tier - drawn %1 at four pixels a room and %2 below that, frames saved at %3 and %4")
                            .arg(fullDetailColour.name(), reducedTierColour.name(), saveFrame(atThreshold, qsl("translucent-at-threshold")), saveFrame(insideTier, qsl("translucent-inside-tier")))));
        }
    }

    // Two of those scanline loops take their colour from environmentColor()
    // while the full-detail non-grid loop works it out for itself, so the two
    // can disagree about a room whose environment has no colour anywhere - and
    // the room then changes appearance on the one zoom step where nothing
    // about it is supposed to change but its size.
    //
    // The way to get there is an mEnvColors entry pointing at an id that
    // carries no colour of its own. It has no Lua setter: only a map file can
    // put an entry in it, and nothing checks what it points at exists.
    void test_anEnvironmentRemappedOntoNothingKeepsTheRoomVisibleAcrossTheThreshold()
    {
        const int areaId = freshArea(qsl("Dangling Environment Area"));
        QVERIFY(areaId > 0);
        QVERIFY(addRoomAt(1, areaId, 0, 0, kEnvRemappedToNothing));
        map()->mEnvColors[kEnvRemappedToNothing] = kEnvWithNoColourOfItsOwn;

        T2DMap* p2dMap = prepareWidget(areaId, kZoomAtThreshold, 1.0, 10.0);
        QVERIFY(p2dMap);
        const QImage atThreshold = renderFrame(p2dMap);
        QCOMPARE(p2dMap->mRoomWidth, 4.0f);
        const QColor fullDetailColour = brightestPixel(atThreshold);

        p2dMap = prepareWidget(areaId, kZoomThreePixelRooms, 1.0, 10.0);
        QVERIFY(p2dMap);
        const QImage insideTier = renderFrame(p2dMap);
        QVERIFY2(p2dMap->mRoomWidth < 4.0f, "the second frame was not drawn inside the reduced level of detail");
        const QColor reducedTierColour = brightestPixel(insideTier);

        // The background is filled black here, so a room that took the
        // background colour is a room that is not there any more.
        if (countMarkedPixels(atThreshold) < 1 || countMarkedPixels(insideTier) < 1) {
            QFAIL(qPrintable(qsl("the room disappeared into the background because its environment is remapped onto an id with no colour - %1 marked pixels at four pixels a room and %2 below "
                                 "that, frames saved at %3 and %4")
                                     .arg(QString::number(countMarkedPixels(atThreshold)),
                                          QString::number(countMarkedPixels(insideTier)),
                                          saveFrame(atThreshold, qsl("dangling-env-at-threshold")),
                                          saveFrame(insideTier, qsl("dangling-env-inside-tier")))));
        }
        if (!coloursMatch(reducedTierColour, fullDetailColour, 4)) {
            QFAIL(qPrintable(
                    qsl("the room changed colour on crossing into the reduced tier - drawn %1 at four pixels a room and %2 below that, frames saved at %3 and %4")
                            .arg(fullDetailColour.name(), reducedTierColour.name(), saveFrame(atThreshold, qsl("dangling-env-at-threshold")), saveFrame(insideTier, qsl("dangling-env-inside-tier")))));
        }
    }

    // Zoom out far enough and a room rounds down to no pixels at all. The blob
    // is floored at one pixel for that, because the alternative is the area
    // disappearing from the map rather than getting smaller.
    void test_roomsSurviveAZoomWhereTheyRoundDownToNothing()
    {
        const int areaId = freshArea(qsl("Sub Pixel Area"));
        QVERIFY(areaId > 0);
        // Spread out enough that each still lands in a pixel of its own.
        constexpr int roomCount = 5;
        for (int i = 0; i < roomCount; ++i) {
            QVERIFY(addRoomAt(i + 1, areaId, (i - (roomCount / 2)) * 20, 0, kEnvRoom));
        }

        T2DMap* p2dMap = prepareWidget(areaId, kZoomHalfPixelRooms, 0.5, 10.0);
        QVERIFY(p2dMap);
        const QImage frame = renderFrame(p2dMap);

        QCOMPARE(p2dMap->mRoomWidth, 0.5f);
        QVERIFY2(qRound(p2dMap->mRoomWidth * static_cast<float>(p2dMap->rSize)) == 0, "the rooms did not round down to nothing, so nothing here needs a floor under the blob size");

        const int roomPixels = countPixels(frame, roomColour());
        if (roomPixels < roomCount - 1) {
            QFAIL(qPrintable(qsl("the area vanished instead of being drawn a pixel a room - only %1 pixels of the environment colour are in the frame, saved at %2")
                                     .arg(QString::number(roomPixels), saveFrame(frame, qsl("sub-pixel-rooms")))));
        }
    }

    // Both room loops draw whatever the viewport query hands them, so a query
    // that hands over too little quietly loses the rooms at the edges of the
    // widget - the one place someone looking at the map would blame the map.
    // Every other case in this file is inside the reduced tier; this is the
    // full-detail loop, which reaches the same query by the other branch.
    //
    // The rooms are two cells apart so no room can stand in for its neighbour,
    // and the map is panned across a whole cell in fifths, because an
    // off-by-one in the bounds only shows at some sub-cell offsets.
    //
    // What this cannot pin is the query's own margins: it already hands over a
    // cell or two more on each side than the loops go on to draw, so taking
    // all four away leaves the frame identical. It is the cull closing in past
    // that slack which shows up here.
    void test_theViewportCullKeepsEveryRoomThatTouchesTheWidget()
    {
        const int areaId = freshArea(qsl("Viewport Cull Area"));
        QVERIFY(areaId > 0);

        // A row and a column, both running well past the widget at every pan
        // offset used below.
        QList<QPoint> roomPositions;
        for (int i = -8; i <= 8; ++i) {
            roomPositions.append(QPoint(i * 2, 0));
        }
        for (int j = -5; j <= 5; ++j) {
            if (j != 0) {
                roomPositions.append(QPoint(0, j * 2));
            }
        }
        int nextRoomId = 1;
        for (const QPoint& position : roomPositions) {
            QVERIFY(addRoomAt(nextRoomId++, areaId, position.x(), position.y(), kEnvRoom));
        }

        T2DMap* p2dMap = prepareWidget(areaId, kZoomFortyPixelRooms, 1.0, 10.0);
        QVERIFY(p2dMap);

        constexpr int panSteps = 5;
        for (int step = 0; step < panSteps; ++step) {
            const double pan = static_cast<double>(step) / panSteps;
            p2dMap->mMapCenterX = pan;
            p2dMap->mMapCenterY = pan;
            const QImage frame = renderFrame(p2dMap);
            QCOMPARE(p2dMap->mRoomWidth, 40.0f);

            for (const QPoint& position : roomPositions) {
                const double rx = position.x() * static_cast<double>(p2dMap->mRoomWidth) + p2dMap->mRX;
                const double ry = position.y() * -1.0 * static_cast<double>(p2dMap->mRoomHeight) + p2dMap->mRY;
                // The room loop draws a room whose centre is on the widget and
                // skips one whose centre is not, whatever its rectangle does
                // at the edge, so that is the set the query has to cover.
                if (rx < 0.0 || ry < 0.0 || rx > kWidgetWidth || ry > kWidgetHeight) {
                    continue;
                }
                const QRectF roomRectangle(rx - p2dMap->mRoomWidth / 2.0, ry - p2dMap->mRoomHeight / 2.0, p2dMap->mRoomWidth, p2dMap->mRoomHeight);
                const QRect onScreen = roomRectangle.toRect().intersected(frame.rect());
                if (countPixels(frame.copy(onScreen), roomColour()) < 1) {
                    QFAIL(qPrintable(qsl("the room at %1,%2 was culled although its centre is on the widget at %3,%4, at a pan of %5 of a cell. The frame is at %6")
                                             .arg(QString::number(position.x()),
                                                  QString::number(position.y()),
                                                  QString::number(qRound(rx)),
                                                  QString::number(qRound(ry)),
                                                  QString::number(pan),
                                                  saveFrame(frame, qsl("viewport-cull")))));
                }
            }
        }
    }

    // The same query, asked from the other end: a zoom no wheel can reach but
    // a script can set. The room coordinates that could be on screen are then
    // billions apart, which does not fit in the int the query hands back, and
    // a double that does not fit in an int is undefined to cast - this build
    // lands on INT_MIN, leaves the rectangle inside out, and the area is not
    // drawn at all. There is nothing to see at this zoom either way; what
    // matters is that the map is still there when the zoom comes back.
    void test_theViewportSurvivesAZoomFarBeyondAPixelPerRoom()
    {
        const int areaId = freshArea(qsl("Extreme Zoom Area"));
        QVERIFY(areaId > 0);
        for (int i = 0; i < 3; ++i) {
            QVERIFY(addRoomAt(i + 1, areaId, i, 0, kEnvRoom));
        }

        T2DMap* p2dMap = prepareWidget(areaId, kZoomFarBeyondAPixelPerRoom, 1.0, 10.0);
        QVERIFY(p2dMap);
        const QImage frame = renderFrame(p2dMap);

        QVERIFY2(p2dMap->mRoomWidth > 0.0f && p2dMap->mRoomWidth < 1.0e-6f, "the zoom was clamped somewhere, so this is no longer the case it is meant to be");
        if (countMarkedPixels(frame) < 1) {
            QFAIL(qPrintable(
                    qsl("the area was not drawn at all at a zoom of %1 - the frame is empty, saved at %2").arg(QString::number(kZoomFarBeyondAPixelPerRoom), saveFrame(frame, qsl("extreme-zoom")))));
        }
    }

    // The player's room is drawn in full and given its "you are here" ring by
    // paintEvent() rather than by the room loop, so each loop has to hand back
    // the fact that it saw the room. Nothing else in this file has the player
    // on screen - kPlayerRoomY parks it far away so its ring cannot be counted
    // as something else - which leaves the reduced tier's half of that
    // handover untested: cut it and the marker goes missing from every map
    // zoomed past four pixels a room, with every other case here still green.
    //
    // A blob written for the player room as well would leave no mark of its
    // own to assert on, since the room and then the ring are painted over the
    // same pixels afterwards.
    void test_thePlayerRoomKeepsItsMarkerInTheReducedTier_data()
    {
        QTest::addColumn<double>("zoom");
        QTest::addColumn<bool>("reducedTier");

        // The full-detail row is the control: it says the ring is found the
        // same way in both, so a missing one is the tier and not the search.
        QTest::newRow("full detail") << kZoomAtThreshold << false;
        QTest::newRow("reduced tier") << kZoomOnePixelRooms << true;
    }

    void test_thePlayerRoomKeepsItsMarkerInTheReducedTier()
    {
        QFETCH(double, zoom);
        QFETCH(bool, reducedTier);

        const int areaId = freshArea(qsl("Player Marker Area"));
        QVERIFY(areaId > 0);
        // The one test that wants the player in the viewport rather than
        // parked outside it.
        QVERIFY(map()->setRoomCoordinates(kPlayerRoomId, 0, 0, 0));
        // A witness that the map was drawn at all, far enough off that the
        // marker cannot reach it.
        QVERIFY(addRoomAt(1, areaId, 20, 0, kEnvRoom));

        T2DMap* p2dMap = prepareWidget(areaId, zoom, 1.0, 2.0);
        QVERIFY(p2dMap);
        const QImage frame = renderFrame(p2dMap);

        if (reducedTier) {
            QVERIFY2(p2dMap->mRoomWidth < 4.0f, "the rooms were not drawn small enough to reach the reduced level of detail");
        } else {
            QCOMPARE(p2dMap->mRoomWidth, 4.0f);
        }

        const int witnessPixels = countPixels(frame, roomColour());
        if (witnessPixels < 1) {
            QFAIL(qPrintable(qsl("the map was not drawn - no pixels of the witness room's environment colour are in the frame, saved at %1").arg(saveFrame(frame, qsl("player-marker")))));
        }

        // The default player room style is a red ring around a white centre,
        // and it is the only red in this palette.
        const QRect markerRect(p2dMap->mRX - 12, p2dMap->mRY - 12, 25, 25);
        const int markerPixels = countPixels(frame.copy(markerRect.intersected(frame.rect())), QColor(255, 0, 0));
        if (markerPixels < 1) {
            QFAIL(qPrintable(qsl("the player room was drawn without its marker - no ring pixels within %1,%2 %3x%4, frame saved at %5")
                                     .arg(QString::number(markerRect.x()),
                                          QString::number(markerRect.y()),
                                          QString::number(markerRect.width()),
                                          QString::number(markerRect.height()),
                                          saveFrame(frame, qsl("player-marker")))));
        }
    }

    // The tier starts below four pixels a room, not at it. A room drawn
    // exactly four pixels across keeps its border; the next zoom out loses it.
    void test_fullDetailIsKeptAtTheThresholdAndDroppedBelowIt()
    {
        const int areaId = freshArea(qsl("Threshold Area"));
        QVERIFY(areaId > 0);
        // Filled black like the background, so the only green in the frame is
        // the border, which is drawn with a fading alpha at these sizes.
        QVERIFY(addRoomAt(1, areaId, 0, 0, kEnvDark));
        // A witness that the map was drawn at all in both frames.
        QVERIFY(addRoomAt(2, areaId, 5, 0, kEnvRoom));
        TRoom* pRoom = map()->mpRoomDB->getRoom(1);
        QVERIFY(pRoom);
        pRoom->mBorderColor = borderColour();

        T2DMap* p2dMap = prepareWidget(areaId, kZoomAtThreshold, 1.0, 2.0);
        QVERIFY(p2dMap);
        const QImage atThreshold = renderFrame(p2dMap);
        QCOMPARE(p2dMap->mRoomWidth, 4.0f);
        const int borderPixelsAtThreshold = countPixels(atThreshold, borderColour());
        const int roomPixelsAtThreshold = countPixels(atThreshold, roomColour());

        p2dMap = prepareWidget(areaId, kZoomJustInsideTier, 1.0, 2.0);
        QVERIFY(p2dMap);
        const QImage insideTier = renderFrame(p2dMap);
        QVERIFY2(p2dMap->mRoomWidth < 4.0f, "the second frame was not drawn inside the reduced tier");
        QVERIFY2(p2dMap->mRoomWidth > 3.5f, "the second frame was drawn much smaller than the first, so any difference could be the size rather than the tier");
        const int borderPixelsInsideTier = countPixels(insideTier, borderColour());
        const int roomPixelsInsideTier = countPixels(insideTier, roomColour());

        if (roomPixelsAtThreshold < 8 || roomPixelsInsideTier < 8) {
            QFAIL(qPrintable(qsl("the map was not drawn in one of the two frames - %1 and %2 pixels of the witness room's colour, saved at %3 and %4")
                                     .arg(QString::number(roomPixelsAtThreshold),
                                          QString::number(roomPixelsInsideTier),
                                          saveFrame(atThreshold, qsl("at-threshold")),
                                          saveFrame(insideTier, qsl("inside-tier")))));
        }
        if (borderPixelsAtThreshold <= 0) {
            QFAIL(qPrintable(qsl("a room drawn exactly at the threshold lost its border, so the reduced tier starts one zoom step too early. The frame is at %1")
                                     .arg(saveFrame(atThreshold, qsl("at-threshold")))));
        }
        if (borderPixelsInsideTier != 0) {
            QFAIL(qPrintable(qsl("a room drawn inside the reduced tier still had its border, so the tier was never entered - %1 pixels of the border colour, frame at %2")
                                     .arg(QString::number(borderPixelsInsideTier), saveFrame(insideTier, qsl("inside-tier")))));
        }
    }

    // The reduced tier no longer asks every room on the level about its exits:
    // the area hands over just the rooms whose exits can beat the cut, from an
    // index it keeps current as the map changes. These tests are about that
    // upkeep. Each draws a frame first, so the index is built and believed,
    // then changes the map in a way that gives a room something to draw, and
    // requires the next frame to show it - a missed invalidation leaves the
    // first frame's answer in charge forever, which no amount of superset
    // generosity in the index itself can cover for.
    //
    // The filler rooms are what keep the index path taken at all: it is only
    // used while it offers fewer rooms than the viewport query does, so a map
    // where most rooms are exit-survivors would fall back to the old loop and
    // pass these tests without consulting the index.

    // A brand new long exit has to be drawn by the very next frame, whichever
    // of the two setters made it. They are separate paths with a hook each:
    // TMap::setExit() writes through the room's per-direction setters, while
    // the room exits dialog calls TRoom::setExit() instead.
    void test_aLongExitCreatedAfterAFrameIsDrawnByTheNextFrame_data()
    {
        QTest::addColumn<bool>("throughRoomSetter");

        QTest::newRow("map setter") << false;
        QTest::newRow("room exits dialog setter") << true;
    }

    void test_aLongExitCreatedAfterAFrameIsDrawnByTheNextFrame()
    {
        QFETCH(bool, throughRoomSetter);

        TMap* pMap = map();
        const int areaId = freshArea(qsl("Fresh Exit Area"));
        QVERIFY(areaId > 0);
        QVERIFY(addRoomAt(1, areaId, 0, 0, kEnvRoom));
        QVERIFY(addRoomAt(2, areaId, 150, 0, kEnvRoom));
        for (int i = 0; i < 6; ++i) {
            QVERIFY(addRoomAt(10 + i, areaId, i * 2 - 6, 3, kEnvRoom));
        }

        T2DMap* p2dMap = prepareWidget(areaId, kZoomOnePixelRooms, 0.5, 10.0);
        QVERIFY(p2dMap);
        const QImage before = renderFrame(p2dMap);
        QVERIFY2(indexPathTaken(p2dMap), "the reduced tier fell back to the loop over every room, so this test never consulted the exit index");
        QVERIFY2(p2dMap->mRoomWidth < 4.0f, "the rooms were not drawn small enough to reach the reduced level of detail");
        const int exitPixelsBefore = countPixels(before, exitColour());
        QCOMPARE(exitPixelsBefore, 0);

        if (throughRoomSetter) {
            TRoom* pSource = pMap->mpRoomDB->getRoom(1);
            TRoom* pDestination = pMap->mpRoomDB->getRoom(2);
            QVERIFY(pSource && pDestination);
            QVERIFY(pSource->setExit(2, DIR_EAST));
            QVERIFY(pDestination->setExit(1, DIR_WEST));
        } else {
            QVERIFY(pMap->setExit(1, 2, DIR_EAST));
            QVERIFY(pMap->setExit(2, 1, DIR_WEST));
        }
        const QImage after = renderFrame(p2dMap);
        QVERIFY2(indexPathTaken(p2dMap), "the reduced tier fell back to the loop over every room, so this test never consulted the exit index");

        const int exitPixels = countPixels(after, exitColour());
        if (exitPixels <= 100) {
            QFAIL(qPrintable(qsl("the 150 unit long exit created after the first frame was not drawn by the second - only %1 pixels of the exit colour are in the frame, saved at %2")
                                     .arg(QString::number(exitPixels), saveFrame(after, qsl("fresh-long-exit")))));
        }
    }

    // Moving a room does not touch any exit, but it changes how far every exit
    // to it has to reach, so a move has to reopen the question too. The two
    // rooms start next door to each other - an exit the tier drops, and one
    // too short for the index to even store - and end 150 cells apart.
    void test_anExitStretchedByMovingItsDestinationIsDrawnByTheNextFrame()
    {
        TMap* pMap = map();
        const int areaId = freshArea(qsl("Stretched Exit Area"));
        QVERIFY(areaId > 0);
        QVERIFY(addRoomAt(1, areaId, 0, 0, kEnvRoom));
        QVERIFY(addRoomAt(2, areaId, 1, 0, kEnvRoom));
        QVERIFY(pMap->setExit(1, 2, DIR_EAST));
        QVERIFY(pMap->setExit(2, 1, DIR_WEST));
        for (int i = 0; i < 6; ++i) {
            QVERIFY(addRoomAt(10 + i, areaId, i * 2 - 6, 3, kEnvRoom));
        }

        T2DMap* p2dMap = prepareWidget(areaId, kZoomOnePixelRooms, 0.5, 10.0);
        QVERIFY(p2dMap);
        const QImage before = renderFrame(p2dMap);
        QVERIFY2(indexPathTaken(p2dMap), "the reduced tier fell back to the loop over every room, so this test never consulted the exit index");
        QVERIFY2(p2dMap->mRoomWidth < 4.0f, "the rooms were not drawn small enough to reach the reduced level of detail");
        QCOMPARE(countPixels(before, exitColour()), 0);

        QVERIFY(pMap->setRoomCoordinates(2, 150, 0, 0));
        const QImage after = renderFrame(p2dMap);
        QVERIFY2(indexPathTaken(p2dMap), "the reduced tier fell back to the loop over every room, so this test never consulted the exit index");

        const int exitPixels = countPixels(after, exitColour());
        if (exitPixels <= 100) {
            QFAIL(qPrintable(qsl("the exit stretched to 150 units by moving its destination was not drawn by the next frame - only %1 pixels of the exit colour are in the frame, saved at %2")
                                     .arg(QString::number(exitPixels), saveFrame(after, qsl("stretched-exit")))));
        }
    }

    // An exit stub is drawn at every zoom - it is half a room long however
    // small that is, and it is all that records an unmapped exit - so a room
    // whose only feature is a stub has to be handed over by the index, and a
    // stub added after a frame has to reopen the question like an exit does.
    void test_anExitStubAddedAfterAFrameIsDrawnByTheNextFrame()
    {
        const int areaId = freshArea(qsl("Stub Area"));
        QVERIFY(areaId > 0);
        QVERIFY(addRoomAt(1, areaId, 0, 0, kEnvDark));
        for (int i = 0; i < 6; ++i) {
            QVERIFY(addRoomAt(10 + i, areaId, i * 2 - 6, 4, kEnvDark));
        }

        // The fat exit pen from the area exit marker test, for the same
        // reason: a stub is short, so what it has must land solid.
        T2DMap* p2dMap = prepareWidget(areaId, kZoomThreePixelRooms, 1.0, 10.0, 1.0);
        QVERIFY(p2dMap);
        const QImage before = renderFrame(p2dMap);
        QVERIFY2(indexPathTaken(p2dMap), "the reduced tier fell back to the loop over every room, so this test never consulted the exit index");
        QVERIFY2(p2dMap->mRoomWidth < 4.0f, "the rooms were not drawn small enough to reach the reduced level of detail");
        QCOMPARE(countPixels(before, exitColour()), 0);

        TRoom* pRoom = map()->mpRoomDB->getRoom(1);
        QVERIFY(pRoom);
        pRoom->setExitStub(DIR_EAST, true);
        const QImage after = renderFrame(p2dMap);
        QVERIFY2(indexPathTaken(p2dMap), "the reduced tier fell back to the loop over every room, so this test never consulted the exit index");

        const int stubPixels = countPixels(after, exitColour());
        if (stubPixels < 1) {
            QFAIL(qPrintable(qsl("the exit stub added after the first frame was not drawn by the second - no pixels of the exit colour are in the frame, saved at %1")
                                     .arg(saveFrame(after, qsl("fresh-stub")))));
        }
    }

    // A room can join the area without an exit or a coordinate changing, which
    // is what setRoomArea() does, and it brings its own exits with it. Nothing
    // else here reopens the question: the coordinates are untouched, so nothing
    // reaches TArea::moveRoom(), and the exit was set while the room belonged
    // somewhere else.
    //
    // The exit is deliberately one way, which draws it as a dotted line and so
    // as fewer pixels than the tests above look for. Giving room 1 the return
    // exit would put room 1 into the index at the first frame - as a room with
    // an exit into another area - and it would then draw that same line from
    // its stale entry whether or not the arrival was ever noticed.
    void test_aRoomJoiningTheAreaBringsItsLongExitToTheNextFrame()
    {
        TMap* pMap = map();
        const int areaId = freshArea(qsl("Joined Room Area"));
        QVERIFY(areaId > 0);
        const int otherAreaId = pMap->mpRoomDB->addArea(qsl("Departure Area"));
        QVERIFY(otherAreaId > 0);
        QVERIFY(addRoomAt(1, areaId, 0, 0, kEnvRoom));
        QVERIFY(addRoomAt(2, otherAreaId, 150, 0, kEnvRoom));
        QVERIFY(pMap->setExit(2, 1, DIR_WEST));
        for (int i = 0; i < 6; ++i) {
            QVERIFY(addRoomAt(10 + i, areaId, i * 2 - 6, 3, kEnvRoom));
        }

        T2DMap* p2dMap = prepareWidget(areaId, kZoomOnePixelRooms, 0.5, 10.0);
        QVERIFY(p2dMap);
        const QImage before = renderFrame(p2dMap);
        QVERIFY2(indexPathTaken(p2dMap), "the reduced tier fell back to the loop over every room, so this test never consulted the exit index");
        QVERIFY2(p2dMap->mRoomWidth < 4.0f, "the rooms were not drawn small enough to reach the reduced level of detail");
        QCOMPARE(countPixels(before, exitColour()), 0);

        QVERIFY(pMap->setRoomArea(2, areaId));
        const QImage after = renderFrame(p2dMap);
        QVERIFY2(indexPathTaken(p2dMap), "the reduced tier fell back to the loop over every room, so this test never consulted the exit index");

        const int exitPixels = countPixels(after, exitColour());
        if (exitPixels <= 20) {
            QFAIL(qPrintable(qsl("the 150 unit long exit of the room that joined the area was not drawn by the next frame - only %1 pixels of the exit colour are in the frame, saved at %2")
                                     .arg(QString::number(exitPixels), saveFrame(after, qsl("joined-room-exit")))));
        }
    }

    // The other half of the same move: a room that leaves the area has to
    // leave the index with it. Nothing downstream of the index asks a room it
    // is handed which area that room belongs to, so a leftover entry paints
    // the departed room's exits into the frame of the area it has left - the
    // one direction in which a stale index is not merely wasteful. The exit
    // here is one way so that only the departed room can draw the line, which
    // is what makes its absence the thing being counted.
    void test_aRoomLeavingTheAreaTakesItsLongExitWithIt()
    {
        TMap* pMap = map();
        const int areaId = freshArea(qsl("Departed Room Area"));
        QVERIFY(areaId > 0);
        const int otherAreaId = pMap->mpRoomDB->addArea(qsl("Arrival Area"));
        QVERIFY(otherAreaId > 0);
        QVERIFY(addRoomAt(1, areaId, 0, 0, kEnvAreaExitDestination));
        QVERIFY(addRoomAt(2, areaId, 150, 0, kEnvAreaExitDestination));
        QVERIFY(pMap->setExit(2, 1, DIR_WEST));
        for (int i = 0; i < 6; ++i) {
            QVERIFY(addRoomAt(10 + i, areaId, i * 2 - 6, 3, kEnvRoom));
        }

        T2DMap* p2dMap = prepareWidget(areaId, kZoomOnePixelRooms, 0.5, 10.0);
        QVERIFY(p2dMap);
        const QImage before = renderFrame(p2dMap);
        QVERIFY2(indexPathTaken(p2dMap), "the reduced tier fell back to the loop over every room, so this test never consulted the exit index");
        QVERIFY2(p2dMap->mRoomWidth < 4.0f, "the rooms were not drawn small enough to reach the reduced level of detail");
        QVERIFY2(countPixels(before, exitColour()) > 20, "the exit that the departing room has to take with it was not drawn in the first place");

        QVERIFY(pMap->setRoomArea(2, otherAreaId));
        const QImage after = renderFrame(p2dMap);
        QVERIFY2(indexPathTaken(p2dMap), "the reduced tier fell back to the loop over every room, so this test never consulted the exit index");

        const int exitPixels = countPixels(after, exitColour());
        if (exitPixels > 0) {
            QFAIL(qPrintable(qsl("the exit of the room that left the area was still drawn into it - %1 pixels of the exit colour are in the frame, saved at %2")
                                     .arg(QString::number(exitPixels), saveFrame(after, qsl("departed-room-exit")))));
        }
    }

    // Every hook above re-files the one room it touched. The whole-area
    // rebuild behind them is the fall-back for a map that changed without
    // saying so, and it costs a pass over every room with a lookup per exit -
    // 215 ms on the largest map this has been measured against, against the
    // 511 ms the surrounding calcSpan() takes there - so an ordinary edit
    // reaching for it would trade one slow frame for a much slower one on
    // every edit. No frame can tell a rebuilt index from a re-filed one,
    // which is why the rebuild count is what this asserts on; the frames are
    // checked as well so that an index which simply stopped being maintained
    // could not pass.
    void test_anOrdinaryEditReFilesOneRoomRatherThanRebuildingTheArea()
    {
        TMap* pMap = map();
        const int areaId = freshArea(qsl("Incrementally Maintained Area"));
        QVERIFY(areaId > 0);
        const int otherAreaId = pMap->mpRoomDB->addArea(qsl("Somewhere Else"));
        QVERIFY(otherAreaId > 0);
        QVERIFY(addRoomAt(1, areaId, 0, 0, kEnvRoom));
        QVERIFY(addRoomAt(2, areaId, 1, 0, kEnvRoom));
        for (int i = 0; i < 6; ++i) {
            QVERIFY(addRoomAt(10 + i, areaId, i * 2 - 6, 3, kEnvRoom));
        }

        T2DMap* p2dMap = prepareWidget(areaId, kZoomOnePixelRooms, 0.5, 10.0);
        QVERIFY(p2dMap);
        const QImage before = renderFrame(p2dMap);
        QVERIFY2(p2dMap->mRoomWidth < 4.0f, "the rooms were not drawn small enough to reach the reduced level of detail");
        QVERIFY2(indexPathTaken(p2dMap), "the reduced tier fell back to the loop over every room, so this test never consulted the exit index");
        QCOMPARE(countPixels(before, exitColour()), 0);

        TArea* pArea = pMap->mpRoomDB->getArea(areaId);
        QVERIFY(pArea);
        const quint32 rebuildsAfterFirstFrame = pArea->lodExitIndexRebuildCount();
        QVERIFY2(rebuildsAfterFirstFrame > 0, "the first frame did not build the index, so this test cannot tell a rebuild from the lack of one");

        // One edit of each kind that has a hook: an exit, a move, an arrival
        // and a departure.
        QVERIFY(pMap->setExit(1, 2, DIR_EAST));
        QVERIFY(pMap->setExit(2, 1, DIR_WEST));
        QVERIFY(pMap->setRoomCoordinates(2, 150, 0, 0));
        QVERIFY(addRoomAt(3, otherAreaId, -150, 0, kEnvRoom));
        QVERIFY(pMap->setRoomArea(3, areaId));
        QVERIFY(pMap->setRoomArea(3, otherAreaId));

        const QImage after = renderFrame(p2dMap);
        QVERIFY2(indexPathTaken(p2dMap), "the reduced tier fell back to the loop over every room, so this test never consulted the exit index");
        const int exitPixels = countPixels(after, exitColour());
        if (exitPixels <= 100) {
            QFAIL(qPrintable(qsl("the exit stretched to 150 units across those edits was not drawn by the next frame - only %1 pixels of the exit colour are in the frame, saved at %2")
                                     .arg(QString::number(exitPixels), saveFrame(after, qsl("incremental-edits")))));
        }
        QCOMPARE(pArea->lodExitIndexRebuildCount(), rebuildsAfterFirstFrame);
    }

    // The room that changed is not always the room that has to be re-filed.
    // An exit one room long draws nothing in this tier and so keeps its source
    // out of the index entirely - until the room it leads to leaves the area,
    // at which point that same exit becomes an area exit and owes a marker
    // whatever the zoom. Nothing about the source room changed, so only the
    // pass over the rooms with exits leading to the departed one can catch it.
    void test_aShortExitBecomesVisibleWhenItsDestinationLeavesTheArea()
    {
        TMap* pMap = map();
        const int areaId = freshArea(qsl("Losing A Room"));
        QVERIFY(areaId > 0);
        const int otherAreaId = pMap->mpRoomDB->addArea(qsl("Gaining A Room"));
        QVERIFY(otherAreaId > 0);
        QVERIFY(addRoomAt(1, areaId, 0, 0, kEnvRoom));
        QVERIFY(addRoomAt(2, areaId, 1, 0, kEnvRoom));
        QVERIFY(pMap->setExit(1, 2, DIR_EAST));
        for (int i = 0; i < 6; ++i) {
            QVERIFY(addRoomAt(10 + i, areaId, i * 2 - 6, 4, kEnvRoom));
        }

        // The same geometry and fat pen as the area exit marker test above,
        // for the same reasons.
        T2DMap* p2dMap = prepareWidget(areaId, kZoomNearlyFourPixelRooms, 1.0, 10.0, 1.0);
        QVERIFY(p2dMap);
        p2dMap->mLargeAreaExitArrows = true;
        const QImage before = renderFrame(p2dMap);
        QVERIFY2(p2dMap->mRoomWidth < 4.0f, "the rooms were not drawn small enough to reach the reduced level of detail");
        QVERIFY2(indexPathTaken(p2dMap), "the reduced tier fell back to the loop over every room, so this test never consulted the exit index");
        QCOMPARE(countPixels(before, areaExitColour()), 0);

        // The colour only tells the marker apart from the rooms once the room
        // it points at is somewhere else, so it is given late:
        TRoom* pDestination = pMap->mpRoomDB->getRoom(2);
        QVERIFY(pDestination);
        pDestination->environment = kEnvAreaExitDestination;
        QVERIFY(pMap->setRoomArea(2, otherAreaId));

        const QImage after = renderFrame(p2dMap);
        QVERIFY2(indexPathTaken(p2dMap), "the reduced tier fell back to the loop over every room, so this test never consulted the exit index");
        const int markerPixels = countPixels(after, areaExitColour());
        if (markerPixels < 3) {
            QFAIL(qPrintable(qsl("the exit became an area exit when its destination left, but its marker was not drawn - only %1 pixels of the destination area's colour are in the frame, saved at %2")
                                     .arg(QString::number(markerPixels), saveFrame(after, qsl("newly-area-exit")))));
        }
    }

    // The index is only ever allowed to hand over too many rooms, never too
    // few: a room it leaves out loses whatever that room would have drawn,
    // and nothing downstream can put it back. The tests above each name one
    // room that has to be there, which only covers the cases somebody thought
    // of. This one asks the question the other way round - the same map drawn
    // both ways has to paint the same pixels - so a span, a Z level or a kind
    // of exit nobody considered still has to survive.
    //
    // The map is built to sit on the edges: exits of every span from one room
    // unit to ninety, along both axes and diagonally, a stub, an exit out of
    // the area, a custom line, and a second Z level the frame must not show
    // anything of.
    void test_theIndexPathDrawsTheSameFrameAsTheLoopOverEveryRoom_data()
    {
        QTest::addColumn<double>("zoom");
        QTest::addColumn<double>("roomSize");
        QTest::addColumn<double>("lineSize");
        QTest::addColumn<double>("panX");
        QTest::addColumn<double>("panY");

        // Each row moves the cut-off along the ladder of exit spans the map is
        // built from, so between them every span sits on both sides of it and
        // several land right on the boundary. Rows where nothing at all can be
        // dropped are not in the list: the index is not consulted there, so
        // there would be nothing to compare.
        //
        // The first row is the only one where an exit stub still has pixels of
        // its own - a stub is about a room long, so anything zoomed out far
        // enough to drop a one room exit drops the stub with it.
        QTest::newRow("rooms as wide as the shortest exit") << kZoomThreePixelRooms << 1.0 << 1.0 << 0.0 << 0.0;
        QTest::newRow("one pixel rooms") << kZoomOnePixelRooms << 0.5 << 10.0 << 0.0 << 0.0;
        QTest::newRow("half a pixel per room") << kZoomHalfPixelRooms << 0.5 << 10.0 << 0.0 << 0.0;
        QTest::newRow("a quarter of a pixel per room") << 1600.0 << 0.5 << 10.0 << 0.0 << 0.0;
        QTest::newRow("a tenth of a pixel per room") << 4000.0 << 0.5 << 10.0 << 0.0 << 0.0;
        QTest::newRow("every exit on the map too short to show") << 40000.0 << 0.5 << 10.0 << 0.0 << 0.0;
        QTest::newRow("far beyond a pixel per room") << kZoomFarBeyondAPixelPerRoom << 0.5 << 10.0 << 0.0 << 0.0;

        // The rows above all look at the level head on, where the viewport
        // holds every room of it. The index knows nothing of where the view
        // is, so the two paths only agree off centre because both cull inside
        // the loop - and that is the one leg of the argument nothing above
        // puts any weight on. These two park the view so that a good part of
        // the level, exits and all, is off the left and top edges.
        QTest::newRow("panned off the left and top") << kZoomThreePixelRooms << 1.0 << 1.0 << 60.0 << 30.0;
        QTest::newRow("panned off the right and bottom") << kZoomThreePixelRooms << 1.0 << 1.0 << -60.0 << -30.0;
    }

    void test_theIndexPathDrawsTheSameFrameAsTheLoopOverEveryRoom()
    {
        QFETCH(double, zoom);
        QFETCH(double, roomSize);
        QFETCH(double, lineSize);
        QFETCH(double, panX);
        QFETCH(double, panY);

        TMap* pMap = map();
        const int areaId = freshArea(qsl("Equivalence Area"));
        QVERIFY(areaId > 0);
        const int otherAreaId = pMap->mpRoomDB->addArea(qsl("Across The Border"));
        QVERIFY(otherAreaId > 0);

        // A pair per span, each pair on its own row so the lines cannot land
        // on top of one another.
        int nextRoomId = 1;
        const QList<int> spans{1, 2, 3, 4, 5, 6, 7, 8, 12, 30, 90};
        for (int i = 0; i < spans.size(); ++i) {
            const int span = spans.at(i);
            const int row = i * 2 - spans.size();
            const int from = nextRoomId++;
            const int to = nextRoomId++;
            QVERIFY(addRoomAt(from, areaId, -span / 2, row, kEnvRoom));
            QVERIFY(addRoomAt(to, areaId, -span / 2 + span, row, kEnvRoom));
            QVERIFY(pMap->setExit(from, to, DIR_EAST));
            QVERIFY(pMap->setExit(to, from, DIR_WEST));
        }
        // The same spans again on the other axis and on a diagonal, since a
        // blob is a rectangle and the cut is made per axis.
        for (const int span : spans) {
            const int from = nextRoomId++;
            const int to = nextRoomId++;
            QVERIFY(addRoomAt(from, areaId, 40, -span / 2, kEnvRoom));
            QVERIFY(addRoomAt(to, areaId, 40, -span / 2 + span, kEnvRoom));
            QVERIFY(pMap->setExit(from, to, DIR_NORTH));
            const int diagonalFrom = nextRoomId++;
            const int diagonalTo = nextRoomId++;
            QVERIFY(addRoomAt(diagonalFrom, areaId, -40, -span / 2, kEnvRoom));
            QVERIFY(addRoomAt(diagonalTo, areaId, -40 + span, -span / 2 + span, kEnvRoom));
            QVERIFY(pMap->setExit(diagonalFrom, diagonalTo, DIR_NORTHEAST));
        }

        // A stub, an exit out of the area, and a custom line - none of them a
        // plain line between two rooms of this area.
        const int stubRoomId = nextRoomId++;
        QVERIFY(addRoomAt(stubRoomId, areaId, 60, 0, kEnvRoom));
        TRoom* pStubRoom = pMap->mpRoomDB->getRoom(stubRoomId);
        QVERIFY(pStubRoom);
        pStubRoom->setExitStub(DIR_EAST, true);

        const int borderRoomId = nextRoomId++;
        const int abroadRoomId = nextRoomId++;
        QVERIFY(addRoomAt(borderRoomId, areaId, 60, 5, kEnvAreaExitDestination));
        QVERIFY(addRoomAt(abroadRoomId, otherAreaId, 61, 5, kEnvAreaExitDestination));
        QVERIFY(pMap->setExit(borderRoomId, abroadRoomId, DIR_EAST));

        const int customLineRoomId = nextRoomId++;
        QVERIFY(addRoomAt(customLineRoomId, areaId, -60, 8, kEnvRoom));
        TRoom* pCustomLineRoom = pMap->mpRoomDB->getRoom(customLineRoomId);
        QVERIFY(pCustomLineRoom);
        pCustomLineRoom->customLines[qsl("n")] = QList<QPointF>{QPointF(-60, 8), QPointF(-20, 60)};
        pCustomLineRoom->customLinesColor[qsl("n")] = customLineColour();
        pCustomLineRoom->customLinesStyle[qsl("n")] = Qt::SolidLine;
        pCustomLineRoom->customLinesArrow[qsl("n")] = false;
        pCustomLineRoom->calcRoomDimensions();

        // A whole second level, long exits and all, that the frame below must
        // show nothing of.
        for (int i = 0; i < 8; ++i) {
            const int from = nextRoomId++;
            const int to = nextRoomId++;
            QVERIFY(addRoomAt(from, areaId, i * 3 - 12, 0, kEnvHidden, 1));
            QVERIFY(addRoomAt(to, areaId, i * 3 - 12, 120, kEnvHidden, 1));
            QVERIFY(pMap->setExit(from, to, DIR_NORTH));
        }

        T2DMap* p2dMap = prepareWidget(areaId, zoom, roomSize, 10.0, lineSize);
        QVERIFY(p2dMap);
        p2dMap->mMapCenterX = panX;
        p2dMap->mMapCenterY = panY;
        const QImage throughTheIndex = renderFrame(p2dMap);
        QVERIFY2(p2dMap->mRoomWidth < 4.0f, "the rooms were not drawn small enough to reach the reduced level of detail");
        QVERIFY2(indexPathTaken(p2dMap), "the reduced tier fell back to the loop over every room, so there was nothing to compare it against");
        if (panX != 0.0 || panY != 0.0) {
            // A pan the widget is wide enough to swallow proves nothing, so
            // insist the level really does run past an edge.
            QVERIFY2(touchesAnEdge(throughTheIndex), "the pan left the whole level inside the widget, so nothing was culled");
        }

        p2dMap->mLodExitIndexDisabled = true;
        const QImage throughTheLoop = renderFrame(p2dMap);
        p2dMap->mLodExitIndexDisabled = false;
        QVERIFY2(!indexPathTaken(p2dMap), "the fall-back was asked for and not taken");

        // Which pixels got painted, not what colour they came out. The two
        // paths hand the rooms over in different orders, and two overlapping
        // lines blend to whichever was drawn second, so a handful of pixels
        // where an arrow head crosses a line legitimately differ in shade.
        // What may not differ is a pixel one frame paints and the other
        // leaves as background: that is a room the index left out.
        const QRgb background = throughTheLoop.pixel(0, 0);
        int firstDifferenceX = -1;
        int firstDifferenceY = -1;
        int differences = 0;
        for (int y = 0; y < throughTheLoop.height(); ++y) {
            for (int x = 0; x < throughTheLoop.width(); ++x) {
                if ((throughTheIndex.pixel(x, y) == background) == (throughTheLoop.pixel(x, y) == background)) {
                    continue;
                }
                if (!differences++) {
                    firstDifferenceX = x;
                    firstDifferenceY = y;
                }
            }
        }
        if (differences) {
            QFAIL(qPrintable(qsl("the index path painted %1 pixels that the loop over every room did not, or left that many unpainted - first at %2,%3; frames saved at %4 and %5")
                                     .arg(QString::number(differences),
                                          QString::number(firstDifferenceX),
                                          QString::number(firstDifferenceY),
                                          saveFrame(throughTheIndex, qsl("equivalence-index")),
                                          saveFrame(throughTheLoop, qsl("equivalence-loop")))));
        }
    }
};

#include "MapLevelOfDetailTest.moc"
MUDLET_GROUPED_TEST_MAIN(MapLevelOfDetailTest)
