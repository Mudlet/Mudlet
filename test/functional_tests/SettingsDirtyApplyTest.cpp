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
 * Issue #10165: a script changes a setting while the settings dialog is open,
 * and the dialog's next write-back reverts it - because it wrote every control
 * back, stale values included. Instant apply makes that happen on any edit at
 * all rather than only when Save is pressed, so applyAll() now writes only the
 * settings whose controls the user really changed.
 *
 * The mechanism behind that is a value snapshot taken after population and
 * again after each apply. Both halves are covered here: the snapshot exists at
 * all, and it is retaken - a snapshot that went stale after the first apply
 * would go on treating a control the user once touched as changed for the rest
 * of the session, and revert whatever a script did to that setting afterwards.
 * mValueSnapshot itself is private, so all of this goes through behaviour.
 *
 * Run with: ctest -R SettingsDirtyApplyTest -V
 */

#include <QDir>
#include <QFileInfo>
#include <QMargins>
#include <QTemporaryDir>
#include <QtTest/QtTest>

#include <QAction>
#include <QCheckBox>
#include <QComboBox>
#include <QLineEdit>
#include <QListWidget>
#include <QMenu>
#include <QSettings>
#include <QSignalSpy>
#include <QSpinBox>
#include <QToolButton>

#include "PortableModeTestHelper.h"
#include "ProfileTestHelper.h"
#include "Host.h"
#include "MudletInstanceCoordinator.h"
#include "ShortcutsManager.h"
#include "TelnetServerStub.h"
#include "dlgProfilePreferences.h"
#include "mudlet.h"

#include "GroupedTest.h"

class SettingsDirtyApplyTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    // The Editor page's theme refresh creates the cache directory before it
    // checks whether it has anything to fetch, so it runs on the cached branch
    // too - and without this it would be the developer's own ~/.cache
    QTemporaryDir mCacheDir;
    QByteArray mSavedXdgCache;
    TelnetServerStub* mpServer = nullptr;
    Host* mpHost = nullptr;
    dlgProfilePreferences* mpPreferences = nullptr;
    const QString mProfileName = qsl("SettingsDirtyApply-Test");
    QString mPort; // assigned the stub's actual ephemeral port in initTestCase()
    const QString mLocalhost = qsl("localhost");

    static constexpr int scmApplyTimeout = 10000;

    // The borders as this test found them, put back after each case
    QMargins mBordersBefore;
    bool mAnnounceBefore = false;
    bool mTextAnalyzerBefore = false;
    QString mLogDirBefore;
    QString mEditorThemeBefore;
    QString mEditorThemeDarkBefore;
    QMap<QString, QKeySequence> mShortcutsBefore;

    void deleteProfileDirectory(const QString& profileName)
    {
        QDir dir(mudlet::getMudletPath(enums::profileHomePath, profileName));
        if (dir.exists()) {
            dir.removeRecursively();
        }
    }

    static bool waitForApply(QSignalSpy& spy) { return !spy.isEmpty() || spy.wait(scmApplyTimeout); }

    // The themes the editor page offers are read from this file. Written fresh
    // each time, which is also what keeps the first visit to the Editor page on
    // maybeDownloadEditorThemes()'s cached branch instead of a network fetch.
    static void writeEditorThemesFile(const QByteArray& contents)
    {
        const QString file = mudlet::getMudletPath(enums::editorWidgetThemeJsonFile);
        QVERIFY(QDir().mkpath(QFileInfo(file).absolutePath()));
        QFile themes(file);
        QVERIFY(themes.open(QIODevice::WriteOnly | QIODevice::Truncate));
        QVERIFY(themes.write(contents) == contents.size());
    }

    void selectCategory(const QString& key)
    {
        QListWidget* pList = mpPreferences->findChild<QListWidget*>(qsl("settingsCategoryList"));
        QVERIFY2(pList, "the settings shell has no category sidebar");
        for (int row = 0, rows = pList->count(); row < rows; ++row) {
            if (pList->item(row)->data(Qt::UserRole).toString() == key) {
                pList->setCurrentRow(row);
                QCoreApplication::processEvents();
                return;
            }
        }
        QFAIL(qPrintable(qsl("no sidebar item for category '%1'").arg(key)));
    }

    void openPreferences()
    {
        mpPreferences = new dlgProfilePreferences(mudlet::self(), mpHost);
        mpPreferences->resize(1060, 760);
        mpPreferences->show();
        QVERIFY(QTest::qWaitForWindowExposed(mpPreferences));
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
        QVERIFY(mCacheDir.isValid());
        mSavedXdgCache = qgetenv("XDG_CACHE_HOME");
        qputenv("XDG_CACHE_HOME", mCacheDir.path().toUtf8());

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
        mSavedXdgCache.isNull() ? qunsetenv("XDG_CACHE_HOME") : qputenv("XDG_CACHE_HOME", mSavedXdgCache);
    }

    void init()
    {
        mBordersBefore = mpHost->userBorders();
        mAnnounceBefore = mpHost->mAnnounceIncomingText;
        mTextAnalyzerBefore = mpHost->mEnableTextAnalyzer;
        mLogDirBefore = mpHost->mLogDir;
        mEditorThemeBefore = mpHost->mEditorTheme;
        mEditorThemeDarkBefore = mpHost->mEditorThemeDark;
        mShortcutsBefore.clear();
        for (const auto& [key, pSequence] : mpHost->profileShortcuts) {
            mShortcutsBefore.insert(key, *pSequence);
        }
    }

    void cleanup()
    {
        // The whole class shares one profile, so a case that leaves the dialog
        // open or a setting changed would be writing the next one's fixture
        delete mpPreferences;
        mpPreferences = nullptr;
        mpHost->setUserBorders(mBordersBefore);
        mpHost->mAnnounceIncomingText = mAnnounceBefore;
        mpHost->mEnableTextAnalyzer = mTextAnalyzerBefore;
        mpHost->mLogDir = mLogDirBefore;
        mpHost->mEditorTheme = mEditorThemeBefore;
        mpHost->mEditorThemeDark = mEditorThemeDarkBefore;
        for (const auto& [key, pSequence] : mpHost->profileShortcuts) {
            *pSequence = mShortcutsBefore.value(key);
        }
    }

    // The #10165 scenario itself: a script sets a border while the settings are
    // open, the user then ticks something on another page, and the border must
    // still be what the script set once the debounce has run.
    void test_anExternalChangeSurvivesAnUnrelatedEdit()
    {
        mpHost->setUserBorders(QMargins(0, 0, 0, 0));
        openPreferences();
        QCOMPARE(mpPreferences->topBorderHeight->value(), 0);

        // what `lua setBorderTop(50)` does to the Host
        const int setElsewhere = 50;
        mpHost->setUserBorders(QMargins(0, setElsewhere, 0, 0));
        QVERIFY2(mpPreferences->topBorderHeight->value() != setElsewhere, "the spin box already showed the border the script set, so writing it back could not lose anything");

        QSignalSpy applySpy(mpPreferences, &dlgProfilePreferences::signal_preferencesSaved);
        const bool announceBefore = mpHost->mAnnounceIncomingText;
        mpPreferences->checkBox_announceIncomingText->click();
        QVERIFY2(waitForApply(applySpy), "the debounce never wrote the settings back");

        QCOMPARE(mpHost->userBorders().top(), setElsewhere);
        QCOMPARE(mpHost->mAnnounceIncomingText, !announceBefore);
    }

    // ...and the snapshot the guard compares against is retaken on every apply.
    // If it were not, the checkbox this case ticks would count as changed for
    // the rest of the session, and the next apply would put its value back over
    // whatever a script had set in the meantime.
    void test_theDirtySetIsClearedAfterEachApply()
    {
        openPreferences();

        QSignalSpy applySpy(mpPreferences, &dlgProfilePreferences::signal_preferencesSaved);
        const bool announceBefore = mpHost->mAnnounceIncomingText;
        mpPreferences->checkBox_announceIncomingText->click();
        QVERIFY2(waitForApply(applySpy), "the first edit never reached the Host");
        QCOMPARE(mpHost->mAnnounceIncomingText, !announceBefore);

        // now a script puts that same setting back, with the control still
        // showing what the user chose
        mpHost->mAnnounceIncomingText = announceBefore;
        QCOMPARE(mpPreferences->checkBox_announceIncomingText->isChecked(), !announceBefore);

        applySpy.clear();
        const bool analyzerBefore = mpHost->mEnableTextAnalyzer;
        mpPreferences->checkBox_enableTextAnalyzer->click();
        QVERIFY2(waitForApply(applySpy), "the second edit never reached the Host");

        QCOMPARE(mpHost->mEnableTextAnalyzer, !analyzerBefore);
        QVERIFY2(mpHost->mAnnounceIncomingText == announceBefore, "an apply wrote back a control the user had changed before the previous apply, reverting what was set from outside");
    }

    // The other half of the guard: a control the user really did edit still has
    // to be written, or dirty-aware apply would simply be no apply at all
    void test_theUsersOwnBorderEditIsStillWritten()
    {
        mpHost->setUserBorders(QMargins(0, 0, 0, 0));
        openPreferences();

        QSignalSpy applySpy(mpPreferences, &dlgProfilePreferences::signal_preferencesSaved);
        const int typedIn = 77;
        mpPreferences->topBorderHeight->setValue(typedIn);
        QVERIFY2(waitForApply(applySpy), "editing the border never wrote the settings back");

        QCOMPARE(mpHost->userBorders().top(), typedIn);
    }

    // The log folder is picked with a button and shown in a read-only line edit,
    // so nothing about that exchange emits a signal instant apply listens to:
    // unasked for, the write waits on an unrelated edit that may never come.
    void test_resettingTheLogFolderIsApplied()
    {
        mpHost->mLogDir = QDir::tempPath();
        openPreferences();
        QCOMPARE(mpPreferences->lineEdit_logFileFolder->text(), QDir::tempPath());

        QSignalSpy applySpy(mpPreferences, &dlgProfilePreferences::signal_preferencesSaved);
        mpPreferences->slot_resetLogDir();
        QVERIFY2(waitForApply(applySpy), "resetting the log folder never wrote the settings back");
        QCOMPARE(mpHost->mLogDir, QString());
    }

    // Same shape for the shortcuts page: the reset button writes through
    // currentShortcuts rather than through a control, and it is a tool button,
    // which instant apply deliberately does not listen to.
    void test_resettingTheShortcutsToDefaultsIsApplied()
    {
        auto keys = mudlet::self()->mpShortcutsManager->iterator();
        QVERIFY2(keys.hasNext(), "no main window shortcuts are registered, so resetting them proves nothing");
        const QString key = keys.next();
        const QKeySequence defaultSequence = *mudlet::self()->mpShortcutsManager->getDefault(key);
        // A binding no default uses, so that resetting has to change it back
        const QKeySequence changed(qsl("Ctrl+Alt+Shift+F12"));
        QVERIFY(changed != defaultSequence);
        const auto it = mpHost->profileShortcuts.find(key);
        QVERIFY2(it != mpHost->profileShortcuts.end(), "the profile carries no sequence for the first registered shortcut");
        *it->second = changed;

        openPreferences();
        QSignalSpy applySpy(mpPreferences, &dlgProfilePreferences::signal_preferencesSaved);
        mpPreferences->toolButton_resetMainWindowShortcuts->click();
        QVERIFY2(waitForApply(applySpy), "resetting the shortcuts to their defaults never wrote them back");
        QCOMPARE(*mpHost->profileShortcuts.at(key), defaultSequence);
    }

    // A refresh that no longer offers the profile's theme leaves the box on no
    // item at all. Read as an edit, writing that back would wipe the theme the
    // profile had.
    void test_aThemeListRefreshDoesNotWipeTheChosenTheme()
    {
        writeEditorThemesFile(R"([{"Title": "PhaseDProbe", "FileName": "PhaseDProbe.tmTheme"}])");
        const bool dark = mudlet::self()->inDarkMode();
        (dark ? mpHost->mEditorThemeDark : mpHost->mEditorTheme) = qsl("PhaseDProbe");
        openPreferences();
        QCOMPARE(mpPreferences->code_editor_theme_selection_combobox->currentText(), qsl("PhaseDProbe"));

        // ...and the refresh no longer offers it
        writeEditorThemesFile("[]");
        selectCategory(qsl("editor"));
        QCOMPARE(mpPreferences->code_editor_theme_selection_combobox->currentIndex(), -1);

        QSignalSpy applySpy(mpPreferences, &dlgProfilePreferences::signal_preferencesSaved);
        mpPreferences->checkBox_announceIncomingText->click();
        QVERIFY2(waitForApply(applySpy), "the debounce never wrote the settings back");

        QCOMPARE(dark ? mpHost->mEditorThemeDark : mpHost->mEditorTheme, qsl("PhaseDProbe"));
    }

    // The protocol menu's QActions are snapshotted like any control, so a
    // protocol a script turned on is not written back over by the next edit
    // somewhere else in the dialog.
    void test_anExternalProtocolChangeSurvivesAnUnrelatedEdit()
    {
        openPreferences();
        QMenu* pMenu = mpPreferences->pushButton_chooseProtocols->menu();
        QVERIFY2(pMenu, "the Choose protocols button has no menu");
        QAction* pMsp = nullptr;
        for (auto* pAction : pMenu->actions()) {
            if (pAction->text().startsWith(qsl("MSP"))) {
                pMsp = pAction;
                break;
            }
        }
        QVERIFY2(pMsp, "the protocol menu has no MSP entry");

        // what `lua enableMSP()` would do, with the menu still showing the old
        // state
        const bool shown = pMsp->isChecked();
        mpHost->mEnableMSP = !shown;

        QSignalSpy applySpy(mpPreferences, &dlgProfilePreferences::signal_preferencesSaved);
        mpPreferences->checkBox_announceIncomingText->click();
        QVERIFY2(waitForApply(applySpy), "the debounce never wrote the settings back");

        QCOMPARE(mpHost->mEnableMSP, !shown);
        mpHost->mEnableMSP = shown;
    }

    // Dirty-aware apply covers the settings that live in Mudlet's own QSettings
    // rather than in the profile, and those have no signal back to the dialog -
    // so the control goes on showing the old value while the setting has moved.
    void test_anExternalGlobalSettingSurvivesAnUnrelatedEdit()
    {
        openPreferences();
        QSettings* pSettings = mudlet::getQSettings();
        const bool shown = mpPreferences->telnetHandlerEnabled->isChecked();
        // what another profile's settings dialog, or a hand-edited Mudlet.ini,
        // leaves behind
        pSettings->setValue(qsl("telnetHandlerEnabled"), !shown);

        QSignalSpy applySpy(mpPreferences, &dlgProfilePreferences::signal_preferencesSaved);
        mpPreferences->checkBox_announceIncomingText->click();
        QVERIFY2(waitForApply(applySpy), "the debounce never wrote the settings back");

        QCOMPARE(pSettings->value(qsl("telnetHandlerEnabled")).toBool(), !shown);
        pSettings->setValue(qsl("telnetHandlerEnabled"), shown);
    }
};

#include "SettingsDirtyApplyTest.moc"
MUDLET_GROUPED_TEST_MAIN(SettingsDirtyApplyTest)
