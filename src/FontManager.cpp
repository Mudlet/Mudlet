/***************************************************************************
 *   Copyright (C) 2009, 2018 by Vadim Peretokin - vperetokin@gmail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2017 by Stephen Lyons - slysven@viginmedia.com          *
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

#include "pre_guard.h"
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QFontDatabase>
#include <QString>
#include <QStringList>
#include "post_guard.h"

void FontManager::addFonts()
{
    // load all font files we see. I'd also like to load files with mime type of "application/x-font-ttf" but Qt lacks a function to check for them

    QDir dir(mudlet::getMudletPath(mudlet::mainFontsPath));

    if (!dir.exists()) {
        return;
    }

    // load all fonts (in the 'fonts') folder
    loadFonts(dir.absolutePath());

    // load all fonts in subfolders (of the 'font' folder)
    foreach (QString fontfolder, dir.entryList(QDir::Dirs | QDir::Readable | QDir::NoDotAndDotDot)) {
        loadFonts(QStringLiteral("%1/%2").arg(dir.absolutePath(), fontfolder));
    }
}

// loads all of the fonts in the given folder
void FontManager::loadFonts(const QString& folder)
{
    // Check what happens with this: "Adding application fonts on Unix/X11 platforms without fontconfig is currently not supported."
    QStringList filters;
    filters << QStringLiteral("*.ttf") << QStringLiteral("*.otf");
    QDir dir = folder;
    dir.setNameFilters(filters);

    foreach (QString fontFile, dir.entryList(QDir::Files | QDir::Readable | QDir::NoDotAndDotDot)) {
        QString fontFilePathName = QStringLiteral("%1/%2").arg(dir.absolutePath(), fontFile);
        loadFont(fontFilePathName);
    }
}

void FontManager::loadFont(const QString& filePath)
{
    if (fontAlreadyLoaded(filePath)) {
        return;
    }

    auto fontID = QFontDatabase::addApplicationFont(filePath);

    // remember even if the font failed to load so we don't spam messages on fonts that repeat
    rememberFont(filePath, fontID);

    if (fontID == -1) {
        qWarning() << "FontManager::loadFonts() warning - Could not load the font(s) in the file: " << filePath;
    }
}

bool FontManager::fontAlreadyLoaded(const QString& filePath)
{
    QFileInfo fontFile(filePath);
    auto fileName = fontFile.fileName();

    return loadedFonts.contains(fileName);
}

void FontManager::rememberFont(const QString& filePath, int fontID)
{
    QFileInfo fontFile(filePath);
    auto fileName = fontFile.fileName();

    if (loadedFonts.contains(fileName)) {
        return;
    }

    loadedFonts.insert(fileName, fontID);
}
