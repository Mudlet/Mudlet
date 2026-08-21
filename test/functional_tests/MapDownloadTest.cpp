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
 * createTransferProgress() puts progress on that widget when there is a visible
 * one. Most of the tests below deliberately run with no mapper at all, which is
 * the other branch: the standalone progress signals a frontend renders as its
 * own dialog, and the one a hidden-mapper profile takes. Creating a mapper
 * cannot be undone within the process, so the tests that need one all come last.
 * The JSON export and import never consult the mapper either way.
 *
 * It also covers what the frontend does with those signals, which no amount of
 * spying on TMap can: the QProgressDialog TMainConsole raises for a transfer or
 * a JSON operation, its Abort button reaching TMap::slot_mapProgressDialogCancelled,
 * and the two of TMap's signals to dlgMapper that carry an indicator - a map
 * save that failed, and a game that has advertised a map to download.
 *
 * A real profile rather than a bare Host, because the interesting part of the
 * chain is what happens after the bytes land: the reply handler writes the file,
 * parses it, and reports every outcome to the console through Host::postMessage()
 * - and hands a non-XML file to TMainConsole::loadMap(). A Host with no console
 * would stack those messages up unread and crash on that last call. It is also
 * the only way to get the frontend wiring at all: those connects are made in
 * mudlet::addConsoleForNewHost(), so a profile built through HostManager::addHost
 * has none of them.
 *
 * Run with: ctest -R MapDownloadTest -V
 */

#include <QFileInfo>
#include <QFrame>
#include <QHash>
#include <QHostAddress>
#include <QProgressDialog>
#include <QPushButton>
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

    QList<QProgressDialog*> consoleProgressDialogs() const { return mpHost->mpConsole->findChildren<QProgressDialog*>(); }

    // Relies on the map's being the console's only progress dialog: a package
    // download would put a second one there (TMainConsole::showPackageDownloadProgress),
    // and so does a map dialog still awaiting its deleteLater(). Answering
    // "no dialog" to either is what turns that into a visible failure rather
    // than an arbitrary pick.
    QProgressDialog* consoleProgressDialog() const
    {
        const auto dialogs = consoleProgressDialogs();
        return dialogs.size() == 1 ? dialogs.constFirst() : nullptr;
    }

    // closeMapProgressDialog() disposes of the dialog with deleteLater(), and a
    // DeferredDelete posted at the event-loop level we are already at is not
    // delivered by a plain processEvents() - so ask for it by name.
    static void settleDeferredDeletes()
    {
        qApp->processEvents();
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    }

    // What TMainConsole's seven map-progress slots did, read back off the dialog
    // they did it to rather than off the signals that carried it.
    struct ConsoleDialogTrace
    {
        int starts{0};
        int dialogsAtStart{0};
        QString title;
        QString label;
        QString cancelButtonText;
        int maximum{-1};
        int labels{0};
        int ranges{0};
        int values{0};
        int cancelDisables{0};
        bool cancelButtonSurvivedDisable{false};
        int closes{0};
        // An entry per payload the dialog did not end up carrying, so a failure
        // names the step rather than only a count.
        QStringList missed;
    };

    // Attached after the production connects made in
    // mudlet::addConsoleForNewHost(), so Qt runs each of these once
    // TMainConsole's slot has had the payload: what they read off the dialog is
    // what that slot did with it.
    QList<QMetaObject::Connection> observeConsoleProgressDialog(ConsoleDialogTrace& trace) const
    {
        TMap* pMap = mpHost->mpMap.data();
        auto recordStart = [this, &trace](const QString& title, const QString& label, const QString& cancelButtonText) {
            ++trace.starts;
            trace.dialogsAtStart = static_cast<int>(consoleProgressDialogs().count());
            auto* dialog = consoleProgressDialog();
            if (!dialog) {
                trace.missed << qsl("start \"%1\" put no dialog on the console").arg(title);
                return;
            }
            trace.title = dialog->windowTitle();
            trace.label = dialog->labelText();
            trace.maximum = dialog->maximum();
            auto* cancelButton = dialog->findChild<QPushButton*>();
            trace.cancelButtonText = cancelButton ? cancelButton->text() : QString();
            if (trace.label != label) {
                trace.missed << qsl("start \"%1\" label").arg(title);
            }
            if (trace.cancelButtonText != cancelButtonText) {
                trace.missed << qsl("start \"%1\" cancel button text").arg(title);
            }
        };

        return {
                connect(pMap, &TMap::signal_mapTransferProgressStart, pMap, recordStart),
                connect(pMap,
                        &TMap::signal_mapJsonProgressStart,
                        pMap,
                        [recordStart](const QString& title, const QString& label, const QString& cancelButtonText, const int) {
                            recordStart(title, label, cancelButtonText);
                        }),
                connect(pMap,
                        &TMap::signal_mapProgressSetLabel,
                        pMap,
                        [this, &trace](const QString& text) {
                            ++trace.labels;
                            auto* dialog = consoleProgressDialog();
                            if (!dialog || dialog->labelText() != text) {
                                trace.missed << qsl("label %1").arg(trace.labels);
                            }
                        }),
                connect(pMap,
                        &TMap::signal_mapProgressSetRange,
                        pMap,
                        [this, &trace](const int minimum, const int maximum) {
                            ++trace.ranges;
                            auto* dialog = consoleProgressDialog();
                            if (!dialog || dialog->minimum() != minimum || dialog->maximum() != maximum) {
                                trace.missed << qsl("range %1 (%2-%3)").arg(QString::number(trace.ranges), QString::number(minimum), QString::number(maximum));
                            }
                        }),
                connect(pMap,
                        &TMap::signal_mapProgressSetValue,
                        pMap,
                        [this, &trace](const int value) {
                            ++trace.values;
                            auto* dialog = consoleProgressDialog();
                            if (!dialog || dialog->value() != value) {
                                trace.missed << qsl("value %1 (%2)").arg(QString::number(trace.values), QString::number(value));
                            }
                        }),
                connect(pMap,
                        &TMap::signal_mapProgressDisableCancel,
                        pMap,
                        [this, &trace]() {
                            ++trace.cancelDisables;
                            auto* dialog = consoleProgressDialog();
                            if (!dialog) {
                                trace.missed << qsl("cancel disable %1").arg(trace.cancelDisables);
                                return;
                            }
                            trace.cancelButtonSurvivedDisable = dialog->findChild<QPushButton*>();
                        }),
                connect(pMap,
                        &TMap::signal_mapProgressClose,
                        pMap,
                        [&trace]() {
                            ++trace.closes;
                        }),
        };
    }

    static void stopObserving(const QList<QMetaObject::Connection>& observers)
    {
        for (const QMetaObject::Connection& observer : observers) {
            disconnect(observer);
        }
    }

    // The mapper only exists from test_downloadedBinaryMapIsLoadedThroughTheConsole
    // onwards; making one here keeps the tests that need it from depending on
    // that. Guarded because showHideOrCreateMapper() toggles the visibility of a
    // mapper that already exists rather than leaving it alone.
    dlgMapper* ensureMapper() const
    {
        if (mpHost->mpMap->mpMapper.isNull()) {
            mpHost->showHideOrCreateMapper(false);
        }
        return mpHost->mpMap->mpMapper;
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
        // progress path the signals under test belong to, until the tests at the
        // end of this file that deliberately create one.
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
        // Not just processEvents(): a progress dialog left pending deletion
        // would still answer findChildren<QProgressDialog*>() in the next test.
        settleDeferredDeletes();
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

    // Everything above watches TMap's progress signals, which keep firing with
    // no frontend attached at all - TMap::warnIfMapProgressUnwired() exists
    // because that is a state the engine can reach. This one watches the dialog
    // TMainConsole builds out of them, so cutting any of the six connects in
    // mudlet::addConsoleForNewHost() that a download uses shows up here. The
    // counters are not the gate - they all count emissions, and would survive
    // the console being disconnected - they are what stops trace.missed being
    // trivially empty.
    void test_downloadProgressReachesTheConsoleDialog()
    {
        TMap* pMap = mpHost->mpMap.data();
        pMap->mapClear();

        ConsoleDialogTrace trace;
        const QSignalSpy valueSpy(pMap, &TMap::signal_mapProgressSetValue);
        const auto observers = observeConsoleProgressDialog(trace);
        // A paced answer ends only when the test says so, which is what makes
        // the reply report often enough to correct the range from its own total
        // instead of leaving the guess standing as the only range ever set.
        const QMetaObject::Connection pacer = paceUntilReported(valueSpy, 2);
        const bool finished = runDownload(mpMapServer->url(qsl("/paced.xml")));
        disconnect(pacer);
        stopObserving(observers);
        QVERIFY2(finished, "the map download never finished");

        QCOMPARE(trace.starts, 1);
        QCOMPARE(trace.dialogsAtStart, 1);
        QCOMPARE(trace.title, qsl("Map download"));
        QVERIFY2(trace.label.contains(mProfileName), qPrintable(qsl("the dialog's label does not name the profile: \"%1\"").arg(trace.label)));
        QCOMPARE(trace.cancelButtonText, qsl("Abort"));
        // A floor over the whole run, not a split: the download sets the range
        // from the guess and again from the reply, and the parse sets it again
        // per stage.
        QVERIFY2(trace.ranges >= 2, qPrintable(qsl("the range was set %1 times, too few for both the guess and the reply's own size").arg(trace.ranges)));
        QVERIFY(trace.values > 0);
        // Only the parse that follows the download asks for a relabel:
        QVERIFY2(trace.labels > 0, "no relabel was ever asked for, so nothing checked one");
        QCOMPARE(trace.cancelDisables, 1);
        QVERIFY2(!trace.cancelButtonSurvivedDisable, "the Abort button was still on the dialog once the download could no longer be stopped");
        QCOMPARE(trace.closes, 1);
        QVERIFY2(trace.missed.isEmpty(), qPrintable(qsl("the dialog did not carry: %1").arg(trace.missed.join(qsl(", ")))));

        settleDeferredDeletes();
        QVERIFY2(consoleProgressDialogs().isEmpty(), "the download's progress dialog outlived the download");
    }

    // signal_mapJsonProgressStart is the one of the seven the download path never
    // emits, and the only start signal carrying a maximum.
    void test_jsonExportAndImportProgressReachTheConsoleDialog()
    {
        TMap* pMap = mpHost->mpMap.data();
        pMap->mapClear();
        QVERIFY(pMap->mpRoomDB->addArea(qsl("Exported Area")) > 0);
        for (const int roomId : {1, 2, 3}) {
            QVERIFY(pMap->addRoom(roomId));
            QVERIFY(pMap->setRoomArea(roomId, 1));
        }
        QTemporaryDir jsonDir;
        QVERIFY(jsonDir.isValid());
        const QString file = qsl("%1/progress.json").arg(jsonDir.path());

        ConsoleDialogTrace exportTrace;
        auto observers = observeConsoleProgressDialog(exportTrace);
        const auto [wrote, writeMessage] = pMap->writeJsonMapFile(file);
        stopObserving(observers);
        QVERIFY2(wrote, qPrintable(writeMessage));

        QCOMPARE(exportTrace.starts, 1);
        QCOMPARE(exportTrace.dialogsAtStart, 1);
        QCOMPARE(exportTrace.title, qsl("Map JSON export"));
        QVERIFY2(exportTrace.label.contains(mProfileName), qPrintable(qsl("the dialog's label does not name the profile: \"%1\"").arg(exportTrace.label)));
        QCOMPARE(exportTrace.cancelButtonText, qsl("Abort"));
        QCOMPARE(exportTrace.maximum, 3);
        QVERIFY(exportTrace.labels > 0);
        QVERIFY(exportTrace.values > 0);
        QCOMPARE(exportTrace.cancelDisables, 1);
        QVERIFY2(!exportTrace.cancelButtonSurvivedDisable, "the Abort button was still on the dialog once the export could no longer be stopped");
        QCOMPARE(exportTrace.closes, 1);
        QVERIFY2(exportTrace.missed.isEmpty(), qPrintable(qsl("the dialog did not carry: %1").arg(exportTrace.missed.join(qsl(", ")))));
        settleDeferredDeletes();
        QVERIFY2(consoleProgressDialogs().isEmpty(), "the JSON export's progress dialog outlived the export");

        ConsoleDialogTrace importTrace;
        observers = observeConsoleProgressDialog(importTrace);
        const auto [read, readMessage] = pMap->readJsonMapFile(file);
        stopObserving(observers);
        QVERIFY2(read, qPrintable(readMessage));

        QCOMPARE(importTrace.starts, 1);
        QCOMPARE(importTrace.dialogsAtStart, 1);
        QCOMPARE(importTrace.title, qsl("Map JSON import"));
        QCOMPARE(importTrace.maximum, 3);
        QVERIFY(importTrace.labels > 0);
        QVERIFY(importTrace.values > 0);
        QCOMPARE(importTrace.closes, 1);
        QVERIFY2(importTrace.missed.isEmpty(), qPrintable(qsl("the dialog did not carry: %1").arg(importTrace.missed.join(qsl(", ")))));
        settleDeferredDeletes();
        QVERIFY2(consoleProgressDialogs().isEmpty(), "the JSON import's progress dialog outlived the import");
    }

    // The back edge: the dialog's Abort button reaches
    // TMap::slot_mapProgressDialogCancelled through the connect() in
    // TMainConsole::createMapProgressDialog(). MapProgressDialogSeamTest calls
    // that slot from a lambda of its own, which proves the slot and substitutes
    // the wire; this presses the button the user presses.
    //
    // click() rather than QTest::mouseClick(): showMapJsonProgress() leaves the
    // dialog to show itself off its own minimumDuration timer, so at the moment
    // of the press there is no laid-out geometry for a synthetic one to land in.
    // QProgressDialog::cancel() is no use either - it is the slot canceled()
    // calls, not a way to emit it.
    void test_theDialogsAbortButtonCancelsAJsonImport()
    {
        TMap* pMap = mpHost->mpMap.data();
        pMap->mapClear();
        QVERIFY(pMap->mpRoomDB->addArea(qsl("Abortable Area")) > 0);
        QVERIFY(pMap->addRoom(1));
        QVERIFY(pMap->setRoomArea(1, 1));
        QTemporaryDir jsonDir;
        QVERIFY(jsonDir.isValid());
        const QString file = qsl("%1/abort.json").arg(jsonDir.path());
        const auto [wrote, writeMessage] = pMap->writeJsonMapFile(file);
        QVERIFY2(wrote, qPrintable(writeMessage));
        // Or the export's dialog is still awaiting deletion when the import
        // raises its own, and there are two of them to choose between.
        settleDeferredDeletes();

        bool pressed = false;
        const QMetaObject::Connection abortOnStart = connect(pMap, &TMap::signal_mapJsonProgressStart, pMap, [this, &pressed]() {
            auto* dialog = consoleProgressDialog();
            auto* cancelButton = dialog ? dialog->findChild<QPushButton*>() : nullptr;
            if (!cancelButton) {
                return;
            }
            pressed = true;
            cancelButton->click();
        });
        const auto [read, readMessage] = pMap->readJsonMapFile(file);
        disconnect(abortOnStart);

        QVERIFY2(pressed, "the JSON import's progress dialog offered no Abort button to press");
        QVERIFY2(!read, "pressing Abort on the map progress dialog did not stop the JSON import");
        QCOMPARE(readMessage, qsl("aborted by user"));
        QVERIFY(!pMap->hasActiveTransferProgress());
    }

    // The other half of that back edge. disableMapProgressDialogCancel() drops
    // the connection as well as the Abort button, because QProgressDialog goes
    // on emitting canceled() from Escape and from a window close whether it has
    // a button or not. Leave the connection behind and a download that is past
    // the point of no return still reports itself canceled and takes its own
    // progress display down mid-parse.
    void test_theDialogsAbortIsDroppedOnceADownloadCannotBeStopped()
    {
        TMap* pMap = mpHost->mpMap.data();
        pMap->mapClear();

        bool closed = false;
        const QMetaObject::Connection closeOnDisable = connect(pMap, &TMap::signal_mapProgressDisableCancel, pMap, [this, &closed]() {
            auto* dialog = consoleProgressDialog();
            if (!dialog) {
                return;
            }
            closed = true;
            dialog->close();
        });
        const bool finished = runDownload(mpMapServer->url(qsl("/map.xml")));
        disconnect(closeOnDisable);
        QVERIFY2(finished, "the map download never finished");

        QVERIFY2(closed, "there was no progress dialog left to close once the download could no longer be stopped");
        QVERIFY2(!consoleShows(qsl("canceled, on user's request")), qPrintable(consoleTextSinceMark()));
        QVERIFY2(mapDownloadEventCountIs(1), "closing the progress dialog after the point of no return lost the downloaded map");
    }

    void test_theDialogsAbortButtonCancelsAMapDownload()
    {
        TMap* pMap = mpHost->mpMap.data();

        pMap->downloadMap(mpMapServer->url(qsl("/stalled.xml")));
        QVERIFY2(QTest::qWaitFor(
                         [this]() {
                             return !mpMapServer->requestedPaths().isEmpty();
                         },
                         10000),
                 "the download never reached the server");

        auto* dialog = consoleProgressDialog();
        QVERIFY2(dialog, "the map download put no progress dialog on the console");
        auto* cancelButton = dialog->findChild<QPushButton*>();
        QVERIFY2(cancelButton, "the map download's progress dialog offered no Abort button to press");
        cancelButton->click();

        QVERIFY2(consoleShows(qsl("canceled, on user's request")), qPrintable(consoleTextSinceMark()));
        QVERIFY(!pMap->hasActiveTransferProgress());
        QVERIFY2(mapDownloadEventCountIs(0), "a download aborted from the dialog raised sysMapDownloadEvent");

        QVERIFY2(runDownload(mpMapServer->url(qsl("/map.xml"))), "the download after an aborted one was refused, so the import flag was left set");
    }

    // A destination file name that does not end in "xml" - which an URL not
    // ending in "xml" is what gives it by default - is a binary map file, and
    // goes to TMainConsole::loadMap() rather than the XML reader.
    //
    // After every standalone-progress test on purpose: loadMap() creates the
    // mapper widget, and from then on TMap puts its progress on that widget
    // instead of emitting the signals those tests watch. A profile's mapper
    // cannot be destroyed again, so this cannot be undone within the process -
    // which is what the two mapper tests below rely on.
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

    // The mapper's map-save-failed indicator, which nothing but
    // TMap::signal_saveErrorChanged ever raises. Cut that and the map autosave -
    // which reports failure through this indicator and nothing else - fails in
    // silence.
    //
    // isHidden() rather than isVisible(): it reads exactly the flag
    // slot_saveErrorChanged sets, so the assertion cannot start depending on
    // whether the window hierarchy above the button happens to be shown.
    void test_aFailedMapSaveRaisesTheMapperWarningIndicator()
    {
        dlgMapper* pMapper = ensureMapper();
        QVERIFY2(pMapper, "the profile has no mapper widget to raise the warning on");
        TMap* pMap = mpHost->mpMap.data();
        pMap->setSaveError(false);
        QVERIFY2(pMapper->toolButton_saveWarning->isHidden(), "the map-save warning was already up before a save had failed");

        QTemporaryDir saveDir;
        QVERIFY(saveDir.isValid());
        const QString destination = qsl("%1/map.dat").arg(saveDir.path());
        // A format version this Mudlet cannot write, which is what Lua's
        // saveMap(fileName, version) hands straight through. A destination that
        // cannot be opened is no use for this: TMainConsole::saveMap() returns
        // from that one before it ever reaches the flag.
        QVERIFY2(!mpHost->mpConsole->saveMap(destination, 9999), "the map save was supposed to fail");
        QVERIFY2(!QFileInfo::exists(destination), "the failed save wrote a map file anyway");
        QVERIFY(pMap->hasSaveError());
        QVERIFY2(!pMapper->toolButton_saveWarning->isHidden(), "a failed map save left the mapper's warning indicator down");

        QVERIFY2(mpHost->mpConsole->saveMap(destination), "the map save with a supported format version failed");
        QVERIFY(!pMap->hasSaveError());
        QVERIFY2(pMapper->toolButton_saveWarning->isHidden(), "a successful map save left the mapper's warning indicator up");
    }

    // The mapper's empty-state "Download from game" button is the only thing an
    // MMP map location moves while the user is looking at it - the map context
    // menu and the preferences' Mapper tab read the location when they are
    // built, so they are not on this signal at all.
    void test_anMmpMapLocationOffersTheMapperItsDownloadButton()
    {
        dlgMapper* pMapper = ensureMapper();
        QVERIFY2(pMapper, "the profile has no mapper widget to show the empty state on");
        TMap* pMap = mpHost->mpMap.data();
        pMap->setMmpMapLocation(QString());

        auto* overlay = pMapper->findChild<QFrame*>(qsl("emptyStateOverlay"));
        QVERIFY2(overlay, "the mapper has no empty-state overlay");
        // The overlay's buttons carry no object names, so the text is the only
        // handle on the right one - through translate() rather than a literal,
        // so a run with a translator loaded still finds it.
        QPushButton* downloadButton = nullptr;
        const auto buttons = overlay->findChildren<QPushButton*>();
        for (QPushButton* button : buttons) {
            if (button->text() == QCoreApplication::translate("dlgMapper", "Download from game")) {
                downloadButton = button;
                break;
            }
        }
        QVERIFY2(downloadButton, "the mapper's empty state has no \"Download from game\" button");
        QVERIFY2(downloadButton->isHidden(), "the download button was offered before the game had advertised a map");

        pMap->setMmpMapLocation(mpMapServer->url(qsl("/mmp.xml")));
        QVERIFY2(!downloadButton->isHidden(), "an advertised MMP map location never reached the mapper's empty state");

        pMap->setMmpMapLocation(QString());
        QVERIFY2(downloadButton->isHidden(), "withdrawing the MMP map location never reached the mapper's empty state");
    }
};

#include "MapDownloadTest.moc"
MUDLET_GROUPED_TEST_MAIN(MapDownloadTest)
