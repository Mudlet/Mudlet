#ifndef MUDLET_TTEXTEDIT_H
#define MUDLET_TTEXTEDIT_H

/***************************************************************************
 *   Copyright (C) 2008-2011 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2015 by Stephen Lyons - slysven@virginmedia.com         *
 *   Copyright (C) 2016-2017 by Ian Adkins - ieadkins@gmail.com            *
 *   Copyright (C) 2017 by Chris Reid - WackyWormer@hotmail.com            *
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


#include "pre_guard.h"
#include <QMap>
#include <QPointer>
#include <QTime>
#include <QWidget>
#include "post_guard.h"

#include <string>

class Host;
class TBuffer;
class TConsole;

class QScrollBar;
class QString;


class TTextEdit : public QWidget
{
    Q_OBJECT

public:
    Q_DISABLE_COPY(TTextEdit)
    TTextEdit(TConsole*, QWidget*, TBuffer* pB, Host* pH, bool isDebugConsole, bool isSplitScreen);
    void paintEvent(QPaintEvent*) override;
    void contextMenuEvent(QContextMenuEvent* event) override;
    void drawForeground(QPainter&, const QRect&);
    void drawFrame(QPainter&, const QRect&);
    void drawBackground(QPainter&, const QRect&, const QColor&);
    void updateLastLine();
    void drawCharacters(QPainter& painter, const QRect& rect, QString& text, bool isBold, bool isUnderline, bool isItalics, bool isStrikeOut, QColor& fgColor, QColor& bgColor);
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
    void highlight();
    void unHighlight();
    void swap(QPoint& p1, QPoint& p2);
    void focusInEvent(QFocusEvent* event) override;
    int imageTopLine();
    int bufferScrollUp(int lines);
    int bufferScrollDown(int lines);
    bool isTailMode();
    void copySelectionToClipboard();
    void setConsoleFgColor(int r, int g, int b) { mFgColor = QColor(r, g, b); }
    void setConsoleBgColor(int r, int g, int b) { mBgColor = QColor(r, g, b); }
    void setIsMiniConsole() { mIsMiniConsole = true; }
    void copySelectionToClipboardHTML();
    void searchSelectionOnline();
    int getColumnCount();
    int getRowCount();

    QColor mBgColor;
    int mCursorY;
    QFont mDisplayFont;
    QColor mFgColor;
    int mFontAscent;
    int mFontDescent;
    bool mIsCommandPopup;
    bool mIsTailMode;
    QMap<QString, QString> mPopupCommands;
    int mScrollVector;
    QRegion mSelectedRegion;
    bool mShowTimeStamps;
    int mWrapAt;
    int mWrapIndentCount;
    qreal mLetterSpacing;

public slots:
    void slot_toggleTimeStamps();
    void slot_copySelectionToClipboard();
    void slot_selectAll();
    void slot_scrollBarMoved(int);
    void slot_popupMenu();
    void slot_copySelectionToClipboardHTML();
    void slot_searchSelectionOnline();
    void slot_analyseSelection();

private:
    void initDefaultSettings();
    QString getSelectedText(char newlineChar = '\n');
    QString convertWhiteSpaceToVisual(const QChar& first);
// Not needed?    QString convertWhiteSpaceToVisual(const QChar& first, const QChar& second);


    int mFontHeight;
    int mFontWidth;
    bool mForceUpdate;
    bool mHighlight_on;
    bool mHighlightingBegin;
    bool mHighlightingEnd;
    bool mInit_OK;
    bool mInversOn;
    bool mIsDebugConsole;
    bool mIsMiniConsole;
    bool mIsSplitScreen;
    int mLastRenderBottom;
    int mLeftMargin;
    bool mMouseTracking;
    bool mCtrlSelecting;
    int mDragStartY;
    int mOldScrollPos;
    QPoint mPA;
    QPoint mPB;
    TBuffer* mpBuffer;
    TConsole* mpConsole;
    QPointer<Host> mpHost;
    QScrollBar* mpScrollBar;
    int mScreenHeight;
    QPixmap mScreenMap;
    int mScreenWidth;
    QTime mLastClickTimer;
    QPointer<QAction> mpContextMenuAnalyser;
};

#endif // MUDLET_TTEXTEDIT_H
