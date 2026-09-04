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

#include "MudletPaths.h"

#include "utils.h"

#include <QCoreApplication>
#include <QFile>
#include <QLibraryInfo>

void MudletPaths::setConfigPath(const QString& path)
{
    smConfigPath = path;
}

bool MudletPaths::usingMudletDictionaries()
{
    return smUsingMudletDictionaries;
}

QString MudletPaths::getMudletPath(const enums::mudletPathType mode, const QString& extra1, const QString& extra2)
{
    const QString confPath = smConfigPath;
    switch (mode) {
    case enums::mainPath:
        // The root of all mudlet data for the user - does not end in a '/'
        return confPath;
    case enums::mainDataItemPath:
        // Takes one extra argument as a file (or directory) relating to
        // (profile independent) mudlet data - may end with a '/' if the extra
        // argument does:
        return qsl("%1/%2").arg(confPath, extra1);
    case enums::mainFontsPath:
        // (Added for 3.5.0) a revised location to store Mudlet provided fonts
        return qsl("%1/fonts").arg(confPath);
    case enums::profilesPath:
        // The directory containing all the saved user's profiles - does not end
        // in '/'
        return qsl("%1/profiles").arg(confPath);
    case enums::profileHomePath:
        // Takes one extra argument (profile name) that returns the base
        // directory for that profile - does NOT end in a '/' unless the
        // supplied profle name does:
        return qsl("%1/profiles/%2").arg(confPath, extra1);
    case enums::profileMediaPath:
        // Takes one extra argument (profile name) that returns the directory
        // for the profile's cached media files - does NOT end in a '/'
        return qsl("%1/profiles/%2/media").arg(confPath, extra1);
    case enums::profileMediaPathFileName:
        // Takes two extra arguments (profile name, mediaFileName) that returns
        // the pathFile name for any media file:
        return qsl("%1/profiles/%2/media/%3").arg(confPath, extra1, extra2);
    case enums::profileXmlFilesPath:
        // Takes one extra argument (profile name) that returns the directory
        // for the profile game save XML files - ends in a '/'
        return qsl("%1/profiles/%2/current/").arg(confPath, extra1);
    case enums::profileMapsPath:
        // Takes one extra argument (profile name) that returns the directory
        // for the profile game save maps files - does NOT end in a '/'
        return qsl("%1/profiles/%2/map").arg(confPath, extra1);
    case enums::profileDateTimeStampedMapPathFileName:
        // Takes two extra arguments (profile name, dataTime stamp) that returns
        // the pathFile name for a dateTime stamped map file:
        return qsl("%1/profiles/%2/map/%3map.dat").arg(confPath, extra1, extra2);
    case enums::profileDateTimeStampedJsonMapPathFileName:
        // Takes two extra arguments (profile name, dataTime stamp) that returns
        // the pathFile name for a dateTime stamped JSON map file:
        return qsl("%1/profiles/%2/map/%3map.json").arg(confPath, extra1, extra2);
    case enums::profileMapPathFileName:
        // Takes two extra arguments (profile name, mapFileName) that returns
        // the pathFile name for any map file:
        return qsl("%1/profiles/%2/map/%3").arg(confPath, extra1, extra2);
    case enums::profileXmlMapPathFileName:
        // Takes one extra argument (profile name) that returns the pathFile
        // name for the downloaded IRE Server provided XML map:
        return qsl("%1/profiles/%2/map.xml").arg(confPath, extra1);
    case enums::profileDataItemPath:
        // Takes two extra arguments (profile name, data item) that gives a
        // path file name for, typically a data item stored as a single item
        // (binary) profile data) file (ideally these can be moved to a per
        // profile QSettings file but that is a future pipe-dream on my part
        // SlySven):
        return qsl("%1/profiles/%2/%3").arg(confPath, extra1, extra2);
    case enums::profilePackagePath:
        // Takes two extra arguments (profile name, package name) returns the
        // per profile directory used to store (unpacked) package contents
        // - ends with a '/':
        return qsl("%1/profiles/%2/%3/").arg(confPath, extra1, extra2);
    case enums::profilePackagePathFileName:
        // Takes two extra arguments (profile name, package name) returns the
        // filename of the XML file that contains the (per profile, unpacked)
        // package mudlet items in that package/module:
        return qsl("%1/profiles/%2/%3/%3.xml").arg(confPath, extra1, extra2);
    case enums::profileReplayAndLogFilesPath:
        // Takes one extra argument (profile name) that returns the directory
        // that contains replays (*.dat files) and logs (*.html or *.txt) files
        // for that profile - does NOT end in '/':
        return qsl("%1/profiles/%2/log").arg(confPath, extra1);
    case enums::profileLogErrorsFilePath:
        // Takes one extra argument (profile name) that returns the pathFileName
        // to the map auditing report file that is appended to each time a
        // map is loaded:
        return qsl("%1/profiles/%2/log/errors.txt").arg(confPath, extra1);
    case enums::editorWidgetThemePathFile:
        // Takes two extra arguments (profile name, theme name) that returns the
        // pathFileName of the theme file used by the edbee editor - also
        // handles the special case of the default theme "mudlet.tmTheme" that
        // is carried internally in the resource file:
        if (extra1.compare(qsl("Mudlet.tmTheme"), Qt::CaseSensitive)) {
            // No match
            return qsl("%1/edbee/Colorsublime-Themes-master/themes/%2").arg(confPath, extra1);
        }
        // Match - return path to copy held in resource file
        return qsl(":/edbee_defaults/Mudlet.tmTheme");
    case enums::editorWidgetThemeJsonFile:
        // Returns the pathFileName to the external JSON file needed to process
        // an edbee editor widget theme:
        return qsl("%1/edbee/Colorsublime-Themes-master/themes.json").arg(confPath);
    case enums::moduleBackupsPath:
        // Returns the directory used to store module backups that is used in
        // when saving/resyncing packages/modules - ends in a '/'
        return qsl("%1/moduleBackups/").arg(confPath);
    case enums::qtTranslationsPath:
        return QLibraryInfo::path(QLibraryInfo::TranslationsPath);
    case enums::hunspellDictionaryPath:
        // Added for 3.18.0 when user dictionary capability added
#if defined(Q_OS_MACOS)
        smUsingMudletDictionaries = true;
        return qsl("%1/../Resources/").arg(QCoreApplication::applicationDirPath());
#elif defined(Q_OS_FREEBSD)
        if (QFile::exists(qsl("/usr/local/share/hunspell/%1.aff").arg(extra1))) {
            smUsingMudletDictionaries = false;
            return QLatin1String("/usr/local/share/hunspell/");
        }
        if (QFile::exists(qsl("/usr/share/hunspell/%1.aff").arg(extra1))) {
            smUsingMudletDictionaries = false;
            return QLatin1String("/usr/share/hunspell/");
        }
        if (QFile::exists(qsl("%1/../../src/%2.aff").arg(QCoreApplication::applicationDirPath(), extra1))) {
            // From debug or release subdirectory of a shadow build directory alongside the ./src one:
            smUsingMudletDictionaries = true;
            return qsl("%1/../../src/").arg(QCoreApplication::applicationDirPath());
        }
        if (QFile::exists(qsl("%1/../src/%2.aff").arg(QCoreApplication::applicationDirPath(), extra1))) {
            // From shadow build directory alongside the ./src one:
            smUsingMudletDictionaries = true;
            return qsl("%1/../src/").arg(QCoreApplication::applicationDirPath());
        }
        // From build within ./src
        smUsingMudletDictionaries = true;
        return qsl("%1/").arg(QCoreApplication::applicationDirPath());
#elif defined(Q_OS_OPENBSD)
        // OpenBSD uses dictionary files from Mozilla rather than direct from,
        // Hunspell, but it does not ship a en_us one so we cannot use that on
        // the first run to find the rest - instead try for the en_GB one
        // - some of the entries for some of the locale/language/other parts of
        // the filesnames seem to be a bit random:
        if (QFile::exists(qsl("/usr/local/share/mozilla-dicts/%1.aff").arg(extra1))) {
            smUsingMudletDictionaries = false;
            return QLatin1String("/usr/local/share/mozilla-dicts/");
        }
        if (QFile::exists(qsl("/usr/share/mozilla-dicts/%1.aff").arg(extra1))) {
            smUsingMudletDictionaries = false;
            return QLatin1String("/usr/share/mozilla-dicts/");
        }
        if (QFile::exists(qsl("%1/../../src/%2.aff").arg(QCoreApplication::applicationDirPath(), extra1))) {
            // From debug or release subdirectory of a shadow build directory alongside the ./src one:
            smUsingMudletDictionaries = true;
            return qsl("%1/../../src/").arg(QCoreApplication::applicationDirPath());
        }
        if (QFile::exists(qsl("%1/../src/%2.aff").arg(QCoreApplication::applicationDirPath(), extra1))) {
            // From shadow build directory alongside the ./src one:
            smUsingMudletDictionaries = true;
            return qsl("%1/../src/").arg(QCoreApplication::applicationDirPath());
        }
        // From build within ./src
        smUsingMudletDictionaries = true;
        return qsl("%1/").arg(QCoreApplication::applicationDirPath());
#elif defined(Q_OS_LINUX)
        if (QFile::exists(qsl("/usr/share/hunspell/%1.aff").arg(extra1))) {
            smUsingMudletDictionaries = false;
            return QLatin1String("/usr/share/hunspell/");
        }
        if (QFile::exists(qsl("%1/../../src/%2.aff").arg(QCoreApplication::applicationDirPath(), extra1))) {
            // From debug or release subdirectory of a shadow build directory
            // alongside the ./src one. {Typically QMake builds from Qtcreator
            // with CONFIG containing both 'debug_and_release' and
            // 'debug_and_release_target' (this is normal also on Windows):
            smUsingMudletDictionaries = true;
            return qsl("%1/../../src/").arg(QCoreApplication::applicationDirPath());
        }
        if (QFile::exists(qsl("%1/../src/%2.aff").arg(QCoreApplication::applicationDirPath(), extra1))) {
            // From shadow build directory alongside the ./src one. {Typically
            // QMake builds from Qtcreator with CONFIG NOT containing both
            // 'debug_and_release' and 'debug_and_release_target':
            smUsingMudletDictionaries = true;
            return qsl("%1/../src/").arg(QCoreApplication::applicationDirPath());
        }
        if (QFile::exists(qsl("%1/../../mudlet/src/%2.aff").arg(QCoreApplication::applicationDirPath(), extra1))) {
            // From shadow build directory above the ./src one. {Typically
            // CMake builds from Qtcreator which are outside of the unpacked
            // source code from a git repo or tarball - which has to have been
            // unpacked/placed in a directory called 'mudlet'}:
            smUsingMudletDictionaries = true;
            return qsl("%1/../../mudlet/src/").arg(QCoreApplication::applicationDirPath());
        }
        // From build within ./src AND installer builds that bundle
        // dictionaries in the same directory as the executable:
        smUsingMudletDictionaries = true;
        return qsl("%1/").arg(QCoreApplication::applicationDirPath());
#else
        // Probably Windows!
        smUsingMudletDictionaries = true;
        if (QFile::exists(qsl("%1/../../src/%2.aff").arg(QCoreApplication::applicationDirPath(), extra1))) {
            // From debug or release subdirectory of a shadow build directory alongside the ./src one:
            return qsl("%1/../../src/").arg(QCoreApplication::applicationDirPath());
        }
        if (QFile::exists(qsl("%1/../src/%2.aff").arg(QCoreApplication::applicationDirPath(), extra1))) {
            // From shadow build directory alongside the ./src one:
            return qsl("%1/../src/").arg(QCoreApplication::applicationDirPath());
        }
        // From build within ./src
        return qsl("%1/").arg(QCoreApplication::applicationDirPath());
#endif
    }
    Q_UNREACHABLE();
    return QString();
}
