/***************************************************************************
 *   Copyright (C) 2009-2014 by Vadim Peretokin - vperetokin@gmail.com     *
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
#include <QDir>
#include <QDebug>
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
    filters << QStringLiteral("*.ttf")
            << QStringLiteral("*.otf");
    QDir dir = folder;
    dir.setNameFilters(filters);

    foreach (QString fontfile, dir.entryList(QDir::Files | QDir::Readable | QDir::NoDotAndDotDot)) {
        QString fontFilePathName = QStringLiteral("%1/%2").arg(dir.absolutePath(), fontfile);
        if (QFontDatabase::addApplicationFont(fontFilePathName) == -1) {
            // At the point that addFonts() is called we have a GUI application
            // in the making and can use qDebug() and not rely on iostream class
            qWarning() << "FontManager::loadFonts() ERROR - Could not load the font in the file: " << fontFilePathName;
        }
    }
}
