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
#include <QDir>
#include <QFontInfo>
#include <QFontMetrics>
#include <QFontMetricsF>
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

#include "Host.h"
#include "HostManager.h"
#include "MudletApp.h"
#include "PortableModeTestHelper.h"
#include "TAppFrontend.h"
#include "TConsoleModel.h"
#include "TLabelModel.h"
#include "TLuaInterpreter.h"

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
        QVERIFY2(!TAppFrontend::instance(), "A main window exists, so this run is not headless.");

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
        QVERIFY2(!TAppFrontend::instance(), "A main window exists, so this run is not headless.");

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

    void test_profileConnectsAndLogsInWithNoMainWindow()
    {
        QVERIFY2(!TAppFrontend::instance(), "A main window exists, so this run is not headless.");

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
        QVERIFY2(!TAppFrontend::instance(), "A main window exists, so this run is not headless.");

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
        QVERIFY2(!TAppFrontend::instance(), "A main window exists, so this run is not headless.");
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
};

#include "HeadlessHostSmokeTest.moc"
MUDLET_GROUPED_TEST_MAIN(HeadlessHostSmokeTest)
