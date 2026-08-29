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
 * "Find in settings" *lends* the matching cards to a results page: they are
 * moved out of their own category page and put back at the (layout, index) they
 * were indexed at once the query ends.
 *
 * That restoration is the risk the feature carries - a card that does not come
 * home silently loses settings from the page they belong on, and nothing else
 * would notice - so the central case walks every page before and after a search.
 *
 * Run with: ctest -R SettingsSearchTest -V
 */

#include <QDir>
#include <QTemporaryDir>
#include <QtTest/QtTest>

#include <QBoxLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QScrollArea>
#include <QStackedWidget>
#include <QTimer>
#include <QToolButton>

#include "PortableModeTestHelper.h"
#include "ProfileTestHelper.h"
#include "SettingsTestHelper.h"
#include "Host.h"
#include "MudletInstanceCoordinator.h"
#include "TelnetServerStub.h"
#include "dlgProfilePreferences.h"
#include "mudlet.h"

#include "GroupedTest.h"

class SettingsSearchTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    TelnetServerStub* mpServer = nullptr;
    Host* mpHost = nullptr;
    dlgProfilePreferences* mpPreferences = nullptr;
    const QString mProfileName = qsl("SettingsSearch-Test");
    QString mPort; // assigned the stub's actual ephemeral port in initTestCase()
    const QString mLocalhost = qsl("localhost");

    static void deleteProfileDirectory(const QString& profileName) { TestSettings::deleteProfileDirectory(profileName); }

    QListWidget* sidebar() const { return TestSettings::sidebar(mpPreferences); }

    QStackedWidget* stack() const { return TestSettings::stack(mpPreferences); }
    QScrollArea* pageOf(const QString& key) const { return TestSettings::pageOf(mpPreferences, key); }

    QLineEdit* searchField() const { return mpPreferences->findChild<QLineEdit*>(qsl("settingsSearchField")); }

    QWidget* resultsColumn() const { return mpPreferences->findChild<QWidget*>(qsl("settingsColumn_searchResults")); }

    void search(const QString& query) { QVERIFY2(TestSettings::search(mpPreferences, query), "the search never ran"); }

    // Where every card on every category page sits, and whether it is showing,
    // as text so that a mismatch reads as a diff rather than as two pointers
    QStringList cardPlacements() const
    {
        QStringList placements;
        QStackedWidget* pStack = stack();
        for (int page = 0, pages = pStack->count(); page < pages; ++page) {
            auto* pScrollArea = qobject_cast<QScrollArea*>(pStack->widget(page));
            if (!pScrollArea || pScrollArea->objectName() == qsl("settingsPage_searchResults")) {
                continue;
            }
            QWidget* pColumn = pScrollArea->widget();
            auto* pColumnLayout = qobject_cast<QBoxLayout*>(pColumn->layout());
            // Counted over the cards rather than over the layout, because the
            // migration banner rides at the top of whichever page is showing:
            // it is not a card any page is meant to get back, and its coming
            // and going would otherwise shift every index under it
            int cardPosition = 0;
            for (int item = 0, items = pColumnLayout->count(); item < items; ++item) {
                QWidget* pCard = pColumnLayout->itemAt(item)->widget();
                if (!pCard || pCard->objectName() == qsl("settingsMigrationBanner")) {
                    continue;
                }
                placements
                        << qsl("%1[%2] = %3, parented to %4, hidden %5")
                                   .arg(pColumn->objectName(), QString::number(cardPosition++), pCard->objectName(), pCard->parentWidget()->objectName(), pCard->isHidden() ? qsl("yes") : qsl("no"));
            }
        }
        return placements;
    }

    int visibleCategoryHeaders() const
    {
        int headers = 0;
        for (const auto* pLabel : mpPreferences->findChildren<QLabel*>(qsl("settingsSearchHeader"))) {
            if (pLabel->isVisible()) {
                ++headers;
            }
        }
        return headers;
    }

    QStringList highlightedWidgets() const
    {
        QStringList highlighted;
        for (const auto* pWidget : mpPreferences->findChildren<QWidget*>()) {
            if (pWidget->property("searchMatch").toBool()) {
                highlighted << pWidget->objectName();
            }
        }
        return highlighted;
    }

    void openPreferences()
    {
        mpPreferences = new dlgProfilePreferences(mudlet::self(), mpHost);
        mpPreferences->resize(1060, 760);
        mpPreferences->show();
        QVERIFY(QTest::qWaitForWindowExposed(mpPreferences));
        QVERIFY2(searchField(), "the settings shell has no search field");
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

    void init() { openPreferences(); }

    void cleanup()
    {
        delete mpPreferences;
        mpPreferences = nullptr;
    }

    // The invariant behind lending cards out: after any number of searches,
    // every page holds exactly the cards it held before, in the same order and
    // parented to the same column
    void test_searchingAndClearingLeavesEveryCardWhereItWas()
    {
        const QStringList before = cardPlacements();
        QVERIFY2(before.size() > 12, "fewer cards were found than there are categories, so this case is not walking the pages");

        search(qsl("color"));
        QVERIFY2(!resultsColumn()->findChildren<QGroupBox*>().isEmpty(), "the search borrowed no cards at all, so putting them back proves nothing");
        search(qsl("gmcp"));
        search(qsl("proxy"));
        search(QString());

        QCOMPARE(cardPlacements(), before);
    }

    // Results are cross-category and grouped under the category they came from,
    // which is the half of the feature a per-page filter would not give
    void test_aSearchShowsMatchesFromMoreThanOneCategory()
    {
        search(qsl("color"));

        QCOMPARE(stack()->currentWidget(), pageOf(qsl("searchResults")));
        QCOMPARE(mpPreferences->groupBox_displayColors->parentWidget(), resultsColumn());
        QCOMPARE(mpPreferences->groupBox_mapperColors->parentWidget(), resultsColumn());
        QVERIFY2(visibleCategoryHeaders() >= 2, "the results were not grouped under a header per category");
        QVERIFY2(mpPreferences->findChild<QLabel*>(qsl("settingsSearchEmpty"))->isHidden(), "a search that matched showed the empty state as well");
    }

    // The protocol names only exist inside the menu that button pops up, where
    // a walk over the widget tree cannot see them, so the button carries them
    // as invisible synonyms - and is what gets highlighted for them
    void test_aKeywordFindsWhatOnlyASubmenuNames()
    {
        search(qsl("gmcp"));

        QCOMPARE(mpPreferences->groupBox_protocols->parentWidget(), resultsColumn());
        QVERIFY2(mpPreferences->pushButton_chooseProtocols->property("searchMatch").toBool(), "the button carrying the protocol keywords was not highlighted");
    }

    // A lone Latin letter matches most of the dialog, and answering it means
    // moving most of the cards onto the results page and back on the next
    // keystroke - which is the lag, not the matching. An ideograph is a word,
    // so one of those is a query.
    void test_aSingleLetterIsNotAQueryButASingleIdeographIs()
    {
        const QStringList before = cardPlacements();
        QWidget* pPageBefore = stack()->currentWidget();

        search(qsl("s"));
        QCOMPARE(stack()->currentWidget(), pPageBefore);
        QCOMPARE(cardPlacements(), before);

        // Whether it matches anything in the language the dialog is in is
        // beside the point: reaching the results page at all says it was run
        search(QString::fromUtf8("\xe8\x89\xb2"));
        QCOMPARE(stack()->currentWidget(), pageOf(qsl("searchResults")));
    }

    void test_aQueryThatMatchesNothingShowsTheEmptyState()
    {
        search(qsl("zzzznothing"));

        auto* pEmpty = mpPreferences->findChild<QLabel*>(qsl("settingsSearchEmpty"));
        QVERIFY2(pEmpty->isVisible(), "nothing matched and the empty state was not shown");
        QVERIFY2(pEmpty->text().contains(qsl("zzzznothing")), qPrintable(qsl("the empty state does not echo the query: %1").arg(pEmpty->text())));
        QVERIFY2(resultsColumn()->findChildren<QGroupBox*>().isEmpty(), "a query that matched nothing still borrowed cards");
    }

    // The highlight is a dynamic property that a stylesheet paints from, so a
    // query that ends without taking it off again leaves the marker pen on
    void test_highlightsAreClearedWhenTheSearchEnds()
    {
        search(qsl("color"));
        QVERIFY2(!highlightedWidgets().isEmpty(), "nothing was highlighted, so clearing the highlights proves nothing");

        search(QString());
        QCOMPARE(highlightedWidgets(), QStringList());
    }

    // Picking a category is the other way out of the results, and it has to put
    // the cards back just as clearing the field does
    void test_choosingACategoryLeavesSearchMode()
    {
        const QStringList before = cardPlacements();
        search(qsl("color"));

        const int mapperRow = TestSettings::rowOf(mpPreferences, qsl("mapper"));
        QVERIFY(mapperRow >= 0);
        sidebar()->setCurrentRow(mapperRow);
        QCoreApplication::processEvents();

        QVERIFY2(searchField()->text().isEmpty(), "the search field still holds the query after a category was chosen");
        QCOMPARE(stack()->currentWidget(), pageOf(qsl("mapper")));
        QCOMPARE(cardPlacements(), before);
    }

    // A click on a control of a result card leaves the keyboard focus inside a
    // card the results only borrowed. The next run of the query hands that card
    // back to its own page, clearing the focus it carries, so the focus falls to
    // the sidebar - where an item view with no current index answers a focus-in
    // by taking the first one, which reads as the user having chosen General and
    // ends the search.
    void test_refiningTheQueryWithTheFocusOnAResultCardStaysInSearch()
    {
        search(qsl("color"));
        QCOMPARE(mpPreferences->groupBox_displayColors->parentWidget(), resultsColumn());

        mpPreferences->checkBox_allowServerToRedefineColors->setFocus(Qt::MouseFocusReason);
        QCOMPARE(QApplication::focusWidget(), mpPreferences->checkBox_allowServerToRedefineColors);

        searchField()->setText(qsl("colors"));
        QVERIFY2(TestSettings::waitForSearch(mpPreferences), "the search never ran");

        QCOMPARE(searchField()->text(), qsl("colors"));
        QCOMPARE(stack()->currentWidget(), pageOf(qsl("searchResults")));
    }

    // A card the profile's state has hidden - the updater's when there is no
    // updater, Discord's without the library - is not an option anyone can take
    // up, so it is not a result either. And the search must hand it back hidden.
    void test_aHiddenCardIsNeitherAResultNorUnhiddenByTheSearch()
    {
        QGroupBox* pCard = mpPreferences->groupBox_spellCheck;
        QVERIFY2(!pCard->isHidden(), "the card this case hides was hidden already, so hiding it proves nothing");
        // before the first search, because the index is built on it
        pCard->hide();
        const QStringList before = cardPlacements();

        search(qsl("spell"));
        QVERIFY2(pCard->parentWidget() != resultsColumn(), "a hidden card was lent to the search results");

        search(QString());
        QVERIFY2(pCard->isHidden(), "the search handed a hidden card back showing");
        QCOMPARE(cardPlacements(), before);
    }

    // The chevron beside the "Search results" heading is the way out for
    // someone who reached the results and wants the page they came from back -
    // the same door the sidebar is, rather than a second one of its own
    void test_theBackChevronReturnsToTheCategoryTheSearchInterrupted()
    {
        const QStringList before = cardPlacements();
        const int mapperRow = TestSettings::rowOf(mpPreferences, qsl("mapper"));
        QVERIFY(mapperRow >= 0);
        sidebar()->setCurrentRow(mapperRow);
        QCoreApplication::processEvents();

        auto* pBack = mpPreferences->findChild<QToolButton*>(qsl("settingsSearchBack"));
        QVERIFY2(pBack, "the search results have no back button");
        QVERIFY2(pBack->isHidden(), "the back button is on show while a category page is");

        search(qsl("color"));
        QVERIFY2(pBack->isVisible(), "the search results came up without a way back");
        // A keyboard user has to be able to reach it, which a tool button is
        // not set up for by default
        QVERIFY2((pBack->focusPolicy() & Qt::TabFocus) == Qt::TabFocus, "the back button cannot be reached with the keyboard");

        pBack->click();
        QCoreApplication::processEvents();

        QVERIFY2(searchField()->text().isEmpty(), "the back button left the query standing in the search field");
        QCOMPARE(stack()->currentWidget(), pageOf(qsl("mapper")));
        QVERIFY2(pBack->isHidden(), "the back button is still showing after the search ended");
        QCOMPARE(cardPlacements(), before);
    }

    // A result header says which category the cards under it live on, with a name
    // and an icon. The icon set is single-colour line art tinted at runtime, so a
    // header carries the picture itself rather than a path - which leaves the
    // shape as the only thing that says whose icon it is.
    void test_eachResultHeaderCarriesItsCategorysIcon()
    {
        search(qsl("color"));

        // Spelled out rather than as a raw string: moc does not parse one
        // inside a macro, and fails the whole file with "missing ')'"
        const QRegularExpression glyphPattern(qsl("<img src=\"data:image/png;base64,([^\"]+)\""));
        QStringList headerTexts;
        QStringList glyphs;
        QString mainDisplayGlyph;
        for (const auto* pLabel : mpPreferences->findChildren<QLabel*>(qsl("settingsSearchHeader"))) {
            if (!pLabel->isVisible()) {
                continue;
            }
            headerTexts << pLabel->text();
            const QRegularExpressionMatch match = glyphPattern.match(pLabel->text());
            QVERIFY2(match.hasMatch(), qPrintable(qsl("a results header carries no category icon: %1").arg(pLabel->text())));
            glyphs << match.captured(1);
            if (pLabel->text().contains(qsl("Main display"))) {
                mainDisplayGlyph = match.captured(1);
            }
        }
        QVERIFY2(headerTexts.size() >= 2, "fewer than two categories matched, so this case is not looking at more than one header");
        // One picture repeated on every header would be a decoration rather
        // than each category's own icon, and every assertion above would still
        // pass
        QCOMPARE(QSet<QString>(glyphs.cbegin(), glyphs.cend()).size(), glyphs.size());

        // The icon that category's sidebar row was built with, and the name
        // still beside it rather than replaced by the picture
        QVERIFY2(!mainDisplayGlyph.isEmpty(), "the Main display category matched no header, so this case cannot name the icon it expects");
        QImage carried;
        QVERIFY(carried.loadFromData(QByteArray::fromBase64(mainDisplayGlyph.toLatin1()), "PNG"));
        // Tinting replaces the colour and keeps the alpha, so the shape is what
        // survives out of the file and into the header
        const QImage expected(qsl(":/icons/settings-display.png"));
        QCOMPARE(carried.size(), expected.size());
        QCOMPARE(carried.convertToFormat(QImage::Format_Alpha8), expected.convertToFormat(QImage::Format_Alpha8));
    }

    // What someone types is often not a word the settings use - an acronym, or
    // what another client calls it - and a synonym is only worth having if it is
    // attached to the card the answer is on
    void test_aSynonymFindsASettingThatDoesNotUseTheWord()
    {
        search(qsl("keyring"));
        QCOMPARE(mpPreferences->findChild<QGroupBox*>(qsl("card_passwords"))->parentWidget(), resultsColumn());

        search(qsl("scrollback"));
        QCOMPARE(mpPreferences->groupBox_consoleBuffer->parentWidget(), resultsColumn());

        search(qsl("nvda"));
        QCOMPARE(mpPreferences->groupBox_accessibility->parentWidget(), resultsColumn());
    }

    // Accents and keyboard accelerators are folded out of both sides of the
    // comparison, so a query typed without either still finds what carries them
    void test_aQueryWithoutAccentsFindsACardThatHasThem()
    {
        // Set before the first search of this dialog, because that is when the
        // index is built off the widget tree
        mpPreferences->checkBox_askTlsAvailable->setProperty("searchKeywords", QString::fromUtf8("Réseau &Privé"));

        search(qsl("reseau prive"));

        QCOMPARE(mpPreferences->findChild<QGroupBox*>(qsl("card_secureConnectionReminder"))->parentWidget(), resultsColumn());
    }

    // What a subpage holds is findable too, but a card on one is not lent to
    // the results - taking it would leave the page it belongs to empty behind
    // the row that opens it. The result is the way in instead, and following it
    // lands on the subpage with the card it found outlined.
    void test_aWordOnlyASubpageUsesOffersTheWayIntoIt()
    {
        // Set before the first search of this dialog, because that is when the
        // index is built off the widget tree. A synonym rather than a word from
        // the descriptions, so that rewording them cannot quietly stop this
        // case from testing anything.
        auto* pSubpageCard = mpPreferences->findChild<QGroupBox*>(qsl("card_protocolList"));
        QVERIFY2(pSubpageCard, "the game protocols subpage has no card");
        pSubpageCard->setProperty("searchKeywords", qsl("zzsubpageonlyword"));

        search(qsl("zzsubpageonlyword"));

        QVERIFY2(pSubpageCard->parentWidget() != resultsColumn(), "a card on a subpage was lent to the results, which would empty the page behind the row that opens it");
        auto* pLink = mpPreferences->findChild<QPushButton*>(qsl("settingsSearchSubpageResult"));
        QVERIFY2(pLink, "a match on a subpage offered no way into it");
        QVERIFY2(pLink->isVisible(), "the way into the subpage was built but not shown");
        QCOMPARE(pLink->parentWidget(), resultsColumn());
        QVERIFY2(!pLink->text().isEmpty(), "the way into the subpage is unlabelled");

        pLink->click();
        QCoreApplication::processEvents();
        QVERIFY2(searchField()->text().isEmpty(), "following a result left the query standing in the search field");
        QCOMPARE(stack()->currentWidget(), pageOf(qsl("connection_protocols")));
        QCOMPARE(sidebar()->currentItem()->data(Qt::UserRole).toString(), qsl("connection"));
        QVERIFY2(QTest::qWaitFor(
                         [this]() {
                             return mpPreferences->findChild<QWidget*>(qsl("settingsSpotlight")) != nullptr;
                         },
                         5000),
                 "following a subpage result drew no spotlight over the card it found");
    }

    // ...and where the row that opens the subpage is itself a result, that row
    // is the answer: offering both would be the same setting listed twice.
    void test_aMatchOnBothTheOpenerAndItsSubpageIsListedOnce()
    {
        // "gmcp" is on the opener as a synonym and on the subpage as the real
        // name of a checkbox, so it matches both
        search(qsl("gmcp"));

        QCOMPARE(mpPreferences->groupBox_protocols->parentWidget(), resultsColumn());
        auto* pLink = mpPreferences->findChild<QPushButton*>(qsl("settingsSearchSubpageResult"));
        QVERIFY2(!pLink || pLink->isHidden(), "the subpage was offered as a second result beside the card that opens it");
    }

    void test_ctrlFFocusesTheSearchField()
    {
        mpPreferences->activateWindow();
        QVERIFY2(QTest::qWaitForWindowActive(mpPreferences), "the dialog never became the active window, so a window shortcut cannot fire");

        sidebar()->setFocus();
        QCOMPARE(QApplication::focusWidget(), sidebar());

        QTest::keyClick(mpPreferences, Qt::Key_F, Qt::ControlModifier);
        QCOMPARE(QApplication::focusWidget(), searchField());
    }
};

#include "SettingsSearchTest.moc"
MUDLET_GROUPED_TEST_MAIN(SettingsSearchTest)
