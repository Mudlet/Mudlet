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

#include <QPair>
#include <QString>

// Every on-disk location Mudlet uses, derived from one config root: named by a portable.txt beside the
// executable or in ~/.config/mudlet, else $XDG_CONFIG_HOME/mudlet, else ~/.config/mudlet. Not part of
// class mudlet because engine code needs these before, or headless without, a main window.
namespace MudletPaths {

struct ConfigDirResolution
{
    QString path;
    // A portable.txt marker named the path; the caller decides whether it is usable
    bool portable = false;
    // The marker's root is unusable, so path is the non-portable location. Startup refuses to run on
    // one; a caller resolving before then carries on with the fallback.
    bool portableRootRejected = false;
    // XDG_CONFIG_HOME is set, but an existing legacy dir was used anyway, so
    // the caller can hint at the migration
    bool migrationPending = false;
    // The legacy default, when it holds profiles that the chosen dir now hides.
    // The caller has to name it, or those profiles read as gone.
    QString shadowedProfilesPath;
};

// Where the running executable lives, or the AppImage when running from one
QString executableDir();

// ~/.config/mudlet: the pre-XDG default, and the second place a portable.txt
// marker is looked for
QString legacyConfigDir();

// The governing portable.txt, or empty; the one beside the executable outranks the config dir's.
// Only stats, so cheap enough to ask on every operation.
QString portableMarkerPath(const QString& execDir, const QString& configDir = legacyConfigDir());

// Whether a portable.txt's root is usable: non-empty, not an existing non-directory (a dangling symlink
// included, as mkpath() can't create through one), and with an existing parent. Warns why it refuses.
bool portableRootUsable(const QString& path);

// The whole precedence for a given executable directory. Stateless, so the Mudlet.ini read before
// QApplication exists can share it. Never returns an empty root: an unusable portable one falls back to
// the non-portable location with portableRootRejected set.
ConfigDirResolution resolveConfigRoot(const QString& execDir, const QString& configDir = legacyConfigDir());

// The XDG leg on its own: $XDG_CONFIG_HOME/mudlet takes a tie with
// legacyDefault so that a fresh install lands there
ConfigDirResolution xdgConfigDir(const QString& legacyDefault);

// A directory that cannot be listed must never read as "nothing here": that
// inference is what hides profiles, so assume the strongest content instead.
bool configDirHoldsProfiles(const QString& dir);

// Resolves the root on first use and caches it. setConfigPath() replaces it (that is how setupConfig()
// installs the validated root); an empty path forgets it instead.
QString getMudletPath(enums::mudletPathType mode, const QString& extra1 = QString(), const QString& extra2 = QString());
void setConfigPath(const QString& path);

// Whether the main dictionary files are the ones bundled with Mudlet (true) or
// ones provided by the system (false). Settled as a side effect of resolving
// enums::hunspellDictionaryPath, so it only answers once that has been asked for.
bool usingMudletDictionaries();

// Replaces filesystem-unsafe characters with underscores and bounds the length. A shortened name carries
// a digest of the whole input, as callers file data (e.g. passwords) under it and must not collide.
QString sanitizeForPath(const QString& input);

QString readProfileData(const QString& profile, const QString& item);

// Creates the profile's directory if missing, so writing for a nonexistent profile brings it into being
QPair<bool, QString> writeProfileData(const QString& profile, const QString& item, const QString& what);

// The on-disk spelling of a profile named in any case, or an empty string if
// neither an existing profile nor a predefined game goes by that name
QString getCanonicalProfileName(const QString& profileName);

} // namespace MudletPaths

#endif // MUDLET_MUDLETPATHS_H
