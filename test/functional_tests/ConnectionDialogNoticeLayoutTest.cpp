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
 * Picking a profile on the connection dialog must not move the games list
 * under the pointer: the notices beneath the list come and go as profiles are
 * picked, and the second click of a double-click has to land on the same game
 * as the first.
 *
 * Run with: ctest -R ConnectionDialogNoticeLayoutTest -V
 */

#include "PortableModeTestHelper.h"
#include "HostManager.h"
#include "MudletInstanceCoordinator.h"
#include "MudletApp.h"
#include "dlgConnectionProfiles.h"
#include "mudlet.h"

#include <QtTest/QtTest>

#include <QListWidget>
#include <QScrollBar>
#include <chrono>
#include <initializer_list>
#include <utility>

#include "GroupedTest.h"

using namespace std::chrono_literals;

class ConnectionDialogNoticeLayoutTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mXdgDir;
    QByteArray mSavedXdg;

    // enough games that the list has to scroll at three to a row
    static constexpr int scmProfileCount = 80;
    // sorts last, so it is on screen with the list scrolled to the bottom
    const QString mLoadedProfile = qsl("Z-ConnDialogNoticeLoaded");
    // no server address, so picking it raises a three-line warning
    const QString mUnaddressedProfile = qsl("A-ConnDialogNoticeUnaddressed");
    static constexpr int scmMyGamesTab = 0;

    dlgConnectionProfiles* dialog() const { return mudlet::self()->mpConnectionDialog.data(); }

    // slot_itemClicked() ignores a repeat of the profile it saw less than 100ms ago
    void select(QListWidgetItem* pItem) const
    {
        QTest::qWait(150ms);
        dialog()->listWidget_profiles->setCurrentItem(pItem);
        // the layout settles on a posted LayoutRequest
        QTest::qWait(50ms);
    }

    QListWidgetItem* itemFor(const QString& name) const
    {
        const auto items = dialog()->findData(*dialog()->listWidget_profiles, name, dlgConnectionProfiles::csmNameRole);
        return items.isEmpty() ? nullptr : items.first();
    }

    bool inView(const QListWidgetItem* pItem) const
    {
        auto* pList = dialog()->listWidget_profiles;
        return pList->viewport()->rect().contains(pList->visualItemRect(pItem));
    }

    // another game in view, as the one a player would click next
    QListWidgetItem* visibleOrdinaryItem() const
    {
        auto* pList = dialog()->listWidget_profiles;
        for (int i = pList->count() - 1; i >= 0; --i) {
            auto* pItem = pList->item(i);
            if (pItem->data(dlgConnectionProfiles::csmNameRole).toString().startsWith(qsl("ConnDialogNotice-")) && inView(pItem)) {
                return pItem;
            }
        }
        return nullptr;
    }

    // picks pTarget and checks that what was under the pointer stayed there
    void selectWithoutShift(QListWidgetItem* pTarget, const bool noticeExpected) const
    {
        auto* pList = dialog()->listWidget_profiles;
        auto* pScrollBar = pList->verticalScrollBar();
        QVERIFY2(inView(pTarget), "The game to pick is not on screen, so a player could not click it");
        const QRect listGeometry = pList->geometry();
        const int scrollPosition = pScrollBar->value();
        const QPoint clickPoint = pList->visualItemRect(pTarget).center();

        select(pTarget);
        QCOMPARE(dialog()->notificationArea->isVisible(), noticeExpected);
        QCOMPARE(pList->geometry(), listGeometry);
        QCOMPARE(pScrollBar->value(), scrollPosition);
        QVERIFY2(pList->itemAt(clickPoint) == pTarget, "A second click where the first one landed would pick a different game");
    }

    // each pick paired with whether it shows a notice; stops at the first that moves the list
    void selectEachWithoutShift(const std::initializer_list<std::pair<QListWidgetItem*, bool>> picks) const
    {
        for (const auto& [pTarget, noticeExpected] : picks) {
            selectWithoutShift(pTarget, noticeExpected);
            if (QTest::currentTestFailed()) {
                return;
            }
        }
    }

    void scrollToTheBottom() const
    {
        auto* pScrollBar = dialog()->listWidget_profiles->verticalScrollBar();
        pScrollBar->setValue(pScrollBar->maximum());
        QTest::qWait(50ms);
    }

private slots:
    void initTestCase()
    {
        if (portableMarkerPresent()) {
            QSKIP("portable.txt present - cannot redirect the config dir for this test");
        }

        mSavedXdg = qgetenv("XDG_CONFIG_HOME");
        QVERIFY(mXdgDir.isValid());
        QVERIFY(QDir().mkpath(qsl("%1/mudlet/profiles").arg(mXdgDir.path()))); // profiles/ = XDG opt-in
        qputenv("XDG_CONFIG_HOME", mXdgDir.path().toUtf8());

        mudlet::start();
        mudlet::self()->setupConfig();
        QVERIFY(MudletApp::getMudletPath(enums::profilesPath).startsWith(mXdgDir.path()));

        QStringList profiles{mLoadedProfile};
        for (int i = 0; i < scmProfileCount; ++i) {
            profiles << qsl("ConnDialogNotice-%1").arg(i, 2, 10, QLatin1Char('0'));
        }
        // a profile with no server address shows a warning of its own
        QVERIFY(QDir().mkpath(qsl("%1/mudlet/profiles/%2").arg(mXdgDir.path(), mUnaddressedProfile)));
        for (const auto& profile : profiles) {
            QVERIFY(MudletApp::writeProfileData(profile, qsl("url"), qsl("127.0.0.1")).first);
            QVERIFY(MudletApp::writeProfileData(profile, qsl("port"), qsl("4000")).first);
        }

        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);
        QVERIFY2(HostManager::self()->addHost(mLoadedProfile, QString(), QString(), QString()), "failed to put a loaded profile in the pool");

        // the "My games" tab, which lists only the profiles made above
        MudletApp::getQSettings()->setValue(qsl("connectionDialogActiveTab"), scmMyGamesTab);
        mudlet::self()->startAutoLogin({});
        QVERIFY(QTest::qWaitFor(
                []() {
                    return mudlet::self()->mpConnectionDialog && mudlet::self()->mpConnectionDialog->isVisible();
                },
                5s));
        QVERIFY(QTest::qWaitForWindowExposed(dialog()));
    }

    void cleanupTestCase()
    {
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
        delete mudlet::self();
    }

    void test_theListStaysPutAsTheLoadedNoticeComesAndGoes()
    {
        QVERIFY2(dialog(), "No connection dialog to test against");
        auto* pLoadedItem = itemFor(mLoadedProfile);
        QVERIFY(pLoadedItem);
        select(pLoadedItem);
        QVERIFY2(dialog()->notificationArea->isVisible(), "The loaded-profile notice never showed, so there was nothing to make the list move");
        // Notices differ in height, so in the games list's column any of them would resize the list
        QVERIFY2(!dialog()->widget_topLeft->isAncestorOf(dialog()->notificationArea), "The notice shares the games list's column");

        scrollToTheBottom();
        QVERIFY2(dialog()->listWidget_profiles->verticalScrollBar()->value() > 0, "The games list does not scroll, so a shift in its contents could not show");

        auto* pOtherItem = visibleOrdinaryItem();
        QVERIFY(pOtherItem);
        selectEachWithoutShift({{pOtherItem, false}, {pLoadedItem, true}});
    }

    // A styled scroll bar is wider than the style's extent QListView sets aside for it when it wraps the rows
    void test_aListThatScrollsDoesNotScrollSideways()
    {
        auto* pList = dialog()->listWidget_profiles;
        QVERIFY2(pList->verticalScrollBar()->isVisible(), "SETUP: the games list does not scroll, so its scroll bar takes no room from the rows");
        QVERIFY2(pList->horizontalScrollBar()->maximum() == 0,
                 qPrintable(qsl("the rows are %1px wider than the list's %2px viewport").arg(QString::number(pList->horizontalScrollBar()->maximum()), QString::number(pList->viewport()->width()))));
    }

    // The checks that fill the notice add each problem between line breaks, which left blank lines over and under the words
    void test_aNoticeIsAsTallAsItsWords()
    {
        auto* pItem = itemFor(mUnaddressedProfile);
        QVERIFY(pItem);
        select(pItem);
        QFrame* pNotice = dialog()->notificationArea;
        QVERIFY2(pNotice->isVisible(), "a game with no server address raised no notice");
        QLabel* pText = dialog()->notificationAreaMessageBox;
        const QFontMetrics metrics = pText->fontMetrics();
        const int wordsHeight = metrics.boundingRect(QRect(0, 0, pText->contentsRect().width(), QWIDGETSIZE_MAX), Qt::TextWordWrap, pText->text().trimmed()).height();
        const QMargins margins = pNotice->layout()->contentsMargins();
        // Within half a line: the label lays its words out by its own rules, not the font metrics'
        QTRY_VERIFY2(pNotice->height() <= margins.top() + wordsHeight + margins.bottom() + metrics.lineSpacing() / 2,
                     qPrintable(qsl("the notice is %1px tall for %2px of words").arg(QString::number(pNotice->height()), QString::number(wordsHeight))));
    }
};

#include "ConnectionDialogNoticeLayoutTest.moc"
MUDLET_GROUPED_TEST_MAIN(ConnectionDialogNoticeLayoutTest)
