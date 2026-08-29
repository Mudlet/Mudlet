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
 * The settings dialog has no Save button: an edit restarts a 400ms debounce that
 * writes every changed setting back through applyAll(), and closing does the same
 * once more for whatever the debounce had not reached.
 *
 * Run with: ctest -R SettingsInstantApplyTest -V
 */

#include <QDir>
#include <QTemporaryDir>
#include <QtTest/QtTest>

#include <QCheckBox>
#include <QLineEdit>
#include <QListWidget>
#include <QScrollArea>
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

class SettingsInstantApplyTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    TelnetServerStub* mpServer = nullptr;
    Host* mpHost = nullptr;
    dlgProfilePreferences* mpPreferences = nullptr;
    const QString mProfileName = qsl("SettingsInstantApply-Test");
    QString mPort; // assigned the stub's actual ephemeral port in initTestCase()
    const QString mLocalhost = qsl("localhost");


    static void deleteProfileDirectory(const QString& profileName) { TestSettings::deleteProfileDirectory(profileName); }

    QListWidget* sidebar() const { return TestSettings::sidebar(mpPreferences); }

    void selectCategory(const QString& key)
    {
        QListWidget* pList = sidebar();
        for (int row = 0, rows = pList->count(); row < rows; ++row) {
            if (pList->item(row)->data(Qt::UserRole).toString() == key) {
                pList->setCurrentRow(row);
                QCoreApplication::processEvents();
                return;
            }
        }
        QFAIL(qPrintable(qsl("no sidebar item for category '%1'").arg(key)));
    }

    // applyAll() emits this whatever it ends up writing, so it is what says an
    // apply happened rather than that one particular setting moved

    // Split so that a case can watch for an apply from before the dialog is on
    // screen: the debounce is armed by the last control population writes, and
    // waiting for exposure first could let it run before the spy exists
    void constructPreferences()
    {
        mpPreferences = new dlgProfilePreferences(mudlet::self(), mpHost);
        mpPreferences->resize(1060, 760);
    }

    void showPreferences()
    {
        mpPreferences->show();
        QVERIFY(QTest::qWaitForWindowExposed(mpPreferences));
    }

    void openPreferences()
    {
        constructPreferences();
        showPreferences();
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
    }

    // Nothing is pressed: the user stops editing and the setting is written
    void test_aToggleReachesTheHostAfterTheDebounce()
    {
        openPreferences();
        const bool before = mpHost->mHighlightHistory;
        QCOMPARE(mpPreferences->checkBox_highlightHistory->isChecked(), before);

        QSignalSpy applySpy(mpPreferences, &dlgProfilePreferences::signal_preferencesSaved);
        mpPreferences->checkBox_highlightHistory->click();
        QCOMPARE(mpPreferences->checkBox_highlightHistory->isChecked(), !before);
        // ...and not before the debounce, or there would be nothing to debounce
        QCOMPARE(mpHost->mHighlightHistory, before);

        QVERIFY2(TestSettings::waitForApply(applySpy), "the debounce never wrote the settings back");
        QCOMPARE(mpHost->mHighlightHistory, !before);
    }

    // A line edit that applied per keystroke would write half-typed values -
    // "a" and "ab" on the way to "abc" - so they report editingFinished instead
    void test_typingAppliesOnlyOnceTheEditIsFinished()
    {
        openPreferences();
        selectCategory(qsl("inputLine"));

        QLineEdit* pSeparator = mpPreferences->command_separator_lineedit;
        const QString before = mpHost->getCommandSeparator();
        QCOMPARE(pSeparator->text(), before);
        const QString typed = (before == qsl("##")) ? qsl("@@") : qsl("##");

        QSignalSpy applySpy(mpPreferences, &dlgProfilePreferences::signal_preferencesSaved);
        pSeparator->setFocus();
        pSeparator->selectAll();
        QTest::keyClicks(pSeparator, typed);
        QCOMPARE(pSeparator->text(), typed);

        QVERIFY2(!applySpy.wait(TestSettings::scmQuietWindow), "typing into a line edit applied the settings before the edit was finished");
        QCOMPARE(mpHost->getCommandSeparator(), before);

        // Return is what a user finishing with the field presses
        QTest::keyClick(pSeparator, Qt::Key_Return);
        QVERIFY2(TestSettings::waitForApply(applySpy), "finishing the edit never wrote the settings back");
        QCOMPARE(mpHost->getCommandSeparator(), typed);
    }

    // initWithHost() writes several hundred controls, every one of which would
    // otherwise restart the debounce - and then the dialog would write settings
    // back that nobody asked it to
    void test_openingTheDialogWritesNothing()
    {
        constructPreferences();
        QSignalSpy applySpy(mpPreferences, &dlgProfilePreferences::signal_preferencesSaved);
        showPreferences();
        QVERIFY2(!applySpy.wait(TestSettings::scmQuietWindow), "populating the dialog applied the settings");

        // ...and neither does walking around it. The Editor category is left
        // out on purpose: its first visit refreshes the edbee themes, which is
        // a network round trip rather than anything to do with applying.
        for (const QString& category : {qsl("mainDisplay"), qsl("mapper"), qsl("privacy"), qsl("general")}) {
            selectCategory(category);
        }
        QVERIFY2(!applySpy.wait(TestSettings::scmQuietWindow), "switching category applied the settings");
    }

    // Closing is not a discard, so an edit made inside the last 400ms still has
    // to be written on the way out
    void test_aChangeMadeJustBeforeClosingIsStillApplied()
    {
        openPreferences();
        const bool before = mpHost->mEnableTextAnalyzer;
        QCOMPARE(mpPreferences->checkBox_enableTextAnalyzer->isChecked(), before);

        mpPreferences->checkBox_enableTextAnalyzer->click();
        QCOMPARE(mpHost->mEnableTextAnalyzer, before);

        // no wait in between: the debounce has not had a chance to run
        mpPreferences->close();
        QCOMPARE(mpHost->mEnableTextAnalyzer, !before);
    }

    // The debounce is shared, so an apply another control started can land in
    // the middle of a word being typed somewhere else. A field being typed into
    // is not finished with, so it waits for the apply its own editingFinished()
    // schedules.
    void test_anApplyStartedElsewhereDoesNotCommitAHalfTypedField()
    {
        openPreferences();
        selectCategory(qsl("inputLine"));
        QLineEdit* pSeparator = mpPreferences->command_separator_lineedit;
        const QString before = mpHost->getCommandSeparator();
        const QString halfTyped = (before == qsl("@")) ? qsl("~") : qsl("@");

        QSignalSpy applySpy(mpPreferences, &dlgProfilePreferences::signal_preferencesSaved);
        // ...something else starts the debounce, and the user is mid-word when
        // it runs out
        mpPreferences->checkBox_announceIncomingText->click();
        pSeparator->setFocus();
        pSeparator->selectAll();
        QTest::keyClicks(pSeparator, halfTyped);
        QVERIFY2(TestSettings::waitForApply(applySpy), "the unrelated edit never wrote the settings back");
        QCOMPARE(mpHost->getCommandSeparator(), before);

        // ...and finishing with the field still writes it
        applySpy.clear();
        QTest::keyClick(pSeparator, Qt::Key_Return);
        QVERIFY2(TestSettings::waitForApply(applySpy), "finishing the edit never wrote the settings back");
        QCOMPARE(mpHost->getCommandSeparator(), halfTyped);

        mpHost->mCommandSeparator = before;
    }

    // The ten protocol toggles are ordinary checkboxes on the subpage the
    // Connection page's protocols card leads to, so instant apply reaches them
    // the same way it reaches every other checkbox.
    void test_aProtocolToggledOnItsSubpageReachesTheHost()
    {
        openPreferences();
        auto* pGmcp = mpPreferences->findChild<QCheckBox*>(qsl("checkBox_enableGMCP"));
        QVERIFY2(pGmcp, "the game protocols subpage has no GMCP checkbox");
        QVERIFY2(mpPreferences->findChild<QScrollArea*>(qsl("settingsPage_connection_protocols")), "the game protocols subpage was not built");
        const bool before = mpHost->mEnableGMCP;
        QCOMPARE(pGmcp->isChecked(), before);

        QSignalSpy applySpy(mpPreferences, &dlgProfilePreferences::signal_preferencesSaved);
        pGmcp->toggle();
        QCOMPARE(pGmcp->isChecked(), !before);
        QVERIFY2(TestSettings::waitForApply(applySpy), "toggling a protocol never wrote the settings back");

        QCOMPARE(mpHost->mEnableGMCP, !before);
        mpHost->mEnableGMCP = before;
    }


    // Esc reaches closeEvent() through QDialog::reject(), which makes it a close
    // rather than a discard.
    void test_escapeAppliesRatherThanDiscarding()
    {
        openPreferences();
        const bool before = mpHost->mEnableTextAnalyzer;
        mpPreferences->checkBox_enableTextAnalyzer->click();
        // ...and not before the dialog is closed, or the close proves nothing
        QCOMPARE(mpHost->mEnableTextAnalyzer, before);

        QTest::keyClick(mpPreferences, Qt::Key_Escape);

        QCOMPARE(mpHost->mEnableTextAnalyzer, !before);
        mpHost->mEnableTextAnalyzer = before;
    }

    // The profile XML is written once per close rather than on every apply, so
    // the write on the way out has to really reach the disk, not just the Host.
    void test_closingWritesTheProfileToDisk()
    {
        const bool saveOnExitBefore = mpHost->mFORCE_SAVE_ON_EXIT;
        const bool analyzerBefore = mpHost->mEnableTextAnalyzer;
        mpHost->mFORCE_SAVE_ON_EXIT = true;

        const QString saveDirPath = mudlet::getMudletPath(enums::profileXmlFilesPath, mProfileName);
        QDir saveDir(saveDirPath);
        QVERIFY(QDir().mkpath(saveDirPath));
        for (const QString& file : saveDir.entryList({qsl("*.xml")}, QDir::Files)) {
            QVERIFY(saveDir.remove(file));
        }
        QVERIFY2(saveDir.entryList({qsl("*.xml")}, QDir::Files).isEmpty(), "the profile's save directory still holds an XML from before this case");

        openPreferences();
        mpPreferences->checkBox_enableTextAnalyzer->click();
        mpPreferences->close();

        // the write itself runs off the main thread
        QVERIFY2(QTest::qWaitFor(
                         [&saveDir]() {
                             return !saveDir.entryList({qsl("*.xml")}, QDir::Files).isEmpty();
                         },
                         30000),
                 "closing the settings never wrote the profile out");

        mpHost->mFORCE_SAVE_ON_EXIT = saveOnExitBefore;
        mpHost->mEnableTextAnalyzer = analyzerBefore;
    }
};

#include "SettingsInstantApplyTest.moc"
MUDLET_GROUPED_TEST_MAIN(SettingsInstantApplyTest)
