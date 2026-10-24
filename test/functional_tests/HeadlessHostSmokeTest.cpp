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
#include <QWidget>
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

#include "EventLoopPump.h"
#include "FontManager.h"
#include "Host.h"
#include "HostManager.h"
#include "MudletApp.h"
#include "PortableModeTestHelper.h"
#include "TAppFrontend.h"
#include "TConsoleFrontend.h"
#include "TConsoleModel.h"
#include "TLabel.h"
#include "TLabelModel.h"
#include "TLuaInterpreter.h"
#include "TMap.h"
#include "TMxpFrameFrontend.h"
#include "TRoomDB.h"
#include "XMLimport.h"

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
  local badName, badNameMsg = setLabelCursor("stateLabel", "NoSuchCursor")
  assert(badName == nil and badNameMsg:find("cursor shape 'NoSuchCursor' not found", 1, true) == 1 and badNameMsg:find("wiki.mudlet.org/w/CursorShapes", 1, true), "an unknown cursor name was not refused")
  local noLabel, noLabelMsg = setLabelCursor("noSuchStateLabel", "NoSuchCursor")
  assert(noLabel == nil and noLabelMsg == "label name 'noSuchStateLabel' not found", "a missing label was not refused before an unknown cursor name")
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
  local miniSize, miniSizeMsg = getUserWindowSize("gridMini")
  assert(miniSize == nil and type(miniSizeMsg) == "string", "getUserWindowSize did not refuse the miniconsole with a message")
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
        // has no window for, where showWindow answers false rather than nil and a message
        const QStringList missingAnswers{qsl("echo=[2]nil:msg"),
                                         qsl("moveWindow=[2]nil:msg"),
                                         qsl("resizeWindow=[2]nil:msg"),
                                         qsl("showWindow=[1]false"),
                                         qsl("hideWindow=[2]nil:msg"),
                                         qsl("windowVisible=[2]nil:msg"),
                                         qsl("windowType=[2]nil:msg"),
                                         qsl("getWindowGeometry=[2]nil:msg"),
                                         qsl("getColumnCount=[2]nil:msg"),
                                         qsl("getRowCount=[2]nil:msg"),
                                         qsl("getFont=[2]nil:msg"),
                                         qsl("getFontSize=[2]nil:msg"),
                                         qsl("setFontSize=[2]nil:msg"),
                                         qsl("calcFontSize=[2]nil:msg"),
                                         qsl("clearWindow=[2]nil:msg"),
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

    // The answers are those of the same Lua run in the GUI self-test profile.
    void test_headlessProfileStyleSheetContract()
    {
        const QString hostname = qsl("Test-Headless-Host-Profile-Sheet");
        QVERIFY2(HostManager::self()->addHost(hostname, QString(), QString(), QString()), "Could not create a profile with no main window.");
        Host* host = HostManager::self()->getHost(hostname);
        QVERIFY2(host, "The profile is not in the pool.");
        QVERIFY(!host->hasConsoleView());

        const bool ran = host->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(
headlessProfileSheet = "not run"
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
  local answers = {}
  local function step(label, f, ...)
    answers[#answers + 1] = label .. "=" .. described(pcall(f, ...))
  end
  step("openA", openUserWindow, "psA", false, false, "f")
  step("openB", openUserWindow, "psB", true)
  step("ownB", setUserWindowStyleSheet, "psB", "color: blue;")
  step("sheetA", getUserWindowStyleSheet, "psA")
  step("sheetB", getUserWindowStyleSheet, "psB")
  step("set", setProfileStyleSheet, "QWidget { color: rgb(1,2,3); }")
  step("sheetA", getUserWindowStyleSheet, "psA")
  step("sheetB", getUserWindowStyleSheet, "psB")
  step("openC", openUserWindow, "psC", false, false, "f")
  step("sheetC", getUserWindowStyleSheet, "psC")
  step("ownA", setUserWindowStyleSheet, "psA", "color: green;")
  step("sheetA", getUserWindowStyleSheet, "psA")
  step("closeB", closeUserWindow, "psB")
  step("set", setProfileStyleSheet, "QWidget { color: red; }")
  step("sheetB", getUserWindowStyleSheet, "psB")
  step("reopenB", openUserWindow, "psB", true)
  step("sheetB", getUserWindowStyleSheet, "psB")
  step("sheetA", getUserWindowStyleSheet, "psA")
  step("mini", createMiniConsole, "psMini", 0, 0, 10, 10)
  step("sheetMini", getUserWindowStyleSheet, "psMini")
  step("same", setProfileStyleSheet, "QWidget { color: red; }")
  step("unclosed", setProfileStyleSheet, "QWidget { color: ")
  step("sheetA", getUserWindowStyleSheet, "psA")
  step("number", setProfileStyleSheet, 5)
  step("sheetA", getUserWindowStyleSheet, "psA")
  step("none", setProfileStyleSheet)
  step("empty", setProfileStyleSheet, "")
  step("sheetA", getUserWindowStyleSheet, "psA")
  step("sheetC", getUserWindowStyleSheet, "psC")
  headlessProfileSheetAnswers = table.concat(answers, "\n")
end)
headlessProfileSheet = ok and "ok" or tostring(err)
)lua"));

        QVERIFY2(ran, "The Lua chunk did not run.");
        QCOMPARE(luaGlobalString(host, "headlessProfileSheet"), qsl("ok"));
        // Every user window takes the profile sheet, a closed one too, until one sets its own; a mini console has none to read
        const QStringList answers{qsl("openA=[1]true"),
                                  qsl("openB=[1]true"),
                                  qsl("ownB=[1]true"),
                                  qsl("sheetA=[1]"),
                                  qsl("sheetB=[1]color: blue;"),
                                  qsl("set=[1]true"),
                                  qsl("sheetA=[1]QWidget { color: rgb(1,2,3); }"),
                                  qsl("sheetB=[1]QWidget { color: rgb(1,2,3); }"),
                                  qsl("openC=[1]true"),
                                  qsl("sheetC=[1]QWidget { color: rgb(1,2,3); }"),
                                  qsl("ownA=[1]true"),
                                  qsl("sheetA=[1]color: green;"),
                                  qsl("closeB=[1]true"),
                                  qsl("set=[1]true"),
                                  qsl("sheetB=[1]QWidget { color: red; }"),
                                  qsl("reopenB=[1]true"),
                                  qsl("sheetB=[1]QWidget { color: red; }"),
                                  qsl("sheetA=[1]QWidget { color: red; }"),
                                  qsl("mini=[1]true"),
                                  qsl("sheetMini=[2]nil:msg"),
                                  qsl("same=[1]true"),
                                  qsl("unclosed=[1]true"),
                                  qsl("sheetA=[1]QWidget { color: "),
                                  qsl("number=[1]true"),
                                  qsl("sheetA=[1]5"),
                                  qsl("none=raised"),
                                  qsl("empty=[1]true"),
                                  qsl("sheetA=[1]"),
                                  qsl("sheetC=[1]")};
        QCOMPARE(luaGlobalString(host, "headlessProfileSheetAnswers"), answers.join(QLatin1Char('\n')));
        QVERIFY2(QApplication::topLevelWidgets().isEmpty(), "Setting the profile sheet created a widget.");
    }

    // The answers are those of the same Lua run in the GUI self-test profile, with files of the same bytes.
    void test_headlessLabelMovieContract()
    {
        const QString hostname = qsl("Test-Headless-Host-Label-Movie");
        QVERIFY2(HostManager::self()->addHost(hostname, QString(), QString(), QString()), "Could not create a profile with no main window.");
        Host* host = HostManager::self()->getHost(hostname);
        QVERIFY2(host, "The profile is not in the pool.");
        QVERIFY(!host->hasConsoleView());

        QTemporaryDir movieDir;
        QVERIFY(movieDir.isValid());
        const auto writeFile = [&movieDir](const QString& name, const QByteArray& data) {
            QFile file(movieDir.filePath(name));
            return file.open(QIODevice::WriteOnly) && file.write(data) == data.size();
        };
        // Three 40x16 frames, then a 24x10 still image
        QVERIFY(writeFile(qsl("anim.gif"),
                          QByteArray::fromBase64("R0lGODlhKAAQAIEAAP8AAAAAAAAAAAAAACH/"
                                                 "C05FVFNDQVBFMi4wAwEAAAAh+"
                                                 "QQACgAAACwAAAAAKAAQAAAIKwABCBxIsKDBgwgTKlzIsKHDhxAjSpxIsaLFixgzatzIsaPHjyBDihypMCAAIfkEAQoAAQAsAAAAACgAEACBAP8AAAAAAAAAAAAACCsAAQgcSLCgwYMIEypcyLChw4"
                                                 "cQI0qcSLGixYsYM2rcyLGjx48gQ4ocqTAgACH5BAEKAAEALAAAAAAoABAAgQAA/wAAAAAAAAAAAAgrAAEIHEiwoMGDCBMqXMiwocOHECNKnEixosWLGDNq3Mixo8ePIEOKHKkwIAA7")));
        QVERIFY(writeFile(qsl("still.png"), QByteArray::fromBase64("iVBORw0KGgoAAAANSUhEUgAAABgAAAAKCAIAAAAhCwk/AAAAGUlEQVR4nGPk5ORkoAZgooopowaNGkQlAABsJgAvM+NphgAAAABJRU5ErkJggg==")));
        QVERIFY(writeFile(qsl("fake.gif"), QByteArrayLiteral("not an image")));
        QVERIFY(host->getLuaInterpreter()->compileAndExecuteScript(qsl("mvDir = [==[%1]==]").arg(movieDir.path())));

        const bool ran = host->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(
headlessMovie = "not run"
local ok, err = pcall(function()
  local function described(called, ...)
    if not called then
      return "raised"
    end
    local count = select("#", ...)
    local values = {}
    for i = 1, count do
      values[i] = tostring((select(i, ...)))
    end
    return "[" .. count .. "]" .. table.concat(values, ":")
  end
  local answers = {}
  local function step(label, f, ...)
    answers[#answers + 1] = label .. "=" .. described(pcall(f, ...))
  end
  local startGifs = getProfileStats().gifs
  local function state(label, name)
    local gifs = getProfileStats().gifs
    local width, height = getLabelSizeHint(name)
    answers[#answers + 1] = string.format("%s:hint=%sx%s,text=%s,gifs=%d/%d", label, tostring(width), tostring(height), tostring(getLabelText(name)), gifs.total - startGifs.total, gifs.active - startGifs.active)
  end
  local function textState(label, name, twin)
    local gifs = getProfileStats().gifs
    local width, height = getLabelSizeHint(name)
    local twinWidth, twinHeight = getLabelSizeHint(twin)
    local hint = (width == twinWidth and height == twinHeight) and twin or string.format("%sx%s", tostring(width), tostring(height))
    answers[#answers + 1] = string.format("%s:hint=%s,text=%s,gifs=%d/%d", label, hint, tostring(getLabelText(name)), gifs.total - startGifs.total, gifs.active - startGifs.active)
  end
  local gif, still, fake = mvDir .. "/anim.gif", mvDir .. "/still.png", mvDir .. "/fake.gif"
  assert(createLabel("mvA", 0, 0, 100, 20, 1) == true, "createLabel did not answer true")
  assert(createLabel("mvB", 0, 0, 100, 20, 1) == true, "createLabel did not answer true")
  step("echo", echo, "mvA", "words")
  for _, name in ipairs({"startMovie", "pauseMovie", "scaleMovie"}) do
    step(name, _G[name], "mvA")
  end
  step("setMovieFrame", setMovieFrame, "mvA", 1)
  step("setMovieSpeed", setMovieSpeed, "mvA", 50)
  step("startMissing", startMovie, "mvNoSuch")
  step("startEmpty", startMovie, "")
  step("startNumber", startMovie, 5)
  step("setMissing", setMovie, "mvNoSuch", gif)
  step("setEmpty", setMovie, "", gif)
  step("setNoFile", setMovie, "mvA", mvDir .. "/nosuch.gif")
  step("setNotImage", setMovie, "mvA", fake)
  step("setStill", setMovie, "mvA", still)
  state("still", "mvA")
  step("startStill", startMovie, "mvA")
  step("set", setMovie, "mvA", gif)
  state("set", "mvA")
  step("start", startMovie, "mvA")
  step("pause", pauseMovie, "mvA")
  state("paused", "mvA")
  step("frame0", setMovieFrame, "mvA", 0)
  step("frame2", setMovieFrame, "mvA", 2)
  step("frameNegative", setMovieFrame, "mvA", -1)
  step("framePast", setMovieFrame, "mvA", 3)
  step("frameText", setMovieFrame, "mvA", "x")
  step("speed", setMovieSpeed, "mvA", 200)
  step("speedText", setMovieSpeed, "mvA", "x")
  step("scale", scaleMovie, "mvA")
  state("scaled", "mvA")
  step("setScaled", setMovie, "mvA", gif)
  state("setScaled", "mvA")
  step("pauseScaled", pauseMovie, "mvA")
  step("frame1", setMovieFrame, "mvA", 1)
  state("scaledFrame", "mvA")
  step("resize", resizeWindow, "mvA", 200, 40)
  step("frame2", setMovieFrame, "mvA", 2)
  state("resizedFrame", "mvA")
  step("setFollowed", setMovie, "mvA", gif)
  state("followed", "mvA")
  step("unscale", scaleMovie, "mvA", false)
  step("scaleText", scaleMovie, "mvA", "x")
  step("resize", resizeWindow, "mvA", 50, 10)
  step("frame0", setMovieFrame, "mvA", 0)
  state("unscaledFrame", "mvA")
  step("start", startMovie, "mvA")
  step("setB", setMovie, "mvB", gif)
  state("two", "mvB")
  step("setAgain", setMovie, "mvA", gif)
  state("again", "mvA")
  step("setNotImageAfter", setMovie, "mvA", fake)
  step("startAfter", startMovie, "mvA")
  step("echoEmpty", echo, "mvA", "")
  state("echoEmpty", "mvA")
  step("startEchoEmpty", startMovie, "mvA")
  state("startEchoEmpty", "mvA")
  step("echoText", echo, "mvA", "again")
  assert(createLabel("mvText", 0, 0, 100, 20, 1) == true, "createLabel did not answer true")
  echo("mvText", "again")
  textState("echoText", "mvA", "mvText")
  deleteLabel("mvText")
  step("startEchoText", startMovie, "mvA")
  step("setText", setMovie, "mvA", gif)
  step("sheet", setLabelStyleSheet, "mvA", "padding: 3px; border: 2px solid red;")
  state("sheet", "mvA")
  assert(openUserWindow("mvWindow", false, false, "f") == true, "openUserWindow did not answer true")
  step("move", setWindow, "mvWindow", "mvA", 0, 0, true)
  step("startMoved", startMovie, "mvA")
  step("hide", hideWindow, "mvA")
  step("pauseHidden", pauseMovie, "mvA")
  state("hidden", "mvA")
  step("scaleHidden", scaleMovie, "mvA")
  step("resizeHidden", resizeWindow, "mvA", 120, 30)
  step("setHidden", setMovie, "mvA", gif)
  state("resizedHidden", "mvA")
  step("show", showWindow, "mvA")
  step("setShown", setMovie, "mvA", gif)
  state("resizedShown", "mvA")
  step("delete", deleteLabel, "mvA")
  step("startDeleted", startMovie, "mvA")
  state("deleted", "mvB")
  step("deleteB", deleteLabel, "mvB")
  local gifs = getProfileStats().gifs
  answers[#answers + 1] = string.format("gone:gifs=%d/%d", gifs.total - startGifs.total, gifs.active - startGifs.active)
  closeUserWindow("mvWindow")
  headlessMovieAnswers = table.concat(answers, "\n"):gsub(mvDir:gsub("%p", "%%%0"), "<dir>")
end)
headlessMovie = ok and "ok" or tostring(err)
)lua"));

        QVERIFY2(ran, "The Lua chunk did not run.");
        QCOMPARE(luaGlobalString(host, "headlessMovie"), qsl("ok"));
        // A text label drops the movie but keeps it for the next setMovie(); a movie's size hint is its frame's
        const QStringList answers{qsl("echo=[1]true"),
                                  qsl("startMovie=[2]nil:no movie found at label 'mvA'"),
                                  qsl("pauseMovie=[2]nil:no movie found at label 'mvA'"),
                                  qsl("scaleMovie=[2]nil:no movie found at label 'mvA'"),
                                  qsl("setMovieFrame=[2]nil:no movie found at label 'mvA'"),
                                  qsl("setMovieSpeed=[2]nil:no movie found at label 'mvA'"),
                                  qsl("startMissing=[2]nil:label \"mvNoSuch\" not found"),
                                  qsl("startEmpty=[2]nil:label name cannot be an empty string"),
                                  qsl("startNumber=[2]nil:label \"5\" not found"),
                                  qsl("setMissing=[2]nil:label 'mvNoSuch' does not exist"),
                                  qsl("setEmpty=[2]nil:label name cannot be an empty string"),
                                  qsl("setNoFile=[2]nil:no valid movie found at '<dir>/nosuch.gif'"),
                                  qsl("setNotImage=[2]nil:no valid movie found at '<dir>/fake.gif'"),
                                  qsl("setStill=[1]true"),
                                  qsl("still:hint=24x10,text=,gifs=1/1"),
                                  qsl("startStill=[1]true"),
                                  qsl("set=[1]true"),
                                  qsl("set:hint=40x16,text=,gifs=1/1"),
                                  qsl("start=[1]true"),
                                  qsl("pause=[1]true"),
                                  qsl("paused:hint=40x16,text=,gifs=1/0"),
                                  qsl("frame0=[1]true"),
                                  qsl("frame2=[1]true"),
                                  qsl("frameNegative=[1]false"),
                                  qsl("framePast=[1]true"),
                                  qsl("frameText=raised"),
                                  qsl("speed=[1]true"),
                                  qsl("speedText=raised"),
                                  qsl("scale=[1]true"),
                                  qsl("scaled:hint=40x16,text=,gifs=1/0"),
                                  qsl("setScaled=[1]true"),
                                  qsl("setScaled:hint=100x20,text=,gifs=1/1"),
                                  qsl("pauseScaled=[1]true"),
                                  qsl("frame1=[1]true"),
                                  qsl("scaledFrame:hint=100x20,text=,gifs=1/0"),
                                  qsl("resize=[1]true"),
                                  qsl("frame2=[1]true"),
                                  qsl("resizedFrame:hint=200x40,text=,gifs=1/0"),
                                  qsl("setFollowed=[1]true"),
                                  qsl("followed:hint=200x40,text=,gifs=1/1"),
                                  qsl("unscale=[1]true"),
                                  qsl("scaleText=raised"),
                                  qsl("resize=[1]true"),
                                  qsl("frame0=[1]true"),
                                  qsl("unscaledFrame:hint=200x40,text=,gifs=1/1"),
                                  qsl("start=[1]true"),
                                  qsl("setB=[1]true"),
                                  qsl("two:hint=40x16,text=,gifs=2/2"),
                                  qsl("setAgain=[1]true"),
                                  qsl("again:hint=200x40,text=,gifs=2/2"),
                                  qsl("setNotImageAfter=[2]nil:no valid movie found at '<dir>/fake.gif'"),
                                  qsl("startAfter=[1]true"),
                                  qsl("echoEmpty=[1]true"),
                                  qsl("echoEmpty:hint=200x40,text=,gifs=2/1"),
                                  qsl("startEchoEmpty=[1]true"),
                                  qsl("startEchoEmpty:hint=200x40,text=,gifs=2/2"),
                                  qsl("echoText=[1]true"),
                                  qsl("echoText:hint=mvText,text=again,gifs=2/1"),
                                  qsl("startEchoText=[2]nil:no movie found at label 'mvA'"),
                                  qsl("setText=[1]true"),
                                  qsl("sheet=[1]true"),
                                  qsl("sheet:hint=210x50,text=,gifs=2/2"),
                                  qsl("move=[1]true"),
                                  qsl("startMoved=[1]true"),
                                  qsl("hide=[1]true"),
                                  qsl("pauseHidden=[1]true"),
                                  qsl("hidden:hint=210x50,text=,gifs=2/1"),
                                  qsl("scaleHidden=[1]true"),
                                  qsl("resizeHidden=[1]true"),
                                  qsl("setHidden=[1]true"),
                                  qsl("resizedHidden:hint=60x20,text=,gifs=2/2"),
                                  qsl("show=[1]true"),
                                  qsl("setShown=[1]true"),
                                  qsl("resizedShown:hint=130x40,text=,gifs=2/2"),
                                  qsl("delete=[1]true"),
                                  qsl("startDeleted=[2]nil:label \"mvA\" not found"),
                                  qsl("deleted:hint=40x16,text=,gifs=1/1"),
                                  qsl("deleteB=[1]true"),
                                  qsl("gone:gifs=0/0")};
        QCOMPARE(luaGlobalString(host, "headlessMovieAnswers"), answers.join(QLatin1Char('\n')));
        QVERIFY2(QApplication::topLevelWidgets().isEmpty(), "Playing the movies created a widget.");
    }

    // The answers are those of the same Lua run in the GUI self-test profile, with files of the same bytes.
    void test_headlessBackgroundImageContract()
    {
        const QString hostname = qsl("Test-Headless-Host-Background-Image");
        QVERIFY2(HostManager::self()->addHost(hostname, QString(), QString(), QString()), "Could not create a profile with no main window.");
        Host* host = HostManager::self()->getHost(hostname);
        QVERIFY2(host, "The profile is not in the pool.");
        QVERIFY(!host->hasConsoleView());

        QTemporaryDir imageDir;
        QVERIFY(imageDir.isValid());
        const auto writeFile = [&imageDir](const QString& name, const QByteArray& data) {
            QFile file(imageDir.filePath(name));
            return file.open(QIODevice::WriteOnly) && file.write(data) == data.size();
        };
        // Three 40x16 frames, then a 24x10 still image
        QVERIFY(writeFile(qsl("anim.gif"),
                          QByteArray::fromBase64("R0lGODlhKAAQAIEAAP8AAAAAAAAAAAAAACH/"
                                                 "C05FVFNDQVBFMi4wAwEAAAAh+"
                                                 "QQACgAAACwAAAAAKAAQAAAIKwABCBxIsKDBgwgTKlzIsKHDhxAjSpxIsaLFixgzatzIsaPHjyBDihypMCAAIfkEAQoAAQAsAAAAACgAEACBAP8AAAAAAAAAAAAACCsAAQgcSLCgwYMIEypcyLChw4"
                                                 "cQI0qcSLGixYsYM2rcyLGjx48gQ4ocqTAgACH5BAEKAAEALAAAAAAoABAAgQAA/wAAAAAAAAAAAAgrAAEIHEiwoMGDCBMqXMiwocOHECNKnEixosWLGDNq3Mixo8ePIEOKHKkwIAA7")));
        QVERIFY(writeFile(qsl("still.png"), QByteArray::fromBase64("iVBORw0KGgoAAAANSUhEUgAAABgAAAAKCAIAAAAhCwk/AAAAGUlEQVR4nGPk5ORkoAZgooopowaNGkQlAABsJgAvM+NphgAAAABJRU5ErkJggg==")));
        QVERIFY(writeFile(qsl("fake.gif"), QByteArrayLiteral("not an image")));
        QVERIFY(writeFile(qsl("sized.svg"), QByteArrayLiteral("<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"30\" height=\"12\"><rect width=\"30\" height=\"12\" fill=\"red\"/></svg>\n")));
        QVERIFY(writeFile(qsl("nosize.svg"), QByteArrayLiteral("<svg xmlns=\"http://www.w3.org/2000/svg\"><rect width=\"5\" height=\"5\" fill=\"red\"/></svg>\n")));
        QVERIFY(writeFile(qsl("bad.svg"), QByteArrayLiteral("<notsvg>\n")));
        QVERIFY(writeFile(qsl("blank.svg"), QByteArrayLiteral("<svg xmlns=\"http://www.w3.org/2000/svg\"/>\n")));
        QVERIFY(host->getLuaInterpreter()->compileAndExecuteScript(qsl("bgDir = [==[%1]==]").arg(imageDir.path())));

        const bool ran = host->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(
headlessBackground = "not run"
local ok, err = pcall(function()
  local function described(called, ...)
    if not called then
      return "raised"
    end
    local count = select("#", ...)
    local values = {}
    for i = 1, count do
      values[i] = tostring((select(i, ...)))
    end
    return "[" .. count .. "]" .. table.concat(values, ":")
  end
  local answers = {}
  local function step(label, f, ...)
    answers[#answers + 1] = label .. "=" .. described(pcall(f, ...))
  end
  local startGifs = getProfileStats().gifs
  local sheet = ""
  local function state(label, name, emptied)
    local gifs = getProfileStats().gifs
    local width, height = getLabelSizeHint(name)
    local text = getLabelText(name)
    -- What a label measures for text, or for nothing, depends on the font, so that is read against a label with only the same text
    createLabel("bgTwin", 0, 0, 100, 20, 1)
    if sheet ~= "" then
      setLabelStyleSheet("bgTwin", sheet)
    end
    -- A label whose text was emptied measures a line, which a fresh one does not
    if emptied then
      echo("bgTwin", "words")
      echo("bgTwin", "")
    end
    if text ~= "" then
      echo("bgTwin", text)
    end
    local twinWidth, twinHeight = getLabelSizeHint("bgTwin")
    deleteLabel("bgTwin")
    local hint = (width == twinWidth and height == twinHeight) and "twin" or string.format("%sx%s", tostring(width), tostring(height))
    local movie = setMovieSpeed(name, 100) == true
    answers[#answers + 1] = string.format("%s:hint=%s,text=%s,movie=%s,gifs=%d/%d", label, hint, tostring(text), tostring(movie), gifs.total - startGifs.total, gifs.active - startGifs.active)
  end
  local gif, still, fake = bgDir .. "/anim.gif", bgDir .. "/still.png", bgDir .. "/fake.gif"
  local sized, nosize, bad = bgDir .. "/sized.svg", bgDir .. "/nosize.svg", bgDir .. "/bad.svg"
  local blank = bgDir .. "/blank.svg"
  assert(createLabel("bgA", 0, 0, 100, 20, 1) == true, "createLabel did not answer true")
  state("fresh", "bgA")
  step("still", setBackgroundImage, "bgA", still)
  state("still", "bgA")
  step("missing", setBackgroundImage, "bgA", bgDir .. "/nosuch.png")
  step("notImage", setBackgroundImage, "bgA", fake)
  step("badSvg", setBackgroundImage, "bgA", bad)
  state("kept", "bgA")
  step("empty", setBackgroundImage, "bgA", "")
  state("empty", "bgA")
  step("emptyAgain", setBackgroundImage, "bgA", "")
  step("echoText", echo, "bgA", "words")
  step("emptyOnText", setBackgroundImage, "bgA", "")
  state("emptyOnText", "bgA")
  step("mode3", setBackgroundImage, "bgA", still, 3)
  step("mode4", setBackgroundImage, "bgA", "background-color: red;", 4)
  step("mode5", setBackgroundImage, "bgA", still, 5)
  step("mode0", setBackgroundImage, "bgA", still, 0)
  step("full", setBackgroundImage, "bgA", still, 1, true)
  step("echoEmpty", echo, "bgA", "")
  state("echoEmpty", "bgA")
  step("echoText", echo, "bgA", "words")
  state("echoText", "bgA")
  step("onText", setBackgroundImage, "bgA", still)
  state("onText", "bgA")
  step("resetImage", resetBackgroundImage, "bgA")
  state("resetImage", "bgA")
  step("resetAgain", resetBackgroundImage, "bgA")
  step("echoText", echo, "bgA", "words")
  step("resetText", resetBackgroundImage, "bgA")
  state("resetText", "bgA")
  step("setMovie", setMovie, "bgA", gif)
  state("movie", "bgA")
  step("onMovie", setBackgroundImage, "bgA", still)
  state("onMovie", "bgA")
  step("startMovie", startMovie, "bgA")
  step("setMovieAgain", setMovie, "bgA", gif)
  state("movieAgain", "bgA")
  step("emptyOnMovie", setBackgroundImage, "bgA", "")
  state("emptyOnMovie", "bgA")
  step("setMovieAgain", setMovie, "bgA", gif)
  step("resetMovie", resetBackgroundImage, "bgA")
  state("resetMovie", "bgA")
  step("sized", setBackgroundImage, "bgA", sized)
  state("sized", "bgA")
  step("echoOnSvg", echo, "bgA", "svg text")
  state("echoOnSvg", "bgA")
  step("echoEmptyOnSvg", echo, "bgA", "")
  state("echoEmptyOnSvg", "bgA")
  step("rasterOnSvg", setBackgroundImage, "bgA", still)
  state("rasterOnSvg", "bgA")
  step("svgOnRaster", setBackgroundImage, "bgA", sized)
  state("svgOnRaster", "bgA")
  step("badOnSvg", setBackgroundImage, "bgA", bad)
  state("badOnSvg", "bgA")
  step("nosize", setBackgroundImage, "bgA", nosize)
  state("nosize", "bgA")
  step("sized", setBackgroundImage, "bgA", sized)
  step("movieOnSvg", setMovie, "bgA", gif)
  state("movieOnSvg", "bgA")
  step("svgOnMovie", setBackgroundImage, "bgA", nosize)
  state("svgOnMovie", "bgA")
  step("rasterOnSvgMovie", setBackgroundImage, "bgA", still)
  state("rasterOnSvgMovie", "bgA")
  step("movieOnRaster", setMovie, "bgA", gif)
  state("movieOnRaster", "bgA")
  step("svgOnRasterMovie", setBackgroundImage, "bgA", sized)
  state("svgOnRasterMovie", "bgA")
  step("resetSvgMovie", resetBackgroundImage, "bgA")
  state("resetSvgMovie", "bgA")
  step("sized", setBackgroundImage, "bgA", sized)
  step("resetSvg", resetBackgroundImage, "bgA")
  state("resetSvg", "bgA")
  step("sized", setBackgroundImage, "bgA", sized)
  step("emptyOnSvg", setBackgroundImage, "bgA", "")
  state("emptyOnSvg", "bgA")
  step("sized", setBackgroundImage, "bgA", sized)
  step("still", setBackgroundImage, "bgA", still)
  step("echoText", echo, "bgA", "words")
  step("echoEmpty", echo, "bgA", "")
  state("rasterDroppedSvg", "bgA", true)
  step("blank", setBackgroundImage, "bgA", blank)
  state("blank", "bgA", true)
  step("sized", setBackgroundImage, "bgA", sized)
  sheet = "padding: 3px; border: 2px solid red;"
  step("sheet", setLabelStyleSheet, "bgA", sheet)
  state("sheetSvg", "bgA")
  step("still", setBackgroundImage, "bgA", still)
  state("sheetStill", "bgA")
  step("echoText", echo, "bgA", "words")
  step("svgOnText", setBackgroundImage, "bgA", sized)
  state("svgOnText", "bgA")
  step("resetSvgText", resetBackgroundImage, "bgA")
  state("resetSvgText", "bgA")
  step("missingLabel", setBackgroundImage, "bgNoSuch", still)
  step("resetMissing", resetBackgroundImage, "bgNoSuch")
  step("emptyName", setBackgroundImage, "", still)
  step("number", setBackgroundImage, 5, still)
  step("noArgs", setBackgroundImage)
  step("main", setBackgroundImage, "main", still)
  step("mainMissing", setBackgroundImage, "main", bgDir .. "/nosuch.png")
  step("mainMode2", setBackgroundImage, "main", still, 2)
  step("mainMode4", setBackgroundImage, "main", "background-color: red;", 4)
  step("pathOnly", setBackgroundImage, still)
  step("pathMode", setBackgroundImage, still, 3)
  step("mainReset", resetBackgroundImage, "main")
  step("resetNoArgs", resetBackgroundImage)
  step("fullStill", setBackgroundImage, "main", still, 1, true)
  step("fullCover", setBackgroundImage, "main", still, 5, true)
  step("fullCoverMissing", setBackgroundImage, "main", bgDir .. "/nosuch.png", 5, true)
  step("fullCoverFake", setBackgroundImage, "main", fake, 5, true)
  step("fullMissing", setBackgroundImage, "main", bgDir .. "/nosuch.png", 1, true)
  step("fullReset", resetBackgroundImage, "main", true)
  step("fullResetName", resetBackgroundImage, "bgA", true)
  step("fullOther", setBackgroundImage, "bgA", still, 1, true)
  assert(createMiniConsole("bgMini", 0, 0, 100, 100) == true, "createMiniConsole did not answer true")
  step("mini", setBackgroundImage, "bgMini", still)
  step("miniMissing", setBackgroundImage, "bgMini", bgDir .. "/nosuch.png")
  step("miniMode5", setBackgroundImage, "bgMini", still, 5)
  step("miniReset", resetBackgroundImage, "bgMini")
  assert(openUserWindow("bgWindow", false, false, "f") == true, "openUserWindow did not answer true")
  step("window", setBackgroundImage, "bgWindow", still, 3)
  step("windowReset", resetBackgroundImage, "bgWindow")
  step("closeWindow", closeUserWindow, "bgWindow")
  step("closedWindow", setBackgroundImage, "bgWindow", still)
  step("closedWindowReset", resetBackgroundImage, "bgWindow")
  createBuffer("bgBuffer")
  step("buffer", setBackgroundImage, "bgBuffer", still)
  step("bufferReset", resetBackgroundImage, "bgBuffer")
  step("move", setWindow, "bgWindow", "bgA", 0, 0, true)
  step("movedLabel", setBackgroundImage, "bgA", still)
  state("moved", "bgA")
  step("delete", deleteLabel, "bgA")
  step("deleted", setBackgroundImage, "bgA", still)
  step("deletedReset", resetBackgroundImage, "bgA")
  step("deleteMini", deleteMiniConsole, "bgMini")
  step("deletedMini", setBackgroundImage, "bgMini", still)
  local gifs = getProfileStats().gifs
  answers[#answers + 1] = string.format("gone:gifs=%d/%d", gifs.total - startGifs.total, gifs.active - startGifs.active)
  resetBackgroundImage("main")
  resetBackgroundImage("main", true)
  headlessBackgroundAnswers = table.concat(answers, "\n"):gsub(bgDir:gsub("%p", "%%%0"), "<dir>")
end)
headlessBackground = ok and "ok" or tostring(err)
)lua"));

        QVERIFY2(ran, "The Lua chunk did not run.");
        QCOMPARE(luaGlobalString(host, "headlessBackground"), qsl("ok"));
        // A raster takes the place of a label's text or movie, an SVG paints under them; a console takes any path
        const QStringList answers{qsl("fresh:hint=twin,text=,movie=false,gifs=0/0"),
                                  qsl("still=[1]true"),
                                  qsl("still:hint=24x10,text=,movie=false,gifs=0/0"),
                                  qsl("missing=[2]nil:console or label 'bgA' not found, or '<dir>/nosuch.png' could not be loaded as an image"),
                                  qsl("notImage=[2]nil:console or label 'bgA' not found, or '<dir>/fake.gif' could not be loaded as an image"),
                                  qsl("badSvg=[2]nil:console or label 'bgA' not found, or '<dir>/bad.svg' could not be loaded as an image"),
                                  qsl("kept:hint=24x10,text=,movie=false,gifs=0/0"),
                                  qsl("empty=[1]true"),
                                  qsl("empty:hint=twin,text=,movie=false,gifs=0/0"),
                                  qsl("emptyAgain=[1]true"),
                                  qsl("echoText=[1]true"),
                                  qsl("emptyOnText=[1]true"),
                                  qsl("emptyOnText:hint=twin,text=,movie=false,gifs=0/0"),
                                  qsl("mode3=[1]true"),
                                  qsl("mode4=[2]nil:console or label 'bgA' not found, or 'background-color: red;' could not be loaded as an image"),
                                  qsl("mode5=[2]nil:mode 'cover' is not supported for the main display - pass true as the 4th argument to apply it to the full window background instead"),
                                  qsl("mode0=[2]nil:0 is not a valid mode! Valid modes are 1 'border', 2 'center', 3 'tile', 4 'style', 5 'cover'"),
                                  qsl("full=[2]nil:the full window background can only be used with the main console"),
                                  qsl("echoEmpty=[1]true"),
                                  qsl("echoEmpty:hint=24x10,text=,movie=false,gifs=0/0"),
                                  qsl("echoText=[1]true"),
                                  qsl("echoText:hint=twin,text=words,movie=false,gifs=0/0"),
                                  qsl("onText=[1]true"),
                                  qsl("onText:hint=24x10,text=,movie=false,gifs=0/0"),
                                  qsl("resetImage=[1]true"),
                                  qsl("resetImage:hint=twin,text=,movie=false,gifs=0/0"),
                                  qsl("resetAgain=[1]true"),
                                  qsl("echoText=[1]true"),
                                  qsl("resetText=[1]true"),
                                  qsl("resetText:hint=twin,text=words,movie=false,gifs=0/0"),
                                  qsl("setMovie=[1]true"),
                                  qsl("movie:hint=40x16,text=,movie=true,gifs=1/1"),
                                  qsl("onMovie=[1]true"),
                                  qsl("onMovie:hint=24x10,text=,movie=false,gifs=1/0"),
                                  qsl("startMovie=[2]nil:no movie found at label 'bgA'"),
                                  qsl("setMovieAgain=[1]true"),
                                  qsl("movieAgain:hint=40x16,text=,movie=true,gifs=1/1"),
                                  qsl("emptyOnMovie=[1]true"),
                                  qsl("emptyOnMovie:hint=twin,text=,movie=false,gifs=1/0"),
                                  qsl("setMovieAgain=[1]true"),
                                  qsl("resetMovie=[1]true"),
                                  qsl("resetMovie:hint=twin,text=,movie=false,gifs=1/0"),
                                  qsl("sized=[1]true"),
                                  qsl("sized:hint=30x12,text=,movie=false,gifs=1/0"),
                                  qsl("echoOnSvg=[1]true"),
                                  qsl("echoOnSvg:hint=twin,text=svg text,movie=false,gifs=1/0"),
                                  qsl("echoEmptyOnSvg=[1]true"),
                                  qsl("echoEmptyOnSvg:hint=30x12,text=,movie=false,gifs=1/0"),
                                  qsl("rasterOnSvg=[1]true"),
                                  qsl("rasterOnSvg:hint=24x10,text=,movie=false,gifs=1/0"),
                                  qsl("svgOnRaster=[1]true"),
                                  qsl("svgOnRaster:hint=30x12,text=,movie=false,gifs=1/0"),
                                  qsl("badOnSvg=[2]nil:console or label 'bgA' not found, or '<dir>/bad.svg' could not be loaded as an image"),
                                  qsl("badOnSvg:hint=30x12,text=,movie=false,gifs=1/0"),
                                  qsl("nosize=[1]true"),
                                  qsl("nosize:hint=5x5,text=,movie=false,gifs=1/0"),
                                  qsl("sized=[1]true"),
                                  qsl("movieOnSvg=[1]true"),
                                  qsl("movieOnSvg:hint=40x16,text=,movie=true,gifs=1/1"),
                                  qsl("svgOnMovie=[1]true"),
                                  qsl("svgOnMovie:hint=40x16,text=,movie=true,gifs=1/1"),
                                  qsl("rasterOnSvgMovie=[1]true"),
                                  qsl("rasterOnSvgMovie:hint=24x10,text=,movie=false,gifs=1/0"),
                                  qsl("movieOnRaster=[1]true"),
                                  qsl("movieOnRaster:hint=40x16,text=,movie=true,gifs=1/1"),
                                  qsl("svgOnRasterMovie=[1]true"),
                                  qsl("svgOnRasterMovie:hint=40x16,text=,movie=true,gifs=1/1"),
                                  qsl("resetSvgMovie=[1]true"),
                                  qsl("resetSvgMovie:hint=twin,text=,movie=false,gifs=1/0"),
                                  qsl("sized=[1]true"),
                                  qsl("resetSvg=[1]true"),
                                  qsl("resetSvg:hint=twin,text=,movie=false,gifs=1/0"),
                                  qsl("sized=[1]true"),
                                  qsl("emptyOnSvg=[1]true"),
                                  qsl("emptyOnSvg:hint=twin,text=,movie=false,gifs=1/0"),
                                  qsl("sized=[1]true"),
                                  qsl("still=[1]true"),
                                  qsl("echoText=[1]true"),
                                  qsl("echoEmpty=[1]true"),
                                  qsl("rasterDroppedSvg:hint=twin,text=,movie=false,gifs=1/0"),
                                  qsl("blank=[1]true"),
                                  qsl("blank:hint=twin,text=,movie=false,gifs=1/0"),
                                  qsl("sized=[1]true"),
                                  qsl("sheet=[1]true"),
                                  qsl("sheetSvg:hint=40x22,text=,movie=false,gifs=1/0"),
                                  qsl("still=[1]true"),
                                  qsl("sheetStill:hint=34x20,text=,movie=false,gifs=1/0"),
                                  qsl("echoText=[1]true"),
                                  qsl("svgOnText=[1]true"),
                                  qsl("svgOnText:hint=twin,text=words,movie=false,gifs=1/0"),
                                  qsl("resetSvgText=[1]true"),
                                  qsl("resetSvgText:hint=twin,text=words,movie=false,gifs=1/0"),
                                  qsl("missingLabel=[2]nil:console or label 'bgNoSuch' not found, or '<dir>/still.png' could not be loaded as an image"),
                                  qsl("resetMissing=[2]nil:console 'bgNoSuch' not found"),
                                  qsl("emptyName=[1]true"),
                                  qsl("number=[2]nil:console or label '5' not found, or '<dir>/still.png' could not be loaded as an image"),
                                  qsl("noArgs=raised"),
                                  qsl("main=[1]true"),
                                  qsl("mainMissing=[1]true"),
                                  qsl("mainMode2=[1]true"),
                                  qsl("mainMode4=[1]true"),
                                  qsl("pathOnly=[1]true"),
                                  qsl("pathMode=[1]true"),
                                  qsl("mainReset=[1]true"),
                                  qsl("resetNoArgs=[1]true"),
                                  qsl("fullStill=[1]true"),
                                  qsl("fullCover=[1]true"),
                                  qsl("fullCoverMissing=[2]nil:could not use '<dir>/nosuch.png' as a full window background image"),
                                  qsl("fullCoverFake=[2]nil:could not use '<dir>/fake.gif' as a full window background image"),
                                  qsl("fullMissing=[1]true"),
                                  qsl("fullReset=[1]true"),
                                  qsl("fullResetName=[2]nil:the full window background can only be reset on the main console"),
                                  qsl("fullOther=[2]nil:the full window background can only be used with the main console"),
                                  qsl("mini=[1]true"),
                                  qsl("miniMissing=[1]true"),
                                  qsl("miniMode5=[2]nil:mode 'cover' is not supported for the main display - pass true as the 4th argument to apply it to the full window background instead"),
                                  qsl("miniReset=[1]true"),
                                  qsl("window=[1]true"),
                                  qsl("windowReset=[1]true"),
                                  qsl("closeWindow=[1]true"),
                                  qsl("closedWindow=[1]true"),
                                  qsl("closedWindowReset=[1]true"),
                                  qsl("buffer=[1]true"),
                                  qsl("bufferReset=[1]true"),
                                  qsl("move=[1]true"),
                                  qsl("movedLabel=[1]true"),
                                  qsl("moved:hint=34x20,text=,movie=false,gifs=1/0"),
                                  qsl("delete=[1]true"),
                                  qsl("deleted=[2]nil:console or label 'bgA' not found, or '<dir>/still.png' could not be loaded as an image"),
                                  qsl("deletedReset=[2]nil:console 'bgA' not found"),
                                  qsl("deleteMini=[1]true"),
                                  qsl("deletedMini=[2]nil:console or label 'bgMini' not found, or '<dir>/still.png' could not be loaded as an image"),
                                  qsl("gone:gifs=0/0")};
        QCOMPARE(luaGlobalString(host, "headlessBackgroundAnswers"), answers.join(QLatin1Char('\n')));
        QVERIFY2(QApplication::topLevelWidgets().isEmpty(), "Setting the background images created a widget.");
    }

    // The answers are those GeyserLabel_spec.lua gets in the GUI self-test profile
    void test_headlessResetTakesStyleSheetImage()
    {
        const QString hostname = qsl("Test-Headless-Host-Sheet-Image");
        QVERIFY2(HostManager::self()->addHost(hostname, QString(), QString(), QString()), "Could not create a profile with no main window.");
        Host* host = HostManager::self()->getHost(hostname);
        QVERIFY2(host, "The profile is not in the pool.");
        QVERIFY(!host->hasConsoleView());

        const bool ran = host->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(
local answers = {}
createLabel("sheetImage", 0, 0, 40, 20, 1)
for _, sheet in ipairs({
  "border: 1px solid red; background-image: url(data:image/png;base64,AAAA); color: blue;",
  "QLabel{color: red;} QLabel::hover{background-image: url(hover.png);}",
  "Background-Image: url(:/icons/mudlet.png); color: blue;",
}) do
  setLabelStyleSheet("sheetImage", sheet)
  answers[#answers + 1] = tostring(resetBackgroundImage("sheetImage")) .. ":" .. getLabelStyleSheet("sheetImage")
end
headlessSheetImageAnswers = table.concat(answers, "\n")
)lua"));

        QVERIFY2(ran, "The Lua chunk did not run.");
        const QStringList answers{qsl("true:border: 1px solid red;  color: blue;"), qsl("true:QLabel{color: red;} QLabel::hover{background-image: url(hover.png);}"), qsl("true: color: blue;")};
        QCOMPARE(luaGlobalString(host, "headlessSheetImageAnswers"), answers.join(QLatin1Char('\n')));
    }

    // The answers are those of the same Lua run in the GUI self-test profile
    void test_headlessRaiseLowerContract()
    {
        const QString hostname = qsl("Test-Headless-Host-Raise-Lower");
        QVERIFY2(HostManager::self()->addHost(hostname, QString(), QString(), QString()), "Could not create a profile with no main window.");
        Host* host = HostManager::self()->getHost(hostname);
        QVERIFY2(host, "The profile is not in the pool.");
        QVERIFY(!host->hasConsoleView());

        const bool ran = host->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(
headlessRaise = "not run"
local ok, err = pcall(function()
  local function described(called, ...)
    if not called then
      return "raised:" .. tostring((...))
    end
    local count = select("#", ...)
    local values = {}
    for i = 1, count do
      values[i] = tostring((select(i, ...)))
    end
    return "[" .. count .. "]" .. table.concat(values, ":")
  end
  local answers = {}
  local function step(label, f, ...)
    answers[#answers + 1] = label .. "=" .. described(pcall(f, ...))
  end
  local function both(label, name)
    step(label .. "Raise", raiseWindow, name)
    step(label .. "Lower", lowerWindow, name)
  end
  assert(createLabel("rlLabel", 0, 0, 50, 20, 1) == true, "createLabel did not answer true")
  assert(createMiniConsole("rlMini", 0, 0, 100, 100) == true, "createMiniConsole did not answer true")
  assert(openUserWindow("rlWindow", false, false, "f") == true, "openUserWindow did not answer true")
  createBuffer("rlBuffer")
  assert(createLabel("rlWindow", "rlInner", 0, 0, 50, 20, 1) == true, "createLabel in a user window did not answer true")
  assert(createMiniConsole("rlWindow", "rlInnerMini", 0, 0, 50, 50) == true, "createMiniConsole in a user window did not answer true")
  both("label", "rlLabel")
  both("mini", "rlMini")
  both("window", "rlWindow")
  both("buffer", "rlBuffer")
  both("innerLabel", "rlInner")
  both("innerMini", "rlInnerMini")
  both("main", "main")
  both("empty", "")
  both("missing", "rlNoSuch")
  both("case", "RLLABEL")
  both("mapper", "mapper")
  both("number", 5)
  step("nilRaise", raiseWindow, nil)
  step("noArgsRaise", raiseWindow)
  step("noArgsLower", lowerWindow)
  step("tableRaise", raiseWindow, {})
  step("tableLower", lowerWindow, {})
  step("hide", hideWindow, "rlLabel")
  both("hiddenLabel", "rlLabel")
  step("hideMini", hideWindow, "rlMini")
  both("hiddenMini", "rlMini")
  step("closeWindow", closeUserWindow, "rlWindow")
  both("closedWindow", "rlWindow")
  both("closedInner", "rlInner")
  step("deleteLabel", deleteLabel, "rlLabel")
  both("deletedLabel", "rlLabel")
  step("deleteMini", deleteMiniConsole, "rlMini")
  both("deletedMini", "rlMini")
  step("deleteInner", deleteLabel, "rlInner")
  step("deleteInnerMini", deleteMiniConsole, "rlInnerMini")
  headlessRaiseAnswers = table.concat(answers, "\n")
end)
headlessRaise = ok and "ok" or tostring(err)
)lua"));
        QVERIFY2(ran, "The Lua chunk did not run.");
        QCOMPARE(luaGlobalString(host, "headlessRaise"), qsl("ok"));
        const QStringList answers{qsl("labelRaise=[1]true"),
                                  qsl("labelLower=[1]true"),
                                  qsl("miniRaise=[1]true"),
                                  qsl("miniLower=[1]true"),
                                  qsl("windowRaise=[1]true"),
                                  qsl("windowLower=[1]true"),
                                  qsl("bufferRaise=[1]true"),
                                  qsl("bufferLower=[1]true"),
                                  qsl("innerLabelRaise=[1]true"),
                                  qsl("innerLabelLower=[1]true"),
                                  qsl("innerMiniRaise=[1]true"),
                                  qsl("innerMiniLower=[1]true"),
                                  qsl("mainRaise=[1]false"),
                                  qsl("mainLower=[1]false"),
                                  qsl("emptyRaise=[1]false"),
                                  qsl("emptyLower=[1]false"),
                                  qsl("missingRaise=[1]false"),
                                  qsl("missingLower=[1]false"),
                                  qsl("caseRaise=[1]false"),
                                  qsl("caseLower=[1]false"),
                                  qsl("mapperRaise=[1]false"),
                                  qsl("mapperLower=[1]false"),
                                  qsl("numberRaise=[1]false"),
                                  qsl("numberLower=[1]false"),
                                  qsl("nilRaise=[1]false"),
                                  qsl("noArgsRaise=[1]false"),
                                  qsl("noArgsLower=[1]false"),
                                  qsl("tableRaise=raised:raiseWindow: bad argument #1 type (window name as string expected, got table)!"),
                                  qsl("tableLower=raised:lowerWindow: bad argument #1 type (window name as string expected, got table)!"),
                                  qsl("hide=[1]true"),
                                  qsl("hiddenLabelRaise=[1]true"),
                                  qsl("hiddenLabelLower=[1]true"),
                                  qsl("hideMini=[1]true"),
                                  qsl("hiddenMiniRaise=[1]true"),
                                  qsl("hiddenMiniLower=[1]true"),
                                  qsl("closeWindow=[1]true"),
                                  qsl("closedWindowRaise=[1]true"),
                                  qsl("closedWindowLower=[1]true"),
                                  qsl("closedInnerRaise=[1]true"),
                                  qsl("closedInnerLower=[1]true"),
                                  qsl("deleteLabel=[1]true"),
                                  qsl("deletedLabelRaise=[1]false"),
                                  qsl("deletedLabelLower=[1]false"),
                                  qsl("deleteMini=[1]true"),
                                  qsl("deletedMiniRaise=[1]false"),
                                  qsl("deletedMiniLower=[1]false"),
                                  qsl("deleteInner=[1]true"),
                                  qsl("deleteInnerMini=[1]true")};
        QCOMPARE(luaGlobalString(host, "headlessRaiseAnswers"), answers.join(QLatin1Char('\n')));
        QVERIFY2(QApplication::topLevelWidgets().isEmpty(), "Raising and lowering the windows created a widget.");
    }

    // The answers are those of the same Lua run in the GUI self-test profile, but for the last ones:
    // a spec cannot wait for deferred deletes, which take what a real deleted scroll box held out of
    // the registry as TLabel's and TConsole's destructors and TMainConsole::deregisterScrollBox() run
    void test_headlessScrollBoxContract()
    {
        const QString hostname = qsl("Test-Headless-Host-Scroll-Box");
        QVERIFY2(HostManager::self()->addHost(hostname, QString(), QString(), QString()), "Could not create a profile with no main window.");
        Host* host = HostManager::self()->getHost(hostname);
        QVERIFY2(host, "The profile is not in the pool.");
        QVERIFY(!host->hasConsoleView());

        bool ran = host->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(
headlessScroll = "not run"
local ok, err = pcall(function()
  local function described(called, ...)
    if not called then
      return "raised:" .. tostring((...))
    end
    local count = select("#", ...)
    local values = {}
    for i = 1, count do
      values[i] = tostring((select(i, ...)))
    end
    return "[" .. count .. "]" .. table.concat(values, ":")
  end
  local answers = {}
  local function step(label, f, ...)
    answers[#answers + 1] = label .. "=" .. described(pcall(f, ...))
  end
  local function look(label, name)
    answers[#answers + 1] = label .. ":type=" .. described(pcall(windowType, name)) .. ",geometry=" .. described(pcall(getWindowGeometry, name)) .. ",visible=" .. described(pcall(windowVisible, name))
  end
  -- A label measures its text in the font it inherits, which is read against labels in the main window and in a user window
  local function font(label, name)
    local width, height = getLabelSizeHint(name)
    local kinds = {}
    for _, parent in ipairs({"main", "sbWindow"}) do
      createLabel(parent, "sbTwin", 0, 0, 50, 20, 1)
      echo("sbTwin", getLabelText(name))
      local twinWidth, twinHeight = getLabelSizeHint("sbTwin")
      deleteLabel("sbTwin")
      if width == twinWidth and height == twinHeight then
        kinds[#kinds + 1] = parent
      end
    end
    answers[#answers + 1] = label .. ":font=" .. (#kinds > 0 and table.concat(kinds, "+") or "other")
  end
  local deletedEvents = {}
  -- By name, as the C++ registerAnonymousEventHandler() takes no function
  function headlessScrollDeleted(_, name)
    deletedEvents[#deletedEvents + 1] = name
  end
  registerAnonymousEventHandler("sysScrollBoxDeleted", "headlessScrollDeleted")
  local function events(label)
    answers[#answers + 1] = label .. ":events=" .. table.concat(deletedEvents, ",")
    deletedEvents = {}
  end
  assert(openUserWindow("sbWindow", false, false, "f") == true, "openUserWindow did not answer true")
  setFont("sbWindow", "Bitstream Vera Sans Mono")
  setFontSize("sbWindow", 20)
  step("create", createScrollBox, "sbBox", 10, 20, 200, 100)
  look("box", "sbBox")
  step("createAgain", createScrollBox, "sbBox", 15, 25, 210, 110)
  look("boxAgain", "sbBox")
  step("createMain", createScrollBox, "main", "sbMainBox", 1, 2, 30, 40)
  look("mainBox", "sbMainBox")
  step("createInWindow", createScrollBox, "sbWindow", "sbWinBox", 3, 4, 150, 80)
  look("winBox", "sbWinBox")
  step("createInBox", createScrollBox, "sbBox", "sbInnerBox", 5, 6, 60, 50)
  look("innerBox", "sbInnerBox")
  step("createMissingParent", createScrollBox, "sbNoSuch", "sbOrphan", 0, 0, 10, 10)
  look("orphan", "sbOrphan")
  step("createNoName", createScrollBox)
  step("createBadX", createScrollBox, "sbBad", "x", 0, 10, 10)
  step("createLabel", createLabel, "sbBox", "sbLabel", 7, 8, 40, 20, 1)
  look("label", "sbLabel")
  step("echo", echo, "sbLabel", "words")
  font("label", "sbLabel")
  step("createWinBoxLabel", createLabel, "sbWinBox", "sbWinBoxLabel", 1, 1, 40, 20, 1)
  step("echoWinBoxLabel", echo, "sbWinBoxLabel", "words")
  font("winBoxLabel", "sbWinBoxLabel")
  step("createInnerLabel", createLabel, "sbInnerBox", "sbInnerLabel", 2, 2, 20, 10, 1)
  look("innerLabel", "sbInnerLabel")
  step("createMini", createMiniConsole, "sbBox", "sbMini", 9, 10, 50, 40)
  look("mini", "sbMini")
  step("miniFontSize", getFontSize, "sbMini")
  step("createWinBoxMini", createMiniConsole, "sbWinBox", "sbWinBoxMini", 0, 0, 50, 40)
  step("winBoxMiniFontSize", getFontSize, "sbWinBoxMini")
  step("move", moveWindow, "sbBox", 11, 12)
  step("resize", resizeWindow, "sbBox", 120, 90)
  look("moved", "sbBox")
  step("raise", raiseWindow, "sbBox")
  step("lower", lowerWindow, "sbBox")
  step("hide", hideWindow, "sbBox")
  look("hidden", "sbBox")
  look("hiddenLabel", "sbLabel")
  look("hiddenMini", "sbMini")
  look("hiddenInnerBox", "sbInnerBox")
  look("hiddenInnerLabel", "sbInnerLabel")
  step("showLabelInHidden", showWindow, "sbLabel")
  look("labelInHidden", "sbLabel")
  step("show", showWindow, "sbBox")
  look("shown", "sbBox")
  look("shownLabel", "sbLabel")
  look("shownInnerLabel", "sbInnerLabel")
  step("hideWindow", hideWindow, "sbWindow")
  look("winBoxInHidden", "sbWinBox")
  look("winBoxLabelInHidden", "sbWinBoxLabel")
  step("showWindow", showWindow, "sbWindow")
  look("winBoxShown", "sbWinBox")
  -- A scroll box may share a user window's name: a label goes into the user window, a mini console into the box
  step("createClash", createScrollBox, "main", "sbWindow", 30, 30, 20, 20)
  step("createClashLabel", createLabel, "sbWindow", "sbClashLabel", 0, 0, 40, 20, 1)
  step("echoClashLabel", echo, "sbClashLabel", "words")
  font("clashLabel", "sbClashLabel")
  step("createClashMini", createMiniConsole, "sbWindow", "sbClashMini", 0, 0, 40, 20)
  step("hideClash", hideWindow, "sbWindow")
  step("clashHidden", windowVisible, "sbWindow")
  look("clashLabelHidden", "sbClashLabel")
  look("clashMiniHidden", "sbClashMini")
  step("showClash", showWindow, "sbWindow")
  step("deleteClash", deleteScrollBox, "sbWindow")
  step("clashDeleted", windowType, "sbWindow")
  step("bgImage", setBackgroundImage, "sbBox", "nothing.png")
  step("bgColor", setBackgroundColor, "sbBox", 1, 2, 3)
  events("beforeDelete")
  step("delete", deleteScrollBox, "sbBox")
  events("delete")
  look("deleted", "sbBox")
  look("deletedLabelAtOnce", "sbInnerLabel")
  look("deletedMiniAtOnce", "sbMini")
  look("deletedInnerAtOnce", "sbInnerBox")
  step("deleteAgain", deleteScrollBox, "sbBox")
  step("deleteEmpty", deleteScrollBox, "")
  step("deleteNumber", deleteScrollBox, 5)
  step("deleteNoArgs", deleteScrollBox)
  step("createAfterDelete", createScrollBox, "sbBox", 1, 1, 10, 10)
  look("recreated", "sbBox")
  step("deleteRecreated", deleteScrollBox, "sbBox")
  step("deleteMainBox", deleteScrollBox, "sbMainBox")
  step("deleteWinBox", deleteScrollBox, "sbWinBox")
  look("notYetGoneLabel", "sbLabel")
  look("notYetGoneWinBoxMini", "sbWinBoxMini")
  events("end")
  closeUserWindow("sbWindow")
  headlessScrollAnswers = table.concat(answers, "\n")
end)
headlessScroll = ok and "ok" or tostring(err)
-- Run once deferred deletes have, which a spec cannot wait for
function headlessScrollAfter()
  local answers = {}
  for _, name in ipairs({"sbLabel", "sbInnerLabel", "sbMini", "sbInnerBox", "sbWinBoxLabel", "sbWinBoxMini", "sbBox", "sbClashMini", "sbClashLabel", "sbWindow"}) do
    answers[#answers + 1] = name .. ":" .. tostring(windowType(name))
  end
  return table.concat(answers, "\n")
end
)lua"));
        QVERIFY2(ran, "The Lua chunk did not run.");
        QCOMPARE(luaGlobalString(host, "headlessScroll"), qsl("ok"));
        const QStringList answers{qsl("create=[1]true"),
                                  qsl("box:type=[1]scrollbox,geometry=[4]10:20:200:100,visible=[1]true"),
                                  qsl("createAgain=[2]false:scrollBox 'sbBox' already exists, moving/resizing 'sbBox'"),
                                  qsl("boxAgain:type=[1]scrollbox,geometry=[4]15:25:210:110,visible=[1]true"),
                                  qsl("createMain=[1]true"),
                                  qsl("mainBox:type=[1]scrollbox,geometry=[4]1:2:30:40,visible=[1]true"),
                                  qsl("createInWindow=[1]true"),
                                  qsl("winBox:type=[1]scrollbox,geometry=[4]3:4:150:80,visible=[1]true"),
                                  qsl("createInBox=[1]true"),
                                  qsl("innerBox:type=[1]scrollbox,geometry=[4]5:6:60:50,visible=[1]true"),
                                  qsl("createMissingParent=[2]false:window 'sbNoSuch' not found"),
                                  qsl("orphan:type=[2]nil:'sbOrphan' is not a known label, any type of console, command line, text edit, nor scroll box,geometry=[2]nil:window \"sbOrphan\" not "
                                      "found,visible=[2]nil:window \"sbOrphan\" not found"),
                                  qsl("createNoName=raised:createScrollBox: bad argument #1 type (scrollBox name as string expected, got no value!)"),
                                  qsl("createBadX=raised:createScrollBox: bad argument #6 type (scrollBox height as number expected, got no value!)"),
                                  qsl("createLabel=[1]true"),
                                  qsl("label:type=[1]label,geometry=[4]7:8:40:20,visible=[1]true"),
                                  qsl("echo=[1]true"),
                                  qsl("label:font=main"),
                                  qsl("createWinBoxLabel=[1]true"),
                                  qsl("echoWinBoxLabel=[1]true"),
                                  qsl("winBoxLabel:font=sbWindow"),
                                  qsl("createInnerLabel=[1]true"),
                                  qsl("innerLabel:type=[1]label,geometry=[4]2:2:20:10,visible=[1]true"),
                                  qsl("createMini=[1]true"),
                                  qsl("mini:type=[1]miniconsole,geometry=[4]9:10:50:40,visible=[1]true"),
                                  qsl("miniFontSize=[1]12"),
                                  qsl("createWinBoxMini=[1]true"),
                                  qsl("winBoxMiniFontSize=[1]12"),
                                  qsl("move=[1]true"),
                                  qsl("resize=[1]true"),
                                  qsl("moved:type=[1]scrollbox,geometry=[4]11:12:120:90,visible=[1]true"),
                                  qsl("raise=[1]true"),
                                  qsl("lower=[1]true"),
                                  qsl("hide=[1]true"),
                                  qsl("hidden:type=[1]scrollbox,geometry=[4]11:12:120:90,visible=[1]false"),
                                  qsl("hiddenLabel:type=[1]label,geometry=[4]7:8:40:20,visible=[1]false"),
                                  qsl("hiddenMini:type=[1]miniconsole,geometry=[4]9:10:50:40,visible=[1]false"),
                                  qsl("hiddenInnerBox:type=[1]scrollbox,geometry=[4]5:6:60:50,visible=[1]false"),
                                  qsl("hiddenInnerLabel:type=[1]label,geometry=[4]2:2:20:10,visible=[1]false"),
                                  qsl("showLabelInHidden=[1]true"),
                                  qsl("labelInHidden:type=[1]label,geometry=[4]7:8:40:20,visible=[1]false"),
                                  qsl("show=[1]true"),
                                  qsl("shown:type=[1]scrollbox,geometry=[4]11:12:120:90,visible=[1]true"),
                                  qsl("shownLabel:type=[1]label,geometry=[4]7:8:40:20,visible=[1]true"),
                                  qsl("shownInnerLabel:type=[1]label,geometry=[4]2:2:20:10,visible=[1]true"),
                                  qsl("hideWindow=[1]true"),
                                  qsl("winBoxInHidden:type=[1]scrollbox,geometry=[4]3:4:150:80,visible=[1]false"),
                                  qsl("winBoxLabelInHidden:type=[1]label,geometry=[4]1:1:40:20,visible=[1]false"),
                                  qsl("showWindow=[1]true"),
                                  qsl("winBoxShown:type=[1]scrollbox,geometry=[4]3:4:150:80,visible=[1]true"),
                                  qsl("createClash=[1]true"),
                                  qsl("createClashLabel=[1]true"),
                                  qsl("echoClashLabel=[1]true"),
                                  qsl("clashLabel:font=sbWindow"),
                                  qsl("createClashMini=[1]true"),
                                  qsl("hideClash=[1]true"),
                                  qsl("clashHidden=[1]false"),
                                  qsl("clashLabelHidden:type=[1]label,geometry=[4]0:0:40:20,visible=[1]false"),
                                  qsl("clashMiniHidden:type=[1]miniconsole,geometry=[4]0:0:40:20,visible=[1]true"),
                                  qsl("showClash=[1]true"),
                                  qsl("deleteClash=[1]true"),
                                  qsl("clashDeleted=[1]userwindow"),
                                  qsl("bgImage=[2]nil:console or label 'sbBox' not found, or 'nothing.png' could not be loaded as an image"),
                                  qsl("bgColor=[2]nil:'sbBox' is a scroll box, use setProfileStyleSheet() to set its background"),
                                  qsl("beforeDelete:events=sbWindow"),
                                  qsl("delete=[1]true"),
                                  qsl("delete:events=sbBox"),
                                  qsl("deleted:type=[2]nil:'sbBox' is not a known label, any type of console, command line, text edit, nor scroll box,geometry=[2]nil:window \"sbBox\" not "
                                      "found,visible=[2]nil:window \"sbBox\" not found"),
                                  qsl("deletedLabelAtOnce:type=[1]label,geometry=[4]2:2:20:10,visible=[1]true"),
                                  qsl("deletedMiniAtOnce:type=[1]miniconsole,geometry=[4]9:10:50:40,visible=[1]true"),
                                  qsl("deletedInnerAtOnce:type=[1]scrollbox,geometry=[4]5:6:60:50,visible=[1]true"),
                                  qsl("deleteAgain=[2]false:scrollbox name 'sbBox' not found"),
                                  qsl("deleteEmpty=[2]false:a scrollbox cannot have an empty string as its name"),
                                  qsl("deleteNumber=[2]false:scrollbox name '5' not found"),
                                  qsl("deleteNoArgs=raised:deleteScrollBox: bad argument #1 type (scrollbox name as string expected, got no value!)"),
                                  qsl("createAfterDelete=[1]true"),
                                  qsl("recreated:type=[1]scrollbox,geometry=[4]1:1:10:10,visible=[1]true"),
                                  qsl("deleteRecreated=[1]true"),
                                  qsl("deleteMainBox=[1]true"),
                                  qsl("deleteWinBox=[1]true"),
                                  qsl("notYetGoneLabel:type=[1]label,geometry=[4]7:8:40:20,visible=[1]true"),
                                  qsl("notYetGoneWinBoxMini:type=[1]miniconsole,geometry=[4]0:0:50:40,visible=[1]true"),
                                  qsl("end:events=sbBox,sbMainBox,sbWinBox")};
        QCOMPARE(luaGlobalString(host, "headlessScrollAnswers"), answers.join(QLatin1Char('\n')));

        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        ran = host->getLuaInterpreter()->compileAndExecuteScript(qsl("headlessScrollAfterAnswers = headlessScrollAfter()"));
        QVERIFY2(ran, "The Lua chunk after the deferred deletes did not run.");
        const QStringList after{qsl("sbLabel:nil"),
                                qsl("sbInnerLabel:nil"),
                                qsl("sbMini:nil"),
                                qsl("sbInnerBox:nil"),
                                qsl("sbWinBoxLabel:nil"),
                                qsl("sbWinBoxMini:nil"),
                                qsl("sbBox:nil"),
                                qsl("sbClashMini:nil"),
                                qsl("sbClashLabel:label"),
                                qsl("sbWindow:userwindow")};
        QCOMPARE(luaGlobalString(host, "headlessScrollAfterAnswers"), after.join(QLatin1Char('\n')));
        QVERIFY2(QApplication::topLevelWidgets().isEmpty(), "Making and deleting the scroll boxes created a widget.");
    }

    // A real scroll box, a command line and what the box holds are destroyed with the user window they are in
    void test_headlessUserWindowTakesItsScrollBoxes()
    {
        const QString hostname = qsl("Test-Headless-Host-User-Window-Scroll-Box");
        QVERIFY2(HostManager::self()->addHost(hostname, QString(), QString(), QString()), "Could not create a profile with no main window.");
        Host* host = HostManager::self()->getHost(hostname);
        QVERIFY2(host, "The profile is not in the pool.");
        QVERIFY(!host->hasConsoleView());

        bool ran = host->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(
assert(openUserWindow("ubWindow", false, false, "f"))
assert(createScrollBox("ubWindow", "ubBox", 0, 0, 100, 100))
assert(createLabel("ubBox", "ubLabel", 0, 0, 10, 10, 1))
assert(createCommandLine("ubWindow", "ubLine", 0, 0, 10, 10))
assert(createCommandLine("ubBox", "ubBoxLine", 0, 0, 10, 10))
assert(createScrollBox("ubWindow", "ubShared", 0, 0, 100, 100))
hideWindow("ubShared")
assert(createCommandLine("ubShared", 3, 4, 40, 20))
assert(createScrollBox("ubLoneBox", 0, 0, 100, 100))
assert(createCommandLine("ubLoneBox", "ubLoneLine", 0, 0, 10, 10))
-- Weak keys, so a function only a command line still holds is collected
ubActions = setmetatable({}, {__mode = "k"})
for _, name in ipairs({"ubLine", "ubBoxLine", "ubLoneLine"}) do
  local action = function() end
  ubActions[action] = name
  setCmdLineAction(name, action)
end
assert(deleteMiniConsole("ubWindow"))
assert(deleteScrollBox("ubLoneBox"))
ubTypes = tostring(windowType("ubBox")) .. "," .. tostring(windowType("ubLabel")) .. "," .. tostring(windowType("ubLine")) .. "," .. tostring(windowType("ubBoxLine"))
)lua"));
        QVERIFY2(ran, "The Lua chunk did not run.");
        QCOMPARE(luaGlobalString(host, "ubTypes"), qsl("nil,nil,nil,nil"));
        QVERIFY(!host->windowRegistry().hasScrollBox(qsl("ubBox")));
        QVERIFY(!host->windowRegistry().hasLabel(qsl("ubLabel")));
        QVERIFY(!host->windowRegistry().hasCommandLine(qsl("ubLine")));
        QVERIFY(!host->windowRegistry().hasCommandLine(qsl("ubBoxLine")));

        // A main window command line of a scroll box's name is what is left of the name once the box is
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        ran = host->getLuaInterpreter()->compileAndExecuteScript(
                qsl(R"lua(ubShared = tostring(windowType("ubShared")) .. ":" .. table.concat({getWindowGeometry("ubShared")}, ",") .. ":" .. tostring(windowVisible("ubShared")))lua"));
        QVERIFY2(ran, "The Lua chunk after the deferred deletes did not run.");
        QCOMPARE(luaGlobalString(host, "ubShared"), qsl("commandline:3,4,40,20:true"));
        ran = host->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(
collectgarbage("collect")
local kept = {}
for _, name in pairs(ubActions) do
  kept[#kept + 1] = name
end
table.sort(kept)
ubActionsKept = table.concat(kept, ",")
)lua"));
        QVERIFY2(ran, "The Lua chunk after collecting garbage did not run.");
        QCOMPARE(luaGlobalString(host, "ubActionsKept"), QString());
    }

    // The answers are those of the same Lua run in the GUI self-test profile
    void test_headlessWindowSizesContract()
    {
        const QString hostname = qsl("Test-Headless-Host-Window-Sizes");
        QVERIFY2(HostManager::self()->addHost(hostname, QString(), QString(), QString()), "Could not create a profile with no main window.");
        Host* host = HostManager::self()->getHost(hostname);
        QVERIFY2(host, "The profile is not in the pool.");
        QVERIFY(!host->hasConsoleView());

        const bool ran = host->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(
headlessWindowSizes = "not run"
local ok, err = pcall(function()
  local function described(called, ...)
    if not called then
      return "raised:" .. tostring((...))
    end
    local count = select("#", ...)
    local values = {}
    for i = 1, count do
      values[i] = tostring((select(i, ...)))
    end
    return "[" .. count .. "]" .. table.concat(values, ":")
  end
  local answers = {}
  local function look(label, name)
    answers[#answers + 1] = label .. ":geometry=" .. described(pcall(getWindowGeometry, name)) .. ",visible=" .. described(pcall(windowVisible, name))
  end
  -- QWIDGETSIZE_MAX is 16777215
  local huge = 20000000
  createLabel("szNegativeLabel", 1, 2, -5, -7, 1)
  look("negativeLabel", "szNegativeLabel")
  createLabel("szHugeLabel", 1, 2, huge, huge, 1)
  look("hugeLabel", "szHugeLabel")
  createMiniConsole("szNegativeMini", 1, 2, -5, -7)
  look("negativeMini", "szNegativeMini")
  createMiniConsole("szHugeMini", 1, 2, huge, huge)
  look("hugeMini", "szHugeMini")
  createScrollBox("szNegativeBox", 1, 2, -5, -7)
  look("negativeBox", "szNegativeBox")
  createScrollBox("szHugeBox", 1, 2, huge, huge)
  look("hugeBox", "szHugeBox")
  for _, name in ipairs({"szNegativeLabel", "szNegativeMini", "szNegativeBox"}) do
    resizeWindow(name, huge, huge)
    look(name .. ":resizedHuge", name)
    resizeWindow(name, -3, -4)
    look(name .. ":resizedNegative", name)
  end
  -- A scroll box is bounded only as it is made
  createScrollBox("szNegativeBox", 1, 2, -6, -8)
  look("negativeBox:remadeNegative", "szNegativeBox")
  createScrollBox("szHugeBox", 1, 2, huge, huge)
  look("hugeBox:remadeHuge", "szHugeBox")
  -- A label made where nothing has been shown, then moved out with show false
  openUserWindow("szHidden", false, false, "f")
  hideWindow("szHidden")
  createScrollBox("szHidden", "szHiddenBox", 0, 0, 50, 50)
  resizeWindow("szHiddenBox", -3, -4)
  look("hiddenBox:resizedNegative", "szHiddenBox")
  createLabel("szHiddenBox", "szNeverShown", 1, 1, 20, 20, 1)
  look("neverShown", "szNeverShown")
  setWindow("main", "szNeverShown", 3, 3, false)
  look("neverShownMoved", "szNeverShown")
  for _, name in ipairs({"szNegativeLabel", "szHugeLabel", "szNeverShown"}) do
    deleteLabel(name)
  end
  for _, name in ipairs({"szNegativeMini", "szHugeMini"}) do
    deleteMiniConsole(name)
  end
  for _, name in ipairs({"szNegativeBox", "szHugeBox", "szHiddenBox"}) do
    deleteScrollBox(name)
  end
  closeUserWindow("szHidden")
  headlessWindowSizesAnswers = table.concat(answers, "\n")
end)
headlessWindowSizes = ok and "ok" or tostring(err)
)lua"));
        QVERIFY2(ran, "The Lua chunk did not run.");
        QCOMPARE(luaGlobalString(host, "headlessWindowSizes"), qsl("ok"));
        const QStringList answers{qsl("negativeLabel:geometry=[4]1:2:0:0,visible=[1]true"),
                                  qsl("hugeLabel:geometry=[4]1:2:16777215:16777215,visible=[1]true"),
                                  qsl("negativeMini:geometry=[4]1:2:0:0,visible=[1]true"),
                                  qsl("hugeMini:geometry=[4]1:2:16777215:16777215,visible=[1]true"),
                                  qsl("negativeBox:geometry=[4]1:2:0:0,visible=[1]true"),
                                  qsl("hugeBox:geometry=[4]1:2:16777215:16777215,visible=[1]true"),
                                  qsl("szNegativeLabel:resizedHuge:geometry=[4]1:2:16777215:16777215,visible=[1]true"),
                                  qsl("szNegativeLabel:resizedNegative:geometry=[4]1:2:0:0,visible=[1]true"),
                                  qsl("szNegativeMini:resizedHuge:geometry=[4]1:2:16777215:16777215,visible=[1]true"),
                                  qsl("szNegativeMini:resizedNegative:geometry=[4]1:2:0:0,visible=[1]true"),
                                  qsl("szNegativeBox:resizedHuge:geometry=[4]1:2:20000000:20000000,visible=[1]true"),
                                  qsl("szNegativeBox:resizedNegative:geometry=[4]1:2:-3:-4,visible=[1]true"),
                                  qsl("negativeBox:remadeNegative:geometry=[4]1:2:-6:-8,visible=[1]true"),
                                  qsl("hugeBox:remadeHuge:geometry=[4]1:2:20000000:20000000,visible=[1]true"),
                                  qsl("hiddenBox:resizedNegative:geometry=[4]0:0:-3:-4,visible=[1]false"),
                                  qsl("neverShown:geometry=[4]1:1:20:20,visible=[1]false"),
                                  qsl("neverShownMoved:geometry=[4]3:3:20:20,visible=[1]false")};
        QCOMPARE(luaGlobalString(host, "headlessWindowSizesAnswers"), answers.join(QLatin1Char('\n')));
    }

    // The answers are those of the same Lua run in the GUI self-test profile, but for the last ones,
    // which a spec cannot wait for: what was moved into a deleted scroll box goes with it
    void test_headlessSetWindowContract()
    {
        const QString hostname = qsl("Test-Headless-Host-Set-Window");
        QVERIFY2(HostManager::self()->addHost(hostname, QString(), QString(), QString()), "Could not create a profile with no main window.");
        Host* host = HostManager::self()->getHost(hostname);
        QVERIFY2(host, "The profile is not in the pool.");
        QVERIFY(!host->hasConsoleView());

        bool ran = host->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(
headlessSetWindow = "not run"
local ok, err = pcall(function()
  local function described(called, ...)
    if not called then
      return "raised:" .. tostring((...))
    end
    local count = select("#", ...)
    local values = {}
    for i = 1, count do
      values[i] = tostring((select(i, ...)))
    end
    return "[" .. count .. "]" .. table.concat(values, ":")
  end
  local answers = {}
  local function step(label, f, ...)
    answers[#answers + 1] = label .. "=" .. described(pcall(f, ...))
  end
  local function look(label, name)
    answers[#answers + 1] = label .. ":type=" .. described(pcall(windowType, name)) .. ",geometry=" .. described(pcall(getWindowGeometry, name)) .. ",visible=" .. described(pcall(windowVisible, name))
  end
  -- A label measures its text in the font it has, which is read against labels made in the main window and in a user window
  local function font(label, name)
    local width, height = getLabelSizeHint(name)
    local kinds = {}
    for _, parent in ipairs({"main", "swWindow"}) do
      createLabel(parent, "swTwin", 0, 0, 50, 20, 1)
      echo("swTwin", getLabelText(name))
      local twinWidth, twinHeight = getLabelSizeHint("swTwin")
      deleteLabel("swTwin")
      if width == twinWidth and height == twinHeight then
        kinds[#kinds + 1] = parent
      end
    end
    answers[#answers + 1] = label .. ":font=" .. (#kinds > 0 and table.concat(kinds, "+") or "other")
  end
  assert(openUserWindow("swWindow", false, false, "f") == true, "openUserWindow did not answer true")
  setFont("swWindow", "Bitstream Vera Sans Mono")
  setFontSize("swWindow", 20)
  assert(createScrollBox("swBox", 10, 20, 200, 100))
  assert(createScrollBox("swWindow", "swWinBox", 3, 4, 150, 80))
  assert(createScrollBox("swBox", "swInner", 5, 6, 60, 50))
  createLabel("swInner", "swDeep", 1, 2, 30, 15, 1)
  echo("swDeep", "deep")
  createLabel("swInner", "swDeepPlain", 1, 20, 40, 20, 1)
  setLabelStyleSheet("swDeepPlain", "")
  echo("swDeepPlain", "words")

  createLabel("main", "swLabel", 7, 8, 40, 20, 1)
  echo("swLabel", "words")
  font("label", "swLabel")
  step("labelIntoBox", setWindow, "swBox", "swLabel", 3, 3, true)
  look("labelInBox", "swLabel")
  font("labelInBox", "swLabel")
  hideWindow("swBox")
  look("labelInHiddenBox", "swLabel")
  showWindow("swBox")
  step("labelIntoWinBox", setWindow, "swWinBox", "swLabel", 4, 4, false)
  look("labelInWinBox", "swLabel")
  font("labelInWinBox", "swLabel")
  step("showLabel", showWindow, "swLabel")
  look("labelShown", "swLabel")
  hideWindow("swWindow")
  look("labelInHiddenWindow", "swLabel")
  showWindow("swWindow")
  step("labelIntoInner", setWindow, "swInner", "swLabel", 1, 1, true)
  hideWindow("swBox")
  look("labelInHiddenOuter", "swLabel")
  step("labelIntoHiddenBox", setWindow, "swBox", "swLabel", 2, 2, true)
  look("labelIntoHidden", "swLabel")
  showWindow("swBox")
  look("labelOutOfHidden", "swLabel")
  step("labelIntoWindow", setWindow, "swWindow", "swLabel", 5, 5, true)
  look("labelInWindow", "swLabel")
  font("labelInWindow", "swLabel")
  step("labelIntoMain", setWindow, "main", "swLabel", 6, 6, true)
  look("labelInMain", "swLabel")
  font("labelInMain", "swLabel")
  -- QWidget::setParent(QWidget*) leaves a widget whose parent stays the same shown or hidden
  step("labelSameHidden", setWindow, "main", "swLabel", 6, 6, false)
  look("labelSameHidden", "swLabel")
  hideWindow("swLabel")
  step("hiddenLabelSame", setWindow, "main", "swLabel", 6, 6, false)
  look("hiddenLabelSame", "swLabel")
  step("labelSameShown", setWindow, "main", "swLabel", 6, 6, true)
  look("labelSameShown", "swLabel")

  createLabel("main", "swPlain", 0, 0, 40, 20, 1)
  setLabelStyleSheet("swPlain", "")
  echo("swPlain", "words")
  font("plain", "swPlain")
  step("plainIntoWinBox", setWindow, "swWinBox", "swPlain", 0, 0, true)
  font("plainInWinBox", "swPlain")
  step("plainIntoBox", setWindow, "swBox", "swPlain", 0, 0, true)
  font("plainInBox", "swPlain")
  step("plainOutToWinBox", setWindow, "swWinBox", "swPlain", 1, 1, true)

  createMiniConsole("main", "swMini", 9, 10, 50, 40)
  step("miniIntoBox", setWindow, "swBox", "swMini", 5, 5, true)
  look("miniInBox", "swMini")
  step("miniFontSize", getFontSize, "swMini")
  hideWindow("swBox")
  look("miniInHiddenBox", "swMini")
  showWindow("swBox")
  step("miniIntoWinBox", setWindow, "swWinBox", "swMini", 6, 6, false)
  look("miniInWinBox", "swMini")
  step("showMini", showWindow, "swMini")
  look("miniShown", "swMini")
  step("miniIntoMain", setWindow, "main", "swMini", 2, 2, true)
  look("miniInMain", "swMini")
  step("miniIntoWindow", setWindow, "swWindow", "swMini", 1, 1, true)
  hideWindow("swWindow")
  look("miniInHiddenWindow", "swMini")
  showWindow("swWindow")
  step("miniWinFontSize", getFontSize, "swMini")
  step("miniSameHidden", setWindow, "swWindow", "swMini", 1, 1, false)
  look("miniSameHidden", "swMini")
  hideWindow("swMini")
  step("hiddenMiniSame", setWindow, "swWindow", "swMini", 1, 1, false)
  look("hiddenMiniSame", "swMini")
  step("miniSameShown", setWindow, "swWindow", "swMini", 1, 1, true)
  look("miniSameShown", "swMini")

  step("boxIntoWindow", setWindow, "swWindow", "swBox", 6, 6, true)
  look("boxInWindow", "swBox")
  font("deepInWindowBox", "swDeep")
  font("deepPlainInWindowBox", "swDeepPlain")
  hideWindow("swWindow")
  look("boxInHiddenWindow", "swBox")
  look("deepInHiddenWindow", "swDeep")
  showWindow("swWindow")
  step("boxSameHidden", setWindow, "swWindow", "swBox", 6, 6, false)
  look("boxSameHidden", "swBox")
  look("deepInSameHidden", "swDeep")
  hideWindow("swBox")
  step("hiddenBoxSame", setWindow, "swWindow", "swBox", 6, 6, false)
  look("hiddenBoxSame", "swBox")
  look("deepInHiddenBoxSame", "swDeep")
  step("boxSameShown", setWindow, "swWindow", "swBox", 6, 6, true)
  look("boxSameShown", "swBox")
  look("deepInSameShown", "swDeep")
  step("boxIntoItself", setWindow, "swBox", "swBox", 0, 0, true)
  step("boxIntoChild", setWindow, "swInner", "swBox", 0, 0, true)
  step("boxIntoMissing", setWindow, "swNoSuch", "swBox", 0, 0, true)
  step("missingIntoBox", setWindow, "swBox", "swNoSuch", 0, 0, true)
  step("windowIntoBox", setWindow, "swBox", "swWindow", 0, 0, true)
  step("mapperIntoBox", setWindow, "swBox", "mapper", 0, 0, true)
  step("mainIntoBox", setWindow, "swBox", "main", 0, 0, true)
  step("innerToMainHidden", setWindow, "main", "swInner", 7, 7, false)
  look("innerInMain", "swInner")
  look("deepInHiddenInner", "swDeep")
  step("innerBack", setWindow, "swBox", "swInner", 5, 6, true)
  look("innerBack", "swInner")
  look("deepBack", "swDeep")
  step("innerSameHidden", setWindow, "swBox", "swInner", 5, 6, false)
  look("innerSameHidden", "swInner")
  hideWindow("swInner")
  step("hiddenInnerSame", setWindow, "swBox", "swInner", 5, 6, false)
  look("hiddenInnerSame", "swInner")
  step("innerSameShown", setWindow, "swBox", "swInner", 5, 6, true)
  look("innerSameShown", "swInner")
  step("boxIntoMain", setWindow, "main", "swBox", 11, 12, true)
  look("boxInMain", "swBox")
  step("boxIntoWinBox", setWindow, "swWinBox", "swBox", 1, 1, true)
  hideWindow("swWindow")
  look("boxInHiddenWinBox", "swBox")
  look("deepInHiddenWinBox", "swDeep")
  showWindow("swWindow")
  font("deepPlainInWinBox", "swDeepPlain")
  step("boxBackToMain", setWindow, "main", "swBox", 11, 12, true)
  font("deepPlainInMain", "swDeepPlain")

  -- setWindow takes a scroll box before a user window of the same name
  assert(createScrollBox("swWindow", 30, 30, 20, 20))
  step("plainIntoClash", setWindow, "swWindow", "swPlain", 0, 0, true)
  font("plainInClash", "swPlain")
  step("plainOutOfClash", setWindow, "swWinBox", "swPlain", 1, 1, true)
  deleteScrollBox("swWindow")
  step("miniIntoBoxAgain", setWindow, "swBox", "swMini", 5, 5, true)
  step("labelIntoInnerAgain", setWindow, "swInner", "swLabel", 1, 1, true)
  step("delete", deleteScrollBox, "swBox")
  look("labelAtOnce", "swLabel")
  look("miniAtOnce", "swMini")
  look("plainOutside", "swPlain")
  -- Made while their user window is hidden
  assert(openUserWindow("swHidden", false, false, "f") == true, "openUserWindow did not answer true")
  hideWindow("swHidden")
  createMiniConsole("swHidden", "swHiddenMini", 0, 0, 20, 20)
  assert(createScrollBox("swHidden", "swHiddenBox", 0, 30, 20, 20))
  look("hiddenMiniMade", "swHiddenMini")
  step("hiddenMiniSameHidden", setWindow, "swHidden", "swHiddenMini", 0, 0, false)
  step("hiddenBoxSameShown", setWindow, "swHidden", "swHiddenBox", 0, 30, true)
  showWindow("swHidden")
  look("hiddenMiniSameHidden", "swHiddenMini")
  look("hiddenBoxSameShown", "swHiddenBox")
  hideWindow("swHidden")
  step("hiddenMiniSameShown", setWindow, "swHidden", "swHiddenMini", 0, 0, true)
  step("hiddenBoxSameHidden", setWindow, "swHidden", "swHiddenBox", 0, 30, false)
  showWindow("swHidden")
  look("hiddenMiniSameShown", "swHiddenMini")
  look("hiddenBoxSameHidden", "swHiddenBox")
  closeUserWindow("swHidden")
  closeUserWindow("swWindow")
  headlessSetWindowAnswers = table.concat(answers, "\n")
end)
headlessSetWindow = ok and "ok" or tostring(err)
-- Run once deferred deletes have, which a spec cannot wait for
function headlessSetWindowAfter()
  local answers = {}
  for _, name in ipairs({"swLabel", "swMini", "swDeep", "swInner", "swPlain", "swWinBox", "swDeepPlain"}) do
    answers[#answers + 1] = name .. ":" .. tostring(windowType(name))
  end
  return table.concat(answers, "\n")
end
)lua"));
        QVERIFY2(ran, "The Lua chunk did not run.");
        QCOMPARE(luaGlobalString(host, "headlessSetWindow"), qsl("ok"));
        const QStringList answers{qsl("label:font=main"),
qsl("labelIntoBox=[1]true"),
qsl("labelInBox:type=[1]label,geometry=[4]3:3:40:20,visible=[1]true"),
qsl("labelInBox:font=other"),
qsl("labelInHiddenBox:type=[1]label,geometry=[4]3:3:40:20,visible=[1]false"),
qsl("labelIntoWinBox=[1]true"),
qsl("labelInWinBox:type=[1]label,geometry=[4]4:4:40:20,visible=[1]false"),
qsl("labelInWinBox:font=other"),
qsl("showLabel=[1]true"),
qsl("labelShown:type=[1]label,geometry=[4]4:4:40:20,visible=[1]true"),
qsl("labelInHiddenWindow:type=[1]label,geometry=[4]4:4:40:20,visible=[1]false"),
qsl("labelIntoInner=[1]true"),
qsl("labelInHiddenOuter:type=[1]label,geometry=[4]1:1:40:20,visible=[1]false"),
qsl("labelIntoHiddenBox=[1]true"),
qsl("labelIntoHidden:type=[1]label,geometry=[4]2:2:40:20,visible=[1]false"),
qsl("labelOutOfHidden:type=[1]label,geometry=[4]2:2:40:20,visible=[1]true"),
qsl("labelIntoWindow=[1]true"),
qsl("labelInWindow:type=[1]label,geometry=[4]5:5:40:20,visible=[1]true"),
qsl("labelInWindow:font=other"),
qsl("labelIntoMain=[1]true"),
qsl("labelInMain:type=[1]label,geometry=[4]6:6:40:20,visible=[1]true"),
qsl("labelInMain:font=other"),
qsl("labelSameHidden=[1]true"),
qsl("labelSameHidden:type=[1]label,geometry=[4]6:6:40:20,visible=[1]true"),
qsl("hiddenLabelSame=[1]true"),
qsl("hiddenLabelSame:type=[1]label,geometry=[4]6:6:40:20,visible=[1]false"),
qsl("labelSameShown=[1]true"),
qsl("labelSameShown:type=[1]label,geometry=[4]6:6:40:20,visible=[1]true"),
qsl("plain:font=other"),
qsl("plainIntoWinBox=[1]true"),
qsl("plainInWinBox:font=swWindow"),
qsl("plainIntoBox=[1]true"),
qsl("plainInBox:font=main"),
qsl("plainOutToWinBox=[1]true"),
qsl("miniIntoBox=[1]true"),
qsl("miniInBox:type=[1]miniconsole,geometry=[4]5:5:50:40,visible=[1]true"),
qsl("miniFontSize=[1]12"),
qsl("miniInHiddenBox:type=[1]miniconsole,geometry=[4]5:5:50:40,visible=[1]false"),
qsl("miniIntoWinBox=[1]true"),
qsl("miniInWinBox:type=[1]miniconsole,geometry=[4]6:6:50:40,visible=[1]false"),
qsl("showMini=[1]true"),
qsl("miniShown:type=[1]miniconsole,geometry=[4]6:6:50:40,visible=[1]true"),
qsl("miniIntoMain=[1]true"),
qsl("miniInMain:type=[1]miniconsole,geometry=[4]2:2:50:40,visible=[1]true"),
qsl("miniIntoWindow=[1]true"),
qsl("miniInHiddenWindow:type=[1]miniconsole,geometry=[4]1:1:50:40,visible=[1]false"),
qsl("miniWinFontSize=[1]12"),
qsl("miniSameHidden=[1]true"),
qsl("miniSameHidden:type=[1]miniconsole,geometry=[4]1:1:50:40,visible=[1]true"),
qsl("hiddenMiniSame=[1]true"),
qsl("hiddenMiniSame:type=[1]miniconsole,geometry=[4]1:1:50:40,visible=[1]false"),
qsl("miniSameShown=[1]true"),
qsl("miniSameShown:type=[1]miniconsole,geometry=[4]1:1:50:40,visible=[1]true"),
qsl("boxIntoWindow=[1]true"),
qsl("boxInWindow:type=[1]scrollbox,geometry=[4]6:6:200:100,visible=[1]true"),
qsl("deepInWindowBox:font=main"),
qsl("deepPlainInWindowBox:font=swWindow"),
qsl("boxInHiddenWindow:type=[1]scrollbox,geometry=[4]6:6:200:100,visible=[1]false"),
qsl("deepInHiddenWindow:type=[1]label,geometry=[4]1:2:30:15,visible=[1]false"),
qsl("boxSameHidden=[1]true"),
qsl("boxSameHidden:type=[1]scrollbox,geometry=[4]6:6:200:100,visible=[1]true"),
qsl("deepInSameHidden:type=[1]label,geometry=[4]1:2:30:15,visible=[1]true"),
qsl("hiddenBoxSame=[1]true"),
qsl("hiddenBoxSame:type=[1]scrollbox,geometry=[4]6:6:200:100,visible=[1]false"),
qsl("deepInHiddenBoxSame:type=[1]label,geometry=[4]1:2:30:15,visible=[1]false"),
qsl("boxSameShown=[1]true"),
qsl("boxSameShown:type=[1]scrollbox,geometry=[4]6:6:200:100,visible=[1]true"),
qsl("deepInSameShown:type=[1]label,geometry=[4]1:2:30:15,visible=[1]true"),
qsl("boxIntoItself=[2]nil:element 'swBox' cannot be moved into itself or into one of its own children"),
qsl("boxIntoChild=[2]nil:element 'swBox' cannot be moved into itself or into one of its own children"),
qsl("boxIntoMissing=[2]nil:window 'swNoSuch' not found"),
qsl("missingIntoBox=[2]nil:element 'swNoSuch' not found"),
qsl("windowIntoBox=[2]nil:element 'swWindow' is the base of a floating/dockable user window and may not be moved"),
qsl("mapperIntoBox=[2]nil:element 'mapper' not found"),
qsl("mainIntoBox=[2]nil:element 'main' not found"),
qsl("innerToMainHidden=[1]true"),
qsl("innerInMain:type=[1]scrollbox,geometry=[4]7:7:60:50,visible=[1]false"),
qsl("deepInHiddenInner:type=[1]label,geometry=[4]1:2:30:15,visible=[1]false"),
qsl("innerBack=[1]true"),
qsl("innerBack:type=[1]scrollbox,geometry=[4]5:6:60:50,visible=[1]true"),
qsl("deepBack:type=[1]label,geometry=[4]1:2:30:15,visible=[1]true"),
qsl("innerSameHidden=[1]true"),
qsl("innerSameHidden:type=[1]scrollbox,geometry=[4]5:6:60:50,visible=[1]true"),
qsl("hiddenInnerSame=[1]true"),
qsl("hiddenInnerSame:type=[1]scrollbox,geometry=[4]5:6:60:50,visible=[1]false"),
qsl("innerSameShown=[1]true"),
qsl("innerSameShown:type=[1]scrollbox,geometry=[4]5:6:60:50,visible=[1]true"),
qsl("boxIntoMain=[1]true"),
qsl("boxInMain:type=[1]scrollbox,geometry=[4]11:12:200:100,visible=[1]true"),
qsl("boxIntoWinBox=[1]true"),
qsl("boxInHiddenWinBox:type=[1]scrollbox,geometry=[4]1:1:200:100,visible=[1]false"),
qsl("deepInHiddenWinBox:type=[1]label,geometry=[4]1:2:30:15,visible=[1]false"),
qsl("deepPlainInWinBox:font=swWindow"),
qsl("boxBackToMain=[1]true"),
qsl("deepPlainInMain:font=main"),
qsl("plainIntoClash=[1]true"),
qsl("plainInClash:font=main"),
qsl("plainOutOfClash=[1]true"),
qsl("miniIntoBoxAgain=[1]true"),
qsl("labelIntoInnerAgain=[1]true"),
qsl("delete=[1]true"),
qsl("labelAtOnce:type=[1]label,geometry=[4]1:1:40:20,visible=[1]true"),
qsl("miniAtOnce:type=[1]miniconsole,geometry=[4]5:5:50:40,visible=[1]true"),
qsl("plainOutside:type=[1]label,geometry=[4]1:1:40:20,visible=[1]true"),
qsl("hiddenMiniMade:type=[1]miniconsole,geometry=[4]0:0:20:20,visible=[1]false"),
qsl("hiddenMiniSameHidden=[1]true"),
qsl("hiddenBoxSameShown=[1]true"),
qsl("hiddenMiniSameHidden:type=[1]miniconsole,geometry=[4]0:0:20:20,visible=[1]true"),
qsl("hiddenBoxSameShown:type=[1]scrollbox,geometry=[4]0:30:20:20,visible=[1]true"),
qsl("hiddenMiniSameShown=[1]true"),
qsl("hiddenBoxSameHidden=[1]true"),
qsl("hiddenMiniSameShown:type=[1]miniconsole,geometry=[4]0:0:20:20,visible=[1]true"),
qsl("hiddenBoxSameHidden:type=[1]scrollbox,geometry=[4]0:30:20:20,visible=[1]true")};
        QCOMPARE(luaGlobalString(host, "headlessSetWindowAnswers"), answers.join(QLatin1Char('\n')));

        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        ran = host->getLuaInterpreter()->compileAndExecuteScript(qsl("headlessSetWindowAfterAnswers = headlessSetWindowAfter()"));
        QVERIFY2(ran, "The Lua chunk after the deferred deletes did not run.");
        const QStringList after{qsl("swLabel:nil"), qsl("swMini:nil"), qsl("swDeep:nil"), qsl("swInner:nil"), qsl("swPlain:label"), qsl("swWinBox:scrollbox"), qsl("swDeepPlain:nil")};
        QCOMPARE(luaGlobalString(host, "headlessSetWindowAfterAnswers"), after.join(QLatin1Char('\n')));
        QVERIFY2(QApplication::topLevelWidgets().isEmpty(), "Moving the windows created a widget.");
    }

    // The answers are those of the same Lua run in the GUI self-test profile, but for the last ones,
    // which a spec cannot wait for: a command line in a deleted scroll box goes with it
    void test_headlessCommandLineContract()
    {
        const QString hostname = qsl("Test-Headless-Host-Command-Line");
        QVERIFY2(HostManager::self()->addHost(hostname, QString(), QString(), QString()), "Could not create a profile with no main window.");
        Host* host = HostManager::self()->getHost(hostname);
        QVERIFY2(host, "The profile is not in the pool.");
        QVERIFY(!host->hasConsoleView());

        bool ran = host->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(
headlessCommandLine = "not run"
local ok, err = pcall(function()
  local function described(called, ...)
    if not called then
      return "raised:" .. tostring((...))
    end
    local count = select("#", ...)
    local values = {}
    for i = 1, count do
      -- Bytes a line of answers cannot hold as they are, by number
      values[i] = tostring((select(i, ...))):gsub("[%c\128-\255]", function(byte)
        return "\\" .. byte:byte()
      end)
    end
    return "[" .. count .. "]" .. table.concat(values, ":")
  end
  local answers = {}
  local function step(label, f, ...)
    answers[#answers + 1] = label .. "=" .. described(pcall(f, ...))
  end
  local function look(label, name)
    answers[#answers + 1] = label .. ":type=" .. described(pcall(windowType, name)) .. ",geometry=" .. described(pcall(getWindowGeometry, name)) .. ",visible=" .. described(pcall(windowVisible, name))
  end
  local deletedEvents = {}
  -- By name, as the C++ registerAnonymousEventHandler() takes no function
  function headlessCommandLineDeleted(_, name)
    deletedEvents[#deletedEvents + 1] = name
  end
  registerAnonymousEventHandler("sysCommandLineDeleted", "headlessCommandLineDeleted")
  local function events(label)
    answers[#answers + 1] = label .. ":events=" .. table.concat(deletedEvents, ",")
    deletedEvents = {}
  end
  step("create", createCommandLine, "clMain", 10, 20, 200, 30)
  look("created", "clMain")
  step("createAgain", createCommandLine, "clMain", 0, 0, 10, 10)
  step("createEmpty", createCommandLine, "", 0, 0, 10, 10)
  step("createInMissing", createCommandLine, "clNoSuch", "clLost", 0, 0, 10, 10)
  look("lost", "clLost")
  step("createInMain", createCommandLine, "main", "clInMain", 1, 2, 30, 40)
  look("inMain", "clInMain")
  step("createNegative", createCommandLine, "clNegative", 3, 4, -5, -6)
  look("negative", "clNegative")
  step("createBadName", createCommandLine, {}, 0, 0, 10, 10)
  step("createBadParent", createCommandLine, {}, "clBad", 0, 0, 10, 10)
  step("createBadX", createCommandLine, "clBad", "x", 0, 10, 10)
  step("createBadHeight", createCommandLine, "main", "clBad", 0, 0, 10, "h")
  look("bad", "clBad")

  step("getNew", getCmdLine, "clMain")
  step("print", printCmdLine, "clMain", "hello")
  step("getPrinted", getCmdLine, "clMain")
  step("append", appendCmdLine, "clMain", " world")
  step("getAppended", getCmdLine, "clMain")
  step("printOdd", printCmdLine, "clMain", "a\tb\r\nc\rd\194\160e\226\128\168f\226\128\169g\0h")
  step("getOdd", getCmdLine, "clMain")
  step("appendOdd", appendCmdLine, "clMain", "\194\160x\n")
  step("getAppendedOdd", getCmdLine, "clMain")
  step("printNumber", printCmdLine, "clMain", 42)
  step("getNumber", getCmdLine, "clMain")
  step("clear", clearCmdLine, "clMain")
  step("getCleared", getCmdLine, "clMain")
  step("appendToEmpty", appendCmdLine, "clMain", "x")
  step("getAppendedToEmpty", getCmdLine, "clMain")
  look("afterText", "clMain")
  step("printMissing", printCmdLine, "clNoSuch", "x")
  step("appendMissing", appendCmdLine, "clNoSuch", "x")
  step("clearMissing", clearCmdLine, "clNoSuch")
  step("getMissing", getCmdLine, "clNoSuch")
  step("printBadName", printCmdLine, {}, "x")
  step("printBadText", printCmdLine, "clMain", {})
  step("getBadName", getCmdLine, {})
  step("getOther", getCmdLine, "clInMain")

  step("action", setCmdLineAction, "clMain", function() end)
  step("actionAgain", setCmdLineAction, "clMain", function() end)
  step("actionMissing", setCmdLineAction, "clNoSuch", function() end)
  step("actionEmpty", setCmdLineAction, "", function() end)
  step("actionMain", setCmdLineAction, "main", function() end)
  step("actionBadName", setCmdLineAction, {}, function() end)
  step("reset", resetCmdLineAction, "clMain")
  step("resetAgain", resetCmdLineAction, "clMain")
  step("resetMissing", resetCmdLineAction, "clNoSuch")
  step("resetEmpty", resetCmdLineAction, "")
  step("resetMain", resetCmdLineAction, "main")

  step("raise", raiseWindow, "clMain")
  step("lower", lowerWindow, "clMain")
  step("hide", hideWindow, "clMain")
  look("hidden", "clMain")
  step("show", showWindow, "clMain")
  look("shown", "clMain")
  step("move", moveWindow, "clMain", 5, 6)
  step("resize", resizeWindow, "clMain", 70, 25)
  look("moved", "clMain")
  step("resizeNegative", resizeWindow, "clMain", -1, -2)
  look("resizedNegative", "clMain")
  step("resizeBack", resizeWindow, "clMain", 70, 25)

  assert(openUserWindow("clWindow", false, false, "f") == true, "openUserWindow did not answer true")
  step("createInWindow", createCommandLine, "clWindow", "clWin", 1, 1, 50, 20)
  look("inWindow", "clWin")
  hideWindow("clWindow")
  look("inHiddenWindow", "clWin")
  showWindow("clWindow")
  look("inShownWindow", "clWin")
  assert(createScrollBox("clWindow", "clBox", 0, 30, 100, 50))
  step("createInBox", createCommandLine, "clBox", "clBoxed", 2, 2, 40, 20)
  look("inBox", "clBoxed")
  hideWindow("clBox")
  look("inHiddenBox", "clBoxed")
  step("createInHiddenBox", createCommandLine, "clBox", "clLate", 2, 25, 40, 20)
  look("madeInHiddenBox", "clLate")
  step("showInHiddenBox", showWindow, "clLate")
  look("shownInHiddenBox", "clLate")
  showWindow("clBox")
  step("hideBoxed", hideWindow, "clBoxed")
  hideWindow("clBox")
  showWindow("clBox")
  look("boxedStaysHidden", "clBoxed")
  showWindow("clBoxed")
  step("textInBox", printCmdLine, "clBoxed", "boxed")
  step("getInBox", getCmdLine, "clBoxed")

  step("intoBox", setWindow, "clBox", "clMain", 3, 3, true)
  look("movedIntoBox", "clMain")
  hideWindow("clWindow")
  look("movedInHiddenWindow", "clMain")
  showWindow("clWindow")
  step("intoMainHidden", setWindow, "main", "clMain", 4, 4, false)
  look("movedToMainHidden", "clMain")
  step("showMoved", showWindow, "clMain")
  step("sameParentHidden", setWindow, "main", "clMain", 7, 8, false)
  look("sameParent", "clMain")
  step("intoWindow", setWindow, "clWindow", "clMain", 9, 9, true)
  look("movedIntoWindow", "clMain")
  step("textAfterMove", getCmdLine, "clMain")
  step("intoMain", setWindow, "main", "clMain", 10, 20, true)
  -- A scroll box takes a command line before a user window of the same name
  assert(openUserWindow("clTwin", false, false, "f") == true, "openUserWindow did not answer true")
  assert(createScrollBox("clTwin", 0, 0, 100, 50))
  step("createInTwin", createCommandLine, "clTwin", "clInTwin", 1, 1, 40, 20)
  hideWindow("clTwin")
  look("twinWindowHidden", "clInTwin")
  showWindow("clTwin")
  -- What a scroll box holds moves with it
  assert(createScrollBox("clWindow", "clMover", 0, 0, 100, 50))
  step("createInMover", createCommandLine, "clMover", "clRider", 1, 1, 40, 20)
  step("moverIntoMain", setWindow, "main", "clMover", 0, 0, true)
  hideWindow("clWindow")
  look("riderAfterMove", "clRider")
  showWindow("clWindow")
  deleteScrollBox("clTwin")
  deleteScrollBox("clMover")

  -- A command line may share a scroll box's name, and what is read for the name is the box's while there is one
  assert(createScrollBox("clShared", 1, 2, 100, 100))
  hideWindow("clShared")
  step("lineAfterBox", createCommandLine, "clShared", 5, 6, 40, 20)
  look("boxThenLine", "clShared")
  step("resizeShared", resizeWindow, "clShared", 60, 70)
  look("sharedResized", "clShared")
  step("deleteSharedLine", deleteCommandLine, "clShared")
  look("sharedLineDeleted", "clShared")
  deleteScrollBox("clShared")
  step("lineBeforeBox", createCommandLine, "clSharedLater", 5, 6, 40, 20)
  moveWindow("clSharedLater", 9, 9)
  resizeWindow("clSharedLater", 45, 25)
  step("boxAfterLine", createScrollBox, "clSharedLater", 1, 2, 100, 100)
  look("lineThenBox", "clSharedLater")
  hideWindow("clSharedLater")
  look("sharedHidden", "clSharedLater")
  step("deleteSharedBox", deleteScrollBox, "clSharedLater")
  look("sharedBoxDeleted", "clSharedLater")
  moveWindow("clSharedLater", 7, 8)
  look("sharedLineMoved", "clSharedLater")
  deleteCommandLine("clSharedLater")
  createCommandLine("clSharedMoved", 5, 6, 40, 20)
  moveWindow("clSharedMoved", 9, 9)
  createScrollBox("clSharedMoved", 1, 2, 100, 100)
  deleteScrollBox("clSharedMoved")
  look("movedBeforeBox", "clSharedMoved")
  deleteCommandLine("clSharedMoved")

  step("delete", deleteCommandLine, "clInMain")
  events("delete")
  look("deleted", "clInMain")
  step("getDeleted", getCmdLine, "clInMain")
  step("printDeleted", printCmdLine, "clInMain", "x")
  step("actionDeleted", setCmdLineAction, "clInMain", function() end)
  step("raiseDeleted", raiseWindow, "clInMain")
  step("deleteAgain", deleteCommandLine, "clInMain")
  step("deleteMain", deleteCommandLine, "main")
  step("deleteEmpty", deleteCommandLine, "")
  step("deleteMissing", deleteCommandLine, "clNoSuch")
  step("deleteBadName", deleteCommandLine, {})
  events("failedDeletes")
  step("recreate", createCommandLine, "clInMain", 1, 2, 30, 40)
  step("getRecreated", getCmdLine, "clInMain")
  step("deleteRecreated", deleteCommandLine, "clInMain")
  events("recreated")

  step("deleteBox", deleteScrollBox, "clBox")
  events("box")
  look("boxedAtOnce", "clBoxed")
  step("deleteNegative", deleteCommandLine, "clNegative")
  step("deleteFirst", deleteCommandLine, "clMain")
  events("end")
  -- A box in a box gives up its name when the outer one goes
  assert(createScrollBox("clOuter", 0, 0, 100, 100))
  assert(createScrollBox("clOuter", "clNested", 0, 0, 50, 50))
  hideWindow("clNested")
  step("lineNamedForNested", createCommandLine, "clNested", 11, 12, 40, 20)
  deleteScrollBox("clOuter")
  closeUserWindow("clWindow")
  closeUserWindow("clTwin")
  headlessCommandLineAnswers = table.concat(answers, "\n")
end)
headlessCommandLine = ok and "ok" or tostring(err)
-- Run once deferred deletes have, which a spec cannot wait for
function headlessCommandLineAfter()
  local answers = {}
  for _, name in ipairs({"clMain", "clInMain", "clWin", "clBoxed", "clBox", "clWindow"}) do
    answers[#answers + 1] = name .. ":" .. tostring(windowType(name))
  end
  answers[#answers + 1] = "clNested:" .. tostring(windowType("clNested")) .. ":" .. table.concat({getWindowGeometry("clNested")}, ",") .. ":" .. tostring(windowVisible("clNested"))
  return table.concat(answers, "\n")
end
)lua"));
        QVERIFY2(ran, "The Lua chunk did not run.");
        QCOMPARE(luaGlobalString(host, "headlessCommandLine"), qsl("ok"));
        const QStringList answers{qsl("create=[1]true"),
                                  qsl("created:type=[1]commandline,geometry=[4]10:20:200:30,visible=[1]true"),
                                  qsl("createAgain=[2]false:couldn't create commandLine"),
                                  qsl("createEmpty=[2]false:a commandLine cannot have an empty string as its name"),
                                  qsl("createInMissing=[2]false:window 'clNoSuch' not found"),
                                  qsl("lost:type=[2]nil:'clLost' is not a known label, any type of console, command line, text edit, nor scroll box,geometry=[2]nil:window \"clLost\" not "
                                      "found,visible=[2]nil:window \"clLost\" not found"),
                                  qsl("createInMain=[1]true"),
                                  qsl("inMain:type=[1]commandline,geometry=[4]1:2:30:40,visible=[1]true"),
                                  qsl("createNegative=[1]true"),
                                  qsl("negative:type=[1]commandline,geometry=[4]3:4:0:0,visible=[1]true"),
                                  qsl("createBadName=raised:createCommandLine: bad argument #1 type (commandLine name as string expected, got table!)"),
                                  qsl("createBadParent=raised:createCommandLine: bad argument #1 type (parent window name as string expected, got table!)"),
                                  qsl("createBadX=raised:createCommandLine: bad argument #2 type (commandline x-coordinate as number expected, got string!)"),
                                  qsl("createBadHeight=raised:createCommandLine: bad argument #6 type (commandline height as number expected, got string!)"),
                                  qsl("bad:type=[2]nil:'clBad' is not a known label, any type of console, command line, text edit, nor scroll box,geometry=[2]nil:window \"clBad\" not "
                                      "found,visible=[2]nil:window \"clBad\" not found"),
                                  qsl("getNew=[1]"),
                                  qsl("print=[0]"),
                                  qsl("getPrinted=[1]hello"),
                                  qsl("append=[0]"),
                                  qsl("getAppended=[1]hello world"),
                                  qsl("printOdd=[0]"),
                                  qsl("getOdd=[1]a\\9b\\10c\\10d e\\10f\\10g"),
                                  qsl("appendOdd=[0]"),
                                  qsl("getAppendedOdd=[1]a\\9b\\10c\\10d e\\10f\\10g x\\10"),
                                  qsl("printNumber=[0]"),
                                  qsl("getNumber=[1]42"),
                                  qsl("clear=[0]"),
                                  qsl("getCleared=[1]"),
                                  qsl("appendToEmpty=[0]"),
                                  qsl("getAppendedToEmpty=[1]x"),
                                  qsl("afterText:type=[1]commandline,geometry=[4]10:20:200:30,visible=[1]true"),
                                  qsl("printMissing=[2]nil:command line \"clNoSuch\" not found"),
                                  qsl("appendMissing=[2]nil:command line \"clNoSuch\" not found"),
                                  qsl("clearMissing=[2]nil:command line \"clNoSuch\" not found"),
                                  qsl("getMissing=[2]nil:command line \"clNoSuch\" not found"),
                                  qsl("printBadName=raised:printCmdLine: bad argument #1 type (command line name as string expected, got table)!"),
                                  qsl("printBadText=raised:printCmdLine: bad argument #2 type (text to set on command line as string expected, got table!)"),
                                  qsl("getBadName=raised:getCmdLine: bad argument #1 type (command line name as string expected, got table)!"),
                                  qsl("getOther=[1]"),
                                  qsl("action=[1]true"),
                                  qsl("actionAgain=[1]true"),
                                  qsl("actionMissing=[2]nil:command line name 'clNoSuch' not found"),
                                  qsl("actionEmpty=[2]nil:command line name cannot be an empty string"),
                                  qsl("actionMain=[2]nil:the main command line's action cannot be changed, use an alias to act on what is typed there"),
                                  qsl("actionBadName=raised:setCmdLineAction: bad argument #1 type (command line name as string expected, got table!)"),
                                  qsl("reset=[1]true"),
                                  qsl("resetAgain=[1]true"),
                                  qsl("resetMissing=[2]nil:command line name 'clNoSuch' not found"),
                                  qsl("resetEmpty=[2]nil:command line name cannot be an empty string"),
                                  qsl("resetMain=[2]nil:the main command line's action cannot be changed, use an alias to act on what is typed there"),
                                  qsl("raise=[1]true"),
                                  qsl("lower=[1]true"),
                                  qsl("hide=[1]true"),
                                  qsl("hidden:type=[1]commandline,geometry=[4]10:20:200:30,visible=[1]false"),
                                  qsl("show=[1]true"),
                                  qsl("shown:type=[1]commandline,geometry=[4]10:20:200:30,visible=[1]true"),
                                  qsl("move=[1]true"),
                                  qsl("resize=[1]true"),
                                  qsl("moved:type=[1]commandline,geometry=[4]5:6:70:25,visible=[1]true"),
                                  qsl("resizeNegative=[1]true"),
                                  qsl("resizedNegative:type=[1]commandline,geometry=[4]5:6:0:0,visible=[1]true"),
                                  qsl("resizeBack=[1]true"),
                                  qsl("createInWindow=[1]true"),
                                  qsl("inWindow:type=[1]commandline,geometry=[4]1:1:50:20,visible=[1]true"),
                                  qsl("inHiddenWindow:type=[1]commandline,geometry=[4]1:1:50:20,visible=[1]false"),
                                  qsl("inShownWindow:type=[1]commandline,geometry=[4]1:1:50:20,visible=[1]true"),
                                  qsl("createInBox=[1]true"),
                                  qsl("inBox:type=[1]commandline,geometry=[4]2:2:40:20,visible=[1]true"),
                                  qsl("inHiddenBox:type=[1]commandline,geometry=[4]2:2:40:20,visible=[1]false"),
                                  qsl("createInHiddenBox=[1]true"),
                                  qsl("madeInHiddenBox:type=[1]commandline,geometry=[4]2:25:40:20,visible=[1]false"),
                                  qsl("showInHiddenBox=[1]true"),
                                  qsl("shownInHiddenBox:type=[1]commandline,geometry=[4]2:25:40:20,visible=[1]false"),
                                  qsl("hideBoxed=[1]true"),
                                  qsl("boxedStaysHidden:type=[1]commandline,geometry=[4]2:2:40:20,visible=[1]false"),
                                  qsl("textInBox=[0]"),
                                  qsl("getInBox=[1]boxed"),
                                  qsl("intoBox=[1]true"),
                                  qsl("movedIntoBox:type=[1]commandline,geometry=[4]3:3:70:25,visible=[1]true"),
                                  qsl("movedInHiddenWindow:type=[1]commandline,geometry=[4]3:3:70:25,visible=[1]false"),
                                  qsl("intoMainHidden=[1]true"),
                                  qsl("movedToMainHidden:type=[1]commandline,geometry=[4]4:4:70:25,visible=[1]false"),
                                  qsl("showMoved=[1]true"),
                                  qsl("sameParentHidden=[1]true"),
                                  qsl("sameParent:type=[1]commandline,geometry=[4]7:8:70:25,visible=[1]true"),
                                  qsl("intoWindow=[1]true"),
                                  qsl("movedIntoWindow:type=[1]commandline,geometry=[4]9:9:70:25,visible=[1]true"),
                                  qsl("textAfterMove=[1]x"),
                                  qsl("intoMain=[1]true"),
                                  qsl("createInTwin=[1]true"),
                                  qsl("twinWindowHidden:type=[1]commandline,geometry=[4]1:1:40:20,visible=[1]true"),
                                  qsl("createInMover=[1]true"),
                                  qsl("moverIntoMain=[1]true"),
                                  qsl("riderAfterMove:type=[1]commandline,geometry=[4]1:1:40:20,visible=[1]true"),
                                  qsl("lineAfterBox=[1]true"),
                                  qsl("boxThenLine:type=[1]scrollbox,geometry=[4]1:2:100:100,visible=[1]false"),
                                  qsl("resizeShared=[1]true"),
                                  qsl("sharedResized:type=[1]scrollbox,geometry=[4]1:2:60:70,visible=[1]false"),
                                  qsl("deleteSharedLine=[1]true"),
                                  qsl("sharedLineDeleted:type=[1]scrollbox,geometry=[4]1:2:60:70,visible=[1]false"),
                                  qsl("lineBeforeBox=[1]true"),
                                  qsl("boxAfterLine=[1]true"),
                                  qsl("lineThenBox:type=[1]scrollbox,geometry=[4]1:2:100:100,visible=[1]true"),
                                  qsl("sharedHidden:type=[1]scrollbox,geometry=[4]1:2:100:100,visible=[1]false"),
                                  qsl("deleteSharedBox=[1]true"),
                                  qsl("sharedBoxDeleted:type=[1]commandline,geometry=[4]9:9:45:25,visible=[1]true"),
                                  qsl("sharedLineMoved:type=[1]commandline,geometry=[4]7:8:45:25,visible=[1]true"),
                                  qsl("movedBeforeBox:type=[1]commandline,geometry=[4]9:9:40:20,visible=[1]true"),
                                  qsl("delete=[1]true"),
                                  qsl("delete:events=clShared,clSharedLater,clSharedMoved,clInMain"),
                                  qsl("deleted:type=[2]nil:'clInMain' is not a known label, any type of console, command line, text edit, nor scroll box,geometry=[2]nil:window \"clInMain\" not "
                                      "found,visible=[2]nil:window \"clInMain\" not found"),
                                  qsl("getDeleted=[2]nil:command line \"clInMain\" not found"),
                                  qsl("printDeleted=[2]nil:command line \"clInMain\" not found"),
                                  qsl("actionDeleted=[2]nil:command line name 'clInMain' not found"),
                                  qsl("raiseDeleted=[1]false"),
                                  qsl("deleteAgain=[2]false:command line name 'clInMain' not found"),
                                  qsl("deleteMain=[2]false:the main command line cannot be deleted"),
                                  qsl("deleteEmpty=[2]false:the main command line cannot be deleted"),
                                  qsl("deleteMissing=[2]false:command line name 'clNoSuch' not found"),
                                  qsl("deleteBadName=raised:deleteCommandLine: bad argument #1 type (command line name as string expected, got table!)"),
                                  qsl("failedDeletes:events="),
                                  qsl("recreate=[1]true"),
                                  qsl("getRecreated=[1]"),
                                  qsl("deleteRecreated=[1]true"),
                                  qsl("recreated:events=clInMain"),
                                  qsl("deleteBox=[1]true"),
                                  qsl("box:events="),
                                  qsl("boxedAtOnce:type=[1]commandline,geometry=[4]2:2:40:20,visible=[1]true"),
                                  qsl("deleteNegative=[1]true"),
                                  qsl("deleteFirst=[1]true"),
                                  qsl("end:events=clNegative,clMain"),
                                  qsl("lineNamedForNested=[1]true")};
        QCOMPARE(luaGlobalString(host, "headlessCommandLineAnswers"), answers.join(QLatin1Char('\n')));

        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        ran = host->getLuaInterpreter()->compileAndExecuteScript(qsl("headlessCommandLineAfterAnswers = headlessCommandLineAfter()"));
        QVERIFY2(ran, "The Lua chunk after the deferred deletes did not run.");
        const QStringList after{
                qsl("clMain:nil"), qsl("clInMain:nil"), qsl("clWin:commandline"), qsl("clBoxed:nil"), qsl("clBox:nil"), qsl("clWindow:userwindow"), qsl("clNested:commandline:11,12,40,20:true")};
        QCOMPARE(luaGlobalString(host, "headlessCommandLineAfterAnswers"), after.join(QLatin1Char('\n')));
        QVERIFY2(QApplication::topLevelWidgets().isEmpty(), "Making the command lines created a widget.");
    }

    // The answers are those of the same Lua run in the GUI self-test profile
    void test_headlessCommandLineSettingsContract()
    {
        const QString hostname = qsl("Test-Headless-Host-Command-Line-Settings");
        // Opened rather than added, as saveProfile() refuses a profile that was never loaded
        Host* host = HostManager::self()->openProfile(hostname, false);
        QVERIFY2(host, "The profile did not open.");
        QVERIFY(!host->hasConsoleView());
        QVERIFY2(host->mLoadedOk, qPrintable(host->mProfileLoadError));

        // With no view, pumpEvents() takes Mudlet for shutting down and pumps nothing, so the waits for a save use the
        // pump it would
        lua_register(host->getLuaInterpreter()->getLuaGlobalState(), "pumpEvents", [](lua_State* L) -> int {
            static_cast<void>(EventLoopPump::pumpFor(static_cast<int>(lua_tointeger(L, 1))));
            return 0;
        });
        // Run from the event loop, as a script in the GUI is, so the saves it waits out deliver no deferred deletes
        bool ran = false;
        bool pendingDeleteHeld = false;
        QEventLoop loop;
        QTimer::singleShot(0, &loop, [&ran, &pendingDeleteHeld, &loop, host]() {
            // The rows rely on the waits for a save delivering no deferred delete posted here, as in the GUI
            QPointer<QObject> pPending = new QObject();
            pPending->deleteLater();
            ran = host->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(
headlessCommandLineSettings = "not run"
-- A save still running refuses the next one until the event loop has finished it
function headlessCommandLineSave(...)
  local saved, message
  for _ = 1, 100 do
    saved, message = saveProfile(...)
    if saved or not tostring(message):find("a save is already in progress", 1, true) then
      break
    end
    pumpEvents(50)
  end
  if not saved then
    error("the profile was not saved: " .. tostring(message))
  end
  return saved
end
-- What a save wrote to profile.ini, once the event loop has written it out
function headlessCommandLineIni(name)
  pumpEvents(300)
  local file = io.open(getMudletHomeDir() .. "/profile.ini", "r")
  if not file then
    return nil
  end
  local text = file:read("*a")
  file:close()
  local key = "SaveHistory\\" .. name .. "="
  local at = text:find(key, 1, true)
  return at and text:sub(at + #key):match("^%a+")
end
local ok, err = pcall(function()
  local function described(called, ...)
    if not called then
      return "raised:" .. tostring((...))
    end
    local count = select("#", ...)
    local values = {}
    for i = 1, count do
      values[i] = tostring((select(i, ...)))
    end
    return "[" .. count .. "]" .. table.concat(values, ":")
  end
  local answers = {}
  local function step(label, f, ...)
    answers[#answers + 1] = label .. "=" .. described(pcall(f, ...))
  end
  local function look(label, name)
    answers[#answers + 1] = label .. ":type=" .. described(pcall(windowType, name)) .. ",visible=" .. described(pcall(windowVisible, name))
  end
  local save = headlessCommandLineSave
  step("create", createCommandLine, "csLine", 10, 20, 200, 30)

  step("suggest", addCmdLineSuggestion, "csLine", "word")
  step("suggestAgain", addCmdLineSuggestion, "csLine", "word")
  step("unsuggest", removeCmdLineSuggestion, "csLine", "word")
  step("unsuggestAbsent", removeCmdLineSuggestion, "csLine", "absent")
  step("clearSuggestions", clearCmdLineSuggestions, "csLine")
  step("suggestMissing", addCmdLineSuggestion, "csNoSuch", "word")
  step("unsuggestMissing", removeCmdLineSuggestion, "csNoSuch", "word")
  step("clearSuggestionsMissing", clearCmdLineSuggestions, "csNoSuch")
  step("suggestEmpty", addCmdLineSuggestion, "csLine", "")
  step("suggestBadText", addCmdLineSuggestion, "csLine", {})

  step("blacklist", addCmdLineBlacklist, "csLine", "word")
  step("unblacklist", removeCmdLineBlacklist, "csLine", "word")
  step("unblacklistAbsent", removeCmdLineBlacklist, "csLine", "absent")
  step("clearBlacklist", clearCmdLineBlacklist, "csLine")
  step("blacklistMissing", addCmdLineBlacklist, "csNoSuch", "word")
  step("unblacklistMissing", removeCmdLineBlacklist, "csNoSuch", "word")
  step("clearBlacklistMissing", clearCmdLineBlacklist, "csNoSuch")

  step("menu", addCommandLineMenuEvent, "csLine", "Item", "csEvent")
  step("menuAgain", addCommandLineMenuEvent, "csLine", "Item", "csOtherEvent")
  step("menuOther", addCommandLineMenuEvent, "csLine", "Other", "csEvent")
  step("unmenu", removeCommandLineMenuEvent, "csLine", "Item")
  step("unmenuAgain", removeCommandLineMenuEvent, "csLine", "Item")
  step("unmenuOther", removeCommandLineMenuEvent, "csLine", "Other")
  step("menuMissing", addCommandLineMenuEvent, "csNoSuch", "Item", "csEvent")
  step("unmenuMissing", removeCommandLineMenuEvent, "csNoSuch", "Item")

  step("select", selectCmdLineText, "csLine")
  printCmdLine("csLine", "some text")
  step("selectText", selectCmdLineText, "csLine")
  step("textAfterSelect", getCmdLine, "csLine")
  step("selectMissing", selectCmdLineText, "csNoSuch")

  step("history", getSaveCommandHistory, "csLine")
  step("historyOff", setSaveCommandHistory, "csLine", false)
  step("historyAfterOff", getSaveCommandHistory, "csLine")
  step("historyOn", setSaveCommandHistory, "csLine", true)
  step("historyAfterOn", getSaveCommandHistory, "csLine")
  step("historyOffMissing", setSaveCommandHistory, "csNoSuch", false)
  step("historyMissing", getSaveCommandHistory, "csNoSuch")

  step("disable", disableCommandLine, "csLine")
  look("disabled", "csLine")
  step("disableAgain", disableCommandLine, "csLine")
  step("enable", enableCommandLine, "csLine")
  look("enabled", "csLine")
  step("disableMissing", disableCommandLine, "csNoSuch")
  step("enableMissing", enableCommandLine, "csNoSuch")
  step("disableMain", disableCommandLine, "main")
  step("enableEmpty", enableCommandLine, "")
  hideWindow("csLine")
  step("enableHidden", enableCommandLine, "csLine")
  look("enabledAfterHide", "csLine")
  step("disableShown", disableCommandLine, "csLine")
  step("showDisabled", showWindow, "csLine")
  look("shownAfterDisable", "csLine")

  -- A command line in a hidden user window stays out of sight when enabled
  assert(openUserWindow("csWindow", false, false, "f") == true, "openUserWindow did not answer true")
  createCommandLine("csWindow", "csWin", 1, 1, 50, 20)
  disableCommandLine("csWin")
  hideWindow("csWindow")
  step("enableInHidden", enableCommandLine, "csWin")
  look("enabledInHidden", "csWin")
  showWindow("csWindow")
  look("windowShown", "csWin")

  -- The name reads as the scroll box while there is one, but these act on the command line
  assert(createScrollBox("csShared", 1, 2, 100, 100))
  createCommandLine("csShared", 5, 6, 40, 20)
  step("disableShared", disableCommandLine, "csShared")
  look("sharedDisabled", "csShared")
  step("menuShared", addCommandLineMenuEvent, "csShared", "Item", "csEvent")
  step("historyShared", setSaveCommandHistory, "csShared", false)
  step("historySharedAfter", getSaveCommandHistory, "csShared")
  deleteScrollBox("csShared")
  look("sharedBoxDeleted", "csShared")
  step("unmenuShared", removeCommandLineMenuEvent, "csShared", "Item")

  -- A text edit is not a command line
  createTextEdit("csEdit", 0, 0, 50, 50)
  step("suggestEdit", addCmdLineSuggestion, "csEdit", "word")
  step("menuEdit", addCommandLineMenuEvent, "csEdit", "Item", "csEvent")
  step("unmenuEdit", removeCommandLineMenuEvent, "csEdit", "Item")
  step("selectEdit", selectCmdLineText, "csEdit")
  step("historyEdit", setSaveCommandHistory, "csEdit", false)
  step("enableEdit", enableCommandLine, "csEdit")
  step("disableEdit", disableCommandLine, "csEdit")
  look("editAfterDisable", "csEdit")

  -- What is set goes with a deleted command line
  addCommandLineMenuEvent("csLine", "Kept", "csEvent")
  setSaveCommandHistory("csLine", false)
  deleteCommandLine("csLine")
  step("suggestDeleted", addCmdLineSuggestion, "csLine", "word")
  step("blacklistDeleted", addCmdLineBlacklist, "csLine", "word")
  step("unmenuDeleted", removeCommandLineMenuEvent, "csLine", "Kept")
  step("selectDeleted", selectCmdLineText, "csLine")
  step("historyDeleted", setSaveCommandHistory, "csLine", true)
  step("getHistoryDeleted", getSaveCommandHistory, "csLine")
  step("enableDeleted", enableCommandLine, "csLine")
  createCommandLine("csLine", 10, 20, 200, 30)
  step("unmenuRecreated", removeCommandLineMenuEvent, "csLine", "Kept")
  step("historyRecreated", getSaveCommandHistory, "csLine")

  -- The history setting a profile save writes is the one a command line of the name is made with
  setSaveCommandHistory("csLine", false)
  step("save", save)
  deleteCommandLine("csLine")
  createCommandLine("csLine", 10, 20, 200, 30)
  step("historyAfterSave", getSaveCommandHistory, "csLine")
  setSaveCommandHistory("csLine", true)
  deleteCommandLine("csLine")
  createCommandLine("csLine", 10, 20, 200, 30)
  step("historyNotSaved", getSaveCommandHistory, "csLine")
  setSaveCommandHistory("csLine", true)
  step("saveTrue", save)
  deleteCommandLine("csLine")
  createCommandLine("csLine", 10, 20, 200, 30)
  step("historyAfterSavedTrue", getSaveCommandHistory, "csLine")
  -- Only a save with no folder or name is the end of session save that writes the setting
  setSaveCommandHistory("csLine", false)
  step("saveElsewhere", save, getMudletHomeDir() .. "/csElsewhere")
  deleteCommandLine("csLine")
  createCommandLine("csLine", 10, 20, 200, 30)
  step("historyAfterSaveElsewhere", getSaveCommandHistory, "csLine")
  setSaveCommandHistory("csLine", false)
  deleteCommandLine("csLine")
  step("saveAfterDelete", save)
  createCommandLine("csLine", 10, 20, 200, 30)
  step("historyDeletedBeforeSave", getSaveCommandHistory, "csLine")
  -- The command line made last writes last, as the GUI connects them to the save in the order they are made
  openUserWindow("csOrder", false, false, "f")
  step("createOrder", createCommandLine, "csOrder", "csOrder", 0, 0, 50, 20)
  setSaveCommandHistory("csOrder", false)
  step("enableOrder", enableCommandLine, "csOrder")
  deleteMiniConsole("csOrder")
  step("saveOrder", save)
  step("iniOrder", headlessCommandLineIni, "csOrder")
  createCommandLine("csKey_b", 0, 0, 50, 20)
  setSaveCommandHistory("csKey_b", false)
  createCommandLine("csKey/b", 0, 0, 50, 20)
  step("saveKey", save)
  step("iniKey", headlessCommandLineIni, "csKey_b")
  deleteCommandLine("csKey_b")
  deleteCommandLine("csKey/b")
  -- An MXP frame goes at once, its command line with it
  setConfig("specialForceMXPProcessorOn", true)
  feedTriggers([[<FRAME Name="csFrame" Align="right" Width="20%" Height="30%">]] .. "\n")
  step("enableFrame", enableCommandLine, "csFrame")
  setSaveCommandHistory("csFrame", false)
  feedTriggers([[<FRAME csFrame ACTION="close">]] .. "\n")
  setConfig("specialForceMXPProcessorOn", false)
  step("frameClosed", windowType, "csFrame")
  step("saveFrame", save)
  step("iniFrame", headlessCommandLineIni, "csFrame")
  -- A closed frame takes a command line deleted from it at once, as does one whose console was deleted
  setConfig("specialForceMXPProcessorOn", true)
  feedTriggers([[<FRAME Name="csFrameLine" Align="right" Width="20%" Height="30%">]] .. "\n")
  enableCommandLine("csFrameLine")
  setSaveCommandHistory("csFrameLine", false)
  deleteCommandLine("csFrameLine")
  feedTriggers([[<FRAME csFrameLine ACTION="close">]] .. "\n")
  feedTriggers([[<FRAME Name="csFrameConsole" Align="right" Width="20%" Height="30%">]] .. "\n")
  enableCommandLine("csFrameConsole")
  setSaveCommandHistory("csFrameConsole", false)
  deleteMiniConsole("csFrameConsole")
  feedTriggers([[<FRAME csFrameConsole ACTION="close">]] .. "\n")
  setConfig("specialForceMXPProcessorOn", false)
  step("saveFrames", save)
  step("iniFrameLine", headlessCommandLineIni, "csFrameLine")
  step("iniFrameConsole", headlessCommandLineIni, "csFrameConsole")
  -- A frame's console moved out of the frame outlives it, out of the registry, with its command line
  setConfig("specialForceMXPProcessorOn", true)
  openUserWindow("csFrameHome", false, false, "f")
  feedTriggers([[<FRAME Name="csMovedFrame" Align="right" Width="20%" Height="30%">]] .. "\n")
  enableCommandLine("csMovedFrame")
  setSaveCommandHistory("csMovedFrame", false)
  step("moveFrameConsole", setWindow, "csFrameHome", "csMovedFrame", 0, 0, true)
  feedTriggers([[<FRAME csMovedFrame ACTION="close">]] .. "\n")
  setConfig("specialForceMXPProcessorOn", false)
  step("movedFrameType", windowType, "csMovedFrame")
  step("movedFrameHistory", getSaveCommandHistory, "csMovedFrame")
  step("movedFrameText", getCmdLine, "csMovedFrame")
  step("movedFrameVisible", windowVisible, "csMovedFrame")
  setConfig("specialForceMXPProcessorOn", true)
  feedTriggers([[<FRAME Name="csHiddenFrame" Align="right" Width="20%" Height="30%">]] .. "\n")
  enableCommandLine("csHiddenFrame")
  setWindow("csFrameHome", "csHiddenFrame", 0, 0, true)
  disableCommandLine("csHiddenFrame")
  feedTriggers([[<FRAME csHiddenFrame ACTION="close">]] .. "\n")
  setConfig("specialForceMXPProcessorOn", false)
  step("hiddenFrameLineVisible", windowVisible, "csHiddenFrame")
  step("hiddenFrameLineEnable", enableCommandLine, "csHiddenFrame")
  step("hiddenFrameLineShown", windowVisible, "csHiddenFrame")
  setConfig("specialForceMXPProcessorOn", true)
  feedTriggers([[<FRAME Name="csLateFrame" Align="right" Width="20%" Height="30%">]] .. "\n")
  setWindow("csFrameHome", "csLateFrame", 0, 0, true)
  enableCommandLine("csLateFrame")
  feedTriggers([[<FRAME csLateFrame ACTION="close">]] .. "\n")
  step("lateFrameLineVisibleAtOnce", windowVisible, "csLateFrame")
  feedTriggers([[<FRAME Name="csHiddenConsole" Align="right" Width="20%" Height="30%">]] .. "\n")
  setWindow("csFrameHome", "csHiddenConsole", 0, 0, true)
  enableCommandLine("csHiddenConsole")
  hideWindow("csHiddenConsole")
  feedTriggers([[<FRAME csHiddenConsole ACTION="close">]] .. "\n")
  step("hiddenConsoleLineVisibleAtOnce", windowVisible, "csHiddenConsole")
  setConfig("specialForceMXPProcessorOn", false)
  hideWindow("csFrameHome")
  showWindow("csFrameHome")
  step("lateFrameLineVisible", windowVisible, "csLateFrame")
  step("hiddenConsoleLineVisible", windowVisible, "csHiddenConsole")
  -- Moving a frame's console into the main window takes it out of the frame, which hides it
  setConfig("specialForceMXPProcessorOn", true)
  feedTriggers([[<FRAME Name="csMainFrame" Align="right" Width="20%" Height="30%">]] .. "\n")
  enableCommandLine("csMainFrame")
  showWindow("csMainFrame")
  step("moveFrameToMain", setWindow, "main", "csMainFrame", 0, 0, false)
  step("frameInMainVisible", windowVisible, "csMainFrame")
  feedTriggers([[<FRAME csMainFrame ACTION="close">]] .. "\n")
  setConfig("specialForceMXPProcessorOn", false)
  step("frameInMainLineVisible", windowVisible, "csMainFrame")
  -- Moving a console's command line out of the console takes it out of the console's layer
  step("moveHiddenFrameLine", setWindow, "main", "csHiddenFrame", 0, 0, true)
  step("hiddenFrameLineMovedVisible", windowVisible, "csHiddenFrame")
  step("moveLateFrameLine", setWindow, "csFrameHome", "csLateFrame", 5, 5, false)
  step("lateFrameLineMovedVisible", windowVisible, "csLateFrame")
  -- An external frame is its console, which goes with it wherever it was moved
  setConfig("specialForceMXPProcessorOn", true)
  feedTriggers([[<FRAME Name="csExternal" EXTERNAL Width="20%" Height="30%">]] .. "\n")
  enableCommandLine("csExternal")
  setSaveCommandHistory("csExternal", false)
  step("moveExternal", setWindow, "csFrameHome", "csExternal", 0, 0, true)
  feedTriggers([[<FRAME csExternal ACTION="close">]] .. "\n")
  step("externalType", windowType, "csExternal")
  step("externalHistory", getSaveCommandHistory, "csExternal")
  -- An external frame is not made over a script's console of its name
  createMiniConsole("csClaimed", 0, 0, 50, 50)
  enableCommandLine("csClaimed")
  printCmdLine("csClaimed", "mine")
  feedTriggers([[<FRAME Name="csClaimed" EXTERNAL Width="20%" Height="30%">]] .. "\n")
  feedTriggers([[<FRAME csClaimed ACTION="close">]] .. "\n")
  setConfig("specialForceMXPProcessorOn", false)
  step("claimedType", windowType, "csClaimed")
  step("claimedText", getCmdLine, "csClaimed")
  step("claimedDisable", disableCommandLine, "csClaimed")
  step("claimedTextAfter", getCmdLine, "csClaimed")
  deleteMiniConsole("csClaimed")
  -- A command line a console's own one took the name from keeps its Lua function
  local held = setmetatable({}, {__mode = "k"})
  local action = function() end
  held[action] = true
  createCommandLine("csHeldAction", 0, 0, 50, 20)
  setCmdLineAction("csHeldAction", action)
  action = nil
  createMiniConsole("csHeldAction", 0, 0, 50, 50)
  enableCommandLine("csHeldAction")
  collectgarbage()
  collectgarbage()
  step("displacedAction", function()
    return next(held) and "kept" or "freed"
  end)
  deleteMiniConsole("csHeldAction")
  -- One deleted in an event loop run from a script goes as that loop delivers deferred deletes
  createCommandLine("csOuter", 0, 0, 50, 20)
  createCommandLine("csNested", 0, 30, 50, 20)
  deleteCommandLine("csOuter")
  local nestedFired, nestedOk, nestedError = false, false, nil
  -- Caught here, as a timer's own error goes to the error console and would read as one that never fired
  tempTimer(0, function()
    nestedOk, nestedError = pcall(function()
      setSaveCommandHistory("csNested", false)
      deleteCommandLine("csNested")
    end)
    nestedFired = true
  end)
  -- A loaded machine can fire the timer on a wait's last pass, which returns before delivering the delete
  for _ = 1, 200 do
    if nestedFired then
      break
    end
    pumpEvents(50)
  end
  if not nestedFired then
    error("the timer deleting csNested did not fire")
  end
  if not nestedOk then
    error("the timer deleting csNested raised: " .. tostring(nestedError))
  end
  pumpEvents(50)
  step("saveNested", save)
  step("iniNested", headlessCommandLineIni, "csNested")

  -- A console's own command line answers to the console's name, and is made the first time it is shown
  createMiniConsole("csMini", 0, 0, 100, 100)
  step("disableMini", disableCommandLine, "csMini")
  step("getMini", getCmdLine, "csMini")
  step("suggestMini", addCmdLineSuggestion, "csMini", "word")
  step("enableMini", enableCommandLine, "csMini")
  look("miniEnabled", "csMini")
  step("getMiniEnabled", getCmdLine, "csMini")
  step("printMini", printCmdLine, "csMini", "mini text")
  step("getMiniPrinted", getCmdLine, "csMini")
  step("suggestMiniEnabled", addCmdLineSuggestion, "csMini", "word")
  step("menuMini", addCommandLineMenuEvent, "csMini", "Item", "csEvent")
  step("historyMini", getSaveCommandHistory, "csMini")
  step("disableMiniEnabled", disableCommandLine, "csMini")
  look("miniDisabled", "csMini")
  step("getMiniDisabled", getCmdLine, "csMini")
  step("createOverMini", createCommandLine, "csMini", 0, 0, 10, 10)
  step("deleteMiniLine", deleteCommandLine, "csMini")
  step("getMiniLineDeleted", getCmdLine, "csMini")
  step("deleteMini", deleteMiniConsole, "csMini")
  step("enableMiniDeleted", enableCommandLine, "csMini")
  -- Until deferred deletes run a deleted console's command line still writes its setting on a save
  createMiniConsole("csGone", 0, 0, 100, 100)
  enableCommandLine("csGone")
  setSaveCommandHistory("csGone", false)
  deleteMiniConsole("csGone")
  step("saveGone", save)
  createMiniConsole("csGone", 0, 0, 100, 100)
  step("enableGoneRemade", enableCommandLine, "csGone")
  step("historyGoneRemade", getSaveCommandHistory, "csGone")
  deleteMiniConsole("csGone")
  -- A deleted console's command line is not made again until deferred deletes run
  createMiniConsole("csDying", 0, 0, 100, 100)
  enableCommandLine("csDying")
  deleteCommandLine("csDying")
  step("enableDying", enableCommandLine, "csDying")
  step("getDying", getCmdLine, "csDying")
  step("createOverDying", createCommandLine, "csDying", 5, 6, 50, 20)
  printCmdLine("csDying", "standalone")
  step("enableOverDying", enableCommandLine, "csDying")
  step("getOverDying", getCmdLine, "csDying")
  step("disableOverDying", disableCommandLine, "csDying")
  deleteMiniConsole("csDying")
  look("dyingMiniDeleted", "csDying")
  deleteCommandLine("csDying")
  -- A console made again under the name has none yet
  createMiniConsole("csDying", 0, 0, 100, 100)
  step("enableRemadeDying", enableCommandLine, "csDying")
  step("getRemadeDying", getCmdLine, "csDying")
  deleteMiniConsole("csDying")
  -- Hiding a console's command line it never showed leaves a command line of the same name alone
  createMiniConsole("csTwin", 0, 0, 100, 100)
  createCommandLine("csTwin", 5, 6, 50, 20)
  step("disableTwin", disableCommandLine, "csTwin")
  look("twinDisabled", "csTwin")
  deleteMiniConsole("csTwin")
  look("twinMiniDeleted", "csTwin")
  step("disableTwinLine", disableCommandLine, "csTwin")
  look("twinLineDisabled", "csTwin")
  deleteCommandLine("csTwin")
  -- Showing it takes the name from a command line of the same name
  createMiniConsole("csChat", 0, 0, 100, 100)
  createCommandLine("csChat", 5, 6, 50, 20)
  printCmdLine("csChat", "standalone")
  addCommandLineMenuEvent("csChat", "Item", "csEvent")
  step("enableChat", enableCommandLine, "csChat")
  step("getChat", getCmdLine, "csChat")
  step("unmenuChat", removeCommandLineMenuEvent, "csChat", "Item")
  step("deleteChatLine", deleteCommandLine, "csChat")
  step("getChatLineDeleted", getCmdLine, "csChat")
  deleteMiniConsole("csChat")
  look("chatDeleted", "csChat")
  -- A user window and a buffer are consoles too, a label is not
  step("disableWindow", disableCommandLine, "csWindow")
  step("enableWindow", enableCommandLine, "csWindow")
  step("getWindow", getCmdLine, "csWindow")
  look("windowEnabled", "csWindow")
  createBuffer("csBuffer")
  step("disableBuffer", disableCommandLine, "csBuffer")
  createLabel("csLabel", 0, 0, 10, 10, 1)
  step("disableLabel", disableCommandLine, "csLabel")
  deleteLabel("csLabel")

  deleteCommandLine("csLine")
  deleteCommandLine("csShared")
  deleteTextEdit("csEdit")
  closeUserWindow("csWindow")
  headlessCommandLineSettingsAnswers = table.concat(answers, "\n")

  -- The main command line
  answers = {}
  step("historyNoName", getSaveCommandHistory)
  step("historyMain", getSaveCommandHistory, "main")
  step("setHistoryNoName", setSaveCommandHistory, false)
  step("setHistoryMain", setSaveCommandHistory, "main", false)
  setSaveCommandHistory("main", true)
  step("suggestMain", addCmdLineSuggestion, "main", "word")
  step("suggestNoName", addCmdLineSuggestion, "", "word")
  step("blacklistMain", addCmdLineBlacklist, "main", "word")
  step("selectMain", selectCmdLineText, "main")
  step("menuMain", addCommandLineMenuEvent, "main", "Item", "csEvent")
  step("unmenuMain", removeCommandLineMenuEvent, "main", "Item")
  headlessCommandLineMainAnswers = table.concat(answers, "\n")
end)
headlessCommandLineSettings = ok and "ok" or tostring(err)
)lua"));
            pendingDeleteHeld = !pPending.isNull();
            loop.quit();
        });
        loop.exec();
        QVERIFY2(ran, "The Lua chunk did not run.");
        QVERIFY2(pendingDeleteHeld, "Waiting for a save delivered a deferred delete, which the GUI's waits do not.");
        QCOMPARE(luaGlobalString(host, "headlessCommandLineSettings"), qsl("ok"));
        const QStringList answers{qsl("create=[1]true"),
                                  qsl("suggest=[0]"),
                                  qsl("suggestAgain=[0]"),
                                  qsl("unsuggest=[0]"),
                                  qsl("unsuggestAbsent=[0]"),
                                  qsl("clearSuggestions=[0]"),
                                  qsl("suggestMissing=[2]nil:command line \"csNoSuch\" not found"),
                                  qsl("unsuggestMissing=[2]nil:command line \"csNoSuch\" not found"),
                                  qsl("clearSuggestionsMissing=[2]nil:command line \"csNoSuch\" not found"),
                                  qsl("suggestEmpty=[0]"),
                                  qsl("suggestBadText=raised:addCmdLineSuggestion: bad argument #2 type (suggestion text as string expected, got table!)"),
                                  qsl("blacklist=[0]"),
                                  qsl("unblacklist=[0]"),
                                  qsl("unblacklistAbsent=[0]"),
                                  qsl("clearBlacklist=[0]"),
                                  qsl("blacklistMissing=[2]nil:command line \"csNoSuch\" not found"),
                                  qsl("unblacklistMissing=[2]nil:command line \"csNoSuch\" not found"),
                                  qsl("clearBlacklistMissing=[2]nil:command line \"csNoSuch\" not found"),
                                  qsl("menu=[1]true"),
                                  qsl("menuAgain=[1]true"),
                                  qsl("menuOther=[1]true"),
                                  qsl("unmenu=[1]true"),
                                  qsl("unmenuAgain=[2]nil:cannot remove 'Item', menu item does not exist"),
                                  qsl("unmenuOther=[1]true"),
                                  qsl("menuMissing=[2]nil:command line \"csNoSuch\" not found"),
                                  qsl("unmenuMissing=[2]nil:command line \"csNoSuch\" not found"),
                                  qsl("select=[1]true"),
                                  qsl("selectText=[1]true"),
                                  qsl("textAfterSelect=[1]some text"),
                                  qsl("selectMissing=[2]nil:command line \"csNoSuch\" not found"),
                                  qsl("history=[2]true:enabled (500 lines will be saved)"),
                                  qsl("historyOff=[1]true"),
                                  qsl("historyAfterOff=[2]false:disabled"),
                                  qsl("historyOn=[1]true"),
                                  qsl("historyAfterOn=[2]true:enabled (500 lines will be saved)"),
                                  qsl("historyOffMissing=[2]nil:command line \"csNoSuch\" not found"),
                                  qsl("historyMissing=[2]nil:command line \"csNoSuch\" not found"),
                                  qsl("disable=[1]true"),
                                  qsl("disabled:type=[1]commandline,visible=[1]false"),
                                  qsl("disableAgain=[1]true"),
                                  qsl("enable=[1]true"),
                                  qsl("enabled:type=[1]commandline,visible=[1]true"),
                                  qsl("disableMissing=[2]nil:command line \"csNoSuch\" not found"),
                                  qsl("enableMissing=[2]nil:command line \"csNoSuch\" not found"),
                                  qsl("disableMain=[2]nil:this function is not permitted on the main command line"),
                                  qsl("enableEmpty=[2]nil:this function is not permitted on the main command line"),
                                  qsl("enableHidden=[1]true"),
                                  qsl("enabledAfterHide:type=[1]commandline,visible=[1]true"),
                                  qsl("disableShown=[1]true"),
                                  qsl("showDisabled=[1]true"),
                                  qsl("shownAfterDisable:type=[1]commandline,visible=[1]true"),
                                  qsl("enableInHidden=[1]true"),
                                  qsl("enabledInHidden:type=[1]commandline,visible=[1]false"),
                                  qsl("windowShown:type=[1]commandline,visible=[1]true"),
                                  qsl("disableShared=[1]true"),
                                  qsl("sharedDisabled:type=[1]scrollbox,visible=[1]true"),
                                  qsl("menuShared=[1]true"),
                                  qsl("historyShared=[1]true"),
                                  qsl("historySharedAfter=[2]false:disabled"),
                                  qsl("sharedBoxDeleted:type=[1]commandline,visible=[1]false"),
                                  qsl("unmenuShared=[1]true"),
                                  qsl("suggestEdit=[2]nil:command line \"csEdit\" not found"),
                                  qsl("menuEdit=[2]nil:command line \"csEdit\" not found"),
                                  qsl("unmenuEdit=[2]nil:command line \"csEdit\" not found"),
                                  qsl("selectEdit=[2]nil:command line \"csEdit\" not found"),
                                  qsl("historyEdit=[2]nil:command line \"csEdit\" not found"),
                                  qsl("enableEdit=[2]nil:command line \"csEdit\" not found"),
                                  qsl("disableEdit=[2]nil:command line \"csEdit\" not found"),
                                  qsl("editAfterDisable:type=[1]textedit,visible=[1]true"),
                                  qsl("suggestDeleted=[2]nil:command line \"csLine\" not found"),
                                  qsl("blacklistDeleted=[2]nil:command line \"csLine\" not found"),
                                  qsl("unmenuDeleted=[2]nil:command line \"csLine\" not found"),
                                  qsl("selectDeleted=[2]nil:command line \"csLine\" not found"),
                                  qsl("historyDeleted=[2]nil:command line \"csLine\" not found"),
                                  qsl("getHistoryDeleted=[2]nil:command line \"csLine\" not found"),
                                  qsl("enableDeleted=[2]nil:command line \"csLine\" not found"),
                                  qsl("unmenuRecreated=[2]nil:cannot remove 'Kept', menu item does not exist"),
                                  qsl("historyRecreated=[2]true:enabled (500 lines will be saved)"),
                                  qsl("save=[1]true"),
                                  qsl("historyAfterSave=[2]false:disabled"),
                                  qsl("historyNotSaved=[2]false:disabled"),
                                  qsl("saveTrue=[1]true"),
                                  qsl("historyAfterSavedTrue=[2]true:enabled (500 lines will be saved)"),
                                  qsl("saveElsewhere=[1]true"),
                                  qsl("historyAfterSaveElsewhere=[2]true:enabled (500 lines will be saved)"),
                                  qsl("saveAfterDelete=[1]true"),
                                  qsl("historyDeletedBeforeSave=[2]false:disabled"),
                                  qsl("createOrder=[1]true"),
                                  qsl("enableOrder=[1]true"),
                                  qsl("saveOrder=[1]true"),
                                  qsl("iniOrder=[1]true"),
                                  qsl("saveKey=[1]true"),
                                  qsl("iniKey=[1]true"),
                                  qsl("enableFrame=[1]true"),
                                  qsl("frameClosed=[2]nil:'csFrame' is not a known label, any type of console, command line, text edit, nor scroll box"),
                                  qsl("saveFrame=[1]true"),
                                  qsl("iniFrame=[1]true"),
                                  qsl("saveFrames=[1]true"),
                                  qsl("iniFrameLine=[1]true"),
                                  qsl("iniFrameConsole=[1]true"),
                                  qsl("moveFrameConsole=[1]true"),
                                  qsl("movedFrameType=[1]commandline"),
                                  qsl("movedFrameHistory=[2]false:disabled"),
                                  qsl("movedFrameText=[1]"),
                                  qsl("movedFrameVisible=[1]true"),
                                  qsl("hiddenFrameLineVisible=[1]false"),
                                  qsl("hiddenFrameLineEnable=[1]true"),
                                  qsl("hiddenFrameLineShown=[1]false"),
                                  qsl("lateFrameLineVisibleAtOnce=[1]true"),
                                  qsl("hiddenConsoleLineVisibleAtOnce=[1]false"),
                                  qsl("lateFrameLineVisible=[1]true"),
                                  qsl("hiddenConsoleLineVisible=[1]false"),
                                  qsl("moveFrameToMain=[1]true"),
                                  qsl("frameInMainVisible=[1]false"),
                                  qsl("frameInMainLineVisible=[1]false"),
                                  qsl("moveHiddenFrameLine=[1]true"),
                                  qsl("hiddenFrameLineMovedVisible=[1]true"),
                                  qsl("moveLateFrameLine=[1]true"),
                                  qsl("lateFrameLineMovedVisible=[1]false"),
                                  qsl("moveExternal=[1]true"),
                                  qsl("externalType=[2]nil:'csExternal' is not a known label, any type of console, command line, text edit, nor scroll box"),
                                  qsl("externalHistory=[2]nil:command line \"csExternal\" not found"),
                                  qsl("claimedType=[1]miniconsole"),
                                  qsl("claimedText=[1]mine"),
                                  qsl("claimedDisable=[1]true"),
                                  qsl("claimedTextAfter=[1]mine"),
                                  qsl("displacedAction=[1]kept"),
                                  qsl("saveNested=[1]true"),
                                  qsl("iniNested=[1]true"),
                                  qsl("disableMini=[1]true"),
                                  qsl("getMini=[2]nil:command line \"csMini\" not found"),
                                  qsl("suggestMini=[2]nil:command line \"csMini\" not found"),
                                  qsl("enableMini=[1]true"),
                                  qsl("miniEnabled:type=[1]miniconsole,visible=[1]true"),
                                  qsl("getMiniEnabled=[1]"),
                                  qsl("printMini=[0]"),
                                  qsl("getMiniPrinted=[1]mini text"),
                                  qsl("suggestMiniEnabled=[0]"),
                                  qsl("menuMini=[1]true"),
                                  qsl("historyMini=[2]true:enabled (500 lines will be saved)"),
                                  qsl("disableMiniEnabled=[1]true"),
                                  qsl("miniDisabled:type=[1]miniconsole,visible=[1]true"),
                                  qsl("getMiniDisabled=[1]mini text"),
                                  qsl("createOverMini=[2]false:couldn't create commandLine"),
                                  qsl("deleteMiniLine=[1]true"),
                                  qsl("getMiniLineDeleted=[2]nil:command line \"csMini\" not found"),
                                  qsl("deleteMini=[1]true"),
                                  qsl("enableMiniDeleted=[2]nil:command line \"csMini\" not found"),
                                  qsl("saveGone=[1]true"),
                                  qsl("enableGoneRemade=[1]true"),
                                  qsl("historyGoneRemade=[2]false:disabled"),
                                  qsl("enableDying=[1]true"),
                                  qsl("getDying=[2]nil:command line \"csDying\" not found"),
                                  qsl("createOverDying=[1]true"),
                                  qsl("enableOverDying=[1]true"),
                                  qsl("getOverDying=[1]standalone"),
                                  qsl("disableOverDying=[1]true"),
                                  qsl("dyingMiniDeleted:type=[1]commandline,visible=[1]true"),
                                  qsl("enableRemadeDying=[1]true"),
                                  qsl("getRemadeDying=[1]"),
                                  qsl("disableTwin=[1]true"),
                                  qsl("twinDisabled:type=[1]miniconsole,visible=[1]true"),
                                  qsl("twinMiniDeleted:type=[1]commandline,visible=[1]true"),
                                  qsl("disableTwinLine=[1]true"),
                                  qsl("twinLineDisabled:type=[1]commandline,visible=[1]false"),
                                  qsl("enableChat=[1]true"),
                                  qsl("getChat=[1]"),
                                  qsl("unmenuChat=[2]nil:cannot remove 'Item', menu item does not exist"),
                                  qsl("deleteChatLine=[1]true"),
                                  qsl("getChatLineDeleted=[2]nil:command line \"csChat\" not found"),
                                  qsl("chatDeleted:type=[2]nil:'csChat' is not a known label, any type of console, command line, text edit, nor scroll box,visible=[2]nil:window \"csChat\" not found"),
                                  qsl("disableWindow=[1]true"),
                                  qsl("enableWindow=[1]true"),
                                  qsl("getWindow=[1]"),
                                  qsl("windowEnabled:type=[1]userwindow,visible=[1]true"),
                                  qsl("disableBuffer=[1]true"),
                                  qsl("disableLabel=[2]nil:command line \"csLabel\" not found")};
        QCOMPARE(luaGlobalString(host, "headlessCommandLineSettingsAnswers"), answers.join(QLatin1Char('\n')));
        const QStringList mainAnswers{qsl("historyNoName=[2]true:enabled (500 lines will be saved)"),
                                      qsl("historyMain=[2]true:enabled (500 lines will be saved)"),
                                      qsl("setHistoryNoName=[1]true"),
                                      qsl("setHistoryMain=[1]true"),
                                      qsl("suggestMain=[0]"),
                                      qsl("suggestNoName=[0]"),
                                      qsl("blacklistMain=[0]"),
                                      qsl("selectMain=[1]true"),
                                      qsl("menuMain=[1]true"),
                                      qsl("unmenuMain=[1]true")};
        QCOMPARE(luaGlobalString(host, "headlessCommandLineMainAnswers"), mainAnswers.join(QLatin1Char('\n')));

        // A command line removed before deferred deletes run still writes its setting, as its TCommandLine
        // does until then, and one removed after does not
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        ran = host->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(
createCommandLine("csLine", 10, 20, 200, 30)
setSaveCommandHistory("csLine", true)
headlessCommandLineSave()
setSaveCommandHistory("csLine", false)
deleteCommandLine("csLine")
createCommandLine("csSavedLater", 0, 0, 50, 20)
setSaveCommandHistory("csSavedLater", false)
)lua"));
        QVERIFY2(ran, "The Lua chunk deleting the command line did not run.");
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        ran = host->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(
headlessCommandLineSave()
createCommandLine("csLine", 10, 20, 200, 30)
headlessCommandLineDeferred = tostring(getSaveCommandHistory("csLine"))
deleteCommandLine("csLine")
)lua"));
        QVERIFY2(ran, "The Lua chunk after the deferred deletes did not run.");
        QCOMPARE(luaGlobalString(host, "headlessCommandLineDeferred"), qsl("true"));
        QCOMPARE(host->readProfileIniData(qsl("CommandLines/SaveHistory/csSavedLater")), qsl("false"));

        // A console's own command line goes with the console, even one moved out of a closed frame, and one in a
        // deleted user window writes its setting until deferred deletes run
        QTRY_VERIFY(!host->currentlySavingProfile());
        ran = host->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(
createMiniConsole("csGone", 0, 0, 100, 100)
enableCommandLine("csGone")
deleteMiniConsole("csGone")
createMiniConsole("csRevived", 0, 0, 100, 100)
enableCommandLine("csRevived")
deleteCommandLine("csRevived")
openUserWindow("csGoneWindow", false, false, "f")
createCommandLine("csGoneWindow", "csInGoneWindow", 0, 0, 50, 20)
setSaveCommandHistory("csInGoneWindow", false)
deleteMiniConsole("csGoneWindow")
setConfig("specialForceMXPProcessorOn", true)
openUserWindow("csOrphanHome", false, false, "f")
feedTriggers([[<FRAME Name="csOrphanA" Align="right" Width="20%" Height="30%">]] .. "\n")
enableCommandLine("csOrphanA")
setWindow("csOrphanHome", "csOrphanA", 0, 0, true)
feedTriggers([[<FRAME Name="csOrphanB" Align="right" Width="20%" Height="30%">]] .. "\n")
setWindow("csOrphanHome", "csOrphanB", 0, 0, true)
enableCommandLine("csOrphanB")
feedTriggers([[<FRAME csOrphanA ACTION="close">]] .. "\n")
feedTriggers([[<FRAME csOrphanB ACTION="close">]] .. "\n")
setConfig("specialForceMXPProcessorOn", false)
deleteMiniConsole("csOrphanHome")
headlessCommandLineSave()
)lua"));
        QVERIFY2(ran, "The Lua chunk deleting the consoles did not run.");
        QTRY_VERIFY(!host->currentlySavingProfile());
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        ran = host->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(
headlessCommandLineGone = tostring(getCmdLine("csGone")) .. ":" .. tostring(windowType("csGone"))
headlessCommandLineOrphans = tostring(windowType("csOrphanA")) .. ":" .. tostring(windowType("csOrphanB"))
enableCommandLine("csRevived")
headlessCommandLineRevived = tostring(getCmdLine("csRevived"))
deleteMiniConsole("csRevived")
createCommandLine("csInGoneWindow", 0, 0, 50, 20)
headlessCommandLineRemade = tostring(getSaveCommandHistory("csInGoneWindow"))
deleteCommandLine("csInGoneWindow")
)lua"));
        QVERIFY2(ran, "The Lua chunk after the consoles went did not run.");
        QCOMPARE(luaGlobalString(host, "headlessCommandLineGone"), qsl("nil:nil"));
        QCOMPARE(luaGlobalString(host, "headlessCommandLineOrphans"), qsl("nil:nil"));
        QCOMPARE(luaGlobalString(host, "headlessCommandLineRevived"), QString());
        QCOMPARE(luaGlobalString(host, "headlessCommandLineRemade"), qsl("false"));

        // A command line a console's own one took the name from goes on writing its setting, and a text edit of
        // the name reads as itself again once the console goes
        ran = host->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(
createMiniConsole("csDisplaced", 0, 0, 100, 100)
createCommandLine("csDisplaced", 5, 5, 50, 20)
setSaveCommandHistory("csDisplaced", false)
enableCommandLine("csDisplaced")
deleteMiniConsole("csDisplaced")
createTextEdit("csShadowed", 1, 2, 50, 50)
createMiniConsole("csShadowed", 0, 0, 100, 100)
enableCommandLine("csShadowed")
disableCommandLine("csShadowed")
openUserWindow("csShadowWindow", false, false, "f")
hideWindow("csShadowWindow")
showWindow("csShadowWindow")
deleteMiniConsole("csShadowed")
)lua"));
        QVERIFY2(ran, "The Lua chunk displacing the command line did not run.");
        QTRY_VERIFY(!host->currentlySavingProfile());
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        ran = host->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(
headlessCommandLineSave()
headlessCommandLineShadowed = table.concat({tostring(windowType("csShadowed")), tostring(windowVisible("csShadowed")), table.concat({getWindowGeometry("csShadowed")}, ",")}, ":")
deleteTextEdit("csShadowed")
)lua"));
        QVERIFY2(ran, "The Lua chunk after the console went did not run.");
        QTRY_VERIFY(!host->currentlySavingProfile());
        QCOMPARE(host->readProfileIniData(qsl("CommandLines/SaveHistory/csDisplaced")), qsl("false"));
        QCOMPARE(luaGlobalString(host, "headlessCommandLineShadowed"), qsl("textedit:true:1,2,50,50"));

        // Once deferred deletes run, a command line in a deleted window writes nothing more, wherever it was
        // moved, and so does one a console's own one took the name from
        ran = host->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(
createScrollBox("csBoxed", 0, 0, 100, 100)
createMiniConsole("csBoxed", "csBoxedMini", 0, 0, 50, 50)
enableCommandLine("csBoxedMini")
setSaveCommandHistory("csBoxedMini", false)
createCommandLine("csBoxed", "csBoxedLine", 0, 60, 50, 20)
setSaveCommandHistory("csBoxedLine", false)
deleteScrollBox("csBoxed")
createScrollBox("csMoved", 0, 0, 100, 100)
createCommandLine("csMoved", "csMovedLine", 0, 0, 50, 20)
setSaveCommandHistory("csMovedLine", false)
createMiniConsole("csMovedLine", 0, 0, 50, 50)
enableCommandLine("csMovedLine")
openUserWindow("csMovedWindow", false, false, "f")
setWindow("csMovedWindow", "csMoved", 0, 0, true)
deleteMiniConsole("csMovedLine")
deleteMiniConsole("csMovedWindow")
createMiniConsole("csOwnGone", 0, 0, 50, 50)
enableCommandLine("csOwnGone")
setSaveCommandHistory("csOwnGone", false)
deleteMiniConsole("csOwnGone")
setSaveCommandHistory("csSavedLater", true)
)lua"));
        QVERIFY2(ran, "The Lua chunk deleting the windows did not run.");
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        ran = host->getLuaInterpreter()->compileAndExecuteScript(qsl("headlessCommandLineSave()"));
        QVERIFY2(ran, "The save after the windows went did not run.");
        QTRY_VERIFY(!host->currentlySavingProfile());
        for (const QString& name : {qsl("csBoxedMini"), qsl("csBoxedLine"), qsl("csMovedLine"), qsl("csOwnGone"), qsl("csSavedLater")}) {
            QCOMPARE(host->readProfileIniData(qsl("CommandLines/SaveHistory/%1").arg(name)), qsl("true"));
        }

        // A frame nested in another goes with it as deferred deletes run, with its command line
        TMxpFrameFrontend& frames = host->consoleFrontend()->mxpFrames();
        QVERIFY(frames.createExternalFrame(qsl("csHostFrame"), qsl("Host"), QSize(200, 200), true));
        frames.createInternalFrame(qsl("csNestedFrame"), qsl("csHostFrame"), qsl("Nested"), QRect(0, 0, 100, 100), false, true);
        ran = host->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(
headlessCommandLineNested = tostring(windowType("csNestedFrame"))
enableCommandLine("csNestedFrame")
setSaveCommandHistory("csNestedFrame", false)
deleteMiniConsole("csHostFrame")
setSaveCommandHistory("csSavedLater", false)
)lua"));
        QVERIFY2(ran, "The Lua chunk deleting the host frame did not run.");
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        QTRY_VERIFY(!host->currentlySavingProfile());
        ran = host->getLuaInterpreter()->compileAndExecuteScript(
                qsl(R"lua(headlessCommandLineNested = headlessCommandLineNested .. ":" .. tostring(windowType("csNestedFrame")) headlessCommandLineSave())lua"));
        QVERIFY2(ran, "The save after the host frame went did not run.");
        QTRY_VERIFY(!host->currentlySavingProfile());
        QCOMPARE(luaGlobalString(host, "headlessCommandLineNested"), qsl("miniconsole:nil"));
        QCOMPARE(host->readProfileIniData(qsl("CommandLines/SaveHistory/csNestedFrame")), qsl("true"));
        QCOMPARE(host->readProfileIniData(qsl("CommandLines/SaveHistory/csSavedLater")), qsl("false"));
        // An external frame's widget is its console, which a script's deletion leaves until deferred deletes run
        QVERIFY(frames.createExternalFrame(qsl("csDeletedFrame"), qsl("Deleted"), QSize(200, 200), true));
        QVERIFY(host->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(deleteMiniConsole("csDeletedFrame"))lua")));
        QVERIFY2(frames.hasFrameWidget(qsl("csDeletedFrame")), "A deleted external frame's console went before deferred deletes ran.");
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        QVERIFY2(!frames.hasFrameWidget(qsl("csDeletedFrame")), "A deleted external frame's console outlived deferred deletes.");
        // Closing a frame deletes a frame nested in it at once, with its command line
        frames.createInternalFrame(qsl("csOuterFrame"), QString(), qsl("Outer"), QRect(0, 0, 200, 200), false, true);
        frames.createInternalFrame(qsl("csInnerFrame"), qsl("csOuterFrame"), qsl("Inner"), QRect(0, 0, 100, 100), false, true);
        QVERIFY(host->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(
enableCommandLine("csInnerFrame")
setSaveCommandHistory("csInnerFrame", false)
setSaveCommandHistory("csSavedLater", true)
)lua")));
        frames.destroyFrame(qsl("csOuterFrame"));
        QTRY_VERIFY(!host->currentlySavingProfile());
        QVERIFY(host->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(
headlessCommandLineInner = tostring(windowType("csInnerFrame")) .. ":" .. tostring(getCmdLine("csInnerFrame"))
headlessCommandLineSave()
)lua")));
        QTRY_VERIFY(!host->currentlySavingProfile());
        QCOMPARE(luaGlobalString(host, "headlessCommandLineInner"), qsl("nil:nil"));
        QCOMPARE(host->readProfileIniData(qsl("CommandLines/SaveHistory/csInnerFrame")), qsl("true"));
        QCOMPARE(host->readProfileIniData(qsl("CommandLines/SaveHistory/csSavedLater")), qsl("true"));
        // Nothing is built in a frame that has no widget
        frames.createInternalFrame(qsl("csHostlessFrame"), qsl("csDeletedFrame"), qsl("Hostless"), QRect(0, 0, 100, 100), false, true);
        QVERIFY(host->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(headlessCommandLineHostless = tostring(windowType("csHostlessFrame")))lua")));
        QCOMPARE(luaGlobalString(host, "headlessCommandLineHostless"), qsl("nil"));

        // A command line named "main" writes the main command line's key with the setting it was made with, as
        // in the GUI
        host->writeProfileIniData(qsl("CommandLines/SaveHistory/main"), qsl("false"));
        ran = host->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(headlessCommandLineMainMade = tostring(createCommandLine("main", 0, 0, 10, 10)))lua"));
        QVERIFY2(ran, "The Lua chunk making a command line named main did not run.");
        QCOMPARE(luaGlobalString(host, "headlessCommandLineMainMade"), qsl("true"));
        host->writeProfileIniData(qsl("CommandLines/SaveHistory/main"), qsl("true"));
        QTRY_VERIFY(!host->currentlySavingProfile());
        QVERIFY2(host->getLuaInterpreter()->compileAndExecuteScript(qsl("headlessCommandLineSave()")), "The save with a command line named main failed.");
        QCOMPARE(host->readProfileIniData(qsl("CommandLines/SaveHistory/main")), qsl("false"));

        // TMainConsole::resetMainConsole() deletes the command lines and scroll boxes, but not a command line
        // displaced in the main window, which goes on writing its setting
        ran = host->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(
createCommandLine("csResetLine", 0, 0, 50, 20)
setSaveCommandHistory("csResetLine", false)
createScrollBox("csResetBox", 0, 0, 100, 100)
createCommandLine("csResetBox", "csResetBoxed", 0, 0, 50, 20)
setSaveCommandHistory("csResetBoxed", false)
createMiniConsole("csResetBoxed", 0, 0, 50, 50)
enableCommandLine("csResetBoxed")
deleteMiniConsole("csResetBoxed")
createMiniConsole("csReset", 0, 0, 100, 100)
createCommandLine("csReset", 5, 5, 50, 20)
setSaveCommandHistory("csReset", false)
enableCommandLine("csReset")
deleteMiniConsole("csReset")
)lua"));
        QVERIFY2(ran, "The Lua chunk displacing a command line before the reset did not run.");
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        QTRY_VERIFY(!host->currentlySavingProfile());
        QVERIFY2(host->resetProfile_phase1(), "The profile reset did not start.");
        QTRY_VERIFY(!host->profileResetInProgress());
        QTRY_VERIFY(!host->currentlySavingProfile());
        ran = host->getLuaInterpreter()->compileAndExecuteScript(qsl("assert(saveProfile())"));
        QVERIFY2(ran, "The save after the reset failed.");
        QTRY_VERIFY(!host->currentlySavingProfile());
        QCOMPARE(host->readProfileIniData(qsl("CommandLines/SaveHistory/csReset")), qsl("false"));
        QCOMPARE(host->readProfileIniData(qsl("CommandLines/SaveHistory/csResetLine")), qsl("true"));
        QCOMPARE(host->readProfileIniData(qsl("CommandLines/SaveHistory/csResetBoxed")), qsl("true"));
        QVERIFY2(QApplication::topLevelWidgets().isEmpty(), "Using the command lines created a widget.");
        HostManager::self()->deleteHost(hostname);
    }

    // The answers are those of the same Lua run in the GUI self-test profile
    void test_headlessCommandLineStyleSheetContract()
    {
        const QString hostname = qsl("Test Headless Host Command Line Style");
        QVERIFY2(HostManager::self()->addHost(hostname, QString(), QString(), QString()), "Could not create a profile with no main window.");
        Host* host = HostManager::self()->getHost(hostname);
        QVERIFY2(host, "The profile is not in the pool.");
        QVERIFY(!host->hasConsoleView());
        const bool ran = host->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(
headlessCmdLineStyle = "not run"
local ok, err = pcall(function()
  local function described(called, ...)
    if not called then
      return "raised:" .. tostring((...))
    end
    local count = select("#", ...)
    local values = {}
    for i = 1, count do
      values[i] = tostring((select(i, ...)))
    end
    return "[" .. count .. "]" .. table.concat(values, ":")
  end
  local answers = {}
  local function step(label, f, ...)
    answers[#answers + 1] = label .. "=" .. described(pcall(f, ...))
  end
  local function look(label, name)
    answers[#answers + 1] = label .. "=" .. described(pcall(getWindowGeometry, name))
  end
  local profileId = (getProfileName():gsub(" ", "\\ "))
  local function line(key, sheet, w, h)
    local name = "cs" .. key
    createCommandLine(name, 0, 0, w or 10, h or 10)
    local parts = {described(pcall(setCmdLineStyleSheet, name, sheet)), described(pcall(getCmdLineStyleSheet, name)), described(pcall(getWindowGeometry, name))}
    resizeWindow(name, 5, 500)
    parts[#parts + 1] = described(pcall(getWindowGeometry, name))
    resizeWindow(name, 500, 5)
    parts[#parts + 1] = described(pcall(getWindowGeometry, name))
    answers[#answers + 1] = key .. "=" .. table.concat(parts, " ")
  end
  createCommandLine("csFresh", 0, 0, 10, 10)
  step("fresh.sheet", getCmdLineStyleSheet, "csFresh")
  line("min", "min-width: 50px; min-height: 30px")
  line("max", "max-width: 20px; max-height: 8px", 100, 100)
  line("padded", "min-width: 50px; padding: 2px; border: 3px solid", 1, 1)
  line("width", "width: 100px; min-width: 50px", 1, 1)
  line("rule", "QPlainTextEdit { min-width: 60px }", 1, 1)
  line("ownRule", "TCommandLine { min-height: 60px }", 1, 1)
  line("textBoxRule", "TTextBox { min-height: 60px }", 1, 1)
  line("scrollRule", "QAbstractScrollArea { min-height: 61px }", 1, 1)
  line("frameRule", "QFrame { min-width: 62px }", 1, 1)
  line("labelRule", "QLabel { min-width: 60px }", 1, 1)
  line("native", "border-style: native; min-width: 50px", 1, 1)
  line("nativeWidth", "border: 4px native; min-width: 50px", 1, 1)
  line("solid", "border: 2px solid; min-width: 50px", 1, 1)
  line("none", "border: none; min-width: 50px", 1, 1)
  line("radius", "border-radius: 2px; min-width: 50px", 1, 1)
  line("em", "font-size: 20px; min-width: 2em", 1, 1)
  line("editable", "QPlainTextEdit:editable { min-width: 60px }", 1, 1)
  line("readOnly", "QPlainTextEdit:read-only { min-width: 60px }", 1, 1)
  line("notReadOnly", "QPlainTextEdit:!read-only { min-width: 60px }", 1, 1)
  line("active", "QPlainTextEdit:active { min-width: 60px }", 1, 1)
  line("inactive", "QPlainTextEdit:!active { padding: 2px; min-width: 50px }", 1, 1)
  line("focus", "QPlainTextEdit:focus { min-width: 60px }", 1, 1)
  line("named", "#commandLine_" .. profileId .. "_csnamed { min-width: 60px }", 1, 1)
  line("otherNamed", "#commandLine_" .. profileId .. "_csother { min-width: 60px }", 1, 1)
  line("textEditNamed", "#textEdit_" .. profileId .. "_cstextEditNamed { min-width: 60px }", 1, 1)
  line("star", "* { min-width: 60px }", 1, 1)
  line("background", "QPlainTextEdit { background-color: red; }", 1, 1)
  line("minOverMax", "min-width: 50px; max-width: 20px; min-height: 9px; max-height: 40px", 30, 30)
  -- Clearing a sheet keeps the size and lets it go
  line("cleared", "min-width: 50px; max-height: 8px", 1, 100)
  step("cleared.clear", setCmdLineStyleSheet, "cscleared", "")
  step("cleared.sheet", getCmdLineStyleSheet, "cscleared")
  look("cleared.after", "cscleared")
  resizeWindow("cscleared", 5, 500)
  look("cleared.free", "cscleared")
  -- Missing, deleted and odd names
  step("missing", setCmdLineStyleSheet, "csMissing", "min-width: 50px")
  step("missing.get", getCmdLineStyleSheet, "csMissing")
  createCommandLine("csGone", 0, 0, 10, 10)
  deleteCommandLine("csGone")
  step("gone", setCmdLineStyleSheet, "csGone", "min-width: 50px")
  step("gone.get", getCmdLineStyleSheet, "csGone")
  step("number", setCmdLineStyleSheet, "csFresh", 5)
  step("number.get", getCmdLineStyleSheet, "csFresh")
  step("noSheet", setCmdLineStyleSheet, "csFresh", nil)
  createLabel("csLabel", 0, 0, 10, 10, 1)
  step("labelName", setCmdLineStyleSheet, "csLabel", "x")
  deleteLabel("csLabel")
  -- In a user window and a scroll box, and moved between them
  openUserWindow("csWindow", false, false, "f")
  createCommandLine("csWindow", "csInWindow", 0, 0, 1, 1)
  step("inWindow", setCmdLineStyleSheet, "csInWindow", "min-width: 40px")
  look("inWindow", "csInWindow")
  createScrollBox("csBox", 0, 0, 100, 100)
  createCommandLine("csBox", "csInBox", 0, 0, 1, 1)
  step("inBox", setCmdLineStyleSheet, "csInBox", "min-width: 41px")
  look("inBox", "csInBox")
  step("inBox.moveBox", setWindow, "csWindow", "csBox", 0, 0, true)
  resizeWindow("csInBox", 5, 5)
  look("inBox.box", "csInBox")
  step("inBox.moveOut", setWindow, "main", "csInBox", 3, 3, true)
  resizeWindow("csInBox", 5, 5)
  look("inBox.out", "csInBox")
  step("inBox.get", getCmdLineStyleSheet, "csInBox")
  -- Hidden
  createCommandLine("csHidden", 0, 0, 1, 1)
  hideWindow("csHidden")
  step("hidden", setCmdLineStyleSheet, "csHidden", "min-width: 42px")
  look("hidden", "csHidden")
  -- Sharing a name with a scroll box or a text edit
  createScrollBox("csSharedBox", 0, 0, 30, 30)
  createCommandLine("csSharedBox", 0, 0, 1, 1)
  step("sharedBox", setCmdLineStyleSheet, "csSharedBox", "min-width: 50px")
  step("sharedBox.get", getCmdLineStyleSheet, "csSharedBox")
  look("sharedBox", "csSharedBox")
  deleteScrollBox("csSharedBox")
  look("sharedBox.alone", "csSharedBox")
  resizeWindow("csSharedBox", 5, 5)
  look("sharedBox.resized", "csSharedBox")
  createTextEdit("csSharedEdit", 0, 0, 1, 1)
  createCommandLine("csSharedEdit", 0, 0, 2, 2)
  step("sharedEdit", setCmdLineStyleSheet, "csSharedEdit", "min-width: 50px")
  look("sharedEdit", "csSharedEdit")
  deleteCommandLine("csSharedEdit")
  look("sharedEdit.edit", "csSharedEdit")
  createCommandLine("csSharedEdit2", 0, 0, 2, 2)
  createTextEdit("csSharedEdit2", 0, 0, 1, 1)
  setTextEditStyleSheet("csSharedEdit2", "min-width: 70px")
  step("sharedEdit2", setCmdLineStyleSheet, "csSharedEdit2", "min-height: 30px")
  look("sharedEdit2", "csSharedEdit2")
  -- A console's own command line
  createMiniConsole("csMini", 0, 0, 100, 100)
  step("mini.before", setCmdLineStyleSheet, "csMini", "min-height: 50px")
  enableCommandLine("csMini")
  step("mini.sheet", getCmdLineStyleSheet, "csMini")
  step("mini.set", setCmdLineStyleSheet, "csMini", "min-height: 300px; min-width: 300px")
  step("mini.get", getCmdLineStyleSheet, "csMini")
  disableCommandLine("csMini")
  step("mini.disabled", setCmdLineStyleSheet, "csMini", "min-height: 30px")
  step("mini.disabledGet", getCmdLineStyleSheet, "csMini")
  step("mini.reenabled", enableCommandLine, "csMini")
  step("mini.reenabledGet", getCmdLineStyleSheet, "csMini")
  createCommandLine("csDisplaced", 0, 0, 1, 1)
  setCmdLineStyleSheet("csDisplaced", "min-width: 50px")
  createMiniConsole("csDisplaced", 0, 0, 100, 100)
  enableCommandLine("csDisplaced")
  step("displaced.get", getCmdLineStyleSheet, "csDisplaced")
  openUserWindow("csOwnWindow", false, false, "f")
  enableCommandLine("csOwnWindow")
  step("window.set", setCmdLineStyleSheet, "csOwnWindow", "min-height: 30px")
  step("window.get", getCmdLineStyleSheet, "csOwnWindow")
  closeUserWindow("csOwnWindow")
  createBuffer("csBuffer")
  enableCommandLine("csBuffer")
  step("buffer.set", setCmdLineStyleSheet, "csBuffer", "color: red")
  step("buffer.get", getCmdLineStyleSheet, "csBuffer")
  for key in ("Fresh min max padded width rule ownRule textBoxRule scrollRule frameRule labelRule native nativeWidth solid none radius em editable readOnly notReadOnly active inactive focus named otherNamed textEditNamed star background minOverMax cleared InWindow InBox Hidden SharedBox SharedEdit2"):gmatch("%S+") do
    deleteCommandLine("cs" .. key)
  end
  deleteTextEdit("csSharedEdit")
  deleteTextEdit("csSharedEdit2")
  deleteScrollBox("csBox")
  deleteMiniConsole("csMini")
  deleteMiniConsole("csDisplaced")
  closeUserWindow("csWindow")
  -- A frame console's own command line outlives a frame it was moved out of, and shows its own geometry then
  setConfig("specialForceMXPProcessorOn", true)
  openUserWindow("csFrameWindow", false, false, "f")
  -- Where its console's layout puts it is not modelled, so one a minimum does not hold is only read once moved
  local frames = {
    {"csFrame", "csFrameWindow", "min-height: 2000px; min-width: 2000px"},
    {"csFrameMain", "main", "min-height: 2000px; min-width: 2000px"},
    {"csFrameMax", "csFrameWindow", "max-height: 20px; max-width: 50px"},
  }
  for _, frame in ipairs(frames) do
    local key, window, sheet = frame[1], frame[2], frame[3]
    local minimum = sheet:find("min", 1, true)
    feedTriggers([[<FRAME Name="]] .. key .. [[" Left="0" Top="0" Width="20%" Height="30%">]] .. "\n")
    enableCommandLine(key)
    step(key .. ".sheet", setCmdLineStyleSheet, key, sheet)
    step(key .. ".move", setWindow, window, key, 0, 0, true)
    feedTriggers([[<FRAME ]] .. key .. [[ ACTION="close">]] .. "\n")
    step(key .. ".type", windowType, key)
    if minimum then
      look(key, key)
    end
    resizeWindow(key, 5, 5)
    if minimum then
      look(key .. ".resized", key)
    end
    resizeWindow(key, 500, 500)
    step(key .. ".moveOut", setWindow, "main", key, 1, 1, true)
    look(key .. ".out", key)
    resizeWindow(key, 5, 5)
    look(key .. ".outResized", key)
    resizeWindow(key, 500, 500)
    look(key .. ".outResizedLarge", key)
    step(key .. ".get", getCmdLineStyleSheet, key)
    deleteCommandLine(key)
  end
  closeUserWindow("csFrameWindow")
  setConfig("specialForceMXPProcessorOn", false)
  headlessCmdLineStyleAnswers = table.concat(answers, "\n")
  local mainAnswers = {}
  local function mainStep(label, f, ...)
    mainAnswers[#mainAnswers + 1] = label .. "=" .. described(pcall(f, ...))
  end
  mainStep("main", setCmdLineStyleSheet, "main", "min-height: 5px")
  mainStep("mainGet", getCmdLineStyleSheet, "main")
  mainStep("noName", setCmdLineStyleSheet, "min-height: 5px")
  mainStep("empty", setCmdLineStyleSheet, "", "min-height: 5px")
  mainStep("nilGet", getCmdLineStyleSheet)
  headlessCmdLineStyleMainAnswers = table.concat(mainAnswers, "\n")
end)
headlessCmdLineStyle = ok and "ok" or tostring(err)
)lua"));
        QVERIFY2(ran, "The Lua chunk did not run.");
        QCOMPARE(luaGlobalString(host, "headlessCmdLineStyle"), qsl("ok"));
        // QCss measures a command line's em in the application font at the sheet's size
        QFont sheetFont;
        sheetFont.setPixelSize(20);
        const int twoEm = 2 * QFontMetrics(sheetFont).height();
        // As the chunk escapes the profile name in a selector
        const QString profileId = QString(hostname).replace(QLatin1Char(' '), qsl("\\ "));
        const QStringList answers{qsl("fresh.sheet=[1]QPlainTextEdit{background-color: rgb(0, 0, 0);}"),
                                  qsl("min=[1]true [1]min-width: 50px; min-height: 30px [4]0:0:52:32 [4]0:0:52:500 [4]0:0:500:32"),
                                  qsl("max=[1]true [1]max-width: 20px; max-height: 8px [4]0:0:22:10 [4]0:0:5:10 [4]0:0:22:5"),
                                  qsl("padded=[1]true [1]min-width: 50px; padding: 2px; border: 3px solid [4]0:0:60:1 [4]0:0:60:500 [4]0:0:500:5"),
                                  qsl("width=[1]true [1]width: 100px; min-width: 50px [4]0:0:102:1 [4]0:0:102:500 [4]0:0:500:5"),
                                  qsl("rule=[1]true [1]QPlainTextEdit { min-width: 60px } [4]0:0:62:1 [4]0:0:62:500 [4]0:0:500:5"),
                                  qsl("ownRule=[1]true [1]TCommandLine { min-height: 60px } [4]0:0:1:62 [4]0:0:5:500 [4]0:0:500:62"),
                                  qsl("textBoxRule=[1]true [1]TTextBox { min-height: 60px } [4]0:0:1:1 [4]0:0:5:500 [4]0:0:500:5"),
                                  qsl("scrollRule=[1]true [1]QAbstractScrollArea { min-height: 61px } [4]0:0:1:63 [4]0:0:5:500 [4]0:0:500:63"),
                                  qsl("frameRule=[1]true [1]QFrame { min-width: 62px } [4]0:0:64:1 [4]0:0:64:500 [4]0:0:500:5"),
                                  qsl("labelRule=[1]true [1]QLabel { min-width: 60px } [4]0:0:1:1 [4]0:0:5:500 [4]0:0:500:5"),
                                  qsl("native=[1]true [1]border-style: native; min-width: 50px [4]0:0:52:1 [4]0:0:52:500 [4]0:0:500:5"),
                                  qsl("nativeWidth=[1]true [1]border: 4px native; min-width: 50px [4]0:0:58:1 [4]0:0:58:500 [4]0:0:500:5"),
                                  qsl("solid=[1]true [1]border: 2px solid; min-width: 50px [4]0:0:54:1 [4]0:0:54:500 [4]0:0:500:5"),
                                  qsl("none=[1]true [1]border: none; min-width: 50px [4]0:0:50:1 [4]0:0:50:500 [4]0:0:500:5"),
                                  qsl("radius=[1]true [1]border-radius: 2px; min-width: 50px [4]0:0:50:1 [4]0:0:50:500 [4]0:0:500:5"),
                                  qsl("em=[1]true [1]font-size: 20px; min-width: 2em [4]0:0:%1:1 [4]0:0:%1:500 [4]0:0:500:5").arg(twoEm + 2),
                                  qsl("editable=[1]true [1]QPlainTextEdit:editable { min-width: 60px } [4]0:0:62:1 [4]0:0:62:500 [4]0:0:500:5"),
                                  qsl("readOnly=[1]true [1]QPlainTextEdit:read-only { min-width: 60px } [4]0:0:1:1 [4]0:0:5:500 [4]0:0:500:5"),
                                  qsl("notReadOnly=[1]true [1]QPlainTextEdit:!read-only { min-width: 60px } [4]0:0:62:1 [4]0:0:62:500 [4]0:0:500:5"),
                                  qsl("active=[1]true [1]QPlainTextEdit:active { min-width: 60px } [4]0:0:1:1 [4]0:0:5:500 [4]0:0:500:5"),
                                  qsl("inactive=[1]true [1]QPlainTextEdit:!active { padding: 2px; min-width: 50px } [4]0:0:56:1 [4]0:0:56:500 [4]0:0:500:5"),
                                  qsl("focus=[1]true [1]QPlainTextEdit:focus { min-width: 60px } [4]0:0:1:1 [4]0:0:5:500 [4]0:0:500:5"),
                                  qsl("named=[1]true [1]#commandLine_%1_csnamed { min-width: 60px } [4]0:0:62:1 [4]0:0:62:500 [4]0:0:500:5").arg(profileId),
                                  qsl("otherNamed=[1]true [1]#commandLine_%1_csother { min-width: 60px } [4]0:0:1:1 [4]0:0:5:500 [4]0:0:500:5").arg(profileId),
                                  qsl("textEditNamed=[1]true [1]#textEdit_%1_cstextEditNamed { min-width: 60px } [4]0:0:1:1 [4]0:0:5:500 [4]0:0:500:5").arg(profileId),
                                  qsl("star=[1]true [1]* { min-width: 60px } [4]0:0:62:1 [4]0:0:62:500 [4]0:0:500:5"),
                                  qsl("background=[1]true [1]QPlainTextEdit { background-color: red; } [4]0:0:1:1 [4]0:0:5:500 [4]0:0:500:5"),
                                  qsl("minOverMax=[1]true [1]min-width: 50px; max-width: 20px; min-height: 9px; max-height: 40px [4]0:0:52:30 [4]0:0:52:42 [4]0:0:52:11"),
                                  qsl("cleared=[1]true [1]min-width: 50px; max-height: 8px [4]0:0:52:10 [4]0:0:52:10 [4]0:0:500:5"),
                                  qsl("cleared.clear=[1]true"),
                                  qsl("cleared.sheet=[1]"),
                                  qsl("cleared.after=[4]0:0:500:5"),
                                  qsl("cleared.free=[4]0:0:5:500"),
                                  qsl("missing=[2]nil:command-line name 'csMissing' not found"),
                                  qsl("missing.get=[2]nil:command-line name 'csMissing' not found"),
                                  qsl("gone=[2]nil:command-line name 'csGone' not found"),
                                  qsl("gone.get=[2]nil:command-line name 'csGone' not found"),
                                  qsl("number=[1]true"),
                                  qsl("number.get=[1]5"),
                                  qsl("noSheet=raised:setCmdLineStyleSheet: bad argument #2 type (StyleSheet as string expected, got nil!)"),
                                  qsl("labelName=[2]nil:command-line name 'csLabel' not found"),
                                  qsl("inWindow=[1]true"),
                                  qsl("inWindow=[4]0:0:42:1"),
                                  qsl("inBox=[1]true"),
                                  qsl("inBox=[4]0:0:43:1"),
                                  qsl("inBox.moveBox=[1]true"),
                                  qsl("inBox.box=[4]0:0:43:5"),
                                  qsl("inBox.moveOut=[1]true"),
                                  qsl("inBox.out=[4]3:3:43:5"),
                                  qsl("inBox.get=[1]min-width: 41px"),
                                  qsl("hidden=[1]true"),
                                  qsl("hidden=[4]0:0:44:1"),
                                  qsl("sharedBox=[1]true"),
                                  qsl("sharedBox.get=[1]min-width: 50px"),
                                  qsl("sharedBox=[4]0:0:30:30"),
                                  qsl("sharedBox.alone=[4]0:0:52:1"),
                                  qsl("sharedBox.resized=[4]0:0:52:5"),
                                  qsl("sharedEdit=[1]true"),
                                  qsl("sharedEdit=[4]0:0:52:2"),
                                  qsl("sharedEdit.edit=[4]0:0:1:1"),
                                  qsl("sharedEdit2=[1]true"),
                                  qsl("sharedEdit2=[4]0:0:2:32"),
                                  qsl("mini.before=[2]nil:command-line name 'csMini' not found"),
                                  qsl("mini.sheet=[1]QPlainTextEdit{background-color: rgb(0, 0, 0);}"),
                                  qsl("mini.set=[1]true"),
                                  qsl("mini.get=[1]min-height: 300px; min-width: 300px"),
                                  qsl("mini.disabled=[1]true"),
                                  qsl("mini.disabledGet=[1]min-height: 30px"),
                                  qsl("mini.reenabled=[1]true"),
                                  qsl("mini.reenabledGet=[1]min-height: 30px"),
                                  qsl("displaced.get=[1]QPlainTextEdit{background-color: rgb(0, 0, 0);}"),
                                  qsl("window.set=[1]true"),
                                  qsl("window.get=[1]min-height: 30px"),
                                  qsl("buffer.set=[1]true"),
                                  qsl("buffer.get=[1]color: red"),
                                  qsl("csFrame.sheet=[1]true"),
                                  qsl("csFrame.move=[1]true"),
                                  qsl("csFrame.type=[1]commandline"),
                                  qsl("csFrame=[4]0:0:2002:2002"),
                                  qsl("csFrame.resized=[4]0:0:2002:2002"),
                                  qsl("csFrame.moveOut=[1]true"),
                                  qsl("csFrame.out=[4]1:1:2002:2002"),
                                  qsl("csFrame.outResized=[4]1:1:2002:2002"),
                                  qsl("csFrame.outResizedLarge=[4]1:1:2002:2002"),
                                  qsl("csFrame.get=[1]min-height: 2000px; min-width: 2000px"),
                                  qsl("csFrameMain.sheet=[1]true"),
                                  qsl("csFrameMain.move=[1]true"),
                                  qsl("csFrameMain.type=[1]commandline"),
                                  qsl("csFrameMain=[4]0:0:2002:2002"),
                                  qsl("csFrameMain.resized=[4]0:0:2002:2002"),
                                  qsl("csFrameMain.moveOut=[1]true"),
                                  qsl("csFrameMain.out=[4]1:1:2002:2002"),
                                  qsl("csFrameMain.outResized=[4]1:1:2002:2002"),
                                  qsl("csFrameMain.outResizedLarge=[4]1:1:2002:2002"),
                                  qsl("csFrameMain.get=[1]min-height: 2000px; min-width: 2000px"),
                                  qsl("csFrameMax.sheet=[1]true"),
                                  qsl("csFrameMax.move=[1]true"),
                                  qsl("csFrameMax.type=[1]commandline"),
                                  qsl("csFrameMax.moveOut=[1]true"),
                                  qsl("csFrameMax.out=[4]1:1:52:22"),
                                  qsl("csFrameMax.outResized=[4]1:1:5:5"),
                                  qsl("csFrameMax.outResizedLarge=[4]1:1:52:22"),
                                  qsl("csFrameMax.get=[1]max-height: 20px; max-width: 50px")};
        const QStringList actual = luaGlobalString(host, "headlessCmdLineStyleAnswers").split(QLatin1Char('\n'));
        QStringList mismatches;
        for (qsizetype i = 0; i < std::max(actual.size(), answers.size()); ++i) {
            if (actual.value(i) != answers.value(i)) {
                mismatches << qsl("%1, not %2").arg(actual.value(i), answers.value(i));
            }
        }
        QVERIFY2(mismatches.isEmpty(), qPrintable(mismatches.join(QChar::LineFeed)));
        const QStringList mainAnswers{qsl("main=[1]true"), qsl("mainGet=[1]min-height: 5px"), qsl("noName=[1]true"), qsl("empty=[1]true"), qsl("nilGet=[1]min-height: 5px")};
        QCOMPARE(luaGlobalString(host, "headlessCmdLineStyleMainAnswers"), mainAnswers.join(QLatin1Char('\n')));

        // A new command line's sheet is built from the profile's command line background as TCommandLine's constructor
        // builds it, which the GUI profile's black leaves untested
        host->mCommandLineBgColor = QColor(10, 20, 30);
        QVERIFY(host->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(
createCommandLine("csColour", 0, 0, 10, 10)
createMiniConsole("csColourMini", 0, 0, 10, 10)
enableCommandLine("csColourMini")
headlessCmdLineColours = getCmdLineStyleSheet("csColour") .. "|" .. getCmdLineStyleSheet("csColourMini")
)lua")));
        QCOMPARE(luaGlobalString(host, "headlessCmdLineColours"), qsl("QPlainTextEdit{background-color: rgb(10, 20, 30);}|QPlainTextEdit{background-color: rgb(10, 20, 30);}"));
        HostManager::self()->deleteHost(hostname);
    }

    // The answers are those of the same Lua run in the GUI self-test profile
    void test_headlessMainCommandLineContract()
    {
        const QString hostname = qsl("Test-Headless-Host-Main-Command-Line");
        // Opened rather than added, as saveProfile() refuses a profile that was never loaded
        Host* host = HostManager::self()->openProfile(hostname, false);
        QVERIFY2(host, "The profile did not open.");
        QVERIFY(!host->hasConsoleView());
        QVERIFY2(host->mLoadedOk, qPrintable(host->mProfileLoadError));

        const bool ran = host->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(
headlessMainCmdLine = "not run"
local ok, err = pcall(function()
  local function described(called, ...)
    if not called then
      return "raised:" .. tostring((...))
    end
    local count = select("#", ...)
    local values = {}
    for i = 1, count do
      -- Bytes a line of answers cannot hold as they are, by number
      values[i] = tostring((select(i, ...))):gsub("[%c\128-\255]", function(byte)
        return "\\" .. byte:byte()
      end)
    end
    return "[" .. count .. "]" .. table.concat(values, ":")
  end
  local answers = {}
  local function step(label, f, ...)
    answers[#answers + 1] = label .. "=" .. described(pcall(f, ...))
  end
  local startSheet = getCmdLineStyleSheet()
  local startHistory = getSaveCommandHistory()
  clearCmdLine()

  step("get", getCmdLine)
  step("print", printCmdLine, "one")
  step("get.print", getCmdLine)
  step("get.main", getCmdLine, "main")
  step("get.empty", getCmdLine, "")
  step("printMain", printCmdLine, "main", "a\tb\r\nc\rd\194\160e\226\128\168f\226\128\169g\0h")
  step("get.printMain", getCmdLine)
  step("printEmpty", printCmdLine, "", "two")
  step("get.printEmpty", getCmdLine, "main")
  step("printNumber", printCmdLine, 42)
  step("get.printNumber", getCmdLine)
  step("append", appendCmdLine, " more")
  step("appendMain", appendCmdLine, "main", "\194\160x\n")
  step("appendEmpty", appendCmdLine, "", "!")
  step("get.append", getCmdLine)
  step("select", selectCmdLineText)
  step("selectMain", selectCmdLineText, "main")
  step("selectEmpty", selectCmdLineText, "")
  step("get.select", getCmdLine)
  step("clear", clearCmdLine)
  step("get.clear", getCmdLine)
  step("appendToEmpty", appendCmdLine, "y")
  step("get.appendToEmpty", getCmdLine)
  step("clearMain", clearCmdLine, "main")
  step("get.clearMain", getCmdLine)
  printCmdLine("z")
  step("clearEmpty", clearCmdLine, "")
  step("get.clearEmpty", getCmdLine)
  step("send", sendCmdLine, "sent\194\160line\nnext")
  step("get.send", getCmdLine)
  clearCmdLine()

  for _, name in ipairs({"", "main"}) do
    step("suggest." .. name, addCmdLineSuggestion, name, "word")
    step("unsuggest." .. name, removeCmdLineSuggestion, name, "word")
    step("clearSuggestions." .. name, clearCmdLineSuggestions, name)
    step("blacklist." .. name, addCmdLineBlacklist, name, "word")
    step("unblacklist." .. name, removeCmdLineBlacklist, name, "word")
    step("clearBlacklist." .. name, clearCmdLineBlacklist, name)
  end
  step("suggest", addCmdLineSuggestion, "word")
  step("unsuggest", removeCmdLineSuggestion, "word")
  step("clearSuggestions", clearCmdLineSuggestions)
  step("blacklist", addCmdLineBlacklist, "word")
  step("unblacklist", removeCmdLineBlacklist, "word")
  step("clearBlacklist", clearCmdLineBlacklist)

  step("menu", addCommandLineMenuEvent, "mcItem", "mcEvent")
  step("unmenu", removeCommandLineMenuEvent, "mcItem")
  step("unmenuAgain", removeCommandLineMenuEvent, "mcItem")
  step("menuMain", addCommandLineMenuEvent, "main", "mcItem", "mcEvent")
  step("unmenuEmpty", removeCommandLineMenuEvent, "", "mcItem")
  step("menuEmpty", addCommandLineMenuEvent, "", "mcOther", "mcEvent")
  step("menuEmptyAgain", addCommandLineMenuEvent, "", "mcOther", "mcEvent2")
  step("unmenuMain", removeCommandLineMenuEvent, "main", "mcOther")
  step("unmenuMainAgain", removeCommandLineMenuEvent, "main", "mcOther")
  createCommandLine("mcLine", 0, 0, 10, 10)
  addCommandLineMenuEvent("mcLine", "mcShared", "mcEvent")
  step("unmenuMainShared", removeCommandLineMenuEvent, "main", "mcShared")
  step("unmenuLineShared", removeCommandLineMenuEvent, "mcLine", "mcShared")
  addCommandLineMenuEvent("main", "mcMainOnly", "mcEvent")
  step("unmenuLineMainOnly", removeCommandLineMenuEvent, "mcLine", "mcMainOnly")
  step("unmenuMainOnly", removeCommandLineMenuEvent, "mcMainOnly")
  printCmdLine("mcLine", "sub text")
  printCmdLine("main text")
  step("get.sub", getCmdLine, "mcLine")
  step("get.mainBesideSub", getCmdLine)
  setCmdLineStyleSheet("mcLine", "color: blue;")
  step("sheet.mainBesideSub", getCmdLineStyleSheet)
  setSaveCommandHistory("mcLine", false)
  step("history.mainBesideSub", getSaveCommandHistory)
  deleteCommandLine("mcLine")
  clearCmdLine()

  step("history", getSaveCommandHistory)
  step("historyMain", getSaveCommandHistory, "main")
  step("historyEmpty", getSaveCommandHistory, "")
  step("setHistory", setSaveCommandHistory, false)
  step("history.off", getSaveCommandHistory)
  step("historyMain.off", getSaveCommandHistory, "main")
  step("setHistoryMain", setSaveCommandHistory, "main")
  step("historyMain.on", getSaveCommandHistory, "main")
  step("setHistoryEmpty", setSaveCommandHistory, "", false)
  step("historyEmpty.off", getSaveCommandHistory, "")
  step("setHistoryNone", setSaveCommandHistory)
  step("history.none", getSaveCommandHistory)
  setSaveCommandHistory("main", startHistory ~= false)

  step("sheet", getCmdLineStyleSheet)
  step("sheetMain", getCmdLineStyleSheet, "main")
  step("sheetEmpty", getCmdLineStyleSheet, "")
  step("sheetNil", getCmdLineStyleSheet, nil)
  step("setSheet", setCmdLineStyleSheet, "QPlainTextEdit { color: red; }")
  step("sheet.set", getCmdLineStyleSheet)
  step("setSheetMain", setCmdLineStyleSheet, "main", "color: green;")
  step("sheet.setMain", getCmdLineStyleSheet, "main")
  step("setSheetEmpty", setCmdLineStyleSheet, "", "")
  step("sheet.setEmpty", getCmdLineStyleSheet, "")
  setCmdLineStyleSheet("main", startSheet or "")

  printCmdLine("kept")
  step("echo.fed", feedTelnet, "<T_IAC><T_WILL><O_ECHO>")
  step("echo.get", getCmdLine)
  step("echo.print", printCmdLine, "secret")
  step("echo.get.print", getCmdLine)
  step("echo.append", appendCmdLine, "main", "more")
  step("echo.clear", clearCmdLine)
  step("echo.select", selectCmdLineText)
  step("echo.send", sendCmdLine, "sent")
  step("echo.get.after", getCmdLine)
  step("echo.printMain", printCmdLine, "main", "x")
  step("echo.ended", feedTelnet, "<T_IAC><T_WONT><O_ECHO>")
  step("echo.get.ended", getCmdLine)
  step("echo.print.ended", printCmdLine, "after")
  step("echo.get.printEnded", getCmdLine)
  clearCmdLine()
  headlessMainCmdLineAnswers = table.concat(answers, "\n")
end)
headlessMainCmdLine = ok and "ok" or tostring(err)
)lua"));
        QVERIFY2(ran, "The Lua chunk did not run.");
        QCOMPARE(luaGlobalString(host, "headlessMainCmdLine"), qsl("ok"));
        const QStringList answers{qsl("get=[1]"),
                                  qsl("print=[0]"),
                                  qsl("get.print=[1]one"),
                                  qsl("get.main=[1]one"),
                                  qsl("get.empty=[1]one"),
                                  qsl("printMain=[0]"),
                                  qsl("get.printMain=[1]a\\9b\\10c\\10d e\\10f\\10g"),
                                  qsl("printEmpty=[0]"),
                                  qsl("get.printEmpty=[1]two"),
                                  qsl("printNumber=[0]"),
                                  qsl("get.printNumber=[1]42"),
                                  qsl("append=[0]"),
                                  qsl("appendMain=[0]"),
                                  qsl("appendEmpty=[0]"),
                                  qsl("get.append=[1]42 more x\\10!"),
                                  qsl("select=[1]true"),
                                  qsl("selectMain=[1]true"),
                                  qsl("selectEmpty=[1]true"),
                                  qsl("get.select=[1]42 more x\\10!"),
                                  qsl("clear=[0]"),
                                  qsl("get.clear=[1]"),
                                  qsl("appendToEmpty=[0]"),
                                  qsl("get.appendToEmpty=[1]y"),
                                  qsl("clearMain=[0]"),
                                  qsl("get.clearMain=[1]"),
                                  qsl("clearEmpty=[0]"),
                                  qsl("get.clearEmpty=[1]"),
                                  qsl("send=[1]true"),
                                  qsl("get.send=[1]sent line\\10next"),
                                  qsl("suggest.=[0]"),
                                  qsl("unsuggest.=[0]"),
                                  qsl("clearSuggestions.=[0]"),
                                  qsl("blacklist.=[0]"),
                                  qsl("unblacklist.=[0]"),
                                  qsl("clearBlacklist.=[0]"),
                                  qsl("suggest.main=[0]"),
                                  qsl("unsuggest.main=[0]"),
                                  qsl("clearSuggestions.main=[0]"),
                                  qsl("blacklist.main=[0]"),
                                  qsl("unblacklist.main=[0]"),
                                  qsl("clearBlacklist.main=[0]"),
                                  qsl("suggest=[0]"),
                                  qsl("unsuggest=[0]"),
                                  qsl("clearSuggestions=[0]"),
                                  qsl("blacklist=[0]"),
                                  qsl("unblacklist=[0]"),
                                  qsl("clearBlacklist=[0]"),
                                  qsl("menu=[1]true"),
                                  qsl("unmenu=[1]true"),
                                  qsl("unmenuAgain=[2]nil:cannot remove 'mcItem', menu item does not exist"),
                                  qsl("menuMain=[1]true"),
                                  qsl("unmenuEmpty=[1]true"),
                                  qsl("menuEmpty=[1]true"),
                                  qsl("menuEmptyAgain=[1]true"),
                                  qsl("unmenuMain=[1]true"),
                                  qsl("unmenuMainAgain=[2]nil:cannot remove 'mcOther', menu item does not exist"),
                                  qsl("unmenuMainShared=[2]nil:cannot remove 'mcShared', menu item does not exist"),
                                  qsl("unmenuLineShared=[1]true"),
                                  qsl("unmenuLineMainOnly=[2]nil:cannot remove 'mcMainOnly', menu item does not exist"),
                                  qsl("unmenuMainOnly=[1]true"),
                                  qsl("get.sub=[1]sub text"),
                                  qsl("get.mainBesideSub=[1]main text"),
                                  qsl("sheet.mainBesideSub=[1]"),
                                  qsl("history.mainBesideSub=[2]true:enabled (500 lines will be saved)"),
                                  qsl("history=[2]true:enabled (500 lines will be saved)"),
                                  qsl("historyMain=[2]true:enabled (500 lines will be saved)"),
                                  qsl("historyEmpty=[2]true:enabled (500 lines will be saved)"),
                                  qsl("setHistory=[1]true"),
                                  qsl("history.off=[2]false:disabled"),
                                  qsl("historyMain.off=[2]false:disabled"),
                                  qsl("setHistoryMain=[1]true"),
                                  qsl("historyMain.on=[2]true:enabled (500 lines will be saved)"),
                                  qsl("setHistoryEmpty=[1]true"),
                                  qsl("historyEmpty.off=[2]false:disabled"),
                                  qsl("setHistoryNone=[1]true"),
                                  qsl("history.none=[2]true:enabled (500 lines will be saved)"),
                                  qsl("sheet=[1]"),
                                  qsl("sheetMain=[1]"),
                                  qsl("sheetEmpty=[1]"),
                                  qsl("sheetNil=[1]"),
                                  qsl("setSheet=[1]true"),
                                  qsl("sheet.set=[1]QPlainTextEdit { color: red; }"),
                                  qsl("setSheetMain=[1]true"),
                                  qsl("sheet.setMain=[1]color: green;"),
                                  qsl("setSheetEmpty=[1]true"),
                                  qsl("sheet.setEmpty=[1]"),
                                  qsl("echo.fed=[1]true"),
                                  qsl("echo.get=[1]kept"),
                                  qsl("echo.print=[0]"),
                                  qsl("echo.get.print=[1]kept"),
                                  qsl("echo.append=[0]"),
                                  qsl("echo.clear=[0]"),
                                  qsl("echo.select=[1]true"),
                                  qsl("echo.send=[1]true"),
                                  qsl("echo.get.after=[1]kept"),
                                  qsl("echo.printMain=[0]"),
                                  qsl("echo.ended=[1]true"),
                                  qsl("echo.get.ended=[1]kept"),
                                  qsl("echo.print.ended=[0]"),
                                  qsl("echo.get.printEnded=[1]after")};
        const QStringList actual = luaGlobalString(host, "headlessMainCmdLineAnswers").split(QLatin1Char('\n'));
        QStringList mismatches;
        for (qsizetype i = 0; i < std::max(actual.size(), answers.size()); ++i) {
            if (actual.value(i) != answers.value(i)) {
                mismatches << qsl("%1, not %2").arg(actual.value(i), answers.value(i));
            }
        }
        QVERIFY2(mismatches.isEmpty(), qPrintable(mismatches.join(QChar::LineFeed)));
        // The main command line writes its setting on an end of session save, as its TCommandLine does. The key is
        // set the other way first, so the save is seen to write it.
        QVERIFY(host->getLuaInterpreter()->compileAndExecuteScript(qsl("setSaveCommandHistory(false)")));
        host->writeProfileIniData(qsl("CommandLines/SaveHistory/main"), qsl("true"));
        QTRY_VERIFY(!host->currentlySavingProfile());
        QVERIFY(std::get<0>(host->saveProfile()));
        QCOMPARE(host->readProfileIniData(qsl("CommandLines/SaveHistory/main")), qsl("false"));
        QTRY_VERIFY(!host->currentlySavingProfile());
        // A profile opened again starts from what was saved, as TCommandLine's constructor reads it
        HostManager::self()->deleteHost(hostname);
        host = HostManager::self()->openProfile(hostname, false);
        QVERIFY2(host, "The profile did not open again.");
        QVERIFY(host->getLuaInterpreter()->compileAndExecuteScript(qsl("headlessMainHistoryReopened = tostring(getSaveCommandHistory())")));
        QCOMPARE(luaGlobalString(host, "headlessMainHistoryReopened"), qsl("false"));
        QVERIFY2(QApplication::topLevelWidgets().isEmpty(), "Using the main command line created a widget.");
        HostManager::self()->deleteHost(hostname);
    }

    // The answers are those of the same Lua run in the GUI self-test profile, but for the last ones,
    // which a spec cannot wait for: a text edit in a deleted scroll box goes with it
    void test_headlessTextEditContract()
    {
        const QString hostname = qsl("Test-Headless-Host-Text-Edit");
        QVERIFY2(HostManager::self()->addHost(hostname, QString(), QString(), QString()), "Could not create a profile with no main window.");
        Host* host = HostManager::self()->getHost(hostname);
        QVERIFY2(host, "The profile is not in the pool.");
        QVERIFY(!host->hasConsoleView());

        bool ran = host->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(
headlessTextEdit = "not run"
local deletedEvents = {}
local ok, err = pcall(function()
  local function described(called, ...)
    if not called then
      return "raised:" .. tostring((...))
    end
    local count = select("#", ...)
    local values = {}
    for i = 1, count do
      -- Bytes a line of answers cannot hold as they are, by number
      values[i] = tostring((select(i, ...))):gsub("[%c\128-\255]", function(byte)
        return "\\" .. byte:byte()
      end)
    end
    return "[" .. count .. "]" .. table.concat(values, ":")
  end
  local answers = {}
  local function step(label, f, ...)
    answers[#answers + 1] = label .. "=" .. described(pcall(f, ...))
  end
  local function look(label, name)
    answers[#answers + 1] = label .. ":type=" .. described(pcall(windowType, name)) .. ",geometry=" .. described(pcall(getWindowGeometry, name)) .. ",visible=" .. described(pcall(windowVisible, name))
  end
  -- By name, as the C++ registerAnonymousEventHandler() takes no function
  function headlessTextEditDeleted(_, name)
    deletedEvents[#deletedEvents + 1] = name
  end
  registerAnonymousEventHandler("sysTextEditDeleted", "headlessTextEditDeleted")
  local function events(label)
    answers[#answers + 1] = label .. ":events=" .. table.concat(deletedEvents, ",")
    for i = #deletedEvents, 1, -1 do
      deletedEvents[i] = nil
    end
  end
  step("create", createTextEdit, "teMain", 10, 20, 200, 100)
  look("created", "teMain")
  step("createAgain", createTextEdit, "teMain", 0, 0, 10, 10)
  step("createEmpty", createTextEdit, "", 0, 0, 10, 10)
  step("createInMissing", createTextEdit, "teNoSuch", "teLost", 0, 0, 10, 10)
  look("lost", "teLost")
  step("createInMain", createTextEdit, "main", "teInMain", 1, 2, 30, 40)
  look("inMain", "teInMain")
  step("createInEmpty", createTextEdit, "", "teInEmpty", 1, 2, 30, 40)
  look("inEmpty", "teInEmpty")
  step("createNegative", createTextEdit, "teNegative", 3, 4, -5, -6)
  look("negative", "teNegative")
  step("createBadName", createTextEdit, {}, 0, 0, 10, 10)
  step("createBadX", createTextEdit, "teBad", "x", 0, 10, 10)
  look("bad", "teBad")

  step("getNew", getTextEditText, "teMain")
  step("set", setTextEditText, "teMain", "hello")
  step("getSet", getTextEditText, "teMain")
  step("setOdd", setTextEditText, "teMain", "a\tb\r\nc\rd\194\160e\226\128\168f\226\128\169g\0h")
  step("getOdd", getTextEditText, "teMain")
  step("setNumber", setTextEditText, "teMain", 42)
  step("getNumber", getTextEditText, "teMain")
  step("clear", clearTextEdit, "teMain")
  step("getCleared", getTextEditText, "teMain")
  step("readOnly", setTextEditReadOnly, "teMain", true)
  step("setReadOnly", setTextEditText, "teMain", "read only")
  step("getReadOnly", getTextEditText, "teMain")
  step("writable", setTextEditReadOnly, "teMain", false)
  step("placeholder", setTextEditPlaceholder, "teMain", "type here")
  step("sheet", setTextEditStyleSheet, "teMain", "color: red; font-size: 20pt")
  step("font", setTextEditFont, "teMain", "Bitstream Vera Sans Mono")
  step("fontSize", setTextEditFontSize, "teMain", 15)
  step("fontSizeNegative", setTextEditFontSize, "teMain", -3)
  step("tab", setTextEditTabMovesFocus, "teMain", true)
  step("getOther", getTextEditText, "teInMain")
  look("afterSetters", "teMain")
  for _, f in ipairs({"getTextEditText", "clearTextEdit"}) do
    step("missing." .. f, _G[f], "teNoSuch")
  end
  for _, f in ipairs({"setTextEditText", "setTextEditPlaceholder", "setTextEditStyleSheet", "setTextEditFont"}) do
    step("missing." .. f, _G[f], "teNoSuch", "x")
  end
  step("missing.setTextEditFontSize", setTextEditFontSize, "teNoSuch", 10)
  step("missing.setTextEditReadOnly", setTextEditReadOnly, "teNoSuch", true)
  step("missing.setTextEditTabMovesFocus", setTextEditTabMovesFocus, "teNoSuch", true)
  step("setEmptyName", setTextEditText, "", "x")
  step("setBadName", setTextEditText, {}, "x")
  step("setBadText", setTextEditText, "teMain", {})

  step("raise", raiseWindow, "teMain")
  step("lower", lowerWindow, "teMain")
  step("hide", hideWindow, "teMain")
  look("hidden", "teMain")
  step("show", showWindow, "teMain")
  look("shown", "teMain")
  step("move", moveWindow, "teMain", 5, 6)
  step("resize", resizeWindow, "teMain", 70, 25)
  look("moved", "teMain")
  step("resizeNegative", resizeWindow, "teMain", -1, -2)
  look("resizedNegative", "teMain")
  step("resizeBack", resizeWindow, "teMain", 70, 25)

  assert(openUserWindow("teWindow", false, false, "f") == true, "openUserWindow did not answer true")
  step("createInWindow", createTextEdit, "teWindow", "teWin", 1, 1, 50, 20)
  look("inWindow", "teWin")
  hideWindow("teWindow")
  look("inHiddenWindow", "teWin")
  showWindow("teWindow")
  look("inShownWindow", "teWin")
  assert(createScrollBox("teWindow", "teBox", 0, 30, 100, 50))
  step("createInBox", createTextEdit, "teBox", "teBoxed", 2, 2, 40, 20)
  look("inBox", "teBoxed")
  hideWindow("teBox")
  look("inHiddenBox", "teBoxed")
  step("createInHiddenBox", createTextEdit, "teBox", "teLate", 2, 25, 40, 20)
  look("madeInHiddenBox", "teLate")
  showWindow("teBox")
  step("hideBoxed", hideWindow, "teBoxed")
  hideWindow("teBox")
  showWindow("teBox")
  look("boxedStaysHidden", "teBoxed")
  showWindow("teBoxed")
  step("textInBox", setTextEditText, "teBoxed", "boxed")
  step("getInBox", getTextEditText, "teBoxed")

  step("intoBox", setWindow, "teBox", "teMain", 3, 3, true)
  look("movedIntoBox", "teMain")
  hideWindow("teWindow")
  look("movedInHiddenWindow", "teMain")
  showWindow("teWindow")
  step("intoMainHidden", setWindow, "main", "teMain", 4, 4, false)
  look("movedToMainHidden", "teMain")
  step("showMoved", showWindow, "teMain")
  step("intoWindow", setWindow, "teWindow", "teMain", 9, 9, true)
  look("movedIntoWindow", "teMain")
  step("textAfterMove", getTextEditText, "teMain")
  step("intoMain", setWindow, "main", "teMain", 10, 20, true)
  -- What a scroll box holds moves with it
  assert(createScrollBox("teWindow", "teMover", 0, 0, 100, 50))
  step("createInMover", createTextEdit, "teMover", "teRider", 1, 1, 40, 20)
  step("moverIntoMain", setWindow, "main", "teMover", 0, 0, true)
  hideWindow("teWindow")
  look("riderAfterMove", "teRider")
  showWindow("teWindow")
  deleteScrollBox("teMover")
  -- A scroll box takes a text edit before a user window of the same name
  assert(openUserWindow("teTwin", false, false, "f") == true, "openUserWindow did not answer true")
  assert(createScrollBox("teTwin", 0, 0, 100, 50))
  step("createInTwin", createTextEdit, "teTwin", "teInTwin", 1, 1, 40, 20)
  hideWindow("teTwin")
  look("twinWindowHidden", "teInTwin")
  showWindow("teTwin")

  -- A command line of the same name is what the name's geometry and visibility are read from
  step("lineFirst", createCommandLine, "teShared", 1, 1, 10, 10)
  step("editAfterLine", createTextEdit, "teShared", 2, 2, 20, 20)
  look("lineThenEdit", "teShared")
  step("sharedEditText", setTextEditText, "teShared", "edit")
  step("sharedLineText", printCmdLine, "teShared", "line")
  step("sharedGetEdit", getTextEditText, "teShared")
  step("sharedGetLine", getCmdLine, "teShared")
  hideWindow("teShared")
  moveWindow("teShared", 7, 8)
  look("sharedHiddenMoved", "teShared")
  hideWindow("teWindow")
  showWindow("teWindow")
  look("sharedAfterReport", "teShared")
  step("deleteSharedLine", deleteCommandLine, "teShared")
  look("sharedLineDeleted", "teShared")
  step("lineAfterEdit", createCommandLine, "teShared", 3, 3, 30, 30)
  look("editThenLine", "teShared")
  deleteCommandLine("teShared")
  look("lineGoneAgain", "teShared")
  deleteTextEdit("teShared")
  -- And a scroll box before either
  assert(createScrollBox("teTriple", 1, 2, 100, 100))
  assert(createCommandLine("teTriple", 3, 3, 30, 30))
  step("editAfterBoxAndLine", createTextEdit, "teTriple", 5, 5, 50, 50)
  hideWindow("teTriple")
  deleteCommandLine("teTriple")
  look("tripleLineDeleted", "teTriple")
  deleteScrollBox("teTriple")
  look("tripleBoxDeleted", "teTriple")
  deleteTextEdit("teTriple")
  assert(createScrollBox("teBoxShared", 1, 2, 100, 100))
  hideWindow("teBoxShared")
  step("editAfterBox", createTextEdit, "teBoxShared", 5, 6, 40, 20)
  look("boxThenEdit", "teBoxShared")
  step("resizeBoxShared", resizeWindow, "teBoxShared", 60, 70)
  step("deleteBoxShared", deleteScrollBox, "teBoxShared")
  look("sharedBoxDeleted", "teBoxShared")
  step("editBeforeBox", createTextEdit, "teBoxLater", 5, 6, 40, 20)
  moveWindow("teBoxLater", 9, 9)
  createScrollBox("teBoxLater", 1, 2, 100, 100)
  deleteScrollBox("teBoxLater")
  look("movedBeforeBox", "teBoxLater")

  step("delete", deleteTextEdit, "teInMain")
  events("delete")
  look("deleted", "teInMain")
  step("getDeleted", getTextEditText, "teInMain")
  step("setDeleted", setTextEditText, "teInMain", "x")
  step("fontDeleted", setTextEditFontSize, "teInMain", 10)
  step("raiseDeleted", raiseWindow, "teInMain")
  step("deleteAgain", deleteTextEdit, "teInMain")
  step("deleteEmpty", deleteTextEdit, "")
  step("deleteMissing", deleteTextEdit, "teNoSuch")
  step("deleteBadName", deleteTextEdit, {})
  events("failedDeletes")
  step("recreate", createTextEdit, "teInMain", 1, 2, 30, 40)
  step("getRecreated", getTextEditText, "teInMain")
  step("deleteRecreated", deleteTextEdit, "teInMain")
  events("recreated")

  assert(createTextEdit("teBoxedLine", 6, 7, 30, 30))
  step("lineInBoxForEdit", createCommandLine, "teBox", "teBoxedLine", 1, 1, 20, 20)
  look("boxedLineOverEdit", "teBoxedLine")
  step("deleteBox", deleteScrollBox, "teBox")
  events("box")
  look("boxedAtOnce", "teBoxed")
  step("getBoxedAtOnce", getTextEditText, "teBoxed")
  -- A box in a box gives up its name when the outer one goes
  assert(createScrollBox("teOuter", 0, 0, 100, 100))
  assert(createScrollBox("teOuter", "teNested", 0, 0, 50, 50))
  hideWindow("teNested")
  step("editNamedForNested", createTextEdit, "teNested", 11, 12, 40, 20)
  deleteScrollBox("teOuter")
  closeUserWindow("teWindow")
  closeUserWindow("teTwin")
  deleteScrollBox("teTwin")
  -- What a deleted user window holds goes with it, once its deferred delete has run
  assert(openUserWindow("teDoomed", false, false, "f") == true, "openUserWindow did not answer true")
  assert(createTextEdit("teDoomed", "teDoomedEdit", 0, 0, 40, 20))
  assert(createTextEdit("teDoomedLine", 8, 9, 30, 30))
  assert(createCommandLine("teDoomed", "teDoomedLine", 1, 1, 20, 20))
  deleteMiniConsole("teDoomed")
  for _, name in ipairs({"teMain", "teNegative", "teInEmpty", "teBoxShared", "teBoxLater"}) do
    deleteTextEdit(name)
  end
  events("end")
  headlessTextEditAnswers = table.concat(answers, "\n")
end)
headlessTextEdit = ok and "ok" or tostring(err)
-- Run once deferred deletes have, which a spec cannot wait for
function headlessTextEditAfter()
  local answers = {}
  for _, name in ipairs({"teMain", "teWin", "teBoxed", "teLate", "teRider", "teBox", "teWindow", "teInTwin", "teDoomedEdit", "teDoomed"}) do
    answers[#answers + 1] = name .. ":" .. tostring(windowType(name))
  end
  for _, name in ipairs({"teNested", "teBoxedLine", "teDoomedLine"}) do
    answers[#answers + 1] = name .. ":" .. tostring(windowType(name)) .. ":" .. table.concat({getWindowGeometry(name)}, ",") .. ":" .. tostring(windowVisible(name))
  end
  answers[#answers + 1] = "events:" .. table.concat(deletedEvents, ",")
  return table.concat(answers, "\n")
end
)lua"));
        QVERIFY2(ran, "The Lua chunk did not run.");
        QCOMPARE(luaGlobalString(host, "headlessTextEdit"), qsl("ok"));
        const QStringList answers{qsl("create=[1]true"),
                                  qsl("created:type=[1]textedit,geometry=[4]10:20:200:100,visible=[1]true"),
                                  qsl("createAgain=[2]false:couldn't create text edit"),
                                  qsl("createEmpty=[2]false:a text edit cannot have an empty string as its name"),
                                  qsl("createInMissing=[2]false:window 'teNoSuch' not found"),
                                  qsl("lost:type=[2]nil:'teLost' is not a known label, any type of console, command line, text edit, nor scroll box,geometry=[2]nil:window \"teLost\" not "
                                      "found,visible=[2]nil:window \"teLost\" not found"),
                                  qsl("createInMain=[1]true"),
                                  qsl("inMain:type=[1]textedit,geometry=[4]1:2:30:40,visible=[1]true"),
                                  qsl("createInEmpty=[1]true"),
                                  qsl("inEmpty:type=[1]textedit,geometry=[4]1:2:30:40,visible=[1]true"),
                                  qsl("createNegative=[1]true"),
                                  qsl("negative:type=[1]textedit,geometry=[4]3:4:0:0,visible=[1]true"),
                                  qsl("createBadName=raised:createTextEdit: bad argument #1 type (text edit name as string expected, got table!)"),
                                  qsl("createBadX=raised:createTextEdit: bad argument #2 type (text edit x-coordinate as number expected, got string!)"),
                                  qsl("bad:type=[2]nil:'teBad' is not a known label, any type of console, command line, text edit, nor scroll box,geometry=[2]nil:window \"teBad\" not "
                                      "found,visible=[2]nil:window \"teBad\" not found"),
                                  qsl("getNew=[1]"),
                                  qsl("set=[1]true"),
                                  qsl("getSet=[1]hello"),
                                  qsl("setOdd=[1]true"),
                                  qsl("getOdd=[1]a\\9b\\10c\\10d e\\10f\\10g"),
                                  qsl("setNumber=[1]true"),
                                  qsl("getNumber=[1]42"),
                                  qsl("clear=[1]true"),
                                  qsl("getCleared=[1]"),
                                  qsl("readOnly=[1]true"),
                                  qsl("setReadOnly=[1]true"),
                                  qsl("getReadOnly=[1]read only"),
                                  qsl("writable=[1]true"),
                                  qsl("placeholder=[1]true"),
                                  qsl("sheet=[1]true"),
                                  qsl("font=[1]true"),
                                  qsl("fontSize=[1]true"),
                                  qsl("fontSizeNegative=[2]nil:size cannot be 0 or negative"),
                                  qsl("tab=[1]true"),
                                  qsl("getOther=[1]"),
                                  qsl("afterSetters:type=[1]textedit,geometry=[4]10:20:200:100,visible=[1]true"),
                                  qsl("missing.getTextEditText=[2]nil:text edit name 'teNoSuch' not found"),
                                  qsl("missing.clearTextEdit=[2]nil:text edit name 'teNoSuch' not found"),
                                  qsl("missing.setTextEditText=[2]nil:text edit name 'teNoSuch' not found"),
                                  qsl("missing.setTextEditPlaceholder=[2]nil:text edit name 'teNoSuch' not found"),
                                  qsl("missing.setTextEditStyleSheet=[2]nil:text edit name 'teNoSuch' not found"),
                                  qsl("missing.setTextEditFont=[2]nil:text edit name 'teNoSuch' not found"),
                                  qsl("missing.setTextEditFontSize=[2]nil:text edit name 'teNoSuch' not found"),
                                  qsl("missing.setTextEditReadOnly=[2]nil:text edit name 'teNoSuch' not found"),
                                  qsl("missing.setTextEditTabMovesFocus=[2]nil:text edit name 'teNoSuch' not found"),
                                  qsl("setEmptyName=[2]nil:text edit name '' not found"),
                                  qsl("setBadName=raised:setTextEditText: bad argument #1 type (text edit name as string expected, got table!)"),
                                  qsl("setBadText=raised:setTextEditText: bad argument #2 type (text as string expected, got table!)"),
                                  qsl("raise=[1]true"),
                                  qsl("lower=[1]true"),
                                  qsl("hide=[1]true"),
                                  qsl("hidden:type=[1]textedit,geometry=[4]10:20:200:100,visible=[1]false"),
                                  qsl("show=[1]true"),
                                  qsl("shown:type=[1]textedit,geometry=[4]10:20:200:100,visible=[1]true"),
                                  qsl("move=[1]true"),
                                  qsl("resize=[1]true"),
                                  qsl("moved:type=[1]textedit,geometry=[4]5:6:70:25,visible=[1]true"),
                                  qsl("resizeNegative=[1]true"),
                                  qsl("resizedNegative:type=[1]textedit,geometry=[4]5:6:0:0,visible=[1]true"),
                                  qsl("resizeBack=[1]true"),
                                  qsl("createInWindow=[1]true"),
                                  qsl("inWindow:type=[1]textedit,geometry=[4]1:1:50:20,visible=[1]true"),
                                  qsl("inHiddenWindow:type=[1]textedit,geometry=[4]1:1:50:20,visible=[1]false"),
                                  qsl("inShownWindow:type=[1]textedit,geometry=[4]1:1:50:20,visible=[1]true"),
                                  qsl("createInBox=[1]true"),
                                  qsl("inBox:type=[1]textedit,geometry=[4]2:2:40:20,visible=[1]true"),
                                  qsl("inHiddenBox:type=[1]textedit,geometry=[4]2:2:40:20,visible=[1]false"),
                                  qsl("createInHiddenBox=[1]true"),
                                  qsl("madeInHiddenBox:type=[1]textedit,geometry=[4]2:25:40:20,visible=[1]false"),
                                  qsl("hideBoxed=[1]true"),
                                  qsl("boxedStaysHidden:type=[1]textedit,geometry=[4]2:2:40:20,visible=[1]false"),
                                  qsl("textInBox=[1]true"),
                                  qsl("getInBox=[1]boxed"),
                                  qsl("intoBox=[1]true"),
                                  qsl("movedIntoBox:type=[1]textedit,geometry=[4]3:3:70:25,visible=[1]true"),
                                  qsl("movedInHiddenWindow:type=[1]textedit,geometry=[4]3:3:70:25,visible=[1]false"),
                                  qsl("intoMainHidden=[1]true"),
                                  qsl("movedToMainHidden:type=[1]textedit,geometry=[4]4:4:70:25,visible=[1]false"),
                                  qsl("showMoved=[1]true"),
                                  qsl("intoWindow=[1]true"),
                                  qsl("movedIntoWindow:type=[1]textedit,geometry=[4]9:9:70:25,visible=[1]true"),
                                  qsl("textAfterMove=[1]read only"),
                                  qsl("intoMain=[1]true"),
                                  qsl("createInMover=[1]true"),
                                  qsl("moverIntoMain=[1]true"),
                                  qsl("riderAfterMove:type=[1]textedit,geometry=[4]1:1:40:20,visible=[1]true"),
                                  qsl("createInTwin=[1]true"),
                                  qsl("twinWindowHidden:type=[1]textedit,geometry=[4]1:1:40:20,visible=[1]true"),
                                  qsl("lineFirst=[1]true"),
                                  qsl("editAfterLine=[1]true"),
                                  qsl("lineThenEdit:type=[1]commandline,geometry=[4]1:1:10:10,visible=[1]true"),
                                  qsl("sharedEditText=[1]true"),
                                  qsl("sharedLineText=[0]"),
                                  qsl("sharedGetEdit=[1]edit"),
                                  qsl("sharedGetLine=[1]line"),
                                  qsl("sharedHiddenMoved:type=[1]commandline,geometry=[4]7:8:10:10,visible=[1]false"),
                                  qsl("sharedAfterReport:type=[1]commandline,geometry=[4]7:8:10:10,visible=[1]false"),
                                  qsl("deleteSharedLine=[1]true"),
                                  qsl("sharedLineDeleted:type=[1]textedit,geometry=[4]2:2:20:20,visible=[1]true"),
                                  qsl("lineAfterEdit=[1]true"),
                                  qsl("editThenLine:type=[1]commandline,geometry=[4]3:3:30:30,visible=[1]true"),
                                  qsl("lineGoneAgain:type=[1]textedit,geometry=[4]2:2:20:20,visible=[1]true"),
                                  qsl("editAfterBoxAndLine=[1]true"),
                                  qsl("tripleLineDeleted:type=[1]scrollbox,geometry=[4]1:2:100:100,visible=[1]false"),
                                  qsl("tripleBoxDeleted:type=[1]textedit,geometry=[4]5:5:50:50,visible=[1]true"),
                                  qsl("editAfterBox=[1]true"),
                                  qsl("boxThenEdit:type=[1]scrollbox,geometry=[4]1:2:100:100,visible=[1]false"),
                                  qsl("resizeBoxShared=[1]true"),
                                  qsl("deleteBoxShared=[1]true"),
                                  qsl("sharedBoxDeleted:type=[1]textedit,geometry=[4]5:6:40:20,visible=[1]true"),
                                  qsl("editBeforeBox=[1]true"),
                                  qsl("movedBeforeBox:type=[1]textedit,geometry=[4]9:9:40:20,visible=[1]true"),
                                  qsl("delete=[1]true"),
                                  qsl("delete:events=teShared,teTriple,teInMain"),
                                  qsl("deleted:type=[2]nil:'teInMain' is not a known label, any type of console, command line, text edit, nor scroll box,geometry=[2]nil:window \"teInMain\" not "
                                      "found,visible=[2]nil:window \"teInMain\" not found"),
                                  qsl("getDeleted=[2]nil:text edit name 'teInMain' not found"),
                                  qsl("setDeleted=[2]nil:text edit name 'teInMain' not found"),
                                  qsl("fontDeleted=[2]nil:text edit name 'teInMain' not found"),
                                  qsl("raiseDeleted=[1]false"),
                                  qsl("deleteAgain=[2]false:text edit name 'teInMain' not found"),
                                  qsl("deleteEmpty=[2]false:a text edit cannot have an empty string as its name"),
                                  qsl("deleteMissing=[2]false:text edit name 'teNoSuch' not found"),
                                  qsl("deleteBadName=raised:deleteTextEdit: bad argument #1 type (text edit name as string expected, got table!)"),
                                  qsl("failedDeletes:events="),
                                  qsl("recreate=[1]true"),
                                  qsl("getRecreated=[1]"),
                                  qsl("deleteRecreated=[1]true"),
                                  qsl("recreated:events=teInMain"),
                                  qsl("lineInBoxForEdit=[1]true"),
                                  qsl("boxedLineOverEdit:type=[1]commandline,geometry=[4]1:1:20:20,visible=[1]true"),
                                  qsl("deleteBox=[1]true"),
                                  qsl("box:events="),
                                  qsl("boxedAtOnce:type=[1]textedit,geometry=[4]2:2:40:20,visible=[1]true"),
                                  qsl("getBoxedAtOnce=[1]boxed"),
                                  qsl("editNamedForNested=[1]true"),
                                  qsl("end:events=teMain,teNegative,teInEmpty,teBoxShared,teBoxLater")};
        QCOMPARE(luaGlobalString(host, "headlessTextEditAnswers"), answers.join(QLatin1Char('\n')));

        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        ran = host->getLuaInterpreter()->compileAndExecuteScript(qsl("headlessTextEditAfterAnswers = headlessTextEditAfter()"));
        QVERIFY2(ran, "The Lua chunk after the deferred deletes did not run.");
        const QStringList after{qsl("teMain:nil"),
                                qsl("teWin:textedit"),
                                qsl("teBoxed:nil"),
                                qsl("teLate:nil"),
                                qsl("teRider:nil"),
                                qsl("teBox:nil"),
                                qsl("teWindow:userwindow"),
                                qsl("teInTwin:nil"),
                                qsl("teDoomedEdit:nil"),
                                qsl("teDoomed:nil"),
                                qsl("teNested:textedit:11,12,40,20:true"),
                                qsl("teBoxedLine:textedit:6,7,30,30:true"),
                                qsl("teDoomedLine:textedit:8,9,30,30:true"),
                                qsl("events:")};
        QCOMPARE(luaGlobalString(host, "headlessTextEditAfterAnswers"), after.join(QLatin1Char('\n')));
        QVERIFY2(QApplication::topLevelWidgets().isEmpty(), "Making the text edits created a widget.");
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
        QCOMPARE(luaGlobalString(host, "scrollMainForms"), qsl("[0][1]30;[0][1]25;[0][1]20;[0][1]30;[2]nil:msg[1]true[2]nil:msg[1]true"));
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
                                      qsl("frozen=[2]nil:msg[1]true[0][1]60"),
                                      qsl("thawed=[2]nil:msg[1]true[0][1]60"),
                                      qsl("to10=[0][1]60"),
                                      qsl("to20=[0][1]60"),
                                      qsl("toMinus5=[0][1]60"),
                                      qsl("up3=[0][1]60"),
                                      qsl("down2=[0][1]60"),
                                      qsl("fedWhileBack=[1]65[1]65"),
                                      qsl("frozenWhileBack=[2]nil:msg[1]65"),
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
end)
headlessFontWrap = ok and "ok" or tostring(err)
)lua"));

        QVERIFY2(ran, "The Lua chunk did not run.");
        QCOMPARE(luaGlobalString(host, "headlessFontWrap"), qsl("ok"));
        // Main and fwWindow start in one family, so only the sizes show which console a label copied
        const auto labelPointSize = [host](const QString& name) {
            const auto font = host->labelFont(name);
            return font ? font->pointSize() : -1;
        };
        QVERIFY(startFont.pointSize() != 10);
        // The walk's setFontSize(13) reaches a label too
        QCOMPARE(labelPointSize(qsl("fwLabel")), 13);
        QCOMPARE(labelPointSize(qsl("fwWindowLabel")), 10);
        QCOMPARE(labelPointSize(qsl("fwLateLabel")), 13);
        QCOMPARE(labelPointSize(qsl("fwWindowLateLabel")), 13);

        const bool moved = host->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(
headlessFontWrap = "not run"
local ok, err = pcall(function()
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

        QVERIFY2(moved, "The Lua chunk moving labels did not run.");
        QCOMPARE(luaGlobalString(host, "headlessFontWrap"), qsl("ok"));

        const auto sized = [](const QFont& font, const int pointSize) {
            TFontAttributes attributes(font);
            attributes.mPointSize = pointSize;
            return attributes.makeFont();
        };
        // A sub-console's font is built as the TConsole constructor builds one, not copied from main's, and the
        // two differ in pitch, kerning and antialiasing, so where the default family is missing they fall back apart
        const auto subConsoleFont = [host](const int pointSize) {
            TFontAttributes attributes(host->fontsAntiAlias());
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
                               qsl("wrapLine=[1]true"),
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
        // A label has a font and a point size but no measured size, wrap or text of its own; a missing name has none of them
        const auto otherSteps = [&](const QString& font, const QString& familyAnswer, const int pointSize = 0) {
            const QString refused = qsl("[2]nil:msg");
            const bool label = pointSize > 0;
            return qsl("size=[2]nil:msg;avg=[2]nil:msg;font=%1;pt=%4;wrap=[2]nil:msg;wrap0=[2]nil:msg;wrap30=[2]nil:msg;wrapNow=[2]nil:msg;"
                       "indent=[2]nil:msg;indentNeg=[2]nil:msg;hanging=[2]nil:msg;wrapLine=[2]nil:msg;wrapBack=[2]nil:msg;pt0=[2]nil:msg;pt13=%5;"
                       "size13=[2]nil:msg;avg13=[2]nil:msg;fontEmpty=[2]nil:msg;fontMissing=[2]nil:msg;fontFamily=%2;familyFont=%3;familyPt=%6;"
                       "familySize=[2]nil:msg;familyAvg=[2]nil:msg")
                    .arg(font,
                         familyAnswer.isEmpty() ? refused : qsl("[1]true"),
                         familyAnswer.isEmpty() ? refused : familyAnswer,
                         label ? qsl("[1]%1").arg(pointSize) : refused,
                         label ? qsl("[1]true") : refused,
                         label ? qsl("[1]13") : refused);
        };

        QCOMPARE(luaGlobalString(host, "fwGlobal"), qsl("%1;%2;[1]%3;[1]%4;[1]100").arg(measured(startFont), averaged(startFont), family(startFont)).arg(startFont.pointSize()));
        QCOMPARE(luaGlobalString(host, "fwEmpty"), consoleSteps(startFont, 100));
        QCOMPARE(luaGlobalString(host, "fwMain"), consoleSteps(familyFont, 100));
        // Unlike main, a miniconsole or user window starts with no wrap at all
        QCOMPARE(luaGlobalString(host, "fwMiniSteps"), consoleSteps(subConsoleFont(12), 99999999));
        // Attribute by attribute too, as here the fallbacks of the two constructions measure alike
        QVERIFY(host->getLuaInterpreter()->compileAndExecuteScript(qsl("createMiniConsole('fwFreshMini', 0, 0, 300, 200)")));
        const TConsoleModel* pFreshMini = host->consoleModelNamed(qsl("fwFreshMini"));
        QVERIFY(pFreshMini);
        QVERIFY(TFontAttributes(pFreshMini->mUpperPaneFont) == TFontAttributes(subConsoleFont(12)));
        QCOMPARE(luaGlobalString(host, "fwWindowSteps"), consoleSteps(subConsoleFont(10), 99999999));
        QCOMPARE(luaGlobalString(host, "fwBufferSteps"), consoleSteps(subConsoleFont(14), 100));
        QCOMPARE(luaGlobalString(host, "fwLabelSteps"), otherSteps(qsl("[1]%1").arg(family(startFont)), qsl("[1]%1").arg(family(familyFont)), startFont.pointSize()));
        QCOMPARE(luaGlobalString(host, "fwMissing"), otherSteps(qsl("[2]nil:msg"), QString()));
        QCOMPARE(luaGlobalString(host, "fwLabelFonts"), qsl("%1;%2;%2").arg(family(subConsoleFont(10)), family(familyFont)));
        // Moved to another window, one given no font by setFont() takes the application's
        QCOMPARE(luaGlobalString(host, "fwMovedFonts"), qsl("%1;%1;%2;%2;%2").arg(family(QFont()), family(familyFont)));
        QVERIFY2(QApplication::topLevelWidgets().isEmpty(), "The font and wrap calls created a widget.");
    }

    // A label's sheet sets its font as Qt resolves one, and getLabelSizeHint() measures it, so each
    // step is taken by a real TLabel too, made as TMainConsole makes one, and the answers compared
    void test_headlessLabelStyleSheetFontContract()
    {
        const QString hostname = qsl("Test-Headless-Host-Label-Sheet-Font");
        QVERIFY2(HostManager::self()->addHost(hostname, QString(), QString(), QString()), "Could not create a profile with no main window.");
        Host* host = HostManager::self()->getHost(hostname);
        QVERIFY2(host, "The profile is not in the pool.");
        QVERIFY(!host->hasConsoleView());

        const bool ran = host->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(
function lsAnswer(f, ...)
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
  return described(pcall(f, ...))
end
lsMissing = lsAnswer(getLabelSizeHint, "lsNoSuchLabel")
createLabel("lsA", 0, 0, 100, 20, 1)
openUserWindow("lsWin")
createLabel("lsWin", "lsB", 0, 0, 100, 20, 1)
createLabel("lsC", 0, 0, 100, 20, 1)
createLabel("lsD", 0, 0, 100, 20, 1)
createLabel("lsE", 0, 0, 100, 20, 1)
)lua"));
        QVERIFY2(ran, "The Lua chunk did not run.");
        QCOMPARE(luaGlobalString(host, "lsMissing"), qsl("[2]nil:msg"));

        // setFont() refuses a family this machine does not have, so each role takes one it does
        const QStringList installed = FontManager::availableFonts();
        QStringList chosen{host->getDisplayFont().family()};
        const auto pick = [&installed, &chosen](const QStringList& candidates) {
            const auto usable = [&chosen](const QString& family) {
                return !chosen.contains(family) && !family.startsWith(QLatin1Char('.')) && !family.contains(QLatin1Char('\''));
            };
            QString family;
            for (const QString& candidate : candidates) {
                if (installed.contains(candidate) && usable(candidate)) {
                    family = candidate;
                    break;
                }
            }
            for (const QString& any : installed) {
                if (!family.isEmpty()) {
                    break;
                }
                if (usable(any)) {
                    family = any;
                }
            }
            chosen << family;
            return family;
        };
        const QString setFamily = pick({qsl("Liberation Serif"), qsl("Times New Roman"), qsl("Georgia")});
        const QString sheetFamily = pick({qsl("DejaVu Sans"), qsl("Helvetica"), qsl("Arial")});
        const QString shorthandFamily = pick({qsl("Liberation Mono"), qsl("Courier New"), qsl("Menlo")});
        QVERIFY2(!setFamily.isEmpty() && !sheetFamily.isEmpty() && !shorthandFamily.isEmpty(), "This machine has too few fonts to tell the steps apart.");

        QStringList mismatches;
        {
            QWidget mainParent;
            mainParent.setFont(host->getDisplayFont());
            mainParent.show();
            QWidget windowParent;
            // The user window's console font, which a label made in it starts with
            windowParent.setFont(host->labelFont(qsl("lsB")).value_or(QFont()));
            windowParent.show();
            const auto makeReference = [host](const QString& name, QWidget* pParent) {
                auto pLabel = new TLabel(host, name, pParent);
                pLabel->setAutoFillBackground(true);
                pLabel->resize(100, 20);
                pLabel->setContentsMargins(0, 0, 0, 0);
                pLabel->show();
                pLabel->setBackgroundColor(QColor(32, 32, 32, 255));
                return pLabel;
            };
            TLabel* pRefA = makeReference(qsl("lsRefA"), &mainParent);
            TLabel* pRefB = makeReference(qsl("lsRefB"), &windowParent);

            const auto compare = [&](const QString& step, const QString& name, const TLabel* pReference) {
                host->getLuaInterpreter()->compileAndExecuteScript(qsl("lsLast = lsAnswer(getLabelSizeHint, '%1')").arg(name));
                const QSize hint = pReference->sizeHint();
                const QString expected = qsl("[2]%1:%2").arg(hint.width()).arg(hint.height());
                if (const QString answer = luaGlobalString(host, "lsLast"); answer != expected) {
                    mismatches << qsl("%1: hint %2, a real label's %3").arg(step, answer, expected);
                }
                const QFont font = host->labelFont(name).value_or(QFont());
                if (font.key() != pReference->font().key()) {
                    mismatches << qsl("%1: font %2, a real label's %3").arg(step, font.toString(), pReference->font().toString());
                }
                if (const QString labelText = host->labelText(name).value_or(QString()); labelText != pReference->text()) {
                    mismatches << qsl("%1: text '%2', a real label's '%3'").arg(step, labelText, pReference->text());
                }
            };
            const auto step = [&](const QString& name, const QString& call, const QString& expected) {
                host->getLuaInterpreter()->compileAndExecuteScript(qsl("lsLast = lsAnswer(function() return %1 end)").arg(call));
                if (const QString answer = luaGlobalString(host, "lsLast"); answer != expected) {
                    mismatches << qsl("%1: %2 answered %3, not %4").arg(name, call, answer, expected);
                }
            };
            const auto sheet = [&](const QString& name, TLabel* pReference, const QString& styleSheet) {
                step(name, qsl("setLabelStyleSheet('%1', [[%2]])").arg(name, styleSheet), qsl("[1]true"));
                pReference->restyle(styleSheet);
                compare(qsl("%1 sheet '%2'").arg(name, styleSheet), name, pReference);
            };
            const auto text = [&](const QString& name, TLabel* pReference, const QString& labelText) {
                step(name, qsl("echo('%1', [[%2]])").arg(name, labelText), qsl("[1]true"));
                pReference->setText(labelText);
                compare(qsl("%1 text '%2'").arg(name, labelText), name, pReference);
            };
            const auto family = [&](const QString& name, TLabel* pReference, const QString& fontFamily) {
                step(name, qsl("setFont('%1', '%2')").arg(name, fontFamily), qsl("[1]true"));
                // TLuaInterpreter::setFont()'s font for a label
                pReference->setFont(host->createFontWithSettings(fontFamily, pReference->font().pointSize()));
                compare(qsl("%1 font %2").arg(name, fontFamily), name, pReference);
            };

            compare(qsl("lsA made"), qsl("lsA"), pRefA);
            // QLabel keeps the hint it gave before any text, through a font change
            family(qsl("lsA"), pRefA, setFamily);
            text(qsl("lsA"), pRefA, qsl("WWWWWWWWWW"));
            sheet(qsl("lsA"), pRefA, qsl("font-family: '%1';").arg(sheetFamily));
            sheet(qsl("lsA"), pRefA, qsl("font-size: 20px;"));
            sheet(qsl("lsA"), pRefA, qsl("font: bold 18px '%1';").arg(shorthandFamily));
            sheet(qsl("lsA"), pRefA, qsl("font-size: 16pt; font-style: italic;"));
            family(qsl("lsA"), pRefA, setFamily);
            step(qsl("lsA"), qsl("setLinkStyle('lsA', 'red', 'blue', false)"), qsl("[1]true"));
            pRefA->setLinkStyle(qsl("red"), qsl("blue"), false);
            sheet(qsl("lsA"), pRefA, qsl("font-size: 16pt; font-style: italic;"));
            sheet(qsl("lsA"), pRefA, qsl("color: red;"));
            text(qsl("lsA"), pRefA, qsl("<b>bold</b> text<br>second line"));
            // The link style replaces an anchor's own inline style, font size and all
            text(qsl("lsA"), pRefA, qsl("<a href=\"x\" style=\"font-size: 40pt\">WWW</a> and <a href=\"y\">y</a>"));
            step(qsl("lsA"), qsl("setLinkStyle('lsA', 'green', 'blue', true)"), qsl("[1]true"));
            pRefA->setLinkStyle(qsl("green"), qsl("blue"), true);
            step(qsl("lsA"), qsl("clearVisitedLinks('lsA')"), qsl("[1]true"));
            pRefA->clearVisitedLinks();
            compare(qsl("lsA visited links cleared"), qsl("lsA"), pRefA);
            sheet(qsl("lsA"), pRefA, QString());
            step(qsl("lsA"), qsl("setBackgroundColor('lsA', 10, 20, 30)"), qsl("[1]true"));
            pRefA->setBackgroundColor(QColor(10, 20, 30));
            compare(qsl("lsA background"), qsl("lsA"), pRefA);

            // A sheet handed back unchanged is skipped, so the cached hint stays, unless link colours were set since
            TLabel* pRefC = makeReference(qsl("lsRefC"), &mainParent);
            compare(qsl("lsC made"), qsl("lsC"), pRefC);
            family(qsl("lsC"), pRefC, setFamily);
            sheet(qsl("lsC"), pRefC, pRefC->styleSheet());
            step(qsl("lsC"), qsl("setLinkStyle('lsC', 'red', 'blue', false)"), qsl("[1]true"));
            pRefC->setLinkStyle(qsl("red"), qsl("blue"), false);
            sheet(qsl("lsC"), pRefC, pRefC->styleSheet());

            // Rules match a label by its class names and its object name, and apply in specificity and then sheet order
            TLabel* pRefD = makeReference(qsl("lsRefD"), &mainParent);
            const QString objectName = qsl("label_%1_lsD").arg(hostname);
            pRefD->setObjectName(objectName);
            text(qsl("lsD"), pRefD, qsl("WWWWWWWWWW"));
            const QStringList ruleSheets{
                    qsl("QLabel { font-size: 20px; }"),
                    qsl("QLabel#%1 { font-family: '%2'; } QLabel { font-size: 18px; font-family: '%3'; }").arg(objectName, sheetFamily, shorthandFamily),
                    qsl("QLabel#lsD { font-size: 40px; } TLabel { font-weight: bold; }"),
                    qsl("* { font-size: 11pt; } QLabel { font-style: italic; } * { font-size: 30px; }"),
                    qsl("QLabel:hover { font-size: 30px; } QLabel:!hover { font-size: 13pt; } QLabel::hover { font-weight: bold; }"),
                    qsl("QLabel:enabled:active { font-size: 21px; } QLabel:!enabled { font-size: 40px; } QLabel:hover:no-such-state { font-style: italic; }"),
                    qsl("QLabel::item { font-size: 40px; } QLabel:no-such-state { font-size: 40px; } QLabel:!focus { font-weight: bold; }"),
                    qsl(".TLabel { font-size: 15px; } .QLabel { font-size: 40px; } QPushButton, QFrame { font-style: italic; }"),
                    qsl("QLabel, TLabel#%1 { font-size: 14px; } QLabel { font-size: 24px; }").arg(objectName),
                    qsl("/* { */ QLabel { font: bold 16px '%1'; } QLabel { color: red; font-size: 19px; }").arg(shorthandFamily),
                    qsl(R"(QLabel { font-family: 'Rock\'n Roll'; font-size: 27px; } QLabel { font-weight: bold; })"),
                    qsl("@media screen { QLabel { font-size: 40px; } } QLabel { font-size: 17px; }"),
                    qsl("QLabel { font-size: 22px; } QLabel { font-size: 40px; "),
                    qsl("QLabel { font-size: 25px; } QLabel"),
                    qsl("QLabel { font-size: 26px; } QLabel { font-weight: bold; { font-size: 40px; } }"),
                    qsl("font-size: 23px;"),
            };
            for (const QString& ruleSheet : ruleSheets) {
                sheet(qsl("lsD"), pRefD, ruleSheet);
            }

            // A sheet's margin, border and padding frame the label, which QLabel then indents by an x
            const QStringList boxSheets{
                    qsl("padding: 4px;"),
                    qsl("padding: 1px 2px; border: 3px solid red;"),
                    qsl("padding: 1px 2px 5px; margin: 2px 3px 4px 6px;"),
                    qsl("padding-left: 7px; padding-top: 3; margin-right: 2px; margin-bottom: 1PX;"),
                    qsl("border-width: 4px;"),
                    qsl("border-width: 4px 2px; border-style: solid none;"),
                    qsl("border: 2px red;"),
                    qsl("border: solid 2px;"),
                    qsl("border: 2px solid; border-style: wavy;"),
                    qsl("border-left: 5px dashed; border-top-width: 3px; border-top-style: solid; border-bottom: 2px none;"),
                    qsl("padding: 2pt; margin: 1em; border: 0.5ex solid;"),
                    qsl("font-size: 20px; padding: 1em 1ex;"),
                    qsl("padding: 10%;"),
                    qsl("margin: 0;"),
                    qsl("margin: -2px;"),
                    qsl("color: red;"),
                    qsl("QLabel { padding: 6px; } QLabel#%1 { padding-left: 1px; } QPushButton { margin: 9px; }").arg(objectName),
                    qsl("QLabel:hover { padding: 9px; } * { border: 1px solid; } QLabel { border-right-width: 4px; }"),
                    // QCss::Parser reads a priority marker and nothing weighs it, so a later rule still wins
                    qsl("padding: 4px !important;"),
                    qsl("border-top-style: solid !important; border-top-width: 3px;"),
                    qsl("QLabel { padding: 6px !important; font-size: 30px !important; } QLabel { padding: 2px ! IMPORTANT; font-size: 18px; }"),
                    qsl("QLabel { padding: 2px; } QLabel { padding: 1px 5px!important; }"),
                    QString(),
            };
            for (const QString& boxSheet : boxSheets) {
                sheet(qsl("lsD"), pRefD, boxSheet);
            }
            TLabel* pRefE = makeReference(qsl("lsRefE"), &mainParent);
            compare(qsl("lsE made"), qsl("lsE"), pRefE);
            for (const QString& boxSheet : {qsl("padding: 3px 4px;"), qsl("border: 2px solid; margin: 1px 2px 3px;"), QString()}) {
                sheet(qsl("lsE"), pRefE, boxSheet);
            }

            text(qsl("lsB"), pRefB, qsl("WWWWWWWWWW"));
            sheet(qsl("lsB"), pRefB, qsl("font-size: 20px;"));
            step(qsl("lsB"), qsl("setWindow('main', 'lsB', 0, 0, true)"), qsl("[1]true"));
            pRefB->setParent(&mainParent);
            pRefB->show();
            compare(qsl("lsB moved to main"), qsl("lsB"), pRefB);
            sheet(qsl("lsB"), pRefB, QString());
            step(qsl("lsB"), qsl("setWindow('lsWin', 'lsB', 0, 0, true)"), qsl("[1]true"));
            pRefB->setParent(&windowParent);
            pRefB->show();
            compare(qsl("lsB moved back unstyled"), qsl("lsB"), pRefB);
        }
        QVERIFY2(mismatches.isEmpty(), qPrintable(mismatches.join(QChar::LineFeed)));
        QVERIFY2(QApplication::topLevelWidgets().isEmpty(), "The reference labels outlived the test.");
    }

    // The trim that raises sysBufferShrinkEvent runs inside TBuffer::append(), and echo goes on using the
    // console's model after that returns, so a handler deleting the console must not free it there and then.
    // What a label's and a text edit's sheet limit their size to, taken from the same Lua run in the GUI self-test profile
    void test_headlessSizeLimitsContract()
    {
        const QString hostname = qsl("Test Headless Host Size Limits");
        QVERIFY2(HostManager::self()->addHost(hostname, QString(), QString(), QString()), "Could not create a profile with no main window.");
        Host* host = HostManager::self()->getHost(hostname);
        QVERIFY2(host, "The profile is not in the pool.");
        QVERIFY(!host->hasConsoleView());

        QTemporaryDir imageDir;
        QVERIFY(imageDir.isValid());
        const auto writeFile = [&imageDir](const QString& name, const QByteArray& data) {
            QFile file(imageDir.filePath(name));
            return file.open(QIODevice::WriteOnly) && file.write(data) == data.size();
        };
        // Three 40x16 frames, a 24x10 image, and an SVG that is 30x12
        QVERIFY(writeFile(qsl("anim.gif"),
                          QByteArray::fromBase64("R0lGODlhKAAQAIEAAP8AAAAAAAAAAAAAACH/"
                                                 "C05FVFNDQVBFMi4wAwEAAAAh+"
                                                 "QQACgAAACwAAAAAKAAQAAAIKwABCBxIsKDBgwgTKlzIsKHDhxAjSpxIsaLFixgzatzIsaPHjyBDihypMCAAIfkEAQoAAQAsAAAAACgAEACBAP8AAAAAAAAAAAAACCsAAQgcSLCgwYMIEypcyLChw4"
                                                 "cQI0qcSLGixYsYM2rcyLGjx48gQ4ocqTAgACH5BAEKAAEALAAAAAAoABAAgQAA/wAAAAAAAAAAAAgrAAEIHEiwoMGDCBMqXMiwocOHECNKnEixosWLGDNq3Mixo8ePIEOKHKkwIAA7")));
        QVERIFY(writeFile(qsl("still.png"), QByteArray::fromBase64("iVBORw0KGgoAAAANSUhEUgAAABgAAAAKCAIAAAAhCwk/AAAAGUlEQVR4nGPk5ORkoAZgooopowaNGkQlAABsJgAvM+NphgAAAABJRU5ErkJggg==")));
        QVERIFY(writeFile(qsl("sized.svg"), QByteArrayLiteral("<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"30\" height=\"12\"><rect width=\"30\" height=\"12\" fill=\"red\"/></svg>\n")));
        QVERIFY(host->getLuaInterpreter()->compileAndExecuteScript(qsl("lsDir = [==[%1]==]").arg(imageDir.path())));
        const bool ran = host->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(
headlessSizeLimits = "not run"
local ok, err = pcall(function()
  local function described(called, ...)
    if not called then
      return "raised:" .. tostring((...))
    end
    local count = select("#", ...)
    local values = {}
    for i = 1, count do
      values[i] = tostring((select(i, ...)))
    end
    return "[" .. count .. "]" .. table.concat(values, ":")
  end
  local answers = {}
  local function step(label, f, ...)
    answers[#answers + 1] = label .. "=" .. described(pcall(f, ...))
  end
  local function look(label, name)
    answers[#answers + 1] = label .. "=" .. described(pcall(getWindowGeometry, name))
  end
  -- What a label answers to the sheet, its hint, its geometry, and its geometry once resized past the limits each way
  -- Each label's key, sheet, size and text, for the test to make the same label with
  local labelSpecs = {}
  local function label(key, sheet, w, h, text)
    labelSpecs[#labelSpecs + 1] = table.concat({key, sheet, w or 10, h or 10, text or ""}, "\t")
    local name = "ls" .. key
    createLabel(name, 0, 0, w or 10, h or 10, 1)
    if text then
      echo(name, text)
    end
    local parts = {described(pcall(setLabelStyleSheet, name, sheet)), described(pcall(getLabelSizeHint, name)), described(pcall(getWindowGeometry, name))}
    resizeWindow(name, 5, 500)
    parts[#parts + 1] = described(pcall(getWindowGeometry, name))
    resizeWindow(name, 500, 5)
    parts[#parts + 1] = described(pcall(getWindowGeometry, name))
    answers[#answers + 1] = key .. "=" .. table.concat(parts, " ")
  end
  -- A selector escapes the spaces of a profile name
  local profileId = (getProfileName():gsub(" ", "\\ "))
  label("min", "min-width: 50px; min-height: 30px")
  label("max", "max-width: 20px; max-height: 8px", 100, 100)
  label("minText", "min-width: 80px; padding: 2px", 10, 10, "hello")
  label("maxText", "max-width: 8px; padding: 2px", 10, 10, "hello")
  label("fixed", "width: 40px; height: 12px")
  label("widthOverMin", "width: 100px; min-width: 50px; height: 5px; min-height: 20px")
  label("heightOverMin", "height: 30px; min-height: 20px")
  label("heightUnderMax", "height: 30px; max-height: 50px", 100, 100)
  label("negativeMinHeight", "height: 10px; min-height: -5px; padding: 1px")
  label("huge", "min-width: 16777215px; padding: 2px", 1, 1)
  -- QRenderRule::boxSize() adds the box to the size in int, which wraps past INT_MAX, and QCss reads a length above it as INT_MAX
  label("wrapped", "min-width: 2147483640px; padding: 10px", 1, 1)
  label("wrappedMax", "max-width: 2147483640px; padding: 10px", 30, 30)
  label("overInt", "min-width: 3000000000px", 1, 1)
  label("overIntPadded", "min-width: 3000000000px; padding: 10px", 1, 1)
  label("hugePadding", "padding: 2147483640px; min-width: 20px", 1, 1)
  label("underInt", "min-width: -3000000000px; padding: 1px", 1, 1)
  label("hugeEm", "min-width: 200000000em", 1, 1)
  label("widthUnderMax", "width: 30px; max-width: 50px; height: 70px; max-height: 50px", 100, 100)
  label("minOverMax", "min-width: 50px; max-width: 20px; min-height: 9px; max-height: 40px", 30, 30)
  label("em", "font-size: 20px; min-width: 2em; max-height: 1em", 10, 100)
  label("ex", "min-width: 3ex", 1, 1)
  label("number", "min-width: 33; max-height: 44", 1, 100)
  label("percent", "min-width: 40%", 1, 1)
  label("pt", "min-width: 30pt", 1, 1)
  label("negativeMin", "min-width: -10px; padding: 3px", 1, 1)
  label("negativeMax", "max-width: -5px; max-height: -1px", 30, 30)
  label("zeroMax", "max-width: 0px; padding: 3px", 30, 30)
  label("important", "min-width: 45px !important", 1, 1)
  label("framed", "margin: 3px; border: 2px solid red; padding: 1px; min-height: 10px; max-width: 10px", 100, 1)
  label("borderNone", "border: 4px none; min-width: 10px", 1, 1)
  label("nativeBorder", "border: 4px native; padding: 1px", 10, 10, "hi")
  label("nativeStyle", "border-width: 4px; border-style: native", 10, 10, "hi")
  label("nativeStylePadded", "border-width: 4px; border-style: native; padding: 1px", 10, 10, "hi")
  label("nativeMin", "border: 4px native; min-width: 50px", 1, 1, "hi")
  label("nativeZeroMargin", "border: 4px native; margin: 0px", 10, 10, "hi")
  label("nativeRadius", "border: 4px native; border-radius: 0px; padding: 1px", 10, 10, "hi")
  label("nativeRadiusMin", "border: 4px native; border-radius: 3px; min-width: 50px", 1, 1, "hi")
  label("nativeTop", "border-top: 4px native; border-left: 3px solid", 10, 10, "hi")
  label("nativeLeft", "border-left: 3px native; border-top: 2px solid", 10, 10, "hi")
  label("nativeShort", "border: native", 10, 10, "hi")
  label("borderWidth", "border-width: 4px; padding: 1px", 10, 10, "hi")
  label("borderNoStyle", "border: 4px; padding: 1px", 10, 10, "hi")
  label("solidLeft", "border-left: 3px solid", 10, 10, "hi")
  label("leftWidth", "border-left-width: 3px; padding: 1px", 10, 10, "hi")
  label("leftStyled", "border-left-style: solid; border-left-width: 3px", 10, 10, "hi")
  label("topNone", "border-top-style: none; border-left: 3px solid", 10, 10, "hi")
  label("topLeft", "border-top: 2px solid; border-left: 3px solid", 10, 10, "hi")
  label("borderColor", "border-color: red; padding: 1px", 10, 10, "hi")
  label("radiusWidth", "border-radius: 2px; border-width: 4px; padding: 1px", 10, 10, "hi")
  label("cornerRadius", "border-top-left-radius: 2px; border-width: 4px; padding: 1px", 10, 10, "hi")
  label("zeroNative", "border-width: 0px; border-style: native; padding: 1px", 10, 10, "hi")
  label("widthMin", "border-width: 4px; min-width: 50px", 1, 1, "hi")
  label("solidSides", "border-style: solid; border-width: 2px 0px; min-width: 20px", 1, 1, "hi")
  label("borderImage", "border-image: none; border: 4px native; min-width: 50px", 1, 1, "hi")
  label("url", "min-width: 20px; background-image: url(C:/nothing.png)", 1, 1)
  label("quoted", [[min-width: 21px; font-family: "a:b"]], 1, 1)
  label("stray", "min-width: 22px; color", 1, 1)
  label("upper", "MIN-WIDTH: 23PX", 1, 1)
  label("twoValues", "min-width: 24px 30px", 1, 1)
  label("rule", "QLabel { min-width: 60px }", 1, 1)
  label("otherClass", "QPlainTextEdit { min-width: 60px }", 1, 1)
  label("enabled", "QLabel:enabled { min-width: 60px }", 1, 1)
  label("active", "QLabel:active { min-width: 60px }", 1, 1)
  label("inactive", "QLabel:!active { min-width: 60px }", 1, 1)
  label("hover", "QLabel:hover { min-width: 60px }", 1, 1)
  label("notEditable", "QLabel:!editable { min-width: 30px }", 1, 1)
  label("readOnly", "QLabel:read-only { min-width: 30px }", 1, 1)
  label("named", "#label_" .. profileId .. "_lsnamed { min-width: 60px }", 1, 1)
  label("otherNamed", "#label_" .. profileId .. "_lsother { min-width: 60px }", 1, 1)
  label("activePadded", "QLabel:active { padding: 3px; min-width: 40px }", 1, 1, "hi")
  label("inactivePadded", "QLabel:!active { padding: 3px; min-width: 40px }", 1, 1, "hi")
  -- A label made smaller keeps its size once its sheet lets it go, and is held again by a new one
  label("cleared", "min-width: 50px; max-height: 8px", 1, 100)
  step("cleared.clear", setLabelStyleSheet, "lscleared", "")
  look("cleared.after", "lscleared")
  resizeWindow("lscleared", 5, 500)
  look("cleared.free", "lscleared")
  step("cleared.hint", getLabelSizeHint, "lscleared")
  step("cleared.again", setLabelStyleSheet, "lscleared", "max-width: 3px;")
  look("cleared.held", "lscleared")
  step("cleared.color", setBackgroundColor, "lscleared", 255, 0, 0, 255)
  resizeWindow("lscleared", 50, 50)
  look("cleared.colored", "lscleared")
  -- A hidden label is held too
  createLabel("lshidden", 0, 0, 10, 10, 1)
  hideWindow("lshidden")
  step("hidden.sheet", setLabelStyleSheet, "lshidden", "min-width: 40px")
  look("hidden", "lshidden")
  showWindow("lshidden")
  look("hidden.shown", "lshidden")
  -- The hint of an image or a movie is held by the minimum, that of an SVG is not
  local png = lsDir .. "/still.png"
  local svg = lsDir .. "/sized.svg"
  createLabel("lsimage", 0, 0, 10, 10, 1)
  step("image.set", setBackgroundImage, "lsimage", png)
  step("image.hint", getLabelSizeHint, "lsimage")
  step("image.sheet", setLabelStyleSheet, "lsimage", "min-width: 50px; min-height: 3px")
  step("image.hintMin", getLabelSizeHint, "lsimage")
  createLabel("lsmovie", 0, 0, 10, 10, 1)
  step("movie.set", setMovie, "lsmovie", lsDir .. "/anim.gif")
  step("movie.sheet", setLabelStyleSheet, "lsmovie", "min-width: 60px; min-height: 3px")
  step("movie.hint", getLabelSizeHint, "lsmovie")
  createLabel("lssvg", 0, 0, 10, 10, 1)
  step("svg.set", setBackgroundImage, "lssvg", svg)
  step("svg.sheet", setLabelStyleSheet, "lssvg", "min-width: 50px")
  step("svg.hint", getLabelSizeHint, "lssvg")
  look("svg", "lssvg")
  -- The sheet of a text edit limits it the same way
  local function textEdit(key, sheet, w, h)
    local name = "ls" .. key
    createTextEdit(name, 0, 0, w or 10, h or 10)
    local parts = {described(pcall(setTextEditStyleSheet, name, sheet)), described(pcall(getWindowGeometry, name))}
    resizeWindow(name, 5, 500)
    parts[#parts + 1] = described(pcall(getWindowGeometry, name))
    resizeWindow(name, 500, 5)
    parts[#parts + 1] = described(pcall(getWindowGeometry, name))
    answers[#answers + 1] = key .. "=" .. table.concat(parts, " ")
  end
  textEdit("teMin", "min-width: 50px; min-height: 30px")
  textEdit("teMax", "max-width: 20px; max-height: 8px", 100, 100)
  textEdit("tePadded", "min-width: 50px; padding: 2px; border: 3px solid", 1, 1)
  textEdit("teWidth", "width: 100px; min-width: 50px", 1, 1)
  textEdit("teRule", "QPlainTextEdit { min-width: 60px }", 1, 1)
  textEdit("teOwnRule", "TTextBox { min-height: 60px }", 1, 1)
  textEdit("teScrollRule", "QAbstractScrollArea { min-height: 61px }", 1, 1)
  textEdit("teLabelRule", "QLabel { min-width: 60px }", 1, 1)
  textEdit("teNative", "border-style: native; min-width: 50px", 1, 1)
  textEdit("teNativeWidth", "border: 4px native; min-width: 50px", 1, 1)
  textEdit("teEm", "font-size: 20px; min-width: 2em", 1, 1)
  textEdit("teSolid", "border: 2px solid; min-width: 50px", 1, 1)
  textEdit("teNoStyle", "border: 4px; min-width: 50px", 1, 1)
  textEdit("teWidthOnly", "border-width: 3px; min-width: 50px", 1, 1)
  textEdit("teSolidOnly", "border-style: solid; min-width: 50px", 1, 1)
  textEdit("tePadOnly", "padding: 2px; min-width: 50px", 1, 1)
  textEdit("teMargin", "margin: 1px; min-width: 50px", 1, 1)
  textEdit("teTop", "border-top: 2px solid; min-height: 50px; min-width: 50px", 1, 1)
  textEdit("teLeftNative", "border-left: 2px native; min-height: 50px; min-width: 50px", 1, 1)
  textEdit("teRadius", "border-radius: 2px; min-width: 50px", 1, 1)
  textEdit("teCorner", "border-top-left-radius: 2px; min-width: 50px", 1, 1)
  textEdit("teColor", "border-color: red; min-width: 50px", 1, 1)
  textEdit("teNone", "border: none; min-width: 50px", 1, 1)
  textEdit("teColorOnly", "color: red; min-width: 50px", 1, 1)
  textEdit("teEditable", "QPlainTextEdit:editable { min-width: 60px }", 1, 1)
  textEdit("teNotReadOnly", "QPlainTextEdit:!read-only { min-width: 60px }", 1, 1)
  textEdit("teReadOnlyRule", "QPlainTextEdit:read-only { min-width: 60px }", 1, 1)
  textEdit("teActive", "QPlainTextEdit:active { min-width: 60px }", 1, 1)
  textEdit("teInactive", "QPlainTextEdit:!active { padding: 2px; min-width: 50px }", 1, 1)
  textEdit("teFocus", "QPlainTextEdit:focus { min-width: 60px }", 1, 1)
  textEdit("teNamed", "#textEdit_" .. profileId .. "_lsteNamed { min-width: 60px }", 1, 1)
  textEdit("teLabelNamed", "#label_" .. profileId .. "_lsteLabelNamed { min-width: 60px }", 1, 1)
  textEdit("teFrame", "QFrame { min-width: 60px }", 1, 1)
  textEdit("teStar", "* { min-width: 60px }", 1, 1)
  textEdit("teRuleLast", "QPlainTextEdit { min-width: 50px } QAbstractScrollArea { min-width: 70px }", 1, 1)
  textEdit("teRuleFirst", "QAbstractScrollArea { min-width: 70px } QPlainTextEdit { min-width: 50px }", 1, 1)
  createTextEdit("lsteReadOnly", 0, 0, 1, 1)
  setTextEditReadOnly("lsteReadOnly", true)
  step("teReadOnly.sheet", setTextEditStyleSheet, "lsteReadOnly", "QPlainTextEdit:read-only { min-width: 60px }")
  look("teReadOnly", "lsteReadOnly")
  for _, key in ipairs({"Editable", "NotReadOnly"}) do
    createTextEdit("lsteReadOnly" .. key, 0, 0, 1, 1)
    setTextEditReadOnly("lsteReadOnly" .. key, true)
    step("teReadOnly" .. key .. ".sheet", setTextEditStyleSheet, "lsteReadOnly" .. key, key == "Editable" and "QPlainTextEdit:editable { min-width: 60px }" or "QPlainTextEdit:!read-only { min-width: 60px }")
    look("teReadOnly" .. key, "lsteReadOnly" .. key)
  end
  -- Being made read-only does not restyle a text edit, but its next sheet sees it
  createTextEdit("lsteLater", 0, 0, 1, 1)
  setTextEditStyleSheet("lsteLater", "QPlainTextEdit:read-only { min-width: 60px }")
  step("teLater.readOnly", setTextEditReadOnly, "lsteLater", true)
  resizeWindow("lsteLater", 5, 5)
  look("teLater.made", "lsteLater")
  setTextEditStyleSheet("lsteLater", "QPlainTextEdit:read-only { min-width: 60px } ")
  look("teLater.restyled", "lsteLater")
  setTextEditReadOnly("lsteLater", false)
  resizeWindow("lsteLater", 5, 5)
  look("teLater.editable", "lsteLater")
  setTextEditStyleSheet("lsteLater", "QPlainTextEdit:read-only { min-width: 60px }")
  resizeWindow("lsteLater", 5, 5)
  look("teLater.editableRestyled", "lsteLater")
  textEdit("teCleared", "min-width: 50px", 1, 1)
  step("teCleared.clear", setTextEditStyleSheet, "lsteCleared", "")
  resizeWindow("lsteCleared", 5, 5)
  look("teCleared.free", "lsteCleared")
  -- A text edit behind a command line of the same name is still held
  createTextEdit("lsteShared", 0, 0, 10, 10)
  createCommandLine("lsteShared", 0, 0, 10, 10)
  step("teShared.sheet", setTextEditStyleSheet, "lsteShared", "min-width: 50px")
  look("teShared", "lsteShared")
  deleteCommandLine("lsteShared")
  look("teShared.alone", "lsteShared")
  resizeWindow("lsteShared", 5, 5)
  look("teShared.resized", "lsteShared")
  -- and one behind a scroll box of the same name leaves the name to the scroll box
  createScrollBox("lsteBoxed", 0, 0, 40, 40)
  createTextEdit("lsteBoxed", 0, 0, 10, 10)
  step("teBoxed.sheet", setTextEditStyleSheet, "lsteBoxed", "min-width: 50px")
  look("teBoxed", "lsteBoxed")
  step("teMissing.sheet", setTextEditStyleSheet, "lsteMissing", "min-width: 50px")
  -- Moving a text edit to another parent restyles it in the state it is in then, as moving its scroll box does not
  local readOnlyRule = "QPlainTextEdit:read-only { min-width: 60px }"
  openUserWindow("lsWindow", false, false, "f")
  createTextEdit("lsteMoved", 0, 0, 1, 1)
  setTextEditStyleSheet("lsteMoved", readOnlyRule)
  setTextEditReadOnly("lsteMoved", true)
  step("teMoved.toSame", setWindow, "main", "lsteMoved", 3, 3, true)
  resizeWindow("lsteMoved", 5, 5)
  look("teMoved.same", "lsteMoved")
  step("teMoved.toWindow", setWindow, "lsWindow", "lsteMoved", 0, 0, true)
  resizeWindow("lsteMoved", 5, 5)
  look("teMoved.window", "lsteMoved")
  setTextEditReadOnly("lsteMoved", false)
  step("teMoved.toMain", setWindow, "main", "lsteMoved", 0, 0, true)
  resizeWindow("lsteMoved", 5, 5)
  look("teMoved.back", "lsteMoved")
  createScrollBox("lsBox", 0, 0, 100, 100)
  createTextEdit("lsBox", "lsteInBox", 0, 0, 1, 1)
  setTextEditStyleSheet("lsteInBox", readOnlyRule)
  setTextEditReadOnly("lsteInBox", true)
  step("teInBox.moveBox", setWindow, "lsWindow", "lsBox", 0, 0, true)
  resizeWindow("lsteInBox", 5, 5)
  look("teInBox.box", "lsteInBox")
  step("teInBox.moveOut", setWindow, "lsWindow", "lsteInBox", 0, 0, true)
  resizeWindow("lsteInBox", 5, 5)
  look("teInBox.out", "lsteInBox")
  for key in ("borderWidth borderNoStyle solidLeft leftWidth leftStyled topNone topLeft borderColor radiusWidth cornerRadius zeroNative widthMin solidSides borderImage url quoted stray upper twoValues notEditable readOnly activePadded inactivePadded named otherNamed min max minText maxText fixed widthOverMin heightOverMin heightUnderMax negativeMinHeight huge wrapped wrappedMax overInt overIntPadded hugePadding underInt hugeEm widthUnderMax minOverMax em ex number percent pt negativeMin negativeMax zeroMax important framed borderNone nativeBorder nativeStyle nativeStylePadded nativeMin nativeZeroMargin nativeRadius nativeRadiusMin nativeTop nativeLeft nativeShort rule otherClass enabled active inactive hover cleared hidden image movie svg"):gmatch("%S+") do
    deleteLabel("ls" .. key)
  end
  for key in ("teMin teMax tePadded teWidth teRule teOwnRule teScrollRule teLabelRule teNative teNativeWidth teEm teReadOnly teCleared teShared teSolid teNoStyle teWidthOnly teSolidOnly tePadOnly teMargin teTop teLeftNative teRadius teCorner teColor teNone teColorOnly teEditable teNotReadOnly teActive teInactive teFocus teNamed teLabelNamed teFrame teStar teRuleLast teRuleFirst teReadOnlyRule teReadOnlyEditable teReadOnlyNotReadOnly teLater teBoxed teMoved teInBox"):gmatch("%S+") do
    deleteTextEdit("ls" .. key)
  end
  deleteScrollBox("lsteBoxed")
  deleteScrollBox("lsBox")
  closeUserWindow("lsWindow")
  headlessSizeLimitsLabels = table.concat(labelSpecs, "\n")
  headlessSizeLimitsAnswers = table.concat(answers, "\n")
end)
headlessSizeLimits = ok and "ok" or tostring(err)
)lua"));
        QVERIFY2(ran, "The Lua chunk did not run.");
        QCOMPARE(luaGlobalString(host, "headlessSizeLimits"), qsl("ok"));
        // QCss measures a text edit's em in the application font at the sheet's size
        QFont sheetFont;
        sheetFont.setPixelSize(20);
        const int twoEm = 2 * QFontMetrics(sheetFont).height();
        QStringList answers{qsl("min=[1]true {hint} [4]0:0:50:30 [4]0:0:50:500 [4]0:0:500:30"),
                            qsl("max=[1]true {hint} [4]0:0:20:8 [4]0:0:5:8 [4]0:0:20:5"),
                            qsl("minText=[1]true {hint} [4]0:0:84:10 [4]0:0:84:500 [4]0:0:500:5"),
                            qsl("maxText=[1]true {hint} [4]0:0:10:10 [4]0:0:5:500 [4]0:0:12:5"),
                            qsl("fixed=[1]true {hint} [4]0:0:10:10 [4]0:0:5:500 [4]0:0:500:5"),
                            qsl("widthOverMin=[1]true {hint} [4]0:0:100:20 [4]0:0:100:500 [4]0:0:500:20"),
                            qsl("heightOverMin=[1]true {hint} [4]0:0:10:30 [4]0:0:5:500 [4]0:0:500:30"),
                            qsl("heightUnderMax=[1]true {hint} [4]0:0:100:30 [4]0:0:5:30 [4]0:0:500:5"),
                            qsl("negativeMinHeight=[1]true {hint} [4]0:0:10:12 [4]0:0:5:500 [4]0:0:500:12"),
                            qsl("huge=[1]true {hint} [4]0:0:16777215:1 [4]0:0:16777215:500 [4]0:0:16777215:5"),
                            qsl("wrapped=[1]true {hint} [4]0:0:1:1 [4]0:0:5:500 [4]0:0:500:5"),
                            qsl("wrappedMax=[1]true {hint} [4]0:0:30:30 [4]0:0:5:500 [4]0:0:500:5"),
                            qsl("overInt=[1]true {hint} [4]0:0:16777215:1 [4]0:0:16777215:500 [4]0:0:16777215:5"),
                            qsl("overIntPadded=[1]true {hint} [4]0:0:1:1 [4]0:0:5:500 [4]0:0:500:5"),
                            qsl("hugePadding=[1]true {hint} [4]0:0:4:1 [4]0:0:5:500 [4]0:0:500:5"),
                            qsl("underInt=[1]true {hint} [4]0:0:1:1 [4]0:0:5:500 [4]0:0:500:5"),
                            qsl("hugeEm=[1]true {hint} [4]0:0:16777215:1 [4]0:0:16777215:500 [4]0:0:16777215:5"),
                            qsl("widthUnderMax=[1]true {hint} [4]0:0:30:50 [4]0:0:5:50 [4]0:0:30:5"),
                            qsl("minOverMax=[1]true {hint} [4]0:0:50:30 [4]0:0:50:40 [4]0:0:50:9"),
                            qsl("em=[1]true {hint} {made} {tall} {wide}"),
                            qsl("ex=[1]true {hint} {made} {tall} {wide}"),
                            qsl("number=[1]true {hint} [4]0:0:33:44 [4]0:0:33:44 [4]0:0:500:5"),
                            qsl("percent=[1]true {hint} [4]0:0:40:1 [4]0:0:40:500 [4]0:0:500:5"),
                            qsl("pt=[1]true {hint} [4]0:0:1:1 [4]0:0:5:500 [4]0:0:500:5"),
                            qsl("negativeMin=[1]true {hint} [4]0:0:1:1 [4]0:0:5:500 [4]0:0:500:5"),
                            qsl("negativeMax=[1]true {hint} [4]0:0:0:30 [4]0:0:0:500 [4]0:0:0:5"),
                            qsl("zeroMax=[1]true {hint} [4]0:0:6:30 [4]0:0:5:500 [4]0:0:6:5"),
                            qsl("important=[1]true {hint} [4]0:0:45:1 [4]0:0:45:500 [4]0:0:500:5"),
                            qsl("framed=[1]true {hint} [4]0:0:22:22 [4]0:0:5:500 [4]0:0:22:22"),
                            qsl("borderNone=[1]true {hint} [4]0:0:10:1 [4]0:0:10:500 [4]0:0:500:5"),
                            qsl("nativeBorder=[1]true {hint} [4]0:0:10:10 [4]0:0:5:500 [4]0:0:500:5"),
                            qsl("nativeStyle=[1]true {hint} [4]0:0:10:10 [4]0:0:5:500 [4]0:0:500:5"),
                            qsl("nativeStylePadded=[1]true {hint} [4]0:0:10:10 [4]0:0:5:500 [4]0:0:500:5"),
                            qsl("nativeMin=[1]true {hint} [4]0:0:58:1 [4]0:0:58:500 [4]0:0:500:5"),
                            qsl("nativeZeroMargin=[1]true {hint} [4]0:0:10:10 [4]0:0:5:500 [4]0:0:500:5"),
                            qsl("nativeRadius=[1]true {hint} [4]0:0:10:10 [4]0:0:5:500 [4]0:0:500:5"),
                            qsl("nativeRadiusMin=[1]true {hint} [4]0:0:50:1 [4]0:0:50:500 [4]0:0:500:5"),
                            qsl("nativeTop=[1]true {hint} [4]0:0:10:10 [4]0:0:5:500 [4]0:0:500:5"),
                            qsl("nativeLeft=[1]true {hint} [4]0:0:10:10 [4]0:0:5:500 [4]0:0:500:5"),
                            qsl("nativeShort=[1]true {hint} [4]0:0:10:10 [4]0:0:5:500 [4]0:0:500:5"),
                            qsl("borderWidth=[1]true {hint} [4]0:0:10:10 [4]0:0:5:500 [4]0:0:500:5"),
                            qsl("borderNoStyle=[1]true {hint} [4]0:0:10:10 [4]0:0:5:500 [4]0:0:500:5"),
                            qsl("solidLeft=[1]true {hint} [4]0:0:10:10 [4]0:0:5:500 [4]0:0:500:5"),
                            qsl("leftWidth=[1]true {hint} [4]0:0:10:10 [4]0:0:5:500 [4]0:0:500:5"),
                            qsl("leftStyled=[1]true {hint} [4]0:0:10:10 [4]0:0:5:500 [4]0:0:500:5"),
                            qsl("topNone=[1]true {hint} [4]0:0:10:10 [4]0:0:5:500 [4]0:0:500:5"),
                            qsl("topLeft=[1]true {hint} [4]0:0:10:10 [4]0:0:5:500 [4]0:0:500:5"),
                            qsl("borderColor=[1]true {hint} [4]0:0:10:10 [4]0:0:5:500 [4]0:0:500:5"),
                            qsl("radiusWidth=[1]true {hint} [4]0:0:10:10 [4]0:0:5:500 [4]0:0:500:5"),
                            qsl("cornerRadius=[1]true {hint} [4]0:0:10:10 [4]0:0:5:500 [4]0:0:500:5"),
                            qsl("zeroNative=[1]true {hint} [4]0:0:10:10 [4]0:0:5:500 [4]0:0:500:5"),
                            qsl("widthMin=[1]true {hint} [4]0:0:58:1 [4]0:0:58:500 [4]0:0:500:5"),
                            qsl("solidSides=[1]true {hint} [4]0:0:20:1 [4]0:0:20:500 [4]0:0:500:5"),
                            qsl("borderImage=[1]true {hint} [4]0:0:58:1 [4]0:0:58:500 [4]0:0:500:5"),
                            qsl("url=[1]true {hint} [4]0:0:20:1 [4]0:0:20:500 [4]0:0:500:5"),
                            qsl("quoted=[1]true {hint} [4]0:0:21:1 [4]0:0:21:500 [4]0:0:500:5"),
                            qsl("stray=[1]true {hint} [4]0:0:22:1 [4]0:0:22:500 [4]0:0:500:5"),
                            qsl("upper=[1]true {hint} [4]0:0:23:1 [4]0:0:23:500 [4]0:0:500:5"),
                            qsl("twoValues=[1]true {hint} [4]0:0:24:1 [4]0:0:24:500 [4]0:0:500:5"),
                            qsl("rule=[1]true {hint} [4]0:0:60:1 [4]0:0:60:500 [4]0:0:500:5"),
                            qsl("otherClass=[1]true {hint} [4]0:0:1:1 [4]0:0:5:500 [4]0:0:500:5"),
                            qsl("enabled=[1]true {hint} [4]0:0:60:1 [4]0:0:60:500 [4]0:0:500:5"),
                            qsl("active=[1]true {hint} [4]0:0:1:1 [4]0:0:5:500 [4]0:0:500:5"),
                            qsl("inactive=[1]true {hint} [4]0:0:60:1 [4]0:0:60:500 [4]0:0:500:5"),
                            qsl("hover=[1]true {hint} [4]0:0:1:1 [4]0:0:5:500 [4]0:0:500:5"),
                            qsl("notEditable=[1]true {hint} [4]0:0:30:1 [4]0:0:30:500 [4]0:0:500:5"),
                            qsl("readOnly=[1]true {hint} [4]0:0:1:1 [4]0:0:5:500 [4]0:0:500:5"),
                            qsl("named=[1]true {hint} [4]0:0:60:1 [4]0:0:60:500 [4]0:0:500:5"),
                            qsl("otherNamed=[1]true {hint} [4]0:0:1:1 [4]0:0:5:500 [4]0:0:500:5"),
                            qsl("activePadded=[1]true {hint} [4]0:0:1:1 [4]0:0:5:500 [4]0:0:500:5"),
                            qsl("inactivePadded=[1]true {hint} [4]0:0:46:1 [4]0:0:46:500 [4]0:0:500:5"),
                            qsl("cleared=[1]true {hint} [4]0:0:50:8 [4]0:0:50:8 [4]0:0:500:5"),
                            qsl("cleared.clear=[1]true"),
                            qsl("cleared.after=[4]0:0:500:5"),
                            qsl("cleared.free=[4]0:0:5:500"),
                            qsl("cleared.hint={hint}"),
                            qsl("cleared.again=[1]true"),
                            qsl("cleared.held=[4]0:0:3:500"),
                            qsl("cleared.color=[1]true"),
                            qsl("cleared.colored=[4]0:0:3:50"),
                            qsl("hidden.sheet=[1]true"),
                            qsl("hidden=[4]0:0:40:10"),
                            qsl("hidden.shown=[4]0:0:40:10"),
                            qsl("image.set=[1]true"),
                            qsl("image.hint=[2]24:10"),
                            qsl("image.sheet=[1]true"),
                            qsl("image.hintMin=[2]50:10"),
                            qsl("movie.set=[1]true"),
                            qsl("movie.sheet=[1]true"),
                            qsl("movie.hint=[2]60:16"),
                            qsl("svg.set=[1]true"),
                            qsl("svg.sheet=[1]true"),
                            qsl("svg.hint=[2]30:12"),
                            qsl("svg=[4]0:0:50:10"),
                            qsl("teMin=[1]true [4]0:0:52:32 [4]0:0:52:500 [4]0:0:500:32"),
                            qsl("teMax=[1]true [4]0:0:22:10 [4]0:0:5:10 [4]0:0:22:5"),
                            qsl("tePadded=[1]true [4]0:0:60:1 [4]0:0:60:500 [4]0:0:500:5"),
                            qsl("teWidth=[1]true [4]0:0:102:1 [4]0:0:102:500 [4]0:0:500:5"),
                            qsl("teRule=[1]true [4]0:0:62:1 [4]0:0:62:500 [4]0:0:500:5"),
                            qsl("teOwnRule=[1]true [4]0:0:1:62 [4]0:0:5:500 [4]0:0:500:62"),
                            qsl("teScrollRule=[1]true [4]0:0:1:63 [4]0:0:5:500 [4]0:0:500:63"),
                            qsl("teLabelRule=[1]true [4]0:0:1:1 [4]0:0:5:500 [4]0:0:500:5"),
                            qsl("teNative=[1]true [4]0:0:52:1 [4]0:0:52:500 [4]0:0:500:5"),
                            qsl("teNativeWidth=[1]true [4]0:0:58:1 [4]0:0:58:500 [4]0:0:500:5"),
                            qsl("teEm=[1]true [4]0:0:%1:1 [4]0:0:%1:500 [4]0:0:500:5").arg(twoEm + 2),
                            qsl("teSolid=[1]true [4]0:0:54:1 [4]0:0:54:500 [4]0:0:500:5"),
                            qsl("teNoStyle=[1]true [4]0:0:50:1 [4]0:0:50:500 [4]0:0:500:5"),
                            qsl("teWidthOnly=[1]true [4]0:0:56:1 [4]0:0:56:500 [4]0:0:500:5"),
                            qsl("teSolidOnly=[1]true [4]0:0:50:1 [4]0:0:50:500 [4]0:0:500:5"),
                            qsl("tePadOnly=[1]true [4]0:0:56:1 [4]0:0:56:500 [4]0:0:500:5"),
                            qsl("teMargin=[1]true [4]0:0:54:1 [4]0:0:54:500 [4]0:0:500:5"),
                            qsl("teTop=[1]true [4]0:0:52:53 [4]0:0:52:500 [4]0:0:500:53"),
                            qsl("teLeftNative=[1]true [4]0:0:53:52 [4]0:0:53:500 [4]0:0:500:52"),
                            qsl("teRadius=[1]true [4]0:0:50:1 [4]0:0:50:500 [4]0:0:500:5"),
                            qsl("teCorner=[1]true [4]0:0:50:1 [4]0:0:50:500 [4]0:0:500:5"),
                            qsl("teColor=[1]true [4]0:0:52:1 [4]0:0:52:500 [4]0:0:500:5"),
                            qsl("teNone=[1]true [4]0:0:50:1 [4]0:0:50:500 [4]0:0:500:5"),
                            qsl("teColorOnly=[1]true [4]0:0:52:1 [4]0:0:52:500 [4]0:0:500:5"),
                            qsl("teEditable=[1]true [4]0:0:62:1 [4]0:0:62:500 [4]0:0:500:5"),
                            qsl("teNotReadOnly=[1]true [4]0:0:62:1 [4]0:0:62:500 [4]0:0:500:5"),
                            qsl("teReadOnlyRule=[1]true [4]0:0:1:1 [4]0:0:5:500 [4]0:0:500:5"),
                            qsl("teActive=[1]true [4]0:0:1:1 [4]0:0:5:500 [4]0:0:500:5"),
                            qsl("teInactive=[1]true [4]0:0:56:1 [4]0:0:56:500 [4]0:0:500:5"),
                            qsl("teFocus=[1]true [4]0:0:1:1 [4]0:0:5:500 [4]0:0:500:5"),
                            qsl("teNamed=[1]true [4]0:0:62:1 [4]0:0:62:500 [4]0:0:500:5"),
                            qsl("teLabelNamed=[1]true [4]0:0:1:1 [4]0:0:5:500 [4]0:0:500:5"),
                            qsl("teFrame=[1]true [4]0:0:62:1 [4]0:0:62:500 [4]0:0:500:5"),
                            qsl("teStar=[1]true [4]0:0:62:1 [4]0:0:62:500 [4]0:0:500:5"),
                            qsl("teRuleLast=[1]true [4]0:0:72:1 [4]0:0:72:500 [4]0:0:500:5"),
                            qsl("teRuleFirst=[1]true [4]0:0:52:1 [4]0:0:52:500 [4]0:0:500:5"),
                            qsl("teReadOnly.sheet=[1]true"),
                            qsl("teReadOnly=[4]0:0:62:1"),
                            qsl("teReadOnlyEditable.sheet=[1]true"),
                            qsl("teReadOnlyEditable=[4]0:0:1:1"),
                            qsl("teReadOnlyNotReadOnly.sheet=[1]true"),
                            qsl("teReadOnlyNotReadOnly=[4]0:0:1:1"),
                            qsl("teLater.readOnly=[1]true"),
                            qsl("teLater.made=[4]0:0:5:5"),
                            qsl("teLater.restyled=[4]0:0:62:5"),
                            qsl("teLater.editable=[4]0:0:62:5"),
                            qsl("teLater.editableRestyled=[4]0:0:5:5"),
                            qsl("teCleared=[1]true [4]0:0:52:1 [4]0:0:52:500 [4]0:0:500:5"),
                            qsl("teCleared.clear=[1]true"),
                            qsl("teCleared.free=[4]0:0:5:5"),
                            qsl("teShared.sheet=[1]true"),
                            qsl("teShared=[4]0:0:10:10"),
                            qsl("teShared.alone=[4]0:0:52:10"),
                            qsl("teShared.resized=[4]0:0:52:5"),
                            qsl("teBoxed.sheet=[1]true"),
                            qsl("teBoxed=[4]0:0:40:40"),
                            qsl("teMissing.sheet=[2]nil:text edit name 'lsteMissing' not found"),
                            qsl("teMoved.toSame=[1]true"),
                            qsl("teMoved.same=[4]3:3:5:5"),
                            qsl("teMoved.toWindow=[1]true"),
                            qsl("teMoved.window=[4]0:0:62:5"),
                            qsl("teMoved.toMain=[1]true"),
                            qsl("teMoved.back=[4]0:0:5:5"),
                            qsl("teInBox.moveBox=[1]true"),
                            qsl("teInBox.box=[4]0:0:5:5"),
                            qsl("teInBox.moveOut=[1]true"),
                            qsl("teInBox.out=[4]0:0:62:5")};
        // A label's hint, and a length in em or ex, follow fonts nothing here pins, so labels made as the GUI makes them
        // in this process give those
        {
            QWidget parent;
            parent.setFont(host->getDisplayFont());
            parent.show();
            // The GUI's main window is active, which a hint's sheet sees
            parent.activateWindow();
            QVERIFY(QTest::qWaitForWindowActive(&parent));
            const auto hint = [](const QWidget* pWidget) {
                const QSize size = pWidget->sizeHint();
                return qsl("[2]%1:%2").arg(size.width()).arg(size.height());
            };
            const auto geometry = [](const QWidget* pWidget) {
                const QRect rect = pWidget->geometry();
                return qsl("[4]%1:%2:%3:%4").arg(rect.x()).arg(rect.y()).arg(rect.width()).arg(rect.height());
            };
            const auto fill = [&answers](const QString& key, const QString& placeholder, const QString& value) {
                for (QString& answer : answers) {
                    if (answer.startsWith(key + QLatin1Char('='))) {
                        answer.replace(placeholder, value);
                    }
                }
            };
            for (const QString& spec : luaGlobalString(host, "headlessSizeLimitsLabels").split(QLatin1Char('\n'))) {
                // Key, sheet, width, height and text, made as TMainConsole::createLabel(), echo() and setLabelStyleSheet() do
                const QStringList fields = spec.split(QLatin1Char('\t'));
                QCOMPARE(fields.size(), 5);
                const QString& key = fields.at(0);
                auto pLabel = new TLabel(host, qsl("ls%1").arg(key), &parent);
                pLabel->setAutoFillBackground(true);
                pLabel->resize(fields.at(2).toInt(), fields.at(3).toInt());
                pLabel->setContentsMargins(0, 0, 0, 0);
                pLabel->show();
                pLabel->setBackgroundColor(QColor(32, 32, 32, 255));
                if (!fields.at(4).isEmpty()) {
                    pLabel->setText(fields.at(4));
                }
                pLabel->restyle(fields.at(1));
                fill(key, qsl("{hint}"), hint(pLabel));
                fill(key, qsl("{made}"), geometry(pLabel));
                pLabel->resize(5, 500);
                fill(key, qsl("{tall}"), geometry(pLabel));
                pLabel->resize(500, 5);
                fill(key, qsl("{wide}"), geometry(pLabel));
                if (key == qsl("cleared")) {
                    pLabel->restyle(QString());
                    pLabel->resize(5, 500);
                    fill(qsl("cleared.hint"), qsl("{hint}"), hint(pLabel));
                }
            }
        }
        QVERIFY2(!answers.join(QString()).contains(QLatin1Char('{')), "A font-dependent answer was left unfilled.");
        const QStringList actual = luaGlobalString(host, "headlessSizeLimitsAnswers").split(QLatin1Char('\n'));
        QStringList mismatches;
        for (qsizetype i = 0; i < std::max(actual.size(), answers.size()); ++i) {
            if (actual.value(i) != answers.value(i)) {
                mismatches << qsl("%1, not %2").arg(actual.value(i), answers.value(i));
            }
        }
        QVERIFY2(mismatches.isEmpty(), qPrintable(mismatches.join(QChar::LineFeed)));
        QVERIFY2(QApplication::topLevelWidgets().isEmpty(), "Making the labels and text edits created a widget.");
    }

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

    // Text echoed to main takes the profile's colours from birth, as the view applies them when it is
    // built, and then the colours a profile save brings.
    void test_mainConsoleTextTakesTheProfileColours()
    {
        const QString hostname = qsl("Test-Headless-Host-Colours");
        QVERIFY2(HostManager::self()->addHost(hostname, QString(), QString(), QString()), "Could not create a profile with no main window.");
        Host* host = HostManager::self()->getHost(hostname);
        QVERIFY2(host, "The profile is not in the pool.");
        const QString echoAndRead = qsl(R"lua(
echo("main", "\n%1")
moveCursorEnd("main")
selectString("main", "%1", 1)
local fr, fg, fb = getFgColor("main")
local br, bg, bb = getBgColor("main")
deselect("main")
headlessColours = table.concat({fr, fg, fb, br, bg, bb}, ",")
)lua");
        const auto echoedColours = [host, &echoAndRead](const QString& text) {
            host->getLuaInterpreter()->compileAndExecuteScript(echoAndRead.arg(text));
            return luaGlobalString(host, "headlessColours");
        };
        const auto coloursOf = [](const QColor& fg, const QColor& bg) {
            return qsl("%1,%2,%3,%4,%5,%6").arg(fg.red()).arg(fg.green()).arg(fg.blue()).arg(bg.red()).arg(bg.green()).arg(bg.blue());
        };
        QCOMPARE(echoedColours(qsl("fresh text")), coloursOf(QColorConstants::LightGray, QColorConstants::Black));

        const QByteArray save = "<?xml version=\"1.0\"?><MudletPackage version=\"1.001\"><HostPackage><Host>"
                                "<mFgColor>#102030</mFgColor><mBgColor>#405060</mBgColor>"
                                "</Host></HostPackage></MudletPackage>";
        QTemporaryDir saveDir;
        QVERIFY(saveDir.isValid());
        QFile file(saveDir.filePath(qsl("save.xml")));
        QVERIFY(file.open(QIODevice::WriteOnly));
        QCOMPARE(file.write(save), save.size());
        file.close();
        QVERIFY(file.open(QIODevice::ReadOnly | QIODevice::Text));
        XMLimport importer(host);
        const auto [imported, importError] = importer.importPackage(&file);
        QVERIFY2(imported, qPrintable(importError));
        QCOMPARE(echoedColours(qsl("loaded text")), coloursOf(QColor(0x10, 0x20, 0x30), QColor(0x40, 0x50, 0x60)));
    }

    // The GUI resolves "main" to the main console before it looks for a label of that name
    void test_setFontPrefersMainConsoleOverALabelNamedMain()
    {
        const QString hostname = qsl("Test-Headless-Host-Font-Label-Main");
        QVERIFY2(HostManager::self()->addHost(hostname, QString(), QString(), QString()), "Could not create a profile with no main window.");
        Host* host = HostManager::self()->getHost(hostname);
        QVERIFY2(host, "The profile is not in the pool.");
        QVERIFY(!host->hasConsoleView());
        const QFont startFont = host->getDisplayFont();
        const QString startFamily = QFontInfo(startFont).family();

        // setFont() refuses a family this machine does not have, and macOS CI has none of the Linux ones
        const QStringList installed = FontManager::availableFonts();
        QString family;
        for (const QString& candidate : QStringList{qsl("DejaVu Sans"), qsl("Ubuntu Mono"), qsl("Menlo"), qsl("Courier New")} + installed) {
            if (installed.contains(candidate) && !candidate.startsWith(QLatin1Char('.')) && !candidate.contains(QLatin1Char('"')) && !candidate.contains(QLatin1Char('\\'))
                && QFontInfo(host->createFontWithSettings(candidate, startFont.pointSize())).family() != startFamily) {
                family = candidate;
                break;
            }
        }
        QVERIFY2(!family.isEmpty(), "No font family other than the main console one is installed.");

        QVERIFY(host->getLuaInterpreter()->compileAndExecuteScript(qsl("labelMainFamily = \"%1\"").arg(family)));
        const bool ran = host->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(
headlessLabelMain = "not run"
local ok, err = pcall(function()
  assert(createLabel("main", 0, 0, 50, 20, 1) == true, "createLabel did not answer true")
end)
headlessLabelMain = ok and "ok" or tostring(err)
)lua"));
        QVERIFY2(ran, "The Lua chunk did not run.");
        QCOMPARE(luaGlobalString(host, "headlessLabelMain"), qsl("ok"));
        const auto labelBefore = host->labelFont(qsl("main"));
        QVERIFY2(labelBefore, "The label named main was not made.");

        const bool set = host->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(
headlessLabelMain = "not run"
local ok, err = pcall(function()
  assert(setFont("main", labelMainFamily) == true, "setFont did not answer true for main")
  labelMainSetFont = getFont("main")
end)
headlessLabelMain = ok and "ok" or tostring(err)
)lua"));
        QVERIFY2(set, "The Lua chunk setting the font did not run.");
        QCOMPARE(luaGlobalString(host, "headlessLabelMain"), qsl("ok"));
        const auto labelAfter = host->labelFont(qsl("main"));
        QVERIFY(labelAfter);
        QCOMPARE(labelAfter->family(), labelBefore->family());

        const bool deleted = host->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(
headlessLabelMain = "not run"
local ok, err = pcall(function()
  assert(deleteLabel("main") == true, "deleteLabel did not answer true")
  labelMainAfterDelete = getFont("main")
end)
headlessLabelMain = ok and "ok" or tostring(err)
)lua"));
        QVERIFY2(deleted, "The Lua chunk deleting the label did not run.");
        QCOMPARE(luaGlobalString(host, "headlessLabelMain"), qsl("ok"));
        const QString expected = QFontInfo(host->createFontWithSettings(family, startFont.pointSize())).family();
        QCOMPARE(luaGlobalString(host, "labelMainSetFont"), expected);
        QCOMPARE(luaGlobalString(host, "labelMainAfterDelete"), expected);
        QCOMPARE(QFontInfo(host->getDisplayFont()).family(), expected);
        QVERIFY2(QApplication::topLevelWidgets().isEmpty(), "A profile with no main window made a widget.");
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
  -- Stored for a mapper made later, as in the GUI before its mapper exists; a build without the 3D mapper does not know the key
  if setMapPerspective then
    check("setConfig show3dMapView", answered(true), setConfig("show3dMapView", true))
    check("getConfig show3dMapView", answered(true), getConfig("show3dMapView"))
    check("setConfig show3dMapView false", answered(true), setConfig("show3dMapView", false))
    check("getConfig show3dMapView false", answered(false), getConfig("show3dMapView"))
  else
    check("setConfig show3dMapView", refused(notAnOption), setConfig("show3dMapView", false))
  end
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
