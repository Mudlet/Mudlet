#ifndef MUDLET_TCOMMANDLINE_H
#define MUDLET_TCOMMANDLINE_H

/***************************************************************************
 *   Copyright (C) 2008-2011 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2018 by Stephen Lyons - slysven@virginmedia.com         *
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
#include "post_guard.h"

#include <hunspell/hunspell.h>

class TConsole;
class KeyUnit;
class Host;
class TConsole;


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
    QString mLastCompletion;
    void handleAutoCompletion();
    void spellCheck();
    void handleTabCompletion(bool direction);
    void historyUp(QKeyEvent* event);
    void historyDown(QKeyEvent* event);
    bool event(QEvent* event) override;
    void enterCommand(QKeyEvent* event);
    void adjustHeight();
    void mousePressEvent(QMouseEvent* event) override;
    void processNormalKey(QEvent*);
    bool processPotentialKeyBinding(QKeyEvent*);

    int mHistoryBuffer;
    QStringList mHistoryList;
    QMap<QString, int> mHistoryMap;
    bool mAutoCompletion;
    bool mTabCompletion;
    QPointer<Host> mpHost;
    int mTabCompletionCount;
    int mAutoCompletionCount;
    QString mTabCompletionTyped;
    QString mAutoCompletionTyped;
    bool mUserKeptOnTyping;

    QPalette mTabCompletionPalette;
    QPalette mAutoCompletionPalette;
    KeyUnit* mpKeyUnit;
    TConsole* mpConsole;
    QString mSelectedText;
    int mSelectionStart;
    QString mTabCompletionOld;
    Hunhandle* mpHunspell;
    QPoint mPopupPosition;
    int mHunspellSuggestionNumber;
    char** mpHunspellSuggestionList;


public slots:
    void slot_popupMenu();
};

#endif // MUDLET_TCOMMANDLINE_H
