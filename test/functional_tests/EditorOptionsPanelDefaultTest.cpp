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
 * The trigger options strip opens closed in every session, even under an older
 * configuration that stored showAllTriggerControls=true, and writing the settings
 * clears that retired key.
 *
 * Run with: ctest -R EditorOptionsPanelDefaultTest -V
 */

#include <QGridLayout>
#include <QLineEdit>
#include <QSettings>
#include <QToolButton>
#include <QtTest/QtTest>
#include <chrono>

#include "Host.h"
#include "HostDialogs.h"
#include "MudletApp.h"
#include "PixelTestHelper.h"
#include "ProfileSessionTestHelper.h"
#include "dlgTriggerEditor.h"
#include "dlgTriggersMainArea.h"
#include "mudlet.h"

#include "GroupedTest.h"

using namespace std::chrono_literals;

class EditorOptionsPanelDefaultTest : public QObject
{
    Q_OBJECT

private:
    TestProfile::Session mSession;
    dlgTriggerEditor* mpEditor = nullptr;
    Host* mpHost = nullptr;
    const QString mProfileName = qsl("EditorOptionsPanelDefault-Test-Profile");

    dlgTriggersMainArea* form() const { return mpEditor->mpTriggersMainArea; }

    QToolButton* optionsButton() const { return form()->toolButton_toggleExtraControls; }

    int topEdgeOf(const QWidget* pWidget) const { return pWidget->mapTo(mpEditor, QPoint(0, 0)).y(); }

    int bottomEdgeOf(const QWidget* pWidget) const { return topEdgeOf(pWidget) + pWidget->height(); }

    int leftEdgeOf(const QWidget* pWidget) const { return pWidget->mapTo(mpEditor, QPoint(0, 0)).x(); }

    int rightEdgeOf(const QWidget* pWidget) const { return leftEdgeOf(pWidget) + pWidget->width(); }

    int verticalCentreOf(const QWidget* pWidget) const { return pWidget->mapTo(mpEditor, pWidget->rect().center()).y(); }

private slots:
    void initTestCase()
    {
        if (TestProfile::Session::configCannotBeRedirected()) {
            QSKIP(TestProfile::Session::configCannotBeRedirectedReason());
        }

        QVERIFY2(mSession.startMudlet(), qPrintable(mSession.why()));

        // Seeded before the profile opens: loading a profile builds its editor, which reads its
        // settings then. The size gives the strip room, so a closed strip is not a squeeze.
        MudletApp::getQSettings()->setValue(qsl("showAllTriggerControls"), true);
        MudletApp::getQSettings()->setValue(qsl("script_editor_size"), QSize(1100, 900));
        MudletApp::getQSettings()->sync();

        QVERIFY2(mSession.openProfile(mProfileName), qPrintable(mSession.why()));
        mpHost = mSession.host();

        mudlet::self()->slot_showTriggerDialog();
        QTest::qWait(100ms);
        mpEditor = HostDialogs::of(mpHost).mpEditorDialog;
        QVERIFY2(mpEditor != nullptr, "Editor dialog should be created");
        mpEditor->resize(1000, 800);
        mpEditor->slot_showTriggers();
        mpEditor->addTrigger(false);
        QTest::qWait(100ms);
    }

    void cleanupTestCase()
    {
        mpEditor = nullptr;
        mpHost = nullptr;
        mSession.stop();
    }

    void test_theOptionsOpenClosedDespiteTheStoredSetting()
    {
        QVERIFY2(mpEditor->mpWidget_triggerOptionsRow != nullptr, "the trigger form has no options strip");
        QVERIFY2(!mpEditor->mpWidget_triggerOptionsRow->isVisible(), "the options strip is on show in a freshly opened editor");
        QVERIFY2(!mpEditor->mpLabel_optionsRow->isVisible(), "the word leading the options strip is on show with nothing beside it");
        QVERIFY2(optionsButton()->isVisible(), "the Options button is not on the head row, so there is no way to open the strip");
        QVERIFY2(!optionsButton()->isChecked(), "the Options button says the strip is open where it is not");
        QVERIFY2(!mpEditor->mShowAllTriggerControls, "the editor is holding the strip as open in a session that has not opened it");
    }

    void test_theButtonSitsOnTheHeadRowInLineWithTheCommandField()
    {
        QLineEdit* pCommand = form()->lineEdit_trigger_command;
        qInfo().noquote() << qsl("  the button is %1px tall at y=%2..%3, the command field %4px at y=%5..%6")
                                     .arg(optionsButton()->height())
                                     .arg(topEdgeOf(optionsButton()))
                                     .arg(bottomEdgeOf(optionsButton()))
                                     .arg(pCommand->height())
                                     .arg(topEdgeOf(pCommand))
                                     .arg(bottomEdgeOf(pCommand));

        QCOMPARE(optionsButton()->height(), pCommand->height());
        QVERIFY2(std::abs(verticalCentreOf(optionsButton()) - verticalCentreOf(pCommand)) <= 1,
                 qPrintable(qsl("the button's middle is at %1 and the command field's at %2, so the two are not on one line").arg(verticalCentreOf(optionsButton())).arg(verticalCentreOf(pCommand))));

        QVERIFY2(leftEdgeOf(optionsButton()) > rightEdgeOf(pCommand),
                 qPrintable(qsl("the button starts at x=%1, which is not past the command field's end at x=%2").arg(leftEdgeOf(optionsButton())).arg(rightEdgeOf(pCommand))));
        QVERIFY2(std::abs(rightEdgeOf(optionsButton()) - rightEdgeOf(form()->widget_top)) <= 1,
                 qPrintable(qsl("the button ends at x=%1 and the head row at x=%2, so it is not the last thing on the row").arg(rightEdgeOf(optionsButton())).arg(rightEdgeOf(form()->widget_top))));

        QCOMPARE(optionsButton()->text(), qsl("Options"));
    }

    // QGridLayout gives a row with no visible widget no height and no spacing
    void test_withTheStripAwayThePatternsFollowTheHeadRowAtTheGridsSpacing()
    {
        mpEditor->setTriggerOptionsShown(false);
        TestPixels::settle();

        auto* pGrid = qobject_cast<QGridLayout*>(form()->layout());
        QVERIFY2(pGrid != nullptr, "the trigger form is not laid out in a grid");
        const int gap = topEdgeOf(form()->widget_left) - bottomEdgeOf(form()->widget_top);
        qInfo().noquote() << qsl("  the head row ends at y=%1, the patterns start at y=%2, a gap of %3 where the grid's spacing is %4")
                                     .arg(bottomEdgeOf(form()->widget_top))
                                     .arg(topEdgeOf(form()->widget_left))
                                     .arg(gap)
                                     .arg(pGrid->verticalSpacing());
        QCOMPARE(gap, pGrid->verticalSpacing());
    }

    void test_theButtonOpensAndClosesTheStrip()
    {
        mpEditor->setTriggerOptionsShown(false);
        TestPixels::settle();
        const int patternsWhenAway = topEdgeOf(form()->widget_left);

        QTest::mouseClick(optionsButton(), Qt::LeftButton, Qt::NoModifier, optionsButton()->rect().center());
        TestPixels::settle();
        QVERIFY2(mpEditor->mpWidget_triggerOptionsRow->isVisible(), "clicking the Options button did not put the strip on show");
        QVERIFY2(mpEditor->mpLabel_optionsRow->isVisible(), "the strip is on show without the word leading it");
        QVERIFY2(optionsButton()->isChecked(), "the button does not say the strip it opened is open");
        QVERIFY2(mpEditor->mShowAllTriggerControls, "the click did not leave the session holding the strip as open");
        const int patternsWhenShown = topEdgeOf(form()->widget_left);
        qInfo().noquote() << qsl("  the patterns start at y=%1 with the strip away and y=%2 with it on show").arg(patternsWhenAway).arg(patternsWhenShown);
        QVERIFY2(patternsWhenShown > patternsWhenAway,
                 qPrintable(qsl("the pattern list is at y=%1 with the strip on show, where it was at y=%2 without it - the strip is taking no room").arg(patternsWhenShown).arg(patternsWhenAway)));

        QTest::mouseClick(optionsButton(), Qt::LeftButton, Qt::NoModifier, optionsButton()->rect().center());
        TestPixels::settle();
        QVERIFY2(!mpEditor->mpWidget_triggerOptionsRow->isVisible(), "clicking the Options button again did not put the strip away");
        QVERIFY2(!mpEditor->mpLabel_optionsRow->isVisible(), "the word leading the strip stayed behind after the strip went");
        QVERIFY2(!optionsButton()->isChecked(), "the button says the strip is open after it closed it");
        QVERIFY2(!mpEditor->mShowAllTriggerControls, "closing the strip left the session still holding it as open");
        QCOMPARE(topEdgeOf(form()->widget_left), patternsWhenAway);
    }

    void test_theChoiceIsKeptForTheSession()
    {
        mpEditor->setTriggerOptionsShown(true);
        TestPixels::settle();

        mpEditor->addTrigger(false);
        TestPixels::settle();
        QVERIFY2(mpEditor->mpWidget_triggerOptionsRow->isVisible(), "choosing another trigger put the options strip away");
        QVERIFY2(optionsButton()->isChecked(), "choosing another trigger left the button saying the strip is closed");

        mpEditor->slot_showAliases();
        mpEditor->slot_showTriggers();
        TestPixels::settle();
        QVERIFY2(mpEditor->mpWidget_triggerOptionsRow->isVisible(), "leaving the Triggers view and coming back put the options strip away");
        QVERIFY2(optionsButton()->isChecked(), "coming back to the Triggers view left the button saying the strip is closed");

        mpEditor->setTriggerOptionsShown(false);
        TestPixels::settle();
    }

    void test_writingSettingsLeavesNoRetiredKey()
    {
        mpEditor->writeSettings();
        QVERIFY2(!MudletApp::getQSettings()->contains(qsl("showAllTriggerControls")),
                 qPrintable(qsl("the editor still keeps showAllTriggerControls, which it is now %1").arg(MudletApp::getQSettings()->value(qsl("showAllTriggerControls")).toString())));
    }
};

#include "EditorOptionsPanelDefaultTest.moc"
MUDLET_GROUPED_TEST_MAIN(EditorOptionsPanelDefaultTest)
