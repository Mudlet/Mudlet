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
 * Loading a profile builds its editor unshown, and closeEvent() reaches every
 * editor on quit: a never-shown editor must not overwrite the app-wide
 * script_editor_pos/size another profile's editor stored (upstream PR #10317).
 */

#include <QGuiApplication>
#include <QScreen>
#include <QtTest/QtTest>

#include "Host.h"
#include "HostDialogs.h"
#include "MudletApp.h"
#include "PixelTestHelper.h"
#include "ProfileSessionTestHelper.h"
#include "dlgTriggerEditor.h"
#include "mudlet.h"

#include "GroupedTest.h"

class EditorGeometryWriteGuardTest : public QObject
{
    Q_OBJECT

private:
    TestProfile::Session mSession;
    dlgTriggerEditor* mpEditor = nullptr;
    Host* mpHost = nullptr;
    const QString mProfileName = qsl("EditorGeometryWriteGuard-Test-Profile");

    QPoint mSeededPos;
    QSize mSeededSize;
    QPoint mDraggedPos;

    static QString describe(const QPoint& point) { return qsl("(%1,%2)").arg(QString::number(point.x()), QString::number(point.y())); }

    static QPoint storedPos() { return MudletApp::getQSettings()->value(qsl("script_editor_pos")).toPoint(); }

    static QSize storedSize() { return MudletApp::getQSettings()->value(qsl("script_editor_size")).toSize(); }

private slots:
    void initTestCase()
    {
        if (TestProfile::Session::configCannotBeRedirected()) {
            QSKIP(TestProfile::Session::configCannotBeRedirectedReason());
        }

        QVERIFY2(mSession.startMudlet(), qPrintable(mSession.why()));

        const QScreen* pScreen = QGuiApplication::primaryScreen();
        QVERIFY(pScreen);
        const QRect availableArea = pScreen->availableGeometry();
        mSeededPos = availableArea.topLeft() + QPoint(37, 29);
        mDraggedPos = availableArea.topLeft() + QPoint(116, 83);
        // Below the editor's minimum, so an unguarded write stores the layout's floor
        // and fails loudly rather than by a few pixels
        mSeededSize = QSize(640, 460).boundedTo(availableArea.size());
        QVERIFY2(mSeededSize.height() >= 300, "The screen is too small for this test to say anything");

        MudletApp::getQSettings()->setValue(qsl("script_editor_pos"), mSeededPos);
        MudletApp::getQSettings()->setValue(qsl("script_editor_size"), mSeededSize);
        qInfo().noquote() << qsl("  seeded: pos %1 size %2").arg(describe(mSeededPos), TestPixels::describe(mSeededSize));

        QVERIFY2(mSession.openProfile(mProfileName), qPrintable(mSession.why()));
        mpHost = mSession.host();
        // Built and never shown, as a script or the settings can build it before the player opens it
        mpEditor = HostDialogs::editor(mpHost);
        QVERIFY2(mpEditor != nullptr, "the profile has no editor to build");
    }

    void cleanupTestCase()
    {
        mpEditor = nullptr;
        mpHost = nullptr;
        mSession.stop();
    }

    void test_anEditorNobodyOpenedLeavesThePlacementAlone()
    {
        QVERIFY2(!mpEditor->isVisible(), "The editor was shown, so this is no longer the case being covered");
        qInfo().noquote() << qsl("  the unshown editor holds: pos %1 size %2").arg(describe(mpEditor->pos()), TestPixels::describe(mpEditor->size()));

        mpEditor->close();
        QCoreApplication::processEvents();
        qInfo().noquote() << qsl("  in QSettings after its close: pos %1 size %2").arg(describe(storedPos()), TestPixels::describe(storedSize()));

        QVERIFY2(storedSize() == mSeededSize,
                 qPrintable(qsl("An editor nobody opened overwrote the stored size %1 with %2").arg(TestPixels::describe(mSeededSize), TestPixels::describe(storedSize()))));
        QVERIFY2(storedPos() == mSeededPos, qPrintable(qsl("An editor nobody opened overwrote the stored position %1 with %2").arg(describe(mSeededPos), describe(storedPos()))));
    }

    void test_anEditorTheUserOpenedStoresWhereItWasLeft()
    {
        mudlet::self()->slot_showEditorDialog();
        QCoreApplication::processEvents();
        QVERIFY2(HostDialogs::of(mpHost).mpEditorDialog == mpEditor, "Showing the editor replaced it, so the two halves are not about the same window");

        mpEditor->move(mDraggedPos);
        QCoreApplication::processEvents();
        const QSize sizeBefore = mpEditor->size();
        qInfo().noquote() << qsl("  the shown editor was left at: pos %1 size %2").arg(describe(mpEditor->pos()), TestPixels::describe(sizeBefore));

        mpEditor->close();
        QCoreApplication::processEvents();
        qInfo().noquote() << qsl("  in QSettings after its close: pos %1 size %2").arg(describe(storedPos()), TestPixels::describe(storedSize()));

        QVERIFY2(storedPos() == mDraggedPos, qPrintable(qsl("The editor was left at %1 and stored %2").arg(describe(mDraggedPos), describe(storedPos()))));
        QCOMPARE(storedSize(), sizeBefore);
    }
};

#include "EditorGeometryWriteGuardTest.moc"
MUDLET_GROUPED_TEST_MAIN(EditorGeometryWriteGuardTest)
