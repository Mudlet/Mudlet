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
 * TStyle is the QProxyStyle the profile tab bar is given, and it decides both
 * which tab a bold/italic/underline mark belongs to and which style actually
 * draws the tab. Both have been got wrong before.
 *
 * Run with: ctest -R TabBarStyleTest -V
 */

#include <QApplication>
#include <QPainter>
#include <QPixmap>
#include <QProxyStyle>
#include <QStyleFactory>
#include <QtTest/QtTest>

#include "TTabBar.h"
#include "utils.h"

#include "GroupedTest.h"

// Stands in for the application style, so the tests can see what TStyle hands
// on to it: how many tabs, and in what font.
class TabDrawRecordingStyle : public QProxyStyle
{
public:
    mutable int mTabDraws = 0;
    mutable bool mSawBoldTab = false;

    void drawControl(ControlElement element, const QStyleOption* option, QPainter* painter, const QWidget* widget) const override
    {
        if (element == CE_TabBarTab) {
            ++mTabDraws;
            mSawBoldTab = mSawBoldTab || painter->font().bold();
        }
        QProxyStyle::drawControl(element, option, painter, widget);
    }
};

class TabBarStyleTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase()
    {
        mpRecorder = new TabDrawRecordingStyle;
        // An explicit base, so the recorder draws the same way whatever style
        // the platform would otherwise hand a base-less proxy.
        mpRecorder->setBaseStyle(QStyleFactory::create(qsl("Fusion")));
        // Takes ownership, and deletes the style it replaces - so saving the
        // old one to restore later would leave a dangling pointer. Never put
        // back, which is safe only because every case in a grouped test binary
        // runs in its own process.
        qApp->setStyle(mpRecorder);
    }

    void init()
    {
        mpRecorder->mTabDraws = 0;
        mpRecorder->mSawBoldTab = false;
    }

    // On some platforms the text a tab displays is not the profile name - #2056
    // hit a '&' accelerator appearing in tabText() on FreeBSD - and in Mudlet's
    // own code applyPrefixToDisplayedText() puts a prefix in front of it. The
    // profile name is kept in the tab's data for that reason, and since #2056
    // that is the key the mark is both stored under and painted from.
    void test_aTabMarkKeyedOnTheProfileNameReachesThePainting()
    {
        TTabBar bar(nullptr);
        const QString profileName = qsl("Gorbash");
        const int index = bar.addTab(profileName);
        bar.setTabData(index, profileName);
        bar.setTabText(index, qsl("&Gorbash"));

        bar.setTabBold(profileName, true);
        render(bar);

        QVERIFY2(mpRecorder->mTabDraws > 0, "no tab was drawn at all, so the font it was drawn in proves nothing");
        QVERIFY2(mpRecorder->mSawBoldTab, "the tab was painted in the ordinary font, so the mark never reached the painting");
    }

    // A QProxyStyle with no base style of its own does not fall back to the
    // application's style object - it builds a fresh instance of the platform's
    // native style, which on Windows 10 paints from the OS theme rather than
    // Mudlet's palette. TStyle's proxy base is null, so since #8999 it hands
    // the drawing to the application style instead.
    void test_tabsAreDrawnThroughTheApplicationStyle()
    {
        TTabBar bar(nullptr);
        bar.setTabData(bar.addTab(qsl("Gorbash")), qsl("Gorbash"));
        QVERIFY2(bar.style() != qApp->style(), "the tab bar wears no style of its own, so it would reach the application style whatever TStyle did");

        render(bar);

        QVERIFY2(mpRecorder->mTabDraws > 0, "the tab bar drew its tabs through a style of its own rather than the application's");
    }

private:
    void render(TTabBar& bar)
    {
        bar.resize(bar.sizeHint());
        QPixmap canvas(bar.size());
        bar.render(&canvas);
    }

    TabDrawRecordingStyle* mpRecorder = nullptr;
};

#include "TabBarStyleTest.moc"
MUDLET_GROUPED_TEST_MAIN(TabBarStyleTest)
