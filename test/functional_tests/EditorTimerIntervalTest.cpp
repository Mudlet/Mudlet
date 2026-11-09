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
 * A timer's interval is one sentence row holding the four .ui fields at the form's
 * size, saving and loading through them, and an offset timer's reads "Fires once". The
 * clock's captions and separators must stay gone, or the interval shows twice.
 *
 * Run with: ctest -R EditorTimerIntervalTest -V
 */

#include <QFontInfo>
#include <QLabel>
#include <QRegularExpression>
#include <QTimeEdit>
#include <QtTest/QtTest>
#include <chrono>

#include "Host.h"
#include "HostDialogs.h"
#include "ProfileSessionTestHelper.h"
#include "TTimer.h"
#include "TimerUnit.h"
#include "dlgSourceEditorArea.h"
#include "dlgTimersMainArea.h"
#include "dlgTriggerEditor.h"
#include "mudlet.h"

#include "GroupedTest.h"

using namespace std::chrono_literals;

class EditorTimerIntervalTest : public QObject
{
    Q_OBJECT

private:
    TestProfile::Session mSession;
    dlgTriggerEditor* mpEditor = nullptr;
    Host* mpHost = nullptr;
    QTreeWidgetItem* mpTimerItem = nullptr;
    QTreeWidgetItem* mpOffsetItem = nullptr;
    const QString mProfileName = qsl("EditorTimerInterval-Test-Profile");

    // Must match the sentences dlgTriggerEditor builds the row from
    static QString everySentence() { return qsl("Fires every %1 h %2 min %3 s %4 ms"); }
    static QString onceSentence() { return qsl("Fires once, %1 h %2 min %3 s %4 ms after the timer above it fires"); }

    static QString wordsOf(const QString& sentence)
    {
        static const QRegularExpression placeholder(qsl("%\\d"));
        return QString(sentence).remove(placeholder).simplified();
    }

    QWidget* row() const { return mpEditor->mpWidget_timerInterval; }

    QList<QTimeEdit*> fields() const
    {
        return {mpEditor->mpTimersMainArea->timeEdit_timer_hours,
                mpEditor->mpTimersMainArea->timeEdit_timer_minutes,
                mpEditor->mpTimersMainArea->timeEdit_timer_seconds,
                mpEditor->mpTimersMainArea->timeEdit_timer_msecs};
    }

    QStringList reading() const
    {
        QStringList reading;
        QLayout* pLayout = row() ? row()->layout() : nullptr;
        if (!pLayout) {
            return reading;
        }
        const QList<QTimeEdit*> edits = fields();
        for (int i = 0, total = pLayout->count(); i < total; ++i) {
            QWidget* pWidget = pLayout->itemAt(i)->widget();
            if (!pWidget) {
                continue;
            }
            if (auto* pEdit = qobject_cast<QTimeEdit*>(pWidget); pEdit) {
                reading << qsl("[%1]").arg(QString::number(edits.indexOf(pEdit) + 1));
                continue;
            }
            if (auto* pLabel = qobject_cast<QLabel*>(pWidget); pLabel) {
                reading << pLabel->text();
            }
        }
        return reading;
    }

    QString shownWords() const
    {
        QStringList words;
        for (const QString& part : reading()) {
            if (!part.startsWith(QLatin1Char('['))) {
                words << part;
            }
        }
        return words.join(QChar::Space).simplified();
    }

    TTimer* timerOf(QTreeWidgetItem* pItem) const { return pItem ? mpHost->getTimerUnit()->getTimer(pItem->data(0, Qt::UserRole).toInt()) : nullptr; }

    void choose(QTreeWidgetItem* pItem)
    {
        mpEditor->slot_timerSelected(pItem);
        QCoreApplication::processEvents();
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

        mudlet::self()->slot_showTriggerDialog();
        QTest::qWait(100ms);
        mpEditor = HostDialogs::of(mpHost).mpEditorDialog;
        QVERIFY2(mpEditor != nullptr, "Editor dialog should be created");
        mpEditor->resize(1100, 900);

        mpEditor->slot_showTimers();
        mpEditor->addTimer(false);
        QTest::qWait(100ms);
        mpTimerItem = mpEditor->mpCurrentTimerItem;
        QVERIFY2(mpTimerItem != nullptr, "addTimer() left no current timer item");

        // The editor offers no way to make an offset timer (a timer inside a timer), so one is
        // built here with the tree row a loaded profile would have given it
        TTimer* pParent = timerOf(mpTimerItem);
        QVERIFY2(pParent != nullptr, "the new timer is not in the timer unit");
        auto* pOffset = new TTimer(pParent, mpHost);
        pOffset->setName(qsl("An offset timer"));
        pOffset->setTime(QTime(0, 0, 2));
        mpHost->getTimerUnit()->registerTimer(pOffset);
        mpOffsetItem = new QTreeWidgetItem(mpTimerItem, QStringList{pOffset->getName()});
        mpOffsetItem->setData(0, Qt::UserRole, pOffset->getID());

        choose(mpTimerItem);
    }

    void cleanupTestCase()
    {
        mpTimerItem = nullptr;
        mpOffsetItem = nullptr;
        mpEditor = nullptr;
        mpHost = nullptr;
        mSession.stop();
    }

    void test_theIntervalIsOneSentence()
    {
        QVERIFY2(row() != nullptr, "the timers form has no interval row");
        QCOMPARE(row()->objectName(), qsl("editorTimerInterval"));

        const QStringList shown = reading();
        QVERIFY2(shown == QStringList({qsl("Fires every"), qsl("[1]"), qsl("h"), qsl("[2]"), qsl("min"), qsl("[3]"), qsl("s"), qsl("[4]"), qsl("ms")}),
                 qPrintable(qsl("the interval reads as: %1").arg(shown.join(qsl(" | ")))));

        // QFontInfo, not QFont::pointSize(): that answers -1 for any font set in pixels, so two
        // such fonts compare equal however far apart they are
        const qreal formSize = QFontInfo(mpEditor->mpTimersMainArea->font()).pointSizeF();
        QVERIFY2(formSize > 0.0, "the timers form's own font has no resolved size, so there is nothing to hold the fields to");
        for (QTimeEdit* pField : fields()) {
            QVERIFY2(pField->parentWidget() == row(), qPrintable(qsl("%1 is not in the interval row").arg(pField->objectName())));
            const qreal fieldSize = QFontInfo(pField->font()).pointSizeF();
            QVERIFY2(qAbs(fieldSize - formSize) <= 0.5,
                     qPrintable(qsl("%1 is set at %2pt while the form it is on runs at %3pt").arg(pField->objectName(), QString::number(fieldSize, 'f', 2), QString::number(formSize, 'f', 2))));
        }

        // A screen reader announces a field by its accessible name, never by the word beside it
        for (QTimeEdit* pField : fields()) {
            QVERIFY2(!pField->accessibleName().isEmpty(), qPrintable(qsl("%1 is announced by nothing but the word beside it").arg(pField->objectName())));
        }

        for (const QString& gone : {qsl("label_timer_hours"),
                                    qsl("label_timer_mins"),
                                    qsl("label_timer_secs"),
                                    qsl("label_timer_millis"),
                                    qsl("label_timer_hour_min_separator"),
                                    qsl("label_timer_min_sec_separator"),
                                    qsl("label_timer_sec_decimal_separator")}) {
            QVERIFY2(mpEditor->mpTimersMainArea->findChild<QWidget*>(gone) == nullptr, qPrintable(qsl("the timers form still carries %1").arg(gone)));
        }
    }

    void test_whatTheFieldsHoldIsWhatTheTimerHolds()
    {
        choose(mpTimerItem);
        mpEditor->mpTimersMainArea->timeEdit_timer_hours->setTime(QTime(1, 0, 0, 0));
        mpEditor->mpTimersMainArea->timeEdit_timer_minutes->setTime(QTime(0, 2, 0, 0));
        mpEditor->mpTimersMainArea->timeEdit_timer_seconds->setTime(QTime(0, 0, 3, 0));
        mpEditor->mpTimersMainArea->timeEdit_timer_msecs->setTime(QTime(0, 0, 0, 4));
        mpEditor->saveTimer();
        QTest::qWait(50ms);

        TTimer* pTimer = timerOf(mpTimerItem);
        QVERIFY2(pTimer != nullptr, "the timer is not in the timer unit");
        QCOMPARE(pTimer->getTime(), QTime(1, 2, 3, 4));

        choose(mpOffsetItem);
        choose(mpTimerItem);
        QCOMPARE(mpEditor->mpTimersMainArea->timeEdit_timer_hours->time().hour(), 1);
        QCOMPARE(mpEditor->mpTimersMainArea->timeEdit_timer_minutes->time().minute(), 2);
        QCOMPARE(mpEditor->mpTimersMainArea->timeEdit_timer_seconds->time().second(), 3);
        QCOMPARE(mpEditor->mpTimersMainArea->timeEdit_timer_msecs->time().msec(), 4);
    }

    void test_anOffsetTimerSaysItFiresOnce()
    {
        choose(mpTimerItem);
        QCOMPARE(shownWords(), wordsOf(everySentence()));

        choose(mpOffsetItem);
        QCOMPARE(shownWords(), wordsOf(onceSentence()));
        QCOMPARE(reading().count(qsl("[1]")), 1);

        choose(mpTimerItem);
        QCOMPARE(shownWords(), wordsOf(everySentence()));
    }

    // A drag onto another timer changes nothing the form was told to load, so the sentence has
    // to be re-read from the timer, not only when another timer is chosen
    void test_movingATimerUnderAnotherChangesTheSentence()
    {
        mpEditor->addTimer(false);
        QTest::qWait(100ms);
        QTreeWidgetItem* pMovedItem = mpEditor->mpCurrentTimerItem;
        QVERIFY2(pMovedItem != nullptr && pMovedItem != mpTimerItem, "addTimer() left no new timer to move");
        choose(pMovedItem);
        QCOMPARE(shownWords(), wordsOf(everySentence()));

        TTimer* pMoved = timerOf(pMovedItem);
        QVERIFY2(pMoved != nullptr && !pMoved->isOffsetTimer(), "the timer this moves is already an offset timer, so the move would change nothing");
        const int movedID = pMovedItem->data(0, Qt::UserRole).toInt();
        const int newParentID = mpTimerItem->data(0, Qt::UserRole).toInt();
        const int oldParentID = pMoved->getParent() ? pMoved->getParent()->getID() : 0;

        // The two halves of a drop, in TTreeWidget::rowsInserted()'s order. The tree row stays: the
        // timer unit decides the timer's kind, and moving the row would change the selection.
        mpEditor->slot_itemMoved(movedID, oldParentID, newParentID, 0, 0);
        mpHost->getTimerUnit()->reParentTimer(movedID, oldParentID, newParentID, 0, 0);
        QCoreApplication::processEvents();
        QTest::qWait(50ms);

        QVERIFY2(pMoved->isOffsetTimer(), "the move did not make the timer an offset timer, so there is nothing for the sentence to say");
        QCOMPARE(shownWords(), wordsOf(onceSentence()));
    }
};

#include "EditorTimerIntervalTest.moc"
MUDLET_GROUPED_TEST_MAIN(EditorTimerIntervalTest)
