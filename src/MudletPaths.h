#ifndef MUDLET_MUDLETPATHS_H
#define MUDLET_MUDLETPATHS_H

/***************************************************************************
 *   Copyright (C) 2017 by WackyWormer - WackyWormer@hotmail.com           *
 *   Copyright (C) 2017, 2019-2020, 2022 by Stephen Lyons                  *
 *                                               - slysven@virginmedia.com *
 *   Copyright (C) 2017, 2021 by Vadim Peretokin - vperetokin@gmail.com    *
 *   Copyright (C) 2019 by Mike Conley - sousesider@gmail.com              *
 *   Copyright (C) 2023 by Geert Konijnendijk - geert@konijnendijk.info    *
 *   Copyright (C) 2024 by ConcurrentCrab                                  *
 *   Copyright (C) 2025-2026 by Vadim Peretokin                            *
 *                                            - vadim.peretokin@mudlet.org *
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

#include "enums.h"

#include <QString>

// Every on-disk location Mudlet reads or writes, derived from the one
// configuration root that setConfigPath() settles at startup. Engine code needs
// those paths before - and, headless, without ever - a main window, so they are
// deliberately not part of class mudlet.
class MudletPaths
{
public:
    MudletPaths() = delete;
    MudletPaths(const MudletPaths&) = delete;
    MudletPaths& operator=(const MudletPaths&) = delete;

    // Answers with a path rooted at an empty string until setConfigPath() has run,
    // so an unconfigured call yields "/profiles/<name>" rather than failing
    static QString getMudletPath(enums::mudletPathType mode, const QString& extra1 = QString(), const QString& extra2 = QString());
    static void setConfigPath(const QString& path);
    // Whether the main dictionary files are the ones bundled with Mudlet (true)
    // or ones provided by the system (false). Settled as a side effect of
    // resolving enums::hunspellDictionaryPath, so it only answers once that has
    // been asked for.
    static bool usingMudletDictionaries();

private:
    inline static QString smConfigPath;
    inline static bool smUsingMudletDictionaries = false;
};

#endif // MUDLET_MUDLETPATHS_H
