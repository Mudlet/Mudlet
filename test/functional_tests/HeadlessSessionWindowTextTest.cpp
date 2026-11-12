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

/*
 * The text of the mini consoles and user windows a profile's scripts write to, as
 * an app that embeds Mudlet reads it from THeadlessSession to draw them. Each test
 * makes a session of its own, and with it the process's one profile pool.
 *
 * Run with: ctest -R HeadlessSessionWindowTextTest -V
 */

#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QtTest/QtTest>

#include <optional>

#include "Host.h"
#include "HostManager.h"
#include "MudletApp.h"
#include "PortableModeTestHelper.h"
#include "THeadlessSession.h"

#include "GroupedTest.h"

class HeadlessSessionWindowTextTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;

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
        saveFile.write(QByteArrayLiteral(R"(<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE MudletPackage>
<MudletPackage version="1.001">
<HostPackage><Host></Host></HostPackage>
</MudletPackage>
)"));
        return true;
    }

    static QString luaString(THeadlessSession& session, const QString& expression)
    {
        if (!session.runLua(qsl("headlessWindowTextAnswer = tostring(%1)").arg(expression))) {
            return {};
        }
        lua_State* L = session.host()->getLuaInterpreter()->getLuaGlobalState();
        lua_getglobal(L, "headlessWindowTextAnswer");
        const QString answer = QString::fromUtf8(lua_tostring(L, -1));
        lua_pop(L, 1);
        return answer;
    }

    // What a script reads of the same rows, joined by line feeds
    static QString luaLines(THeadlessSession& session, const QString& window) { return luaString(session, qsl("table.concat(getLines(\"%1\", 0, getLineCount(\"%1\") + 1), \"\\n\")").arg(window)); }

    static QString joined(const THeadlessSession::WindowText& text)
    {
        QStringList lines;
        for (const THeadlessSession::WindowLine& line : text.lines) {
            lines << line.text;
        }
        return lines.join(QChar::LineFeed);
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

    void test_aMiniConsolesRowsReadBackWithTheirColours()
    {
        const QString name = qsl("Headless Window Text Rows");
        QVERIFY(writeProfile(name));
        THeadlessSession session;
        QVERIFY(!session.windowLines(qsl("mini"), 0, 10));
        QVERIFY(!session.windowLines(qsl("main"), 0, 10));
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QVERIFY(session.setMainWindowSize(QSize(800, 600)));
        QVERIFY(!session.windowLines(qsl("mini"), 0, 10));
        QVERIFY(session.runLua(qsl(R"lua(
createMiniConsole("mini", 10, 20, 300, 200)
setBackgroundColor("mini", 10, 20, 30, 255)
cecho("mini", "<red>ab<green:blue>cde\n")
echo("mini", "plain\n")
echo("mini", "last")
)lua")));

        const std::optional<THeadlessSession::WindowText> text = session.windowLines(qsl("mini"), 0, 100);
        QVERIFY(text);
        QCOMPARE(text->first, 0);
        QCOMPARE(text->lineCount, 3);
        // A script counts up to the row a write carries on
        QCOMPARE(text->lineCount, luaString(session, qsl("getLineCount(\"mini\")")).toInt() + 1);
        QCOMPARE(joined(*text), qsl("abcde\nplain\nlast"));
        QCOMPARE(joined(*text), luaLines(session, qsl("mini")));

        const THeadlessSession::WindowLine& coloured = text->lines.at(0);
        QVERIFY(!coloured.isPrompt);
        QVERIFY(!coloured.wrapped);
        QVERIFY(coloured.arrivalMSecs > 0);
        QCOMPARE(coloured.runs.size(), 2);
        QCOMPARE(coloured.runs.at(0).start, 0);
        QCOMPARE(coloured.runs.at(0).length, 2);
        QCOMPARE(coloured.runs.at(0).foreground, QColor(255, 0, 0));
        QVERIFY(!coloured.runs.at(0).defaultForeground);
        QVERIFY(coloured.runs.at(0).defaultBackground);
        QCOMPARE(coloured.runs.at(1).start, 2);
        QCOMPARE(coloured.runs.at(1).length, 3);
        QCOMPARE(coloured.runs.at(1).foreground, QColor(0, 255, 0));
        QCOMPARE(coloured.runs.at(1).background, QColor(0, 0, 255));
        QVERIFY(!coloured.runs.at(1).defaultBackground);

        // The console's own colours are the ones its ScriptWindow is drawn in, not the main console's
        const THeadlessSession::WindowLine& plain = text->lines.at(1);
        QCOMPARE(plain.runs.size(), 1);
        QCOMPARE(plain.runs.at(0).length, 5);
        QVERIFY(plain.runs.at(0).defaultForeground);
        QVERIFY(plain.runs.at(0).defaultBackground);
        QCOMPARE(plain.runs.at(0).background, session.scriptWindow(qsl("mini"))->background);
        QCOMPARE(plain.runs.at(0).background, QColor(10, 20, 30));
        QVERIFY(session.consoleBackground() != QColor(10, 20, 30));

        // A part of them, and from the end
        const std::optional<THeadlessSession::WindowText> middle = session.windowLines(qsl("mini"), 1, 1);
        QVERIFY(middle);
        QCOMPARE(middle->first, 1);
        QCOMPARE(middle->lineCount, 3);
        QCOMPARE(joined(*middle), qsl("plain"));
        const std::optional<THeadlessSession::WindowText> tail = session.windowLines(qsl("mini"), -2, 2);
        QVERIFY(tail);
        QCOMPARE(tail->first, 1);
        QCOMPARE(joined(*tail), qsl("plain\nlast"));
        QVERIFY(tail->lines.at(0) == plain);
        // cut to the rows there are
        const std::optional<THeadlessSession::WindowText> over = session.windowLines(qsl("mini"), -10, 2);
        QVERIFY(over);
        QCOMPARE(over->first, 0);
        QCOMPARE(joined(*over), qsl("abcde\nplain"));
        const std::optional<THeadlessSession::WindowText> past = session.windowLines(qsl("mini"), 7, 2);
        QVERIFY(past);
        QCOMPARE(past->first, 3);
        QVERIFY(past->lines.isEmpty());
        QVERIFY(session.windowLines(qsl("mini"), 1, -4)->lines.isEmpty());

        // A hidden console keeps its text
        QVERIFY(session.runLua(qsl("hideWindow('mini')")));
        QVERIFY(!session.scriptWindow(qsl("mini"))->shown);
        QCOMPARE(joined(*session.windowLines(qsl("mini"), 0, 100)), qsl("abcde\nplain\nlast"));

        // What clearWindow() leaves is what a script counts
        QVERIFY(session.runLua(qsl("clearWindow('mini')")));
        const std::optional<THeadlessSession::WindowText> cleared = session.windowLines(qsl("mini"), 0, 100);
        QVERIFY(cleared);
        QCOMPARE(cleared->lineCount, 1);
        QCOMPARE(cleared->lines.size(), 1);
        QCOMPARE(joined(*cleared), luaLines(session, qsl("mini")));
        QCOMPARE(cleared->lines.at(0).text, QString());
        QVERIFY(cleared->lines.at(0).runs.isEmpty());
        QVERIFY(session.shutdown(false));
    }

    void test_aRowTheWrapSplitOffIsMarked()
    {
        const QString name = qsl("Headless Window Text Wrap");
        QVERIFY(writeProfile(name));
        THeadlessSession session;
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QVERIFY(session.setMainWindowSize(QSize(800, 600)));
        QVERIFY(session.runLua(qsl(R"lua(
createMiniConsole("mini", 10, 20, 300, 200)
setWindowWrap("mini", 10)
echo("mini", "one two three four\n")
echo("mini", "short\n")
)lua")));
        const std::optional<THeadlessSession::WindowText> text = session.windowLines(qsl("mini"), 0, 100);
        QVERIFY(text);
        QCOMPARE(joined(*text), luaLines(session, qsl("mini")));
        QVERIFY2(text->lines.size() >= 3, qPrintable(joined(*text)));
        QCOMPARE(text->lines.at(0).text, qsl("one two "));
        QVERIFY(!text->lines.at(0).wrapped);
        QVERIFY(text->lines.at(1).wrapped);
        QCOMPARE(text->lines.at(1).arrivalMSecs, text->lines.at(0).arrivalMSecs);
        QVERIFY(text->lines.at(0).arrivalMSecs > 0);
        const qsizetype shortRow = joined(*text).split(QChar::LineFeed).indexOf(qsl("short"));
        QVERIFY(shortRow > 1);
        QVERIFY(!text->lines.at(shortRow).wrapped);
        for (qsizetype row = 1; row < shortRow; ++row) {
            QVERIFY2(text->lines.at(row).wrapped, qPrintable(text->lines.at(row).text));
        }
        QVERIFY(session.shutdown(false));
    }

    void test_onlyConsolesAnAppDrawsAreRead()
    {
        const QString name = qsl("Headless Window Text Names");
        QVERIFY(writeProfile(name));
        THeadlessSession session;
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QVERIFY(session.setMainWindowSize(QSize(800, 600)));
        QVERIFY(session.runLua(qsl(R"lua(
createMiniConsole("mini", 10, 20, 300, 200)
enableCommandLine("mini")
echo("mini", "in mini\n")
openUserWindow("uw")
echo("uw", "in the user window\n")
createLabel("lbl", 5, 6, 70, 80, 1)
echo("lbl", "label text")
createScrollBox("main", "sb", 400, 10, 200, 100)
createTextEdit("sb", "tb", 1, 2, 50, 40)
createBuffer("buf")
echo("buf", "in the buffer\n")
openMapWidget()
createMiniConsole("gone", 0, 0, 50, 50)
echo("gone", "text\n")
echo("to the main console\n")
)lua")));
        QVERIFY(session.scriptWindow(qsl("mapper")));
        // With its own command line, which shares its name, it is the console that is read
        QVERIFY(session.scriptWindow(qsl("mini"), THeadlessSession::ScriptWindow::Kind::commandLine));
        QCOMPARE(joined(*session.windowLines(qsl("mini"), 0, 1)), qsl("in mini"));
        const std::optional<THeadlessSession::WindowText> userWindow = session.windowLines(qsl("uw"), 0, 100);
        QVERIFY(userWindow);
        QCOMPARE(joined(*userWindow), luaLines(session, qsl("uw")));
        QCOMPARE(userWindow->lines.at(0).text, qsl("in the user window"));

        for (const QString& refused : {qsl("lbl"), qsl("sb"), qsl("tb"), qsl("buf"), qsl("mapper"), qsl("nothing")}) {
            QVERIFY2(!session.windowLines(refused, 0, 100), qPrintable(refused));
        }
        // A script still reads the buffer
        QCOMPARE(luaString(session, qsl("getLines(\"buf\", 0, 1)[1]")), qsl("in the buffer"));

        QVERIFY(session.windowLines(qsl("gone"), 0, 100));
        QVERIFY(session.runLua(qsl("deleteMiniConsole('gone')")));
        QVERIFY(!session.windowLines(qsl("gone"), 0, 100));

        // The main console under both of its names, with the profile's own colours
        for (const QString& mainName : {qsl("main"), QString()}) {
            const std::optional<THeadlessSession::WindowText> main = session.windowLines(mainName, 0, 1000);
            QVERIFY(main);
            QCOMPARE(main->lineCount, luaString(session, qsl("getLineCount()")).toInt() + 1);
            QCOMPARE(joined(*main), luaString(session, qsl("table.concat(getLines(0, getLineCount() + 1), \"\\n\")")));
            const qsizetype row = joined(*main).split(QChar::LineFeed).indexOf(qsl("to the main console"));
            QVERIFY(row >= 0);
            QCOMPARE(main->lines.at(row).runs.size(), 1);
            QVERIFY(main->lines.at(row).runs.at(0).defaultForeground);
            QCOMPARE(main->lines.at(row).runs.at(0).foreground, session.consoleForeground());
        }
        QVERIFY(session.shutdown(false));
    }
};

#include "HeadlessSessionWindowTextTest.moc"
MUDLET_GROUPED_TEST_MAIN(HeadlessSessionWindowTextTest)
