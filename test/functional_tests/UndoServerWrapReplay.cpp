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

// Manual verification tool, not part of the ctest suite: replays a captured
// game login stream (JSONL of {"t": seconds, "b64": chunk}) through the real
// telnet/TBuffer pipeline and dumps the resulting buffer lines, so that the
// mUndoServerWrap behaviour can be compared off vs on against real games.
//
// Environment:
//   REPLAY_CAPTURE - path to the capture file (required)
//   REPLAY_OUT     - path to write buffer dump to (required)
//   REPLAY_UNWRAP  - "1" to enable mUndoServerWrap (default off)
//   REPLAY_WIDTH   - wrap column to undo (default 80)
//   REPLAY_PORT    - local stub port (default: an ephemeral OS-assigned one)

#include <QFileInfo>
#include <QTemporaryDir>
#include <QtTest/QtTest>

#include <QJsonDocument>
#include <QJsonObject>

#include "ProfileTestHelper.h"
#include "Host.h"
#include "MudletInstanceCoordinator.h"
#include "TelnetServerStub.h"
#include "ctelnet.h"
#include "dlgConnectionProfiles.h"
#include "mudlet.h"

extern void qInitResources_mudlet();
extern void qInitResources_qm();
extern void qInitResources_additional_splash_screens();
extern void qInitResources_mudlet_fonts_common();
extern void qInitResources_mudlet_fonts_posix();
void initializeQRCResources();

class UndoServerWrapReplay : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    TelnetServerStub* mpServer = nullptr;
    QString mProfileName;
    QString mPort;
    const QString mpLocalhost = "localhost";

    // setupConfig() consults portable.txt before the XDG logic
    static bool portableMarkerPresent()
    {
        return QFileInfo::exists(qsl("%1/portable.txt").arg(QCoreApplication::applicationDirPath())) || QFileInfo::exists(qsl("%1/.config/mudlet/portable.txt").arg(QDir::homePath()));
    }

private slots:
    void initTestCase()
    {
        initializeQRCResources();

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
    }

    void cleanupTestCase() { mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg); }

    void init()
    {
        // An unset or unparseable REPLAY_PORT gives 0, which the stub takes as
        // "any free port", so concurrent replays share neither socket nor profile.
        mpServer = new TelnetServerStub(qApp);
        mpServer->start(mpLocalhost, qEnvironmentVariable("REPLAY_PORT").toUShort());
        mPort = QString::number(mpServer->serverPort());
        mProfileName = qsl("Replay-%1").arg(mPort);
        mudlet::start();
        mudlet::self()->setupConfig();
        QCOMPARE(mudlet::getMudletPath(enums::mainPath), qsl("%1/mudlet").arg(mConfigDir.path()));
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);
        deleteProfileDirectory(mProfileName);
    }

    void test_replay()
    {
        const QString capturePath = qEnvironmentVariable("REPLAY_CAPTURE");
        const QString outPath = qEnvironmentVariable("REPLAY_OUT");
        if (capturePath.isEmpty() || outPath.isEmpty()) {
            QSKIP("REPLAY_CAPTURE/REPLAY_OUT not set - this is a manual tool");
        }
        QFile captureFile(capturePath);
        QVERIFY2(captureFile.open(QIODevice::ReadOnly), "cannot open capture file");
        struct Chunk
        {
            double at;
            QByteArray data;
        };
        QList<Chunk> chunks;
        while (!captureFile.atEnd()) {
            const QByteArray line = captureFile.readLine().trimmed();
            if (line.isEmpty()) {
                continue;
            }
            const QJsonObject object = QJsonDocument::fromJson(line).object();
            chunks.append({object.value(qsl("t")).toDouble(), QByteArray::fromBase64(object.value(qsl("b64")).toString().toLatin1())});
        }
        QVERIFY2(!chunks.isEmpty(), "capture file holds no chunks");

        startProfile(mProfileName, mpLocalhost, mPort);
        auto host = mudlet::self()->getActiveHost();
        QVERIFY(host);
        if (qEnvironmentVariable("REPLAY_UNWRAP") == qsl("1")) {
            host->mUndoServerWrap = true;
            host->mUndoServerWrapWidth = qEnvironmentVariable("REPLAY_WIDTH", qsl("80")).toInt();
        }
        // Keep display wrapping out of the comparison - only logical lines
        // are of interest:
        host->mWrapAt = 1000;
        host->mpConsole->buffer.mWrapAt = 1000;
        QVERIFY(QTest::qWaitFor(
                [&]() {
                    return mpServer->clientConnected();
                },
                3000));

        double previous = chunks.first().at;
        for (const auto& chunk : chunks) {
            const int gap = qBound(0, static_cast<int>((chunk.at - previous) * 1000), 500);
            if (gap > 15) {
                QTest::qWait(gap);
            }
            previous = chunk.at;
            mpServer->sendRaw(chunk.data);
        }
        // Give held lines, posting timers and the flush timer time to settle:
        QTest::qWait(900);

        QFile out(outPath);
        QVERIFY2(out.open(QIODevice::WriteOnly | QIODevice::Truncate), "cannot open output file");
        auto console = host->mpConsole;
        for (int i = 0; i <= console->buffer.getLastLineNumber(); ++i) {
            const bool prompt = (i < console->buffer.promptBuffer.size()) && console->buffer.promptBuffer.at(i);
            out.write(prompt ? "P\t" : ".\t");
            out.write(console->buffer.line(i).toUtf8());
            out.write("\n");
        }
        out.close();
    }

    void cleanup()
    {
        delete mpServer;
        mpServer = nullptr;
        deleteProfileDirectory(mProfileName);
        delete mudlet::self();
    }

private:
    void startProfile(const QString& hostname, const QString& address, const QString& port)
    {
        auto host = TestProfile::create(hostname, address, port);
        if (!host) {
            QFAIL("No active host available.");
        }
        QSignalSpy spy2(&(host->mTelnet), &cTelnet::signal_connected);
        if (!spy2.wait(2000)) {
            QFAIL("Could not connect with the host.");
        }
    }

    void deleteProfileDirectory(const QString& profileName)
    {
        const QString path = mudlet::getMudletPath(enums::profileHomePath, profileName);
        QDir dir(path);
        if (dir.exists()) {
            dir.removeRecursively();
        }
    }
};

void initializeQRCResources()
{
#ifdef INCLUDE_VARIABLE_SPLASH_SCREEN
    qInitResources_additional_splash_screens();
#endif
#ifdef INCLUDE_FONTS
    qInitResources_mudlet_fonts_common();
#if defined(Q_OS_LINUX) || defined(Q_OS_FREEBSD)
    qInitResources_mudlet_fonts_posix();
#endif
#endif
    qInitResources_mudlet();
    qInitResources_qm();
}

#include "UndoServerWrapReplay.moc"
QTEST_MAIN(UndoServerWrapReplay)
