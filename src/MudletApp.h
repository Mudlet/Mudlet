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
// deliberately not part of class mudlet. The config root, the settings store and
// the build suffix are settled once and remembered; everything else recomputes on
// every call.
//
// The settings store is the one service that is main-window-only today, and
// deliberately so: getQSettings() stays null until mudlet::setupConfig() has
// settled the root. The two resolve the root the same way - setupConfig()
// validates nothing getMudletPath() does not - so this is about ordering, not
// about one answer being better than the other. setupConfig() is what reports a
// rejected portable.txt to the user, and no Mudlet.ini may be opened before that
// has had its chance: a root resolved on first use can be replaced afterwards,
// but a store opened under one hands out a pointer the Updater keeps for the
// life of the process. Settling the root without a main window is for libmudlet
// (#8681) to add, not for a caller here to work around.
class MudletApp
{
public:
    // A bag of statics, never an object
    MudletApp() = delete;

    struct ConfigDirResolution
    {
        QString path;
        // A portable.txt marker was in force. True even when the root it named had
        // to be refused, in which case path is the fallback rather than anything
        // the marker named - so check portableRootRejected before treating path as
        // the portable location.
        bool portable = false;
        // The marker named a root that cannot be used, so path holds the
        // non-portable location instead. The caller decides how loudly to say so.
        bool portableRootRejected = false;
        // The portable.txt that governs, empty when none does, and what it named
        // when that had to be refused. Carried so a caller can name the file and
        // the directory without going looking for them a second time.
        QString portableMarker;
        QString rejectedRoot;
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

    // ~/.config/mudlet: the pre-XDG default, and the second place a portable.txt
    // marker is looked for
    static QString legacyConfigDir();

    // The portable.txt that governs, or an empty string when there is none - the
    // one beside the executable outranks the one in the config dir. Two stats and
    // no file read, so callers that only want to know whether portable mode is on
    // can ask on every operation.
    static QString portableMarkerPath(const QString& execDir, const QString& configDir = legacyConfigDir());

    // Whether a root a portable.txt named can be used at all: it has to name
    // something, must not already exist as anything other than a directory - a
    // symlink with no target included, since mkpath() cannot create through one -
    // and its parent has to exist. Says why when it refuses.
    static bool portableRootUsable(const QString& path);

    // Applies the whole precedence to a given executable directory. Remembers
    // nothing, so the Mudlet.ini read that happens before QApplication exists can
    // share it. Never hands back an empty root: a portable.txt naming an unusable
    // one falls back to the non-portable location with portableRootRejected set.
    static ConfigDirResolution resolveConfigRoot(const QString& execDir, const QString& configDir = legacyConfigDir());

    // The XDG leg on its own: $XDG_CONFIG_HOME/mudlet takes a tie with
    // legacyDefault so that a fresh install lands there
    static ConfigDirResolution xdgConfigDir(const QString& legacyDefault);

    // A directory that cannot be listed must never read as "nothing here": that
    // inference is what hides profiles, so assume the strongest content instead.
    static bool configDirHoldsProfiles(const QString& dir);

    // Resolves the config root itself on first use and then remembers it, so the
    // resolver runs once however many paths are asked for; setConfigPath()
    // replaces it, which is how setupConfig() installs the root it has validated.
    // An empty path passed to setConfigPath() forgets the resolution instead.
    static QString getMudletPath(enums::mudletPathType mode, const QString& extra1 = QString(), const QString& extra2 = QString());

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

    // The persistent application settings in Mudlet.ini under the config root, or
    // nullptr until the root has been settled - see the note on the class above.
    // Callers that can run before mudlet::setupConfig() have to check; the rest
    // run long after startup and dereference it directly.
    static QSettings* getQSettings();

    // Whether the config root in force actually came from a portable.txt. False
    // when a marker was present but named a root that had to be refused, which is
    // the distinction a marker stat cannot make - and getting it wrong sends
    // credentials to portable-mode files on an install running non-portably.
    static bool portableRootInUse();

    // Has default form of "en_US" but can be just an ISO language code e.g. "fr"
    // for french, without a country designation. Replaces xx in "mudlet_xx.qm" to
    // provide the translation file for GUI translation.
    // Read under a lock and so returned by value: a reference would escape it, and
    // the language is written both at startup by mudlet::readEarlySettings() and
    // later by the preferences dialog.
    static QString getInterfaceLanguage();

    // Which build of Mudlet this is, and how it names itself to the outside world.

    // The suffix CMake writes into :/app-build.txt: empty for an official release,
    // otherwise MUDLET_VERSION_BUILD (or "-dev" when it is unset) followed by the
    // commit - so "-ptb", "-dev" and "-test" are all values seen in practice.
    // Which build that makes it is publicTest() and development()'s answer, not
    // something to read off the suffix.
    static const QString& buildSuffix();
    static const QString& scmVersion();
    static bool release();
    static bool publicTest();
    static bool development();
    static void setNetworkRequestDefaults(const QUrl& url, QNetworkRequest& request);

private:
    // Only the main window moves the config root or the language in a running
    // Mudlet: setupConfig() installs the root it has settled, and the language
    // follows the preferences dialog through mudlet::setInterfaceLanguage()
    friend class mudlet;
    // Resolving the root once is the point, so the only way to test it is to be
    // able to forget the answer - which setConfigPath(QString()) does
    friend class ConfigDirOverrideTest;
    // Discards any settings store built under the previous root - and with it any
    // pointer a caller cached, so this must not run once init() has handed the
    // store to the Updater. portable says whether the root came from a
    // portable.txt that was honoured.
    static void setConfigPath(const QString& path, bool portable = false);
    static void setInterfaceLanguage(const QString& language);
};

#endif // MUDLET_MUDLETAPP_H
