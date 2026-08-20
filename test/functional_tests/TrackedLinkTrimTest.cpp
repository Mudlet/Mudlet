/***************************************************************************
 *   Copyright (C) 2026 by Mudlet Makers                                   *
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
 * THyperlinkVisibilityManager addresses each concealed OSC 8 hyperlink by line
 * number, so a buffer trim invalidates every entry it holds unless it is told.
 * Revealing a link then writes into whichever line has taken that index - or,
 * where that line is too short to accept it, writes nothing and leaves the link
 * blanked out for good.
 *
 * The manager is driven directly rather than through a fed OSC 8 sequence: a
 * reveal is timer-driven, so an end-to-end version would rest on a delay
 * elapsing before a trim, which is how flaky tests start.
 */

#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest/QtTest>
#include <chrono>

#include "PortableModeTestHelper.h"
#include "ProfileTestHelper.h"
#include "Host.h"
#include "HostManager.h"
#include "MudletInstanceCoordinator.h"
#include "TConsole.h"
#include "THyperlinkVisibilityManager.h"
#include "TMainConsole.h"
#include "TelnetServerStub.h"
#include "ctelnet.h"
#include "mudlet.h"

#include "GroupedTest.h"

using namespace std::chrono_literals;

class TrackedLinkTrimTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    TelnetServerStub* mpServer = nullptr;
    Host* mpHost = nullptr;
    const QString mHostname = qsl("TrackedLinkTrim");
    QString mPort;
    const QString mLocalhost = qsl("localhost");
    const QString mLinkText = qsl("SECRET");
    static constexpr int mLinesLimit = 100;
    static constexpr int mBatchDeleteSize = 20;

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

        mpServer = new TelnetServerStub(qApp);
        mpServer->start(mLocalhost, 0);
        mPort = QString::number(mpServer->serverPort());
        mudlet::start();
        mudlet::self()->setupConfig();
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);

        deleteProfileDirectory(mHostname);

        auto host = TestProfile::create(mHostname, mLocalhost, mPort);
        QVERIFY2(host, "no active host available for the test");
        QSignalSpy connectionSpy(&(host->mTelnet), &cTelnet::signal_connected);
        QVERIFY2(connectionSpy.wait(2000), "could not connect with the host");

        mpHost = mudlet::self()->getHostManager().getHost(mHostname);
        QVERIFY(mpHost);
        QVERIFY(mpHost->mpConsole);
    }

    void cleanupTestCase()
    {
        delete mpServer;
        mpServer = nullptr;
        mpHost = nullptr;
        if (mudlet::self()) {
            deleteProfileDirectory(mHostname);
            delete mudlet::self();
        }
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
    }

    // A link whose line survives the trim has to move with it, so revealing it
    // still writes into its own line.
    void test_revealAfterATrimWritesToTheLinksOwnLine()
    {
        auto* pConsole = mpHost->mpConsole.data();
        auto& buffer = pConsole->buffer;
        auto& manager = pConsole->getHyperlinkVisibilityManager();
        buffer.setBufferSize(mLinesLimit, mBatchDeleteSize);

        fill(pConsole, qsl("seed"), 45);
        pConsole->echo(qsl("MARKERLINE\n"));
        qApp->processEvents();

        const int registeredOn = lineContaining(buffer, qsl("MARKERLINE"));
        QVERIFY2(registeredOn >= 0, "the marker line never reached the buffer");
        QVERIFY2(manager.registerHyperlink(1, registeredOn, 0, mLinkText.length(), mLinkText, concealedRevealStyling()), "the link did not start concealed, so there is nothing to reveal");

        fill(pConsole, qsl("filler"), 60);
        qApp->processEvents();

        const int movedTo = lineContaining(buffer, qsl("MARKERLINE"));
        QVERIFY2(movedTo >= 0 && movedTo != registeredOn, "the buffer did not trim, so this test proves nothing");

        manager.revealLink(1);
        qApp->processEvents();

        QVERIFY2(buffer.lineBuffer.at(movedTo).startsWith(mLinkText), qPrintable(qsl("revealing wrote to some other line: line %1 reads \"%2\"").arg(movedTo).arg(buffer.lineBuffer.at(movedTo))));
        QCOMPARE(lineContaining(buffer, mLinkText), movedTo);
    }

    // A link whose line is trimmed away has to be forgotten, so revealing it
    // cannot write into whichever line has taken its index.
    void test_revealAfterItsLineIsTrimmedAwayWritesNothing()
    {
        auto* pConsole = mpHost->mpConsole.data();
        auto& buffer = pConsole->buffer;
        auto& manager = pConsole->getHyperlinkVisibilityManager();
        buffer.setBufferSize(mLinesLimit, mBatchDeleteSize);

        fill(pConsole, qsl("seed"), 45);
        qApp->processEvents();

        // an early line, well inside the first batch the next trim removes
        const int doomedLine = 3;
        QVERIFY(buffer.lineBuffer.at(doomedLine).length() >= mLinkText.length());
        QVERIFY(manager.registerHyperlink(2, doomedLine, 0, mLinkText.length(), mLinkText, concealedRevealStyling()));

        fill(pConsole, qsl("filler"), 60);
        qApp->processEvents();

        QStringList before;
        for (const auto& line : buffer.lineBuffer) {
            before << line;
        }

        manager.revealLink(2);
        qApp->processEvents();

        for (int i = 0; i < before.size() && i < buffer.lineBuffer.size(); ++i) {
            QVERIFY2(before.at(i) == buffer.lineBuffer.at(i),
                     qPrintable(qsl("revealing a link whose line was trimmed away rewrote line %1: \"%2\" became \"%3\"").arg(i).arg(before.at(i), buffer.lineBuffer.at(i))));
        }
    }

private:
    // A reveal needs a delay or an expire trigger, otherwise registerHyperlink()
    // refuses to conceal it - nothing would ever reveal it again. The delay is
    // long enough that the timer cannot fire while the test runs.
    Mudlet::HyperlinkStyling concealedRevealStyling() const
    {
        Mudlet::HyperlinkStyling styling;
        styling.visibility.hasVisibilitySettings = true;
        styling.visibility.action = Mudlet::HyperlinkStyling::VisibilitySettings::Action::Reveal;
        styling.visibility.isConcealed = true;
        styling.visibility.delayMs = 600000;
        return styling;
    }

    void fill(TConsole* pConsole, const QString& tag, const int lines) const
    {
        for (int i = 0; i < lines; ++i) {
            pConsole->echo(qsl("%1 line %2\n").arg(tag).arg(i));
        }
    }

    int lineContaining(const TBuffer& buffer, const QString& needle) const
    {
        for (int i = 0; i < static_cast<int>(buffer.lineBuffer.size()); ++i) {
            if (buffer.lineBuffer.at(i).contains(needle)) {
                return i;
            }
        }
        return -1;
    }

    void deleteProfileDirectory(const QString& profileName)
    {
        QDir dir(mudlet::getMudletPath(enums::profileHomePath, profileName));
        if (dir.exists()) {
            dir.removeRecursively();
        }
    }
};

#include "TrackedLinkTrimTest.moc"
MUDLET_GROUPED_TEST_MAIN(TrackedLinkTrimTest)
