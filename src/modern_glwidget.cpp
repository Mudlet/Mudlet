/***************************************************************************
 *   Copyright (C) 2008-2013 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2014, 2016, 2019-2021, 2023 by Stephen Lyons            *
 *                                               - slysven@virginmedia.com *
 *   Copyright (C) 2025 by Vadim Peretokin - vadim.peretokin@mudlet.org    *
 *   Copyright (C) 2025 by Lecker Kebap - Leris@mudlet.org                 *
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

#include "modern_glwidget.h"

#include "Host.h"
#include "RoomAppearance.h"
#include "TArea.h"
#include "TRoom.h"
#include "TRoomDB.h"
#include "dlgMapper.h"
#include "mudlet.h"

#include <QtEvents>
#include <QDebug>
#include <QLabel>
#include <QPainter>
#include <QKeyEvent>
#include <QSurfaceFormat>
#include <QVBoxLayout>
#include <chrono>

using namespace std::chrono_literals;

ModernGLWidget::ModernGLWidget(TMap* pMap, Host* pHost, QWidget* parent)
: QOpenGLWidget(parent)
, mpMap(pMap)
, mShaderManager(&mResourceManager, this)
, mVertexBuffer(QOpenGLBuffer::VertexBuffer)
, mColorBuffer(QOpenGLBuffer::VertexBuffer)
, mNormalBuffer(QOpenGLBuffer::VertexBuffer)
, mIndexBuffer(QOpenGLBuffer::IndexBuffer)
, mpHost(pHost)
{
    // Per widget rather than application wide, so the classic 3D view keeps its compatibility profile context
    QSurfaceFormat format = QSurfaceFormat::defaultFormat();
    format.setVersion(3, 3);
    format.setProfile(QSurfaceFormat::CoreProfile);
    format.setSamples(4);
    setFormat(format);

    if (mpHost->mBgColor_2.alpha() < 255) {
        setAttribute(Qt::WA_OpaquePaintEvent, false);
        setAttribute(Qt::WA_AlwaysStackOnTop);
    } else {
        setAttribute(Qt::WA_OpaquePaintEvent);
    }

    // Initialize smooth camera animation
    mCameraAnimationTimer = new QTimer(this);
    mCameraAnimationTimer->setInterval(17ms); // ~60fps updates for smoother animation
    connect(mCameraAnimationTimer, &QTimer::timeout, this, &ModernGLWidget::onCameraAnimationTick);
    mEasingCurve.setType(QEasingCurve::OutQuart); // Natural deceleration
}

ModernGLWidget::~ModernGLWidget()
{
    cleanup();
}

void ModernGLWidget::cleanup()
{
    makeCurrent();
    mLabelTextureCache.cleanup();
    mResourceManager.cleanup();
    mRenderCommandQueue.cleanup();
    mGeometryManager.cleanup();
    mShaderManager.cleanup();
    mVertexBuffer.destroy();
    mColorBuffer.destroy();
    mNormalBuffer.destroy();
    mIndexBuffer.destroy();
    mTexCoordBuffer.destroy();
    mVAO.destroy();
    doneCurrent();
    // ~QOpenGLWidget destroys the context after this class's members are gone
    if (context()) {
        disconnect(context(), &QOpenGLContext::aboutToBeDestroyed, this, &ModernGLWidget::cleanup);
    }
}

QSize ModernGLWidget::minimumSizeHint() const
{
    return QSize(50, 50);
}

QSize ModernGLWidget::sizeHint() const
{
    return QSize(400, 400);
}

void ModernGLWidget::showEvent(QShowEvent* event)
{
    QOpenGLWidget::showEvent(event);
    // Qt creates the context once the widget is laid out; if that failed, initializeGL never runs to say so
    QTimer::singleShot(0, this, [this]() {
        if (isVisible() && !isValid() && mFailureMessage.isEmpty()) {
            //: Shown in place of the 3D map when the graphics driver cannot create the OpenGL context the modern 3D mapper needs
            showFailure(tr("The modern 3D mapper could not create an OpenGL 3.3 context on this system."));
        }
    });
}

void ModernGLWidget::showFailure(const QString& message)
{
    mFailureMessage = message;
    qWarning().noquote() << "ModernGLWidget:" << message;
    if (!mpFailureLabel) {
        mpFailureLabel = new QLabel(this);
        mpFailureLabel->setAlignment(Qt::AlignCenter);
        mpFailureLabel->setWordWrap(true);
        mpFailureLabel->setAutoFillBackground(true);
        mpFailureLabel->setMargin(16);
        auto layout = new QVBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->addWidget(mpFailureLabel);
    }
    //: Second paragraph of the message shown in place of the 3D map when the modern 3D mapper cannot run; keep setConfig("experiment.3dmap.modernmapper", false) exactly as written, it is Lua code the user types
    mpFailureLabel->setText(qsl("%1\n\n%2").arg(message, tr("Use setConfig(\"experiment.3dmap.modernmapper\", false) to switch back to the classic 3D view.")));
    mpFailureLabel->show();
}

void ModernGLWidget::initializeGL()
{
    initializeOpenGLFunctions();

    // A context rebuilt by floating or redocking the map gets a fresh check
    mFailureMessage.clear();
    if (mpFailureLabel) {
        mpFailureLabel->hide();
    }

    // Reparenting to another window replaces the context, and everything made in the old one goes with it
    connect(context(), &QOpenGLContext::aboutToBeDestroyed, this, &ModernGLWidget::cleanup);

    const QSurfaceFormat actualFormat = context()->format();
    if (actualFormat.version() < qMakePair(3, 3)) {
        //: Shown in place of the 3D map when the graphics driver is too old for the modern 3D mapper; %1.%2 is the OpenGL version it offers, such as 2.1
        showFailure(tr("The modern 3D mapper needs OpenGL 3.3, but this system only provides OpenGL %1.%2.").arg(actualFormat.majorVersion()).arg(actualFormat.minorVersion()));
        return;
    }
    qDebug().nospace() << "ModernGLWidget: OpenGL " << actualFormat.majorVersion() << "." << actualFormat.minorVersion()
                       << (actualFormat.profile() == QSurfaceFormat::CoreProfile ? " core" : " compatibility") << " on " << reinterpret_cast<const char*>(glGetString(GL_RENDERER));

    const QColor color(mpHost->mBgColor_2);
    glClearColor(color.redF(), color.greenF(), color.blueF(), color.alphaF());

    // Camera controller will initialize with default view parameters

    // Enable features
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glClearDepth(1.0);

    is2DView = false;

    if (!mShaderManager.initialize()) {
        qWarning() << "Failed to initialize ShaderManager";
        //: Shown in place of the 3D map when the graphics driver rejects the modern 3D mapper's shaders
        showFailure(tr("The modern 3D mapper could not compile its shaders on this graphics driver."));
        return;
    }

    connect(&mShaderManager, &ShaderManager::shadersReloaded, this, QOverload<>::of(&QWidget::update), Qt::UniqueConnection);

    // setupBuffers() checks for GL errors through it, which it skips until initialized
    mResourceManager.initialize();

    setupBuffers();

    // Initialize geometry manager
    mGeometryManager.initialize();

    // Initialize render command queue
    mRenderCommandQueue.initialize();

    // Initialize label texture cache
    mLabelTextureCache.initialize();
}


void ModernGLWidget::setupBuffers()
{
    // Create VAO
    mVAO.create();
    mResourceManager.onVAOCreated();
    mResourceManager.checkGLError(qsl("VAO creation"));
    QOpenGLVertexArrayObject::Binder vaoBinder(&mVAO);

    // Create vertex buffer
    mVertexBuffer.create();
    mResourceManager.onBufferCreated();
    mVertexBuffer.bind();
    mVertexBuffer.setUsagePattern(QOpenGLBuffer::DynamicDraw);
    mResourceManager.checkGLError(qsl("Vertex buffer creation"));

    // Create color buffer
    mColorBuffer.create();
    mResourceManager.onBufferCreated();
    mColorBuffer.bind();
    mColorBuffer.setUsagePattern(QOpenGLBuffer::DynamicDraw);
    mResourceManager.checkGLError(qsl("Color buffer creation"));

    // Create normal buffer
    mNormalBuffer.create();
    mResourceManager.onBufferCreated();
    mNormalBuffer.bind();
    mNormalBuffer.setUsagePattern(QOpenGLBuffer::DynamicDraw);
    mResourceManager.checkGLError(qsl("Normal buffer creation"));

    // Create index buffer
    mIndexBuffer.create();
    mResourceManager.onBufferCreated();
    mIndexBuffer.bind();
    mIndexBuffer.setUsagePattern(QOpenGLBuffer::DynamicDraw);
    mResourceManager.checkGLError(qsl("Index buffer creation"));

    // Create texture coordinate buffer
    mTexCoordBuffer.create();
    mResourceManager.onBufferCreated();
    mTexCoordBuffer.bind();
    mTexCoordBuffer.setUsagePattern(QOpenGLBuffer::DynamicDraw);
    mResourceManager.checkGLError(qsl("Texture coordinate buffer creation"));

    // Configure vertex attribute pointers (will be set during rendering)
}

void ModernGLWidget::updateMatrices()
{
    // Update camera controller with current state, but skip position updates during smooth animation
    if (!mCameraSmoothAnimating && !mPanMode) {
        mCameraController.setTarget(static_cast<float>(mMapCenterX), static_cast<float>(mMapCenterY), static_cast<float>(mMapCenterZ));
    }
    mCameraController.setViewportSize(width(), height());
    mCameraController.updateMatrices();
}

void ModernGLWidget::resizeGL(int w, int h)
{
    glViewport(0, 0, w, h);
    updateMatrices();
}

void ModernGLWidget::paintGL()
{
    // Start frame timing
    mFrameTimer.start();

    if (!mpMap || !mFailureMessage.isEmpty()) {
        return;
    }

    QOpenGLShaderProgram* shaderProgram = mShaderManager.getMainShaderProgram();
    if (!shaderProgram) {
        return;
    }

    glEnable(GL_MULTISAMPLE);

    if (mShiftMode && (mRID != mpMap->mRoomIdHash.value(mpMap->mProfileName) || mpMap->mNewMove)) {
        mShiftMode = false;
    }

    if (!mShiftMode) {
        // Taken here as well as by the 2D map, so a move seen in 3D does not recenter the 2D map later
        mpMap->mNewMove = false;
        mRID = mpMap->mRoomIdHash.value(mpMap->mProfileName);
        TRoom* pRID = mpMap->mpRoomDB->getRoom(mRID);
        if (!pRID) {
            glClearDepth(1.0);
            glDepthFunc(GL_LESS);
            glClearColor(0.0, 0.0, 0.0, 1.0);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            QPainter painter(this);
            painter.setPen(QColorConstants::White);
            painter.setFont(QFont("Bitstream Vera Sans Mono", 30));
            painter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);

            QString message;
            if (mpMap->mpRoomDB) {
                if (mpMap->mpRoomDB->isEmpty()) {
                    message = tr("No rooms in the map - load another one, or start mapping from scratch to begin.");
                } else {
                    message = tr("You have a map loaded (%n room(s)), but Mudlet does not know where you are at the moment.", nullptr, mpMap->mpRoomDB->size());
                }
            } else {
                message = tr("You do not have a map yet - load one, or start mapping from scratch to begin.");
            }
            painter.drawText(0, 0, (width() - 1), (height() - 1), Qt::AlignCenter | Qt::TextWordWrap, message);
            painter.end();

            return;
        }

        const int targetAID = pRID->getArea();
        if (targetAID != mAID) {
            jumpTo(targetAID, pRID->x(), pRID->y(), pRID->z());
        } else if (mRID != mPreviousRID || mMapCenterX != pRID->x() || mMapCenterY != pRID->y() || mMapCenterZ != pRID->z()) {
            startSmoothTransition(targetAID, pRID->x(), pRID->y(), pRID->z());
        }
        mPreviousRID = mRID;
    }

    TArea* pArea = mpMap->mpRoomDB->getArea(mAID);
    if (!pArea) {
        return;
    }

    if (pArea->gridMode) {
        mCameraController.setGridMode(true);
    }

    if (mFramePending && !mDistanceRequested) {
        frameArea();
    }
    mFramePending = false;
    mDistanceRequested = false;

    zmax = static_cast<float>(pArea->max_z);
    zmin = static_cast<float>(pArea->min_z);

    // Clear the screen
    const QColor color(mpHost->mBgColor_2);
    glClearColor(color.redF(), color.greenF(), color.blueF(), color.alphaF());
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    // The QPainter that draws the overlay after the scene resets these when it ends
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Update transformation matrices
    updateMatrices();

    // Use our shader program
    shaderProgram->bind();

    // Enable room flattening
    zFlattening = 8.0f;

    // Build up render commands in correct order:
    // 1. Connections (bottom layer)
    // 2. Background labels (showOnTop=false)
    // 3. Rooms
    // 4. Foreground labels (showOnTop=true)
    renderConnections();
    renderBackgroundLabels();
    renderRooms();
    renderForegroundLabels();

    // Execute all queued commands
    mRenderCommandQueue.executeAll(shaderProgram, &mGeometryManager, &mResourceManager, mVAO, mVertexBuffer, mColorBuffer, mNormalBuffer, mIndexBuffer, mTexCoordBuffer);

    shaderProgram->release();

    QPainter painter(this);

    // Draw map info using contributor manager
    QColor infoColor;
    if (mpHost->mBgColor_2.lightness() > 127) {
        infoColor = QColor(Qt::black);
    } else {
        infoColor = QColor(Qt::white);
    }
    dlgMapper::paintMapInfo(mFrameTimer, painter, mpHost, mpMap, mRID, mAID, 0, infoColor, 10, 10, width(), mFontHeight);

    painter.end();
}

namespace {
// Texels across the textures drawn on room tops
constexpr int scmRoomTextureSize = 128;
// Past this many, those textures are dropped to be made again as they are next needed
constexpr qsizetype scmMaxRoomTextures = 512;

// A colour dimmed for its level's distance from the one being viewed
QVector4D levelFaded(const QColor& color, const int levelDistance, const bool aboveLevel, const bool moreTransparent)
{
    float darkness = 1.0f;
    float alpha = color.alphaF();
    if (moreTransparent) {
        if (levelDistance == 1) {
            darkness = 0.5f;
        } else if (levelDistance == 2) {
            darkness = 0.2f;
        } else if (levelDistance > 2) {
            darkness = 0.05f;
        }
    } else if (aboveLevel) {
        darkness = 0.25f;
        alpha *= 0.2f;
    }
    return QVector4D(color.redF() * darkness, color.greenF() * darkness, color.blueF() * darkness, alpha);
}

// An image whose clear texels keep edgeColor's hue, so that filtering does not fringe what is drawn on it with black
QImage clearRoomTextureImage(QColor edgeColor)
{
    QImage image(scmRoomTextureSize, scmRoomTextureSize, QImage::Format_ARGB32);
    edgeColor.setAlpha(0);
    image.fill(edgeColor);
    return image;
}
} // namespace

GLuint ModernGLWidget::symbolTexture(const QString& symbol, const QColor& color)
{
    const QFont& font = mpMap->mMapSymbolFont;
    const qreal fudgeFactor = mpMap->mMapSymbolFontFudgeFactor;
    const QString key = qsl("symbol|%1|%2|%3|%4").arg(color.name(QColor::HexArgb), font.key(), QString::number(fudgeFactor), symbol);
    if (const GLuint texture = mLabelTextureCache.imageTexture(key)) {
        return texture;
    }

    QImage image = clearRoomTextureImage(color);
    QPainter painter(&image);
    ushort fontSize = 1;
    RoomAppearance::paintSymbol(painter, image.rect(), symbol, color, font, fudgeFactor, fontSize);
    painter.end();
    return mLabelTextureCache.addImageTexture(key, image);
}

GLuint ModernGLWidget::discTexture(const QString& key, const QGradientStops& stops)
{
    if (const GLuint texture = mLabelTextureCache.imageTexture(key)) {
        return texture;
    }

    QImage image = clearRoomTextureImage(stops.isEmpty() ? QColor(Qt::transparent) : stops.constLast().second);
    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing);
    const qreal radius = scmRoomTextureSize / 2.0;
    const QPointF center(radius, radius);
    QRadialGradient gradient(center, radius);
    gradient.setStops(stops);
    painter.setPen(Qt::NoPen);
    painter.setBrush(gradient);
    painter.drawEllipse(center, radius, radius);
    painter.end();
    return mLabelTextureCache.addImageTexture(key, image);
}

void ModernGLWidget::renderRooms()
{
    if (!mpMap || !mpMap->mpRoomDB) {
        return;
    }

    TArea* pArea = mpMap->mpRoomDB->getArea(mAID);
    if (!pArea) {
        return;
    }

    // Always enable depth testing
    auto enableDepthCommand = std::make_unique<GLStateCommand>(GLStateCommand::ENABLE_DEPTH_TEST);
    mRenderCommandQueue.addCommand(std::move(enableDepthCommand));

    float pz = static_cast<float>(mMapCenterZ);
    const int playerRoomId = mpMap->mRoomIdHash.value(mpMap->mProfileName);

    const bool moreTransparent = mpHost->experimentEnabled(qsl("experiment.rendering.more-transparent"));
    const bool playerIcon = mpHost->experimentEnabled(qsl("experiment.3d-player-icon"));
    const bool inOutExits = mpHost->experimentEnabled(qsl("experiment.render-in-out-exits"));

    const float roomSize = 2.0f / scale;
    const float roomHeight = 2.0f / scale / zFlattening;
    // As on the 2D map, a border of thickness 1 is this much of the room's width, and grows outward
    const float borderUnit = mpHost->mRoomBorderSize > 0.0 ? roomSize / static_cast<float>(mpHost->mRoomBorderSize) : 0.0f;
    const QVector3D screenRight = mCameraController.screenRight();
    const QVector3D screenUp = mCameraController.screenUp();

    mLabelTextureCache.limitImageTextures(scmMaxRoomTextures);

    QVector<CubeInstanceData> roomInstances;
    QVector<CubeInstanceData> borderInstances;
    QVector<float> indicatorVertices;
    QVector<float> indicatorColors;
    // Keyed by texture, so each is one draw call however many rooms share it
    QHash<GLuint, GeometryData> symbolDecals;
    QHash<GLuint, GeometryData> highlightDecals;
    GeometryData playerRoomDecal;

    QSetIterator<int> itRoom(pArea->getAreaRooms());
    while (itRoom.hasNext()) {
        int currentRoomId = itRoom.next();
        TRoom* pR = mpMap->mpRoomDB->getRoom(currentRoomId);
        if (!pR) {
            continue;
        }

        if (pR->isHidden()) {
            continue;
        }

        auto rx = static_cast<float>(pR->x());
        auto ry = static_cast<float>(pR->y());
        auto rz = static_cast<float>(pR->z());

        // Level filtering logic from original
        if (rz > pz) {
            if (abs(rz - pz) > mShowTopLevels) {
                continue;
            }
        }
        if (rz < pz) {
            if (abs(rz - pz) > mShowBottomLevels) {
                continue;
            }
        }

        const bool isCurrentRoom = currentRoomId == playerRoomId;
        const bool aboveLevel = rz > pz;
        const int levelDistance = static_cast<int>(std::abs(rz - pz));

        const QColor roomColor = RoomAppearance::environmentColor(*mpMap, *mpHost, pR->environment);
        const QVector4D roomTint = levelFaded(roomColor, levelDistance, aboveLevel, moreTransparent);
        QMatrix4x4 transform;
        transform.translate(rx, ry, rz);
        transform.scale(1.0f / scale, 1.0f / scale, 1.0f / scale / zFlattening);
        roomInstances.append(CubeInstanceData(transform, roomTint.x(), roomTint.y(), roomTint.z(), roomTint.w()));

        if (mpHost->mMapperShowRoomBorders || pR->mBorderColor.isValid() || pR->mBorderThickness > 0) {
            const float borderWidth = (pR->mBorderThickness > 0 ? pR->mBorderThickness : 1) * borderUnit;
            const float spread = (roomSize + 2.0f * borderWidth) / roomSize;
            const QColor borderColor = pR->mBorderColor.isValid() ? pR->mBorderColor : mpHost->mRoomBorderColor;
            const QVector4D borderTint = levelFaded(borderColor, levelDistance, aboveLevel, moreTransparent);
            // Lower than the room, so that its top is seen only around the room's
            QMatrix4x4 borderTransform;
            borderTransform.translate(rx, ry, rz);
            borderTransform.scale(spread / scale, spread / scale, 0.6f / scale / zFlattening);
            borderInstances.append(CubeInstanceData(borderTransform, borderTint.x(), borderTint.y(), borderTint.z(), borderTint.w()));
        }

        const QVector3D roomTop(rx, ry, rz + roomHeight / 2.0f);
        const QVector4D decalTint = levelFaded(QColor(Qt::white), levelDistance, aboveLevel, moreTransparent);

        if (!pR->mSymbol.isEmpty()) {
            const QColor symbolColor = RoomAppearance::symbolColor(pR->mSymbolColor, roomColor);
            if (const GLuint texture = symbolTexture(pR->mSymbol, symbolColor)) {
                GeometryData& batch = symbolDecals[texture];
                batch.textureId = texture;
                GeometryManager::appendGroundQuad(batch, roomTop, roomSize, screenRight, screenUp, decalTint);
            }
        }

        if (pR->highlight) {
            const QString key = qsl("highlight|%1|%2").arg(pR->highlightColor.name(QColor::HexArgb), pR->highlightColor2.name(QColor::HexArgb));
            if (const GLuint texture = discTexture(key, RoomAppearance::highlightStops(pR->highlightColor, pR->highlightColor2))) {
                GeometryData& batch = highlightDecals[texture];
                batch.textureId = texture;
                // highlightRadius is in rooms' spacing, which is one unit here
                GeometryManager::appendGroundQuad(batch, roomTop, pR->highlightRadius, screenRight, screenUp, decalTint);
            }
        }

        if (isCurrentRoom) {
            const int style = mpHost->mMapStrongHighlight ? 0 : mpMap->mPlayerRoomStyle;
            const QString key = qsl("player|%1|%2|%3|%4")
                                        .arg(QString::number(style),
                                             QString::number(mpMap->mPlayerRoomInnerDiameterPercentage),
                                             mpMap->mPlayerRoomInnerColor.name(QColor::HexArgb),
                                             mpMap->mPlayerRoomOuterColor.name(QColor::HexArgb));
            const QGradientStops stops = RoomAppearance::playerRoomStops(style, mpMap->mPlayerRoomInnerDiameterPercentage, mpMap->mPlayerRoomInnerColor, mpMap->mPlayerRoomOuterColor);
            if (const GLuint texture = discTexture(key, stops)) {
                playerRoomDecal.textureId = texture;
                const float radius = static_cast<float>(RoomAppearance::playerRoomRadius(mpMap->mPlayerRoomOuterDiameterPercentage, roomSize + 2.0f * borderUnit));
                GeometryManager::appendGroundQuad(playerRoomDecal, roomTop, 2.0f * radius, screenRight, screenUp, decalTint);
            }

            if (playerIcon) {
                GeometryData playerIcon = mGeometryManager.generatePlayerIconGeometry(mPlayerIconScale, mPlayerIconRotationX, mPlayerIconRotationY, mPlayerIconRotationZ);
                if (!playerIcon.isEmpty()) {
                    // Create modified geometry positioned slightly above the current room
                    GeometryData positionedIcon = playerIcon;

                    // Position above the room using the adjustable height
                    for (int i = 2; i < positionedIcon.vertices.size(); i += 3) {
                        positionedIcon.vertices[i] += (rz + mPlayerIconHeight);
                    }
                    for (int i = 0; i < positionedIcon.vertices.size(); i += 3) {
                        positionedIcon.vertices[i] += rx;
                        positionedIcon.vertices[i + 1] += ry;
                    }

                    // Use textured rendering for the player icon
                    auto command = std::make_unique<RenderTexturedTrianglesCommand>(
                            positionedIcon, mCameraController.getProjectionMatrix(), mCameraController.getViewMatrix(), mCameraController.getModelMatrix());
                    mRenderCommandQueue.addCommand(std::move(command));
                }
            }
        }

        // Up/down and in/out exit indicators just above the room, batched into one draw for the area
        const float indicatorZ = roomTop.z() + 0.1f / zFlattening;
        addUpDownIndicators(pR, rx, ry, indicatorZ, indicatorVertices, indicatorColors);
        if (inOutExits) {
            addInOutIndicators(pR, rx, ry, indicatorZ, indicatorVertices, indicatorColors);
        }
    }

    if (!borderInstances.isEmpty()) {
        mRenderCommandQueue.addCommand(
                std::make_unique<RenderInstancedCubesCommand>(borderInstances, mCameraController.getProjectionMatrix(), mCameraController.getViewMatrix(), mCameraController.getModelMatrix()));
    }
    if (!roomInstances.isEmpty()) {
        mRenderCommandQueue.addCommand(
                std::make_unique<RenderInstancedCubesCommand>(roomInstances, mCameraController.getProjectionMatrix(), mCameraController.getViewMatrix(), mCameraController.getModelMatrix()));
    }

    renderTriangles(indicatorVertices, indicatorColors);

    if (symbolDecals.isEmpty() && highlightDecals.isEmpty() && playerRoomDecal.isEmpty()) {
        return;
    }
    // Drawn on the room tops in the 2D map's order: symbol, then highlight, then the player's ring
    mRenderCommandQueue.addCommand(std::make_unique<GLStateCommand>(GLStateCommand::DISABLE_DEPTH_WRITE));
    mRenderCommandQueue.addCommand(std::make_unique<GLStateCommand>(GLStateCommand::ENABLE_POLYGON_OFFSET));
    for (const QHash<GLuint, GeometryData>* decals : {&symbolDecals, &highlightDecals}) {
        for (const GeometryData& batch : *decals) {
            mRenderCommandQueue.addCommand(std::make_unique<RenderTexturedTrianglesCommand>(
                    batch, mCameraController.getProjectionMatrix(), mCameraController.getViewMatrix(), mCameraController.getModelMatrix(), RenderTexturedTrianglesCommand::Shading::Unlit));
        }
    }
    if (!playerRoomDecal.isEmpty()) {
        mRenderCommandQueue.addCommand(std::make_unique<RenderTexturedTrianglesCommand>(
                playerRoomDecal, mCameraController.getProjectionMatrix(), mCameraController.getViewMatrix(), mCameraController.getModelMatrix(), RenderTexturedTrianglesCommand::Shading::Unlit));
    }
    mRenderCommandQueue.addCommand(std::make_unique<GLStateCommand>(GLStateCommand::DISABLE_POLYGON_OFFSET));
    mRenderCommandQueue.addCommand(std::make_unique<GLStateCommand>(GLStateCommand::ENABLE_DEPTH_WRITE));
}

void ModernGLWidget::renderConnections()
{
    if (!mpMap || !mpMap->mpRoomDB) {
        return;
    }

    TArea* pArea = mpMap->mpRoomDB->getArea(mAID);
    if (!pArea) {
        return;
    }

    QVector<CubeInstanceData> roomConnectionInstances;
    const QVector3D zVector = QVector3D(0, 0, 1);

    float pz = static_cast<float>(mMapCenterZ);
    const bool inOutExits = mpHost->experimentEnabled(qsl("experiment.render-in-out-exits"));
    const QColor exitColor = mpHost->mFgColor_2;

    // Initialize instance queue
    QVector<CubeInstanceData> areaExitInstances;

    // Collect all lines to draw
    QVector<float> lineVertices;
    QVector<float> lineColors;

    QSetIterator<int> itRoom(pArea->getAreaRooms());
    while (itRoom.hasNext()) {
        const int roomId = itRoom.next();
        TRoom* pR = mpMap->mpRoomDB->getRoom(roomId);
        if (!pR) {
            continue;
        }

        if (pR->isHidden()) {
            continue;
        }

        auto rx = static_cast<float>(pR->x());
        auto ry = static_cast<float>(pR->y());
        auto rz = static_cast<float>(pR->z());

        // Level filtering logic (same as rooms)
        if (rz > pz) {
            if (abs(rz - pz) > mShowTopLevels) {
                continue;
            }
        }
        if (rz < pz) {
            if (abs(rz - pz) > mShowBottomLevels) {
                continue;
            }
        }

        // Get all exits for this room
        QList<int> exitList;
        exitList.push_back(pR->getNorth());
        exitList.push_back(pR->getNortheast());
        exitList.push_back(pR->getEast());
        exitList.push_back(pR->getSoutheast());
        exitList.push_back(pR->getSouth());
        exitList.push_back(pR->getSouthwest());
        exitList.push_back(pR->getWest());
        exitList.push_back(pR->getNorthwest());
        exitList.push_back(pR->getUp());
        exitList.push_back(pR->getDown());
        exitList.push_back(pR->getIn());
        exitList.push_back(pR->getOut());

        // The 2D map's exit colour
        const auto r = static_cast<float>(exitColor.redF());
        const auto g = static_cast<float>(exitColor.greenF());
        const auto b = static_cast<float>(exitColor.blueF());

        for (int i = 0; i < exitList.size(); ++i) {
            int k = exitList[i];
            if (k == -1) {
                continue;
            }

            TRoom* pExit = mpMap->mpRoomDB->getRoom(k);
            if (!pExit) {
                continue;
            }

            if (pExit->isHidden()) {
                continue;
            }

            bool areaExit = (pExit->getArea() != mAID);
            bool inOut = ((i == 10 || i == 11) && inOutExits);

            if (!areaExit) {
                // Normal connection within same area
                auto ex = static_cast<float>(pExit->x());
                auto ey = static_cast<float>(pExit->y());
                auto ez = static_cast<float>(pExit->z());

                // Add line from current room to exit room
                lineVertices << rx << ry << rz; // Start point
                lineVertices << ex << ey << ez; // End point

                // Determine translucency based on destination room level
                bool exitAboveCurrentLevel = (ez > pz);
                float connectionAlpha = exitAboveCurrentLevel ? 0.2f : 1.0f;

                // Add colors for both vertices with appropriate alpha
                lineColors << r << g << b << connectionAlpha; // Start color
                lineColors << r << g << b << connectionAlpha; // End color

                // for volume exits we calculate the cube transformation we need
                const QVector3D exitVector = QVector3D(ex - rx, ey - ry, ez - rz);
                const QQuaternion alignmentQuat = QQuaternion::rotationTo(zVector, exitVector);
                QMatrix4x4 transform = QMatrix4x4();
                if (inOut) {
                    transform.translate(3.0f * exitVector / 8.0f);
                    transform.translate(rx, ry, rz);
                    transform.rotate(alignmentQuat);
                    transform.scale(0.02f, 0.02f, exitVector.length() / 16.0f);
                    roomConnectionInstances.append(CubeInstanceData(transform, r, g, b, connectionAlpha));
                } else {
                    transform.translate(exitVector / 4.0f);
                    transform.translate(rx, ry, rz);
                    transform.rotate(alignmentQuat);
                    transform.scale(0.02f, 0.02f, exitVector.length() / 4.0f);
                    roomConnectionInstances.append(CubeInstanceData(transform, r, g, b, connectionAlpha));
                }
            } else {
                // Area exit - draw directional stub
                float dx = rx, dy = ry, dz = rz;

                // Calculate direction offset based on exit type
                if (i == 0) { // North
                    dy += 1.0f;
                } else if (i == 1) { // Northeast
                    dx += 1.0f;
                    dy += 1.0f;
                } else if (i == 2) { // East
                    dx += 1.0f;
                } else if (i == 3) { // Southeast
                    dx += 1.0f;
                    dy -= 1.0f;
                } else if (i == 4) { // South
                    dy -= 1.0f;
                } else if (i == 5) { // Southwest
                    dx -= 1.0f;
                    dy -= 1.0f;
                } else if (i == 6) { // West
                    dx -= 1.0f;
                } else if (i == 7) { // Northwest
                    dx -= 1.0f;
                    dy += 1.0f;
                } else if (i == 8) { // Up
                    dz += 1.0f;
                } else if (i == 9) { // Down
                    dz -= 1.0f;
                } else if (i == 10) { // In
                    dx -= 1.0f;
                    dy += 0.5f;
                } else if (i == 11) { // Out
                    dx += 1.0f;
                    dy -= 0.5f;
                }

                // Add line from current room to direction offset
                lineVertices << rx << ry << rz; // Start point
                lineVertices << dx << dy << dz; // End point (offset)

                // Determine translucency for area exits based on destination level
                bool exitAboveCurrentLevel = (dz > pz);
                float exitAlpha = exitAboveCurrentLevel ? 0.2f : 1.0f;

                // Darken area exit colors if above current level
                float exitRed = 85.0f / 255.0f;
                float exitGreen = 170.0f / 255.0f;
                float exitBlue = 0.0f;

                if (exitAboveCurrentLevel) {
                    // Drastically darken area exits above current level
                    const float darkenFactor = 0.25f; // Keep only 25% of original brightness
                    exitRed *= darkenFactor;
                    exitGreen *= darkenFactor;
                    exitBlue *= darkenFactor;
                }

                // Use different color for area exits (greenish) with appropriate alpha and darkening
                lineColors << exitRed << exitGreen << exitBlue << exitAlpha; // Start color
                lineColors << exitRed << exitGreen << exitBlue << exitAlpha; // End color

                // for volume exits we calculate the cube transformation we need
                const QVector3D exitVector = QVector3D(dx - rx, dy - ry, dz - rz);
                const QQuaternion alignmentQuat = QQuaternion::rotationTo(zVector, exitVector);
                // double length of normal exits since this exit is one sided
                QMatrix4x4 transform = QMatrix4x4();
                if (inOut) {
                    transform.translate(3.0f * exitVector / 8.0f);
                    transform.translate(rx, ry, rz);
                    transform.rotate(alignmentQuat);
                    transform.scale(0.02f, 0.02f, exitVector.length() / 16.0f);
                    roomConnectionInstances.append(CubeInstanceData(transform, exitRed, exitGreen, exitBlue, exitAlpha));
                    transform.setToIdentity();
                    transform.translate(5.0f * exitVector / 8.0f);
                    transform.translate(rx, ry, rz);
                    transform.rotate(alignmentQuat);
                    transform.scale(0.02f, 0.02f, exitVector.length() / 16.0f);
                    roomConnectionInstances.append(CubeInstanceData(transform, exitRed, exitGreen, exitBlue, exitAlpha));
                } else {
                    transform.translate(exitVector / 2.0f);
                    transform.translate(rx, ry, rz);
                    transform.rotate(alignmentQuat);
                    transform.scale(0.02f, 0.02f, exitVector.length() / 2.0f);
                    roomConnectionInstances.append(CubeInstanceData(transform, exitRed, exitGreen, exitBlue, exitAlpha));
                }

                // Render green area exit cube at the destination position with translucency and darkening
                transform.setToIdentity();
                transform.translate(dx, dy, dz);
                transform.scale(1.0f / scale, 1.0f / scale, 1.0f / scale / zFlattening);
                areaExitInstances.append(CubeInstanceData(transform, exitRed, exitGreen, exitBlue, exitAlpha));

                // Render smaller environment overlay rectangle on top with translucency and darkening
                const QColor envColor = RoomAppearance::environmentColor(*mpMap, *mpHost, pExit->environment);
                float overlayZ = dz + 0.25f / zFlattening;
                float overlayAlpha = exitAboveCurrentLevel ? 0.16f : 0.8f; // 0.2 * 0.8 for above level

                // Darken area exit environment overlay if above current level
                float exitEnvRed = envColor.redF();
                float exitEnvGreen = envColor.greenF();
                float exitEnvBlue = envColor.blueF();

                if (exitAboveCurrentLevel) {
                    // Drastically darken area exit environment overlays above current level
                    const float darkenFactor = 0.25f; // Keep only 25% of original brightness
                    exitEnvRed *= darkenFactor;
                    exitEnvGreen *= darkenFactor;
                    exitEnvBlue *= darkenFactor;
                }

                transform.setToIdentity();
                transform.translate(dx, dy, overlayZ);
                transform.scale(0.5f / scale, 0.5f / scale, 1.0f / scale / zFlattening);
                areaExitInstances.append(CubeInstanceData(transform, exitEnvRed, exitEnvGreen, exitEnvBlue, overlayAlpha));
            }
        }
    }

    // Always enable depth testing
    auto enableDepthCommand = std::make_unique<GLStateCommand>(GLStateCommand::ENABLE_DEPTH_TEST);
    mRenderCommandQueue.addCommand(std::move(enableDepthCommand));

    // Always render room connection volumes
    if (!roomConnectionInstances.isEmpty()) {
        auto command =
                std::make_unique<RenderInstancedCubesCommand>(roomConnectionInstances, mCameraController.getProjectionMatrix(), mCameraController.getViewMatrix(), mCameraController.getModelMatrix());
        mRenderCommandQueue.addCommand(std::move(command));

    } else {
        // Render all collected lines
        if (!lineVertices.isEmpty()) {
            renderLines(lineVertices, lineColors);
        }
    }

    if (!areaExitInstances.isEmpty()) {
        auto command = std::make_unique<RenderInstancedCubesCommand>(areaExitInstances, mCameraController.getProjectionMatrix(), mCameraController.getViewMatrix(), mCameraController.getModelMatrix());
        mRenderCommandQueue.addCommand(std::move(command));
    }
}

void ModernGLWidget::renderCube(float x, float y, float z, float size, float r, float g, float b, float a)
{
    // Create render command and queue it
    auto command = std::make_unique<RenderCubeCommand>(x, y, z, size, r, g, b, a, mCameraController.getProjectionMatrix(), mCameraController.getViewMatrix(), mCameraController.getModelMatrix());
    mRenderCommandQueue.addCommand(std::move(command));
}

void ModernGLWidget::shiftCamera(float verticalAngle, float horizontalAngle, float rotationAngle)
{
    mCameraController.shiftPerspective(verticalAngle, horizontalAngle, rotationAngle);
    emitCameraControls();
    update();
}

void ModernGLWidget::setCameraPosition(float r, float theta, float phi)
{
    mCameraController.setPosition(r, theta, phi);
    mDistanceRequested = true;
    emitCameraControls();
    update();
}

namespace {
// The scale slider runs 0 to 1000, from farthest to closest, evenly on a log scale
constexpr int scmScaleSliderSteps = 1000;
// The tilt slider runs -100 (low, toward the horizon) to 100 (looking straight down)
constexpr float scmTiltPerSliderStep = (CameraController::scmMaxTilt - CameraController::scmMinTilt) / 200.0f;
constexpr float scmTiltAtSliderCenter = (CameraController::scmMaxTilt + CameraController::scmMinTilt) / 2.0f;

float distanceForScaleSlider(int value)
{
    const float fraction = static_cast<float>(value) / scmScaleSliderSteps;
    return CameraController::scmMaxDistance * std::pow(CameraController::scmMinDistance / CameraController::scmMaxDistance, fraction);
}

int scaleSliderForDistance(float distance)
{
    return qRound(scmScaleSliderSteps * std::log(distance / CameraController::scmMaxDistance) / std::log(CameraController::scmMinDistance / CameraController::scmMaxDistance));
}

float tiltForSlider(int value)
{
    return scmTiltAtSliderCenter - scmTiltPerSliderStep * static_cast<float>(value);
}

int sliderForTilt(float tilt)
{
    return qRound((scmTiltAtSliderCenter - tilt) / scmTiltPerSliderStep);
}

// Into -180 to 180, the middle of the sliders' range
int sliderForAngle(float degrees)
{
    return qRound(std::remainder(degrees, 360.0f));
}
} // namespace

void ModernGLWidget::emitCameraControls()
{
    const QVector3D position = mCameraController.getPosition();
    emit cameraControlsChanged(scaleSliderForDistance(mCameraController.getScale()),
                               sliderForTilt(position.y()),
                               sliderForAngle(mCameraController.getRoll()),
                               sliderForAngle(position.z() - CameraController::scmNorthUpAzimuth));
}

void ModernGLWidget::setOrientationKeepingRoll(float theta, float phi)
{
    mCameraController.setOrientation(theta, phi, mCameraController.getRoll());
}

void ModernGLWidget::frameArea()
{
    mFramePending = false;
    if (!mpMap || !mpMap->mpRoomDB) {
        return;
    }
    frameRooms(mpMap->mpRoomDB->get2DMapZoom(mAID));
}

// The same span of rooms the 2D map shows at this zoom
void ModernGLWidget::frameRooms(const qreal zoom)
{
    const float aspectRatio = height() > 0 ? static_cast<float>(width()) / static_cast<float>(height()) : 1.0f;
    mCameraController.setScale(CameraController::distanceToShow(static_cast<float>(zoom), aspectRatio));
    emitCameraControls();
}

void ModernGLWidget::applyMapZoom(const qreal zoom, const int areaId)
{
    if (areaId != mAID) {
        return;
    }
    frameRooms(zoom);
    update();
}

// Implement slot methods (same interface as original)
void ModernGLWidget::slot_showAllLevels()
{
    mShowTopLevels = 999999;
    mShowBottomLevels = 999999;
    update();
}

void ModernGLWidget::slot_shiftDown()
{
    mShiftMode = true;
    mCameraController.translateTargetBackward();
    update();
}

void ModernGLWidget::slot_shiftUp()
{
    mShiftMode = true;
    mCameraController.translateTargetForward();
    update();
}

void ModernGLWidget::slot_shiftLeft()
{
    mShiftMode = true;
    mCameraController.translateTargetLeft();
    update();
}

void ModernGLWidget::slot_shiftRight()
{
    mShiftMode = true;
    mCameraController.translateTargetRight();
    update();
}

void ModernGLWidget::slot_shiftZup()
{
    mShiftMode = true;
    // Not the camera target: each paint sets that from the view center
    ++mMapCenterZ;
    update();
}

void ModernGLWidget::slot_shiftZdown()
{
    mShiftMode = true;
    // Not the camera target: each paint sets that from the view center
    --mMapCenterZ;
    update();
}

void ModernGLWidget::slot_singleLevelView()
{
    mShowTopLevels = 0;
    mShowBottomLevels = 0;
    update();
}

void ModernGLWidget::slot_showMoreUpperLevels()
{
    mShowTopLevels += 1;
    update();
}

void ModernGLWidget::slot_showLessUpperLevels()
{
    mShowTopLevels--;
    if (mShowTopLevels < 0) {
        mShowTopLevels = 0;
    }
    update();
}

void ModernGLWidget::slot_showMoreLowerLevels()
{
    mShowBottomLevels++;
    update();
}

void ModernGLWidget::slot_showLessLowerLevels()
{
    mShowBottomLevels--;
    if (mShowBottomLevels < 0) {
        mShowBottomLevels = 0;
    }
    update();
}

void ModernGLWidget::slot_defaultView()
{
    mCameraController.setDefaultView();
    is2DView = false;
    emitCameraControls();
    update();
}

void ModernGLWidget::slot_sideView()
{
    mCameraController.setSideView();
    is2DView = false;
    emitCameraControls();
    update();
}

void ModernGLWidget::slot_topView()
{
    mCameraController.setTopView();
    is2DView = true;
    emitCameraControls();
    update();
}

void ModernGLWidget::slot_setScale(int value)
{
    mCameraController.setScale(distanceForScaleSlider(value));
    update();
}

void ModernGLWidget::slot_setCameraPositionX(int value)
{
    setOrientationKeepingRoll(tiltForSlider(value), mCameraController.getPosition().z());
    is2DView = false;
    update();
}

void ModernGLWidget::slot_setCameraPositionY(int value)
{
    const QVector3D position = mCameraController.getPosition();
    mCameraController.setOrientation(position.y(), position.z(), static_cast<float>(value));
    is2DView = false;
    update();
}

void ModernGLWidget::slot_setCameraPositionZ(int value)
{
    setOrientationKeepingRoll(mCameraController.getPosition().y(), CameraController::scmNorthUpAzimuth + static_cast<float>(value));
    is2DView = false;
    update();
}

void ModernGLWidget::slot_shiftCameraDown()
{
    const QVector3D position = mCameraController.getPosition();
    setOrientationKeepingRoll(position.y() + 3.0f, position.z());
    is2DView = false;
    emitCameraControls();
    update();
}

void ModernGLWidget::slot_shiftCameraUp()
{
    const QVector3D position = mCameraController.getPosition();
    setOrientationKeepingRoll(position.y() - 3.0f, position.z());
    is2DView = false;
    emitCameraControls();
    update();
}

void ModernGLWidget::slot_shiftCameraLeft()
{
    const QVector3D position = mCameraController.getPosition();
    setOrientationKeepingRoll(position.y(), position.z() - 3.0f);
    is2DView = false;
    emitCameraControls();
    update();
}

void ModernGLWidget::slot_shiftCameraRight()
{
    const QVector3D position = mCameraController.getPosition();
    setOrientationKeepingRoll(position.y(), position.z() + 3.0f);
    is2DView = false;
    emitCameraControls();
    update();
}

void ModernGLWidget::setViewCenter(int areaId, int xPos, int yPos, int zPos)
{
    mShiftMode = true;
    // Hold this view until the player moves on, as the 2D map does
    mRID = mpMap ? mpMap->mRoomIdHash.value(mpMap->mProfileName) : 0;
    mPreviousRID = mRID;

    if (areaId != mAID) {
        jumpTo(areaId, xPos, yPos, zPos);
    } else {
        startSmoothTransition(areaId, xPos, yPos, zPos);
    }
}

void ModernGLWidget::syncView(int areaId, int x, int y, int z, int roomId)
{
    mShiftMode = true;
    mRID = roomId;
    mPreviousRID = roomId;
    jumpTo(areaId, x, y, z);
}

bool ModernGLWidget::followingPlayer() const
{
    return !mShiftMode || !mpMap || mRID != mpMap->mRoomIdHash.value(mpMap->mProfileName) || mpMap->mNewMove;
}

void ModernGLWidget::followPlayer()
{
    mShiftMode = false;
    update();
}

void ModernGLWidget::jumpTo(int areaId, int x, int y, int z)
{
    stopSmoothTransition();
    if (areaId != mAID) {
        mFramePending = true;
    }
    mAID = areaId;
    mMapCenterX = x;
    mMapCenterY = y;
    mMapCenterZ = z;
    mCameraController.setTarget(static_cast<float>(x), static_cast<float>(y), static_cast<float>(z));
    update();
}

void ModernGLWidget::wheelEvent(QWheelEvent* e)
{
    const float steps = CameraController::wheelZoomSteps(e->angleDelta().y(), e->modifiers().testFlag(Qt::ControlModifier), mudlet::self()->invertMapZoom());
    e->accept();
    if (qFuzzyIsNull(steps)) {
        return;
    }
    mCameraController.zoomBy(steps);
    emitCameraControls();
    update();
}

void ModernGLWidget::mousePressEvent(QMouseEvent* event)
{
    // Implement mouse handling (placeholder)
    mudlet::self()->activateProfile(mpHost);
    if (!mpMap || !mpMap->mpRoomDB) {
        return;
    }

    if (event->buttons() & Qt::LeftButton) { // translation on xy-plane
        auto eventPos = event->position().toPoint();
        const int x = eventPos.x();
        const int y = height() - eventPos.y(); // the opengl origin is at bottom left
        mPanMode = true;
        mPanXStart = x;
        mPanYStart = y;
    }
}

void ModernGLWidget::mouseMoveEvent(QMouseEvent* event)
{
    if (!mpMap || !mpMap->mpRoomDB) {
        return;
    }
    if (mPanMode) {
        auto eventPos = event->position();
        auto x = static_cast<float>(eventPos.x());
        auto y = static_cast<float>(height()) - static_cast<float>(eventPos.y()); // the opengl origin is at bottom left
        if ((mPanXStart - x) > 1.0f) {
            if (event->modifiers() & (Qt::ControlModifier | Qt::ShiftModifier)) {
                slot_shiftCameraRight();
            } else {
                slot_shiftLeft();
            }
            mPanXStart = x;
        } else if ((mPanXStart - x) < -1.0f) {
            if (event->modifiers() & (Qt::ControlModifier | Qt::ShiftModifier)) {
                slot_shiftCameraLeft();
            } else {
                slot_shiftRight();
            }
            mPanXStart = x;
        }
        if ((mPanYStart - y) > 1.0f) {
            if (event->modifiers() & (Qt::ControlModifier | Qt::ShiftModifier)) {
                slot_shiftCameraUp();
            } else {
                slot_shiftUp();
            }
            mPanYStart = y;
        } else if ((mPanYStart - y) < -1.0f) {
            if (event->modifiers() & (Qt::ControlModifier | Qt::ShiftModifier)) {
                slot_shiftCameraDown();
            } else {
                slot_shiftDown();
            }
            mPanYStart = y;
        }
    }
    QOpenGLWidget::mouseMoveEvent(event);
}

void ModernGLWidget::mouseReleaseEvent(QMouseEvent* event)
{
    mPanMode = false;
    mCameraController.snapTargetToGrid();
    const QVector3D newCenter = mCameraController.getTarget();
    mMapCenterX = static_cast<int>(newCenter.x());
    mMapCenterY = static_cast<int>(newCenter.y());
    mMapCenterZ = static_cast<int>(newCenter.z());
    QOpenGLWidget::mouseReleaseEvent(event);
    update();
}

void ModernGLWidget::keyPressEvent(QKeyEvent* event)
{
    QOpenGLWidget::keyPressEvent(event);
}

void ModernGLWidget::slot_setPlayerIconHeight(int value)
{
    mPlayerIconHeight = static_cast<float>(value) / 100.0f; // Convert slider value to units (-2.0 to +5.0)
#ifdef DEBUG_PLAYER_ICON_CONTROLS
    qDebug() << "Player Icon - Height:" << mPlayerIconHeight << "RotX:" << mPlayerIconRotationX << "RotY:" << mPlayerIconRotationY << "RotZ:" << mPlayerIconRotationZ << "Scale:" << mPlayerIconScale;
#endif
    update();
}

void ModernGLWidget::slot_setPlayerIconRotationX(int angle)
{
    mPlayerIconRotationX = static_cast<float>(angle);
#ifdef DEBUG_PLAYER_ICON_CONTROLS
    qDebug() << "Player Icon - Height:" << mPlayerIconHeight << "RotX:" << mPlayerIconRotationX << "RotY:" << mPlayerIconRotationY << "RotZ:" << mPlayerIconRotationZ << "Scale:" << mPlayerIconScale;
#endif
    update();
}

void ModernGLWidget::slot_setPlayerIconRotationY(int angle)
{
    mPlayerIconRotationY = static_cast<float>(angle);
#ifdef DEBUG_PLAYER_ICON_CONTROLS
    qDebug() << "Player Icon - Height:" << mPlayerIconHeight << "RotX:" << mPlayerIconRotationX << "RotY:" << mPlayerIconRotationY << "RotZ:" << mPlayerIconRotationZ << "Scale:" << mPlayerIconScale;
#endif
    update();
}

void ModernGLWidget::slot_setPlayerIconRotationZ(int angle)
{
    mPlayerIconRotationZ = static_cast<float>(angle);
#ifdef DEBUG_PLAYER_ICON_CONTROLS
    qDebug() << "Player Icon - Height:" << mPlayerIconHeight << "RotX:" << mPlayerIconRotationX << "RotY:" << mPlayerIconRotationY << "RotZ:" << mPlayerIconRotationZ << "Scale:" << mPlayerIconScale;
#endif
    update();
}

void ModernGLWidget::slot_setPlayerIconScale(int value)
{
    mPlayerIconScale = static_cast<float>(value) / 10000.0f; // Convert slider value to scale (0.001 to 0.02)
#ifdef DEBUG_PLAYER_ICON_CONTROLS
    qDebug() << "Player Icon - Height:" << mPlayerIconHeight << "RotX:" << mPlayerIconRotationX << "RotY:" << mPlayerIconRotationY << "RotZ:" << mPlayerIconRotationZ << "Scale:" << mPlayerIconScale;
#endif
    update();
}

void ModernGLWidget::slot_resetPlayerIcon()
{
    mPlayerIconHeight = 0.51f;
    mPlayerIconRotationX = 1.0f;
    mPlayerIconRotationY = -56.0f;
    mPlayerIconRotationZ = 20.0f;
    mPlayerIconScale = 0.0055f;

#ifdef DEBUG_PLAYER_ICON_CONTROLS
    qDebug() << "Player Icon RESET - Height:" << mPlayerIconHeight << "RotX:" << mPlayerIconRotationX << "RotY:" << mPlayerIconRotationY << "RotZ:" << mPlayerIconRotationZ
             << "Scale:" << mPlayerIconScale;
#endif

    // Reset the slider values to their defaults (need to emit signals to update UI)
    // Convert back to slider values
    emit resetPlayerIconSliders(static_cast<int>(mPlayerIconHeight * 100.0f),   // height: 51
                                static_cast<int>(mPlayerIconRotationX),         // rotX: 1
                                static_cast<int>(mPlayerIconRotationY),         // rotY: -56
                                static_cast<int>(mPlayerIconRotationZ),         // rotZ: 20
                                static_cast<int>(mPlayerIconScale * 10000.0f)); // scale: 55

    update();
}

void ModernGLWidget::renderLines(const QVector<float>& vertices, const QVector<float>& colors)
{
    if (vertices.isEmpty() || colors.isEmpty()) {
        return;
    }

    // Create render command and queue it
    auto command = std::make_unique<RenderLinesCommand>(vertices, colors, mCameraController.getProjectionMatrix(), mCameraController.getViewMatrix(), mCameraController.getModelMatrix());
    mRenderCommandQueue.addCommand(std::move(command));
}

void ModernGLWidget::renderTriangles(const QVector<float>& vertices, const QVector<float>& colors)
{
    if (vertices.isEmpty() || colors.isEmpty()) {
        return;
    }

    // Create render command and queue it
    auto command = std::make_unique<RenderTrianglesCommand>(vertices, colors, mCameraController.getProjectionMatrix(), mCameraController.getViewMatrix(), mCameraController.getModelMatrix());
    mRenderCommandQueue.addCommand(std::move(command));
}

void ModernGLWidget::addUpDownIndicators(TRoom* pRoom, float x, float y, float z, QVector<float>& triangleVertices, QVector<float>& triangleColors) const
{
    if (!pRoom) {
        return;
    }

    // Gray color for indicators (same as original)
    float gray[] = {128.0f / 255.0f, 128.0f / 255.0f, 128.0f / 255.0f, 1.0f};

    // Triangle size (from original: ±0.95/scale for points, ±0.25/scale for base)
    float pointSize = 0.95f / scale;
    float baseSize = 0.25f / scale;

    // Down arrow (if room has down exit)
    if (pRoom->getDown() > -1) {
        // Triangle pointing down: top-left, top-right, bottom-center
        triangleVertices << (x - pointSize) << (y - baseSize) << z; // Top-left
        triangleVertices << (x + pointSize) << (y - baseSize) << z; // Top-right
        triangleVertices << x << (y - pointSize) << z;              // Bottom-center

        // Add gray color for all three vertices
        for (int i = 0; i < 3; ++i) {
            triangleColors << gray[0] << gray[1] << gray[2] << gray[3];
        }
    }

    // Up arrow (if room has up exit)
    if (pRoom->getUp() > -1) {
        // Triangle pointing up: bottom-left, bottom-right, top-center
        triangleVertices << (x - pointSize) << (y + baseSize) << z; // Bottom-left
        triangleVertices << (x + pointSize) << (y + baseSize) << z; // Bottom-right
        triangleVertices << x << (y + pointSize) << z;              // Top-center

        // Add gray color for all three vertices
        for (int i = 0; i < 3; ++i) {
            triangleColors << gray[0] << gray[1] << gray[2] << gray[3];
        }
    }
}

void ModernGLWidget::addInOutIndicators(TRoom* pRoom, float x, float y, float z, QVector<float>& triangleVertices, QVector<float>& triangleColors) const
{
    if (!pRoom) {
        return;
    }

    // Gray color for indicators (same as original)
    float gray[] = {128.0f / 255.0f, 128.0f / 255.0f, 128.0f / 255.0f, 1.0f};

    // Triangle size
    float halfHeight = 0.25f / scale;
    float width = 0.5f / scale;

    // In arrows (if room has in exit)
    if (pRoom->getIn() > -1) {
        // triangle pointing inwards: top-left, bottom-left, mid-center
        triangleVertices << (x - width) << (y + halfHeight) << z; // Top-left
        triangleVertices << (x - width) << (y - halfHeight) << z; // Bottom-left
        triangleVertices << x << y << z;                          // Mid-center
        // triangle pointing inwards: top-right, bottom-right, mid-center
        triangleVertices << (x + width) << (y + halfHeight) << z; // Top-right
        triangleVertices << (x + width) << (y - halfHeight) << z; // Bottom-right
        triangleVertices << x << y << z;                          // Mid-center

        // Add gray color for all six vertices
        for (int i = 0; i < 6; ++i) {
            triangleColors << gray[0] << gray[1] << gray[2] << gray[3];
        }
    }

    // Out arrows (if room has out exit)
    if (pRoom->getOut() > -1) {
        // triangle pointing outwards: top-left, bottom-left, out-center
        triangleVertices << (x - width) << (y + halfHeight) << z; // Top-left
        triangleVertices << (x - width) << (y - halfHeight) << z; // Bottom-left
        triangleVertices << x - 1.0f / scale << y << z;           // Outside-center
        // triangle pointing outwards: top-right, bottom-right, out-center
        triangleVertices << (x + width) << (y + halfHeight) << z; // Top-right
        triangleVertices << (x + width) << (y - halfHeight) << z; // Bottom-right
        triangleVertices << x + 1.0f / scale << y << z;           // Outside-center

        // Add gray color for all six vertices
        for (int i = 0; i < 6; ++i) {
            triangleColors << gray[0] << gray[1] << gray[2] << gray[3];
        }
    }
}

void ModernGLWidget::renderBackgroundLabels()
{
    if (!mpMap || !mpMap->mpRoomDB) {
        return;
    }

    TArea* pArea = mpMap->mpRoomDB->getArea(mAID);
    if (!pArea || pArea->mMapLabels.isEmpty()) {
        return;
    }

    // Disable depth writing for transparent labels
    mRenderCommandQueue.addCommand(std::make_unique<GLStateCommand>(GLStateCommand::DISABLE_DEPTH_WRITE));

    float pz = static_cast<float>(mMapCenterZ);

    for (auto it = pArea->mMapLabels.constBegin(); it != pArea->mMapLabels.constEnd(); ++it) {
        const TMapLabel& label = it.value();
        int labelId = it.key();

        // Only render labels with showOnTop=false in this pass
        if (label.showOnTop) {
            continue;
        }

        // Z-level filtering - same logic as rooms
        float labelZ = label.pos.z();
        if (labelZ > pz) {
            if (std::abs(labelZ - pz) > mShowTopLevels) {
                continue;
            }
        }
        if (labelZ < pz) {
            if (std::abs(labelZ - pz) > mShowBottomLevels) {
                continue;
            }
        }

        // Skip labels without a pixmap
        if (label.pix.isNull()) {
            continue;
        }

        // Get or create texture for this label
        GLuint textureId = mLabelTextureCache.getTexture(mAID, labelId, label.pix);
        if (textureId == 0) {
            continue;
        }

        // Calculate label dimensions - size is in map coordinates
        float labelWidth = static_cast<float>(label.size.width());
        float labelHeight = static_cast<float>(label.size.height());

        // In 2D, label.pos is the top-left corner and extends right and down
        // (with Y inverted). For 3D billboard, we need the center position.
        // Offset: center = pos + (width/2, -height/2, 0)
        float centerX = label.pos.x() + labelWidth / 2.0f;
        float centerY = label.pos.y() - labelHeight / 2.0f;
        float centerZ = label.pos.z();

        // Create render command for this label
        auto command = std::make_unique<RenderLabelCommand>(centerX,
                                                            centerY,
                                                            centerZ,
                                                            labelWidth,
                                                            labelHeight,
                                                            textureId,
                                                            mCameraController.screenRight(),
                                                            mCameraController.screenUp(),
                                                            label.highlight,
                                                            mCameraController.getProjectionMatrix(),
                                                            mCameraController.getViewMatrix(),
                                                            mCameraController.getModelMatrix());

        mRenderCommandQueue.addCommand(std::move(command));
    }

    // Re-enable depth writing for subsequent geometry
    mRenderCommandQueue.addCommand(std::make_unique<GLStateCommand>(GLStateCommand::ENABLE_DEPTH_WRITE));
}

void ModernGLWidget::renderForegroundLabels()
{
    if (!mpMap || !mpMap->mpRoomDB) {
        return;
    }

    TArea* pArea = mpMap->mpRoomDB->getArea(mAID);
    if (!pArea || pArea->mMapLabels.isEmpty()) {
        return;
    }

    // Disable depth writing for transparent labels
    mRenderCommandQueue.addCommand(std::make_unique<GLStateCommand>(GLStateCommand::DISABLE_DEPTH_WRITE));

    float pz = static_cast<float>(mMapCenterZ);

    for (auto it = pArea->mMapLabels.constBegin(); it != pArea->mMapLabels.constEnd(); ++it) {
        const TMapLabel& label = it.value();
        int labelId = it.key();

        // Only render labels with showOnTop=true in this pass
        if (!label.showOnTop) {
            continue;
        }

        // Z-level filtering - same logic as rooms
        float labelZ = label.pos.z();
        if (labelZ > pz) {
            if (std::abs(labelZ - pz) > mShowTopLevels) {
                continue;
            }
        }
        if (labelZ < pz) {
            if (std::abs(labelZ - pz) > mShowBottomLevels) {
                continue;
            }
        }

        // Skip labels without a pixmap
        if (label.pix.isNull()) {
            continue;
        }

        // Get or create texture for this label
        GLuint textureId = mLabelTextureCache.getTexture(mAID, labelId, label.pix);
        if (textureId == 0) {
            continue;
        }

        // Calculate label dimensions
        float labelWidth = static_cast<float>(label.size.width());
        float labelHeight = static_cast<float>(label.size.height());

        // In 2D, label.pos is the top-left corner and extends right and down
        // (with Y inverted). For 3D billboard, we need the center position.
        float centerX = label.pos.x() + labelWidth / 2.0f;
        float centerY = label.pos.y() - labelHeight / 2.0f;
        float centerZ = label.pos.z();

        // Create render command for this label
        auto command = std::make_unique<RenderLabelCommand>(centerX,
                                                            centerY,
                                                            centerZ,
                                                            labelWidth,
                                                            labelHeight,
                                                            textureId,
                                                            mCameraController.screenRight(),
                                                            mCameraController.screenUp(),
                                                            label.highlight,
                                                            mCameraController.getProjectionMatrix(),
                                                            mCameraController.getViewMatrix(),
                                                            mCameraController.getModelMatrix());

        mRenderCommandQueue.addCommand(std::move(command));
    }

    // Re-enable depth writing
    mRenderCommandQueue.addCommand(std::make_unique<GLStateCommand>(GLStateCommand::ENABLE_DEPTH_WRITE));
}

void ModernGLWidget::startSmoothTransition(int targetAID, int targetX, int targetY, int targetZ)
{
    // Set up animation parameters
    mTargetAID = targetAID;
    mTargetMapCenterX = static_cast<float>(targetX);
    mTargetMapCenterY = static_cast<float>(targetY);
    mTargetMapCenterZ = static_cast<float>(targetZ);

    // Store current position as start position
    if (mCameraSmoothAnimating) {
        mStartMapCenterX = mCurrentAnimationX;
        mStartMapCenterY = mCurrentAnimationY;
        mStartMapCenterZ = mCurrentAnimationZ;
    } else {
        mStartMapCenterX = static_cast<float>(mMapCenterX);
        mStartMapCenterY = static_cast<float>(mMapCenterY);
        mStartMapCenterZ = static_cast<float>(mMapCenterZ);
    }

    // Initialize current animation position
    mCurrentAnimationX = mStartMapCenterX;
    mCurrentAnimationY = mStartMapCenterY;
    mCurrentAnimationZ = mStartMapCenterZ;

    // update map's actual position
    mMapCenterX = static_cast<float>(targetX);
    mMapCenterY = static_cast<float>(targetY);
    mMapCenterZ = static_cast<float>(targetZ);

    // Reset animation progress
    mAnimationProgress = 0.0;

    // Set animation flag to prevent interference
    mCameraSmoothAnimating = true;

    // Start animation timer
    mCameraAnimationTimer->start();
}

void ModernGLWidget::stopSmoothTransition()
{
    mCameraAnimationTimer->stop();
    mCameraSmoothAnimating = false;
}

void ModernGLWidget::onCameraAnimationTick()
{
    // Update animation progress
    mAnimationProgress += static_cast<qreal>(mCameraAnimationTimer->interval()) / mAnimationDuration;


    if (mAnimationProgress >= 1.0) {
        // Animation complete - set final position and stop
        mAnimationProgress = 1.0;
        mCameraAnimationTimer->stop();

        mAID = mTargetAID;
        mCameraController.setTarget(static_cast<int>(mTargetMapCenterX), static_cast<int>(mTargetMapCenterY), static_cast<int>(mTargetMapCenterZ));

        // Set final floating-point position
        mCurrentAnimationX = mTargetMapCenterX;
        mCurrentAnimationY = mTargetMapCenterY;
        mCurrentAnimationZ = mTargetMapCenterZ;

        // Clear animation flag to resume normal camera tracking
        mCameraSmoothAnimating = false;

    } else {
        // Interpolate between start and target positions using floating-point
        qreal easedProgress = mEasingCurve.valueForProgress(mAnimationProgress);

        mCurrentAnimationX = mStartMapCenterX + (mTargetMapCenterX - mStartMapCenterX) * easedProgress;
        mCurrentAnimationY = mStartMapCenterY + (mTargetMapCenterY - mStartMapCenterY) * easedProgress;
        mCurrentAnimationZ = mStartMapCenterZ + (mTargetMapCenterZ - mStartMapCenterZ) * easedProgress;
    }

    // Update camera controller with current floating-point position
    mCameraController.setTarget(mCurrentAnimationX, mCurrentAnimationY, mCurrentAnimationZ);

    // Trigger a repaint
    update();
}
