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

#include "TCommandLineCompletion.h"

#include "TBuffer.h"
#include "utils.h"

#include <QRegularExpression>
#include <QStringList>

#include <algorithm>

// Tab completion finds words in what the game sent recently that start with the letters being typed, so the player
// need only type the first letters of a complicated name. Each further Tab or Shift+Tab moves through the others,
// and Escape leaves the cycle.
std::optional<TCommandLineCompletion::TabCompletion>
TCommandLineCompletion::tabComplete(const QString& line, const bool forward, TBuffer& recent, const QSet<QString>& suggestions, const QSet<QString>& blacklist)
{
    if ((mTabCount < 0) || (mUserKeptOnTyping)) {
        mTabTyped = line;
        if (mTabTyped.isEmpty()) {
            return std::nullopt;
        }
        mUserKeptOnTyping = false;
        mTabCount = -1;
    }
    int amount = recent.size();
    if (amount > 500) {
        amount = 500;
    }

    const QStringList bufferList = recent.getEndLines(amount);
    QString buffer = bufferList.join(QChar::Space);

    buffer.replace(QChar(0x21af), QChar::LineFeed);
    buffer.replace(QChar::LineFeed, QChar::Space);

    if (forward) {
        mTabCount++;
    } else {
        mTabCount--;
    }
    const QRegularExpression reg = QRegularExpression(qsl(R"(\b(\w+)$)"), QRegularExpression::UseUnicodePropertiesOption);
    const QRegularExpressionMatch match = reg.match(mTabTyped);
    // Nothing to complete after a space or punctuation: an empty part-word would match every word in the buffer
    if (!match.hasMatch()) {
        return std::nullopt;
    }
    const int typePosition = match.capturedStart();
    const QString lastWord = match.captured(1);

    // One pass over the buffer collects only the words that can complete lastWord, so
    // deduplicating and the blacklist work on those rather than on every word
    const auto options = QRegularExpression::CaseInsensitiveOption | QRegularExpression::UseUnicodePropertiesOption;
    QStringList candidates;
    const QRegularExpression bufferWord(qsl(R"((?<!\w)%1\w+)").arg(lastWord), options);
    for (const QRegularExpressionMatch& wordMatch : bufferWord.globalMatch(buffer)) {
        candidates.append(wordMatch.captured());
    }
    const QRegularExpression suggestion(qsl(R"(^%1\w+)").arg(lastWord), options);
    for (const QString& word : suggestions) {
        if (word.contains(suggestion)) {
            candidates.append(word);
        }
    }

    // Each word once, the most recently seen first
    QStringList filterList;
    QSet<QString> seen;
    for (auto it = candidates.crbegin(); it != candidates.crend(); ++it) {
        if (seen.contains(*it)) {
            continue;
        }
        seen.insert(*it);
        const bool blacklisted = std::any_of(blacklist.cbegin(), blacklist.cend(), [it](const QString& banned) {
            return it->compare(banned, Qt::CaseInsensitive) == 0;
        });
        if (!blacklisted) {
            filterList.append(*it);
        }
    }

    if (filterList.empty()) {
        return std::nullopt;
    }
    if (mTabCount >= filterList.size()) {
        mTabCount = filterList.size() - 1;
    }
    if (mTabCount < 0) {
        mTabCount = 0;
    }

    const QString proposal = filterList[mTabCount];
    const QString userWords = mTabTyped.left(typePosition);
    return TabCompletion{userWords + proposal, proposal};
}

bool TCommandLineCompletion::lineEdited(const QString& line)
{
    mUserKeptOnTyping = mShownLine != line;
    return mUserKeptOnTyping;
}

// TCommandLine's keys each reset part of this; the next Tab reads the whole line again after any of them
void TCommandLineCompletion::reset()
{
    mTabCount = -1;
    mTabTyped.clear();
    mUserKeptOnTyping = false;
}

TCommandLineCompletion::HistoryCompletion TCommandLineCompletion::historyComplete(const QString& line, const bool tailSelected, const QStringList& history, const bool forward)
{
    mHistoryCount += forward ? 1 : -1;
    HistoryCompletion result;
    result.typed = line;
    // Only the tail the previous completion filled in may be cut; any other selection is the player's own text
    result.tailCut = line == mLastCompletion && tailSelected;
    if (result.tailCut) {
        result.typed.truncate(mShownLine.size());
    }
    mShownLine = result.typed;
    if (mHistoryCount >= history.size()) {
        mHistoryCount = history.size() - 1;
    }
    if (mHistoryCount < 0) {
        mHistoryCount = 0;
    }
    for (int i = mHistoryCount; i < history.size(); i++) {
        if (history[i].startsWith(result.typed)) {
            mHistoryCount = i;
            result.line = history[i];
            return result;
        }
    }
    mHistoryCount = -1;
    return result;
}

void TCommandLineCompletion::resetHistoryWalk()
{
    mHistoryCount = -1;
    mLastCompletion.clear();
}
