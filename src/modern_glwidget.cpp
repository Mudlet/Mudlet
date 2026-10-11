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

#include "ExitAppearance.h"
#include "ExitScene.h"
#include "Host.h"
#include "RoomAppearance.h"
#include "RoomContextMenuHandler.h"
#include "T2DMap.h"
#include "TArea.h"
#include "TCommandLine.h"
#include "TEvent.h"
#include "TMainConsole.h"
#include "TMap.h"
#include "TRoom.h"
#include "TRoomDB.h"
#include "dlgMapper.h"
#include "mudlet.h"

#include <QtEvents>
#include <QApplication>
#include <QDebug>
#include <QFontMetricsF>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLinearGradient>
#include <QMenu>
#include <QPainter>
#include <QKeyEvent>
#include <QSurfaceFormat>
#include <QToolTip>
#include <QVBoxLayout>
#include <algorithm>
#include <chrono>
#include <limits>
#include <optional>
#include <utility>

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

    // A click takes focus only so that a floating view can pass typed keys on to the command line; focus from
    // Tab also lets the view pan and zoom with keys
    setFocusPolicy(Qt::StrongFocus);
    loadSavedCameras();
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
        if (isVisible() && !context() && !size().isEmpty()) {
            // Qt drops a context not yet composed when the view is shown (switching renderers does this) and rebuilds it
            // only on a later resize or paint; resize synchronously so the check below sees the result, which update() would not
            QResizeEvent resize(size(), size());
            QCoreApplication::sendEvent(this, &resize);
        }
        if (isVisible() && !isValid()) {
            reportContextFailure();
        }
    });
}

void ModernGLWidget::hideEvent(QHideEvent* event)
{
    endEditGesture();
    saveCamera();
    QOpenGLWidget::hideEvent(event);
}

namespace {
// Per profile: for each area by ID, how the 3D view last looked at it
constexpr QLatin1StringView scmSavedCamerasItem{"3dmapper_cameras"};
} // namespace

void ModernGLWidget::saveCamera()
{
    // While a frame is pending the camera is still the one set up for the area shown before
    // A floating view is hidden while its dock is torn down, after the Host and its TMap are freed
    // A map window's view would overwrite the cameras the mapper's view keeps for the profile
    if (mSecondaryView || !mpHost || !mAID || mFramePending || !mpMap || !mpMap->mpRoomDB || !mpMap->mpRoomDB->getArea(mAID)) {
        return;
    }
    const QVector3D position = mCameraController.getPosition();
    const SavedCamera camera{mCameraController.getScale(), position.y(), position.z(), mCameraController.getRoll()};
    const auto it = mSavedCameras.constFind(mAID);
    if (it != mSavedCameras.cend() && it->distance == camera.distance && it->tilt == camera.tilt && it->azimuth == camera.azimuth && it->roll == camera.roll) {
        return;
    }
    mSavedCameras.insert(mAID, camera);
    writeSavedCameras();
}

bool ModernGLWidget::restoreCamera(const bool distanceOnly)
{
    const auto it = mSavedCameras.constFind(mAID);
    if (it == mSavedCameras.cend()) {
        return false;
    }
    mCameraController.setScale(it->distance);
    if (!distanceOnly) {
        mCameraController.setOrientation(it->tilt, it->azimuth, it->roll);
    }
    emitCameraControls();
    return true;
}

void ModernGLWidget::loadSavedCameras()
{
    if (!mpHost) {
        return;
    }
    const QJsonObject cameras = QJsonDocument::fromJson(mpHost->readProfileData(QString(scmSavedCamerasItem)).toUtf8()).object();
    for (auto it = cameras.constBegin(); it != cameras.constEnd(); ++it) {
        bool isNumber = false;
        const int areaId = it.key().toInt(&isNumber);
        const QJsonArray values = it.value().toArray();
        if (!isNumber || values.size() != 4) {
            continue;
        }
        mSavedCameras.insert(
                areaId,
                {static_cast<float>(values.at(0).toDouble()), static_cast<float>(values.at(1).toDouble()), static_cast<float>(values.at(2).toDouble()), static_cast<float>(values.at(3).toDouble())});
    }
}

void ModernGLWidget::writeSavedCameras() const
{
    if (!mpHost) {
        return;
    }
    QJsonObject cameras;
    for (auto it = mSavedCameras.cbegin(); it != mSavedCameras.cend(); ++it) {
        cameras.insert(QString::number(it.key()), QJsonArray{it->distance, it->tilt, it->azimuth, it->roll});
    }
    mpHost->writeProfileData(QString(scmSavedCamerasItem), QString::fromUtf8(QJsonDocument(cameras).toJson(QJsonDocument::Compact)));
}

// Each event that tries for a context reports a failure straight away, so a script that shows the view and moves
// its camera in one go is refused; showEvent()'s timer is only a fallback, a turn of the event loop later
void ModernGLWidget::reportContextFailure()
{
    if (mFailureMessage.isEmpty()) {
        //: Shown in place of the 3D map when the graphics driver cannot create the OpenGL context the modern 3D mapper needs
        showFailure(tr("The modern 3D mapper could not create an OpenGL 3.3 context on this system."));
    }
}

// QOpenGLWidget retries a context it could not create on every show, resize and paint, and each retry
// leaks the context that failed (QOpenGLWidgetPrivate::initialize() overwrites it), so ask only once
bool ModernGLWidget::contextCreationFailed() const
{
    return context() && !isValid();
}

bool ModernGLWidget::event(QEvent* event)
{
    if (event->type() == QEvent::ToolTip) {
        auto* helpEvent = static_cast<QHelpEvent*>(event);
        const QString text = roomToolTip(roomAt(helpEvent->pos()));
        if (text.isEmpty()) {
            QToolTip::hideText();
            event->ignore();
        } else {
            QToolTip::showText(helpEvent->globalPos(), text, this);
        }
        return true;
    }
    // Of the events this sees, only these try for a context; after some others, such as DeferredDelete, this object is gone
    if (event->type() != QEvent::Show && event->type() != QEvent::WindowChangeInternal) {
        return QOpenGLWidget::event(event);
    }
    if (contextCreationFailed()) {
        return QWidget::event(event);
    }
    const bool handled = QOpenGLWidget::event(event);
    if (contextCreationFailed()) {
        reportContextFailure();
    }
    return handled;
}

void ModernGLWidget::resizeEvent(QResizeEvent* event)
{
    // Framed before the base class, whose resize paints at once. A visible resize settles it, since
    // with no context no paint follows to do it
    if (mFrameWhenShown) {
        frameArea();
        if (isVisible()) {
            mFrameWhenShown = false;
        }
    }
    if (contextCreationFailed()) {
        QWidget::resizeEvent(event);
        return;
    }
    QOpenGLWidget::resizeEvent(event);
    if (contextCreationFailed()) {
        reportContextFailure();
    }
}

void ModernGLWidget::paintEvent(QPaintEvent* event)
{
    if (contextCreationFailed()) {
        return;
    }
    QOpenGLWidget::paintEvent(event);
    if (contextCreationFailed()) {
        reportContextFailure();
    }
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
    //: Second paragraph of the message shown in place of the 3D map when the modern 3D mapper cannot run; keep setConfig("experiment.3dmap.modernmapper", false) and the word true exactly as written, they are Lua code the user types
    mpFailureLabel->setText(qsl("%1\n\n%2")
                                    .arg(message,
                                         tr("Use setConfig(\"experiment.3dmap.modernmapper\", false) to try the classic 3D view; if that stays blank too, use the 2D map. Setting it back to true "
                                            "tries the modern one again.")));
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

    // Not while rooms are dragged, as they follow the pointer across the map and would go along with the view
    if (mShiftMode && !mSecondaryView && !mMovingRooms && (mRID != mpMap->mRoomIdHash.value(mpMap->mProfileName) || mpMap->mNewMove)) {
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
        const QVector3D upBefore = mCameraController.getUpVector();
        const QVector3D positionBefore = mCameraController.getPosition();
        mCameraController.setGridMode(true);
        if (mCameraController.getUpVector() != upBefore || mCameraController.getPosition() != positionBefore) {
            emitCameraControls();
        }
    }

    if (mFramePending && !mDistanceRequested) {
        frameArea();
    }
    mFramePending = false;
    // QOpenGLWidget paints inside a resize sent before the view is marked visible, at a size the
    // layout may change again once the 3D controls appear, so only a visible paint settles the framing
    if (mFrameWhenShown && isVisible()) {
        frameArea();
        mFrameWhenShown = false;
    }
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

    // Labels beneath the rooms come before them without writing depth, so that the translucent rooms
    // of other levels show over them rather than cutting holes in them
    renderConnections();
    renderLabels(false);
    renderRooms();
    renderLabels(true);

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

    if (mRectangleSelecting) {
        painter.fillRect(mSelectionRectangle, QColor(190, 190, 190, 60));
    }

    painter.end();
}

namespace {
// Texels across the textures drawn on room tops
constexpr int scmRoomTextureSize = 128;
// Past this many, textures this frame has not used yet are dropped to make room and remade if needed;
// once the frame itself has used this many, the rest are not drawn
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
QImage clearImage(const QSize& size, QColor edgeColor)
{
    QImage image(size, QImage::Format_ARGB32);
    edgeColor.setAlpha(0);
    image.fill(edgeColor);
    return image;
}

QImage clearRoomTextureImage(const QColor& edgeColor)
{
    return clearImage(QSize(scmRoomTextureSize, scmRoomTextureSize), edgeColor);
}

// Texels across a room, for the text drawn beside or on it at the 2D map's proportions
constexpr int scmTextTexelsPerRoom = 48;

// Names drawn shorter than this on screen are left out, as unreadable
constexpr qreal scmMinimumNamePixels = 7.0;

// Measured as the image text is drawn on will lay it out, whatever the screen's resolution
QFontMetricsF imageFontMetrics(const QFont& font)
{
    static const QImage metricsDevice(1, 1, QImage::Format_ARGB32);
    return QFontMetricsF(font, &metricsDevice);
}

// What sets a room's up/down/in/out markers apart: for each, none, a stub or an exit, and its door
QString exitMarkerKey(const TRoom& room)
{
    QString key;
    for (const int direction : {DIR_UP, DIR_DOWN, DIR_IN, DIR_OUT}) {
        const int state = room.getExit(direction) > 0 ? 2 : (room.exitStubs.contains(direction) ? 1 : 0);
        key.append(QString::number(state));
        if (state) {
            key.append(QString::number(room.doors.value(TRoom::dirCodeToShortString(direction))));
        }
    }
    return key;
}
} // namespace

GLuint ModernGLWidget::cachedRoomTexture(const QString& key)
{
    const GLuint texture = mLabelTextureCache.imageTexture(key);
    if (texture) {
        mRoomTexturesInUse.insert(key);
    }
    return texture;
}

bool ModernGLWidget::roomForAnotherTexture(const QString& key)
{
    if (mLabelTextureCache.makeRoomForImageTexture(scmMaxRoomTextures, mRoomTexturesInUse)) {
        return true;
    }
    if (!mRoomTextureLimitWarned) {
        mRoomTextureLimitWarned = true;
        qWarning().nospace() << "ModernGLWidget: this view needs more than " << scmMaxRoomTextures << " different room symbols, IDs, markers and names, so some are not shown, the first being " << key;
    }
    return false;
}

GLuint ModernGLWidget::addRoomTexture(const QString& key, const QImage& image)
{
    const GLuint texture = mLabelTextureCache.addImageTexture(key, image);
    if (texture) {
        mRoomTexturesInUse.insert(key);
    }
    return texture;
}

GLuint ModernGLWidget::symbolTexture(const QString& symbol, const QColor& color)
{
    const QFont& font = mpMap->mMapSymbolFont;
    const qreal fudgeFactor = mpMap->mMapSymbolFontFudgeFactor;
    const QString key = qsl("symbol|%1|%2|%3|%4").arg(color.name(QColor::HexArgb), font.key(), QString::number(fudgeFactor), symbol);
    if (const GLuint texture = cachedRoomTexture(key)) {
        return texture;
    }
    if (mLabelTextureCache.imageTextureFailed(key) || !roomForAnotherTexture(key)) {
        return 0;
    }

    QImage image = clearRoomTextureImage(color);
    QPainter painter(&image);
    ushort fontSize = 1;
    RoomAppearance::paintSymbol(painter, image.rect(), symbol, color, font, fudgeFactor, fontSize);
    painter.end();
    return addRoomTexture(key, image);
}

GLuint ModernGLWidget::discTexture(const QString& key, const QGradientStops& stops)
{
    if (const GLuint texture = cachedRoomTexture(key)) {
        return texture;
    }
    if (mLabelTextureCache.imageTextureFailed(key) || !roomForAnotherTexture(key)) {
        return 0;
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
    return addRoomTexture(key, image);
}

GLuint ModernGLWidget::squareTexture(const QColor& color)
{
    const QString key = qsl("square|%1").arg(color.name(QColor::HexArgb));
    if (const GLuint texture = cachedRoomTexture(key)) {
        return texture;
    }
    if (mLabelTextureCache.imageTextureFailed(key) || !roomForAnotherTexture(key)) {
        return 0;
    }

    QImage image(scmRoomTextureSize, scmRoomTextureSize, QImage::Format_ARGB32);
    image.fill(color);
    return addRoomTexture(key, image);
}

// As the 2D map shades a selected room: from clear at one corner to blue at the opposite one
GLuint ModernGLWidget::selectionTexture()
{
    const QString key = qsl("selection");
    if (const GLuint texture = cachedRoomTexture(key)) {
        return texture;
    }
    if (mLabelTextureCache.imageTextureFailed(key) || !roomForAnotherTexture(key)) {
        return 0;
    }

    QImage image = clearRoomTextureImage(QColor(Qt::blue));
    QPainter painter(&image);
    QLinearGradient gradient(QPointF(0.0, 0.0), QPointF(scmRoomTextureSize, scmRoomTextureSize));
    QColor clearBlue(Qt::blue);
    clearBlue.setAlpha(0);
    gradient.setColorAt(0.2, clearBlue);
    gradient.setColorAt(1.0, QColor(Qt::blue));
    painter.fillRect(image.rect(), gradient);
    painter.end();
    return addRoomTexture(key, image);
}

GLuint ModernGLWidget::exitMarkerTexture(const TRoom& room, const QColor& roomColor)
{
    const QString markers = exitMarkerKey(room);
    if (markers.count(QLatin1Char('0')) == markers.size()) {
        return 0;
    }
    const QColor markerColor = ExitAppearance::contrastColor(roomColor);
    const QString key = qsl("markers|%1|%2").arg(markerColor.name(), markers);
    if (const GLuint texture = cachedRoomTexture(key)) {
        return texture;
    }
    if (mLabelTextureCache.imageTextureFailed(key) || !roomForAnotherTexture(key)) {
        return 0;
    }

    QImage image = clearRoomTextureImage(markerColor);
    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing);
    QPen pen;
    const qreal center = scmRoomTextureSize / 2.0;
    ExitAppearance::paintExitMarkers(painter, pen, room, center, center, scmRoomTextureSize, scmRoomTextureSize, roomColor, false);
    painter.end();
    return addRoomTexture(key, image);
}

GLuint ModernGLWidget::textTexture(const QString& key, const QString& text, const QFont& font, const QFontMetricsF& metrics, const QColor& color, QSize& size)
{
    size = QSize(qMax(1, qCeil(metrics.horizontalAdvance(text))), qMax(1, qCeil(metrics.height())));
    if (const GLuint texture = cachedRoomTexture(key)) {
        return texture;
    }
    if (mLabelTextureCache.imageTextureFailed(key) || !roomForAnotherTexture(key)) {
        return 0;
    }

    QImage image = clearImage(size, color);
    QPainter painter(&image);
    painter.setRenderHint(QPainter::TextAntialiasing);
    painter.setFont(font);
    painter.setPen(color);
    painter.drawText(image.rect(), Qt::AlignCenter, text);
    painter.end();
    return addRoomTexture(key, image);
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

    const float roomSize = 2.0f / scale;
    const float roomHeight = 2.0f / scale / zFlattening;
    // As on the 2D map, a border of thickness 1 is this much of the room's width, and grows outward
    const float borderUnit = mpHost->mRoomBorderSize > 0.0 ? roomSize / static_cast<float>(mpHost->mRoomBorderSize) : 0.0f;
    const QVector3D screenRight = mCameraController.screenRight();
    const QVector3D screenUp = mCameraController.screenUp();
    const auto [groundRight, groundUp] = GeometryManager::groundAxes(screenRight, screenUp);

    // As on the 2D map: room IDs in place of symbols, bold, sized so that the area's longest fits
    const bool showRoomIds = mpHost->mShowRoomID;
    QFont idFont = mpMap->mMapSymbolFont;
    float idTexel = 0.0f;
    QString idFontKey;
    std::optional<QFontMetricsF> idMetrics;
    if (showRoomIds) {
        int maxRoomId = 0;
        for (const int roomId : pArea->getAreaRooms()) {
            maxRoomId = qMax(maxRoomId, roomId);
        }
        idFont.setBold(true);
        idFont.setStyleStrategy(QFont::StyleStrategy(QFont::PreferNoShaping | QFont::PreferAntialias | QFont::PreferOutline));
        idFont.setPixelSize(scmTextTexelsPerRoom);
        idFontKey = idFont.key();
        idMetrics.emplace(imageFontMetrics(idFont));
        const qreal widest = idMetrics->horizontalAdvance(QString(QString::number(maxRoomId).size(), QLatin1Char('8')));
        idTexel = 0.9f * roomSize / static_cast<float>(qMax(widest, idMetrics->height()));
    }
    // Cleared before any of this frame's textures is asked for, as only those in it are kept from eviction
    mRoomTexturesInUse.clear();
    // A digit's texture and size by its color and character. Made before any room's other textures, so
    // that a view short of room for textures keeps its IDs whole
    QHash<std::pair<QRgb, char16_t>, std::pair<GLuint, QSize>> digitTextures;
    if (showRoomIds) {
        for (const QColor& idColor : {QColor(Qt::black), QColor(Qt::white)}) {
            for (char16_t digit = u'0'; digit <= u'9'; ++digit) {
                const QString text(QChar{digit});
                QSize size;
                const GLuint texture = textTexture(qsl("digit|%1|%2|%3").arg(idColor.name(), idFontKey, text), text, idFont, *idMetrics, idColor, size);
                digitTextures.insert({idColor.rgba(), digit}, {texture, size});
            }
        }
    }

    const bool showRoomNames = mpMap->getRoomNamesShown() && !pArea->gridMode;
    QFont nameFont = mpMap->mMapSymbolFont;
    if (showRoomNames) {
        if (QFont font; font.fromString(mpMap->mUserData.value(ROOM_UI_NAMEFONT))) {
            nameFont = font;
        }
        nameFont.setItalic(false);
        nameFont.setUnderline(false);
        nameFont.setOverline(false);
        nameFont.setStrikeOut(false);
        nameFont.setBold(true);
        nameFont.setStyleStrategy(QFont::StyleStrategy(QFont::PreferNoShaping | QFont::PreferAntialias | QFont::PreferOutline));
        nameFont.setPointSizeF(scmTextTexelsPerRoom / 2.0);
    }
    struct RoomName
    {
        QVector3D center;
        QColor color;
        QString name;
        QVector4D tint;
    };
    QList<RoomName> roomNames;
    const float textTexel = roomSize / scmTextTexelsPerRoom;

    // Made before any symbol, so a view short of room for textures still shows where the player is
    GLuint playerRoomTexture = 0;
    GLuint playerRoomFillTexture = 0;
    if (pArea->getAreaRooms().contains(playerRoomId)) {
        // The strong highlight is the original style over a translucent red room
        const int style = mpHost->mMapStrongHighlight ? 0 : mpMap->mPlayerRoomStyle;
        const QString key = qsl("player|%1|%2|%3|%4")
                                    .arg(QString::number(style),
                                         QString::number(mpMap->mPlayerRoomInnerDiameterPercentage),
                                         mpMap->mPlayerRoomInnerColor.name(QColor::HexArgb),
                                         mpMap->mPlayerRoomOuterColor.name(QColor::HexArgb));
        playerRoomTexture = discTexture(key, RoomAppearance::playerRoomStops(style, mpMap->mPlayerRoomInnerDiameterPercentage, mpMap->mPlayerRoomInnerColor, mpMap->mPlayerRoomOuterColor));
        if (mpHost->mMapStrongHighlight) {
            playerRoomFillTexture = squareTexture(RoomAppearance::strongHighlightFillColor());
        }
    }

    QVector<CubeInstanceData> roomInstances;
    QVector<CubeInstanceData> borderInstances;
    // Keyed by texture, so each is one draw call however many rooms share it
    QHash<GLuint, GeometryData> symbolDecals;
    QHash<GLuint, GeometryData> highlightDecals;
    QHash<GLuint, GeometryData> textDecals;
    QHash<GLuint, GeometryData> markerDecals;
    GeometryData playerRoomDecal;
    GeometryData playerRoomFillDecal;
    GeometryData selectionDecal;
    const QSet<int> emptySelection;
    const QSet<int>& selection = mp2DMap ? mp2DMap->mMultiSelectionSet : emptySelection;
    if (!selection.isEmpty()) {
        selectionDecal.textureId = selectionTexture();
    }

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

        if (!levelShown(pR->z())) {
            continue;
        }

        auto rx = static_cast<float>(pR->x());
        auto ry = static_cast<float>(pR->y());
        auto rz = static_cast<float>(pR->z());

        const bool isCurrentRoom = currentRoomId == playerRoomId;
        const bool aboveLevel = rz > pz;
        const int levelDistance = static_cast<int>(std::abs(rz - pz));

        const QColor roomColor = RoomAppearance::environmentColor(*mpMap, *mpHost, pR->environment);
        QVector4D roomTint = levelFaded(roomColor, levelDistance, aboveLevel, moreTransparent);
        if (isCurrentRoom && !playerRoomTexture) {
            // Without its marker the player's room is drawn solid red, so it still stands out
            roomTint = QVector4D(1.0f, 0.0f, 0.0f, 1.0f);
        }
        QMatrix4x4 transform;
        transform.translate(rx, ry, rz);
        transform.scale(1.0f / scale, 1.0f / scale, 1.0f / scale / zFlattening);
        roomInstances.append(CubeInstanceData(transform, roomTint.x(), roomTint.y(), roomTint.z(), roomTint.w()));

        if ((mpHost->mMapperShowRoomBorders || pR->mBorderColor.isValid() || pR->mBorderThickness > 0) && !pArea->gridMode) {
            const float borderWidth = (pR->mBorderThickness > 0 ? pR->mBorderThickness : 1) * borderUnit;
            const float spread = (roomSize + 2.0f * borderWidth) / roomSize;
            const QColor borderColor = pR->mBorderColor.isValid() ? pR->mBorderColor : mpHost->mRoomBorderColor;
            const QVector4D borderTint = levelFaded(borderColor, levelDistance, aboveLevel, moreTransparent);
            // Lower than the room, so that its top is seen only as a rim around the room's edges
            QMatrix4x4 borderTransform;
            borderTransform.translate(rx, ry, rz);
            borderTransform.scale(spread / scale, spread / scale, 0.6f / scale / zFlattening);
            borderInstances.append(CubeInstanceData(borderTransform, borderTint.x(), borderTint.y(), borderTint.z(), borderTint.w()));
        }

        const QVector3D roomTop(rx, ry, rz + roomHeight / 2.0f);
        const QVector4D decalTint = levelFaded(QColor(Qt::white), levelDistance, aboveLevel, moreTransparent);

        if (selectionDecal.textureId && selection.contains(currentRoomId)) {
            GeometryManager::appendGroundQuad(selectionDecal, roomTop, roomSize, screenRight, screenUp, decalTint);
        }

        if (!showRoomIds && !pR->mSymbol.isEmpty()) {
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

        if (showRoomIds) {
            const QColor idColor = ExitAppearance::contrastColor(roomColor);
            const QString number = QString::number(currentRoomId);
            QList<std::pair<GLuint, QSize>> digits;
            int numberWidth = 0;
            bool whole = true;
            for (const QChar digit : number) {
                const std::pair<GLuint, QSize> found = digitTextures.value({idColor.rgba(), digit.unicode()});
                whole = whole && found.first;
                digits.append(found);
                numberWidth += found.second.width();
            }
            // None rather than a number with digits missing, which would read as another room's
            if (whole) {
                QVector3D corner = roomTop - groundRight * (numberWidth * idTexel / 2.0f);
                for (const auto& [texture, size] : digits) {
                    const QVector3D across = groundRight * (size.width() * idTexel);
                    const QVector3D upward = groundUp * (size.height() * idTexel);
                    GeometryData& batch = textDecals[texture];
                    batch.textureId = texture;
                    GeometryManager::appendQuad(batch, corner - upward / 2.0f, across, upward, decalTint);
                    corner += across;
                }
            }
        }

        // Only the viewed level's, as only the 2D map's are
        if (showRoomNames && rz == pz && !pR->name.isEmpty()) {
            // One room's size below the room on screen, then moved by its offset along the map, in rooms' spacing, y downward
            const QPointF offset = RoomAppearance::nameOffset(pR->userData.value(ROOM_UI_NAMEPOS));
            const QVector3D center = roomTop - groundUp * roomSize + QVector3D(static_cast<float>(offset.x()), -static_cast<float>(offset.y()), 0.0f);
            roomNames.append({center, RoomAppearance::nameColor(pR->userData.value(ROOM_UI_NAMECOLOR), mpHost->mBgColor_2), pR->name, decalTint});
        }

        if (const GLuint texture = exitMarkerTexture(*pR, roomColor)) {
            GeometryData& batch = markerDecals[texture];
            batch.textureId = texture;
            GeometryManager::appendGroundQuad(batch, roomTop, roomSize, screenRight, screenUp, decalTint);
        }

        if (isCurrentRoom) {
            if (playerRoomFillTexture) {
                playerRoomFillDecal.textureId = playerRoomFillTexture;
                GeometryManager::appendGroundQuad(playerRoomFillDecal, roomTop, roomSize, screenRight, screenUp, decalTint);
            }
            if (playerRoomTexture) {
                playerRoomDecal.textureId = playerRoomTexture;
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
    }

    // Names come after every symbol and marker, so that a view short of room for textures loses names first
    if (!roomNames.isEmpty()) {
        const QString nameFontKey = nameFont.key();
        const QFontMetricsF nameMetrics = imageFontMetrics(nameFont);
        // Nearest first, so that where names overlap the one in front is the one kept
        const QMatrix4x4 viewModel = mCameraController.getViewMatrix() * mCameraController.getModelMatrix();
        std::stable_sort(roomNames.begin(), roomNames.end(), [&viewModel](const RoomName& a, const RoomName& b) {
            return viewModel.map(a.center).z() > viewModel.map(b.center).z();
        });
        QList<QRectF> nameRects;
        nameRects.reserve(roomNames.size());
        const QRectF nameBounds = QRectF(rect()).adjusted(-100.0, -100.0, 100.0, 100.0);
        for (const RoomName& roomName : std::as_const(roomNames)) {
            const QSizeF size(qMax(1, qCeil(nameMetrics.horizontalAdvance(roomName.name))), qMax(1, qCeil(nameMetrics.height())));
            const QVector3D across = groundRight * static_cast<float>(size.width() * textTexel);
            const QVector3D upward = groundUp * static_cast<float>(size.height() * textTexel);
            const QVector3D corner = roomName.center - across / 2.0f - upward / 2.0f;
            qreal left = std::numeric_limits<qreal>::max();
            qreal top = std::numeric_limits<qreal>::max();
            qreal right = std::numeric_limits<qreal>::lowest();
            qreal bottom = std::numeric_limits<qreal>::lowest();
            bool onScreen = true;
            for (const QVector3D& point : {corner, corner + across, corner + upward, corner + across + upward}) {
                const std::optional<QPointF> pixel = mCameraController.pixelForPoint(point);
                if (!pixel) {
                    onScreen = false;
                    break;
                }
                left = qMin(left, pixel->x());
                top = qMin(top, pixel->y());
                right = qMax(right, pixel->x());
                bottom = qMax(bottom, pixel->y());
            }
            // An invalid rectangle, for a name partly behind the camera or off screen, is never kept; one
            // nearly edge on can span millions of pixels, so each is cut down to about the view
            const QRectF rect(QPointF(left, top), QPointF(right, bottom));
            nameRects.append(onScreen && rect.intersects(nameBounds) ? rect.intersected(nameBounds) : QRectF());
        }
        for (const qsizetype index : GeometryManager::declutteredRects(nameRects, scmMinimumNamePixels)) {
            const RoomName& roomName = roomNames.at(index);
            QSize size;
            const QString key = qsl("name|%1|%2|%3").arg(roomName.color.name(QColor::HexArgb), nameFontKey, roomName.name);
            if (const GLuint texture = textTexture(key, roomName.name, nameFont, nameMetrics, roomName.color, size)) {
                const QVector3D across = groundRight * (size.width() * textTexel);
                const QVector3D upward = groundUp * (size.height() * textTexel);
                GeometryData& batch = textDecals[texture];
                batch.textureId = texture;
                GeometryManager::appendQuad(batch, roomName.center - across / 2.0f - upward / 2.0f, across, upward, roomName.tint);
            }
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

    if (selectionDecal.isEmpty() && symbolDecals.isEmpty() && highlightDecals.isEmpty() && textDecals.isEmpty() && markerDecals.isEmpty() && playerRoomDecal.isEmpty()
        && playerRoomFillDecal.isEmpty()) {
        return;
    }
    // In the 2D map's order: outside grid mode a highlight is drawn over the symbol, in grid mode under it;
    // then the ID and name, the exit markers and the player's ring
    const QHash<GLuint, GeometryData>& lowerDecals = pArea->gridMode ? highlightDecals : symbolDecals;
    const QHash<GLuint, GeometryData>& upperDecals = pArea->gridMode ? symbolDecals : highlightDecals;
    mRenderCommandQueue.addCommand(std::make_unique<GLStateCommand>(GLStateCommand::DISABLE_DEPTH_WRITE));
    mRenderCommandQueue.addCommand(std::make_unique<GLStateCommand>(GLStateCommand::ENABLE_POLYGON_OFFSET));
    // The selection shades the room itself, as the 2D map's does, so it is under everything drawn on it
    if (!selectionDecal.isEmpty()) {
        mRenderCommandQueue.addCommand(std::make_unique<RenderTexturedTrianglesCommand>(
                selectionDecal, mCameraController.getProjectionMatrix(), mCameraController.getViewMatrix(), mCameraController.getModelMatrix(), RenderCommand::Shading::Unlit));
    }
    const QHash<GLuint, GeometryData>* const decalLayers[] = {&lowerDecals, &upperDecals, &textDecals, &markerDecals};
    for (const QHash<GLuint, GeometryData>* decals : decalLayers) {
        for (const GeometryData& batch : *decals) {
            mRenderCommandQueue.addCommand(std::make_unique<RenderTexturedTrianglesCommand>(
                    batch, mCameraController.getProjectionMatrix(), mCameraController.getViewMatrix(), mCameraController.getModelMatrix(), RenderCommand::Shading::Unlit));
        }
    }
    for (const GeometryData* playerDecal : {&playerRoomFillDecal, &playerRoomDecal}) {
        if (!playerDecal->isEmpty()) {
            mRenderCommandQueue.addCommand(std::make_unique<RenderTexturedTrianglesCommand>(
                    *playerDecal, mCameraController.getProjectionMatrix(), mCameraController.getViewMatrix(), mCameraController.getModelMatrix(), RenderCommand::Shading::Unlit));
        }
    }
    mRenderCommandQueue.addCommand(std::make_unique<GLStateCommand>(GLStateCommand::DISABLE_POLYGON_OFFSET));
    mRenderCommandQueue.addCommand(std::make_unique<GLStateCommand>(GLStateCommand::ENABLE_DEPTH_WRITE));
}

void ModernGLWidget::renderConnections()
{
    if (!mpMap || !mpMap->mpRoomDB || !mpHost) {
        return;
    }

    const float roomSize = 2.0f / scale;
    const float roomHeight = roomSize / zFlattening;
    ExitScene::Options options;
    options.areaId = mAID;
    options.centerZ = mMapCenterZ;
    options.levelsAbove = mShowTopLevels;
    options.levelsBelow = mShowBottomLevels;
    // As on the 2D map, the exit size setting is how many line widths make a room's width
    options.lineWidth = roomSize / static_cast<float>(qMax(0.1, mpHost->mLineSize));
    options.lineHeight = qMin(options.lineWidth, 0.8f * roomHeight);
    options.inOutStubs = mpHost->experimentEnabled(qsl("experiment.render-in-out-exits"));
    options.largeAreaExitArrows = mpHost->getLargeAreaExitArrows();
    options.exitColor = mpHost->mFgColor_2;
    options.background = mpHost->mBgColor_2;
    const ExitScene::Scene scene = ExitScene::build(*mpMap, *mpHost, options);
    if (scene.solidDashRoomId && !mSolidDashWarned) {
        mSolidDashWarned = true;
        qWarning().nospace() << "ModernGLWidget: room " << scene.solidDashRoomId
                             << "'s dotted or dashed exit line is drawn solid, as its pattern would repeat too many times along it: the line is very long or the exit lines very thin";
    }
    if (scene.segments.isEmpty() && scene.triangles.isEmpty()) {
        return;
    }

    const bool moreTransparent = mpHost->experimentEnabled(qsl("experiment.rendering.more-transparent"));
    const auto faded = [&](const QColor& color, const int level) {
        return levelFaded(color, std::abs(level - mMapCenterZ), level > mMapCenterZ, moreTransparent);
    };

    mRenderCommandQueue.addCommand(std::make_unique<GLStateCommand>(GLStateCommand::ENABLE_DEPTH_TEST));

    QVector<CubeInstanceData> bars;
    bars.reserve(scene.segments.size());
    for (const ExitScene::Segment& segment : scene.segments) {
        const QVector4D tint = faded(segment.color, segment.level);
        bars.append(CubeInstanceData(GeometryManager::barTransform(segment.from, segment.to, segment.width, segment.height), tint.x(), tint.y(), tint.z(), tint.w()));
    }
    if (!bars.isEmpty()) {
        mRenderCommandQueue.addCommand(std::make_unique<RenderInstancedCubesCommand>(
                bars, mCameraController.getProjectionMatrix(), mCameraController.getViewMatrix(), mCameraController.getModelMatrix(), RenderCommand::Shading::Unlit));
    }

    QVector<float> vertices;
    QVector<float> colors;
    vertices.reserve(scene.triangles.size() * 9);
    colors.reserve(scene.triangles.size() * 12);
    for (const ExitScene::Triangle& triangle : scene.triangles) {
        const QVector4D tint = faded(triangle.color, triangle.level);
        for (const QVector3D& corner : {triangle.a, triangle.b, triangle.c}) {
            vertices << corner.x() << corner.y() << corner.z();
            colors << tint.x() << tint.y() << tint.z() << tint.w();
        }
    }
    if (!vertices.isEmpty()) {
        // Arrowheads and dots lie on the lines' tops
        mRenderCommandQueue.addCommand(std::make_unique<GLStateCommand>(GLStateCommand::ENABLE_POLYGON_OFFSET));
        mRenderCommandQueue.addCommand(std::make_unique<RenderTrianglesCommand>(
                vertices, colors, mCameraController.getProjectionMatrix(), mCameraController.getViewMatrix(), mCameraController.getModelMatrix(), RenderCommand::Shading::Unlit));
        mRenderCommandQueue.addCommand(std::make_unique<GLStateCommand>(GLStateCommand::DISABLE_POLYGON_OFFSET));
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

bool ModernGLWidget::cameraFixedByGridMode() const
{
    if (!mpMap || !mpMap->mpRoomDB) {
        return false;
    }
    const TArea* pArea = mpMap->mpRoomDB->getArea(areaShownNext());
    return pArea && pArea->gridMode;
}

int ModernGLWidget::areaShownNext() const
{
    // The next paint moves a view that follows the player to the player's area
    if (mpMap && mpMap->mpRoomDB && followingPlayer()) {
        if (const TRoom* pPlayerRoom = mpMap->mpRoomDB->getRoom(mpMap->mRoomIdHash.value(mpMap->mProfileName))) {
            return pPlayerRoom->getArea();
        }
    }
    return mAID;
}

void ModernGLWidget::setCameraDistance(float r)
{
    mCameraController.setScale(r);
    mDistanceRequested = true;
    mFrameWhenShown = false;
    emitCameraControls();
    update();
}

void ModernGLWidget::setCameraPosition(float r, float theta, float phi)
{
    mCameraController.setPosition(r, theta, phi);
    mDistanceRequested = true;
    // Set for the area on view, so it is that area's camera now even before a paint
    mFramePending = false;
    mFrameWhenShown = false;
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
        // So that the area is framed for this zoom when it is next shown
        if (mSavedCameras.remove(areaId)) {
            writeSavedCameras();
        }
        return;
    }
    frameRooms(zoom);
    mFrameWhenShown = !isVisible();
    mDistanceRequested = false;
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
    mFrameWhenShown = false;
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
    if (mSecondaryView) {
        return false;
    }
    return !mShiftMode || !mpMap || mRID != mpMap->mRoomIdHash.value(mpMap->mProfileName) || mpMap->mNewMove;
}

void ModernGLWidget::followPlayer()
{
    if (mSecondaryView) {
        // Its map window goes to the player's room when asked, but stays there as the player moves on
        const TRoom* pRoom = mpMap && mpMap->mpRoomDB ? mpMap->mpRoomDB->getRoom(mpMap->mRoomIdHash.value(mpMap->mProfileName)) : nullptr;
        if (pRoom) {
            setViewCenter(pRoom->getArea(), pRoom->x(), pRoom->y(), pRoom->z());
        }
        return;
    }
    mShiftMode = false;
    update();
}

void ModernGLWidget::setSecondaryView(const bool secondary)
{
    mSecondaryView = secondary;
    if (secondary) {
        mShiftMode = true;
    }
}

void ModernGLWidget::holdView()
{
    // Held as a view moved by hand is, so it does not chase the player's room if that is among those moved
    mShiftMode = true;
    mRID = mpMap ? mpMap->mRoomIdHash.value(mpMap->mProfileName) : 0;
    mPreviousRID = mRID;
}

void ModernGLWidget::jumpTo(int areaId, int x, int y, int z)
{
    stopSmoothTransition();
    if (areaId != mAID) {
        saveCamera();
        mAID = areaId;
        const TArea* pArea = mpMap && mpMap->mpRoomDB ? mpMap->mpRoomDB->getArea(areaId) : nullptr;
        mFramePending = !restoreCamera(pArea && pArea->gridMode);
        mFrameWhenShown = mFramePending && !isVisible();
        // As the 2D map does on changing area, so that room actions never reach rooms out of sight
        if (mp2DMap) {
            mp2DMap->pruneRoomSelectionToArea(areaId);
        }
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
    zoomBySteps(steps);
}

void ModernGLWidget::zoomBySteps(const float steps)
{
    mCameraController.zoomBy(steps);
    mFrameWhenShown = false;
    emitCameraControls();
    update();
}

bool ModernGLWidget::levelShown(const int z) const
{
    const int levelsAbove = z - mMapCenterZ;
    return levelsAbove >= 0 ? levelsAbove <= mShowTopLevels : -levelsAbove <= mShowBottomLevels;
}

namespace {
// Where a ray first meets an axis-aligned box, as a distance along it, by the slab method
std::optional<float> rayHitsBox(const QVector3D& origin, const QVector3D& direction, const QVector3D& lowCorner, const QVector3D& highCorner)
{
    float nearest = 0.0f;
    float farthest = std::numeric_limits<float>::max();
    for (int axis = 0; axis < 3; ++axis) {
        if (std::abs(direction[axis]) < 1e-9f) {
            if (origin[axis] < lowCorner[axis] || origin[axis] > highCorner[axis]) {
                return std::nullopt;
            }
            continue;
        }
        float entry = (lowCorner[axis] - origin[axis]) / direction[axis];
        float exit = (highCorner[axis] - origin[axis]) / direction[axis];
        if (entry > exit) {
            std::swap(entry, exit);
        }
        nearest = qMax(nearest, entry);
        farthest = qMin(farthest, exit);
        if (nearest > farthest) {
            return std::nullopt;
        }
    }
    return nearest;
}

// A room seen nearly edge on is a sliver, so one whose center is this close to the pointer counts as hit
constexpr qreal scmPickSlopPixels = 6.0;
} // namespace

int ModernGLWidget::roomAt(const QPoint& position, const bool orNear)
{
    if (!mpMap || !mpMap->mpRoomDB) {
        return 0;
    }
    const TArea* pArea = mpMap->mpRoomDB->getArea(mAID);
    if (!pArea) {
        return 0;
    }

    updateMatrices();
    const auto [origin, direction] = mCameraController.rayThroughPixel(QPointF(position));
    const QVector3D halfSize(1.0f / scale, 1.0f / scale, 1.0f / scale / zFlattening);
    int hitRoom = 0;
    float hitDistance = std::numeric_limits<float>::max();
    int nearRoom = 0;
    qreal nearPixels = scmPickSlopPixels;
    for (const int roomId : pArea->getAreaRooms()) {
        const TRoom* pR = mpMap->mpRoomDB->getRoom(roomId);
        if (!pR || pR->isHidden() || !levelShown(pR->z())) {
            continue;
        }
        const QVector3D center(static_cast<float>(pR->x()), static_cast<float>(pR->y()), static_cast<float>(pR->z()));
        if (const std::optional<float> distance = rayHitsBox(origin, direction, center - halfSize, center + halfSize)) {
            if (*distance < hitDistance) {
                hitDistance = *distance;
                hitRoom = roomId;
            }
        } else if (orNear && !hitRoom) {
            if (const std::optional<QPointF> pixel = mCameraController.pixelForPoint(center)) {
                const qreal pixels = QLineF(*pixel, QPointF(position)).length();
                if (pixels < nearPixels) {
                    nearPixels = pixels;
                    nearRoom = roomId;
                }
            }
        }
    }
    return hitRoom ? hitRoom : nearRoom;
}

QString ModernGLWidget::roomToolTip(const int roomId) const
{
    const TRoom* pR = mpMap && mpMap->mpRoomDB ? mpMap->mpRoomDB->getRoom(roomId) : nullptr;
    if (!pR) {
        return QString();
    }
    QStringList lines;
    if (!pR->name.isEmpty()) {
        lines << qsl("<b>%1</b>").arg(pR->name.toHtmlEscaped());
    }
    //: Tooltip over a room in the 3D map view, %1 is the room's ID number
    lines << tr("Room ID: %1").arg(roomId);
    const QString areaName = mpMap->mpRoomDB->getAreaNamesMap().value(pR->getArea());
    if (!areaName.isEmpty()) {
        //: Tooltip over a room in the 3D map view, %1 is the name of the area the room is in
        lines << tr("Area: %1").arg(areaName.toHtmlEscaped());
    }
    return qsl("<p style='white-space:pre'>%1</p>").arg(lines.join(qsl("<br>")));
}

void ModernGLWidget::setCompanion2DMap(T2DMap* map)
{
    mp2DMap = map;
}

void ModernGLWidget::selectRoom(const int roomId, const Qt::KeyboardModifiers modifiers)
{
    if (!mp2DMap) {
        return;
    }
    mp2DMap->selectRoomFromOtherView(roomId, modifiers);
    update();
}

bool ModernGLWidget::editingMap() const
{
    return mp2DMap && !mp2DMap->mMapViewOnly;
}

namespace {
// Toward the horizon each pixel spans ever more of a level, so beyond this many times the camera's distance from the
// view center a drag would throw rooms far off the map
constexpr float scmMaxDragReach = 10.0f;
} // namespace

std::optional<QPointF> ModernGLWidget::mapPointOnLevel(const QPoint& position, const int z)
{
    updateMatrices();
    const auto [origin, direction] = mCameraController.rayThroughPixel(QPointF(position));
    if (std::abs(direction.z()) < 1e-6f) {
        return std::nullopt;
    }
    const float distance = (static_cast<float>(z) - origin.z()) / direction.z();
    if (distance < 0.0f || distance > scmMaxDragReach * (mCameraController.getTarget() - origin).length()) {
        return std::nullopt;
    }
    const QVector3D point = origin + direction * distance;
    return QPointF(point.x(), point.y());
}

QSet<int> ModernGLWidget::roomsInRectangle(const QRect& rectangle, const bool allLevels)
{
    QSet<int> rooms;
    const TArea* pArea = mpMap && mpMap->mpRoomDB ? mpMap->mpRoomDB->getArea(mAID) : nullptr;
    if (!pArea) {
        return rooms;
    }
    updateMatrices();
    const QRectF bounds(rectangle.normalized());
    const QVector3D halfSize(1.0f / scale, 1.0f / scale, 1.0f / scale / zFlattening);
    for (const int roomId : pArea->getAreaRooms()) {
        const TRoom* pR = mpMap->mpRoomDB->getRoom(roomId);
        if (!pR || pR->isHidden() || !levelShown(pR->z()) || (!allLevels && pR->z() != mMapCenterZ)) {
            continue;
        }
        const QVector3D center(static_cast<float>(pR->x()), static_cast<float>(pR->y()), static_cast<float>(pR->z()));
        // As on the 2D map, only a room the rectangle covers whole is picked
        bool inside = true;
        for (int corner = 0; corner < 8 && inside; ++corner) {
            const QVector3D offset((corner & 1) ? halfSize.x() : -halfSize.x(), (corner & 2) ? halfSize.y() : -halfSize.y(), (corner & 4) ? halfSize.z() : -halfSize.z());
            const std::optional<QPointF> pixel = mCameraController.pixelForPoint(center + offset);
            inside = pixel && bounds.contains(*pixel);
        }
        if (inside) {
            rooms.insert(roomId);
        }
    }
    return rooms;
}

void ModernGLWidget::startRoomMove(const int roomId)
{
    const TRoom* pRoom = mpMap->mpRoomDB->getRoom(roomId);
    if (!pRoom) {
        return;
    }
    mp2DMap->selectRoomsToMove({roomId}, roomId);
    mp2DMap->updateSelectionWidget();
    mMovingRooms = true;
    mRoomsMoved = false;
    mRoomMoveLastPoint = QPointF(pRoom->x(), pRoom->y());
    mRoomMoveLevel = pRoom->z();
    holdView();
    update();
}

void ModernGLWidget::endEditGesture()
{
    if (mRectangleSelecting && mp2DMap) {
        mp2DMap->endRectangleSelection();
    }
    // The other views showing the area redraw only on this
    if (std::exchange(mRoomsMoved, false) && mpHost && mpMap) {
        mpMap->updateArea(mAID);
    }
    mMovingRooms = false;
    mRectangleSelecting = false;
    mSelectionRectangle = QRect();
}

void ModernGLWidget::raiseMousePressEvent()
{
    if (!mpHost) {
        return;
    }
    TEvent sysMapWindowMousePressEvent{};
    sysMapWindowMousePressEvent.mArgumentList.append(QLatin1String("sysMapWindowMousePressEvent"));
    sysMapWindowMousePressEvent.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
    mpHost->raiseEvent(sysMapWindowMousePressEvent);
}

void ModernGLWidget::mousePressEvent(QMouseEvent* event)
{
    mudlet::self()->activateProfile(mpHost);
    if (!mpMap || !mpMap->mpRoomDB) {
        return;
    }

    mKeysNavigate = false;
    mPressPosition = event->position().toPoint();
    mClickPending = false;
    // A release that went elsewhere, such as to a menu opened mid-drag, must not leave the last drag running
    endEditGesture();
    // As on the 2D map in editing mode: a drag from a room moves the selection and one from empty space,
    // or with Ctrl or Shift, draws a selection rectangle; Alt keeps the viewing mode's drags to pan and orbit
    if (event->button() == Qt::LeftButton && editingMap() && !event->modifiers().testFlag(Qt::AltModifier)) {
        if (mCameraSmoothAnimating) {
            // Stopped where it is, so the press picks what is under the pointer and a drag does not glide on with it
            stopSmoothTransition();
            mMapCenterX = qRound(mCurrentAnimationX);
            mMapCenterY = qRound(mCurrentAnimationY);
            mMapCenterZ = qRound(mCurrentAnimationZ);
        }
        // Only a room right under the pointer is picked up, so a drag beside rooms seen small draws a rectangle
        const int roomId = roomAt(mPressPosition, false);
        if (roomId && !(event->modifiers() & (Qt::ControlModifier | Qt::ShiftModifier))) {
            startRoomMove(roomId);
        } else {
            mp2DMap->beginRectangleSelection(roomId ? roomId : roomAt(mPressPosition), event->modifiers());
            mRectangleSelecting = true;
            mRectangleDragged = false;
            mSelectionRectangle = QRect(mPressPosition, QSize());
            update();
        }
        raiseMousePressEvent();
        return;
    }
    if (event->button() == Qt::LeftButton) {
        // A plain click on a room selects it at once, as on the 2D map; anything else waits for the release, as a
        // drag from empty space pans and a Ctrl or Shift drag orbits
        const int roomId = event->modifiers() == Qt::NoModifier ? roomAt(mPressPosition) : 0;
        if (roomId) {
            selectRoom(roomId, event->modifiers());
        } else {
            mClickPending = true;
        }
    }

    if (event->buttons() & Qt::LeftButton) { // translation on xy-plane
        auto eventPos = event->position().toPoint();
        const int x = eventPos.x();
        const int y = height() - eventPos.y(); // the opengl origin is at bottom left
        mPanMode = true;
        mPanXStart = x;
        mPanYStart = y;
    }

    // The 2D map raises this after the press has changed the selection, so a script handling it reads the new one
    mPressEventPending = mClickPending;
    if (!mPressEventPending) {
        raiseMousePressEvent();
    }
}

void ModernGLWidget::mouseMoveEvent(QMouseEvent* event)
{
    if (!mpMap || !mpMap->mpRoomDB) {
        return;
    }
    if (mClickPending && (event->position().toPoint() - mPressPosition).manhattanLength() >= QApplication::startDragDistance()) {
        mClickPending = false;
        if (std::exchange(mPressEventPending, false)) {
            raiseMousePressEvent();
        }
    }
    if ((mMovingRooms || mRectangleSelecting) && !event->buttons().testFlag(Qt::LeftButton)) {
        endEditGesture();
        update();
    }
    if (mMovingRooms) {
        // As RoomMoveDragHandler: the rooms follow the pointer in whole grid steps
        if (const std::optional<QPointF> point = mapPointOnLevel(event->position().toPoint(), mRoomMoveLevel)) {
            const int dx = qRound(point->x() - mRoomMoveLastPoint.x());
            const int dy = qRound(point->y() - mRoomMoveLastPoint.y());
            if ((dx || dy) && mp2DMap) {
                mRoomMoveLastPoint += QPointF(dx, dy);
                mp2DMap->moveSelectedRoomsBy(dx, dy);
                mRoomsMoved = true;
                update();
            }
        }
        return;
    }
    if (mRectangleSelecting) {
        mSelectionRectangle = QRect(mPressPosition, event->position().toPoint()).normalized();
        if (!mRectangleDragged && (event->position().toPoint() - mPressPosition).manhattanLength() >= QApplication::startDragDistance()) {
            mRectangleDragged = true;
        }
        // Until it is a drag, a Ctrl or Shift click keeps the room it picked
        if (mRectangleDragged && mp2DMap) {
            mp2DMap->applyRectangleSelection(roomsInRectangle(mSelectionRectangle, event->modifiers().testFlag(Qt::ShiftModifier)), event->modifiers());
        }
        update();
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
    if (event->button() == Qt::LeftButton && (mMovingRooms || mRectangleSelecting)) {
        endEditGesture();
        QOpenGLWidget::mouseReleaseEvent(event);
        update();
        return;
    }
    if (mPanMode) {
        mPanMode = false;
        mCameraController.snapTargetToGrid();
        const QVector3D newCenter = mCameraController.getTarget();
        mMapCenterX = static_cast<int>(newCenter.x());
        mMapCenterY = static_cast<int>(newCenter.y());
        mMapCenterZ = static_cast<int>(newCenter.z());
    }
    if (event->button() == Qt::LeftButton && mClickPending) {
        mClickPending = false;
        selectRoom(roomAt(event->position().toPoint()), event->modifiers());
    } else if (event->button() == Qt::RightButton && mpMap && mpMap->mpRoomDB) {
        showContextMenu(event->position().toPoint());
    }
    if (event->button() == Qt::LeftButton && std::exchange(mPressEventPending, false)) {
        raiseMousePressEvent();
    }
    QOpenGLWidget::mouseReleaseEvent(event);
    update();
}

void ModernGLWidget::mouseDoubleClickEvent(QMouseEvent* event)
{
    // Qt sends this in place of the second press, and the release after it must not select again
    mClickPending = false;
    if (event->button() != Qt::LeftButton || !mp2DMap || !mpMap || !mpMap->mpRoomDB) {
        QOpenGLWidget::mouseDoubleClickEvent(event);
        return;
    }
    const int roomId = roomAt(event->position().toPoint());
    if (roomId) {
        mp2DMap->initiateSpeedWalk(mpMap->mRoomIdHash.value(mpMap->mProfileName), roomId);
        update();
    }
    event->accept();
}

void ModernGLWidget::showContextMenu(const QPoint& position)
{
    if (!mp2DMap) {
        return;
    }
    const int roomId = roomAt(position);
    RoomContextMenuHandler handler(*mp2DMap);
    QMenu* menu = handler.menuForOtherView(roomId, this);
    if (!menu) {
        return;
    }
    //: 3D map view context menu item: shows the room clicked on, or else the middle of the view, on the 2D map
    auto* editIn2D = new QAction(tr("Edit in 2D"), menu);
    connect(editIn2D, &QAction::triggered, this, [this, roomId]() {
        emit editIn2DRequested(roomId);
    });
    QAction* firstAction = menu->actions().value(0);
    menu->insertAction(firstAction, editIn2D);
    if (firstAction) {
        menu->insertSeparator(firstAction);
    }
    connect(menu, &QObject::destroyed, this, qOverload<>(&QWidget::update));
    update();
    menu->popup(mapToGlobal(position));
}

void ModernGLWidget::panByRooms(const int right, const int up)
{
    // A step of about a sixth of what the view shows from top to bottom, along the screen's axes laid on the map
    const int step = qMax(1, qRound(mCameraController.getScale() * 2.0f));
    const auto [groundRight, groundUp] = GeometryManager::groundAxes(mCameraController.screenRight(), mCameraController.screenUp());
    const QVector3D offset = (groundRight * static_cast<float>(right) + groundUp * static_cast<float>(up)) * static_cast<float>(step);
    // Rooms may sit at the ends of the int range, where the sum would wrap to the far side of the map
    const auto shifted = [](const int coordinate, const float by) {
        const qint64 sum = static_cast<qint64>(coordinate) + qRound64(by);
        return static_cast<int>(std::clamp<qint64>(sum, std::numeric_limits<int>::min(), std::numeric_limits<int>::max()));
    };
    setViewCenter(mAID, shifted(mMapCenterX, offset.x()), shifted(mMapCenterY, offset.y()), mMapCenterZ);
}

void ModernGLWidget::focusInEvent(QFocusEvent* event)
{
    // After a click the keys typed next are a command, as with the 2D map, which never takes focus
    mKeysNavigate = event->reason() == Qt::TabFocusReason || event->reason() == Qt::BacktabFocusReason;
    QOpenGLWidget::focusInEvent(event);
}

void ModernGLWidget::keyPressEvent(QKeyEvent* event)
{
    // Shift is needed for + on many layouts; keypad keys are left to key bindings on the command line
    const bool navigates = mKeysNavigate && !(event->modifiers() & (Qt::ControlModifier | Qt::AltModifier | Qt::MetaModifier | Qt::KeypadModifier));
    if (navigates && mpMap && mpMap->mpRoomDB) {
        switch (event->key()) {
        case Qt::Key_Left:
        case Qt::Key_A:
            panByRooms(-1, 0);
            return;
        case Qt::Key_Right:
        case Qt::Key_D:
            panByRooms(1, 0);
            return;
        case Qt::Key_Up:
        case Qt::Key_W:
            panByRooms(0, 1);
            return;
        case Qt::Key_Down:
        case Qt::Key_S:
            panByRooms(0, -1);
            return;
        case Qt::Key_Plus:
        case Qt::Key_Equal:
            zoomBySteps(1.0f);
            return;
        case Qt::Key_Minus:
            zoomBySteps(-1.0f);
            return;
        case Qt::Key_Home:
            followPlayer();
            return;
        default:
            break;
        }
    }

    TMainConsole* console = mpHost ? mpHost->mainConsoleView() : nullptr;
    if (console && console->mpCommandLine) {
        TCommandLine* commandLine = console->mpCommandLine;
        // A floating map is a window of its own, and focus in an inactive window waits for it to be activated
        commandLine->activateWindow();
        commandLine->setFocus(Qt::OtherFocusReason);
        QCoreApplication::sendEvent(commandLine, event);
        return;
    }
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

void ModernGLWidget::renderLabels(const bool onTop)
{
    if (!mpMap || !mpMap->mpRoomDB) {
        return;
    }

    TArea* pArea = mpMap->mpRoomDB->getArea(mAID);
    if (!pArea || pArea->mMapLabels.isEmpty()) {
        return;
    }

    const float roomHeight = 2.0f / scale / zFlattening;
    const bool moreTransparent = mpHost->experimentEnabled(qsl("experiment.rendering.more-transparent"));
    const QVector3D screenRight = mCameraController.screenRight();
    const QVector3D screenUp = mCameraController.screenUp();
    // Turned around while the top of the screen faces south, so that it never reads upside down
    const bool turned = GeometryManager::groundAxes(screenRight, screenUp).second.y() < 0.0f;

    mRenderCommandQueue.addCommand(std::make_unique<GLStateCommand>(GLStateCommand::DISABLE_DEPTH_WRITE));
    if (onTop) {
        mRenderCommandQueue.addCommand(std::make_unique<GLStateCommand>(GLStateCommand::ENABLE_POLYGON_OFFSET));
    }

    for (auto it = pArea->mMapLabels.constBegin(); it != pArea->mMapLabels.constEnd(); ++it) {
        const TMapLabel& label = it.value();
        if (label.showOnTop != onTop || label.pix.isNull()) {
            continue;
        }

        const std::optional<int> level = GeometryManager::labelLevel(label.pos.z());
        if (!level) {
            continue;
        }
        const qint64 levelsAbove = qint64{*level} - mMapCenterZ;
        if (levelsAbove > mShowTopLevels || -levelsAbove > mShowBottomLevels) {
            continue;
        }

        const GLuint textureId = mLabelTextureCache.getTexture(mAID, it.key(), label.pix);
        if (textureId == 0) {
            continue;
        }

        // pos is the top-left corner, on the map's plane: on the rooms' tops if shown on top of them, else beneath them
        const auto x = static_cast<float>(label.pos.x());
        const auto y = static_cast<float>(label.pos.y());
        const float z = static_cast<float>(label.pos.z()) + (onTop ? roomHeight / 2.0f : -roomHeight / 2.0f);
        if (label.noScaling) {
            // Its own size in pixels whatever the zoom, as on the 2D map, so it faces the screen
            const QVector3D corner(x, y, z);
            const float worldPerPixel = mCameraController.worldUnitsPerPixel(corner);
            const QSizeF pixels = label.pix.deviceIndependentSize();
            const auto width = static_cast<float>(pixels.width()) * worldPerPixel;
            const auto height = static_cast<float>(pixels.height()) * worldPerPixel;
            const QVector3D center = corner + screenRight * (width / 2.0f) - screenUp * (height / 2.0f);
            if (onTop) {
                // Standing up off the map, it would pass through the rooms it is meant to be over
                mRenderCommandQueue.addCommand(std::make_unique<GLStateCommand>(GLStateCommand::DISABLE_DEPTH_TEST));
            }
            mRenderCommandQueue.addCommand(std::make_unique<RenderLabelCommand>(center.x(),
                                                                                center.y(),
                                                                                center.z(),
                                                                                width,
                                                                                height,
                                                                                textureId,
                                                                                screenRight,
                                                                                screenUp,
                                                                                label.highlight,
                                                                                mCameraController.getProjectionMatrix(),
                                                                                mCameraController.getViewMatrix(),
                                                                                mCameraController.getModelMatrix()));
            if (onTop) {
                mRenderCommandQueue.addCommand(std::make_unique<GLStateCommand>(GLStateCommand::ENABLE_DEPTH_TEST));
            }
            continue;
        }

        const float width = std::abs(static_cast<float>(label.size.width()));
        const float height = std::abs(static_cast<float>(label.size.height()));
        QVector4D tint = levelFaded(QColor(Qt::white), static_cast<int>(std::abs(levelsAbove)), levelsAbove > 0, moreTransparent);
        if (label.highlight) {
            tint = QVector4D(1.0f, 0.65f, 0.0f, tint.w());
        }
        GeometryData geometry;
        geometry.textureId = textureId;
        GeometryManager::appendLabelQuad(geometry, QVector3D(x, y, z), width, height, turned, tint);
        mRenderCommandQueue.addCommand(std::make_unique<RenderTexturedTrianglesCommand>(
                geometry, mCameraController.getProjectionMatrix(), mCameraController.getViewMatrix(), mCameraController.getModelMatrix(), RenderCommand::Shading::Unlit));
    }

    if (onTop) {
        mRenderCommandQueue.addCommand(std::make_unique<GLStateCommand>(GLStateCommand::DISABLE_POLYGON_OFFSET));
    }
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
    mMapCenterX = targetX;
    mMapCenterY = targetY;
    mMapCenterZ = targetZ;

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
