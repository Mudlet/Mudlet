#ifndef MUDLET_MUDLETAPP_H
#define MUDLET_MUDLETAPP_H

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

class QNetworkRequest;
class QSettings;
class QUrl;

// The application-wide services that need no main window: where Mudlet keeps
// its files, its persistent settings and which build it is. Engine code needs
// these before - and, headless, without ever - a main window, so they are
// deliberately not part of class mudlet. Everything here is read on first use.
class MudletApp
{
public:
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

    // Every on-disk location Mudlet reads or writes, derived from one config root:
    // a portable.txt marker beside the executable or in ~/.config/mudlet names it,
    // otherwise $XDG_CONFIG_HOME/mudlet and then ~/.config/mudlet are tried.

    // Where the running executable lives, or where the AppImage sits when running
    // from one
    static QString executableDir();

    // Applies the whole precedence to a given executable directory. Remembers
    // nothing, so the Mudlet.ini read that happens before QApplication exists can
    // share it.
    static ConfigDirResolution resolveConfigRoot(const QString& execDir);

    // Only looks for the two markers, so it is cheap enough to ask on every
    // password load and save
    static bool portableModeActive(const QString& execDir);

    // The XDG leg on its own: $XDG_CONFIG_HOME/mudlet takes a tie with
    // legacyDefault so that a fresh install lands there
    static ConfigDirResolution xdgConfigDir(const QString& legacyDefault);

    // A directory that cannot be listed must never read as "nothing here": that
    // inference is what hides profiles, so assume the strongest content instead.
    static bool configDirHoldsProfiles(const QString& dir);

    // Resolves the config root itself on first use; setConfigPath() replaces it,
    // which is how setupConfig() installs the root it has validated
    static QString getMudletPath(enums::mudletPathType mode, const QString& extra1 = QString(), const QString& extra2 = QString());
    static void setConfigPath(const QString& path);

    // Whether the main dictionary files are the ones bundled with Mudlet (true) or
    // ones provided by the system (false). Settled as a side effect of resolving
    // enums::hunspellDictionaryPath, so it only answers once that has been asked for.
    static bool usingMudletDictionaries();

    // Replaces filesystem-unsafe characters with underscores and bounds the length.
    // Callers file data under the result, so shortening has to keep distinct
    // inputs distinct: a shortened name carries a digest of the whole input, since
    // plain truncation made two long profile names share - and overwrite - one
    // stored password.
    static QString sanitizeForPath(const QString& input);

    static QString readProfileData(const QString& profile, const QString& item);

    // Creates the profile's directory when it is not there yet, so a write for a
    // profile that does not exist brings one into being
    static QPair<bool, QString> writeProfileData(const QString& profile, const QString& item, const QString& what);

    // The on-disk spelling of a profile named in any case, or an empty string if
    // neither an existing profile nor a predefined game goes by that name
    static QString getCanonicalProfileName(const QString& profileName);

    // The persistent application settings in Mudlet.ini under the config root
    static QSettings* getQSettings();
    static void resetSettings();
    static const QString& getInterfaceLanguage();
    static void setInterfaceLanguage(const QString& language);

    // Which build of Mudlet this is, and how it names itself to the outside world.

    // The suffix CMake writes into :/app-build.txt - empty for an official release,
    // "-ptb..." for a public test build, "-dev..." for everything else.
    static const QString& buildSuffix();
    static const QString& scmVersion();
    static bool release();
    static bool publicTest();
    static bool development();
    static void setNetworkRequestDefaults(const QUrl& url, QNetworkRequest& request);
};

#endif // MUDLET_MUDLETAPP_H
