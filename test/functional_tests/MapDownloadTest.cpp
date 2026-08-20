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

/*
 * Covers TMap::downloadMap() and the four slots that finish the job it starts -
 * slot_setDownloadProgress(), slot_downloadError(), slot_downloadCancel() and
 * slot_replyFinished() - against a stub HTTP server standing in for a game's
 * MMP map.
 *
 * In production nothing but a widget starts one: the mapper's empty-state
 * "Download from game" button, the map context menu, and the preferences'
 * Mapper tab, which creates the mapper first if there is none. There is no Lua
 * entry point, so the busted suite cannot reach any of this.
 *
 * All three of those callers therefore have a visible mapper, and
 * createTransferProgress() puts progress on that widget when there is one. The
 * tests below deliberately run with no mapper at all, which is the other branch:
 * the standalone progress signals a frontend renders as its own dialog, and the
 * one a hidden-mapper profile takes. The last test in the file creates a mapper
 * and cannot be undone, which is why it is last.
 *
 * A real profile rather than a bare Host, because the interesting part of the
 * chain is what happens after the bytes land: the reply handler writes the file,
 * parses it, and reports every outcome to the console through Host::postMessage()
 * - and hands a non-XML file to TMainConsole::loadMap(). A Host with no console
 * would stack those messages up unread and crash on that last call.
 *
 * Run with: ctest -R MapDownloadTest -V
 */

#include <QFileInfo>
#include <QHash>
#include <QHostAddress>
#include <QSettings>
#include <QSignalSpy>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTemporaryDir>
#include <QTimer>
#include <QtTest/QtTest>

#include "Host.h"
#include "MudletInstanceCoordinator.h"
#include "ProfileTestHelper.h"
#include "TMainConsole.h"
#include "TMap.h"
#include "TRoom.h"
#include "TRoomDB.h"
#include "TelnetServerStub.h"
#include "ctelnet.h"
#include "dlgMapper.h"
#include "mudlet.h"

#include "GroupedTest.h"

#include <chrono>

namespace {

// The shape I.R.E. games serve: a <map> document of areas and rooms, which is
// what XMLimport::readMap() consumes. Deliberately hand-written - Mudlet has no
// XML map exporter to generate one with.
const QByteArray scmMapXml = QByteArrayLiteral("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
                                               "<map>\n"
                                               " <areas>\n"
                                               "  <area id=\"1\" name=\"Downloaded Area\"/>\n"
                                               " </areas>\n"
                                               " <rooms>\n"
                                               "  <room id=\"1\" area=\"1\" title=\"Entrance\" environment=\"2\">\n"
                                               "   <coord x=\"0\" y=\"0\" z=\"0\"/>\n"
                                               "   <exit direction=\"east\" target=\"2\"/>\n"
                                               "  </room>\n"
                                               "  <room id=\"2\" area=\"1\" title=\"Hallway\" environment=\"2\">\n"
                                               "   <coord x=\"1\" y=\"0\" z=\"0\"/>\n"
                                               "   <exit direction=\"west\" target=\"1\"/>\n"
                                               "  </room>\n"
                                               " </rooms>\n"
                                               "</map>\n");

// TMap::downloadMap() has no idea how big the file will be, so it seeds
// mExpectedFileSize with this until the reply reports a real total:
constexpr int scmAssumedFileSize = 4000000;

} // namespace

// A minimum-viable HTTP server serving one canned answer per path. Every route
// is opt-in so an unregistered path is a 404, which is what a game that has no
// map to offer produces.
class StubMapServer : public QObject
{
    Q_OBJECT

public:
    enum class Answer {
        // 200 with a Content-Length, sent in one go
        Whole,
        // 200 with a Content-Length, released a slice at a time until
        // finishAnswer() ends it - see paceUntilReported()
        Paced,
        // 200 with no Content-Length, released the same way, so the reply
        // reports no total until the connection closes
        PacedUnsized,
        // 200 with a Content-Length far larger than what is ever sent, so the
        // download stays in flight until the client gives up on it
        Stalled,
    };

    explicit StubMapServer(QObject* parent = nullptr)
    : QObject(parent)
    {
        connect(&mServer, &QTcpServer::newConnection, this, &StubMapServer::acceptConnections);
        mSliceTimer.setInterval(scmSliceInterval);
        connect(&mSliceTimer, &QTimer::timeout, this, &StubMapServer::writeSlice);
    }

    // Port 0: the OS picks a free one, so concurrent runs of this test cannot
    // collide on it. Read it back with url().
    bool listen() { return mServer.listen(QHostAddress::LocalHost, 0); }

    QString url(const QString& path) const { return qsl("http://127.0.0.1:%1%2").arg(QString::number(mServer.serverPort()), path); }

    void serve(const QString& path, const QByteArray& body, const Answer answer = Answer::Whole) { mRoutes.insert(path, {answer, body}); }

    QStringList requestedPaths() const { return mRequestedPaths; }
    void forgetRequests() { mRequestedPaths.clear(); }

    // Ends a paced answer: writes whatever body is left and closes. The test
    // calls this once the reply has reported progress as often as it needs.
    void finishAnswer()
    {
        mSliceTimer.stop();
        if (!mpPacedSocket) {
            return;
        }
        mpPacedSocket->write(mPacedRemainder);
        mpPacedSocket->flush();
        mpPacedSocket->disconnectFromHost();
        mpPacedSocket = nullptr;
        mPacedRemainder.clear();
    }

private:
    struct Route
    {
        Answer answer{Answer::Whole};
        QByteArray body;
    };

    void acceptConnections()
    {
        while (auto* socket = mServer.nextPendingConnection()) {
            connect(socket, &QTcpSocket::readyRead, this, [this, socket]() {
                readRequest(socket);
            });
            connect(socket, &QTcpSocket::disconnected, this, [this, socket]() {
                // A client that gives up mid-answer - the cancel tests, and
                // cleanup() after any failure - takes the connection with it,
                // so the slices still queued for it have nowhere to go.
                if (mpPacedSocket == socket) {
                    mSliceTimer.stop();
                    mpPacedSocket = nullptr;
                    mPacedRemainder.clear();
                }
                mBuffers.remove(socket);
                socket->deleteLater();
            });
        }
    }

    void readRequest(QTcpSocket* socket)
    {
        QByteArray& buffer = mBuffers[socket];
        buffer.append(socket->readAll());
        if (!buffer.contains("\r\n\r\n")) {
            return;
        }
        const QList<QByteArray> requestLine = buffer.left(buffer.indexOf("\r\n")).split(' ');
        mBuffers.remove(socket);
        const QString path = requestLine.size() > 1 ? QString::fromUtf8(requestLine.at(1)) : QString();
        mRequestedPaths << path;

        if (!mRoutes.contains(path)) {
            socket->write("HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\nConnection: close\r\n\r\n");
            socket->disconnectFromHost();
            return;
        }

        const Route route = mRoutes.value(path);
        switch (route.answer) {
        case Answer::Whole:
            sendHeader(socket, route.body.size());
            socket->write(route.body);
            socket->flush();
            socket->disconnectFromHost();
            return;
        case Answer::Paced:
            sendHeader(socket, route.body.size());
            startPacing(socket, route.body);
            return;
        case Answer::PacedUnsized:
            socket->write("HTTP/1.1 200 OK\r\nContent-Type: text/xml\r\nConnection: close\r\n\r\n");
            startPacing(socket, route.body);
            return;
        case Answer::Stalled:
            sendHeader(socket, scmAssumedFileSize);
            socket->write(route.body);
            socket->flush();
            return;
        }
    }

    static void sendHeader(QTcpSocket* socket, const qsizetype length)
    {
        socket->write("HTTP/1.1 200 OK\r\nContent-Type: text/xml\r\nContent-Length: " + QByteArray::number(length) + "\r\nConnection: close\r\n\r\n");
    }

    void startPacing(QTcpSocket* socket, const QByteArray& body)
    {
        mpPacedSocket = socket;
        mPacedRemainder = body;
        mSliceTimer.start();
    }

    void writeSlice()
    {
        if (!mpPacedSocket) {
            mSliceTimer.stop();
            return;
        }
        if (mPacedRemainder.isEmpty()) {
            // The body ran out before the test had the reports it was waiting
            // for: end the answer anyway, so that test fails on its own count
            // assertion rather than on a download that never finishes.
            finishAnswer();
            return;
        }
        mpPacedSocket->write(mPacedRemainder.left(scmSliceSize));
        mpPacedSocket->flush();
        mPacedRemainder.remove(0, scmSliceSize);
    }

    // Small enough, and often enough, that a paced body outlasts by a wide
    // margin the handful of progress reports the tests wait for: the padded map
    // is 64 slices, well over a second of an answer that reports several times
    // a second.
    static constexpr qsizetype scmSliceSize = 4096;
    static constexpr auto scmSliceInterval = std::chrono::milliseconds(20);

    QTcpServer mServer;
    QHash<QTcpSocket*, QByteArray> mBuffers;
    QHash<QString, Route> mRoutes;
    QStringList mRequestedPaths;
    QTimer mSliceTimer;
    QTcpSocket* mpPacedSocket = nullptr;
    QByteArray mPacedRemainder;
};

class MapDownloadTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    TelnetServerStub* mpTelnetServer = nullptr;
    StubMapServer* mpMapServer = nullptr;
    Host* mpHost = nullptr;
    const QString mProfileName = qsl("MapDownload-Test");
    const QString mLocalhost = qsl("localhost");
    int mConsoleMark = 0;
    QByteArray mPaddedMapXml;

    // setupConfig() consults portable.txt before the XDG logic
    static bool portableMarkerPresent()
    {
        return QFileInfo::exists(qsl("%1/portable.txt").arg(QCoreApplication::applicationDirPath())) || QFileInfo::exists(qsl("%1/.config/mudlet/portable.txt").arg(QDir::homePath()));
    }

    void deleteProfileDirectory() const
    {
        QDir dir(mudlet::getMudletPath(enums::profileHomePath, mProfileName));
        if (dir.exists()) {
            dir.removeRecursively();
        }
    }

    // Everything the download chain reports, it reports to the console. Reading
    // from a mark set at the start of each test keeps one test's messages from
    // answering another's assertion.
    QString consoleTextSinceMark() const
    {
        QString text;
        auto& buffer = mpHost->mpConsole->buffer;
        // From the mark itself, not past it: every console message ends in a
        // newline, so the line the mark names is the empty one the next message
        // starts writing into rather than the previous test's last line.
        for (int i = mConsoleMark, last = buffer.getLastLineNumber(); i <= last; ++i) {
            text.append(buffer.line(i)).append(QChar::Space);
        }
        return text;
    }

    bool consoleShows(const QString& needle) const { return consoleTextSinceMark().contains(needle); }

    // Every exit from slot_replyFinished() runs the same cleanup, and closing
    // the progress display is the only part of it anything outside can see. The
    // import flag is cleared just after, so waiting on the signal through the
    // event loop - rather than reading the spy the instant it fires - is what
    // makes "the download is over" true by the time this returns.
    bool runDownload(const QString& remoteUrl, const QString& localFileName = QString())
    {
        TMap* pMap = mpHost->mpMap.data();
        QSignalSpy closeSpy(pMap, &TMap::signal_mapProgressClose);
        pMap->downloadMap(remoteUrl, localFileName);
        return closeSpy.count() == 1 || closeSpy.wait(15000);
    }

    void watchMapDownloadEvent() const
    {
        mpHost->getLuaInterpreter()->compileAndExecuteScript(qsl("mapDownloadsSeen = 0\n"
                                                                 "registerAnonymousEventHandler('sysMapDownloadEvent', function() mapDownloadsSeen = mapDownloadsSeen + 1 end)"));
    }

    void forgetMapDownloadEvents() const { mpHost->getLuaInterpreter()->compileAndExecuteScript(qsl("mapDownloadsSeen = 0")); }

    bool mapDownloadEventCountIs(const int expected) const { return mpHost->getLuaInterpreter()->compileAndExecuteScript(qsl("assert(mapDownloadsSeen == %1)").arg(expected)); }

    // The parse that follows a download reports its own progress on the same
    // display, so the download's reports are the ones before the cancel button
    // is taken away. Counting the emissions at that moment separates the two.
    struct ProgressPhases
    {
        int ranges{0};
        int values{0};
    };

    QMetaObject::Connection markParseStart(const QSignalSpy& rangeSpy, const QSignalSpy& valueSpy, ProgressPhases& phases) const
    {
        TMap* pMap = mpHost->mpMap.data();
        return connect(pMap, &TMap::signal_mapProgressDisableCancel, pMap, [&rangeSpy, &valueSpy, &phases]() {
            phases.ranges = rangeSpy.count();
            phases.values = valueSpy.count();
        });
    }

    // A reply reports its progress no more often than an interval of Qt's own,
    // merging everything that lands inside one into a single report - so what a
    // server writes in parts can be answered in fewer reports than it had
    // parts, and on a loaded machine regularly is. The two tests that need
    // several reports each therefore keep the answer flowing until the reports
    // have actually arrived rather than counting on a write to produce one: the
    // body is released a slice at a time and only ended here, which makes the
    // count the tests then assert on a fact rather than a hope.
    QMetaObject::Connection paceUntilReported(const QSignalSpy& valueSpy, const int reports) const
    {
        TMap* pMap = mpHost->mpMap.data();
        return connect(pMap, &TMap::signal_mapProgressSetValue, pMap, [this, &valueSpy, reports]() {
            if (valueSpy.count() >= reports) {
                mpMapServer->finishAnswer();
            }
        });
    }

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

        // Every assertion below reads the console, and the console is
        // translated: with nothing in this run's own config to say otherwise,
        // mudlet takes its language from the desktop's locale, so on an en_GB
        // machine this looks for "canceled" in a console that said "cancelled".
        // Written before setupConfig() opens that same file, because the value
        // is read out of it later by init(), via readEarlySettings().
        QSettings language(qsl("%1/mudlet/Mudlet.ini").arg(mConfigDir.path()), QSettings::IniFormat);
        language.setValue(qsl("interfaceLanguage"), qsl("en_US"));
        language.sync();

        mpTelnetServer = new TelnetServerStub(qApp);
        mpTelnetServer->start(mLocalhost, 0);

        mudlet::start();
        mudlet::self()->setupConfig();
        QCOMPARE(mudlet::getMudletPath(enums::mainPath), qsl("%1/mudlet").arg(mConfigDir.path()));
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);
        deleteProfileDirectory();

        mpHost = TestProfile::create(mProfileName, mLocalhost, QString::number(mpTelnetServer->serverPort()));
        QVERIFY2(mpHost, "the test profile could not be created");
        QSignalSpy connectionSpy(&(mpHost->mTelnet), &cTelnet::signal_connected);
        QVERIFY2(connectionSpy.count() == 1 || connectionSpy.wait(5000), "the test profile never connected to the stub game");
        QVERIFY(mpHost->mpConsole);
        // No mapper widget yet, so every download below takes the standalone
        // progress path the signals under test belong to. The last test in this
        // file creates one, which is why it is last.
        QVERIFY2(mpHost->mpMap->mpMapper.isNull(), "a mapper widget already exists, so the standalone progress path will not be taken");

        watchMapDownloadEvent();

        // Big enough to be released in many slices, so a paced answer can keep
        // reporting progress for as long as the tests below need it to:
        mPaddedMapXml = scmMapXml;
        mPaddedMapXml.insert(mPaddedMapXml.indexOf("</map>"), "<!-- " + QByteArray(256 * 1024, 'x') + " -->\n");

        mpMapServer = new StubMapServer(qApp);
        QVERIFY2(mpMapServer->listen(), "the stub map server could not listen on localhost");
        mpMapServer->serve(qsl("/map.xml"), scmMapXml);
        mpMapServer->serve(qsl("/mmp.xml"), scmMapXml);
        mpMapServer->serve(qsl("/paced.xml"), mPaddedMapXml, StubMapServer::Answer::Paced);
        mpMapServer->serve(qsl("/unsized.xml"), mPaddedMapXml, StubMapServer::Answer::PacedUnsized);
        mpMapServer->serve(qsl("/stalled.xml"), QByteArray(64, 'x'), StubMapServer::Answer::Stalled);
    }

    void cleanupTestCase()
    {
        mpHost = nullptr;
        delete mpMapServer;
        mpMapServer = nullptr;
        delete mpTelnetServer;
        mpTelnetServer = nullptr;
        // Null when initTestCase skipped or failed ahead of mudlet::start(), and
        // getMudletPath() dereferences the instance rather than checking it
        if (mudlet::self()) {
            deleteProfileDirectory();
            delete mudlet::self();
        }
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
    }

    void init()
    {
        mConsoleMark = mpHost->mpConsole->buffer.getLastLineNumber();
        mpMapServer->forgetRequests();
        forgetMapDownloadEvents();
    }

    // A test that fails part way through can leave a download in flight, and
    // the import flag it holds would refuse every download after it - so the
    // one failure would be reported as several. Unconditional because the flag
    // is not observable from here and a cancel with nothing to cancel costs
    // only a console line, which the next test's mark leaves behind anyway.
    void cleanup()
    {
        mpHost->mpMap->slot_downloadCancel();
        qApp->processEvents();
        // Here rather than at the end of the case that sets it, so a failure
        // part way through that case cannot leak it into the next one:
        mpHost->mpMap->setMmpMapLocation(QString());
    }

    // The whole chain on a good day: request, save, parse, announce.
    void test_downloadedXmlMapIsSavedAndParsed()
    {
        TMap* pMap = mpHost->mpMap.data();
        pMap->mapClear();
        QVERIFY(pMap->mpRoomDB->isEmpty());

        QSignalSpy startSpy(pMap, &TMap::signal_mapTransferProgressStart);
        QSignalSpy disableCancelSpy(pMap, &TMap::signal_mapProgressDisableCancel);

        QVERIFY2(runDownload(mpMapServer->url(qsl("/map.xml"))), "the map download never finished");

        QCOMPARE(mpMapServer->requestedPaths(), QStringList{qsl("/map.xml")});
        QCOMPARE(startSpy.count(), 1);
        QCOMPARE(startSpy.at(0).at(0).toString(), qsl("Map download"));
        QVERIFY2(startSpy.at(0).at(1).toString().contains(mProfileName), "the progress label does not name the profile the map is for");
        // The download can be aborted; the parse that follows it cannot, so the
        // cancel button has to go away between the two.
        QCOMPARE(disableCancelSpy.count(), 1);

        // A download with no local name of its own and an URL ending in "xml"
        // lands on the profile's map.xml, inside this test's own config root:
        const QString stored = mudlet::getMudletPath(enums::profileXmlMapPathFileName, mProfileName);
        QFile file(stored);
        QVERIFY2(file.open(QIODevice::ReadOnly), qPrintable(qsl("the downloaded map was not saved to %1").arg(stored)));
        QCOMPARE(file.readAll(), scmMapXml);

        QCOMPARE(pMap->mpRoomDB->getRoomMap().size(), 2);
        QCOMPARE(pMap->mpRoomDB->getAreaNamesMap().value(1), qsl("Downloaded Area"));
        TRoom* pRoom = pMap->mpRoomDB->getRoom(1);
        QVERIFY(pRoom);
        QCOMPARE(pRoom->name, qsl("Entrance"));
        QCOMPARE(pRoom->getEast(), 2);

        QVERIFY2(consoleShows(qsl("Map download initiated")), qPrintable(consoleTextSinceMark()));
        QVERIFY2(consoleShows(qsl("map downloaded and stored")), qPrintable(consoleTextSinceMark()));
        QVERIFY2(mapDownloadEventCountIs(1), "sysMapDownloadEvent was not raised exactly once for a successful download");
        QVERIFY(!pMap->hasActiveTransferProgress());
    }

    // The first progress report always seeds the range from the guess without
    // consulting the total it was handed, even when the reply knew its size all
    // along - so a real total can only take effect on a later report, which is
    // why this download is served in parts.
    void test_progressRangeStartsAsAGuessAndIsCorrectedFromTheReply()
    {
        TMap* pMap = mpHost->mpMap.data();
        QSignalSpy rangeSpy(pMap, &TMap::signal_mapProgressSetRange);
        QSignalSpy valueSpy(pMap, &TMap::signal_mapProgressSetValue);
        ProgressPhases phases;
        const QMetaObject::Connection mark = markParseStart(rangeSpy, valueSpy, phases);
        // Two reports: the first seeds the range from the guess, and the second
        // is the earliest one that can correct it from the reply's own total.
        const QMetaObject::Connection pacer = paceUntilReported(valueSpy, 2);

        const bool finished = runDownload(mpMapServer->url(qsl("/paced.xml")));
        disconnect(mark);
        disconnect(pacer);
        QVERIFY2(finished, "the map download never finished");

        QCOMPARE(phases.ranges, 2);
        QCOMPARE(rangeSpy.at(0).at(0).toInt(), 0);
        QCOMPARE(rangeSpy.at(0).at(1).toInt(), scmAssumedFileSize);
        QCOMPARE(rangeSpy.at(1).at(1).toInt(), static_cast<int>(mPaddedMapXml.size()));
        QVERIFY(phases.values > 0);
        QCOMPARE(valueSpy.at(phases.values - 1).at(0).toInt(), static_cast<int>(mPaddedMapXml.size()));
    }

    // A reply with no Content-Length reports its total as -1 while it is being
    // received - the I.R.E. games' behaviour the guard was written for - and
    // only learns its own size once the connection closes. Every range the
    // download sets has to be a maximum a progress bar can use, so none of the
    // -1s may reach one.
    void test_progressRangeIsNeverSetFromAReplyWithNoTotal()
    {
        TMap* pMap = mpHost->mpMap.data();
        QSignalSpy rangeSpy(pMap, &TMap::signal_mapProgressSetRange);
        QSignalSpy valueSpy(pMap, &TMap::signal_mapProgressSetValue);
        ProgressPhases phases;
        const QMetaObject::Connection mark = markParseStart(rangeSpy, valueSpy, phases);
        // Three, so that a progress report carrying no total lands after the
        // range has already been set from the guess - which is the only moment
        // the "is the total known" guard decides anything.
        const QMetaObject::Connection pacer = paceUntilReported(valueSpy, 3);

        const bool finished = runDownload(mpMapServer->url(qsl("/unsized.xml")));
        disconnect(mark);
        disconnect(pacer);
        QVERIFY2(finished, "the map download never finished");

        // Without a report that arrives while the total is still unknown AND
        // the range already set, the guard below is never consulted and the
        // rest of this would pass without testing anything:
        QVERIFY2(phases.values >= 3, qPrintable(qsl("the reply reported progress %1 times, too few to reach the guard").arg(phases.values)));
        QVERIFY(phases.ranges >= 1);
        QCOMPARE(rangeSpy.at(0).at(1).toInt(), scmAssumedFileSize);
        for (int i = 0; i < phases.ranges; ++i) {
            const int maximum = rangeSpy.at(i).at(1).toInt();
            QVERIFY2(maximum > 0, qPrintable(qsl("progress range %1 of %2 was given a maximum of %3").arg(i).arg(phases.ranges).arg(maximum)));
        }
    }

    // A game with no map to serve answers 404. Nothing must be left behind: the
    // next download has to be accepted, which is what proves the import flag was
    // cleared on the way out.
    void test_httpErrorIsReportedAndLeavesNothingBehind()
    {
        TMap* pMap = mpHost->mpMap.data();
        const QString stored = mudlet::getMudletPath(enums::profileXmlMapPathFileName, mProfileName);
        QFile::remove(stored);

        QVERIFY2(runDownload(mpMapServer->url(qsl("/nosuchmap.xml"))), "the failed map download never finished");

        QVERIFY2(consoleShows(qsl("Map download encountered an error")), qPrintable(consoleTextSinceMark()));
        QVERIFY2(!QFileInfo::exists(stored), "a failed download still wrote a map file");
        QVERIFY(!pMap->hasActiveTransferProgress());
        QVERIFY2(mapDownloadEventCountIs(0), "a failed download raised sysMapDownloadEvent");

        // ...and the chain is free to run again:
        QVERIFY2(runDownload(mpMapServer->url(qsl("/map.xml"))), "the download after a failed one never finished");
        QVERIFY2(QFileInfo::exists(stored), "the download after a failed one was refused, so the import flag was left set");
        QVERIFY2(mapDownloadEventCountIs(1), "the download after a failed one did not raise sysMapDownloadEvent");
    }

    void test_secondDownloadIsRefusedWhileOneIsStillRunning()
    {
        TMap* pMap = mpHost->mpMap.data();
        QSignalSpy startSpy(pMap, &TMap::signal_mapTransferProgressStart);

        pMap->downloadMap(mpMapServer->url(qsl("/stalled.xml")));
        QVERIFY2(QTest::qWaitFor(
                         [this]() {
                             return !mpMapServer->requestedPaths().isEmpty();
                         },
                         10000),
                 "the first download never reached the server");

        pMap->downloadMap(mpMapServer->url(qsl("/map.xml")));

        QVERIFY2(consoleShows(qsl("when one has already been")), qPrintable(consoleTextSinceMark()));
        QCOMPARE(mpMapServer->requestedPaths(), QStringList{qsl("/stalled.xml")});
        // Refused before it could put a second progress display over the first:
        QCOMPARE(startSpy.count(), 1);
    }

    // A JSON import or export owns the same standalone progress display. Letting
    // a download start on top of it would leave the JSON operation's dialog to be
    // closed by the download's cleanup, and the download's own parse would
    // mapClear() the JSON import out from under itself.
    void test_downloadIsRefusedWhileAJsonOperationOwnsTheProgressDisplay()
    {
        TMap* pMap = mpHost->mpMap.data();
        // A map with rooms in it, so the export the download interrupts is
        // doing real work rather than writing an empty one - the case before
        // this can leave the map cleared:
        pMap->mapClear();
        QVERIFY(pMap->mpRoomDB->addArea(qsl("Exported Area")) > 0);
        QVERIFY(pMap->addRoom(1));
        QVERIFY(pMap->setRoomArea(1, 1));
        QTemporaryDir jsonDir;
        QVERIFY(jsonDir.isValid());
        QSignalSpy exportStartSpy(pMap, &TMap::signal_mapJsonProgressStart);
        QSignalSpy exportCloseSpy(pMap, &TMap::signal_mapProgressClose);

        bool downloadAttempted = false;
        const QMetaObject::Connection reenter = connect(pMap, &TMap::signal_mapJsonProgressStart, pMap, [this, pMap, &downloadAttempted]() {
            downloadAttempted = true;
            pMap->downloadMap(mpMapServer->url(qsl("/map.xml")));
        });
        const auto [wrote, writeMessage] = pMap->writeJsonMapFile(qsl("%1/reentrancy.json").arg(jsonDir.path()));
        disconnect(reenter);

        QVERIFY(downloadAttempted);
        QVERIFY2(wrote, qPrintable(writeMessage));
        QVERIFY2(consoleShows(qsl("already in progress")), qPrintable(consoleTextSinceMark()));
        QVERIFY2(mpMapServer->requestedPaths().isEmpty(), "a download started on top of a JSON export");
        // The export it interrupted still took its own display down, exactly once:
        QCOMPARE(exportStartSpy.count(), 1);
        QCOMPARE(exportCloseSpy.count(), 1);
        QVERIFY(!pMap->hasActiveTransferProgress());
    }

    void test_invalidUrlIsRefusedBeforeAnyRequest()
    {
        TMap* pMap = mpHost->mpMap.data();
        QSignalSpy startSpy(pMap, &TMap::signal_mapTransferProgressStart);

        // QUrl::fromUserInput() falls back to prepending a scheme and takes
        // almost anything, so reaching this guard needs an input that cannot be
        // read as a host either - a stray percent escape is one.
        pMap->downloadMap(qsl("%zz"));

        QVERIFY2(consoleShows(qsl("invalid URL")), qPrintable(consoleTextSinceMark()));
        QCOMPARE(startSpy.count(), 0);
        QVERIFY(mpMapServer->requestedPaths().isEmpty());
        QVERIFY(!pMap->hasActiveTransferProgress());

        // The import flag has to have been cleared on that early exit, or the
        // profile could never download a map again:
        QVERIFY2(runDownload(mpMapServer->url(qsl("/map.xml"))), "the download after an invalid URL was refused, so the import flag was left set");
    }

    // An abort is not a download error, so slot_downloadError() has to stay
    // quiet about it and leave the console with only the alert the cancel posts.
    //
    // QNetworkReply::abort() delivers finished() synchronously, so the whole
    // chain has unwound by the time slot_downloadCancel() returns - no waiting
    // needed. It unwinds through the save-and-parse path rather than the error
    // one, since slot_replyFinished() deliberately excludes a cancel from its
    // error exit: what reaches the destination file is whatever the aborted
    // reply still has to give, which is nothing, and the empty result is then
    // handed to the map reader. So a second, different error line - about the
    // parse - does follow, which is why the assertion below names the download
    // error rather than looking for the absence of errors.
    void test_cancelStopsTheDownloadWithoutReportingAnError()
    {
        TMap* pMap = mpHost->mpMap.data();

        pMap->downloadMap(mpMapServer->url(qsl("/stalled.xml")));
        QVERIFY2(QTest::qWaitFor(
                         [this]() {
                             return !mpMapServer->requestedPaths().isEmpty();
                         },
                         10000),
                 "the download never reached the server");

        pMap->slot_downloadCancel();

        QVERIFY2(consoleShows(qsl("canceled, on user's request")), qPrintable(consoleTextSinceMark()));
        QVERIFY2(!consoleShows(qsl("Map download encountered an error")), qPrintable(consoleTextSinceMark()));
        QVERIFY(!pMap->hasActiveTransferProgress());
        QVERIFY2(mapDownloadEventCountIs(0), "a canceled download raised sysMapDownloadEvent");

        QVERIFY2(runDownload(mpMapServer->url(qsl("/map.xml"))), "the download after a canceled one was refused, so the import flag was left set");
    }

    void test_undownloadableDestinationIsReported()
    {
        TMap* pMap = mpHost->mpMap.data();
        // Inside the profile's own directory but below a path component that is
        // not there, so QSaveFile cannot open it:
        const QString unwritable = qsl("%1/no-such-directory/map.xml").arg(mudlet::getMudletPath(enums::profileHomePath, mProfileName));

        QVERIFY2(runDownload(mpMapServer->url(qsl("/map.xml")), unwritable), "the map download never finished");

        QVERIFY2(consoleShows(qsl("unable to open destination file")), qPrintable(consoleTextSinceMark()));
        QVERIFY2(!QFileInfo::exists(unwritable), "the destination file was written after all");
        QVERIFY(!pMap->hasActiveTransferProgress());
        QVERIFY2(runDownload(mpMapServer->url(qsl("/map.xml"))), "the download after an unwritable destination was refused, so the import flag was left set");
    }

    // What the mapper's "Download from game" does: no URL, so the location the
    // game advertised over MMP is used.
    void test_mmpMapLocationIsUsedWhenNoUrlIsGiven()
    {
        TMap* pMap = mpHost->mpMap.data();
        pMap->setMmpMapLocation(mpMapServer->url(qsl("/mmp.xml")));

        QVERIFY2(runDownload(QString()), "the map download never finished");

        QCOMPARE(mpMapServer->requestedPaths(), QStringList{qsl("/mmp.xml")});
    }

    // The game answers, but with a document that stops part way through - the
    // shape a server cutting the response short produces. The reader has already
    // cleared the map by the time the parse fails, so the least that has to be
    // true is that the failure is reported, no download event goes out, and the
    // next attempt is accepted.
    void test_aTruncatedMapIsReportedAsAParseFailure()
    {
        TMap* pMap = mpHost->mpMap.data();
        mpMapServer->serve(qsl("/truncated.xml"), QByteArrayLiteral("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<map>\n <areas>\n  <area id=\"1\" name=\"Half an"));

        QVERIFY2(runDownload(mpMapServer->url(qsl("/truncated.xml"))), "the map download never finished");

        QVERIFY2(consoleShows(qsl("failure in parsing destination file")), qPrintable(consoleTextSinceMark()));
        QVERIFY2(mapDownloadEventCountIs(0), "a map that failed to parse still raised sysMapDownloadEvent");
        QVERIFY(!pMap->hasActiveTransferProgress());
        QVERIFY2(runDownload(mpMapServer->url(qsl("/map.xml"))), "the download after an unparseable one was refused, so the import flag was left set");
    }

    // A destination file name that does not end in "xml" - which an URL not
    // ending in "xml" is what gives it by default - is a binary map file, and
    // goes to TMainConsole::loadMap() rather than the XML reader.
    //
    // Last on purpose: loadMap() creates the mapper widget, and from then on
    // TMap puts its progress on that widget instead of emitting the standalone
    // signals every test above watches. A profile's mapper cannot be destroyed
    // again, so this cannot be undone within the process.
    void test_downloadedBinaryMapIsLoadedThroughTheConsole()
    {
        TMap* pMap = mpHost->mpMap.data();
        pMap->mapClear();
        QVERIFY(pMap->mpRoomDB->addArea(qsl("Serialized Area")) > 0);
        QVERIFY(pMap->addRoom(7));
        QVERIFY(pMap->setRoomArea(7, 1));
        QVERIFY(pMap->setRoomCoordinates(7, 3, 4, 5));

        QByteArray serialized;
        QDataStream out(&serialized, QIODevice::WriteOnly);
        if (mudlet::scmRunTimeQtVersion >= QVersionNumber(5, 13, 0)) {
            out.setVersion(mudlet::scmQDataStreamFormat_5_12);
        }
        QVERIFY(pMap->serialize(out));
        mpMapServer->serve(qsl("/map.dat"), serialized);

        pMap->mapClear();
        QVERIFY(pMap->mpRoomDB->isEmpty());

        QVERIFY2(runDownload(mpMapServer->url(qsl("/map.dat"))), "the binary map download never finished");

        // A non-XML download is stored under the profile's map directory, not as
        // its map.xml:
        const QString stored = mudlet::getMudletPath(enums::profileMapPathFileName, mProfileName, qsl("map.dat"));
        QVERIFY2(QFileInfo::exists(stored), qPrintable(qsl("the downloaded map was not saved to %1").arg(stored)));

        TRoom* pRoom = pMap->mpRoomDB->getRoom(7);
        QVERIFY2(pRoom, "the binary map was not loaded back through the console");
        QCOMPARE(pRoom->x(), 3);
        QCOMPARE(pRoom->y(), 4);
        QCOMPARE(pRoom->z(), 5);
        QVERIFY2(consoleShows(qsl("map downloaded and stored")), qPrintable(consoleTextSinceMark()));
        QVERIFY2(mapDownloadEventCountIs(1), "the binary path did not raise sysMapDownloadEvent");
    }
};

#include "MapDownloadTest.moc"
MUDLET_GROUPED_TEST_MAIN(MapDownloadTest)
