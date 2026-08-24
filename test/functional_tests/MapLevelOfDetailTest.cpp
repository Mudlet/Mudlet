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
 * A Lua spec cannot reach any of this: the only evidence is pixels, and
 * nothing in the Lua API reads back what the mapper drew.
 *
 * Run with: ctest -R MapLevelOfDetailTest -V
 */

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
    // Half a pixel per room, so a quarter of a pixel of room once the room size
    // fraction is applied, which rounds to none.
    static constexpr double kZoomHalfPixelRooms = 800.0;

    // Parked outside every viewport used here, so the player room's own
    // full-detail drawing and its highlight gradient cannot be mistaken for
    // anything a test is counting.
    static constexpr int kPlayerRoomId = 999;
    static constexpr int kPlayerRoomY = 1000;

    // Environment ids above 255 miss every branch of the built-in palette, so
    // the colour is whatever mCustomEnvColors was given.
    static constexpr int kEnvRoom = 501;
    static constexpr int kEnvHidden = 502;
    static constexpr int kEnvAreaExitDestination = 503;
    static constexpr int kEnvDark = 504;
    // setCustomEnvColor() takes an alpha, so an environment colour that is
    // only partly opaque is something a script can ask for.
    static constexpr int kEnvTranslucent = 505;

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
        QDir dir(mudlet::getMudletPath(enums::profileHomePath, mProfileName));
        if (dir.exists()) {
            dir.removeRecursively();
        }
    }

    TMap* map() const { return mpHost->mpMap.data(); }

    bool addRoomAt(const int id, const int areaId, const int x, const int y, const int environment) const
    {
        if (!map()->addRoom(id) || !map()->setRoomArea(id, areaId) || !map()->setRoomCoordinates(id, x, y, 0)) {
            return false;
        }
        TRoom* pRoom = map()->mpRoomDB->getRoom(id);
        if (!pRoom) {
            return false;
        }
        pRoom->environment = environment;
        return true;
    }

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
};

#include "MapLevelOfDetailTest.moc"
MUDLET_GROUPED_TEST_MAIN(MapLevelOfDetailTest)
