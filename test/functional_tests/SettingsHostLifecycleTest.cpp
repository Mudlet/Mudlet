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
 * The settings can be opened with no profile loaded, and a profile can appear or
 * go away underneath an open dialog. Four hand-maintained lists of widget
 * pointers grey the profile settings out and back in again; the shell moves every
 * one of those widgets onto a different page, so a reparenting mistake breaks
 * them silently - a control on the wrong page still looks fine.
 *
 * Repopulating for a profile that has just appeared also has to stay under the
 * populate guard, or writing several hundred controls would schedule an apply.
 *
 * Run with: ctest -R SettingsHostLifecycleTest -V
 */

#include <QDir>
#include <QTemporaryDir>
#include <QtTest/QtTest>

#include <QBoxLayout>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QKeySequenceEdit>
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

class SettingsHostLifecycleTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    TelnetServerStub* mpServer = nullptr;
    Host* mpHost = nullptr;
    dlgProfilePreferences* mpPreferences = nullptr;
    dlgProfilePreferences* mpSecondPreferences = nullptr;
    const QString mProfileName = qsl("SettingsHostLifecycle-Test");
    QString mPort; // assigned the stub's actual ephemeral port in initTestCase()
    const QString mLocalhost = qsl("localhost");


    static void deleteProfileDirectory(const QString& profileName) { TestSettings::deleteProfileDirectory(profileName); }


    // Takes the dialog to write by reference rather than returning it, because
    // QVERIFY expands to a bare return and so needs a void function - and the
    // exposure wait belongs here, where no case can leave it out
    void openPreferences(dlgProfilePreferences*& pDialog, Host* pHost)
    {
        pDialog = new dlgProfilePreferences(mudlet::self(), pHost);
        pDialog->resize(1060, 760);
        pDialog->show();
        QVERIFY(QTest::qWaitForWindowExposed(pDialog));
    }

    // Where every card on every category page sits - the search lends cards to
    // a results page, and a profile arriving while it is showing has to find
    // them all back home
    QStringList cardPlacements() const
    {
        QStringList placements;
        QStackedWidget* pStack = TestSettings::stack(mpPreferences);
        for (int page = 0, pages = pStack->count(); page < pages; ++page) {
            auto* pScrollArea = qobject_cast<QScrollArea*>(pStack->widget(page));
            if (!pScrollArea || pScrollArea->objectName() == qsl("settingsPage_searchResults")) {
                continue;
            }
            QWidget* pColumn = pScrollArea->widget();
            auto* pColumnLayout = qobject_cast<QBoxLayout*>(pColumn->layout());
            for (int item = 0, items = pColumnLayout->count(); item < items; ++item) {
                QWidget* pCard = pColumnLayout->itemAt(item)->widget();
                if (!pCard) {
                    continue;
                }
                placements << qsl("%1[%2] = %3, parented to %4").arg(pColumn->objectName(), QString::number(item), pCard->objectName(), pCard->parentWidget()->objectName());
            }
        }
        return placements;
    }

    // Everything a profile brings with it or fills up, counted in one string so
    // that a failure names which of them doubled rather than only that one did
    QString controlInventory() const
    {
        return qsl("%1 map symbol scaling spin box(es), %2 shortcut editor(s), %3 item(s) in the shortcuts grid, "
                   "%4 search engine(s), %5 log name format(s), %6 encoding(s), %7 dictionary/-ies, "
                   "%8 map save format(s), %9 map history entry/-ies")
                .arg(QString::number(mpPreferences->groupBox_mapSymbols->findChildren<QDoubleSpinBox*>().size()),
                     QString::number(mpPreferences->groupBox_main_window_shortcuts->findChildren<QKeySequenceEdit*>().size()),
                     QString::number(mpPreferences->gridLayout_groupBox_shortcuts->count()),
                     QString::number(mpPreferences->search_engine_combobox->count()),
                     QString::number(mpPreferences->comboBox_logFileNameFormat->count()),
                     QString::number(mpPreferences->comboBox_encoding->count()),
                     QString::number(mpPreferences->comboBox_dictionary->count()),
                     QString::number(mpPreferences->comboBox_mapFileSaveFormatVersion->count()),
                     QString::number(mpPreferences->comboBox_mapHistory->count()));
    }

    // What disableHostDetails() greys out, and what it deliberately leaves
    // alone because it is application-wide rather than a profile's
    void verifyProfileSettingsAre(const bool enabled)
    {
        QCOMPARE(mpPreferences->groupBox_logOptions->isEnabled(), enabled);
        QCOMPARE(mpPreferences->groupBox_input->isEnabled(), enabled);
        QCOMPARE(mpPreferences->groupBox_font->isEnabled(), enabled);
        QCOMPARE(mpPreferences->comboBox_encoding->isEnabled(), enabled);
        QCOMPARE(mpPreferences->groupBox_main_window_shortcuts->isEnabled(), enabled);
        // The three that disableHostDetails() has to name one by one:
        QCOMPARE(mpPreferences->doubleclick_ignore_lineedit->isEnabled(), enabled);
        QCOMPARE(mpPreferences->checkBox_enableOSC8Hyperlinks->isEnabled(), enabled);
        QCOMPARE(mpPreferences->checkBox_echoLuaErrors->isEnabled(), enabled);

        // ...and the application-wide settings stay usable either way
        QVERIFY2(mpPreferences->comboBox_appearance->isEnabled(), "the theme selector is not a profile setting and must stay usable");
        QVERIFY2(mpPreferences->groupBox_iconsAndToolbars->isEnabled(), "the icon and toolbar options are not profile settings and must stay usable");
        QVERIFY2(mpPreferences->comboBox_guiLanguage->isEnabled(), "the interface language is not a profile setting and must stay usable");
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
        delete mpSecondPreferences;
        mpSecondPreferences = nullptr;
        delete mpPreferences;
        mpPreferences = nullptr;
    }

    // Opening the settings from the profile chooser: nothing belonging to a
    // profile can be edited, but the application's own settings still can be
    void test_aDialogWithNoProfileGreysOutTheProfileSettings()
    {
        openPreferences(mpPreferences, nullptr);

        verifyProfileSettingsAre(false);
    }

    // The profile chooser's dialog is still up when a profile is opened, and
    // has to come to life around it - without that counting as an edit
    void test_aProfileAppearingEnablesTheDialogWithoutApplying()
    {
        openPreferences(mpPreferences, nullptr);
        verifyProfileSettingsAre(false);

        QSignalSpy applySpy(mpPreferences, &dlgProfilePreferences::signal_preferencesSaved);
        mpPreferences->slot_handleHostAddition(mpHost, 1);

        verifyProfileSettingsAre(true);
        QCOMPARE(mpPreferences->command_separator_lineedit->text(), mpHost->getCommandSeparator());
        QCOMPARE(mpPreferences->checkBox_highlightHistory->isChecked(), mpHost->mHighlightHistory);
        QVERIFY2(!applySpy.wait(TestSettings::scmQuietWindow), "repopulating the dialog for a profile that had just appeared applied the settings");
    }

    // ...and the mirror image: the profile is closed while its settings are
    // open, which puts the dialog back into the state above
    void test_aProfileGoingAwayGreysTheDialogOutAgain()
    {
        openPreferences(mpPreferences, nullptr);
        mpPreferences->slot_handleHostAddition(mpHost, 1);
        verifyProfileSettingsAre(true);

        QSignalSpy applySpy(mpPreferences, &dlgProfilePreferences::signal_preferencesSaved);
        mpPreferences->slot_handleHostDeletion(mpHost);

        verifyProfileSettingsAre(false);
        QVERIFY2(!applySpy.wait(TestSettings::scmQuietWindow), "clearing the dialog for a profile that had gone away applied the settings");
    }

    // One dialog per profile plus the profile chooser's own means two can be up
    // at once, and an application-wide setting changed in either has to reach
    // the other - which is what mudlet's settings signals are for
    void test_twoOpenDialogsStillSyncTheApplicationSettings()
    {
        openPreferences(mpPreferences, mpHost);
        openPreferences(mpSecondPreferences, nullptr);

        // applied live rather than through the debounce, so the second dialog
        // hears about it as the box is ticked
        const bool indicatorsBefore = mpPreferences->checkBox_showTabConnectionIndicators->isChecked();
        QCOMPARE(mpSecondPreferences->checkBox_showTabConnectionIndicators->isChecked(), indicatorsBefore);
        mpPreferences->checkBox_showTabConnectionIndicators->click();
        QCOMPARE(mpSecondPreferences->checkBox_showTabConnectionIndicators->isChecked(), !indicatorsBefore);

        // ...and one that only reaches the application when the debounce runs
        QSignalSpy applySpy(mpPreferences, &dlgProfilePreferences::signal_preferencesSaved);
        const int iconSizeBefore = mpPreferences->MainIconSize->value();
        const int iconSize = (iconSizeBefore % mpPreferences->MainIconSize->maximum()) + 1;
        QVERIFY(iconSize != iconSizeBefore);
        mpPreferences->MainIconSize->setValue(iconSize);
        QVERIFY2(TestSettings::waitForApply(applySpy), "the debounce never wrote the settings back");

        QCOMPARE(mpSecondPreferences->MainIconSize->value(), iconSize);
    }

    // applyShellStyle()'s palette fix-ups run at construction, so the controls a
    // profile brings with it when it appears afterwards have to be taken through
    // them too - the map symbol scaling factor is one of those controls.
    void test_controlsAProfileBringsWithItAreStyledLikeTheRest()
    {
        openPreferences(mpPreferences, nullptr);
        QVERIFY2(mpPreferences->groupBox_mapSymbols->findChildren<QDoubleSpinBox*>().isEmpty(),
                 "the map symbol scaling factor is there without a profile, so this case is not watching a control a profile brought with it");

        mpPreferences->slot_handleHostAddition(mpHost, 1);

        const auto fudgeBoxes = mpPreferences->groupBox_mapSymbols->findChildren<QDoubleSpinBox*>();
        QCOMPARE(fudgeBoxes.size(), 1);
        const QPalette shellStyled = mpPreferences->topBorderHeight->palette();
        QCOMPARE(fudgeBoxes.constFirst()->palette().color(QPalette::PlaceholderText), shellStyled.color(QPalette::PlaceholderText));
        QCOMPARE(fudgeBoxes.constFirst()->palette().color(QPalette::Window), shellStyled.color(QPalette::Window));
    }

    // The dialog outlives profiles, so everything it builds for one - the map
    // symbol scaling spin box, the shortcut editors - and every list it fills
    // from one has to be built and filled once however many profiles come and
    // go. Without that the second profile leaves a second set below the first,
    // laid out and visible and still wired to write through, and every list on
    // the page holds two of everything.
    void test_aProfileArrivingASecondTimeDoesNotDoubleTheDialogsControls()
    {
        openPreferences(mpPreferences, nullptr);
        mpPreferences->slot_handleHostAddition(mpHost, 1);
        const QString afterFirstProfile = controlInventory();

        QSignalSpy applySpy(mpPreferences, &dlgProfilePreferences::signal_preferencesSaved);
        mpPreferences->slot_handleHostDeletion(mpHost);
        mpPreferences->slot_handleHostAddition(mpHost, 1);

        QCOMPARE(controlInventory(), afterFirstProfile);
        QVERIFY2(!applySpy.wait(TestSettings::scmQuietWindow), "walking a profile out and back in again applied the settings");
    }

    // A profile can arrive while the search results are showing, and those
    // results hold cards that have been moved off their own pages. They have to
    // be home before anything walks those pages for the profile - which is what
    // repopulating for it does, several hundred controls at a time.
    void test_aProfileAppearingMidSearchSendsEveryCardHome()
    {
        openPreferences(mpPreferences, nullptr);
        QLineEdit* pSearch = mpPreferences->findChild<QLineEdit*>(qsl("settingsSearchField"));
        QVERIFY2(pSearch, "the settings shell has no search field");

        const QStringList before = cardPlacements();
        QVERIFY2(TestSettings::search(mpPreferences, qsl("color")), "the search never ran");
        QVERIFY2(cardPlacements() != before, "the search borrowed no cards at all, so sending them home proves nothing");

        QSignalSpy applySpy(mpPreferences, &dlgProfilePreferences::signal_preferencesSaved);
        mpPreferences->slot_handleHostAddition(mpHost, 1);

        QVERIFY2(pSearch->text().isEmpty(), "the profile arriving left the query standing in the search field");
        QCOMPARE(cardPlacements(), before);
        QVERIFY2(!applySpy.wait(TestSettings::scmQuietWindow), "repopulating for a profile that arrived mid-search applied the settings");
    }
};

#include "SettingsHostLifecycleTest.moc"
MUDLET_GROUPED_TEST_MAIN(SettingsHostLifecycleTest)
