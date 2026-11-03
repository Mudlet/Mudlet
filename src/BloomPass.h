#ifndef MUDLET_BLOOM_PASS_H
#define MUDLET_BLOOM_PASS_H

/***************************************************************************
 *   Copyright (C) 2026 by Mudlet Developers                               *
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

#include <QOpenGLExtraFunctions>
#include <QOpenGLVertexArrayObject>
#include <QSize>

#include <memory>

class QOpenGLFramebufferObject;
class QOpenGLShaderProgram;

// A soft glow around whatever is drawn bright: the bright parts of a frame are taken at a quarter of its size,
// blurred, and added back onto it
class BloomPass : protected QOpenGLExtraFunctions
{
public:
    BloomPass();
    ~BloomPass();
    Q_DISABLE_COPY(BloomPass)

    // Glows what framebuffer, of size device pixels, holds, and leaves it bound with the viewport over all of it;
    // false, with nothing changed, if the pass could not be set up
    bool apply(GLuint framebuffer, const QSize& size, float strength);
    // With the context it was used in current
    void cleanup();

private:
    bool initialize();
    bool resize(const QSize& size);

    std::unique_ptr<QOpenGLShaderProgram> mProgram;
    QOpenGLVertexArrayObject mVAO;
    // The frame as it is, resolved from its samples, then its bright parts and those blurred one way and back
    std::unique_ptr<QOpenGLFramebufferObject> mFrame;
    std::unique_ptr<QOpenGLFramebufferObject> mBright;
    std::unique_ptr<QOpenGLFramebufferObject> mBlurred;
    QSize mSize;
    bool mInitialized = false;
    bool mFailed = false;
};

#endif // MUDLET_BLOOM_PASS_H
