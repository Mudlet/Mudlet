/***************************************************************************
 *   Copyright (C) 2008-2012 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2014-2016, 2018-2019 by Stephen Lyons                   *
 *                                               - slysven@virginmedia.com *
 *   Copyright (C) 2016-2017 by Ian Adkins - ieadkins@gmail.com            *
 *   Copyright (C) 2017 by Chris Reid - WackyWormer@hotmail.com            *
 *   Copyright (C) 2018 by Huadong Qi - novload@outlook.com                *
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


#include "TTextEdit.h"

#include "TConsole.h"
#include "TDockWidget.h"
#include "TEvent.h"
#include "mudlet.h"
#include "wcwidth.h"

#include "pre_guard.h"
#include <QtEvents>
#include <QtGlobal>
#include <QApplication>
#include <QChar>
#include <QClipboard>
#include <QDesktopServices>
#include <QPainter>
#include <QScrollBar>
#include <QTextBoundaryFinder>
#include <QToolTip>
#include "post_guard.h"
#include <chrono>


TTextEdit::TTextEdit(TConsole* pC, QWidget* pW, TBuffer* pB, Host* pH, bool isLowerPane)
: QWidget(pW)
, mCursorY(0)
, mIsCommandPopup(false)
, mIsTailMode(true)
, mShowTimeStamps(false)
, mForceUpdate(false)
, mIsLowerPane(isLowerPane)
, mLastRenderBottom(0)
, mMouseTracking(false)
, mpBuffer(pB)
, mpConsole(pC)
, mpHost(pH)
, mpScrollBar(nullptr)
, mWideAmbigousWidthGlyphs(pH->wideAmbiguousEAsianGlyphs())
{
    mLastClickTimer.start();
    if (pC->getType() != TConsole::CentralDebugConsole) {
        mFontHeight = QFontMetrics(mpHost->mDisplayFont).height();
        mFontWidth = QFontMetrics(mpHost->mDisplayFont).width(QChar('W'));
        mScreenWidth = 100;
        if ((width() / mFontWidth) < mScreenWidth) {
            mScreenWidth = 100; //width()/mFontWidth;
        }

        mpHost->mDisplayFont.setFixedPitch(true);
#if defined(Q_OS_MACOS) || defined(Q_OS_LINUX)
        QPixmap pixmap = QPixmap(mScreenWidth * mFontWidth * 2, mFontHeight * 2);
        QPainter p(&pixmap);
        p.setFont(mpHost->mDisplayFont);
        const QRectF r = QRectF(0, 0, mScreenWidth * mFontWidth * 2, mFontHeight * 2);
        QRectF r2;
        const QString t = "1234";
        p.drawText(r, 1, t, &r2);
        mLetterSpacing = (qreal)((qreal)mFontWidth - (qreal)(r2.width() / t.size()));
        mpHost->mDisplayFont.setLetterSpacing(QFont::AbsoluteSpacing, mLetterSpacing);
#endif
        setFont(mpHost->mDisplayFont);
    } else {
        // This is part of the Central Debug Console
        mShowTimeStamps = true;
        mFontHeight = QFontMetrics(mDisplayFont).height();
        mFontWidth = QFontMetrics(mDisplayFont).width(QChar('W'));
        mScreenWidth = 100;
        mDisplayFont.setFixedPitch(true);
#if defined(Q_OS_MACOS) || defined(Q_OS_LINUX)
        QPixmap pixmap = QPixmap(mScreenWidth * mFontWidth * 2, mFontHeight * 2);
        QPainter p(&pixmap);
        p.setFont(mDisplayFont);
        const QRectF r = QRectF(0, 0, mScreenWidth * mFontWidth * 2, mFontHeight * 2);
        QRectF r2;
        const QString t = "1234";
        p.drawText(r, 1, t, &r2);
        mLetterSpacing = (qreal)((qreal)mFontWidth - (qreal)(r2.width() / t.size()));
        mDisplayFont.setLetterSpacing(QFont::AbsoluteSpacing, mLetterSpacing);
#endif
        setFont(mDisplayFont);
        // initialize after mFontHeight and mFontWidth have been set, because the function uses them!
        initDefaultSettings();
    }
    mScreenHeight = height() / mFontHeight;

    mScreenWidth = 100;

    setMouseTracking(true);
    setFocusPolicy(Qt::NoFocus);
    QCursor cursor;
    cursor.setShape(Qt::IBeamCursor);
    setCursor(cursor);
    setAttribute(Qt::WA_OpaquePaintEvent); //was disabled
    setAttribute(Qt::WA_DeleteOnClose);

    QPalette palette;
    palette.setColor(QPalette::Text, mFgColor);
    palette.setColor(QPalette::Highlight, QColor(55, 55, 255));
    palette.setColor(QPalette::Base, mBgColor);
    setPalette(palette);
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


void TTextEdit::slot_toggleTimeStamps()
{
    mShowTimeStamps = !mShowTimeStamps;
    forceUpdate();
    update();
}

void TTextEdit::slot_scrollBarMoved(int line)
{
    if (mpConsole->mpScrollBar) {
        disconnect(mpConsole->mpScrollBar, &QAbstractSlider::valueChanged, this, &TTextEdit::slot_scrollBarMoved);
        mpConsole->mpScrollBar->setRange(0, mpBuffer->getLastLineNumber());
        mpConsole->mpScrollBar->setSingleStep(1);
        mpConsole->mpScrollBar->setPageStep(mScreenHeight);
        mpConsole->mpScrollBar->setValue(line);
        scrollTo(line);
        connect(mpConsole->mpScrollBar, &QAbstractSlider::valueChanged, this, &TTextEdit::slot_scrollBarMoved);
    }
}

void TTextEdit::initDefaultSettings()
{
    mFgColor = QColor(192, 192, 192);
    mBgColor = QColor(Qt::black);
    mDisplayFont = QFont("Bitstream Vera Sans Mono", 10, QFont::Normal);
#if defined(Q_OS_MACOS) || defined(Q_OS_LINUX)
    int width = mScreenWidth * mFontWidth * 2;
    int height = mFontHeight * 2;
    // sometimes mScreenWidth is 0, and QPainter doesn't like dimensions of 0x#. Need to work out why is
    // mScreenWidth ever zero and it gets used in the follow calculations.
    if (width > 0 && height > 0) {
        QPixmap pixmap = QPixmap(width, height);
        QPainter p(&pixmap);
        p.setFont(mDisplayFont);
        const QRectF r = QRectF(0, 0, width, height);
        QRectF r2;
        const QString t = "1234";
        p.drawText(r, 1, t, &r2);
        mLetterSpacing = (qreal)((qreal)mFontWidth - (qreal)(r2.width() / t.size()));
        mDisplayFont.setLetterSpacing(QFont::AbsoluteSpacing, mLetterSpacing);
    }
#endif
    mDisplayFont.setLetterSpacing(QFont::AbsoluteSpacing, mLetterSpacing);
    mDisplayFont.setFixedPitch(true);
    setFont(mDisplayFont);
    mWrapAt = 100;
    mWrapIndentCount = 5;
}

void TTextEdit::updateScreenView()
{
    if (isHidden()) {
        mFontWidth = QFontMetrics(mDisplayFont).width(QChar(' '));
        mFontDescent = QFontMetrics(mDisplayFont).descent();
        mFontAscent = QFontMetrics(mDisplayFont).ascent();
        mFontHeight = mFontAscent + mFontDescent;
#if defined(Q_OS_MACOS) || defined(Q_OS_LINUX)
        QPixmap pixmap = QPixmap(2000, 600);
        QPainter p(&pixmap);
        mDisplayFont.setLetterSpacing(QFont::AbsoluteSpacing, 0);
        if (!p.isActive()) {
            return;
        }
        p.setFont(mDisplayFont);
        const QRectF r = QRectF(0, 0, 2000, 600);
        QRectF r2;
        const QString t = "1234";
        p.drawText(r, 1, t, &r2);
        mLetterSpacing = (qreal)((qreal)mFontWidth - (qreal)(r2.width() / t.size()));
        mDisplayFont.setLetterSpacing(QFont::AbsoluteSpacing, mLetterSpacing);
#endif
        return; //NOTE: das ist wichtig, damit ich keine floating point exception bekomme, wenn mScreenHeight==0, was hier der Fall wäre
    }
    // This was "if (pC->mType == TConsole::MainConsole) {"
    // and mIsMiniConsole is true for user created Mini Consoles and User Windows
    if (mpConsole->getType() == TConsole::MainConsole) {
        mFontWidth = QFontMetrics(mpHost->mDisplayFont).width(QChar('W'));
        mFontDescent = QFontMetrics(mpHost->mDisplayFont).descent();
        mFontAscent = QFontMetrics(mpHost->mDisplayFont).ascent();
        mFontHeight = mFontAscent + mFontDescent;
        mBgColor = mpHost->mBgColor;
        mFgColor = mpHost->mFgColor;
#if defined(Q_OS_MACOS) || defined(Q_OS_LINUX)
        QPixmap pixmap = QPixmap(mScreenWidth * mFontWidth * 2, mFontHeight * 2);
        QPainter p(&pixmap);
        mpHost->mDisplayFont.setLetterSpacing(QFont::AbsoluteSpacing, 0);
        if (p.isActive()) {
            p.setFont(mpHost->mDisplayFont);
            const QRectF r = QRectF(0, 0, mScreenWidth * mFontWidth * 2, mFontHeight * 2);
            QRectF r2;
            const QString t = "1234";
            p.drawText(r, 1, t, &r2);
            mLetterSpacing = (qreal)((qreal)mFontWidth - (qreal)(r2.width() / t.size()));
            mpHost->mDisplayFont.setLetterSpacing(QFont::AbsoluteSpacing, mLetterSpacing);
        }
#endif
    } else {
        mFontWidth = QFontMetrics(mDisplayFont).width(QChar('W'));
        mFontDescent = QFontMetrics(mDisplayFont).descent();
        mFontAscent = QFontMetrics(mDisplayFont).ascent();
        mFontHeight = mFontAscent + mFontDescent;
#if defined(Q_OS_MACOS) || defined(Q_OS_LINUX)
        int width = mScreenWidth * mFontWidth * 2;
        int height = mFontHeight * 2;
        // sometimes mScreenWidth is 0, and QPainter doesn't like dimensions of 0x#. Need to work out why is
        // mScreenWidth ever zero and it gets used in the follow calculations.
        if (width > 0 && height > 0) {
            QPixmap pixmap = QPixmap(width, height);
            QPainter p(&pixmap);
            mDisplayFont.setLetterSpacing(QFont::AbsoluteSpacing, 0);
            if (p.isActive()) {
                p.setFont(mDisplayFont);
                const QRectF r = QRectF(0, 0, width, height);
                QRectF r2;
                const QString t = "1234";
                p.drawText(r, 1, t, &r2);
                mLetterSpacing = (qreal)((qreal)mFontWidth - (qreal)(r2.width() / t.size()));
                mDisplayFont.setLetterSpacing(QFont::AbsoluteSpacing, mLetterSpacing);
            }
        }
#endif
    }
    mScreenHeight = visibleRegion().boundingRect().height() / mFontHeight;
    int currentScreenWidth = visibleRegion().boundingRect().width() / mFontWidth;
    if (mpConsole->getType() == TConsole::MainConsole) {
        // This is the MAIN console - we do not want it to ever disappear!
        mScreenWidth = qMax(40, currentScreenWidth);

        // Note the values in the "parent" Host instance
        mpHost->mScreenWidth = mScreenWidth;
        mpHost->mScreenHeight = mScreenHeight;
    } else {
        mScreenWidth = currentScreenWidth;
    }
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
    if (!mIsLowerPane) {
        mpBuffer->mCursorY = mpBuffer->size();
    }

    mOldScrollPos = mpBuffer->getLastLineNumber();

    if (!mIsLowerPane) {
        // This is ONLY for the upper pane
        if (mpConsole->mpScrollBar && mOldScrollPos > 0) {
            disconnect(mpConsole->mpScrollBar, &QAbstractSlider::valueChanged, this, &TTextEdit::slot_scrollBarMoved);
            mpConsole->mpScrollBar->setRange(0, mpBuffer->getLastLineNumber());
            mpConsole->mpScrollBar->setSingleStep(1);
            mpConsole->mpScrollBar->setPageStep(mScreenHeight);
            if (mIsTailMode) {
                mpConsole->mpScrollBar->setValue(mpBuffer->mCursorY);
            }
            connect(mpConsole->mpScrollBar, &QAbstractSlider::valueChanged, this, &TTextEdit::slot_scrollBarMoved);
        }
    }
    update();
}

void TTextEdit::scrollTo(int line)
{
    // Protect against modifying mIsTailMode on the lower pane where it would
    // be wrong:
    Q_ASSERT_X(!mIsLowerPane, "Inappropriate use of method on lower pane which should only be used for the upper one", "TTextEdit::scrollTo()");

    if ((line > -1) && (line < mpBuffer->size())) {
        if ((line < (mpBuffer->getLastLineNumber() - mScreenHeight) && mIsTailMode)) {
            mIsTailMode = false;
            mpConsole->mLowerPane->mCursorY = mpBuffer->size();
            mpConsole->mLowerPane->show();
            mpConsole->mLowerPane->forceUpdate();
        } else if ((line > (mpBuffer->getLastLineNumber() - mScreenHeight)) && !mIsTailMode) {
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

void TTextEdit::scrollUp(int lines)
{
    if (mIsLowerPane) {
        return;
    }

    if (bufferScrollUp(lines)) {
        mIsTailMode = false;
        mScrollVector = 0;
        update();
    }
}

void TTextEdit::scrollDown(int lines)
{
    if (mIsLowerPane) {
        return;
    }

    if (bufferScrollDown(lines)) {
        mScrollVector = 0;
        update();
    }
}

inline void TTextEdit::drawBackground(QPainter& painter, const QRect& rect, const QColor& bgColor) const
{
    QRect bR = rect;
    painter.fillRect(bR.x(), bR.y(), bR.width(), bR.height(), bgColor);
}

inline void TTextEdit::drawCharacters(QPainter& painter, const QRect& rect, QString& text, const QColor& fgColor, const TChar::AttributeFlags flags)
{
    if ( (painter.font().bold() != (flags & TChar::Bold))
       ||(painter.font().underline() != (flags & TChar::Underline))
       ||(painter.font().italic() != (flags & TChar::Italic))
       ||(painter.font().strikeOut() != (flags & TChar::StrikeOut))
       ||(painter.font().overline() != (flags & TChar::Overline))) {

        QFont font = painter.font();
        font.setBold(flags & TChar::Bold);
        font.setUnderline(flags & TChar::Underline);
        font.setItalic(flags & TChar::Italic);
        font.setStrikeOut(flags & TChar::StrikeOut);
        font.setOverline(flags & TChar::Overline);
#if defined(Q_OS_MACOS) || defined(Q_OS_LINUX)
        font.setLetterSpacing(QFont::AbsoluteSpacing, mLetterSpacing);
#endif
        painter.setFont(font);
    }

    if (painter.pen().color() != fgColor) {
        painter.setPen(fgColor);
    }

#if defined(Q_OS_MACOS) || defined(Q_OS_LINUX)
    QPointF _p(rect.x(), rect.bottom() - mFontDescent);
    painter.drawText(_p, text);
#else
    // The second argument is the y-position and is the baseline of the text:
    painter.drawText(rect.x(), rect.bottom() - mFontDescent, text);
#endif
}

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
        } else if (first.isLowSurrogate() && second.isHighSurrogate()) {
            return QChar::surrogateToUcs4(second, first);
        } else {
            // str format error
            return first.unicode();
        }
    } else {
        return first.unicode();
    }
}

void TTextEdit::drawLine(QPainter& painter, int lineNumber, int lineOfScreen) const
{
    QPoint cursor(0, lineOfScreen);
    QString lineText = mpBuffer->lineBuffer.at(lineNumber);
    QTextBoundaryFinder boundaryFinder(QTextBoundaryFinder::Grapheme, lineText);

    if (mShowTimeStamps) {
        TChar timeStampStyle(QColor(200, 150, 0), QColor(22, 22, 22));
        QString timestamp(mpBuffer->timeBuffer.at(lineNumber));
        for (QChar c : timestamp) {
            cursor.setX(cursor.x() + drawGrapheme(painter, cursor, c, 0, timeStampStyle));
        }
    }

    int columnWithOutTimestamp = 0;
    for (int indexOfChar = 0, total = lineText.size(); indexOfChar < total;) {
        int nextBoundary = boundaryFinder.toNextBoundary();

        TChar &charStyle = mpBuffer->buffer.at(lineNumber).at(indexOfChar);
        int graphemeWidth = drawGrapheme(painter, cursor, lineText.mid(indexOfChar, nextBoundary - indexOfChar), columnWithOutTimestamp, charStyle);
        cursor.setX(cursor.x() + graphemeWidth);
        indexOfChar = nextBoundary;
        columnWithOutTimestamp += graphemeWidth;
    }
}

/**
 * @brief TTextEdit::drawGrapheme
 * @param painter
 * @param cursor
 * @param grapheme
 * @param column Used to calculate the width of Tab
 * @param charStyle
 * @return Return the display width of the grapheme
 */
int TTextEdit::drawGrapheme(QPainter& painter, const QPoint& cursor, const QString& grapheme, int column, TChar& charStyle) const
{
    uint unicode = getGraphemeBaseCharacter(grapheme);
    int charWidth;
    if (unicode == '\t') {
        charWidth = column / 8 * 8 + 8;
    } else {
        // mk_wcwidth returns -1 (on error), 0 on a control or combining
        // diacritical codepoint or 1 for a normal character or 2 on a wide
        // character.
        // mk_wcwidth_cjk does the same except it returns 2 instead of 1 for
        // characters that have an "ambiguous" East Asian width:
        switch (mWideAmbigousWidthGlyphs ? mk_wcwidth_cjk(unicode) : mk_wcwidth(unicode)) {
        case 2: // Draw as wide
            charWidth = 2;
            break;
        case 1: // Draw as normal/narrow
            charWidth = 1;
            break;
        case 0: // Control or Combining Diacritial - should not occur:
            qWarning() << "TTextEdit::drawGrapheme(...) WARN - trying to draw UCS4 character which should be zero width and not a base character: " << unicode;
            charWidth = 1; // Previous code treated this as a normal width character
            break;
        default: // case -1: is an error condition detected in the lower code
            qWarning() << "TTextEdit::drawGrapheme(...) WARN - trying to draw UCS4 character which should be an error: " << unicode;
            charWidth = 1; // Previous code treated this as a normal width character
            break;
        }
    }

    TChar::AttributeFlags attributes = charStyle.allDisplayAttributes();
    bool isBold = attributes & TChar::Bold;
    bool isItalics = attributes & TChar::Italic;
    bool isOverline = attributes & TChar::Overline;
    bool isStrikeOut = attributes & TChar::StrikeOut;
    bool isUnderline = attributes & TChar::Underline;
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
#if defined(Q_OS_MACOS) || defined(Q_OS_LINUX)
        font.setLetterSpacing(QFont::AbsoluteSpacing, mLetterSpacing);
#endif
        painter.setFont(font);
    }

    QColor bgColor;
    QColor fgColor;
    if (static_cast<bool>(attributes & TChar::Reverse) != charStyle.isSelected()) {
        fgColor = charStyle.background();
        bgColor = charStyle.foreground();
    } else {
        fgColor = charStyle.foreground();
        bgColor = charStyle.background();
    }

    auto textRect = QRect(mFontWidth * cursor.x(), mFontHeight * cursor.y(), mFontWidth * charWidth, mFontHeight);
    drawBackground(painter, textRect, bgColor);

    if (painter.pen().color() != fgColor) {
        painter.setPen(fgColor);
    }

    painter.drawText(textRect.x(), textRect.bottom() - mFontDescent, grapheme);
    return charWidth;
}

void TTextEdit::drawForeground(QPainter& painter, const QRect& r)
{
    qreal dpr = devicePixelRatioF();
    QPixmap screenPixmap;
    QPixmap pixmap = QPixmap(mScreenWidth * mFontWidth * dpr, mScreenHeight * mFontHeight * dpr);
    pixmap.setDevicePixelRatio(dpr);
    pixmap.fill(palette().base().color());

    QPainter p(&pixmap);
    p.setCompositionMode(QPainter::CompositionMode_Source);
    if (mpConsole->getType() == TConsole::MainConsole) {
        p.setFont(mpHost->mDisplayFont);
        p.setRenderHint(QPainter::TextAntialiasing, !mpHost->mNoAntiAlias);
    } else {
        p.setFont(mDisplayFont);
        p.setRenderHint(QPainter::TextAntialiasing, false);
    }

    QPoint P_topLeft = r.topLeft();
    QPoint P_bottomRight = r.bottomRight();
    int x_topLeft = 0;
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
        // Was: mScrollVector = lineOffset - mLastRenderBottom;
        if (mLastRenderBottom) {
            mScrollVector = lineOffset - mLastRenderBottom;
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
    QRect deleteRect = QRect(0, from * mFontHeight, x2 * mFontHeight, (y2 + 1) * mFontHeight);
    drawBackground(p, deleteRect, mBgColor);
    for (int i = from; i <= y2; ++i) {
        if (static_cast<int>(mpBuffer->buffer.size()) <= i + lineOffset) {
            break;
        }
        mpBuffer->dirty[lineOffset + i] = false;
        drawLine(p, i + lineOffset, i);
    }
    p.end();
    painter.setCompositionMode(QPainter::CompositionMode_Source);
    painter.drawPixmap(0, 0, pixmap);
    if (!noCopy) {
        mScreenMap = pixmap.copy();
    }
    mScrollVector = 0;
    mLastRenderBottom = lineOffset;
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
        if (mScreenHeight <= 0) {
            return;
        }
        mScreenWidth = 100;
    }

    QPainter painter(this);
    if (!painter.isActive()) {
        return;
    }

    QRect borderRect = QRect(0, mScreenHeight * mFontHeight, rect.width(), rect.height());
    drawBackground(painter, borderRect, mBgColor);
    QRect borderRect2 = QRect(rect.width() - mScreenWidth, 0, rect.width(), rect.height());
    drawBackground(painter, borderRect2, mBgColor);
    drawForeground(painter, rect);
}


void TTextEdit::highlight()
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
        QRect rectFirstLine(mPA.x() * mFontWidth, (mPA.y() - imageTopLine()) * mFontHeight, (mPB.x() - mPA.x()) * mFontWidth, mFontHeight);
        newRegion += rectFirstLine;
    }

    QRect _r = mSelectedRegion.boundingRect();
    if (lineDelta < 0) {
        _r.setWidth(mScreenWidth * mFontWidth);
    }
    update(_r);

    mSelectedRegion = mSelectedRegion.subtracted(newRegion);

    int y1 = mPA.y();
    for (int y = y1, total = mPB.y(); y <= total; ++y) {
        int x = 0;
        if (y == y1) {
            x = mPA.x();
        }

        if (y >= static_cast<int>(mpBuffer->buffer.size())) {
            break;
        }

        for (;; ++x) {
            if ((y == mPB.y()) && (x > mPB.x())) {
                break;
            }
            if (x < static_cast<int>(mpBuffer->buffer.at(y).size())) {
                if (!(mpBuffer->buffer.at(y).at(x).isSelected())) {
                    mpBuffer->buffer.at(y).at(x).select();
                    mpBuffer->dirty[y] = true;
                }
            } else {
                break;
            }
        }
    }

    update(mSelectedRegion.boundingRect());
    mSelectedRegion = newRegion;
}

void TTextEdit::unHighlight()
{
    int y1 = mPA.y();
    if (y1 < 0) {
        return;
    }
    for (int y = y1, total = mPB.y(); y <= total; ++y) {
        int x = 0;
        if (y == y1) {
            x = mPA.x();
        }

        if (y >= static_cast<int>(mpBuffer->buffer.size())) {
            break;
        }

        for (;; ++x) {
            if ((y == mPB.y()) && (x > mPB.x())) {
                break;
            }
            if (x < static_cast<int>(mpBuffer->buffer.at(y).size())) {
                if (mpBuffer->buffer.at(y).at(x).isSelected()) {
                    mpBuffer->buffer.at(y).at(x).deselect();
                    mpBuffer->dirty[y] = true;
                }
            } else {
                break;
            }
        }
    }
    mForceUpdate = true;
    update();
}

void TTextEdit::swap(QPoint& p1, QPoint& p2)
{
    QPoint tmp = p1;
    p1 = p2;
    p2 = tmp;
}

void TTextEdit::mouseMoveEvent(QMouseEvent* event)
{
    if ((mFontWidth == 0) | (mFontHeight == 0)) {
        return;
    }
    int x = event->x() / mFontWidth; // bugfix by BenH (used to be mFontWidth-1)
    if (mShowTimeStamps) {
        x -= 13;
    }
    int y = (event->y() / mFontHeight) + imageTopLine();
    if (x < 0) {
        x = 0;
    }
    if (y < 0) {
        y = 0;
    }
    if (y < static_cast<int>(mpBuffer->buffer.size())) {
        if (x < static_cast<int>(mpBuffer->buffer[y].size())) {
            if (mpBuffer->buffer.at(y).at(x).linkIndex()) {
                setCursor(Qt::PointingHandCursor);
                QStringList tooltip = mpBuffer->mHintStore[mpBuffer->buffer.at(y).at(x).linkIndex()];
                QToolTip::showText(event->globalPos(), tooltip.join("\n"));
            } else {
                setCursor(Qt::IBeamCursor);
                QToolTip::hideText();
            }
        }
    }

    if (!mMouseTracking) {
        return;
    }

    if (event->y() < 10) {
        mpConsole->scrollUp(3);
    }
    if (event->y() >= height() - 10) {
        mpConsole->scrollDown(3);
    }
    if ((y > (int)mpBuffer->size() - 1)) {
        return;
    }

    QPoint PC(x, y);

    if (mCtrlSelecting) {
        int oldAY = mPA.y();
        int oldBY = mPB.y();
        if (PC.y() == mDragStartY) {
            mPA.setY(PC.y());
            mPB.setY(PC.y());
        } else if (PC.y() < mDragStartY) {
            mPA.setY(PC.y());
            mPB.setY(mDragStartY);
        } else if (PC.y() > mDragStartY) {
            mPA.setY(mDragStartY);
            mPB.setY(PC.y());
        }

        if (oldAY < mPA.y()) {
            for (int y = oldAY, total = mPA.y(); y < total; ++y) {
                for (auto& x : mpBuffer->buffer[y]) {
                    x.deselect();
                }
            }
        }
        if (oldBY > mPB.y()) {
            for (int y = mPB.y() + 1; y <= oldBY; ++y) {
                for (auto& x : mpBuffer->buffer[y]) {
                    x.deselect();
                }
            }
        }

        mPA.setX(0);
        mPB.setX(static_cast<int>(mpBuffer->buffer[mPB.y()].size()) - 1);

        highlight();
        return;
    }
    if ((mPA.y() == mPB.y()) && (mPA.x() > mPB.x())) {
        swap(mPA, mPB);
    }
    if (mPA.y() > mPB.y()) {
        swap(mPA, mPB);
    }
    QPoint p1 = mPA - PC;
    QPoint p2 = mPB - PC;
    if (p1.manhattanLength() < p2.manhattanLength()) {
        if (mPA.y() < PC.y() || ((mPA.x() < PC.x()) && (mPA.y() == PC.y()))) {
            int y1 = PC.y();
            for (int y = y1, total = mPA.y(); y >= total; --y) {
                if (y >= static_cast<int>(mpBuffer->buffer.size()) || y < 0) {
                    break;
                }
                int x = mpBuffer->buffer.at(y).size() - 1;
                if (y == y1) {
                    x = PC.x();
                    if (x >= static_cast<int>(mpBuffer->buffer.at(y).size())) {
                        x = static_cast<int>(mpBuffer->buffer.at(y).size()) - 1;
                    }
                    if (x < 0) {
                        x = 0;
                    }
                }
                for (;; --x) {
                    if ((y == mPA.y()) && (x < mPA.x())) {
                        break;
                    }

                    if (x < static_cast<int>(mpBuffer->buffer[y].size()) && x >= 0) {
                        if (mpBuffer->buffer.at(y).at(x).isSelected()) {
                            mpBuffer->buffer.at(y).at(x).deselect();
                            mpBuffer->dirty[y] = true;
                        }
                    } else {
                        break;
                    }
                }
            }
        }
        mPA = PC;
    } else {
        if (mPB.y() > PC.y() || (mPB.x() > PC.x() && mPB.y() == PC.y())) {
            int y1 = PC.y();
            for (int y = y1, total = mPB.y(); y <= total; ++y) {
                int x = 0;
                if (y == y1) {
                    x = PC.x();
                }
                if (y >= static_cast<int>(mpBuffer->buffer.size()) || y < 0) {
                    break;
                }
                for (;; ++x) {
                    if ((y == mPB.y()) && (x > mPB.x())) {
                        break;
                    }
                    if (x < static_cast<int>(mpBuffer->buffer.at(y).size())) {
                        if (mpBuffer->buffer.at(y).at(x).isSelected()) {
                            mpBuffer->buffer.at(y).at(x).deselect();
                            mpBuffer->dirty[y] = true;
                        }
                    } else {
                        break;
                    }
                }
            }
        }

        mPB = PC;
    }
    if ((mPA.y() == mPB.y()) && (mPA.x() > mPB.x())) {
        swap(mPA, mPB);
    }
    if (mPA.y() > mPB.y()) {
        swap(mPA, mPB);
    }

    highlight();
}


void TTextEdit::contextMenuEvent(QContextMenuEvent* event)
{
    event->accept();
    return;
}

void TTextEdit::slot_popupMenu()
{
    auto* pA = qobject_cast<QAction*>(sender());
    if (!pA) {
        return;
    }
    QString cmd;
    if (mPopupCommands.contains(pA->text())) {
        cmd = mPopupCommands[pA->text()];
    }
    mpHost->mLuaInterpreter.compileAndExecuteScript(cmd);
}

void TTextEdit::mousePressEvent(QMouseEvent* event)
{
    if (mpConsole->getType() & (TConsole::MainConsole|TConsole::Buffer)) {
        TEvent mudletEvent;
        mudletEvent.mArgumentList.append(QLatin1String("sysWindowMousePressEvent"));
        switch (event->button()) {
        case Qt::LeftButton:
            mudletEvent.mArgumentList.append(QString::number(1));
            break;
        case Qt::RightButton:
            mudletEvent.mArgumentList.append(QString::number(2));
            break;
        case Qt::MidButton:
            mudletEvent.mArgumentList.append(QString::number(3));
            break;
        default: // TODO: What about those of us with more than three mouse buttons?
            mudletEvent.mArgumentList.append(QString());
            break;
        }
        mudletEvent.mArgumentList.append(QString::number(event->x()));
        mudletEvent.mArgumentList.append(QString::number(event->y()));
        mudletEvent.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
        mudletEvent.mArgumentTypeList.append(ARGUMENT_TYPE_NUMBER);
        mudletEvent.mArgumentTypeList.append(ARGUMENT_TYPE_NUMBER);
        mudletEvent.mArgumentTypeList.append(ARGUMENT_TYPE_NUMBER);
        mpHost->raiseEvent(mudletEvent);
    }

    if (event->button() == Qt::LeftButton) {
        if (event->modifiers() & Qt::ControlModifier) {
            mCtrlSelecting = true;
        }
        int x = event->x() / mFontWidth;
        if (mShowTimeStamps) {
            if (x < 13) {
                mCtrlSelecting = true;
            }
            x -= 13;
        }
        int y = (event->y() / mFontHeight) + imageTopLine();
        if (x < 0) {
            x = 0;
        }
        if (y < 0) {
            y = 0;
        }
        if (y < static_cast<int>(mpBuffer->buffer.size())) {
            if (x < static_cast<int>(mpBuffer->buffer[y].size())) {
                if (mpBuffer->buffer.at(y).at(x).linkIndex()) {
                    QStringList command = mpBuffer->mLinkStore[mpBuffer->buffer.at(y).at(x).linkIndex()];
                    QString func;
                    if (!command.empty()) {
                        func = command.at(0);
                        mpHost->mLuaInterpreter.compileAndExecuteScript(func);
                        return;
                    }
                }
            }
        }
        unHighlight();
        mSelectedRegion = QRegion(0, 0, 0, 0);
        if (mLastClickTimer.elapsed() < 300) {
            int xind = x;
            int yind = y;

            if (yind >= mpBuffer->lineBuffer.size()) {
                return;
            }
            if (xind >= mpBuffer->lineBuffer[yind].size()) {
                return;
            }
            while (xind < static_cast<int>(mpBuffer->buffer[yind].size())) {
                QChar c = mpBuffer->lineBuffer[yind].at(xind);
                if (c == QChar::Space) {
                    break;
                }
                xind++;
            }
            // For ignoring user specified characters, we first stop at space boundaries, then we
            // proceed to search within these spaces for ignored characters and chop off any we find.
            while (xind > 0 && mpHost->mDoubleClickIgnore.contains(mpBuffer->lineBuffer[yind].at(xind - 1))) {
                xind--;
            }
            mPB.setX(xind - 1);
            mPB.setY(yind);
            for (xind = x - 1; xind > 0; --xind) {
                if (mpBuffer->lineBuffer.at(yind).at(xind) == QChar::Space) {
                    break;
                }
            }
            int lsize = mpBuffer->lineBuffer[yind].size();
            while (xind + 1 < lsize && mpHost->mDoubleClickIgnore.contains(mpBuffer->lineBuffer[yind].at(xind + 1))) {
                xind++;
            }
            if (xind > 0) {
                mPA.setX(xind + 1);
            } else {
                mPA.setX(0);
            }
            mPA.setY(yind);
            highlight();
            event->accept();
            return;
        } else {
            mLastClickTimer.start();
            mMouseTracking = true;
            if (y >= mpBuffer->size()) {
                return;
            }
            if (mCtrlSelecting) {
                mPA.setX(0);
                mPA.setY(y);
                mPB.setX(static_cast<int>(mpBuffer->buffer[y].size()) - 1);
                mPB.setY(y);
                mDragStartY = y;
                highlight();
            } else {
                mPA.setX(x);
                mPA.setY(y);
                mPB = mPA;
            }
            event->accept();
            return;
        }
    }

    if (event->button() == Qt::RightButton) {
        int x = event->x() / mFontWidth;
        if (mShowTimeStamps) {
            x -= 13;
        }
        int y = (event->y() / mFontHeight) + imageTopLine();
        if (x < 0) {
            x = 0;
        }
        if (y < 0) {
            y = 0;
        }
        if (y < static_cast<int>(mpBuffer->buffer.size())) {
            if (x < static_cast<int>(mpBuffer->buffer[y].size())) {
                if (mpBuffer->buffer.at(y).at(x).linkIndex()) {
                    QStringList command = mpBuffer->mLinkStore[mpBuffer->buffer.at(y).at(x).linkIndex()];
                    QStringList hint = mpBuffer->mHintStore[mpBuffer->buffer.at(y).at(x).linkIndex()];
                    if (command.size() > 1) {
                        auto popup = new QMenu(this);
                        for (int i = 0, total = command.size(); i < total; ++i) {
                            QAction* pA;
                            if (i < hint.size()) {
                                pA = popup->addAction(hint[i]);
                                mPopupCommands[hint[i]] = command[i];
                            } else {
                                pA = popup->addAction(command[i]);
                                mPopupCommands[command[i]] = command[i];
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
            action->setIcon(QIcon::fromTheme(QStringLiteral("edit-copy"), QIcon(QStringLiteral(":/icons/edit-copy.png"))));
            action3->setIcon(QIcon::fromTheme(QStringLiteral("edit-select-all"), QIcon(QStringLiteral(":/icons/edit-select-all.png"))));
            action4->setIcon(QIcon::fromTheme(QStringLiteral("edit-web-search"), QIcon(QStringLiteral(":/icons/edit-web-search.png"))));
        }

        auto popup = new QMenu(this);
        popup->setToolTipsVisible(true); // Not the default...
        popup->addAction(action);
        popup->addAction(action2);
        popup->addAction(actionCopyImage);
        popup->addSeparator();
        popup->addAction(action3);

        if (mPA != mPB && mpHost->mEnableTextAnalyzer) {
            mpContextMenuAnalyser = new QAction(tr("Analyse characters"), this);
            // NOTE: If running inside the Qt Creator IDE using the debugger with
            // the hovered() signal can be *problematic* - as hitting a
            // breakpoint - or getting an OS signal (like a Segment Violation)
            // can hang not only Mudlet but also Qt Creator and possibly even
            // your Desktop - though for *nix users swithing to a console and
            // killing the gdb debugger instance run by Qt Creator will restore
            // normality.
            connect(mpContextMenuAnalyser, &QAction::hovered, this, &TTextEdit::slot_analyseSelection);
            mpContextMenuAnalyser->setToolTip(tr("<p>Hover on this item to display the Unicode codepoints in the selection <i>(only the first line!)</i></p>"));
            popup->addSeparator();
            popup->addAction(mpContextMenuAnalyser);
        }

        popup->addSeparator();
        popup->addAction(action4);

        if (!mudlet::self()->isControlsVisible()) {
            QAction* actionRestoreMainMenu = new QAction(tr("restore Main menu"), this);
            connect(actionRestoreMainMenu, &QAction::triggered, mudlet::self(), &mudlet::slot_restoreMainMenu);
            actionRestoreMainMenu->setToolTip(QStringLiteral("<html><head/><body>%1</body></html>").arg(tr("Use this to restore the Main menu to get access to controls.")));

            QAction* actionRestoreMainToolBar = new QAction(tr("restore Main Toolbar"), this);
            connect(actionRestoreMainToolBar, &QAction::triggered, mudlet::self(), &mudlet::slot_restoreMainToolBar);
            actionRestoreMainToolBar->setToolTip(QStringLiteral("<html><head/><body>%1</body></html>").arg(tr("Use this to restore the Main Toolbar to get access to controls.")));

            popup->addSeparator();
            popup->addAction(actionRestoreMainMenu);
            popup->addAction(actionRestoreMainToolBar);
        }

        popup->popup(mapToGlobal(event->pos()), action);
        event->accept();
        return;
    }

    if (event->button() == Qt::MidButton) {
        mpConsole->mLowerPane->mCursorY = mpConsole->buffer.size(); //
        mpConsole->mLowerPane->hide();
        mpBuffer->mCursorY = mpBuffer->size();
        mpConsole->mUpperPane->mCursorY = mpConsole->buffer.size(); //
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
    mPA = QPoint(0, 0);
    mPB = mpBuffer->getEndPos();
    highlight();
    update();
}

void TTextEdit::slot_searchSelectionOnline()
{
    searchSelectionOnline();
}


void TTextEdit::slot_copySelectionToClipboard()
{
    QString selectedText = getSelectedText();
    QClipboard* clipboard = QApplication::clipboard();
    clipboard->setText(selectedText);
}

void TTextEdit::slot_copySelectionToClipboardHTML()
{
    if ((mPA.y() == mPB.y()) && (mPA.x() > mPB.x())) {
        swap(mPA, mPB);
    }
    if (mPA.y() > mPB.y()) {
        swap(mPA, mPB);
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
    fontsList << QStringLiteral("Courier New");
    fontsList << QStringLiteral("Monospace");
    fontsList << QStringLiteral("Courier");
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
    bool isSingleLine = (mPA.y() == mPB.y());
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
    text.append(QStringLiteral(" </div></body>\n"
                               "</html>"));
    // The last two of these tags were missing and meant the HTML was not terminated properly
    QClipboard* clipboard = QApplication::clipboard();
    clipboard->setText(text);
    mSelectedRegion = QRegion(0, 0, 0, 0);
    forceUpdate();
    return;
}

void TTextEdit::slot_copySelectionToClipboardImage()
{
    mCopyImageStartTime = std::chrono::high_resolution_clock::now();

    // mPA QPoint where selection started
    // mPB QPoint where selection ended

    // if selection was made backwards swap
    // right to left
    if ((mPA.y() == mPB.y()) && (mPA.x() > mPB.x())) {
        swap(mPA, mPB);
    }
    // down to up
    if (mPA.y() > mPB.y()) {
        swap(mPA, mPB);
    }

    if (mFontWidth <= 0 || mFontHeight <= 0) {
        return;
    }

    if (mSelectedRegion == QRegion(0, 0, 0, 0)) {
        return;
    }

    if (mScreenHeight <= 0 || mScreenWidth <= 0) {
        mScreenHeight = height() / mFontHeight;
        mScreenWidth = 100;
        if (mScreenHeight <= 0 || mScreenWidth <= 0) {
            return;
        }
    }

    // Qt says: "Maximum supported image dimension is 65500 pixels" in stdout
    auto heightpx = qMin(65500, (mPB.y() - mPA.y() + 1) * mFontHeight);
    auto lineOffset = mPA.y();

    // find the biggest width of text we need to work with
    int characterWidth = 0;
    for (int y = mPA.y(), total = mPB.y() + 1; y < total; ++y) {
        const auto lineWidth = static_cast<int>(mpBuffer->buffer.at(y).size());
        characterWidth = qMax(lineWidth, characterWidth);
    }

    auto widthpx = qMin(65500, characterWidth * mFontWidth);

    auto rect = QRect(mPA.x(), mPA.y(), widthpx, heightpx);

    auto pixmap = QPixmap(widthpx, heightpx);
    pixmap.fill(palette().base().color());

    QPainter painter(&pixmap);
    if (!painter.isActive()) {
        return;
    }

    auto oldMpa = mPA;
    auto oldMpb = mPB;

    // deselect to prevent inverted colours in image
    unHighlight();
    mSelectedRegion = QRegion(0, 0, 0, 0);

    auto result = drawTextForClipboard(painter, rect, lineOffset);

    mPA = oldMpa;
    mPB = oldMpb;
    highlight();

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
    painter.setCompositionMode(QPainter::CompositionMode_Source);
    if (mpConsole->getType() == TConsole::MainConsole) {
        painter.setFont(mpHost->mDisplayFont);
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
            return std::make_pair(false, linesDrawn);
        }
    }
    return std::make_pair(true, linesDrawn);
}

void TTextEdit::searchSelectionOnline()
{
    QString selectedText = getSelectedText(' ');
    QString url = QUrl::toPercentEncoding(selectedText.trimmed());
    url.prepend(mpHost->getSearchEngine().second);
    QDesktopServices::openUrl(QUrl(url));
}

QString TTextEdit::getSelectedText(char newlineChar)
{
    // mPA QPoint where selection started
    // mPB QPoint where selection ended

    // if selection was made backwards swap
    // right to left
    if ((mPA.y() == mPB.y()) && (mPA.x() > mPB.x())) {
        swap(mPA, mPB);
    }
    // down to up
    if (mPA.y() > mPB.y()) {
        swap(mPA, mPB);
    }

    QString text;

    // for each selected line
    bool isSingleLine = (mPA.y() == mPB.y());
    // CHECKME: the <= together with the +1 in the test looks suspecious:
    for (int y = mPA.y(), total = mPB.y() + 1; y <= total; ++y) {
        // stop if we are at the end of the buffer lines
        if (y >= static_cast<int>(mpBuffer->buffer.size())) {
            mSelectedRegion = QRegion(0, 0, 0, 0);
            forceUpdate();
            return text;
        }

        int x = 0;
        // if the selection started on this line
        if (y == mPA.y()) {
            // start from the column where the selection started
            x = mPA.x();
            if (!isSingleLine) {
                // insert the number of spaces to push the first line to the right
                // so it lines up with the following lines - but only if there
                // is MORE than a single line:
                text.append(QStringLiteral(" ").repeated(x));
            }
        }
        // while we are not at the end of the buffer line
        while (x < static_cast<int>(mpBuffer->buffer[y].size())) {
            text.append(mpBuffer->lineBuffer[y].at(x));
            // if the selection ended on this line
            if (y >= mPB.y()) {
                // stop if the selection ended on this column or the buffer line is ending
                if ((x == mPB.x()) || (x >= static_cast<int>(mpBuffer->buffer[y].size() - 1))) {
                    mSelectedRegion = QRegion(0, 0, 0, 0);
                    forceUpdate();
                    return text;
                }
            }
            x++;
        }
        // we never append the last character of a buffer line se we set our own
        text.append(newlineChar);
    }
    qDebug() << "TTextEdit::getSelectedText(...) INFO - unexpectedly hit bottom of method so returning:" << text;
    return text;
}

void TTextEdit::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        mMouseTracking = false;
        mCtrlSelecting = false;
    }

    if (mpConsole->getType() == TConsole::MainConsole) {
        TEvent mudletEvent;
        mudletEvent.mArgumentList.append(QLatin1String("sysWindowMouseReleaseEvent"));
        switch (event->button()) {
        case Qt::LeftButton:
            mudletEvent.mArgumentList.append(QString::number(1));
            break;
        case Qt::RightButton:
            mudletEvent.mArgumentList.append(QString::number(2));
            break;
        case Qt::MidButton:
            mudletEvent.mArgumentList.append(QString::number(3));
            break;
        default:
            mudletEvent.mArgumentList.append(QString::number(0));
            break;
        }
        mudletEvent.mArgumentList.append(QString::number(event->x()));
        mudletEvent.mArgumentList.append(QString::number(event->y()));
        mudletEvent.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
        mudletEvent.mArgumentTypeList.append(ARGUMENT_TYPE_NUMBER);
        mudletEvent.mArgumentTypeList.append(ARGUMENT_TYPE_NUMBER);
        mudletEvent.mArgumentTypeList.append(ARGUMENT_TYPE_NUMBER);
        mpHost->raiseEvent(mudletEvent);
    } else if (mpConsole->getType() == TConsole::UserWindow && mpConsole->mpDockWidget && mpConsole->mpDockWidget->isFloating()) {
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
    if (!mIsLowerPane && mpConsole->getType() != TConsole::CentralDebugConsole) {
        // CHECKME: This looks suspect - it would seem to be called on resizing
        // floating user windows, and the Editor's Error TConsole as well as
        // components in the main window - for NAWS purposes it seems more
        // likely to be needed ONLY on the main TConsole for the profile
        mpHost->adjustNAWS();
    }

    QWidget::resizeEvent(event);
}

void TTextEdit::wheelEvent(QWheelEvent* e)
{
    int k = 3;
    if (e->delta() < 0) {
        mpConsole->scrollDown(abs(k));
        e->accept();
        return;
    }
    if (e->delta() > 0) {
        mpConsole->scrollUp(k);
        e->accept();
        return;
    }
    e->ignore();
    return;
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

// Ensure we return 0 if the whole buffer fits within the space on-screen which
// should stop the split appearing if there is nothing to scroll up/down.
// This should only be used on the upper pane:
int TTextEdit::bufferScrollUp(int lines)
{
    if (Q_UNLIKELY((mpBuffer->mCursorY - lines) >= mScreenHeight)) {
        mpBuffer->mCursorY -= lines;
        return lines;

    } else {
        mpBuffer->mCursorY -= lines;
        if (mCursorY < 0) {
            int delta = mCursorY;
            mpBuffer->mCursorY = 0;
            return delta;

        } else {
            return 0;
        }
    }
}

// This should only be used on the upper pane:
int TTextEdit::bufferScrollDown(int lines)
{
    if ((mpBuffer->mCursorY + lines) < static_cast<int>(mpBuffer->size())) {
        if (mpBuffer->mCursorY < mScreenHeight) {
            mpBuffer->mCursorY = mScreenHeight + lines;
            if (mpBuffer->mCursorY > static_cast<int>(mpBuffer->size() - 1)) {
                mpBuffer->mCursorY = mpBuffer->size() - 1;
                mIsTailMode = true;
            }

        } else {
            mpBuffer->mCursorY += lines;
            mIsTailMode = false;
        }
        return lines;

    } else if (mpBuffer->mCursorY >= (int)(mpBuffer->size() - 1)) {
        mIsTailMode = true;
        mpBuffer->mCursorY = mpBuffer->lineBuffer.size();
        forceUpdate();
        return 0;

    } else {
        lines = (int)(mpBuffer->size() - 1) - mpBuffer->mCursorY;
        if (mpBuffer->mCursorY + lines < mScreenHeight + lines) {
            mpBuffer->mCursorY = mScreenHeight + lines;
            if (mpBuffer->mCursorY > (int)(mpBuffer->size() - 1)) {
                mpBuffer->mCursorY = mpBuffer->size() - 1;
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
        charWidth = qRound(QFontMetricsF(mpHost->mDisplayFont).averageCharWidth());
    } else {
        charWidth = qRound(QFontMetricsF(mDisplayFont).averageCharWidth());
    }

    return width() / charWidth;
}

int TTextEdit::getRowCount()
{
    int rowHeight;

    if (mpConsole->getType() == TConsole::MainConsole) {
        rowHeight = qRound(QFontMetricsF(mpHost->mDisplayFont).lineSpacing());
    } else {
        rowHeight = qRound(QFontMetricsF(mDisplayFont).lineSpacing());
    }

    return height() / rowHeight;
}

inline QString TTextEdit::htmlCenter(const QString& text)
{
    return QStringLiteral("<center>%1</center>").arg(text);
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
        case 0x003c:                    return htmlCenter(QStringLiteral("&lt;")); break; // As '<' gets interpreted as an opening HTML tag we have to handle it specially
        case 0x003e:                    return htmlCenter(QStringLiteral("&gt;")); break; // '>' does not seem to get interpreted as a closing HTML tag but for symetry it is probably best to also handle it in the same way
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
        case 0x3000:                    return htmlCenter(tr("{ideaographic space}", "Unicode U+3000 codepoint - ideaographic (CJK Wide) space")); break;
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
            [[clang::fallthrough]];
        case 0xFFFF:
            [[clang::fallthrough]];
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
        return QStringLiteral("\\%1").arg(static_cast<quint8>(*byte), 3, 10, QLatin1Char('0'));
    } else if (static_cast<quint8>(*byte) == 0x3C) {
        // less-then - which is noticed by the Qt library code and taken as an
        // HTML/Rich-text formatting opening tag and has to be converted to
        // "&lt;":
        return QStringLiteral("&lt;");
    } else {
        return QStringLiteral("%1").arg(*byte);
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
    bool isSingleLine = false;
    startColumn = mPA.x();
    if (mPA.y() == mPB.y()) {
        isSingleLine = true;
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
                utf16indexes.append(QStringLiteral("<th colspan=\"%1\"><center>%2 & %3</center></th>").arg(QString::number(columnsToUse), QString::number(index + 1), QString::number(index + 2)));

                // The use of one QStringLiteral inside another is because it is
                // impossible to force an upper-case alphabet to Hex digits otherwise
                // just for that number (and not the rest of the resultant String):
                // &#8232; is the Unicode Line Separator
                utf16Vals.append(
                        QStringLiteral("<td colspan=\"%1\" style=\"white-space:no-wrap vertical-align:top\"><center>%2</centre>&#8232;<center>(0x%3:0x%4)</center></td>")
                                .arg(QString::number(columnsToUse))
                                .arg(QStringLiteral("%1").arg(QChar::surrogateToUcs4(mpBuffer->lineBuffer.at(line).at(index), mpBuffer->lineBuffer.at(line).at(index + 1)), 4, 16, zero).toUpper())
                                .arg(mpBuffer->lineBuffer.at(line).at(index).unicode(), 4, 16, zero)
                                .arg(mpBuffer->lineBuffer.at(line).at(index + 1).unicode(), 4, 16, zero));

                // Note the addition to the index here to jump over the low-surrogate:
                graphemes.append(QStringLiteral("<td colspan=\"%1\">%2</td>")
                                         .arg(QString::number(columnsToUse))
                                         .arg(convertWhitespaceToVisual(mpBuffer->lineBuffer.at(line).at(index), mpBuffer->lineBuffer.at(line).at(index + 1))));
            }

            switch (utf8Width) {
            case 4:
                if (includeThisCodePoint) {
                    utf8Indexes.append(QStringLiteral("<th><center>%1</center></th><td><center>%2</center></td><td><center>%3</center></td><td><center>%4</center></td></b>")
                                               .arg(QString::number(utf8Index), QString::number(utf8Index + 1), QString::number(utf8Index + 2), QString::number(utf8Index + 3)));
                    utf8Vals.append(QStringLiteral("<td><center>0x%1</center></td><td><center>0x%2</center></td><td><center>0x%3</center></td><td><center>0x%4</center></td>")
                                            .arg(static_cast<quint8>(utf8Bytes[0]), 2, 16, zero)
                                            .arg(static_cast<quint8>(utf8Bytes[1]), 2, 16, zero)
                                            .arg(static_cast<quint8>(utf8Bytes[2]), 2, 16, zero)
                                            .arg(static_cast<quint8>(utf8Bytes[3]), 2, 16, zero));
                    luaCodes.append(QStringLiteral("<td><center>%1</center></td><td><center>%2</center></td><td><center>%3</center></td><td><center>%4</center></td>")
                                            .arg(byteToLuaCodeOrChar(&utf8Bytes[0]), byteToLuaCodeOrChar(&utf8Bytes[1]), byteToLuaCodeOrChar(&utf8Bytes[2]), byteToLuaCodeOrChar(&utf8Bytes[3])));
                }
                utf8Index += 4;
                break;
            case 3:
                if (includeThisCodePoint) {
                    utf8Indexes.append(QStringLiteral("<th><center>%1</center></th><td><center>%2</center></td><td><center>%3</center></td>")
                                               .arg(QString::number(utf8Index), QString::number(utf8Index + 1), QString::number(utf8Index + 2)));
                    utf8Vals.append(QStringLiteral("<td><center>0x%1</center></td><td><center>0x%2</center></td><td><center>0x%3</center></td>")
                                            .arg(static_cast<quint8>(utf8Bytes[0]), 2, 16, zero)
                                            .arg(static_cast<quint8>(utf8Bytes[1]), 2, 16, zero)
                                            .arg(static_cast<quint8>(utf8Bytes[2]), 2, 16, zero));
                    luaCodes.append(QStringLiteral("<td><center>%1</center></td><td><center>%2</center></td><td><center>%3</center></td>")
                                            .arg(byteToLuaCodeOrChar(&utf8Bytes[0]), byteToLuaCodeOrChar(&utf8Bytes[1]), byteToLuaCodeOrChar(&utf8Bytes[2])));
                }
                utf8Index += 3;
                break;
            case 2:
                if (includeThisCodePoint) {
                    utf8Indexes.append(QStringLiteral("<th><center>%1</center></th><td><center>%2</center></td>").arg(QString::number(utf8Index), QString::number(utf8Index + 1)));
                    utf8Vals.append(QStringLiteral("<td><center>0x%1</center></td><td><center>0x%2</center></td>")
                                            .arg(static_cast<quint8>(utf8Bytes[0]), 2, 16, zero)
                                            .arg(static_cast<quint8>(utf8Bytes[1]), 2, 16, zero));
                    luaCodes.append(QStringLiteral("<td><center>%1</center></td><td><center>%2</center></td>").arg(byteToLuaCodeOrChar(&utf8Bytes[0]), byteToLuaCodeOrChar(&utf8Bytes[1])));
                }
                utf8Index += 2;
                break;
            default:
                if (includeThisCodePoint) {
                    utf8Indexes.append(QStringLiteral("<th><center>%1</center></th>").arg(QString::number(utf8Index)));
                    utf8Vals.append(QStringLiteral("<td><center>0x%1</center></td>").arg(static_cast<quint8>(utf8Bytes[0]), 2, 16, zero));
                    luaCodes.append(QStringLiteral("<td><center>%1</center></td>").arg(byteToLuaCodeOrChar(&utf8Bytes[0])));
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
                utf16indexes.append(QStringLiteral("<th colspan=\"%1\"><center>%2</center></th>").arg(QString::number(columnsToUse), QString::number(index + 1)));

                utf16Vals.append(QStringLiteral("<td colspan=\"%1\" style=\"white-space:no-wrap vertical-align:top\"><center>%2</center></td>")
                                         .arg(QString::number(columnsToUse))
                                         .arg(mpBuffer->lineBuffer.at(line).at(index).unicode(), 4, 16, QChar('0'))
                                         .toUpper());

                graphemes.append(QStringLiteral("<td colspan=\"%1\">%2</td>").arg(QString::number(columnsToUse), convertWhitespaceToVisual(mpBuffer->lineBuffer.at(line).at(index))));
            }

            switch (utf8Width) {
            case 4: // Maybe a BMP character cannot use 4 utf-8 bytes?
                if (includeThisCodePoint) {
                    utf8Indexes.append(QStringLiteral("<th><center>%1</center></th><td><center>%2</center></td><td><center>%3</center></td><td><center>%4</center></td>")
                                               .arg(QString::number(utf8Index), QString::number(utf8Index + 1), QString::number(utf8Index + 2), QString::number(utf8Index + 3)));

                    utf8Vals.append(QStringLiteral("<td><center>0x%1</center></td><td><center>0x%2</center></td><td><center>0x%3</center></td><td><center>0x%4</center></td>")
                                            .arg(static_cast<quint8>(utf8Bytes[0]), 2, 16, zero)
                                            .arg(static_cast<quint8>(utf8Bytes[1]), 2, 16, zero)
                                            .arg(static_cast<quint8>(utf8Bytes[2]), 2, 16, zero)
                                            .arg(static_cast<quint8>(utf8Bytes[3]), 2, 16, zero));

                    luaCodes.append(QStringLiteral("<td><center>%1</center></td><td><center>%2</center></td><td><center>%3</center></td><td><center>%4</center></td>")
                                            .arg(byteToLuaCodeOrChar(&utf8Bytes[0]), byteToLuaCodeOrChar(&utf8Bytes[1]), byteToLuaCodeOrChar(&utf8Bytes[2]), byteToLuaCodeOrChar(&utf8Bytes[3])));
                }
                utf8Index += 4;
                break;
            case 3:
                if (includeThisCodePoint) {
                    utf8Indexes.append(QStringLiteral("<th><center>%1</center></th><td><center>%2</center></td><td><center>%3</center></td>")
                                               .arg(QString::number(utf8Index), QString::number(utf8Index + 1), QString::number(utf8Index + 2)));

                    utf8Vals.append(QStringLiteral("<td><center>0x%1</center></td><td><center>0x%2</center></td><td><center>0x%3</center></td>")
                                            .arg(static_cast<quint8>(utf8Bytes[0]), 2, 16, zero)
                                            .arg(static_cast<quint8>(utf8Bytes[1]), 2, 16, zero)
                                            .arg(static_cast<quint8>(utf8Bytes[2]), 2, 16, zero));

                    luaCodes.append(QStringLiteral("<td><center>%1</center></td><td><center>%2</center></td><td><center>%3</center></td>")
                                            .arg(byteToLuaCodeOrChar(&utf8Bytes[0]), byteToLuaCodeOrChar(&utf8Bytes[1]), byteToLuaCodeOrChar(&utf8Bytes[2])));
                }
                utf8Index += 3;
                break;

            case 2:
                if (includeThisCodePoint) {
                    utf8Indexes.append(QStringLiteral("<th><center>%1</center></th><td><center>%2</center></td>").arg(QString::number(utf8Index), QString::number(utf8Index + 1)));

                    utf8Vals.append(QStringLiteral("<td><center>0x%1</center></td><td><center>0x%2</center></td>")
                                            .arg(static_cast<quint8>(utf8Bytes[0]), 2, 16, zero)
                                            .arg(static_cast<quint8>(utf8Bytes[1]), 2, 16, zero));

                    luaCodes.append(QStringLiteral("<td><center>%1</center></td><td><center>%2</center></td>").arg(byteToLuaCodeOrChar(&utf8Bytes[0]), byteToLuaCodeOrChar(&utf8Bytes[1])));
                }
                utf8Index += 2;
                break;

            default:
                if (includeThisCodePoint) {
                    utf8Indexes.append(QStringLiteral("<th><center>%1</center></th>").arg(QString::number(utf8Index)));

                    utf8Vals.append(QStringLiteral("<td><center>0x%1</center></td>").arg(static_cast<quint8>(utf8Bytes[0]), 2, 16, zero));

                    luaCodes.append(QStringLiteral("<td><center>%1</center></td>").arg(byteToLuaCodeOrChar(&utf8Bytes[0])));
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
                        QStringLiteral("<small><table border=\"1\" style=\"margin-top:5px; margin-bottom:5px; margin-left:5px; margin-right:5px;\" width=\"100%\" cellspacing=\"2\" cellpadding=\"0\">"
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
                        QStringLiteral("<small><table border=\"1\" style=\"margin-top:5px; margin-bottom:5px; margin-left:5px; margin-right:5px;\" width=\"100%\" cellspacing=\"2\" cellpadding=\"0\">"
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
                    QStringLiteral("<html><head/><body>%1"
                                   "<small><table border=\"1\" style=\"margin-top:5px; margin-bottom:5px; margin-left:5px; margin-right:5px;\" width=\"100%\" cellspacing=\"2\" cellpadding=\"0\">"
                                   "<tr><th>%2</th>%3</tr>"
                                   "<tr><th>%4</th>%5</tr>"
                                   "<tr><th>%6</th>%7</tr>"
                                   "<tr><th>%8</th>%9</tr>"
                                   "<tr><th>%10</th>%11</tr>"
                                   "<tr><th>%12</th>%13</tr>"
                                   "</table></small></body></html>")
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
                    QStringLiteral("<html><head/><body>%1"
                                   "<small><table border=\"1\" style=\"margin-top:5px; margin-bottom:5px; margin-left:5px; margin-right:5px;\" width=\"100%\" cellspacing=\"2\" cellpadding=\"0\">"
                                   "<tr>%2</tr>"
                                   "<tr>%3</tr>"
                                   "<tr>%4</tr>"
                                   "<tr>%5</tr>"
                                   "<tr>%6</tr>"
                                   "<tr>%7</tr>"
                                   "</table></small></body></html>")
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
