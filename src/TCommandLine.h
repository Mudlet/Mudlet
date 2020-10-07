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
#include <QPlainTextEdit>
#include <QPointer>
#include <QString>
#include <QStringList>
#include <QTextDecoder>
#include "post_guard.h"


class TConsole;
class KeyUnit;
class Host;


class TCommandLine : public QPlainTextEdit //QLineEdit
{
    Q_OBJECT

public:
    enum CommandLineTypeFlag {
        UnknownType = 0x0,     // Should not be encountered but left as a trap value
        MainCommandLine = 0x1, // One per profile
        SubCommandLine = 0x2,  // Overlaid on top of MainConsole instance, should be uniquely named in pool of SubCommandLine/SubConsole/UserWindow/Buffers AND Labels
        ConsoleCommandLine = 0x4,  // Integrated in MiniConsoles
    };

    Q_DECLARE_FLAGS(CommandLineType, CommandLineTypeFlag)

    Q_DISABLE_COPY(TCommandLine)
    explicit TCommandLine(Host*, CommandLineType type = UnknownType, TConsole* pConsole = nullptr, QWidget* parent = nullptr);
    void focusInEvent(QFocusEvent*) override;
    void focusOutEvent(QFocusEvent*) override;
    void hideEvent(QHideEvent*) override;
    void recheckWholeLine();
    void clearMarksOnWholeLine();
    void setAction(const int);
    void resetAction();
    void releaseFunc(const int, const int);
    CommandLineType getType() const { return mType; }

    int mActionFunction = 0;
    QPalette mRegularPalette;
    QString mCommandLineName;


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
    bool keybindingMatched(QKeyEvent*);
    CommandLineType mType;

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
    void slot_addWord();
    void slot_removeWord();


private:
    int mHistoryBuffer;
    QStringList mHistoryList;
    QString mSelectedText;
    int mSelectionStart;
    QString mTabCompletionOld;
    QPoint mPopupPosition;
    QString mSpellCheckedWord;
    int mSystemDictionarySuggestionsCount;
    int mUserDictionarySuggestionsCount;
    char** mpSystemSuggestionsList;
    char** mpUserSuggestionsList;
    void spellCheckWord(QTextCursor& c);
    bool handleCtrlTabChange(QKeyEvent* key, int tabNumber);
};

Q_DECLARE_OPERATORS_FOR_FLAGS(TCommandLine::CommandLineType)

#endif // MUDLET_TCOMMANDLINE_H
