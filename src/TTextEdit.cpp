/***************************************************************************
 *   Copyright (C) 2008-2012 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2014-2016, 2018-2022 by Stephen Lyons                   *
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

#include "Announcer.h"
#include "TConsole.h"
#include "TDockWidget.h"
#include "TEvent.h"
#include "mudlet.h"
#if defined(Q_OS_WIN32)
#include "uiawrapper.h"
#endif
#include "widechar_width.h"

#include "pre_guard.h"
#include <chrono>
//#include <icecream.hpp>
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
#include <QScrollBar>
#include <QTextBoundaryFinder>
#include <QToolTip>
#include <QVersionNumber>
#include "post_guard.h"


TTextEdit::TTextEdit(TConsole* pC, QWidget* pW, TBuffer* pB, Host* pH, bool isLowerPane)
: QWidget(pW)
, mCursorY(0)
, mCursorX(0)
, mCaretLine(0)
, mCaretColumn(0)
, mOldCaretColumn(0)
, mIsCommandPopup(false)
, mIsTailMode(true)
, mShowTimeStamps(false)
, mForceUpdate(false)
, mIsLowerPane(isLowerPane)
, mLastRenderedOffset(0)
, mMouseTracking(false)
, mMouseTrackLevel(0)
, mOldScrollPos(0)
, mpBuffer(pB)
, mpConsole(pC)
, mpHost(pH)
, mScreenOffset(0)
, mMaxHRange(0)
, mWideAmbigousWidthGlyphs(pH->wideAmbiguousEAsianGlyphs())
, mTabStopwidth(8)
// Should be the same as the size of the timeStampFormat constant in the TBuffer
// class:
, mTimeStampWidth(13)
, mShowAllCodepointIssues(false)
, mMouseWheelRemainder()
{
    mLastClickTimer.start();
    if (pC->getType() != TConsole::CentralDebugConsole) {
        const auto hostFont = mpHost->getDisplayFont();
        mFontHeight = QFontMetrics(hostFont).height();
        mFontWidth = QFontMetrics(hostFont).averageCharWidth();

        mpHost->setDisplayFontFixedPitch(true);
        setFont(hostFont);

        // There is no point in setting this option on the Central Debug Console
        // as A) it is shared and B) any codepoints that it can't handle will
        // probably have already cropped up on another TConsole:
        if (!mIsLowerPane) {
            mShowAllCodepointIssues = mpHost->debugShowAllProblemCodepoints();
            connect(mpHost, &Host::signal_changeDebugShowAllProblemCodepoints, this, &TTextEdit::slot_changeDebugShowAllProblemCodepoints);
        }
    } else {
        // This is part of the Central Debug Console
        mShowTimeStamps = true;
        mFontHeight = QFontMetrics(mDisplayFont).height();
        mFontWidth = QFontMetrics(mDisplayFont).averageCharWidth();
        mFgColor = QColor(192, 192, 192);
        mBgColor = Qt::black;
        mDisplayFont = QFont(qsl("Bitstream Vera Sans Mono"), 14, QFont::Normal);
        mDisplayFont.setFixedPitch(true);
        setFont(mDisplayFont);
    }
    mScreenHeight = height() / mFontHeight;

    setMouseTracking(true);
    setFocusPolicy(Qt::NoFocus);
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


void TTextEdit::slot_toggleTimeStamps(const bool state)
{
    if (mShowTimeStamps != state) {
        mShowTimeStamps = state;
        forceUpdate();
        update();
    }
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
    if (mIsTailMode){
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
    if (isHidden()) {
        mFontWidth = QFontMetrics(mDisplayFont).averageCharWidth();
        mFontDescent = QFontMetrics(mDisplayFont).descent();
        mFontAscent = QFontMetrics(mDisplayFont).ascent();
        mFontHeight = mFontAscent + mFontDescent;
        return; //NOTE: otherwise mScreenHeight==0 would cause a floating point exception
    }
    // This was "if (pC->mType == TConsole::MainConsole) {"
    // and mIsMiniConsole is true for user created Mini Consoles and User Windows
    if (mpConsole->getType() == TConsole::MainConsole) {
        mFontWidth = QFontMetrics(mpHost->getDisplayFont()).averageCharWidth();
        mFontDescent = QFontMetrics(mpHost->getDisplayFont()).descent();
        mFontAscent = QFontMetrics(mpHost->getDisplayFont()).ascent();
        mFontHeight = mFontAscent + mFontDescent;
        mBgColor = mpHost->mBgColor;
        mFgColor = mpHost->mFgColor;
    } else {
        mFontWidth = QFontMetrics(mDisplayFont).averageCharWidth();
        mFontDescent = QFontMetrics(mDisplayFont).descent();
        mFontAscent = QFontMetrics(mDisplayFont).ascent();
        mFontHeight = mFontAscent + mFontDescent;
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


    if (QAccessible::isActive() && mpConsole->getType() == TConsole::MainConsole && mpHost->mAnnounceIncomingText
#if defined (Q_OS_WINDOWS)
            && UiaWrapper::self()->clientsAreListening()
#endif
            ) {
        QString newLines;
        //IC(previousOldScrollPos, mOldScrollPos);

        // content have been deleted
        if (previousOldScrollPos > mOldScrollPos) {
            QAccessibleTextInterface* ti = QAccessible::queryAccessibleInterface(this)->textInterface();
            auto totalCharacterCount = ti->characterCount();
            ti->setCursorPosition(totalCharacterCount);
            qDebug() << "moved cursor to" << totalCharacterCount << "; announcing DELETION at" << totalCharacterCount;
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

        QAccessibleTextInterface* ti = QAccessible::queryAccessibleInterface(this)->textInterface();

        // cursor has to be moved manually - https://doc.qt.io/qt-5/qaccessibletextinsertevent.html#QAccessibleTextInsertEvent
        auto totalCharacterCount = ti->characterCount();
        ti->setCursorPosition(totalCharacterCount);

        auto insertedat = totalCharacterCount - newLines.length();
        //QAccessibleTextInsertEvent event(this, insertedat, newLines);
        //qDebug() << "moved cursor to" << totalCharacterCount << "; announcing INSERTION at" << insertedat << newLines;
        //QAccessible::updateAccessibility(&event);

        // update for deletions and clearWindow() !!!
        // have to track the old text that was removed -_-

        mudlet::self()->announce(newLines);
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

// Extract the base (first) part which will be one or two QChars
// and if they ARE a surrogate pair convert them back to the single
// Unicode codepoint (needs around 21 bits, can be contained in a
// 32bit unsigned integer) value:
inline uint TTextEdit::getGraphemeBaseCharacter(const QString& str) const
{
    if (str.isEmpty()) {
        return 0;
    }

    QChar first = str.at(0);
    if (first.isSurrogate() && str.size() >= 2) {
        QChar second = str.at(1);
        if (first.isHighSurrogate() && second.isLowSurrogate()) {
            return QChar::surrogateToUcs4(first, second);
        }

        if (Q_UNLIKELY(first.isLowSurrogate() && second.isHighSurrogate())) {
            qDebug().noquote().nospace() << "TTextEdit::getGraphemeBaseCharacter(\"str\") INFO - passed a QString comprising a Low followed by a High surrogate QChar, this is not expected, they will be swapped around to try and recover but if this causes mojibake (text corrupted into meaningless symbols) please report this to the developers!";
            return QChar::surrogateToUcs4(second, first);
        }

        // str format error ?
        return first.unicode();
    }

    return first.unicode();
}

void TTextEdit::drawLine(QPainter& painter, int lineNumber, int lineOfScreen, int* offset) const
{
    QPoint cursor(-mCursorX, lineOfScreen);
    QString lineText = mpBuffer->lineBuffer.at(lineNumber);
    QTextBoundaryFinder boundaryFinder(QTextBoundaryFinder::Grapheme, lineText);
    int currentSize = lineText.size();
    if (mShowTimeStamps) {
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
        currentSize += mTimeStampWidth;
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
    if (mudlet::self()->isCaretModeEnabled() && mCaretLine == lineNumber && lineText.isEmpty()) {
        auto textRect = QRect(0, mFontHeight * lineOfScreen, mFontWidth, mFontHeight);
        painter.fillRect(textRect, mFgColor);
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
    uint unicode = getGraphemeBaseCharacter(grapheme);
    int charWidth = 0;

    switch (mpConsole->mControlCharacter) {
    default:
        // No special handling, except for these:
        if (Q_UNLIKELY(unicode == '\a' || unicode == '\t')) {
            if (unicode == '\t') {
                charWidth = mTabStopwidth - (column % mTabStopwidth);
                graphemes.append(QString(QChar::Tabulation));
            } else {
                // The alert character could make a sound when it is processed
                // in cTelnet::proccessSocketData(...) but it does not have a
                // visible representation - so lets give it one - a double
                // note:
                charWidth = 1;
                graphemes.append(QChar(0x266B));
            }

        } else {
            charWidth = getGraphemeWidth(unicode);
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

    TChar::AttributeFlags attributes = charStyle.allDisplayAttributes();
    QRect textRect;
    if (charWidth > 0) {
        textRect = QRect(mFontWidth * cursor.x(), mFontHeight * cursor.y(), mFontWidth * charWidth, mFontHeight);
    }
    textRects.append(textRect);
    QColor bgColor;
    bool caretIsHere = mudlet::self()->isCaretModeEnabled() && mCaretLine == line && mCaretColumn == column;
    if (Q_UNLIKELY(static_cast<bool>(attributes & TChar::Reverse) != (charStyle.isSelected() != caretIsHere))) {
        fgColors.append(charStyle.background());
        bgColor = charStyle.foreground();
    } else {
        fgColors.append(charStyle.foreground());
        bgColor = charStyle.background();
    }
    if (!textRect.isNull()) {
        painter.fillRect(textRect, bgColor);
    }
    return charWidth;
}

void TTextEdit::drawGraphemeForeground(QPainter& painter, const QColor& fgColor, const QRect& textRect, const QString& grapheme, TChar& charStyle) const
{
    TChar::AttributeFlags attributes = charStyle.allDisplayAttributes();
    const bool isBold = attributes & TChar::Bold;
    const bool isItalics = attributes & TChar::Italic;
    const bool isOverline = attributes & TChar::Overline;
    const bool isStrikeOut = attributes & TChar::StrikeOut;
    const bool isUnderline = attributes & TChar::Underline;
    if ((painter.font().bold() != isBold)
            || (painter.font().italic() != isItalics)
            || (painter.font().overline() != isOverline)
            || (painter.font().strikeOut() != isStrikeOut)
            || (painter.font().underline() != isUnderline)) {

        QFont font = painter.font();
        font.setBold(isBold);
        font.setItalic(isItalics);
        font.setOverline(isOverline);
        font.setStrikeOut(isStrikeOut);
        font.setUnderline(isUnderline);
        painter.setFont(font);
    }

    if (textRect.isNull()) {
        return;
    }

    if (painter.pen().color() != fgColor) {
        painter.setPen(fgColor);
    }
    painter.drawText(textRect.x(), textRect.bottom() - mFontDescent, grapheme);
}

int TTextEdit::getGraphemeWidth(uint unicode) const
{
    // https://github.com/ridiculousfish/widecharwidth/issues/11
    if (unicode == 0x1F6E1) {
        return 2;
    }

    switch (widechar_wcwidth(unicode)) {
    case 1: // Draw as normal/narrow
        return 1;
    case 2: // Draw as wide
        return 2;
    case widechar_nonprint:
        // -1 = The character is not printable - so put in a replacement
        // character instead - and so it can be seen it need a space:
        if (!mIsLowerPane) {
            bool newCodePointToWarnAbout = !mProblemCodepoints.contains(unicode);
            if (mShowAllCodepointIssues || newCodePointToWarnAbout) {
                qDebug().nospace().noquote() << "TTextEdit::getGraphemeWidth(...) WARN - trying to get width of a Unicode character which is unprintable, codepoint number: U+"
                                             << qsl("%1").arg(unicode, 4, 16, QLatin1Char('0')).toUtf8().constData() << ".";
            }
            if (Q_UNLIKELY(newCodePointToWarnAbout)) {
                mProblemCodepoints.insert(unicode, std::make_tuple(1, "Unprintable"));
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
            if (mShowAllCodepointIssues || newCodePointToWarnAbout) {
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
            if (mShowAllCodepointIssues || newCodePointToWarnAbout) {
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
    case widechar_ambiguous:
        // -3 = The character is East-Asian ambiguous width.
        return mWideAmbigousWidthGlyphs ? 2 : 1;
    case widechar_private_use:
        // -4 = The character is for private use - we cannot know for certain
        // what width to used - let's assume 1 for the moment:
        if (!mIsLowerPane) {
            bool newCodePointToWarnAbout = !mProblemCodepoints.contains(unicode);
            if (mShowAllCodepointIssues || newCodePointToWarnAbout) {
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
            if (mShowAllCodepointIssues || newCodePointToWarnAbout) {
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
    case widechar_widened_in_9: // -6 = Width is 1 in Unicode 8, 2 in Unicode 9+.
        return 2;
    default:
        return 1; // Got an uncoded return value from widechar_wcwidth(...)
    }
}

void TTextEdit::drawForeground(QPainter& painter, const QRect& r)
{
    qreal dpr = devicePixelRatioF();
    QPixmap screenPixmap;
    QPixmap pixmap = QPixmap(mScreenWidth * mFontWidth * dpr, mScreenHeight * mFontHeight * dpr);
    pixmap.setDevicePixelRatio(dpr);
    pixmap.fill(Qt::transparent);

    QPainter p(&pixmap);
    p.setCompositionMode(QPainter::CompositionMode_SourceOver);
    if (mpConsole->getType() == TConsole::MainConsole) {
        p.setFont(mpHost->getDisplayFont());
        p.setRenderHint(QPainter::TextAntialiasing, !mpHost->mNoAntiAlias);
    } else {
        p.setFont(mDisplayFont);
        p.setRenderHint(QPainter::TextAntialiasing, false);
    }

    QPoint P_topLeft = r.topLeft();
    QPoint P_bottomRight = r.bottomRight();

    int y_topLeft = P_topLeft.y();
    int x_bottomRight = P_bottomRight.x();
    int y_bottomRight = P_bottomRight.y();

    if (x_bottomRight > mScreenWidth * mFontWidth) {
        x_bottomRight = mScreenWidth * mFontWidth;
    }

    //    int x1 = x_topLeft / mFontWidth;
    int y1 = y_topLeft / mFontHeight;
    int x2 = x_bottomRight / mFontWidth;
    int y2 = y_bottomRight / mFontHeight;

    int lineOffset = imageTopLine();
    int from = 0;
    if (lineOffset == 0) {
        mScrollVector = 0;
    } else {
        // Was: mScrollVector = lineOffset - mLastRenderedOffset;
        if (mLastRenderedOffset) {
            mScrollVector = lineOffset - mLastRenderedOffset;
        } else {
            mScrollVector = y2 + lineOffset;
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
        if (!mForceUpdate && !mMouseTracking) {
            from = y1;
            noScroll = true;
            noCopy = true;
        } else {
            from = y1;
            y2 = mScreenHeight;
            noScroll = true;
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
            y2 = abs(mScrollVector);
        }
    }

    //delete non used characters.
    //needed for horizontal scrolling because there sometimes characters didn't get cleared
    QRect deleteRect = QRect(0, from * mFontHeight, x2 * mFontHeight, (y2 + 1) * mFontHeight);
    p.setCompositionMode(QPainter::CompositionMode_Source);
    p.fillRect(deleteRect, Qt::transparent);

    p.setCompositionMode(QPainter::CompositionMode_SourceOver);
    for (int i = from; i <= y2; ++i) {
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
}

void TTextEdit::unHighlight()
{
    // clang-format off
    for (int y = std::max(0, mPA.y()), endY = std::min((mPB.y() + 1), static_cast<int>(mpBuffer->buffer.size()));
         y < endY;
         ++y) {

        for (int x = (y == mPA.y()) ? std::max(0, mPA.x()) : 0,
                 endX = (y == (mPB.y()))
                     ? std::min((mPB.x() + 1), static_cast<int>(mpBuffer->buffer.at(y).size()))
                     : static_cast<int>(mpBuffer->buffer.at(y).size());
             x < endX;
             ++x) {

            mpBuffer->buffer.at(y).at(x).deselect();
        }
    }
    // clang-format on
}

// ensure that mPA is top-right and mPB is bottom-right
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
    for (; xind >= 0; --xind) {
        if (mpBuffer->lineBuffer.at(yind).at(xind) == QChar::Space
            || mpHost->mDoubleClickIgnore.contains(mpBuffer->lineBuffer.at(yind).at(xind))) {
            break;
        }
    }
    mDragStart.setX(xind + 1);
    mPA.setX(xind + 1);

    yind = mPB.y();
    xind = mPB.x();
    for (; xind < static_cast<int>(mpBuffer->lineBuffer.at(yind).size()); ++xind) {
        if (mpBuffer->lineBuffer.at(yind).at(xind) == QChar::Space
            || mpHost->mDoubleClickIgnore.contains(mpBuffer->lineBuffer.at(yind).at(xind))) {
            break;
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
    int lineIndex = std::max(0, (event->y() / mFontHeight) + imageTopLine());
    int tCharIndex = convertMouseXToBufferX(event->x(), lineIndex, &isOutOfbounds);

    updateTextCursor(event, lineIndex, tCharIndex, isOutOfbounds);

    if (!mMouseTracking) {
        return;
    }

    if (event->y() < 10) {
        mpConsole->scrollUp(3);
    }
    if (event->y() >= height() - 10) {
        mpConsole->scrollDown(3);
    }

    if (event->x() < 10) {
        scrollH(std::max(0, mCursorX - 2));
    }
    if (event->x() >= width() - 10) {
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

    if ((mDragStart.y() < cursorLocation.y() || (mDragStart.y() == cursorLocation.y() && mDragStart.x() < cursorLocation.x()))) {
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
    // FIXME: There is an issue now that deselecting a selection upwards in the
    // left column will leave the first column highlighted - it turns out that
    // those first columns are being deselected but the highlight() below is not
    // including the portion of the display with the now deselected portion on
    // the left margin within the area that gets repainted...
    highlightSelection();
    mDragSelectionEnd = cursorLocation;
    forceUpdate();
}

void TTextEdit::updateTextCursor(const QMouseEvent* event, int lineIndex, int tCharIndex, bool isOutOfbounds)
{
    if (lineIndex < static_cast<int>(mpBuffer->buffer.size())) {
        if (tCharIndex < static_cast<int>(mpBuffer->buffer[lineIndex].size())) {
            if (mpBuffer->buffer.at(lineIndex).at(tCharIndex).linkIndex() && !isOutOfbounds) {
                setCursor(Qt::PointingHandCursor);
                QStringList tooltip = mpBuffer->mLinkStore.getHints(mpBuffer->buffer.at(lineIndex).at(tCharIndex).linkIndex());
                QToolTip::showText(event->globalPos(), tooltip.join("\n"));
            } else {
                setCursor(Qt::IBeamCursor);
                QToolTip::hideText();
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
            const uint unicode = getGraphemeBaseCharacter(grapheme);
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
            if (Q_UNLIKELY(isOverTimeStamp && mShowTimeStamps && indexOfChar == 0)) {
                if ((mouseX + offset) < (mTimeStampWidth * mFontWidth)) {
                    // The mouse position is actually over the timestamp region
                    // to the left of the main text:
                    *isOverTimeStamp = true;
                }
            }

            leftX = rightX;
            //mCursorX relevant for horizontal scrollbars
            //Otherwise the value is always 0
            if (mShowTimeStamps) {
                rightX = (mTimeStampWidth + column - mCursorX) * mFontWidth;
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
    if (!pA) {
        return;
    }
    QString cmd;
    int luaReference{0};
    if (mPopupCommands.contains(pA->text())) {
        cmd = mPopupCommands[pA->text()].first;
        luaReference = mPopupCommands[pA->text()].second;
    }
    if (!luaReference) {
        mpHost->mLuaInterpreter.compileAndExecuteScript(cmd);
    } else {
        mpHost->mLuaInterpreter.callAnonymousFunction(luaReference, qsl("echoPopup"));
    }
}

void TTextEdit::mousePressEvent(QMouseEvent* event)
{
    //new event to get mouse position on the parent window
    QMouseEvent newEvent(event->type(), mpConsole->parentWidget()->mapFromGlobal(event->globalPos()), event->button(), event->buttons(), event->modifiers());
    if (mpConsole->getType() == TConsole::SubConsole) {
        qApp->sendEvent(mpConsole->parentWidget(), &newEvent);
    }

    if (mpConsole->getType() == TConsole::MainConsole || mpConsole->getType() == TConsole::UserWindow) {
        mpConsole->raiseMudletMousePressOrReleaseEvent(&newEvent, true);
    }

    if (event->button() == Qt::LeftButton) {
        int y = (event->y() / mFontHeight) + imageTopLine();
        int x = 0;
        y = std::max(y, 0);

        if (event->modifiers() & Qt::ControlModifier) {
            mCtrlSelecting = true;
        }

        bool isOutOfbounds = false;
        if (!mCtrlSelecting && mShowTimeStamps) {
            bool isOverTimeStamp = false;
            x = convertMouseXToBufferX(event->x(), y, &isOutOfbounds, &isOverTimeStamp);
            if (isOverTimeStamp) {
                // If we have clicked on the timestamp then emulate the effect
                // of control clicking - i.e. select the WHOLE line:
                mCtrlSelecting = true;
            }
        } else {
            x = convertMouseXToBufferX(event->x(), y, &isOutOfbounds);
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
                    QStringList command = mpBuffer->mLinkStore.getLinks(mpBuffer->buffer.at(y).at(x).linkIndex());
                    int luaReference = mpBuffer->mLinkStore.getReference(mpBuffer->buffer.at(y).at(x).linkIndex()).value(0, false);
                    QString func;
                    if (!command.empty()) {
                        func = command.at(0);
                        if (!luaReference){
                            mpHost->mLuaInterpreter.compileAndExecuteScript(func);
                        } else {
                            mpHost->mLuaInterpreter.callAnonymousFunction(luaReference, qsl("echoLink"));
                        }
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

            if (mMouseTrackLevel == 3){
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
        } else {
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
    }

    if (event->button() == Qt::RightButton) {
        int y = (event->y() / mFontHeight) + imageTopLine();
        y = std::max(y, 0);
        bool isOutOfbounds = false;
        int x = convertMouseXToBufferX(event->x(), y, &isOutOfbounds);

        if (y < static_cast<int>(mpBuffer->buffer.size())) {
            if (x < static_cast<int>(mpBuffer->buffer[y].size()) && !isOutOfbounds) {
                if (mpBuffer->buffer.at(y).at(x).linkIndex()) {
                    QStringList command = mpBuffer->mLinkStore.getLinks(mpBuffer->buffer.at(y).at(x).linkIndex());
                    QStringList hint = mpBuffer->mLinkStore.getHints(mpBuffer->buffer.at(y).at(x).linkIndex());
                    QVector<int> luaReference = mpBuffer->mLinkStore.getReference(mpBuffer->buffer.at(y).at(x).linkIndex());
                    if (command.size() > 1) {
                        auto popup = new QMenu(this);
                        for (int i = 0, total = command.size(); i < total; ++i) {
                            QAction* pA;
                            if (i < hint.size()) {
                                pA = popup->addAction(hint[i]);
                                mPopupCommands[hint[i]] = {command[i], luaReference.value(i, 0)};
                            } else {
                                pA = popup->addAction(command[i]);
                                mPopupCommands[command[i]] = {command[i], luaReference.value(i, 0)};
                            }
                            connect(pA, &QAction::triggered, this, &TTextEdit::slot_popupMenu);
                        }
                        popup->popup(event->globalPos());
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

        QAction* action3 = new QAction(tr("Select All"), this);
        action3->setToolTip(QString());
        connect(action3, &QAction::triggered, this, &TTextEdit::slot_selectAll);

        QString selectedEngine = mpHost->getSearchEngine().first;
        QAction* action4 = new QAction(tr("Search on %1").arg(selectedEngine), this);
        action4->setToolTip(QString());
        connect(action4, &QAction::triggered, this, &TTextEdit::slot_searchSelectionOnline);
        if (!qApp->testAttribute(Qt::AA_DontShowIconsInMenus)) {
            action->setIcon(QIcon::fromTheme(qsl("edit-copy"), QIcon(qsl(":/icons/edit-copy.png"))));
            action3->setIcon(QIcon::fromTheme(qsl("edit-select-all"), QIcon(qsl(":/icons/edit-select-all.png"))));
            action4->setIcon(QIcon::fromTheme(qsl("edit-web-search"), QIcon(qsl(":/icons/edit-web-search.png"))));
        }

        auto popup = new QMenu(this);
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
            connect(clearErrorConsole, &QAction::triggered, this, [=]() {
                mpConsole->buffer.clear();
                mpConsole->print(tr("*** starting new session ***\n"));
            });
            popup->addAction(clearErrorConsole);
        }

        // Add user actions
        QMapIterator<QString, QStringList> it(mpHost->mConsoleActions);
        while (it.hasNext()) {
            it.next();
            QStringList actionInfo = it.value();
            const QString &actionName = actionInfo.at(1);
            QAction * action = new QAction(actionName, this);
            action->setToolTip(actionInfo.at(2));
            popup->addAction(action);
            connect(action, &QAction::triggered, this, [this, actionName] { slot_mouseAction(actionName); });
        }
        popup->popup(mapToGlobal(event->pos()), action);
        event->accept();
        return;
    }

    if (event->button() == Qt::MiddleButton) {
        mpConsole->mLowerPane->mCursorY = mpConsole->buffer.size(); //
        mpConsole->mLowerPane->hide();
        mpBuffer->mCursorY = mpBuffer->size();
        mpConsole->mUpperPane->mCursorY = mpConsole->buffer.size(); //
        mpConsole->mUpperPane->mCursorX = 0;
        mpConsole->mUpperPane->mIsTailMode = true;
        mpConsole->mUpperPane->updateScreenView();
        mpConsole->mUpperPane->forceUpdate();
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
    if (!establishSelectedText()) {
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
    text.append(APP_BUILD);
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
                text.append(mpBuffer->bufferToHtml(mShowTimeStamps, y, mPB.x() + 1, mPA.x(), 0));
            } else { // Not single line
                text.append(mpBuffer->bufferToHtml(mShowTimeStamps, y, -1, mPA.x(), mPA.x()));
            }
        } else if (y == mPB.y()) { // Last line of selection
            text.append(mpBuffer->bufferToHtml(mShowTimeStamps, y, mPB.x() + 1));
        } else { // inside lines of selection
            text.append(mpBuffer->bufferToHtml(mShowTimeStamps, y));
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
        int lineWidth{(mShowTimeStamps ? mTimeStampWidth : 0) * mFontWidth};
        // Accumulated width in "normal" width characters:
        int column{};
        QTextBoundaryFinder boundaryFinder(QTextBoundaryFinder::Grapheme, lineText);
        for (int indexOfChar{}, total{lineText.size()}; indexOfChar < total;) {
            int nextBoundary{boundaryFinder.toNextBoundary()};
            // Width in "normal" width equivalent of this grapheme:
            int charWidth{};
            const QString grapheme = lineText.mid(indexOfChar, nextBoundary - indexOfChar);
            const uint unicode = getGraphemeBaseCharacter(grapheme);
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
            lineWidth = (mShowTimeStamps ? mTimeStampWidth + column : column) * mFontWidth;
            indexOfChar = nextBoundary;
        }
        largestLine = std::max(lineWidth, largestLine);
    }

    auto widthpx = std::min(65500, largestLine);
    auto rect = QRect(mPA.x(), mPA.y(), widthpx, heightpx);
    auto pixmap = QPixmap(widthpx, heightpx);
    pixmap.fill(mBgColor);

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
    if (mpConsole->getType() == TConsole::MainConsole) {
        painter.setFont(mpHost->getDisplayFont());
        painter.setRenderHint(QPainter::TextAntialiasing, !mpHost->mNoAntiAlias);
    } else {
        painter.setFont(mDisplayFont);
        painter.setRenderHint(QPainter::TextAntialiasing, false);
    }

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
    QString selectedText = getSelectedText(QChar::Space);
    QString url = QUrl::toPercentEncoding(selectedText.trimmed());
    url.prepend(mpHost->getSearchEngine().second);
    QDesktopServices::openUrl(QUrl(url));
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
    int startLine = std::max(0, mPA.y());
    int endLine = std::min(mPB.y(), (mpBuffer->lineBuffer.size() - 1));
    int offset = endLine - startLine;
    int startPos = std::max(0, mPA.x());
    int endPos = std::min(mPB.x(), (mpBuffer->lineBuffer.at(endLine).size() - 1));
    QStringList textLines = mpBuffer->lineBuffer.mid(startLine, endLine - startLine + 1);

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
    if (event->button() == Qt::LeftButton) {
        mMouseTracking = false;
        mCtrlSelecting = false;
    }
    QMouseEvent newEvent(event->type(), mpConsole->parentWidget()->mapFromGlobal(event->globalPos()), event->button(), event->buttons(), event->modifiers());

    if (mpConsole->getType() == TConsole::SubConsole) {
        qApp->sendEvent(mpConsole->parentWidget(), &newEvent);
        auto subConsoleParent = qobject_cast<TConsole*>(mpConsole->parent());
        if (subConsoleParent && subConsoleParent->mpDockWidget && subConsoleParent->mpDockWidget->isFloating()) {
            mpHost->mpConsole->activateWindow();
            mpHost->mpConsole->setFocus();
        }
    }

    if (mpConsole->getType() == TConsole::MainConsole || mpConsole->getType() == TConsole::UserWindow) {
        mpConsole->raiseMudletMousePressOrReleaseEvent(&newEvent, false);
    }
    if (mpConsole->getType() == TConsole::UserWindow && mpConsole->mpDockWidget && mpConsole->mpDockWidget->isFloating()) {
        // Need to take an extra step to return focus to main profile TConsole's
        // instance - using same method as TAction::execute():
        mpHost->mpConsole->activateWindow();
        mpHost->mpConsole->setFocus();
    }
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
    if (!mIsLowerPane && mpConsole->getType() == TConsole::MainConsole) {
        mpHost->updateDisplayDimensions();
    }

    QWidget::resizeEvent(event);
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

int TTextEdit::getColumnCount()
{
    int charWidth;

    if (mpConsole->getType() == TConsole::MainConsole) {
        charWidth = qRound(QFontMetricsF(mpHost->getDisplayFont()).averageCharWidth());
    } else {
        charWidth = qRound(QFontMetricsF(mDisplayFont).averageCharWidth());
    }

    return width() / charWidth;
}

int TTextEdit::getRowCount()
{
    int rowHeight;

    if (mpConsole->getType() == TConsole::MainConsole) {
        rowHeight = qRound(QFontMetricsF(mpHost->getDisplayFont()).lineSpacing());
    } else {
        rowHeight = qRound(QFontMetricsF(mDisplayFont).lineSpacing());
    }

    return height() / rowHeight;
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
        case QChar::Tabulation:         return htmlCenter(tr("{tab}", "Unicode U+0009 codepoint.")); break;
        case QChar::LineFeed:           return htmlCenter(tr("{line-feed}", "Unicode U+000A codepoint. Not likely to be seen as it gets filtered out.")); break;
        case QChar::CarriageReturn:     return htmlCenter(tr("{carriage-return}", "Unicode U+000D codepoint. Not likely to be seen as it gets filtered out.")); break;
        case QChar::Space:              return htmlCenter(tr("{space}", "Unicode U+0020 codepoint.")); break;
        case QChar::Nbsp:               return htmlCenter(tr("{non-breaking space}", "Unicode U+00A0 codepoint.")); break;
        case QChar::SoftHyphen:         return htmlCenter(tr("{soft hyphen}", "Unicode U+00AD codepoint.")); break;
        case 0x034F:                    return htmlCenter(tr("{combining grapheme joiner}", "Unicode U+034F codepoint (badly named apparently - see Wikipedia!)")); break;
        case 0x1680:                    return htmlCenter(tr("{ogham space mark}", "Unicode U+1680 codepoint.")); break;
        case 0x2000:                    return htmlCenter(tr("{'n' quad}", "Unicode U+2000 codepoint.")); break;
        case 0x2001:                    return htmlCenter(tr("{'m' quad}", "Unicode U+2001 codepoint.")); break;
        case 0x2002:                    return htmlCenter(tr("{'n' space}", "Unicode U+2002 codepoint - En ('n') wide space.")); break;
        case 0x2003:                    return htmlCenter(tr("{'m' space}", "Unicode U+2003 codepoint - Em ('m') wide space.")); break;
        case 0x2004:                    return htmlCenter(tr("{3-per-em space}", "Unicode U+2004 codepoint - three-per-em ('m') wide (thick) space.")); break;
        case 0x2005:                    return htmlCenter(tr("{4-per-em space}", "Unicode U+2005 codepoint - four-per-em ('m') wide (Middle) space.")); break;
        case 0x2006:                    return htmlCenter(tr("{6-per-em space}", "Unicode U+2006 codepoint - six-per-em ('m') wide (Sometimes the same as a Thin) space.")); break;
        case 0x2007:                    return htmlCenter(tr("{digit space}", "Unicode U+2007 codepoint - figure (digit) wide space.")); break;
        case 0x2008:                    return htmlCenter(tr("{punctuation wide space}", "Unicode U+2008 codepoint.")); break;
        case 0x2009:                    return htmlCenter(tr("{5-per-em space}", "Unicode U+2009 codepoint - five-per-em ('m') wide space.")); break;
        case 0x200A:                    return htmlCenter(tr("{hair width space}", "Unicode U+200A codepoint - thinnest space.")); break;
        case 0x200B:                    return htmlCenter(tr("{zero width space}", "Unicode U+200B codepoint.")); break;
        case 0x200C:                    return htmlCenter(tr("{Zero width non-joiner}", "Unicode U+200C codepoint.")); break;
        case 0x200D:                    return htmlCenter(tr("{zero width joiner}", "Unicode U+200D codepoint.")); break;
        case 0x200E:                    return htmlCenter(tr("{left-to-right mark}", "Unicode U+200E codepoint.")); break;
        case 0x200F:                    return htmlCenter(tr("{right-to-left mark}", "Unicode U+200F codepoint.")); break;
        case QChar::LineSeparator:      return htmlCenter(tr("{line separator}", "Unicode 0x2028 codepoint.")); break;
        case QChar::ParagraphSeparator: return htmlCenter(tr("{paragraph separator}", "Unicode U+2029 codepoint.")); break;
        case 0x202A:                    return htmlCenter(tr("{Left-to-right embedding}", "Unicode U+202A codepoint.")); break;
        case 0x202B:                    return htmlCenter(tr("{right-to-left embedding}", "Unicode U+202B codepoint.")); break;
        case 0x202C:                    return htmlCenter(tr("{pop directional formatting}", "Unicode U+202C codepoint - pop (undo last) directional formatting.")); break;
        case 0x202D:                    return htmlCenter(tr("{Left-to-right override}", "Unicode U+202D codepoint.")); break;
        case 0x202E:                    return htmlCenter(tr("{right-to-left override}", "Unicode U+202E codepoint.")); break;
        case 0x202F:                    return htmlCenter(tr("{narrow width no-break space}", "Unicode U+202F codepoint.")); break;
        case 0x205F:                    return htmlCenter(tr("{medium width mathematical space}", "Unicode U+205F codepoint.")); break;
        case 0x2060:                    return htmlCenter(tr("{zero width non-breaking space}", "Unicode U+2060 codepoint.")); break;
        case 0x2061:                    return htmlCenter(tr("{function application}", "Unicode U+2061 codepoint - function application (whatever that means!)")); break;
        case 0x2062:                    return htmlCenter(tr("{invisible times}", "Unicode U+2062 codepoint.")); break;
        case 0x2063:                    return htmlCenter(tr("{invisible separator}", "Unicode U+2063 codepoint - invisible separator or comma.")); break;
        case 0x2064:                    return htmlCenter(tr("{invisible plus}", "Unicode U+2064 codepoint.")); break;
        case 0x2066:                    return htmlCenter(tr("{left-to-right isolate}", "Unicode U+2066 codepoint.")); break;
        case 0x2067:                    return htmlCenter(tr("{right-to-left isolate}", "Unicode U+2067 codepoint.")); break;
        case 0x2068:                    return htmlCenter(tr("{first strong isolate}", "Unicode U+2068 codepoint.")); break;
        case 0x2069:                    return htmlCenter(tr("{pop directional isolate}", "Unicode U+2069 codepoint - pop (undo last) directional isolate.")); break;
        case 0x206A:                    return htmlCenter(tr("{inhibit symmetrical swapping}", "Unicode U+206A codepoint.")); break;
        case 0x206B:                    return htmlCenter(tr("{activate symmetrical swapping}", "Unicode U+206B codepoint.")); break;
        case 0x206C:                    return htmlCenter(tr("{inhibit arabic form-shaping}", "Unicode U+206C codepoint.")); break;
        case 0x206D:                    return htmlCenter(tr("{activate arabic form-shaping}", "Unicode U+206D codepoint.")); break;
        case 0x206E:                    return htmlCenter(tr("{national digit shapes}", "Unicode U+206E codepoint.")); break;
        case 0x206F:                    return htmlCenter(tr("{nominal Digit shapes}", "Unicode U+206F codepoint.")); break;
        case 0x3000:                    return htmlCenter(tr("{ideographic space}", "Unicode U+3000 codepoint - ideographic (CJK Wide) space")); break;
        case 0xFE00:                    return htmlCenter(tr("{variation selector 1}", "Unicode U+FE00 codepoint.")); break;
        case 0xFE01:                    return htmlCenter(tr("{variation selector 2}", "Unicode U+FE01 codepoint.")); break;
        case 0xFE02:                    return htmlCenter(tr("{variation selector 3}", "Unicode U+FE02 codepoint.")); break;
        case 0xFE03:                    return htmlCenter(tr("{variation selector 4}", "Unicode U+FE03 codepoint.")); break;
        case 0xFE04:                    return htmlCenter(tr("{variation selector 5}", "Unicode U+FE04 codepoint.")); break;
        case 0xFE05:                    return htmlCenter(tr("{variation selector 6}", "Unicode U+FE05 codepoint.")); break;
        case 0xFE06:                    return htmlCenter(tr("{variation selector 7}", "Unicode U+FE06 codepoint.")); break;
        case 0xFE07:                    return htmlCenter(tr("{variation selector 8}", "Unicode U+FE07 codepoint.")); break;
        case 0xFE08:                    return htmlCenter(tr("{variation selector 9}", "Unicode U+FE08 codepoint.")); break;
        case 0xFE09:                    return htmlCenter(tr("{variation selector 10}", "Unicode U+FE09 codepoint.")); break;
        case 0xFE0A:                    return htmlCenter(tr("{variation selector 11}", "Unicode U+FE0A codepoint.")); break;
        case 0xFE0B:                    return htmlCenter(tr("{variation selector 12}", "Unicode U+FE0B codepoint.")); break;
        case 0xFE0C:                    return htmlCenter(tr("{variation selector 13}", "Unicode U+FE0C codepoint.")); break;
        case 0xFE0D:                    return htmlCenter(tr("{variation selector 14}", "Unicode U+FE0D codepoint.")); break;
        case 0xFE0E:                    return htmlCenter(tr("{variation selector 15}", "Unicode U+FE0E codepoint - after an Emoji codepoint forces the textual (black & white) rendition.")); break;
        case 0xFE0F:                    return htmlCenter(tr("{variation selector 16}", "Unicode U+FE0F codepoint - after an Emoji codepoint forces the proper coloured 'Emoji' rendition.")); break;
        case 0xFEFF:                    return htmlCenter(tr("{zero width no-break space}", "Unicode U+FEFF codepoint - also known as the Byte-order-mark at start of text!).")); break;
        /*
         * case 0xFFF0:
         * to
         * case 0xFFF8: see default code-block
         */
        case 0xFFF9:                    return htmlCenter(tr("{interlinear annotation anchor}", "Unicode U+FFF9 codepoint.")); break;
        case 0xFFFA:                    return htmlCenter(tr("{interlinear annotation separator}", "Unicode U+FFFA codepoint.")); break;
        case 0xFFFB:                    return htmlCenter(tr("{interlinear annotation terminator}", "Unicode U+FFFB codepoint.")); break;
        case 0xFFFC:                    return htmlCenter(tr("{object replacement character}", "Unicode U+FFFC codepoint.")); break;
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
                return htmlCenter(tr("{noncharacter}", "Unicode codepoint in range U+FFD0 to U+FDEF - not a character.")); break;
            } else if ((value >= 0xFFF0 && value <= 0xFFF8) || value == 0xFFFE || value == 0xFFFF) {
                return htmlCenter(tr("{noncharacter}", "Unicode codepoint in range U+FFFx - not a character.")); break;
            } else {
                return htmlCenter(first);
            }
        }
    } else {
        // The code point is NOT on the BMP
        quint32 value = QChar::surrogateToUcs4(first, second);
        switch (value) {
        case 0x1F3FB:                   return htmlCenter(tr("{FitzPatrick modifier 1 or 2}", "Unicode codepoint U+0001F3FB - FitzPatrick modifier (Emoji Human skin-tone) 1-2.")); break;
        case 0x1F3FC:                   return htmlCenter(tr("{FitzPatrick modifier 3}", "Unicode codepoint U+0001F3FC - FitzPatrick modifier (Emoji Human skin-tone) 3.")); break;
        case 0x1F3FD:                   return htmlCenter(tr("{FitzPatrick modifier 4}", "Unicode codepoint U+0001F3FD - FitzPatrick modifier (Emoji Human skin-tone) 4.")); break;
        case 0x1F3FE:                   return htmlCenter(tr("{FitzPatrick modifier 5}", "Unicode codepoint U+0001F3FE - FitzPatrick modifier (Emoji Human skin-tone) 5.")); break;
        case 0x1F3FF:                   return htmlCenter(tr("{FitzPatrick modifier 6}", "Unicode codepoint U+0001F3FF - FitzPatrick modifier (Emoji Human skin-tone) 6.")); break;
        default:
            // The '%' is the modulus operator here:
            if ((value % 0x10000 == 0xFFFE) || (value % 0x10000 == 0xFFFF)) {
                return htmlCenter(tr("{noncharacter}", "Unicode codepoint is U+00xxFFFE or U+00xxFFFF - not a character.")); break;
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
                                .arg(qsl("%1").arg(QChar::surrogateToUcs4(mpBuffer->lineBuffer.at(line).at(index), mpBuffer->lineBuffer.at(line).at(index + 1)), 4, 16, zero).toUpper())
                                .arg(mpBuffer->lineBuffer.at(line).at(index).unicode(), 4, 16, zero)
                                .arg(mpBuffer->lineBuffer.at(line).at(index + 1).unicode(), 4, 16, zero));

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
                                         .arg(mpBuffer->lineBuffer.at(line).at(index).unicode(), 4, 16, QChar('0'))
                                         .toUpper());

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
                                .arg(tr("Index (UTF-16)",
                                        "1st Row heading for Text analyser output, table item is the count into the QChars/TChars that make up the text {this translation used 2 times}"),
                                     utf16indexes)
                                .arg(tr("U+<i>####</i> Unicode Code-point <i>(High:Low Surrogates)</i>",
                                        "2nd Row heading for Text analyser output, table item is the unicode code point (will be "
                                        "between 000001 and 10FFFF in hexadecimal) {this translation used 2 times}"),
                                     utf16Vals)
                                .arg(tr("Visual",
                                        "3rd Row heading for Text analyser output, table item is a visual representation of the character/part of the character or a '{'...'}' wrapped "
                                        "letter code if the character is whitespace or otherwise unshowable {this translation used 2 times}"),
                                     graphemes)
                                .arg(tr("Index (UTF-8)",
                                        "4th Row heading for Text analyser output, table item is the count into the bytes that make up the UTF-8 form of the text that the Lua system "
                                        "uses {this translation used 2 times}"),
                                     utf8Indexes)
                                .arg(tr("Byte",
                                        "5th Row heading for Text analyser output, table item is the unsigned 8-bit integer for the particular byte in the UTF-8 form of the text that the Lua "
                                        "system uses {this translation used 2 times}"),
                                     utf8Vals)
                                .arg(tr("Lua character or code",
                                        "6th Row heading for Text analyser output, table item is either the ASCII character or the numeric code for the byte in the row about "
                                        "this item in the table, as displayed the thing shown can be used in a Lua string entry to reproduce this byte {this translation used "
                                        "2 times}"),
                                     luaCodes);
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
                        .arg(tr("Index (UTF-16)", "1st Row heading for Text analyser output, table item is the count into the QChars/TChars that make up the text {this translation used 2 times}"),
                             utf16indexes)
                        .arg(tr("U+<i>####</i> Unicode Code-point <i>(High:Low Surrogates)</i>",
                                "2nd Row heading for Text analyser output, table item is the unicode code point (will be between "
                                "000001 and 10FFFF in hexadecimal) {this translation used 2 times}"),
                             utf16Vals)
                        .arg(tr("Visual",
                                "3rd Row heading for Text analyser output, table item is a visual representation of the character/part of the character or a '{'...'}' wrapped letter "
                                "code if the character is whitespace or otherwise unshowable {this translation used 2 times}"),
                             graphemes)
                        .arg(tr("Index (UTF-8)",
                                "4th Row heading for Text analyser output, table item is the count into the bytes that make up the UTF-8 form of the text that the Lua system "
                                "uses {this translation used 2 times}"),
                             utf8Indexes)
                        .arg(tr("Byte",
                                "5th Row heading for Text analyser output, table item is the unsigned 8-bit integer for the particular byte in the UTF-8 form of the text that the Lua "
                                "system uses {this translation used 2 times}"),
                             utf8Vals)
                        .arg(tr("Lua character or code",
                                "6th Row heading for Text analyser output, table item is either the ASCII character or the numeric code for the byte in the row about "
                                "this item in the table, as displayed the thing shown can be used in a Lua string entry to reproduce this byte {this translation used 2 "
                                "times}"),
                             luaCodes));
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

void TTextEdit::slot_changeDebugShowAllProblemCodepoints(const bool state)
{
    if (mShowAllCodepointIssues != state) {
        mShowAllCodepointIssues = state;
    }
}

void TTextEdit::slot_mouseAction(const QString &uniqueName)
{
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

void TTextEdit::setCaretPosition(int line, int column)
{
    qDebug() << "TTextEdit::setCaretPosition(" << line << ", " << column << ")";
    mCaretLine = line;
    mCaretColumn = column;

    if (!mudlet::self()->isCaretModeEnabled()) {
        return;
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
            scrollTo(mCaretLine);
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

        qDebug() << "raising" << event;
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
    if (!mudlet::self()->isCaretModeEnabled()) {
        QWidget::keyPressEvent(event);
        return;
    }

    int newCaretLine = -1;
    int newCaretColumn = -1;
    qDebug() << "before keypress:" << mCaretLine << mCaretColumn;

    switch (event->key()) {
    case Qt::Key_Up:
        if (mCaretLine > 0) {
            newCaretLine = mCaretLine - 1;
        }
        break;
    case Qt::Key_Down: {
            // FIXME: Is the last line in lineBuffer always empty?
            int emptyLastLine = mpBuffer->lineBuffer.last().isEmpty();
            if (mCaretLine < mpBuffer->lineBuffer.length() - 1 - emptyLastLine) {
                newCaretLine = mCaretLine + 1;
            }
        }
        break;
    case Qt::Key_Left:
        if (mCaretColumn > 0) {
            newCaretColumn = mCaretColumn - 1;
        }
        break;
    case Qt::Key_Right:
        if (mCaretColumn < mpBuffer->lineBuffer[mCaretLine].length() - 1) {
            newCaretColumn = mCaretColumn + 1;
        }
        break;
    case Qt::Key_Home:
        newCaretColumn = 0;
        break;
    case Qt::Key_End:
        newCaretColumn = mpBuffer->lineBuffer[mCaretLine].length() - 1;
        break;
    case Qt::Key_PageUp:
        newCaretLine = std::max(mCaretLine - mScreenHeight, 0);
        break;
    case Qt::Key_PageDown:
        newCaretLine = std::min(mCaretLine + mScreenHeight, mpBuffer->lineBuffer.length() - 2);
        break;
    }

    // Did the key press change the caret position?
    if (newCaretLine == -1 && newCaretColumn == -1) {
        QWidget::keyPressEvent(event);
        return;
    }

    if (newCaretLine == -1) {
        newCaretLine = mCaretLine;
    } else {
        // If the new line is shorter, we need to adjust the column.
        int newLineLength = mpBuffer->line(newCaretLine).length();
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
    }

    if (newCaretColumn == -1) {
        newCaretColumn = mCaretColumn;
    }

    setCaretPosition(newCaretLine, newCaretColumn);

    qDebug() << "after keypress:" << mCaretLine << mCaretColumn;
}
