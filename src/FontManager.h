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

#include <QMap>
#include <QMultiMap>
#include <QStringList>


// The fonts Mudlet and its profiles' packages add to the application's font
// database. A value member of the application object like HostManager, so a
// profile reaches it through self() rather than through the main window.
class FontManager
{
public:
    Q_DISABLE_COPY_MOVE(FontManager)
    FontManager();
    ~FontManager();

    // Null outside the application object's lifetime
    static FontManager* self() { return smpSelf; }
    // Every family the font database knows, including those added here
    static QStringList availableFonts();

    void addFonts();
    void loadFont(const QString& filePath, const QString& profileName, const QString& belongsTo = "main");
    bool fontAlreadyLoaded(const QString& filePath, const QString& profileName);
    void unloadFonts(const QString& profileName, const QString& belongsTo);
    void addEmojiFont();

private:
    inline static FontManager* smpSelf = nullptr;

    void loadFonts(const QString& folder);
    void rememberFont(const QString& filePath, int fontID, const QString& profileName, const QString& belongsTo);

    // map of profile-prefixed file path to Qt font ID (-1 if load failed), per-profile deduplication
    QMap<QString, int> loadedFontPaths;
    // map of file path to Qt font ID, global deduplication (load each file into Qt only once)
    QMap<QString, int> sharedFontPaths;
    // map of font affiliation ("main" or profileName/packageName for package fonts) & font IDs
    QMultiMap<QString, int> loadedFontAffiliation;
};

#endif // MUDLET_FONTMANAGER_H
