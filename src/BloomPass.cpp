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

#include "BloomPass.h"

#include "utils.h"

#include <QDebug>
#include <QOpenGLFramebufferObject>
#include <QOpenGLShaderProgram>
#include <QVector2D>

namespace {
// How many times smaller each side of the blurred image is than the frame's
constexpr int scmDownscale = 4;
enum Pass { BrightPass = 0, BlurPass = 1, AddPass = 2 };

std::unique_ptr<QOpenGLFramebufferObject> linearTarget(QOpenGLExtraFunctions* gl, const QSize& size)
{
    auto target = std::make_unique<QOpenGLFramebufferObject>(size, QOpenGLFramebufferObject::NoAttachment, GL_TEXTURE_2D, GL_RGBA8);
    // Made with nearest filtering, which the taps between texels rely on not having
    gl->glBindTexture(GL_TEXTURE_2D, target->texture());
    gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    gl->glBindTexture(GL_TEXTURE_2D, 0);
    return target;
}
} // namespace

BloomPass::BloomPass() = default;

BloomPass::~BloomPass() = default;

bool BloomPass::initialize()
{
    if (mInitialized || mFailed) {
        return mInitialized;
    }
    initializeOpenGLFunctions();
    mProgram = std::make_unique<QOpenGLShaderProgram>();
    if (!mProgram->addShaderFromSourceFile(QOpenGLShader::Vertex, qsl(":/shaders/bloom_vertex.glsl"))
        || !mProgram->addShaderFromSourceFile(QOpenGLShader::Fragment, qsl(":/shaders/bloom_fragment.glsl")) || !mProgram->link()) {
        qWarning() << "BloomPass: shaders failed, the map will be drawn without a glow:" << mProgram->log();
        mProgram.reset();
        mFailed = true;
        return false;
    }
    // A core profile draws nothing without a vertex array bound, though this one holds no buffers
    if (!mVAO.create()) {
        mProgram.reset();
        mFailed = true;
        return false;
    }
    mInitialized = true;
    return true;
}

bool BloomPass::resize(const QSize& size)
{
    if (size == mSize && mFrame) {
        return true;
    }
    const QSize small((size.width() + scmDownscale - 1) / scmDownscale, (size.height() + scmDownscale - 1) / scmDownscale);
    mFrame = linearTarget(this, size);
    mBright = linearTarget(this, small);
    mBlurred = linearTarget(this, small);
    mSize = size;
    if (!mFrame->isValid() || !mBright->isValid() || !mBlurred->isValid()) {
        mFrame.reset();
        mBright.reset();
        mBlurred.reset();
        mSize = QSize();
        return false;
    }
    return true;
}

bool BloomPass::apply(const GLuint framebuffer, const QSize& size, const float strength)
{
    if (size.isEmpty() || !initialize() || !resize(size)) {
        return false;
    }
    const QSize small = mBright->size();

    // The frame may be multisampled, which only a blit to one of its own size can read
    glBindFramebuffer(GL_READ_FRAMEBUFFER, framebuffer);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, mFrame->handle());
    glBlitFramebuffer(0, 0, size.width(), size.height(), 0, 0, size.width(), size.height(), GL_COLOR_BUFFER_BIT, GL_NEAREST);

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    glDisable(GL_MULTISAMPLE);
    mProgram->bind();
    mVAO.bind();
    glActiveTexture(GL_TEXTURE0);
    mProgram->setUniformValue("uSource", 0);
    const auto pass = [this](QOpenGLFramebufferObject* source, const QSize& sourceSize, const int kind, const QVector2D& direction = QVector2D()) {
        glBindTexture(GL_TEXTURE_2D, source->texture());
        mProgram->setUniformValue("uPass", kind);
        mProgram->setUniformValue("uTexel", QVector2D(1.0f / static_cast<float>(sourceSize.width()), 1.0f / static_cast<float>(sourceSize.height())));
        mProgram->setUniformValue("uDirection", direction);
        glDrawArrays(GL_TRIANGLES, 0, 3);
    };

    glViewport(0, 0, small.width(), small.height());
    glBindFramebuffer(GL_FRAMEBUFFER, mBright->handle());
    pass(mFrame.get(), size, BrightPass);
    glBindFramebuffer(GL_FRAMEBUFFER, mBlurred->handle());
    pass(mBright.get(), small, BlurPass, QVector2D(1.0f, 0.0f));
    glBindFramebuffer(GL_FRAMEBUFFER, mBright->handle());
    pass(mBlurred.get(), small, BlurPass, QVector2D(0.0f, 1.0f));

    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glViewport(0, 0, size.width(), size.height());
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE);
    mProgram->setUniformValue("uStrength", strength);
    pass(mBright.get(), small, AddPass);

    glBindTexture(GL_TEXTURE_2D, 0);
    mVAO.release();
    mProgram->release();
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_MULTISAMPLE);
    glEnable(GL_DEPTH_TEST);
    return true;
}

void BloomPass::cleanup()
{
    mFrame.reset();
    mBright.reset();
    mBlurred.reset();
    mSize = QSize();
    if (mVAO.isCreated()) {
        mVAO.destroy();
    }
    mProgram.reset();
    mInitialized = false;
    mFailed = false;
}
