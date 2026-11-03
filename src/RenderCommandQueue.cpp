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

#include "RenderCommandQueue.h"

#include <QDebug>
#include <cstring>

RenderCommandQueue::RenderCommandQueue() {}

RenderCommandQueue::~RenderCommandQueue()
{
    cleanup();
}

void RenderCommandQueue::initialize()
{
    if (mInitialized) {
        return;
    }

    initializeOpenGLFunctions();
    mInitialized = true;
}

void RenderCommandQueue::cleanup()
{
    clear();
    mInitialized = false;
}

void RenderCommandQueue::addCommand(std::unique_ptr<RenderCommand> command)
{
    if (command) {
        mCommands.push_back(std::move(command));
        mDeferred.push_back(mDeferring);
    }
}

void RenderCommandQueue::setDeferring(const bool deferring)
{
    mDeferring = deferring;
}

bool RenderCommandQueue::deferred(const size_t index) const
{
    return index < mDeferred.size() && mDeferred[index];
}

void RenderCommandQueue::executeAll(QOpenGLShaderProgram* shader,
                                    GeometryManager* geometryManager,
                                    ResourceManager* resourceManager,
                                    QOpenGLVertexArrayObject& vao,
                                    QOpenGLBuffer& vertexBuffer,
                                    QOpenGLBuffer& colorBuffer,
                                    QOpenGLBuffer& normalBuffer,
                                    QOpenGLBuffer& indexBuffer,
                                    QOpenGLBuffer& texCoordBuffer)
{
    execute(false, shader, geometryManager, resourceManager, vao, vertexBuffer, colorBuffer, normalBuffer, indexBuffer, texCoordBuffer);
    std::vector<std::unique_ptr<RenderCommand>> deferred;
    size_t index = 0;
    for (auto& command : mCommands) {
        if (mDeferred[index++]) {
            deferred.push_back(std::move(command));
        }
    }
    const bool deferring = mDeferring;
    clear();
    mCommands = std::move(deferred);
    mDeferred.assign(mCommands.size(), true);
    mDeferring = deferring;
}

void RenderCommandQueue::executeDeferred(QOpenGLShaderProgram* shader,
                                         GeometryManager* geometryManager,
                                         ResourceManager* resourceManager,
                                         QOpenGLVertexArrayObject& vao,
                                         QOpenGLBuffer& vertexBuffer,
                                         QOpenGLBuffer& colorBuffer,
                                         QOpenGLBuffer& normalBuffer,
                                         QOpenGLBuffer& indexBuffer,
                                         QOpenGLBuffer& texCoordBuffer)
{
    if (!mCommands.empty()) {
        execute(true, shader, geometryManager, resourceManager, vao, vertexBuffer, colorBuffer, normalBuffer, indexBuffer, texCoordBuffer);
    }
    clear();
}

void RenderCommandQueue::execute(const bool deferredOnes,
                                 QOpenGLShaderProgram* shader,
                                 GeometryManager* geometryManager,
                                 ResourceManager* resourceManager,
                                 QOpenGLVertexArrayObject& vao,
                                 QOpenGLBuffer& vertexBuffer,
                                 QOpenGLBuffer& colorBuffer,
                                 QOpenGLBuffer& normalBuffer,
                                 QOpenGLBuffer& indexBuffer,
                                 QOpenGLBuffer& texCoordBuffer)
{
    if (!mInitialized) {
        qWarning() << "RenderCommandQueue: Not initialized";
        // Dropped with the rest, so that the deferred ones do not warn again
        clear();
        return;
    }
    if (!shader) {
        qWarning() << "RenderCommandQueue: No shader program provided";
        return;
    }
    if (!geometryManager) {
        qWarning() << "RenderCommandQueue: No geometry manager provided";
        return;
    }
    if (!resourceManager) {
        qWarning() << "RenderCommandQueue: No resource manager provided";
        return;
    }

    for (size_t index = 0; index < mCommands.size(); ++index) {
        const auto& command = mCommands[index];
        if (command && mDeferred[index] == deferredOnes) {
            command->execute(this, shader, geometryManager, resourceManager, vao, vertexBuffer, colorBuffer, normalBuffer, indexBuffer, texCoordBuffer);

            // Update statistics
            mTotalCommandsExecuted++;
            const char* commandName = command->getCommandName();
            if (strcmp(commandName, "RenderCube") == 0) {
                mCubeCommandsExecuted++;
            } else if (strcmp(commandName, "RenderLines") == 0) {
                mLineCommandsExecuted++;
            } else if (strcmp(commandName, "RenderTriangles") == 0) {
                mTriangleCommandsExecuted++;
            } else if (strcmp(commandName, "GLState") == 0) {
                mStateCommandsExecuted++;
            }
        }
    }
}

void RenderCommandQueue::clear()
{
    mCommands.clear();
    mDeferred.clear();
    mDeferring = false;
}

size_t RenderCommandQueue::getCommandCount() const
{
    return mCommands.size();
}

void RenderCommandQueue::printStatistics() const
{
    qDebug() << "RenderCommandQueue Statistics:";
    qDebug() << "  Total commands executed:" << mTotalCommandsExecuted;
    qDebug() << "  Cube commands:" << mCubeCommandsExecuted;
    qDebug() << "  Line commands:" << mLineCommandsExecuted;
    qDebug() << "  Triangle commands:" << mTriangleCommandsExecuted;
    qDebug() << "  State commands:" << mStateCommandsExecuted;
}
