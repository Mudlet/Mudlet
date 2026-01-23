#ifndef MUDLET_RENDER_COMMAND_QUEUE_H
#define MUDLET_RENDER_COMMAND_QUEUE_H

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

#include <vector>
#include <memory>
#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLShaderProgram>
#include <QOpenGLFunctions>

#include "RenderCommand.h"
#include "GeometryManager.h"

class RenderCommandQueue : protected QOpenGLFunctions
{
public:
    RenderCommandQueue();
    ~RenderCommandQueue();

    void initialize();
    void cleanup();

    // Add commands to the queue
    void addCommand(std::unique_ptr<RenderCommand> command);

    // Execute all queued commands
    void executeAll(QOpenGLShaderProgram* shader,
                   GeometryManager* geometryManager,
                   ResourceManager* resourceManager,
                   QOpenGLVertexArrayObject& vao,
                   QOpenGLBuffer& vertexBuffer,
                   QOpenGLBuffer& colorBuffer,
                   QOpenGLBuffer& normalBuffer,
                   QOpenGLBuffer& indexBuffer,
                   QOpenGLBuffer& texCoordBuffer);

    // Clear the command queue
    void clear();

    // Get queue statistics
    size_t getCommandCount() const;
    void printStatistics() const;

private:
    std::vector<std::unique_ptr<RenderCommand>> mCommands;
    bool mInitialized = false;

    // Statistics
    mutable size_t mTotalCommandsExecuted = 0;
    mutable size_t mCubeCommandsExecuted = 0;
    mutable size_t mLineCommandsExecuted = 0;
    mutable size_t mTriangleCommandsExecuted = 0;
    mutable size_t mStateCommandsExecuted = 0;
};

#endif // MUDLET_RENDER_COMMAND_QUEUE_H
