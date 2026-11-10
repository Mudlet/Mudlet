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
 * A profile name may contain "&", which QTabBar reads as a mnemonic marker: a
 * profile called "R&D" would show as "RD" on its tab and grab Alt+D. The main
 * window and every detached window share TTabBar, so it is checked here on a
 * bare tab bar with no profile to load.
 *
 * Run with: ctest -R ProfileTabLabelTest -V
 */

#include <QtTest/QtTest>

#include <QAccessible>

#include "TTabBar.h"
#include "utils.h"

#include "GroupedTest.h"

class ProfileTabLabelTest : public QObject
{
    Q_OBJECT

private:
    const QString mPlainName = qsl("Gorbash");
    const QString mAmpersandName = qsl("R&D");

    void addTab(TTabBar& bar, const QString& profileName) { bar.setTabData(bar.addTab(profileName), profileName); }

    // What a screen reader is told, and the text Qt draws once its mnemonic marks are taken out
    static QString spokenName(TTabBar& bar, const int index)
    {
        QAccessibleInterface* pBar = QAccessible::queryAccessibleInterface(&bar);
        if (!pBar) {
            return QString();
        }
        QAccessibleInterface* pTab = pBar->child(index);
        return pTab ? pTab->text(QAccessible::Name) : QString();
    }

private slots:
    void test_ampersandInAProfileNameIsShown()
    {
        TTabBar bar(nullptr);
        addTab(bar, mPlainName);
        addTab(bar, mAmpersandName);

        QCOMPARE(spokenName(bar, bar.tabIndex(mAmpersandName)), mAmpersandName);
        QCOMPARE(spokenName(bar, bar.tabIndex(mPlainName)), mPlainName);
    }

    // Reattaching a detached profile puts its tab back with insertTab()
    void test_ampersandInAnInsertedTabIsShown()
    {
        TTabBar bar(nullptr);
        addTab(bar, mPlainName);
        bar.setTabData(bar.insertTab(0, mAmpersandName), mAmpersandName);

        QCOMPARE(spokenName(bar, bar.tabIndex(mAmpersandName)), mAmpersandName);
    }

    void test_ampersandSurvivesAPrefix()
    {
        TTabBar bar(nullptr);
        addTab(bar, mPlainName);
        addTab(bar, mAmpersandName);

        bar.applyPrefixToDisplayedText(mAmpersandName, qsl("[A] "));
        QCOMPARE(spokenName(bar, bar.tabIndex(mAmpersandName)), qsl("[A] R&D"));

        bar.applyPrefixToDisplayedText(mAmpersandName);
        QCOMPARE(spokenName(bar, bar.tabIndex(mAmpersandName)), mAmpersandName);
    }

    // Marking activity re-sets the tab's own text to make Qt measure it again
    void test_activityMarkKeepsTheAmpersand()
    {
        TTabBar bar(nullptr);
        addTab(bar, mPlainName);
        addTab(bar, mAmpersandName);

        bar.markActivity(mAmpersandName, false);
        QCOMPARE(spokenName(bar, bar.tabIndex(mAmpersandName)), mAmpersandName);

        bar.clearActivity(bar.tabIndex(mAmpersandName));
        QCOMPARE(spokenName(bar, bar.tabIndex(mAmpersandName)), mAmpersandName);
    }

    void test_ampersandDoesNotMakeAShortcut()
    {
        TTabBar bar(nullptr);
        addTab(bar, mPlainName);
        addTab(bar, mAmpersandName);
        bar.setCurrentIndex(bar.tabIndex(mPlainName));
        bar.show();
        bar.activateWindow();
        QVERIFY(QTest::qWaitForWindowActive(&bar));

        QTest::keyClick(&bar, Qt::Key_D, Qt::AltModifier);

        QVERIFY2(bar.currentIndex() == bar.tabIndex(mPlainName), "Alt+D switched to the profile whose name has \"&D\" in it");
    }
};

#include "ProfileTabLabelTest.moc"
MUDLET_GROUPED_TEST_MAIN(ProfileTabLabelTest)
