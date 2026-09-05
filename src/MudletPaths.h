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

// Every on-disk location Mudlet reads or writes, derived from one config root:
// a portable.txt marker beside the executable or in ~/.config/mudlet names it,
// otherwise $XDG_CONFIG_HOME/mudlet and then ~/.config/mudlet are tried. Engine
// code needs these paths before - and, headless, without ever - a main window,
// so they are deliberately not part of class mudlet.
namespace MudletPaths {

struct ConfigDirResolution
{
    QString path;
    // A portable.txt marker named the path; the caller decides whether what it
    // named is usable
    bool portable = false;
    // XDG_CONFIG_HOME is set, but an existing legacy dir was used anyway, so
    // the caller can hint at the migration
    bool migrationPending = false;
    // The legacy default, when it holds profiles that the chosen dir now hides.
    // The caller has to name it, or those profiles read as gone.
    QString shadowedProfilesPath;
};

// Where the running executable lives, or where the AppImage sits when running
// from one
QString executableDir();

// Applies the whole precedence to a given executable directory. Remembers
// nothing, so the Mudlet.ini read that happens before QApplication exists can
// share it.
ConfigDirResolution resolveConfigRoot(const QString& execDir);

// The XDG leg on its own: $XDG_CONFIG_HOME/mudlet takes a tie with
// legacyDefault so that a fresh install lands there
ConfigDirResolution xdgConfigDir(const QString& legacyDefault);

// A directory that cannot be listed must never read as "nothing here": that
// inference is what hides profiles, so assume the strongest content instead.
bool configDirHoldsProfiles(const QString& dir);

// Resolves the config root itself on first use; setConfigPath() replaces it,
// which is how setupConfig() installs the root it has validated
QString getMudletPath(enums::mudletPathType mode, const QString& extra1 = QString(), const QString& extra2 = QString());
void setConfigPath(const QString& path);

// Whether the main dictionary files are the ones bundled with Mudlet (true) or
// ones provided by the system (false). Settled as a side effect of resolving
// enums::hunspellDictionaryPath, so it only answers once that has been asked for.
bool usingMudletDictionaries();

// Replaces filesystem-unsafe characters with underscores and bounds the length.
// Callers file data under the result, so shortening has to keep distinct
// inputs distinct: a shortened name carries a digest of the whole input, since
// plain truncation made two long profile names share - and overwrite - one
// stored password.
QString sanitizeForPath(const QString& input);

} // namespace MudletPaths

#endif // MUDLET_MUDLETPATHS_H
