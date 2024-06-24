#ifndef TGAMEDETAILS_H
#define TGAMEDETAILS_H
/***************************************************************************
 *   Copyright (C) 2022 by Stephen Lyons - slysven@virginmedia.com         *
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

#include "utils.h" // For qsl(...)

#include "pre_guard.h"
#include <QString>
#include <QList>
#include "post_guard.h"

struct GameDetail
{
    QString name;
    QString hostUrl;
    int port = 0;
    bool tlsEnabled = false;
    QString websiteInfo;
    QString icon;
    QString description;
};

class TGameDetails
{
public:
    inline static const QList<GameDetail>::const_iterator findGame(const QString& key, const Qt::CaseSensitivity sensitivity = Qt::CaseSensitive)
    {
        QList<GameDetail>::const_iterator i;
        for (i = scmDefaultGames.constBegin(); i != scmDefaultGames.constEnd(); ++i) {
            if (!(*i).name.compare(key, sensitivity)) {
                return i;
            }
        }
        // Return the past-the-end iterator if not found:
        return i;
    }

    inline static const QStringList keys()
    {
        QStringList result;
        for (auto i = scmDefaultGames.constBegin(); i != scmDefaultGames.constEnd(); ++i) {
            result << (*i).name;
        }
        return result;
    }

    // clang-format off
    // games are to be added here in alphabetical order
    inline static const QList<GameDetail> scmDefaultGames = {
            {qsl("Cleft of Dimensions"), // Name
             qsl("cleftofdimensions.net"), // address to connect to
             4354, // port to connect on
             false, // secure connection possible?
             qsl("<a href='https://www.cleftofdimensions.net/'>cleftofdimensions.net</a><br>"
                 "<a href='https://discord.gg/cSqkpbu'>Discord Guild</a>"), // Website or other URLs
             qsl(":/icons/cleftofdimensions.png"),
             qsl("Do you have a soft spot for an old SNES RPG? Are you a fan of retro gaming? The "
                 "Cleft of Dimensions is an adventure-driven MUD with content inspired by a variety "
                 "of classic video games. Do you want to jump on goombas? Maybe you'd rather "
                 "immolate them with lava or bombard them with meteors. Then again, why fight when "
                 "enslavement's an option? If that doesn't work out, you've got this motorcycle you "
                 "could crash into them. The Cleft has 16 character classes, each with a "
                 "distinctive playstyle."
                 "\n\n"
                 "Gameplay in the Cleft features exploration, puzzles, quests, and combat. At time "
                 "of writing, the world contains 98 areas. Quests range from deciphering treasure "
                 "maps and committing industrial espionage to seeking the blessings of the mana "
                 "spirits or just going fishing. A remort system facilitates repeat playthroughs to "
                 "find content you missed the first time around."
                 "\n\n"
                 "The Cleft opened in July 2000 and has been in active development ever since. We're "
                 "always innovating. Recent features include Discord integration "
                 "and areas written with artificial intelligence. Check us out!")}, // Text to use for description
            };
    // clang-format on
};

#endif // TGAMEDETAILS_H
