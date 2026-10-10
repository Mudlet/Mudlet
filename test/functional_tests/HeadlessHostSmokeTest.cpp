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

#include <QApplication>
#include <QDataStream>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFontInfo>
#include <QFontMetrics>
#include <QFontMetricsF>
#include <QImage>
#include <QSvgRenderer>
#include <QTemporaryDir>
#include <QtNetwork/QTcpServer>
#include <QtNetwork/QTcpSocket>
// After a QtNetwork header, which is what defines QT_NO_SSL
#if !defined(QT_NO_SSL)
#include <QtNetwork/QSslKey>
#include <QtNetwork/QSslServer>
#include <QtNetwork/QSslSocket>
#endif
#include <QtTest/QtTest>

#include <memory>

#include "FontManager.h"
#include "Host.h"
#include "HostManager.h"
#include "MudletApp.h"
#include "PortableModeTestHelper.h"
#include "TAppFrontend.h"
#include "TConsoleModel.h"
#include "TLabelModel.h"
#include "TLuaInterpreter.h"
#include "TMap.h"
#include "TRoomDB.h"

#include "GroupedTest.h"

// A profile made without ever starting the main window, so it only has the null
// console view: its Lua, triggers and main console model have to work regardless.
class HeadlessHostSmokeTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    // The main window owns the profile pool in the app; here nothing else would.
    std::unique_ptr<HostManager> mpHostManager;
    const QString mHostname = qsl("Test-Headless-Host-Smoke");
    const QString mConnectingHostname = qsl("Test-Headless-Host-Connect");
    const QString mSecureHostname = qsl("Test-Headless-Host-Secure");
    const QString mHeldLineHostname = qsl("Test-Headless-Host-Held-Line");
    const QString mWindowsHostname = qsl("Test-Headless-Host-Windows");

    static QString luaGlobalString(Host* host, const char* name)
    {
        lua_State* L = host->getLuaInterpreter()->getLuaGlobalState();
        lua_getglobal(L, name);
        const QString value = lua_isstring(L, -1) ? QString::fromUtf8(lua_tostring(L, -1)) : QString();
        lua_pop(L, 1);
        return value;
    }

    static bool mainBufferHolds(Host* host, const QString& text) { return host->mainConsoleModel().buffer.lineBuffer.join(QChar::LineFeed).contains(text); }

    // The shape a game serves for an MMP map download
    static QByteArray xmlMap()
    {
        return QByteArrayLiteral(R"(<?xml version="1.0" encoding="UTF-8"?>
<map>
 <areas>
  <area id="1" name="Headless Xml Area"/>
 </areas>
 <rooms>
  <room id="3" area="1" title="Headless Xml Room" environment="2">
   <coord x="0" y="0" z="0"/>
  </room>
 </rooms>
</map>
)");
    }

    static QString veraFontFolder() { return qsl("%1/ttf-bitstream-vera-1.10").arg(MudletApp::getMudletPath(enums::mainFontsPath)); }

    // A second of silence, for media that only has to be played and stopped
    static QByteArray silentWav()
    {
        constexpr quint32 sampleRate = 8000;
        constexpr quint32 dataBytes = sampleRate * 2;
        QByteArray wav;
        QDataStream out(&wav, QIODevice::WriteOnly);
        out.setVersion(QDataStream::Qt_5_12);
        out.setByteOrder(QDataStream::LittleEndian);
        out.writeRawData("RIFF", 4);
        out << static_cast<quint32>(36 + dataBytes);
        out.writeRawData("WAVEfmt ", 8);
        out << static_cast<quint32>(16) << static_cast<quint16>(1) << static_cast<quint16>(1) << sampleRate << sampleRate * 2 << static_cast<quint16>(2) << static_cast<quint16>(16);
        out.writeRawData("data", 4);
        out << dataBytes;
        wav.append(QByteArray(dataBytes, '\0'));
        return wav;
    }

    // A binary map of the profile's own, holding just room roomId; the profile's map is left empty
    static QByteArray binaryMapOf(Host* host, const int roomId)
    {
        TMap* map = host->mpMap.data();
        map->mapClear();
        const int areaId = map->mpRoomDB->addArea(qsl("Headless Binary Area"));
        map->addRoom(roomId);
        map->setRoomArea(roomId, areaId);
        QByteArray serialized;
        QDataStream out(&serialized, QIODevice::WriteOnly);
        out.setVersion(QDataStream::Qt_5_12);
        map->serialize(out);
        map->mapClear();
        return serialized;
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
#ifdef INCLUDE_FONTS
        // Staged before anything can ask for a font: one bundled font copied out, as a run of the executable
        // can leave a bundle, so the rest of that bundle has to come from the resources
        QVERIFY(QDir().mkpath(veraFontFolder()));
        QVERIFY(QFile::copy(qsl(":/fonts/ttf-bitstream-vera-1.10/Vera.ttf"), qsl("%1/Vera.ttf").arg(veraFontFolder())));
#endif

        QVERIFY2(!HostManager::self(), "A profile pool already exists, so this run is not headless.");
        mpHostManager = std::make_unique<HostManager>();
    }

    void cleanupTestCase()
    {
        mpHostManager.reset();
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
    }

    void test_profileRunsLuaAndTriggersWithNoMainWindow()
    {
        QVERIFY2(!TAppFrontend::hasView(), "A main window exists, so this run is not headless.");

        QVERIFY2(HostManager::self()->addHost(mHostname, QString(), QString(), QString()), "Could not create a profile with no main window.");
        Host* host = HostManager::self()->getHost(mHostname);
        QVERIFY2(host, "The profile is not in the pool.");
        QVERIFY(!host->hasConsoleView());
        QVERIFY2(host->consoleFrontend(), "consoleFrontend() must never be null.");

        // pcall keeps the failing assert's message, which compileAndExecuteScript() only logs
        const bool ran = host->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(
headlessResult = 'not run'
local ok, err = pcall(function()
  assert(echo("main", "headless echo line\n") == true, "echo to main did not answer true")
  local missingOk, missingMsg = echo("noSuchHeadlessWindow", "text")
  assert(missingOk == nil and type(missingMsg) == "string", "echo to a missing window did not answer nil and a message")
  headlessTriggerHit = 'none'
  local id = tempTrigger("headless fed line", [[headlessTriggerHit = line; echo("main", "headless trigger echo\n")]])
  assert(id, "tempTrigger made no trigger")
  assert(feedTriggers("headless fed line\n") == true, "feedTriggers did not answer true")
end)
headlessResult = ok and 'ok' or tostring(err)
)lua"));

        QVERIFY2(ran, "The Lua chunk did not run.");
        QCOMPARE(luaGlobalString(host, "headlessResult"), qsl("ok"));
        QCOMPARE(luaGlobalString(host, "headlessTriggerHit"), qsl("headless fed line"));
        QVERIFY2(mainBufferHolds(host, qsl("headless echo line")), "echo() to main never reached the main console model.");
        QVERIFY2(mainBufferHolds(host, qsl("headless fed line")), "feedTriggers() never reached the main console model.");
        QVERIFY2(mainBufferHolds(host, qsl("headless trigger echo")), "The trigger's echo never reached the main console model.");
        // Lets work the profile deferred, such as its first-launch timer, run before looking
        QCoreApplication::processEvents();
        QVERIFY2(QApplication::topLevelWidgets().isEmpty(), "Making and running the profile created a widget.");
    }

    void test_windowsMadeWithNoMainWindowHaveModels()
    {
        QVERIFY2(!TAppFrontend::hasView(), "A main window exists, so this run is not headless.");

        QVERIFY2(HostManager::self()->addHost(mWindowsHostname, QString(), QString(), QString()), "Could not create a profile with no main window.");
        Host* host = HostManager::self()->getHost(mWindowsHostname);
        QVERIFY2(host, "The profile is not in the pool.");
        QVERIFY(!host->hasConsoleView());

        // registerAnonymousEventHandler() is Lua from LuaGlobal, which these tests do not load
        host->registerAnonymousEventHandler(qsl("sysLabelDeleted"), qsl("onHeadlessDeleted"));
        host->registerAnonymousEventHandler(qsl("sysMiniConsoleDeleted"), qsl("onHeadlessDeleted"));
        const bool ran = host->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(
headlessWindows = 'not run'
headlessDeleted = {}
function onHeadlessDeleted(_, name) headlessDeleted[#headlessDeleted + 1] = name end
local ok, err = pcall(function()
  assert(createMiniConsole("headlessMini", 10, 20, 300, 200) == true, "createMiniConsole did not answer true")
  assert(windowType("headlessMini") == "miniconsole", "the miniconsole has no type")
  assert(echo("headlessMini", "mini line\n") == true, "echo to the miniconsole did not answer true")
  local again, againMsg = createMiniConsole("headlessMini", 0, 0, 10, 10)
  assert(again == false and againMsg:find("already exists"), "a second miniconsole of that name was not refused")

  assert(openUserWindow("headlessWindow") == true, "openUserWindow did not answer true")
  assert(windowType("headlessWindow") == "userwindow", "the user window has no type")
  assert(echo("headlessWindow", "window line\n") == true, "echo to the user window did not answer true")
  local placed, placedMsg = openUserWindow("headlessWindow", false, false, "sideways")
  assert(placed == nil and placedMsg:find("docking option"), "an unknown docking area was not refused")
  assert(windowType("headlessWindow") == "userwindow", "refusing the area took the user window away")
  assert(createMiniConsole("headlessWindow", "headlessNested", 0, 0, 50, 50) == true, "a miniconsole could not go in the user window")
  assert(createLabel("headlessWindow", "headlessNestedLabel", 0, 0, 50, 50, 1) == true, "a label could not go in the user window")
  local orphan, orphanMsg = createMiniConsole("noSuchHeadlessWindow", "headlessOrphan", 0, 0, 50, 50)
  assert(orphan == false and orphanMsg:find("not found"), "a miniconsole went into a window that does not exist")

  assert(createLabel("headlessLabel", 0, 0, 100, 20, 1) == true, "createLabel did not answer true")
  assert(windowType("headlessLabel") == "label", "the label has no type")
  assert(echo("headlessLabel", "label text") == true, "echo to the label did not answer true")
  local clash, clashMsg = createLabel("headlessMini", 0, 0, 10, 10, 1)
  assert(clash == false and clashMsg:find("already exists"), "a label took the name of a miniconsole")

  createBuffer("headlessBuffer")
  assert(windowType("headlessBuffer") == "buffer", "the buffer has no type")
  assert(echo("headlessBuffer", "buffer line\n") == true, "echo to the buffer did not answer true")

  assert(deleteLabel("headlessLabel") == true, "deleteLabel did not answer true")
  assert(windowType("headlessLabel") == nil, "the deleted label still has a type")
  assert(echo("headlessLabel", "text") == nil, "echo to the deleted label did not fail")
  local gone, goneMsg = deleteLabel("headlessLabel")
  assert(gone == false and goneMsg:find("not found"), "deleting a missing label did not fail")

  assert(deleteMiniConsole("headlessMini") == true, "deleteMiniConsole did not answer true")
  assert(windowType("headlessMini") == nil, "the deleted miniconsole still has a type")
  assert(echo("headlessMini", "text") == nil, "echo to the deleted miniconsole did not fail")

  assert(deleteMiniConsole("headlessWindow") == true, "deleting the user window did not answer true")
  assert(windowType("headlessWindow") == nil, "the deleted user window still has a type")
  assert(windowType("headlessNested") == nil, "a miniconsole in the user window outlived it")
  assert(windowType("headlessNestedLabel") == nil, "a label in the user window outlived it")
end)
headlessWindows = ok and 'ok' or tostring(err)
headlessDeletedNames = table.concat(headlessDeleted, ",")
)lua"));

        QVERIFY2(ran, "The Lua chunk did not run.");
        QCOMPARE(luaGlobalString(host, "headlessWindows"), qsl("ok"));
        // Only what a script deleted, as the GUI raises nothing for what goes with a user window
        QCOMPARE(luaGlobalString(host, "headlessDeletedNames"), qsl("headlessLabel,headlessMini,headlessWindow"));

        const TConsoleModel* buffer = host->windowRegistry().subConsoleModel(qsl("headlessBuffer"));
        QVERIFY2(buffer, "The buffer has no model.");
        QVERIFY(buffer->buffer.lineBuffer.join(QChar::LineFeed).contains(qsl("buffer line")));
        QVERIFY2(!mainBufferHolds(host, qsl("buffer line")), "Text echoed to the buffer reached the main console.");
        QVERIFY2(QApplication::topLevelWidgets().isEmpty(), "Making the windows created a widget.");
    }

    void test_labelTextReachesModelWithNoMainWindow()
    {
        Host* host = HostManager::self()->getHost(mWindowsHostname);
        QVERIFY2(host, "test_windowsMadeWithNoMainWindowHaveModels() did not leave its profile.");

        QVERIFY(host->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(createLabel("headlessTextLabel", 5, 6, 70, 80, 1); echo("headlessTextLabel", "label words"))lua")));
        const TLabelModel* label = host->windowRegistry().labelModel(qsl("headlessTextLabel"));
        QVERIFY2(label, "The label has no model.");
        QCOMPARE(label->mText, qsl("label words"));
        QCOMPARE(label->mGeometry, QRect(5, 6, 70, 80));
        QVERIFY(host->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(headlessLabelText = getLabelText("headlessTextLabel"))lua")));
        QCOMPARE(luaGlobalString(host, "headlessLabelText"), qsl("label words"));
    }

    // What a real view records as it moves, shows, styles or titles a window, the null view records too.
    void test_windowStateReadsBackWithNoMainWindow()
    {
        Host* host = HostManager::self()->getHost(mWindowsHostname);
        QVERIFY2(host, "test_windowsMadeWithNoMainWindowHaveModels() did not leave its profile.");

        const bool ran = host->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(
headlessState = 'not run'
local ok, err = pcall(function()
  assert(createLabel("stateLabel", 1, 2, 30, 40, 1) == true, "createLabel did not answer true")
  assert(getLabelStyleSheet("stateLabel") == "background-color: rgba(32, 32, 32, 255);", "the new label is not styled with its colour")
  setBackgroundColor("stateLabel", 1, 2, 3, 4)
  local lr, lg, lb, la = getBackgroundColor("stateLabel")
  assert(lr == 1 and lg == 2 and lb == 3 and la == 4, "the background colour of the label did not read back")
  assert(getLabelStyleSheet("stateLabel") == "background-color: rgba(1, 2, 3, 4);", "the label was not restyled with its colour")
  assert(setLabelStyleSheet("stateLabel", "color: red;") == true, "setLabelStyleSheet did not answer true")
  assert(getLabelStyleSheet("stateLabel") == "color: red;", "the style sheet did not read back")
  assert(setLabelToolTip("stateLabel", "a tip") == true, "setLabelToolTip did not answer true")
  assert(getLabelToolTip("stateLabel") == "a tip", "the tool tip did not read back")
  assert(setLabelCursor("stateLabel", 2) == true, "setLabelCursor did not answer true")
  local badShape, badShapeMsg = setLabelCursor("stateLabel", 99)
  assert(badShape == nil and badShapeMsg:find("cursor shape"), "an unknown cursor shape was not refused")
  local missing, missingMsg = setLabelStyleSheet("noSuchStateLabel", "color: red;")
  assert(missing == nil and missingMsg:find("not found"), "styling a missing label did not answer nil and a message")
  assert(setLabelClickCallback("stateLabel", function() end) == true, "setLabelClickCallback did not answer true")
  assert(windowVisible("stateLabel") == true, "the new label is not visible")
  hideWindow("stateLabel")
  assert(windowVisible("stateLabel") == false, "the hidden label is visible")
  assert(showWindow("stateLabel") == true, "showWindow did not answer true")
  assert(windowVisible("stateLabel") == true, "the shown label is not visible")
  moveWindow("stateLabel", 5, 6)
  resizeWindow("stateLabel", 70, 80)

  assert(createMiniConsole("stateMini", 0, 0, 10, 10) == true, "createMiniConsole did not answer true")
  moveWindow("stateMini", 7, 8)
  resizeWindow("stateMini", 90, 100)
  hideWindow("stateMini")
  assert(windowVisible("stateMini") == false, "the hidden miniconsole is visible")
  setBackgroundColor("stateMini", 10, 20, 30, 255)
  local r, g, b, a = getBackgroundColor("stateMini")
  assert(r == 10 and g == 20 and b == 30 and a == 255, "the background colour of the miniconsole did not read back")

  assert(openUserWindow("stateWindow") == true, "openUserWindow did not answer true")
  assert(createLabel("stateWindow", "stateInner", 0, 0, 10, 10, 1) == true, "a label could not go in the user window")
  hideWindow("stateWindow")
  assert(windowVisible("stateWindow") == false, "the hidden user window is visible")
  assert(windowVisible("stateInner") == false, "a label in a hidden user window is visible")
  showWindow("stateWindow")
  assert(windowVisible("stateInner") == true, "a label in a shown user window is not visible")
  assert(setWindow("main", "stateInner", 3, 4, false) == true, "setWindow did not move the label")
  hideWindow("stateWindow")
  assert(windowVisible("stateInner") == false, "a label moved out unshown is visible")
  showWindow("stateWindow")
  assert(setUserWindowTitle("stateWindow", "A title") == true, "setUserWindowTitle did not answer true")
  assert(getUserWindowTitle("stateWindow") == "A title", "the title did not read back")
  assert(setUserWindowTitle("stateWindow") == true, "resetting the title did not answer true")
  assert(getUserWindowTitle("stateWindow"):find("stateWindow", 1, true), "the default title does not name the window")
  local notWindow, notWindowMsg = setUserWindowTitle("stateMini", "A title")
  assert(notWindow == nil and notWindowMsg:find("not a user window"), "a miniconsole took a title")
  assert(setUserWindowStyleSheet("stateWindow", "background: blue;") == true, "setUserWindowStyleSheet did not answer true")
  assert(getUserWindowStyleSheet("stateWindow") == "background: blue;", "the user window style sheet did not read back")
  resizeWindow("stateWindow", 120, 130)

  setBorderColor(1, 2, 3)
  local br, bg, bb = getBorderColor()
  assert(br == 1 and bg == 2 and bb == 3, "the border colour did not read back")
end)
headlessState = ok and 'ok' or tostring(err)
)lua"));

        QVERIFY2(ran, "The Lua chunk did not run.");
        QCOMPARE(luaGlobalString(host, "headlessState"), qsl("ok"));
        const TLabelModel* label = host->windowRegistry().labelModel(qsl("stateLabel"));
        QVERIFY2(label, "The label has no model.");
        QCOMPARE(label->mGeometry, QRect(5, 6, 70, 80));
        QVERIFY(label->mClickFunction);
        QCOMPARE(host->windowRegistry().subConsoleGeometry(qsl("stateMini")), std::optional<QRect>(QRect(7, 8, 90, 100)));
        QCOMPARE(host->windowRegistry().labelModel(qsl("stateInner"))->mGeometry.topLeft(), QPoint(3, 4));
        QCOMPARE(host->windowRegistry().userWindowSize(qsl("stateWindow")), std::optional<QSize>(QSize(120, 130)));
        QVERIFY2(QApplication::topLevelWidgets().isEmpty(), "Working the windows created a widget.");
    }

    // With nothing to lay the windows out, the main console is the character grid NAWS reports and every
    // other window's grid is its size in its own font, counted as TTextEdit counts a real one's.
    void test_windowsHaveCharacterGridGeometryWithNoMainWindow()
    {
        Host* host = HostManager::self()->getHost(mWindowsHostname);
        QVERIFY2(host, "test_windowsMadeWithNoMainWindowHaveModels() did not leave its profile.");

        const bool ran = host->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(
headlessGrid = 'not run'
local ok, err = pcall(function()
  local function joined(...)
    return table.concat({...}, ",")
  end
  gridMainSize = joined(getMainWindowSize())
  gridMainGeometry = joined(getWindowGeometry("main"))
  gridMainCells = joined(getColumnCount("main"), getRowCount("main"))
  gridMainFont = joined(getFont("main"), getFontSize("main"))
  gridMainCharSize = joined(calcFontSize("main"))
  assert(joined(getUserWindowSize("main")) == gridMainSize, "the main window has a user window size of its own")

  gridFontEvents = {}
  function gridFontChanged(_, window, family, size)
    gridFontEvents[#gridFontEvents + 1] = joined(window, size)
  end
  registerAnonymousEventHandler("sysFontChangeEvent", "gridFontChanged")
  assert(createMiniConsole("gridMini", 0, 0, 300, 200) == true, "createMiniConsole did not answer true")
  gridMiniCells = joined(getColumnCount("gridMini"), getRowCount("gridMini"), getFontSize("gridMini"))
  gridMiniCharSize = joined(calcFontSize("gridMini"))
  assert(setFontSize("gridMini", 15) == true, "setFontSize did not answer true for the miniconsole")
  assert(setFontSize("gridMini", 15) == true, "setFontSize did not answer true for an unchanged size")
  gridMiniBiggerCells = joined(getColumnCount("gridMini"), getRowCount("gridMini"), getFontSize("gridMini"))
  resizeWindow("gridMini", 600, 400)
  gridMiniResizedCells = joined(getColumnCount("gridMini"), getRowCount("gridMini"))
  gridMiniColumnSize = joined(calcFontSize("gridMini", true))
  local family
  for name in pairs(getAvailableFonts()) do
    if name ~= getFont("gridMini") and name ~= "Bitstream Vera Sans Mono" then
      family = name
      break
    end
  end
  assert(setFont("gridMini", family) == true, "setFont did not answer true for the miniconsole")
  assert(setFont("gridMini", family) == true, "setFont did not answer true for an unchanged family")
  assert(joined(getUserWindowSize("gridMini")) == gridMainSize, "a miniconsole has a user window size of its own")
  local missing, missingMsg = setFontSize("noSuchGridWindow", 10)
  assert(missing == nil and type(missingMsg) == "string", "setFontSize on a missing window did not answer nil and a message")

  assert(openUserWindow("gridWindow") == true, "openUserWindow did not answer true")
  gridWindowGeometry = joined(getWindowGeometry("gridWindow"))
  gridWindowSize = joined(getUserWindowSize("gridWindow"))
  gridWindowCells = joined(getColumnCount("gridWindow"), getRowCount("gridWindow"), getFontSize("gridWindow"))

  createBuffer("gridBuffer")
  gridBufferCells = joined(getColumnCount("gridBuffer"), getRowCount("gridBuffer"), getFontSize("gridBuffer"))
  assert(setFontSize("gridBuffer", 9) == true, "setFontSize did not answer true for the buffer")
  gridFontEvents = table.concat(gridFontEvents, ";")
end)
headlessGrid = ok and 'ok' or tostring(err)
)lua"));

        QVERIFY2(ran, "The Lua chunk did not run.");
        QCOMPARE(luaGlobalString(host, "headlessGrid"), qsl("ok"));

        const auto joined = [](const QList<int>& values) {
            QStringList parts;
            for (const int value : values) {
                parts << QString::number(value);
            }
            return parts.join(QLatin1Char(','));
        };
        // What TTextEdit::getColumnCount() and getRowCount() would count in that font
        const auto cells = [](const QSize& size, const QFont& font) {
            const QFontMetricsF metrics(font);
            return QList<int>{qRound(size.width() / metrics.averageCharWidth()), qRound(size.height() / metrics.lineSpacing())};
        };
        const auto subConsoleFont = [host](const int pointSize) {
            TFontAttributes attributes(host->fontsAntiAlias());
            attributes.mPointSize = pointSize;
            return attributes.makeFont();
        };

        const QFont displayFont = host->getDisplayFont();
        const QFontMetrics displayMetrics(displayFont);
        const QSize mainSize(host->mScreenWidth * displayMetrics.averageCharWidth(), host->mScreenHeight * displayMetrics.height());
        QCOMPARE(luaGlobalString(host, "gridMainSize"), joined({mainSize.width(), mainSize.height()}));
        QCOMPARE(luaGlobalString(host, "gridMainGeometry"), joined({0, 0, mainSize.width(), mainSize.height()}));
        QCOMPARE(luaGlobalString(host, "gridMainCells"), joined({host->mScreenWidth, host->mScreenHeight}));
        QCOMPARE(luaGlobalString(host, "gridMainFont"), qsl("%1,%2").arg(QFontInfo(displayFont).family()).arg(displayFont.pointSize()));
        QCOMPARE(luaGlobalString(host, "gridMainCharSize"), joined({displayMetrics.horizontalAdvance(QChar('W')), displayMetrics.height()}));

        const QFont miniFont = subConsoleFont(12);
        const QFontMetrics miniMetrics(miniFont);
        QCOMPARE(luaGlobalString(host, "gridMiniCells"), joined(cells(QSize(300, 200), miniFont) << 12));
        QCOMPARE(luaGlobalString(host, "gridMiniCharSize"), joined({miniMetrics.horizontalAdvance(QChar('W')), miniMetrics.height()}));
        QCOMPARE(luaGlobalString(host, "gridMiniBiggerCells"), joined(cells(QSize(300, 200), subConsoleFont(15)) << 15));
        QCOMPARE(luaGlobalString(host, "gridMiniResizedCells"), joined(cells(QSize(600, 400), subConsoleFont(15))));
        const QFontMetrics biggerMiniMetrics(subConsoleFont(15));
        QCOMPARE(luaGlobalString(host, "gridMiniColumnSize"), joined({biggerMiniMetrics.averageCharWidth(), biggerMiniMetrics.height()}));

        QCOMPARE(luaGlobalString(host, "gridWindowGeometry"), joined({0, 0, mainSize.width(), mainSize.height()}));
        QCOMPARE(luaGlobalString(host, "gridWindowSize"), joined({mainSize.width(), mainSize.height()}));
        QCOMPARE(luaGlobalString(host, "gridWindowCells"), joined(cells(mainSize, subConsoleFont(10)) << 10));

        QCOMPARE(luaGlobalString(host, "gridBufferCells"), joined({0, 0, 14}));
        // As a real view tells scripts of each, and of none for a buffer
        QCOMPARE(luaGlobalString(host, "gridFontEvents"), qsl("gridMini,12;gridMini,15;gridMini,15;gridWindow,10"));

        QVERIFY(host->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(
gridFontEvents = {}
function gridSettingChanged(_, setting, family, size)
  gridFontEvents[#gridFontEvents + 1] = table.concat({setting, size}, ",")
end
registerAnonymousEventHandler("sysSettingChanged", "gridSettingChanged")
assert(setFontSize("main", 20) == true, "setFontSize did not answer true for main")
assert(setFontSize("main", 20) == true, "setFontSize did not answer true for an unchanged main size")
gridMainFontEvents = table.concat(gridFontEvents, ";")
gridMainBiggerSize = table.concat({getMainWindowSize()}, ",")
gridMainBiggerCells = table.concat({getColumnCount("main"), getRowCount("main"), getFontSize("main")}, ",")
)lua")));
        const QFontMetrics biggerMetrics(host->getDisplayFont());
        QCOMPARE(luaGlobalString(host, "gridMainBiggerSize"), joined({host->mScreenWidth * biggerMetrics.averageCharWidth(), host->mScreenHeight * biggerMetrics.height()}));
        QCOMPARE(luaGlobalString(host, "gridMainBiggerCells"), joined({host->mScreenWidth, host->mScreenHeight, 20}));
        QCOMPARE(luaGlobalString(host, "gridMainFontEvents"), qsl("main,20;main window font,20"));

        QVERIFY(host->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(
gridFontEvents = {}
gridBumped = false
function gridBumpMainFont(_, window)
  if window == "main" and not gridBumped then
    gridBumped = true
    setFontSize("main", 22)
  end
end
registerAnonymousEventHandler("sysFontChangeEvent", "gridBumpMainFont")
assert(setFontSize("main", 21) == true, "setFontSize did not answer true for main")
gridReentrantFontEvents = table.concat(gridFontEvents, ";")
)lua")));
        // As Host::updateConsolesFont() does, the outer change reports the font a handler left, not the one it set
        QCOMPARE(luaGlobalString(host, "gridReentrantFontEvents"), qsl("main,21;main,22;main window font,22;main window font,22"));
        QVERIFY2(QApplication::topLevelWidgets().isEmpty(), "Asking for the geometry created a widget.");
    }

    // What a profile with no view promises scripts: each window call answers as a real view would,
    // from the models alone, on a profile nothing else has touched.
    void test_headlessContract()
    {
        const QString hostname = qsl("Test-Headless-Host-Contract");
        QVERIFY2(HostManager::self()->addHost(hostname, QString(), QString(), QString()), "Could not create a profile with no main window.");
        Host* host = HostManager::self()->getHost(hostname);
        QVERIFY2(host, "The profile is not in the pool.");
        QVERIFY(!host->hasConsoleView());
        QVERIFY2(!host->mainConsoleShowsTimeStamps(), "A new profile shows time stamps.");

        const bool ran = host->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(
headlessContract = "not run"
local ok, err = pcall(function()
  assert(echo("main", "contract line\n") == true, "echo to main did not answer true")
  contractGrid = table.concat({getColumnCount(), getRowCount()}, ",")
  contractTimeStamps = tostring(timeStampsEnabled())
  -- "[count]" and then each value, so no values reads apart from an explicit nil
  local function described(called, ...)
    if not called then
      return "raised"
    end
    local count = select("#", ...)
    local values = {}
    for i = 1, count do
      local value = select(i, ...)
      values[i] = (i > 1 and type(value) == "string") and "msg" or tostring(value)
    end
    return "[" .. count .. "]" .. table.concat(values, ":")
  end
  local function answer(f, ...)
    return described(pcall(f, ...))
  end
  local function joined(...)
    return table.concat({...}, ",")
  end
  local missing = "noSuchContractWindow"
  local answers = {}
  for _, call in ipairs({
    {"echo", echo, missing, "text"},
    {"moveWindow", moveWindow, missing, 1, 1},
    {"resizeWindow", resizeWindow, missing, 10, 10},
    {"showWindow", showWindow, missing},
    {"hideWindow", hideWindow, missing},
    {"windowVisible", windowVisible, missing},
    {"windowType", windowType, missing},
    {"getWindowGeometry", getWindowGeometry, missing},
    {"getColumnCount", getColumnCount, missing},
    {"getRowCount", getRowCount, missing},
    {"getFont", getFont, missing},
    {"getFontSize", getFontSize, missing},
    {"setFontSize", setFontSize, missing, 10},
    {"calcFontSize", calcFontSize, missing},
    {"clearWindow", clearWindow, missing},
    {"getLineCount", getLineCount, missing},
    {"setBackgroundColor", setBackgroundColor, missing, 1, 2, 3, 255},
    {"getBackgroundColor", getBackgroundColor, missing},
    {"getLabelText", getLabelText, missing},
    {"setUserWindowTitle", setUserWindowTitle, missing, "title"},
    {"timeStampsEnabled", timeStampsEnabled, missing},
  }) do
    answers[#answers + 1] = call[1] .. "=" .. answer(unpack(call, 2))
  end
  contractMissing = table.concat(answers, ";")

  assert(createMiniConsole("contractMini", 10, 20, 200, 100) == true, "createMiniConsole did not answer true")
  local miniEcho = answer(echo, "contractMini", "mini line\n")
  local miniType = answer(windowType, "contractMini")
  contractMini = joined(miniEcho, miniType, answer(windowVisible, "contractMini"), joined(getWindowGeometry("contractMini")), getLineCount("contractMini"))
  assert(deleteMiniConsole("contractMini") == true, "deleteMiniConsole did not answer true")
  contractMiniGone = joined(answer(echo, "contractMini", "text"), answer(windowType, "contractMini"))

  assert(createLabel("contractLabel", 5, 6, 70, 80, 1) == true, "createLabel did not answer true")
  local before = getLabelText("contractLabel")
  local labelEcho = answer(echo, "contractLabel", "label words")
  contractLabel = joined(before, labelEcho, getLabelText("contractLabel"), getLabelStyleSheet("contractLabel"), joined(getBackgroundColor("contractLabel")), joined(getWindowGeometry("contractLabel")), answer(windowVisible, "contractLabel"), answer(windowType, "contractLabel"))
  deleteLabel("contractLabel")
end)
headlessContract = ok and "ok" or tostring(err)
)lua"));

        QVERIFY2(ran, "The Lua chunk did not run.");
        QCOMPARE(luaGlobalString(host, "headlessContract"), qsl("ok"));
        QVERIFY2(mainBufferHolds(host, qsl("contract line")), "echo() to main never reached the main console model.");
        // The grid NAWS reports, which with no view draws no time stamp gutter
        QCOMPARE(luaGlobalString(host, "contractGrid"), qsl("%1,%2").arg(host->mScreenWidth).arg(host->mScreenHeight));
        QCOMPARE(luaGlobalString(host, "contractTimeStamps"), qsl("false"));
        // Taken from the same Lua run in the GUI self-test profile: what a real view answers for a name it
        // has no window for, where a few calls have always answered nothing or false rather than nil and a message
        const QStringList missingAnswers{qsl("echo=[2]nil:msg"),
                                         qsl("moveWindow=[0]"),
                                         qsl("resizeWindow=[0]"),
                                         qsl("showWindow=[1]false"),
                                         qsl("hideWindow=[0]"),
                                         qsl("windowVisible=[2]nil:msg"),
                                         qsl("windowType=[2]nil:msg"),
                                         qsl("getWindowGeometry=[2]nil:msg"),
                                         qsl("getColumnCount=[2]nil:msg"),
                                         qsl("getRowCount=[2]nil:msg"),
                                         qsl("getFont=[2]nil:msg"),
                                         qsl("getFontSize=[2]nil:msg"),
                                         qsl("setFontSize=[2]nil:msg"),
                                         qsl("calcFontSize=[1]nil"),
                                         qsl("clearWindow=[0]"),
                                         qsl("getLineCount=[2]nil:msg"),
                                         qsl("setBackgroundColor=[2]nil:msg"),
                                         qsl("getBackgroundColor=[2]nil:msg"),
                                         qsl("getLabelText=[2]nil:msg"),
                                         qsl("setUserWindowTitle=[2]nil:msg"),
                                         qsl("timeStampsEnabled=[2]nil:msg")};
        QCOMPARE(luaGlobalString(host, "contractMissing"), missingAnswers.join(QLatin1Char(';')));
        QCOMPARE(luaGlobalString(host, "contractMini"), qsl("[1]true,[1]miniconsole,[1]true,10,20,200,100,1"));
        QCOMPARE(luaGlobalString(host, "contractMiniGone"), qsl("[2]nil:msg,[2]nil:msg"));
        QCOMPARE(luaGlobalString(host, "contractLabel"), qsl(",[1]true,label words,background-color: rgba(32, 32, 32, 255);,32,32,32,255,5,6,70,80,[1]true,[1]label"));
        QVERIFY2(QApplication::topLevelWidgets().isEmpty(), "Keeping the contract created a widget.");
    }

    // What a profile with no view answers the scroll calls, taken from the same Lua run in the GUI self-test
    // profile, which lets each real pane repaint before it is read and re-issues each scroll until it holds.
    void test_headlessScrollContract()
    {
        const QString hostname = qsl("Test-Headless-Host-Scroll");
        QVERIFY2(HostManager::self()->addHost(hostname, QString(), QString(), QString()), "Could not create a profile with no main window.");
        Host* host = HostManager::self()->getHost(hostname);
        QVERIFY2(host, "The profile is not in the pool.");
        QVERIFY(!host->hasConsoleView());
        // For scrollUp() and scrollDown(), which GUIUtils.lua builds on getScroll() and scrollTo()
        host->getLuaInterpreter()->loadGlobal();

        const bool ran = host->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(
headlessScroll = "not run"
local ok, err = pcall(function()
  local function described(called, ...)
    if not called then
      return "raised"
    end
    local count = select("#", ...)
    local values = {}
    for i = 1, count do
      local value = select(i, ...)
      values[i] = (i > 1 and type(value) == "string") and "msg" or tostring(value)
    end
    return "[" .. count .. "]" .. table.concat(values, ":")
  end
  local function answer(f, ...)
    return described(pcall(f, ...))
  end
  local function settled(name)
    return answer(getScroll, name)
  end
  local function park(name, line)
    local called = answer(scrollTo, name, line)
    return called .. settled(name)
  end
  local function fill(name, count)
    for i = 1, count do
      echo(name, "scroll line " .. i .. "\n")
    end
  end

  local missing = "noSuchScrollWindow"
  local answers = {}
  for _, call in ipairs({
    {"getScroll", getScroll, missing},
    {"scrollTo", scrollTo, missing, 1},
    {"scrollToEnd", scrollTo, missing},
    {"scrollUp", scrollUp, missing, 1},
    {"scrollDown", scrollDown, missing, 1},
    {"getScrollBarVisible", getScrollBarVisible, missing},
    {"enableScrollBar", enableScrollBar, missing},
    {"disableScrollBar", disableScrollBar, missing},
    {"enableHorizontalScrollBar", enableHorizontalScrollBar, missing},
    {"disableHorizontalScrollBar", disableHorizontalScrollBar, missing},
    {"enableScrolling", enableScrolling, missing},
    {"disableScrolling", disableScrolling, missing},
    {"scrollingActive", scrollingActive, missing},
  }) do
    answers[#answers + 1] = call[1] .. "=" .. answer(unpack(call, 2))
  end
  scrollMissing = table.concat(answers, ";")

  -- main is cleared first, so every console walks from no lines and reads the same in any profile
  local function walk(name)
    local steps = {}
    local function note(label, value)
      steps[#steps + 1] = label .. "=" .. value
    end
    note("fresh", answer(getScroll, name) .. answer(getScrollBarVisible, name) .. answer(scrollingActive, name))
    fill(name, 60)
    note("filled", settled(name))
    note("bar", answer(enableScrollBar, name) .. answer(getScrollBarVisible, name) .. answer(disableScrollBar, name) .. answer(getScrollBarVisible, name))
    note("hbar", answer(enableHorizontalScrollBar, name) .. answer(disableHorizontalScrollBar, name))
    note("frozen", answer(disableScrolling, name) .. answer(scrollingActive, name) .. park(name, 10))
    note("thawed", answer(enableScrolling, name) .. answer(scrollingActive, name) .. park(name, 60))
    note("to10", park(name, 10))
    note("to20", park(name, 20))
    note("toMinus5", park(name, -5))
    note("up3", answer(scrollUp, name, 3) .. settled(name))
    note("down2", answer(scrollDown, name, 2) .. settled(name))
    fill(name, 5)
    note("fedWhileBack", settled(name) .. answer(getLastLineNumber, name))
    note("frozenWhileBack", answer(disableScrolling, name) .. settled(name))
    enableScrolling(name)
    note("toMinus100", park(name, -100))
    note("toEnd", answer(scrollTo, name) .. settled(name))
    note("to1000", park(name, 1000))
    fill(name, 5)
    note("fedAtEnd", settled(name) .. answer(getLastLineNumber, name))
    park(name, 10)
    clearWindow(name)
    note("cleared", settled(name) .. answer(getLastLineNumber, name))
    fill(name, 30)
    note("fedAfterClear", settled(name) .. answer(getLastLineNumber, name))
    return table.concat(steps, ";")
  end

  fill("main", 3)
  scrollMainAtEnd = tostring(getScroll() == getLastLineNumber("main"))
  clearWindow("main")
  scrollMain = walk("main")
  scrollMainForms = table.concat({park("", 30), park("main", 25), answer(scrollTo, 20) .. settled("main"), answer(scrollTo) .. settled("main"),
    answer(disableScrolling, "") .. answer(scrollingActive, "") .. answer(enableScrolling, "") .. answer(scrollingActive)}, ";")
  assert(createMiniConsole("scrollMini", 0, 0, 300, 200) == true, "createMiniConsole did not answer true")
  scrollMini = walk("scrollMini")
  assert(openUserWindow("scrollWindow") == true, "openUserWindow did not answer true")
  scrollWindow = walk("scrollWindow")
  createBuffer("scrollBuffer")
  scrollBuffer = walk("scrollBuffer")
  deleteMiniConsole("scrollMini")
  scrollMiniGone = answer(getScroll, "scrollMini") .. answer(scrollTo, "scrollMini", 1)
end)
headlessScroll = ok and "ok" or tostring(err)
)lua"));

        QVERIFY2(ran, "The Lua chunk did not run.");
        QCOMPARE(luaGlobalString(host, "headlessScroll"), qsl("ok"));
        QCOMPARE(luaGlobalString(host, "scrollMainAtEnd"), qsl("true"));
        const QStringList missingAnswers{qsl("getScroll=[2]nil:msg"),
                                         qsl("scrollTo=[2]nil:msg"),
                                         qsl("scrollToEnd=[2]nil:msg"),
                                         qsl("scrollUp=[2]nil:msg"),
                                         qsl("scrollDown=[2]nil:msg"),
                                         qsl("getScrollBarVisible=[2]nil:msg"),
                                         qsl("enableScrollBar=[2]nil:msg"),
                                         qsl("disableScrollBar=[2]nil:msg"),
                                         qsl("enableHorizontalScrollBar=[2]nil:msg"),
                                         qsl("disableHorizontalScrollBar=[2]nil:msg"),
                                         qsl("enableScrolling=[2]nil:msg"),
                                         qsl("disableScrolling=[2]nil:msg"),
                                         qsl("scrollingActive=[2]nil:msg")};
        QCOMPARE(luaGlobalString(host, "scrollMissing"), missingAnswers.join(QLatin1Char(';')));
        const QStringList mainSteps{qsl("fresh=[1]0[1]true[1]true"),
                                    qsl("filled=[1]60"),
                                    qsl("bar=[0][1]true[0][1]false"),
                                    qsl("hbar=[0][0]"),
                                    qsl("frozen=[2]nil:msg[1]true[0][1]10"),
                                    qsl("thawed=[2]nil:msg[1]true[0][1]60"),
                                    qsl("to10=[0][1]10"),
                                    qsl("to20=[0][1]20"),
                                    qsl("toMinus5=[0][1]55"),
                                    qsl("up3=[0][1]52"),
                                    qsl("down2=[0][1]54"),
                                    qsl("fedWhileBack=[1]54[1]65"),
                                    qsl("frozenWhileBack=[2]nil:msg[1]54"),
                                    qsl("toMinus100=[0][1]0"),
                                    qsl("toEnd=[0][1]65"),
                                    qsl("to1000=[0][1]65"),
                                    qsl("fedAtEnd=[1]70[1]70"),
                                    qsl("cleared=[1]0[1]0"),
                                    qsl("fedAfterClear=[1]30[1]30")};
        QCOMPARE(luaGlobalString(host, "scrollMain"), mainSteps.join(QLatin1Char(';')));
        QCOMPARE(luaGlobalString(host, "scrollMainForms"), qsl("[0][1]30;[0][1]25;[0][1]20;[0][1]30;[1]true[1]true[1]true[1]true"));
        const QStringList subConsoleSteps{qsl("fresh=[1]0[1]false[1]true"),
                                          qsl("filled=[1]60"),
                                          qsl("bar=[0][1]true[0][1]false"),
                                          qsl("hbar=[0][0]"),
                                          qsl("frozen=[1]true[1]false[0][1]60"),
                                          qsl("thawed=[1]true[1]true[0][1]60"),
                                          qsl("to10=[0][1]10"),
                                          qsl("to20=[0][1]20"),
                                          qsl("toMinus5=[0][1]55"),
                                          qsl("up3=[0][1]52"),
                                          qsl("down2=[0][1]54"),
                                          qsl("fedWhileBack=[1]54[1]65"),
                                          qsl("frozenWhileBack=[1]true[1]65"),
                                          qsl("toMinus100=[0][1]0"),
                                          qsl("toEnd=[0][1]65"),
                                          qsl("to1000=[0][1]65"),
                                          qsl("fedAtEnd=[1]70[1]70"),
                                          qsl("cleared=[1]0[1]0"),
                                          qsl("fedAfterClear=[1]30[1]30")};
        QCOMPARE(luaGlobalString(host, "scrollMini"), subConsoleSteps.join(QLatin1Char(';')));
        QCOMPARE(luaGlobalString(host, "scrollWindow"), subConsoleSteps.join(QLatin1Char(';')));
        // Never painted, so a real buffer never reads back as scrolled
        const QStringList bufferSteps{qsl("fresh=[1]0[1]false[1]true"),
                                      qsl("filled=[1]60"),
                                      qsl("bar=[0][1]true[0][1]false"),
                                      qsl("hbar=[0][0]"),
                                      qsl("frozen=[1]true[1]true[0][1]60"),
                                      qsl("thawed=[1]true[1]true[0][1]60"),
                                      qsl("to10=[0][1]60"),
                                      qsl("to20=[0][1]60"),
                                      qsl("toMinus5=[0][1]60"),
                                      qsl("up3=[0][1]60"),
                                      qsl("down2=[0][1]60"),
                                      qsl("fedWhileBack=[1]65[1]65"),
                                      qsl("frozenWhileBack=[1]true[1]65"),
                                      qsl("toMinus100=[0][1]65"),
                                      qsl("toEnd=[0][1]65"),
                                      qsl("to1000=[0][1]65"),
                                      qsl("fedAtEnd=[1]70[1]70"),
                                      qsl("cleared=[1]0[1]0"),
                                      qsl("fedAfterClear=[1]30[1]30")};
        QCOMPARE(luaGlobalString(host, "scrollBuffer"), bufferSteps.join(QLatin1Char(';')));
        QCOMPARE(luaGlobalString(host, "scrollMiniGone"), qsl("[2]nil:msg[2]nil:msg"));
        QVERIFY2(QApplication::topLevelWidgets().isEmpty(), "Scrolling created a widget.");
    }

    // A console scrolled back stays on the same text as the buffer trims the lines above it, as a real view's
    // does once it repaints, so the line it reads back moves up by each trim.
    void test_headlessScrollFollowsTrimmedLines()
    {
        const QString hostname = qsl("Test-Headless-Host-Scroll-Trim");
        QVERIFY2(HostManager::self()->addHost(hostname, QString(), QString(), QString()), "Could not create a profile with no main window.");
        Host* host = HostManager::self()->getHost(hostname);
        QVERIFY2(host, "The profile is not in the pool.");
        QVERIFY(!host->hasConsoleView());
        host->getLuaInterpreter()->loadGlobal();

        const bool ran = host->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(
headlessTrim = "not run"
local ok, err = pcall(function()
  local function fill(name, count)
    for i = 1, count do
      echo(name, "trim line " .. i .. "\n")
    end
  end
  local function at(name)
    return getScroll(name) .. "/" .. getLastLineNumber(name)
  end
  local function walk(name)
    local steps = {}
    clearWindow(name)
    setConsoleBufferSize(name, 100, 10)
    fill(name, 100)
    steps[#steps + 1] = "filled=" .. at(name)
    scrollTo(name, 50)
    steps[#steps + 1] = "to50=" .. at(name)
    fill(name, 5)
    steps[#steps + 1] = "fed5=" .. at(name)
    fill(name, 10)
    steps[#steps + 1] = "trimmed=" .. at(name)
    fill(name, 10)
    steps[#steps + 1] = "trimmedAgain=" .. at(name)
    scrollTo(name, 5)
    fill(name, 10)
    steps[#steps + 1] = "trimmedPast=" .. at(name)
    scrollDown(name, 2)
    steps[#steps + 1] = "down2=" .. at(name)
    scrollTo(name)
    fill(name, 10)
    steps[#steps + 1] = "trimmedAtEnd=" .. at(name)
    return table.concat(steps, ";")
  end
  trimMain = walk("main")
  assert(createMiniConsole("trimMini", 0, 0, 300, 200) == true, "createMiniConsole did not answer true")
  trimMini = walk("trimMini")
  assert(openUserWindow("trimWindow") == true, "openUserWindow did not answer true")
  trimWindow = walk("trimWindow")
end)
headlessTrim = ok and "ok" or tostring(err)
)lua"));

        QVERIFY2(ran, "The Lua chunk did not run.");
        QCOMPARE(luaGlobalString(host, "headlessTrim"), qsl("ok"));
        // What the same steps read in the GUI self-test profile once each console has repainted
        const QString steps = qsl("filled=90/90;to50=50/90;fed5=50/95;trimmed=40/95;trimmedAgain=30/95;trimmedPast=0/95;down2=2/95;trimmedAtEnd=95/95");
        QCOMPARE(luaGlobalString(host, "trimMain"), steps);
        QCOMPARE(luaGlobalString(host, "trimMini"), steps);
        QCOMPARE(luaGlobalString(host, "trimWindow"), steps);
    }

    // The font and wrap calls answer every kind of window as the GUI self-test profile does, with the pixel
    // sizes those fonts measure; a label takes the font of the console it is made on, and keeps it.
    void test_headlessFontAndWrapContract()
    {
        const QString hostname = qsl("Test-Headless-Host-Font-Wrap");
        QVERIFY2(HostManager::self()->addHost(hostname, QString(), QString(), QString()), "Could not create a profile with no main window.");
        Host* host = HostManager::self()->getHost(hostname);
        QVERIFY2(host, "The profile is not in the pool.");
        QVERIFY(!host->hasConsoleView());
        const QFont startFont = host->getDisplayFont();

        const bool ran = host->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(
headlessFontWrap = "not run"
local ok, err = pcall(function()
  local function described(called, ...)
    if not called then
      return "raised"
    end
    local count = select("#", ...)
    local values = {}
    for i = 1, count do
      local value = select(i, ...)
      values[i] = (i > 1 and type(value) == "string") and "msg" or tostring(value)
    end
    return "[" .. count .. "]" .. table.concat(values, ":")
  end
  local function answer(f, ...)
    return described(pcall(f, ...))
  end
  local startFamily = getFont("main")
  local fonts = getAvailableFonts()
  for _, candidate in ipairs({"DejaVu Sans", "Ubuntu Mono", "Liberation Mono", "Noto Sans Mono"}) do
    if fonts[candidate] then
      fwFamily = candidate
      break
    end
  end
  if not fwFamily then
    for name in pairs(fonts) do
      if name ~= getFont("main") and name ~= "Bitstream Vera Sans Mono" then
        fwFamily = name
        break
      end
    end
  end
  fwGlobal = table.concat({answer(calcFontSize), answer(calcFontSize, true), answer(getFont), answer(getFontSize), answer(getWindowWrap)}, ";")

  assert(createMiniConsole("fwMini", 0, 0, 300, 200) == true, "createMiniConsole did not answer true")
  assert(openUserWindow("fwWindow") == true, "openUserWindow did not answer true")
  createBuffer("fwBuffer")
  assert(createLabel("fwLabel", 0, 0, 100, 20, 1) == true, "createLabel did not answer true")
  assert(createLabel("fwWindow", "fwWindowLabel", 0, 0, 100, 20, 1) == true, "createLabel did not answer true in a user window")
  local long = string.rep("word ", 14) .. "end"
  local function walk(name)
    local steps = {}
    local function note(label, ...)
      steps[#steps + 1] = label .. "=" .. answer(...)
    end
    note("size", calcFontSize, name)
    note("avg", calcFontSize, name, true)
    note("font", getFont, name)
    note("pt", getFontSize, name)
    note("wrap", getWindowWrap, name)
    note("wrap0", setWindowWrap, name, 0)
    note("wrap30", setWindowWrap, name, 30)
    note("wrapNow", getWindowWrap, name)
    note("indent", setWindowWrapIndent, name, 2)
    note("indentNeg", setWindowWrapIndent, name, -1)
    note("hanging", setWindowWrapHangingIndent, name, 3)
    if getLineCount(name) then
      echo(name, "\n" .. long .. "\n")
      local last = getLastLineNumber(name)
      local lengths = {}
      for i = last - 4, last do
        lengths[#lengths + 1] = #(getLines(name, i, i + 1)[1] or "")
      end
      steps[#steps + 1] = "lengths=" .. table.concat(lengths, ",")
    end
    note("wrapLine", wrapLine, name, 0)
    note("wrapBack", setWindowWrap, name, 100)
    note("pt0", setFontSize, name, 0)
    note("pt13", setFontSize, name, 13)
    note("size13", calcFontSize, name)
    note("avg13", calcFontSize, name, true)
    note("fontEmpty", setFont, name, "")
    note("fontMissing", setFont, name, "No Such Font Family Zz")
    note("fontFamily", setFont, name, fwFamily)
    note("familyFont", getFont, name)
    note("familyPt", getFontSize, name)
    note("familySize", calcFontSize, name)
    note("familyAvg", calcFontSize, name, true)
    return table.concat(steps, ";")
  end
  fwEmpty = walk("")
  fwMain = walk("main")
  fwMiniSteps = walk("fwMini")
  fwWindowSteps = walk("fwWindow")
  fwBufferSteps = walk("fwBuffer")
  fwLabelSteps = walk("fwLabel")
  fwMissing = walk("noSuchFontWindow")

  assert(createLabel("fwLateLabel", 0, 0, 100, 20, 1) == true, "createLabel did not answer true")
  assert(setFont("main", startFamily) == true, "setFont did not answer true for main")
  assert(createLabel("fwWindow", "fwWindowLateLabel", 0, 0, 100, 20, 1) == true, "createLabel did not answer true in a user window")
  fwLabelFonts = table.concat({getFont("fwWindowLabel"), getFont("fwLateLabel"), getFont("fwWindowLateLabel")}, ";")

  assert(createLabel("fwMovedLabel", 0, 0, 100, 20, 1) == true, "createLabel did not answer true")
  assert(setWindow("fwWindow", "fwMovedLabel", 0, 0, false) == true, "setWindow did not answer true for a label")
  assert(setWindow("main", "fwWindowLabel", 0, 0, true) == true, "setWindow did not answer true for a label")
  assert(setWindow("fwWindow", "fwLabel", 0, 0, true) == true, "setWindow did not answer true for a label")
  assert(setWindow("main", "fwLateLabel", 0, 0, true) == true, "setWindow did not answer true for a label")
  assert(setWindow("fwWindow", "fwWindowLateLabel", 0, 0, true) == true, "setWindow did not answer true for a label")
  fwMovedFonts = table.concat({getFont("fwMovedLabel"), getFont("fwWindowLabel"), getFont("fwLabel"), getFont("fwLateLabel"), getFont("fwWindowLateLabel")}, ";")
end)
headlessFontWrap = ok and "ok" or tostring(err)
)lua"));

        QVERIFY2(ran, "The Lua chunk did not run.");
        QCOMPARE(luaGlobalString(host, "headlessFontWrap"), qsl("ok"));

        const auto sized = [](const QFont& font, const int pointSize) {
            TFontAttributes attributes(font);
            attributes.mPointSize = pointSize;
            return attributes.makeFont();
        };
        const auto measured = [](const QFont& font) {
            const QFontMetrics metrics(font);
            return qsl("[2]%1:%2").arg(metrics.horizontalAdvance(QChar('W'))).arg(metrics.height());
        };
        const auto averaged = [](const QFont& font) {
            const QFontMetrics metrics(font);
            return qsl("[2]%1:%2").arg(metrics.averageCharWidth()).arg(metrics.height());
        };
        const auto family = [](const QFont& font) {
            return QFontInfo(font).family();
        };
        const QFont familyFont = host->createFontWithSettings(luaGlobalString(host, "fwFamily"), 13);
        // What the same steps read in the GUI self-test profile, its sizes measured here in the fonts it had
        const auto consoleSteps = [&](const QFont& font, const int wrap) {
            return QStringList{qsl("size=%1").arg(measured(font)),
                               qsl("avg=%1").arg(averaged(font)),
                               qsl("font=[1]%1").arg(family(font)),
                               qsl("pt=[1]%1").arg(font.pointSize()),
                               qsl("wrap=[1]%1").arg(wrap),
                               qsl("wrap0=[2]nil:msg"),
                               qsl("wrap30=[1]true"),
                               qsl("wrapNow=[1]30"),
                               qsl("indent=[1]true"),
                               qsl("indentNeg=[2]nil:msg"),
                               qsl("hanging=[1]true"),
                               qsl("lengths=0,27,28,26,0"),
                               qsl("wrapLine=[0]"),
                               qsl("wrapBack=[1]true"),
                               qsl("pt0=[2]nil:msg"),
                               qsl("pt13=[1]true"),
                               qsl("size13=%1").arg(measured(sized(font, 13))),
                               qsl("avg13=%1").arg(averaged(sized(font, 13))),
                               qsl("fontEmpty=[2]nil:msg"),
                               qsl("fontMissing=[2]nil:msg"),
                               qsl("fontFamily=[1]true"),
                               qsl("familyFont=[1]%1").arg(family(familyFont)),
                               qsl("familyPt=[1]13"),
                               qsl("familySize=%1").arg(measured(familyFont)),
                               qsl("familyAvg=%1").arg(averaged(familyFont))}
                    .join(QLatin1Char(';'));
        };
        // A label has a font but no size, wrap or text of its own; a missing name has none of them
        const auto otherSteps = [&](const QString& font, const QString& familyAnswer) {
            return qsl("size=[1]nil;avg=[1]nil;font=%1;pt=[2]nil:msg;wrap=[2]nil:msg;wrap0=[2]nil:msg;wrap30=[2]nil:msg;wrapNow=[2]nil:msg;"
                       "indent=[2]nil:msg;indentNeg=[2]nil:msg;hanging=[2]nil:msg;wrapLine=[0];wrapBack=[2]nil:msg;pt0=[2]nil:msg;pt13=[2]nil:msg;"
                       "size13=[1]nil;avg13=[1]nil;fontEmpty=[2]nil:msg;fontMissing=[2]nil:msg;fontFamily=%2;familyFont=%3;familyPt=[2]nil:msg;"
                       "familySize=[1]nil;familyAvg=[1]nil")
                    .arg(font, familyAnswer.isEmpty() ? qsl("[2]nil:msg") : qsl("[1]true"), familyAnswer.isEmpty() ? qsl("[2]nil:msg") : familyAnswer);
        };

        QCOMPARE(luaGlobalString(host, "fwGlobal"), qsl("%1;%2;[1]%3;[1]%4;[1]100").arg(measured(startFont), averaged(startFont), family(startFont)).arg(startFont.pointSize()));
        QCOMPARE(luaGlobalString(host, "fwEmpty"), consoleSteps(startFont, 100));
        QCOMPARE(luaGlobalString(host, "fwMain"), consoleSteps(familyFont, 100));
        // Unlike main, a miniconsole or user window starts with no wrap at all
        QCOMPARE(luaGlobalString(host, "fwMiniSteps"), consoleSteps(sized(startFont, 12), 99999999));
        QCOMPARE(luaGlobalString(host, "fwWindowSteps"), consoleSteps(sized(startFont, 10), 99999999));
        QCOMPARE(luaGlobalString(host, "fwBufferSteps"), consoleSteps(startFont, 100));
        QCOMPARE(luaGlobalString(host, "fwLabelSteps"), otherSteps(qsl("[1]%1").arg(family(startFont)), qsl("[1]%1").arg(family(familyFont))));
        QCOMPARE(luaGlobalString(host, "fwMissing"), otherSteps(qsl("[2]nil:msg"), QString()));
        QCOMPARE(luaGlobalString(host, "fwLabelFonts"), qsl("%1;%2;%2").arg(family(sized(startFont, 10)), family(familyFont)));
        // Moved to another window, one given no font by setFont() takes the application's
        QCOMPARE(luaGlobalString(host, "fwMovedFonts"), qsl("%1;%1;%2;%2;%2").arg(family(QFont()), family(familyFont)));
        QVERIFY2(QApplication::topLevelWidgets().isEmpty(), "The font and wrap calls created a widget.");
    }

    // The trim that raises sysBufferShrinkEvent runs inside TBuffer::append(), and echo goes on using the
    // console's model after that returns, so a handler deleting the console must not free it there and then.
    void test_consoleDeletedByItsOwnShrinkEventWithNoMainWindow()
    {
        Host* host = HostManager::self()->getHost(mWindowsHostname);
        QVERIFY2(host, "test_windowsMadeWithNoMainWindowHaveModels() did not leave its profile.");
        host->registerAnonymousEventHandler(qsl("sysBufferShrinkEvent"), qsl("onHeadlessShrink"));

        QVERIFY(host->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(
headlessShrinks = 0
function onHeadlessShrink(_, name)
  if name == "headlessShrinkBuffer" then
    headlessShrinks = headlessShrinks + 1
    deleteMiniConsole("headlessShrinkBuffer")
  elseif name == "headlessShrinkNested" then
    headlessShrinks = headlessShrinks + 1
    deleteMiniConsole("headlessShrinkWindow")
  end
end
local lines = string.rep("shrink line\n", 150)
createBuffer("headlessShrinkBuffer")
setConsoleBufferSize("headlessShrinkBuffer", 100, 10)
headlessShrinkEcho = tostring(echo("headlessShrinkBuffer", lines))
openUserWindow("headlessShrinkWindow")
createMiniConsole("headlessShrinkWindow", "headlessShrinkNested", 0, 0, 50, 50)
setConsoleBufferSize("headlessShrinkNested", 100, 10)
headlessShrinkNestedEcho = tostring(echo("headlessShrinkNested", lines))
headlessShrinkLeft = table.concat({tostring(windowType("headlessShrinkBuffer")), tostring(windowType("headlessShrinkWindow")), tostring(windowType("headlessShrinkNested"))}, ",")
)lua")));
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);

        QVERIFY2(luaGlobalString(host, "headlessShrinks").toInt() >= 2, "A shrink event never reached the handler, so nothing was deleted mid-echo.");
        QCOMPARE(luaGlobalString(host, "headlessShrinkEcho"), qsl("true"));
        QCOMPARE(luaGlobalString(host, "headlessShrinkNestedEcho"), qsl("true"));
        QCOMPARE(luaGlobalString(host, "headlessShrinkLeft"), qsl("nil,nil,nil"));
        QVERIFY(host->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(createBuffer("headlessShrinkBuffer"); echo("headlessShrinkBuffer", "made again\n"))lua")));
        const TConsoleModel* remade = host->windowRegistry().subConsoleModel(qsl("headlessShrinkBuffer"));
        QVERIFY2(remade, "A buffer of the deleted one's name could not be made again.");
        QVERIFY(remade->buffer.lineBuffer.join(QChar::LineFeed).contains(qsl("made again")));
    }

    // As MXP_spec.lua's checks of the same tags in the GUI: a frame is a miniconsole under its name, and
    // what the game sends between <DEST> and </DEST> goes there and not into the main window.
    void test_mxpFrameTakesDestTextWithNoMainWindow()
    {
        Host* host = HostManager::self()->getHost(mWindowsHostname);
        QVERIFY2(host, "test_windowsMadeWithNoMainWindowHaveModels() did not leave its profile.");

        const bool ran = host->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(
headlessFrames = 'not run'
local ok, err = pcall(function()
  setConfig("specialForceMXPProcessorOn", true)
  headlessDestTriggered = "no"
  local id = tempTrigger("MXPDEST", [[headlessDestTriggered = "yes"]])
  feedTriggers([[<FRAME Name="headlessFrame" Align="right" Width="20%" Height="30%">]] .. "\n")
  assert(windowType("headlessFrame") == "miniconsole", "the frame has no console")
  feedTriggers([[<DEST headlessFrame>MXPDEST first</DEST>]] .. "\n")
  feedTriggers([[<DEST headlessFrame>MXPDEST second</DEST>]] .. "\n")
  headlessFrameLines = table.concat(getLines("headlessFrame", 0, getLineCount("headlessFrame") + 1), "|")
  feedTriggers([[<DEST headlessFrame EOF>MXPDEST after clear</DEST>]] .. "\n")
  headlessClearedLines = table.concat(getLines("headlessFrame", 0, getLineCount("headlessFrame") + 1), "|")
  killTrigger(id)
  feedTriggers([[<FRAME headlessFrame ACTION="close">]] .. "\n")
  assert(windowType("headlessFrame") == nil, "the closed frame still has a console")
  setConfig("specialForceMXPProcessorOn", false)
end)
headlessFrames = ok and 'ok' or tostring(err)
)lua"));

        QVERIFY2(ran, "The Lua chunk did not run.");
        QCOMPARE(luaGlobalString(host, "headlessFrames"), qsl("ok"));
        QCOMPARE(luaGlobalString(host, "headlessFrameLines"), qsl("MXPDEST first|MXPDEST second|"));
        QCOMPARE(luaGlobalString(host, "headlessClearedLines"), qsl("MXPDEST after clear|"));
        QVERIFY2(!mainBufferHolds(host, qsl("MXPDEST")), "Text sent to the frame reached the main console.");
        QCOMPARE(luaGlobalString(host, "headlessDestTriggered"), qsl("no"));
        QVERIFY2(QApplication::topLevelWidgets().isEmpty(), "The frame created a widget.");
    }

    void test_profileConnectsAndLogsInWithNoMainWindow()
    {
        QVERIFY2(!TAppFrontend::hasView(), "A main window exists, so this run is not headless.");

        QTcpServer server;
        QVERIFY2(server.listen(QHostAddress::LocalHost), "Could not start the local test server.");
        QByteArray received;
        QTcpSocket* client = nullptr;
        connect(&server, &QTcpServer::newConnection, this, [&]() {
            client = server.nextPendingConnection();
            connect(client, &QTcpSocket::readyRead, this, [&]() {
                received.append(client->readAll());
            });
            client->write("Welcome to the headless test server.\r\n");
        });

        QVERIFY2(HostManager::self()->addHost(mConnectingHostname, QString(), QString(), QString()), "Could not create a profile with no main window.");
        Host* host = HostManager::self()->getHost(mConnectingHostname);
        QVERIFY2(host, "The profile is not in the pool.");
        QVERIFY(!host->hasConsoleView());
        host->setLogin(qsl("headlesshero"));
        host->setPass(qsl("headlesssecret"));
        QVERIFY(host->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(headlessGreeting = 'none'; tempTrigger("headless test server", [[headlessGreeting = line]]))lua")));

        host->mTelnet.connectIt(qsl("127.0.0.1"), server.serverPort());

        QTRY_COMPARE_WITH_TIMEOUT(luaGlobalString(host, "headlessGreeting"), qsl("Welcome to the headless test server."), 10000);
        QVERIFY2(mainBufferHolds(host, qsl("Welcome to the headless test server.")), "The game's line never reached the main console model.");
        QVERIFY2(mainBufferHolds(host, qsl("Open connection made")), "The connection's own messages never reached the main console model.");
        // The login and password go out on timers, 2s and then 1s by default
        QTRY_VERIFY_WITH_TIMEOUT(received.contains("headlesshero") && received.contains("headlesssecret"), 10000);

        host->mTelnet.disconnectIt();
        QTRY_COMPARE_WITH_TIMEOUT(host->mTelnet.getConnectionState(), QAbstractSocket::UnconnectedState, 10000);
        QVERIFY2(QApplication::topLevelWidgets().isEmpty(), "Connecting the profile created a widget.");
    }

    void test_heldLineCommitsBeforeDisconnectWithNoMainWindow()
    {
        QVERIFY2(!TAppFrontend::hasView(), "A main window exists, so this run is not headless.");

        // Prose that runs right up to the wrap column, so undoing server wrap holds it back
        const QByteArray heldLine = "Welcome traveller, the gate stands open and";
        QTcpServer server;
        QVERIFY2(server.listen(QHostAddress::LocalHost), "Could not start the local test server.");
        connect(&server, &QTcpServer::newConnection, this, [&]() {
            QTcpSocket* client = server.nextPendingConnection();
            client->write(heldLine + "\r\n");
            client->disconnectFromHost();
        });

        QVERIFY2(HostManager::self()->addHost(mHeldLineHostname, QString(), QString(), QString()), "Could not create a profile with no main window.");
        Host* host = HostManager::self()->getHost(mHeldLineHostname);
        QVERIFY2(host, "The profile is not in the pool.");
        host->mUndoServerWrap = true;
        host->mUndoServerWrapWidth = static_cast<int>(heldLine.size());
        // Long enough that only the disconnect, not either timer, can commit the held line
        host->mTelnet.setPostingTimeout(60000);
        host->mServerWrapFlushTimer.setInterval(60000);
        QSignalSpy held(&host->mainConsoleModel().mNotifier, &TConsoleModelNotifier::serverWrapLineHeld);
        QVERIFY(host->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(headlessHeldLine = 'none'; tempTrigger("the gate stands open", [[headlessHeldLine = line]]))lua")));
        QString heldLineAtDisconnect;
        // Scoped to this test, as the profile outlives the locals the slot writes to
        QObject receiver;
        connect(&host->mTelnet, &cTelnet::signal_disconnected, &receiver, [&]() {
            heldLineAtDisconnect = luaGlobalString(host, "headlessHeldLine");
        });

        host->mTelnet.connectIt(qsl("127.0.0.1"), server.serverPort());

        QTRY_VERIFY_WITH_TIMEOUT(!heldLineAtDisconnect.isEmpty(), 10000);
        QCOMPARE(held.count(), 1);
        QCOMPARE(heldLineAtDisconnect, QString::fromUtf8(heldLine));
    }

#if !defined(QT_NO_SSL)
    void test_untrustedCertificateWithNoMainWindow()
    {
        QVERIFY2(!TAppFrontend::hasView(), "A main window exists, so this run is not headless.");
        if (QSslSocket::activeBackend() != QLatin1String("openssl")) {
            QSKIP("Serving a certificate from an in-memory PEM key is only relied on with the OpenSSL backend.");
        }

        // Self-signed, so the profile refuses it; valid until 2126 so it never fails as expired instead
        static constexpr char certificatePem[] = R"(-----BEGIN CERTIFICATE-----
MIIBfzCCASWgAwIBAgIUNVnXBt8EptvmkFkhxe/ibqLOLRIwCgYIKoZIzj0EAwIw
FDESMBAGA1UEAwwJbG9jYWxob3N0MCAXDTI2MTAwOTIxNTA1OFoYDzIxMjYwOTE1
MjE1MDU4WjAUMRIwEAYDVQQDDAlsb2NhbGhvc3QwWTATBgcqhkjOPQIBBggqhkjO
PQMBBwNCAASh4Kz7zVzveu+VpaQSoceVFsH6I6qOfbYT0tapBHTFGBkf6NgxBGen
wL5TDeL9g3w57+FWiHtIKUylQhCoNb20o1MwUTAdBgNVHQ4EFgQUdeivXGb0CJyG
TeJIhMFeiKCyPV0wHwYDVR0jBBgwFoAUdeivXGb0CJyGTeJIhMFeiKCyPV0wDwYD
VR0TAQH/BAUwAwEB/zAKBggqhkjOPQQDAgNIADBFAiEA4ktl+ztKx3yhNaOQRVvo
t7BeROX3QMOcjmtFD1z7gMMCIBCdl4RZ9RpZqyngwieUaEVXNou189dJOrb5/4Iu
62HC
-----END CERTIFICATE-----)";
        static constexpr char keyPem[] = R"(-----BEGIN PRIVATE KEY-----
MIGHAgEAMBMGByqGSM49AgEGCCqGSM49AwEHBG0wawIBAQQgrC6UFloRMFbttYkA
mx8Y3UWOzQ/JtyVfFEEK23/u4iGhRANCAASh4Kz7zVzveu+VpaQSoceVFsH6I6qO
fbYT0tapBHTFGBkf6NgxBGenwL5TDeL9g3w57+FWiHtIKUylQhCoNb20
-----END PRIVATE KEY-----)";
        QSslConfiguration configuration = QSslConfiguration::defaultConfiguration();
        configuration.setLocalCertificate(QSslCertificate(QByteArray(certificatePem)));
        configuration.setPrivateKey(QSslKey(QByteArray(keyPem), QSsl::Ec));
        QSslServer server;
        server.setSslConfiguration(configuration);
        QVERIFY2(server.listen(QHostAddress::LocalHost), "Could not start the local test server.");

        QVERIFY2(HostManager::self()->addHost(mSecureHostname, QString(), QString(), QString()), "Could not create a profile with no main window.");
        Host* host = HostManager::self()->getHost(mSecureHostname);
        QVERIFY2(host, "The profile is not in the pool.");
        host->mSslTsl = true;
        QSignalSpy disconnected(&host->mTelnet, &cTelnet::signal_disconnected);

        // Not 127.0.0.1: a secure connect dials the name the lookup returns, and Windows
        // reverse-resolves 127.0.0.1 to the machine's own name, whose addresses this server is not on
        host->mTelnet.connectIt(qsl("localhost"), server.serverPort());

        // The app opens the profile's connection preferences here, which needs a main window
        QTRY_VERIFY_WITH_TIMEOUT(!disconnected.isEmpty(), 10000);
        QVERIFY2(!host->mTelnet.getSslErrors().isEmpty(), "The connection did not fail on the certificate.");
        QVERIFY2(mainBufferHolds(host, qsl("self-signed")), "Why the connection was refused never reached the main console model.");
        QVERIFY2(QApplication::topLevelWidgets().isEmpty(), "The refused connection created a widget.");
    }
#endif

    // A deleted window's model outlives the call that deleted it, as a real view's widget does, and goes
    // when deferred deletes run; twice, as each release has to queue the next.
    void test_deletedWindowModelGoesWithDeferredDeletes()
    {
        const QString hostname = qsl("Test-Headless-Host-Release");
        QVERIFY2(HostManager::self()->addHost(hostname, QString(), QString(), QString()), "Could not create a profile with no main window.");
        Host* host = HostManager::self()->getHost(hostname);
        QVERIFY2(host, "The profile is not in the pool.");

        for (int round = 0; round < 2; ++round) {
            QVERIFY(host->getLuaInterpreter()->compileAndExecuteScript(qsl("assert(createMiniConsole('releasedMini', 0, 0, 10, 10))")));
            TConsoleModel* model = host->windowRegistry().subConsoleModel(qsl("releasedMini"));
            QVERIFY2(model, "The mini console has no model.");
            const QPointer<TConsoleModelNotifier> notifier = &model->mNotifier;
            QVERIFY(host->getLuaInterpreter()->compileAndExecuteScript(qsl("assert(deleteMiniConsole('releasedMini'))")));
            QVERIFY2(notifier, "The model went before deferred deletes ran.");
            QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
            QVERIFY2(!notifier, "The model outlived deferred deletes.");
        }
    }

    void test_profileGetsNullAppViewWithNoMainWindow()
    {
        QVERIFY2(!TAppFrontend::hasView(), "A main window exists, so this run is not headless.");
        TAppFrontend* app = TAppFrontend::instance();
        QVERIFY2(app, "TAppFrontend::instance() must never be null.");
        QVERIFY(!app->getActiveHost());
        QCOMPARE(app->profileTabIndex(mHostname), -1);
        QVERIFY(app->getOpenFileName(qsl("title"), QDir::tempPath()).isEmpty());
        QVERIFY(!app->quitting());

        // Scripts get this text once the command bindings stop checking for a main window themselves
        const TAppFrontend::CommandRequest request{.name = qsl("headless command")};
        QString error = qsl("left over from an earlier call");
        QCOMPARE(app->addAddonCommand(request, nullptr, QString(), error), -1);
        QCOMPARE(error, qsl("mudlet instance not available"));
        error = qsl("left over from an earlier call");
        QCOMPARE(app->setAddonCommandPulse(1, true, qsl("red"), qsl("blue"), 500, nullptr, error), false);
        QCOMPARE(error, qsl("mudlet instance not available"));

        const QString hostname = qsl("Test-Headless-Host-App-View");
        QVERIFY2(HostManager::self()->addHost(hostname, QString(), QString(), QString()), "Could not create a profile with no main window.");
        Host* host = HostManager::self()->getHost(hostname);
        QVERIFY2(host, "The profile is not in the pool.");

        // Each of these calls the app view without asking whether there is one
        const bool ran = host->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(
headlessAppResult = 'not run'
local ok, err = pcall(function()
  local tab, tabMsg = getProfileTabNumber()
  assert(tab == nil and type(tabMsg) == "string", "getProfileTabNumber did not answer nil and a message")
  assert(openWebPage("about:blank") == false, "openWebPage did not answer false")
  assert(loadWindowLayout() == false, "loadWindowLayout did not answer false")
  assert(showNotification("headless title", "headless text") == true, "showNotification did not answer true")
end)
headlessAppResult = ok and 'ok' or tostring(err)
)lua"));
        QVERIFY2(ran, "The Lua chunk did not run.");
        QCOMPARE(luaGlobalString(host, "headlessAppResult"), qsl("ok"));
    }

    void test_scriptsSeeNoMainWindowAsTheyDidBeforeTheNullAppView()
    {
        QVERIFY2(!TAppFrontend::hasView(), "A main window exists, so this run is not headless.");

        const QString hostname = qsl("Test-Headless-Host-App-Answers");
        QVERIFY2(HostManager::self()->addHost(hostname, QString(), QString(), QString()), "Could not create a profile with no main window.");
        Host* host = HostManager::self()->getHost(hostname);
        QVERIFY2(host, "The profile is not in the pool.");

        // addCommand and setCommandPulse take this answer from the null view, removeCommand from its own check
        const bool ran = host->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(
headlessAnswersResult = 'not run'
local ok, err = pcall(function()
  local missing = "mudlet instance not available"
  local id, idMsg = addCommand({name = "Headless", menuPath = "Headless"})
  assert(id == nil and idMsg == missing, "addCommand answered " .. tostring(id) .. ", " .. tostring(idMsg))
  local pulse, pulseMsg = setCommandPulse(1, true)
  assert(pulse == nil and pulseMsg == missing, "setCommandPulse answered " .. tostring(pulse) .. ", " .. tostring(pulseMsg))
  local removed, removedMsg = removeCommand(1)
  assert(removed == nil and removedMsg == missing, "removeCommand answered " .. tostring(removed) .. ", " .. tostring(removedMsg))
  assert(invokeFileDialog(true, "headless") == "", "invokeFileDialog did not answer an empty string")
  alert(0)
  local shown, shownMsg = showToolBar("headless missing toolbar")
  assert(shown == nil and type(shownMsg) == "string", "showToolBar did not answer nil and a message")
end)
headlessAnswersResult = ok and 'ok' or tostring(err)
)lua"));
        QVERIFY2(ran, "The Lua chunk did not run.");
        QCOMPARE(luaGlobalString(host, "headlessAnswersResult"), qsl("ok"));
    }

    // As the GUI answers with no saved settings
    void test_mainWindowSettingsRoundTripWithNoMainWindow()
    {
        QVERIFY2(!TAppFrontend::hasView(), "A main window exists, so this run is not headless.");

        const QString hostname = qsl("Test-Headless-Host-App-Settings");
        QVERIFY2(HostManager::self()->addHost(hostname, QString(), QString(), QString()), "Could not create a profile with no main window.");
        Host* host = HostManager::self()->getHost(hostname);
        QVERIFY2(host, "The profile is not in the pool.");

        const bool ran = host->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(
headlessSettingsResult = 'not run'
local ok, err = pcall(function()
  local function check(label, want, ...)
    local n = select("#", ...)
    local got = ...
    assert(n == 1 and got == want, label .. " answered " .. n .. " values: " .. tostring(got))
  end
  local key = "showTabConnectionIndicators"
  check("getConfig default", false, getConfig(key))
  check("setConfig true", true, setConfig(key, true))
  check("getConfig after true", true, getConfig(key))
  check("setConfig false", true, setConfig(key, false))
  check("getConfig after false", false, getConfig(key))
end)
headlessSettingsResult = ok and 'ok' or tostring(err)
)lua"));
        QVERIFY2(ran, "The Lua chunk did not run.");
        QCOMPARE(luaGlobalString(host, "headlessSettingsResult"), qsl("ok"));
    }

    // As Other_spec.lua pins for the GUI: one event per real change, none for a write of the value held
    void test_settingChangesRaiseEventsWithNoMainWindow()
    {
        QVERIFY2(!TAppFrontend::hasView(), "A main window exists, so this run is not headless.");

        const QString hostname = qsl("Test-Headless-Host-Setting-Events");
        QVERIFY2(HostManager::self()->addHost(hostname, QString(), QString(), QString()), "Could not create a profile with no main window.");
        Host* host = HostManager::self()->getHost(hostname);
        QVERIFY2(host, "The profile is not in the pool.");

        // registerAnonymousEventHandler() is Lua from LuaGlobal, which these tests do not load
        host->registerAnonymousEventHandler(qsl("sysSettingChanged"), qsl("onHeadlessSettingChanged"));
        const bool ran = host->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(
headlessSettingEvents = 'not run'
local events = {}
function onHeadlessSettingChanged(_, key, value)
  events[#events + 1] = {key = key, value = value, readBack = getConfig(key)}
end
local ok, err = pcall(function()
  assert(getConfig("muteMediaAPI") == false and getConfig("muteMediaGame") == false, "media starts muted")
  local keys = {"muteMediaAPI", "muteMediaGame", "compactInputLine", "mapperPanelVisible", "enableClosedCaption", "advertiseScreenReader", "announceIncomingText"}
  for _, key in ipairs(keys) do
    local start = getConfig(key)
    for _, step in ipairs({{not start, 1}, {not start, 0}, {start, 1}}) do
      local value, want = step[1], step[2]
      events = {}
      assert(setConfig(key, value) == true, "setConfig " .. key .. " refused")
      assert(#events == want, key .. " set to " .. tostring(value) .. " raised " .. #events .. " events, not " .. want)
      if want == 1 then
        local event = events[1]
        assert(event.key == key and event.value == value and event.readBack == value,
          key .. " raised " .. tostring(event.key) .. " = " .. tostring(event.value) .. ", read back " .. tostring(event.readBack))
      end
    end
  end
end)
headlessSettingEvents = ok and 'ok' or tostring(err)
)lua"));
        QVERIFY2(ran, "The Lua chunk did not run.");
        QCOMPARE(luaGlobalString(host, "headlessSettingEvents"), qsl("ok"));
    }

    // The answers and events are those a GUI probe of the same calls gave, with the profile names changed
    void test_appStyleSheetWithNoMainWindow()
    {
        QVERIFY2(!TAppFrontend::hasView(), "A main window exists, so this run is not headless.");

        const QString hostname = qsl("Test-Headless-Host-App-Style");
        const QString otherHostname = qsl("Test-Headless-Host-App-Style-Other");
        QVERIFY2(HostManager::self()->addHost(hostname, QString(), QString(), QString()), "Could not create a profile with no main window.");
        QVERIFY2(HostManager::self()->addHost(otherHostname, QString(), QString(), QString()), "Could not create a second profile with no main window.");
        Host* host = HostManager::self()->getHost(hostname);
        Host* otherHost = HostManager::self()->getHost(otherHostname);
        QVERIFY2(host && otherHost, "The profiles are not in the pool.");

        const QString recorder = qsl(R"lua(
headlessStyleLog = {}
function onHeadlessAppStyleSheet(...)
  local parts = {}
  for i = 1, select("#", ...) do parts[#parts + 1] = tostring((select(i, ...))) end
  headlessStyleLog[#headlessStyleLog + 1] = "  event: " .. table.concat(parts, ", ") .. " (" .. select("#", ...) .. " args)"
end
)lua");
        for (Host* profile : {host, otherHost}) {
            QVERIFY2(profile->getLuaInterpreter()->compileAndExecuteScript(recorder), "The Lua chunk did not run.");
            // registerAnonymousEventHandler() is Lua from LuaGlobal, which these tests do not load
            profile->registerAnonymousEventHandler(qsl("sysAppStyleSheetChange"), qsl("onHeadlessAppStyleSheet"));
        }

        const QString calls = qsl(R"lua(
local function call(label, f, ...)
  local res = {pcall(f, ...)}
  local parts = {}
  for i = 1, table.maxn(res) do parts[#parts + 1] = tostring(res[i]) end
  headlessStyleLog[#headlessStyleLog + 1] = label .. " -> " .. table.concat(parts, ", ")
end
local sheet = "QToolTip { color: #102030; }"
call("setAppStyleSheet(sheet)", setAppStyleSheet, sheet)
call("setAppStyleSheet(sheet, tag)", setAppStyleSheet, sheet, "probeTag")
call("setAppStyleSheet(empty)", setAppStyleSheet, "")
call("setAppStyleSheet()", setAppStyleSheet)
call("setAppStyleSheet(number)", setAppStyleSheet, 5)
call("setAppStyleSheet(table)", setAppStyleSheet, {})
call("setAppStyleSheet(sheet, number)", setAppStyleSheet, sheet, 7)
call("setAppStyleSheet(sheet, nil)", setAppStyleSheet, sheet, nil)
call("setAppStyleSheet(bool)", setAppStyleSheet, true)
)lua");
        const QString styleSheetBefore = qApp->styleSheet();
        QVERIFY2(host->getLuaInterpreter()->compileAndExecuteScript(calls), "The Lua chunk did not run.");

        const QString expected = qsl(R"(  event: sysAppStyleSheetChange, , %1 (3 args)
setAppStyleSheet(sheet) -> true, true
  event: sysAppStyleSheetChange, probeTag, %1 (3 args)
setAppStyleSheet(sheet, tag) -> true, true
  event: sysAppStyleSheetChange, , %1 (3 args)
setAppStyleSheet(empty) -> true, true
setAppStyleSheet() -> false, setAppStyleSheet: bad argument #1 type (style sheet as string expected, got no value!)
  event: sysAppStyleSheetChange, , %1 (3 args)
setAppStyleSheet(number) -> true, true
setAppStyleSheet(table) -> false, setAppStyleSheet: bad argument #1 type (style sheet as string expected, got table!)
  event: sysAppStyleSheetChange, 7, %1 (3 args)
setAppStyleSheet(sheet, number) -> true, true
setAppStyleSheet(sheet, nil) -> false, setAppStyleSheet: bad argument #2 type (tag as string expected, got nil!)
setAppStyleSheet(bool) -> false, setAppStyleSheet: bad argument #1 type (style sheet as string expected, got boolean!))")
                                         .arg(hostname);
        for (Host* profile : {host, otherHost}) {
            QVERIFY2(profile->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(headlessStyleLogText = table.concat(headlessStyleLog, "\n"))lua")), "The Lua chunk did not run.");
        }
        const QString answered = luaGlobalString(host, "headlessStyleLogText");
        QCOMPARE(answered, expected);

        // Every profile hears of it, not only the one that set it, and each is told which one that was
        QStringList otherExpected;
        for (const QString& line : expected.split(QChar::LineFeed)) {
            if (line.startsWith(qsl("  event: "))) {
                otherExpected << line;
            }
        }
        const QString otherHeard = luaGlobalString(otherHost, "headlessStyleLogText");
        QCOMPARE(otherHeard, otherExpected.join(QChar::LineFeed));

        QVERIFY2(qApp->styleSheet() == styleSheetBefore, "The style sheet was applied with no main window to style.");
    }

    // The lines and trigger match are those a GUI probe of the same feeds gave
    void test_stalledMxpTagIsWrittenOutWithNoMainWindow()
    {
        QVERIFY2(!TAppFrontend::hasView(), "A main window exists, so this run is not headless.");

        const QString hostname = qsl("Test-Headless-Host-Mxp-Stall");
        QVERIFY2(HostManager::self()->addHost(hostname, QString(), QString(), QString()), "Could not create a profile with no main window.");
        Host* host = HostManager::self()->getHost(hostname);
        QVERIFY2(host, "The profile is not in the pool.");

        QVERIFY2(host->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(
headlessStallLog = {}
local function record(text) headlessStallLog[#headlessStallLog + 1] = text end
local mark = getLastLineNumber("main")
function headlessStallTail(label)
  record(label)
  for _, l in ipairs(getLines("main", mark, getLastLineNumber("main") + 1)) do record("  line: [" .. l .. "]") end
end
record("force -> " .. tostring(setConfig("specialForceMXPProcessorOn", true)))
headlessStallTrigger = tempRegexTrigger("stall", function()
  record("  trigger: [" .. line .. "]")
  headlessStallFired = "yes"
end)
feedTriggers("plain line\n")
feedTriggers("stall <send")
headlessStallTail("fed")
)lua")),
                 "The Lua chunk did not run.");

        // The watchdog gives a stalled tag two of its 1300ms timeouts before it writes it out
        QTest::qWaitFor(
                [host]() {
                    return luaGlobalString(host, "headlessStallFired") == qsl("yes");
                },
                8000);

        QVERIFY2(host->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(
headlessStallTail("after the watchdog")
killTrigger(headlessStallTrigger)
feedTriggers(">done\n")
headlessStallTail("after >done")
setConfig("specialForceMXPProcessorOn", false)
headlessStallLogText = table.concat(headlessStallLog, "\n")
)lua")),
                 "The Lua chunk did not run.");

        QCOMPARE(luaGlobalString(host, "headlessStallLogText"), qsl(R"(force -> true
fed
  line: [plain line]
  line: []
  trigger: [stall <send]
after the watchdog
  line: [plain line]
  line: [stall <send]
  line: []
after >done
  line: [plain line]
  line: [stall <send]
  line: [>done]
  line: [])"));
    }

    // As Media_spec.lua pins for the GUI: with closed captions on, an ended playback is captioned in the main console
    void test_closedCaptionsReachTheMainConsoleWithNoMainWindow()
    {
        QVERIFY2(!TAppFrontend::hasView(), "A main window exists, so this run is not headless.");

        const QString hostname = qsl("Test-Headless-Host-Captions");
        QVERIFY2(HostManager::self()->addHost(hostname, QString(), QString(), QString()), "Could not create a profile with no main window.");
        Host* host = HostManager::self()->getHost(hostname);
        QVERIFY2(host, "The profile is not in the pool.");

        const QString soundFile = qsl("%1/headless-caption.wav").arg(mConfigDir.path());
        QFile file(soundFile);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write(silentWav());
        file.close();

        // Stopped at once, so the backend need not be able to play it: a stop is captioned whether or not the sound had started
        const QString script = qsl(R"lua(
local ok, err = pcall(function()
  assert(setConfig("enableClosedCaption", true), "captions refused")
  assert(playSoundFile({name = [[%1]], key = "headlessCaption", caption = "headless caption"}), "playSoundFile refused")
  assert(stopSounds(), "stopSounds refused")
end)
headlessCaptions = ok and 'ok' or tostring(err)
)lua")
                                       .arg(soundFile);
        QVERIFY2(host->getLuaInterpreter()->compileAndExecuteScript(script), "The Lua chunk did not run.");
        QCOMPARE(luaGlobalString(host, "headlessCaptions"), qsl("ok"));
        QTRY_VERIFY2(mainBufferHolds(host, qsl("[headless caption stops]")), "The stopped sound was not captioned in the main console.");
    }

    // Nothing copies the bundled fonts out to the config directory here, as the executable does for the GUI
    void test_bundledFontsAreAvailableWithNoMainWindow()
    {
#ifndef INCLUDE_FONTS
        QSKIP("Built with WITH_FONTS=NO, so there are no bundled fonts to list");
#else
        QVERIFY2(!TAppFrontend::hasView(), "A main window exists, so this run is not headless.");

        const QString hostname = qsl("Test-Headless-Host-Fonts");
        QVERIFY2(HostManager::self()->addHost(hostname, QString(), QString(), QString()), "Could not create a profile with no main window.");
        Host* host = HostManager::self()->getHost(hostname);
        QVERIFY2(host, "The profile is not in the pool.");

        const bool ran = host->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(
local ok, err = pcall(function()
  local fonts = getAvailableFonts()
  assert(fonts["Bitstream Vera Sans Mono"], "the bundled default font is not listed")
  assert(fonts["Ubuntu Mono"], "the bundled Ubuntu Mono is not listed")
  local set, why = setConfig("mapSymbolFont", "Bitstream Vera Sans Mono")
  assert(set, "the bundled default was refused as the map symbol font: " .. tostring(why))
  assert(getConfig("mapSymbolFont") == "Bitstream Vera Sans Mono", "the map symbol font reads back as " .. tostring(getConfig("mapSymbolFont")))
end)
headlessFonts = ok and 'ok' or tostring(err)
)lua"));
        QVERIFY2(ran, "The Lua chunk did not run.");
        QCOMPARE(luaGlobalString(host, "headlessFonts"), qsl("ok"));

        // The families alone could be fonts this machine has installed itself
        FontManager* fonts = FontManager::self();
        QVERIFY2(fonts, "The profile pool has no font manager.");
        QVERIFY2(fonts->fontAlreadyLoaded(qsl("%1/Vera.ttf").arg(veraFontFolder()), QString()), "The font copied out to the fonts folder was not loaded from there.");
        QVERIFY2(!fonts->fontAlreadyLoaded(qsl(":/fonts/ttf-bitstream-vera-1.10/Vera.ttf"), QString()), "The font copied out to the fonts folder was loaded from the resources too.");
        QVERIFY2(fonts->fontAlreadyLoaded(qsl(":/fonts/ttf-bitstream-vera-1.10/VeraMono.ttf"), QString()), "A font missing from a copied-out bundle was not loaded from the resources.");
        QVERIFY2(fonts->fontAlreadyLoaded(qsl(":/fonts/ubuntu-font-family-0.83/UbuntuMono-R.ttf"), QString()), "A bundle never copied out was not loaded from the resources.");
#endif
    }

    // As the GUI answers the same files
    void test_imageSizeWithNoMainWindow()
    {
        QVERIFY2(!TAppFrontend::hasView(), "A main window exists, so this run is not headless.");

        const QString hostname = qsl("Test-Headless-Host-Image-Size");
        QVERIFY2(HostManager::self()->addHost(hostname, QString(), QString(), QString()), "Could not create a profile with no main window.");
        Host* host = HostManager::self()->getHost(hostname);
        QVERIFY2(host, "The profile is not in the pool.");

        QTemporaryDir images;
        QVERIFY(images.isValid());
        const auto writeFile = [&images](const QString& name, const QByteArray& content) {
            QFile file(images.filePath(name));
            return file.open(QIODevice::WriteOnly) && file.write(content) == content.size();
        };
        QVERIFY(QImage(37, 21, QImage::Format_ARGB32).save(images.filePath(qsl("raster.png"))));
        QVERIFY(writeFile(qsl("document.svg"), QByteArrayLiteral("<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"64\" height=\"48\"/>")));
        // QImage reads this by its content, which the SVG renderer cannot
        QVERIFY(QImage(5, 9, QImage::Format_ARGB32).save(images.filePath(qsl("raster.svg")), "PNG"));
        // Not named .svgz, so only its content says it is gzipped
        QVERIFY(writeFile(
                qsl("gzipped.svg"),
                QByteArrayLiteral(
                        "\x1f\x8b\x08\x00\x00\x00\x00\x00\x02\xff\xb3\x29\x2e\x4b\x57\xa8\xc8\xcd\xc9\x2b\xb6\x55\xca\x28\x29\x29\xb0\xd2\xd7\x2f\x2f\x2f\xd7\x2b\x37\xd6\xcb\x2f\x4a\xd7\x37\x32\x30"
                        "\x30\xd0\x07\xaa\x50\x52\x28\xcf\x4c\x29\xc9\xb0\x55\x32\x31\x50\x52\xc8\x48\xcd\x4c\xcf\x28\xb1\x55\x32\x36\x50\xd2\xb7\x03\x00\x54\x4e\x04\xa0\x40\x00\x00\x00")));

        const QString missing = images.filePath(qsl("missing.png"));
        // The refusal is outside the raw string: moc pairs up the apostrophes in one, and an odd count breaks it
        const QString script = qsl("imageDir = \"%1/\"\nmissingRefusal = \"couldn't retrieve image size, is the location '%2' correct?\"\n").arg(images.path(), missing) + qsl(R"lua(
headlessImageSize = 'not run'
local ok, err = pcall(function()
  local function check(label, wantA, wantB, ...)
    local n = select("#", ...)
    local a, b = ...
    assert(n == 2 and a == wantA and b == wantB, label .. " answered " .. n .. " values: " .. tostring(a) .. ", " .. tostring(b))
  end
  check("PNG", 37, 21, getImageSize(imageDir .. "raster.png"))
  check("SVG", 64, 48, getImageSize(imageDir .. "document.svg"))
  check("raster named .svg", 5, 9, getImageSize(imageDir .. "raster.svg"))
  check("gzipped SVG", 40, 30, getImageSize(imageDir .. "gzipped.svg"))
  check("missing file", nil, missingRefusal, getImageSize(imageDir .. "missing.png"))
  check("empty path", nil, "image location cannot be an empty string", getImageSize(""))
end)
headlessImageSize = ok and 'ok' or tostring(err)
)lua");
        const bool ran = host->getLuaInterpreter()->compileAndExecuteScript(script);

        QVERIFY2(ran, "The Lua chunk did not run.");
        QCOMPARE(luaGlobalString(host, "headlessImageSize"), qsl("ok"));

        // QImage reads SVG too where the qsvg plugin is deployed, which would hide a broken helper from getImageSize
        QVERIFY(TLabelModel::svgCandidate(images.filePath(qsl("document.svg"))));
        QVERIFY(TLabelModel::svgCandidate(images.filePath(qsl("gzipped.svg"))));
        QVERIFY(!TLabelModel::svgCandidate(images.filePath(qsl("raster.svg"))));
        QSvgRenderer renderer;
        QVERIFY(TLabelModel::loadSvg(renderer, images.filePath(qsl("gzipped.svg"))));
        QCOMPARE(renderer.defaultSize(), QSize(40, 30));
    }

    void test_profileWithNoSettingsStoreTakesTheDefaults()
    {
        if (MudletApp::getQSettings()) {
            QSKIP("A settings store exists with no main window, so there are no defaults for having none to check.");
        }

        const QString hostname = qsl("Test-Headless-Host-Defaults");
        QVERIFY2(HostManager::self()->addHost(hostname, QString(), QString(), QString()), "Could not create a profile with no settings store.");
        Host* host = HostManager::self()->getHost(hostname);
        QVERIFY2(host, "The profile is not in the pool.");
        QVERIFY(!host->mMapperCenterSmallAreas);
    }

    // As MapDownloadTest's checks of the same downloads in the GUI: the map is stored and loaded,
    // and scripts get sysMapDownloadEvent, whichever kind of map the game serves
    void test_mapDownloadLoadsTheMapWithNoMainWindow()
    {
        QVERIFY2(!TAppFrontend::hasView(), "A main window exists, so this run is not headless.");
        // Nothing renders map progress with no main window, so a warning about it would be a false alarm
        QTest::failOnWarning(QRegularExpression(qsl("no frontend is connected to show the map progress dialog")));

        const QString hostname = qsl("Test-Headless-Host-Map-Download");
        QVERIFY2(HostManager::self()->addHost(hostname, QString(), QString(), QString()), "Could not create a profile with no main window.");
        Host* host = HostManager::self()->getHost(hostname);
        QVERIFY2(host, "The profile is not in the pool.");
        TMap* map = host->mpMap.data();
        const QByteArray binaryMap = binaryMapOf(host, 7);
        QVERIFY(!binaryMap.isEmpty());

        QTcpServer server;
        QVERIFY2(server.listen(QHostAddress::LocalHost), "Could not start the local test server.");
        connect(&server, &QTcpServer::newConnection, this, [&]() {
            QTcpSocket* client = server.nextPendingConnection();
            // The request line names the map, and it comes first
            connect(
                    client,
                    &QTcpSocket::readyRead,
                    client,
                    [client, &binaryMap]() {
                        const QByteArray body = client->readAll().startsWith("GET /map.xml ") ? xmlMap() : binaryMap;
                        client->write("HTTP/1.1 200 OK\r\nContent-Length: " + QByteArray::number(body.size()) + "\r\nConnection: close\r\n\r\n" + body);
                        client->disconnectFromHost();
                    },
                    Qt::SingleShotConnection);
        });
        host->registerAnonymousEventHandler(qsl("sysMapDownloadEvent"), qsl("onHeadlessMapDownload"));
        QVERIFY(host->getLuaInterpreter()->compileAndExecuteScript(qsl("headlessMapDownloads = 0; function onHeadlessMapDownload() headlessMapDownloads = headlessMapDownloads + 1 end")));

        map->downloadMap(qsl("http://127.0.0.1:%1/map.xml").arg(server.serverPort()));
        QVERIFY2(map->hasActiveTransferProgress(), "The download did not start.");
        QTRY_VERIFY_WITH_TIMEOUT(!map->hasActiveTransferProgress(), 10000);
        QVERIFY2(map->mpRoomDB->getRoom(3), "The downloaded XML map was not loaded.");
        QCOMPARE(luaGlobalString(host, "headlessMapDownloads"), qsl("1"));

        map->downloadMap(qsl("http://127.0.0.1:%1/map.dat").arg(server.serverPort()));
        QVERIFY2(map->hasActiveTransferProgress(), "The second download did not start, so the first one never finished.");
        QTRY_VERIFY_WITH_TIMEOUT(!map->hasActiveTransferProgress(), 10000);
        QVERIFY2(!mainBufferHolds(host, qsl("failure in parsing")), "The downloaded binary map was reported as unreadable.");
        QVERIFY2(map->mpRoomDB->getRoom(7), "The downloaded binary map was not loaded.");
        QVERIFY2(!map->mpRoomDB->getRoom(3), "The binary map was loaded over the XML one rather than in its place.");
        QCOMPARE(luaGlobalString(host, "headlessMapDownloads"), qsl("2"));
        QVERIFY2(QApplication::topLevelWidgets().isEmpty(), "Downloading a map created a widget.");
    }

    // As Mapper_spec.lua's checks of saveMap and loadMap in the GUI
    void test_loadMapReadsTheMapWithNoMainWindow()
    {
        QVERIFY2(!TAppFrontend::hasView(), "A main window exists, so this run is not headless.");
        QTest::failOnWarning(QRegularExpression(qsl("no frontend is connected to show the map progress dialog")));

        const QString hostname = qsl("Test-Headless-Host-Map-Load");
        QVERIFY2(HostManager::self()->addHost(hostname, QString(), QString(), QString()), "Could not create a profile with no main window.");
        Host* host = HostManager::self()->getHost(hostname);
        QVERIFY2(host, "The profile is not in the pool.");
        QTemporaryDir mapDir;
        QVERIFY(mapDir.isValid());
        QFile xmlFile(mapDir.filePath(qsl("headless.xml")));
        QVERIFY(xmlFile.open(QIODevice::WriteOnly));
        xmlFile.write(xmlMap());
        xmlFile.close();
        QFile binaryFile(mapDir.filePath(qsl("headless.dat")));
        QVERIFY(binaryFile.open(QIODevice::WriteOnly));
        binaryFile.write(binaryMapOf(host, 7));
        binaryFile.close();

        const QString script = qsl("headlessMapDir = [[%1]]\n").arg(mapDir.path()) + qsl(R"lua(
headlessMapLoad = 'not run'
local ok, err = pcall(function()
  local loaded, loadedMsg = loadMap(headlessMapDir .. "/headless.dat")
  assert(loaded == true, "loadMap of a binary map answered " .. tostring(loaded) .. ", " .. tostring(loadedMsg))
  assert(roomExists(7), "loadMap did not load the binary map")
  local imported, importedMsg = loadMap(headlessMapDir .. "/headless.xml")
  assert(imported == true, "loadMap of an XML map answered " .. tostring(imported) .. ", " .. tostring(importedMsg))
  assert(roomExists(3) and not roomExists(7), "loadMap did not replace the map with the XML one")
  assert(loadMap(headlessMapDir .. "/nosuchmap.dat") == false, "loadMap of a missing binary map did not answer false")
  local missing, missingMsg = loadMap(headlessMapDir .. "/nosuchmap.xml")
  assert(missing == nil and missingMsg:find("was not found"), "loadMap of a missing XML map answered " .. tostring(missing) .. ", " .. tostring(missingMsg))
end)
headlessMapLoad = ok and 'ok' or tostring(err)
)lua");
        const bool ran = host->getLuaInterpreter()->compileAndExecuteScript(script);

        QVERIFY2(ran, "The Lua chunk did not run.");
        QCOMPARE(luaGlobalString(host, "headlessMapLoad"), qsl("ok"));
        QVERIFY2(QApplication::topLevelWidgets().isEmpty(), "Loading a map created a widget.");
    }
    // As the GUI answers once loadMap has made its mapper; with no main window the model is the open map
    void test_playerRoomAndJsonMapWithNoMainWindow()
    {
        QVERIFY2(!TAppFrontend::hasView(), "A main window exists, so this run is not headless.");
        QTest::failOnWarning(QRegularExpression(qsl("no frontend is connected to show the map progress dialog")));

        const QString hostname = qsl("Test-Headless-Host-Map-Lua");
        QVERIFY2(HostManager::self()->addHost(hostname, QString(), QString(), QString()), "Could not create a profile with no main window.");
        Host* host = HostManager::self()->getHost(hostname);
        QVERIFY2(host, "The profile is not in the pool.");
        QTemporaryDir mapDir;
        QVERIFY(mapDir.isValid());

        const QString script = qsl("headlessMapDir = [[%1]]\n").arg(mapDir.path()) + qsl(R"lua(
headlessMapLua = 'not run'
local ok, err = pcall(function()
  local function answers(...) return select("#", ...), ... end
  local count, first, second = answers(getPlayerRoom())
  assert(count == 2 and first == nil and second == "the player does not have a valid roomID set",
    "getPlayerRoom with no player room answered " .. tostring(first) .. ", " .. tostring(second))

  local area = addAreaName("Headless Lua Area")
  local kept = createRoomID()
  addRoom(kept); setRoomArea(kept, area)
  local moved = createRoomID()
  addRoom(moved); setRoomArea(moved, area)
  assert(saveMap(headlessMapDir .. "/rooms.dat") == true, "saveMap did not save the map")
  assert(loadMap(headlessMapDir .. "/rooms.dat") == true, "loadMap did not load the map")

  count, first, second = answers(centerview(moved))
  assert(count == 1 and first == true, "centerview answered " .. tostring(first) .. ", " .. tostring(second))
  count, first, second = answers(getPlayerRoom())
  assert(count == 1 and first == moved, "getPlayerRoom after centerview answered " .. tostring(first) .. ", " .. tostring(second))

  count, first, second = answers(saveJsonMap(headlessMapDir .. "/rooms.json"))
  assert(count == 1 and first == true, "saveJsonMap answered " .. tostring(first) .. ", " .. tostring(second))
  deleteRoom(kept)
  count, first, second = answers(loadJsonMap(headlessMapDir .. "/rooms.json"))
  assert(count == 1 and first == true, "loadJsonMap answered " .. tostring(first) .. ", " .. tostring(second))
  assert(roomExists(kept), "loadJsonMap did not read back the map saveJsonMap wrote")
  count, first, second = answers(loadJsonMap(headlessMapDir .. "/nosuchmap.json"))
  assert(count == 2 and first == nil and tostring(second):find("could not open file", 1, true),
    "loadJsonMap of a missing file answered " .. tostring(first) .. ", " .. tostring(second))
end)
headlessMapLua = ok and 'ok' or tostring(err)
)lua");
        const bool ran = host->getLuaInterpreter()->compileAndExecuteScript(script);

        QVERIFY2(ran, "The Lua chunk did not run.");
        QCOMPARE(luaGlobalString(host, "headlessMapLua"), qsl("ok"));
        QVERIFY2(QApplication::topLevelWidgets().isEmpty(), "The map functions created a widget.");
    }

    // As the GUI answers with its mapper up and no secondary map view open
    void test_mapperSettingsAndViewsWithNoMainWindow()
    {
        QVERIFY2(!TAppFrontend::hasView(), "A main window exists, so this run is not headless.");

        const QString hostname = qsl("Test-Headless-Host-Map-Config");
        QVERIFY2(HostManager::self()->addHost(hostname, QString(), QString(), QString()), "Could not create a profile with no main window.");
        Host* host = HostManager::self()->getHost(hostname);
        QVERIFY2(host, "The profile is not in the pool.");
        const int area = host->mpMap->mpRoomDB->addArea(qsl("Headless Config Area"));
        QVERIFY(host->mpMap->mpRoomDB->set2DMapZoom(area, 7.5));

        // The refusal is outside the raw string: moc pairs up the apostrophes in one, and an odd count breaks it
        const QString script = qsl("headlessArea = %1\nnotAnOption = \"'show3dMapView' isn't a valid configuration option\"\n").arg(area) + qsl(R"lua(
headlessMapConfig = 'not run'
local ok, err = pcall(function()
  local function check(label, want, ...)
    local got = {n = select("#", ...), ...}
    local same = got.n == want.n
    for i = 1, math.max(got.n, want.n) do
      same = same and got[i] == want[i]
    end
    assert(same, label .. " answered " .. got.n .. " values: " .. tostring(got[1]) .. ", " .. tostring(got[2]))
  end
  local function refused(message) return {n = 2, nil, message} end
  local function answered(value) return {n = 1, value} end

  local room = createRoomID()
  addRoom(room); setRoomArea(room, headlessArea)
  local ids = getMapViewIds()
  assert(type(ids) == "table" and next(ids) == nil, "getMapViewIds answered " .. tostring(ids))
  check("closeAllMapViews", answered(0), closeAllMapViews())
  local function counted(...) return select("#", ...), ... end
  local selectionCount, selection = counted(getMapSelection())
  assert(selectionCount == 1 and type(selection) == "table" and next(selection) == nil, "getMapSelection answered " .. selectionCount .. " values: " .. tostring(selection))
  check("clearMapSelection", answered(false), clearMapSelection())
  check("closeMapView", refused("view 99 not found"), closeMapView(99))
  check("getMapViewInfo", refused("view 99 not found"), getMapViewInfo(99))
  check("centerview in a view", refused("view 99 not found"), centerview(room, 99))
  check("getMapZoom in a view", refused("view 99 not found"), getMapZoom(headlessArea, 99))
  check("setMapZoom in a view", refused("view 99 not found"), setMapZoom(5, headlessArea, 99))
  check("createMapView", refused("no view manager available"), createMapView(headlessArea))

  check("getMapZoom of an area", answered(7.5), getMapZoom(headlessArea))
  check("getMapZoom of no such area", refused("number 9999 is not a valid areaID"), getMapZoom(9999))
  check("getMapZoom of the shown area", refused("no active mapper"), getMapZoom())
  check("setMapZoom of an area", answered(true), setMapZoom(12.25, headlessArea))
  check("getMapZoom after setMapZoom", answered(12.25), getMapZoom(headlessArea))
  check("setMapZoom too small", refused("zoom 2 is invalid, it must be at least 3"), setMapZoom(2, headlessArea))
  check("setMapZoom not finite", refused("zoom nan is invalid, it must be a finite number"), setMapZoom(0/0))
  check("setMapZoom of no such area", refused("number 9999 is not a valid areaID"), setMapZoom(5, 9999))
  check("setMapZoom of the shown area", refused("no active mapper"), setMapZoom(5))
  check("getMapZoom after refusals", answered(12.25), getMapZoom(headlessArea))
  check("setDefaultAreaVisible", answered(true), setDefaultAreaVisible(false))

  check("setConfig mapRoomSize", answered(true), setConfig("mapRoomSize", 6))
  check("getConfig mapRoomSize", answered(6), getConfig("mapRoomSize"))
  check("setConfig mapRoomSize 0", refused("mapRoomSize must be at least 1, got 0"), setConfig("mapRoomSize", 0))
  check("setConfig mapExitSize", answered(true), setConfig("mapExitSize", 3.5))
  check("getConfig mapExitSize", answered(3.5), getConfig("mapExitSize"))
  for _, key in ipairs({"mapRoundRooms", "showRoomIdsOnMap", "mapShowGrid"}) do
    check("setConfig " .. key, answered(true), setConfig(key, true))
    check("getConfig " .. key, answered(true), getConfig(key))
  end
  check("setConfig mapShowRoomBorders", answered(true), setConfig("mapShowRoomBorders", false))
  check("getConfig mapShowRoomBorders", answered(false), getConfig("mapShowRoomBorders"))
  check("setConfig showMapInfo", answered(true), setConfig("showMapInfo", "Headless"))
  check("setConfig mapInfoColor", answered(true), setConfig("mapInfoColor", {1, 2, 3}))
  local color = getConfig("mapInfoColor")
  assert(color[1] == 1 and color[2] == 2 and color[3] == 3 and color[4] == 255, "getConfig mapInfoColor did not read back the color set")
  check("setConfig mapInfoColor 'x'", refused("mapInfoColor requires a table {r, g, b} or {r, g, b, a}"), setConfig("mapInfoColor", "x"))
  check("getConfig showUpperLowerLevels default", answered(true), getConfig("showUpperLowerLevels"))
  check("setConfig showUpperLowerLevels", answered(true), setConfig("showUpperLowerLevels", false))
  check("getConfig showUpperLowerLevels", answered(false), getConfig("showUpperLowerLevels"))
  -- This drives the mapper widget, as in the GUI before its mapper exists
  check("setConfig show3dMapView", refused(notAnOption), setConfig("show3dMapView", false))
end)
headlessMapConfig = ok and 'ok' or tostring(err)
)lua");
        const bool ran = host->getLuaInterpreter()->compileAndExecuteScript(script);

        QVERIFY2(ran, "The Lua chunk did not run.");
        QCOMPARE(luaGlobalString(host, "headlessMapConfig"), qsl("ok"));
        QVERIFY2(!host->mpMap->getDefaultAreaShown(), "setDefaultAreaVisible(false) did not hide the default area.");
        QVERIFY2(host->mMapInfoContributors.contains(qsl("Headless")), "setConfig showMapInfo did not add the contributor.");
        QVERIFY2(QApplication::topLevelWidgets().isEmpty(), "The map functions created a widget.");
    }

    // An embedder closes a profile by deleting it from the pool, and quits by destroying the pool
    void test_exitEventIsRaisedOnceAsTheProfileGoes()
    {
        QTemporaryDir recordDir;
        QVERIFY(recordDir.isValid());
        const QString recordPath = qsl("%1/exits.txt").arg(recordDir.path());
        // The record has to outlive the profile, so it goes to a file, opened binary so Windows writes no \r. A
        // reset is refused under the handler, as in the GUI, since one reached while the handler pumps events would
        // close the Lua state it runs on.
        const QString handler = qsl(R"lua(
function onHeadlessExit(event)
  local file = io.open([[%1]], "ab")
  file:write(getProfileName(), " ", event, " reset:", tostring(resetProfile()), "\n")
  file:close()
end
)lua")
                                        .arg(recordPath);
        const auto recordExits = [&handler](Host* host) {
            host->registerAnonymousEventHandler(qsl("sysExitEvent"), qsl("onHeadlessExit"));
            return host->getLuaInterpreter()->compileAndExecuteScript(handler);
        };
        const auto recorded = [&recordPath]() {
            QFile file(recordPath);
            return file.open(QIODevice::ReadOnly) ? QString::fromUtf8(file.readAll()) : QString();
        };

        const QString deletedHostname = qsl("Test-Headless-Host-Exit-Deleted");
        QVERIFY2(HostManager::self()->addHost(deletedHostname, QString(), QString(), QString()), "Could not create a profile with no main window.");
        QVERIFY2(recordExits(HostManager::self()->getHost(deletedHostname)), "The Lua chunk did not run.");
        QVERIFY2(recorded().isEmpty(), "sysExitEvent was raised before the profile was closed.");
        HostManager::self()->deleteHost(deletedHostname);
        QCOMPARE(recorded(), qsl("%1 sysExitEvent reset:false\n").arg(deletedHostname));

        // A second pool, as this test's own has to last until cleanupTestCase()
        const QString quitHostname = qsl("Test-Headless-Host-Exit-Quit");
        auto pool = std::make_unique<HostManager>();
        QVERIFY2(pool->addHost(quitHostname, QString(), QString(), QString()), "Could not create a profile with no main window.");
        QVERIFY2(recordExits(pool->getHost(quitHostname)), "The Lua chunk did not run.");
        pool.reset();
        QCOMPARE(recorded(), qsl("%1 sysExitEvent reset:false\n%2 sysExitEvent reset:false\n").arg(deletedHostname, quitHostname));
    }

    void test_openProfileLoadsAndStartsTheSaveWithNoMainWindow()
    {
        QVERIFY2(!TAppFrontend::hasView(), "A main window exists, so this run is not headless.");

        const QString makerHostname = qsl("Test-Headless-Host-Open-Map-Maker");
        QVERIFY2(HostManager::self()->addHost(makerHostname, QString(), QString(), QString()), "Could not create a profile with no main window.");
        const QByteArray map = binaryMapOf(HostManager::self()->getHost(makerHostname), 9);
        HostManager::self()->deleteHost(makerHostname);

        const QString hostname = qsl("Test-Headless-Host-Open");
        const QString mapFolder = MudletApp::getMudletPath(enums::profileMapsPath, hostname);
        QVERIFY(QDir().mkpath(mapFolder));
        QFile mapFile(qsl("%1/headless.dat").arg(mapFolder));
        QVERIFY(mapFile.open(QIODevice::WriteOnly));
        mapFile.write(map);
        mapFile.close();

#ifdef INCLUDE_FONTS
        const QString fontPath = qsl("%1HeadlessMono.ttf").arg(MudletApp::getMudletPath(enums::profilePackagePath, hostname, qsl("HeadlessFontPack")));
        QVERIFY(QDir().mkpath(QFileInfo(fontPath).path()));
        QVERIFY(QFile::copy(qsl(":/fonts/ttf-bitstream-vera-1.10/VeraMono.ttf"), fontPath));
#endif

        const auto script = [](const QString& name, const QString& code) {
            return qsl(R"(<Script isActive="yes" isFolder="no"><name>%1</name><packageName></packageName><script>%2</script><eventHandlerList/></Script>)").arg(name, code);
        };
        // A Host element in a module would be read into the profile's settings too
        const auto package = [](const QString& hostPackage, const QString& scripts) {
            return qsl(R"(<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE MudletPackage>
<MudletPackage version="1.001">
%1<ScriptPackage>%2</ScriptPackage>
</MudletPackage>
)")
                    .arg(hostPackage, scripts);
        };
        QTemporaryDir moduleDir;
        QVERIFY(moduleDir.isValid());
        const QString modulePath = moduleDir.filePath(qsl("HeadlessModule.xml"));
        QFile moduleFile(modulePath);
        QVERIFY(moduleFile.open(QIODevice::WriteOnly));
        // The broken one would be reported on the console were the profile not marked as loading
        moduleFile.write(package(QString(), script(qsl("headlessModule"), qsl("headlessOpenModule = 'ran'")) + script(qsl("headlessBroken"), qsl("this is not lua"))).toUtf8());
        moduleFile.close();

        const QString saveFolder = MudletApp::getMudletPath(enums::profileXmlFilesPath, hostname);
        QVERIFY(QDir().mkpath(saveFolder));
        QFile saveFile(qsl("%1/2020-01-01#00-00-00.xml").arg(saveFolder));
        QVERIFY(saveFile.open(QIODevice::WriteOnly));
        const QString hostDetails = qsl("<HostPackage><Host><mInstalledPackages><string>HeadlessFontPack</string></mInstalledPackages>"
                                        "<mInstalledModules><key>HeadlessModule</key><filepath>%1</filepath><globalSave>0</globalSave><priority>-1</priority></mInstalledModules></Host></HostPackage>")
                                            .arg(modulePath);
        // table.size() comes from LuaGlobal, and a priority -1 module is in before the profile's scripts start
        const QString profileScript = qsl(R"lua(headlessOpenModuleSeen = headlessOpenModule
registerAnonymousEventHandler("sysLoadEvent", function(_, fresh)
  headlessOpenLoad = tostring(fresh) .. " rooms:" .. table.size(getRooms()) .. " room9:" .. tostring(roomExists(9))
end))lua");
        saveFile.write(package(hostDetails, script(qsl("headlessOpen"), profileScript)).toUtf8());
        saveFile.close();
        QFile autolog(MudletApp::getMudletPath(enums::profileDataItemPath, hostname, qsl("autolog")));
        QVERIFY(autolog.open(QIODevice::WriteOnly));
        autolog.close();
        // An IRE game's address, which only a new profile gets the IRE driver fix for
        QFile url(MudletApp::getMudletPath(enums::profileDataItemPath, hostname, qsl("url")));
        QVERIFY(url.open(QIODevice::WriteOnly));
        QDataStream urlStream(&url);
        urlStream.setVersion(QDataStream::Qt_5_12);
        urlStream << qsl("achaea.com");
        url.close();

        Host* host = HostManager::self()->openProfile(hostname, false);
        QVERIFY2(host, "The saved profile did not open.");
        QVERIFY(!host->hasConsoleView());
        QVERIFY2(host->mLoadedOk, qPrintable(host->mProfileLoadError));
        QCOMPARE(luaGlobalString(host, "headlessOpenModuleSeen"), qsl("ran"));
        QCOMPARE(luaGlobalString(host, "headlessOpenLoad"), qsl("true rooms:1 room9:true"));
        QVERIFY2(!host->mIsProfileLoadingSequence, "The profile is still marked as loading.");
        QVERIFY2(mainBufferHolds(host, qsl("loaded in offline mode")), "The offline load was not reported on the main console.");
        QVERIFY2(!mainBufferHolds(host, qsl("not working")), "Reinstalling the module as the profile opened reported its broken script.");
#ifdef INCLUDE_FONTS
        QVERIFY2(HostManager::self()->fonts().fontAlreadyLoaded(fontPath, hostname), "The installed package's font was not loaded.");
#endif
        QVERIFY2(host->mainConsoleModel().mLogToLogFile, "The profile's saved logging did not resume.");
        QVERIFY2(!host->mUSE_IRE_DRIVER_BUGFIX, "A profile with a save of its own was given the new profile settings.");

        QVERIFY2(!HostManager::self()->openProfile(hostname, false), "An open profile was opened again.");

        // Named for a listed game, so a profile with no save yet takes that game's address
        Host* fresh = HostManager::self()->openProfile(qsl("Achaea"), false);
        QVERIFY2(fresh, "A profile with no save did not open.");
        QVERIFY(fresh->mLoadedOk);
        QCOMPARE(fresh->getUrl(), qsl("achaea.com"));
        QVERIFY2(fresh->mUSE_IRE_DRIVER_BUGFIX, "A new profile for an IRE game did not get the IRE driver fix.");
        QVERIFY2(!fresh->mainConsoleModel().mLogToLogFile, "A profile that never logged started logging.");
        QCoreApplication::processEvents();
        QVERIFY2(QApplication::topLevelWidgets().isEmpty(), "Opening the profile created a widget.");
    }

    void test_deleteHostKeepsAProfileReopenedUnderItsExitHandler()
    {
        const QString hostname = qsl("Test-Headless-Host-Exit-Reopened");
        QVERIFY2(HostManager::self()->addHost(hostname, QString(), QString(), QString()), "Could not create a profile with no main window.");
        Host* host = HostManager::self()->getHost(hostname);
        // Stands in for an embedder's slot that runs while an exit handler pumps events
        lua_register(host->getLuaInterpreter()->getLuaGlobalState(), "reopenProfile", [](lua_State* L) -> int {
            const QString name = QString::fromUtf8(lua_tostring(L, 1));
            HostManager::self()->deleteHost(name);
            lua_pushboolean(L, HostManager::self()->addHost(name, QString(), QString(), QString()));
            return 1;
        });
        host->registerAnonymousEventHandler(qsl("sysExitEvent"), qsl("onReopenExit"));
        QVERIFY2(host->getLuaInterpreter()->compileAndExecuteScript(qsl("function onReopenExit() assert(reopenProfile(getProfileName())) end")), "The Lua chunk did not run.");

        HostManager::self()->deleteHost(hostname);
        QVERIFY2(HostManager::self()->getHost(hostname), "Closing the profile also removed the one reopened under its name.");
        HostManager::self()->deleteHost(hostname);
    }
};

#include "HeadlessHostSmokeTest.moc"
MUDLET_GROUPED_TEST_MAIN(HeadlessHostSmokeTest)
