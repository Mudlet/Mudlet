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

// Two of the older out-of-band protocols, and what a payload from them does
// once cTelnet has taken it off the wire.
//
// ATCP (setATCPVariables) hands its variables to Lua, so the tests read the
// atcp table back out of the interpreter - the same place a script would look.
// Its one reply, to Auth.Request, goes out on the socket and is read off a
// recording server.
//
// MSP (setMSPVariables) hands a parsed TMediaData to TMedia, which keeps no
// public record of it, so the visible consequence is the media file Mudlet then
// goes and fetches: the request URL carries the file name and, through the
// extension TMedia picks when there is none, whether MSP was parsed as a sound
// or a piece of music. What a malformed message must do is produce no request at
// all, which is checked by following it with a well-formed one and finding only
// that one's request. Playback itself is out of reach without an audio backend,
// so the parameters that only ever reach the player (volume, loops, priority,
// continue and tag) are exercised only far enough to show they do not stop the
// file being fetched.

#include <QFileInfo>
#include <QStringList>
#include <QTemporaryDir>
#include <QtNetwork/QTcpServer>
#include <QtNetwork/QTcpSocket>
#include <QtTest/QtTest>

#include <chrono>

#include "Host.h"
#include "MudletInstanceCoordinator.h"
#include "ProfileTestHelper.h"
#include "RecordingTelnetServer.h"
#include "TLuaInterpreter.h"
#include "TMap.h"
#include "ctelnet.h"
#include "dlgComposer.h"
#include "dlgConnectionProfiles.h"
#include "mudlet.h"
#include "utils.h"

#include "GroupedTest.h"

using namespace std::chrono_literals;

// Answers everything with 404 and remembers what was asked for. A 404 keeps the
// profile's media directory empty, which matters because a media file that
// exists is one Mudlet stops fetching.
class RecordingHttpServer : public QObject
{
public:
    explicit RecordingHttpServer(QObject* parent = nullptr)
    : QObject(parent)
    {
        connect(&mServer, &QTcpServer::newConnection, this, [this]() {
            QTcpSocket* socket = mServer.nextPendingConnection();
            if (!socket) {
                return;
            }
            connect(socket, &QTcpSocket::readyRead, this, [this, socket]() {
                // Accumulated rather than read once: a request line split across
                // two reads would otherwise be dropped without a trace, and the
                // test would blame Mudlet for a request the stub lost.
                QByteArray request = socket->property("request").toByteArray();
                request.append(socket->readAll());
                const int requestLineEnd = request.indexOf("\r\n");
                if (requestLineEnd < 0) {
                    socket->setProperty("request", request);
                    return;
                }
                const QList<QByteArray> parts = request.left(requestLineEnd).split(' ');
                if (parts.size() >= 2) {
                    mRequestedPaths.append(QString::fromLatin1(parts.at(1)));
                }
                socket->write("HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\nConnection: close\r\n\r\n");
                socket->disconnectFromHost();
            });
            connect(socket, &QTcpSocket::disconnected, socket, &QObject::deleteLater);
        });
    }

    // Port 0: an ephemeral port, so concurrent test runs cannot collide.
    [[nodiscard]] bool start() { return mServer.listen(QHostAddress::LocalHost, 0); }
    quint16 serverPort() const { return mServer.serverPort(); }

    QStringList requestedPaths() const { return mRequestedPaths; }
    void forgetRequests() { mRequestedPaths.clear(); }

private:
    // The path list is declared first so it outlives the server, and with it the
    // accepted sockets whose read handlers write into it.
    QStringList mRequestedPaths;
    QTcpServer mServer;
};

class TelnetAtcpMspTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    RecordingTelnetServer* mpServer = nullptr;
    RecordingHttpServer* mpMediaServer = nullptr;
    Host* mpHost = nullptr;
    const QString mHostname = qsl("Test-Telnet-Atcp-Msp");
    const QString mLocalhost = qsl("localhost");

    // setupConfig() consults portable.txt before the XDG logic
    static bool portableMarkerPresent()
    {
        return QFileInfo::exists(qsl("%1/portable.txt").arg(QCoreApplication::applicationDirPath())) || QFileInfo::exists(qsl("%1/.config/mudlet/portable.txt").arg(QDir::homePath()));
    }

    void feedSubnegotiation(const char option, const QByteArray& payload)
    {
        QByteArray data;
        data.append(TN_IAC).append(TN_SB).append(option).append(payload).append(TN_IAC).append(TN_SE);
        mpHost->mTelnet.loopbackTest(data);
    }

    void feedAtcp(const QByteArray& message) { feedSubnegotiation(static_cast<char>(OPT_ATCP), message); }

    void announce(const char command, const char option)
    {
        QByteArray data;
        data.append(TN_IAC).append(command).append(option);
        mpHost->mTelnet.loopbackTest(data);
    }

    // Mudlet answers a DO TIMING_MARK with WONT TIMING_MARK whatever else is
    // going on (RFC 860), so that answer arriving means everything written
    // before it has reached the server - which is what lets a forgetReceived()
    // after it drop all of the earlier traffic rather than some of it.
    void waitForEverythingSentSoFar()
    {
        announce(TN_DO, OPT_TIMING_MARK);
        QByteArray marker;
        marker.append(TN_IAC).append(TN_WONT).append(OPT_TIMING_MARK);
        QVERIFY2(QTest::qWaitFor(
                         [this, &marker]() {
                             return mpServer->received().contains(marker);
                         },
                         10000),
                 "the telnet marker never came back");
    }
    void feedMsp(const QByteArray& message) { feedSubnegotiation(OPT_MSP, message); }

    // What a script would see. The table is created by LuaGlobal.lua, so a
    // missing one means the profile's Lua support never loaded rather than that
    // ATCP failed.
    QString atcpValue(const QString& key) const
    {
        lua_State* L = mpHost->mLuaInterpreter.getLuaGlobalState();
        lua_getglobal(L, "atcp");
        if (!lua_istable(L, -1)) {
            lua_pop(L, 1);
            return QString();
        }
        lua_getfield(L, -1, key.toUtf8().constData());
        const QString value = lua_isstring(L, -1) ? QString::fromUtf8(lua_tostring(L, -1)) : QString();
        lua_pop(L, 2);
        return value;
    }

    // Scanned off the top-level widgets rather than read from cTelnet::mpComposer,
    // so a window the pointer has already let go of still shows up.
    static QList<dlgComposer*> openComposers()
    {
        QList<dlgComposer*> found;
        const QWidgetList widgets = QApplication::topLevelWidgets();
        for (QWidget* widget : widgets) {
            if (auto* composer = qobject_cast<dlgComposer*>(widget)) {
                found.append(composer);
            }
        }
        return found;
    }

    QString mediaLocation() const { return qsl("http://127.0.0.1:%1/msp/").arg(mpMediaServer->serverPort()); }

    // Waits for the media requests a test expects, so a test that sends a
    // message expected to produce nothing can follow it with one that does and
    // then check what the whole run asked for.
    bool waitForMediaRequests(const int expected)
    {
        return QTest::qWaitFor(
                [this, expected]() {
                    return mpMediaServer->requestedPaths().size() >= expected;
                },
                15000);
    }

    // The well-formed message that follows a malformed one is issued second, so
    // its request all but always arrives second - but two requests are two TCP
    // connections and nothing orders their arrival, so a short settle after it
    // is what stops a regression passing by losing that race.
    void settleMediaRequests() { QTest::qWait(200ms); }

private slots:
    void initTestCase()
    {
        if (portableMarkerPresent()) {
            QSKIP("portable.txt present - it takes precedence over XDG_CONFIG_HOME, so the config dir cannot be redirected");
        }

        // A config root of this process's own. Sharing the developer's
        // ~/.config/mudlet means sharing a profile list, so a second copy of
        // this test running at the same time is told the name it types is
        // already in use and never gets an enabled Connect button. Since #9712
        // the opt-in that makes setupConfig() adopt a directory is
        // $XDG_CONFIG_HOME/mudlet/profiles, not the mudlet directory alone.
        QVERIFY(mConfigDir.isValid());
        QVERIFY(QDir().mkpath(qsl("%1/mudlet/profiles").arg(mConfigDir.path())));
        mSavedXdg = qgetenv("XDG_CONFIG_HOME");
        qputenv("XDG_CONFIG_HOME", mConfigDir.path().toUtf8());

        mpServer = new RecordingTelnetServer(qApp);
        QVERIFY2(mpServer->start(), "RecordingTelnetServer failed to bind a loopback port");
        mpMediaServer = new RecordingHttpServer(qApp);
        QVERIFY2(mpMediaServer->start(), "RecordingHttpServer failed to bind a loopback port");

        mudlet::start();
        mudlet::self()->setupConfig();
        QCOMPARE(mudlet::getMudletPath(enums::mainPath), qsl("%1/mudlet").arg(mConfigDir.path()));
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);
        QDir(mudlet::getMudletPath(enums::profileHomePath, mHostname)).removeRecursively();

        mpHost = TestProfile::create(mHostname, mLocalhost, QString::number(mpServer->serverPort()));
        QVERIFY2(mpHost, "Could not create the test profile - see the warning above for the step that timed out.");

        QSignalSpy connected(&mpHost->mTelnet, &cTelnet::signal_connected);
        if (connected.isEmpty()) {
            QVERIFY2(connected.wait(15s), "The test profile never connected to the recording server.");
        }
    }

    void cleanupTestCase()
    {
        mpHost = nullptr;
        delete mpMediaServer;
        mpMediaServer = nullptr;
        delete mpServer;
        mpServer = nullptr;
        // Null when initTestCase skipped or failed ahead of mudlet::start(), and
        // getMudletPath() dereferences the instance rather than checking it
        if (mudlet::self()) {
            QDir(mudlet::getMudletPath(enums::profileHomePath, mHostname)).removeRecursively();
            delete mudlet::self();
        }
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
    }

    void init()
    {
        QVERIFY(mpHost);
        mpHost->mEnableMSP = true;
        mpHost->mEnableGMCP = true;
        // Every media message is answered by the local stub rather than by a
        // lookup of "www.localhost", which is where an MSP message with no URL
        // of its own would otherwise send Mudlet.
        mpHost->setMediaLocationMSP(mediaLocation());
        mpMediaServer->forgetRequests();
        mpServer->forgetReceived();
    }

    // The single-line form: everything up to the first space is the variable,
    // the rest its value, and the dots come out of the name so a script can write
    // atcp.RoomBrief.
    void test_singleLineAtcpReachesTheLuaTable()
    {
        feedAtcp("Room.Brief Cave entrance");
        QCOMPARE(atcpValue(qsl("RoomBrief")), qsl("Cave entrance"));
    }

    // The multi-line form: the first line names the variable and everything
    // after it is the value, with the line breaks taken out.
    void test_multiLineAtcpKeepsEveryLineOfTheValue()
    {
        feedAtcp("Char.Vitals\nhp:100\nmana:50");
        QCOMPARE(atcpValue(qsl("CharVitals")), qsl("hp:100mana:50"));
    }

    // A first line that carries a word after the variable name: the word belongs
    // to the value, not to the name, or the name Lua is given is one no script
    // could have been written against.
    void test_wordsAfterTheVariableNameBelongToTheValue()
    {
        feedAtcp("Char.Status ready\nhp:100");
        QCOMPARE(atcpValue(qsl("CharStatus")), qsl("ready hp:100"));
    }

    // Room.Num is the one ATCP variable that both reaches the Lua table and moves
    // state inside cTelnet: it is what tells the mapper where the player is.
    void test_roomNumMovesTheMapper()
    {
        QVERIFY2(mpHost->mpMap, "the profile has no map, so there is nothing for Room.Num to move");
        feedAtcp("Room.Num 4242");
        QCOMPARE(atcpValue(qsl("RoomNum")), qsl("4242"));
        QCOMPARE(mpHost->mpMap->mRoomIdHash.value(mHostname), 4242);
    }

    // The only thing Mudlet sends unprompted in reply to an incoming ATCP
    // message. A game that asks for it and gets nothing has no way to know what
    // it is talking to.
    void test_authRequestIsAnsweredWithTheClientHandshake()
    {
        feedAtcp("Auth.Request");

        QVERIFY2(QTest::qWaitFor(
                         [this]() {
                             return !subnegotiationsFor(mpServer->received(), OPT_ATCP).isEmpty();
                         },
                         10000),
                 "Mudlet never answered Auth.Request");

        const QList<Subnegotiation> replies = subnegotiationsFor(mpServer->received(), OPT_ATCP);
        QCOMPARE(replies.size(), 1);
        const QByteArray hello = replies.first().payload;
        QVERIFY2(hello.startsWith("hello Mudlet "), qPrintable(qsl("the ATCP handshake did not name the client: %1").arg(QString::fromLatin1(hello))));
        for (const char* capability : {"composer 1", "char_vitals 1", "room_brief 1", "room_exits 1", "map_display 1"}) {
            QVERIFY2(hello.contains(capability), qPrintable(qsl("the ATCP handshake did not offer %1: %2").arg(QString::fromLatin1(capability), QString::fromLatin1(hello))));
        }
    }

    // Client.Compose puts an editor up. A game that repeats the request - or
    // spams it - must not stack a second one on top, since only the newest
    // would be the one cTelnet then answers for.
    void test_composeOpensOneEditorHoweverOftenItIsAskedFor()
    {
        QCOMPARE(openComposers().size(), 0);

        feedAtcp("Client.Compose Note\nfirst draft");
        QCOMPARE(openComposers().size(), 1);
        dlgComposer* composer = openComposers().first();
        QCOMPARE(composer->title->text(), qsl("Note"));
        QCOMPARE(composer->edit->toPlainText(), qsl("first draft"));

        feedAtcp("Client.Compose Second\nanother draft");
        QCOMPARE(openComposers().size(), 1);
        QCOMPARE(openComposers().first()->title->text(), qsl("Note"));

        // Cancelling tells the game the buffer was abandoned, and takes the
        // window with it - the editor is deleted on close.
        mpHost->mTelnet.atcpComposerCancel();
        QVERIFY2(QTest::qWaitFor(
                         []() {
                             return openComposers().isEmpty();
                         },
                         5000),
                 "cancelling the composer left its window behind");
        QVERIFY2(QTest::qWaitFor(
                         [this]() {
                             return mpServer->received().contains("*q\nno\n");
                         },
                         10000),
                 "cancelling the composer did not tell the game the buffer was abandoned");
    }

    // Saving sends the buffer back. With GMCP on - the default - that is an
    // IRE.Composer.SetBuffer with the text quoted, so a draft carrying a quote,
    // a backslash or a line break cannot end the message early.
    void test_savingTheComposerQuotesTheBufferItSendsBack()
    {
        // The preference picks the protocol and the negotiated flag then has to
        // agree, so GMCP has to be both preferred (it is, by default) and offered
        // - which the recording server never does on its own.
        announce(TN_WILL, static_cast<char>(OPT_GMCP));
        QVERIFY2(mpHost->mTelnet.isGMCPEnabled(), "GMCP did not turn on, so the composer had nothing to send its buffer over");
        // Turning GMCP on makes Mudlet introduce itself over it, so wait that
        // out before forgetting - otherwise the handshake lands in the capture
        // this test then reads.
        waitForEverythingSentSoFar();
        mpServer->forgetReceived();

        feedAtcp("Client.Compose Note");
        QCOMPARE(openComposers().size(), 1);
        QCOMPARE(openComposers().first()->title->text(), qsl("Note"));
        QCOMPARE(openComposers().first()->edit->toPlainText(), QString());

        mpHost->mTelnet.atcpComposerSave(qsl("a \"quoted\" back\\slash\nand a second line"));

        QVERIFY2(QTest::qWaitFor(
                         [this]() {
                             return !subnegotiationsFor(mpServer->received(), OPT_GMCP).isEmpty();
                         },
                         10000),
                 "saving the composer sent no GMCP message");
        const QList<Subnegotiation> sent = subnegotiationsFor(mpServer->received(), OPT_GMCP);
        QCOMPARE(sent.size(), 1);
        // moc mis-lexes this particular raw string - the quotes inside it leave it
        // thinking the class is still inside a string, and it then emits no meta
        // object at all - so it stays escaped. Raw strings are fine elsewhere.
        QCOMPARE(sent.first().payload, QByteArray("IRE.Composer.SetBuffer \"a \\\"quoted\\\" back\\\\slash\\nand a second line\""));

        QVERIFY2(QTest::qWaitFor(
                         []() {
                             return openComposers().isEmpty();
                         },
                         5000),
                 "saving the composer left its window behind");
    }

    // The other arm of the same save: a profile with GMCP turned off sends the
    // buffer as ATCP instead, in a completely different shape - and with no
    // quoting at all, which is what an ATCP-only game receives.
    void test_savingTheComposerFallsBackToAtcpWithGmcpOff()
    {
        mpHost->mEnableGMCP = false;
        announce(TN_WILL, static_cast<char>(OPT_ATCP));
        QVERIFY2(mpHost->mTelnet.isATCPEnabled(), "ATCP did not turn on, so the composer had nothing to send its buffer over");
        waitForEverythingSentSoFar();
        mpServer->forgetReceived();

        feedAtcp("Client.Compose Note");
        QCOMPARE(openComposers().size(), 1);
        mpHost->mTelnet.atcpComposerSave(qsl("a \"quoted\" line"));

        QVERIFY2(QTest::qWaitFor(
                         [this]() {
                             return !subnegotiationsFor(mpServer->received(), OPT_ATCP).isEmpty();
                         },
                         10000),
                 "saving the composer sent no ATCP message");
        const QList<Subnegotiation> sent = subnegotiationsFor(mpServer->received(), OPT_ATCP);
        QCOMPARE(sent.size(), 1);
        QCOMPARE(sent.first().payload, QByteArray("olesetbuf \n a \"quoted\" line\n"));
        QVERIFY2(mpServer->received().contains("*s\n"), "the ATCP save did not tell the game the buffer was submitted");

        QVERIFY2(QTest::qWaitFor(
                         []() {
                             return openComposers().isEmpty();
                         },
                         5000),
                 "saving the composer left its window behind");
    }

    // MSP names a file without an extension and leaves the client to pick one
    // per media type, so the file requested is where a sound and a piece of
    // music part company.
    void test_soundAndMusicPickTheirOwnDefaultExtension()
    {
        feedMsp(QByteArray("!!SOUND(bark U=") + mediaLocation().toUtf8() + ")");
        QVERIFY2(waitForMediaRequests(1), "no media was fetched for a well-formed !!SOUND");
        feedMsp("!!MUSIC(theme)");
        QVERIFY2(waitForMediaRequests(2), "no media was fetched for a well-formed !!MUSIC");

        QCOMPARE(mpMediaServer->requestedPaths(), QStringList({qsl("/msp/bark.wav"), qsl("/msp/theme.mid")}));
    }

    // An extension the game supplied is left alone, and a sub-directory in the
    // name is part of the file rather than something to flatten away.
    void test_anExplicitExtensionAndSubdirectoryAreKept()
    {
        feedMsp("!!SOUND(fx/step.ogg)");
        QVERIFY2(waitForMediaRequests(1), "no media was fetched for a file name with a sub-directory");
        QCOMPARE(mpMediaServer->requestedPaths(), QStringList({qsl("/msp/fx/step.ogg")}));
    }

    // The URL a game supplies is remembered on the profile, which is what lets
    // its later messages leave it out.
    void test_theMediaUrlIsRememberedOnTheProfile()
    {
        const QString supplied = qsl("http://127.0.0.1:%1/other/").arg(mpMediaServer->serverPort());
        feedMsp(QByteArray("!!SOUND(bark.wav U=") + supplied.toUtf8() + ")");
        QVERIFY2(waitForMediaRequests(1), "no media was fetched for a message carrying its own URL");
        QCOMPARE(mpMediaServer->requestedPaths(), QStringList({qsl("/other/bark.wav")}));
        QCOMPARE(mpHost->mediaLocationMSP(), supplied);
    }

    // Anything that does not meet the MSP standard has to be dropped rather than
    // half-parsed. The well-formed message at the end is what proves the ones
    // before it were dropped and not merely slow.
    void test_messagesThatDoNotMeetTheStandardAreDropped_data()
    {
        QTest::addColumn<QByteArray>("message");

        QTest::newRow("no closing parenthesis") << QByteArray("!!SOUND(bark.wav");
        QTest::newRow("not a sound or music command") << QByteArray("!!VIDEO(bark.wav)");
        QTest::newRow("no command at all") << QByteArray("bark.wav)");
        QTest::newRow("a parameter with no value") << QByteArray("!!SOUND(bark.wav V)");
        QTest::newRow("a parameter with two values") << QByteArray("!!SOUND(bark.wav V=1=2)");
    }

    void test_messagesThatDoNotMeetTheStandardAreDropped()
    {
        QFETCH(QByteArray, message);

        feedMsp(message);
        feedMsp("!!SOUND(sentinel.wav)");
        QVERIFY2(waitForMediaRequests(1), "the well-formed message that follows the malformed one fetched nothing either");
        settleMediaRequests();
        QCOMPARE(mpMediaServer->requestedPaths(), QStringList({qsl("/msp/sentinel.wav")}));
    }

    // The robustness principle: a parameter Mudlet does not understand is
    // skipped, and the ones it does understand are still applied - so the file
    // is still fetched rather than the whole message being thrown away.
    void test_parametersMudletDoesNotUnderstandAreSkipped()
    {
        feedMsp("!!SOUND(bark.wav V=50 L=3 P=70 C=0 T=combat Z=9)");
        QVERIFY2(waitForMediaRequests(1), "a message with an unrecognised parameter was thrown away instead of being fetched");
        QCOMPARE(mpMediaServer->requestedPaths(), QStringList({qsl("/msp/bark.wav")}));
    }

    // A server-supplied file name that names somewhere other than inside the
    // profile's media directory must not be fetched, since fetching it is what
    // would write it there.
    void test_aFileNameOutsideTheMediaDirectoryIsRefused_data()
    {
        QTest::addColumn<QByteArray>("message");

        QTest::newRow("parent directory traversal") << QByteArray("!!SOUND(../escape.wav)");
        QTest::newRow("absolute path") << QByteArray("!!SOUND(/tmp/escape.wav)");
    }

    void test_aFileNameOutsideTheMediaDirectoryIsRefused()
    {
        QFETCH(QByteArray, message);

        feedMsp(message);
        feedMsp("!!SOUND(sentinel.wav)");
        QVERIFY2(waitForMediaRequests(1), "the sentinel after the refused file name fetched nothing");
        settleMediaRequests();
        QCOMPARE(mpMediaServer->requestedPaths(), QStringList({qsl("/msp/sentinel.wav")}));
    }

    // The standard puts a line ending on an MSP message. It has to come off
    // before the closing parenthesis is looked for, or every well-formed message
    // a game sends reads as malformed.
    void test_theLineEndingTheStandardPutsOnAMessageIsStripped()
    {
        feedMsp("!!SOUND(bark.wav)\r\n");
        QVERIFY2(waitForMediaRequests(1), "a message with the line ending the standard puts on it fetched nothing");
        QCOMPARE(mpMediaServer->requestedPaths(), QStringList({qsl("/msp/bark.wav")}));
    }

    // MSP turned off is MSP not acted on, however well-formed the message.
    void test_nothingIsFetchedWhileMspIsTurnedOff()
    {
        mpHost->mEnableMSP = false;
        feedMsp("!!SOUND(refused.wav)");
        mpHost->mEnableMSP = true;
        feedMsp("!!SOUND(sentinel.wav)");
        QVERIFY2(waitForMediaRequests(1), "the sentinel sent once MSP was back on fetched nothing");
        settleMediaRequests();
        QCOMPARE(mpMediaServer->requestedPaths(), QStringList({qsl("/msp/sentinel.wav")}));
    }
};

#include "TelnetAtcpMspTest.moc"
MUDLET_GROUPED_TEST_MAIN(TelnetAtcpMspTest)
