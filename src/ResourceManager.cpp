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

#include "ResourceManager.h"
#include "utils.h"

#include <QOpenGLContext>

ResourceManager::ResourceManager()
{
}

ResourceManager::~ResourceManager()
{
    cleanup();
}

void ResourceManager::initialize()
{
    if (mInitialized) {
        return;
    }

    initializeOpenGLFunctions();

    // Always enable error checking now that we have debug context
    mErrorCheckingEnabled = true;

    mInitialized = true;
    qDebug() << "ResourceManager: Initialized with error checking"
             << (mErrorCheckingEnabled ? "enabled" : "disabled");
}

void ResourceManager::cleanup()
{
    if (mInitialized) {
        printStats();
        mInitialized = false;
    }
}

bool ResourceManager::checkGLError(const QString& operation) const
{
    if (!mErrorCheckingEnabled || !mInitialized) {
        return true;
    }

    // Need to cast away const for OpenGL calls
    ResourceManager* nonConstThis = const_cast<ResourceManager*>(this);
    GLenum error = nonConstThis->glGetError();
    if (error != GL_NO_ERROR) {
        const char* errorString;
        switch (error) {
            case GL_INVALID_ENUM:
                errorString = "GL_INVALID_ENUM";
                break;
            case GL_INVALID_VALUE:
                errorString = "GL_INVALID_VALUE";
                break;
            case GL_INVALID_OPERATION:
                errorString = "GL_INVALID_OPERATION";
                break;
            case GL_OUT_OF_MEMORY:
                errorString = "GL_OUT_OF_MEMORY";
                break;
            case GL_INVALID_FRAMEBUFFER_OPERATION:
                errorString = "GL_INVALID_FRAMEBUFFER_OPERATION";
                break;
            default:
                errorString = "UNKNOWN_GL_ERROR";
                break;
        }

        qWarning() << "OpenGL Error in" << operation << ":" << errorString << "(" << error << ")";
        return false;
    }

    return true;
}

bool ResourceManager::isContextValid() const
{
    QOpenGLContext* context = QOpenGLContext::currentContext();
    if (!context) {
        qWarning() << "ResourceManager: No current OpenGL context";
        return false;
    }

    if (!context->isValid()) {
        qWarning() << "ResourceManager: Current OpenGL context is invalid";
        return false;
    }

    return true;
}

void ResourceManager::printStats() const
{
    qDebug() << "ResourceManager Statistics:";
    qDebug() << "  Buffers created:" << mStats.totalBuffersCreated;
    qDebug() << "  VAOs created:" << mStats.totalVAOsCreated;
    qDebug() << "  Shaders created:" << mStats.totalShadersCreated;
    qDebug() << "  Total draw calls:" << mStats.totalDrawCalls;
    qDebug() << "  Total vertices rendered:" << mStats.totalVerticesRendered;

    if (mStats.totalDrawCalls > 0) {
        double avgVerticesPerDraw = static_cast<double>(mStats.totalVerticesRendered) / mStats.totalDrawCalls;
        qDebug() << "  Average vertices per draw call:" << avgVerticesPerDraw;
    }
}

void ResourceManager::resetFrameStats()
{
    mStats.currentFrameDrawCalls = 0;
    mStats.currentFrameVertices = 0;
}
