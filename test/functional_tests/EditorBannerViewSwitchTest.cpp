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
 * another section's banner (or the dismissal undo toast) lingering on screen,
 * the undo toast's close button must only close the toast, undo must restore
 * the dismissed banner, and error messages must survive a section switch.
 *
 * Run with: ctest -R EditorBannerViewSwitchTest -V
 */

#include <QFileInfo>
#include <QSettings>
#include <QTemporaryDir>
#include <QtTest/QtTest>
#include <chrono>

#include "PortableModeTestHelper.h"
#include "ProfileTestHelper.h"
#include "Host.h"
#include "MudletInstanceCoordinator.h"
#include "TelnetServerStub.h"
#include "ctelnet.h"
#include "dlgConnectionProfiles.h"
#include "dlgSystemMessageArea.h"
#include "dlgTriggerEditor.h"
#include "mudlet.h"

#include "GroupedTest.h"

using namespace std::chrono_literals;

class EditorBannerViewSwitchTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
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
        if (dir.exists() && !dir.removeRecursively()) {
            qWarning() << "deleteProfileDirectory: could not remove" << path << "- later failures may stem from this stale state";
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
        mpHost = TestProfile::create(profileName, address, port);
        if (!mpHost) {
            QFAIL("No active host available for the test.");
        }

        QSignalSpy spy2(&(mpHost->mTelnet), &cTelnet::signal_connected);
        if (!spy2.wait(2000)) {
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

        mpServer = new TelnetServerStub(qApp);
        mpServer->start(mLocalhost, 0);
        QVERIFY2(mpServer->isListening(), qPrintable(qsl("TelnetServerStub failed to start: %1").arg(mpServer->errorString())));
        mPort = QString::number(mpServer->serverPort());
        mudlet::start();
        mudlet::self()->setupConfig();
        QCOMPARE(mudlet::getMudletPath(enums::mainPath), qsl("%1/mudlet").arg(mConfigDir.path()));
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);
        clearBannerSettings();
        deleteProfileDirectory(mProfileName);
        startProfile(mProfileName, mLocalhost, mPort);
        // QFAIL inside startProfile() only returns from that helper - bail out
        // here too or the mpHost dereference below crashes and buries the
        // recorded diagnostic under a segfault
        if (QTest::currentTestFailed()) {
            return;
        }

        mudlet::self()->slot_showScriptDialog();
        QTest::qWait(100ms);

        mpEditor = mpHost->mpEditorDialog;
        QVERIFY2(mpEditor != nullptr, "Editor dialog should be created");
    }

    void cleanupTestCase()
    {
        mpEditor = nullptr;
        mpHost = nullptr;
        delete mpServer;
        mpServer = nullptr;
        // Null when initTestCase skipped or failed ahead of mudlet::start(), and
        // getQSettings() and getMudletPath() dereference the instance rather
        // than checking it
        if (mudlet::self()) {
            clearBannerSettings();
            deleteProfileDirectory(mProfileName);
            delete mudlet::self();
        }
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
    }

    // Reset the banner state - in-memory and this profile's persisted
    // preferences - so each test starts from "nothing dismissed yet"
    void init()
    {
        mpEditor->cancelBannerUndoTimer();
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
        QCOMPARE(mpEditor->mCurrentBannerKey, qsl("intro"));

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
        QVERIFY2(mpEditor->mpSystemMessageArea->isVisible(), "Scripts banner should show initially");
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
        QVERIFY2(mpEditor->mpSystemMessageArea->isVisible(), "Scripts banner should show initially");
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

    // Undo after a single dismissal still restores the banner - driven through
    // the toast's link wiring, not by calling undoBannerDismiss() directly, so
    // a broken linkActivated connection is caught too
    void testUndoRestoresBanner()
    {
        mpEditor->slot_showScripts();
        QTest::qWait(50ms);
        QVERIFY2(mpEditor->mpSystemMessageArea->isVisible(), "Scripts banner should show initially");
        const QString scriptsBanner = bannerText();

        clickBannerCloseButton();
        QVERIFY(mpEditor->mpSystemMessageArea->isVisible());

        QMetaObject::invokeMethod(mpEditor->mpSystemMessageArea->notificationAreaMessageBox, "linkActivated", Q_ARG(QString, qsl("undo")));
        QTest::qWait(50ms);
        QVERIFY2(mpEditor->mpSystemMessageArea->isVisible(), "Undo should restore the dismissed banner");
        QCOMPARE(bannerText(), scriptsBanner);
    }

    // Errors are not banners: they must survive a section switch instead of
    // being cleared by the new-view banner handling
    void testErrorSurvivesViewSwitch()
    {
        mpEditor->slot_showScripts();
        QTest::qWait(50ms);
        const QString errorText = qsl("test error message");
        mpEditor->showError(errorText);
        QVERIFY(mpEditor->mpSystemMessageArea->isVisible());

        mpEditor->slot_showTimers();
        QTest::qWait(50ms);
        QVERIFY2(mpEditor->mpSystemMessageArea->isVisible(), "An error message must survive a view switch");
        QCOMPARE(bannerText(), errorText);
    }

    // An error raised while the undo toast's 5s expiry timer is still running
    // must survive both the timer and a view switch
    void testErrorShownDuringToastWindowSurvivesViewSwitch()
    {
        mpEditor->slot_showScripts();
        QTest::qWait(50ms);
        QVERIFY2(mpEditor->mpSystemMessageArea->isVisible(), "Scripts banner should show initially");

        clickBannerCloseButton(); // toast up, expiry timer running
        const QString errorText = qsl("error raised during toast");
        mpEditor->showError(errorText);

        mpEditor->slot_showTimers();
        QTest::qWait(50ms);
        QVERIFY2(mpEditor->mpSystemMessageArea->isVisible(), "An error shown while the undo toast timer was live must survive a view switch");
        QCOMPARE(bannerText(), errorText);
    }
};

#include "EditorBannerViewSwitchTest.moc"
MUDLET_GROUPED_TEST_MAIN(EditorBannerViewSwitchTest)
