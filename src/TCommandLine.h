#ifndef MUDLET_TCOMMANDLINE_H
#define MUDLET_TCOMMANDLINE_H

/***************************************************************************
 *   Copyright (C) 2008-2011 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2018-2019 by Stephen Lyons - slysven@virginmedia.com    *
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
#include <QPlainTextEdit>
#include <QPointer>
#include <QString>
#include <QStringList>
#include <QTextDecoder>
#include "post_guard.h"

#include <hunspell/hunspell.hxx>
#include <hunspell/hunspell.h>

class TConsole;
class KeyUnit;
class Host;


class TCommandLine : public QPlainTextEdit //QLineEdit
{
    Q_OBJECT

public:
    Q_DISABLE_COPY(TCommandLine)
    TCommandLine(Host*, TConsole*, QWidget*);
    ~TCommandLine();
    void focusInEvent(QFocusEvent*) override;
    void focusOutEvent(QFocusEvent*) override;


    QPalette mRegularPalette;


private:
    bool event(QEvent*) override;
    void mousePressEvent(QMouseEvent*) override;
    void handleAutoCompletion();
    void spellCheck();
    void handleTabCompletion(bool);
    void historyUp(QKeyEvent*);
    void historyDown(QKeyEvent*);
    void enterCommand(QKeyEvent*);
    void adjustHeight();
    void processNormalKey(QEvent*);
    bool processPotentialKeyBinding(QKeyEvent*);


    QPointer<Host> mpHost;
    KeyUnit* mpKeyUnit;
    TConsole* mpConsole;
    QString mLastCompletion;
    int mTabCompletionCount;
    int mAutoCompletionCount;
    QString mTabCompletionTyped;
    bool mUserKeptOnTyping;


public slots:
    void slot_popupMenu();
    void slot_changeSpellDict(const QString&);


private:
    int mHistoryBuffer;
    QStringList mHistoryList;
    QString mSelectedText;
    int mSelectionStart;
    QString mTabCompletionOld;
    QPoint mPopupPosition;
    Hunhandle* mpHunspell;
    QByteArray mHunspellCodecName;
    QTextCodec* mpHunspellCodec;
    int mHunspellSuggestionNumber;
    char** mpHunspellSuggestionList;
};

#endif // MUDLET_TCOMMANDLINE_H
