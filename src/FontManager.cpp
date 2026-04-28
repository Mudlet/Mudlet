/***************************************************************************
 *   Copyright (C) 2009, 2018 by Vadim Peretokin - vperetokin@gmail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2017-2018 by Stephen Lyons - slysven@viginmedia.com     *
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


#include "FontManager.h"
#include "mudlet.h"

#include <QDir>
#include <QFileInfo>
#include <QDesktopServices>
#include <QFontDatabase>

void FontManager::addFonts()
{
    const QDir dir(mudlet::getMudletPath(enums::mainFontsPath));

    if (!dir.exists()) {
        return;
    }

    // load all fonts (in the 'fonts') folder
    loadFonts(dir.absolutePath());

    // load all fonts in subfolders (of the 'font' folder)
    for (auto fontfolder : dir.entryList(QDir::Dirs | QDir::Readable | QDir::NoDotAndDotDot)) {
        loadFonts(qsl("%1/%2").arg(dir.absolutePath(), fontfolder));
    }
}

// loads all of the fonts in the given folder
void FontManager::loadFonts(const QString& folder)
{
    // Check what happens with this: "Adding application fonts on Unix/X11 platforms without fontconfig is currently not supported."
    QStringList filters;
    filters << qsl("*.ttf") << qsl("*.otf");
    QDir dir = folder;
    dir.setNameFilters(filters);

    for (auto fontFile : dir.entryList(QDir::Files | QDir::Readable | QDir::NoDotAndDotDot)) {
        const QString fontFilePathName = qsl("%1/%2").arg(dir.absolutePath(), fontFile);
        // Global built-in fonts are not profile-specific — use empty profileName
        loadFont(fontFilePathName, QString());
    }
}

void FontManager::loadFont(const QString& filePath, const QString& profileName, const QString& belongsTo)
{
    if (fontAlreadyLoaded(filePath, profileName)) {
        return;
    }

    int fontID;
    // Reuse existing Qt font ID if another profile already loaded this file — Qt's global
    // font database is shared, so calling addApplicationFont twice returns the same ID,
    // and a later removeApplicationFont by the first profile would orphan the second.
    const int sharedId = sharedFontPaths.value(filePath, -2);
    if (sharedId > -1) {
        fontID = sharedId;
    } else {
        fontID = QFontDatabase::addApplicationFont(filePath);
        if (fontID == -1) {
            qWarning() << "FontManager::loadFont() WARNING - Could not load the font(s) in the file: " << filePath;
        }
    }

    // remember even if the font failed to load so we don't spam messages on fonts that repeat
    rememberFont(filePath, fontID, profileName, belongsTo);
}

bool FontManager::fontAlreadyLoaded(const QString& filePath, const QString& profileName)
{
    // Use the full path prefixed by profile name as the key so that different profiles
    // loading a font file with the same filename are tracked independently.
    const QString key = profileName.isEmpty() ? filePath : qsl("%1/%2").arg(profileName, filePath);
    return loadedFontPaths.contains(key);
}

void FontManager::rememberFont(const QString& filePath, int fontID, const QString& profileName, const QString& belongsTo)
{
    const QString key = profileName.isEmpty() ? filePath : qsl("%1/%2").arg(profileName, filePath);

    if (loadedFontPaths.contains(key)) {
        return;
    }

    loadedFontPaths.insert(key, fontID);

    if (fontID != -1 && !sharedFontPaths.contains(filePath)) {
        sharedFontPaths.insert(filePath, fontID);
    }

    // Affiliation key combines profile and package so that unloading one profile's
    // fonts does not affect another profile's copy of the same package.
    const QString affiliationKey = profileName.isEmpty() ? belongsTo : qsl("%1/%2").arg(profileName, belongsTo);
    loadedFontAffiliation.insert(affiliationKey, fontID);
}

void FontManager::unloadFonts(const QString& profileName, const QString& belongsTo)
{
    const QString affiliationKey = profileName.isEmpty() ? belongsTo : qsl("%1/%2").arg(profileName, belongsTo);
    const auto fontIds = loadedFontAffiliation.values(affiliationKey);
    loadedFontAffiliation.remove(affiliationKey);

    for (const int id : fontIds) {
        if (id == -1) {
            continue;
        }
        // Only remove from Qt when no other profile still references this font ID.
        if (!loadedFontAffiliation.values().contains(id)) {
            QFontDatabase::removeApplicationFont(id);
            for (auto it = sharedFontPaths.begin(); it != sharedFontPaths.end();) {
                if (it.value() == id) {
                    it = sharedFontPaths.erase(it);
                } else {
                    ++it;
                }
            }
        }
    }

    // Remove stale path entries for this profile+package so fonts can be reloaded
    // correctly if the package is reinstalled later. Exclude -1 entries from the
    // predicate to avoid matching failed-load entries from unrelated packages.
    const QString pathPrefix = profileName.isEmpty() ? QString() : qsl("%1/").arg(profileName);
    auto it = loadedFontPaths.begin();
    while (it != loadedFontPaths.end()) {
        if (it.value() != -1 && fontIds.contains(it.value()) && (pathPrefix.isEmpty() || it.key().startsWith(pathPrefix))) {
            it = loadedFontPaths.erase(it);
        } else {
            ++it;
        }
    }
}

void FontManager::addEmojiFont()
{
#if defined(Q_OS_LINUX) || defined(Q_OS_FREEBSD)
#if QT_VERSION >= QT_VERSION_CHECK(6, 9, 0)
    // Use the new Qt 6.9 function for emoji fonts
    QFontDatabase::addApplicationEmojiFontFamily(qsl("Noto Color Emoji"));
#else
    // Fallback for older Qt versions - this will be handled by individual components
    // using QFont::insertSubstitution as before
#endif
#endif // defined(Q_OS_LINUX) || defined(Q_OS_FREEBSD)
}
