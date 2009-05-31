/*
 *      FontManager.cpp
 *      
 *      Copyright 2009 Vadim Peretokin <vperetokin@gmail.com>
 * 
 *      Add your name to the copyright notice above if you do any work with
 *      this file.
 * 
 *      This program is free software; you can redistribute it and/or modify
 *      it under the terms of the GNU General Public License as published by
 *      the Free Software Foundation; either version 2 of the License, or
 *      (at your option) any later version.
 *      
 *      This program is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *      GNU General Public License for more details.
 *      
 *      You should have received a copy of the GNU General Public License
 *      along with this program; if not, write to the Free Software
 *      Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 *      MA 02110-1301, USA.
 */

#include "FontManager.h"
#include "TDebug.h"
#include <QStringList>
#include <QtCore>
#include <QDir>

void FontManager::addFonts()
{
    // load all font files we see. I'd also like to load files with mime type of "application/x-font-ttf" but Qt lacks a function to check for them

    QDir dir = QDir::homePath() + "/.config/mudlet/";

    if (!dir.exists())
        return;

    // load all fonts in the 'fonts' folder
    loadFonts(dir.absolutePath());

    // load all fonts in the subfolders of 'font'
    foreach(QString fontfolder, dir.entryList(QDir::Dirs | QDir::Readable | QDir::NoDotAndDotDot)) {
        loadFonts(dir.absolutePath() + "/" + fontfolder);
    }
}

// loads all of the fonts in the given folder
void FontManager::loadFonts(QString folder)
{
    // Check what happens with this: "Adding application fonts on Unix/X11 platforms without fontconfig is currently not supported."
    QStringList filters;
    filters << "*.ttf" << "*.otf";
    QDir dir = folder;
    dir.setNameFilters(filters);

    foreach(QString fontfile, dir.entryList(QDir::Files | QDir::Readable | QDir::NoDotAndDotDot)) {
        if (QFontDatabase::addApplicationFont(dir.absolutePath() + "/" + fontfile) == -1)
            std::cout << "Couldn't load the font in the file " << dir.absolutePath().toUtf8().
                data() << "/" << fontfile.toUtf8().data() << std::endl;
        else
            std::cout << "Loaded the font the file " << fontfile.toUtf8().data() << std::endl;
    }

}
