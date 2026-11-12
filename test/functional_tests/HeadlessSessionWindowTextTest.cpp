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
#include <QSignalSpy>
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

    // The notifier's queued call, then the session's queued relay of it
    static void settle()
    {
        QCoreApplication::processEvents();
        QCoreApplication::processEvents();
    }

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

    void test_theWritesOfATurnAreToldOnce()
    {
        const QString name = qsl("Headless Window Text Changed");
        QVERIFY(writeProfile(name));
        THeadlessSession session;
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QVERIFY(session.setMainWindowSize(QSize(800, 600)));
        QVERIFY(session.runLua(qsl(R"lua(
createMiniConsole("mini", 10, 20, 300, 200)
createMiniConsole("another", 10, 220, 300, 200)
openUserWindow("uw")
createLabel("lbl", 5, 6, 70, 80, 1)
createBuffer("buf")
)lua")));
        // A new console is told too
        settle();
        QSignalSpy changes(&session, &THeadlessSession::windowTextChanged);
        const auto told = [&changes, &name]() {
            settle();
            QStringList all;
            for (const QList<QVariant>& change : std::as_const(changes)) {
                all << (change.at(0).toString() == name ? change.at(1).toStringList().join(QLatin1Char(',')) : qsl("for another profile"));
            }
            changes.clear();
            return all.join(QLatin1Char(';'));
        };

        QVERIFY(session.runLua(qsl("echo('mini', 'one\\n') echo('mini', 'two\\n')")));
        // From the event loop
        QVERIFY(changes.isEmpty());
        QCOMPARE(told(), qsl("mini"));
        QCOMPARE(told(), QString());

        // Sorted, each once
        QVERIFY(session.runLua(qsl("echo('uw', 'u\\n') echo('mini', 'three\\n') echo('another', 'a\\n') echo('uw', 'v\\n')")));
        QCOMPARE(told(), qsl("another,mini,uw"));

        // The main console has its own signals, a label's text is scriptWindowsChanged's, and a buffer is not drawn
        QVERIFY(session.runLua(qsl("echo('to main\\n') echo('lbl', 'label text') echo('buf', 'buffered\\n') clearWindow() clearWindow('buf')")));
        QCOMPARE(told(), QString());

        // A hidden window's text still changes
        QVERIFY(session.runLua(qsl("hideWindow('mini')")));
        QCOMPARE(told(), QString());
        QVERIFY(session.runLua(qsl("echo('mini', 'unseen\\n')")));
        QCOMPARE(told(), qsl("mini"));

        // One written to and deleted in the same turn is not named
        QVERIFY(session.runLua(qsl("echo('another', 'b\\n') deleteMiniConsole('another')")));
        QCOMPARE(told(), QString());
        QVERIFY(session.runLua(qsl("createMiniConsole('another', 10, 220, 300, 200) echo('another', 'b\\n') echo('mini', 'c\\n') deleteMiniConsole('another')")));
        QCOMPARE(told(), qsl("mini"));

        // One deleted and made again in the same turn has lost its rows, though it is the same among the windows
        QVERIFY(session.runLua(qsl("hideWindow('mini') deleteMiniConsole('mini') createMiniConsole('mini', 10, 20, 300, 200) hideWindow('mini')")));
        QCOMPARE(told(), qsl("mini"));
        QVERIFY(!joined(*session.windowLines(qsl("mini"), 0, 100)).contains(qsl("one")));
        QVERIFY(session.shutdown(false));
    }

    void test_aWindowMadeAndWrittenInOneTurnIsListedBeforeItsTextIsTold()
    {
        const QString name = qsl("Headless Window Text Order");
        QVERIFY(writeProfile(name));
        THeadlessSession session;
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QVERIFY(session.setMainWindowSize(QSize(800, 600)));
        QVERIFY(session.runLua(qsl("createMiniConsole('mini', 10, 20, 300, 200)")));
        settle();
        QStringList told;
        connect(&session, &THeadlessSession::scriptWindowsChanged, this, [&told, &session](const QString&, const QStringList& names) {
            told << qsl("windows %1").arg(names.join(QLatin1Char(',')));
            // A handler reads what it was told of
            QVERIFY(session.windowLines(qsl("made"), 0, 10));
        });
        connect(&session, &THeadlessSession::windowTextChanged, this, [&told, &session](const QString&, const QStringList& names) {
            told << qsl("text %1 (%2)").arg(names.join(QLatin1Char(',')), session.windowLines(qsl("made"), 0, 1)->lines.at(0).text);
        });
        // The write to the window there already comes first, so its text is noted before the new window is
        QVERIFY(session.runLua(qsl("echo('mini', 'first\\n') createMiniConsole('made', 10, 220, 300, 200) echo('made', 'new\\n')")));
        QVERIFY(told.isEmpty());
        settle();
        QCOMPARE(told.join(QLatin1Char(';')), qsl("windows made;text made,mini (new)"));
        QVERIFY(session.shutdown(false));
    }

    void test_eachWayAScriptChangesAConsolesTextIsTold_data()
    {
        QTest::addColumn<QString>("lua");
        QTest::newRow("echo") << qsl("echo('mini', 'more\\n')");
        QTest::newRow("cecho") << qsl("cecho('mini', '<red>more\\n')");
        QTest::newRow("insertText in a line") << qsl("moveCursor('mini', 2, 0) insertText('mini', 'X')");
        QTest::newRow("insertText at the end") << qsl("moveCursorEnd('mini') insertText('mini', 'X')");
        QTest::newRow("replace") << qsl("moveCursor('mini', 0, 0) selectString('mini', 'two', 1) replace('mini', '2')");
        QTest::newRow("setFgColor") << qsl("moveCursor('mini', 0, 0) selectString('mini', 'two', 1) setFgColor('mini', 255, 0, 0)");
        QTest::newRow("setBgColor") << qsl("moveCursor('mini', 0, 0) selectString('mini', 'two', 1) setBgColor('mini', 255, 0, 0)");
        QTest::newRow("setItalics") << qsl("moveCursor('mini', 0, 0) selectString('mini', 'two', 1) setItalics('mini', true)");
        QTest::newRow("deleteLine") << qsl("moveCursor('mini', 0, 1) deleteLine('mini')");
        QTest::newRow("wrapLine") << qsl("setWindowWrap('mini', 5) wrapLine('mini', 0)");
        QTest::newRow("clearWindow") << qsl("clearWindow('mini')");
        QTest::newRow("echoLink") << qsl("echoLink('mini', 'go', [[send('go')]], 'hint')");
        QTest::newRow("insertLink") << qsl("moveCursor('mini', 2, 0) insertLink('mini', 'go', [[send('go')]], 'hint')");
        QTest::newRow("setLink") << qsl("moveCursor('mini', 0, 0) selectString('mini', 'two', 1) setLink('mini', [[send('go')]], 'hint')");
        QTest::newRow("echoPopup") << qsl("echoPopup('mini', 'pick', {[[send('a')]], [[send('b')]]}, {'a', 'b'})");
        QTest::newRow("appendBuffer") << qsl("moveCursor('mini', 0, 0) selectCurrentLine('mini') copy('mini') appendBuffer('mini')");
        QTest::newRow("paste") << qsl("moveCursor('mini', 0, 0) selectCurrentLine('mini') copy('mini') moveCursor('mini', 0, 1) paste('mini')");
        // 100 rows is the least a console keeps
        QTest::newRow("a trim") << qsl("setConsoleBufferSize('mini', 100, 50) for i = 1, 120 do echo('mini', i .. '\\n') end");
    }

    void test_eachWayAScriptChangesAConsolesTextIsTold()
    {
        QFETCH(QString, lua);
        const QString name = qsl("Headless Window Text Writers");
        QVERIFY(writeProfile(name));
        THeadlessSession session;
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QVERIFY(session.setMainWindowSize(QSize(800, 600)));
        QVERIFY(session.runLua(qsl("createMiniConsole('mini', 10, 20, 300, 200) echo('mini', 'one two three\\nfour five six\\nseven\\n')")));
        settle();
        const std::optional<THeadlessSession::WindowText> before = session.windowLines(qsl("mini"), 0, 100);
        QVERIFY(before);
        QSignalSpy changes(&session, &THeadlessSession::windowTextChanged);
        QVERIFY2(session.runLua(lua), qPrintable(session.errorString()));
        // The row is no audit of the writer unless it changed what an app would draw
        const std::optional<THeadlessSession::WindowText> after = session.windowLines(qsl("mini"), 0, 100);
        QVERIFY(after);
        QVERIFY2(after->lines != before->lines, "the script changed nothing");
        settle();
        QCOMPARE(changes.size(), 1);
        QCOMPARE(changes.at(0).at(1).toStringList(), QStringList{qsl("mini")});
        QVERIFY(session.shutdown(false));
    }
};

#include "HeadlessSessionWindowTextTest.moc"
MUDLET_GROUPED_TEST_MAIN(HeadlessSessionWindowTextTest)
