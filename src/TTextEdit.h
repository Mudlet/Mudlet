#ifndef MUDLET_TTEXTEDIT_H
#define MUDLET_TTEXTEDIT_H

/***************************************************************************
 *   Copyright (C) 2008-2011 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2015, 2018, 2020 by Stephen Lyons                       *
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


#include "TBuffer.h"

#include "pre_guard.h"
#include <QElapsedTimer>
#include <QMap>
#include <QPointer>
#include <QWidget>
#include <chrono>
#include "post_guard.h"

#include <string>

class Host;
class TConsole;
class TChar;

class QScrollBar;
class QString;


class TTextEdit : public QWidget
{
    Q_OBJECT

public:
    Q_DISABLE_COPY(TTextEdit)
    TTextEdit(TConsole*, QWidget*, TBuffer* pB, Host* pH, bool isLowerPane);
    void paintEvent(QPaintEvent*) override;
    void contextMenuEvent(QContextMenuEvent* event) override;
    void drawForeground(QPainter&, const QRect&);
    static uint getGraphemeBaseCharacter(const QString& str);
    void drawLine(QPainter& painter, int lineNumber, int lineOfScreen, int* offset = nullptr) const;
    int drawGraphemeBackground(QPainter&, QVector<QColor>&, QVector<QRect>&, QVector<QString>&, QVector<int>&, QPoint&, const QString&, const int, TChar&) const;
    void drawGraphemeForeground(QPainter&, const QColor&, const QRect&, const QString&, TChar&) const;
    void showNewLines();
    void forceUpdate();
    void needUpdate(int, int);
    void scrollTo(int);
    void scrollH(int);
    void scrollUp(int lines);
    void scrollDown(int lines);
    void wheelEvent(QWheelEvent* e) override;
    void resizeEvent(QResizeEvent* event) override;
    void mousePressEvent(QMouseEvent*) override;
    void mouseReleaseEvent(QMouseEvent*) override;
    void mouseMoveEvent(QMouseEvent*) override;
    void showEvent(QShowEvent* event) override;
    void updateScreenView();
    void updateScrollBar(int);
    void calculateHMaxRange();
    void updateHorizontalScrollBar();
    void highlightSelection();
    void unHighlight();
    void focusInEvent(QFocusEvent* event) override;
    int imageTopLine();
    int bufferScrollDown(int lines);
    // Not used:    void setConsoleFgColor(int r, int g, int b) { mFgColor = QColor(r, g, b); }
    void setConsoleBgColor(int r, int g, int b, int a) { mBgColor = QColor(r, g, b, a); }
    void resetHScrollbar() { mScreenOffset = 0; mMaxHRange = 0; }
    int getScreenHeight() { return mScreenHeight; }
    void searchSelectionOnline();
    int getColumnCount();
    int getRowCount();
    void reportCodepointErrors();

    QColor mBgColor;
    // position of cursor, in characters, across the entire buffer
    int mCursorY;
    int mCursorX;
    QFont mDisplayFont;
    QColor mFgColor;
    int mFontAscent{};
    int mFontDescent{};
    bool mIsCommandPopup;
    // If true, this TTextEdit is to display the last lines in
    // mpConsole.mpBuffer. This is always true for the lower main window panel
    // but it is RESET when the upper one is scrolled upwards. The name appears
    // to be related to the file monitoring feature in the *nix tail command.
    // See, e.g.: https://en.wikipedia.org/wiki/Tail_(Unix)#File_monitoring
    bool mIsTailMode;
    QMap<QString, std::pair<QString, int>> mPopupCommands;
    int mScrollVector{};
    QRegion mSelectedRegion;
    bool mShowTimeStamps;
    int mWrapAt{};
    int mWrapIndentCount{};

public slots:
    void slot_toggleTimeStamps(const bool);
    void slot_copySelectionToClipboard();
    void slot_selectAll();
    void slot_scrollBarMoved(int);
    void slot_hScrollBarMoved(int);
    void slot_popupMenu();
    void slot_copySelectionToClipboardHTML();
    void slot_searchSelectionOnline();
    void slot_analyseSelection();
    void slot_changeIsAmbigousWidthGlyphsToBeWide(bool);
    void slot_changeDebugShowAllProblemCodepoints(const bool);
    void slot_mouseAction(const QString&);

private slots:
    void slot_copySelectionToClipboardImage();

private:
    void initDefaultSettings();
    QString getSelectedText(const QChar& newlineChar = QChar::LineFeed, const bool showTimestamps = false);
    static QString htmlCenter(const QString&);
    static QString convertWhitespaceToVisual(const QChar& first, const QChar& second = QChar::Null);
    static QString byteToLuaCodeOrChar(const char*);
    std::pair<bool, int> drawTextForClipboard(QPainter& p, QRect r, int lineOffset) const;
    int convertMouseXToBufferX(const int mouseX, const int lineNumber, bool* isOutOfbounds, bool* isOverTimeStamp = nullptr) const;
    int getGraphemeWidth(uint unicode) const;
    void normaliseSelection();
    void updateTextCursor(const QMouseEvent* event, int lineIndex, int tCharIndex, bool isOutOfbounds);
    bool establishSelectedText();
    void expandSelectionToWords();
    void expandSelectionToLine(int);

    int mFontHeight;
    int mFontWidth;
    bool mForceUpdate;

    // Each TConsole instance uses two instances of this class, one above the
    // other but they need to behave differently in some ways; this flag is set
    // or reset on creation and is used to adjust the behaviour depending on
    // which one this instance is:
    const bool mIsLowerPane;
    // last line offset rendered
    int mLastRenderBottom;
    bool mMouseTracking;
    // 1/2/3 for single/double/triple click seen so far
    int mMouseTrackLevel;
    bool mCtrlSelecting{};
    int mCtrlDragStartY{};
    QPoint mDragStart, mDragSelectionEnd;
    int mOldScrollPos{};
    // top-left point of the selection
    QPoint mPA;
    // bottom-right point of the selection
    QPoint mPB;
    TBuffer* mpBuffer;
    TConsole* mpConsole;
    QPointer<Host> mpHost;
    // screen height in characters
    int mScreenHeight;
    // currently viewed screen area
    QPixmap mScreenMap;
    int mScreenWidth;
    int mScreenOffset;
    int mMaxHRange;
    QElapsedTimer mLastClickTimer;
    QPointer<QAction> mpContextMenuAnalyser;
    bool mWideAmbigousWidthGlyphs;
    std::chrono::high_resolution_clock::time_point mCopyImageStartTime;
    // Set in constructor for run-time Qt versions less than 5.11 which only
    // supports up to Unicode 8.0:
    bool mUseOldUnicode8;
    // How many "normal" width "characters" are each tab stop apart, while
    // there is no current mechanism to adjust this, sensible values will
    // probably be 1 (so that a tab is just treated as a space), 2, 4 and 8,
    // in the past it was typically 8 and this is what we'll use at present:
    int mTabStopwidth;
    // How many normal width characters that are used for the time stamps; it
    // would only be valid to change this by clearing the buffer first - so
    // making this a const value for the moment:
    const int mTimeStampWidth;
    bool mShowAllCodepointIssues;
    // Marked mutable so that it is permissible to change this in class methods
    // that are otherwise const!
    mutable QHash<uint, std::tuple<uint, std::string>> mProblemCodepoints;
    // We scroll on the basis that one vertical mouse wheel click is one line
    // (vertically, not really concerned about horizontal stuff at present).
    // According to Qt: "Most mouse types work in steps of 15 degrees, in which
    // case the delta value is a multiple of 120;
    // i.e., 120 units * 1/8 = 15 degrees.
    // However, some mice have finer-resolution wheels and send delta values
    // that are less than 120 units (less than 15 degrees). To support this
    // possibility, you can either cumulatively add the delta values from events
    // until the value of 120 is reached, then scroll the widget, or you can
    // partially scroll the widget in response to each wheel event. But to
    // provide a more native feel, you should prefer pixelDelta() on platforms
    // where it's available."
    // We use the following to store the remainder (modulus 120):
    QPoint mMouseWheelRemainder;
};

#endif // MUDLET_TTEXTEDIT_H
