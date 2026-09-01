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
 * The settings dialog stays open while scripts and the game move the settings on
 * underneath it. Coming back to its window is when it re-reads them.
 *
 * Re-reading is indistinguishable from discarding whatever it writes over, so
 * every case here asks the same question: whose value is on the control. The
 * dialog's own, and the settings win; the user's, and they do. And re-reading
 * must never turn into writing: a spin box shown a value it cannot hold exactly
 * must not answer with the rounded one.
 *
 * Run with: ctest -R SettingsLiveSyncTest -V
 */

#include <memory>

#include <QDir>
#include <QTemporaryDir>
#include <QtTest/QtTest>

#include <QCheckBox>
#include <QFrame>
#include <QLineEdit>
#include <QScrollArea>
#include <QSignalSpy>
#include <QSpinBox>
#include <QStackedWidget>

#include "PortableModeTestHelper.h"
#include "ProfileTestHelper.h"
#include "SettingsTestHelper.h"
#include "Host.h"
#include "MudletInstanceCoordinator.h"
#include "TelnetServerStub.h"
#include "dlgProfilePreferences.h"
#include "mudlet.h"

#include "GroupedTest.h"

class SettingsLiveSyncTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    QByteArray mSavedNoThemeDownload;
    TelnetServerStub* mpServer = nullptr;
    Host* mpHost = nullptr;
    dlgProfilePreferences* mpPreferences = nullptr;
    const QString mProfileName = qsl("SettingsLiveSync-Test");
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

    // Every list on the page that a profile fills, counted in one string so
    // that a failure names which of them grew rather than only that one did
    QString listLengths() const
    {
        return qsl("%1 search engine(s), %2 log name format(s), %3 encoding(s), %4 dictionary/-ies, "
                   "%5 map save format(s), %6 map history entry/-ies, %7 editor theme(s), %8 previewable script(s)")
                .arg(QString::number(mpPreferences->search_engine_combobox->count()),
                     QString::number(mpPreferences->comboBox_logFileNameFormat->count()),
                     QString::number(mpPreferences->comboBox_encoding->count()),
                     QString::number(mpPreferences->comboBox_dictionary->count()),
                     QString::number(mpPreferences->comboBox_mapFileSaveFormatVersion->count()),
                     QString::number(mpPreferences->comboBox_mapHistory->count()),
                     QString::number(mpPreferences->code_editor_theme_selection_combobox->count()),
                     QString::number(mpPreferences->script_preview_combobox->count()));
    }

    // The user coming back to the window. Sent rather than driven through the
    // window manager so that every case says the same thing on every platform;
    // test_theRealWindowActivationIsWhatCarriesIt drives the real one once, to
    // prove the handler is on the event a window manager actually delivers.
    void returnToTheDialog()
    {
        QEvent activation(QEvent::WindowActivate);
        QCoreApplication::sendEvent(mpPreferences, &activation);
        QCoreApplication::processEvents();
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
        // Re-reading the settings takes the dialog through the Editor page's
        // theme list every time, and that is the one population step that can
        // reach the network
        mSavedNoThemeDownload = qgetenv("MUDLET_TEST_NO_THEME_DOWNLOAD");
        qputenv("MUDLET_TEST_NO_THEME_DOWNLOAD", "1");

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
        mSavedNoThemeDownload.isNull() ? qunsetenv("MUDLET_TEST_NO_THEME_DOWNLOAD") : qputenv("MUDLET_TEST_NO_THEME_DOWNLOAD", mSavedNoThemeDownload);
    }

    void cleanup()
    {
        delete mpPreferences;
        mpPreferences = nullptr;
    }

    // The dialog was populated when it opened; a script has moved two settings
    // on since. Coming back to the window is what it takes to see them.
    void test_aSettingChangedElsewhereShowsWhenTheUserComesBackToTheDialog()
    {
        openPreferences();
        QCOMPARE(mpPreferences->wrap_at_spinBox->value(), mpHost->mWrapAt);

        QSignalSpy applySpy(mpPreferences, &dlgProfilePreferences::signal_preferencesSaved);
        const int wrapFromAScript = mpHost->mWrapAt + 23;
        mpHost->mWrapAt = wrapFromAScript;
        mpHost->mCommandSeparator = qsl("!!");
        QVERIFY2(mpPreferences->wrap_at_spinBox->value() != wrapFromAScript, "the dialog showed the new value before it was told to look again");

        returnToTheDialog();

        QCOMPARE(mpPreferences->wrap_at_spinBox->value(), wrapFromAScript);
        QCOMPARE(mpPreferences->command_separator_lineedit->text(), qsl("!!"));
        QVERIFY2(!applySpy.wait(TestSettings::scmQuietWindow), "re-reading the settings applied them");
    }

    // ...and it is the activation a window manager delivers that does it, not
    // only the event this file synthesises for the cases above
    void test_theRealWindowActivationIsWhatCarriesIt()
    {
        openPreferences();
        // Something else has to hold the activation first, or activating the
        // dialog it already has changes nothing and delivers no event
        auto pOther = std::make_unique<QWidget>();
        pOther->resize(200, 200);
        pOther->show();
        QVERIFY(QTest::qWaitForWindowExposed(pOther.get()));
        pOther->activateWindow();
        if (!QTest::qWaitForWindowActive(pOther.get())) {
            QSKIP("this platform does not hand window activation around, so the real path cannot be driven here");
        }

        const int wrapFromAScript = mpHost->mWrapAt + 17;
        mpHost->mWrapAt = wrapFromAScript;

        mpPreferences->activateWindow();
        QVERIFY(QTest::qWaitForWindowActive(mpPreferences));
        QCoreApplication::processEvents();

        QCOMPARE(mpPreferences->wrap_at_spinBox->value(), wrapFromAScript);
    }

    // The user changed a control and walked away inside the 400ms debounce.
    // What they left on the control is theirs until it has been written, so
    // coming back must not replace it with the value it is about to overwrite.
    void test_anEditWaitingToBeAppliedIsNotOverwritten()
    {
        openPreferences();

        QSignalSpy applySpy(mpPreferences, &dlgProfilePreferences::signal_preferencesSaved);
        const int wrapTheUserTyped = mpHost->mWrapAt + 5;
        mpPreferences->wrap_at_spinBox->setValue(wrapTheUserTyped);
        mpHost->mWrapAt = wrapTheUserTyped + 40;

        returnToTheDialog();

        QCOMPARE(mpPreferences->wrap_at_spinBox->value(), wrapTheUserTyped);
        QVERIFY2(TestSettings::waitForApply(applySpy), "the debounce never wrote the settings back");
        QCOMPARE(mpHost->mWrapAt, wrapTheUserTyped);
    }

    // The same thing one keystroke at a time: a field being typed into is not
    // "dirty" as far as the apply is concerned - that is what stops a shared
    // debounce committing half a word - so it has to be asked about separately
    void test_aWordHalfTypedSurvivesTheUserComingBack()
    {
        openPreferences();

        QLineEdit* pSeparator = mpPreferences->command_separator_lineedit;
        pSeparator->setFocus();
        QVERIFY2(pSeparator->hasFocus(), "the command separator field could not be given the focus, so nothing here is mid-edit");
        QTest::keyClicks(pSeparator, qsl("::"));
        QVERIFY(pSeparator->isModified());
        const QString halfTyped = pSeparator->text();
        mpHost->mCommandSeparator = qsl("&&");
        QVERIFY(halfTyped != mpHost->mCommandSeparator);

        returnToTheDialog();

        QCOMPARE(pSeparator->text(), halfTyped);
    }

    // Re-reading a setting must not be a way of writing it. The room size is a
    // qreal shown on a 1-11 integer scale, so a value a script set between two
    // of those steps is exactly what a control answering its own population
    // would round away.
    void test_readingASettingBackDoesNotWriteTheRoundedValueOut()
    {
        mpHost->mRoomSize = 0.5;
        openPreferences();
        QCOMPARE(mpPreferences->spinBox_roomSize->value(), 5);

        QSignalSpy applySpy(mpPreferences, &dlgProfilePreferences::signal_preferencesSaved);
        // Between the 7 and the 8 the scale offers, so the step the box moves
        // to is not the value it was moved there by
        const double sizeFromAScript = 0.75;
        mpHost->mRoomSize = sizeFromAScript;

        returnToTheDialog();

        QCOMPARE(mpPreferences->spinBox_roomSize->value(), 7);
        QCOMPARE(mpHost->mRoomSize, sizeFromAScript);
        QVERIFY2(!applySpy.wait(TestSettings::scmQuietWindow), "re-reading the settings applied them");
    }

    // An apply is the other moment nothing of the user's is outstanding, so the
    // settings are re-read once it has finished - here, a change made from a
    // script while the debounce for an unrelated edit was running
    void test_theSettingsAreRereadOnceAnApplyHasFinished()
    {
        openPreferences();

        QSignalSpy applySpy(mpPreferences, &dlgProfilePreferences::signal_preferencesSaved);
        mpPreferences->checkBox_echoLuaErrors->click();
        const int wrapFromAScript = mpHost->mWrapAt + 31;
        mpHost->mWrapAt = wrapFromAScript;

        QVERIFY2(TestSettings::waitForApply(applySpy), "the debounce never wrote the settings back");
        QCoreApplication::processEvents();

        QCOMPARE(mpPreferences->wrap_at_spinBox->value(), wrapFromAScript);
        // ...and the edit that started the apply still reached the profile
        QCOMPARE(mpHost->mEchoLuaErrors, mpPreferences->checkBox_echoLuaErrors->isChecked());
    }

    // Re-reading the settings walks the same population that fills the lists a
    // profile decides the contents of. Nothing clears them in between - the
    // host-swap path has clearHostDetails() for that and this one has nothing -
    // so each of those lists has to be rebuilt rather than added to.
    void test_readingTheSettingsBackDoesNotGrowTheDialogsLists()
    {
        openPreferences();
        const QString whenOpened = listLengths();

        returnToTheDialog();
        returnToTheDialog();

        QCOMPARE(listLengths(), whenOpened);
    }

    // A subpage is a page like any other as far as the stack is concerned, but
    // it is the one the sidebar cannot get back to, so a repopulation that
    // navigated anywhere would strand the user on the parent category
    void test_aSubpageIsStillShowingAfterTheSettingsAreReread()
    {
        openPreferences();
        mpPreferences->setTab(qsl("connection/protocols"));
        QCoreApplication::processEvents();
        auto* pStack = TestSettings::stack(mpPreferences);
        QVERIFY(pStack);
        QCOMPARE(pStack->currentWidget()->objectName(), qsl("settingsPage_connection_protocols"));

        const bool msspFromAScript = !mpHost->mEnableMSSP;
        mpHost->mEnableMSSP = msspFromAScript;
        returnToTheDialog();

        QCOMPARE(pStack->currentWidget()->objectName(), qsl("settingsPage_connection_protocols"));
        auto* pMssp = mpPreferences->findChild<QCheckBox*>(qsl("checkBox_enableMSSP"));
        QVERIFY2(pMssp, "the MSSP checkbox is not on the protocols subpage");
        QCOMPARE(pMssp->isChecked(), msspFromAScript);
    }

    // The banner belongs to no category: it is lent to the top of whichever
    // page is showing. Re-reading the settings must neither take it off that
    // page nor leave a second one behind it.
    void test_theMigrationBannerStaysWhereItWasAfterTheSettingsAreReread()
    {
        openPreferences();
        QFrame* pBanner = mpPreferences->findChild<QFrame*>(qsl("settingsMigrationBanner"));
        QVERIFY2(pBanner, "the migration banner was not built, so this case is watching nothing");
        auto* pStack = TestSettings::stack(mpPreferences);
        QWidget* pColumn = qobject_cast<QScrollArea*>(pStack->currentWidget())->widget();
        QCOMPARE(pColumn->layout()->indexOf(pBanner), 0);

        const int wrapFromAScript = mpHost->mWrapAt + 11;
        mpHost->mWrapAt = wrapFromAScript;
        returnToTheDialog();
        QCOMPARE(mpPreferences->wrap_at_spinBox->value(), wrapFromAScript); // ...so the refresh this is watching did happen

        QCOMPARE(mpPreferences->findChildren<QFrame*>(qsl("settingsMigrationBanner")).size(), 1);
        QVERIFY2(pBanner->isVisible(), "re-reading the settings hid the migration banner");
        QCOMPARE(pColumn->layout()->indexOf(pBanner), 0);
    }

    // A deep link outlines the card it meant for a second or two. The pulse is
    // an overlay parented to the column the re-read then re-measures, so this
    // is the one moment where re-reading meets something animating over it.
    void test_aSpotlightPulseSurvivesTheSettingsBeingReread()
    {
        openPreferences();
        mpPreferences->setTab(qsl("mapper/groupBox_playerRoomStyle"));
        QVERIFY2(QTest::qWaitFor(
                         [this]() {
                             return mpPreferences->findChild<QWidget*>(qsl("settingsSpotlight")) != nullptr;
                         },
                         2000),
                 "the deep link never spotlighted anything, so this case is watching nothing");

        const int wrapFromAScript = mpHost->mWrapAt + 13;
        mpHost->mWrapAt = wrapFromAScript;
        returnToTheDialog();

        QCOMPARE(mpPreferences->wrap_at_spinBox->value(), wrapFromAScript);
        QVERIFY2(mpPreferences->findChild<QWidget*>(qsl("settingsSpotlight")), "re-reading the settings took the spotlight off the card mid-pulse");
        // ...and it still ends by itself rather than being left on the page
        QVERIFY2(QTest::qWaitFor(
                         [this]() {
                             return mpPreferences->findChild<QWidget*>(qsl("settingsSpotlight")) == nullptr;
                         },
                         5000),
                 "the spotlight outlived its pulse once the settings had been re-read");
    }

    // A query is an interaction in progress like any other. Re-reading the
    // settings replaces the text the search index was built from, and the only
    // honest way to fix that up is to clear the query - which is the user's.
    void test_aQueryStandingInTheSearchFieldSurvivesTheUserComingBack()
    {
        openPreferences();
        QLineEdit* pSearch = mpPreferences->findChild<QLineEdit*>(qsl("settingsSearchField"));
        QVERIFY2(pSearch, "the settings shell has no search field");
        auto* pStack = TestSettings::stack(mpPreferences);
        QVERIFY(pStack);

        QVERIFY2(TestSettings::search(mpPreferences, qsl("color")), "the search never ran");
        const int resultsPage = pStack->currentIndex();
        QCOMPARE(pStack->widget(resultsPage)->objectName(), qsl("settingsPage_searchResults"));

        mpHost->mWrapAt = mpHost->mWrapAt + 7;
        returnToTheDialog();

        QCOMPARE(pSearch->text(), qsl("color"));
        QCOMPARE(pStack->currentIndex(), resultsPage);
    }
};

#include "SettingsLiveSyncTest.moc"
MUDLET_GROUPED_TEST_MAIN(SettingsLiveSyncTest)
