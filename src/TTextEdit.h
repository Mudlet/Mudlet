#ifndef MUDLET_TTEXTEDIT_H
#define MUDLET_TTEXTEDIT_H

/***************************************************************************
 *   Copyright (C) 2008-2011 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2015, 2018 by Stephen Lyons - slysven@virginmedia.com   *
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
#include <QMap>
#include <QPointer>
#include <QTime>
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
    void drawBackground(QPainter&, const QRect&, const QColor&) const;
    uint getGraphemeBaseCharacter(const QString& str) const;
    void drawLine(QPainter& painter, int lineNumber, int rowOfScreen) const;
    int drawGrapheme(QPainter &painter, const QPoint &cursor, const QString &c, int column, TChar &style) const;
    void drawCharacters(QPainter&, const QRect&, QString&, const QColor&, const TChar::AttributeFlags);
    void showNewLines();
    void forceUpdate();
    void needUpdate(int, int);
    void scrollTo(int);
    void scrollUp(int lines);
    void scrollDown(int lines);
    void wheelEvent(QWheelEvent* e) override;
    void resizeEvent(QResizeEvent* event) override;
    void mousePressEvent(QMouseEvent*) override;
    void mouseReleaseEvent(QMouseEvent*) override;
    void mouseMoveEvent(QMouseEvent*) override;
    void showEvent(QShowEvent* event) override;
    void updateScreenView();
    void highlightSelection();
    void unHighlight();
    void focusInEvent(QFocusEvent* event) override;
    int imageTopLine();
    int bufferScrollUp(int lines);
    int bufferScrollDown(int lines);
// Not used:    void setConsoleFgColor(int r, int g, int b) { mFgColor = QColor(r, g, b); }
    void setConsoleBgColor(int r, int g, int b) { mBgColor = QColor(r, g, b); }
    void searchSelectionOnline();
    int getColumnCount();
    int getRowCount();

    QColor mBgColor;
    // position of cursor, in characters, across the entire buffer
    int mCursorY;
    QFont mDisplayFont;
    QColor mFgColor;
    int mFontAscent;
    int mFontDescent;
    bool mIsCommandPopup;
    // If true, this TTextEdit is to display the last lines in
    // mpConsole.mpBuffer. This is always true for the lower main window panel
    // but it is RESET when the upper one is scrolled upwards. The name appears
    // to be related to the file monitoring feature in the *nix tail command.
    // See, e.g.: https://en.wikipedia.org/wiki/Tail_(Unix)#File_monitoring
    bool mIsTailMode;
    QMap<QString, QString> mPopupCommands;
    int mScrollVector;
    QRegion mSelectedRegion;
    bool mShowTimeStamps;
    int mWrapAt;
    int mWrapIndentCount {};

public slots:
    void slot_toggleTimeStamps(const bool);
    void slot_copySelectionToClipboard();
    void slot_selectAll();
    void slot_scrollBarMoved(int);
    void slot_popupMenu();
    void slot_copySelectionToClipboardHTML();
    void slot_searchSelectionOnline();
    void slot_analyseSelection();
    void slot_changeIsAmbigousWidthGlyphsToBeWide(bool);

private slots:
    void slot_copySelectionToClipboardImage();

private:
    void initDefaultSettings();
    QString getSelectedText(char newlineChar = '\n');
    static QString htmlCenter(const QString&);
    static QString convertWhitespaceToVisual(const QChar& first, const QChar& second = QChar::Null);
    static QString byteToLuaCodeOrChar(const char*);
    std::pair<bool, int> drawTextForClipboard(QPainter& p, QRect r, int lineOffset) const;
    int convertMouseXToBufferX(const int mouseX, const int lineNumber, bool *isOverTimeStamp = nullptr) const;
    int getGraphemeWidth(uint unicode) const;
    void normaliseSelection();
    void updateTextCursor(const QMouseEvent* event, int lineIndex, int tCharIndex);
    void raiseMudletMousePressOrReleaseEvent(QMouseEvent*, const bool);

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
    bool mCtrlSelecting {};
    int mCtrlDragStartY {};
    QPoint mDragStart, mDragSelectionEnd;
    int mOldScrollPos;
    // top-left point of the selection
    QPoint mPA;
    // bottom-right point of the selection
    QPoint mPB;
    TBuffer* mpBuffer;
    TConsole* mpConsole;
    QPointer<Host> mpHost;
    QScrollBar* mpScrollBar;
    // screen height in characters
    int mScreenHeight;
    // currently viewed screen area
    QPixmap mScreenMap;
    int mScreenWidth;
    QTime mLastClickTimer;
    QPointer<QAction> mpContextMenuAnalyser;
    bool mWideAmbigousWidthGlyphs;
    std::chrono::high_resolution_clock::time_point mCopyImageStartTime;
    // Set in constructor for run-time Qt versions less than 5.11 which only
    // supports up to Unicode 8.0:
    bool mUseOldUnicode8;
    // How many "normal" width "characters" are each tab stop apart, whilst
    // there is no current mechanism to adjust this, sensible values will
    // probably be 1 (so that a tab is just treated as a space), 2, 4 and 8,
    // in the past it was typically 8 and this is what we'll use at present:
    int mTabStopwidth;
    // How many normal width characters that are used for the time stamps; it
    // would only be valid to change this by clearing the buffer first - so
    // making this a const value for the moment:
    const int mTimeStampWidth;
};

#endif // MUDLET_TTEXTEDIT_H
