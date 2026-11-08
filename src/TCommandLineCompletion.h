#ifndef MUDLET_TCOMMANDLINECOMPLETION_H
#define MUDLET_TCOMMANDLINECOMPLETION_H

/***************************************************************************
 *   Copyright (C) 2026 by Mudlet Developers                               *
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

#include <QSet>
#include <QString>

#include <optional>

class TBuffer;

// The cycle Tab and Shift+Tab go through on a command line, shared by the GUI's command lines and the main one of a
// profile with no main window, so both complete alike
class TCommandLineCompletion
{
public:
    struct TabCompletion
    {
        // The whole line, its last word completed
        QString line;
        // The word it was completed to
        QString word;
    };

    // The last word of the line completed to the next word that starts with it, ignoring case, from the last 500 lines
    // of recent, the main console's buffer, and suggestions, less blacklist, the most recently seen first, stopping at
    // either end; Shift+Tab is !forward. line is read as a cycle starts, after an edit or reset(). Nullopt when there is
    // nothing to complete.
    std::optional<TabCompletion> tabComplete(const QString& line, bool forward, TBuffer& recent, const QSet<QString>& suggestions, const QSet<QString>& blacklist);
    // What the line shows, once set to what was completed: the next edit is told against it
    void setShownLine(const QString& line) { mShownLine = line; }
    const QString& shownLine() const { return mShownLine; }
    // The line changed by any other key, so the next Tab completes what it now holds; true if it differs from what was
    // last shown
    bool lineEdited(const QString& line);
    // A space, a deletion, Escape or a send ends the cycle
    void reset();

    struct HistoryCompletion
    {
        // What the player typed: the line, less the tail the last completion filled in if that is still selected
        QString typed;
        // The first line of history from the walk's place on that starts with typed; nullopt with none, and the walk
        // starts again from the newest
        std::optional<QString> line;
        bool tailCut = false;
    };
    // Up, or Down for !forward, on a line typed into, with "Highlight history" on: each call goes on to the next older
    // line of history that starts with what was typed. Once line is shown, setHistoryCompletion() with what the line
    // then holds.
    HistoryCompletion historyComplete(const QString& line, bool tailSelected, const QStringList& history, bool forward);
    void setHistoryCompletion(const QString& line) { mLastCompletion = line; }
    // An edit: the next Up looks from the newest line again
    void restartHistoryWalk() { mHistoryCount = -1; }
    // A space, a deletion or a send also forgets the line the walk last found
    void resetHistoryWalk();

private:
    // Starts at 0, not -1, as TCommandLine's always has
    int mTabCount = 0;
    int mHistoryCount = 0;
    QString mLastCompletion;
    QString mTabTyped;
    bool mUserKeptOnTyping = false;
    QString mShownLine;
};

#endif // MUDLET_TCOMMANDLINECOMPLETION_H
