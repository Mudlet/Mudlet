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
        loadFont(fontFilePathName);
    }
}

void FontManager::loadFont(const QString& filePath, const QString& belongsTo)
{
    if (fontAlreadyLoaded(filePath)) {
        return;
    }

    auto fontID = QFontDatabase::addApplicationFont(filePath);

    // remember even if the font failed to load so we don't spam messages on fonts that repeat
    rememberFont(filePath, fontID, belongsTo);

    if (fontID == -1) {
        qWarning() << "FontManager::loadFont() WARNING - Could not load the font(s) in the file: " << filePath;
// Uncomment the next pair of lines to get details about fonts that ARE loaded:
//    } else {
//        qDebug().noquote().nospace() << "FontManager::loadFont() INFO - Loaded font(s) in the file: \"" << filePath << "\"\n    with ID: " << fontID << " providing: \"" << QFontDatabase::applicationFontFamilies(fontID).join(QLatin1String("\", \"")).append(QLatin1String("\"\n"));
    }
}

bool FontManager::fontAlreadyLoaded(const QString& filePath)
{
    const QFileInfo fontFile(filePath);
    auto fileName = fontFile.fileName();

    return loadedFontPaths.contains(fileName);
}

void FontManager::rememberFont(const QString& filePath, int fontID, const QString& belongsTo)
{
    const QFileInfo fontFile(filePath);
    auto fileName = fontFile.fileName();

    if (loadedFontPaths.contains(fileName)) {
        return;
    }

    loadedFontPaths.insert(fileName, fontID);
    loadedFontAffiliation.insert(belongsTo, fontID);
}

void FontManager::unloadFonts(const QString& belongsTo)
{
    const auto fontIds = loadedFontAffiliation.values(belongsTo);
    for (const int id : fontIds) {
        QFontDatabase::removeApplicationFont(id);
    }
    loadedFontAffiliation.remove(belongsTo);
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
