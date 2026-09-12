/***************************************************************************
 *   Copyright (C) 2026 by Mudlet Developers - mudlet@mudlet.org           *
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
 * Three MMCP settings - auto-start the chat server, auto-accept incoming calls
 * and allow peek requests - have no controls on the settings form: the check
 * boxes are commented out of profile_preferences.ui pending a future release.
 * applyAll() nonetheless wrote all three back as false, outside the dirty
 * guards, so every apply reset them. Once Esc became a close rather than a
 * discard (#10237), merely looking at the settings turned off a setting the
 * profile XML is the only way to set - and, with no control to turn it back on,
 * unrecoverably from the UI.
 *
 * Run with: ctest -R SettingsMmcpFlagsTest -V
 */

#include <QDir>
#include <QTemporaryDir>
#include <QtTest/QtTest>

#include <QCheckBox>
#include <QSignalSpy>

#include "PortableModeTestHelper.h"
#include "ProfileTestHelper.h"
#include "SettingsTestHelper.h"
#include "Host.h"
#include "MudletInstanceCoordinator.h"
#include "TelnetServerStub.h"
#include "dlgProfilePreferences.h"
#include "mudlet.h"

#include "GroupedTest.h"

class SettingsMmcpFlagsTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    TelnetServerStub* mpServer = nullptr;
    Host* mpHost = nullptr;
    dlgProfilePreferences* mpPreferences = nullptr;
    const QString mProfileName = qsl("SettingsMmcpFlags-Test");
    QString mPort; // assigned the stub's actual ephemeral port in initTestCase()
    const QString mLocalhost = qsl("localhost");

    static void deleteProfileDirectory(const QString& profileName) { TestSettings::deleteProfileDirectory(profileName); }

    void openPreferences()
    {
        mpPreferences = new dlgProfilePreferences(mudlet::self(), mpHost);
        mpPreferences->resize(1060, 760);
        mpPreferences->show();
        QVERIFY(QTest::qWaitForWindowExposed(mpPreferences));
    }

    // Auto-accept calls is on unless a profile XML says otherwise, which is what
    // makes it the one of the three an apply can be seen to lose. The other two
    // are off by default and are checked in the same breath, since the same
    // three lines wrote all of them.
    void verifyTheProfilesOptionsSurvived(const char* what)
    {
        QVERIFY2(mpHost->getMMCPAutoAcceptCalls(), what);
        QVERIFY2(!mpHost->getMMCPAutoStartServer(), "an apply turned the chat server's auto-start on, which no control on the form asked for");
        QVERIFY2(!mpHost->getMMCPAllowPeekRequests(), "an apply turned peek requests on, which no control on the form asked for");
    }

    void verifyTheFixtureIsUsable()
    {
        QVERIFY2(mpHost->getMMCPAutoAcceptCalls(),
                 "auto-accept calls was already off before the settings were opened, so this case cannot show an apply turning it off - the profile default has changed");
        QVERIFY2(!mpHost->getMMCPAutoStartServer() && !mpHost->getMMCPAllowPeekRequests(), "the other two MMCP options are not at their defaults");
    }

private slots:
    void initTestCase()
    {
        if (portableMarkerPresent()) {
            QSKIP("portable.txt present - it takes precedence over XDG_CONFIG_HOME, so the config dir cannot be redirected");
        }

        // A config root of this process's own - see the same block in
        // DialogTeardownTest for why sharing the developer's one does not work
        QVERIFY(mConfigDir.isValid());
        QVERIFY(QDir().mkpath(qsl("%1/mudlet/profiles").arg(mConfigDir.path())));
        mSavedXdg = qgetenv("XDG_CONFIG_HOME");
        qputenv("XDG_CONFIG_HOME", mConfigDir.path().toUtf8());

        mpServer = new TelnetServerStub(qApp);
        mpServer->start(mLocalhost, 0); // ephemeral OS-assigned port avoids collisions across concurrent test runs
        mPort = QString::number(mpServer->serverPort());
        mudlet::start();
        mudlet::self()->setupConfig();
        QCOMPARE(mudlet::getMudletPath(enums::mainPath), qsl("%1/mudlet").arg(mConfigDir.path()));
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>(qsl("MudletInstanceCoordinator")));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);
        deleteProfileDirectory(mProfileName);

        mpHost = TestProfile::create(mProfileName, mLocalhost, mPort);
        QVERIFY2(mpHost, "No active host after profile creation");
    }

    void cleanupTestCase()
    {
        mpHost = nullptr;
        delete mpServer;
        mpServer = nullptr;
        // Null when initTestCase skipped or failed ahead of mudlet::start(), and
        // getMudletPath() dereferences the instance rather than checking it
        if (mudlet::self()) {
            deleteProfileDirectory(mProfileName);
            delete mudlet::self();
        }
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
    }

    void cleanup()
    {
        delete mpPreferences;
        mpPreferences = nullptr;
        mpHost->waitForProfileSave();
    }

    // The finding: open the settings, press Esc, and the profile's MMCP options
    // are gone.
    void test_dismissingWithEscapeKeepsTheMmcpOptions()
    {
        verifyTheFixtureIsUsable();
        openPreferences();

        QTest::keyClick(mpPreferences, Qt::Key_Escape);
        QVERIFY2(!mpPreferences->isVisible(), "Escape did not close the settings, so the apply it triggers never ran");

        verifyTheProfilesOptionsSurvived("dismissing the settings with Escape turned auto-accept calls off, and no control on the form can turn it back on");
    }

    // ...and the same for an apply the user really did ask for, by editing
    // something else entirely.
    void test_anUnrelatedEditKeepsTheMmcpOptions()
    {
        verifyTheFixtureIsUsable();
        openPreferences();

        QSignalSpy applySpy(mpPreferences, &dlgProfilePreferences::signal_preferencesSaved);
        const bool announceBefore = mpHost->mAnnounceIncomingText;
        mpPreferences->checkBox_announceIncomingText->click();
        QVERIFY2(TestSettings::waitForApply(applySpy), "the debounce never wrote the settings back");
        QCOMPARE(mpHost->mAnnounceIncomingText, !announceBefore);

        verifyTheProfilesOptionsSurvived("applying the settings turned auto-accept calls off, and no control on the form can turn it back on");
        mpHost->mAnnounceIncomingText = announceBefore;
    }

    // The MMCP settings that do have controls still have to be written, or this
    // would be a fix that simply stopped applying the page.
    void test_theMmcpSettingsWithControlsAreStillApplied()
    {
        openPreferences();
        const bool before = mpHost->getMMCPPrefixEmotes();
        QCOMPARE(mpPreferences->checkBox_mmcpPrefixEmotes->isChecked(), before);

        QSignalSpy applySpy(mpPreferences, &dlgProfilePreferences::signal_preferencesSaved);
        mpPreferences->checkBox_mmcpPrefixEmotes->click();
        QVERIFY2(TestSettings::waitForApply(applySpy), "ticking an MMCP option never wrote the settings back");

        QVERIFY2(mpHost->getMMCPPrefixEmotes() == !before, "an MMCP option the user really did tick was not applied");
    }
};

#include "SettingsMmcpFlagsTest.moc"
MUDLET_GROUPED_TEST_MAIN(SettingsMmcpFlagsTest)
