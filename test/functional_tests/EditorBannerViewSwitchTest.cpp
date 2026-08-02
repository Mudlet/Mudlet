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
 * Regression tests for the editor's dismissible help banners when switching
 * between editor sections: a dismissed section's suppression must not leave
 * another section's banner (or the dismissal undo toast) lingering on screen.
 *
 * Run with: ctest -R EditorBannerViewSwitchTest -V
 */

#include <QtTest/QtTest>
#include <chrono>

#include "Host.h"
#include "MudletInstanceCoordinator.h"
#include "TelnetServerStub.h"
#include "ctelnet.h"
#include "dlgConnectionProfiles.h"
#include "dlgSystemMessageArea.h"
#include "dlgTriggerEditor.h"
#include "mudlet.h"

using namespace std::chrono_literals;

extern void qInitResources_mudlet();
extern void qInitResources_qm();
extern void qInitResources_additional_splash_screens();
extern void qInitResources_mudlet_fonts_common();
extern void qInitResources_mudlet_fonts_posix();

static void initializeQRCResources()
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

class EditorBannerViewSwitchTest : public QObject
{
    Q_OBJECT

private:
    TelnetServerStub* mpServer = nullptr;
    dlgTriggerEditor* mpEditor = nullptr;
    Host* mpHost = nullptr;
    const QString mProfileName = qsl("BannerViewSwitch-Test-Profile");
    QString mPort;
    const QString mLocalhost = qsl("localhost");

    void deleteProfileDirectory(const QString& profileName)
    {
        const QString path = mudlet::getMudletPath(enums::profileHomePath, profileName);
        QDir dir(path);
        if (dir.exists()) {
            dir.removeRecursively();
        }
    }

    // The permanently-hidden banner preferences are stored in QSettings under a
    // per-profile prefix; wipe this profile's slice so earlier runs cannot bleed
    // into the assertions (the profile name is unique to this test, so nothing
    // belonging to a real profile is touched)
    void clearBannerSettings()
    {
        QSettings* settings = mudlet::getQSettings();
        settings->remove(qsl("Editor/banner_permanently_hidden/profiles/%1").arg(mProfileName));
    }

    void startProfile(const QString& profileName, const QString& address, const QString& port)
    {
        QTimer::singleShot(0ms, qApp, [profileName, address, port]() {
            mudlet::self()->startAutoLogin({});
            QTest::qWait(100ms);

            QTest::mouseClick(mudlet::self()->mpConnectionDialog->new_profile_button, Qt::LeftButton);
            QTest::qWait(100ms);
            QTest::keyClicks(QApplication::focusWidget(), profileName);
            QTest::qWait(100ms);
            QTest::keyClick(QApplication::focusWidget(), Qt::Key_Tab);
            QTest::qWait(100ms);
            QTest::keyClicks(QApplication::focusWidget(), address);
            QTest::qWait(100ms);
            QTest::keyClick(QApplication::focusWidget(), Qt::Key_Tab);
            QTest::qWait(100ms);
            QTest::keyClicks(QApplication::focusWidget(), port);
            QTest::qWait(100ms);
            QTest::keyClick(QApplication::focusWidget(), Qt::Key_Return);
        });

        QSignalSpy spy(mudlet::self(), &mudlet::signal_profileLoaded);
        if (!spy.wait(2000)) {
            QFAIL("Profile took too long to load.");
        }

        mpHost = mudlet::self()->getActiveHost();
        if (!mpHost) {
            QFAIL("No active host available for the test.");
        }

        QSignalSpy spy2(&(mpHost->mTelnet), &cTelnet::signal_connected);
        if (!spy2.wait(1000)) {
            QFAIL("Could not connect with the host.");
        }
    }

    QString bannerText() const { return mpEditor->mpSystemMessageArea->notificationAreaMessageBox->text(); }

    void clickBannerCloseButton()
    {
        QTest::mouseClick(mpEditor->mpSystemMessageArea->messageAreaCloseButton, Qt::LeftButton);
        QTest::qWait(50ms);
    }

private slots:
    void initTestCase()
    {
        initializeQRCResources();

        mpServer = new TelnetServerStub(qApp);
        mpServer->start(mLocalhost, 0);
        QVERIFY2(mpServer->isListening(), qPrintable(qsl("TelnetServerStub failed to start: %1").arg(mpServer->errorString())));
        mPort = QString::number(mpServer->serverPort());
        mudlet::start();
        mudlet::self()->setupConfig();
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);
        clearBannerSettings();
        deleteProfileDirectory(mProfileName);
        startProfile(mProfileName, mLocalhost, mPort);

        mudlet::self()->slot_showScriptDialog();
        QTest::qWait(100ms);

        mpEditor = mpHost->mpEditorDialog;
        QVERIFY2(mpEditor != nullptr, "Editor dialog should be created");
    }

    void cleanupTestCase()
    {
        clearBannerSettings();
        mpEditor = nullptr;
        mpHost = nullptr;
        delete mpServer;
        mpServer = nullptr;
        deleteProfileDirectory(mProfileName);
        delete mudlet::self();
    }

    // Reset the in-memory banner state so each test starts from "nothing
    // dismissed yet", like a fresh editor session
    void init()
    {
        if (mpEditor->mpBannerUndoTimer) {
            mpEditor->mpBannerUndoTimer->stop();
            mpEditor->mpBannerUndoTimer->deleteLater();
            mpEditor->mpBannerUndoTimer = nullptr;
        }
        mpEditor->mTemporarilyHiddenBanners.clear();
        mpEditor->mLastDismissedBannerView = EditorViewType::cmUnknownView;
        mpEditor->mLastDismissedBannerContent.clear();
        mpEditor->mLastDismissedBannerKey.clear();
        mpEditor->mCurrentBannerKey.clear();
        mpEditor->mpSystemMessageArea->hide();
        clearBannerSettings();
    }

    // The reported repro: dismiss the Scripts banner (X on the banner, then X
    // on the "Banner hidden" undo toast), visit Timers, come back to Scripts -
    // the Timers banner must not linger over the Scripts section
    void testDismissedBannerDoesNotLeakAcrossViews()
    {
        mpEditor->slot_showScripts();
        QTest::qWait(50ms);
        QVERIFY2(mpEditor->mpSystemMessageArea->isVisible(), "Scripts banner should show initially");
        const QString scriptsBanner = bannerText();

        clickBannerCloseButton(); // dismisses the banner, shows the undo toast
        clickBannerCloseButton(); // closes the undo toast

        mpEditor->slot_showTimers();
        QTest::qWait(50ms);
        QVERIFY2(mpEditor->mpSystemMessageArea->isVisible(), "Timers banner should show after switching to Timers");
        const QString timersBanner = bannerText();
        QVERIFY2(timersBanner != scriptsBanner, "Timers banner content should differ from the Scripts one");

        mpEditor->slot_showScripts();
        QTest::qWait(50ms);
        QVERIFY2(!mpEditor->mpSystemMessageArea->isVisible(),
                 "The dismissed Scripts banner must stay hidden - and the Timers "
                 "banner must not linger over the Scripts section");
    }

    // Same leak, without touching the toast: a single dismissal then switching
    // views back and forth must not leave the other view's banner behind
    void testSingleDismissDoesNotLeakAcrossViews()
    {
        mpEditor->slot_showScripts();
        QTest::qWait(50ms);
        QVERIFY(mpEditor->mpSystemMessageArea->isVisible());

        clickBannerCloseButton();

        mpEditor->slot_showTimers();
        QTest::qWait(50ms);
        QVERIFY2(mpEditor->mpSystemMessageArea->isVisible(), "Timers banner should show after switching to Timers");

        mpEditor->slot_showScripts();
        QTest::qWait(50ms);
        QVERIFY2(!mpEditor->mpSystemMessageArea->isVisible(), "No banner should show in Scripts after its banner was dismissed");
    }

    // The X on the undo toast must only close the toast - not register another
    // dismissal that suppresses the whole view's banners and stashes the toast
    // text as restorable banner content
    void testToastCloseButtonJustClosesToast()
    {
        mpEditor->slot_showScripts();
        QTest::qWait(50ms);
        const QString scriptsBanner = bannerText();

        clickBannerCloseButton();
        QVERIFY2(mpEditor->mpSystemMessageArea->isVisible(), "Undo toast should show after dismissing the banner");
        QVERIFY(bannerText() != scriptsBanner);

        clickBannerCloseButton();
        QVERIFY2(!mpEditor->mpSystemMessageArea->isVisible(), "Closing the undo toast should hide the message area");
        QCOMPARE(mpEditor->mLastDismissedBannerKey, qsl("intro"));
        QCOMPARE(mpEditor->mLastDismissedBannerContent, scriptsBanner);
        const QString baseKey = mpEditor->bannerSettingsKey(EditorViewType::cmScriptView, QString());
        QVERIFY2(!mpEditor->mTemporarilyHiddenBanners.contains(baseKey), "Closing the toast must not suppress all banners for the view");
    }

    // A pending undo toast belongs to the view it was shown in - switching
    // views must clear it and show the new view's banner instead
    void testToastHiddenOnViewSwitch()
    {
        mpEditor->slot_showScripts();
        QTest::qWait(50ms);
        const QString scriptsBanner = bannerText();

        clickBannerCloseButton();
        QVERIFY(mpEditor->mpSystemMessageArea->isVisible());

        mpEditor->slot_showTimers();
        QTest::qWait(50ms);
        QVERIFY2(mpEditor->mpSystemMessageArea->isVisible(), "Timers banner should show after switching to Timers");
        QCOMPARE(mpEditor->mCurrentBannerKey, qsl("intro"));
        QVERIFY2(bannerText() != scriptsBanner, "The Timers banner, not stale Scripts content, should show");
        QVERIFY2(!bannerText().contains(qsl("href='undo'")), "The undo toast must not linger after a view switch");
    }

    // Undo after a single dismissal still restores the banner
    void testUndoRestoresBanner()
    {
        mpEditor->slot_showScripts();
        QTest::qWait(50ms);
        const QString scriptsBanner = bannerText();

        clickBannerCloseButton();
        QVERIFY(mpEditor->mpSystemMessageArea->isVisible());

        mpEditor->undoBannerDismiss();
        QTest::qWait(50ms);
        QVERIFY2(mpEditor->mpSystemMessageArea->isVisible(), "Undo should restore the dismissed banner");
        QCOMPARE(bannerText(), scriptsBanner);
    }
};

#include "EditorBannerViewSwitchTest.moc"
QTEST_MAIN(EditorBannerViewSwitchTest)
