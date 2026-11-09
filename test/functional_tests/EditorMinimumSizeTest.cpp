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
 * The editor must open at its stored size and drag small again. Its seven main
 * areas are hidden in the constructor, but a hide only propagates upwards while
 * the widget is visible, so the size restore was floored by a ~1600px minimum.
 */

#include <QtTest/QtTest>
#include <chrono>

#include "Host.h"
#include "HostDialogs.h"
#include "MudletApp.h"
#include "ProfileSessionTestHelper.h"
#include "dlgTriggerEditor.h"
#include "mudlet.h"

#include "GroupedTest.h"

using namespace std::chrono_literals;

class EditorMinimumSizeTest : public QObject
{
    Q_OBJECT

private:
    TestProfile::Session mSession;
    dlgTriggerEditor* mpEditor = nullptr;
    Host* mpHost = nullptr;
    const QString mProfileName = qsl("EditorMinimumSize-Test-Profile");
    QSize mStoredSize;

    // Kept in step with the bound the editor is allowed to claim
    static constexpr int scmMaxMinimumHeight = 600;
    static constexpr int scmMaxMinimumWidth = 900;

    // Every widget over the threshold, so the report names the one carrying the
    // number rather than only the ancestors that inherit it
    QString tallWidgetReport(const int threshold) const
    {
        QString report;
        const QList<QWidget*> children = mpEditor->findChildren<QWidget*>();
        for (const QWidget* pWidget : children) {
            const int hint = pWidget->minimumSizeHint().height();
            const int floor = pWidget->minimumHeight();
            if (qMax(hint, floor) < threshold) {
                continue;
            }
            report.append(qsl("\n  %1 (%2) hint=%3 min=%4 max=%5 vis=%6")
                                  .arg(pWidget->objectName().isEmpty() ? qsl("<unnamed>") : pWidget->objectName(),
                                       QString::fromLatin1(pWidget->metaObject()->className()),
                                       QString::number(hint),
                                       QString::number(floor),
                                       QString::number(pWidget->maximumHeight()),
                                       pWidget->isVisible() ? qsl("y") : qsl("n")));
        }
        return report;
    }

private slots:
    void initTestCase()
    {
        if (TestProfile::Session::configCannotBeRedirected()) {
            QSKIP(TestProfile::Session::configCannotBeRedirectedReason());
        }

        QVERIFY2(mSession.startMudlet(), qPrintable(mSession.why()));

        // Before the profile opens: the editor's constructor reads this, and the
        // layout pass right after it is what could overrule it
        const QScreen* pScreen = QGuiApplication::primaryScreen();
        QVERIFY(pScreen);
        // The size restore clamps a stored size to the desktop, and the offscreen
        // platform's screen is smaller than a real one
        mStoredSize = QSize(989, 643).boundedTo(pScreen->availableGeometry().size());
        QVERIFY2(mStoredSize.height() >= 400, "The screen is too small for this test to say anything");
        MudletApp::getQSettings()->setValue(qsl("script_editor_size"), mStoredSize);
        MudletApp::getQSettings()->remove(qsl("script_editor_pos"));

        QVERIFY2(mSession.openProfile(mProfileName), qPrintable(mSession.why()));
        mpHost = mSession.host();

        mudlet::self()->slot_showScriptDialog();
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

    // restoreWindowGeometry() floors a stored size, and a window manager a drag,
    // with this hint
    void test_editorMinimumSizeHintStaysSmall()
    {
        const QSize hint = mpEditor->minimumSizeHint();

        QVERIFY2(hint.height() <= scmMaxMinimumHeight,
                 qPrintable(qsl("The editor's minimum height is %1, over the %2 it "
                                "is allowed - it will override a smaller stored "
                                "size and cannot be dragged smaller. Widgets at "
                                "least %2 tall:%3")
                                    .arg(QString::number(hint.height()), QString::number(scmMaxMinimumHeight), tallWidgetReport(scmMaxMinimumHeight))));
        QVERIFY2(hint.width() <= scmMaxMinimumWidth,
                 qPrintable(qsl("The editor's minimum width is %1, over the %2 it "
                                "is allowed")
                                    .arg(QString::number(hint.width()), QString::number(scmMaxMinimumWidth))));
    }

    void test_editorOpensAtStoredSize()
    {
        QVERIFY2(mpEditor->size() == mStoredSize,
                 qPrintable(qsl("The editor was left at %1x%2 but opened at %3x%4. "
                                "Its minimum is %5x%6.")
                                    .arg(QString::number(mStoredSize.width()),
                                         QString::number(mStoredSize.height()),
                                         QString::number(mpEditor->size().width()),
                                         QString::number(mpEditor->size().height()),
                                         QString::number(mpEditor->minimumSize().width()),
                                         QString::number(mpEditor->minimumSize().height()))));
    }

    // The draggable minimum is the one Qt derives from the layout, not the hint.
    // Restores the opening size for the cases that follow.
    void test_editorResizesDownToASmallWindow()
    {
        const QSize small(700, 500);
        mpEditor->resize(small);
        QCoreApplication::processEvents();
        const QSize reached = mpEditor->size();
        mpEditor->resize(mStoredSize);
        QCoreApplication::processEvents();

        QVERIFY2(reached == small,
                 qPrintable(qsl("Asked for %1x%2, got %3x%4 - the editor cannot be "
                                "made that small. Its minimum is %5x%6.")
                                    .arg(QString::number(small.width()),
                                         QString::number(small.height()),
                                         QString::number(reached.width()),
                                         QString::number(reached.height()),
                                         QString::number(mpEditor->minimumSize().width()),
                                         QString::number(mpEditor->minimumSize().height()))));
    }
};

#include "EditorMinimumSizeTest.moc"
MUDLET_GROUPED_TEST_MAIN(EditorMinimumSizeTest)
