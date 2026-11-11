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
 * A second Mudlet hands a link or package to the running one over a local
 * socket. Windows pipes have no buffer, so the write only completes once the
 * running instance's event loop reads it. The running instance is played here
 * by a server whose thread is busy for a while, so this only bites on Windows.
 *
 * Run with: ctest -R InstanceForwardTest -V
 */

#include <QtTest/QtTest>

#include "GroupedTest.h"
#include "MudletInstanceCoordinator.h"

#include <QLocalServer>
#include <QLocalSocket>
#include <chrono>
#include <thread>

using namespace std::chrono_literals;

class InstanceForwardTest : public QObject
{
    Q_OBJECT

private:
    static QString uniqueServerName() { return qsl("MudletInstanceForwardTest-%1-%2").arg(QCoreApplication::applicationPid()).arg(QRandomGenerator::global()->generate()); }

    // Busy without reading, as a running Mudlet loading a profile is, then collects what the client sent
    static QByteArray receiveAfterBusyPeriod(QLocalServer& server)
    {
        std::this_thread::sleep_for(1500ms);
        QByteArray received;
        QLocalSocket* connection = nullptr;
        QElapsedTimer timer;
        timer.start();
        while (timer.elapsed() < 10000 && !received.endsWith('\n')) {
            QCoreApplication::processEvents(QEventLoop::AllEvents, 20);
            if (!connection) {
                connection = server.nextPendingConnection();
            }
            if (connection) {
                connection->waitForReadyRead(20);
                received += connection->readAll();
            }
        }
        return received;
    }

private slots:
    void telnetUriSurvivesBusyRunningInstance()
    {
        const QString name = uniqueServerName();
        QLocalServer server;
        QVERIFY(server.listen(name));

        MudletInstanceCoordinator coordinator(name);
        coordinator.queueTelnetUri(qsl("telnet://localhost:23"));

        bool forwarded = false;
        std::thread client([&coordinator, &forwarded] {
            forwarded = coordinator.forwardTelnetUriToRunningInstance();
        });
        const QByteArray received = receiveAfterBusyPeriod(server);
        client.join();

        QCOMPARE(received, QByteArrayLiteral("TELNET_URI:telnet://localhost:23\n"));
        QVERIFY2(forwarded, "the link reached the running instance, yet a second instance would have started too");
    }

    void packagesSurviveBusyRunningInstance()
    {
        const QString name = uniqueServerName();
        QLocalServer server;
        QVERIFY(server.listen(name));

        MudletInstanceCoordinator coordinator(name);
        coordinator.queuePackage(qsl("/tmp/example.mpackage"));

        bool installed = false;
        std::thread client([&coordinator, &installed] {
            installed = coordinator.installPackagesRemotely();
        });
        const QByteArray received = receiveAfterBusyPeriod(server);
        client.join();

        QCOMPARE(received, QByteArrayLiteral("/tmp/example.mpackage\n"));
        QVERIFY2(installed, "the package reached the running instance, yet a second instance would have started too");
    }
};

#include "InstanceForwardTest.moc"
MUDLET_GROUPED_TEST_MAIN(InstanceForwardTest)
