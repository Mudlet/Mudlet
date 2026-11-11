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

#include <cmath>
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

    // Rooms 7 and 8 a room apart, 30 pixels to a room at a zoom of 10 in 300 pixels, so room 8 is 60 pixels right of
    // room 7 in the middle
    void test_anAppClicksDoubleClicksAndZoomsTheMap()
    {
        using Pointer = THeadlessSession::MapPointerEvent;
        const QString name = qsl("Headless Map Pointer");
        QVERIFY(writeProfile(name));
        THeadlessSession session;
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        const Pointer onRoom8{.position = QPointF(scmWidth / 2 + 60, scmHeight / 2)};
        QVERIFY(!session.mapPointer(onRoom8));
        QCOMPARE(session.errorString(), qsl("No map is open; a script opens one, as with openMapWidget()."));

        QVERIFY(session.runLua(qsl(R"lua(
openMapWidget()
headlessArea = addAreaName("Headless Pointer")
for id, x in pairs({[7] = 0, [8] = 2}) do
  addRoom(id)
  setRoomArea(id, headlessArea)
  setRoomCoordinates(id, x, 0, 0)
end
setExit(7, 8, "e")
setExit(8, 7, "w")
setMapZoom(10, headlessArea)
centerview(7)
headlessPresses = 0
registerAnonymousEventHandler("sysMapWindowMousePressEvent", function() headlessPresses = headlessPresses + 1 end)
function doSpeedWalk() headlessWalk = table.concat(speedWalkDir, ",") .. " to " .. table.concat(speedWalkPath, ",") end
function headlessSelection()
  local selection = getMapSelection()
  local rooms = selection.rooms or {}
  table.sort(rooms)
  return table.concat(rooms, ",") .. " centre " .. tostring(selection.center)
end
)lua")));
        QTest::qWait(50);
        QVERIFY(!session.mapPointer(onRoom8));
        QCOMPARE(session.errorString(), qsl("The map has not been drawn yet; the position is in what renderMap() drew."));
        QVERIFY(rendered(session));

        QSignalSpy repaints(&session, &THeadlessSession::mapRepaintRequested);
        QVERIFY(session.mapPointer(onRoom8));
        QCOMPARE(luaGlobalString(session.host(), "headlessPresses"), qsl("1"));
        QVERIFY(session.runLua(qsl("headlessSelected = headlessSelection()")));
        QCOMPARE(luaGlobalString(session.host(), "headlessSelected"), qsl("8 centre 8"));
        QTRY_COMPARE_WITH_TIMEOUT(repaints.count(), 1, 5000);
        QVERIFY(session.mapPointer({.position = QPointF(scmWidth / 2, scmHeight / 2), .modifiers = Qt::ControlModifier}));
        QVERIFY(session.runLua(qsl("headlessSelected = headlessSelection()")));
        // Either is nearest the middle of two, so which is the centre is left to the order of a QSet
        QVERIFY(luaGlobalString(session.host(), "headlessSelected").startsWith(qsl("7,8 centre ")));
        // A press away from the rooms clears the selection, and a right press only raises the event, its menu being
        // the GUI's
        QVERIFY(session.mapPointer({.position = QPointF(10, 10)}));
        QVERIFY(session.runLua(qsl("headlessSelected = headlessSelection()")));
        QCOMPARE(luaGlobalString(session.host(), "headlessSelected"), qsl(" centre nil"));
        QVERIFY(session.mapPointer({.position = onRoom8.position, .button = Qt::RightButton}));
        QVERIFY(session.runLua(qsl("headlessSelected = headlessSelection()")));
        QCOMPARE(luaGlobalString(session.host(), "headlessSelected"), qsl(" centre nil"));
        QCOMPARE(luaGlobalString(session.host(), "headlessPresses"), qsl("4"));

        QVERIFY(!session.mapPointer({.type = Pointer::Type::doubleClick, .position = onRoom8.position, .button = Qt::RightButton}));
        QCOMPARE(session.errorString(), qsl("Only a double-click of the left button acts on the map."));
        QCOMPARE(luaGlobalString(session.host(), "headlessWalk"), QString());
        QVERIFY(session.mapPointer({.type = Pointer::Type::doubleClick, .position = onRoom8.position}));
        QCOMPARE(luaGlobalString(session.host(), "headlessWalk"), qsl("e to 8"));

        // As the GUI's with invertMapZoom off, five notches turned towards the player zoom out about the pointer
        QVERIFY(!session.mapPointer({.type = Pointer::Type::wheel, .position = onRoom8.position, .angleDelta = QPoint(0, 59)}));
        QCOMPARE(session.errorString(), qsl("Less than half a notch of the wheel does not zoom the map."));
        QVERIFY(session.mapPointer({.type = Pointer::Type::wheel, .position = onRoom8.position, .angleDelta = QPoint(0, -600)}));
        QVERIFY(session.runLua(qsl("headlessZoom = getMapZoom()")));
        QVERIFY2(qFuzzyCompare(luaGlobalString(session.host(), "headlessZoom").toDouble(), 10.0 * std::pow(1.07, 5)), qPrintable(luaGlobalString(session.host(), "headlessZoom")));
        // Room 8 stays under the pointer
        const std::optional<QImage> zoomed = rendered(session);
        QVERIFY(zoomed);
        QVERIFY2(zoomed->pixelColor(onRoom8.position.toPoint()) != Qt::black, qPrintable(saved(*zoomed, qsl("zoomed"))));
        QVERIFY2(zoomed->pixelColor(onRoom8.position.toPoint() + QPoint(0, 12)) == Qt::black, qPrintable(saved(*zoomed, qsl("zoomed"))));

        QVERIFY(session.shutdown(false));
        QVERIFY(!session.mapPointer(onRoom8));
        QCOMPARE(session.errorString(), qsl("No profile is open."));
    }

    // Rooms 7 and 8 in one area and room 9 in another, with the map widget open: a selection loses a room deleted
    // from the map, and the rooms left behind when the shown map follows the player into another area, as the GUI's
    void test_aSelectionLosesDeletedRoomsAndThoseLeftBehind()
    {
        const QString name = qsl("Headless Map Selection");
        QVERIFY(writeProfile(name));
        THeadlessSession session;
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QVERIFY(session.runLua(qsl(R"lua(
openMapWidget()
local area = addAreaName("Headless Selected")
for id, x in pairs({[7] = 0, [8] = 2}) do
  addRoom(id)
  setRoomArea(id, area)
  setRoomCoordinates(id, x, 0, 0)
end
addRoom(9)
setRoomArea(9, addAreaName("Headless Elsewhere"))
setMapZoom(10, area)
centerview(7)
function headlessSelection()
  local selection = getMapSelection()
  local rooms = selection.rooms or {}
  table.sort(rooms)
  return table.concat(rooms, ",") .. " centre " .. tostring(selection.center)
end
)lua")));
        QTest::qWait(50);
        QVERIFY(rendered(session));
        const auto selection = [&session]() {
            return session.runLua(qsl("headlessSelected = headlessSelection()")) ? luaGlobalString(session.host(), "headlessSelected") : qsl("(failed)");
        };

        QVERIFY(session.mapPointer({.position = QPointF(scmWidth / 2 + 60, scmHeight / 2)}));
        QCOMPARE(selection(), qsl("8 centre 8"));
        QVERIFY(session.runLua(qsl("deleteRoom(8)")));
        QCOMPARE(selection(), qsl(" centre nil"));

        QVERIFY(session.mapPointer({.position = QPointF(scmWidth / 2, scmHeight / 2)}));
        QCOMPARE(selection(), qsl("7 centre 7"));
        QVERIFY(session.runLua(qsl("centerview(9)")));
        QTest::qWait(100);
        QCOMPARE(selection(), qsl(" centre nil"));
        QVERIFY(session.shutdown(false));
    }

    // Rooms 7 and 8 in one area and rooms 9 and 10 in another, each pair a room apart, with the map widget closed so
    // that only the app's draws move the view: a double-click acts on the frame the app drew, and no later one, nor
    // one that has followed the player since
    void test_aDoubleClickActsOnlyOnTheFrameTheAppDrew()
    {
        const QString name = qsl("Headless Map Double Click");
        QVERIFY(writeProfile(name));
        THeadlessSession session;
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QVERIFY(session.runLua(qsl(R"lua(
openMapWidget()
local here = addAreaName("Headless Here")
local there = addAreaName("Headless There")
for id, room in pairs({[7] = {here, 0}, [8] = {here, 2}, [9] = {there, 0}, [10] = {there, 2}}) do
  addRoom(id)
  setRoomArea(id, room[1])
  setRoomCoordinates(id, room[2], 0, 0)
end
setExit(7, 8, "e")
setExit(8, 7, "w")
setExit(9, 10, "e")
setMapZoom(10, here)
setMapZoom(10, there)
centerview(7)
closeMapWidget()
function doSpeedWalk() headlessWalk = table.concat(speedWalkDir, ",") .. " to " .. table.concat(speedWalkPath, ",") end
)lua")));
        QTest::qWait(50);
        QVERIFY(rendered(session));

        // Nothing is a room west of room 7, until the player moves to room 8 and the view with them
        QVERIFY(session.mapPointer({.type = THeadlessSession::MapPointerEvent::Type::doubleClick, .position = QPointF(scmWidth / 2 - 60, scmHeight / 2)}));
        QCOMPARE(luaGlobalString(session.host(), "headlessWalk"), QString());
        QVERIFY(session.runLua(qsl("centerview(8)")));
        QVERIFY(rendered(session));
        QCOMPARE(luaGlobalString(session.host(), "headlessWalk"), QString());

        // Room 10 is east of room 9, but the app has not drawn the move there
        QVERIFY(session.runLua(qsl("centerview(9)")));
        QVERIFY(session.mapPointer({.type = THeadlessSession::MapPointerEvent::Type::doubleClick, .position = QPointF(scmWidth / 2 + 60, scmHeight / 2)}));
        QCOMPARE(luaGlobalString(session.host(), "headlessWalk"), QString());
        QVERIFY(rendered(session));
        QVERIFY(session.mapPointer({.type = THeadlessSession::MapPointerEvent::Type::doubleClick, .position = QPointF(scmWidth / 2 + 60, scmHeight / 2)}));
        QCOMPARE(luaGlobalString(session.host(), "headlessWalk"), qsl("e to 10"));
        QVERIFY(session.shutdown(false));
    }

    // Rooms 7 and 8 a room apart in one area at a zoom of 10 and room 9 in another at 20, with the map widget open, so
    // the view follows the player as the event loop runs: a press, a double-click and a wheel turn still act on what
    // the app drew, until it draws the move
    void test_thePointerActsOnWhatTheAppDrewBeforeItDrawsAMove()
    {
        using Pointer = THeadlessSession::MapPointerEvent;
        const QString name = qsl("Headless Map Undrawn Move");
        QVERIFY(writeProfile(name));
        THeadlessSession session;
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QVERIFY(session.runLua(qsl(R"lua(
openMapWidget()
headlessHere = addAreaName("Headless Drawn")
headlessThere = addAreaName("Headless Not Drawn")
for id, room in pairs({[7] = {headlessHere, 0}, [8] = {headlessHere, 2}, [9] = {headlessThere, 0}}) do
  addRoom(id)
  setRoomArea(id, room[1])
  setRoomCoordinates(id, room[2], 0, 0)
end
setExit(8, 7, "w")
setMapZoom(10, headlessHere)
setMapZoom(20, headlessThere)
centerview(7)
function doSpeedWalk() headlessWalk = table.concat(speedWalkDir, ",") .. " to " .. table.concat(speedWalkPath, ",") end
function headlessSelection()
  local selection = getMapSelection()
  local rooms = selection.rooms or {}
  table.sort(rooms)
  return table.concat(rooms, ",") .. " centre " .. tostring(selection.center)
end
)lua")));
        QTest::qWait(50);
        QVERIFY(rendered(session));
        const auto selection = [&session]() {
            return session.runLua(qsl("headlessSelected = headlessSelection()")) ? luaGlobalString(session.host(), "headlessSelected") : qsl("(failed)");
        };
        const QPointF onRoom7(scmWidth / 2, scmHeight / 2);
        const QPointF onRoom8(scmWidth / 2 + 60, scmHeight / 2);

        QVERIFY(session.runLua(qsl("centerview(8)")));
        QTest::qWait(50);
        QVERIFY(session.mapPointer({.position = onRoom8}));
        QCOMPARE(selection(), qsl("8 centre 8"));
        QVERIFY(session.mapPointer({.position = onRoom7}));
        QCOMPARE(selection(), qsl("7 centre 7"));
        QVERIFY(session.mapPointer({.type = Pointer::Type::doubleClick, .position = onRoom7}));
        QCOMPARE(luaGlobalString(session.host(), "headlessWalk"), qsl("w to 7"));

        QVERIFY(session.runLua(qsl("centerview(9)")));
        QTest::qWait(50);
        QVERIFY(session.mapPointer({.type = Pointer::Type::wheel, .position = onRoom7, .angleDelta = QPoint(0, -600)}));
        QVERIFY(session.runLua(qsl("headlessZoomHere, headlessZoomThere = getMapZoom(headlessHere), getMapZoom(headlessThere)")));
        QVERIFY2(qFuzzyCompare(luaGlobalString(session.host(), "headlessZoomHere").toDouble(), 10.0 * std::pow(1.07, 5)), qPrintable(luaGlobalString(session.host(), "headlessZoomHere")));
        QCOMPARE(luaGlobalString(session.host(), "headlessZoomThere"), qsl("20"));

        // Drawn, the move is what the pointer acts on
        QVERIFY(rendered(session));
        QVERIFY(session.mapPointer({.position = onRoom7}));
        QCOMPARE(selection(), qsl("9 centre 9"));
        QVERIFY(session.shutdown(false));
    }

    // Rooms 7, 8 and 11 two map units apart in a row at a zoom of 10, so 60 pixels apart: a zoom from a script or the
    // wheel that the app has not drawn yet leaves a double-click and a press to hit the room drawn under them
    void test_thePointerActsOnWhatTheAppDrewBeforeItDrawsAZoom()
    {
        using Pointer = THeadlessSession::MapPointerEvent;
        const QString name = qsl("Headless Map Undrawn Zoom");
        QVERIFY(writeProfile(name));
        THeadlessSession session;
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QVERIFY(session.runLua(qsl(R"lua(
openMapWidget()
local area = addAreaName("Headless Zoomed")
for id, x in pairs({[7] = 0, [8] = 2, [11] = 4}) do
  addRoom(id)
  setRoomArea(id, area)
  setRoomCoordinates(id, x, 0, 0)
end
setExit(7, 8, "e")
setExit(8, 11, "e")
setMapZoom(10, area)
centerview(7)
function doSpeedWalk() headlessWalk = table.concat(speedWalkDir, ",") .. " to " .. table.concat(speedWalkPath, ",") end
function headlessSelection()
  local selection = getMapSelection()
  local rooms = selection.rooms or {}
  table.sort(rooms)
  return table.concat(rooms, ",") .. " centre " .. tostring(selection.center)
end
)lua")));
        QTest::qWait(50);
        QVERIFY(rendered(session));
        const QPointF onRoom8(scmWidth / 2 + 60, scmHeight / 2);

        // At a zoom of 20 room 11 would be where room 8 was drawn
        QVERIFY(session.runLua(qsl("setMapZoom(20)")));
        QVERIFY(session.mapPointer({.type = Pointer::Type::doubleClick, .position = onRoom8}));
        QCOMPARE(luaGlobalString(session.host(), "headlessWalk"), qsl("e to 8"));
        QVERIFY(session.runLua(qsl("assert(getMapZoom() == 20)")));

        // A wheel turn about room 8 moves the view's centre and changes the zoom, neither of which is drawn yet
        const auto selection = [&session]() {
            return session.runLua(qsl("headlessSelected = headlessSelection()")) ? luaGlobalString(session.host(), "headlessSelected") : qsl("(failed)");
        };
        const QPointF onRoom7(scmWidth / 2, scmHeight / 2);
        QVERIFY(session.runLua(qsl("setMapZoom(10)")));
        QVERIFY(rendered(session));
        QVERIFY(session.mapPointer({.type = Pointer::Type::wheel, .position = onRoom8, .angleDelta = QPoint(0, -600)}));
        QVERIFY(session.mapPointer({.position = onRoom7}));
        QCOMPARE(selection(), qsl("7 centre 7"));
        QVERIFY(session.mapPointer({.position = onRoom8}));
        QCOMPARE(selection(), qsl("8 centre 8"));

        // Drawn, room 8 stays under the pointer the wheel turned about, with room 7 drawn closer to it
        QVERIFY(rendered(session));
        QVERIFY(session.runLua(qsl("headlessZoom = getMapZoom()")));
        const qreal zoom = luaGlobalString(session.host(), "headlessZoom").toDouble();
        QVERIFY2(qFuzzyCompare(zoom, 10.0 * std::pow(1.07, 5)), qPrintable(QString::number(zoom)));
        QVERIFY(session.mapPointer({.position = onRoom8 - QPointF(60 * 10 / zoom, 0)}));
        QCOMPARE(selection(), qsl("7 centre 7"));
        QVERIFY(session.mapPointer({.position = onRoom8}));
        QCOMPARE(selection(), qsl("8 centre 8"));
        QVERIFY(session.shutdown(false));
    }

    // A double-click paints the drawn area at its own zoom to find the room under it, which makes the symbol pixmaps
    // at that size; a draw of another area at the size last drawn must make them again
    void test_aDoubleClickLeavesTheSymbolsToBeRemadeAtTheSizeDrawnNext()
    {
        using Pointer = THeadlessSession::MapPointerEvent;
        const QString name = qsl("Headless Map Double Click Symbols");
        QVERIFY(writeProfile(name));
        THeadlessSession session;
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QVERIFY(session.runLua(qsl(R"lua(
openMapWidget()
headlessHere = addAreaName("Headless Drawn")
headlessThere = addAreaName("Headless Moved To")
for id, room in pairs({[7] = {headlessHere, 0}, [8] = {headlessHere, 2}, [9] = {headlessThere, 0}, [10] = {headlessThere, 2}}) do
  addRoom(id)
  setRoomArea(id, room[1])
  setRoomCoordinates(id, room[2], 0, 0)
  setRoomChar(id, "W")
end
setMapZoom(10, headlessHere)
setMapZoom(10, headlessThere)
centerview(7)
)lua")));
        QTest::qWait(50);
        QVERIFY(rendered(session));

        QVERIFY(session.runLua(qsl("centerview(9)")));
        QTest::qWait(50);
        QVERIFY(session.mapPointer({.type = Pointer::Type::wheel, .position = QPointF(scmWidth / 2, scmHeight / 2), .angleDelta = QPoint(0, -600)}));
        QVERIFY(session.mapPointer({.type = Pointer::Type::doubleClick, .position = QPointF(5, 5)}));
        const std::optional<QImage> drawn = rendered(session);
        QVERIFY(drawn);

        QVERIFY(session.runLua(qsl("setConfig('mapRoomSize', getConfig('mapRoomSize'))")));
        const std::optional<QImage> remade = rendered(session);
        QVERIFY(remade);
        QVERIFY2(*drawn == *remade, qPrintable(saved(*drawn, qsl("double-click-symbols")) + qsl("; ") + saved(*remade, qsl("double-click-symbols-remade"))));
        QVERIFY(session.shutdown(false));
    }

    // A map with no rooms is drawn without a room to take a double-click, which is spent all the same rather than
    // left for the first frame that has rooms
    void test_aDoubleClickOnAMapWithNoRoomsIsSpent()
    {
        const QString name = qsl("Headless Map Empty Double Click");
        QVERIFY(writeProfile(name));
        THeadlessSession session;
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QVERIFY(session.runLua(qsl(R"lua(
openMapWidget()
function doSpeedWalk() headlessWalk = table.concat(speedWalkDir, ",") .. " to " .. table.concat(speedWalkPath, ",") end
)lua")));
        QTest::qWait(50);
        QVERIFY(rendered(session));
        QVERIFY(session.mapPointer({.type = THeadlessSession::MapPointerEvent::Type::doubleClick, .position = QPointF(scmWidth / 2 + 60, scmHeight / 2)}));

        QVERIFY(session.runLua(qsl(R"lua(
local area = addAreaName("Headless Was Empty")
for id, x in pairs({[1] = 0, [2] = 2}) do
  addRoom(id)
  setRoomArea(id, area)
  setRoomCoordinates(id, x, 0, 0)
end
setExit(1, 2, "e")
setMapZoom(10, area)
centerview(1)
)lua")));
        QTest::qWait(50);
        QVERIFY(rendered(session));
        QCOMPARE(luaGlobalString(session.host(), "headlessWalk"), QString());
        QVERIFY(session.shutdown(false));
    }

    // A map with no rooms draws why it shows none, which a press may land on but a wheel turn has nothing to zoom
    void test_aWheelTurnOnAMapShowingNoAreaSaysSo()
    {
        const QString name = qsl("Headless Map No Area");
        QVERIFY(writeProfile(name));
        THeadlessSession session;
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QVERIFY(session.runLua(qsl(R"lua(
openMapWidget()
headlessPresses = 0
registerAnonymousEventHandler("sysMapWindowMousePressEvent", function() headlessPresses = headlessPresses + 1 end)
)lua")));
        QTest::qWait(50);
        const THeadlessSession::MapPointerEvent notch{.type = THeadlessSession::MapPointerEvent::Type::wheel, .position = QPointF(scmWidth / 2, scmHeight / 2), .angleDelta = QPoint(0, 120)};
        QVERIFY2(rendered(session), qPrintable(session.errorString()));
        QVERIFY(session.mapPointer({.position = QPointF(scmWidth / 2, scmHeight / 2)}));
        QCOMPARE(luaGlobalString(session.host(), "headlessPresses"), qsl("1"));
        QVERIFY(!session.mapPointer(notch));
        QCOMPARE(session.errorString(), qsl("The map drawn last shows no area to zoom."));

        // Its last room gone, an area is still there but no longer drawn
        QVERIFY(session.runLua(qsl(R"lua(
local area = addAreaName("Headless Emptied")
addRoom(1)
setRoomArea(1, area)
centerview(1)
)lua")));
        QTest::qWait(50);
        QVERIFY(rendered(session));
        QVERIFY2(session.mapPointer(notch), qPrintable(session.errorString()));
        QVERIFY(session.runLua(qsl("deleteRoom(1)")));
        QVERIFY(rendered(session));
        QVERIFY(!session.mapPointer(notch));
        QCOMPARE(session.errorString(), qsl("The map drawn last shows no area to zoom."));
        QVERIFY(session.shutdown(false));
    }

    // A press handler may replace the hidden map widget with an embedded mapper while the press still has its view
    void test_aPressHandlerReplacingTheMapWidgetLeavesThePressItsView()
    {
        const QString name = qsl("Headless Map Press Replaced");
        QVERIFY(writeProfile(name));
        THeadlessSession session;
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QVERIFY(session.runLua(qsl(R"lua(
openMapWidget()
local area = addAreaName("Headless Press Replaced")
addRoom(1)
setRoomArea(1, area)
centerview(1)
closeMapWidget()
registerAnonymousEventHandler("sysMapWindowMousePressEvent", function()
  headlessReplaced = headlessReplaced or tostring(createMapper(0, 0, 400, 300))
end)
)lua")));
        QTest::qWait(50);
        QVERIFY(rendered(session));
        const QPointer<QObject> widgetView = dynamic_cast<QObject*>(session.host()->mapViewFrontend());
        QVERIFY(widgetView);

        QVERIFY(session.mapPointer({.position = QPointF(scmWidth / 2, scmHeight / 2)}));
        QCOMPARE(luaGlobalString(session.host(), "headlessReplaced"), qsl("true"));
        QVERIFY(widgetView);
        QTRY_VERIFY_WITH_TIMEOUT(!widgetView, 5000);
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
        std::optional<bool> pointedMidWrite;
        QString errorMidWrite;
        connect(&session, &THeadlessSession::outputLine, this, [&session, &drawnMidWrite, &errorMidWrite, &pointedMidWrite](const QString&, const QString& text) {
            if (text == qsl("headless map line")) {
                drawnMidWrite = rendered(session).has_value();
                errorMidWrite = session.errorString();
                pointedMidWrite = session.mapPointer({.position = QPointF(scmWidth / 2, scmHeight / 2)});
            }
        });
        QTest::ignoreMessage(QtWarningMsg, QRegularExpression(qsl("refused")));
        QTest::ignoreMessage(QtWarningMsg, QRegularExpression(qsl("refused")));
        QVERIFY(session.runLua(qsl(R"lua(feedTriggers("headless map line\n"))lua")));
        QCOMPARE(drawnMidWrite, std::optional(false));
        QCOMPARE(pointedMidWrite, std::optional(false));
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
