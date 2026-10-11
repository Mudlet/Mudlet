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
#include <QTemporaryDir>
#include <QtTest/QtTest>

#include <memory>

#include "FontManager.h"
#include "Host.h"
#include "HostManager.h"
#include "MudletApp.h"
#include "PortableModeTestHelper.h"
#include "TAppFrontend.h"
#include "TLuaInterpreter.h"

#include "GroupedTest.h"

// The main console's font as scripts read it with no main window. A class of its own, and so a
// process of its own, because the bundled fonts are added once per process: whichever query
// comes first is the only one that can show it adds them.
class HeadlessMainConsoleFontTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    std::unique_ptr<HostManager> mpHostManager;

    static QString luaGlobalString(Host* host, const char* name)
    {
        lua_State* L = host->getLuaInterpreter()->getLuaGlobalState();
        lua_getglobal(L, name);
        const QString value = lua_isstring(L, -1) ? QString::fromUtf8(lua_tostring(L, -1)) : QString();
        lua_pop(L, 1);
        return value;
    }

    // In a pcall, which keeps a failing assert's message that compileAndExecuteScript() only logs
    static QString runChecks(Host* host, const QString& checks)
    {
        const QString chunk = qsl(R"lua(
fontChecks = 'not run'
local function same(label, got, want)
  assert(got == want, label .. " answered " .. tostring(got) .. " rather than " .. tostring(want))
end
local ok, err = pcall(function()
%1
end)
fontChecks = ok and 'ok' or tostring(err)
)lua")
                                      .arg(checks);
        if (!host->getLuaInterpreter()->compileAndExecuteScript(chunk)) {
            return qsl("the Lua chunk did not run");
        }
        return luaGlobalString(host, "fontChecks");
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

        QVERIFY2(!HostManager::self(), "A profile pool already exists, so this run is not headless.");
        mpHostManager = std::make_unique<HostManager>();
    }

    void cleanupTestCase()
    {
        mpHostManager.reset();
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
    }

    // As the GUI answers for its main console. calcFontSize() with a size and a family measures
    // the same way the main console's own column width is measured, in the GUI as well.
    void test_mainConsoleFontWithNoMainWindow()
    {
#ifndef INCLUDE_FONTS
        QSKIP("Built with WITH_FONTS=NO, so there is no bundled default font to name");
#else
        QVERIFY2(!TAppFrontend::hasView(), "A main window exists, so this run is not headless.");

        const QString hostname = qsl("Test-Headless-Main-Console-Font");
        QVERIFY2(HostManager::self()->addHost(hostname, QString(), QString(), QString()), "Could not create a profile with no main window.");
        Host* host = HostManager::self()->getHost(hostname);
        QVERIFY2(host, "The profile is not in the pool.");
        QVERIFY(!host->hasConsoleView());

        FontManager* fonts = FontManager::self();
        QVERIFY2(fonts, "The profile pool has no font manager.");
        const QString bundledFile = qsl(":/fonts/ttf-bitstream-vera-1.10/VeraMono.ttf");
        QVERIFY2(!fonts->fontAlreadyLoaded(bundledFile, QString()), "The bundled fonts were added before the first query, so this cannot tell whether that query adds them.");

        const QString defaultFontChecks = runChecks(host, qsl(R"lua(
  same("getFont()", getFont(), "Bitstream Vera Sans Mono")
  same("getFont('main')", getFont("main"), "Bitstream Vera Sans Mono")
  same("getFontSize()", getFontSize(), 14)
  same("getFontSize('main')", getFontSize("main"), 14)
  local columnWidth, rowHeight = calcFontSize(14, "Bitstream Vera Sans Mono")
  local width, height = calcFontSize("main", true)
  same("calcFontSize('main', true) width", width, columnWidth)
  same("calcFontSize('main', true) height", height, rowHeight)
  width, height = calcFontSize()
  assert(type(width) == "number" and width > 0, "calcFontSize() gave no width")
  same("calcFontSize() height", height, rowHeight)
  local missing, why = getFont("noSuchHeadlessWindow")
  same("getFont of a missing window", missing, nil)
  same("getFont reason for a missing window", why, 'window "noSuchHeadlessWindow" not found')
)lua"));
        QCOMPARE(defaultFontChecks, qsl("ok"));
        QVERIFY2(fonts->fontAlreadyLoaded(bundledFile, QString()), "getFont() named the bundled default without the bundled fonts added, so it could only be a font this machine has installed.");

        // The calls the GUI's setFont() and setFontSize() make for its main console
        QVERIFY(host->setDisplayFont(host->createFontWithSettings(qsl("Ubuntu Mono"), 14), Host::DisplayFontChange::UserChoice).first);
        host->setDisplayFontSize(11);
        const QString changedFontChecks = runChecks(host, qsl(R"lua(
  same("getFont()", getFont(), "Ubuntu Mono")
  same("getFontSize()", getFontSize(), 11)
  local columnWidth, rowHeight = calcFontSize(11, "Ubuntu Mono")
  local width, height = calcFontSize(true)
  same("calcFontSize(true) width", width, columnWidth)
  same("calcFontSize(true) height", height, rowHeight)
)lua"));
        QCOMPARE(changedFontChecks, qsl("ok"));
#endif
    }
};

#include "HeadlessMainConsoleFontTest.moc"
MUDLET_GROUPED_TEST_MAIN(HeadlessMainConsoleFontTest)
