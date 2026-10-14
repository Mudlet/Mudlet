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
#include "MapDecals.h"
#include "MapStyle.h"
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
#include <QLineF>
#include <QLinearGradient>
#include <QMap>
#include <QMenu>
#include <QPainter>
#include <QPolygonF>
#include <QKeyEvent>
#include <QSurfaceFormat>
#include <QToolTip>
#include <QVBoxLayout>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <limits>
#include <numbers>
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
    // For the room under the pointer to rise
    setMouseTracking(true);
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
        if (!isVisible()) {
            return;
        }
        if (!context() && !size().isEmpty()) {
            // Qt can drop a context not yet composed when the view is shown and, with no RHI yet, rebuild it only
            // on a later resize or paint; resize synchronously so the check below sees the result, which update() would not
            QResizeEvent resize(size(), size());
            QCoreApplication::sendEvent(this, &resize);
        }
        reportContextCreationFailure();
    });
}

void ModernGLWidget::hideEvent(QHideEvent* event)
{
    endEditGesture();
    saveCamera();
    // The release, if any, now goes elsewhere
    mClickPending = false;
    mPanMode = false;
    mPointerInView = false;
    mPointerButtons = Qt::NoButton;
    mHoveredRoomId = 0;
    if (std::exchange(mPressEventButton, Qt::NoButton) != Qt::NoButton) {
        // Queued, as a script handling it could show or delete this view mid-hide
        QTimer::singleShot(0, this, &ModernGLWidget::raiseMousePressEvent);
    }
    // Qt would pass focus to the next widget in the window, which in a floating map is one of its buttons
    if (hasFocus() && !event->spontaneous() && mpHost && !mpHost->isClosingDown()) {
        if (TCommandLine* pCommandLine = commandLine()) {
            pCommandLine->activateWindow();
            pCommandLine->setFocus(Qt::OtherFocusReason);
        }
    }
    QOpenGLWidget::hideEvent(event);
}

namespace {
// Per profile: for each area by ID, how the 3D view last looked at it
constexpr QLatin1StringView scmSavedCamerasItem{"3dmapper_cameras"};
// Fog, in multiples of the camera's distance from the view's center, and at most how far it goes to the background
constexpr float scmFogStart = 1.3f;
constexpr float scmFogEnd = 4.0f;
constexpr float scmFogMax = 0.6f;
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
// its camera in one go is refused; showEvent()'s timer is only a fallback, a turn of the event loop later.
// Qt asks for a context on any resize to a non-empty size, so a view with no size, such as an embedded map
// Geyser has hidden, has not asked yet, while a sized view with none was refused one
void ModernGLWidget::reportContextCreationFailure()
{
    if (!mFailureMessage.isEmpty() || size().isEmpty()) {
        return;
    }
    if (!context()) {
        // Qt makes no context at all when the window is composed with Direct3D, Metal or Vulkan, and refuses the
        // classic 3D view the same way, so only undoing what forced it helps
        const QStringList forcing = rhiForcingVariables();
        //: Shown in place of the 3D map when the window is drawn with another graphics API, such as Direct3D, Metal or Vulkan, so the modern 3D mapper cannot get an OpenGL context
        const QString message = tr("The modern 3D mapper needs OpenGL, but this window is drawn with another graphics API.");
        if (forcing.isEmpty()) {
            //: Second paragraph of the message shown in place of the 3D map when the window is drawn with another graphics API; the classic 3D view is refused the same way
            showFailure(message, tr("The classic 3D view cannot draw here either, so use the 2D map."));
        } else if (forcing.size() == 1) {
            //: Second paragraph of the message shown in place of the 3D map when one environment variable makes Qt draw the window with another graphics API; %1 is its name, such as QT_WIDGETS_RHI, keep it as written
            showFailure(message,
                        tr("%1 is set in Mudlet's environment, which makes Qt draw this window that way, for the classic 3D view too. Remove it and restart Mudlet to use either 3D view; until then, "
                           "use the 2D map.")
                                .arg(forcing.constFirst()));
        } else {
            //: Second paragraph of the message shown in place of the 3D map when several environment variables make Qt draw the window with another graphics API; %1 is their names separated by commas, such as QT_WIDGETS_RHI, QT_WIDGETS_RHI_BACKEND, keep them as written
            showFailure(message,
                        tr("%1 are set in Mudlet's environment, which make Qt draw this window that way, for the classic 3D view too. Remove them and restart Mudlet to use either 3D view; until "
                           "then, use the 2D map.")
                                .arg(forcing.join(qsl(", "))));
        }
    } else if (!isValid()) {
        //: Shown in place of the 3D map when the graphics driver cannot create the OpenGL context the modern 3D mapper needs
        showFailure(tr("The modern 3D mapper could not create an OpenGL 3.3 context on this system."));
    }
}

// Mirrors QBackingStoreRhiSupport::checkForceRhi(), which Qt evaluates once at startup; Mudlet never changes these
QStringList ModernGLWidget::rhiForcingVariables()
{
    QStringList enabling;
    for (const char* name : {"QT_WIDGETS_RHI", "QT_WIDGETS_HIGHDPI_DOWNSCALE"}) {
        if (qEnvironmentVariableIntValue(name)) {
            enabling << QString::fromLatin1(name);
        }
    }
    if (enabling.isEmpty()) {
        return {};
    }
#if defined(Q_OS_WIN) || QT_CONFIG(metal)
    bool openGL = false;
#else
    bool openGL = true;
#endif
    QStringList otherApis;
#if QT_CONFIG(vulkan)
    otherApis << qsl("vulkan");
#endif
#if defined(Q_OS_WIN)
    otherApis << qsl("d3d11") << qsl("d3d") << qsl("d3d12");
#endif
#if QT_CONFIG(metal)
    otherApis << qsl("metal");
#endif
    const QString backend = qEnvironmentVariable("QT_WIDGETS_RHI_BACKEND");
    const bool backendChoseAnotherApi = otherApis.contains(backend);
    if (backend == qsl("opengl") || backend == qsl("gl")) {
        openGL = true;
    } else if (backendChoseAnotherApi) {
        openGL = false;
    }
    if (openGL) {
        return {};
    }
    if (backendChoseAnotherApi) {
        enabling << qsl("QT_WIDGETS_RHI_BACKEND");
    }
    return enabling;
}

// QOpenGLWidget retries a context it could not create on every show, resize and paint, and each retry
// leaks the context that failed (QOpenGLWidgetPrivate::initialize() overwrites it), so ask only once
bool ModernGLWidget::contextCreationFailed() const
{
    return context() && !isValid();
}

bool ModernGLWidget::event(QEvent* event)
{
    // After a click Tab is the command line's too, for completion or the caret key, as with the 2D map; QWidget
    // would take it to move focus before keyPressEvent() sees it
    if (event->type() == QEvent::KeyPress && !mKeysNavigate) {
        auto* keyEvent = static_cast<QKeyEvent*>(event);
        if ((keyEvent->key() == Qt::Key_Tab || keyEvent->key() == Qt::Key_Backtab) && !(keyEvent->modifiers() & (Qt::ControlModifier | Qt::AltModifier))) {
            keyPressEvent(keyEvent);
            return true;
        }
    }
    // The keys go on to the command line, so it must also win the shortcuts it wins with focus of its own
    if (event->type() == QEvent::ShortcutOverride) {
        TCommandLine* pCommandLine = commandLine();
        if (pCommandLine && pCommandLine->claimsShortcutOverride(static_cast<QKeyEvent*>(event))) {
            event->accept();
            return true;
        }
    }
    if (event->type() == QEvent::ToolTip) {
        auto* helpEvent = static_cast<QHelpEvent*>(event);
        QString text;
        if (compassRect().contains(helpEvent->pos())) {
            //: Tooltip of the compass in the corner of the 3D map view
            text = tr("Turn the view north up");
        } else {
            // Over an area exit, the room it leads to and the area that room is in are named
            text = roomToolTip(destinationAt(helpEvent->pos()));
        }
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
        reportContextCreationFailure();
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
    reportContextCreationFailure();
}

void ModernGLWidget::paintEvent(QPaintEvent* event)
{
    if (contextCreationFailed()) {
        return;
    }
    QOpenGLWidget::paintEvent(event);
    if (contextCreationFailed()) {
        reportContextCreationFailure();
    }
}

void ModernGLWidget::showFailure(const QString& message, const QString& advice)
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
    const QString tryClassic = tr("Use setConfig(\"experiment.3dmap.modernmapper\", false) to try the classic 3D view; if that stays blank too, use the 2D map. Setting it back to true "
                                  "tries the modern one again.");
    mpFailureLabel->setText(qsl("%1\n\n%2").arg(message, advice.isEmpty() ? tryClassic : advice));
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

    const QColor color(backgroundColor());
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

    if (mShiftMode && followingPlayer()) {
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

    const MapStyle& style = mapStyle();
    scale = style.roomScale;
    zFlattening = style.roomFlattening;

    // Clear the screen
    const QColor color(backgroundColor());
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
    // Rooms well beyond the view's center recede into the background, so a tilted view reads near to far
    const float distance = mCameraController.getScale();
    shaderProgram->setUniformValue("uFogColor", QVector3D(color.redF(), color.greenF(), color.blueF()));
    shaderProgram->setUniformValue("uFogStart", distance * scmFogStart);
    shaderProgram->setUniformValue("uFogEnd", distance * scmFogEnd);
    shaderProgram->setUniformValue("uFogMax", scmFogMax);

    queueScene();

    // Execute all queued commands
    mRenderCommandQueue.executeAll(shaderProgram, &mGeometryManager, &mResourceManager, mVAO, mVertexBuffer, mColorBuffer, mNormalBuffer, mIndexBuffer, mTexCoordBuffer);

    shaderProgram->release();

    QPainter painter(this);

    // Draw map info using contributor manager
    QColor infoColor;
    if (color.lightness() > 127) {
        infoColor = QColor(Qt::black);
    } else {
        infoColor = QColor(Qt::white);
    }
    const QColor infoBackground = style.infoBackgroundColor(mpHost->mMapInfoBg);
    if (infoBackground != mpHost->mMapInfoBg && style.infoTextColor.isValid()) {
        infoColor = style.infoTextColor;
    }
    dlgMapper::paintMapInfo(mFrameTimer, painter, mpHost, mpMap, mRID, mAID, 0, infoColor, 10, 10, width(), mFontHeight, infoBackground, style.labelFontFamily);

    if (mRectangleSelecting) {
        painter.fillRect(mSelectionRectangle, QColor(190, 190, 190, 60));
    }
    paintCompass(painter);

    painter.end();
}

namespace {
// Texels across the textures drawn on room tops
constexpr int scmRoomTextureSize = 128;
// Past this many, textures this frame has not used yet are dropped to make room and remade if needed;
// once the frame itself has used this many, the rest are not drawn
constexpr qsizetype scmMaxRoomTextures = 512;

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

// Names and IDs drawn shorter than this on screen are left out, as unreadable
constexpr qreal scmMinimumTextPixels = 7.0;
// As on the 2D map, what is drawn on a round room's top is shrunk to fit inside its circle
constexpr float scmRoundRoomInset = 0.707f;

// Outlines' widths, as a part of a room's width: around the viewed level's unbordered, opaque rooms outside grid mode, and the one hovered over
constexpr float scmLevelOutline = 0.06f;
constexpr float scmHoverOutline = 0.14f;
// How strongly a level's floor plate shows
constexpr float scmFloorPlateOpacity = 0.07f;

// What is drawn on the tops of a set of rooms; each map is keyed by texture, so each is one draw call however many rooms share it
struct RoomDecals
{
    // On the ground the rooms stand on, which the rooms hide where they stand
    GeometryData shadows;
    QHash<GLuint, GeometryData> ground;
    GeometryData sheen;
    GeometryData selection;
    QHash<GLuint, GeometryData> symbols;
    QHash<GLuint, GeometryData> highlights;
    QHash<GLuint, GeometryData> text;
    QHash<GLuint, GeometryData> markers;
    GeometryData playerFill;
    GeometryData player;
};

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

GLuint ModernGLWidget::imageTexture(const QString& key, const std::function<QImage()>& makeImage)
{
    if (const GLuint texture = cachedRoomTexture(key)) {
        return texture;
    }
    if (mLabelTextureCache.imageTextureFailed(key) || !roomForAnotherTexture(key)) {
        return 0;
    }
    return addRoomTexture(key, makeImage());
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
GLuint ModernGLWidget::selectionTexture(const bool round)
{
    const QString key = round ? qsl("selection|round") : qsl("selection");
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
    if (round) {
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setPen(Qt::NoPen);
        painter.setBrush(gradient);
        painter.drawEllipse(QRectF(image.rect()));
    } else {
        painter.fillRect(image.rect(), gradient);
    }
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

    const int playerRoomId = mpMap->mRoomIdHash.value(mpMap->mProfileName);
    const MapStyle& style = mapStyle();
    const QColor background = backgroundColor();
    const QColor contrast = ExitAppearance::contrastColor(background);
    const RenderCommand::Shading roomShading = style.lit ? RenderCommand::Shading::Lit : RenderCommand::Shading::Unlit;

    const bool moreTransparent = mpHost->experimentEnabled(qsl("experiment.rendering.more-transparent"));
    const bool playerIcon = mpHost->experimentEnabled(qsl("experiment.3d-player-icon"));

    const float roomSize = 2.0f / scale;
    const float roomHeight = 2.0f / scale / zFlattening;
    const bool round = style.roundRooms(mpHost->mBubbleMode);
    const GeometryManager::InstanceShape roomShape = round ? GeometryManager::InstanceShape::Cylinder : GeometryManager::InstanceShape::Box;
    const float symbolSize = round ? roomSize * scmRoundRoomInset : roomSize;
    // As on the 2D map, a border of thickness 1 is this much of the room's width, and grows outward
    const float borderUnit = mpHost->mRoomBorderSize > 0.0 ? roomSize / static_cast<float>(mpHost->mRoomBorderSize) : 0.0f;
    const QVector3D screenRight = mCameraController.screenRight();
    const QVector3D screenUp = mCameraController.screenUp();
    const auto [groundRight, groundUp] = GeometryManager::groundAxes(screenRight, screenUp);

    // As on the 2D map: room IDs in place of symbols, bold, sized so that the area's longest fits
    const bool showRoomIds = mpHost->mShowRoomID;
    QFont idFont = style.labelFont(mpMap->mMapSymbolFont);
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
        idTexel = 0.9f * symbolSize / static_cast<float>(qMax(widest, idMetrics->height()));
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
    QFont nameFont = style.labelFont(mpMap->mMapSymbolFont);
    bool namesGivenFont = false;
    if (showRoomNames) {
        if (QFont font; font.fromString(mpMap->mUserData.value(ROOM_UI_NAMEFONT))) {
            nameFont = font;
            namesGivenFont = true;
        }
        nameFont.setItalic(false);
        nameFont.setUnderline(false);
        nameFont.setOverline(false);
        nameFont.setStrikeOut(false);
        nameFont.setBold(namesGivenFont || !style.roomNameColor.isValid());
        nameFont.setStyleStrategy(QFont::StyleStrategy(QFont::PreferNoShaping | QFont::PreferAntialias | QFont::PreferOutline));
        nameFont.setPointSizeF(scmTextTexelsPerRoom / 2.0);
    }
    struct RoomName
    {
        QVector3D center;
        QColor color;
        QString name;
        QVector4D tint;
        RoomDecals* decals;
    };
    QList<RoomName> roomNames;
    const float textTexel = roomSize / scmTextTexelsPerRoom;
    const float nameTexel = textTexel * (namesGivenFont ? 1.0f : style.roomNameScale);
    struct RoomIdText
    {
        int roomId = 0;
        bool onViewedLevel = false;
        QVector3D center;
        QList<std::pair<GLuint, QSize>> digits;
        int width = 0;
        int height = 0;
        QVector4D tint;
        RoomDecals* decals = nullptr;
    };
    QList<RoomIdText> roomIdTexts;
    mRoomIdsDrawn.clear();
    struct RoomSymbol
    {
        int roomId = 0;
        QString symbol;
        QColor color;
        QVector3D center;
        QVector4D tint;
        RoomDecals* decals = nullptr;
    };
    const auto queueSymbol = [this, symbolSize, &screenRight, &screenUp](const RoomSymbol& roomSymbol) {
        if (const GLuint texture = symbolTexture(roomSymbol.symbol, roomSymbol.color)) {
            GeometryData& batch = roomSymbol.decals->symbols[texture];
            batch.textureId = texture;
            GeometryManager::appendGroundQuad(batch, roomSymbol.center, symbolSize, screenRight, screenUp, roomSymbol.tint);
        }
    };
    // As on the 2D map, a room shows its symbol where its ID is not drawn
    QList<RoomSymbol> symbolsInPlaceOfIds;

    // Made before any symbol, so a view short of room for textures still shows where the player is
    GLuint playerRoomTexture = 0;
    GLuint playerRoomFillTexture = 0;
    if (pArea->getAreaRooms().contains(playerRoomId)) {
        // The strong highlight is the original style over a translucent red room
        const int style = mpHost->mMapStrongHighlight ? 0 : mpMap->mPlayerRoomStyle;
        if (mapStyle().themesPlayerMarker(mpMap->mPlayerRoomStyle, mpHost->mMapStrongHighlight) && mapStyle().playerRing == MapStyle::PlayerRing::Thin) {
            const QColor ringColor = playerMarkerColor();
            playerRoomTexture = discTexture(qsl("playerThin|%1").arg(ringColor.name(QColor::HexArgb)), MapDecals::thinPlayerRingStops(ringColor));
        } else {
            const QString key = qsl("player|%1|%2|%3|%4")
                                        .arg(QString::number(style),
                                             QString::number(mpMap->mPlayerRoomInnerDiameterPercentage),
                                             mpMap->mPlayerRoomInnerColor.name(QColor::HexArgb),
                                             mpMap->mPlayerRoomOuterColor.name(QColor::HexArgb));
            playerRoomTexture = discTexture(key, RoomAppearance::playerRoomStops(style, mpMap->mPlayerRoomInnerDiameterPercentage, mpMap->mPlayerRoomInnerColor, mpMap->mPlayerRoomOuterColor));
        }
        if (mpHost->mMapStrongHighlight) {
            const QColor fill = RoomAppearance::strongHighlightFillColor();
            playerRoomFillTexture = round ? discTexture(qsl("disc|%1").arg(fill.name(QColor::HexArgb)), {{0.0, fill}, {1.0, fill}}) : squareTexture(fill);
        }
    }

    QVector<CubeInstanceData> roomInstances;
    QVector<CubeInstanceData> borderInstances;
    QVector<CubeInstanceData> outlineInstances;
    // What is on the opaque rooms, by level, drawn with the see-through rooms of its level
    QMap<int, RoomDecals> opaqueDecals;
    // Rooms and borders drawn see-through, with what is on them, by level: drawn after the opaque rooms
    struct SeeThroughLevel
    {
        QVector<CubeInstanceData> cubes;
        RoomDecals decals;
    };
    QMap<int, SeeThroughLevel> seeThroughLevels;
    // Each level's extent, for its floor plate
    QMap<int, QRectF> levelExtents;
    const QSet<int> emptySelection;
    const QSet<int>& selection = mp2DMap ? mp2DMap->mMultiSelectionSet : emptySelection;
    const GLuint selectionTextureId = selection.isEmpty() ? 0 : selectionTexture(round);
    GLuint sheenTexture = 0;
    GLuint shadowTexture = 0;
    if (style.shadedBlocks) {
        sheenTexture = imageTexture(qsl("sheen|%1").arg(round), [round] {
            return MapDecals::blockSheen(round, scmRoomTextureSize);
        });
        shadowTexture = imageTexture(qsl("shadow|%1").arg(round), [round] {
            return MapDecals::blockShadow(round, scmRoomTextureSize);
        });
    }
    // A decal lying flat on the map, its image's first row to the north whichever way the view faces
    const auto appendMapQuad = [](GeometryData& batch, const QVector3D& center, const float size, const QVector4D& tint) {
        GeometryManager::appendQuad(batch, center - QVector3D(size / 2.0f, size / 2.0f, 0.0f), QVector3D(size, 0.0f, 0.0f), QVector3D(0.0f, size, 0.0f), tint);
    };
    mHoveredRoomId = roomUnderPointer();

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
        const bool hovered = currentRoomId == mHoveredRoomId;
        auto rz = static_cast<float>(pR->z()) + (hovered ? roomHeight : 0.0f);
        const QRectF cell(rx - 0.5f, ry - 0.5f, 1.0, 1.0);
        const auto extent = levelExtents.find(pR->z());
        if (extent == levelExtents.end()) {
            levelExtents.insert(pR->z(), cell);
        } else {
            *extent = extent->united(cell);
        }

        const RoomAppearance::Look look = RoomAppearance::look(*mpMap, *mpHost, *pR, playerRoomId, mMapCenterZ);
        const GeometryManager::LevelFade fade = GeometryManager::levelFade(look.levelOffset, moreTransparent);

        const QColor fadeTarget = levelFadeTarget(pR->z());

        const QColor roomColor = style.roomColor(look.color);
        QVector4D roomTint = GeometryManager::fadedColor(roomColor, fade, fadeTarget);
        if (look.playerRoom && !playerRoomTexture) {
            // Without its marker the player's room is drawn solid red, so it still stands out
            roomTint = QVector4D(1.0f, 0.0f, 0.0f, 1.0f);
        }
        const bool seeThrough = roomTint.w() < 1.0f;
        QVector<CubeInstanceData>& instances = seeThrough ? seeThroughLevels[pR->z()].cubes : roomInstances;
        RoomDecals& decals = seeThrough ? seeThroughLevels[pR->z()].decals : opaqueDecals[pR->z()];
        // An outline is a lower, wider block, so that its top shows only as a rim around the room
        const auto outline = [&](const float width, const QVector4D& tint, const float height = 0.6f) {
            const float spread = (roomSize + 2.0f * width) / roomSize;
            QMatrix4x4 outlineTransform;
            outlineTransform.translate(rx, ry, rz);
            outlineTransform.scale(spread / scale, spread / scale, height / scale / zFlattening);
            return CubeInstanceData(outlineTransform, tint.x(), tint.y(), tint.z(), tint.w());
        };

        const bool bordered = (mpHost->mMapperShowRoomBorders || pR->mBorderColor.isValid() || pR->mBorderThickness > 0) && !pArea->gridMode;
        if (hovered) {
            // Taller than a border, so that it shows over one
            outlineInstances.append(outline(roomSize * scmHoverOutline, QVector4D(contrast.redF(), contrast.greenF(), contrast.blueF(), 1.0f), 0.75f));
        } else if (!bordered && !pArea->gridMode && look.onViewedLevel() && !seeThrough) {
            // Halfway from the room's color to the one that stands out on the background, or for a base to the background itself
            const QColor rimTarget = style.edges == MapStyle::Edges::ViewedLevelBase ? background : contrast;
            outlineInstances.append(outline(roomSize * scmLevelOutline, GeometryManager::fadedColor(roomColor, {0.5f, 1.0f}, rimTarget)));
        }
        if (bordered) {
            const float borderWidth = (pR->mBorderThickness > 0 ? pR->mBorderThickness : 1) * borderUnit;
            const QColor borderColor = pR->mBorderColor.isValid() ? pR->mBorderColor : style.roomBorderColor(mpHost->mRoomBorderColor, roomColor, background);
            const QVector4D borderTint = GeometryManager::fadedColor(borderColor, fade, fadeTarget);
            (borderTint.w() < 1.0f ? seeThroughLevels[pR->z()].cubes : borderInstances).append(outline(borderWidth, borderTint));
        }

        QMatrix4x4 transform;
        transform.translate(rx, ry, rz);
        transform.scale(1.0f / scale, 1.0f / scale, 1.0f / scale / zFlattening);
        instances.append(CubeInstanceData(transform, roomTint.x(), roomTint.y(), roomTint.z(), roomTint.w()));

        const QVector3D roomTop(rx, ry, rz + roomHeight / 2.0f);
        const QVector3D roomFoot(rx, ry, static_cast<float>(pR->z()) - roomHeight / 2.0f);
        const QVector4D decalTint = GeometryManager::fadedDecalTint(fade);

        if (sheenTexture) {
            decals.sheen.textureId = sheenTexture;
            appendMapQuad(decals.sheen, roomTop, roomSize, decalTint);
        }
        if (shadowTexture) {
            decals.shadows.textureId = shadowTexture;
            appendMapQuad(decals.shadows, roomFoot + QVector3D(roomSize * 0.3f, -roomSize * 0.3f, 0.0f), roomSize * 1.5f, decalTint);
        }

        if (selectionTextureId && selection.contains(currentRoomId)) {
            decals.selection.textureId = selectionTextureId;
            GeometryManager::appendGroundQuad(decals.selection, roomTop, roomSize, screenRight, screenUp, decalTint);
        }

        if (!look.symbol.isEmpty()) {
            const RoomSymbol roomSymbol{currentRoomId, look.symbol, RoomAppearance::symbolColor(look.symbolColor, roomColor), roomTop, decalTint, &decals};
            if (showRoomIds) {
                symbolsInPlaceOfIds.append(roomSymbol);
            } else {
                queueSymbol(roomSymbol);
            }
        }

        if (look.highlighted) {
            const MapStyle::HighlightShape shape = style.highlightShape;
            const bool disc = shape == MapStyle::HighlightShape::Disc;
            const QColor color = style.highlightColor.isValid() && !disc ? style.highlightColor : look.highlightColor;
            const QColor centerColor = disc ? look.highlightCenterColor : color;
            const QString key = qsl("highlight|%1|%2|%3|%4").arg(QString::number(static_cast<int>(shape)), color.name(QColor::HexArgb), centerColor.name(QColor::HexArgb), QString::number(round));
            if (const GLuint texture = imageTexture(key, [shape, color, centerColor, round] {
                    return MapDecals::highlight(shape, color, centerColor, round, scmRoomTextureSize);
                })) {
                const bool onGround = MapDecals::highlightOnGround(shape);
                GeometryData& batch = (onGround ? decals.ground : decals.highlights)[texture];
                batch.textureId = texture;
                // highlightRadius is in rooms' spacing, which is one unit here
                const float size = MapDecals::highlightFillsRoom(shape) ? roomSize : look.highlightRadius * style.highlightScale;
                appendMapQuad(batch, onGround ? roomFoot : roomTop, size, decalTint);
            }
        }

        if (showRoomIds) {
            const QColor idColor = ExitAppearance::contrastColor(roomColor);
            const QString number = QString::number(currentRoomId);
            RoomIdText idText{currentRoomId, look.onViewedLevel(), roomTop, {}, 0, 0, decalTint, &decals};
            bool whole = true;
            for (const QChar digit : number) {
                const std::pair<GLuint, QSize> found = digitTextures.value({idColor.rgba(), digit.unicode()});
                whole = whole && found.first;
                idText.digits.append(found);
                idText.width += found.second.width();
                idText.height = qMax(idText.height, found.second.height());
            }
            // None rather than a number with digits missing, which would read as another room's
            if (whole) {
                roomIdTexts.append(idText);
            }
        }

        // Only the viewed level's, as only the 2D map's are
        if (showRoomNames && look.onViewedLevel() && !pR->name.isEmpty()) {
            // One room's size below the room on screen, then moved by its offset along the map, in rooms' spacing, y downward
            const QPointF offset = RoomAppearance::nameOffset(pR->userData.value(ROOM_UI_NAMEPOS));
            const QVector3D center = roomTop - groundUp * roomSize + QVector3D(static_cast<float>(offset.x()), -static_cast<float>(offset.y()), 0.0f);
            roomNames.append({center, style.roomNameColorFor(pR->userData.value(ROOM_UI_NAMECOLOR), background), pR->name, decalTint, &decals});
        }

        if (const GLuint texture = exitMarkerTexture(*pR, roomColor)) {
            GeometryData& batch = decals.markers[texture];
            batch.textureId = texture;
            GeometryManager::appendGroundQuad(batch, roomTop, roomSize, screenRight, screenUp, decalTint);
        }

        if (look.playerRoom) {
            if (playerRoomFillTexture) {
                decals.playerFill.textureId = playerRoomFillTexture;
                GeometryManager::appendGroundQuad(decals.playerFill, roomTop, roomSize, screenRight, screenUp, decalTint);
            }
            if (playerRoomTexture) {
                decals.player.textureId = playerRoomTexture;
                const float radius = static_cast<float>(RoomAppearance::playerRoomRadius(mpMap->mPlayerRoomOuterDiameterPercentage, roomSize + 2.0f * borderUnit));
                GeometryManager::appendGroundQuad(decals.player, roomTop, 2.0f * radius, screenRight, screenUp, decalTint);
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

    const QMatrix4x4 viewModel = mCameraController.getViewMatrix() * mCameraController.getModelMatrix();
    const QRectF textBounds = QRectF(rect()).adjusted(-100.0, -100.0, 100.0, 100.0);
    // Where text lying flat on the map shows on screen. An invalid rectangle, for text partly behind the camera or
    // off screen, is never kept; text nearly edge on can span millions of pixels, so each is cut down to about the view
    const auto screenRect = [this, &textBounds](const QVector3D& corner, const QVector3D& across, const QVector3D& upward) {
        qreal left = std::numeric_limits<qreal>::max();
        qreal top = std::numeric_limits<qreal>::max();
        qreal right = std::numeric_limits<qreal>::lowest();
        qreal bottom = std::numeric_limits<qreal>::lowest();
        for (const QVector3D& point : {corner, corner + across, corner + upward, corner + across + upward}) {
            const std::optional<QPointF> pixel = mCameraController.pixelForPoint(point);
            if (!pixel) {
                return QRectF();
            }
            left = qMin(left, pixel->x());
            top = qMin(top, pixel->y());
            right = qMax(right, pixel->x());
            bottom = qMax(bottom, pixel->y());
        }
        const QRectF rect(QPointF(left, top), QPointF(right, bottom));
        return rect.intersects(textBounds) ? rect.intersected(textBounds) : QRectF();
    };

    if (!roomIdTexts.isEmpty()) {
        // The viewed level's first, the only ones the 2D map shows, then nearest first, so where IDs overlap the one in front is kept
        std::stable_sort(roomIdTexts.begin(), roomIdTexts.end(), [&viewModel](const RoomIdText& a, const RoomIdText& b) {
            if (a.onViewedLevel != b.onViewedLevel) {
                return a.onViewedLevel;
            }
            return viewModel.map(a.center).z() > viewModel.map(b.center).z();
        });
        QList<QRectF> idRects;
        idRects.reserve(roomIdTexts.size());
        for (const RoomIdText& idText : std::as_const(roomIdTexts)) {
            const QVector3D across = groundRight * (idText.width * idTexel);
            const QVector3D upward = groundUp * (idText.height * idTexel);
            idRects.append(screenRect(idText.center - across / 2.0f - upward / 2.0f, across, upward));
        }
        for (const qsizetype index : GeometryManager::declutteredRects(idRects, scmMinimumTextPixels)) {
            const RoomIdText& idText = roomIdTexts.at(index);
            QVector3D corner = idText.center - groundRight * (idText.width * idTexel / 2.0f);
            for (const auto& [texture, size] : idText.digits) {
                const QVector3D across = groundRight * (size.width() * idTexel);
                const QVector3D upward = groundUp * (size.height() * idTexel);
                GeometryData& batch = idText.decals->text[texture];
                batch.textureId = texture;
                GeometryManager::appendQuad(batch, corner - upward / 2.0f, across, upward, idText.tint);
                corner += across;
            }
            mRoomIdsDrawn.insert(idText.roomId);
        }
    }
    for (const RoomSymbol& roomSymbol : std::as_const(symbolsInPlaceOfIds)) {
        if (!mRoomIdsDrawn.contains(roomSymbol.roomId)) {
            queueSymbol(roomSymbol);
        }
    }

    // Names come after every symbol and marker, so that a view short of room for textures loses names first
    if (!roomNames.isEmpty()) {
        const QString nameFontKey = nameFont.key();
        const QFontMetricsF nameMetrics = imageFontMetrics(nameFont);
        // Nearest first, so that where names overlap the one in front is the one kept
        std::stable_sort(roomNames.begin(), roomNames.end(), [&viewModel](const RoomName& a, const RoomName& b) {
            return viewModel.map(a.center).z() > viewModel.map(b.center).z();
        });
        QList<QRectF> nameRects;
        nameRects.reserve(roomNames.size());
        for (const RoomName& roomName : std::as_const(roomNames)) {
            const QSizeF size(qMax(1, qCeil(nameMetrics.horizontalAdvance(roomName.name))), qMax(1, qCeil(nameMetrics.height())));
            const QVector3D across = groundRight * static_cast<float>(size.width() * nameTexel);
            const QVector3D upward = groundUp * static_cast<float>(size.height() * nameTexel);
            nameRects.append(screenRect(roomName.center - across / 2.0f - upward / 2.0f, across, upward));
        }
        for (const qsizetype index : GeometryManager::declutteredRects(nameRects, scmMinimumTextPixels)) {
            const RoomName& roomName = roomNames.at(index);
            QSize size;
            const QString key = qsl("name|%1|%2|%3").arg(roomName.color.name(QColor::HexArgb), nameFontKey, roomName.name);
            if (const GLuint texture = textTexture(key, roomName.name, nameFont, nameMetrics, roomName.color, size)) {
                const QVector3D across = groundRight * (size.width() * nameTexel);
                const QVector3D upward = groundUp * (size.height() * nameTexel);
                GeometryData& batch = roomName.decals->text[texture];
                batch.textureId = texture;
                GeometryManager::appendQuad(batch, roomName.center - across / 2.0f - upward / 2.0f, across, upward, roomName.tint);
            }
        }
    }

    if (!borderInstances.isEmpty()) {
        mRenderCommandQueue.addCommand(std::make_unique<RenderInstancedCubesCommand>(
                borderInstances, mCameraController.getProjectionMatrix(), mCameraController.getViewMatrix(), mCameraController.getModelMatrix(), roomShading, roomShape));
    }
    if (!outlineInstances.isEmpty()) {
        mRenderCommandQueue.addCommand(std::make_unique<RenderInstancedCubesCommand>(
                outlineInstances, mCameraController.getProjectionMatrix(), mCameraController.getViewMatrix(), mCameraController.getModelMatrix(), RenderCommand::Shading::Unlit, roomShape));
    }
    if (!roomInstances.isEmpty()) {
        mRenderCommandQueue.addCommand(std::make_unique<RenderInstancedCubesCommand>(
                roomInstances, mCameraController.getProjectionMatrix(), mCameraController.getViewMatrix(), mCameraController.getModelMatrix(), roomShading, roomShape));
    }

    // In the 2D map's order: outside grid mode a highlight is drawn over the symbol, in grid mode under it;
    // then the ID and name, the exit markers and the player's ring. Drawn on what is already there without
    // writing depth, as a set of rooms' decals do not hide each other
    const auto queueDecals = [this, pArea](const RoomDecals& decals) {
        mRenderCommandQueue.addCommand(std::make_unique<GLStateCommand>(GLStateCommand::ENABLE_POLYGON_OFFSET));
        const auto queue = [this](const GeometryData& batch) {
            if (!batch.isEmpty()) {
                mRenderCommandQueue.addCommand(std::make_unique<RenderTexturedTrianglesCommand>(
                        batch, mCameraController.getProjectionMatrix(), mCameraController.getViewMatrix(), mCameraController.getModelMatrix(), RenderCommand::Shading::Unlit));
            }
        };
        queue(decals.shadows);
        for (const GeometryData& batch : decals.ground) {
            queue(batch);
        }
        queue(decals.sheen);
        // The selection shades the room itself, as the 2D map's does, so it is under everything drawn on it
        queue(decals.selection);
        const QHash<GLuint, GeometryData>& lower = pArea->gridMode ? decals.highlights : decals.symbols;
        const QHash<GLuint, GeometryData>& upper = pArea->gridMode ? decals.symbols : decals.highlights;
        for (const QHash<GLuint, GeometryData>* layer : {&lower, &upper, &decals.text, &decals.markers}) {
            for (const GeometryData& batch : *layer) {
                queue(batch);
            }
        }
        queue(decals.playerFill);
        queue(decals.player);
        mRenderCommandQueue.addCommand(std::make_unique<GLStateCommand>(GLStateCommand::DISABLE_POLYGON_OFFSET));
    };

    QColor plateColor = contrast;
    plateColor.setAlphaF(scmFloorPlateOpacity);
    const auto queuePlate = [&](const int level) {
        const auto extent = levelExtents.constFind(level);
        if (extent == levelExtents.cend()) {
            return;
        }
        const QVector4D tint = GeometryManager::fadedColor(plateColor, GeometryManager::levelFade(level - mMapCenterZ, moreTransparent), levelFadeTarget(level));
        const float plateZ = static_cast<float>(level) - roomHeight;
        QVector<float> vertices;
        QVector<float> colors;
        for (const QPointF& corner : {extent->topLeft(), extent->topRight(), extent->bottomRight(), extent->topLeft(), extent->bottomRight(), extent->bottomLeft()}) {
            vertices << static_cast<float>(corner.x()) << static_cast<float>(corner.y()) << plateZ;
            colors << tint.x() << tint.y() << tint.z() << tint.w();
        }
        mRenderCommandQueue.addCommand(std::make_unique<RenderTrianglesCommand>(
                vertices, colors, mCameraController.getProjectionMatrix(), mCameraController.getViewMatrix(), mCameraController.getModelMatrix(), RenderCommand::Shading::Unlit));
    };

    // Everything from here on is drawn over what is behind it, so it does not write depth unless it says so
    mRenderCommandQueue.addCommand(std::make_unique<GLStateCommand>(GLStateCommand::DISABLE_DEPTH_WRITE));

    // From the lowest level up, as each blends over the ones under it: its faint floor, its see-through exits, rooms
    // and borders, what is on its rooms, then its labels shown on top. Every level's labels come before the next
    // level up lays its depth, which would cut them out where it covers them
    QList<int> levels = levelExtents.keys() + mSeeThroughExits.keys() + seeThroughLevels.keys() + opaqueDecals.keys();
    std::sort(levels.begin(), levels.end());
    levels.erase(std::unique(levels.begin(), levels.end()), levels.end());
    qint64 unlabeledFrom = std::numeric_limits<int>::lowest();
    for (const int level : std::as_const(levels)) {
        // Those on levels with nothing else shown, under this one
        if (level > unlabeledFrom) {
            renderLabels(true, static_cast<int>(unlabeledFrom), level - 1);
        }
        queuePlate(level);
        if (const auto exits = mSeeThroughExits.constFind(level); exits != mSeeThroughExits.cend()) {
            if (!exits->bars.isEmpty()) {
                mRenderCommandQueue.addCommand(std::make_unique<RenderInstancedCubesCommand>(
                        exits->bars, mCameraController.getProjectionMatrix(), mCameraController.getViewMatrix(), mCameraController.getModelMatrix(), RenderCommand::Shading::Unlit));
            }
            if (!exits->vertices.isEmpty()) {
                // Arrowheads and dots lie on the lines' tops
                mRenderCommandQueue.addCommand(std::make_unique<GLStateCommand>(GLStateCommand::ENABLE_POLYGON_OFFSET));
                mRenderCommandQueue.addCommand(std::make_unique<RenderTrianglesCommand>(
                        exits->vertices, exits->colors, mCameraController.getProjectionMatrix(), mCameraController.getViewMatrix(), mCameraController.getModelMatrix(), RenderCommand::Shading::Unlit));
                mRenderCommandQueue.addCommand(std::make_unique<GLStateCommand>(GLStateCommand::DISABLE_POLYGON_OFFSET));
            }
        }
        const auto seeThrough = seeThroughLevels.constFind(level);
        if (seeThrough != seeThroughLevels.cend()) {
            if (!seeThrough->cubes.isEmpty()) {
                // Depth first, then color only where that depth is: each pixel blends just the nearest face, not
                // a block's far faces too, nor a border's top through the room on it
                mRenderCommandQueue.addCommand(std::make_unique<GLStateCommand>(GLStateCommand::ENABLE_DEPTH_WRITE));
                mRenderCommandQueue.addCommand(std::make_unique<GLStateCommand>(GLStateCommand::DISABLE_COLOR_WRITE));
                mRenderCommandQueue.addCommand(std::make_unique<RenderInstancedCubesCommand>(
                        seeThrough->cubes, mCameraController.getProjectionMatrix(), mCameraController.getViewMatrix(), mCameraController.getModelMatrix(), roomShading, roomShape));
                mRenderCommandQueue.addCommand(std::make_unique<GLStateCommand>(GLStateCommand::ENABLE_COLOR_WRITE));
                mRenderCommandQueue.addCommand(std::make_unique<GLStateCommand>(GLStateCommand::DISABLE_DEPTH_WRITE));
                mRenderCommandQueue.addCommand(std::make_unique<GLStateCommand>(GLStateCommand::DEPTH_FUNC_LEQUAL));
                mRenderCommandQueue.addCommand(std::make_unique<RenderInstancedCubesCommand>(
                        seeThrough->cubes, mCameraController.getProjectionMatrix(), mCameraController.getViewMatrix(), mCameraController.getModelMatrix(), roomShading, roomShape));
                mRenderCommandQueue.addCommand(std::make_unique<GLStateCommand>(GLStateCommand::DEPTH_FUNC_LESS));
            }
        }
        if (const auto decals = opaqueDecals.constFind(level); decals != opaqueDecals.cend()) {
            queueDecals(*decals);
        }
        if (seeThrough != seeThroughLevels.cend()) {
            queueDecals(seeThrough->decals);
        }
        renderLabels(true, level, level);
        unlabeledFrom = qint64{level} + 1;
    }
    if (unlabeledFrom <= std::numeric_limits<int>::max()) {
        renderLabels(true, static_cast<int>(unlabeledFrom), std::numeric_limits<int>::max());
    }
    mRenderCommandQueue.addCommand(std::make_unique<GLStateCommand>(GLStateCommand::ENABLE_DEPTH_WRITE));
    mSeeThroughExits.clear();
}

ExitScene::Options ModernGLWidget::exitSceneOptions() const
{
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
    options.dashedStairs = mapStyle().dashedStairs;
    options.exitColor = mpHost->mFgColor_2;
    options.background = backgroundColor();
    return options;
}

QColor ModernGLWidget::levelFadeTarget(const int z) const
{
    if (!mudlet::self() || !mudlet::self()->drawUpperLowerLevels()) {
        return backgroundColor();
    }
    return GeometryManager::levelFadeTarget(z - mMapCenterZ, backgroundColor(), mpHost->mUpperLevelColor, mpHost->mLowerLevelColor);
}

const MapStyle& ModernGLWidget::mapStyle() const
{
    return mpHost ? mpHost->mapStylePreset() : MapStyle::findOrDefault(MapStyle::defaultId());
}

QColor ModernGLWidget::backgroundColor() const
{
    return mapStyle().backgroundColor(mpHost->mBgColor_2);
}

QColor ModernGLWidget::playerMarkerColor() const
{
    const MapStyle& style = mapStyle();
    if (style.playerColor.isValid() && style.themesPlayerMarker(mpMap->mPlayerRoomStyle, mpHost->mMapStrongHighlight)) {
        return style.playerColor;
    }
    return RoomAppearance::playerBeaconColor(mpHost->mMapStrongHighlight ? 0 : mpMap->mPlayerRoomStyle, mpMap->mPlayerRoomOuterColor);
}

void ModernGLWidget::queueGrid()
{
    if (!mpHost || !mpHost->mMapperShowGrid || mpHost->mMapGridColor.alpha() == 0 || !mpMap || !mpMap->mpRoomDB || !mpMap->mpRoomDB->getArea(mAID)) {
        return;
    }
    const float roomHeight = 2.0f / scale / zFlattening;
    // At the rooms' bottoms, so they stand on it
    const float gridZ = static_cast<float>(mMapCenterZ) - roomHeight / 2.0f;
    const QVector3D target = mCameraController.getTarget();
    const QPointF center(target.x(), target.y());
    // Roughly where the fog ends, past which the grid would be mostly noise. The fog is measured in eye distances,
    // and the camera's own distance is not in map units
    const qreal eyeDistance = mCameraController.rayThroughPixel(QPointF(width() / 2.0, height() / 2.0)).first.distanceToPoint(target);
    const qreal reach = eyeDistance * scmFogEnd;
    // Not QRectF::united(), which drops empty rectangles such as points
    QPointF lowest = center;
    QPointF highest = center;
    const qreal w = width();
    const qreal h = height();
    for (const QPointF& pixel : {QPointF(0.0, 0.0), QPointF(w / 2.0, 0.0), QPointF(w, 0.0), QPointF(w, h / 2.0), QPointF(w, h), QPointF(w / 2.0, h), QPointF(0.0, h), QPointF(0.0, h / 2.0)}) {
        const auto [origin, direction] = mCameraController.rayThroughPixel(pixel);
        QPointF reached;
        const float distance = std::abs(direction.z()) < 1e-6f ? -1.0f : (gridZ - origin.z()) / direction.z();
        if (distance > 0.0f) {
            const QVector3D point = origin + direction * distance;
            reached = QPointF(point.x(), point.y());
        } else {
            // Above the horizon: as far as the grid shows, that way
            reached = center + QPointF(direction.x(), direction.y()) * reach;
        }
        QLineF toReached(center, reached);
        if (toReached.length() > reach) {
            toReached.setLength(reach);
        }
        lowest = QPointF(qMin(lowest.x(), toReached.p2().x()), qMin(lowest.y(), toReached.p2().y()));
        highest = QPointF(qMax(highest.x(), toReached.p2().x()), qMax(highest.y(), toReached.p2().y()));
    }
    const QList<QLineF> lines = GeometryManager::gridLines(QRectF(lowest, highest));
    if (lines.isEmpty()) {
        return;
    }

    // As on the 2D map, a line size of 50 is a room spacing (one map unit) wide; never thinner than a pixel where the view looks
    const float lineWidth = qMax(static_cast<float>(mpHost->mMapGridLineSize) / 50.0f, mCameraController.worldUnitsPerPixel(QVector3D(target.x(), target.y(), gridZ)));
    const QColor color = mpHost->mMapGridColor;
    QVector<float> vertices;
    QVector<float> colors;
    vertices.reserve(lines.size() * 18);
    colors.reserve(lines.size() * 24);
    for (const QLineF& line : lines) {
        const QPointF side = QPointF(-line.dy(), line.dx()) * (lineWidth / 2.0 / line.length());
        const QPointF a = line.p1() + side;
        const QPointF b = line.p2() + side;
        const QPointF c = line.p2() - side;
        const QPointF d = line.p1() - side;
        for (const QPointF& corner : {a, b, c, a, c, d}) {
            vertices << static_cast<float>(corner.x()) << static_cast<float>(corner.y()) << gridZ;
            colors << color.redF() << color.greenF() << color.blueF() << color.alphaF();
        }
    }
    // Writing no depth, so what is drawn after it, nearer or further, paints over it as on the 2D map
    mRenderCommandQueue.addCommand(std::make_unique<GLStateCommand>(GLStateCommand::DISABLE_DEPTH_WRITE));
    mRenderCommandQueue.addCommand(std::make_unique<RenderTrianglesCommand>(
            vertices, colors, mCameraController.getProjectionMatrix(), mCameraController.getViewMatrix(), mCameraController.getModelMatrix(), RenderCommand::Shading::Unlit));
    mRenderCommandQueue.addCommand(std::make_unique<GLStateCommand>(GLStateCommand::ENABLE_DEPTH_WRITE));
}

void ModernGLWidget::renderConnections()
{
    mSeeThroughExits.clear();
    if (!mpMap || !mpMap->mpRoomDB || !mpHost) {
        return;
    }

    const ExitScene::Scene scene = ExitScene::build(*mpMap, *mpHost, exitSceneOptions());
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
        return GeometryManager::fadedColor(color, GeometryManager::levelFade(level - mMapCenterZ, moreTransparent), levelFadeTarget(level));
    };

    mRenderCommandQueue.addCommand(std::make_unique<GLStateCommand>(GLStateCommand::ENABLE_DEPTH_TEST));

    QVector<CubeInstanceData> bars;
    bars.reserve(scene.segments.size());
    for (const ExitScene::Segment& segment : scene.segments) {
        const QVector4D tint = faded(segment.color, segment.level);
        const CubeInstanceData bar(GeometryManager::barTransform(segment.from, segment.to, segment.width, segment.height), tint.x(), tint.y(), tint.z(), tint.w());
        // Not drawn into the depth buffer, or it would cut its shape out of the levels below drawn after it
        (tint.w() < 1.0f ? mSeeThroughExits[segment.level].bars : bars).append(bar);
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
        LevelExits* seeThrough = tint.w() < 1.0f ? &mSeeThroughExits[triangle.level] : nullptr;
        for (const QVector3D& corner : {triangle.a, triangle.b, triangle.c}) {
            (seeThrough ? seeThrough->vertices : vertices) << corner.x() << corner.y() << corner.z();
            (seeThrough ? seeThrough->colors : colors) << tint.x() << tint.y() << tint.z() << tint.w();
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
    turnCamera(-3.0f);
}

void ModernGLWidget::slot_shiftCameraRight()
{
    turnCamera(3.0f);
}

void ModernGLWidget::turnCamera(const float degrees)
{
    const QVector3D position = mCameraController.getPosition();
    setOrientationKeepingRoll(position.y(), position.z() + degrees);
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
    if (!mShiftMode || !mpMap) {
        return true;
    }
    // Not while rooms are dragged, as they follow the pointer across the map and would go along with the view
    return !mMovingRooms && (mRID != mpMap->mRoomIdHash.value(mpMap->mProfileName) || mpMap->mNewMove);
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

// Where a ray first meets an upright cylinder, as a distance along it
std::optional<float> rayHitsCylinder(const QVector3D& origin, const QVector3D& direction, const QVector3D& center, const float radius, const float halfHeight)
{
    const float low = center.z() - halfHeight;
    const float high = center.z() + halfHeight;
    const auto withinHeight = [&](const float distance) {
        const float z = origin.z() + direction.z() * distance;
        return distance >= 0.0f && z >= low && z <= high;
    };
    const auto withinRadius = [&](const float distance) {
        const QVector3D point = origin + direction * distance - center;
        return distance >= 0.0f && point.x() * point.x() + point.y() * point.y() <= radius * radius;
    };
    std::optional<float> nearest;
    const auto consider = [&nearest](const float distance) {
        if (!nearest || distance < *nearest) {
            nearest = distance;
        }
    };
    if (std::abs(direction.z()) > 1e-9f) {
        for (const float capZ : {low, high}) {
            const float distance = (capZ - origin.z()) / direction.z();
            if (withinRadius(distance)) {
                consider(distance);
            }
        }
    }
    // The side: where the ray's path across the map comes within radius of the axis
    const float dx = direction.x();
    const float dy = direction.y();
    const float ox = origin.x() - center.x();
    const float oy = origin.y() - center.y();
    const float a = dx * dx + dy * dy;
    if (a > 1e-12f) {
        const float b = 2.0f * (ox * dx + oy * dy);
        const float c = ox * ox + oy * oy - radius * radius;
        const float discriminant = b * b - 4.0f * a * c;
        if (discriminant >= 0.0f) {
            const float root = std::sqrt(discriminant);
            for (const float distance : {(-b - root) / (2.0f * a), (-b + root) / (2.0f * a)}) {
                if (withinHeight(distance)) {
                    consider(distance);
                }
            }
        }
    }
    if (!nearest && withinRadius(0.0f) && withinHeight(0.0f)) {
        nearest = 0.0f;
    }
    return nearest;
}

// A room seen nearly edge on is a sliver, so one whose center is this close to the pointer counts as hit
constexpr qreal scmPickSlopPixels = 6.0;
} // namespace

int ModernGLWidget::roomAt(const QPoint& position, const bool orNear, const std::optional<int> onLevel)
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
    const bool round = mpHost && mapStyle().roundRooms(mpHost->mBubbleMode);
    int hitRoom = 0;
    float hitDistance = std::numeric_limits<float>::max();
    int nearRoom = 0;
    qreal nearPixels = scmPickSlopPixels;
    for (const int roomId : pArea->getAreaRooms()) {
        const TRoom* pR = mpMap->mpRoomDB->getRoom(roomId);
        if (!pR || pR->isHidden() || !levelShown(pR->z()) || (onLevel && pR->z() != *onLevel)) {
            continue;
        }
        // The room hovered over is drawn raised by its height. It is hit where it rests too, or the pointer on
        // its near side, which the raised room leaves bare, would drop and raise it again on every frame
        const float lift = roomId == mHoveredRoomId ? 2.0f * halfSize.z() : 0.0f;
        const QVector3D center(static_cast<float>(pR->x()), static_cast<float>(pR->y()), static_cast<float>(pR->z()) + lift);
        const QVector3D bottom = center - halfSize - QVector3D(0.0f, 0.0f, lift);
        const QVector3D top = center + halfSize;
        const std::optional<float> distance =
                round ? rayHitsCylinder(origin, direction, (bottom + top) / 2.0f, halfSize.x(), (top.z() - bottom.z()) / 2.0f) : rayHitsBox(origin, direction, bottom, top);
        if (distance) {
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

int ModernGLWidget::areaExitAt(const QPoint& position)
{
    if (!mpMap || !mpMap->mpRoomDB || !mpHost || !mpMap->mpRoomDB->getArea(mAID)) {
        return 0;
    }
    updateMatrices();
    const QPointF pointer(position);
    int nearestExit = 0;
    qreal nearestPixels = std::numeric_limits<qreal>::max();
    ExitScene::Options options = exitSceneOptions();
    options.areaExitsOnly = true;
    // Only the viewed level's, as only those are offered on the 2D map
    options.levelsAbove = 0;
    options.levelsBelow = 0;
    for (const ExitScene::AreaExit& areaExit : ExitScene::build(*mpMap, *mpHost, options).areaExits) {
        const std::optional<QPointF> from = mCameraController.pixelForPoint(areaExit.from);
        const std::optional<QPointF> to = mCameraController.pixelForPoint(areaExit.to);
        if (!from || !to) {
            continue;
        }
        // A third of a room spacing either side of the arrow, from its room's center to its tip, but never less
        // than the slop for a room. The 2D map takes a square of that size around one point a room out
        const qreal reach = qMax(scmPickSlopPixels, 1.0 / (3.0 * mCameraController.worldUnitsPerPixel(areaExit.to)));
        const QPointF along = *to - *from;
        const qreal lengthSquared = QPointF::dotProduct(along, along);
        const qreal t = lengthSquared > 0.0 ? std::clamp(QPointF::dotProduct(pointer - *from, along) / lengthSquared, 0.0, 1.0) : 0.0;
        const qreal pixels = QLineF(pointer, *from + along * t).length();
        if (pixels <= reach && pixels < nearestPixels) {
            nearestPixels = pixels;
            nearestExit = areaExit.destinationId;
        }
    }
    return nearestExit;
}

int ModernGLWidget::roomPickedAt(const QPoint& position, const bool orNear, const bool areaExits)
{
    if (const int viewedRoom = roomAt(position, false, mMapCenterZ)) {
        return viewedRoom;
    }
    if (areaExits) {
        // A room on another level, and a room only near the pointer, lose to the arrow
        if (const int exitDestination = areaExitAt(position)) {
            return exitDestination;
        }
    }
    return roomAt(position, orNear);
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
    const bool round = mpHost && mapStyle().roundRooms(mpHost->mBubbleMode);
    // The corners of a round room's drawn rim, top and bottom, which must all be inside, as its edges run between them
    constexpr int scmRimPoints = GeometryManager::scmCylinderSegments;
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
        // A round room can be whole inside even where its box's corners are not
        if (!inside && round) {
            inside = true;
            for (int step = 0; step < scmRimPoints * 2 && inside; ++step) {
                const float angle = 2.0f * std::numbers::pi_v<float> * static_cast<float>(step % scmRimPoints) / static_cast<float>(scmRimPoints);
                const QVector3D offset(halfSize.x() * std::cos(angle), halfSize.y() * std::sin(angle), step < scmRimPoints ? -halfSize.z() : halfSize.z());
                const std::optional<QPointF> pixel = mCameraController.pixelForPoint(center + offset);
                inside = pixel && bounds.contains(*pixel);
            }
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

void ModernGLWidget::endLostPress()
{
    endPan();
    const bool rectangleShown = mRectangleSelecting;
    endEditGesture();
    if (rectangleShown) {
        update();
    }
    mClickPending = false;
    if (std::exchange(mPressEventButton, Qt::NoButton) != Qt::NoButton) {
        // Queued, as a script handling it could delete this view
        QTimer::singleShot(0, this, &ModernGLWidget::raiseMousePressEvent);
    }
}

void ModernGLWidget::raiseMousePressEventMidPress()
{
    const QWidget* popup = QApplication::activePopupWidget();
    const Qt::MouseButtons held = QGuiApplication::mouseButtons();
    raiseMousePressEvent();
    // A script handling it may open a popup or a dialog, which takes the release, and no Leave reaches this view
    // while the popup is open or once the button is up
    if (QApplication::activePopupWidget() != popup || (held & ~QGuiApplication::mouseButtons()) != Qt::MouseButtons()) {
        endLostPress();
    }
}

void ModernGLWidget::raisePendingMousePressEvent()
{
    if (std::exchange(mPressEventButton, Qt::NoButton) != Qt::NoButton) {
        raiseMousePressEvent();
    }
}

TCommandLine* ModernGLWidget::commandLine() const
{
    TMainConsole* console = mpHost ? mpHost->mainConsoleView() : nullptr;
    return console ? console->mpCommandLine.data() : nullptr;
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

    // Announced before this one, as with the 2D map, which announces each press as it comes
    raisePendingMousePressEvent();
    // A script handling it may have hidden the view, whose hideEvent() has ended this press too
    if (!isVisible()) {
        return;
    }
    mKeysNavigate = false;
    mPressPosition = event->position().toPoint();
    mPointerPosition = mPressPosition;
    mPointerInView = true;
    mPointerButtons = event->buttons();
    mClickPending = false;
    // A release that went elsewhere, such as to a menu opened mid-drag, must not leave the last drag running
    endEditGesture();
    if (event->button() == Qt::LeftButton && compassRect().contains(mPressPosition)) {
        turnNorthUp();
        return;
    }
    // As on the 2D map in editing mode: a drag from a room moves the selection and one from empty space,
    // or with Ctrl or Shift, draws a selection rectangle; Alt keeps the viewing mode's drags to pan and orbit
    if (event->button() == Qt::LeftButton && editingMap() && !event->modifiers().testFlag(Qt::AltModifier)) {
        // Picked before a glide stops, as stopping it snaps the view to whole grid steps away from what was seen.
        // Only a room right under the pointer is picked up, so a drag beside rooms seen small draws a rectangle
        const int roomId = roomPickedAt(mPressPosition, false);
        const bool moveRoom = roomId && !(event->modifiers() & (Qt::ControlModifier | Qt::ShiftModifier));
        const int rectangleRoomId = roomId ? roomId : roomPickedAt(mPressPosition);
        const TRoom* pRoom = moveRoom ? mpMap->mpRoomDB->getRoom(roomId) : nullptr;
        QPointF snap;
        if (mCameraSmoothAnimating) {
            const std::optional<QPointF> seenAt = pRoom ? mapPointOnLevel(mPressPosition, pRoom->z()) : std::nullopt;
            // Stopped where it is and held there, so neither a drag nor a glide after the player moves it on
            stopSmoothTransition();
            mMapCenterX = qRound(mCurrentAnimationX);
            mMapCenterY = qRound(mCurrentAnimationY);
            mMapCenterZ = qRound(mCurrentAnimationZ);
            holdView();
            const std::optional<QPointF> stoppedAt = pRoom ? mapPointOnLevel(mPressPosition, pRoom->z()) : std::nullopt;
            if (seenAt && stoppedAt) {
                snap = *stoppedAt - *seenAt;
            }
        }
        if (moveRoom) {
            startRoomMove(roomId);
            // The pointer keeps its place on the room, though the view stopping on a whole step moved under it
            mRoomMoveLastPoint += snap;
        } else {
            mp2DMap->beginRectangleSelection(rectangleRoomId, event->modifiers());
            mRectangleSelecting = true;
            mRectangleDragged = false;
            mSelectionRectangle = QRect(mPressPosition, QSize());
            update();
        }
        raiseMousePressEventMidPress();
        return;
    }
    if (event->button() == Qt::LeftButton) {
        // A plain click on a room selects it at once, as on the 2D map; anything else waits for the release, as a
        // drag from empty space pans and a Ctrl or Shift drag orbits
        const int roomId = event->modifiers() == Qt::NoModifier ? roomPickedAt(mPressPosition) : 0;
        if (roomId) {
            selectRoom(roomId, event->modifiers());
        } else {
            mClickPending = true;
        }
    }

    if (event->button() == Qt::LeftButton) { // translation on xy-plane
        auto eventPos = event->position().toPoint();
        const int x = eventPos.x();
        const int y = height() - eventPos.y(); // the opengl origin is at bottom left
        mPanMode = true;
        mPanXStart = x;
        mPanYStart = y;
    }

    // The 2D map raises this after the press has changed the selection, so a script handling it reads the new one;
    // here a right-click selects with the menu, on the release
    if (mClickPending || event->button() == Qt::RightButton) {
        mPressEventButton = event->button();
    } else {
        raiseMousePressEventMidPress();
    }
}

void ModernGLWidget::mouseMoveEvent(QMouseEvent* event)
{
    if (!mpMap || !mpMap->mpRoomDB) {
        return;
    }
    mPointerPosition = event->position().toPoint();
    mPointerInView = true;
    mPointerButtons = event->buttons();
    setHoveredRoom(roomUnderPointer());
    if (mClickPending && (event->position().toPoint() - mPressPosition).manhattanLength() >= QApplication::startDragDistance()) {
        mClickPending = false;
        if (mPressEventButton == Qt::LeftButton) {
            mPressEventButton = Qt::NoButton;
            raiseMousePressEventMidPress();
        }
    }
    if ((mMovingRooms || mRectangleSelecting) && !event->buttons().testFlag(Qt::LeftButton)) {
        endEditGesture();
        update();
    }
    // The release went elsewhere; with mouse tracking on, a move with no button down would pan on
    if (mPanMode && !event->buttons().testFlag(Qt::LeftButton)) {
        endPan();
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

void ModernGLWidget::endPan()
{
    if (!std::exchange(mPanMode, false)) {
        return;
    }
    mCameraController.snapTargetToGrid();
    const QVector3D newCenter = mCameraController.getTarget();
    mMapCenterX = static_cast<int>(newCenter.x());
    mMapCenterY = static_cast<int>(newCenter.y());
    mMapCenterZ = static_cast<int>(newCenter.z());
    update();
}

void ModernGLWidget::mouseReleaseEvent(QMouseEvent* event)
{
    mPointerButtons = event->buttons();
    if (event->button() == Qt::LeftButton && (mMovingRooms || mRectangleSelecting)) {
        endEditGesture();
        QOpenGLWidget::mouseReleaseEvent(event);
        update();
        return;
    }
    endPan();
    if (event->button() == Qt::LeftButton && mClickPending) {
        mClickPending = false;
        selectRoom(roomPickedAt(event->position().toPoint()), event->modifiers());
    } else if (event->button() == Qt::RightButton && mpMap && mpMap->mpRoomDB) {
        showContextMenu(event->position().toPoint());
    }
    if (event->button() == mPressEventButton) {
        raisePendingMousePressEvent();
    }
    QOpenGLWidget::mouseReleaseEvent(event);
    update();
}

void ModernGLWidget::mouseDoubleClickEvent(QMouseEvent* event)
{
    // Qt sends this in place of the second press, and the release after it must not select again
    mClickPending = false;
    if (event->button() == Qt::LeftButton && compassRect().contains(event->position().toPoint())) {
        turnNorthUp();
        event->accept();
        return;
    }
    if (event->button() != Qt::LeftButton || !mp2DMap || !mpMap || !mpMap->mpRoomDB) {
        QOpenGLWidget::mouseDoubleClickEvent(event);
        return;
    }
    if (const int roomId = destinationAt(event->position().toPoint())) {
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
    const int roomId = roomPickedAt(position);
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
    //: 3D map view context menu item: centers and zooms the view to show the whole area
    auto* fitAreaAction = new QAction(tr("Fit area"), menu);
    connect(fitAreaAction, &QAction::triggered, this, &ModernGLWidget::fitArea);
    QAction* firstAction = menu->actions().value(0);
    menu->insertAction(firstAction, editIn2D);
    menu->insertAction(firstAction, fitAreaAction);
    if (firstAction) {
        menu->insertSeparator(firstAction);
    }
    connect(menu, &QObject::destroyed, this, qOverload<>(&QWidget::update));
    update();
    menu->popup(mapToGlobal(position));
}

void ModernGLWidget::leaveEvent(QEvent* event)
{
    // A press still on here with no button down lost its release to wherever the grab went, such as a popup; a
    // Leave while the button is held, mid-drag, must not end the drag
    if (QGuiApplication::mouseButtons() == Qt::NoButton) {
        endLostPress();
    }
    mPointerInView = false;
    mPointerButtons = Qt::NoButton;
    setHoveredRoom(0);
    QOpenGLWidget::leaveEvent(event);
}

int ModernGLWidget::roomUnderPointer()
{
    if (!mPointerInView || mPointerButtons != Qt::NoButton || compassRect().contains(mPointerPosition)) {
        return 0;
    }
    return roomPickedAt(mPointerPosition);
}

void ModernGLWidget::setHoveredRoom(const int roomId)
{
    if (roomId == mHoveredRoomId) {
        return;
    }
    mHoveredRoomId = roomId;
    update();
}

void ModernGLWidget::fitArea()
{
    const TArea* pArea = mpMap && mpMap->mpRoomDB ? mpMap->mpRoomDB->getArea(mAID) : nullptr;
    if (!pArea) {
        return;
    }
    QRectF extent;
    // Each room's cell on its own level, as the levels shown above and below stand out from the viewed one
    QVector<QVector3D> corners;
    for (const int roomId : pArea->getAreaRooms()) {
        const TRoom* pR = mpMap->mpRoomDB->getRoom(roomId);
        if (pR && !pR->isHidden() && levelShown(pR->z())) {
            const QRectF cell(pR->x() - 0.5, pR->y() - 0.5, 1.0, 1.0);
            extent = extent.united(cell);
            for (const QPointF& corner : {cell.topLeft(), cell.topRight(), cell.bottomLeft(), cell.bottomRight()}) {
                corners.append(QVector3D(static_cast<float>(corner.x()), static_cast<float>(corner.y()), static_cast<float>(pR->z())));
            }
        }
    }
    if (extent.isEmpty()) {
        return;
    }
    const int centerX = qRound(extent.center().x());
    const int centerY = qRound(extent.center().y());
    const QVector3D target(static_cast<float>(centerX), static_cast<float>(centerY), static_cast<float>(mMapCenterZ));
    CameraController camera = mCameraController;
    camera.setTarget(target.x(), target.y(), target.z());
    camera.setViewportSize(width(), height());
    const auto fits = [&](const float distance) {
        camera.setScale(distance);
        camera.updateMatrices();
        for (const QVector3D& corner : std::as_const(corners)) {
            const std::optional<QPointF> pixel = camera.pixelForPoint(corner);
            if (!pixel || std::abs(pixel->x() - width() / 2.0) > 0.45 * width() || std::abs(pixel->y() - height() / 2.0) > 0.45 * height()) {
                return false;
            }
        }
        return true;
    };
    // Searched for, as a level near the camera grows on screen faster than the distance shrinks it, so
    // scaling the distance by how far the box overflows can swing ever wider instead of settling
    float closest = CameraController::scmMinDistance;
    float distance = CameraController::scmMaxDistance;
    if (width() <= 0 || height() <= 0) {
        distance = CameraController::distanceToFit(static_cast<float>(extent.width()), static_cast<float>(extent.height()), 1.0f);
    } else if (fits(closest)) {
        distance = closest;
    } else {
        for (int pass = 0; pass < 32 && distance - closest > 1e-4f; ++pass) {
            const float middle = (closest + distance) / 2.0f;
            (fits(middle) ? distance : closest) = middle;
        }
    }
    mCameraController.setScale(distance);
    mFrameWhenShown = false;
    mDistanceRequested = true;
    setViewCenter(mAID, centerX, centerY, mMapCenterZ);
    emitCameraControls();
    update();
}

QRect ModernGLWidget::compassRect() const
{
    if (cameraFixedByGridMode()) {
        return QRect();
    }
    constexpr int diameter = 44;
    constexpr int margin = 12;
    // A view too small to spare the room for it has none
    if (width() < 3 * diameter || height() < 3 * diameter) {
        return QRect();
    }
    return QRect(width() - margin - diameter, height() - margin - diameter, diameter, diameter);
}

void ModernGLWidget::turnNorthUp()
{
    const QVector3D position = mCameraController.getPosition();
    mCameraController.setOrientation(position.y(), CameraController::scmNorthUpAzimuth, 0.0f);
    emitCameraControls();
    update();
}

void ModernGLWidget::paintCompass(QPainter& painter)
{
    const QRect area = compassRect();
    if (area.isEmpty()) {
        return;
    }
    // North as the screen shows it, from the view's center
    const QVector3D target = mCameraController.getTarget();
    const std::optional<QPointF> from = mCameraController.pixelForPoint(target);
    const std::optional<QPointF> to = mCameraController.pixelForPoint(target + QVector3D(0.0f, 1.0f, 0.0f));
    QLineF north(QPointF(), QPointF(0.0, -1.0));
    if (from && to && QLineF(*from, *to).length() > 1e-3) {
        north = QLineF(*from, *to);
    }
    const qreal angle = QLineF(QPointF(), QPointF(0.0, -1.0)).angleTo(north);

    const MapStyle& style = mapStyle();
    const QColor contrast = ExitAppearance::contrastColor(backgroundColor());
    QColor face = backgroundColor();
    face.setAlpha(200);
    if (style.compassFace.isValid()) {
        face = style.compassFace;
    }
    const QColor rim = style.compassRim.isValid() ? style.compassRim : contrast;
    painter.save();
    painter.setRenderHint(QPainter::Antialiasing);
    painter.translate(QRectF(area).center());
    const qreal radius = area.width() / 2.0;
    painter.setPen(QPen(rim, 1.5));
    painter.setBrush(face);
    painter.drawEllipse(QPointF(), radius, radius);
    // QLineF angles run counterclockwise, QPainter's rotation clockwise
    painter.rotate(-angle);
    const qreal needle = radius * 0.7;
    const qreal halfWidth = radius * 0.22;
    painter.setPen(Qt::NoPen);
    painter.setBrush(style.compassNeedle.isValid() ? style.compassNeedle : QColor(220, 40, 40));
    painter.drawPolygon(QPolygonF({QPointF(0.0, -needle), QPointF(halfWidth, 0.0), QPointF(-halfWidth, 0.0)}));
    painter.setBrush(rim);
    painter.drawPolygon(QPolygonF({QPointF(0.0, needle), QPointF(halfWidth, 0.0), QPointF(-halfWidth, 0.0)}));
    painter.restore();
}

void ModernGLWidget::renderPlayerBeacon()
{
    if (!mpMap || !mpMap->mpRoomDB || !mpHost) {
        return;
    }
    const TRoom* pR = mpMap->mpRoomDB->getRoom(mpMap->mRoomIdHash.value(mpMap->mProfileName));
    if (!pR || pR->getArea() != mAID || pR->isHidden() || !levelShown(pR->z())) {
        return;
    }
    const QColor color = playerMarkerColor();
    const float roomSize = 2.0f / scale;
    const float roomHeight = roomSize / zFlattening;
    const float width = roomSize * 0.3f;
    const QVector3D base(static_cast<float>(pR->x()), static_cast<float>(pR->y()), static_cast<float>(pR->z()) + roomHeight / 2.0f);
    // Fading as it rises, past the level above
    constexpr int segments = 8;
    constexpr float columnHeight = 1.4f;
    const auto column = [&](const float opacity) {
        QVector<CubeInstanceData> bars;
        for (int i = 0; i < segments; ++i) {
            const QVector3D from = base + QVector3D(0.0f, 0.0f, columnHeight * i / segments);
            const QVector3D to = base + QVector3D(0.0f, 0.0f, columnHeight * (i + 1) / segments);
            const float alpha = opacity * (1.0f - static_cast<float>(i) / segments);
            bars.append(CubeInstanceData(GeometryManager::barTransform(from, to, width, width), color.redF(), color.greenF(), color.blueF(), alpha));
        }
        return bars;
    };
    mRenderCommandQueue.addCommand(std::make_unique<GLStateCommand>(GLStateCommand::DISABLE_DEPTH_WRITE));
    mRenderCommandQueue.addCommand(std::make_unique<RenderInstancedCubesCommand>(
            column(0.8f), mCameraController.getProjectionMatrix(), mCameraController.getViewMatrix(), mCameraController.getModelMatrix(), RenderCommand::Shading::Unlit));
    // Again, faintly, through whatever hides it: the floors above, most often
    mRenderCommandQueue.addCommand(std::make_unique<GLStateCommand>(GLStateCommand::DISABLE_DEPTH_TEST));
    mRenderCommandQueue.addCommand(std::make_unique<RenderInstancedCubesCommand>(
            column(0.3f), mCameraController.getProjectionMatrix(), mCameraController.getViewMatrix(), mCameraController.getModelMatrix(), RenderCommand::Shading::Unlit));
    mRenderCommandQueue.addCommand(std::make_unique<GLStateCommand>(GLStateCommand::ENABLE_DEPTH_TEST));
    mRenderCommandQueue.addCommand(std::make_unique<GLStateCommand>(GLStateCommand::ENABLE_DEPTH_WRITE));
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

namespace {
// Focus that a window switch, a menu or the menu bar took away comes back with the same reason
bool focusReturnsWithReason(const Qt::FocusReason reason)
{
    return reason == Qt::ActiveWindowFocusReason || reason == Qt::PopupFocusReason || reason == Qt::MenuBarFocusReason;
}
} // namespace

void ModernGLWidget::focusInEvent(QFocusEvent* event)
{
    // After a click the keys typed next are a command, as with the 2D map, which never takes focus
    if (!focusReturnsWithReason(event->reason())) {
        mKeysNavigate = event->reason() == Qt::TabFocusReason || event->reason() == Qt::BacktabFocusReason;
    }
    QOpenGLWidget::focusInEvent(event);
}

void ModernGLWidget::focusOutEvent(QFocusEvent* event)
{
    if (!focusReturnsWithReason(event->reason())) {
        mKeysNavigate = false;
    }
    QOpenGLWidget::focusOutEvent(event);
}

namespace {
constexpr float scmKeyTurnDegrees = 15.0f;
} // namespace

void ModernGLWidget::keyPressEvent(QKeyEvent* event)
{
    // A modifier pressed on its own waits for the key it goes with, such as Shift+Tab or the Shift for +; handed on
    // first, it would take focus to the command line ahead of that key
    if (mKeysNavigate) {
        switch (event->key()) {
        case Qt::Key_Shift:
        case Qt::Key_Control:
        case Qt::Key_Alt:
        case Qt::Key_AltGr:
        case Qt::Key_Meta:
            return;
        default:
            break;
        }
    }
    // Shift is needed for + on many layouts; keypad keys are left to key bindings on the command line
    Qt::KeyboardModifiers otherModifiers = Qt::ControlModifier | Qt::AltModifier | Qt::MetaModifier | Qt::KeypadModifier;
#if defined(Q_OS_MACOS)
    // Where Qt marks every arrow key as on the keypad
    if (event->key() >= Qt::Key_Left && event->key() <= Qt::Key_Down) {
        otherModifiers &= ~Qt::KeypadModifier;
    }
#endif
    const bool navigates = mKeysNavigate && !(event->modifiers() & otherModifiers);
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
        case Qt::Key_F:
            fitArea();
            return;
        case Qt::Key_Q:
            turnCamera(-scmKeyTurnDegrees);
            return;
        case Qt::Key_E:
            turnCamera(scmKeyTurnDegrees);
            return;
        case Qt::Key_PageUp:
            slot_shiftZup();
            return;
        case Qt::Key_PageDown:
            slot_shiftZdown();
            return;
        default:
            break;
        }
    }

    if (TCommandLine* pCommandLine = commandLine()) {
        // A floating map is a window of its own, and focus in an inactive window waits for it to be activated
        pCommandLine->activateWindow();
        pCommandLine->setFocus(Qt::OtherFocusReason);
        QCoreApplication::sendEvent(pCommandLine, event);
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

void ModernGLWidget::queueScene()
{
    // First, as on the 2D map, where the other levels, the labels and everything else paint over it
    queueGrid();
    renderConnections();
    // Labels beneath the rooms come before them without writing depth, so that the translucent rooms
    // of other levels show over them rather than cutting holes in them
    renderLabels(false, std::numeric_limits<int>::lowest(), std::numeric_limits<int>::max());
    renderRooms();
    renderPlayerBeacon();
}

void ModernGLWidget::renderLabels(const bool onTop, const int lowestLevel, const int highestLevel)
{
    if (!mpMap || !mpMap->mpRoomDB) {
        return;
    }

    TArea* pArea = mpMap->mpRoomDB->getArea(mAID);
    if (!pArea || pArea->mMapLabels.isEmpty()) {
        return;
    }
    // Other areas' are kept for going back to them; those and deleted labels go once there are many more than this area has
    if (mThemedLabels.size() > std::max<qsizetype>(scmThemedLabelsKept, 2 * pArea->mMapLabels.size())) {
        mThemedLabels.removeIf([this, pArea](const auto& entry) {
            return entry.key().first != mAID || !pArea->mMapLabels.contains(entry.key().second);
        });
    }

    const float roomHeight = 2.0f / scale / zFlattening;
    const bool moreTransparent = mpHost->experimentEnabled(qsl("experiment.rendering.more-transparent"));
    const MapStyle& style = mapStyle();
    const QVector3D screenRight = mCameraController.screenRight();
    const QVector3D screenUp = mCameraController.screenUp();
    // Turned around while the top of the screen faces south, so that it never reads upside down
    const bool turned = GeometryManager::groundAxes(screenRight, screenUp).second.y() < 0.0f;

    // Those on top are queued by renderRooms(), which has turned depth writes off by then
    if (onTop) {
        mRenderCommandQueue.addCommand(std::make_unique<GLStateCommand>(GLStateCommand::ENABLE_POLYGON_OFFSET));
    } else {
        mRenderCommandQueue.addCommand(std::make_unique<GLStateCommand>(GLStateCommand::DISABLE_DEPTH_WRITE));
    }

    for (auto it = pArea->mMapLabels.constBegin(); it != pArea->mMapLabels.constEnd(); ++it) {
        const TMapLabel& label = it.value();
        if (label.showOnTop != onTop || label.pix.isNull()) {
            continue;
        }

        const std::optional<int> level = GeometryManager::labelLevel(label.pos.z());
        if (!level || *level < lowestLevel || *level > highestLevel) {
            continue;
        }
        const qint64 levelsAbove = qint64{*level} - mMapCenterZ;
        if (levelsAbove > mShowTopLevels || -levelsAbove > mShowBottomLevels) {
            continue;
        }

        const QPixmap* themed = themedLabel(it.key(), label, style);
        const QPixmap& pixmap = themed ? *themed : label.pix;
        const GLuint textureId = mLabelTextureCache.getTexture(mAID, it.key(), pixmap);
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
            const QSizeF pixels = pixmap.deviceIndependentSize();
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
        QVector4D tint = GeometryManager::fadedDecalTint(GeometryManager::levelFade(static_cast<int>(levelsAbove), moreTransparent));
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
    } else {
        mRenderCommandQueue.addCommand(std::make_unique<GLStateCommand>(GLStateCommand::ENABLE_DEPTH_WRITE));
    }
}

const QPixmap* ModernGLWidget::themedLabel(const int labelId, const TMapLabel& label, const MapStyle& style)
{
    if (!style.themedMapLabels) {
        mThemedLabels.clear();
        return nullptr;
    }
    ThemedLabel& themed = mThemedLabels[{mAID, labelId}];
    if (themed.sourceKey != label.pix.cacheKey() || themed.styleId != style.id) {
        const QImage image = MapDecals::themedLabel(label, style);
        themed.sourceKey = label.pix.cacheKey();
        themed.styleId = style.id;
        themed.pixmap = image.isNull() ? QPixmap() : QPixmap::fromImage(image);
    }
    return themed.pixmap.isNull() ? nullptr : &themed.pixmap;
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
