/***************************************************************************
*   Copyright (C) 2025 by Vadim Peretokin - vperetokin@gmail.com           *
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

#ifndef ENUMS_H
#define ENUMS_H

#include <QObject>

// helper Qt class to declare enums and flags throughout Mudlet that don't have a more specific place elsewhere. 
// This class should not include any other files keep compile times down
class enums : public QObject {
    Q_OBJECT
public:
    enum Appearance {
        systemSetting = 0,
        light = 1,
        dark = 2
    };

    enum controlsVisibilityFlag {
        visibleNever = 0,
        visibleOnlyWithoutLoadedProfile = 0x1,
        visibleMaskNormally = 0x2,
        visibleAlways = 0x3
    };
    Q_DECLARE_FLAGS(controlsVisibility, controlsVisibilityFlag)

    enum class PackageModuleType {
        Package = 0,         // Regular package installation
        ModuleFromUI = 1,    // First-time module installation via UI
        ModuleSync = 2,      // Module sync operation
        ModuleFromScript = 3 // Module installation from script
    };
    Q_ENUM(PackageModuleType)

    enum mudletPathType {
        // The root of all mudlet data for the user - does not end in a '/'
        mainPath = 0,
        // Takes one extra argument as a file (or directory) relating to
        // (profile independent) mudlet data - may end with a '/' if the extra
        // argument does:
        mainDataItemPath,
        // (Added for 3.5.0) a revised location to store Mudlet provided fonts:
        mainFontsPath,
        // The directory containing all the saved user's profiles - does not end
        // in '/':
        profilesPath,
        // Takes one extra argument (profile name) that returns the base
        // directory for that profile - does NOT end in a '/' unless the
        // supplied profle name does:
        profileHomePath,
        // Takes one extra argument (profile name) that returns the directory
        // for the profile game save media files - does NOT end in a '/'
        profileMediaPath,
        // Takes two extra arguments (profile name, mediaFileName) that returns
        // the pathFile name for any media file:
        profileMediaPathFileName,
        // Takes one extra argument (profile name) that returns the directory
        // for the profile game save XML files - ends in a '/':
        profileXmlFilesPath,
        // Takes one extra argument (profile name) that returns the directory
        // for the profile game save maps files - does NOT end in a '/'
        profileMapsPath,
        // Takes two extra arguments (profile name, dataTime stamp) that returns
        // the pathFile name for a dateTime stamped map file:
        profileDateTimeStampedMapPathFileName,
        // Takes two extra arguments (profile name, dataTime stamp) that returns
        // the pathFile name for a dateTime stamped JSON map file:
        profileDateTimeStampedJsonMapPathFileName,
        // Takes two extra arguments (profile name, mapFileName) that returns
        // the pathFile name for any map file:
        profileMapPathFileName,
        // Takes one extra argument (profile name) that returns the file
        // location for the downloaded MMP map:
        profileXmlMapPathFileName,
        // Takes two extra arguments (profile name, data item) that gives a
        // path file name for, typically a data item stored as a single item
        // (binary) profile data) file (ideally these can be moved to a per
        // profile QSettings file but that is a future pipe-dream on my part
        // SlySven):
        profileDataItemPath,
        // Takes two extra arguments (profile name, package name) returns the
        // per profile directory used to store (unpacked) package contents
        // - ends with a '/':
        profilePackagePath,
        // Takes two extra arguments (profile name, package name) returns the
        // filename of the XML file that contains the (per profile, unpacked)
        // package mudlet items in that package/module:
        profilePackagePathFileName,
        // Takes one extra argument (profile name) that returns the directory
        // that contains replays (*.dat files) and logs (*.html or *.txt) files
        // for that profile - does NOT end in '/':
        profileReplayAndLogFilesPath,
        // Takes one extra argument (profile name) that returns the pathFileName
        // to the map auditing report file that is appended to each time a
        // map is loaded:
        profileLogErrorsFilePath,
        // Takes two extra arguments (profile name, theme name) that returns the
        // pathFileName of the theme file used by the edbee editor - also
        // handles the special case of the default theme "mudlet.thTheme" that
        // is carried internally in the resource file:
        editorWidgetThemePathFile,
        // Returns the pathFileName to the external JSON file needed to process
        // an edbee edtor widget theme:
        editorWidgetThemeJsonFile,
        // Returns the directory used to store module backups that is used in
        // when saving/resyncing packages/modules - ends in a '/'
        moduleBackupsPath,
        // Returns path to Qt's own translation files
        qtTranslationsPath,
        // Takes one extra argument - a (dictionary) language code that should
        // match a hunspell affix file name e.g. "en_US" in the default case
        // to yield "en_US.aff" that is searched for in one or more OS dependent
        // places - returns the path ending in a '/' to use to get the
        // dictionaries from:
        hunspellDictionaryPath
    };
};

Q_DECLARE_OPERATORS_FOR_FLAGS(enums::controlsVisibility)

#endif //ENUMS_H
