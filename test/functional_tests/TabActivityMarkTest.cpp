/***************************************************************************
 *   Copyright (C) 2026 by the Mudlet development team                     *
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
 * TTabBar::markActivity() and clearActivity() put the bold or italic mark on the
 * tab of a profile with output nobody has looked at yet, and take it off again.
 * The main window and every detached window share them, so the rules they keep
 * are checked here on a bare tab bar, with no profile to load: text from the
 * game outranks text Mudlet wrote itself, and a tab with no neighbour is left
 * alone.
 *
 * Which window's bar a profile's output reaches is DetachedWindowTabActivityTest's
 * to cover.
 *
 * Run with: ctest -R TabActivityMarkTest -V
 */

#include <QtTest/QtTest>

#include "TTabBar.h"
#include "utils.h"

#include "GroupedTest.h"

class TabActivityMarkTest : public QObject
{
    Q_OBJECT

private:
    const QString mShownTab = qsl("Gorbash");
    const QString mHiddenTab = qsl("Zugg the Unyielding");

    // The profile name goes in the tab's data, which is what the marks are keyed on
    void addTab(TTabBar& bar, const QString& profileName) { bar.setTabData(bar.addTab(profileName), profileName); }

    void addBothTabs(TTabBar& bar)
    {
        addTab(bar, mShownTab);
        addTab(bar, mHiddenTab);
    }

private slots:
    void test_textFromTheGameMarksTheTabBold()
    {
        TTabBar bar(nullptr);
        addBothTabs(bar);
        QVERIFY(!bar.tabBold(mHiddenTab));

        bar.markActivity(mHiddenTab, false);

        QVERIFY(bar.tabBold(mHiddenTab));
        QVERIFY(!bar.tabItalic(mHiddenTab));
        QVERIFY2(!bar.tabBold(mShownTab) && !bar.tabItalic(mShownTab), "the mark landed on a tab other than the one named");
    }

    void test_localTextMarksTheTabItalic()
    {
        TTabBar bar(nullptr);
        addBothTabs(bar);
        QVERIFY(!bar.tabItalic(mHiddenTab));

        bar.markActivity(mHiddenTab, true);

        QVERIFY(bar.tabItalic(mHiddenTab));
        QVERIFY(!bar.tabBold(mHiddenTab));
    }

    void test_localTextDoesNotReplaceTheMarkForGameText()
    {
        TTabBar bar(nullptr);
        addBothTabs(bar);

        bar.markActivity(mHiddenTab, false);
        bar.markActivity(mHiddenTab, true);

        QVERIFY2(bar.tabBold(mHiddenTab), "local text arriving later took away the mark saying the game had sent something");
        QVERIFY2(!bar.tabItalic(mHiddenTab), "a tab is either bold or italic, and game text is the one that wins");
    }

    void test_gameTextReplacesTheMarkForLocalText()
    {
        TTabBar bar(nullptr);
        addBothTabs(bar);

        bar.markActivity(mHiddenTab, true);
        bar.markActivity(mHiddenTab, false);

        QVERIFY2(bar.tabBold(mHiddenTab), "game text arriving after local text left the tab saying only local text was waiting");
        QVERIFY2(!bar.tabItalic(mHiddenTab), "a tab is either bold or italic, and game text is the one that wins");
    }

    // The mark tells one tab from the others, so a profile that has the bar to
    // itself is not given one
    void test_aTabWithNoNeighbourIsNotMarked()
    {
        TTabBar bar(nullptr);
        addTab(bar, mHiddenTab);

        bar.markActivity(mHiddenTab, false);
        QVERIFY(!bar.tabBold(mHiddenTab));

        bar.markActivity(mHiddenTab, true);
        QVERIFY(!bar.tabItalic(mHiddenTab));
    }

    void test_clearingTakesEveryMarkOff()
    {
        TTabBar bar(nullptr);
        addBothTabs(bar);
        const int index = bar.tabIndex(mHiddenTab);
        bar.setTabBold(index, true);
        bar.setTabItalic(index, true);
        bar.setTabUnderline(index, true);

        bar.clearActivity(index);

        QVERIFY(!bar.tabBold(index));
        QVERIFY(!bar.tabItalic(index));
        QVERIFY(!bar.tabUnderline(index));
    }
};

#include "TabActivityMarkTest.moc"
MUDLET_GROUPED_TEST_MAIN(TabActivityMarkTest)
