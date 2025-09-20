/***************************************************************************
 *   Copyright (C) 2008-2013 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2014, 2016, 2019-2021, 2023 by Stephen Lyons            *
 *                                               - slysven@virginmedia.com *
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

#include "modern_glwidget.h"

#include "Host.h"
#include "TArea.h"
#include "TRoom.h"
#include "TRoomDB.h"
#include "dlgMapper.h"
#include "mudlet.h"

#include "pre_guard.h"
#include <QtEvents>
#include <QDebug>
#include <QPainter>
#include "post_guard.h"


ModernGLWidget::ModernGLWidget(TMap* pMap, Host* pHost, QWidget* parent)
: QOpenGLWidget(parent), mShaderManager(&mResourceManager, this), mVertexBuffer(QOpenGLBuffer::VertexBuffer), mColorBuffer(QOpenGLBuffer::VertexBuffer), mNormalBuffer(QOpenGLBuffer::VertexBuffer), mIndexBuffer(QOpenGLBuffer::IndexBuffer), mInstanceBuffer(QOpenGLBuffer::VertexBuffer), mpMap(pMap), mpHost(pHost)
{
    if (mpHost->mBgColor_2.alpha() < 255) {
        setAttribute(Qt::WA_OpaquePaintEvent, false);
        setAttribute(Qt::WA_AlwaysStackOnTop);
    } else {
        setAttribute(Qt::WA_OpaquePaintEvent);
    }
    
    // Initialize smooth camera animation
    mCameraAnimationTimer = new QTimer(this);
    mCameraAnimationTimer->setInterval(17); // ~60fps updates for smoother animation
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
    mResourceManager.cleanup();
    mRenderCommandQueue.cleanup();
    mGeometryManager.cleanup();
    mShaderManager.cleanup();
    mVertexBuffer.destroy();
    mColorBuffer.destroy();
    mNormalBuffer.destroy();
    mIndexBuffer.destroy();
    mInstanceBuffer.destroy();
    mVAO.destroy();
    doneCurrent();
}

QSize ModernGLWidget::minimumSizeHint() const
{
    return QSize(50, 50);
}

QSize ModernGLWidget::sizeHint() const
{
    return QSize(400, 400);
}

void ModernGLWidget::initializeGL()
{
    initializeOpenGLFunctions();
    
    // Debug: Check which OpenGL profile is being used
    QOpenGLContext* context = QOpenGLContext::currentContext();
    if (context) {
        QSurfaceFormat format = context->format();
        qDebug() << "OpenGL Version:" << format.majorVersion() << "." << format.minorVersion();
        qDebug() << "OpenGL Profile:" << (format.profile() == QSurfaceFormat::CoreProfile ? "Core" : 
                                         format.profile() == QSurfaceFormat::CompatibilityProfile ? "Compatibility" : "NoProfile");
        qDebug() << "Debug Context:" << (format.testOption(QSurfaceFormat::DebugContext) ? "Enabled" : "Disabled");
    }

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
        return;
    }
    
    connect(&mShaderManager, &ShaderManager::shadersReloaded, this, QOverload<>::of(&QWidget::update));

    setupBuffers();
    
    // Initialize geometry manager
    mGeometryManager.initialize();
    
    // Initialize render command queue
    mRenderCommandQueue.initialize();
    
    // Initialize resource manager
    mResourceManager.initialize();
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

    // Create instance buffer for instanced rendering
    mInstanceBuffer.create();
    mResourceManager.onBufferCreated();
    mInstanceBuffer.bind();
    mInstanceBuffer.setUsagePattern(QOpenGLBuffer::DynamicDraw);
    mResourceManager.checkGLError(qsl("Instance buffer creation"));

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
    
    if (!mpMap) {
        return;
    }

    QOpenGLShaderProgram* shaderProgram = mShaderManager.getMainShaderProgram();
    if (!shaderProgram) {
        return;
    }

    glEnable(GL_MULTISAMPLE);
    
    float px, py, pz;
    if (mRID != mpMap->mRoomIdHash.value(mpMap->mProfileName) && mShiftMode) {
        mShiftMode = false;
    }

    int ox, oy, oz;
    if (!mShiftMode) {
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
                    message = tr("You have a map loaded (%n room(s)), but Mudlet does not know where you are at the moment.", "", mpMap->mpRoomDB->size());
                }
            } else {
                message = tr("You do not have a map yet - load one, or start mapping from scratch to begin.");
            }
            painter.drawText(0, 0, (width() - 1), (height() - 1), Qt::AlignCenter | Qt::TextWordWrap, message);
            painter.end();

            return;
        }
        
        // Check if room ID changed and determine transition type
        if (mRID != mPreviousRID) {
            // Room changed - check if area also changed
            int targetAID = pRID->getArea();
            int targetX = pRID->x();
            int targetY = pRID->y();
            int targetZ = pRID->z();

            if (targetAID != mPreviousAID) {
                // Area changed - instant transition
                mCameraController.setTarget(static_cast<float>(targetX), static_cast<float>(targetY), static_cast<float>(targetZ));
                // Stop any ongoing smooth animation
                if (mCameraSmoothAnimating) {
                    mCameraAnimationTimer->stop();
                    mCameraSmoothAnimating = false;
                }
            } else {
                // Same area - smooth transition
                startSmoothTransition(targetAID, targetX, targetY, targetZ);
            }
        }
        // Instant update map (smooth transition only impacts camera position)
        mAID = pRID->getArea();
        ox = pRID->x();
        oy = pRID->y();
        oz = pRID->z();
        mMapCenterX = ox;
        mMapCenterY = oy;
        mMapCenterZ = oz;
        mPreviousRID = mRID; // Update tracking
        mPreviousAID = mAID; // Update area tracking


    } else {
        ox = mMapCenterX;
        oy = mMapCenterY;
        oz = mMapCenterZ;
    }

    px = static_cast<float>(ox);
    py = static_cast<float>(oy);
    pz = static_cast<float>(oz);

    TArea* pArea = mpMap->mpRoomDB->getArea(mAID);
    if (!pArea) {
        return;
    }

    if (pArea->gridMode) {
        mCameraController.setGridMode(true);
    }

    zmax = static_cast<float>(pArea->max_z);
    zmin = static_cast<float>(pArea->min_z);

    // Clear the screen
    const QColor color(mpHost->mBgColor_2);
    glClearColor(color.redF(), color.greenF(), color.blueF(), color.alphaF());
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Update transformation matrices
    updateMatrices();

    // Use our shader program
    shaderProgram->bind();

    // Enable room flattening
    zFlattening = 8.0f;

    // Build up render commands - render connections first so rooms appear above them
    renderConnections();
    renderRooms();

    // Execute all queued commands
    mRenderCommandQueue.executeAll(shaderProgram, &mGeometryManager, &mResourceManager, mVAO, mVertexBuffer, mColorBuffer, mNormalBuffer, mIndexBuffer);

    shaderProgram->release();
    
    // Draw label to identify this as the modern OpenGL implementation
    QPainter painter(this);
    painter.setPen(QPen(QColor(255, 255, 255, 200))); // Semi-transparent white
    painter.setFont(QFont("Arial", 12, QFont::Bold));
    painter.drawText(10, height() - 20, "Modern OpenGL Mapper");
    
    // Draw map info using contributor manager
    QColor infoColor;
    if (mpHost->mBgColor_2.lightness() > 127) {
        infoColor = QColor(Qt::black);
    } else {
        infoColor = QColor(Qt::white);
    }
    dlgMapper::paintMapInfo(mFrameTimer, painter, mpHost, mpMap, 
                           mRID, mAID, 0, infoColor, 10, 10, width(), mFontHeight);
    
    painter.end();
    
    // Display instant frame time
    qint64 frameTime = mFrameTimer.elapsed();
    qDebug() << "[Modern GLWidget] Frame time:" << frameTime << "ms";
}

void ModernGLWidget::renderRooms()
{
    if (!mpMap || !mpMap->mpRoomDB) {
        qDebug() << "ModernGLWidget: No map or room database";
        return;
    }

    TArea* pArea = mpMap->mpRoomDB->getArea(mAID);
    if (!pArea) {
        qDebug() << "ModernGLWidget: No area found for ID:" << mAID;
        return;
    }

    // Always enable depth testing
    auto enableDepthCommand = std::make_unique<GLStateCommand>(GLStateCommand::ENABLE_DEPTH_TEST);
    mRenderCommandQueue.addCommand(std::move(enableDepthCommand));

    float pz = static_cast<float>(mMapCenterZ);
    float px = static_cast<float>(mMapCenterX);
    float py = static_cast<float>(mMapCenterY);

    // Batched instance data for instanced rendering
    QVector<CubeInstanceData> mainRoomInstances;
    QVector<CubeInstanceData> currentRoomInstances;
    QVector<CubeInstanceData> targetRoomInstances;
    QVector<CubeInstanceData> overlayInstances;

    QSetIterator<int> itRoom(pArea->getAreaRooms());
    while (itRoom.hasNext()) {
        int currentRoomId = itRoom.next();
        TRoom* pR = mpMap->mpRoomDB->getRoom(currentRoomId);
        if (!pR) {
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

        // Check special room states
        bool isCurrentRoom = (rz == pz) && (rx == px) && (ry == py);
        bool isTargetRoom = (currentRoomId == mTargetRoomId);
        bool belowOrAtLevel = (rz <= pz);
        float roomAlpha = 1.0f;
        const float defaultSize = 1.0f / scale;

        // 1. Collect main room cube data
        if (isCurrentRoom) {
            // Current room: red
            QMatrix4x4 transform = QMatrix4x4();
            transform.translate(rx, ry, rz);
            transform.scale(1.0f/scale, 1.0f/scale, 1.0f/scale/zFlattening);
            currentRoomInstances.append(CubeInstanceData(transform, 1.0f, 0.0f, 0.0f, 1.0f));
        } else if (isTargetRoom) {
            // Target room: green
            QMatrix4x4 transform = QMatrix4x4();
            transform.translate(rx, ry, rz);
            transform.scale(1.0f/scale, 1.0f/scale, 1.0f/scale/zFlattening);
            targetRoomInstances.append(CubeInstanceData(transform, 0.0f, 1.0f, 0.0f, 1.0f));
        } else {
            // Normal room: use planeColor logic based on z-level relationship
            QColor roomColor = getPlaneColor(static_cast<int>(rz), belowOrAtLevel);
            float redComponent = roomColor.redF();
            float greenComponent = roomColor.greenF(); 
            float blueComponent = roomColor.blueF();
            float roomAlpha = 1.0f;
            
            // Check for more-transparent experiment
            if (mpHost->experimentEnabled("experiment.rendering.more-transparent")) {
                static bool debugOnce = false;
                if (!debugOnce) {
                    qDebug() << "[Experiment Debug] More-transparent experiment is ENABLED - SLIDING DARKNESS TEST";
                    debugOnce = true;
                }
                // EXPERIMENT: Apply sliding darkness based on level distance
                int levelDistance = abs(static_cast<int>(rz - pz));
                float darknessFactor = 1.0f; // Default: no darkness
                
                if (levelDistance == 1) {
                    darknessFactor = 0.5f; // 50% darker
                } else if (levelDistance == 2) {
                    darknessFactor = 0.2f; // 80% darker
                } else if (levelDistance > 2) {
                    darknessFactor = 0.05f; // 95% darker
                }
                
                // Apply darkness to color components, keep full opacity
                redComponent *= darknessFactor;
                greenComponent *= darknessFactor;
                blueComponent *= darknessFactor;
                roomAlpha = 1.0f; // Full opacity
                
                if (levelDistance > 0) {
                    qDebug() << "[Sliding Darkness] Room Z:" << rz << "Player Z:" << pz 
                             << "Distance:" << levelDistance << "Darkness factor:" << darknessFactor;
                }
            } else {
                // Original rendering: rooms above are dark and transparent
                roomAlpha = belowOrAtLevel ? 1.0f : 0.2f; // 80% transparent (20% opacity) if above current level
                
                if (!belowOrAtLevel) {
                    // Drastically reduce brightness for rooms above current level - match old widget appearance
                    const float darkenFactor = 0.25f; // Keep only 25% of original brightness
                    redComponent *= darkenFactor;
                    greenComponent *= darkenFactor;
                    blueComponent *= darkenFactor;
                }
            }
            
            QMatrix4x4 transform = QMatrix4x4();
            transform.translate(rx, ry, rz);
            transform.scale(1.0f/scale, 1.0f/scale, 1.0f/scale/zFlattening);
            mainRoomInstances.append(CubeInstanceData(transform, redComponent, greenComponent, blueComponent, roomAlpha));
        }

        // 2. Collect environment color overlay data
        QColor envColor = getEnvironmentColor(pR);
        float overlayZ = rz + 0.25f/zFlattening; // Slightly above the main cube
        float envRed = envColor.redF();
        float envGreen = envColor.greenF();
        float envBlue = envColor.blueF();
        float overlayAlpha = 0.8f; // Default overlay transparency
        
        // Apply same sliding darkness to environment overlay
        if (mpHost->experimentEnabled("experiment.rendering.more-transparent")) {
            // EXPERIMENT: Apply same darkness calculation to environment overlay
            int levelDistance = abs(static_cast<int>(rz - pz));
            float darknessFactor = 1.0f; // Default: no darkness
            
            if (levelDistance == 1) {
                darknessFactor = 0.5f; // 50% darker
            } else if (levelDistance == 2) {
                darknessFactor = 0.2f; // 80% darker
            } else if (levelDistance > 2) {
                darknessFactor = 0.05f; // 95% darker
            }
            
            // Apply darkness to environment overlay colors, keep normal alpha
            envRed *= darknessFactor;
            envGreen *= darknessFactor;
            envBlue *= darknessFactor;
            overlayAlpha = 0.8f; // Normal overlay alpha
        } else {
            // Original rendering: darken overlays above player level
            overlayAlpha = belowOrAtLevel ? 0.8f : 0.16f; // 84% transparent if above current level (0.2 * 0.8)
            
            if (!belowOrAtLevel) {
                // Drastically reduce brightness for environment overlays above current level
                const float darkenFactor = 0.25f; // Keep only 25% of original brightness
                envRed *= darkenFactor;
                envGreen *= darkenFactor;
                envBlue *= darkenFactor;
            }
        }
        
        QMatrix4x4 transform = QMatrix4x4();
        transform.translate(rx, ry, overlayZ);
        transform.scale(0.75f / scale, 0.75f / scale, 1.0f / scale / zFlattening);
        overlayInstances.append(CubeInstanceData(transform, envRed, envGreen, envBlue, overlayAlpha));

        // 3. Render up/down exit indicators on the overlay (keep individual rendering for now)
        renderUpDownIndicators(pR, rx, ry, overlayZ + (1.0f / scale / zFlattening) + 0.1f/zFlattening);
        
        // 4. Render in/out exit indicators on the overlay (keep individual rendering for now)
        renderInOutIndicators(pR, rx, ry, overlayZ + (1.0f / scale / zFlattening) + 0.1f/zFlattening);
    }

    // Create instanced render commands for each batch
    if (!mainRoomInstances.isEmpty()) {
        auto command = std::make_unique<RenderInstancedCubesCommand>(mainRoomInstances, 
                                                                    mCameraController.getProjectionMatrix(), 
                                                                    mCameraController.getViewMatrix(), 
                                                                    mCameraController.getModelMatrix());
        mRenderCommandQueue.addCommand(std::move(command));
    }

    if (!currentRoomInstances.isEmpty()) {
        auto command = std::make_unique<RenderInstancedCubesCommand>(currentRoomInstances, 
                                                                    mCameraController.getProjectionMatrix(), 
                                                                    mCameraController.getViewMatrix(), 
                                                                    mCameraController.getModelMatrix());
        mRenderCommandQueue.addCommand(std::move(command));
    }

    if (!targetRoomInstances.isEmpty()) {
        auto command = std::make_unique<RenderInstancedCubesCommand>(targetRoomInstances, 
                                                                    mCameraController.getProjectionMatrix(), 
                                                                    mCameraController.getViewMatrix(), 
                                                                    mCameraController.getModelMatrix());
        mRenderCommandQueue.addCommand(std::move(command));
    }
    
    if (!overlayInstances.isEmpty()) {
        // Keep depth testing enabled for overlays (was conditional with experiment.always-depth-test)

        auto command = std::make_unique<RenderInstancedCubesCommand>(overlayInstances, 
                                                                    mCameraController.getProjectionMatrix(), 
                                                                    mCameraController.getViewMatrix(), 
                                                                    mCameraController.getModelMatrix());
        mRenderCommandQueue.addCommand(std::move(command));

        // Re-enable depth testing for subsequent rendering
        auto enableDepthCommand = std::make_unique<GLStateCommand>(GLStateCommand::ENABLE_DEPTH_TEST);
        mRenderCommandQueue.addCommand(std::move(enableDepthCommand));
    }
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

    // Initialize instance queue
    QVector<CubeInstanceData> areaExitInstances;

    // Collect all lines to draw
    QVector<float> lineVertices;
    QVector<float> lineColors;

    QSetIterator<int> itRoom(pArea->getAreaRooms());
    while (itRoom.hasNext()) {
        TRoom* pR = mpMap->mpRoomDB->getRoom(itRoom.next());
        if (!pR) {
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

        // Check if this is the current room
        bool isCurrentRoom = (rz == pz) && (rx == static_cast<float>(mMapCenterX)) && (ry == static_cast<float>(mMapCenterY));

        // Color for connections: red if current room, gray otherwise
        float r, g, b;
        if (isCurrentRoom) {
            r = 1.0f;
            g = 0.0f;
            b = 0.0f; // Red
        } else {
            r = 0.3f;
            g = 0.3f;
            b = 0.3f; // Gray
        }

        for (int i = 0; i < exitList.size(); ++i) {
            int k = exitList[i];
            if (k == -1) {
                continue;
            }

            TRoom* pExit = mpMap->mpRoomDB->getRoom(k);
            if (!pExit) {
                continue;
            }

            bool areaExit = (pExit->getArea() != mAID);
            bool inOut = ((i == 10 || i == 11) && mpHost && mpHost->experimentEnabled("experiment.render-in-out-exits"));

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
                const QVector3D exitVector = QVector3D(ex-rx, ey-ry, ez-rz);
                const QQuaternion alignmentQuat = QQuaternion::rotationTo(zVector, exitVector);
                QMatrix4x4 transform = QMatrix4x4();
                if (inOut) {
                    transform.translate(3.0f*exitVector/8.0f);
                    transform.translate(rx, ry, rz);
                    transform.rotate(alignmentQuat);
                    transform.scale(0.02f, 0.02f, exitVector.length()/16.0f);
                    roomConnectionInstances.append(CubeInstanceData(transform, r, g, b, connectionAlpha));
                } else {
                    transform.translate(exitVector/4.0f);
                    transform.translate(rx, ry, rz);
                    transform.rotate(alignmentQuat);
                    transform.scale(0.02f, 0.02f, exitVector.length()/4.0f);
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
                const QVector3D exitVector = QVector3D(dx-rx, dy-ry, dz-rz);
                const QQuaternion alignmentQuat = QQuaternion::rotationTo(zVector, exitVector);
                // double length of normal exits since this exit is one sided
                QMatrix4x4 transform = QMatrix4x4();
                if (inOut) {
                    transform.translate(3.0f*exitVector/8.0f);
                    transform.translate(rx, ry, rz);
                    transform.rotate(alignmentQuat);
                    transform.scale(0.02f, 0.02f, exitVector.length()/16.0f);
                    roomConnectionInstances.append(CubeInstanceData(transform, exitRed, exitGreen, exitBlue, exitAlpha));
                    transform.setToIdentity();
                    transform.translate(5.0f*exitVector/8.0f);
                    transform.translate(rx, ry, rz);
                    transform.rotate(alignmentQuat);
                    transform.scale(0.02f, 0.02f, exitVector.length()/16.0f);
                    roomConnectionInstances.append(CubeInstanceData(transform, exitRed, exitGreen, exitBlue, exitAlpha));
                } else {
                    transform.translate(exitVector/2.0f);
                    transform.translate(rx, ry, rz);
                    transform.rotate(alignmentQuat);
                    transform.scale(0.02f, 0.02f, exitVector.length()/2.0f);
                    roomConnectionInstances.append(CubeInstanceData(transform, exitRed, exitGreen, exitBlue, exitAlpha));
                }

                // Render green area exit cube at the destination position with translucency and darkening
                transform.setToIdentity();
                transform.translate(dx, dy, dz);
                transform.scale(1.0f/scale, 1.0f/scale, 1.0f/scale/zFlattening);
                areaExitInstances.append(CubeInstanceData(transform, exitRed, exitGreen, exitBlue, exitAlpha));

                // Render smaller environment overlay rectangle on top with translucency and darkening
                QColor envColor = getEnvironmentColor(pExit);
                float overlayZ = dz + 0.25f/zFlattening;
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
                transform.scale(0.5f/scale, 0.5f/scale, 1.0f/scale/zFlattening);
                areaExitInstances.append(CubeInstanceData(transform, exitEnvRed, exitEnvGreen, exitEnvBlue, overlayAlpha));
            }
        }
    }

    // Always enable depth testing
    auto enableDepthCommand = std::make_unique<GLStateCommand>(GLStateCommand::ENABLE_DEPTH_TEST);
    mRenderCommandQueue.addCommand(std::move(enableDepthCommand));

    // Always render room connection volumes
    if (!roomConnectionInstances.isEmpty()) {
        auto command = std::make_unique<RenderInstancedCubesCommand>(roomConnectionInstances, 
                                                                    mCameraController.getProjectionMatrix(), 
                                                                    mCameraController.getViewMatrix(), 
                                                                    mCameraController.getModelMatrix());
        mRenderCommandQueue.addCommand(std::move(command));

    } else {
        // Render all collected lines
        if (!lineVertices.isEmpty()) {
            renderLines(lineVertices, lineColors);
        }
    }

    if (!areaExitInstances.isEmpty()) {
        auto command = std::make_unique<RenderInstancedCubesCommand>(areaExitInstances, 
                                                                    mCameraController.getProjectionMatrix(), 
                                                                    mCameraController.getViewMatrix(), 
                                                                    mCameraController.getModelMatrix());
        mRenderCommandQueue.addCommand(std::move(command));
    }

}

void ModernGLWidget::renderCube(float x, float y, float z, float size, float r, float g, float b, float a)
{
    // Create render command and queue it
    auto command = std::make_unique<RenderCubeCommand>(x, y, z, size, r, g, b, a,
                                                      mCameraController.getProjectionMatrix(), 
                                                      mCameraController.getViewMatrix(), 
                                                      mCameraController.getModelMatrix());
    mRenderCommandQueue.addCommand(std::move(command));
}

void ModernGLWidget::shiftCamera(float verticalAngle, float horizontalAngle, float rotationAngle)
{
    mCameraController.shiftPerspective(verticalAngle, horizontalAngle, rotationAngle);
    update();
}

void ModernGLWidget::setCameraPosition(float r, float theta, float phi)
{
    mCameraController.setPosition(r, theta, phi);
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
    mCameraController.translateTargetUp();
    update();
}

void ModernGLWidget::slot_shiftZdown()
{
    mShiftMode = true;
    mCameraController.translateTargetDown();
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
    update();
}

void ModernGLWidget::slot_sideView()
{
    mCameraController.setSideView();
    is2DView = false;
    update();
}

void ModernGLWidget::slot_topView()
{
    mCameraController.setTopView();
    is2DView = true;
    update();
}

void ModernGLWidget::slot_setScale(int angle)
{
    float scale = 150 / (static_cast<float>(angle) + 300.0f);
    mCameraController.setScale(scale);
    update();
}

void ModernGLWidget::slot_setCameraPositionX(int angle)
{
    angle *= 10;
    QVector3D currentPosition = mCameraController.getPosition();
    mCameraController.setPosition(currentPosition[0], currentPosition[1], angle);
    is2DView = false;
    update();
}

void ModernGLWidget::slot_setCameraPositionY(int angle)
{
    angle *= 10;
    QVector3D currentPosition = mCameraController.getPosition();
    mCameraController.setPosition(currentPosition[0], currentPosition[1], angle);
    is2DView = false;
    update();
}

void ModernGLWidget::slot_setCameraPositionZ(int angle)
{
    angle *= 10;
    angle = qBound(0, angle, 180);
    QVector3D currentPosition = mCameraController.getPosition();
    mCameraController.setPosition(currentPosition[0], angle, currentPosition[2]);
    is2DView = false;
    update();
}

void ModernGLWidget::slot_shiftCameraDown()
{
    const float angle = 3.0f;
    QVector3D currentPosition = mCameraController.getPosition();
    mCameraController.setPosition(currentPosition[0], currentPosition[1]+angle, currentPosition[2]);
    is2DView = false;
    update();
}

void ModernGLWidget::slot_shiftCameraUp()
{
    const float angle = 3.0f;
    QVector3D currentPosition = mCameraController.getPosition();
    mCameraController.setPosition(currentPosition[0], currentPosition[1]-angle, currentPosition[2]);
    is2DView = false;
    update();
}

void ModernGLWidget::slot_shiftCameraLeft()
{
    const float angle = 3.0f;
    QVector3D currentPosition = mCameraController.getPosition();
    mCameraController.setPosition(currentPosition[0], currentPosition[1], currentPosition[2]-angle);
    is2DView = false;
    update();
}

void ModernGLWidget::slot_shiftCameraRight()
{
    const float angle = 3.0f;
    QVector3D currentPosition = mCameraController.getPosition();
    mCameraController.setPosition(currentPosition[0], currentPosition[1], currentPosition[2]+angle);
    is2DView = false;
    update();
}

void ModernGLWidget::setViewCenter(int areaId, int xPos, int yPos, int zPos)
{
    mShiftMode = true;
    
    // Use smooth transition
    startSmoothTransition(areaId, xPos, yPos, zPos);
}

void ModernGLWidget::wheelEvent(QWheelEvent* e)
{
    // Implement wheel event handling similar to original
    const int delta = e->angleDelta().y();
    float currentScale = mCameraController.getScale();
    if (delta > 0) {
        currentScale *= 1.1f;
    } else {
        currentScale *= 0.9f;
    }
    mCameraController.setScale(currentScale);
    update();
}

void ModernGLWidget::mousePressEvent(QMouseEvent* event)
{
    // Implement mouse handling (placeholder)
    mudlet::self()->activateProfile(mpHost);
    if (!mpMap||!mpMap->mpRoomDB) {
        return;
    }
    if (event->buttons() & Qt::LeftButton) {        // translation on xy-plane
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
    if (!mpMap||!mpMap->mpRoomDB) {
        return;
    }
    if (mPanMode) {
        auto eventPos = event->position();
        auto x = static_cast<float>(eventPos.x());
        auto y = static_cast<float>(height()) - static_cast<float>(eventPos.y()); // the opengl origin is at bottom left
        if ((mPanXStart - x) > 1.0f) {
            if (event->modifiers() & Qt::ControlModifier) {
                slot_shiftCameraRight();
            } else {
                slot_shiftLeft();
            }
            mPanXStart = x;
        } else if ((mPanXStart - x) < -1.0f) {
            if (event->modifiers() & Qt::ControlModifier) {
                slot_shiftCameraLeft();
            } else {
                slot_shiftRight();
            }
            mPanXStart = x;
        }
        if ((mPanYStart - y) > 1.0f) {
            if (event->modifiers() & Qt::ControlModifier) {
                slot_shiftCameraUp();
            } else {
                slot_shiftUp();
            }
            mPanYStart = y;
        } else if ((mPanYStart - y) < -1.0f) {
            if (event->modifiers() & Qt::ControlModifier) {
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
    mMapCenterX = newCenter.x();
    mMapCenterY = newCenter.y();
    mMapCenterZ = newCenter.z();
    QOpenGLWidget::mouseReleaseEvent(event);
    update();
}

void ModernGLWidget::renderLines(const QVector<float>& vertices, const QVector<float>& colors)
{
    if (vertices.isEmpty() || colors.isEmpty()) {
        return;
    }
    
    // Create render command and queue it
    auto command = std::make_unique<RenderLinesCommand>(vertices, colors, 
                                                       mCameraController.getProjectionMatrix(), 
                                                       mCameraController.getViewMatrix(), 
                                                       mCameraController.getModelMatrix());
    mRenderCommandQueue.addCommand(std::move(command));
}

void ModernGLWidget::renderTriangles(const QVector<float>& vertices, const QVector<float>& colors)
{
    if (vertices.isEmpty() || colors.isEmpty()) {
        return;
    }
    
    // Create render command and queue it
    auto command = std::make_unique<RenderTrianglesCommand>(vertices, colors, 
                                                           mCameraController.getProjectionMatrix(), 
                                                           mCameraController.getViewMatrix(), 
                                                           mCameraController.getModelMatrix());
    mRenderCommandQueue.addCommand(std::move(command));
}

void ModernGLWidget::renderUpDownIndicators(TRoom* pRoom, float x, float y, float z)
{
    if (!pRoom) {
        return;
    }

    QVector<float> triangleVertices;
    QVector<float> triangleColors;

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

    // Render the triangles if we have any
    if (!triangleVertices.isEmpty()) {
        renderTriangles(triangleVertices, triangleColors);
    }
}

void ModernGLWidget::renderInOutIndicators(TRoom* pRoom, float x, float y, float z)
{
    if (!pRoom || !(mpHost && mpHost->experimentEnabled("experiment.render-in-out-exits"))) {
        return;
    }

    QVector<float> triangleVertices;
    QVector<float> triangleColors;

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
        triangleVertices << x << y << z;              // Mid-center
        // triangle pointing inwards: top-right, bottom-right, mid-center
        triangleVertices << (x + width) << (y + halfHeight) << z; // Top-right
        triangleVertices << (x + width) << (y - halfHeight) << z; // Bottom-right
        triangleVertices << x << y << z;              // Mid-center

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
        triangleVertices << x - 1.0f/scale << y << z;              // Outside-center
        // triangle pointing outwards: top-right, bottom-right, out-center
        triangleVertices << (x + width) << (y + halfHeight) << z; // Top-right
        triangleVertices << (x + width) << (y - halfHeight) << z; // Bottom-right
        triangleVertices << x + 1.0f/scale << y << z;              // Outside-center

        // Add gray color for all six vertices
        for (int i = 0; i < 6; ++i) {
            triangleColors << gray[0] << gray[1] << gray[2] << gray[3];
        }
    }

    // Render the triangles if we have any
    if (!triangleVertices.isEmpty()) {
        renderTriangles(triangleVertices, triangleColors);
    }
}

QColor ModernGLWidget::getPlaneColor(int zLevel, bool belowOrAtLevel)
{
    // Both color arrays from original glwidget.cpp
    static const float planeColor[][4] = {{0.5f, 0.6f, 0.5f, 0.2f},
                                          {0.233f, 0.498f, 0.113f, 0.2f},
                                          {0.666f, 0.333f, 0.498f, 0.2f},
                                          {0.5f, 0.333f, 0.666f, 0.2f},
                                          {0.69f, 0.458f, 0.0f, 0.2f},
                                          {0.333f, 0.0f, 0.49f, 0.2f},
                                          {133.0f / 255.0f, 65.0f / 255.0f, 98.0f / 255.0f, 0.2f},
                                          {0.3f, 0.3f, 0.0f, 0.2f},
                                          {0.6f, 0.2f, 0.6f, 0.2f},
                                          {0.6f, 0.6f, 0.2f, 0.2f},
                                          {0.4f, 0.1f, 0.4f, 0.2f},
                                          {0.4f, 0.4f, 0.1f, 0.2f},
                                          {0.3f, 0.1f, 0.3f, 0.2f},
                                          {0.3f, 0.3f, 0.1f, 0.2f},
                                          {0.2f, 0.1f, 0.2f, 0.2f},
                                          {0.2f, 0.2f, 0.1f, 0.2f},
                                          {0.24f, 0.1f, 0.5f, 0.2f},
                                          {0.1f, 0.1f, 0.0f, 0.2f},
                                          {0.54f, 0.6f, 0.2f, 0.2f},
                                          {0.2f, 0.2f, 0.5f, 0.2f},
                                          {0.6f, 0.6f, 0.2f, 0.2f},
                                          {0.6f, 0.4f, 0.6f, 0.2f},
                                          {0.4f, 0.4f, 0.1f, 0.2f},
                                          {0.4f, 0.2f, 0.4f, 0.2f},
                                          {0.2f, 0.2f, 0.0f, 0.2f},
                                          {0.2f, 0.1f, 0.3f, 0.2f}};

    static const float planeColor2[][4] = {{0.9f, 0.5f, 0.0f, 1.0f},
                                           {165.0f / 255.0f, 102.0f / 255.0f, 167.0f / 255.0f, 1.0f},
                                           {170.0f / 255.0f, 10.0f / 255.0f, 127.0f / 255.0f, 1.0f},
                                           {203.0f / 255.0f, 135.0f / 255.0f, 101.0f / 255.0f, 1.0f},
                                           {154.0f / 255.0f, 154.0f / 255.0f, 115.0f / 255.0f, 1.0f},
                                           {107.0f / 255.0f, 154.0f / 255.0f, 100.0f / 255.0f, 1.0f},
                                           {154.0f / 255.0f, 184.0f / 255.0f, 111.0f / 255.0f, 1.0f},
                                           {67.0f / 255.0f, 154.0f / 255.0f, 148.0f / 255.0f, 1.0f},
                                           {154.0f / 255.0f, 118.0f / 255.0f, 151.0f / 255.0f, 1.0f},
                                           {208.0f / 255.0f, 213.0f / 255.0f, 164.0f / 255.0f, 1.0f},
                                           {213.0f / 255.0f, 169.0f / 255.0f, 158.0f / 255.0f, 1.0f},
                                           {139.0f / 255.0f, 209.0f / 255.0f, 0.0f, 1.0f},
                                           {163.0f / 255.0f, 209.0f / 255.0f, 202.0f / 255.0f, 1.0f},
                                           {158.0f / 255.0f, 156.0f / 255.0f, 209.0f / 255.0f, 1.0f},
                                           {209.0f / 255.0f, 144.0f / 255.0f, 162.0f / 255.0f, 1.0f},
                                           {209.0f / 255.0f, 183.0f / 255.0f, 78.0f / 255.0f, 1.0f},
                                           {111.0f / 255.0f, 209.0f / 255.0f, 88.0f / 255.0f, 1.0f},
                                           {95.0f / 255.0f, 120.0f / 255.0f, 209.0f / 255.0f, 1.0f},
                                           {31.0f / 255.0f, 209.0f / 255.0f, 126.0f / 255.0f, 1.0f},
                                           {1.0f, 170.0f / 255.0f, 1.0f, 1.0f},
                                           {158.0f / 255.0f, 105.0f / 255.0f, 158.0f / 255.0f, 1.0f},
                                           {68.0f / 255.0f, 189.0f / 255.0f, 189.0f / 255.0f, 1.0f},
                                           {0.1f, 0.69f, 0.49f, 1.0f},
                                           {0.0f, 0.15f, 1.0f, 1.0f},
                                           {0.12f, 0.02f, 0.20f, 1.0f},
                                           {0.0f, 0.3f, 0.1f, 1.0f}};

    int ef = abs(zLevel % 26);
    const float* color;

    // Original logic from line 1382 and 1389:
    // rz <= pz: glColor4f(planeColor[ef]) - use planeColor for rooms below/at level
    // rz > pz:  glColor4f(planeColor2[ef]) - use planeColor2 for rooms above level
    // Try using the brighter array as default since most rooms are likely at the same level
    if (belowOrAtLevel) {
        color = planeColor2[ef]; // Use bright colors for rooms at/below level
        // qDebug() << "Using planeColor2[" << ef << "] for room at/below level";
    } else {
        color = planeColor[ef]; // Use darker colors for rooms above level
        // qDebug() << "Using planeColor[" << ef << "] for room above level";
    }

    return QColor(static_cast<int>(color[0] * 255),
                  static_cast<int>(color[1] * 255),
                  static_cast<int>(color[2] * 255),
                  255); // Use full alpha for room colors
}

QColor ModernGLWidget::getEnvironmentColor(TRoom* pRoom)
{
    if (!pRoom || !mpMap || !mpHost) {
        return QColor(128, 128, 128); // Default gray
    }

    QColor roomColor;
    int roomEnvironment = pRoom->environment;

    // Same logic as T2DMap.cpp
    if (mpMap->mEnvColors.contains(roomEnvironment)) {
        roomEnvironment = mpMap->mEnvColors[roomEnvironment];
    } else {
        if (!mpMap->mCustomEnvColors.contains(roomEnvironment)) {
            roomEnvironment = 1;
        }
    }

    switch (roomEnvironment) {
    case 1:
        roomColor = mpHost->mRed_2;
        break;
    case 2:
        roomColor = mpHost->mGreen_2;
        break;
    case 3:
        roomColor = mpHost->mYellow_2;
        break;
    case 4:
        roomColor = mpHost->mBlue_2;
        break;
    case 5:
        roomColor = mpHost->mMagenta_2;
        break;
    case 6:
        roomColor = mpHost->mCyan_2;
        break;
    case 7:
        roomColor = mpHost->mWhite_2;
        break;
    case 8:
        roomColor = mpHost->mBlack_2;
        break;
    case 9:
        roomColor = mpHost->mLightRed_2;
        break;
    case 10:
        roomColor = mpHost->mLightGreen_2;
        break;
    case 11:
        roomColor = mpHost->mLightYellow_2;
        break;
    case 12:
        roomColor = mpHost->mLightBlue_2;
        break;
    case 13:
        roomColor = mpHost->mLightMagenta_2;
        break;
    case 14:
        roomColor = mpHost->mLightCyan_2;
        break;
    case 15:
        roomColor = mpHost->mLightWhite_2;
        break;
    case 16:
        roomColor = mpHost->mLightBlack_2;
        break;
    default: // user defined room color
        if (mpMap->mCustomEnvColors.contains(roomEnvironment)) {
            roomColor = mpMap->mCustomEnvColors[roomEnvironment];
        } else {
            if (16 < roomEnvironment && roomEnvironment < 232) {
                quint8 const base = roomEnvironment - 16;
                quint8 r = base / 36;
                quint8 g = (base - (r * 36)) / 6;
                quint8 b = (base - (r * 36)) - (g * 6);

                r = r == 0 ? 0 : (r - 1) * 40 + 95;
                g = g == 0 ? 0 : (g - 1) * 40 + 95;
                b = b == 0 ? 0 : (b - 1) * 40 + 95;
                roomColor = QColor(r, g, b, 255);
            } else if (231 < roomEnvironment && roomEnvironment < 256) {
                quint8 const k = ((roomEnvironment - 232) * 10) + 8;
                roomColor = QColor(k, k, k, 255);
            } else {
                roomColor = mpHost->mRed_2; // fallback
            }
        }
    }

    return roomColor;
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

