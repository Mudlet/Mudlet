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

#include <QDir>
#include <QImage>
#include <QPainter>
#include <QPointer>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest/QtTest>

#include <optional>

#include "Host.h"
#include "HostManager.h"
#include "MudletApp.h"
#include "PortableModeTestHelper.h"
#include "THeadlessSession.h"
#include "TLuaInterpreter.h"
#include "TMap.h"
#include "TMapViewFrontend.h"

#include "GroupedTest.h"

// An app that embeds Mudlet drawing the profile's 2D map, as the GUI's mapper shows it
class HeadlessSessionMapTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;

    static constexpr int scmWidth = 400;
    static constexpr int scmHeight = 300;
    static QColor westColour() { return QColor(200, 30, 30); }
    static QColor eastColour() { return QColor(30, 30, 200); }

    static bool writeProfile(const QString& name)
    {
        const QString saveFolder = MudletApp::getMudletPath(enums::profileXmlFilesPath, name);
        if (!QDir().mkpath(saveFolder)) {
            return false;
        }
        QFile saveFile(qsl("%1/2020-01-01#00-00-00.xml").arg(saveFolder));
        if (!saveFile.open(QIODevice::WriteOnly)) {
            return false;
        }
        saveFile.write(R"(<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE MudletPackage>
<MudletPackage version="1.001">
<HostPackage><Host></Host></HostPackage>
</MudletPackage>
)");
        return true;
    }

    static QString luaGlobalString(Host* host, const char* name)
    {
        lua_State* L = host->getLuaInterpreter()->getLuaGlobalState();
        lua_getglobal(L, name);
        const QString value = lua_isstring(L, -1) ? QString::fromUtf8(lua_tostring(L, -1)) : QString();
        lua_pop(L, 1);
        return value;
    }

    static QImage blankImage()
    {
        QImage image(scmWidth, scmHeight, QImage::Format_ARGB32_Premultiplied);
        image.fill(Qt::black);
        return image;
    }

    static std::optional<QImage> rendered(THeadlessSession& session)
    {
        QImage image = blankImage();
        QPainter painter(&image);
        if (!session.renderMap(painter, image.size())) {
            return std::nullopt;
        }
        painter.end();
        return image;
    }

    static QString saved(const QImage& image, const QString& name)
    {
        const QString path = qsl("%1/HeadlessSessionMapTest-%2.png").arg(QDir::tempPath(), name);
        return image.save(path) ? qsl("The image is at %1").arg(path) : qsl("The image could not be saved to %1").arg(path);
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
        QCOMPARE(MudletApp::getMudletPath(enums::mainPath), qsl("%1/mudlet").arg(mConfigDir.path()));
        QVERIFY2(!HostManager::self(), "A profile pool already exists, so a session cannot make its own.");
    }

    void cleanupTestCase() { mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg); }

    // Room 5 in the west area and room 6 in the east one, each the only room of its area and away from its
    // area's origin, so only a view centred on the room has it in the middle
    void test_anAppDrawsTheMapCentredOnThePlayer()
    {
        const QString name = qsl("Headless Map Drawn");
        QVERIFY(writeProfile(name));
        THeadlessSession session;
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);

        QVERIFY(!rendered(session));
        QCOMPARE(session.errorString(), qsl("No map is open; a script opens one, as with openMapWidget()."));

        QVERIFY(session.runLua(qsl(R"lua(
openMapWidget()
headlessWest = addAreaName("Headless West")
headlessEast = addAreaName("Headless East")
setCustomEnvColor(301, 200, 30, 30, 255)
setCustomEnvColor(302, 30, 30, 200, 255)
addRoom(5)
setRoomArea(5, headlessWest)
setRoomCoordinates(5, 3, 2, 0)
setRoomEnv(5, 301)
addRoom(6)
setRoomArea(6, headlessEast)
setRoomCoordinates(6, -4, 1, 0)
setRoomEnv(6, 302)
setMapZoom(10, headlessWest)
setMapZoom(10, headlessEast)
headlessAreaChanges = ""
registerAnonymousEventHandler("sysMapAreaChanged", function(_, areaId, lastAreaId)
  headlessAreaChanges = headlessAreaChanges .. areaId .. ":" .. lastAreaId .. " "
end)
headlessMapInfoCalls = 0
registerMapInfo("HeadlessCount", function(roomId)
  headlessMapInfoCalls = headlessMapInfoCalls + 1
  headlessMapInfoRoom = roomId
  return "counted"
end)
enableMapInfo("HeadlessCount")
-- With no player's room the mapper centres on whichever room the map hands it first
centerview(5)
)lua")));
        const QString westId = luaGlobalString(session.host(), "headlessWest");
        const QString eastId = luaGlobalString(session.host(), "headlessEast");
        // A ring with a clear middle, so the room shows through the player's marker
        TMap* pMap = session.host()->mpMap.data();
        pMap->mPlayerRoomStyle = 1;
        pMap->mPlayerRoomInnerDiameterPercentage = 70;
        QTest::qWait(50);

        QSignalSpy repaints(&session, &THeadlessSession::mapRepaintRequested);
        QVERIFY(session.runLua(qsl("centerview(5)")));
        QCOMPARE(repaints.count(), 0);
        QTRY_COMPARE_WITH_TIMEOUT(repaints.count(), 1, 5000);
        QCOMPARE(repaints.at(0).at(0).toString(), name);
        QTest::qWait(100);
        // centerview() asks for the area to be shown and for the player's area, which the GUI paints as one
        QCOMPARE(repaints.count(), 1);

        const std::optional<QImage> west = rendered(session);
        QVERIFY2(west, qPrintable(session.errorString()));
        QVERIFY2(west->pixelColor(scmWidth / 2, scmHeight / 2) == westColour(), qPrintable(saved(*west, qsl("west"))));
        QCOMPARE(luaGlobalString(session.host(), "headlessMapInfoCalls"), qsl("1"));
        QCOMPARE(luaGlobalString(session.host(), "headlessMapInfoRoom"), qsl("5"));
        QCOMPARE(luaGlobalString(session.host(), "headlessAreaChanges"), qsl("%1:-2 ").arg(westId));
        QVERIFY(rendered(session));
        QCOMPARE(luaGlobalString(session.host(), "headlessMapInfoCalls"), qsl("2"));
        {
            QImage image = blankImage();
            QPainter painter(&image);
            QVERIFY(!session.renderMap(painter, QSize(8, scmHeight)));
            QCOMPARE(session.errorString(), qsl("The map is drawn at 10 by 10 pixels or more, not 8 by 300."));
        }
        QCOMPARE(luaGlobalString(session.host(), "headlessMapInfoCalls"), qsl("2"));

        // Hidden, the map widget's view does not paint for itself, so only the app's draw follows the player
        QVERIFY(session.runLua(qsl("closeMapWidget()")));
        QVERIFY(session.runLua(qsl("centerview(6)")));
        QTRY_COMPARE_WITH_TIMEOUT(repaints.count(), 2, 5000);
        QTest::qWait(100);
        QCOMPARE(luaGlobalString(session.host(), "headlessAreaChanges"), qsl("%1:-2 ").arg(westId));
        const std::optional<QImage> east = rendered(session);
        QVERIFY2(east, qPrintable(session.errorString()));
        QVERIFY2(east->pixelColor(scmWidth / 2, scmHeight / 2) == eastColour(), qPrintable(saved(*east, qsl("east"))));
        QCOMPARE(luaGlobalString(session.host(), "headlessAreaChanges"), qsl("%1:-2 %2:%1 ").arg(westId, eastId));
        QCOMPARE(luaGlobalString(session.host(), "headlessMapInfoRoom"), qsl("6"));
        QVERIFY(rendered(session));
        QCOMPARE(luaGlobalString(session.host(), "headlessAreaChanges"), qsl("%1:-2 %2:%1 ").arg(westId, eastId));

        QVERIFY(session.shutdown(false));
        QVERIFY(!rendered(session));
        QCOMPARE(session.errorString(), qsl("No profile is open."));
    }

    // The GUI paints its map as the widget is shown, so an app hears it should draw when a script opens the map, shows
    // it again or replaces it with an embedded mapper, with nothing else changed
    void test_openingShowingOrReplacingTheMapAsksForADraw()
    {
        const QString name = qsl("Headless Map Shown");
        QVERIFY(writeProfile(name));
        THeadlessSession session;
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QVERIFY(session.runLua(qsl(R"lua(
local area = addAreaName("Headless Shown")
addRoom(1)
setRoomArea(1, area)
)lua")));
        QSignalSpy repaints(&session, &THeadlessSession::mapRepaintRequested);
        QTest::qWait(100);
        QCOMPARE(repaints.count(), 0);

        QVERIFY(session.runLua(qsl("openMapWidget()")));
        QTRY_COMPARE_WITH_TIMEOUT(repaints.count(), 1, 5000);
        QVERIFY(session.runLua(qsl("closeMapWidget()")));
        QTest::qWait(100);
        QCOMPARE(repaints.count(), 1);
        QVERIFY(session.runLua(qsl("openMapWidget()")));
        QTRY_COMPARE_WITH_TIMEOUT(repaints.count(), 2, 5000);
        QVERIFY(session.runLua(qsl("closeMapWidget()")));
        QTest::qWait(100);
        QCOMPARE(repaints.count(), 2);
        QVERIFY(session.runLua(qsl("createMapper(0, 0, 400, 300)")));
        QTRY_COMPARE_WITH_TIMEOUT(repaints.count(), 3, 5000);
        QVERIFY2(rendered(session), qPrintable(session.errorString()));
        QVERIFY(session.shutdown(false));
    }

    // A map info contributor may replace the hidden map widget with an embedded mapper, as it may in the GUI, while
    // the draw that called it still paints the widget's view, which goes once the event loop runs, as the GUI's dock
    void test_aContributorReplacingTheMapWidgetLeavesTheDrawItsView()
    {
        const QString name = qsl("Headless Map Replaced");
        QVERIFY(writeProfile(name));
        THeadlessSession session;
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QVERIFY(session.runLua(qsl(R"lua(
openMapWidget()
local area = addAreaName("Headless Replaced")
addRoom(1)
setRoomArea(1, area)
centerview(1)
closeMapWidget()
registerMapInfo("HeadlessReplace", function()
  if not headlessReplaced then
    headlessReplaced = tostring(createMapper(0, 0, 400, 300))
  end
  return "replacing"
end)
enableMapInfo("HeadlessReplace")
)lua")));
        QTest::qWait(50);
        const QPointer<QObject> widgetView = dynamic_cast<QObject*>(session.host()->mapViewFrontend());
        QVERIFY(widgetView);

        QVERIFY2(rendered(session), qPrintable(session.errorString()));
        QCOMPARE(luaGlobalString(session.host(), "headlessReplaced"), qsl("true"));
        QVERIFY(widgetView);
        QVERIFY(dynamic_cast<QObject*>(session.host()->mapViewFrontend()) != widgetView.data());
        QTRY_VERIFY_WITH_TIMEOUT(!widgetView, 5000);
        QVERIFY2(rendered(session), qPrintable(session.errorString()));
        QVERIFY(session.shutdown(false));
    }

    // Room symbols are kept as pixmaps made at the room's size in the symbol font, so a draw after either changes
    // makes them again as the mapper does, and so does one after an export, which makes them at a size of its own
    void test_symbolsAreRemadeAfterTheirSizeFontOrAnExportChanges()
    {
        const QString name = qsl("Headless Map Symbols");
        QVERIFY(writeProfile(name));
        THeadlessSession session;
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QVERIFY(session.runLua(qsl(R"lua(
openMapWidget()
headlessSymbolsArea = addAreaName("Headless Symbols")
setCustomEnvColor(301, 30, 30, 200, 255)
addRoom(1)
setRoomArea(1, headlessSymbolsArea)
setRoomEnv(1, 301)
setRoomChar(1, "W")
setMapZoom(10, headlessSymbolsArea)
centerview(1)
)lua")));
        TMap* pMap = session.host()->mpMap.data();
        pMap->mPlayerRoomStyle = 1;
        pMap->mPlayerRoomInnerDiameterPercentage = 70;
        QTest::qWait(50);
        // Another zoom is another room size, which makes the pixmaps again
        const auto remade = [&session]() -> std::optional<QImage> {
            if (!session.runLua(qsl("setMapZoom(11)")) || !rendered(session) || !session.runLua(qsl("setMapZoom(10)"))) {
                return std::nullopt;
            }
            return rendered(session);
        };

        const std::optional<QImage> before = rendered(session);
        QVERIFY(before);
        QVERIFY(session.runLua(qsl("setConfig('mapRoomSize', 8)")));
        const std::optional<QImage> resized = rendered(session);
        const std::optional<QImage> resizedRemade = remade();
        QVERIFY(resized && resizedRemade);
        QVERIFY2(*resized != *before, qPrintable(saved(*resized, qsl("resized"))));
        QVERIFY2(*resized == *resizedRemade, qPrintable(saved(*resized, qsl("resized")) + qsl("; ") + saved(*resizedRemade, qsl("resized-remade"))));

        QVERIFY(session.runLua(qsl("setConfig('mapSymbolFontScaling', 2.0)")));
        const std::optional<QImage> rescaled = rendered(session);
        const std::optional<QImage> rescaledRemade = remade();
        QVERIFY(rescaled && rescaledRemade);
        QVERIFY2(*rescaled != *resizedRemade, qPrintable(saved(*rescaled, qsl("rescaled"))));
        QVERIFY2(*rescaled == *rescaledRemade, qPrintable(saved(*rescaled, qsl("rescaled")) + qsl("; ") + saved(*rescaledRemade, qsl("rescaled-remade"))));

        const QString exportPath = qsl("%1/HeadlessSessionMapTest-symbols-export.png").arg(mConfigDir.path());
        QVERIFY(session.runLua(qsl("assert(exportAreaImage(headlessSymbolsArea, [[%1]]))").arg(exportPath)));
        const std::optional<QImage> afterExport = rendered(session);
        QVERIFY(afterExport);
        QVERIFY2(*afterExport == *rescaledRemade, qPrintable(saved(*afterExport, qsl("after-export"))));
        QTRY_VERIFY_WITH_TIMEOUT(QFile::exists(exportPath), 10000);
        QVERIFY(session.shutdown(false));
    }

    // A map info contributor may export an area while the app draws, as it may while the GUI paints, and the draw
    // finishes with its own view: the next draw hears of no area change again, and the area keeps its zoom
    void test_anExportFromAContributorLeavesTheDrawItsView()
    {
        const QString name = qsl("Headless Map Exported Mid-draw");
        QVERIFY(writeProfile(name));
        THeadlessSession session;
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        const QString exportPath = qsl("%1/HeadlessSessionMapTest-mid-draw-export.png").arg(mConfigDir.path());
        QVERIFY(session.runLua(qsl(R"lua(
openMapWidget()
headlessExportArea = addAreaName("Headless Exported Mid-draw")
setCustomEnvColor(301, 30, 30, 200, 255)
addRoom(1)
setRoomArea(1, headlessExportArea)
setRoomEnv(1, 301)
setMapZoom(10, headlessExportArea)
centerview(1)
headlessAreaChanges = ""
registerAnonymousEventHandler("sysMapAreaChanged", function(_, areaId, lastAreaId)
  headlessAreaChanges = headlessAreaChanges .. areaId .. ":" .. lastAreaId .. " "
end)
registerMapInfo("HeadlessExport", function()
  if headlessExportPath and not headlessExported then
    headlessExported = tostring(exportAreaImage(headlessExportArea, headlessExportPath))
  end
  return "exporting"
end)
enableMapInfo("HeadlessExport")
)lua")));
        QTest::qWait(50);
        const QString areaId = luaGlobalString(session.host(), "headlessExportArea");
        const std::optional<QImage> before = rendered(session);
        QVERIFY2(before, qPrintable(session.errorString()));
        QCOMPARE(luaGlobalString(session.host(), "headlessAreaChanges"), qsl("%1:-2 ").arg(areaId));

        QVERIFY(session.runLua(qsl("headlessExportPath = [[%1]]").arg(exportPath)));
        QVERIFY(rendered(session));
        QCOMPARE(luaGlobalString(session.host(), "headlessExported"), qsl("true"));
        const std::optional<QImage> after = rendered(session);
        QVERIFY(after);

        QCOMPARE(luaGlobalString(session.host(), "headlessAreaChanges"), qsl("%1:-2 ").arg(areaId));
        QVERIFY(session.runLua(qsl("assert(getMapZoom(headlessExportArea) == 10)")));
        // Below the info box, whose render time a debug build prints
        const QRect belowInfoBox(0, scmHeight / 3, scmWidth, scmHeight - scmHeight / 3);
        QVERIFY2(after->copy(belowInfoBox) == before->copy(belowInfoBox), qPrintable(saved(*before, qsl("before-mid-draw-export")) + qsl("; ") + saved(*after, qsl("after-mid-draw-export"))));
        QTRY_VERIFY_WITH_TIMEOUT(QFile::exists(exportPath), 10000);
        QVERIFY(session.shutdown(false));
    }

    // A repaint cue still queued as one run ends must not hold back the next run's first
    void test_aRepaintQueuedBeforeARestartDoesNotHoldBackTheNextRunsFirst()
    {
        const QString name = qsl("Headless Map Restarted");
        QVERIFY(writeProfile(name));
        THeadlessSession session;
        QSignalSpy repaints(&session, &THeadlessSession::mapRepaintRequested);
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QVERIFY(session.runLua(qsl("openMapWidget()")));
        QVERIFY(session.shutdown(false));
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        // Whether the first run's cue went out depends on whether shutdown() turned the event loop before the run ended
        const qsizetype firstRunRepaints = repaints.count();
        QVERIFY(session.runLua(qsl("openMapWidget()")));
        // Posted behind the cue, as a later paint of the shown map would send another
        std::optional<qsizetype> repaintsByNextTurn;
        QMetaObject::invokeMethod(
                this,
                [&repaints, &repaintsByNextTurn]() {
                    repaintsByNextTurn = repaints.count();
                },
                Qt::QueuedConnection);
        QTRY_VERIFY_WITH_TIMEOUT(repaintsByNextTurn.has_value(), 5000);
        QCOMPARE(*repaintsByNextTurn, firstRunRepaints + 1);
        QVERIFY(session.shutdown(false));
    }

    // With mapCenterSmallAreas on, T2DMap centres an area that fits on the area rather than the player, which it
    // works out only once it knows its size, so a draw redoes the centring the shown mapper did before
    void test_aSmallAreaIsCentredAsTheMapperCentresIt()
    {
        const QString name = qsl("Headless Map Small Area");
        QVERIFY(writeProfile(name));
        THeadlessSession session;
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        session.host()->mMapperCenterSmallAreas = true;
        QVERIFY(session.runLua(qsl(R"lua(
openMapWidget()
local area = addAreaName("Headless Small")
setCustomEnvColor(301, 200, 30, 30, 255)
setCustomEnvColor(302, 30, 30, 200, 255)
addRoom(1)
setRoomArea(1, area)
setRoomCoordinates(1, 0, 0, 0)
setRoomEnv(1, 301)
addRoom(2)
setRoomArea(2, area)
setRoomCoordinates(2, 4, 0, 0)
setRoomEnv(2, 302)
setMapZoom(10, area)
centerview(1)
)lua")));
        TMap* pMap = session.host()->mpMap.data();
        pMap->mPlayerRoomStyle = 1;
        pMap->mPlayerRoomInnerDiameterPercentage = 70;
        QTest::qWait(50);

        // At a zoom of 10 the 300 pixels high view is 10 rooms high, so the rooms are 60 pixels either side of the middle
        const std::optional<QImage> image = rendered(session);
        QVERIFY2(image, qPrintable(session.errorString()));
        QVERIFY2(image->pixelColor(scmWidth / 2 - 60, scmHeight / 2) == westColour(), qPrintable(saved(*image, qsl("small-west"))));
        QVERIFY2(image->pixelColor(scmWidth / 2 + 60, scmHeight / 2) == eastColour(), qPrintable(saved(*image, qsl("small-east"))));
        QVERIFY(session.shutdown(false));
    }

    // The map info box runs the profile's Lua, which cannot run in the middle of a line being written; a
    // repaint request comes from the event loop, so its handler may draw at once
    void test_drawingTheMapIsRefusedFromAnOutputLineHandler()
    {
        const QString name = qsl("Headless Map Mid Write");
        QVERIFY(writeProfile(name));
        THeadlessSession session;
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QVERIFY(session.runLua(qsl(R"lua(
openMapWidget()
local area = addAreaName("Headless Mid Write")
addRoom(1)
setRoomArea(1, area)
centerview(1)
)lua")));

        std::optional<bool> drawnMidWrite;
        QString errorMidWrite;
        connect(&session, &THeadlessSession::outputLine, this, [&session, &drawnMidWrite, &errorMidWrite](const QString&, const QString& text) {
            if (text == qsl("headless map line")) {
                drawnMidWrite = rendered(session).has_value();
                errorMidWrite = session.errorString();
            }
        });
        QTest::ignoreMessage(QtWarningMsg, QRegularExpression(qsl("refused")));
        QVERIFY(session.runLua(qsl(R"lua(feedTriggers("headless map line\n"))lua")));
        QCOMPARE(drawnMidWrite, std::optional(false));
        QVERIFY2(errorMidWrite.startsWith(qsl("Refused from a handler of outputLine")), qPrintable(errorMidWrite));
        QVERIFY2(rendered(session), qPrintable(session.errorString()));

        std::optional<bool> drawnOnRequest;
        connect(&session, &THeadlessSession::mapRepaintRequested, this, [&session, &drawnOnRequest](const QString&) {
            drawnOnRequest = rendered(session).has_value();
        });
        QVERIFY(session.runLua(qsl("setMapZoom(5)")));
        QTRY_VERIFY_WITH_TIMEOUT(drawnOnRequest.has_value(), 5000);
        QVERIFY2(*drawnOnRequest, qPrintable(session.errorString()));
        QVERIFY(session.shutdown(false));
    }
};

#include "HeadlessSessionMapTest.moc"
MUDLET_GROUPED_TEST_MAIN(HeadlessSessionMapTest)
