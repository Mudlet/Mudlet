/***************************************************************************
 *   Copyright (C) 2026 by Mudlet Makers                                   *
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

#include <QAccessible>
#include <QAction>
#include <QClipboard>
#include <QFileInfo>
#include <QTemporaryDir>
#include <QtTest/QtTest>
#include <algorithm>
#include <chrono>

#include "MudletPaths.h"
#include "PortableModeTestHelper.h"
#include "ProfileTestHelper.h"
#include "Host.h"
#include "MudletInstanceCoordinator.h"
#include "TAccessibleTextEdit.h"
#include "TBuffer.h"
#include "TLuaInterpreter.h"
#include "TMainConsole.h"
#include "TTextEdit.h"
#include "TelnetServerStub.h"
#include "ctelnet.h"
#include "dlgConnectionProfiles.h"
#include "mudlet.h"

#include "GroupedTest.h"

using namespace std::chrono_literals;

// Regression test for #3922: left-clicking into the main console to give it
// focus must not leave a one-character selection behind. Such a stray
// selection hijacks Ctrl+C away from the command line (TCommandLine prioritises
// any console selection over the input box).
class MainConsoleSelectionTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    TelnetServerStub* mpServer = nullptr;
    const QString mpHostname = "Test-Selection";
    QString mpPort; // assigned the stub's actual ephemeral port in init()
    const QString mpLocalhost = "localhost";

    // Send a screenful of text so a click in the middle of the upper pane lands
    // on a real, filled line rather than empty space.
    QString fillerText() const
    {
        const QString line = QString(100, QLatin1Char('X'));
        QString message;
        for (int i = 0; i < 80; ++i) {
            message.append(line);
            message.append(QStringLiteral("\r\n"));
        }
        return message;
    }

    TTextEdit* upperPane() const
    {
        auto host = mudlet::self()->getActiveHost();
        if (!host || !host->mpConsole) {
            return nullptr;
        }
        return host->mpConsole->mUpperPane;
    }

    void sendMouse(QWidget* w, QEvent::Type type, Qt::MouseButton button, Qt::MouseButtons buttons, const QPointF& localPos, Qt::KeyboardModifiers modifiers = Qt::NoModifier)
    {
        const QPointF globalPos = w->mapToGlobal(localPos.toPoint());
        QMouseEvent event(type, localPos, globalPos, button, buttons, modifiers);
        QApplication::sendEvent(w, &event);
    }

    // Prose rather than fillerText(), so a copy that stops at the pointer's
    // column or at a word boundary shows up in the text, and numbered so a copy
    // of the wrong line does too. Two digits throughout, so the columns the
    // tests name hold on every line.
    static constexpr int scmProseFirstNumber = 10;
    static constexpr int scmProseLastNumber = 89;
    QString proseLine(int number) const { return qsl("%1 way off in the distance you faintly hear a grandfather clock strike eleven").arg(number); }

    TTextEdit* paneShowingProse()
    {
        QString message;
        for (int number = scmProseFirstNumber; number <= scmProseLastNumber; ++number) {
            message.append(proseLine(number));
            message.append(qsl("\r\n"));
        }
        mpServer->setWelcomeMessage(message);
        startProfile(mpHostname, mpLocalhost, mpPort);
        // startProfile() can only fail the test, not stop it, from in here
        if (QTest::currentTestFailed() || !waitForTextInBuffer(proseLine(scmProseLastNumber))) {
            return nullptr;
        }
        mudlet::self()->resize(1200, 800);
        QTest::qWait(100ms);
        return upperPane();
    }

    QPointF cellInMiddleRow(TTextEdit* pane, int column) const
    {
        const int row = (pane->height() / 2) / pane->mFontHeight;
        return QPointF(column * pane->mFontWidth + pane->mFontWidth / 2.0, row * pane->mFontHeight + pane->mFontHeight / 2.0);
    }

    QString lineUnder(TTextEdit* pane, const QPointF& pos) const
    {
        return mudlet::self()->getActiveHost()->mpConsole->buffer.line(static_cast<int>(pos.y()) / pane->mFontHeight + pane->imageTopLine());
    }

    // Leaves the button held on the last press, which is where a word (2) or
    // line (3) selection can still be dragged.
    void pressLeftButton(TTextEdit* pane, const QPointF& pos, int count)
    {
        for (int i = 1; i < count; ++i) {
            sendMouse(pane, QEvent::MouseButtonPress, Qt::LeftButton, Qt::LeftButton, pos);
            sendMouse(pane, QEvent::MouseButtonRelease, Qt::LeftButton, Qt::NoButton, pos);
        }
        sendMouse(pane, QEvent::MouseButtonPress, Qt::LeftButton, Qt::LeftButton, pos);
    }

    void moveAndReleaseLeftButton(TTextEdit* pane, const QPointF& pos, Qt::KeyboardModifiers modifiers = Qt::NoModifier)
    {
        sendMouse(pane, QEvent::MouseMove, Qt::NoButton, Qt::LeftButton, pos, modifiers);
        sendMouse(pane, QEvent::MouseButtonRelease, Qt::LeftButton, Qt::NoButton, pos, modifiers);
    }

    // What is on screen as highlighted, read from the per-character flags so it
    // does not depend on the selection endpoints that a copy works from.
    QString highlightedText() const
    {
        TBuffer& buffer = mudlet::self()->getActiveHost()->mpConsole->buffer;
        QStringList lines;
        for (int y = 0; y <= buffer.getLastLineNumber(); ++y) {
            QString selected;
            const QString text = buffer.line(y);
            for (int x = 0; x < static_cast<int>(text.size()); ++x) {
                if (buffer.buffer.at(y).at(x).isSelected()) {
                    selected.append(text.at(x));
                }
            }
            if (!selected.isEmpty()) {
                lines.append(selected);
            }
        }
        return lines.join(QChar::LineFeed);
    }

    // Each copy starts from a marker, so a slot that returns early cannot pass
    // on what an earlier copy left on the clipboard.
    const QString mNothingCopied = qsl("nothing was copied");

    QString copiedText(TTextEdit* pane) const
    {
        QApplication::clipboard()->setText(mNothingCopied);
        pane->slot_copySelectionToClipboard();
        return QApplication::clipboard()->text();
    }

    QString copiedHtml(TTextEdit* pane) const
    {
        QApplication::clipboard()->setText(mNothingCopied);
        pane->slot_copySelectionToClipboardHTML();
        return QApplication::clipboard()->text();
    }

private slots:
    void initTestCase()
    {
        if (portableMarkerPresent()) {
            QSKIP("portable.txt present - it takes precedence over XDG_CONFIG_HOME, so the config dir cannot be redirected");
        }

        // A config root of this process's own. Sharing the developer's
        // ~/.config/mudlet means sharing a profile list, so a second copy of
        // this test running at the same time is told the name it types is
        // already in use and never gets an enabled Connect button. Since #9712
        // the opt-in that makes setupConfig() adopt a directory is
        // $XDG_CONFIG_HOME/mudlet/profiles, not the mudlet directory alone.
        QVERIFY(mConfigDir.isValid());
        QVERIFY(QDir().mkpath(qsl("%1/mudlet/profiles").arg(mConfigDir.path())));
        mSavedXdg = qgetenv("XDG_CONFIG_HOME");
        qputenv("XDG_CONFIG_HOME", mConfigDir.path().toUtf8());
    }

    void cleanupTestCase() { mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg); }

    void init()
    {
        mpServer = new TelnetServerStub(qApp);
        mpServer->start(mpLocalhost, 0); // ephemeral OS-assigned port avoids collisions across concurrent test runs
        mpPort = QString::number(mpServer->serverPort());
        mudlet::start();
        mudlet::self()->setupConfig();
        QCOMPARE(MudletPaths::getMudletPath(enums::mainPath), qsl("%1/mudlet").arg(mConfigDir.path()));
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);
        deleteProfileDirectory(mpHostname);
    }

    // A plain click (press + a move that stays in the same character cell +
    // release) must not create a selection.
    void test_clickWithoutDragLeavesNoSelection()
    {
        mpServer->setWelcomeMessage(fillerText());
        startProfile(mpHostname, mpLocalhost, mpPort);
        QVERIFY2(waitForTextInBuffer(QString(100, QLatin1Char('X'))), "Filler text never reached the buffer");

        mudlet::self()->resize(1200, 800);
        QTest::qWait(100ms);

        TTextEdit* pane = upperPane();
        QVERIFY2(pane, "No upper pane available");
        QVERIFY2(pane->width() > 400 && pane->height() > 100, qPrintable(QStringLiteral("Upper pane too small: %1x%2").arg(pane->width()).arg(pane->height())));

        pane->unHighlight();
        pane->mSelectedRegion = QRegion();
        QVERIFY(pane->mSelectedRegion.isEmpty());

        // Click in the middle of the pane (well past the timestamp gutter), then
        // a move event at the very same pixel - i.e. no movement to a different
        // character cell - then release.
        const QPointF clickPos = QRectF(pane->rect()).center();
        sendMouse(pane, QEvent::MouseButtonPress, Qt::LeftButton, Qt::LeftButton, clickPos);
        sendMouse(pane, QEvent::MouseMove, Qt::NoButton, Qt::LeftButton, clickPos);
        sendMouse(pane, QEvent::MouseButtonRelease, Qt::LeftButton, Qt::NoButton, clickPos);

        QVERIFY2(pane->mSelectedRegion.isEmpty(), "A click with no drag left a stray selection in the console (regression of #3922)");
    }

    // Control case: a genuine drag across cells must still produce a selection,
    // so the fix above does not over-correct.
    void test_dragStillSelects()
    {
        mpServer->setWelcomeMessage(fillerText());
        startProfile(mpHostname, mpLocalhost, mpPort);
        QVERIFY2(waitForTextInBuffer(QString(100, QLatin1Char('X'))), "Filler text never reached the buffer");

        mudlet::self()->resize(1200, 800);
        QTest::qWait(100ms);

        TTextEdit* pane = upperPane();
        QVERIFY2(pane, "No upper pane available");

        pane->unHighlight();
        pane->mSelectedRegion = QRegion();

        const QPointF startPos = QRectF(pane->rect()).center();
        const QPointF endPos = startPos + QPointF(60, 0); // several character cells to the right
        sendMouse(pane, QEvent::MouseButtonPress, Qt::LeftButton, Qt::LeftButton, startPos);
        sendMouse(pane, QEvent::MouseMove, Qt::NoButton, Qt::LeftButton, endPos);
        sendMouse(pane, QEvent::MouseButtonRelease, Qt::LeftButton, Qt::NoButton, endPos);

        QVERIFY2(!pane->mSelectedRegion.isEmpty(), "A real drag failed to create a selection");
    }

    // Regression case for the review fix: once a drag has genuinely selected
    // text, dragging back to the original press cell must collapse the extent
    // instead of leaving the previous selection frozen.
    void test_dragBackToOriginCollapsesSelectionExtent()
    {
        mpServer->setWelcomeMessage(fillerText());
        startProfile(mpHostname, mpLocalhost, mpPort);
        QVERIFY2(waitForTextInBuffer(QString(100, QLatin1Char('X'))), "Filler text never reached the buffer");

        mudlet::self()->resize(1200, 800);
        QTest::qWait(100ms);

        TTextEdit* pane = upperPane();
        QVERIFY2(pane, "No upper pane available");

        pane->unHighlight();
        pane->mSelectedRegion = QRegion();

        const QPointF startPos = QRectF(pane->rect()).center();
        const QPointF endPos = startPos + QPointF(60, 0); // several character cells to the right
        sendMouse(pane, QEvent::MouseButtonPress, Qt::LeftButton, Qt::LeftButton, startPos);
        sendMouse(pane, QEvent::MouseMove, Qt::NoButton, Qt::LeftButton, endPos);

        QVERIFY2(!pane->mSelectedRegion.isEmpty(), "The initial drag failed to create a selection");
        const QRect expandedSelection = pane->mSelectedRegion.boundingRect();

        sendMouse(pane, QEvent::MouseMove, Qt::NoButton, Qt::LeftButton, startPos);
        sendMouse(pane, QEvent::MouseButtonRelease, Qt::LeftButton, Qt::NoButton, startPos);

        const QRect collapsedSelection = pane->mSelectedRegion.boundingRect();
        QVERIFY2(collapsedSelection.width() < expandedSelection.width(), "Dragging back to the press cell left the earlier selection extent frozen");
    }

    // #10606: any pointer move while the last press of a triple-click is held,
    // even within one cell, must not narrow a copy below the highlighted line.
    void test_tripleClickThatMovesThePointerCopiesTheWholeLine()
    {
        TTextEdit* pane = paneShowingProse();
        QVERIFY2(pane, "the prose never reached the upper pane");

        const QPointF pressPos = cellInMiddleRow(pane, 33);
        const QString line = lineUnder(pane, pressPos);
        QVERIFY2(line.endsWith(qsl("eleven")), qPrintable(qsl("the press is not over a line of prose but over \"%1\"").arg(line)));

        pressLeftButton(pane, pressPos, 3);
        QVERIFY2(pane->mMouseTrackLevel == 3, "the three presses were too far apart to count as a triple-click");
        moveAndReleaseLeftButton(pane, pressPos + QPointF(1, 0));

        QCOMPARE(highlightedText(), line);
        QCOMPARE(copiedText(pane), line);

        const QString html = copiedHtml(pane);
        QVERIFY2(html.startsWith(qsl("<!DOCTYPE")), "nothing was copied as HTML");
        QVERIFY2(html.contains(line), "the copy as HTML lost part of the line");

        // the image copy re-highlights from the endpoints it copied with, so a
        // narrowed copy shows as a narrowed highlight
        pane->slot_copySelectionToClipboardImage();
        QCOMPARE(highlightedText(), line);

        // slot_analyseSelection() reports through this action's tooltip and
        // does nothing without one
        pane->mpContextMenuAnalyser = new QAction(pane);
        pane->slot_analyseSelection();
        const QString lastCharacterHeading = qsl("<center>%1</center></th>").arg(line.size());
        QVERIFY2(pane->mpContextMenuAnalyser->toolTip().contains(lastCharacterHeading), "the character analysis lost the end of the line");
    }

    void test_ctrlClickThatMovesThePointerCopiesTheWholeLine()
    {
        TTextEdit* pane = paneShowingProse();
        QVERIFY2(pane, "the prose never reached the upper pane");

        const QPointF pressPos = cellInMiddleRow(pane, 33);
        const QString line = lineUnder(pane, pressPos);
        QVERIFY2(line.endsWith(qsl("eleven")), qPrintable(qsl("the press is not over a line of prose but over \"%1\"").arg(line)));

        sendMouse(pane, QEvent::MouseButtonPress, Qt::LeftButton, Qt::LeftButton, pressPos, Qt::ControlModifier);
        moveAndReleaseLeftButton(pane, pressPos + QPointF(2, 0), Qt::ControlModifier);

        QCOMPARE(highlightedText(), line);
        QCOMPARE(copiedText(pane), line);
    }

    // Upwards as well as downwards: above the anchor line the pointer is the
    // top-left end of the selection rather than the bottom-right.
    void test_draggingALineSelectionCopiesWholeLines_data()
    {
        QTest::addColumn<int>("rowsDragged");
        QTest::newRow("upwards") << -2;
        QTest::newRow("downwards") << 2;
    }

    void test_draggingALineSelectionCopiesWholeLines()
    {
        QFETCH(int, rowsDragged);

        TTextEdit* pane = paneShowingProse();
        QVERIFY2(pane, "the prose never reached the upper pane");

        const QPointF pressPos = cellInMiddleRow(pane, 33);
        const QPointF releasePos = pressPos + QPointF(0, rowsDragged * pane->mFontHeight);
        QStringList lines;
        for (int row = std::min(0, rowsDragged); row <= std::max(0, rowsDragged); ++row) {
            lines.append(lineUnder(pane, pressPos + QPointF(0, row * pane->mFontHeight)));
            QVERIFY2(lines.constLast().endsWith(qsl("eleven")), qPrintable(qsl("the drag crosses \"%1\", which is not a line of prose").arg(lines.constLast())));
        }
        const QString expected = lines.join(QChar::LineFeed);

        pressLeftButton(pane, pressPos, 3);
        QVERIFY2(pane->mMouseTrackLevel == 3, "the three presses were too far apart to count as a triple-click");
        moveAndReleaseLeftButton(pane, releasePos);

        QCOMPARE(highlightedText(), expected);
        QCOMPARE(copiedText(pane), expected);

        const QString html = copiedHtml(pane);
        for (const QString& line : lines) {
            QVERIFY2(html.contains(line), qPrintable(qsl("the copy as HTML lost part of \"%1\"").arg(line)));
        }
    }

    // Dragged leftwards, the pointer's word is only the first of those
    // highlighted.
    void test_doubleClickDraggedLeftCopiesEveryHighlightedWord()
    {
        TTextEdit* pane = paneShowingProse();
        QVERIFY2(pane, "the prose never reached the upper pane");

        // columns 31-37 are "faintly" and 18-25 are "distance"
        const QPointF pressPos = cellInMiddleRow(pane, 33);
        const QString line = lineUnder(pane, pressPos);
        QCOMPARE(line.mid(18, 20), qsl("distance you faintly"));

        pressLeftButton(pane, pressPos, 2);
        QVERIFY2(pane->mMouseTrackLevel == 2, "the two presses were too far apart to count as a double-click");
        QCOMPARE(highlightedText(), qsl("faintly"));
        QCOMPARE(copiedText(pane), qsl("faintly"));
        // one move: over several, expandSelectionToWords() walks the anchor
        // left with the pointer and "faintly" drops out of the highlight itself
        moveAndReleaseLeftButton(pane, cellInMiddleRow(pane, 20));

        QCOMPARE(highlightedText(), qsl("distance you faintly"));
        QCOMPARE(copiedText(pane), qsl("distance you faintly"));
    }

    // A double-click on a space has no word to highlight, and leaves the
    // selection's start one cell past its end.
    void test_doubleClickOnASpaceCopiesNothing()
    {
        TTextEdit* pane = paneShowingProse();
        QVERIFY2(pane, "the prose never reached the upper pane");

        const QPointF pressPos = cellInMiddleRow(pane, 30);
        QCOMPARE(lineUnder(pane, pressPos).at(30), QChar(QChar::Space));

        pressLeftButton(pane, pressPos, 2);
        QVERIFY2(pane->mMouseTrackLevel == 2, "the two presses were too far apart to count as a double-click");
        sendMouse(pane, QEvent::MouseButtonRelease, Qt::LeftButton, Qt::NoButton, pressPos);

        QCOMPARE(highlightedText(), QString());
        QCOMPARE(copiedText(pane), mNothingCopied);
        QCOMPARE(copiedHtml(pane), mNothingCopied);
    }

    // Below the last line there is no line for a Ctrl+click to select, so it
    // must not highlight from wherever the selection's endpoints were left.
    void test_ctrlClickBelowTheTextSelectsNothing()
    {
        TTextEdit* pane = paneShowingProse();
        QVERIFY2(pane, "the prose never reached the upper pane");
        auto host = mudlet::self()->getActiveHost();
        QVERIFY2(host->getLuaInterpreter()->compileAndExecuteScript(qsl("clearWindow()\necho('one two\\nthree four\\nfive six\\n')\n")), "the echo() call failed");
        QTest::qWait(100ms);

        const QPointF secondRow(pane->mFontWidth * 2.5, pane->mFontHeight * 1.5);
        QCOMPARE(lineUnder(pane, secondRow), qsl("three four"));
        pressLeftButton(pane, secondRow, 3);
        sendMouse(pane, QEvent::MouseButtonRelease, Qt::LeftButton, Qt::NoButton, secondRow);
        QCOMPARE(highlightedText(), qsl("three four"));

        // long enough after the triple-click not to count as part of it
        QTest::qWait(350ms);
        const QPointF firstRow(pane->mFontWidth * 2.5, pane->mFontHeight * 0.5);
        pressLeftButton(pane, firstRow, 1);
        sendMouse(pane, QEvent::MouseButtonRelease, Qt::LeftButton, Qt::NoButton, firstRow);
        QCOMPARE(highlightedText(), QString());

        const QPointF belowTheText = cellInMiddleRow(pane, 2);
        const int lineBelowTheText = static_cast<int>(belowTheText.y()) / pane->mFontHeight + pane->imageTopLine();
        QVERIFY2(lineBelowTheText >= static_cast<int>(host->mpConsole->buffer.lineBuffer.size()), "the middle of the pane is not below the text");
        sendMouse(pane, QEvent::MouseButtonPress, Qt::LeftButton, Qt::LeftButton, belowTheText, Qt::ControlModifier);
        sendMouse(pane, QEvent::MouseButtonRelease, Qt::LeftButton, Qt::NoButton, belowTheText, Qt::ControlModifier);

        QCOMPARE(highlightedText(), QString());
        QCOMPARE(copiedText(pane), mNothingCopied);
    }

    // A plain drag has to order its two ends itself whichever way it runs, as
    // the copy does not reorder them.
    void test_plainDragCopiesWhatIsHighlighted_data()
    {
        QTest::addColumn<int>("pressColumn");
        QTest::addColumn<int>("releaseColumn");
        QTest::addColumn<int>("rowsDragged");
        QTest::newRow("left to right") << 18 << 25 << 0;
        QTest::newRow("right to left") << 37 << 31 << 0;
        QTest::newRow("top to bottom") << 31 << 25 << 1;
        QTest::newRow("bottom to top") << 25 << 31 << -1;
    }

    void test_plainDragCopiesWhatIsHighlighted()
    {
        QFETCH(int, pressColumn);
        QFETCH(int, releaseColumn);
        QFETCH(int, rowsDragged);

        TTextEdit* pane = paneShowingProse();
        QVERIFY2(pane, "the prose never reached the upper pane");

        const QPointF pressPos = cellInMiddleRow(pane, pressColumn);
        const QPointF releasePos = cellInMiddleRow(pane, releaseColumn) + QPointF(0, rowsDragged * pane->mFontHeight);
        const bool forwards = rowsDragged > 0 || (rowsDragged == 0 && pressColumn < releaseColumn);
        const QString firstLine = lineUnder(pane, forwards ? pressPos : releasePos);
        const QString lastLine = lineUnder(pane, forwards ? releasePos : pressPos);
        const int firstColumn = forwards ? pressColumn : releaseColumn;
        const int lastColumn = forwards ? releaseColumn : pressColumn;
        QVERIFY2(firstLine.endsWith(qsl("eleven")) && lastLine.endsWith(qsl("eleven")), "the drag is not over lines of prose");
        const QString expected = rowsDragged == 0 ? firstLine.mid(firstColumn, lastColumn - firstColumn + 1) : firstLine.mid(firstColumn) + QChar::LineFeed + lastLine.left(lastColumn + 1);

        pressLeftButton(pane, pressPos, 1);
        moveAndReleaseLeftButton(pane, releasePos);

        QCOMPARE(highlightedText(), expected);
        QCOMPARE(copiedText(pane), expected);
    }

    // A double-click leaves mMouseTrackLevel at 2, which must not widen a
    // selection a screen reader makes afterwards.
    void test_screenReaderSelectionAfterADoubleClickIsNotWidenedToWords()
    {
        QAccessible::installFactory(TAccessibleTextEdit::textEditFactory);
        TTextEdit* pane = paneShowingProse();
        QVERIFY2(pane, "the prose never reached the upper pane");

        const QPointF pressPos = cellInMiddleRow(pane, 33);
        pressLeftButton(pane, pressPos, 2);
        sendMouse(pane, QEvent::MouseButtonRelease, Qt::LeftButton, Qt::NoButton, pressPos);
        QCOMPARE(highlightedText(), qsl("faintly"));
        QCOMPARE(copiedText(pane), qsl("faintly"));

        QAccessibleInterface* pInterface = QAccessible::queryAccessibleInterface(pane);
        QVERIFY2(pInterface && pInterface->textInterface(), "no QAccessibleTextInterface for the console");

        // columns 20-33 start inside "distance" and end inside "faintly"
        const int y = static_cast<int>(pressPos.y()) / pane->mFontHeight + pane->imageTopLine();
        pInterface->textInterface()->removeSelection(0);
        pInterface->textInterface()->addSelection(pane->offsetForPosition(y, 20), pane->offsetForPosition(y, 34));

        QCOMPARE(highlightedText(), qsl("stance you fai"));
        QCOMPARE(copiedText(pane), qsl("stance you fai"));
    }

    // The mouse selection is a flag on each TChar, so whatever the line's
    // characters are held in has to keep them where they are once a selection
    // has been made over them. Text arriving on the line that is already under
    // selection - a prompt, an echo - is how that gets tested in practice.
    void test_appendingToASelectedLineKeepsItHighlighted()
    {
        mpServer->setWelcomeMessage(fillerText());
        startProfile(mpHostname, mpLocalhost, mpPort);
        QVERIFY2(waitForTextInBuffer(QString(100, QLatin1Char('X'))), "Filler text never reached the buffer");

        mudlet::self()->resize(1200, 800);
        QTest::qWait(100ms);

        TTextEdit* pane = upperPane();
        QVERIFY2(pane, "No upper pane available");

        TMainConsole* console = mudlet::self()->getActiveHost()->mpConsole;
        console->print(qsl("\nselected"));
        const int y = console->buffer.getLastLineNumber();
        QCOMPARE(console->buffer.line(y), qsl("selected"));

        pane->slot_selectAll();
        QVERIFY2(console->buffer.buffer.at(y).at(0).isSelected(), "selecting all did not mark the last line, so a dropped flag below could not be told from one that was never set");

        // enough for the line to outgrow whatever it is held in, but short
        // enough that it cannot wrap and move to a line of its own
        console->print(QString(20, QLatin1Char('z')));
        QCOMPARE(console->buffer.getLastLineNumber(), y);

        QVERIFY2(console->buffer.buffer.at(y).at(0).isSelected(), "text arriving on a selected line deselected the characters that were already on it");
    }

    // #6363: the mouse cursor becomes a hand over a link, and the reset back to
    // the I-beam lives inside two bounds checks in updateTextCursor(). Leaving
    // the link sideways lands on a character that answers those checks, so the
    // reset runs; leaving it downwards lands past the last line of the buffer,
    // where neither check is satisfied and the hand is left on screen.
    void test_theCursorStopsBeingAHandAfterTheMouseLeavesALinkDownwards()
    {
        mpServer->setWelcomeMessage(qsl("cursor test\r\n"));
        startProfile(mpHostname, mpLocalhost, mpPort);
        QVERIFY2(waitForTextInBuffer(qsl("cursor test")), "the welcome text never reached the buffer");

        mudlet::self()->resize(1200, 800);
        QTest::qWait(100ms);

        TTextEdit* pane = upperPane();
        QVERIFY2(pane, "No upper pane available");

        auto host = mudlet::self()->getActiveHost();
        // the profile's own startup output otherwise fills the pane, leaving no
        // blank rows under the link for the mouse to move down into
        QVERIFY2(host->getLuaInterpreter()->compileAndExecuteScript(qsl("clearWindow()\nechoLink('LinkForCursorTest', [[ ]], '', true)\n")), "the echoLink() call failed");
        QVERIFY2(waitForTextInBuffer(qsl("LinkForCursorTest")), "the link text never reached the buffer");
        QTest::qWait(100ms);

        // the link's own pixel is found rather than calculated, so a timestamp
        // gutter or a font of another size cannot put this on the wrong cell
        QPoint overTheLink;
        for (int y = pane->mFontHeight / 2; y < pane->height() && overTheLink.isNull(); y += pane->mFontHeight) {
            for (int x = pane->mFontWidth / 2; x < pane->width(); x += pane->mFontWidth) {
                sendMouse(pane, QEvent::MouseMove, Qt::NoButton, Qt::NoButton, QPointF(x, y));
                if (pane->cursor().shape() == Qt::PointingHandCursor) {
                    overTheLink = QPoint(x, y);
                    break;
                }
            }
        }
        QVERIFY2(!overTheLink.isNull(), "no pixel of the pane produced the hand cursor, so the move below proves nothing");
        QCOMPARE(pane->cursor().shape(), Qt::PointingHandCursor);

        // straight down from the link, into the empty part of the pane below
        // every line the buffer holds
        const int emptyRowY = pane->height() - (pane->mFontHeight / 2);
        QVERIFY2((emptyRowY / pane->mFontHeight) + pane->imageTopLine() >= static_cast<int>(pane->mpBuffer->buffer.size()),
                 "the chosen row still holds text, so it does not exercise the reported case");
        sendMouse(pane, QEvent::MouseMove, Qt::NoButton, Qt::NoButton, QPointF(overTheLink.x(), emptyRowY));

        QCOMPARE(pane->cursor().shape(), Qt::IBeamCursor);
    }

    // The same reset, reached by the other bounds check: an empty line inside
    // the buffer answers the line check but has no character to look a link up
    // on, so convertMouseXToBufferX() falls out of its loop and the reset has to
    // come from the character check instead. That check also stands between the
    // condition and its own at(tCharIndex) call, so losing it on this arm throws
    // rather than merely stranding the hand.
    void test_theCursorStopsBeingAHandOverAnEmptyLineInsideTheBuffer()
    {
        mpServer->setWelcomeMessage(qsl("cursor test\r\n"));
        startProfile(mpHostname, mpLocalhost, mpPort);
        QVERIFY2(waitForTextInBuffer(qsl("cursor test")), "the welcome text never reached the buffer");

        mudlet::self()->resize(1200, 800);
        QTest::qWait(100ms);

        TTextEdit* pane = upperPane();
        QVERIFY2(pane, "No upper pane available");

        auto host = mudlet::self()->getActiveHost();
        // Hide would keep the blank line out of the buffer and ReplaceWithSpace
        // would give it a character to find, either of which leaves this
        // exercising the same arm as the test above
        host->mBlankLineBehaviour = Host::BlankLineBehaviour::Show;
        QVERIFY2(host->getLuaInterpreter()->compileAndExecuteScript(qsl("clearWindow()\nechoLink('LinkAboveEmptyRow', [[ ]], '', true)\necho('\\n\\nrowBelowTheBlankOne\\n')\n")),
                 "the echoLink() call failed");
        QVERIFY2(waitForTextInBuffer(qsl("rowBelowTheBlankOne")), "the text under the blank line never reached the buffer");
        QTest::qWait(100ms);

        QPoint overTheLink;
        for (int y = pane->mFontHeight / 2; y < pane->height() && overTheLink.isNull(); y += pane->mFontHeight) {
            for (int x = pane->mFontWidth / 2; x < pane->width(); x += pane->mFontWidth) {
                sendMouse(pane, QEvent::MouseMove, Qt::NoButton, Qt::NoButton, QPointF(x, y));
                if (pane->cursor().shape() == Qt::PointingHandCursor) {
                    overTheLink = QPoint(x, y);
                    break;
                }
            }
        }
        QVERIFY2(!overTheLink.isNull(), "no pixel of the pane produced the hand cursor, so the move below proves nothing");
        QCOMPARE(pane->cursor().shape(), Qt::PointingHandCursor);

        // straight down onto the blank line, which the row under it keeps inside
        // the buffer
        const int blankRowY = overTheLink.y() + pane->mFontHeight;
        const int blankRowLine = (blankRowY / pane->mFontHeight) + pane->imageTopLine();
        QVERIFY2(blankRowLine < static_cast<int>(pane->mpBuffer->buffer.size()),
                 "the row below the link is past the buffer, which is the case the test above covers rather than this one");
        QVERIFY2(pane->mpBuffer->buffer.at(blankRowLine).empty(),
                 qPrintable(qsl("the row below the link holds %1 characters rather than none, so the character check is not what has to reject it")
                                    .arg(pane->mpBuffer->buffer.at(blankRowLine).size())));
        sendMouse(pane, QEvent::MouseMove, Qt::NoButton, Qt::NoButton, QPointF(overTheLink.x(), blankRowY));

        QCOMPARE(pane->cursor().shape(), Qt::IBeamCursor);
    }

    // TConsole::selectSection() refuses a length that would put a selection's
    // end before its start, but TBuffer::replaceInLine() takes the two points
    // as it is given them, and its own bounds checks only ask that each column
    // is on the line. A reversed pair walked erase() over a range of negative
    // length, which moves memory backwards out of the vector's allocation - so
    // the guard inside replaceInLine() is what stands behind the Lua one, and
    // no spec can reach it once selectSection() refuses first.
    //
    // Declared last of the slots: without the guard this corrupts the heap, and
    // the abort that follows lands wherever the next allocation happens to be,
    // taking the rest of the run with it.
    void test_replaceInLineRefusesARangeThatRunsBackwards()
    {
        mpServer->setWelcomeMessage(qsl("selection test\r\n"));
        startProfile(mpHostname, mpLocalhost, mpPort);
        QVERIFY2(waitForTextInBuffer(qsl("selection test")), "the welcome text never reached the buffer");

        TMainConsole* console = mudlet::self()->getActiveHost()->mpConsole;
        console->print(qsl("\nbackwards selection"));
        const int y = console->buffer.getLastLineNumber();
        QCOMPARE(console->buffer.line(y), qsl("backwards selection"));

        // both columns are on the line, so only their order can reject this
        QPoint begin(12, y);
        QPoint end(4, y);
        TChar format;
        QVERIFY2(!console->buffer.replaceInLine(begin, end, qsl("XXXX"), format), "a range whose start is past its end was accepted");
        QCOMPARE(console->buffer.line(y), qsl("backwards selection"));
    }

    // The same reversal, one line at a time rather than over the range as a
    // whole: a range that starts at the very end of its first line leaves that
    // line's start past the end the loop computes for it, and an empty line
    // inside a multi-line range has an end of -1 with a start of 0. Nothing in
    // the codebase builds a P_begin/P_end pair spanning two lines today, so
    // this drives the buffer directly.
    void test_replaceInLineSkipsALineWhoseStartIsPastItsOwnEnd()
    {
        mpServer->setWelcomeMessage(qsl("selection test\r\n"));
        startProfile(mpHostname, mpLocalhost, mpPort);
        QVERIFY2(waitForTextInBuffer(qsl("selection test")), "the welcome text never reached the buffer");

        TMainConsole* console = mudlet::self()->getActiveHost()->mpConsole;
        console->print(qsl("\nfirst\nsecond"));
        const int lastLine = console->buffer.getLastLineNumber();
        const int firstLine = lastLine - 1;
        QCOMPARE(console->buffer.line(firstLine), qsl("first"));
        QCOMPARE(console->buffer.line(lastLine), qsl("second"));

        // starting at the column after the last character of "first" - which
        // the bounds check above permits - leaves nothing of that line in range
        QPoint begin(static_cast<int>(console->buffer.line(firstLine).size()), firstLine);
        QPoint end(3, lastLine);
        TChar format;
        // an empty replacement, the way TBuffer::cut() calls this, so what the
        // lines lose is only what the range covered
        QVERIFY(console->buffer.replaceInLine(begin, end, QString(), format));

        QCOMPARE(console->buffer.line(firstLine), qsl("first"));
        QCOMPARE(console->buffer.line(lastLine), qsl("ond"));

        // the text and the TChar that carries each character's formatting are
        // held apart, and a reversed erase() grows the vector rather than
        // shortening it - which the text above cannot show, since it comes from
        // the QString side alone
        QCOMPARE(static_cast<int>(console->buffer.buffer.at(firstLine).size()), static_cast<int>(console->buffer.line(firstLine).size()));
        QCOMPARE(static_cast<int>(console->buffer.buffer.at(lastLine).size()), static_cast<int>(console->buffer.line(lastLine).size()));
    }

    void cleanup()
    {
        const QString profilePath = MudletPaths::getMudletPath(enums::profileHomePath, mpHostname);

        // Tear down Mudlet (and with it the live cTelnet connection) before the
        // stub server it is talking to, so the socket is closed from the client
        // side rather than being yanked out from under an active connection when
        // the server is destroyed - the latter ordering can flake or crash.
        delete mudlet::self();
        delete mpServer;
        mpServer = nullptr;
        deleteDirectory(profilePath);
    }

private:
    void startProfile(const QString& hostname, const QString& address, const QString& port)
    {
        auto host = TestProfile::create(hostname, address, port);
        if (!host) {
            QFAIL("No active host available for the test.");
        }

        QSignalSpy spy2(&(host->mTelnet), &cTelnet::signal_connected);
        if (!spy2.wait(2000)) {
            QFAIL("Could not connect with the host.");
        }
    }

    bool waitForTextInBuffer(const QString& text, int timeoutMs = 5000)
    {
        auto console = mudlet::self()->getActiveHost()->mpConsole;
        return QTest::qWaitFor(
                [&]() {
                    for (int i = 0; i <= console->buffer.getLastLineNumber(); ++i) {
                        if (console->buffer.line(i) == text) {
                            return true;
                        }
                    }
                    return false;
                },
                timeoutMs);
    }

    void deleteProfileDirectory(const QString& profileName)
    {
        const QString path = MudletPaths::getMudletPath(enums::profileHomePath, profileName);
        deleteDirectory(path);
    }

    void deleteDirectory(const QString& path)
    {
        QDir dir(path);
        if (!dir.exists()) {
            return;
        }
        dir.removeRecursively();
    }
};

#include "MainConsoleSelectionTest.moc"
MUDLET_GROUPED_TEST_MAIN(MainConsoleSelectionTest)
