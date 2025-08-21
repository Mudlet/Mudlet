#ifndef MUDLET_FONTMANAGER_H
#define MUDLET_FONTMANAGER_H

/***************************************************************************
 *   Copyright (C) 2009 by Vadim Peretokin - vperetokin@gmail.com          *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
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

#include "pre_guard.h"
#include <QMap>
#include <QMultiMap>
#include "post_guard.h"

class QString;


class FontManager
{
public:
    void addFonts();
    void loadFont(const QString& filePath, const QString& belongsTo = "main");
    bool fontAlreadyLoaded(const QString& filePath);
    void unloadFonts(const QString& belongsTo);
    void addEmojiFont();

private:
    void loadFonts(const QString& folder);
    void rememberFont(const QString& fileName, int fontID, const QString& belongsTo);

    // map of font file path & font ID
    QMap<QString, int> loadedFontPaths;
    // map of font affiliation ("main" or a specific package) & font IDs
    QMultiMap<QString, int> loadedFontAffiliation;
};

#endif // MUDLET_FONTMANAGER_H
