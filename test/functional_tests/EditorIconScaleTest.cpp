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
 * The actions toolbar and the sidebar sit side by side and draw glyphs at one
 * scale; the "Icon size toolbars" default lands on the design's 18px, as 24px
 * took the toolbar's names off on windows with plenty of room.
 */

#include <QListWidget>
#include <QToolBar>
#include <QToolButton>
#include <QtTest/QtTest>
#include <chrono>

#include "Host.h"
#include "HostDialogs.h"
#include "ProfileSessionTestHelper.h"
#include "dlgTriggerEditor.h"
#include "mudlet.h"

#include "GroupedTest.h"

using namespace std::chrono_literals;

class EditorIconScaleTest : public QObject
{
    Q_OBJECT

private:
    TestProfile::Session mSession;
    dlgTriggerEditor* mpEditor = nullptr;
    Host* mpHost = nullptr;
    const QString mProfileName = qsl("EditorIconScale-Test-Profile");

    // Same as uiDesign::scmSidebarIconSize, the settings sidebar's glyph size
    static constexpr int scmDesignLanguageGlyphSize = 18;
    // Mirrors the default set in mudlet::readSettings()
    static constexpr int scmDefaultPreference = 3;
    // Preference 3 x 8px: the baseline the narrower-window case measures against
    static constexpr int scmOldDefaultGlyphSize = 24;

    // Fine enough to tell two icon sizes apart on a bar of a dozen buttons
    static constexpr int scmSweepStep = 10;
    static constexpr int scmSweepCeiling = 3000;

    // The profile group is the first the bar takes the names off when it runs
    // out of room, so it marks the width the bar stops fitting at
    bool profileGroupIsNamed() const
    {
        const auto& group = mpEditor->mEditorToolBarGroups.constFirst();
        for (QAction* pAction : group.actions) {
            auto* pButton = qobject_cast<QToolButton*>(mpEditor->toolBar->widgetForAction(pAction));
            if (!pButton || pButton->toolButtonStyle() != Qt::ToolButtonTextBesideIcon) {
                return false;
            }
        }
        return true;
    }

    // Swept up from the minimum width, so the figure is one a user could arrive at
    int narrowestWidthWithEveryNameWritten()
    {
        for (int width = mpEditor->minimumWidth(); width <= scmSweepCeiling; width += scmSweepStep) {
            mpEditor->resize(width, 700);
            QCoreApplication::sendPostedEvents();
            QTest::qWait(20ms);
            if (profileGroupIsNamed()) {
                return width;
            }
        }
        return -1;
    }

    // mudlet::setToolBarIconSize() drops a value equal to the one it holds, so a
    // step through another value is what makes the mapping run again
    void setPreference(const int value)
    {
        mudlet::self()->setToolBarIconSize(value == 1 ? 2 : 1);
        mudlet::self()->setToolBarIconSize(value);
        QCoreApplication::sendPostedEvents();
        QTest::qWait(50ms);
    }

private slots:
    void initTestCase()
    {
        if (TestProfile::Session::configCannotBeRedirected()) {
            QSKIP(TestProfile::Session::configCannotBeRedirectedReason());
        }

        QVERIFY2(mSession.start(mProfileName), qPrintable(mSession.why()));
        mpHost = mSession.host();

        mudlet::self()->slot_showEditorDialog();
        QTest::qWait(100ms);
        mpEditor = HostDialogs::of(mpHost).mpEditorDialog;
        QVERIFY2(mpEditor != nullptr, "Editor dialog should be created");
        mpEditor->resize(1200, 700);
        QTest::qWait(50ms);
    }

    void cleanupTestCase()
    {
        mpEditor = nullptr;
        mpHost = nullptr;
        mSession.stop();
    }

    void test_theDefaultPreferenceIsTheDesignLanguagesGlyph()
    {
        setPreference(scmDefaultPreference);
        qInfo().noquote() << qsl("  preference %1 -> toolbar %2px, sidebar %3px")
                                     .arg(QString::number(scmDefaultPreference),
                                          QString::number(mpEditor->toolBar->iconSize().width()),
                                          QString::number(mpEditor->mpListWidget_editorSidebar->iconSize().width()));

        QCOMPARE(mpEditor->toolBar->iconSize(), QSize(scmDesignLanguageGlyphSize, scmDesignLanguageGlyphSize));
    }

    void test_bothBarsFollowThePreferenceTogether()
    {
        for (const int value : {1, 2, 3, 4}) {
            setPreference(value);
            QVERIFY2(mpEditor->toolBar->iconSize() == mpEditor->mpListWidget_editorSidebar->iconSize(),
                     qPrintable(
                             qsl("At preference %1 the toolbar draws %2px and the sidebar %3px")
                                     .arg(QString::number(value), QString::number(mpEditor->toolBar->iconSize().width()), QString::number(mpEditor->mpListWidget_editorSidebar->iconSize().width()))));
        }
        setPreference(4);
        QCOMPARE(mpEditor->toolBar->iconSize(), QSize(24, 24));
    }

    // The baseline is set as 24px outright rather than through a preference
    // value, so the comparison holds whatever the mapping is
    void test_theDefaultWritesEveryNameOutOnANarrowerWindow()
    {
        setPreference(scmDefaultPreference);
        mpEditor->toolBar->setIconSize(QSize(scmOldDefaultGlyphSize, scmOldDefaultGlyphSize));
        QTest::qWait(50ms);
        const int before = narrowestWidthWithEveryNameWritten();

        setPreference(scmDefaultPreference);
        const int after = narrowestWidthWithEveryNameWritten();

        qInfo().noquote() << qsl("  every name is written out from %1px of window at the old default's %2px glyphs, and from %3px at the %4px it maps to now")
                                     .arg(QString::number(before), QString::number(scmOldDefaultGlyphSize), QString::number(after), QString::number(mpEditor->toolBar->iconSize().width()));
        QVERIFY2(before > 0 && after > 0, "The bar wrote every name out at every width swept, so there is nothing to compare");
        QVERIFY2(after < before,
                 qPrintable(qsl("The toolbar needs %1px of window at the preference's default before every name is written out, against %2px at the size that default used to draw - no improvement")
                                    .arg(QString::number(after), QString::number(before))));
    }
};

#include "EditorIconScaleTest.moc"
MUDLET_GROUPED_TEST_MAIN(EditorIconScaleTest)
