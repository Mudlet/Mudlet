#ifndef MUDLET_LABEL_TEXTURE_CACHE_H
#define MUDLET_LABEL_TEXTURE_CACHE_H

/***************************************************************************
 *   Copyright (C) 2025 by Vadim Peretokin - vadim.peretokin@mudlet.org    *
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

#include <QHash>
#include <QPixmap>
#include <QOpenGLFunctions>

// Cache entry storing OpenGL texture ID and pixmap cache key for change detection
struct LabelTextureCacheEntry {
    GLuint textureId = 0;
    qint64 pixmapCacheKey = 0;
};

// Cache for label textures, keyed by (areaId, labelId) pairs
class LabelTextureCache : protected QOpenGLFunctions
{
public:
    LabelTextureCache();
    ~LabelTextureCache();

    void initialize();
    void cleanup();

    // Get or create texture for a label
    // Returns texture ID, creates new texture if pixmap changed or not cached
    GLuint getTexture(int areaId, int labelId, const QPixmap& pixmap);

    // Invalidate specific label
    void invalidateLabel(int areaId, int labelId);

    // Invalidate all labels in an area
    void invalidateArea(int areaId);

    // Clear entire cache
    void clearAll();

private:
    // Create OpenGL texture from pixmap
    GLuint createTextureFromPixmap(const QPixmap& pixmap);

    // Delete OpenGL texture
    void deleteTexture(GLuint textureId);

    // Generate cache key from area and label IDs
    static QPair<int, int> makeCacheKey(int areaId, int labelId);

    // Cache storage: (areaId, labelId) -> cache entry
    QHash<QPair<int, int>, LabelTextureCacheEntry> mCache;

    bool mInitialized = false;
};

#endif // MUDLET_LABEL_TEXTURE_CACHE_H
