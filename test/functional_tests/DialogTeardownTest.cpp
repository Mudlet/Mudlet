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
 * Functional tests for windows that are destroyed while one of their own
 * editing widgets still has the keyboard focus (#9574).
 *
 * A visible window is taken off the screen while its base-class destructors
 * unwind (~QDialog hides it, ~QWidget closes any other window class), which
 * moves the focus off the widget that holds it; a QLineEdit (QAbstractSpinBox
 * and QKeySequenceEdit behave the same) answers that by emitting
 * editingFinished() into a slot of a window whose derived part has already
 * been destroyed.
 *
 * A debug build ends the whole run there, on Qt's "Called object is not of the
 * correct type (class destructor may have already run)". A release build has
 * that assert compiled out and runs the slot against the destroyed object
 * instead, so each test also checks that the edit in the focused field was not
 * acted on - the same assertion holds whichever way the build was configured.
 *
 * Run with: ctest -R DialogTeardownTest -V
 */

#include <QFileInfo>
#include <QTemporaryDir>
#include <QtTest/QtTest>
#include <chrono>

#include <QCheckBox>
#include <QKeySequenceEdit>
#include <QLineEdit>
#include <QScopeGuard>

#include "PortableModeTestHelper.h"
#include "ProfileTestHelper.h"
#include "Host.h"
#include "MudletInstanceCoordinator.h"
#include "TelnetServerStub.h"
#include "TriggerUnit.h"
#include "dlgConnectionProfiles.h"
#include "dlgProfilePreferences.h"
#include "dlgTriggerEditor.h"
#include "mudlet.h"
#if defined(INCLUDE_UPDATER)
#include "updater.h"
#endif

#include "GroupedTest.h"

using namespace std::chrono_literals;

class DialogTeardownTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    TelnetServerStub* mpServer = nullptr;
    Host* mpHost = nullptr;
    const QString mProfileName = qsl("DialogTeardown-Test");
    QString mPort; // assigned the stub's actual ephemeral port in initTestCase()
    const QString mLocalhost = qsl("localhost");

    void deleteProfileDirectory(const QString& profileName)
    {
        const QString path = mudlet::getMudletPath(enums::profileHomePath, profileName);
        QDir dir(path);
        if (dir.exists()) {
            dir.removeRecursively();
        }
    }

    void startProfile(const QString& profileName, const QString& address, const QString& port)
    {
        mpHost = TestProfile::create(profileName, address, port);
        if (!mpHost) {
            QFAIL("No active host available for the test.");
        }
    }

    // Gives the widget the keyboard focus and something to report: a QLineEdit
    // only emits editingFinished() on focus-out once its text has been touched,
    // and the setText() here is what arms that - a field nothing has written to
    // stays quiet and would make this test prove nothing.
    void focusWithText(QLineEdit* lineEdit, const QString& text)
    {
        QVERIFY2(lineEdit->isVisible(), "Field has to be on screen to be able to take the focus");
        lineEdit->setText(text);
        lineEdit->setFocus();
        QCoreApplication::processEvents();
        QCOMPARE(QApplication::focusWidget(), lineEdit);
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
        mpServer->start(mLocalhost, 0); // ephemeral OS-assigned port avoids collisions across concurrent test runs
        mPort = QString::number(mpServer->serverPort());
        mudlet::start();
        mudlet::self()->setupConfig();
        QCOMPARE(mudlet::getMudletPath(enums::mainPath), qsl("%1/mudlet").arg(mConfigDir.path()));
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>(qsl("MudletInstanceCoordinator")));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);
        deleteProfileDirectory(mProfileName);

        startProfile(mProfileName, mLocalhost, mPort);
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

    // Everything below rests on Qt still emitting the focus-out signals while a
    // window is being destroyed. If a future Qt stops doing that the other tests
    // would keep passing while testing nothing, so pin the mechanism itself on
    // widgets of our own - the receiver here outlives them, which is exactly what
    // the windows under test cannot manage.
    void test_teardownEmitsTheSignalsThisIsAllAbout()
    {
        auto* dialog = new QDialog(mudlet::self());
        auto* layout = new QVBoxLayout(dialog);
        auto* lineEdit = new QLineEdit(dialog);
        layout->addWidget(lineEdit);
        dialog->show();
        lineEdit->setText(qsl("some text"));
        lineEdit->setFocus();
        QCoreApplication::processEvents();
        QCOMPARE(QApplication::focusWidget(), lineEdit);

        QSignalSpy lineEditSpy(lineEdit, &QLineEdit::editingFinished);
        delete dialog;

        QVERIFY2(lineEditSpy.count() == 1,
                 "A focused QLineEdit no longer reports editingFinished() when its "
                 "window is destroyed - the rest of this file now proves nothing");

        // the same for the shortcut editors the preferences are full of
        auto* keySequenceDialog = new QDialog(mudlet::self());
        auto* keySequenceLayout = new QVBoxLayout(keySequenceDialog);
        auto* secondKeySequenceEdit = new QKeySequenceEdit(keySequenceDialog);
        keySequenceLayout->addWidget(secondKeySequenceEdit);
        keySequenceDialog->show();
        secondKeySequenceEdit->setKeySequence(QKeySequence(qsl("Ctrl+K")));
        secondKeySequenceEdit->setFocus();
        QCoreApplication::processEvents();
        // it focus-proxies to an inner line edit, so ask the wrapper itself
        QVERIFY2(secondKeySequenceEdit->hasFocus(), "Shortcut editor did not take the focus");

        QSignalSpy secondSpy(secondKeySequenceEdit, &QKeySequenceEdit::editingFinished);
        delete keySequenceDialog;
        QVERIFY2(secondSpy.count() == 1,
                 "A focused QKeySequenceEdit no longer reports editingFinished() "
                 "when its window is destroyed");
    }

    // #9574: the reported crash - the profile name field is connected to
    // slot_saveName() and the dialog is torn down while that field has the focus
    void test_connectionDialogDestroyedWithFocusedNameField()
    {
        // built directly rather than through mudlet::slot_showConnectionDialog()
        // so that the profile this test suite loaded does not have the dialog
        // connect it straight back and close it
        QPointer<dlgConnectionProfiles> dialog = new dlgConnectionProfiles(mudlet::self());
        dialog->fillout_form();
        dialog->show();
        QTest::qWait(100ms);
        QVERIFY2(dialog, "Connection dialog closed itself");

        // pick our own profile, so that the name field is editing something whose
        // renaming can be checked for afterwards
        const auto items = dialog->findData(*dialog->listWidget_profiles, mProfileName, dlgConnectionProfiles::csmNameRole);
        QVERIFY2(!items.isEmpty(), "Test profile is not listed in the dialog");
        dialog->listWidget_profiles->setCurrentItem(items.first());
        QTest::qWait(100ms);

        const QString renamedTo = qsl("DialogTeardown-Renamed");
        focusWithText(dialog->profile_name_entry, renamedTo);

        delete dialog;
        QVERIFY2(dialog.isNull(), "Connection dialog should have been destroyed");
        // slot_saveName() renames the profile's directory, so it running on the way
        // down leaves a trace even in a build where the assert is compiled out
        QVERIFY2(!QDir(mudlet::getMudletPath(enums::profileHomePath, renamedTo)).exists(), "Being destroyed made the dialog rename the profile");
        QVERIFY2(QDir(mudlet::getMudletPath(enums::profileHomePath, mProfileName)).exists(), "The profile lost its directory while the dialog was destroyed");
    }

    // The same exposure through the preferences' chat name field, which is
    // connected to slot_mmcpChatNameChanged()
    void test_preferencesDestroyedWithFocusedChatNameField()
    {
        mudlet::self()->showOptionsDialog(qsl("tab_chat"), mpHost);
        QTest::qWait(100ms);
        auto* preferences = mpHost->mpDlgProfilePreferences.data();
        QVERIFY2(preferences, "Preferences dialog was not created");

        const QString chatNameBefore = mpHost->getMMCPChatName();
        const QString typedChatName = qsl("DialogTeardownChatName");
        QVERIFY2(chatNameBefore != typedChatName, "Test needs to type a chat name that is not the current one");
        focusWithText(preferences->lineEdit_mmcpChatName, typedChatName);

        delete preferences;
        QVERIFY2(mpHost->mpDlgProfilePreferences.isNull(), "Preferences dialog should have been destroyed");
        QCOMPARE(mpHost->getMMCPChatName(), chatNameBefore);
    }

    // The other half of that field. The profile's chat name can change from
    // outside the dialog - a script calling mmcp.chatName() - while the
    // preferences are open, and the dialog only hears about it through
    // Host::mmcpChatNameChanged. Cut that connect and the open dialog goes on
    // showing (and, on Save, writing back) a name the profile no longer has.
    void test_openPreferencesFollowTheProfilesChatName()
    {
        mudlet::self()->showOptionsDialog(qsl("tab_chat"), mpHost);
        QTest::qWait(100ms);
        auto* preferences = mpHost->mpDlgProfilePreferences.data();
        QVERIFY2(preferences, "Preferences dialog was not created");

        // A failed assertion returns from here, and every other case in this
        // class runs in the same process afterwards - so the dialog and the
        // profile's name go back whichever way this ends. Leaving the dialog up
        // makes the next showOptionsDialog() hand back this one.
        const QString chatNameBefore = mpHost->getMMCPChatName();
        auto restoreState = qScopeGuard([this, preferences, chatNameBefore]() {
            delete preferences;
            mpHost->setMMCPChatName(chatNameBefore);
        });
        QCOMPARE(preferences->lineEdit_mmcpChatName->text(), chatNameBefore);

        const QString setElsewhere = qsl("DialogTeardownRenamedElsewhere");
        QVERIFY2(setElsewhere != chatNameBefore, "Test needs to set a chat name that is not the current one");

        // tells "the profile never said it" apart from "the dialog never heard it"
        QSignalSpy chatNameSpy(mpHost, &Host::mmcpChatNameChanged);
        QVERIFY2(mpHost->setMMCPChatName(setElsewhere), "The profile refused the chat name this test set");
        QCOMPARE(chatNameSpy.count(), 1);

        QCOMPARE(preferences->lineEdit_mmcpChatName->text(), setElsewhere);
    }

    // Opening the preferences at all used to be enough to end the run: the
    // dialog asks the updater whether it downloads updates by itself, which on
    // macOS reaches into Sparkle - and Sparkle is only created by
    // checkUpdatesOnStart(), which no test calls. Development builds skip that
    // whole branch, so only PTB and release builds ever crashed and CI stayed
    // green until the nightly PTB. DEV_UPDATER puts this build on the same path.
    void test_preferencesOpensBeforeTheUpdaterIsSetUp()
    {
        qputenv("DEV_UPDATER", "1");
        auto restoreEnvironment = qScopeGuard([]() {
            qunsetenv("DEV_UPDATER");
        });

        mudlet::self()->showOptionsDialog(qsl("tab_specialOptions"), mpHost);
        QTest::qWait(100ms);
        auto* preferences = mpHost->mpDlgProfilePreferences.data();
        QVERIFY2(preferences, "Preferences dialog was not created");

#if defined(INCLUDE_UPDATER)
        auto* updater = mudlet::self()->pUpdater;
        QVERIFY2(updater, "An updater-enabled build has no updater");
        // the dev-build branch disables the checkbox and touches no updater, so
        // this is what says the test is on the crashing path at all
        QVERIFY2(preferences->checkbox_noAutomaticUpdates->isEnabled(), "DEV_UPDATER no longer moves a development build onto the release update path - this test covers nothing now");
        // isHidden() rather than isVisible(): the group box sits on a tab page,
        // and only an explicit hide() should count here
        QCOMPARE(preferences->groupBox_updates->isHidden(), !updater->ready());

        if (!updater->ready()) {
            // the accessors the dialog and the Help menu reach for have to be
            // safe to call in this state, not merely avoidable
            QVERIFY2(!updater->updateAutomatically(), "An updater with no platform updater claimed it auto-updates");
            updater->setAutomaticUpdates(true);
            updater->manuallyCheckUpdates();
            QVERIFY2(!updater->updateAutomatically(), "An updater with no platform updater took a setting it cannot store");
        }
#endif

        delete preferences;
        QVERIFY2(mpHost->mpDlgProfilePreferences.isNull(), "Preferences dialog should have been destroyed");
    }

    // ...and through the editor, where the item name field is connected to
    // slot_saveProperty_TriggerName(). The editor is a QMainWindow rather than a
    // QDialog, which makes no difference: it hides itself on the way down too
    void test_triggerEditorDestroyedWithFocusedNameField()
    {
        mudlet::self()->slot_showScriptDialog();
        QTest::qWait(100ms);
        auto* editor = mpHost->mpEditorDialog.data();
        QVERIFY2(editor, "Editor was not created");

        // the item fields only appear once an item is being edited
        editor->slot_showTriggers();
        editor->slot_addNewItem();
        QTest::qWait(100ms);

        auto* nameField = editor->findChild<QLineEdit*>(qsl("lineEdit_trigger_name"));
        QVERIFY2(nameField, "Trigger name field not found in the editor");
        const QString nameBefore = nameField->text();
        QVERIFY2(mpHost->getTriggerUnit()->findTrigger(nameBefore), "The new trigger is not registered under the name in the field");

        const QString typedName = qsl("DialogTeardown trigger");
        focusWithText(nameField, typedName);

        delete editor;
        QVERIFY2(mpHost->mpEditorDialog.isNull(), "Editor should have been destroyed");
        // slot_saveProperty_TriggerName() renames the trigger itself, so the item
        // shows whether it ran while the editor was being destroyed
        QVERIFY2(!mpHost->getTriggerUnit()->findTrigger(typedName), "Being destroyed made the editor rename the trigger");
        QVERIFY2(mpHost->getTriggerUnit()->findTrigger(nameBefore), "The trigger lost its name while the editor was destroyed");
    }

    void test_protocolTogglesFireAfterPreferencesReopen()
    {
        mudlet::self()->showOptionsDialog(qsl("tab_general"), mpHost);
        QTest::qWait(100ms);
        auto* first = mpHost->mpDlgProfilePreferences.data();
        QVERIFY2(first, "Preferences dialog was not created");
        delete first;
        QVERIFY2(mpHost->mpDlgProfilePreferences.isNull(), "Preferences dialog should have been destroyed");

        mudlet::self()->showOptionsDialog(qsl("tab_general"), mpHost);
        QTest::qWait(100ms);
        auto* preferences = mpHost->mpDlgProfilePreferences.data();
        QVERIFY2(preferences, "Preferences dialog was not recreated");

        auto* gmcpCheckBox = preferences->findChild<QCheckBox*>(qsl("checkBox_enableGMCP"));
        QVERIFY2(gmcpCheckBox, "GMCP protocol checkbox not found under the reopened dialog - the protocols subpage was not built or not populated");

        // buildProtocolsSubpage() wires GMCP's toggled() to this button's
        // setEnabled(), so the button flipping proves the fresh dialog's
        // checkbox is connected - and that the connection was made exactly once
        const bool enabledBefore = preferences->pushButton_forgetSavedSignIn->isEnabled();
        QCOMPARE(enabledBefore, gmcpCheckBox->isChecked());
        gmcpCheckBox->toggle();
        QCOMPARE(preferences->pushButton_forgetSavedSignIn->isEnabled(), !enabledBefore);
        gmcpCheckBox->toggle();
        QCOMPARE(preferences->pushButton_forgetSavedSignIn->isEnabled(), enabledBefore);

        delete preferences;
    }
};

#include "DialogTeardownTest.moc"
MUDLET_GROUPED_TEST_MAIN(DialogTeardownTest)
