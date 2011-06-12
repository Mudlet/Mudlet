/***************************************************************************
 *   Copyright (C) 2008-2009 by Heiko Koehn   *
 *   KoehnHeiko@googlemail.com   *
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

#ifndef TCOMMANDLINE_H
#define TCOMMANDLINE_H

#include <QLineEdit>
#include <QMainWindow>
#include <QCloseEvent>
//#include "ui_console.h"
//#include <QtWebKit>
#include <iostream>
#include "ctelnet.h"
#include "TCommandLine.h"
#include "Host.h"
#include <QPlainTextEdit>
#include <hunspell/hunspell.h>

class TConsole;
class KeyUnit;
class Host;
class TConsole;

class TCommandLine : public QPlainTextEdit//QLineEdit
{
    Q_OBJECT

public:

                      TCommandLine( Host *, TConsole *, QWidget * );
//    void              keyPressEvent(QKeyEvent *event);
    void              focusInEvent ( QFocusEvent * );
    void              focusOutEvent ( QFocusEvent * );
    QPalette          mRegularPalette;

private:
    QString           mLastCompletion;
    void              handleAutoCompletion();
    void              spellCheck();
    void              handleTabCompletion( bool direction );
    void              historyUp(QKeyEvent *event);
    void              historyDown(QKeyEvent *event);
    bool              event(QEvent *event);
    void              enterCommand(QKeyEvent *event);
    void              adjustHeight();
    void              mousePressEvent( QMouseEvent * event );

    int               mHistoryBuffer;
    QStringList       mHistoryList;
    QMap<QString,int> mHistoryMap;
    bool              mAutoCompletion;
    bool              mTabCompletion;
    Host *            mpHost;
    int               mTabCompletionCount;
    int               mAutoCompletionCount;
    QString           mTabCompletionTyped;
    QString           mAutoCompletionTyped;
    bool              mUserKeptOnTyping;

    QPalette          mTabCompletionPalette;
    QPalette          mAutoCompletionPalette;
    KeyUnit *         mpKeyUnit;
    TConsole *        mpConsole;
    QString           mSelectedText;
    int               mSelectionStart;
    QString           mTabCompletionOld;
    Hunhandle *       mpHunspell;
    QPoint            mPopupPosition;
    int               mHunspellSuggestionNumber;
    char **           mpHunspellSuggestionList;

signals:
    void              textChanged ( const QString & );

public slots:
    void              slot_textChanged(const QString &);
    void              slot_sendCommand(const char * pS);
    void              slot_popupMenu();
};

#endif

