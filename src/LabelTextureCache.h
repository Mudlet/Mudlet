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
#include <QSet>
#include <QStringList>
#include <QImage>
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

    // Textures drawn by the caller, such as room symbols, keyed by everything that decides how they look.
    // Mipmapped, as they are drawn far smaller than they are made.
    GLuint imageTexture(const QString& key) const { return mImageCache.value(key); }
    // Returns 0 if the texture could not be made; a key that failed once is not tried again until clearAll()
    GLuint addImageTexture(const QString& key, const QImage& image);
    bool imageTextureFailed(const QString& key) const { return mFailedImageKeys.contains(key); }
    // Drops image textures not in keep until one more fits under maximum, and says whether it does.
    // keep must hold every image texture queued for drawing, as a dropped one is gone at once.
    bool makeRoomForImageTexture(qsizetype maximum, const QSet<QString>& keep);
    static QStringList keysToEvict(const QStringList& cached, qsizetype maximum, const QSet<QString>& keep);

    // Clear entire cache
    void clearAll();

private:
    // Create OpenGL texture from pixmap
    GLuint createTextureFromPixmap(const QPixmap& pixmap);
    GLuint createTextureFromImage(const QImage& image, bool mipmapped);

    // Delete OpenGL texture
    void deleteTexture(GLuint textureId);

    // Generate cache key from area and label IDs
    static QPair<int, int> makeCacheKey(int areaId, int labelId);

    // Cache storage: (areaId, labelId) -> cache entry
    QHash<QPair<int, int>, LabelTextureCacheEntry> mCache;
    QHash<QString, GLuint> mImageCache;
    QSet<QString> mFailedImageKeys;

    bool mInitialized = false;
};

#endif // MUDLET_LABEL_TEXTURE_CACHE_H
