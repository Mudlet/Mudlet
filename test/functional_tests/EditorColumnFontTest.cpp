/***************************************************************************
 *   Copyright (C) 2026 by Mudlet Developers                               *
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
 * A window or application font must reach the item forms in the right-hand column
 * under a stylesheet too, which in Qt's default mode stops a parent's font at any
 * polished widget; a widget's own size, weight or sheet-set size must still hold.
 *
 * Run with: ctest -R EditorColumnFontTest -V
 */

#include <QtTest/QtTest>
#include <chrono>

#include "HostDialogs.h"
#include "ProfileSessionTestHelper.h"
#include "Host.h"
#include "dlgAliasMainArea.h"
#include "dlgScriptsMainArea.h"
#include "dlgTimersMainArea.h"
#include "dlgTriggerEditor.h"
#include "dlgTriggerPatternEdit.h"
#include "mudlet.h"

#include "GroupedTest.h"

using namespace std::chrono_literals;

class EditorColumnFontTest : public QObject
{
    Q_OBJECT

private:
    TestProfile::Session mSession;
    dlgTriggerEditor* mpEditor = nullptr;
    Host* mpHost = nullptr;
    QFont mOriginalApplicationFont;
    const QString mProfileName = qsl("ColumnFont-Test-Profile");
    // Mimics the alias form's own sheet, plus a sheet-set size on one field
    const QString mFormSheet = qsl("QLineEdit { border: 1px solid red; } QLineEdit#lineEdit_alias_command { font-size: 7pt; }");
    static constexpr qreal scmSheetPointSize = 7.0;
    // Pinned by initTestCase(): no widget in the forms' .ui files carries a size of its own
    static constexpr qreal scmPinnedPointSize = 8.0;

    QFont fourPointsLarger(const QFont& base) const
    {
        QFont larger = base;
        larger.setPointSizeF(base.pointSizeF() + 4);
        return larger;
    }

    // QApplication::setFont() reaches a window by a posted event
    void setApplicationFont(const QFont& font)
    {
        QApplication::setFont(font);
        QCoreApplication::processEvents();
    }

    // The profile sheet lands on the editor window, where Qt's stylesheet mode stops the window's font
    void applyProfileStyleSheet()
    {
        QVERIFY(mpHost->setProfileStyleSheet(qsl("QLabel#nothingInParticular { color: red; }")));
        QVERIFY(mpEditor->testAttribute(Qt::WA_StyleSheet));
    }

    // Every link from the window down, so a failure names the link that dropped the size
    void verifyColumnFollows(const qreal expectedPointSize)
    {
        struct Probe
        {
            const char* name;
            const QWidget* widget;
        };
        const QList<Probe> probes = {
                {"window", mpEditor},
                {"frame_left", mpEditor->frame_left},
                {"treeWidget_aliases", mpEditor->treeWidget_aliases},
                {"treeWidget_aliases viewport", mpEditor->treeWidget_aliases->viewport()},
                {"frame_right", mpEditor->frame_right},
                {"splitter_right", mpEditor->splitter_right},
                {"mpNonCodeWidgets", mpEditor->mpNonCodeWidgets},
                {"mpAliasMainArea", mpEditor->mpAliasMainArea},
                {"lineEdit_alias_name", mpEditor->mpAliasMainArea->lineEdit_alias_name},
                {"label_alias_command (bold only)", mpEditor->mpAliasMainArea->label_alias_command},
                {"mpScriptsMainArea", mpEditor->mpScriptsMainArea},
                {"lineEdit_script_name", mpEditor->mpScriptsMainArea->lineEdit_script_name},
        };
        QStringList wrong;
        for (const auto& probe : probes) {
            const qreal actual = probe.widget->font().pointSizeF();
            if (!qFuzzyCompare(actual, expectedPointSize)) {
                wrong << qsl("%1 is %2pt").arg(QLatin1String(probe.name)).arg(actual);
            }
        }
        QVERIFY2(wrong.isEmpty(), qPrintable(qsl("expected %1pt under the window, but: %2").arg(expectedPointSize).arg(wrong.join(qsl(", ")))));
    }

private slots:
    void initTestCase()
    {
        if (TestProfile::Session::configCannotBeRedirected()) {
            QSKIP(TestProfile::Session::configCannotBeRedirectedReason());
        }

        QVERIFY2(mSession.start(mProfileName), qPrintable(mSession.why()));
        mpHost = mSession.host();

        mudlet::self()->slot_showScriptDialog();
        QTest::qWait(100ms);

        mpEditor = HostDialogs::of(mpHost).mpEditorDialog;
        QVERIFY2(mpEditor != nullptr, "Editor dialog should be created");
        mOriginalApplicationFont = QApplication::font();

        // Bold only: the resolve mask lets it take the window's size while staying bold
        QFont boldOnly;
        boldOnly.setBold(true);
        mpEditor->mpAliasMainArea->label_alias_command->setFont(boldOnly);

        QFont pinned = mpEditor->mpTimersMainArea->label_timer_time->font();
        pinned.setPointSizeF(scmPinnedPointSize);
        mpEditor->mpTimersMainArea->label_timer_time->setFont(pinned);
        QCOMPARE(mpEditor->mpTimersMainArea->label_timer_time->font().pointSizeF(), scmPinnedPointSize);
    }

    void cleanupTestCase()
    {
        mpEditor = nullptr;
        mpHost = nullptr;
        mSession.stop();
    }

    // The sheets come off first, so the font resets after them reach every widget whatever the
    // code under test does about sheets
    void cleanup()
    {
        if (mpHost) {
            mpHost->setProfileStyleSheet(QString());
        }
        if (mpEditor) {
            mpEditor->mpAliasMainArea->setStyleSheet(QString());
            // an unresolved QFont hands the window back to the application font
            mpEditor->setFont(QFont());
        }
        setApplicationFont(mOriginalApplicationFont);
    }

    void testFontReachesRightColumn_data()
    {
        QTest::addColumn<QString>("sheet");
        QTest::addColumn<bool>("viaApplication");
        QTest::newRow("window font") << qsl("none") << false;
        QTest::newRow("application font") << qsl("none") << true;
        QTest::newRow("window font, profile stylesheet on the window") << qsl("profile") << false;
        QTest::newRow("application font, profile stylesheet on the window") << qsl("profile") << true;
        QTest::newRow("window font, stylesheet on the alias form") << qsl("form") << false;
        QTest::newRow("application font, stylesheet on the alias form") << qsl("form") << true;
    }

    void testFontReachesRightColumn()
    {
        QFETCH(QString, sheet);
        QFETCH(bool, viaApplication);
        if (sheet == qsl("profile")) {
            applyProfileStyleSheet();
        } else if (sheet == qsl("form")) {
            mpEditor->mpAliasMainArea->setStyleSheet(mFormSheet);
            QVERIFY(mpEditor->mpAliasMainArea->lineEdit_alias_name->testAttribute(Qt::WA_StyleSheet));
            QCOMPARE(mpEditor->mpAliasMainArea->lineEdit_alias_command->font().pointSizeF(), scmSheetPointSize);
        }

        const QFont larger = fourPointsLarger(viaApplication ? QApplication::font() : mpEditor->font());
        if (viaApplication) {
            setApplicationFont(larger);
        } else {
            mpEditor->setFont(larger);
        }

        verifyColumnFollows(larger.pointSizeF());
        QVERIFY2(mpEditor->mpAliasMainArea->label_alias_command->font().bold(), "a label that was only bold has to stay bold");
        QCOMPARE(mpEditor->mpTimersMainArea->label_timer_time->font().pointSizeF(), scmPinnedPointSize);
        QCOMPARE(mpEditor->mpAliasMainArea->lineEdit_alias_command->font().pointSizeF(), sheet == qsl("form") ? scmSheetPointSize : larger.pointSizeF());
    }

    // Under a sheet, Qt gives a widget created later the application font rather than its parent's
    void testNewPatternRowTakesWindowFontUnderProfileStyleSheet()
    {
        // A QLabel class font, as the macOS theme registers: such a widget takes from its parent
        // only what the parent's resolve mask names. Built, not copied: a copy of the application
        // font is taken for that font and resolves against the parent after all.
        const QFont labelFont(QApplication::font().family(), qRound(QApplication::font().pointSizeF()));
        QVERIFY(!labelFont.isCopyOf(QApplication::font()));
        QApplication::setFont(labelFont, "QLabel");
        applyProfileStyleSheet();
        const QFont larger = fourPointsLarger(mpEditor->font());
        mpEditor->setFont(larger);

        const qsizetype rowsBefore = mpEditor->mTriggerPatternEdit.size();
        mpEditor->showPatternItems(rowsBefore + 1);
        QCOMPARE(mpEditor->mTriggerPatternEdit.size(), rowsBefore + 1);
        auto* pRow = mpEditor->mTriggerPatternEdit.last();
        QCOMPARE(pRow->comboBox_patternType->font().pointSizeF(), larger.pointSizeF());
        // Not label_patternNumber: applyPatternWidgetStyle() pins it to the profile's display font.
        // label_prompt is interface text and a QLabel, the class-font case.
        QCOMPARE(pRow->label_prompt->font().pointSizeF(), larger.pointSizeF());
    }

    // Setting the same sheet again, as scripts do, repolishes every widget and hands each its
    // pre-sheet font back resolved against the application font; so does taking the sheet off
    void testWindowFontSurvivesTheProfileStyleSheetBeingSetAgain()
    {
        applyProfileStyleSheet();
        const QFont larger = fourPointsLarger(mpEditor->font());
        mpEditor->setFont(larger);
        verifyColumnFollows(larger.pointSizeF());

        applyProfileStyleSheet();
        verifyColumnFollows(larger.pointSizeF());

        QVERIFY(mpHost->setProfileStyleSheet(QString()));
        QVERIFY(!mpEditor->testAttribute(Qt::WA_StyleSheet));
        verifyColumnFollows(larger.pointSizeF());
    }
};

#include "EditorColumnFontTest.moc"
MUDLET_GROUPED_TEST_MAIN(EditorColumnFontTest)
