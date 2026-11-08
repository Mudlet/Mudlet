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
 * The trigger options strip between the head row and the patterns: how it wraps, one
 * vertical centre per line, the segments' shared hairline, screen-reader names, the
 * tab order, the stepper hover and the check mark.
 *
 * Run with: ctest -R EditorTriggerOptionsStripTest -V
 */

#include <QAbstractButton>
#include <QCheckBox>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QRadioButton>
#include <QScrollArea>
#include <QScrollBar>
#include <QSpinBox>
#include <QScopeGuard>
#include <QSplitter>
#include <QStyle>
#include <QStyleOptionButton>
#include <QStyleOptionSpinBox>
#include <QTemporaryDir>
#include <QToolButton>
#include <QTreeWidget>
#include <QtTest/QtTest>
#include <chrono>
#include <cstdlib>

#include "Host.h"
#include "HostDialogs.h"
#include "MudletApp.h"
#include "MudletInstanceCoordinator.h"
#include "PixelTestHelper.h"
#include "PortableModeTestHelper.h"
#include "ProfileTestHelper.h"
#include "TTrigger.h"
#include "TelnetServerStub.h"
#include "ctelnet.h"
#include "dlgTriggerEditor.h"
#include "dlgTriggerPatternEdit.h"
#include "dlgTriggersMainArea.h"
#include "mudlet.h"
#include "uiDesign.h"

#include "GroupedTest.h"

using namespace std::chrono_literals;

class EditorTriggerOptionsStripTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    TelnetServerStub* mpServer = nullptr;
    dlgTriggerEditor* mpEditor = nullptr;
    Host* mpHost = nullptr;
    QTreeWidgetItem* mpThreePatternRow = nullptr;
    QTreeWidgetItem* mpOnePatternRow = nullptr;
    const QString mProfileName = qsl("EditorTriggerOptionsStrip-Test-Profile");
    QString mPort;
    const QString mLocalhost = qsl("localhost");

    // Strip widths, not window widths: the sidebar and item tree take a fixed amount off the left.
    // At 1400px the whole strip fits on one line; 1000px is about what a maximised editor leaves it.
    static constexpr int scmWideStrip = 1400;
    static constexpr int scmNarrowStrip = 1000;
    static constexpr int scmTallEditor = 900;
    // Short enough that the form wants more than the code pane's share leaves it, also with Windows'
    // smaller fonts, where the form wants 270px of the 393px a 500px window gives the two panes
    static constexpr int scmShortEditor = 440;
    // The seam is placed by hand and lands a pixel or two off what was asked for
    static constexpr int scmSeamTolerance = 2;
    // Mirror the file-local scmEditorSegmentPaddingHorizontal in src/dlgTriggerEditor.cpp and its
    // hairlines; a radio indicator that came back would add some twenty pixels on top
    static constexpr int scmSegmentPadding = 12;
    static constexpr int scmSegmentSlack = 6;
    // Mirrors the editor's file-local scmEditorOptionsSpinBoxSlack
    static constexpr int scmSpinBoxSlack = 2;
    // Mirrors scmEditorRowControlGap, the strip FlowLayout's vertical spacing
    static constexpr int scmStripLineGap = 8;
    // A centre mapped into the window lands on a whole pixel, so level controls can read a pixel apart
    static constexpr int scmCentreSlack = 1;
    // What stands left of the form is measured, not named, so the strip width can be out by this
    static constexpr int scmStripWidthSlack = 2;
    // Ink distance summed over R, G and B. A hairline is drawn flat, so this is only slack; the
    // segment beside it measured 379 away.
    static constexpr int scmInkSlack = 40;
    // A 7px chevron is all antialiased edge, so its darkest pixel is a blend: measured 64 from the
    // accent under the pointer and 152 at rest
    static constexpr int scmChevronInkSlack = 100;
    // A minimum hover shift, so a chevron that never changed cannot pass by sitting near the line
    static constexpr int scmChevronInkShift = 40;
    // Inside the padding, so neither the edge hairline nor the word is sampled
    static constexpr int scmFillDepth = 4;
    // Below this a pixel is the fill's own noise rather than part of the mark
    static constexpr int scmMarkInkSlack = 40;

    void deleteProfileDirectory(const QString& profileName)
    {
        QDir dir(MudletApp::getMudletPath(enums::profileHomePath, profileName));
        if (dir.exists()) {
            dir.removeRecursively();
        }
    }

    // Qt caches a control's size hint and a check-state change does not invalidate it; a
    // StyleChange does, so padding that fails to make up for a given-up hairline shows now
    void remeasure(const QList<QWidget*>& widgets)
    {
        for (QWidget* pWidget : widgets) {
            QEvent styleChange(QEvent::StyleChange);
            QCoreApplication::sendEvent(pWidget, &styleChange);
            pWidget->updateGeometry();
        }
        settle();
    }

    dlgTriggersMainArea* form() const { return mpEditor->mpTriggersMainArea; }

    QGroupBox* optionsRow() const { return form()->findChild<QGroupBox*>(qsl("editorOptionsRow")); }

    int topEdgeOf(const QWidget* pWidget) const { return pWidget->mapTo(mpEditor, QPoint(0, 0)).y(); }

    int leftEdgeOf(const QWidget* pWidget) const { return pWidget->mapTo(mpEditor, QPoint(0, 0)).x(); }

    void chooseTrigger(QTreeWidgetItem* pRow)
    {
        mpEditor->slot_showTriggers();
        settle();
        mpEditor->treeWidget_triggers->setCurrentItem(pRow);
        mpEditor->slot_triggerSelected(pRow);
        settle();
        mpEditor->hideSystemMessageArea();
        settle();
    }

    // Settled twice: the form's new width arrives on the next event-loop pass and the seam's
    // refit on the one after
    // The strip lays out on posted events alone, so this skips TestPixels::settle()'s fixed wait, which is there
    // for the slowest of the tests sharing it
    static void settle()
    {
        QCoreApplication::sendPostedEvents();
        QCoreApplication::processEvents();
        QCoreApplication::sendPostedEvents();
        QCoreApplication::processEvents();
    }

    void resizeTheEditor(const int width, const int height)
    {
        mpEditor->resize(width, height);
        settle();
        settle();
    }

    // Measured, not named: a hard-coded width for the sidebar and item tree would silently
    // measure a different strip once either moves
    void resizeToStripWidth(const int stripWidth, const int height)
    {
        resizeTheEditor(stripWidth + 600, height);
        resizeTheEditor(stripWidth + mpEditor->width() - optionsRow()->width(), height);
        QVERIFY2(std::abs(optionsRow()->width() - stripWidth) <= scmStripWidthSlack,
                 qPrintable(qsl("the strip is %1px wide in a %2px window, where %3px was asked for").arg(optionsRow()->width()).arg(mpEditor->width()).arg(stripWidth)));
    }

    QList<QWidget*> focusableControlsOnTheStrip() const
    {
        QList<QWidget*> controls;
        for (QWidget* pWidget : optionsRow()->findChildren<QWidget*>()) {
            // Qt's inner parts, such as a spin box's line edit, are named by the control around them
            if (pWidget->objectName().startsWith(qsl("qt_"))) {
                continue;
            }
            if (pWidget->focusPolicy() != Qt::NoFocus && pWidget->isVisibleTo(mpEditor)) {
                controls << pWidget;
            }
        }
        return controls;
    }

    int verticalCentreOf(const QWidget* pWidget) const { return pWidget->mapTo(mpEditor, pWidget->rect().center()).y(); }

    QList<QWidget*> tabChainFromTheCommandField() const
    {
        QList<QWidget*> chain;
        QWidget* pAt = form()->lineEdit_trigger_command;
        for (int step = 0; step < 400; ++step) {
            pAt = pAt->nextInFocusChain();
            if (!pAt || pAt == form()->lineEdit_trigger_command) {
                break;
            }
            if (pAt->focusPolicy() != Qt::NoFocus && pAt->isVisibleTo(mpEditor)) {
                chain << pAt;
            }
        }
        return chain;
    }

    static QString describe(const QWidget* pWidget) { return pWidget->objectName().isEmpty() ? QString::fromLatin1(pWidget->metaObject()->className()) : pWidget->objectName(); }

    static QRect inTheGrabsPixels(const QImage& shot, const QRect& box)
    {
        const qreal ratio = shot.devicePixelRatio();
        return QRect(QPoint(qRound(box.left() * ratio), qRound(box.top() * ratio)), QPoint(qRound((box.right() + 1) * ratio) - 1, qRound((box.bottom() + 1) * ratio) - 1)).intersected(shot.rect());
    }

    // Depth 0 is the outermost pixel, where a one-pixel border is drawn; a few pixels in is the fill
    static QColor colourInsideTheEdge(const QImage& shot, const QWidget* pSegment, const bool rightHandSide, const int depth)
    {
        const QRect box = inTheGrabsPixels(shot, pSegment->geometry().adjusted(depth, 0, -depth, 0));
        return shot.pixelColor(rightHandSide ? box.right() : box.left(), box.center().y());
    }

    // Sampled from a grab of the window: a control or form grabbed alone is painted against its
    // own palette where it has no background, so the column the form sits on is missing
    static QRect markBoxIn(const QAbstractButton* pButton, const QWidget* pWindow, const QImage& shot)
    {
        QStyleOptionButton option;
        option.initFrom(pButton);
        QRect box = pButton->style()->subElementRect(QStyle::SE_CheckBoxIndicator, &option, pButton);
        box.moveTopLeft(pButton->mapTo(pWindow, box.topLeft()));
        return inTheGrabsPixels(shot, box);
    }

    static int markedPixelsIn(const QImage& shot, const QRect& box, const QColor& fill)
    {
        const QRect inside = box.adjusted(box.width() / 4, box.height() / 4, -box.width() / 4, -box.height() / 4);
        int marked = 0;
        for (int y = inside.top(); y <= inside.bottom(); ++y) {
            for (int x = inside.left(); x <= inside.right(); ++x) {
                if (TestPixels::distance(shot.pixelColor(x, y), fill) > scmMarkInkSlack) {
                    ++marked;
                }
            }
        }
        return marked;
    }

    static qreal outlineRatioOf(const QImage& shot, const QRect& box, const QColor& behind)
    {
        QColor outline = behind;
        for (int x = box.left(); x <= box.right(); ++x) {
            const QColor sample = shot.pixelColor(x, box.top());
            if (uiDesign::contrastRatio(sample, behind) > uiDesign::contrastRatio(outline, behind)) {
                outline = sample;
            }
        }
        return uiDesign::contrastRatio(outline, behind);
    }

    static int nearestToTheAccentIn(const QImage& shot, const QRect& box, const QColor& accent)
    {
        int nearest = 3 * 255;
        const QRect scanned = inTheGrabsPixels(shot, box);
        for (int y = scanned.top(); y <= scanned.bottom(); ++y) {
            for (int x = scanned.left(); x <= scanned.right(); ++x) {
                nearest = std::min(nearest, TestPixels::distance(shot.pixelColor(x, y), accent));
            }
        }
        return nearest;
    }

private slots:
    void initTestCase()
    {
        if (portableMarkerPresent()) {
            QSKIP("portable.txt present - it takes precedence over XDG_CONFIG_HOME, so the config dir cannot be redirected");
        }

        QVERIFY(mConfigDir.isValid());
        QVERIFY(QDir().mkpath(qsl("%1/mudlet/profiles").arg(mConfigDir.path())));
        mSavedXdg = qgetenv("XDG_CONFIG_HOME");
        qputenv("XDG_CONFIG_HOME", mConfigDir.path().toUtf8());

        mpServer = new TelnetServerStub(qApp);
        mpServer->start(mLocalhost, 0);
        QVERIFY2(mpServer->isListening(), qPrintable(qsl("TelnetServerStub failed to start: %1").arg(mpServer->errorString())));
        mPort = QString::number(mpServer->serverPort());
        mudlet::start();
        mudlet::self()->setupConfig();
        QCOMPARE(MudletApp::getMudletPath(enums::mainPath), qsl("%1/mudlet").arg(mConfigDir.path()));
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>(qsl("MudletInstanceCoordinator")));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);
        deleteProfileDirectory(mProfileName);

        mpHost = TestProfile::create(mProfileName, mLocalhost, mPort);
        QVERIFY2(mpHost != nullptr, "No active host available for the test.");
        QSignalSpy connected(&(mpHost->mTelnet), &cTelnet::signal_connected);
        QVERIFY2(connected.wait(2s), "Could not connect with the host.");

        mudlet::self()->slot_showTriggerDialog();
        QTest::qWait(100ms);
        mpEditor = HostDialogs::of(mpHost).mpEditorDialog;
        QVERIFY2(mpEditor != nullptr, "Editor dialog should be created");
        mpEditor->resize(scmWideStrip + 600, scmTallEditor);
        settle();

        mpEditor->slot_showTriggers();
        mpEditor->addTrigger(false);
        settle();
        mpThreePatternRow = mpEditor->mpCurrentTriggerItem;
        QVERIFY2(mpThreePatternRow != nullptr, "the three-pattern trigger these cases edit could not be made");
        mpEditor->showPatternItems(3);
        mpEditor->mTriggerPatternEdit.at(0)->singleLineTextEdit_pattern->setPlainText(qsl("There's water ahead of you. You'll have to swim in that direction."));
        mpEditor->mTriggerPatternEdit.at(1)->singleLineTextEdit_pattern->setPlainText(qsl("You'll have to swim to make it through the water in that direction."));
        mpEditor->mTriggerPatternEdit.at(2)->singleLineTextEdit_pattern->setPlainText(qsl("The water is too deep for you to walk that way, you must swim."));
        mpEditor->slot_saveEdits();
        settle();

        mpEditor->addTrigger(false);
        settle();
        mpOnePatternRow = mpEditor->mpCurrentTriggerItem;
        QVERIFY2(mpOnePatternRow != nullptr, "the one-pattern trigger these cases edit could not be made");
        mpEditor->showPatternItems(1);
        mpEditor->mTriggerPatternEdit.at(0)->singleLineTextEdit_pattern->setPlainText(qsl("You'll have to SWIM"));
        mpEditor->slot_saveEdits();
        settle();
    }

    void cleanupTestCase()
    {
        mpEditor = nullptr;
        mpHost = nullptr;
        delete mpServer;
        mpServer = nullptr;
        if (mudlet::self()) {
            deleteProfileDirectory(mProfileName);
            delete mudlet::self();
        }
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
    }

    void init()
    {
        if (!mpEditor) {
            return;
        }
        resizeTheEditor(scmWideStrip + 600, scmTallEditor);
        // The editor opens with the strip closed
        mpEditor->setTriggerOptionsShown(true);
        settle();
    }

    void test_theStripSitsBetweenTheNameAndThePatterns()
    {
        chooseTrigger(mpThreePatternRow);

        QVERIFY2(optionsRow() != nullptr, "the trigger form has no options strip");
        QVERIFY2(optionsRow()->isVisible(), "the options strip was opened in init() and is not on show");

        const int headTop = topEdgeOf(form()->widget_top);
        const int stripTop = topEdgeOf(optionsRow());
        const int patternsTop = topEdgeOf(form()->widget_left);
        qInfo().noquote() << qsl("  head row at %1, the strip at %2, patterns at %3").arg(headTop).arg(stripTop).arg(patternsTop);

        QVERIFY2(headTop < stripTop, "the options strip is not under the row the name is typed on");
        QVERIFY2(stripTop < patternsTop, "the pattern list is not under the options strip");

        QLabel* pName = form()->label_trigger_name;
        QLabel* pLead = mpEditor->mpLabel_optionsRow;
        QVERIFY2(pLead->property("editorRowLabel").toBool(), qPrintable(qsl("the word \"%1\" is not written in the quiet ink a form's scaffolding takes").arg(pLead->text())));
        QCOMPARE(leftEdgeOf(pLead), leftEdgeOf(pName));
        qInfo().noquote() << qsl("  the Name label is %1px wide and the lead word \"%2\" %3px, with the Name field at x=%4 and the strip at x=%5")
                                     .arg(pName->width())
                                     .arg(pLead->text())
                                     .arg(pLead->width())
                                     .arg(leftEdgeOf(form()->lineEdit_trigger_name))
                                     .arg(leftEdgeOf(optionsRow()));
        QCOMPARE(leftEdgeOf(optionsRow()), leftEdgeOf(form()->lineEdit_trigger_name));
        QCOMPARE(optionsRow()->accessibleName(), pLead->text());
    }

    void test_aNarrowFormWrapsTheStripRatherThanThePatternRows()
    {
        chooseTrigger(mpThreePatternRow);
        resizeToStripWidth(scmWideStrip, scmTallEditor);
        const int wideStrip = optionsRow()->height();
        const int widePane = mpEditor->splitter_right->sizes().at(0);

        resizeToStripWidth(scmNarrowStrip, scmTallEditor);
        const int narrowStrip = optionsRow()->height();
        const int narrowPane = mpEditor->splitter_right->sizes().at(0);
        const int twoLines = 2 * uiDesign::scmInputHeight + scmStripLineGap;
        qInfo().noquote() << qsl("  a %1px strip is %2px tall in a form pane of %3; a %4px one is %5px in a pane of %6, where two lines are %7px")
                                     .arg(scmWideStrip)
                                     .arg(wideStrip)
                                     .arg(widePane)
                                     .arg(scmNarrowStrip)
                                     .arg(narrowStrip)
                                     .arg(narrowPane)
                                     .arg(twoLines);

        QVERIFY2(narrowStrip > wideStrip,
                 qPrintable(qsl("the strip is %1px tall at %2px wide and %3px at %4px - it did not wrap onto another line").arg(narrowStrip).arg(scmNarrowStrip).arg(wideStrip).arg(scmWideStrip)));
        QVERIFY2(narrowStrip <= twoLines,
                 qPrintable(qsl("the strip is %1px tall at %2px wide, which is more than the %3px two lines come to - the words on it are too long for this width")
                                    .arg(narrowStrip)
                                    .arg(scmNarrowStrip)
                                    .arg(twoLines)));
        QVERIFY2(!mpEditor->mpScrollArea->horizontalScrollBar()->isVisible(),
                 qPrintable(qsl("the pattern list is scrolling sideways in a %1px window, with %2px for a row that wants %3px")
                                    .arg(mpEditor->width())
                                    .arg(mpEditor->mpScrollArea->viewport()->width())
                                    .arg(mpEditor->mpWidget_triggerItems->minimumSizeHint().width())));
        QVERIFY2(narrowPane > widePane,
                 qPrintable(qsl("the strip grew by %1px and the form pane stayed at %2px, so the extra line is being drawn over the patterns").arg(narrowStrip - wideStrip).arg(narrowPane)));
    }

    // FlowLayout puts every item at the top of its line, so a short check box rides above a taller spin box
    void test_everythingOnOneLineOfTheStripSharesAVerticalCentre()
    {
        chooseTrigger(mpThreePatternRow);
        resizeToStripWidth(scmWideStrip, scmTallEditor);

        const QList<QWidget*> controls = focusableControlsOnTheStrip();
        QVERIFY2(controls.size() >= 9, qPrintable(qsl("only %1 focusable controls were found on the strip, so this walk is not covering it").arg(controls.size())));

        QStringList read;
        QStringList failures;
        const int centre = verticalCentreOf(controls.first());
        for (QWidget* pControl : controls) {
            const int at = verticalCentreOf(pControl);
            read << qsl("%1 at %2").arg(describe(pControl)).arg(at);
            if (std::abs(at - centre) > scmCentreSlack) {
                failures << qsl("%1 is centred at %2 against %3 for %4, %5px out").arg(describe(pControl)).arg(at).arg(centre).arg(describe(controls.first())).arg(std::abs(at - centre));
            }
        }
        qInfo().noquote() << qsl("  a %1px strip is %2px tall and holds: %3").arg(scmWideStrip).arg(optionsRow()->height()).arg(read.join(qsl(", ")));
        QVERIFY2(failures.isEmpty(), qPrintable(failures.join(qsl("\n"))));
        // One line, so the shared centre is not a coincidence between two lines
        QCOMPARE(optionsRow()->height(), uiDesign::scmInputHeight);
    }

    void test_theNumberBoxesAreOnlyAsWideAsTheirLargestNumber()
    {
        chooseTrigger(mpThreePatternRow);
        resizeToStripWidth(scmWideStrip, scmTallEditor);

        QStringList misses;
        QStringList read;
        for (QSpinBox* pBox : {mpEditor->mpSpinBox_matchWithinLines, form()->spinBox_stayOpen}) {
            QVERIFY2(pBox->isVisible(), qPrintable(qsl("%1 is not on show, so its width says nothing").arg(pBox->objectName())));

            const QFontMetrics metrics = pBox->fontMetrics();
            const QString largest = QString::number(pBox->maximum());
            QChar widest = QLatin1Char('0');
            for (char digit = '0'; digit <= '9'; ++digit) {
                if (metrics.horizontalAdvance(QLatin1Char(digit)) > metrics.horizontalAdvance(widest)) {
                    widest = QLatin1Char(digit);
                }
            }
            const int room = metrics.horizontalAdvance(QString(largest.length(), widest)) + 2 * (uiDesign::scmInputPaddingHorizontal + uiDesign::scmInputBorderWidth) + uiDesign::scmInputStepperWidth
                             + scmSpinBoxSlack;

            QStyleOptionSpinBox option;
            option.initFrom(pBox);
            option.subControls = QStyle::SC_All;
            const QRect editField = pBox->style()->subControlRect(QStyle::CC_SpinBox, &option, QStyle::SC_SpinBoxEditField, pBox);
            const int largestNumber = metrics.horizontalAdvance(largest);

            read << qsl("%1 is %2px wide against the %3px %4 comes to, and types it in %5px").arg(pBox->objectName()).arg(pBox->width()).arg(room).arg(largest).arg(editField.width());

            if (pBox->width() > room) {
                misses << qsl("%1 is %2px wide, where %3 and the room the field's rule leaves round it come to %4px").arg(pBox->objectName()).arg(pBox->width()).arg(largest).arg(room);
            }
            if (editField.width() < largestNumber) {
                misses << qsl("%1 leaves %2px to type in, where %3 is %4px wide").arg(pBox->objectName()).arg(editField.width()).arg(largest).arg(largestNumber);
            }
        }
        qInfo().noquote() << qsl("  %1").arg(read.join(qsl("; ")));
        QVERIFY2(misses.isEmpty(), qPrintable(qsl("\n  %1").arg(misses.join(qsl("\n  ")))));
    }

    void test_theCodePaneKeepsItsFloorInAShortWindow()
    {
        chooseTrigger(mpThreePatternRow);
        resizeToStripWidth(scmNarrowStrip, scmShortEditor);

        const QList<int> sizes = mpEditor->splitter_right->sizes();
        QVERIFY2(sizes.size() >= 2, "the right hand splitter has lost a pane");
        const int paneTotal = sizes.at(0) + sizes.at(1);
        const int floor = dlgTriggerEditor::scmEditorSourcePaneFloor;
        qInfo().noquote() << qsl("  in a %1x%2 window the panes are %3 / %4 of %5, where the code pane keeps %6")
                                     .arg(mpEditor->width())
                                     .arg(scmShortEditor)
                                     .arg(sizes.at(0))
                                     .arg(sizes.at(1))
                                     .arg(paneTotal)
                                     .arg(floor);

        QVERIFY2(std::abs(sizes.at(0) - (paneTotal - floor)) <= scmSeamTolerance,
                 qPrintable(qsl("the form is at %1 of the %2 the two panes have, rather than at the %3 the code pane's floor leaves it - this window is not short enough for the case to say anything")
                                    .arg(sizes.at(0))
                                    .arg(paneTotal)
                                    .arg(paneTotal - floor)));
        QVERIFY2(sizes.at(1) >= floor - scmSeamTolerance,
                 qPrintable(qsl("the form was given %1 of the %2 the two panes have, leaving the code pane %3 against the %4 it keeps").arg(sizes.at(0)).arg(paneTotal).arg(sizes.at(1)).arg(floor)));
    }

    void test_theSegmentsWriteTheMatchingModeIntoTheHiddenSpinBox()
    {
        chooseTrigger(mpThreePatternRow);

        QRadioButton* pAny = mpEditor->mpRadioButton_matchAny;
        QRadioButton* pAll = mpEditor->mpRadioButton_matchAll;
        QSpinBox* pWithin = mpEditor->mpSpinBox_matchWithinLines;

        pAll->setChecked(true);
        settle();
        QVERIFY2(!pAny->isChecked(), "both segments are on at once, so they are no longer one exclusive choice");
        QVERIFY2(mpEditor->mpWidget_matchWithinRow->isEnabled(), "the line count is greyed out in the All mode, which is the mode it belongs to");
        pWithin->setValue(4);
        settle();
        QCOMPARE(form()->spinBox_lineMargin->value(), 4);

        pAny->setChecked(true);
        settle();
        QVERIFY2(!pAll->isChecked(), "choosing Any left All chosen as well");
        QVERIFY2(!mpEditor->mpWidget_matchWithinRow->isEnabled(), "the line count is still available in the Any mode, which carries none");
        QCOMPARE(form()->spinBox_lineMargin->value(), -1);

        const int textWidth = pAll->fontMetrics().horizontalAdvance(pAll->text());
        const int roundTheText = pAll->width() - textWidth;
        qInfo().noquote() << qsl("  \"%1\" is %2px wide against %3px of text, so %4px is round it").arg(pAll->text()).arg(pAll->width()).arg(textWidth).arg(roundTheText);
        QVERIFY2(pAll->property("editorSegment").toBool(), "the segment does not carry the property its rules select on");
        QVERIFY2(roundTheText <= 2 * (scmSegmentPadding + uiDesign::scmInputBorderWidth) + scmSegmentSlack,
                 qPrintable(qsl("\"%1\" is %2px wide for %3px of text - %4px round it, which is a dot and the gap after it rather than the padding alone")
                                    .arg(pAll->text())
                                    .arg(pAll->width())
                                    .arg(textWidth)
                                    .arg(roundTheText)));
    }

    void test_onceMorePatternsAreThereTheModesBecomeAvailable()
    {
        chooseTrigger(mpOnePatternRow);
        QCOMPARE(mpEditor->mVisiblePatternCount, 1);
        QVERIFY2(!mpEditor->mpWidget_matchModeRows->isEnabled(), "the two segments are available on a trigger with one pattern, which has nothing to combine");
        QVERIFY2(!mpEditor->mpWidget_matchWithinRow->isEnabled(), "the line count is available on a trigger with one pattern");
        QVERIFY2(!mpEditor->mpLabel_matchModeHint->isHidden(), "the caption saying why the modes are greyed out is not on show");

        chooseTrigger(mpThreePatternRow);
        QCOMPARE(mpEditor->mVisiblePatternCount, 3);
        QVERIFY2(mpEditor->mpWidget_matchModeRows->isEnabled(), "the two segments are still greyed out on a trigger with three patterns");
        QVERIFY2(mpEditor->mpLabel_matchModeHint->isHidden(), "the caption is still on show on a trigger that has patterns to combine");
    }

    void test_aChoiceTurnsItsOwnSwitchOn()
    {
        chooseTrigger(mpThreePatternRow);
        form()->checkBox_soundTrigger->setChecked(false);
        form()->checkBox_triggerColorizer->setChecked(false);
        mpEditor->showTriggerSoundFile(QString());
        settle();
        QVERIFY2(!form()->toolButton_clearSoundFile->isVisible(), "the cross that forgets the sound file is there with no file to forget");

        const QString chosen = qsl("/tmp/EditorTriggerOptionsStripTest/water-splash.wav");
        mpEditor->mpUndoStack->clear();
        mpEditor->acceptTriggerSoundFile(chosen);
        settle();
        QVERIFY2(form()->checkBox_soundTrigger->isChecked(), "choosing a sound file left the sound switched off, so the file is set and silent");
        QCOMPARE(mpEditor->triggerSoundFilePath(), chosen);
        QVERIFY2(form()->lineEdit_soundFile->text().contains(qsl("water-splash")), qPrintable(qsl("the field reads \"%1\" rather than the file's name").arg(form()->lineEdit_soundFile->text())));
        QVERIFY2(form()->toolButton_clearSoundFile->isVisible(), "the cross that forgets the sound file is away with a file set");
        // One undo entry for the file, one for the switch
        qInfo().noquote() << qsl("  choosing a sound file pushed %1 undo entries").arg(mpEditor->mpUndoStack->count());
        QCOMPARE(mpEditor->mpUndoStack->count(), 2);

        mpEditor->slot_clearSoundFile();
        settle();
        QVERIFY2(!form()->toolButton_clearSoundFile->isVisible(), "the cross is still there after the file it forgets was forgotten");
        QVERIFY2(mpEditor->triggerSoundFilePath().isEmpty(), "the field still stands for a file after being cleared");

        mpEditor->mpUndoStack->clear();
        mpEditor->turnTriggerHighlightOn();
        settle();
        QVERIFY2(form()->checkBox_triggerColorizer->isChecked(), "choosing a colour left the highlight switched off, so the colour is set and unused");
        QCOMPARE(mpEditor->mpUndoStack->count(), 1);
    }

    void test_everyControlOnTheStripNamesItselfToAScreenReader()
    {
        chooseTrigger(mpThreePatternRow);

        const QList<QWidget*> controls = focusableControlsOnTheStrip();
        QVERIFY2(controls.size() >= 9, qPrintable(qsl("only %1 focusable controls were found on the strip, so this walk is not covering it").arg(controls.size())));
        QStringList failures;
        for (QWidget* pControl : controls) {
            if (pControl->accessibleName().isEmpty()) {
                failures << qsl("%1 has no accessible name").arg(describe(pControl));
            }
            const QString description = pControl->accessibleDescription();
            if (description.isEmpty()) {
                failures << qsl("%1 has no accessible description, so a screen reader falls back to its rich text tooltip").arg(describe(pControl));
            } else if (description.contains(QLatin1Char('<'))) {
                failures << qsl("%1 is described as \"%2\", which is markup being read out").arg(describe(pControl), description);
            }
        }
        qInfo().noquote() << qsl("  %1 focusable controls on the strip were read").arg(controls.size());
        QVERIFY2(failures.isEmpty(), qPrintable(failures.join(qsl("\n"))));

        // The visible words are shortened to fit; a screen reader reaches each control on its own,
        // so it is given the full name
        const QList<QPair<QWidget*, QString>> saidInFull{{mpEditor->mpRadioButton_matchAny, qsl("Any pattern")},
                                                         {mpEditor->mpRadioButton_matchAll, qsl("All patterns")},
                                                         {form()->checkBox_perlSlashGOption, qsl("Every occurrence in a line")},
                                                         {form()->checkBox_filterTrigger, qsl("Only pass matches to children")},
                                                         {form()->checkBox_soundTrigger, qsl("Play a sound")},
                                                         {form()->checkBox_triggerColorizer, qsl("Highlight matches")}};
        for (const auto& [pControl, name] : saidInFull) {
            QVERIFY2(pControl->accessibleName() == name,
                     qPrintable(qsl("%1 reads \"%2\" and is named \"%3\" to a screen reader rather than \"%4\"")
                                        .arg(describe(pControl), qobject_cast<QAbstractButton*>(pControl)->text(), pControl->accessibleName(), name)));
        }
    }

    void test_theTabChainRunsTheStripBeforeThePatterns()
    {
        chooseTrigger(mpThreePatternRow);

        const QList<QWidget*> chain = tabChainFromTheCommandField();
        const auto at = [&chain](QWidget* pWidget) {
            return chain.indexOf(pWidget);
        };

        const int anySegment = at(mpEditor->mpRadioButton_matchAny);
        const int allSegment = at(mpEditor->mpRadioButton_matchAll);
        const int within = at(mpEditor->mpSpinBox_matchWithinLines);
        const int everyOccurrence = at(form()->checkBox_perlSlashGOption);
        const int keepFiring = at(form()->spinBox_stayOpen);
        const int onlyMatches = at(form()->checkBox_filterTrigger);
        const int playSound = at(form()->checkBox_soundTrigger);
        const int soundFile = at(form()->lineEdit_soundFile);
        const int highlight = at(form()->checkBox_triggerColorizer);
        const int foreground = at(form()->pushButtonFgColor);
        const int background = at(form()->pushButtonBgColor);
        const int firstPattern = at(mpEditor->mTriggerPatternEdit.at(0)->singleLineTextEdit_pattern);

        QStringList reading;
        for (QWidget* pWidget : chain) {
            reading << describe(pWidget);
            if (reading.size() >= 16) {
                break;
            }
        }
        qInfo().noquote() << qsl("  the chain from the command field reads: %1").arg(reading.join(qsl(" -> ")));

        const QList<QPair<QString, int>> ordered{{qsl("the Options button"), at(form()->toolButton_toggleExtraControls)},
                                                 {qsl("Any pattern"), anySegment},
                                                 {qsl("All patterns"), allSegment},
                                                 {qsl("within lines"), within},
                                                 {qsl("Every occurrence"), everyOccurrence},
                                                 {qsl("Keep firing"), keepFiring},
                                                 {qsl("Only pass matches"), onlyMatches},
                                                 {qsl("Sound"), playSound},
                                                 {qsl("sound file"), soundFile},
                                                 {qsl("Highlight"), highlight},
                                                 {qsl("Foreground"), foreground},
                                                 {qsl("Background"), background},
                                                 {qsl("the first pattern"), firstPattern}};
        for (const auto& [name, index] : ordered) {
            QVERIFY2(index >= 0, qPrintable(qsl("%1 is not in the tab chain at all").arg(name)));
        }
        for (int step = 1; step < ordered.size(); ++step) {
            QVERIFY2(ordered.at(step - 1).second < ordered.at(step).second,
                     qPrintable(qsl("%1 is reached at %2 and %3 at %4, so the chain does not read left to right along the strip")
                                        .arg(ordered.at(step - 1).first)
                                        .arg(ordered.at(step - 1).second)
                                        .arg(ordered.at(step).first)
                                        .arg(ordered.at(step).second)));
        }
    }

    void test_aClosedStripTakesItsControlsOutOfTheTabChain()
    {
        chooseTrigger(mpThreePatternRow);
        mpEditor->setTriggerOptionsShown(false);
        settle();
        auto reopenTheStrip = qScopeGuard([this]() {
            mpEditor->setTriggerOptionsShown(true);
            settle();
        });

        const QList<QWidget*> chain = tabChainFromTheCommandField();
        const auto at = [&chain](QWidget* pWidget) {
            return chain.indexOf(pWidget);
        };

        QStringList reading;
        for (QWidget* pWidget : chain) {
            reading << describe(pWidget);
            if (reading.size() >= 8) {
                break;
            }
        }
        qInfo().noquote() << qsl("  with the strip away the chain from the command field reads: %1").arg(reading.join(qsl(" -> ")));

        const int optionsButton = at(form()->toolButton_toggleExtraControls);
        const int firstPattern = at(mpEditor->mTriggerPatternEdit.at(0)->singleLineTextEdit_pattern);
        QVERIFY2(optionsButton >= 0, "the Options button is not in the tab chain, so a closed strip cannot be opened from the keyboard");
        QVERIFY2(firstPattern >= 0, "the first pattern is not in the tab chain");
        QVERIFY2(
                optionsButton < firstPattern,
                qPrintable(qsl("the Options button is reached at %1 and the first pattern at %2, so the chain does not run from the head row into the patterns").arg(optionsButton).arg(firstPattern)));

        const QList<QPair<QString, QWidget*>> away{
                {qsl("Any pattern"), mpEditor->mpRadioButton_matchAny}, {qsl("Every occurrence"), form()->checkBox_perlSlashGOption}, {qsl("Background"), form()->pushButtonBgColor}};
        for (const auto& [name, pControl] : away) {
            QVERIFY2(at(pControl) < 0, qPrintable(qsl("%1 is still in the tab chain at %2 with the strip put away").arg(name).arg(at(pControl))));
        }
    }

    void test_aColourlessWellStillReadsKeep()
    {
        // Chosen first so leaving the trigger cannot save over the colours set below; reloaded in place
        chooseTrigger(mpThreePatternRow);
        TTrigger* pT = mpHost->getTriggerUnit()->getTrigger(mpThreePatternRow->data(0, Qt::UserRole).toInt());
        QVERIFY2(pT != nullptr, "the trigger these cases edit is gone from the profile");
        pT->setColorizerFgColor(QColorConstants::Transparent);
        pT->setColorizerBgColor(QColorConstants::Transparent);
        mpEditor->slot_triggerSelected(mpThreePatternRow);
        settle();
        qInfo().noquote() << qsl("  the wells read \"%1\" and \"%2\", and are described as \"%3\" and \"%4\"")
                                     .arg(form()->pushButtonFgColor->text(),
                                          form()->pushButtonBgColor->text(),
                                          form()->pushButtonFgColor->accessibleDescription(),
                                          form()->pushButtonBgColor->accessibleDescription());
        // The exact words: a well still naming its last colour, or described by its own name, would
        // pass a non-empty check
        const QString keep = QCoreApplication::translate("dlgTriggerEditor", "keep");
        const QString keepsTheGamesColour = QCoreApplication::translate("dlgTriggerEditor", "keeps the game's colour");
        QCOMPARE(form()->pushButtonFgColor->text(), keep);
        QCOMPARE(form()->pushButtonBgColor->text(), keep);
        QCOMPARE(form()->pushButtonFgColor->accessibleDescription(), keepsTheGamesColour);
        QCOMPARE(form()->pushButtonBgColor->accessibleDescription(), keepsTheGamesColour);
    }

    void test_theChosenSegmentIsOutlinedOnEverySide()
    {
        chooseTrigger(mpThreePatternRow);
        QRadioButton* pAny = mpEditor->mpRadioButton_matchAny;
        QRadioButton* pAll = mpEditor->mpRadioButton_matchAll;
        QWidget* pPair = mpEditor->mpWidget_matchModeRows;
        const QColor accent = uiDesign::themeTokens().accent;

        pAny->setChecked(true);
        settle();
        remeasure({pAny, pAll});
        const int anyWidth = pAny->width();
        const int allWidth = pAll->width();
        const int pairWidth = pPair->width();
        QVERIFY2(pAny->geometry().right() + 1 == pAll->geometry().left(), "the two segments no longer meet, so there is no shared hairline to measure");

        QImage shot = pPair->grab().toImage();
        const QColor anysSeam = colourInsideTheEdge(shot, pAny, true, 0);
        const QColor anysOuterEdge = colourInsideTheEdge(shot, pAny, false, 0);
        const QColor allsEdgeUnderAny = colourInsideTheEdge(shot, pAll, false, 0);
        const QColor allsFillUnderAny = colourInsideTheEdge(shot, pAll, false, scmFillDepth);

        pAll->setChecked(true);
        settle();
        remeasure({pAny, pAll});
        shot = pPair->grab().toImage();
        const QColor allsSeam = colourInsideTheEdge(shot, pAll, false, 0);
        const QColor allsOuterEdge = colourInsideTheEdge(shot, pAll, true, 0);
        const QColor anysEdgeUnderAll = colourInsideTheEdge(shot, pAny, true, 0);
        const QColor anysFillUnderAll = colourInsideTheEdge(shot, pAny, true, scmFillDepth);

        qInfo().noquote() << qsl("  the accent is %1; Any chosen reads %2 at the seam and %3 outside, with All at %4 against a fill of %5; All chosen reads %6 at the seam and %7 outside, with Any at "
                                 "%8 against a fill of %9")
                                     .arg(accent.name(),
                                          anysSeam.name(),
                                          anysOuterEdge.name(),
                                          allsEdgeUnderAny.name(),
                                          allsFillUnderAny.name(),
                                          allsSeam.name(),
                                          allsOuterEdge.name(),
                                          anysEdgeUnderAll.name(),
                                          anysFillUnderAll.name());

        QVERIFY2(TestPixels::distance(anysSeam, accent) <= scmInkSlack,
                 qPrintable(qsl("with Any chosen its edge at the seam reads %1 against an accent of %2, so its outline is open there").arg(anysSeam.name(), accent.name())));
        QVERIFY2(TestPixels::distance(anysOuterEdge, accent) <= scmInkSlack,
                 qPrintable(qsl("with Any chosen its outer edge reads %1 rather than the accent, so the two sides are not one outline").arg(anysOuterEdge.name())));
        QVERIFY2(TestPixels::distance(allsEdgeUnderAny, allsFillUnderAny) <= scmInkSlack,
                 qPrintable(qsl("with Any chosen, All reads %1 at the seam against its own fill of %2 - it is still drawing a hairline of its own, so the seam is two lines")
                                    .arg(allsEdgeUnderAny.name(), allsFillUnderAny.name())));

        QVERIFY2(TestPixels::distance(allsSeam, accent) <= scmInkSlack,
                 qPrintable(qsl("with All chosen its edge at the seam reads %1 against an accent of %2, so its outline is open there").arg(allsSeam.name(), accent.name())));
        QVERIFY2(TestPixels::distance(allsOuterEdge, accent) <= scmInkSlack,
                 qPrintable(qsl("with All chosen its outer edge reads %1 rather than the accent, so the two sides are not one outline").arg(allsOuterEdge.name())));
        QVERIFY2(TestPixels::distance(anysEdgeUnderAll, anysFillUnderAll) <= scmInkSlack,
                 qPrintable(qsl("with All chosen, Any reads %1 at the seam against its own fill of %2 - it kept a hairline there, so the seam is two lines")
                                    .arg(anysEdgeUnderAll.name(), anysFillUnderAll.name())));

        // What one segment gives up in border the other takes back in padding
        qInfo().noquote() << qsl("  the segments are %1px and %2px in a %3px pair with Any chosen, and %4px and %5px in a %6px pair with All chosen")
                                     .arg(anyWidth)
                                     .arg(allWidth)
                                     .arg(pairWidth)
                                     .arg(pAny->width())
                                     .arg(pAll->width())
                                     .arg(pPair->width());
        QCOMPARE(pAny->width(), anyWidth);
        QCOMPARE(pAll->width(), allWidth);
        QCOMPARE(pPair->width(), pairWidth);
    }

    // Guards against the platform's marks: a grey Fusion square on dark, a blue rounded macOS box on light
    void test_theStripsCheckBoxCarriesTheOneMark()
    {
        const auto appearanceBefore = mudlet::self()->mAppearance;
        auto restore = qScopeGuard([this, appearanceBefore]() {
            TestPixels::setAppearanceAndWait(appearanceBefore, mpEditor);
        });

        chooseTrigger(mpThreePatternRow);
        QCheckBox* pBox = form()->checkBox_perlSlashGOption;
        QVERIFY2(pBox->isVisible(), "the check box this case measures is not on show");
        const bool wasChecked = pBox->isChecked();
        auto putTheBoxBack = qScopeGuard([pBox, wasChecked]() {
            pBox->setChecked(wasChecked);
        });

        QStringList misses;
        for (const auto& appearance : QList<QPair<QString, enums::Appearance>>{{qsl("dark"), enums::Appearance::dark}, {qsl("light"), enums::Appearance::light}}) {
            TestPixels::setAppearanceAndWait(appearance.second, mpEditor);
            // Otherwise a run that never switched would measure one theme twice and pass
            QCOMPARE(uiDesign::themeTokens().darkPage, appearance.second == enums::Appearance::dark);

            pBox->setChecked(false);
            settle();
            const QImage empty = mpEditor->grab().toImage();
            const QRect box = markBoxIn(pBox, mpEditor, empty);
            QVERIFY2(box.width() > 4 && box.height() > 4, qPrintable(qsl("%1: the mark has no rectangle to sample: %2x%3").arg(appearance.first).arg(box.width()).arg(box.height())));

            // Past the mark's antialiased edge and short of its words, in the gap choiceStyleSheet() leaves
            const QColor behind = empty.pixelColor(qMin(box.right() + box.width() / 3, empty.width() - 1), box.center().y());
            const qreal outline = outlineRatioOf(empty, box, behind);
            const QColor fill = empty.pixelColor(box.center());
            const int nothingInIt = markedPixelsIn(empty, box, fill);

            pBox->setChecked(true);
            settle();
            const int aTick = markedPixelsIn(mpEditor->grab().toImage(), box, fill);

            qInfo().noquote() << qsl("  %1: the strip's mark is %2px on a fill of %3, outlined at %4:1 against %5, with %6 pixels in it empty and %7 checked")
                                         .arg(appearance.first)
                                         .arg(box.width())
                                         .arg(fill.name(), QString::number(outline, 'f', 2), behind.name())
                                         .arg(nothingInIt)
                                         .arg(aTick);
            if (outline < uiDesign::scmQuietMinimumRatio) {
                misses << qsl("%1: the mark is outlined at %2:1 against the form behind it").arg(appearance.first, QString::number(outline, 'f', 2));
            }
            if (nothingInIt > 0) {
                misses << qsl("%1: an unchecked box already has %2 pixels in it that are not its fill").arg(appearance.first).arg(nothingInIt);
            }
            if (aTick <= 0) {
                misses << qsl("%1: a checked box paints nothing inside itself, so there is no tick").arg(appearance.first);
            }
        }
        QVERIFY2(misses.isEmpty(), qPrintable(qsl("\n  %1").arg(misses.join(qsl("\n  ")))));
    }

    void test_theStepperChevronTakesTheAccentUnderThePointer()
    {
        chooseTrigger(mpThreePatternRow);
        QSpinBox* pBox = form()->spinBox_stayOpen;
        QVERIFY2(pBox->isVisible(), "the spin box these steppers belong to is not on show");

        QStyleOptionSpinBox option;
        option.initFrom(pBox);
        option.subControls = QStyle::SC_All;
        const QRect upButton = pBox->style()->subControlRect(QStyle::CC_SpinBox, &option, QStyle::SC_SpinBoxUp, pBox);
        QVERIFY2(!upButton.isEmpty(), "the spin box draws no stepper at all, so there is no chevron to light up");
        const QColor accent = uiDesign::themeTokens().accent;

        // Off the steppers, where the pointer goes to put the chevron back
        const QPoint field(2, pBox->height() / 2);
        QTest::mouseMove(pBox, field);
        settle();
        const int atRest = nearestToTheAccentIn(pBox->grab().toImage(), upButton, accent);

        QTest::mouseMove(pBox, upButton.center());
        settle();
        const int hovered = nearestToTheAccentIn(pBox->grab().toImage(), upButton, accent);

        QTest::mouseMove(pBox, field);
        settle();
        const int afterwards = nearestToTheAccentIn(pBox->grab().toImage(), upButton, accent);

        qInfo().noquote() << qsl("  the up stepper is %1 and its nearest pixel to the accent %2 is %3 away at rest, %4 under the pointer and %5 after it left")
                                     .arg(qsl("%1x%2+%3+%4").arg(upButton.width()).arg(upButton.height()).arg(upButton.x()).arg(upButton.y()), accent.name())
                                     .arg(atRest)
                                     .arg(hovered)
                                     .arg(afterwards);

        QVERIFY2(atRest > scmChevronInkSlack, qPrintable(qsl("the resting stepper already has a pixel %1 from the accent, so nothing here says the pointer did anything").arg(atRest)));
        QVERIFY2(hovered <= scmChevronInkSlack, qPrintable(qsl("the stepper under the pointer comes no nearer the accent than %1, so its chevron did not light up").arg(hovered)));
        QVERIFY2(atRest - hovered >= scmChevronInkShift,
                 qPrintable(qsl("the pointer took the stepper from %1 to %2 of the accent, which is inside what the wash behind the chevron moves on its own").arg(atRest).arg(hovered)));
        QVERIFY2(afterwards > scmChevronInkSlack, qPrintable(qsl("the stepper is still %1 from the accent with the pointer off it, so the chevron never went back").arg(afterwards)));
    }

    // QAbstractButton reads QStyle::SH_Button_FocusPolicy once, in its constructor, so a style
    // answering Qt::StrongFocus leaves a clicked mark holding the focus accent. The box is put back
    // to StrongFocus and the form restyled over it, so the run measures the shell, not the platform.
    void test_clickingAnOptionDoesNotLeaveItHoldingTheFocus()
    {
        const auto appearanceBefore = mudlet::self()->mAppearance;
        auto restore = qScopeGuard([this, appearanceBefore]() {
            TestPixels::setAppearanceAndWait(appearanceBefore, mpEditor);
        });

        chooseTrigger(mpThreePatternRow);
        QCheckBox* pBox = form()->checkBox_perlSlashGOption;
        QVERIFY2(pBox->isVisible(), "the check box this case clicks is not on show");

        // A different appearance: applyEditorShellStyle() redoes nothing while the four colours it
        // mixes from stay put
        pBox->setFocusPolicy(Qt::StrongFocus);
        TestPixels::setAppearanceAndWait(uiDesign::themeTokens().darkPage ? enums::Appearance::light : enums::Appearance::dark, mpEditor);
        QCOMPARE(pBox->focusPolicy(), Qt::TabFocus);

        const bool wasChecked = pBox->isChecked();
        auto putTheBoxBack = qScopeGuard([pBox, wasChecked]() {
            pBox->setChecked(wasChecked);
            // The box is focused below, and no other case expects the focus there
            pBox->clearFocus();
        });

        QTest::mouseClick(pBox, Qt::LeftButton, Qt::NoModifier, pBox->rect().center());
        settle();
        QVERIFY2(pBox->isChecked() != wasChecked, "clicking the option left it where it was, so nothing here says the click landed");
        // focusWidget(), not hasFocus(): hasFocus() is false whenever the window never became active,
        // and would pass whatever the policy said
        QVERIFY2(mpEditor->focusWidget() != pBox, "a click left the option holding the keyboard focus, so the accent stayed on its mark");
        QVERIFY2(!pBox->hasFocus(), "a click left the option focused");

        pBox->setFocus(Qt::TabFocusReason);
        settle();
        QVERIFY2(mpEditor->focusWidget() == pBox, "the Tab key cannot reach the option, so a keyboard user has nothing to see the accent on");
    }
};

#include "EditorTriggerOptionsStripTest.moc"
MUDLET_GROUPED_TEST_MAIN(EditorTriggerOptionsStripTest)
