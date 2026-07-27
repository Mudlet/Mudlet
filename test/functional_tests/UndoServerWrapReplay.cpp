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
//   REPLAY_PORT    - local stub port (default 4010)

#include <QtTest/QtTest>

#include <QJsonDocument>
#include <QJsonObject>

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
    TelnetServerStub* mpServer = nullptr;
    QString mProfileName;
    QString mPort;
    const QString mpLocalhost = "localhost";

private slots:
    void initTestCase() { initializeQRCResources(); }

    void init()
    {
        mPort = qEnvironmentVariable("REPLAY_PORT", qsl("4010"));
        mProfileName = qsl("Replay-%1").arg(mPort);
        mpServer = new TelnetServerStub(qApp);
        mpServer->start(mpLocalhost, mPort.toUShort());
        mudlet::start();
        mudlet::self()->setupConfig();
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
        QTimer::singleShot(0, qApp, [hostname, address, port]() {
            mudlet::self()->startAutoLogin({});
            QTest::qWait(100);
            QTest::mouseClick(mudlet::self()->mpConnectionDialog->new_profile_button, Qt::LeftButton);
            QTest::qWait(100);
            QTest::keyClicks(QApplication::focusWidget(), hostname);
            QTest::qWait(100);
            QTest::keyClick(QApplication::focusWidget(), Qt::Key_Tab);
            QTest::qWait(100);
            QTest::keyClicks(QApplication::focusWidget(), address);
            QTest::qWait(100);
            QTest::keyClick(QApplication::focusWidget(), Qt::Key_Tab);
            QTest::qWait(100);
            QTest::keyClicks(QApplication::focusWidget(), port);
            QTest::qWait(100);
            QTest::keyClick(QApplication::focusWidget(), Qt::Key_Return);
        });

        QSignalSpy spy(mudlet::self(), &mudlet::signal_profileLoaded);
        if (!spy.wait(5000)) {
            QFAIL("Profile took too long to load.");
        }
        auto host = mudlet::self()->getActiveHost();
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
