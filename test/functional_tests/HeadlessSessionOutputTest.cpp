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
 * What THeadlessSession tells an app that draws the main console as the GUI
 * draws it. Each test makes a session of its own, and with it the process's one
 * profile pool.
 *
 * Run with: ctest -R HeadlessSessionOutputTest -V
 */

#include <QDir>
#include <QFile>
#include <QSignalSpy>
#include <QTcpServer>
#include <QTemporaryDir>
#include <QtTest/QtTest>

#include <optional>
#include <utility>

#include "Host.h"
#include "HostManager.h"
#include "MudletApp.h"
#include "PortableModeTestHelper.h"
#include "RefusedPort.h"
#include "TConsoleModel.h"
#include "THeadlessSession.h"
#include "TelnetServerStub.h"

#include "GroupedTest.h"

class HeadlessSessionOutputTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;

    static bool writeProfile(const QString& name, const QString& script, const QString& hostXml = QString())
    {
        const QString saveFolder = MudletApp::getMudletPath(enums::profileXmlFilesPath, name);
        if (!QDir().mkpath(saveFolder)) {
            return false;
        }
        QFile saveFile(qsl("%1/2020-01-01#00-00-00.xml").arg(saveFolder));
        if (!saveFile.open(QIODevice::WriteOnly)) {
            return false;
        }
        saveFile.write(qsl(R"(<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE MudletPackage>
<MudletPackage version="1.001">
<HostPackage><Host>%2</Host></HostPackage>
<ScriptPackage><Script isActive="yes" isFolder="no"><name>headlessSessionOutput</name><packageName></packageName><script>%1</script><eventHandlerList/></Script></ScriptPackage>
</MudletPackage>
)")
                               .arg(script.toHtmlEscaped(), hostXml)
                               .toUtf8());
        return true;
    }

    static QList<QVariantList> wrapsSent(const QSignalSpy& spy)
    {
        QList<QVariantList> wraps;
        for (const auto& arguments : spy) {
            wraps << arguments.mid(1);
        }
        return wraps;
    }

    static QVariantList wrap(const int wrapAt, const int indent, const int hangingIndent) { return {wrapAt, indent, hangingIndent}; }

    static void followTheConnection(THeadlessSession& session, QStringList& events)
    {
        connect(&session, &THeadlessSession::connecting, &session, [&events]() {
            events << qsl("connecting");
        });
        connect(&session, &THeadlessSession::connected, &session, [&events]() {
            events << qsl("connected");
        });
        connect(&session, &THeadlessSession::disconnected, &session, [&events]() {
            events << qsl("disconnected");
        });
    }

    // Each run as "text|foreground|background|styles", a colour of the profile's own as "own", a link as "@"
    static QString styledPiece(const QString& text, const QList<THeadlessSession::TextRun>& runs)
    {
        QStringList parts;
        int covered = 0;
        for (const THeadlessSession::TextRun& run : runs) {
            if (run.start != covered || run.length <= 0) {
                return qsl("runs out of step at %1 in \"%2\"").arg(run.start).arg(text);
            }
            covered += run.length;
            parts << qsl("%1|%2|%3|%4%5")
                             .arg(text.mid(run.start, run.length).replace(QChar::LineFeed, qsl("\\n")),
                                  run.defaultForeground ? qsl("own") : run.foreground.name(),
                                  run.defaultBackground ? qsl("own") : run.background.name(),
                                  QString::number(run.styles.toInt()),
                                  run.link ? qsl("@") : QString());
        }
        if (covered != text.size()) {
            return qsl("runs cover %1 of \"%2\"").arg(covered).arg(text);
        }
        return parts.join(qsl(" + "));
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

    void test_theWrapAProfileLoadsWithIsKnownAtProfileLoaded()
    {
        const QString name = qsl("Headless Saved Wrap");
        QVERIFY(writeProfile(name, QString(), qsl("<wrapAt>80</wrapAt><wrapIndentCount>3</wrapIndentCount><wrapHangingIndentCount>5</wrapHangingIndentCount>")));
        THeadlessSession session;
        QCOMPARE(session.consoleWrap(), THeadlessSession::ConsoleWrap{});
        std::optional<THeadlessSession::ConsoleWrap> loadedWith;
        connect(&session, &THeadlessSession::profileLoaded, this, [&]() {
            loadedWith = session.consoleWrap();
        });
        QSignalSpy wraps(&session, &THeadlessSession::consoleWrapChanged);
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QVERIFY(loadedWith);
        QCOMPARE(loadedWith->wrapAt, 80);
        QCOMPARE(loadedWith->indent, 3);
        QCOMPARE(loadedWith->hangingIndent, 5);
        QTest::qWait(100);
        QCOMPARE(wraps.count(), 0);
        QVERIFY(session.shutdown(false));
        QCOMPARE(session.consoleWrap(), THeadlessSession::ConsoleWrap{});
    }

    void test_eachWrapSetterReachesTheSession()
    {
        const QString name = qsl("Headless Wrap");
        QVERIFY(writeProfile(name, QString()));
        THeadlessSession session;
        QSignalSpy wraps(&session, &THeadlessSession::consoleWrapChanged);
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);

        QVERIFY(session.runLua(qsl(R"lua(setWindowWrap("main", 60))lua")));
        QTRY_COMPARE_WITH_TIMEOUT(wraps.count(), 1, 10000);
        QCOMPARE(wraps.constFirst().at(0).toString(), name);
        QVERIFY(session.runLua(qsl(R"lua(setWindowWrapIndent("main", 2))lua")));
        QTRY_COMPARE_WITH_TIMEOUT(wraps.count(), 2, 10000);
        QVERIFY(session.runLua(qsl(R"lua(setWindowWrapHangingIndent("main", 4))lua")));
        QTRY_COMPARE_WITH_TIMEOUT(wraps.count(), 3, 10000);
        QCOMPARE(wrapsSent(wraps), (QList<QVariantList>{wrap(60, 0, 0), wrap(60, 2, 0), wrap(60, 2, 4)}));
        QCOMPARE(session.consoleWrap(), (THeadlessSession::ConsoleWrap{60, 2, 4}));

        // The same again, a mini console's, and a change undone before the event loop runs
        QVERIFY(session.runLua(qsl(R"lua(
setWindowWrap("main", 60) setWindowWrapIndent("main", 2) setWindowWrapHangingIndent("main", 4)
createMiniConsole("headlessWrapMini", 0, 0, 200, 100)
setWindowWrap("headlessWrapMini", 30) setWindowWrapIndent("headlessWrapMini", 1) setWindowWrapHangingIndent("headlessWrapMini", 1)
setWindowWrap("main", 61) setWindowWrap("main", 60)
)lua")));
        QTest::qWait(100);
        QCOMPARE(wraps.count(), 3);
        QVERIFY(session.shutdown(false));
    }

    void test_aWrapChangeOwedToAProfileLetGoIsNotSent()
    {
        const QString name = qsl("Headless Wrap Let Go");
        QVERIFY(writeProfile(name, QString()));
        THeadlessSession session;
        QSignalSpy wraps(&session, &THeadlessSession::consoleWrapChanged);
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QVERIFY(session.runLua(qsl(R"lua(setWindowWrap("main", 70))lua")));
        QVERIFY(session.shutdown(false));
        QTest::qWait(100);
        QCOMPARE(wraps.count(), 0);
    }

    void test_theColoursAProfileLoadsWithAreKnownAtProfileLoaded()
    {
        const QString name = qsl("Headless Saved Colours");
        QVERIFY(writeProfile(name, QString(), qsl(R"(<mFgColor>#a0b0c0</mFgColor><mBgColor alpha="255">#102030</mBgColor>)")));
        THeadlessSession session;
        QCOMPARE(session.consoleForeground(), QColor());
        QCOMPARE(session.consoleBackground(), QColor());
        QColor loadedForeground;
        QColor loadedBackground;
        connect(&session, &THeadlessSession::profileLoaded, this, [&]() {
            loadedForeground = session.consoleForeground();
            loadedBackground = session.consoleBackground();
        });
        QSignalSpy colors(&session, &THeadlessSession::consoleColorsChanged);
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QCOMPARE(loadedForeground, QColor(0xa0, 0xb0, 0xc0));
        QCOMPARE(loadedBackground, QColor(0x10, 0x20, 0x30));
        QTest::qWait(100);
        QCOMPARE(colors.count(), 0);
        QVERIFY(session.shutdown(false));
        QCOMPARE(session.consoleForeground(), QColor());
        QCOMPARE(session.consoleBackground(), QColor());
    }

    void test_aScriptsBackgroundColourReachesTheSession()
    {
        const QString name = qsl("Headless Colours");
        QVERIFY(writeProfile(name, QString()));
        THeadlessSession session;
        QSignalSpy colors(&session, &THeadlessSession::consoleColorsChanged);
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        const QColor foreground = session.consoleForeground();
        QVERIFY(foreground.isValid());

        QVERIFY(session.runLua(qsl(R"lua(setBackgroundColor("main", 10, 20, 30))lua")));
        QTRY_COMPARE_WITH_TIMEOUT(colors.count(), 1, 10000);
        QCOMPARE(colors.constFirst().at(0).toString(), name);
        QCOMPARE(colors.constFirst().at(1).value<QColor>(), foreground);
        QCOMPARE(colors.constFirst().at(2).value<QColor>(), QColor(10, 20, 30));
        QCOMPARE(session.consoleBackground(), QColor(10, 20, 30));

        // The same again, and a mini console's
        QVERIFY(session.runLua(qsl(R"lua(
setBackgroundColor("main", 10, 20, 30)
createMiniConsole("headlessColoursMini", 0, 0, 200, 100)
setBackgroundColor("headlessColoursMini", 200, 100, 50)
)lua")));
        QTest::qWait(100);
        QCOMPARE(colors.count(), 1);

        // The game redefining its palette refreshes the profile's colours, which stay as they were
        Host* host = session.host();
        QSignalSpy refreshed(&host->mainConsoleModel().mNotifier, &TConsoleModelNotifier::colorsChanged);
        host->setMayRedefineColors(true);
        std::string redefine = "\x1b]P1123456\x07";
        host->mainConsoleModel().buffer.translateToPlainText(redefine, true);
        QCOMPARE(host->mRed, QColor(0x12, 0x34, 0x56));
        QCOMPARE(refreshed.count(), 1);
        QTest::qWait(100);
        QCOMPARE(colors.count(), 1);
        QVERIFY(session.shutdown(false));
    }

    void test_theErrorConsolesColoursReachTheSession()
    {
        const QString name = qsl("Headless Error Colours");
        QVERIFY(writeProfile(name, QString()));
        THeadlessSession session;
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QSignalSpy errors(&session, &THeadlessSession::errorConsoleText);
        QVERIFY(session.runLua(qsl(R"lua(tempRegexTrigger("^headless boom$", [[error("headless boom")]]) feedTriggers("headless boom\n"))lua")));
        QCOMPARE(errors.count(), 3);
        QCOMPARE(errors.at(0).at(0).toString(), name);
        QCOMPARE(errors.at(0).at(1).toString(), qsl("[ERROR:]"));
        QVERIFY2(errors.at(2).at(1).toString().contains(qsl("headless boom")), qPrintable(errors.at(2).at(1).toString()));
        const QList<QColor> foregrounds{QColor(Qt::blue), QColor(Qt::green), QColor(Qt::red)};
        for (int piece = 0; piece < 3; ++piece) {
            QCOMPARE(errors.at(piece).at(2).value<QColor>(), foregrounds.at(piece));
            QCOMPARE(errors.at(piece).at(3).value<QColor>(), QColor(Qt::black));
        }
        QVERIFY(session.shutdown(false));
    }

    void test_theLagTheLatencyBoxShowsReachesTheSession()
    {
        const QString name = qsl("Headless Latency");
        QVERIFY(writeProfile(name, QString()));
        TelnetServerStub server;
        server.setWelcomeMessage(qsl("Welcome to the latency test."));
        server.start(qsl("127.0.0.1"));
        QVERIFY(server.isListening());
        QVERIFY(MudletApp::writeProfileData(name, qsl("url"), qsl("127.0.0.1")).first);
        QVERIFY(MudletApp::writeProfileData(name, qsl("port"), QString::number(server.serverPort())).first);

        THeadlessSession session;
        QList<std::pair<std::optional<double>, double>> lags;
        connect(&session, &THeadlessSession::latencyChanged, this, [&](const QString& profile, const std::optional<double> network, const double processing) {
            QCOMPARE(profile, name);
            lags.append({network, processing});
        });
        QCOMPARE(session.start({.profile = name, .connect = true}), THeadlessSession::StartResult::started);
        QTRY_VERIFY_WITH_TIMEOUT(!lags.isEmpty(), 10000);
        for (const auto& [network, processing] : lags) {
            QVERIFY(!network);
            QVERIFY(processing >= 0);
        }

        server.sendRaw("hp 10> \xff\xf9");
        QTRY_VERIFY_WITH_TIMEOUT(lags.constLast().first.has_value(), 10000);
        QVERIFY(lags.constLast().second >= 0);

        QVERIFY(session.submit(qsl("look")));
        QTRY_VERIFY_WITH_TIMEOUT(server.receivedSoFar().contains("look"), 10000);
        QTest::qWait(300);
        server.sendRaw("You see a room.\nhp 10> \xff\xf9");
        QTRY_VERIFY_WITH_TIMEOUT(lags.constLast().first.value_or(0) >= 0.25, 10000);
        QVERIFY2(*lags.constLast().first < 5, qPrintable(QString::number(*lags.constLast().first)));
        QVERIFY(session.shutdown(false));
    }

    void test_noLagIsSentWithNoTextOrForAProfileLetGo()
    {
        const QString name = qsl("Headless Latency Let Go");
        QVERIFY(writeProfile(name, QString()));
        THeadlessSession session;
        QSignalSpy lags(&session, &THeadlessSession::latencyChanged);
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QTest::qWait(100);
        QCOMPARE(lags.count(), 0);
        QVERIFY(session.runLua(qsl(R"lua(feedTriggers("headless lag\n"))lua")));
        QTRY_COMPARE_WITH_TIMEOUT(lags.count(), 1, 10000);
        QVERIFY(session.shutdown(false));

        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QVERIFY(session.runLua(qsl(R"lua(feedTriggers("headless lag\n"))lua")));
        QVERIFY(session.shutdown(false));
        QTest::qWait(100);
        QCOMPARE(lags.count(), 1);
    }

    void test_connectingComesBeforeTheConnectionIsMade()
    {
        const QString name = qsl("Headless Connecting");
        QVERIFY(writeProfile(name, QString()));
        TelnetServerStub server;
        server.start(qsl("127.0.0.1"));
        QVERIFY(server.isListening());
        QVERIFY(MudletApp::writeProfileData(name, qsl("url"), qsl("127.0.0.1")).first);
        QVERIFY(MudletApp::writeProfileData(name, qsl("port"), QString::number(server.serverPort())).first);

        THeadlessSession session;
        QSignalSpy connecting(&session, &THeadlessSession::connecting);
        QStringList events;
        followTheConnection(session, events);
        QCOMPARE(session.start({.profile = name, .connect = true}), THeadlessSession::StartResult::started);
        QTRY_VERIFY_WITH_TIMEOUT(events.contains(qsl("connected")), 10000);
        QCOMPARE(events, (QStringList{qsl("connecting"), qsl("connected")}));
        QCOMPARE(connecting.constFirst().at(0).toString(), name);

        events.clear();
        QVERIFY(session.runLua(qsl("reconnect()")));
        QTRY_VERIFY_WITH_TIMEOUT(events.contains(qsl("connected")), 10000);
        QCOMPARE(events.mid(events.indexOf(qsl("connecting"))), (QStringList{qsl("connecting"), qsl("connected")}));
        QVERIFY(session.shutdown(false));
    }

    void test_connectingComesBeforeAConnectionThatCannotBeMade()
    {
        const QString name = qsl("Headless Connecting Refused");
        QVERIFY(writeProfile(name, QString()));
        RefusedPort refused;
        QVERIFY(refused.reserve());
        const quint16 port = refused.port();
        QVERIFY(MudletApp::writeProfileData(name, qsl("url"), qsl("127.0.0.1")).first);
        QVERIFY(MudletApp::writeProfileData(name, qsl("port"), QString::number(port)).first);

        THeadlessSession session;
        QStringList events;
        followTheConnection(session, events);
        QCOMPARE(session.start({.profile = name, .connect = true}), THeadlessSession::StartResult::started);
        QTRY_VERIFY_WITH_TIMEOUT(events.contains(qsl("disconnected")), 10000);
        QCOMPARE(events, (QStringList{qsl("connecting"), qsl("disconnected")}));
        QVERIFY(session.shutdown(false));
    }

    void test_aConnectCalledOffForASaveThatDidNotLoadEndsInDisconnected()
    {
        const QString name = qsl("Headless Connecting Corrupt");
        const QString saveFolder = MudletApp::getMudletPath(enums::profileXmlFilesPath, name);
        QVERIFY(QDir().mkpath(saveFolder));
        QFile saveFile(qsl("%1/2020-01-01#00-00-00.xml").arg(saveFolder));
        QVERIFY(saveFile.open(QIODevice::WriteOnly));
        saveFile.write("<?xml version=\"1.0\"?><MudletPackage version=\"1.001\"><HostPackage><Host");
        saveFile.close();
        TelnetServerStub server;
        server.start(qsl("127.0.0.1"));
        QVERIFY(server.isListening());
        QVERIFY(MudletApp::writeProfileData(name, qsl("url"), qsl("127.0.0.1")).first);
        QVERIFY(MudletApp::writeProfileData(name, qsl("port"), QString::number(server.serverPort())).first);

        THeadlessSession session;
        QStringList events;
        followTheConnection(session, events);
        QCOMPARE(session.start({.profile = name, .connect = true}), THeadlessSession::StartResult::loadFailed);
        QTRY_VERIFY_WITH_TIMEOUT(events.contains(qsl("disconnected")), 10000);
        QTest::qWait(500);
        QCOMPARE(events, (QStringList{qsl("connecting"), qsl("disconnected")}));
        QVERIFY(session.shutdown(false));
    }

    void test_aConnectCalledOffDuringTheAddressLookupEndsInDisconnected()
    {
        const QString name = qsl("Headless Connecting Called Off");
        QVERIFY(writeProfile(name, QString()));
        THeadlessSession session;
        QStringList events;
        followTheConnection(session, events);
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QVERIFY(session.runLua(qsl(R"lua(connectToServer("nonexistent.invalid", 23) disconnect())lua")));
        QTRY_VERIFY_WITH_TIMEOUT(events.contains(qsl("disconnected")), 10000);
        QTest::qWait(500);
        QCOMPARE(events, (QStringList{qsl("connecting"), qsl("disconnected")}));
        QVERIFY(session.shutdown(false));
    }

    void test_noConnectingIsSentForAProfileLetGo()
    {
        const QString name = qsl("Headless Connecting Let Go");
        QVERIFY(writeProfile(name, QString()));
        QTcpServer server;
        QVERIFY(server.listen(QHostAddress::LocalHost));
        QVERIFY(MudletApp::writeProfileData(name, qsl("url"), qsl("127.0.0.1")).first);
        QVERIFY(MudletApp::writeProfileData(name, qsl("port"), QString::number(server.serverPort())).first);

        THeadlessSession session;
        QSignalSpy connecting(&session, &THeadlessSession::connecting);
        QCOMPARE(session.start({.profile = name, .connect = true}), THeadlessSession::StartResult::started);
        QVERIFY(session.shutdown(false));
        QTest::qWait(100);
        QCOMPARE(connecting.count(), 0);
    }

    void test_whatScriptsEchoComesWithItsColours()
    {
        const QString name = qsl("Headless Echo Colours");
        QVERIFY(writeProfile(name, QString()));
        THeadlessSession session;
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QStringList plain;
        QStringList styled;
        int linkId = 0;
        connect(&session, &THeadlessSession::localText, &session, [&plain](const QString&, const QString& text) {
            plain << text;
        });
        connect(&session, &THeadlessSession::localTextStyled, &session, [&](const QString& profile, const QString& text, const QList<THeadlessSession::TextRun>& runs) {
            QCOMPARE(profile, name);
            // After the plain signal for the same piece
            QCOMPARE(plain.count(), styled.count() + 1);
            QCOMPARE(plain.constLast(), text);
            styled << styledPiece(text, runs);
            for (const THeadlessSession::TextRun& run : runs) {
                if (run.link) {
                    linkId = run.link;
                }
            }
        });
        const auto pieces = [&]() {
            const QStringList taken = styled;
            styled.clear();
            plain.clear();
            return taken.join(qsl(" ; "));
        };
        const QString underline = QString::number(THeadlessSession::TextStyles(THeadlessSession::TextStyle::Underline).toInt());

        QVERIFY(session.runLua(qsl(R"lua(cecho("<red>hot<reset> plain\n"))lua")));
        QCOMPARE(pieces(), qsl("hot|#ff0000|own|0 ;  plain\\n|own|own|0"));
        QVERIFY(session.runLua(qsl(R"lua(decho("<10,20,30:40,50,60>dd<r>\n"))lua")));
        QCOMPARE(pieces(), qsl("dd|#0a141e|#28323c|0 ; \\n|own|own|0"));
        QVERIFY(session.runLua(qsl(R"lua(hecho("#ff00ffpink#r\n"))lua")));
        QCOMPARE(pieces(), qsl("pink|#ff00ff|own|0 ; \\n|own|own|0"));
        QVERIFY(session.runLua(qsl(R"lua(setFgColor(0, 255, 0) setBgColor(0, 0, 255) setUnderline(true) echo("go\non") resetFormat() echo("\n"))lua")));
        QCOMPARE(pieces(), qsl("go\\non|#00ff00|#0000ff|%1 ; \\n|own|own|0").arg(underline));

        // A link in an echo is named, in the colours it was written in
        QVERIFY(session.runLua(qsl(R"lua(echo("see ") echoLink("here", function() end, "the hint", true) echo("\n"))lua")));
        QCOMPARE(pieces(), qsl("see |own|own|0 ; here|own|own|0@ ; \\n|own|own|0"));
        QVERIFY(linkId);
        const std::optional<THeadlessSession::Link> link = session.link(linkId);
        QVERIFY(link);
        QCOMPARE(link->tooltip, qsl("the hint"));

        // A piece that ends a line another left open starts with that line feed, in the colours of what follows
        QVERIFY(session.runLua(qsl(R"lua(echo("open") moveCursor("main", 0, getLineNumber("main")) setFgColor(255, 0, 0) insertText("in") resetFormat() moveCursorEnd("main") echo("\n"))lua")));
        QCOMPARE(pieces(), qsl("open|own|own|0 ; \\nin|#ff0000|own|0 ; \\n\\n|own|own|0"));

        // Text copied in from another buffer keeps its colours, but not the ids of that buffer's links
        QVERIFY(session.runLua(qsl(R"lua(createBuffer("copied")
cecho("copied", "<red>x")
echoLink("copied", "lnk", function() end, "a hint", true)
echo("copied", "\n")
moveCursor("copied", 0, 0)
selectCurrentLine("copied")
copy("copied")
appendBuffer("main"))lua")));
        QCOMPARE(pieces(), qsl("x|#ff0000|own|0 + lnk\\n|own|own|0"));
        QVERIFY(session.shutdown(false));
    }

    void test_aCommandsEchoComesWithItsColours()
    {
        const QString name = qsl("Headless Command Echo Colours");
        QVERIFY(writeProfile(name, QString()));
        QTcpServer server;
        QVERIFY(server.listen(QHostAddress::LocalHost));
        QVERIFY(MudletApp::writeProfileData(name, qsl("url"), qsl("127.0.0.1")).first);
        QVERIFY(MudletApp::writeProfileData(name, qsl("port"), QString::number(server.serverPort())).first);

        THeadlessSession session;
        bool prompted = false;
        connect(&session, &THeadlessSession::outputLine, &session, [&prompted](const QString&, const QString&, const bool isPrompt) {
            prompted = prompted || isPrompt;
        });
        QStringList styled;
        connect(&session, &THeadlessSession::localTextStyled, &session, [&styled](const QString&, const QString& text, const QList<THeadlessSession::TextRun>& runs) {
            // Not the connection's own messages
            if (!text.startsWith(QLatin1Char('[')) && !text.contains(QLatin1String("connection"))) {
                styled << styledPiece(text, runs);
            }
        });
        QCOMPARE(session.start({.profile = name, .connect = true}), THeadlessSession::StartResult::started);
        QVERIFY(session.runLua(qsl(R"(tempTrigger("Welcome", [[send("from a trigger")]]))")));
        QTRY_VERIFY_WITH_TIMEOUT(server.hasPendingConnections(), 10000);
        std::unique_ptr<QTcpSocket> game(server.nextPendingConnection());
        game->write("Welcome\r\nhp 10> \xff\xf9");
        QTRY_VERIFY_WITH_TIMEOUT(prompted, 10000);
        // In a trigger, at the prompt, and below other text, each in the command's own colours
        QVERIFY(session.submit(qsl("look")));
        QVERIFY(session.runLua(qsl(R"lua(echo("later\n"))lua")));
        QVERIFY(session.submit(qsl("again")));
        QCOMPARE(styled.join(qsl(" ; ")), qsl("from a trigger\\n|#717100|own|0 ; look\\n|#717100|own|0 ; later\\n|own|own|0 ; again\\n|#717100|own|0"));
        QVERIFY(session.shutdown(false));
    }
};

#include "HeadlessSessionOutputTest.moc"
MUDLET_GROUPED_TEST_MAIN(HeadlessSessionOutputTest)
