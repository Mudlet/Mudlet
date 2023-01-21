#ifndef MUDLET_TCOMMANDLINE_H
#define MUDLET_TCOMMANDLINE_H

/***************************************************************************
 *   Copyright (C) 2008-2011 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2018-2019, 2022-2023 by Stephen Lyons                   *
 *                                               - slysven@virginmedia.com *
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


#include "TConsole.h"

#include "pre_guard.h"
#include <QPlainTextEdit>
#include <QPointer>
#include <QString>
#include <QStringList>
#include <QTextDecoder>
#include "post_guard.h"


class KeyUnit;
class Host;

class TCommandLine : public QPlainTextEdit //QLineEdit
{
    Q_OBJECT

    enum MoveDirection {
        MOVE_UP,
        MOVE_DOWN
    };

public:
    enum CommandLineTypeFlag {
        UnknownType = 0x0,     // Should not be encountered but left as a trap value
        MainCommandLine = 0x1, // One per profile
        SubCommandLine = 0x2,  // Overlaid on top of TMainConsole or TConsole instance, should be uniquely named in pool of SubCommandLine/SubConsole/UserWindow/Buffers AND Labels
        ConsoleCommandLine = 0x4,  // Integrated in TConsoles other than those derived into a TMainConsole
    };

    Q_DECLARE_FLAGS(CommandLineType, CommandLineTypeFlag)

    Q_DISABLE_COPY(TCommandLine)
    explicit TCommandLine(Host*, const QString&, CommandLineType type = UnknownType, TConsole* pConsole = nullptr, QWidget* parent = nullptr);
    void focusInEvent(QFocusEvent*) override;
    void focusOutEvent(QFocusEvent*) override;
    void hideEvent(QHideEvent*) override;
    void recheckWholeLine();
    void clearMarksOnWholeLine();
    void setAction(const int);
    void resetAction();
    void releaseFunc(const int, const int);
    CommandLineType getType() const { return mType; }
    void addSuggestion(const QString&);
    void removeSuggestion(const QString&);
    void clearSuggestions();
    void adjustHeight();
    TConsole* console() const { return mpConsole; }

    int mActionFunction = 0;
    QPalette mRegularPalette;
    QString mCommandLineName;

    QMap<QString, QString> contextMenuItems;

public slots:
    void slot_popupMenu();
    void slot_addWord();
    void slot_removeWord();
    void slot_clearSelection(bool yes);
    void slot_adjustAccessibleNames();

private:
    bool event(QEvent*) override;
    void mousePressEvent(QMouseEvent*) override;
    void mouseReleaseEvent(QMouseEvent*) override;
    void handleAutoCompletion();
    void spellCheck();
    void handleTabCompletion(bool);
    void historyMove(MoveDirection);
    void enterCommand(QKeyEvent*);
    void processNormalKey(QEvent*);
    bool keybindingMatched(QKeyEvent*);
    void spellCheckWord(QTextCursor& c);
    bool handleCtrlTabChange(QKeyEvent* key, int tabNumber);

    QPointer<Host> mpHost;
    CommandLineType mType = UnknownType;
    KeyUnit* mpKeyUnit = nullptr;
    QPointer<TConsole> mpConsole;
    QString mLastCompletion;
    int mTabCompletionCount = 0;
    int mAutoCompletionCount = 0;
    QString mTabCompletionTyped;
    bool mUserKeptOnTyping = false;
    int mHistoryBuffer = 0;
    QStringList mHistoryList;
    QString mSelectedText;
    int mSelectionStart = 0;
    QString mTabCompletionOld;
    QPoint mPopupPosition;
    QString mSpellCheckedWord;
    bool mSpellChecking = false;
    int mSystemDictionarySuggestionsCount = 0;
    int mUserDictionarySuggestionsCount = 0;
    char** mpSystemSuggestionsList = nullptr;
    char** mpUserSuggestionsList = nullptr;
    QSet<QString> commandLineSuggestions;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(TCommandLine::CommandLineType)

#endif // MUDLET_TCOMMANDLINE_H
