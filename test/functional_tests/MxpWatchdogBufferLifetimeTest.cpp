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

#include <QElapsedTimer>
#include <QSignalSpy>
#include <QtTest/QtTest>
#include <chrono>
#include <string>

#include "ProfileTestHelper.h"
#include "Host.h"
#include "MudletInstanceCoordinator.h"
#include "TBuffer.h"
#include "TMainConsole.h"
#include "TMxpProcessor.h"
#include "TelnetServerStub.h"
#include "ctelnet.h"
#include "dlgConnectionProfiles.h"
#include "mudlet.h"

using namespace std::chrono_literals;

extern void qInitResources_mudlet();
extern void qInitResources_qm();
extern void qInitResources_additional_splash_screens();
extern void qInitResources_mudlet_fonts_common();
extern void qInitResources_mudlet_fonts_posix();
void initializeQRCResourcesForMxpWatchdogBufferLifetimeTest();

// Covers issue #9934, the return of #8784: the MXP tag watchdog unfreezes a
// stalled tag from a deferred continuation that captures a raw TBuffer 'this'.
// #8785 gave that continuation QPointer guards for the Host and the TConsole,
// but tied its lifetime to the console rather than to the buffer it writes
// into, so a buffer that names a console it is not a member of leaves the
// continuation to run over freed memory. Under the sanitisers a regression is
// a use-after-free report rather than a failed assertion.
class MxpWatchdogBufferLifetimeTest : public QObject
{
    Q_OBJECT

private:
    TelnetServerStub* mpServer = nullptr;
    Host* mpHost = nullptr;
    const QString mHostname = "MxpWatchdogBufferLifetime-Test-Host";
    QString mPort;
    const QString mLocalhost = "localhost";

    // A single pass of the loop, so that a timer armed from inside a timeout
    // handler - which is exactly what the watchdog's continuation is - does not
    // get dispatched in the same pass that armed it.
    void pumpOnce() { QCoreApplication::processEvents(QEventLoop::AllEvents); }

private slots:
    void initTestCase()
    {
        initializeQRCResourcesForMxpWatchdogBufferLifetimeTest();

        mpServer = new TelnetServerStub(qApp);
        mpServer->start(mLocalhost, 0);
        mPort = QString::number(mpServer->serverPort());
        mudlet::start();
        mudlet::self()->setupConfig();
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);
        mudlet::self()->resize(1200, 800);

        QDir(mudlet::getMudletPath(enums::profileHomePath, mHostname)).removeRecursively();

        mpHost = TestProfile::create(mHostname, mLocalhost, mPort);
        if (!mpHost) {
            QFAIL("No active host available for the test.");
        }

        QSignalSpy spy2(&(mpHost->mTelnet), &cTelnet::signal_connected);
        if (!spy2.wait(2000)) {
            QFAIL("Could not connect with the host.");
        }
    }

    void cleanupTestCase()
    {
        delete mpServer;
        mpServer = nullptr;
        mpHost = nullptr;
        const QString path = mudlet::getMudletPath(enums::profileHomePath, mHostname);
        delete mudlet::self();
        QDir(path).removeRecursively();
    }

    void init()
    {
        QVERIFY(mpHost);
        QVERIFY(mpHost->mpConsole);
        // The games this matters for do not negotiate MXP, they assume it, so
        // this is how the processor is turned on for them - secure mode included
        mpHost->setForceMXPProcessorOn(true);
        mpHost->mMxpProcessor.enable();
        mpHost->mMxpProcessor.setMode(MXP_MODE_CODE_LOCK_SECURE);
        mpHost->mMxpProcessor.getMxpTagBuilder().reset();
        mpHost->mMxpProcessor.setLastEntityValue(QString());
    }

    void cleanup()
    {
        mpHost->mMxpProcessor.getMxpTagBuilder().reset();
        mpHost->mMxpProcessor.setLastEntityValue(QString());
        mpHost->setForceMXPProcessorOn(false);
    }

    // A tag that never closes arms the watchdog; two timeouts later the
    // unfreeze phase queues a continuation that appends the literal text into
    // the buffer. Destroying the buffer in that window has to take the
    // continuation with it - it writes through a captured raw pointer.
    void test_destroyedBufferCancelsTheWatchdogContinuation()
    {
        // Connecting can leave the main console's own watchdog counting down over
        // the MXP state this shares with it, and that would reset the tag builder
        // out from under the buffer below. Let it run itself out first.
        QTest::qWait(2 * 1300ms + 400ms);
        mpHost->mMxpProcessor.getMxpTagBuilder().reset();
        mpHost->mMxpProcessor.setLastEntityValue(QString());

        auto* pBuffer = new TBuffer(mpHost, mpHost->mpConsole);
        std::string stalledTag{"<send"};
        pBuffer->translateToPlainText(stalledTag, true);
        QVERIFY2(mpHost->mMxpProcessor.getMxpTagBuilder().isInsideTag(), "the feed left no tag open, so no watchdog was armed");

        // The unfreeze phase publishes the literal text as the last entity
        // value immediately before it queues the continuation, so that is the
        // signal that the continuation is pending but has not run yet.
        QElapsedTimer elapsed;
        elapsed.start();
        while (elapsed.elapsed() < 8000 && mpHost->mMxpProcessor.getEntityValue().isEmpty()) {
            pumpOnce();
            QThread::usleep(200);
        }
        QVERIFY2(!mpHost->mMxpProcessor.getEntityValue().isEmpty(), "the watchdog never reached its unfreeze phase, so nothing was queued to test");

        delete pBuffer;

        // Runs the queued continuation, if the destructor left one behind
        QTest::qWait(250ms);

        // Under the sanitisers the regression already showed up above, as a
        // use-after-free inside the continuation. Where they are not compiled in,
        // resetting the tag builder is the continuation's last visible act, so a
        // builder still holding the stalled tag is how a cancelled continuation
        // reads.
        QCOMPARE(QString::fromStdString(mpHost->mMxpProcessor.getMxpTagBuilder().getRawTagContent()), qsl("send"));
    }
};

void initializeQRCResourcesForMxpWatchdogBufferLifetimeTest()
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

#include "MxpWatchdogBufferLifetimeTest.moc"
QTEST_MAIN(MxpWatchdogBufferLifetimeTest)
