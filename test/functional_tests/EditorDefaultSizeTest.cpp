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
 * With nothing stored, the editor opens at a fixed size, not one scaled off the profile window.
 * Offscreen cannot catch cocoa dropping the restore if setUnifiedTitleAndToolBarOnMac()
 * moves before readSettings(): every Mac editor opens at 636x688 while this passes.
 *
 * Run with: ctest -R EditorDefaultSizeTest -V
 */

#include <QGuiApplication>
#include <QScreen>
#include <QtTest/QtTest>
#include <chrono>

#include "Host.h"
#include "HostDialogs.h"
#include "MudletApp.h"
#include "PixelTestHelper.h"
#include "ProfileSessionTestHelper.h"
#include "TConsole.h"
#include "TMainConsole.h"
#include "dlgTriggerEditor.h"
#include "mudlet.h"

#include "GroupedTest.h"

using namespace std::chrono_literals;

class EditorDefaultSizeTest : public QObject
{
    Q_OBJECT

private:
    TestProfile::Session mSession;
    dlgTriggerEditor* mpEditor = nullptr;
    Host* mpHost = nullptr;
    const QString mProfileName = qsl("EditorDefaultSize-Test-Profile");

    // Larger than the default, so a default scaled off the profile window comes out bigger
    static constexpr int scmProfileWindowWidth = 1600;
    static constexpr int scmProfileWindowHeight = 1000;
    // Mirrors the editor's file-local scmEditorDefaultWidth/Height, so the size is
    // held to a figure rather than to whatever the editor answers
    static constexpr int scmCompanionWidth = 1300;
    static constexpr int scmCompanionHeight = 690;

private slots:
    void initTestCase()
    {
        if (TestProfile::Session::configCannotBeRedirected()) {
            QSKIP(TestProfile::Session::configCannotBeRedirectedReason());
        }

        QVERIFY2(mSession.startMudlet(), qPrintable(mSession.why()));

        QVERIFY2(!MudletApp::getQSettings()->contains(qsl("script_editor_size")), "this configuration already carries a stored editor size, so the default is never reached");
        QVERIFY2(!MudletApp::getQSettings()->contains(qsl("script_editor_pos")), "this configuration already carries a stored editor position");

        // Before the profile opens: loading it builds the editor, which reads
        // this geometry as it is built
        mudlet::self()->resize(scmProfileWindowWidth, scmProfileWindowHeight);
        QCoreApplication::processEvents();

        QVERIFY2(mSession.openProfile(mProfileName), qPrintable(mSession.why()));
        mpHost = mSession.host();

        mudlet::self()->slot_showTriggerDialog();
        QTest::qWait(100ms);
        mpEditor = HostDialogs::of(mpHost).mpEditorDialog;
        QVERIFY2(mpEditor != nullptr, "Editor dialog should be created");
    }

    void cleanupTestCase()
    {
        mpEditor = nullptr;
        mpHost = nullptr;
        mSession.stop();
    }

    void test_theEditorOpensAtItsCompanionSizeRatherThanAtTheProfileWindows()
    {
        const QSize companion(scmCompanionWidth, scmCompanionHeight);
        const QScreen* pScreen = mpEditor->screen() ? mpEditor->screen() : QGuiApplication::primaryScreen();
        const QSize available = pScreen ? pScreen->availableGeometry().size() : QSize();
        const QSize profileWindow = mpHost->mainConsoleView()->window()->size();
        // restoreWindowGeometry() clamps to the desktop, then raises to the
        // layout's floor; an offscreen run's 800x800 desktop is smaller than the default
        const QSize expected = companion.boundedTo(available).expandedTo(mpEditor->minimumSizeHint().boundedTo(available));

        qInfo().noquote() << qsl("  the profile window is %1 on a %2 desktop, and the editor opened at %3 against the %4 it defaults to, floored at %5 - so %6 is what it should come to")
                                     .arg(TestPixels::describe(profileWindow),
                                          TestPixels::describe(available),
                                          TestPixels::describe(mpEditor->size()),
                                          TestPixels::describe(companion),
                                          TestPixels::describe(mpEditor->minimumSizeHint()),
                                          TestPixels::describe(expected));

        QVERIFY2(profileWindow.width() > companion.width() && profileWindow.height() > companion.height(),
                 qPrintable(qsl("the profile window is %1, which is not larger than the %2 the editor opens at - a default scaled off it would come out smaller rather than bigger, so nothing "
                                "here would catch one")
                                    .arg(TestPixels::describe(profileWindow), TestPixels::describe(companion))));
        QCOMPARE(mpEditor->defaultEditorSize(), companion);
        QVERIFY2(mpEditor->size() == expected,
                 qPrintable(qsl("the editor opened at %1 rather than at the %2 it defaults to").arg(TestPixels::describe(mpEditor->size()), TestPixels::describe(expected))));
    }
};

#include "EditorDefaultSizeTest.moc"
MUDLET_GROUPED_TEST_MAIN(EditorDefaultSizeTest)
