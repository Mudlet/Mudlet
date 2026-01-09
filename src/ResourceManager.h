#ifndef MUDLET_RESOURCE_MANAGER_H
#define MUDLET_RESOURCE_MANAGER_H

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

#include <QOpenGLFunctions>
#include <QDebug>

// Resource manager for OpenGL error checking and monitoring
class ResourceManager : protected QOpenGLFunctions
{
public:
    ResourceManager();
    ~ResourceManager();

    void initialize();
    void cleanup();

    // OpenGL error checking
    bool checkGLError(const QString& operation) const;
    void enableErrorChecking(bool enable) { mErrorCheckingEnabled = enable; }

    // Resource usage statistics
    struct ResourceStats {
        size_t totalBuffersCreated = 0;
        size_t totalVAOsCreated = 0;
        size_t totalShadersCreated = 0;
        size_t totalDrawCalls = 0;
        size_t totalVerticesRendered = 0;
        size_t currentFrameDrawCalls = 0;
        size_t currentFrameVertices = 0;
    };

    ResourceStats getStats() const { return mStats; }
    void printStats() const;
    void resetFrameStats();

    // Resource tracking for statistics
    void onBufferCreated() { mStats.totalBuffersCreated++; }
    void onVAOCreated() { mStats.totalVAOsCreated++; }
    void onShaderCreated() { mStats.totalShadersCreated++; }
    void onDrawCall(size_t vertexCount) {
        mStats.totalDrawCalls++;
        mStats.totalVerticesRendered += vertexCount;
        mStats.currentFrameDrawCalls++;
        mStats.currentFrameVertices += vertexCount;
    }

    // Context validation
    bool isContextValid() const;

private:
    bool mInitialized = false;
    bool mErrorCheckingEnabled = false; // Disable by default for performance
    mutable ResourceStats mStats;
};

#endif // MUDLET_RESOURCE_MANAGER_H
