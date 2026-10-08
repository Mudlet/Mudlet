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
 * game outranks text Mudlet wrote itself, a tab with no neighbour is left alone,
 * and a tab whose font changed is measured again.
 *
 * Which window's bar a profile's output reaches is DetachedWindowTabActivityTest's
 * to cover.
 *
 * Run with: ctest -R TabActivityMarkTest -V
 */

#include <QFontDatabase>
#include <QFontInfo>
#include <QFontMetrics>
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

    // QTabBar measures a tab once and keeps the answer, so a mark that only
    // repainted would draw wider text into a tab still sized for the narrower:
    // the end of the name is clipped until something else forces a layout.
    void test_aTabIsMeasuredAgainWhenItsMarkChanges()
    {
#ifndef INCLUDE_FONTS
        QSKIP("Built with WITH_FONTS=NO, so there is no font here that is known to be wider in bold");
#else
        // The platform's default font is whatever the machine has, and one whose
        // bold face is no wider would leave this with nothing to see
        for (const QString& file : {qsl(":/fonts/ttf-bitstream-vera-1.10/Vera.ttf"), qsl(":/fonts/ttf-bitstream-vera-1.10/VeraBd.ttf")}) {
            QVERIFY2(QFontDatabase::addApplicationFont(file) != -1, qPrintable(qsl("Could not register the bundled font %1").arg(file)));
        }
        const QFont plain(qsl("Bitstream Vera Sans"), 10);
        QCOMPARE(QFontInfo(plain).family(), qsl("Bitstream Vera Sans"));
        QFont bold(plain);
        bold.setBold(true);
        QVERIFY2(QFontMetrics(bold).horizontalAdvance(mHiddenTab) > QFontMetrics(plain).horizontalAdvance(mHiddenTab), "the name is no wider in bold, so no tab has a reason to grow");

        TTabBar bar(nullptr);
        bar.setFont(plain);
        // Otherwise the tabs share out the bar's spare width between them, and
        // how wide one is says nothing about what its own text needs
        bar.setExpanding(false);
        addBothTabs(bar);
        bar.resize(1000, bar.sizeHint().height());
        const int index = bar.tabIndex(mHiddenTab);
        const int plainWidth = bar.tabRect(index).width();

        bar.markActivity(mHiddenTab, false);
        QVERIFY2(bar.tabRect(index).width() > plainWidth, "the tab went bold and kept the width it had for plain text, which clips the end of its name");

        bar.clearActivity(index);
        QCOMPARE(bar.tabRect(index).width(), plainWidth);
#endif
    }
};

#include "TabActivityMarkTest.moc"
MUDLET_GROUPED_TEST_MAIN(TabActivityMarkTest)
