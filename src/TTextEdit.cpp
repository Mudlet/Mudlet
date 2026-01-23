/***************************************************************************
 *   Copyright (C) 2008-2012 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2014-2016, 2018-2023 by Stephen Lyons                   *
 *                                               - slysven@virginmedia.com *
 *   Copyright (C) 2016-2017 by Ian Adkins - ieadkins@gmail.com            *
 *   Copyright (C) 2017 by Chris Reid - WackyWormer@hotmail.com            *
 *   Copyright (C) 2018 by Huadong Qi - novload@outlook.com                *
 *   Copyright (C) 2022 by Thiago Jung Bauermann - bauermann@kolabnow.com  *
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

#include "TAccessibleTextEdit.h"
#include "TTextEdit.h"

#include "TConsole.h"
#include "TDockWidget.h"
#include "TEvent.h"
#include "mudlet.h"
#include "widechar_width.h"
#include "TTextProperties.h"

#include <chrono>
#include <cmath>
#include <QtEvents>
#include <QtGlobal>
#include <QAccessible>
#include <QAccessibleTextCursorEvent>
#include <QAccessibleTextInsertEvent>
#include <QApplication>
#include <QChar>
#include <QClipboard>
#include <QDesktopServices>
#include <QHash>
#include <QPainter>
#include <QPainterPath>
#include <QScrollBar>
#include <QStringRef>
#include <QTextBoundaryFinder>
#include <QToolTip>
#include <QVersionNumber>

// Renders text on screen
// Text data stored separately in a TBuffer
TTextEdit::TTextEdit(TConsole* pC, QWidget* pW, TBuffer* pB, Host* pH, bool isLowerPane)
: QWidget(pW)
, mIsLowerPane(isLowerPane)
, mpBuffer(pB)
, mpConsole(pC)
, mpHost(pH)
, mWideAmbigousWidthGlyphs(pH->wideAmbiguousEAsianGlyphs())
, mMouseWheelRemainder()
{
    mLastClickTimer.start();
    Q_ASSERT_X(mpHost, "TTextEdit::TTextEdit(...)", "mpHost is a nullptr");
    Q_ASSERT_X(mSearchHighlightFgColor != mSearchHighlightBgColor, "TTextEdit::TTextEdit(...)", "search highlight foreground and background colors must not be the same");
    setFont(mpHost->getDisplayFont());
    mFontHeight = fontMetrics().height();
    mFontWidth = fontMetrics().averageCharWidth();
    if (pC->getType() != TConsole::CentralDebugConsole) {
#if defined(DEBUG_CODEPOINT_PROBLEMS)
        // There is no point in setting this option on the Central Debug Console
        // as A) it is shared and B) any codepoints that it can't handle will
        // probably have already cropped up on another TConsole:
        if (!mIsLowerPane) {
            mShowAllCodepointIssues = mpHost->debugShowAllProblemCodepoints();
            connect(mpHost, &Host::signal_changeDebugShowAllProblemCodepoints, this, &TTextEdit::slot_changeDebugShowAllProblemCodepoints);
        }
#endif
    } else {
        // This is part of the Central Debug Console
        mFgColor = QColor(192, 192, 192);
        mBgColor = Qt::black;
    }
    mScreenHeight = height() / mFontHeight;

    setMouseTracking(true);
    QCursor cursor;
    cursor.setShape(Qt::IBeamCursor);
    setCursor(cursor);
    setAttribute(Qt::WA_OpaquePaintEvent, false);
    setAttribute(Qt::WA_DeleteOnClose);

    showNewLines();
    setMouseTracking(true); // test fix for MAC
    setEnabled(true);       //test fix for MAC

    connect(mpHost, &Host::signal_changeIsAmbigousWidthGlyphsToBeWide, this, &TTextEdit::slot_changeIsAmbigousWidthGlyphsToBeWide, Qt::UniqueConnection);
}

void TTextEdit::forceUpdate()
{
    mForceUpdate = true;
    update();
}

void TTextEdit::needUpdate(int y1, int y2)
{
    if (!mIsTailMode) {
        return;
    }

    if (mScreenHeight == 0) {
        return;
    }
    int top = imageTopLine();
    int bottom = y2 - y1;

    if (top > 0) {
        top = (y1 - top) % mScreenHeight;
    } else {
        top = y1 % mScreenHeight;
    }
    QRect r(0, top * mFontHeight, mScreenWidth * mFontWidth, bottom * mFontHeight);
    mForceUpdate = true;
    update(r);
}

void TTextEdit::focusInEvent(QFocusEvent* event)
{
    update();
    QWidget::focusInEvent(event);
}

void TTextEdit::focusOutEvent(QFocusEvent* event)
{
    // Safety check: during destruction, mpHost might be null
    // Avoid disabling caret mode when the window is merely deactivated
    // (e.g., user Alt+Tabs away). This preserves focus for screen reader
    // users like NVDA so returning to Mudlet restores the original widget
    // rather than forcing focus to the command line.
    if (mpHost && mpHost->caretEnabled() && event->reason() != Qt::ActiveWindowFocusReason) {
        mpHost->setCaretEnabled(false);
    }

    QWidget::focusOutEvent(event);
}
// debug using gammaray to see which events are raised

void TTextEdit::toggleTimeStamps(const bool state)
{
    Q_UNUSED(state)
    forceUpdate();
    update();
}

// Only wired up for the upper pane:
void TTextEdit::slot_scrollBarMoved(int line)
{
    if (mpConsole->mpScrollBar) {
        updateScrollBar(line);
        scrollTo(line);
    }
}

void TTextEdit::updateScrollBar(int line)
{
    Q_ASSERT_X(!mIsLowerPane, "updateScrollBar(...)", "called on LOWER pane when it should only be used on upper one!");
    int screenHeight{mScreenHeight};
    if (mIsTailMode) {
        screenHeight -= mpConsole->mLowerPane->getScreenHeight();
    }
    if (mpConsole->mpScrollBar) {
        disconnect(mpConsole->mpScrollBar, &QAbstractSlider::valueChanged, this, &TTextEdit::slot_scrollBarMoved);
        mpConsole->mpScrollBar->setRange(screenHeight, mpBuffer->getLastLineNumber() + 1);
        mpConsole->mpScrollBar->setSingleStep(1);
        mpConsole->mpScrollBar->setPageStep(screenHeight);
        mpConsole->mpScrollBar->setValue(std::max(0, line));
        connect(mpConsole->mpScrollBar, &QAbstractSlider::valueChanged, this, &TTextEdit::slot_scrollBarMoved);
    }
}

// Only wired up for the upper pane:
void TTextEdit::slot_hScrollBarMoved(int offset)
{
    if (mpConsole->mHScrollBarEnabled && mpConsole->mpHScrollBar) {
        updateHorizontalScrollBar();
        scrollH(offset);
    }
}

void TTextEdit::calculateHMaxRange()
{
    if (mIsLowerPane) {
        return;
    }

    int columnCount = getColumnCount();
    mMaxHRange = mScreenOffset - columnCount;

    if (mMaxHRange < 1) {
        mCursorX = 0;
        mMaxHRange = 0;
        return;
    }
    if (mCursorX > mMaxHRange) {
        mCursorX = mMaxHRange;
    }
}

void TTextEdit::updateHorizontalScrollBar()
{
    if (mIsLowerPane) {
        return;
    }

    int columnCount = getColumnCount();

    if (mMaxHRange < 1 && mpConsole->mpHScrollBar->isVisible()) {
        mpConsole->mpHScrollBar->hide();
    }

    if (mMaxHRange > 0 && !mpConsole->mpHScrollBar->isVisible()) {
        mpConsole->mpHScrollBar->show();
    }

    disconnect(mpConsole->mpHScrollBar, &QAbstractSlider::valueChanged, this, &TTextEdit::slot_hScrollBarMoved);
    mpConsole->mpHScrollBar->setRange(0, mMaxHRange);
    mpConsole->mpHScrollBar->setSingleStep(1);
    mpConsole->mpHScrollBar->setPageStep(columnCount);
    mpConsole->mpHScrollBar->setValue(mCursorX);
    connect(mpConsole->mpHScrollBar, &QAbstractSlider::valueChanged, this, &TTextEdit::slot_hScrollBarMoved);
}

void TTextEdit::updateScreenView()
{
    // Safety check: during destruction, mpHost or mpConsole might be null
    if (!mpHost || !mpConsole) {
        return;
    }

    mFontWidth = fontMetrics().averageCharWidth();
    mFontHeight = fontMetrics().height();
    if (isHidden()) {
        return; //NOTE: otherwise mScreenHeight==0 would cause a floating point exception
    }
    if (mpConsole->getType() == TConsole::MainConsole) {
        mBgColor = mpHost->mBgColor;
        mFgColor = mpHost->mFgColor;
    }
    mScreenHeight = visibleRegion().boundingRect().height() / mFontHeight;
    if (!mIsLowerPane) {
        updateScrollBar(mpBuffer->mCursorY);
    }
    int currentScreenWidth = visibleRegion().boundingRect().width() / mFontWidth;
    if (mpConsole->getType() == TConsole::MainConsole) {
        // This is the MAIN console - we do not want it to ever disappear!
        mScreenWidth = qMax(40, currentScreenWidth);

        // Note the values in the "parent" Host instance - for the UPPER pane
        // so that they are available for NAWS:
        if (!mIsLowerPane) {
            mpHost->setScreenDimensions(mScreenWidth, mScreenHeight);
        }
    } else {
        mScreenWidth = currentScreenWidth;
    }
    mOldScrollPos = mpBuffer->getLastLineNumber();
}

void TTextEdit::showNewLines()
{
    if (mIsLowerPane) {
        if (isHidden()) {
            // If it is not showing there is no point in showing new lines in
            // the lower pane - as it happens it being hidden should be the same
            // as the upper pane having mIsTailMode set!
            return;
        }

    } else {
        if (!mIsTailMode) {
            // If not in tail mode the upper pane is frozen
            return;
        }
    }

    mCursorY = mpBuffer->size();
    mCursorX = 0;
    if (!mIsLowerPane) {
        mpBuffer->mCursorY = mpBuffer->size();
    }

    const int previousOldScrollPos = mOldScrollPos;

    mOldScrollPos = mpBuffer->getLastLineNumber();

    if (!mIsLowerPane) {
        // This is ONLY for the upper pane
        if (mpConsole->mpScrollBar && mOldScrollPos > 0) {
            updateScrollBar(mpBuffer->mCursorY);
        }
    }
    update();


    if (mpHost && QAccessible::isActive() && mpConsole->getType() == TConsole::MainConsole
        && mpHost->mAnnounceIncomingText && mudlet::self()->getActiveHost() == mpHost) {
        QString newLines;

        // content has been deleted
        if (previousOldScrollPos > mOldScrollPos) {
            QAccessibleTextInterface* ti = QAccessible::queryAccessibleInterface(this)->textInterface();
            auto totalCharacterCount = ti->characterCount();
            ti->setCursorPosition(totalCharacterCount);
            QAccessibleTextRemoveEvent event(this, totalCharacterCount, QString());
            QAccessible::updateAccessibility(&event);
            return;
        }

        // content has been added
        for (int i = previousOldScrollPos; i < mOldScrollPos; i++) {
            newLines.append(mpBuffer->line(i));
            newLines.append('\n');
        }

        newLines = newLines.trimmed();
        if (newLines.isEmpty()) {
            return;
        }

        mudlet::self()->announce(newLines, QString(), true);
    }
}

void TTextEdit::scrollTo(int line)
{
    // Protect against modifying mIsTailMode on the lower pane where it would
    // be wrong:
    Q_ASSERT_X(!mIsLowerPane, "Inappropriate use of method on lower pane which should only be used for the upper one", "TTextEdit::scrollTo()");
    if ((line > -1) && (line <= mpBuffer->size())) {
        if ((line < (mpBuffer->getLastLineNumber() + 1) && mIsTailMode)) {
            mIsTailMode = false;
            mpConsole->mLowerPane->mCursorY = mpBuffer->size();
            mpConsole->mLowerPane->show();
            mpConsole->mLowerPane->forceUpdate();
        } else if ((line > (mpBuffer->getLastLineNumber())) && !mIsTailMode) {
            mpConsole->mLowerPane->mCursorY = mpConsole->buffer.getLastLineNumber();
            mpConsole->mLowerPane->hide();
            mIsTailMode = true;
            mCursorY = mpConsole->buffer.getLastLineNumber();
            updateScreenView();
            forceUpdate();
        }
        mpBuffer->mCursorY = line;

        mScrollVector = 0;
        update();
    }
}

void TTextEdit::scrollH(int offset)
{
    mCursorX = offset;
    forceUpdate();
}

void TTextEdit::scrollUp(int lines)
{
    if (mIsLowerPane) {
        return;
    }

    mpBuffer->mCursorY -= lines;
    mScrollVector = 0;
    mIsTailMode = false;
    updateScrollBar(mpBuffer->mCursorY);
    update();
}

void TTextEdit::scrollDown(int lines)
{
    if (mIsLowerPane) {
        return;
    }

    if (bufferScrollDown(lines)) {
        mScrollVector = 0;
        updateScrollBar(mpBuffer->mCursorY);
        update();
    }
}

void TTextEdit::drawLine(QPainter& painter, int lineNumber, int lineOfScreen, int* offset) const
{
    QPoint cursor(-mCursorX, lineOfScreen);
    QString lineText = mpBuffer->lineBuffer.at(lineNumber);
    QTextBoundaryFinder boundaryFinder(QTextBoundaryFinder::Grapheme, lineText);
    int currentSize = lineText.size();
    if (mpConsole->showTimeStamps()) {
        TChar timeStampStyle(QColor(200, 150, 0), QColor(22, 22, 22));
        QString timestamp(mpBuffer->timeBuffer.at(lineNumber));
        QVector<QColor> fgColors;
        QVector<QRect> textRects;
        QVector<int> charWidths;
        QVector<QString> graphemes;
        for (const QChar c : timestamp) {
            // The column argument is not incremented here (is fixed at 0) so
            // the timestamp does not take up any places when it is clicked on
            // by the mouse...
            cursor.setX(cursor.x() + drawGraphemeBackground(painter, fgColors, textRects, graphemes, charWidths, cursor, c, 0, lineNumber, timeStampStyle));
        }
        int index = -1;
        for (const QChar c : timestamp) {
            ++index;
            drawGraphemeForeground(painter, fgColors.at(index), textRects.at(index), c, timeStampStyle);
        }
        currentSize += mudlet::smTimeStampFormat.size();
    }

    //get the longest line
    if (offset && *offset < currentSize) {
        *offset = currentSize;
    }

    int columnWithOutTimestamp = 0;
    QVector<QColor> fgColors;
    QVector<QRect> textRects;
    QVector<int> charWidths;
    QVector<QString> graphemes;
    for (int indexOfChar = 0, total = lineText.size(); indexOfChar < total;) {
        int nextBoundary = boundaryFinder.toNextBoundary();

        TChar& charStyle = mpBuffer->buffer.at(lineNumber).at(indexOfChar);
        int graphemeWidth = drawGraphemeBackground(painter, fgColors, textRects, graphemes, charWidths, cursor, lineText.mid(indexOfChar, nextBoundary - indexOfChar), columnWithOutTimestamp, lineNumber, charStyle);
        cursor.setX(cursor.x() + graphemeWidth);
        indexOfChar = nextBoundary;
        columnWithOutTimestamp += graphemeWidth;
    }
    boundaryFinder.toStart();
    int index = -1;
    for (int indexOfChar = 0, total = lineText.size(); indexOfChar < total;) {
        int nextBoundary = boundaryFinder.toNextBoundary();

        TChar& charStyle = mpBuffer->buffer.at(lineNumber).at(indexOfChar);
        ++index;
        drawGraphemeForeground(painter, fgColors.at(index), textRects.at(index), graphemes.at(index), charStyle);
        indexOfChar = nextBoundary;
    }

    // If caret mode is enabled and the line is empty, still draw the caret.
    if (mpHost && mpHost->caretEnabled() && mCaretLine == lineNumber && lineText.isEmpty()) {
        auto textRect = QRect(0, mFontHeight * lineOfScreen, mFontWidth, mFontHeight);
        painter.fillRect(textRect, mCaretColor);
    }
}

/* inline */ void TTextEdit::replaceControlCharacterWith_Picture(const uint unicode, const QString& grapheme, const int column, QVector<QString>& graphemes, int& charWidth) const
{
    switch (unicode) {
    case 0:     graphemes.append(QChar(0x2400)); charWidth = 1; break; // NUL - not sure that this can appear
    case 1:     graphemes.append(QChar(0x2401)); charWidth = 1; break; // SOH
    case 2:     graphemes.append(QChar(0x2402)); charWidth = 1; break; // STX
    case 3:     graphemes.append(QChar(0x2403)); charWidth = 1; break; // ETX
    case 4:     graphemes.append(QChar(0x2404)); charWidth = 1; break; // EOT
    case 5:     graphemes.append(QChar(0x2405)); charWidth = 1; break; // ENQ
    case 6:     graphemes.append(QChar(0x2406)); charWidth = 1; break; // ACK
    case 7:     graphemes.append(QChar(0x2407)); charWidth = 1; break; // BEL - the (audio) handling of this gets done when it is received, not when it is displayed here:
    case 8:     graphemes.append(QChar(0x2408)); charWidth = 1; break; // BS
    case 9: // HT
        // Makes the spacing behave like a tab
        charWidth = mTabStopwidth - (column % mTabStopwidth);
        // But print the "control picture" on top
        graphemes.append(QChar(0x2409));
        break;
    case 10:    graphemes.append(QChar(0x240A)); charWidth = 1; break; // LF - may not ever appear!
    case 11:    graphemes.append(QChar(0x240B)); charWidth = 1; break; // VT
    case 12:    graphemes.append(QChar(0x240C)); charWidth = 1; break; // FF
    case 13:    graphemes.append(QChar(0x240D)); charWidth = 1; break; // CR - shouldn't appear but does seem to crop up somehow!
    case 14:    graphemes.append(QChar(0x240E)); charWidth = 1; break; // SO
    case 15:    graphemes.append(QChar(0x240F)); charWidth = 1; break; // SI
    case 16:    graphemes.append(QChar(0x2410)); charWidth = 1; break; // DLE
    case 17:    graphemes.append(QChar(0x2411)); charWidth = 1; break; // DC1
    case 18:    graphemes.append(QChar(0x2412)); charWidth = 1; break; // DC2
    case 19:    graphemes.append(QChar(0x2413)); charWidth = 1; break; // DC3
    case 20:    graphemes.append(QChar(0x2414)); charWidth = 1; break; // DC4
    case 21:    graphemes.append(QChar(0x2415)); charWidth = 1; break; // NAK
    case 22:    graphemes.append(QChar(0x2416)); charWidth = 1; break; // SYN
    case 23:    graphemes.append(QChar(0x2417)); charWidth = 1; break; // ETB
    case 24:    graphemes.append(QChar(0x2418)); charWidth = 1; break; // CAN
    case 25:    graphemes.append(QChar(0x2419)); charWidth = 1; break; // EM
    case 26:    graphemes.append(QChar(0x241A)); charWidth = 1; break; // SUB
    case 27:    graphemes.append(QChar(0x241B)); charWidth = 1; break; // ESC - shouldn't appear as will have been intercepted previously
    case 28:    graphemes.append(QChar(0x241C)); charWidth = 1; break; // FS
    case 29:    graphemes.append(QChar(0x241D)); charWidth = 1; break; // GS
    case 30:    graphemes.append(QChar(0x241E)); charWidth = 1; break; // RS
    case 31:    graphemes.append(QChar(0x241F)); charWidth = 1; break; // US
    case 127:   graphemes.append(QChar(0x2421)); charWidth = 1; break; // DEL
    default:
        charWidth = getGraphemeWidth(unicode);
        graphemes.append((charWidth < 1) ? QChar() : grapheme);
    }
}

/* inline */ void TTextEdit::replaceControlCharacterWith_OEMFont(const uint unicode, const QString& grapheme, const int column, QVector<QString>& graphemes, int& charWidth) const
{
    Q_UNUSED(column)
    switch (unicode) {
    case 0:     graphemes.append(QString(QChar::Space)); charWidth = 1; break; // NUL - not sure that this can appear and the OEM font treats it as a space
    case 1:     graphemes.append(QChar(0x263A)); charWidth = 1; break; // SOH - White Smiling Face
    case 2:     graphemes.append(QChar(0x263B)); charWidth = 1; break; // STX - Black Smiling Face
    case 3:     graphemes.append(QChar(0x2665)); charWidth = 1; break; // ETX - Black Heart Suite
    case 4:     graphemes.append(QChar(0x2666)); charWidth = 1; break; // EOT - Black Diamond Suite
    case 5:     graphemes.append(QChar(0x2663)); charWidth = 1; break; // ENQ - Black ClubsSuite
    case 6:     graphemes.append(QChar(0x2660)); charWidth = 1; break; // ACK - Black Spade Suite
    case 7:     graphemes.append(QChar(0x2022)); charWidth = 1; break; // BEL - Bullet - the handling of this gets done when it is received, not when it is displayed here:
    case 8:     graphemes.append(QChar(0x25D8)); charWidth = 1; break; // BS  - Inverse Bullet
    case 9:
        // NOTE THAT WE DO NOT USE TAB SPACING FOR THIS MODE:
                graphemes.append(QChar(0x25CB)); charWidth = 1; break; // HT  - Circle
    case 10:    graphemes.append(QChar(0x25D9)); charWidth = 1; break; // LF  - Inverse Circle
    case 11:    graphemes.append(QChar(0x2642)); charWidth = 1; break; // VT  - Male Sign
    case 12:    graphemes.append(QChar(0x2640)); charWidth = 1; break; // FF  - Female Sign
    case 13:    graphemes.append(QChar(0x266A)); charWidth = 1; break; // CR  - Single Quaver - shouldn't appear but does seem to crop up somehow!
    case 14:    graphemes.append(QChar(0x266B)); charWidth = 1; break; // SO  - Double Quaver
    case 15:    graphemes.append(QChar(0x263C)); charWidth = 1; break; // SI  - White Sun with Rays
    case 16:    graphemes.append(QChar(0x25BA)); charWidth = 1; break; // DLE - Black Right-Pointing Pointer
    case 17:    graphemes.append(QChar(0x25C4)); charWidth = 1; break; // DC1 - Black Left-Pointing Pointer
    case 18:    graphemes.append(QChar(0x2195)); charWidth = 1; break; // DC2 - Up Down ArroW
    case 19:    graphemes.append(QChar(0x203C)); charWidth = 1; break; // DC3 - Double Exclaimation Mark
    case 20:    graphemes.append(QChar(0x00B6)); charWidth = 1; break; // DC4 - Pilcrow
    case 21:    graphemes.append(QChar(0x00A7)); charWidth = 1; break; // NAK - Section Sign
    case 22:    graphemes.append(QChar(0x25AC)); charWidth = 1; break; // SYN - Black Rectangle
    case 23:    graphemes.append(QChar(0x21A8)); charWidth = 1; break; // ETB - Up Down Arrow With Base
    case 24:    graphemes.append(QChar(0x2191)); charWidth = 1; break; // CAN - Up Arrow
    case 25:    graphemes.append(QChar(0x2193)); charWidth = 1; break; // EM  - Down Arrow
    case 26:    graphemes.append(QChar(0x2192)); charWidth = 1; break; // SUB - Right Arrow
    case 27:    graphemes.append(QChar(0x2190)); charWidth = 1; break; // ESC - Left Arrow - shouldn't appear as will have been intercepted previously
    case 28:    graphemes.append(QChar(0x221F)); charWidth = 1; break; // FS  - Right Angle
    case 29:    graphemes.append(QChar(0x2194)); charWidth = 1; break; // GS  - Left Right Arrow
    case 30:    graphemes.append(QChar(0x25B2)); charWidth = 1; break; // RS  - Black Up-Pointing Pointer
    case 31:    graphemes.append(QChar(0x25BC)); charWidth = 1; break; // US  - Black Down-Pointing Pointer
    case 127:   graphemes.append(QChar(0x2302)); charWidth = 1; break; // DEL - House
    default:
        charWidth = getGraphemeWidth(unicode);
        graphemes.append((charWidth < 1) ? QChar() : grapheme);
    }
}

int TTextEdit::drawGraphemeBackground(QPainter& painter, QVector<QColor>& fgColors, QVector<QRect>& textRects, QVector<QString>& graphemes, QVector<int>& charWidths, QPoint& cursor, const QString& grapheme, const int column, const int line, TChar& charStyle) const
{
    uint unicode = graphemeInfo::getBaseCharacter(grapheme);
    int charWidth = 0;

    switch (mpConsole->mControlCharacter) {
    default:
        // No special handling, except for these:
        if (Q_UNLIKELY(unicode == '\t')) {
            charWidth = mTabStopwidth - (column % mTabStopwidth);
            graphemes.append(QString(QChar::Tabulation));
        } else {
            charWidth = graphemeInfo::getWidth(unicode, mWideAmbigousWidthGlyphs);
            graphemes.append((charWidth < 1) ? QChar() : grapheme);
        }
        break;
    case ControlCharacterMode::Picture:
        replaceControlCharacterWith_Picture(unicode, grapheme, column, graphemes, charWidth);
        break;
    case ControlCharacterMode::OEM:
        replaceControlCharacterWith_OEMFont(unicode, grapheme, column, graphemes, charWidth);
        break;
    } // End of switch
    charWidths.append(charWidth);

    QRect textRect;
    if (charWidth > 0) {
        textRect = QRect(mFontWidth * cursor.x(), mFontHeight * cursor.y(), mFontWidth * charWidth, mFontHeight);
    }
    textRects.append(textRect);
    QColor bgColor;
    bool caretIsHere = mpHost && mpHost->caretEnabled() && mCaretLine == line && mCaretColumn == column;
    if (Q_UNLIKELY(charStyle.isFound())) {
        if (Q_UNLIKELY(charStyle.isReversed() != (charStyle.isSelected() != caretIsHere))) {
            fgColors.append(mSearchHighlightBgColor);
            bgColor = mSearchHighlightFgColor;
        } else {
            fgColors.append(mSearchHighlightFgColor);
            bgColor = mSearchHighlightBgColor;
        }
    } else {
        if (Q_UNLIKELY(charStyle.isReversed() != (charStyle.isSelected() != caretIsHere))) {
            // When colors would be swapped (e.g., during selection)
            // and foreground equals background (hidden text),
            // only reverse one color to make the text readable
            if (charStyle.foreground() == charStyle.background()) {
                fgColors.append(charStyle.foreground());
                // Invert background: use white for dark colors, black for light colors
                bgColor = (charStyle.background().lightness() < 128) ? Qt::white : Qt::black;
            } else {
                fgColors.append(charStyle.background());
                bgColor = charStyle.foreground();
            }
        } else {
            fgColors.append(charStyle.foreground());
            bgColor = charStyle.background();
        }
    }
    if (caretIsHere) {
        bgColor = mCaretColor;
    }
    if (!textRect.isNull() && bgColor != mpConsole->getConsoleBgColor()) {
        painter.fillRect(textRect, bgColor);
    }
    return charWidth;
}

void TTextEdit::drawGraphemeForeground(QPainter& painter, const QColor& fgColor, const QRect& textRect, const QString& grapheme, TChar& charStyle) const
{
    TChar::AttributeFlags attributes = charStyle.allDisplayAttributes();
    const bool isBold = attributes & TChar::Bold;
    // At present we cannot display flashing text - and we just make it italic
    // (we ought to eventually add knobs for them so they can be shown in a user
    // preferred style - which might be static for some users) - anyhow Mudlet
    // will still detect the difference between the options:
    const bool isItalics = attributes & (TChar::Italic | TChar::Blink | TChar::FastBlink);
    const bool isOverline = attributes & TChar::Overline;
    const bool isStrikeOut = attributes & TChar::StrikeOut;
    const bool isUnderline = attributes & TChar::Underline;

    // Check for advanced underline styles or custom decoration colors
    const bool isUnderlineWavy = attributes & TChar::UnderlineWavy;
    const bool isUnderlineDotted = attributes & TChar::UnderlineDotted;
    const bool isUnderlineDashed = attributes & TChar::UnderlineDashed;
    const bool hasAdvancedUnderline = isUnderlineWavy || isUnderlineDotted || isUnderlineDashed;

    // If we have advanced underline styles or custom decoration colors, disable Qt's built-in decorations
    // and draw them manually later
    const bool useQtUnderline = isUnderline && !hasAdvancedUnderline && !charStyle.hasCustomUnderlineColor();
    const bool useQtOverline = isOverline && !charStyle.hasCustomOverlineColor();
    const bool useQtStrikeOut = isStrikeOut && !charStyle.hasCustomStrikeoutColor();

    // const bool isConcealed = attributes & TChar::Concealed;
    // const int altFontIndex = charStyle.alternateFont();
    if ((painter.font().bold() != isBold)
            || (painter.font().italic() != isItalics)
            || (painter.font().overline() != useQtOverline)
            || (painter.font().strikeOut() != useQtStrikeOut)
            || (painter.font().underline() != useQtUnderline)) {

        QFont font = painter.font();
        font.setBold(isBold);
        font.setItalic(isItalics);
        font.setOverline(useQtOverline);
        font.setStrikeOut(useQtStrikeOut);
        font.setUnderline(useQtUnderline);
        painter.setFont(font);
    }

    if (textRect.isNull()) {
        return;
    }

    if (painter.pen().color() != fgColor) {
        painter.setPen(fgColor);
    }
    painter.drawText(textRect, Qt::AlignCenter|Qt::TextDontClip|Qt::TextSingleLine, grapheme);

    // Draw custom decorations (colored underlines, overlines, strikethrough)
    drawCustomDecorations(painter, fgColor, textRect, charStyle);
}

void TTextEdit::drawCustomDecorations(QPainter& painter, const QColor& defaultColor, const QRect& textRect, TChar& charStyle) const
{
    TChar::AttributeFlags attributes = charStyle.allDisplayAttributes();
    QFontMetrics fm(painter.font());

    // Calculate decoration positions
    int underlineY = textRect.bottom() - 1;
    int overlineY = textRect.top() + 1;
    int strikeoutY = textRect.top() + textRect.height() / 2;
    int lineWidth = 1;

    // Draw underline decorations
    if (attributes & TChar::Underline) {
        QColor underlineColor = charStyle.hasCustomUnderlineColor() ? charStyle.underlineColor() : defaultColor;
        QPen pen(underlineColor);
        pen.setWidth(lineWidth);

        bool isWavy = attributes & TChar::UnderlineWavy;
        bool isDotted = attributes & TChar::UnderlineDotted;
        bool isDashed = attributes & TChar::UnderlineDashed;

        if (isWavy) {
            // Draw wavy underline
            pen.setStyle(Qt::SolidLine);
            painter.setPen(pen);

            const int amplitude = 1;
            const int wavelength = 8;
            QPainterPath wavePath;

            bool firstPoint = true;
            for (int x = textRect.left(); x <= textRect.right(); x += 2) {
                double phase = (x - textRect.left()) * 2.0 * M_PI / wavelength;
                int y = underlineY + amplitude * sin(phase);

                if (firstPoint) {
                    wavePath.moveTo(x, y);
                    firstPoint = false;
                } else {
                    wavePath.lineTo(x, y);
                }
            }
            painter.drawPath(wavePath);

        } else if (isDotted) {
            pen.setStyle(Qt::DotLine);
            painter.setPen(pen);
            painter.drawLine(textRect.left(), underlineY, textRect.right(), underlineY);

        } else if (isDashed) {
            pen.setStyle(Qt::DashLine);
            painter.setPen(pen);
            painter.drawLine(textRect.left(), underlineY, textRect.right(), underlineY);

        } else {
            // Solid underline (only draw if we have custom color or advanced style)
            if (charStyle.hasCustomUnderlineColor() || isWavy || isDotted || isDashed) {
                pen.setStyle(Qt::SolidLine);
                painter.setPen(pen);
                painter.drawLine(textRect.left(), underlineY, textRect.right(), underlineY);
            }
        }
    }

    // Draw overline decorations
    if (attributes & TChar::Overline) {
        QColor overlineColor = charStyle.hasCustomOverlineColor() ? charStyle.overlineColor() : defaultColor;
        QPen pen(overlineColor);
        pen.setWidth(lineWidth);
        pen.setStyle(Qt::SolidLine);
        painter.setPen(pen);
        painter.drawLine(textRect.left(), overlineY, textRect.right(), overlineY);
    }

    // Draw strikethrough decorations
    if (attributes & TChar::StrikeOut) {
        QColor strikeoutColor = charStyle.hasCustomStrikeoutColor() ? charStyle.strikeoutColor() : defaultColor;
        QPen pen(strikeoutColor);
        pen.setWidth(lineWidth);
        pen.setStyle(Qt::SolidLine);
        painter.setPen(pen);
        painter.drawLine(textRect.left(), strikeoutY, textRect.right(), strikeoutY);
    }
}

int TTextEdit::getGraphemeWidth(uint unicode) const
{
#if defined(DEBUG_CODEPOINT_PROBLEMS)
    switch (widechar_wcwidth(unicode)) {
    case widechar_nonprint:
        // -1 = The character is not printable - so put in a replacement
        // character instead - and so it can be seen it need a space:
        if (!mIsLowerPane) {
            bool newCodePointToWarnAbout = !mProblemCodepoints.contains(unicode);
            if (mShowAllCodepointIssues && newCodePointToWarnAbout) {
                qDebug().nospace().noquote() << "TTextEdit::getGraphemeWidth(...) WARN - trying to get width of a Unicode character which is unprintable, codepoint number: U+"
                                             << qsl("%1").arg(unicode, 4, 16, QLatin1Char('0')).toUtf8().constData() << ".";
            }
            if (Q_UNLIKELY(newCodePointToWarnAbout)) {
                mProblemCodepoints.insert(unicode, std::tuple{1, "Unprintable"});
            } else {
                auto [count, reason] = mProblemCodepoints.value(unicode);
                mProblemCodepoints.insert(unicode, std::tuple{++count, reason});
            }
        }
        return 0;
    case widechar_non_character:
        // -7 = The character is a non-character - we might make use of some of them for
        // internal purposes in the future (in which case we might need additional code here
        // or elsewhere) but we don't right now:
        if (!mIsLowerPane) {
            bool newCodePointToWarnAbout = !mProblemCodepoints.contains(unicode);
            if (mShowAllCodepointIssues && newCodePointToWarnAbout) {
                qWarning().nospace().noquote() << "TTextEdit::getGraphemeWidth(...) WARN - trying to get width of a Unicode character which is a non-character that Mudlet is not itself using, codepoint number: U+"
                                             << qsl("%1").arg(unicode, 4, 16, QLatin1Char('0')).toUtf8().constData() << ".";
            }
            if (Q_UNLIKELY(newCodePointToWarnAbout)) {
                mProblemCodepoints.insert(unicode, std::tuple{1, std::string{"Non-character"}});
            } else {
                auto [count, reason] = mProblemCodepoints.value(unicode);
                mProblemCodepoints.insert(unicode, std::tuple{++count, reason});
            }
        }
        return 0;
    case widechar_combining:
        // -2 = The character is a zero-width combiner - and should not be
        // present as the FIRST codepoint in a grapheme so this indicates an
        // error somewhere - so put in the replacement character
        if (!mIsLowerPane) {
            bool newCodePointToWarnAbout = !mProblemCodepoints.contains(unicode);
            if (mShowAllCodepointIssues && newCodePointToWarnAbout) {
                qWarning().nospace().noquote() << "TTextEdit::getGraphemeWidth(...) WARN - trying to get width of a Unicode character which is a zero width combiner, codepoint number: U+"
                                             << qsl("%1").arg(unicode, 4, 16, QLatin1Char('0')).toUtf8().constData() << ".";
            }
            if (Q_UNLIKELY(newCodePointToWarnAbout)) {
                mProblemCodepoints.insert(unicode, std::tuple{1, std::string{"Zero Width Combiner"}});
            } else {
                auto [count, reason] = mProblemCodepoints.value(unicode);
                mProblemCodepoints.insert(unicode, std::tuple{++count, reason});
            }
        }
        return 0;
    case widechar_private_use:
        // -4 = The character is for private use - we cannot know for certain
        // what width to used - let's assume 1 for the moment:
        if (!mIsLowerPane) {
            bool newCodePointToWarnAbout = !mProblemCodepoints.contains(unicode);
            if (mShowAllCodepointIssues && newCodePointToWarnAbout) {
                qDebug().nospace().noquote() << "TTextEdit::getGraphemeWidth(...) WARN - trying to get width of a Private Use Character, we cannot know how wide it is, codepoint number: U+"
                                             << qsl("%1").arg(unicode, 4, 16, QLatin1Char('0')).toUtf8().constData() << ".";
            }
            if (Q_UNLIKELY(newCodePointToWarnAbout)) {
                mProblemCodepoints.insert(unicode, std::tuple{1, std::string{"Private Use"}});
            } else {
                auto [count, reason] = mProblemCodepoints.value(unicode);
                mProblemCodepoints.insert(unicode, std::tuple{++count, reason});
            }
        }
        return 1;
    case widechar_unassigned:
        // -5 = The character is unassigned - at least for the Unicode version
        // that our widechar_wcwidth(...) was built for - assume 1:
        if (!mIsLowerPane) {
            bool newCodePointToWarnAbout = !mProblemCodepoints.contains(unicode);
            if (mShowAllCodepointIssues && newCodePointToWarnAbout) {
                qWarning().nospace().noquote() << "TTextEdit::getGraphemeWidth(...) WARN - trying to get width of a Unicode character which was not previously assigned and we do not know how wide it is, codepoint number: U+"
                                               << qsl("%1").arg(unicode, 4, 16, QLatin1Char('0')).toUtf8().constData() << ".";
            }
            if (Q_UNLIKELY(newCodePointToWarnAbout)) {
                mProblemCodepoints.insert(unicode, std::tuple{1, std::string{"Unassigned"}});
            } else {
                auto [count, reason] = mProblemCodepoints.value(unicode);
                mProblemCodepoints.insert(unicode, std::tuple{++count, reason});
            }
        }
        return 1;
    }
#endif
    return graphemeInfo::getWidth(unicode, mWideAmbigousWidthGlyphs);
}
void TTextEdit::drawForeground(QPainter& painter, const QRect& r)
{
    qreal dpr = devicePixelRatioF();
    QPixmap screenPixmap;
    QPixmap pixmap = QPixmap(mScreenWidth * mFontWidth * dpr, mScreenHeight * mFontHeight * dpr);
    pixmap.setDevicePixelRatio(dpr);
    pixmap.fill(Qt::transparent);

    QPainter p(&pixmap);
    // Setting the font here isn't academic as the text IS drawn with THIS painter (p)
    p.setFont(painter.font());
    p.setCompositionMode(QPainter::CompositionMode_SourceOver);

    int y_top = r.top() / mFontHeight;
    int y_bottom = r.bottom()/ mFontHeight;
    int x_right = std::min(r.right(), (mScreenWidth * mFontWidth)) / mFontWidth;

    int lineOffset = imageTopLine();
    int from = 0;
    if (lineOffset == 0) {
        mScrollVector = 0;
    } else {
        // Was: mScrollVector = lineOffset - mLastRenderedOffset;
        if (mLastRenderedOffset) {
            mScrollVector = lineOffset - mLastRenderedOffset;
        } else {
            mScrollVector = y_bottom + lineOffset;
        }
    }

    bool noScroll = false;
    bool noCopy = false;
    if (abs(mScrollVector) > mScreenHeight || mForceUpdate || lineOffset < 10) {
        mScrollVector = 0;
        noScroll = true;
    }
    if ((r.height() < rect().height()) && (lineOffset > 0)) {
        p.drawPixmap(0, 0, mScreenMap);
        from = y_top;
        noScroll = true;
        if (!mForceUpdate && !mMouseTracking) {
            noCopy = true;
        } else {
            y_bottom = mScreenHeight;
            mScrollVector = 0;
        }
    }
    if ((!noScroll) && (mScrollVector >= 0) && (mScrollVector <= mScreenHeight) && (!mForceUpdate)) {
        if (mScrollVector * mFontHeight < mScreenMap.height() && mScreenWidth * mFontWidth <= mScreenMap.width() && (mScreenHeight - mScrollVector) * mFontHeight > 0
            && (mScreenHeight - mScrollVector) * mFontHeight <= mScreenMap.height()) {
            screenPixmap = mScreenMap.copy(0, mScrollVector * mFontHeight * dpr, mScreenWidth * mFontWidth * dpr, (mScreenHeight - mScrollVector) * mFontHeight * dpr);
            p.drawPixmap(0, 0, screenPixmap);
            from = mScreenHeight - mScrollVector - 1;
        }
    } else if ((!noScroll) && (mScrollVector < 0 && mScrollVector >= ((-1) * mScreenHeight)) && (!mForceUpdate)) {
        if (abs(mScrollVector) * mFontHeight < mScreenMap.height() && mScreenWidth * mFontWidth <= mScreenMap.width() && (mScreenHeight - abs(mScrollVector)) * mFontHeight > 0
            && (mScreenHeight - abs(mScrollVector)) * mFontHeight <= mScreenMap.height()) {
            screenPixmap = mScreenMap.copy(0, 0, mScreenWidth * mFontWidth * dpr, (mScreenHeight - abs(mScrollVector)) * mFontHeight * dpr);
            p.drawPixmap(0, abs(mScrollVector) * mFontHeight, screenPixmap);
            from = 0;
            y_bottom = abs(mScrollVector);
        }
    }

    //delete non used characters.
    //needed for horizontal scrolling because there sometimes characters didn't get cleared
    QRect deleteRect = QRect(0, from * mFontHeight, x_right * mFontHeight, (y_bottom + 1) * mFontHeight);
    p.setCompositionMode(QPainter::CompositionMode_Source);
    p.fillRect(deleteRect, Qt::transparent);

    p.setCompositionMode(QPainter::CompositionMode_SourceOver);
    for (int i = from; i <= y_bottom; ++i) {
        if (static_cast<int>(mpBuffer->buffer.size()) <= i + lineOffset) {
            break;
        }
        drawLine(p, i + lineOffset, i, &mScreenOffset);
    }
    calculateHMaxRange();
    if (Q_UNLIKELY(mpConsole->mHScrollBarEnabled && mpConsole->mpHScrollBar)) {
        updateHorizontalScrollBar();
    }
    p.end();
    painter.setBackgroundMode(Qt::BGMode::TransparentMode);
    painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
    painter.drawPixmap(0, 0, pixmap);
    if (!noCopy) {
        mScreenMap = pixmap.copy();
    }
    mScrollVector = 0;
    mLastRenderedOffset = lineOffset;
    mForceUpdate = false;
}

void TTextEdit::paintEvent(QPaintEvent* e)
{
    const QRect& rect = e->rect();

    if (mFontWidth <= 0 || mFontHeight <= 0) {
        return;
    }

    if (mScreenHeight <= 0 || mScreenWidth <= 0) {
        mScreenHeight = height() / mFontHeight;
        mScreenWidth = 100;
        if (mScreenHeight <= 0) {
            return;
        }
        if (mpConsole->getType() == TConsole::MainConsole && !mIsLowerPane) {
            mpHost->setScreenDimensions(mScreenWidth, mScreenHeight);
        }
    }

    QPainter painter(this);
    if (!painter.isActive()) {
        return;
    }
    painter.setFont(font());
    drawForeground(painter, rect);
}

// highlights the currently selected text.
// mpA represents the first (zero-based) line/row (y) and position/column
// (x) that IS to be selected, mpB represents the last (zero-based) line and
// column that IS to be selected regardless of the way the user is selecting
// (top-down, bottom-up, left-right, right-left) - which means that in the
// traditional 'for' loop construct where the test is a '<' based one, the test
// limit is mpB.y() + 1 for the row and mpB.x() + 1 for the column:
void TTextEdit::highlightSelection()
{
    QRegion newRegion;

    int lineDelta = abs(mPA.y() - mPB.y()) - 1;
    if (lineDelta > 0) {
        QRect rectFirstLine(mPA.x() * mFontWidth, (mPA.y() - imageTopLine()) * mFontHeight, mScreenWidth * mFontWidth, mFontHeight);
        newRegion += rectFirstLine;

        QRect rectMiddlePart(0, (mPA.y() + 1 - imageTopLine()) * mFontHeight, mScreenWidth * mFontWidth, lineDelta * mFontHeight);
        newRegion += rectMiddlePart;

        QRect rectLastLine(0, (mPB.y() - imageTopLine()) * mFontHeight, mPB.x() * mFontWidth, mFontHeight);
        newRegion += rectLastLine;
    }

    if (lineDelta == 0) {
        QRect rectFirstLine(mPA.x() * mFontWidth, (mPA.y() - imageTopLine()) * mFontHeight, mScreenWidth * mFontWidth, mFontHeight);
        newRegion += rectFirstLine;

        QRect rectLastLine(0, (mPB.y() - imageTopLine()) * mFontHeight, mPB.x() * mFontWidth, mFontHeight);
        newRegion += rectLastLine;
    }

    if (lineDelta < 0) {
        QRect rectFirstLine(mPA.x() * mFontWidth, (mPA.y() - imageTopLine()) * mFontHeight, std::max(mPB.x() - mPA.x(), 1) * mFontWidth, mFontHeight);
        newRegion += rectFirstLine;
    }

    QRect _r = mSelectedRegion.boundingRect();
    if (lineDelta < 0) {
        _r.setWidth(mScreenWidth * mFontWidth);
    }
    update(_r);

    mSelectedRegion = mSelectedRegion.subtracted(newRegion);

    // clang-format off
    for (int y = std::max(0, mPA.y()),
             endY = std::min((mPB.y() + 1), static_cast<int>(mpBuffer->buffer.size()));
         y < endY;
         ++y) {

        for (int x = (y == mPA.y()) ? std::max(0, mPA.x()) : 0,
                 endX = (y == (mPB.y()))
                     ? std::min((mPB.x() + 1), static_cast<int>(mpBuffer->buffer.at(y).size()))
                     : static_cast<int>(mpBuffer->buffer.at(y).size());
             x < endX;
             ++x) {

            mpBuffer->buffer.at(y).at(x).select();
        }
    }
    // clang-format on

    update(mSelectedRegion.boundingRect());
    update(newRegion);
    mSelectedRegion = newRegion;

    QClipboard* clipboard = QApplication::clipboard();
    if (clipboard->supportsSelection()) {
        // X11 has a second clipboard that's updated on any selection
        clipboard->setText(getSelectedText(QChar::LineFeed), QClipboard::Selection);
    }

    if (QAccessible::isActive()) {
        QAccessibleTextSelectionEvent event(this, offsetForPosition(mPA.y(), mPA.x()), offsetForPosition(mPB.y(), mPB.x()));
        QAccessible::updateAccessibility(&event);
    }
}

void TTextEdit::unHighlight()
{
    for (int yIndex = mPA.y(), total = mPB.y(); yIndex <= total; ++yIndex) {
        if (yIndex >= static_cast<int>(mpBuffer->buffer.size()) || yIndex < 0) {
            // Abort if we are considering a line not in the buffer:
            break;
        }

        auto& bufferLine = mpBuffer->buffer.at(yIndex);
        for (int xIndex = 0; xIndex < static_cast<int>(bufferLine.size()); ++xIndex) {
            if (bufferLine.at(xIndex).isSelected()) {
                bufferLine[xIndex].deselect();
            }
        }
    }
    if (QAccessible::isActive()) {
        QAccessibleTextSelectionEvent event(this, -1, -1);
        QAccessible::updateAccessibility(&event);
    }
}

// ensure that mPA is top-left and mPB is bottom-right
void TTextEdit::normaliseSelection()
{
    if (mDragStart.y() < mDragSelectionEnd.y() || ((mDragStart.y() == mDragSelectionEnd.y()) && (mDragStart.x() < mDragSelectionEnd.x()))) {
        mPA = mDragStart;
        mPB = mDragSelectionEnd;
    } else {
        mPA = mDragSelectionEnd;
        mPB = mDragStart;
    }
}

void TTextEdit::expandSelectionToWords()
{
    int yind = mPA.y();
    int xind = mPA.x();

    // Check if yind is within the valid range of lineBuffer
    if (yind >= 0 && yind < static_cast<int>(mpBuffer->lineBuffer.size())) {
        for (; xind >= 0; --xind) {
            // Ensure xind is within the valid range for the current line
            if (xind >= static_cast<int>(mpBuffer->lineBuffer.at(yind).size())) {
                break; // xind is out of bounds, break the loop
            }
            const QChar currentChar = mpBuffer->lineBuffer.at(yind).at(xind);
            if (currentChar == QChar::Space
                || mpHost->mDoubleClickIgnore.contains(currentChar)) {
                break;
            }
        }
    }
    mDragStart.setX(xind + 1);
    mPA.setX(xind + 1);

    yind = mPB.y();
    xind = mPB.x();

    // Repeat the check for yind and xind for the second part
    if (yind >= 0 && yind < static_cast<int>(mpBuffer->lineBuffer.size())) {
        for (; xind < static_cast<int>(mpBuffer->lineBuffer.at(yind).size()); ++xind) {
            const QChar currentChar = mpBuffer->lineBuffer.at(yind).at(xind);
            if (currentChar == QChar::Space
                || mpHost->mDoubleClickIgnore.contains(currentChar)) {
                break;
            }
        }
    }
    mDragSelectionEnd.setX(xind - 1);
    mPB.setX(xind - 1);
}



void TTextEdit::expandSelectionToLine(int y)
{
    if (!(y < mpBuffer->lineBuffer.size())) {
        return;
    }
    unHighlight();
    mDragStart.setX(0);
    mDragStart.setY(y);
    mDragSelectionEnd.setX(mpBuffer->buffer[y].size());
    mDragSelectionEnd.setY(y);
    normaliseSelection();
    highlightSelection();
    mMouseTracking = true;
}


void TTextEdit::mouseMoveEvent(QMouseEvent* event)
{
    if (mFontWidth == 0 || mFontHeight == 0) {
        return;
    }

    bool isOutOfbounds = false;
    auto eventPos = event->position().toPoint();
    int lineIndex = std::max(0, (eventPos.y() / mFontHeight) + imageTopLine());
    int tCharIndex = convertMouseXToBufferX(eventPos.x(), lineIndex, &isOutOfbounds);

    updateTextCursor(event, lineIndex, tCharIndex, isOutOfbounds);

    if (!mMouseTracking) {
        return;
    }

    if (eventPos.y() < 10) {
        mpConsole->scrollUp(3);
    }
    if (eventPos.y() >= height() - 10) {
        mpConsole->scrollDown(3);
    }

    if (eventPos.x() < 10) {
        scrollH(std::max(0, mCursorX - 2));
    }
    if (eventPos.x() >= width() - 10) {
        scrollH(std::min(mMaxHRange, mCursorX + 2));
    }

    if (lineIndex > static_cast<int>(mpBuffer->size() - 1)) {
        return;
    }

    QPoint cursorLocation(tCharIndex, lineIndex);

    if ((mDragSelectionEnd.y() < cursorLocation.y() || (mDragSelectionEnd.y() == cursorLocation.y() && mDragSelectionEnd.x() < cursorLocation.x()))) {
        mPA = mDragSelectionEnd;
        mPB = cursorLocation;
    } else {
        mPA = cursorLocation;
        mPB = mDragSelectionEnd;
    }

    for (int yIndex = mPA.y(), total = mPB.y(); yIndex <= total; ++yIndex) {
        if (yIndex >= static_cast<int>(mpBuffer->buffer.size()) || yIndex < 0) {
            // Abort if we are considering a line not in the buffer:
            break;
        }

        auto& bufferLine = mpBuffer->buffer.at(yIndex);
        for (int xIndex = 0; xIndex < static_cast<int>(bufferLine.size()); ++xIndex) {
            if (bufferLine.at(xIndex).isSelected()) {
                bufferLine[xIndex].deselect();
            }
        }
    }

    if (mDragStart.y() < cursorLocation.y() || (mDragStart.y() == cursorLocation.y() && mDragStart.x() < cursorLocation.x())) {
        mPA = mDragStart;
        mPB = cursorLocation;
    } else {
        mPA = cursorLocation;
        mPB = mDragStart;
    }
    if (mMouseTrackLevel == 2) {
        expandSelectionToWords();
    }
    if (mCtrlSelecting || mMouseTrackLevel == 3) {
        mPA.setX(0);
        mPB.setX(mpBuffer->buffer.at(mPB.y()).size());
    }

    highlightSelection();
    mDragSelectionEnd = cursorLocation;
    forceUpdate();
}

void TTextEdit::updateTextCursor(const QMouseEvent* event, int lineIndex, int tCharIndex, bool isOutOfbounds)
{
    if (lineIndex < static_cast<int>(mpBuffer->buffer.size())) {
        if (tCharIndex < static_cast<int>(mpBuffer->buffer[lineIndex].size())) {
            if (mpBuffer->buffer.at(lineIndex).at(tCharIndex).linkIndex() && !isOutOfbounds) {
                int linkIndex = mpBuffer->buffer.at(lineIndex).at(tCharIndex).linkIndex();

                setCursor(Qt::PointingHandCursor);
                QStringList tooltip = mpBuffer->mLinkStore.getHints(linkIndex);
                QStringList commands = mpBuffer->mLinkStore.getLinks(linkIndex);
                // If a special tooltip hint was given, use that one.
                QToolTip::showText(event->globalPosition().toPoint(), tooltip.size() > commands.size() ? tooltip[0] : tooltip.join(QChar::LineFeed));

                // Update hover state for CSS pseudo-class support
                if (mpBuffer->getHoveredLink() != linkIndex) {
                    mpBuffer->setHoveredLink(linkIndex);
                    forceUpdate(); // Trigger re-render with new hover state
                }
            } else {
                setCursor(Qt::IBeamCursor);
                QToolTip::hideText();

                // Clear hover state if we're not over a link
                if (mpBuffer->getHoveredLink() != 0) {
                    mpBuffer->setHoveredLink(0);
                    forceUpdate(); // Trigger re-render
                }
            }
        }
    }
}

// Returns the index into the relevant TBuffer::lineBuffer of the FIRST QChar
// of the grapheme under the mouse - it ALSO returns zero (which will probably
// NOT be a valid index) if there is no valid index to return.
// If a pointer to a boolean is provided as a third argument then it will
// be set to true if the mouse is positioned over a visible time stamp
// and left unchanged otherwise.
int TTextEdit::convertMouseXToBufferX(const int mouseX, const int lineNumber, bool* isOutOfbounds, bool* isOverTimeStamp) const
{
    if (lineNumber >= 0 && lineNumber < mpBuffer->lineBuffer.size()) {
        // Line number is (should be) within range of lines in the
        // TBuffer::lineBuffer - might need to check that this still works after
        // that buffer has reached the limit when it starts to have the
        // beginning lines deleted!

        // offset will only have a value for errorwindows if they use the horizontal scrollbar (for now)
        int offset = mCursorX * mFontWidth;
        // Count of "normal" width equivalent characters - we will multiply that
        // by the average character width to determine whether the mouse is over
        // a particular grapheme:
        int column = 0;
        // These are the calculated horizontal limits in pixels from the left
        // for the current grapheme being considered in the line:
        int leftX = 0;
        int rightX = 0;
        QString lineText = mpBuffer->lineBuffer.at(lineNumber);
        // QStringList debugText;
        QTextBoundaryFinder boundaryFinder(QTextBoundaryFinder::Grapheme, lineText);
        int indexOfLastChar = 0;
        for (int indexOfChar = 0, total = lineText.size(); indexOfChar < total;) {
            int nextBoundary = boundaryFinder.toNextBoundary();
            // Width in "normal" width characters equivalent of this grapheme:
            int charWidth = 0;
            // This could contain a surrogate pair (i.e. pair of QChars) and/or
            // include suffixed combining diacritical marks (additional QChars):
            const QString grapheme = lineText.mid(indexOfChar, nextBoundary - indexOfChar);
            const uint unicode = graphemeInfo::getBaseCharacter(grapheme);
            if (unicode == '\t') {
                charWidth = mTabStopwidth - (column % mTabStopwidth);
            } else {
                auto reportedWidth = getGraphemeWidth(unicode);
                // The paint code is set to use a replacement character for a
                // zero return value - so handle the space that will need)
                charWidth = (reportedWidth ? reportedWidth : 1);
            }
            column += charWidth;

            // Do an additional check if we need to establish whether we are
            // over just the timestamp part of the line:
            if (Q_UNLIKELY(isOverTimeStamp && mpConsole->showTimeStamps() && indexOfChar == 0)) {
                if ((mouseX + offset) < (mudlet::smTimeStampFormat.size() * mFontWidth)) {
                    // The mouse position is actually over the timestamp region
                    // to the left of the main text:
                    *isOverTimeStamp = true;
                }
            }

            leftX = rightX;
            //mCursorX relevant for horizontal scrollbars
            //Otherwise the value is always 0
            if (mpConsole->showTimeStamps()) {
                rightX = (mudlet::smTimeStampFormat.size() + column - mCursorX) * mFontWidth;
            } else {
                rightX = (column - mCursorX) * mFontWidth;
            }

            // Format of display "[index of FIRST QChar in grapheme|leftX]grapheme[rightX|index of LAST QChar in grapheme (may be same as FIRST)]" ...
            // debugText << qsl("[%1|%2]%3[%4|%5]").arg(QString::number(indexOfChar), QString::number(leftX), grapheme, QString::number(rightX - 1), QString::number(nextBoundary - 1));
            if (leftX <= mouseX && mouseX < rightX) {
                // qDebug().nospace().noquote() << "TTextEdit::convertMouseXToBufferX(" << mouseX << ", " << lineNumber << ") INFO - returning: " << std::max(0, indexOfChar) << " reckoning cursor is over the last grapheme within the calculated limits of:\n" << debugText.join(QString());
                return std::max(0, indexOfChar);
            }
            if (nextBoundary >= 0) {
                // nextBoundary will be -1 at end of line and we do not want THAT:
                indexOfLastChar = indexOfChar;
            }
            indexOfChar = nextBoundary;
        }

        //        qDebug().nospace().noquote() << "TTextEdit::convertMouseXToBufferX(" << mouseX << ", " << lineNumber << ") INFO - falling out of bottom of for loop and returning: " << indexOfLastChar << " !";
        *isOutOfbounds = true;
        return std::max(0, indexOfLastChar);
    }

    return 0;
}

void TTextEdit::contextMenuEvent(QContextMenuEvent* event)
{
    event->accept();
}

void TTextEdit::slot_popupMenu()
{
    auto* pA = qobject_cast<QAction*>(sender());
    if (!pA || !mpHost) {
        return;
    }
    // index is set to be greater than zero for every possible sender():
    const int index = pA->data().toInt();
    if (index && mPopupCommands.contains(index)) {
        const QString cmd = mPopupCommands.value(index).first;
        const int luaReference = mPopupCommands.value(index).second;
        if (!luaReference) {
            mpHost->mLuaInterpreter.compileAndExecuteScript(cmd);
        } else {
            mpHost->mLuaInterpreter.callAnonymousFunction(luaReference, qsl("echoPopup"));
        }
    }
}

void TTextEdit::mousePressEvent(QMouseEvent* event)
{
    //new event to get mouse position on the parent window
    auto eventPos = event->position().toPoint();
    auto eventGlobalPos = event->globalPosition().toPoint();
    QMouseEvent newEvent(event->type(), mpConsole->parentWidget()->mapFromGlobal(eventGlobalPos), eventGlobalPos, event->button(), event->buttons(), event->modifiers());
    if (mpConsole->getType() == TConsole::SubConsole) {
        qApp->sendEvent(mpConsole->parentWidget(), &newEvent);
    }

    if (mpConsole->getType() == TConsole::MainConsole || mpConsole->getType() == TConsole::UserWindow) {
        mpConsole->raiseMudletMousePressOrReleaseEvent(&newEvent, true);
    }

    if (event->button() == Qt::LeftButton) {
        int y = (eventPos.y() / mFontHeight) + imageTopLine();
        int x = 0;
        y = std::max(y, 0);

        if (event->modifiers() & Qt::ControlModifier) {
            mCtrlSelecting = true;
        }

        bool isOutOfbounds = false;
        if (!mCtrlSelecting && mpConsole->showTimeStamps()) {
            bool isOverTimeStamp = false;
            x = convertMouseXToBufferX(eventPos.x(), y, &isOutOfbounds, &isOverTimeStamp);
            if (isOverTimeStamp) {
                // If we have clicked on the timestamp then emulate the effect
                // of control clicking - i.e. select the WHOLE line:
                mCtrlSelecting = true;
            }
        } else {
            x = convertMouseXToBufferX(eventPos.x(), y, &isOutOfbounds);
        }

        if (mCtrlSelecting) {
            expandSelectionToLine(y);
            highlightSelection();
            event->accept();
            return;
        }

        if (y < static_cast<int>(mpBuffer->buffer.size())) {
            if (x < static_cast<int>(mpBuffer->buffer[y].size()) && !isOutOfbounds) {
                if (mpBuffer->buffer.at(y).at(x).linkIndex()) {
                    int linkIndex = mpBuffer->buffer.at(y).at(x).linkIndex();
                    QStringList command = mpBuffer->mLinkStore.getLinks(linkIndex);
                    int luaReference = mpBuffer->mLinkStore.getReference(linkIndex).value(0, false);
                    QString func;
                    if (!command.empty() && mpHost) {
                        func = command.at(0);

                        // Set active state for CSS pseudo-class support
                        mpBuffer->setActiveLink(linkIndex);
                        forceUpdate(); // Trigger re-render with active state

                        if (!luaReference) {
                            mpHost->mLuaInterpreter.compileAndExecuteScript(func);
                        } else {
                            mpHost->mLuaInterpreter.callAnonymousFunction(luaReference, qsl("echoLink"));
                        }

                        // Mark link as visited after execution (will update to visited state)
                        mpBuffer->markLinkAsVisited(linkIndex);

                        return;
                    }
                }
            }
        }
        unHighlight();
        // Ensure BOTH panes are updated if the lower one is showing
        if (mIsLowerPane) {
            // We wouldn't be getting to here if the lower pane was not visible:
            mpConsole->mUpperPane->forceUpdate();
            forceUpdate();
        } else if (!mIsTailMode) {
            // Not in tail mode means the lower pane is also showing (and we are the
            // upper one) - so update both:
            forceUpdate();
            mpConsole->mLowerPane->forceUpdate();
        } else {
            // We are the upper pane and the lower one is not showing
            forceUpdate();
        }
        mSelectedRegion = QRegion(0, 0, 0, 0);
        if (mLastClickTimer.elapsed() < 300) {
            mMouseTracking = true;
            mMouseTrackLevel++;
            if (mMouseTrackLevel > 3) {
                mMouseTrackLevel = 3;
            }

            if (mMouseTrackLevel == 3) {
                expandSelectionToLine(y);
                event->accept();
                return;
            }

            if (y >= mpBuffer->lineBuffer.size()) {
                return;
            }
            if (x >= mpBuffer->lineBuffer[y].size()) {
                return;
            }

            mDragStart.setX(x);
            mDragStart.setY(y);
            mDragSelectionEnd.setX(x);
            mDragSelectionEnd.setY(y);
            normaliseSelection();
            if (mMouseTrackLevel == 2) {
                expandSelectionToWords();
            } else {
                mPA.setX(0);
                mPB.setX(mpBuffer->buffer.at(mPB.y()).size());
            }
            mLastClickTimer.start();
            highlightSelection();
            event->accept();
            return;
        }
        mLastClickTimer.start();
        mMouseTracking = true;
        mMouseTrackLevel = 1;
        if (y >= mpBuffer->size()) {
            return;
        }
        mDragStart.setX(x);
        mDragStart.setY(y);
        mDragSelectionEnd = mDragStart;
        event->accept();
        return;
    }


    if (event->button() == Qt::MiddleButton) {
        mpConsole->clearSplit();
        event->accept();
        return;
    }
    QWidget::mousePressEvent(event);
}

void TTextEdit::slot_selectAll()
{
    mDragStart = QPoint(0, 0);
    mDragSelectionEnd = mpBuffer->getEndPos();
    // need this to convert the above to mPA and mPB:
    normaliseSelection();

    highlightSelection();
    // Ensure BOTH panes are updated if the lower one is showing
    if (mIsLowerPane) {
        // We wouldn't be getting to here if the lower pane was not visible:
        mpConsole->mUpperPane->forceUpdate();
        forceUpdate();
    } else if (!mIsTailMode) {
        // Not in tail mode means the lower pane is also showing (and we are the
        // upper one) - so update both:
        forceUpdate();
        mpConsole->mLowerPane->forceUpdate();
    } else {
        // We are the upper pane and the lower one is not showing
        forceUpdate();
    }
}

void TTextEdit::slot_searchSelectionOnline()
{
    searchSelectionOnline();
}


void TTextEdit::slot_copySelectionToClipboard()
{
    if (!establishSelectedText()) {
        return;
    }

    QString selectedText = getSelectedText(QChar::LineFeed);
    QClipboard* clipboard = QApplication::clipboard();
    clipboard->setText(selectedText);
}

void TTextEdit::slot_copySelectionToClipboardHTML()
{
    if (!establishSelectedText() || !mpHost) {
        return;
    }

    QString title;
    if (mpConsole->getType() == TConsole::CentralDebugConsole) {
        title = tr("Mudlet, debug console extract");
    } else if (mpConsole->getType() == TConsole::SubConsole) {
        title = tr("Mudlet, %1 mini-console extract from %2 profile").arg(mpHost->mpConsole->mSubConsoleMap.key(mpConsole), mpHost->getName());
    } else if (mpConsole->getType() == TConsole::UserWindow) {
        title = tr("Mudlet, %1 user window extract from %2 profile").arg(mpHost->mpConsole->mSubConsoleMap.key(mpConsole), mpHost->getName());
    } else {
        title = tr("Mudlet, main console extract from %1 profile").arg(mpHost->getName());
    }

    QStringList fontsList;                  // List of fonts to become the font-family entry for
                                            // the master css in the header
    fontsList << this->fontInfo().family(); // Seems to be the best way to get the
    // font in use, as different TConsole
    // instances within the same profile
    // might have different fonts in future,
    // and although the font is settable for
    // the main profile window, it is not yet
    // for user miniConsoles, or the Debug one
    fontsList << qsl("Courier New");
    fontsList << qsl("Monospace");
    fontsList << qsl("Courier");
    fontsList.removeDuplicates(); // In case the actual one is one of the defaults here

    QString text = "<!DOCTYPE HTML PUBLIC '-//W3C//DTD HTML 4.01//EN' 'http://www.w3.org/TR/html4/strict.dtd'>\n";
    text.append("<html>\n");
    text.append(" <head>\n");
    text.append("  <meta http-equiv='content-type' content='text/html; charset=utf-8'>");
    // put the charset as early as possible as the parser MUST restart when it
    // switches away from the ASCII default
    text.append("  <meta name='generator' content='Mudlet MUD Client version: ");
    text.append(APP_VERSION);
    text.append(mudlet::self()->mAppBuild);
    text.append("'>\n");
    // Nice to identify what made the file!
    text.append("  <title>");
    text.append(title);
    text.append("</title>\n");
    // Web-page title
    text.append("  <style type='text/css'>\n");
    text.append("   <!-- body { font-family: '");
    text.append(fontsList.join("', '"));
    text.append("'; font-size: 100%; line-height: 1.125em; white-space: nowrap; color:rgb(255,255,255); background-color:rgb(");
    // Line height, I think, should be equal to 18 point for a 16 point default
    // font size by default but this seems to work even when the size is not 16
    // Use a "%age" for a IE compatible font size, 16 point is the default for
    // web-pages, but 14 seems to produce a more reasonable size but that could
    // just be the browsers I tested it on! - Slysven
    text.append(QString::number(mpHost->mBgColor.red()));
    text.append(",");
    text.append(QString::number(mpHost->mBgColor.green()));
    text.append(",");
    text.append(QString::number(mpHost->mBgColor.blue()));
    text.append(");}\n");
    text.append("        span { white-space: pre-wrap; } -->\n");
    text.append("  </style>\n");
    text.append("  </head>\n");
    text.append("  <body><div>");
    // <div></div> tags required around outside of the body <span></spans> for
    // strict HTML 4 as we do not use <p></p>s or anything else

    // Is this a single line then we do NOT need to pad the first (and thus
    // only) line to the right:
    bool isSingleLine = (mDragStart.y() == mDragSelectionEnd.y());
    for (int y = mPA.y(), total = mPB.y(); y <= total; ++y) {
        if (y >= static_cast<int>(mpBuffer->buffer.size())) {
            return;
        }
        if (y == mPA.y()) { // First line of selection
            if (isSingleLine) {
                text.append(mpBuffer->bufferToHtml(mpConsole->showTimeStamps(), y, mPB.x() + 1, mPA.x(), 0));
            } else { // Not single line
                text.append(mpBuffer->bufferToHtml(mpConsole->showTimeStamps(), y, -1, mPA.x(), mPA.x()));
            }
        } else if (y == mPB.y()) { // Last line of selection
            text.append(mpBuffer->bufferToHtml(mpConsole->showTimeStamps(), y, mPB.x() + 1));
        } else { // inside lines of selection
            text.append(mpBuffer->bufferToHtml(mpConsole->showTimeStamps(), y));
        }
    }
    text.append(qsl(" </div></body>\n"
                    "</html>"));
    // The last two of these tags were missing and meant the HTML was not terminated properly
    QClipboard* clipboard = QApplication::clipboard();
    clipboard->setText(text);
    mSelectedRegion = QRegion(0, 0, 0, 0);
    forceUpdate();
}

bool TTextEdit::establishSelectedText()
{
    if (mpBuffer->lineBuffer.isEmpty()) {
        // Prevent problems with trying to do a copy when TBuffer is empty:
        return false;
    }

    // if selection was made backwards swap
    // right to left
    if (mFontWidth <= 0 || mFontHeight <= 0) {
        return false;
    }

    if (mSelectedRegion == QRegion(0, 0, 0, 0)) {
        return false;
    }

    if (mScreenHeight <= 0 || mScreenWidth <= 0) {
        mScreenHeight = height() / mFontHeight;
        mScreenWidth = 100;
        if (mScreenHeight <= 0) {
            return false;
        }
        if (mpConsole->getType() == TConsole::MainConsole && !mIsLowerPane) {
            mpHost->setScreenDimensions(mScreenWidth, mScreenHeight);
        }
    }

    normaliseSelection();
    if (mMouseTrackLevel == 2) {
        expandSelectionToWords();
    }
    return true;
}

// Technically this copies whole lines into the image even if the selection does
// not start at the beginning of the first line or end at the last grapheme on
// the last line.
void TTextEdit::slot_copySelectionToClipboardImage()
{
    mCopyImageStartTime = std::chrono::high_resolution_clock::now();

    if (!establishSelectedText()) {
        return;
    }

    // Qt says: "Maximum supported image dimension is 65500 pixels" in stdout
    auto heightpx = std::min(65500, (mPB.y() - mPA.y() + 1) * mFontHeight);
    auto lineOffset = mPA.y();

    // find the biggest width of text we need to work with
    int largestLine{};
    for (int y = mPA.y(), total = mPB.y() + 1; y < total; ++y) {
        const QString lineText{mpBuffer->lineBuffer.at(y)};
        // Will accumulate the width in pixels of the current line:
        auto lineWidth{(mpConsole->showTimeStamps() ? mudlet::smTimeStampFormat.size() : 0) * mFontWidth};
        // Accumulated width in "normal" width characters:
        int column{};
        QTextBoundaryFinder boundaryFinder(QTextBoundaryFinder::Grapheme, lineText);
        for (qsizetype indexOfChar{}, lineLength{lineText.size()}; indexOfChar < lineLength;) {
            auto nextBoundary{boundaryFinder.toNextBoundary()};
            // Width in "normal" width equivalent of this grapheme:
            int charWidth{};
            const QString grapheme = lineText.mid(indexOfChar, nextBoundary - indexOfChar);
            const uint unicode = graphemeInfo::getBaseCharacter(grapheme);
            if (unicode == '\t') {
                charWidth = mTabStopwidth - (column % mTabStopwidth);
            } else {
                auto reportedWidth = getGraphemeWidth(unicode);
                // The paint code is set to use a replacement character for a
                // zero return value - so handle the space that will need)
                charWidth = (reportedWidth ? reportedWidth : 1);
            }
            column += charWidth;
            // The timestamp is (currently) 13 "normal width" characters
            // but that might not always be the case in some future I18n
            // situations:
            lineWidth = (mpConsole->showTimeStamps() ? mudlet::smTimeStampFormat.size() + column : column) * mFontWidth;
            indexOfChar = nextBoundary;
        }
        largestLine = std::max(static_cast<int>(lineWidth), largestLine);
    }

    auto widthpx = std::min(65500, largestLine);
    auto rect = QRect(mPA.x(), mPA.y(), widthpx, heightpx);
    auto pixmap = QPixmap(widthpx, heightpx);
    auto solidColor = QColor(mBgColor);
    solidColor.setAlpha(255);
    pixmap.fill(solidColor);

    QPainter painter(&pixmap);
    if (!painter.isActive()) {
        return;
    }

    // deselect to prevent inverted colours in image
    unHighlight();
    mSelectedRegion = QRegion(0, 0, 0, 0);

    auto result = drawTextForClipboard(painter, rect, lineOffset);

    highlightSelection();

    // if we cut didn't finish painting the complete picture, trim the bottom of the image
    if (!result.first) {
        const auto& smallerPixmap = pixmap.scaled(QSize(widthpx, result.second * mFontHeight), Qt::KeepAspectRatio);
        QApplication::clipboard()->setImage(smallerPixmap.toImage());
        return;
    }

    QApplication::clipboard()->setImage(pixmap.toImage());
}

// a stateless version of drawForeground that doesn't do any caching
// (and thus doesn't mess up any of the caches)
std::pair<bool, int> TTextEdit::drawTextForClipboard(QPainter& painter, QRect rectangle, int lineOffset) const
{
    painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
    painter.setFont(font());

    int lineCount = rectangle.height() / mFontHeight;
    int linesDrawn = 0;
    auto timeout = mudlet::self()->mCopyAsImageTimeout;
    for (int i = 0; i <= lineCount; i++, linesDrawn++) {
        if (static_cast<int>(mpBuffer->buffer.size()) <= i + lineOffset) {
            break;
        }
        drawLine(painter, i + lineOffset, i);

        if (std::chrono::duration_cast<std::chrono::seconds>(std::chrono::high_resolution_clock::now() - mCopyImageStartTime).count() >= timeout) {
            qDebug().nospace() << "timeout for image copy (" << timeout << "s) reached, managed to draw " << i << " lines";
            return {false, linesDrawn};
        }
    }
    return {true, linesDrawn};
}

void TTextEdit::searchSelectionOnline()
{
    if (!establishSelectedText() || !mpHost) {
        return;
    }

    QString selectedText = getSelectedText(QChar::Space);
    QString url = QUrl::toPercentEncoding(selectedText.trimmed());
    url.prepend(mpHost->getSearchEngine().second);
    QDesktopServices::openUrl(QUrl(url));
}

void TTextEdit::slot_copySelectionToSearchBar()
{
    if (!establishSelectedText()) {
        return;
    }

    QString selectedText = getSelectedText(QChar::LineFeed).trimmed();

    if (mudlet::self()->dactionInputLine->isChecked()) {
        // If hidden then reveal as if pressed Alt-L
        mudlet::self()->dactionInputLine->setChecked(false);
        mudlet::self()->mpCurrentActiveHost->setCompactInputLine(false);
    }
    mpConsole->mpBufferSearchBox->setText(selectedText);
    mpConsole->mpBufferSearchBox->setFocus();
    mpConsole->mpBufferSearchBox->selectAll();
}

QString TTextEdit::getSelectedText(const QChar& newlineChar, const bool showTimestamps)
{
    // mPA QPoint where selection started
    // mPB QPoint where selection ended
    // try to prevent crash if buffer is batch deleted
    if (mPA.y() > mpBuffer->lineBuffer.size() - 1 || mPB.y() > mpBuffer->lineBuffer.size() - 1) {
        mPA.ry() -= mpBuffer->mBatchDeleteSize;
        mPB.ry() -= mpBuffer->mBatchDeleteSize;
    }
    qsizetype startLine = std::max(0, mPA.y());
    qsizetype endLine = std::min<qsizetype>(mPB.y(), (mpBuffer->lineBuffer.size() - 1));
    qsizetype offset = endLine - startLine;
    qsizetype startPos = std::max(0, mPA.x());
    qsizetype endPos = std::min<qsizetype>(mPB.x(), (mpBuffer->lineBuffer.at(endLine).size() - 1));
    QStringList textLines = mpBuffer->lineBuffer.mid(startLine, endLine - startLine + 1);
    if (textLines.isEmpty()) {
        return {};
    }

    if (mPA.y() == mPB.y()) {
        // Is a single line, so trim characters off the beginning and end
        // according to startPos and endPos:
        if (!textLines.at(0).isEmpty()) {
            textLines[0] = textLines.at(0).mid(startPos, endPos - startPos + 1);
        }
    } else {
        // replace a number of QChars at the front with a corresponding
        // number of spaces to push the first line to the right so it lines up
        // with the following lines:
        if (!textLines.at(0).isEmpty()) {
            textLines[0] = textLines.at(0).mid(startPos);
            textLines[0] = QString(QChar::Space).repeated(startPos) % textLines.at(0);
        }
        // and chop off the required number of QChars from the end of the last
        // line:
        if (!textLines.at(offset).isEmpty()) {
            textLines[offset] = textLines.at(offset).left(1 + endPos);
        }
    }

    if (showTimestamps) {
        QStringList timestamps = mpBuffer->timeBuffer.mid(startLine, endLine - startLine + 1);
        QStringList result;
        std::transform(textLines.cbegin(), textLines.cend(), timestamps.cbegin(), std::back_inserter(result),
                               [](const QString& text, const QString& timestamp) { return timestamp + text; });
        textLines = result;
    }

    return textLines.join(newlineChar);
}

void TTextEdit::mouseReleaseEvent(QMouseEvent* event)
{
    auto eventPos = event->position().toPoint();
    auto eventGlobalPos = event->globalPosition().toPoint();
    if (event->button() == Qt::LeftButton) {
        mMouseTracking = false;
        mCtrlSelecting = false;

        // Clear active state on mouse release
        if (mpBuffer->getActiveLink() != 0) {
            mpBuffer->setActiveLink(0);
            forceUpdate(); // Trigger re-render
        }
    }
    if (event->button() == Qt::RightButton) {
        int y = (eventPos.y() / mFontHeight) + imageTopLine();
        y = std::max(y, 0);
        bool isOutOfbounds = false;
        int x = convertMouseXToBufferX(eventPos.x(), y, &isOutOfbounds);

        if (y < static_cast<int>(mpBuffer->buffer.size())) {
            if (x < static_cast<int>(mpBuffer->buffer.at(static_cast<size_t>(y)).size()) && !isOutOfbounds) {
                if (mpBuffer->buffer.at(static_cast<size_t>(y)).at(static_cast<size_t>(x)).linkIndex()) {
                    QStringList command = mpBuffer->mLinkStore.getLinks(mpBuffer->buffer.at(static_cast<size_t>(y)).at(static_cast<size_t>(x)).linkIndex());
                    QStringList hint = mpBuffer->mLinkStore.getHints(mpBuffer->buffer.at(static_cast<size_t>(y)).at(static_cast<size_t>(x)).linkIndex());
                    QVector<int> luaReference = mpBuffer->mLinkStore.getReference(mpBuffer->buffer.at(static_cast<size_t>(y)).at(static_cast<size_t>(x)).linkIndex());
                    if (command.size() > 1) {
                        // This is a popup menu rather than a link as it has
                        // more than one item.

                        // Skip a special tooltip hint (at the start of the
                        // hints), if one was given, i.e. there is (at least)
                        // an extra one:
                        int hint_offset = (hint.size() > command.size()) ? 1 : 0;

                        auto popup = new QMenu(this);
                        popup->setAttribute(Qt::WA_DeleteOnClose);
                        mPopupCommands.clear();
                        for (int i = 0, total = command.size(); i < total; ++i) {
                            QAction* pA = nullptr;
                            // Check to see if this item has a command/function
                            // so we can disable it if not:
                            const bool doesSomething = !command.at(i).isEmpty() || luaReference.value(i, 0);
                            // A safety flag in case we have too few hints:
                            const bool useHintNotCommand = (i + hint_offset) < hint.size();
                            // If it doesn't have a hint either then make it
                            // into a separator in the context menu:
                            const bool makeASeparator = !doesSomething && hint.at(i + hint_offset).isEmpty();
                            const QString actionText = useHintNotCommand ? hint.at(i + hint_offset) : command.at(i);
                            if (makeASeparator) {
                                pA = popup->addSeparator();
                            } else {
                                pA = popup->addAction(actionText);
                            }
                            mPopupCommands[i + 1] = {command.at(i), luaReference.value(i, 0)};
                            // We now use this to index into mPopupCommands
                            // when the action is triggered - but we do offset
                            // it by one so that the first one is NOT zero:
                            pA->setData(i + 1);
                            if (doesSomething) {
                                connect(pA, &QAction::triggered, this, &TTextEdit::slot_popupMenu);
                            } else {
                                pA->setEnabled(false);
                            }
                        }
                        popup->popup(eventGlobalPos);
                    }
                    mIsCommandPopup = true;
                    return;
                }
            }
        }
        mIsCommandPopup = false;


        QAction* action = new QAction(tr("Copy"), this);
        // According to the Qt Documentation:
        // "This text is used for the tooltip."
        // "If no tooltip is specified, the action's text is used."
        // "By default, this property contains the action's text."
        // So it seems that if we turn on tooltips (for all QAction) on a menu
        // (with QMenu::setToolTipsVisible(true)) we should forcible clear
        // the tooltip contents which are presumable filled with the default
        // in the QAction constructor:
        action->setToolTip(QString());
        connect(action, &QAction::triggered, this, &TTextEdit::slot_copySelectionToClipboard);
        QAction* action2 = new QAction(tr("Copy HTML"), this);
        action2->setToolTip(QString());
        connect(action2, &QAction::triggered, this, &TTextEdit::slot_copySelectionToClipboardHTML);

        auto* actionCopyImage = new QAction(tr("Copy as image"), this);
        connect(actionCopyImage, &QAction::triggered, this, &TTextEdit::slot_copySelectionToClipboardImage);

        QAction* action3 = new QAction(tr("Select all"), this);
        action3->setToolTip(QString());
        connect(action3, &QAction::triggered, this, &TTextEdit::slot_selectAll);

        QString selectedEngine = mpHost ? mpHost->getSearchEngine().first : tr("Unknown");
        QAction* action4 = new QAction(tr("Search on %1").arg(selectedEngine), this);
        action4->setToolTip(QString());
        connect(action4, &QAction::triggered, this, &TTextEdit::slot_searchSelectionOnline);
        if (!qApp->testAttribute(Qt::AA_DontShowIconsInMenus)) {
            action->setIcon(QIcon::fromTheme(qsl("edit-copy"), QIcon(qsl(":/icons/edit-copy.png"))));
            action3->setIcon(QIcon::fromTheme(qsl("edit-select-all"), QIcon(qsl(":/icons/edit-select-all.png"))));
            action4->setIcon(QIcon::fromTheme(qsl("edit-web-search"), QIcon(qsl(":/icons/edit-web-search.png"))));
        }

        auto popup = new QMenu(this);
        popup->setAttribute(Qt::WA_DeleteOnClose);
        popup->setToolTipsVisible(true); // Not the default...
        popup->addAction(action);
        popup->addAction(action2);
        popup->addAction(actionCopyImage);
        popup->addSeparator();
        popup->addAction(action3);

        if (mDragStart != mDragSelectionEnd && mpHost->mEnableTextAnalyzer) {
            mpContextMenuAnalyser = new QAction(tr("Analyse characters"), this);
            // NOTE: If running inside the Qt Creator IDE using the debugger with
            // the hovered() signal can be *problematic* - as hitting a
            // breakpoint - or getting an OS signal (like a Segment Violation)
            // can hang not only Mudlet but also Qt Creator and possibly even
            // your Desktop - though for *nix users switching to a console and
            // killing the gdb debugger instance run by Qt Creator will restore
            // normality.
            connect(mpContextMenuAnalyser, &QAction::hovered, this, &TTextEdit::slot_analyseSelection);
            mpContextMenuAnalyser->setToolTip(utils::richText(tr("Hover on this item to display the Unicode codepoints in the selection <i>(only the first line!)</i>")));
            popup->addSeparator();
            popup->addAction(mpContextMenuAnalyser);
        }

        popup->addSeparator();
        popup->addAction(action4);

        if (!mudlet::self()->isControlsVisible()) {
            QAction* actionRestoreMainMenu = new QAction(tr("restore Main menu"), this);
            connect(actionRestoreMainMenu, &QAction::triggered, mudlet::self(), &mudlet::slot_restoreMainMenu);
            actionRestoreMainMenu->setToolTip(utils::richText(tr("Use this to restore the Main menu to get access to controls.")));

            QAction* actionRestoreMainToolBar = new QAction(tr("restore Main Toolbar"), this);
            connect(actionRestoreMainToolBar, &QAction::triggered, mudlet::self(), &mudlet::slot_restoreMainToolBar);
            actionRestoreMainToolBar->setToolTip(utils::richText(tr("Use this to restore the Main Toolbar to get access to controls.")));

            popup->addSeparator();
            popup->addAction(actionRestoreMainMenu);
            popup->addAction(actionRestoreMainToolBar);
        }

        if (mpConsole->getType() == TConsole::ErrorConsole) {
            QAction* clearErrorConsole = new QAction(tr("Clear console"), this);
            connect(clearErrorConsole, &QAction::triggered, this, [=, this]() {
                mpConsole->buffer.clear();
                mpConsole->print(qsl("%1\n").arg(tr("*** starting new session ***")));
            });
            popup->addAction(clearErrorConsole);
        }

        // Add user actions
        if (mpHost) {
            QMapIterator<QString, QStringList> it(mpHost->mConsoleActions);

            while (it.hasNext()) {
                it.next();
                QStringList actionInfo = it.value();
                const QString& uniqueName = it.key();
                const QString& actionName = actionInfo.at(1);
                QAction* mouseAction = new QAction(actionName, this);
                mouseAction->setToolTip(actionInfo.at(2));
                popup->addAction(mouseAction);
                connect(mouseAction, &QAction::triggered, this, [this, uniqueName] { slot_mouseAction(uniqueName); });
            }
        }

        popup->popup(mapToGlobal(eventPos), action);
        event->accept();
        return;
    }

    QMouseEvent newEvent(event->type(), mpConsole->parentWidget()->mapFromGlobal(eventGlobalPos), eventGlobalPos, event->button(), event->buttons(), event->modifiers());
    switch (mpConsole->getType()) {
    case TConsole::CentralDebugConsole:
        [[fallthrough]];
    case TConsole::ErrorConsole:
        return;
    case TConsole::SubConsole:
        qApp->sendEvent(mpConsole->parentWidget(), &newEvent);
        break;
    case TConsole::MainConsole:
        [[fallthrough]];
    case TConsole::UserWindow:
        mpConsole->raiseMudletMousePressOrReleaseEvent(&newEvent, false);
        break;
    }

    // We have already bailed out before here for the Central Debug Console and
    // the editor Error console so those will avoid the focus being changed to
    // this profile now:
    QTimer::singleShot(0, this, [this]() {
        if (mpHost) {
            mudlet::self()->activateProfile(mpHost);
        }
    });
}

void TTextEdit::showEvent(QShowEvent* event)
{
    updateScreenView();
    mScrollVector = 0;
    repaint();
    QWidget::showEvent(event);
}

void TTextEdit::resizeEvent(QResizeEvent* event)
{
    updateScreenView();

    // Safety check: during destruction, mpHost or mpConsole might be null
    if (mpHost && mpConsole) {
        if (!mIsLowerPane && mpConsole->getType() == TConsole::MainConsole) {
            mpHost->updateDisplayDimensions();
        }
    }

    QWidget::resizeEvent(event);

    if (mpConsole && !mIsLowerPane
        && (mpConsole->getType() & (TConsole::MainConsole | TConsole::UserWindow | TConsole::SubConsole))) {

        mpConsole->raiseMudletResizeEvent();
    }
}

void TTextEdit::wheelEvent(QWheelEvent* e)
{
    // Make the speed up be half of the (upper pane) lines - need to round it so
    // that a decimal part does not make the end +/- value for up/down different
    // in magnitude:
    double ySpeedUp = qRound(mpConsole->mUpperPane->getScreenHeight() / 2.0);
    // Just a number plucked out of the air for the x-direction:
    double xSpeedUp = 10.0;

    QPointF delta = e->angleDelta();
    // Convert to degrees:
    delta /= 8.0;
    // Allow the control key to introduce a speed up - but also allow it to be
    // overridden by a shift key to slow the scroll down to one line/character
    // per click:
    delta.rx() *= (e->modifiers() & Qt::ShiftModifier ? 1.0 : (e->modifiers() & Qt::ControlModifier ? xSpeedUp : 3.0));
    delta.ry() *= (e->modifiers() & Qt::ShiftModifier ? 1.0 : (e->modifiers() & Qt::ControlModifier ? ySpeedUp : 3.0));
    // Add on any previously stored (integer) remainder:
    delta += mMouseWheelRemainder;
    // Convert to 15 degree steps and record them:
    int xDelta = qRound(delta.x() / 15.0);
    int yDelta = qRound(delta.y() / 15.0);
    // Store the (rounded) remainder
    mMouseWheelRemainder = QPoint(delta.x() - (15 * xDelta), delta.y() - (15 * yDelta));

    bool used = false;
    if (yDelta > 0) {
        mpConsole->scrollUp(yDelta);
        used = true;
    } else if (yDelta < 0) {
        mpConsole->scrollDown(-yDelta);
        used = true;
    }

    // Space for future use of xDelta

    e->setAccepted(used);
}

int TTextEdit::imageTopLine()
{
    if (!mIsLowerPane) {
        mCursorY = mpBuffer->mCursorY;
    }

    if (mCursorY > mScreenHeight) {
        // mIsTailMode is always true for lower pane and true for upper one when
        // it is scrolled to the bottom and new text is to be appended and the
        // older text is to scroll up:
        if (mIsTailMode && (mpBuffer->lineBuffer.at(mpBuffer->getLastLineNumber()).isEmpty())) {
            return mCursorY - mScreenHeight - 1;
        }

        return mCursorY - mScreenHeight;
    } else {
        return 0;
    }
}


// This should only be used on the upper pane:
int TTextEdit::bufferScrollDown(int lines)
{
    if ((mpBuffer->mCursorY + lines) < static_cast<int>(mpBuffer->size())) {
        if (mpBuffer->mCursorY < mScreenHeight) {
            mpBuffer->mCursorY = mScreenHeight + lines;
            if (mpBuffer->mCursorY > static_cast<int>(mpBuffer->size() - 1)) {
                mpBuffer->mCursorY = mpBuffer->lineBuffer.size();
                mIsTailMode = true;
            }

        } else {
            mpBuffer->mCursorY += lines;
            mIsTailMode = false;
        }
        return lines;

    } else if (mpBuffer->mCursorY >= static_cast<int>(mpBuffer->size() - 1)) {
        mIsTailMode = true;
        mpBuffer->mCursorY = mpBuffer->lineBuffer.size();
        forceUpdate();
        return 0;

    } else {
        lines = static_cast<int>(mpBuffer->size() - 1) - mpBuffer->mCursorY;
        if (mpBuffer->mCursorY + lines < mScreenHeight + lines) {
            mpBuffer->mCursorY = mScreenHeight + lines;
            if (mpBuffer->mCursorY > static_cast<int>(mpBuffer->size() - 1)) {
                mpBuffer->mCursorY = static_cast<int>(mpBuffer->size() - 1);
                mIsTailMode = true;
            }

        } else {
            mpBuffer->mCursorY += lines;
            mIsTailMode = false;
        }

        return lines;
    }
}

int TTextEdit::getColumnCount() const
{
    return qRound(width() / QFontMetricsF(font()).averageCharWidth());
}

int TTextEdit::getRowCount() const
{
    return qRound(height() / QFontMetricsF(font()).lineSpacing());
}

inline QString TTextEdit::htmlCenter(const QString& text)
{
    return qsl("<center>%1</center>").arg(text);
}

// Not just whitespace but also some formatting and other things - it may be
// that some entries do not work like this and we cannot just display a short
// bit of text to indicate them in the analysis of the on-screen content- the
// language directional controls may be like that:
inline QString TTextEdit::convertWhitespaceToVisual(const QChar& first, const QChar& second)
{
    // clang-format off
    if (second.isNull()) {
        // The code point is on the BMP
        quint16 value = first.unicode();
        switch (value) {
        case 0x003c:                    return htmlCenter(qsl("&lt;")); break; // As '<' gets interpreted as an opening HTML tag we have to handle it specially
        case 0x003e:                    return htmlCenter(qsl("&gt;")); break; // '>' does not seem to get interpreted as a closing HTML tag but for symmetry it is probably best to also handle it in the same way
        //: Unicode U+0009 codepoint.
        case QChar::Tabulation:         return htmlCenter(tr("{tab}")); break;
        //: Unicode U+000A codepoint. Not likely to be seen as it gets filtered out.
        case QChar::LineFeed:           return htmlCenter(tr("{line-feed}")); break;
        //: Unicode U+000D codepoint. Not likely to be seen as it gets filtered out.
        case QChar::CarriageReturn:     return htmlCenter(tr("{carriage-return}")); break;
        //: Unicode U+0020 codepoint.
        case QChar::Space:              return htmlCenter(tr("{space}")); break;
        //: Unicode U+00A0 codepoint.
        case QChar::Nbsp:               return htmlCenter(tr("{non-breaking space}")); break;
        //: Unicode U+00AD codepoint.
        case QChar::SoftHyphen:         return htmlCenter(tr("{soft hyphen}")); break;
        //: Unicode U+034F codepoint (badly named apparently - see Wikipedia!)
        case 0x034F:                    return htmlCenter(tr("{combining grapheme joiner}")); break;
        //: Unicode U+1680 codepoint.
        case 0x1680:                    return htmlCenter(tr("{ogham space mark}")); break;
        //: Unicode U+2000 codepoint.
        case 0x2000:                    return htmlCenter(tr("{'n' quad}")); break;
        //: Unicode U+2001 codepoint.
        case 0x2001:                    return htmlCenter(tr("{'m' quad}")); break;
        //: Unicode U+2002 codepoint - En ('n') wide space.
        case 0x2002:                    return htmlCenter(tr("{'n' space}")); break;
        //: Unicode U+2003 codepoint - Em ('m') wide space.
        case 0x2003:                    return htmlCenter(tr("{'m' space}")); break;
        //: Unicode U+2004 codepoint - three-per-em ('m') wide (thick) space.
        case 0x2004:                    return htmlCenter(tr("{3-per-em space}")); break;
        //: Unicode U+2005 codepoint - four-per-em ('m') wide (Middle) space.
        case 0x2005:                    return htmlCenter(tr("{4-per-em space}")); break;
        //: Unicode U+2006 codepoint - six-per-em ('m') wide (Sometimes the same as a Thin) space.
        case 0x2006:                    return htmlCenter(tr("{6-per-em space}")); break;
        //: Unicode U+2007 codepoint - figure (digit) wide space.
        case 0x2007:                    return htmlCenter(tr("{digit space}")); break;
        //: Unicode U+2008 codepoint.
        case 0x2008:                    return htmlCenter(tr("{punctuation wide space}")); break;
        //: Unicode U+2009 codepoint - five-per-em ('m') wide space.
        case 0x2009:                    return htmlCenter(tr("{5-per-em space}")); break;
        //: Unicode U+200A codepoint - thinnest space.
        case 0x200A:                    return htmlCenter(tr("{hair width space}")); break;
        //: Unicode U+200B codepoint.
        case 0x200B:                    return htmlCenter(tr("{zero width space}")); break;
        //: Unicode U+200C codepoint.
        case 0x200C:                    return htmlCenter(tr("{Zero width non-joiner}")); break;
        //: Unicode U+200D codepoint.
        case 0x200D:                    return htmlCenter(tr("{zero width joiner}")); break;
        //: Unicode U+200E codepoint.
        case 0x200E:                    return htmlCenter(tr("{left-to-right mark}")); break;
        //: Unicode U+200F codepoint.
        case 0x200F:                    return htmlCenter(tr("{right-to-left mark}")); break;
        //: Unicode 0x2028 codepoint.
        case QChar::LineSeparator:      return htmlCenter(tr("{line separator}")); break;
        //: Unicode U+2029 codepoint.
        case QChar::ParagraphSeparator: return htmlCenter(tr("{paragraph separator}")); break;
        //: Unicode U+202A codepoint.
        case 0x202A:                    return htmlCenter(tr("{Left-to-right embedding}")); break;
        //: Unicode U+202B codepoint.
        case 0x202B:                    return htmlCenter(tr("{right-to-left embedding}")); break;
        //: Unicode U+202C codepoint - pop (undo last) directional formatting.
        case 0x202C:                    return htmlCenter(tr("{pop directional formatting}")); break;
        //: Unicode U+202D codepoint.
        case 0x202D:                    return htmlCenter(tr("{Left-to-right override}")); break;
        //: Unicode U+202E codepoint.
        case 0x202E:                    return htmlCenter(tr("{right-to-left override}")); break;
        //: Unicode U+202F codepoint.
        case 0x202F:                    return htmlCenter(tr("{narrow width no-break space}")); break;
        //: Unicode U+205F codepoint.
        case 0x205F:                    return htmlCenter(tr("{medium width mathematical space}")); break;
        //: Unicode U+2060 codepoint.
        case 0x2060:                    return htmlCenter(tr("{zero width non-breaking space}")); break;
        //: Unicode U+2061 codepoint - function application (whatever that means!)
        case 0x2061:                    return htmlCenter(tr("{function application}")); break;
        //: Unicode U+2062 codepoint.
        case 0x2062:                    return htmlCenter(tr("{invisible times}")); break;
        //: Unicode U+2063 codepoint - invisible separator or comma.
        case 0x2063:                    return htmlCenter(tr("{invisible separator}")); break;
        //: Unicode U+2064 codepoint.
        case 0x2064:                    return htmlCenter(tr("{invisible plus}")); break;
        //: Unicode U+2066 codepoint.
        case 0x2066:                    return htmlCenter(tr("{left-to-right isolate}")); break;
        //: Unicode U+2067 codepoint.
        case 0x2067:                    return htmlCenter(tr("{right-to-left isolate}")); break;
        //: Unicode U+2068 codepoint.
        case 0x2068:                    return htmlCenter(tr("{first strong isolate}")); break;
        //: Unicode U+2069 codepoint - pop (undo last) directional isolate.
        case 0x2069:                    return htmlCenter(tr("{pop directional isolate}")); break;
        //: Unicode U+206A codepoint.
        case 0x206A:                    return htmlCenter(tr("{inhibit symmetrical swapping}")); break;
        //: Unicode U+206B codepoint.
        case 0x206B:                    return htmlCenter(tr("{activate symmetrical swapping}")); break;
        //: Unicode U+206C codepoint.
        case 0x206C:                    return htmlCenter(tr("{inhibit arabic form-shaping}")); break;
        //: Unicode U+206D codepoint.
        case 0x206D:                    return htmlCenter(tr("{activate arabic form-shaping}")); break;
        //: Unicode U+206E codepoint.
        case 0x206E:                    return htmlCenter(tr("{national digit shapes}")); break;
        //: Unicode U+206F codepoint.
        case 0x206F:                    return htmlCenter(tr("{nominal Digit shapes}")); break;
        //: Unicode U+3000 codepoint - ideographic (CJK Wide) space
        case 0x3000:                    return htmlCenter(tr("{ideographic space}")); break;
        //: Unicode U+FE00 codepoint.
        case 0xFE00:                    return htmlCenter(tr("{variation selector 1}")); break;
        //: Unicode U+FE01 codepoint.
        case 0xFE01:                    return htmlCenter(tr("{variation selector 2}")); break;
        //: Unicode U+FE02 codepoint.
        case 0xFE02:                    return htmlCenter(tr("{variation selector 3}")); break;
        //: Unicode U+FE03 codepoint.
        case 0xFE03:                    return htmlCenter(tr("{variation selector 4}")); break;
        //: Unicode U+FE04 codepoint.
        case 0xFE04:                    return htmlCenter(tr("{variation selector 5}")); break;
        //: Unicode U+FE05 codepoint.
        case 0xFE05:                    return htmlCenter(tr("{variation selector 6}")); break;
        //: Unicode U+FE06 codepoint.
        case 0xFE06:                    return htmlCenter(tr("{variation selector 7}")); break;
        //: Unicode U+FE07 codepoint.
        case 0xFE07:                    return htmlCenter(tr("{variation selector 8}")); break;
        //: Unicode U+FE08 codepoint.
        case 0xFE08:                    return htmlCenter(tr("{variation selector 9}")); break;
        //: Unicode U+FE09 codepoint.
        case 0xFE09:                    return htmlCenter(tr("{variation selector 10}")); break;
        //: Unicode U+FE0A codepoint.
        case 0xFE0A:                    return htmlCenter(tr("{variation selector 11}")); break;
        //: Unicode U+FE0B codepoint.
        case 0xFE0B:                    return htmlCenter(tr("{variation selector 12}")); break;
        //: Unicode U+FE0C codepoint.
        case 0xFE0C:                    return htmlCenter(tr("{variation selector 13}")); break;
        //: Unicode U+FE0D codepoint.
        case 0xFE0D:                    return htmlCenter(tr("{variation selector 14}")); break;
        //: Unicode U+FE0E codepoint - after an Emoji codepoint forces the textual (black & white) rendition.
        case 0xFE0E:                    return htmlCenter(tr("{variation selector 15}")); break;
        //: Unicode U+FE0F codepoint - after an Emoji codepoint forces the proper coloured 'Emoji' rendition.
        case 0xFE0F:                    return htmlCenter(tr("{variation selector 16}")); break;
        //: Unicode U+FEFF codepoint - also known as the Byte-order-mark at start of text!).
        case 0xFEFF:                    return htmlCenter(tr("{zero width no-break space}")); break;
        /*
         * case 0xFFF0:
         * to
         * case 0xFFF8: see default code-block
         */

        //: Unicode U+FFF9 codepoint.
        case 0xFFF9:                    return htmlCenter(tr("{interlinear annotation anchor}")); break;
        //: Unicode U+FFFA codepoint.
        case 0xFFFA:                    return htmlCenter(tr("{interlinear annotation separator}")); break;
        //: Unicode U+FFFB codepoint
        case 0xFFFB:                    return htmlCenter(tr("{interlinear annotation terminator}")); break;
        //: Unicode U+FFFC codepoint.
        case 0xFFFC:                    return htmlCenter(tr("{object replacement character}")); break;
        /*
         * case 0xFFFD: special case, is the replacement character and will mark
         *              characters that have already failed to be decoded
         *              correctly prior to this stage in processing - leave as is!
         */
        case 0xFFFE:
            [[fallthrough]];
        case 0xFFFF:
            [[fallthrough]];
        default:
            if (value >= 0xFDD0 && value <= 0xFDEF) {
                //: Unicode codepoint in range U+FFD0 to U+FDEF - not a character
                return htmlCenter(tr("{noncharacter}")); break;
            } else if ((value >= 0xFFF0 && value <= 0xFFF8) || value == 0xFFFE || value == 0xFFFF) {
                //: Unicode codepoint in range U+FFFx - not a character.
                return htmlCenter(tr("{noncharacter}")); break;
            } else {
                return htmlCenter(first);
            }
        }
    } else {
        // The code point is NOT on the BMP
        quint32 value = QChar::surrogateToUcs4(first, second);
        switch (value) {
        //: Unicode codepoint U+0001F3FB - FitzPatrick modifier (Emoji Human skin-tone) 1-2.
        case 0x1F3FB:                   return htmlCenter(tr("{FitzPatrick modifier 1 or 2}")); break;
        //: Unicode codepoint U+0001F3FC - FitzPatrick modifier (Emoji Human skin-tone) 3.
        case 0x1F3FC:                   return htmlCenter(tr("{FitzPatrick modifier 3}")); break;
        //: Unicode codepoint U+0001F3FD - FitzPatrick modifier (Emoji Human skin-tone) 4.
        case 0x1F3FD:                   return htmlCenter(tr("{FitzPatrick modifier 4}")); break;
        //: Unicode codepoint U+0001F3FE - FitzPatrick modifier (Emoji Human skin-tone) 5.
        case 0x1F3FE:                   return htmlCenter(tr("{FitzPatrick modifier 5}")); break;
        //: Unicode codepoint U+0001F3FF - FitzPatrick modifier (Emoji Human skin-tone) 6.
        case 0x1F3FF:                   return htmlCenter(tr("{FitzPatrick modifier 6}")); break;
        default:
            // The '%' is the modulus operator here:
            if ((value % 0x10000 == 0xFFFE) || (value % 0x10000 == 0xFFFF)) {
                //: Unicode codepoint is U+00xxFFFE or U+00xxFFFF - not a character.
                return htmlCenter(tr("{noncharacter}")); break;
            } else {
                // The '%' is the QStringBuilder append operator here:
                return htmlCenter(first % second);
            }
        }
    }
    // clang-format on
}

inline QString TTextEdit::byteToLuaCodeOrChar(const char* byte)
{
    if (!byte) {
        return QString();
    } else if (static_cast<quint8>(*byte) < 0x20 || static_cast<quint8>(*byte) >= 0x7f) {
        // Control character or not ASCII
        return qsl("\\%1").arg(static_cast<quint8>(*byte), 3, 10, QLatin1Char('0'));
    } else if (static_cast<quint8>(*byte) == 0x3C) {
        // '<' - which is noticed by the Qt library code and taken as an
        // HTML/Rich-text formatting opening tag and has to be converted to
        // "&lt;":
        return qsl("&lt;");
    } else {
        return qsl("%1").arg(*byte);
    }
}

/*
 * Formula to convert High+Low surrogate pairs to Unicode code-point:
 * (HighSurrogate - 0xD800) * 0x400 + (LowSurrogage - 0xDC00) + 0x10000
 */
void TTextEdit::slot_analyseSelection()
{
    if (!mpContextMenuAnalyser || mpBuffer->lineBuffer.isEmpty()) {
        // Menu has gone away or no text on screen
        return;
    }
    // If we get here we must at least have a line 0!

    normaliseSelection();
    // Get the smallest of the two lines in the range, but clamp it to the first
    // line which is zero and then the maximum line in existence:
    int line = qMin(qMax(qMin(mPA.y(), mPB.y()), 0), (mpBuffer->lineBuffer.size() - 1));

    int startColumn = -1;
    int endColumn = -1;
    // Hang on to the line length - we must never try to index a character
    // position equal to or more than this:
    const int lineLength = mpBuffer->lineBuffer.at(line).size();

    // Display the indexes as +1 so that the first character is at 1 not 0
    QString utf16indexes;
    QString utf16Vals;
    QString graphemes;
    QString utf8Indexes;
    QString utf8Vals;
    // utf8Vals converted from hex to decimal for non printable ASCII shown as
    // `\###` decimal codes or ASCII if it is in range - this is to match the
    // decimal numeric codes that lua uses for non-printable characters which
    // the user will need if they wish to enter a multi-byte character
    // (non-ASCII) into a literal string:
    QString luaCodes;
    QString completedRows;
    quint8 rowItems = 0;
    QChar zero('0');
    // Start the UTF-8 indexing at 1 so that it directly maps to string indexing
    // in Lua.
    short int utf8Index = 1;
    char utf8Bytes[5];
    utf8Bytes[4] = '\0';

    int total = 0;
    startColumn = mPA.x();
    if (mPA.y() == mPB.y()) {
        // The selection is from mPA.x() to mPB.x()
        endColumn = mPB.x();
        if (endColumn == -1) {
            // Handle the special case where -1 is used to mean "to the end of
            // the line":
            endColumn = lineLength - 1;
        }

    } else {
        startColumn = mPA.x();
        endColumn = lineLength - 1;
    }
    // total is now (that we only show the selected part of the first line and
    // not the whole line) the number of QChars/TChars to be shown:
    total = 1 + endColumn - startColumn;

    // We do not want more than around 16 code-points per row, but we also do
    // not want orphans (a few odd code-points) on the last row so deduce a
    // number of items to include in a row:
    quint8 rowsCount = qMax(1, qRound((total + 8.5) / 16.0));
    quint8 rowLimit = qMax(8, qRound(total * 1.0 / rowsCount));
    bool isFirstRow = true;

    for (int index = 0; index < lineLength; ++index) {
        bool includeThisCodePoint = false;
        if (index >= startColumn && index <= endColumn) {
            includeThisCodePoint = true;
        }

        if (mpBuffer->lineBuffer.at(line).at(index).isHighSurrogate() && ((index + 1) < lineLength)) {
            strncpy(utf8Bytes, mpBuffer->lineBuffer.at(line).mid(index, 2).toUtf8().constData(), 4);
            size_t utf8Width = strnlen(utf8Bytes, 4);
            quint8 columnsToUse = qMax(static_cast<size_t>(2), utf8Width);

            if (includeThisCodePoint) {
                utf16indexes.append(qsl("<th colspan=\"%1\"><center>%2 & %3</center></th>").arg(QString::number(columnsToUse), QString::number(index + 1), QString::number(index + 2)));

                // The use of one qsl inside another is because it is
                // impossible to force an upper-case alphabet to Hex digits otherwise
                // just for that number (and not the rest of the resultant String):
                // &#8232; is the Unicode Line Separator
                utf16Vals.append(
                        qsl("<td colspan=\"%1\" style=\"white-space:no-wrap vertical-align:top\"><center>%2</center>&#8232;<center>(0x%3:0x%4)</center></td>")
                                .arg(QString::number(columnsToUse))
#if QT_VERSION >= QT_VERSION_CHECK(6, 9, 0)
                                .arg(qsl("%1").arg(static_cast<uint32_t>(QChar::surrogateToUcs4(mpBuffer->lineBuffer.at(line).at(index),
                                                                                                mpBuffer->lineBuffer.at(line).at(index + 1))),
                                                   4, 16, zero).toUpper())
                                .arg(static_cast<uint16_t>(mpBuffer->lineBuffer.at(line).at(index).unicode()),
                                     4, 16, zero)
                                .arg(static_cast<uint16_t>(mpBuffer->lineBuffer.at(line).at(index + 1).unicode()),
                                     4, 16, zero));
#else
                                .arg(qsl("%1").arg(QChar::surrogateToUcs4(mpBuffer->lineBuffer.at(line).at(index),
                                                                          mpBuffer->lineBuffer.at(line).at(index + 1)),
                                                   4, 16, zero).toUpper())
                                .arg(mpBuffer->lineBuffer.at(line).at(index).unicode(),
                                     4, 16, zero)
                                .arg(mpBuffer->lineBuffer.at(line).at(index + 1).unicode(),
                                     4, 16, zero));
#endif

                // Note the addition to the index here to jump over the low-surrogate:
                graphemes.append(qsl("<td colspan=\"%1\">%2</td>")
                                         .arg(QString::number(columnsToUse))
                                         .arg(convertWhitespaceToVisual(mpBuffer->lineBuffer.at(line).at(index), mpBuffer->lineBuffer.at(line).at(index + 1))));
            }

            switch (utf8Width) {
            case 4:
                if (includeThisCodePoint) {
                    utf8Indexes.append(qsl("<th><center>%1</center></th><td><center>%2</center></td><td><center>%3</center></td><td><center>%4</center></td></b>")
                                               .arg(QString::number(utf8Index), QString::number(utf8Index + 1), QString::number(utf8Index + 2), QString::number(utf8Index + 3)));
                    utf8Vals.append(qsl("<td><center>0x%1</center></td><td><center>0x%2</center></td><td><center>0x%3</center></td><td><center>0x%4</center></td>")
                                            .arg(static_cast<quint8>(utf8Bytes[0]), 2, 16, zero)
                                            .arg(static_cast<quint8>(utf8Bytes[1]), 2, 16, zero)
                                            .arg(static_cast<quint8>(utf8Bytes[2]), 2, 16, zero)
                                            .arg(static_cast<quint8>(utf8Bytes[3]), 2, 16, zero));
                    luaCodes.append(qsl("<td><center>%1</center></td><td><center>%2</center></td><td><center>%3</center></td><td><center>%4</center></td>")
                                            .arg(byteToLuaCodeOrChar(&utf8Bytes[0]), byteToLuaCodeOrChar(&utf8Bytes[1]), byteToLuaCodeOrChar(&utf8Bytes[2]), byteToLuaCodeOrChar(&utf8Bytes[3])));
                }
                utf8Index += 4;
                break;
            case 3:
                if (includeThisCodePoint) {
                    utf8Indexes.append(qsl("<th><center>%1</center></th><td><center>%2</center></td><td><center>%3</center></td>")
                                               .arg(QString::number(utf8Index), QString::number(utf8Index + 1), QString::number(utf8Index + 2)));
                    utf8Vals.append(qsl("<td><center>0x%1</center></td><td><center>0x%2</center></td><td><center>0x%3</center></td>")
                                            .arg(static_cast<quint8>(utf8Bytes[0]), 2, 16, zero)
                                            .arg(static_cast<quint8>(utf8Bytes[1]), 2, 16, zero)
                                            .arg(static_cast<quint8>(utf8Bytes[2]), 2, 16, zero));
                    luaCodes.append(qsl("<td><center>%1</center></td><td><center>%2</center></td><td><center>%3</center></td>")
                                            .arg(byteToLuaCodeOrChar(&utf8Bytes[0]), byteToLuaCodeOrChar(&utf8Bytes[1]), byteToLuaCodeOrChar(&utf8Bytes[2])));
                }
                utf8Index += 3;
                break;
            case 2:
                if (includeThisCodePoint) {
                    utf8Indexes.append(qsl("<th><center>%1</center></th><td><center>%2</center></td>").arg(QString::number(utf8Index), QString::number(utf8Index + 1)));
                    utf8Vals.append(qsl("<td><center>0x%1</center></td><td><center>0x%2</center></td>")
                                            .arg(static_cast<quint8>(utf8Bytes[0]), 2, 16, zero)
                                            .arg(static_cast<quint8>(utf8Bytes[1]), 2, 16, zero));
                    luaCodes.append(qsl("<td><center>%1</center></td><td><center>%2</center></td>").arg(byteToLuaCodeOrChar(&utf8Bytes[0]), byteToLuaCodeOrChar(&utf8Bytes[1])));
                }
                utf8Index += 2;
                break;
            default:
                if (includeThisCodePoint) {
                    utf8Indexes.append(qsl("<th><center>%1</center></th>").arg(QString::number(utf8Index)));
                    utf8Vals.append(qsl("<td><center>0x%1</center></td>").arg(static_cast<quint8>(utf8Bytes[0]), 2, 16, zero));
                    luaCodes.append(qsl("<td><center>%1</center></td>").arg(byteToLuaCodeOrChar(&utf8Bytes[0])));
                }
                ++utf8Index;
            }

            if (includeThisCodePoint) {
                rowItems += 2;
            }

            // Need to add an extra 1 to index to account for using 2 QChars
            // for the surrogate pair:
            index += 1;
        } else {
            strncpy(utf8Bytes, mpBuffer->lineBuffer.at(line).mid(index, 1).toUtf8().constData(), 4);
            size_t utf8Width = strnlen(utf8Bytes, 4);
            quint8 columnsToUse = qMax(static_cast<size_t>(1), utf8Width);

            if (includeThisCodePoint) {
                utf16indexes.append(qsl("<th colspan=\"%1\"><center>%2</center></th>").arg(QString::number(columnsToUse), QString::number(index + 1)));

                utf16Vals.append(qsl("<td colspan=\"%1\" style=\"white-space:no-wrap vertical-align:top\"><center>%2</center></td>")
                                         .arg(QString::number(columnsToUse))
#if QT_VERSION >= QT_VERSION_CHECK(6, 9, 0)
                                         .arg(static_cast<uint16_t>(mpBuffer->lineBuffer.at(line).at(index).unicode()),
                                              4, 16, QChar('0'))
                                         .toUpper());
#else
                                         .arg(mpBuffer->lineBuffer.at(line).at(index).unicode(),
                                              4, 16, QChar('0'))
                                         .toUpper());
#endif

                graphemes.append(qsl("<td colspan=\"%1\">%2</td>").arg(QString::number(columnsToUse), convertWhitespaceToVisual(mpBuffer->lineBuffer.at(line).at(index))));
            }

            switch (utf8Width) {
            case 4: // Maybe a BMP character cannot use 4 utf-8 bytes?
                if (includeThisCodePoint) {
                    utf8Indexes.append(qsl("<th><center>%1</center></th><td><center>%2</center></td><td><center>%3</center></td><td><center>%4</center></td>")
                                               .arg(QString::number(utf8Index), QString::number(utf8Index + 1), QString::number(utf8Index + 2), QString::number(utf8Index + 3)));

                    utf8Vals.append(qsl("<td><center>0x%1</center></td><td><center>0x%2</center></td><td><center>0x%3</center></td><td><center>0x%4</center></td>")
                                            .arg(static_cast<quint8>(utf8Bytes[0]), 2, 16, zero)
                                            .arg(static_cast<quint8>(utf8Bytes[1]), 2, 16, zero)
                                            .arg(static_cast<quint8>(utf8Bytes[2]), 2, 16, zero)
                                            .arg(static_cast<quint8>(utf8Bytes[3]), 2, 16, zero));

                    luaCodes.append(qsl("<td><center>%1</center></td><td><center>%2</center></td><td><center>%3</center></td><td><center>%4</center></td>")
                                            .arg(byteToLuaCodeOrChar(&utf8Bytes[0]), byteToLuaCodeOrChar(&utf8Bytes[1]), byteToLuaCodeOrChar(&utf8Bytes[2]), byteToLuaCodeOrChar(&utf8Bytes[3])));
                }
                utf8Index += 4;
                break;
            case 3:
                if (includeThisCodePoint) {
                    utf8Indexes.append(qsl("<th><center>%1</center></th><td><center>%2</center></td><td><center>%3</center></td>")
                                               .arg(QString::number(utf8Index), QString::number(utf8Index + 1), QString::number(utf8Index + 2)));

                    utf8Vals.append(qsl("<td><center>0x%1</center></td><td><center>0x%2</center></td><td><center>0x%3</center></td>")
                                            .arg(static_cast<quint8>(utf8Bytes[0]), 2, 16, zero)
                                            .arg(static_cast<quint8>(utf8Bytes[1]), 2, 16, zero)
                                            .arg(static_cast<quint8>(utf8Bytes[2]), 2, 16, zero));

                    luaCodes.append(qsl("<td><center>%1</center></td><td><center>%2</center></td><td><center>%3</center></td>")
                                            .arg(byteToLuaCodeOrChar(&utf8Bytes[0]), byteToLuaCodeOrChar(&utf8Bytes[1]), byteToLuaCodeOrChar(&utf8Bytes[2])));
                }
                utf8Index += 3;
                break;

            case 2:
                if (includeThisCodePoint) {
                    utf8Indexes.append(qsl("<th><center>%1</center></th><td><center>%2</center></td>").arg(QString::number(utf8Index), QString::number(utf8Index + 1)));

                    utf8Vals.append(qsl("<td><center>0x%1</center></td><td><center>0x%2</center></td>")
                                            .arg(static_cast<quint8>(utf8Bytes[0]), 2, 16, zero)
                                            .arg(static_cast<quint8>(utf8Bytes[1]), 2, 16, zero));

                    luaCodes.append(qsl("<td><center>%1</center></td><td><center>%2</center></td>").arg(byteToLuaCodeOrChar(&utf8Bytes[0]), byteToLuaCodeOrChar(&utf8Bytes[1])));
                }
                utf8Index += 2;
                break;

            default:
                if (includeThisCodePoint) {
                    utf8Indexes.append(qsl("<th><center>%1</center></th>").arg(QString::number(utf8Index)));

                    utf8Vals.append(qsl("<td><center>0x%1</center></td>").arg(static_cast<quint8>(utf8Bytes[0]), 2, 16, zero));

                    luaCodes.append(qsl("<td><center>%1</center></td>").arg(byteToLuaCodeOrChar(&utf8Bytes[0])));
                }
                ++utf8Index;
            }

            if (includeThisCodePoint) {
                ++rowItems;
            }
        }

        if (rowItems > rowLimit) {
            if (isFirstRow) {
                completedRows =
                        qsl("<small><table border=\"1\" style=\"margin-top:5px; margin-bottom:5px; margin-left:5px; margin-right:5px;\" width=\"100%\" cellspacing=\"2\" cellpadding=\"0\">"
                                       "<tr><th>%1</th>%2</tr>"
                                       "<tr><th>%3</th>%4</tr>"
                                       "<tr><th>%5</th>%6</tr>"
                                       "<tr><th>%7</th>%8</tr>"
                                       "<tr><th>%9</th>%10</tr>"
                                       "<tr><th>%11</th>%12</tr>"
                                       "</table></small><br>")
                                //: 1st Row heading for Text analyser output, table item is the count into the QChars/TChars that make up the text {this translation used 2 times}
                                .arg(tr("Index (UTF-16)"), utf16indexes)
                                /*:
                                2nd Row heading for Text analyser output, table item is the unicode code point (will be
                                between 000001 and 10FFFF in hexadecimal) {this translation used 2 times}
                                */
                                .arg(tr("U+<i>####</i> Unicode Code-point <i>(High:Low Surrogates)</i>"), utf16Vals)
                                /*:
                                3rd Row heading for Text analyser output, table item is a visual representation of the character/part of the character or a '{'...'}' wrapped
                                letter code if the character is whitespace or otherwise unshowable {this translation used 2 times}
                                */
                                .arg(tr("Visual"), graphemes)
                                /*:
                                4th Row heading for Text analyser output, table item is the count into the bytes that make up the UTF-8 form of the text that the Lua system
                                uses {this translation used 2 times}
                                */
                                .arg(tr("Index (UTF-8)"), utf8Indexes)
                                /*:
                                5th Row heading for Text analyser output, table item is the unsigned 8-bit integer for the particular byte in the UTF-8 form of the text that the Lua
                                system uses {this translation used 2 times}
                                */
                                .arg(tr("Byte"), utf8Vals)
                                /*:
                                6th Row heading for Text analyser output, table item is either the ASCII character or the numeric code for the byte in the row about
                                this item in the table, as displayed the thing shown can be used in a Lua string entry to reproduce this byte {this translation used
                                2 times}"
                                */
                                .arg(tr("Lua character or code"), luaCodes);
                isFirstRow = false;
            } else {
                completedRows.append(
                        qsl("<small><table border=\"1\" style=\"margin-top:5px; margin-bottom:5px; margin-left:5px; margin-right:5px;\" width=\"100%\" cellspacing=\"2\" cellpadding=\"0\">"
                                       "<tr>%1</tr>"
                                       "<tr>%2</tr>"
                                       "<tr>%3</tr>"
                                       "<tr>%4</tr>"
                                       "<tr>%5</tr>"
                                       "<tr>%6</tr>"
                                       "</table></small><br>")
                                .arg(utf16indexes, utf16Vals, graphemes, utf8Indexes, utf8Vals, luaCodes));
            }
            rowItems = 0;
            utf16indexes.clear();
            utf16Vals.clear();
            graphemes.clear();
            utf8Indexes.clear();
            utf8Vals.clear();
            luaCodes.clear();
        }
    }

    if (mpContextMenuAnalyser) {
        if (isFirstRow) {
            // if this is still true then we only have a short, single line of
            // less than 16 codepoints
            mpContextMenuAnalyser->setToolTip(
                    qsl("%1"
                        "<small><table border=\"1\" style=\"margin-top:5px; margin-bottom:5px; margin-left:5px; margin-right:5px;\" width=\"100%\" cellspacing=\"2\" cellpadding=\"0\">"
                        "<tr><th>%2</th>%3</tr>"
                        "<tr><th>%4</th>%5</tr>"
                        "<tr><th>%6</th>%7</tr>"
                        "<tr><th>%8</th>%9</tr>"
                        "<tr><th>%10</th>%11</tr>"
                        "<tr><th>%12</th>%13</tr>"
                        "</table></small>")
                        .arg(completedRows)
                        //: 1st Row heading for Text analyser output, table item is the count into the QChars/TChars that make up the text {this translation used 2 times}
                        .arg(tr("Index (UTF-16)"), utf16indexes)
                        /*:
                        2nd Row heading for Text analyser output, table item is the unicode code point (will be between
                        000001 and 10FFFF in hexadecimal) {this translation used 2 times}
                        */
                        .arg(tr("U+<i>####</i> Unicode Code-point <i>(High:Low Surrogates)</i>"), utf16Vals)
                        /*:
                        3rd Row heading for Text analyser output, table item is a visual representation of the character/part of the character or a '{'...'}' wrapped letter
                        code if the character is whitespace or otherwise unshowable {this translation used 2 times}
                        */
                        .arg(tr("Visual"), graphemes)
                        /*:
                        4th Row heading for Text analyser output, table item is the count into the bytes that make up the UTF-8 form of the text that the Lua system
                        uses {this translation used 2 times}
                        */
                        .arg(tr("Index (UTF-8)"), utf8Indexes)
                        /*:
                        5th Row heading for Text analyser output, table item is the unsigned 8-bit integer for the particular byte in the UTF-8 form of the text that the Lua
                        system uses {this translation used 2 times}
                        */
                        .arg(tr("Byte"), utf8Vals)
                        /*:
                        6th Row heading for Text analyser output, table item is either the ASCII character or the numeric code for the byte in the row about
                        this item in the table, as displayed the thing shown can be used in a Lua string entry to reproduce this byte {this translation used 2
                        times}
                        */
                        .arg(tr("Lua character or code"), luaCodes));
        } else {
            mpContextMenuAnalyser->setToolTip(
                        qsl("%1"
                            "<small><table border=\"1\" style=\"margin-top:5px; margin-bottom:5px; margin-left:5px; margin-right:5px;\" width=\"100%\" cellspacing=\"2\" cellpadding=\"0\">"
                            "<tr>%2</tr>"
                            "<tr>%3</tr>"
                            "<tr>%4</tr>"
                            "<tr>%5</tr>"
                            "<tr>%6</tr>"
                            "<tr>%7</tr>"
                            "</table></small>")
                        .arg(completedRows, utf16indexes, utf16Vals, graphemes, utf8Indexes, utf8Vals, luaCodes));
        }
    }
}

void TTextEdit::slot_changeIsAmbigousWidthGlyphsToBeWide(const bool state)
{
    if (mWideAmbigousWidthGlyphs != state) {
        mWideAmbigousWidthGlyphs = state;
        update();
    }
}

#if defined(DEBUG_CODEPOINT_PROBLEMS)
void TTextEdit::slot_changeDebugShowAllProblemCodepoints(const bool state)
{
    if (mShowAllCodepointIssues != state) {
        mShowAllCodepointIssues = state;
    }
}
#endif

void TTextEdit::slot_mouseAction(const QString &uniqueName)
{
    if (!mpHost) {
        return;
    }

    TEvent event {};
    QStringList mouseEvent = mpHost->mConsoleActions[uniqueName];
    event.mArgumentList.append(mouseEvent[0]);
    event.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
    event.mArgumentList.append(uniqueName);

    event.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
    event.mArgumentList.append(mpConsole->mConsoleName);

    event.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
    event.mArgumentList.append(QString::number(mPA.x()));
    event.mArgumentTypeList.append(ARGUMENT_TYPE_NUMBER);
    event.mArgumentList.append(QString::number(mPA.y()));
    event.mArgumentTypeList.append(ARGUMENT_TYPE_NUMBER);
    event.mArgumentList.append(QString::number(mPB.x()));
    event.mArgumentTypeList.append(ARGUMENT_TYPE_NUMBER);
    event.mArgumentList.append(QString::number(mPB.y()));
    event.mArgumentTypeList.append(ARGUMENT_TYPE_NUMBER);
    mpHost->raiseEvent(event);
}


#if defined(DEBUG_CODEPOINT_PROBLEMS)
// Originally this was going to be part of the destructor - but it was unable
// to get the parent Console and Profile names at that point:
void TTextEdit::reportCodepointErrors()
{
    if (mIsLowerPane) {
        return;
    }

    if (!mProblemCodepoints.isEmpty()) {
        QList<uint> keyList{mProblemCodepoints.keys()};
        std::sort(keyList.begin(), keyList.end());
        qDebug().nospace().noquote() << "TTextEdit INFO - during the use of TTextEdit::getGraphemeWidth(...) for the \n"
                                     << "    \"" << mpConsole->mConsoleName << "\" console for the \"" << mpConsole->mProfileName << "\" profile\n"
                                     << "    the following awkward Unicode codepoints were detected with the frequency\n"
                                        "    indicated. Note that this does not precisely reflect how many times they\n"
                                        "    were seen in the MUD Game server output but instead how many times they\n"
                                        "    had to be redrawn so also depends on how much scrolling around was done.\n"
                                        "    Nevertheless the higher the number the more desirable to eliminate them\n"
                                        "    (if possible) it would be.\n"
                                        "    You may wish to consult with the Mudlet Makers via any of our support\n"
                                        "    channels to see if these problems are known about and can be fixed:\n";
        qDebug().nospace().noquote() << qsl("%1 %2 %3").arg(qsl("    Codepoint (Hex)")).arg(qsl("Count"), 7).arg(qsl("Reason"), -7);
        for (int i = 0, total = mProblemCodepoints.count(); i < total; ++i) {
            const auto key = keyList.at(i);
            const auto [count, reason] = mProblemCodepoints.value(key);
            if (key <= 0xffff) {
                // Within the BMP:
                qDebug().nospace().noquote() << qsl("             U+%1 %2 %3").arg(key, 4, 16, QLatin1Char('0')).arg(count, 7).arg(reason.c_str());
            } else {
                // Beyond the BMP:
                qDebug().nospace().noquote() << qsl("           U+%1 %2 %3").arg(key, 6, 16, QLatin1Char('0')).arg(count, 7).arg(reason.c_str());
            }
        }
        // Needed to put a blank line after the last entry:
        qDebug().nospace().noquote() << " ";
    }
}
#endif

void TTextEdit::setCaretPosition(int line, int column)
{
    mCaretLine = line;
    mCaretColumn = column;

    if (!mpHost || !mpHost->caretEnabled()) {
        return;
    }

    // Check if the caret has landed on a link and update focus state
    int linkIndex = mpBuffer->getLinkIndexAt(line, column);
    if (linkIndex > 0) {
        // Caret is on a link - set it as focused (keyboard navigation)
        if (mpBuffer->getFocusedLink() != linkIndex) {
            mpBuffer->setFocusedLink(linkIndex);
        }
    } else {
        // Caret is not on a link - clear any focused link
        if (mpBuffer->getFocusedLink() != 0) {
            mpBuffer->setFocusedLink(0);
        }
    }

    updateCaret();
}

void TTextEdit::initializeCaret()
{
    setCaretPosition(mpBuffer->lineBuffer.length() - 2, 0);
}

void TTextEdit::updateCaret()
{
    int lineOffset = imageTopLine();

    if (!mIsLowerPane) {
        if (mCaretLine < lineOffset) {
            scrollTo(mCaretLine + 1);
        } else if (mCaretLine >= lineOffset + mScreenHeight) {
            int emptyLastLine = mpBuffer->lineBuffer.last().isEmpty();
            if (mCaretLine == mpBuffer->lineBuffer.length() - 1 - emptyLastLine) {
                scrollTo(mCaretLine + 2);
            } else {
                scrollTo(mCaretLine + 1);
            }
        }
    }

    // FIXME: Update only the affected region.
    forceUpdate();

    if (QAccessible::isActive()) {
        const QAccessibleTextInterface* ti = QAccessible::queryAccessibleInterface(this)->textInterface();
        QAccessibleTextCursorEvent event(this, ti->cursorPosition());

        QAccessible::updateAccessibility(&event);
    }
}

// This event handler, for event event, can be reimplemented in a subclass to
// receive key press events for the widget.
//
// A widget must call setFocusPolicy() to accept focus initially and have focus
// in order to receive a key press event.
//
// If you reimplement this handler, it is very important that you call the base
// class implementation if you do not act upon the key.
//
// The default implementation closes popup widgets if the user presses the key
// sequence for QKeySequence::Cancel (typically the Escape key). Otherwise the
// event is ignored, so that the widget's parent can interpret it.
//
// Note that QKeyEvent starts with isAccepted() == true, so you do not need to
// call QKeyEvent::accept() - just do not call the base class implementation if
// you act upon the key.
void TTextEdit::keyPressEvent(QKeyEvent* event)
{
    if (!mpHost || !mpHost->caretEnabled()) {
        QWidget::keyPressEvent(event);
        return;
    }

    // #7933 Auto-reditect focus to command line from output window when press alpha-numeric characters
    // skips ctrl,alt, etc. This improves experiencie and makes fast switch to screenreader users focusing on output
    if (!(event->modifiers() & (Qt::ControlModifier | Qt::AltModifier | Qt::MetaModifier)) && !event->text().isEmpty() && event->text().front().isPrint()) {
        if (mpHost && mpConsole && mpConsole->mpCommandLine) {
            mpHost->setCaretEnabled(false);
            mpHost->setFocusOnHostActiveCommandLine();
            QKeyEvent newEvent(event->type(), event->key(), event->modifiers(), event->text(), event->isAutoRepeat(), event->count());
            qApp->sendEvent(mpConsole->mpCommandLine, &newEvent);
            return;
        }
        // if not command line ignore
    }

    qsizetype newCaretLine = -1;
    qsizetype newCaretColumn = -1;

    auto adjustCaretColumn = [&]() {
        // If the new line is shorter, we need to adjust the column.
        qsizetype newLineLength = mpBuffer->line(newCaretLine).length();
        if (mCaretColumn >= newLineLength) {
            newCaretColumn = newLineLength == 0 ? 0 : newLineLength - 1;

            // Don't overwrite a previously saved old column value. We will want
            // to return to the original column that was selected by the user.
            if (mOldCaretColumn == 0) {
                mOldCaretColumn = mCaretColumn;
            }
        } else if (mOldCaretColumn != 0) {
            if (mOldCaretColumn < newLineLength) {
                // Now that the line is long enough again, restore the old column value.
                newCaretColumn = mOldCaretColumn;
                mOldCaretColumn = 0;
            } else {
                newCaretColumn = newLineLength - 1;
            }
        }
    };

    auto updateSelection = [&](bool useNewColumn) {
        if (QGuiApplication::keyboardModifiers().testFlag(Qt::ShiftModifier) && !mShiftSelection) {
            mShiftSelection = true;
            mDragStart.setY(mCaretLine);
            mDragStart.setX(mCaretColumn);
            mDragSelectionEnd.setY(newCaretLine);
            mDragSelectionEnd.setX(mCaretColumn);
            unHighlight();
            normaliseSelection();
            highlightSelection();
        } else if (mShiftSelection) {
            if (!QGuiApplication::keyboardModifiers().testFlag(Qt::ShiftModifier)) {
                mShiftSelection = false;
                unHighlight();
                mSelectedRegion = QRegion(0, 0, 0, 0);

                // keep cursor position as-is because it is only the selection that should be cleared
                newCaretLine = mCaretLine;
                newCaretColumn = mCaretColumn;
                return;
            }
            mDragSelectionEnd.setY(newCaretLine);
            mDragSelectionEnd.setX(useNewColumn ? newCaretColumn : mCaretColumn);

            unHighlight();
            normaliseSelection();
            highlightSelection();
        }
    };

    switch (event->key()) {
        case Qt::Key_Up: {
            if (mCaretLine == 0) {
                newCaretLine = mCaretLine;
                newCaretColumn = 0;
            } else {
                newCaretLine = mCaretLine - 1;
                newCaretColumn = mCaretColumn;
            }

            adjustCaretColumn();
            updateSelection(true);
        }
            break;
        case Qt::Key_Down: {
            int emptyLastLine = mpBuffer->lineBuffer.last().isEmpty();
            if (mCaretLine >= mpBuffer->lineBuffer.length() - 1 - emptyLastLine) {
                newCaretLine = mCaretLine;
                newCaretColumn = mpBuffer->line(mCaretLine).length() - 1;
            } else {
                newCaretLine = mCaretLine + 1;
                newCaretColumn = mCaretColumn;
            }

            adjustCaretColumn();
            updateSelection(true);
        }
            break;
        case Qt::Key_Left: {
            bool jumpedLines = false;
            if (mCaretColumn > 0) {
                if (QGuiApplication::keyboardModifiers().testFlag(Qt::ControlModifier)) {
                    const auto& line = mpBuffer->line(mCaretLine);
                    QTextBoundaryFinder finder(QTextBoundaryFinder::Word, line);
                    finder.setPosition(mCaretColumn);
                    int nextBoundary {};
                    QString currentLetter {};

                    do {
                        nextBoundary = finder.toPreviousBoundary();
                        currentLetter = line.mid(nextBoundary, 1);
                    } while (nextBoundary != 0 && mCtrlSelectionIgnores.contains(currentLetter));

                    newCaretLine = mCaretLine;
                    newCaretColumn = nextBoundary;
                } else {
                    newCaretLine = mCaretLine;
                    newCaretColumn = mCaretColumn - 1;
                }
            } else if (mCaretLine > 0) {
                newCaretLine = mCaretLine - 1;
                newCaretColumn = mpBuffer->lineBuffer.at(newCaretLine).length() - 1;
                jumpedLines = true;
            }

            // use newCaretColumn if we jumped lines or the selection extends to the left of the cursor
            const bool selectionBehindCursor = newCaretColumn < mCaretColumn;

            updateSelection(jumpedLines || selectionBehindCursor);
        }
            break;
        case Qt::Key_Right: {
            bool jumpedLines = false;
            if (mCaretColumn < (mpBuffer->lineBuffer.at(mCaretLine).length() - 1)) {
                if (QGuiApplication::keyboardModifiers().testFlag(Qt::ControlModifier)) {
                    const auto& line = mpBuffer->line(mCaretLine);
                    QTextBoundaryFinder finder(QTextBoundaryFinder::Word, line);
                    finder.setPosition(mCaretColumn);
                    int nextBoundary {};
                    QString currentLetter {};

                    do {
                        nextBoundary = finder.toNextBoundary();
                        currentLetter = line.mid(nextBoundary, 1);
                    } while (nextBoundary != line.length() && mCtrlSelectionIgnores.contains(currentLetter));

                    nextBoundary = std::min<qsizetype>(nextBoundary, line.length() - 1);
                    newCaretColumn = nextBoundary;
                    newCaretLine = mCaretLine;
                } else {
                    newCaretColumn = mCaretColumn + 1;
                    newCaretLine = mCaretLine;
                }
                // last line of the buffer is empty, so we need to check for that:
            } else if (mCaretLine < (mpBuffer->lineBuffer.length() - 2)) {
                newCaretLine = mCaretLine + 1;
                newCaretColumn = 0;
                jumpedLines = true;
            } else {
                // last line, last character in the buffer
                newCaretLine = mCaretLine;
                newCaretColumn = mCaretColumn;
            }

            // use newCaretColumn if we jumped lines or the selection extends to the right of the cursor
            const bool selectionBehindCursor = newCaretColumn > mCaretColumn;

            updateSelection(jumpedLines || selectionBehindCursor);
        }
            break;
        case Qt::Key_Home:
            if (QGuiApplication::keyboardModifiers().testFlag(Qt::ControlModifier)) {
                newCaretLine = 0;
                newCaretColumn = 0;
            } else {
                newCaretColumn = 0;
            }
            newCaretColumn = 0;
            break;
        case Qt::Key_End:
            if (QGuiApplication::keyboardModifiers().testFlag(Qt::ControlModifier)) {
                newCaretLine = mpBuffer->lineBuffer.length() - 1;
                newCaretColumn = mpBuffer->lineBuffer[mCaretLine].length() - 1;
            } else {
                newCaretColumn = mpBuffer->lineBuffer.at(mCaretLine).length() - 1;
            }
            break;
        case Qt::Key_PageUp:
            newCaretLine = std::max(mCaretLine - mScreenHeight, 0);
            break;
        case Qt::Key_PageDown:
            newCaretLine = std::min<qsizetype>(mCaretLine + mScreenHeight, mpBuffer->lineBuffer.length() - 2);
            break;
        case Qt::Key_Return:
        case Qt::Key_Enter:
        case Qt::Key_Space: {
            // Activate the focused link when Enter or Space is pressed in caret mode
            int focusedLink = mpBuffer->getFocusedLink();
            if (focusedLink > 0) {
                // Get the link commands and execute them
                QStringList commands = mpBuffer->mLinkStore.getLinksConst(focusedLink);
                if (!commands.isEmpty()) {
                    // Mark the link as visited
                    mpBuffer->markLinkAsVisited(focusedLink);

                    // Execute the command(s)
                    for (const auto& cmd : commands) {
                        mpHost->send(cmd);
                    }

                    // Don't move the caret for this key press
                    QWidget::keyPressEvent(event);
                    return;
                }
            }
            // If no link is focused or it has no command, handle normally
            break;
        }
        case Qt::Key_C:
            if (QGuiApplication::keyboardModifiers().testFlag(Qt::ControlModifier)) {
                if (!QGuiApplication::keyboardModifiers().testFlag(Qt::ShiftModifier)) {
                    slot_copySelectionToClipboard();
                } else {
                    slot_copySelectionToClipboardHTML();
                }
            }
            break;
        case Qt::Key_Tab: {
            if ((mpHost->mCaretShortcut == Host::CaretShortcut::Tab && !(event->modifiers() & Qt::ControlModifier))
                || (mpHost->mCaretShortcut == Host::CaretShortcut::CtrlTab && (event->modifiers() & Qt::ControlModifier))) {
                mpHost->setCaretEnabled(false);
            }
            break;
        }

        case Qt::Key_F6: {
            if (mpHost->mCaretShortcut == Host::CaretShortcut::F6) {
                mpHost->setCaretEnabled(false);
            }
            break;
        }
    }

    // Did the key press change the caret position?
    if (newCaretLine == -1 && newCaretColumn == -1) {
        QWidget::keyPressEvent(event);
        return;
    }

    if (newCaretLine == -1) {
        newCaretLine = mCaretLine;
    }

    if (newCaretColumn == -1) {
        newCaretColumn = mCaretColumn;
    }

    setCaretPosition(newCaretLine, newCaretColumn);
}

int TTextEdit::offsetForPosition(int line, int column) const {
    int ret = 0;

    for (int i = 0; i < line; i++) {
        // The text() method adds a '\n' to the end of every line, so account
        // for it with the '+ 1' below.
        ret += mpBuffer->line(i).length() + 1;
    }

    ret += column;

    return ret;
}
