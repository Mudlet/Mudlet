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

#include "LabelTextureCache.h"

#include <QImage>
#include <QOpenGLContext>

LabelTextureCache::LabelTextureCache() = default;

LabelTextureCache::~LabelTextureCache()
{
    cleanup();
}

void LabelTextureCache::initialize()
{
    if (mInitialized) {
        return;
    }

    initializeOpenGLFunctions();
    mInitialized = true;
}

void LabelTextureCache::cleanup()
{
    if (!mInitialized) {
        return;
    }

    clearAll();
    mInitialized = false;
}

GLuint LabelTextureCache::getTexture(int areaId, int labelId, const QPixmap& pixmap)
{
    if (!mInitialized || pixmap.isNull()) {
        return 0;
    }

    auto key = makeCacheKey(areaId, labelId);
    qint64 currentCacheKey = pixmap.cacheKey();

    auto it = mCache.find(key);
    if (it != mCache.end()) {
        // Check if pixmap changed
        if (it.value().pixmapCacheKey == currentCacheKey) {
            return it.value().textureId;
        }
        // Pixmap changed, delete old texture
        deleteTexture(it.value().textureId);
    }

    // Create new texture
    GLuint textureId = createTextureFromPixmap(pixmap);
    if (textureId != 0) {
        LabelTextureCacheEntry entry;
        entry.textureId = textureId;
        entry.pixmapCacheKey = currentCacheKey;
        mCache[key] = entry;
    }

    return textureId;
}

void LabelTextureCache::invalidateLabel(int areaId, int labelId)
{
    auto key = makeCacheKey(areaId, labelId);
    auto it = mCache.find(key);
    if (it != mCache.end()) {
        deleteTexture(it.value().textureId);
        mCache.erase(it);
    }
}

void LabelTextureCache::invalidateArea(int areaId)
{
    QList<QPair<int, int>> keysToRemove;
    for (auto it = mCache.begin(); it != mCache.end(); ++it) {
        if (it.key().first == areaId) {
            deleteTexture(it.value().textureId);
            keysToRemove.append(it.key());
        }
    }
    for (const auto& key : keysToRemove) {
        mCache.remove(key);
    }
}

void LabelTextureCache::clearAll()
{
    for (auto it = mCache.begin(); it != mCache.end(); ++it) {
        deleteTexture(it.value().textureId);
    }
    mCache.clear();
}

GLuint LabelTextureCache::createTextureFromPixmap(const QPixmap& pixmap)
{
    if (pixmap.isNull()) {
        return 0;
    }

    QImage image = pixmap.toImage();
    if (image.isNull()) {
        return 0;
    }

    // Convert to RGBA format for OpenGL
#if QT_VERSION >= QT_VERSION_CHECK(6, 9, 0)
    QImage glImage = image.convertToFormat(QImage::Format_RGBA8888).flipped(Qt::Vertical);
#else
    QImage glImage = image.convertToFormat(QImage::Format_RGBA8888).mirrored(false, true);
#endif

    GLuint textureId;
    glGenTextures(1, &textureId);
    glBindTexture(GL_TEXTURE_2D, textureId);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, glImage.width(), glImage.height(),
                 0, GL_RGBA, GL_UNSIGNED_BYTE, glImage.constBits());

    // Set texture parameters for labels - linear filtering for smooth appearance
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glBindTexture(GL_TEXTURE_2D, 0);

    return textureId;
}

void LabelTextureCache::deleteTexture(GLuint textureId)
{
    if (textureId != 0) {
        glDeleteTextures(1, &textureId);
    }
}

QPair<int, int> LabelTextureCache::makeCacheKey(int areaId, int labelId)
{
    return qMakePair(areaId, labelId);
}
