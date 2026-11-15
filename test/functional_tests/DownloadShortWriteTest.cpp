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
 * Downloads that the disk cannot hold whole: a script's downloadFile(), media a
 * game or script asks for, and the package a game offers over GMCP Client.GUI.
 * A full disk cuts a write short without setting an error on the file, so each
 * is run under a file size limit, which does the same.
 *
 * Run with: ctest -R DownloadShortWriteTest -V
 */

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QHostAddress>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTemporaryDir>
#include <QtTest/QtTest>

#include <algorithm>
#include <functional>

#include "Host.h"
#include "MudletApp.h"
#include "MudletInstanceCoordinator.h"
#include "PortableModeTestHelper.h"
#include "ProfileTestHelper.h"
#include "TLuaInterpreter.h"
#include "TMainConsole.h"
#include "TelnetServerStub.h"
#include "mudlet.h"

#include "GroupedTest.h"

#if defined(Q_OS_UNIX)
#include <csignal>
#include <sys/resource.h>
#endif

using namespace std::chrono_literals;

class DownloadShortWriteTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    TelnetServerStub* mpGame = nullptr;
    QTcpServer mWebServer;
    QByteArray mServed;
    Host* mpHost = nullptr;
    int mConsoleMark = 0;
    const QString mProfileName = qsl("DownloadShortWrite-Test");
    const QString mLocalhost = qsl("localhost");
    // Raised for a write that came up short, where a failed fetch gives its own reason
    const QString mShortWriteDetail = qsl("Couldn't write downloaded content into the destination file");
    // Past QFileDevice's 16 KiB write buffer, as a write that fits in it fails at commit() instead
    static constexpr qint64 scmFileSizeLimit = 20 * 1024;

    QString url(const QString& path) const { return qsl("http://127.0.0.1:%1/%2").arg(mWebServer.serverPort()).arg(path); }
    bool lua(const QString& code) const { return mpHost->getLuaInterpreter()->compileAndExecuteScript(code); }
    QString luaString(const QString& name) const { return mpHost->getLuaInterpreter()->getLuaString(name); }

    // As one line, since the console wraps long messages
    QString consoleSinceMark() const
    {
        QStringList lines;
        auto& buffer = mpHost->mainConsoleView()->buffer;
        for (int line = std::max(0, mConsoleMark); line <= buffer.getLastLineNumber(); ++line) {
            lines << buffer.line(line);
        }
        return lines.join(QChar::Space).simplified();
    }

#if defined(Q_OS_UNIX)
    // Started and waited on with the limit in place, so the write lands while it holds
    bool underFileSizeLimit(const std::function<bool()>& start, const std::function<bool()>& finished)
    {
        rlimit saved{};
        if (::getrlimit(RLIMIT_FSIZE, &saved) != 0) {
            return false;
        }
        rlimit limited = saved;
        limited.rlim_cur = std::min<rlim_t>(scmFileSizeLimit, saved.rlim_max);
        // Past the limit the kernel also raises SIGXFSZ, whose default action ends the process
        const auto previousHandler = std::signal(SIGXFSZ, SIG_IGN);
        bool ended = false;
        if (::setrlimit(RLIMIT_FSIZE, &limited) == 0) {
            ended = start() && QTest::qWaitFor(finished, 10s);
            ended = ::setrlimit(RLIMIT_FSIZE, &saved) == 0 && ended;
        }
        std::signal(SIGXFSZ, previousHandler);
        return ended;
    }
#endif

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

        QVERIFY(mWebServer.listen(QHostAddress::LocalHost));
        connect(&mWebServer, &QTcpServer::newConnection, this, [this]() {
            while (QTcpSocket* pClient = mWebServer.nextPendingConnection()) {
                connect(pClient, &QTcpSocket::readyRead, pClient, [this, pClient]() {
                    if (pClient->readAll().contains("\r\n\r\n")) {
                        pClient->write("HTTP/1.1 200 OK\r\nContent-Type: application/octet-stream\r\nContent-Length: " + QByteArray::number(mServed.size()) + "\r\nConnection: close\r\n\r\n"
                                       + mServed);
                        pClient->disconnectFromHost();
                    }
                });
                connect(pClient, &QTcpSocket::disconnected, pClient, &QObject::deleteLater);
            }
        });

        mpGame = new TelnetServerStub(qApp);
        mpGame->start(mLocalhost, 0);
        QVERIFY(mpGame->serverPort() != 0);
        mudlet::start();
        mudlet::self()->setupConfig();
        QCOMPARE(MudletApp::getMudletPath(enums::mainPath), qsl("%1/mudlet").arg(mConfigDir.path()));
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>(qsl("MudletInstanceCoordinator")));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);
        mpHost = TestProfile::create(mProfileName, mLocalhost, QString::number(mpGame->serverPort()));
        QVERIFY2(mpHost, "the profile could not be created");
        QVERIFY(QTest::qWaitFor(
                [this]() {
                    return mpGame->clientConnected();
                },
                5s));
    }

    void cleanupTestCase()
    {
        mpHost = nullptr;
        delete mpGame;
        mpGame = nullptr;
        if (mudlet::self()) {
            TestProfile::removeProfileDirectory(mProfileName);
            delete mudlet::self();
        }
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
    }

    void test_aDownloadFileTheDiskCannotHoldIsAnError()
    {
#if !defined(Q_OS_UNIX)
        QSKIP("needs RLIMIT_FSIZE to cut a write short");
#else
        mServed = QByteArray(scmFileSizeLimit * 4, 'x');
        QTemporaryDir dir;
        const QString target = qsl("%1/downloaded.bin").arg(dir.path());
        QVERIFY(lua(qsl("downloadOutcome = nil\n"
                        "local target = [[%1]]\n"
                        "registerAnonymousEventHandler('sysDownloadDone', function(_, file, size) if file == target then downloadOutcome = 'done ' .. tostring(size) end end, true)\n"
                        "registerAnonymousEventHandler('sysDownloadError', function(_, reason, file, detail) if file == target then downloadOutcome = 'error ' .. tostring(detail) end end, true)")
                            .arg(target)));

        QVERIFY2(underFileSizeLimit(
                         [&]() {
                             return lua(qsl("downloadFile([[%1]], [[%2]])").arg(target, url(qsl("downloaded.bin"))));
                         },
                         [this]() {
                             return !luaString(qsl("downloadOutcome")).isEmpty();
                         }),
                 "the download never finished");

        QCOMPARE(luaString(qsl("downloadOutcome")), qsl("error %1").arg(mShortWriteDetail));
        QVERIFY2(!QFileInfo::exists(target), "the part that landed was put in place");
#endif
    }

    void test_mediaTheDiskCannotHoldIsAnError()
    {
#if !defined(Q_OS_UNIX)
        QSKIP("needs RLIMIT_FSIZE to cut a write short");
#else
        mServed = QByteArray(scmFileSizeLimit * 4, 'x');
        const QString name = qsl("short-write.wav");
        const QString target = qsl("%1/%2").arg(MudletApp::getMudletPath(enums::profileMediaPath, mProfileName), name);
        QFile::remove(target);
        QVERIFY(lua(qsl("mediaOutcome = nil\n"
                        "local target = [[%1]]\n"
                        "registerAnonymousEventHandler('sysDownloadDone', function(_, file, size) if file == target then mediaOutcome = 'done ' .. tostring(size) end end, true)\n"
                        "registerAnonymousEventHandler('sysDownloadError', function(_, reason, file, detail) if file == target then mediaOutcome = 'error ' .. tostring(detail) end end, true)")
                            .arg(target)));

        QVERIFY2(underFileSizeLimit(
                         [&]() {
                             return lua(qsl("loadSoundFile({name = [[%1]], url = [[%2]]})").arg(name, url(QString())));
                         },
                         [this]() {
                             return !luaString(qsl("mediaOutcome")).isEmpty();
                         }),
                 "the media download never finished");

        QCOMPARE(luaString(qsl("mediaOutcome")), qsl("error %1").arg(mShortWriteDetail));
        QVERIFY2(!QFileInfo::exists(target), "the part that landed was put in place");
#endif
    }

    void test_aGamesPackageTheDiskCannotHoldIsNotInstalled()
    {
#if !defined(Q_OS_UNIX)
        QSKIP("needs RLIMIT_FSIZE to cut a write short");
#else
        QFile package(qsl(":/packages/echo/echo.mpackage"));
        QVERIFY(package.open(QIODevice::ReadOnly));
        mServed = package.readAll();
        QVERIFY(mServed.size() > scmFileSizeLimit);
        mpHost->mEnableGMCP = true;
        mpHost->mAcceptServerGUI = true;
        mpGame->sendRaw(QByteArray("\xff\xfb\xc9"));
        QVERIFY(QTest::qWaitFor(
                [this]() {
                    return mpHost->mTelnet.isGMCPEnabled();
                },
                5s));
        // Not a package the profile has, as replacing one saves the profile while the limit holds
        const QString saved = MudletApp::getMudletPath(enums::profileDataItemPath, mProfileName, qsl("short-write.mpackage"));
        QFile::remove(saved);
        mConsoleMark = mpHost->mainConsoleView()->buffer.getLastLineNumber();
        const QByteArray offer = qsl("Client.GUI {\"version\": \"7\", \"url\": \"%1\"}").arg(url(qsl("short-write.mpackage"))).toUtf8();

        QVERIFY2(underFileSizeLimit(
                         [&]() {
                             mpGame->sendRaw(QByteArray("\xff\xfa\xc9") + offer + QByteArray("\xff\xf0"));
                             return true;
                         },
                         [this]() {
                             return consoleSinceMark().contains(qsl("Package download failed")) || consoleSinceMark().contains(qsl("Package install failed"));
                         }),
                 "the package download never finished");

        const QString reason = qsl("could not save file, reason: only %1 of %2 bytes could be written").arg(scmFileSizeLimit).arg(mServed.size());
        QVERIFY2(consoleSinceMark().contains(reason), qPrintable(consoleSinceMark()));
        QVERIFY2(mpHost->mServerGUI_Package_version != qsl("7"), "the package was recorded as installed");
        QVERIFY2(!QFileInfo::exists(saved), "the part that landed was put in place");
#endif
    }
};

#include "DownloadShortWriteTest.moc"
MUDLET_GROUPED_TEST_MAIN(DownloadShortWriteTest)
