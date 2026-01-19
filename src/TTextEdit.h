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


#include <QElapsedTimer>
#include <QMap>
#include <QPointer>
#include <QWidget>
#include <chrono>

#include <string>


class Host;
class TBuffer;
class TConsole;
class TChar;

class QScrollBar;
class QString;


class TTextEdit : public QWidget
{
    Q_OBJECT

    friend class TAccessibleTextEdit;

public:
    Q_DISABLE_COPY(TTextEdit)
    TTextEdit(TConsole*, QWidget*, TBuffer* pB, Host* pH, bool isLowerPane);
    void paintEvent(QPaintEvent*) override;
    void contextMenuEvent(QContextMenuEvent* event) override;
    void drawForeground(QPainter&, const QRect&);
    void drawLine(QPainter& painter, int lineNumber, int rowOfScreen, int *offset = nullptr) const;
    int drawGraphemeBackground(QPainter&, QVector<QColor>&, QVector<QRect>&, QVector<QString>&, QVector<int>&, QPoint&, const QString&, const int, const int, TChar&) const;
    void drawGraphemeForeground(QPainter&, const QColor&, const QRect&, const QString&, TChar &) const;
    void drawCustomDecorations(QPainter&, const QColor&, const QRect&, TChar&) const;
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
    void setConsoleBgColor(int r, int g, int b, int a ) { mBgColor = QColor(r, g, b, a); }
    void resetHScrollbar() { mScreenOffset = 0; mMaxHRange = 0; }
    int getScreenHeight() const { return mScreenHeight; }
    void searchSelectionOnline();
    int getColumnCount() const;
    int getRowCount() const;
    void toggleTimeStamps(const bool);

#if defined(DEBUG_CODEPOINT_PROBLEMS)
    void reportCodepointErrors();
#endif

    void initializeCaret();
    void setCaretPosition(int line, int column);
    void updateCaret();

    QColor mBgColor;
    // position of cursor, in characters, across the entire buffer
    int mCursorY = 0;
    int mCursorX = 0;

    // Position of "caret", the cursor used for accessibility purposes.
    int mCaretLine = 0;
    int mCaretColumn = 0;
    // If the current line is shorter than the previous one, hold here the
    // previous column value so that we can return to it if the next line is
    // long enough again.
    int mOldCaretColumn = 0;

    QColor mFgColor;
    bool mIsCommandPopup = false;
    // If true, this TTextEdit is to display the last lines in
    // mpConsole.mpBuffer. This is always true for the lower main window panel
    // but it is RESET when the upper one is scrolled upwards. The name appears
    // to be related to the file monitoring feature in the *nix tail command.
    // See, e.g.: https://en.wikipedia.org/wiki/Tail_(Unix)#File_monitoring
    bool mIsTailMode = true;
    // The content to use for the current popup (link)
    // Key: is an index stored when the popup is created - this has been
    // changed from the previous "text to show for each popup" to avoid
    // problems with duplicate texts:
    // Value: is the lua code as a string (first) or the lua function reference number (second)
    QMap<int, std::pair<QString, int>> mPopupCommands;
    // How many lines the screen scrolled since it was last rendered.
    int mScrollVector;
    QRegion mSelectedRegion;

public slots:
    void slot_copySelectionToClipboard();
    void slot_copySelectionToSearchBar();
    void slot_selectAll();
    void slot_scrollBarMoved(int);
    void slot_hScrollBarMoved(int);
    void slot_popupMenu();
    void slot_copySelectionToClipboardHTML();
    void slot_searchSelectionOnline();
    void slot_analyseSelection();
    void slot_changeIsAmbigousWidthGlyphsToBeWide(bool);
#if defined(DEBUG_CODEPOINT_PROBLEMS)
    void slot_changeDebugShowAllProblemCodepoints(const bool);
#endif
    void slot_mouseAction(const QString&);

protected:
    void keyPressEvent(QKeyEvent* event) override;
    void focusOutEvent(QFocusEvent* event) override;

private slots:
    void slot_copySelectionToClipboardImage();

private:
    QString getSelectedText(const QChar& newlineChar = QChar::LineFeed, const bool showTimestamps = false);
    static QString htmlCenter(const QString&);
    static QString convertWhitespaceToVisual(const QChar& first, const QChar& second = QChar::Null);
    static QString byteToLuaCodeOrChar(const char*);
    std::pair<bool, int> drawTextForClipboard(QPainter& p, QRect r, int lineOffset) const;
    int convertMouseXToBufferX(const int mouseX, const int lineNumber, bool *isOutOfbounds, bool *isOverTimeStamp = nullptr) const;
    int getGraphemeWidth(uint unicode) const;
    void normaliseSelection();
    void updateTextCursor(const QMouseEvent* event, int lineIndex, int tCharIndex, bool isOutOfbounds);
    bool establishSelectedText();
    void expandSelectionToWords();
    void expandSelectionToLine(int);
    inline void replaceControlCharacterWith_Picture(const uint, const QString&, const int, QVector<QString>&, int&) const;
    inline void replaceControlCharacterWith_OEMFont(const uint, const QString&, const int, QVector<QString>&, int&) const;
    int offsetForPosition(int line, int column) const;

    int mFontHeight;
    int mFontWidth;
    bool mForceUpdate = false;
    const QColor mCaretColor = QColorConstants::Gray;
    const QColor mSearchHighlightFgColor = QColorConstants::Black;
    const QColor mSearchHighlightBgColor = QColorConstants::Yellow;

    // Each TConsole instance uses two instances of this class, one above the
    // other but they need to behave differently in some ways; this flag is set
    // or reset on creation and is used to adjust the behaviour depending on
    // which one this instance is:
    const bool mIsLowerPane;
    // last line offset rendered
    int mLastRenderedOffset = 0;
    bool mMouseTracking = false;
    // 1/2/3 for single/double/triple click seen so far
    int  mMouseTrackLevel = 0;
    bool mCtrlSelecting {};
    // tracks status of the Shift key for keyboard-based selection
    bool mShiftSelection {};
    int mCtrlDragStartY {};
    QPoint mDragStart, mDragSelectionEnd;
    int mOldScrollPos = 0;
    // top-left point of the selection
    QPoint mPA;
    // bottom-right point of the selection
    QPoint mPB;
    TBuffer* mpBuffer;
    // Needs to be a QPointer as is used in a couple of lambda functions:
    QPointer<TConsole> mpConsole;
    QPointer<Host> mpHost;
    // screen height in characters
    int mScreenHeight;
    // currently viewed screen area
    QPixmap mScreenMap;
    int mScreenWidth = 100;
    int mScreenOffset = 0;
    int mMaxHRange = 0;
    QElapsedTimer mLastClickTimer;
    QPointer<QAction> mpContextMenuAnalyser;
    bool mWideAmbigousWidthGlyphs;
    std::chrono::high_resolution_clock::time_point mCopyImageStartTime;
    // How many "normal" width "characters" are each tab stop apart, while
    // there is no current mechanism to adjust this, sensible values will
    // probably be 1 (so that a tab is just treated as a space), 2, 4 and 8,
    // in the past it was typically 8 and this is what we'll use at present:
    int mTabStopwidth = 8;

#if defined(DEBUG_CODEPOINT_PROBLEMS)
    bool mShowAllCodepointIssues = false;
    // Marked mutable so that it is permissible to change this in class methods
    // that are otherwise const!
    mutable QHash<uint, std::tuple<uint, std::string>> mProblemCodepoints;
#endif

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

    // skip over the following characters when searching for a word
    const QStringList mCtrlSelectionIgnores = {" ", ".", ",", ";", ":", "\"", "'", "`", "!", "?", "\\", "/", "|", "~", "*", "(", ")", "[", "]", "{", "}", "<", ">"};
};

#endif // MUDLET_TTEXTEDIT_H
