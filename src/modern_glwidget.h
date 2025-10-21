#ifndef MUDLET_MODERN_GLWIDGET_H
#define MUDLET_MODERN_GLWIDGET_H

/***************************************************************************
 *   Copyright (C) 2010-2011 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2016, 2020-2021 by Stephen Lyons                        *
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

// (2 of 2) This must be included before any Qt library tries to include
// windows.h which pulls in winsock.h to avoid (multiple):
// "#warning Please include winsock2.h before windows.h [-Wcpp]" warnings
#if defined(INCLUDE_WINSOCK2)
#include <winsock2.h>
#endif

#include <QElapsedTimer>
#include <QMatrix4x4>
#include <QVector3D>
#include <QQuaternion>
#include <QOpenGLBuffer>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLWidget>
#include <QPointer>
#include <QTimer>
#include <QEasingCurve>

#include "GeometryManager.h"
#include "RenderCommandQueue.h"
#include "ResourceManager.h"
#include "ShaderManager.h"
#include "CameraController.h"

class Host;
class TMap;
class TRoom;
struct MapInfoProperties;

class ModernGLWidget : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT

public:
    Q_DISABLE_COPY(ModernGLWidget)
    ModernGLWidget(TMap*, Host*, QWidget* parent = nullptr);
    ~ModernGLWidget() override;

    void wheelEvent(QWheelEvent* e) override;
    void setViewCenter(int, int, int, int);
    void shiftCamera(float, float, float);
    void setCameraPosition(float, float, float);

    QSize minimumSizeHint() const override;
    QSize sizeHint() const override;

public slots:
    void slot_shiftUp();
    void slot_shiftDown();
    void slot_shiftLeft();
    void slot_shiftRight();
    void slot_shiftCameraUp();
    void slot_shiftCameraDown();
    void slot_shiftCameraLeft();
    void slot_shiftCameraRight();
    void slot_shiftZup();
    void slot_shiftZdown();
    void slot_setCameraPositionX(int angle);
    void slot_setCameraPositionY(int angle);
    void slot_setCameraPositionZ(int angle);
    void slot_setScale(int);
    void slot_showAllLevels();
    void slot_singleLevelView();
    void slot_showMoreUpperLevels();
    void slot_showLessUpperLevels();
    void slot_showMoreLowerLevels();
    void slot_showLessLowerLevels();
    void slot_defaultView();
    void slot_sideView();
    void slot_topView();

private slots:
    void onCameraAnimationTick();

protected:
    void initializeGL() override;
    void paintGL() override;
    void resizeGL(int width, int height) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

public:
    TMap* mpMap = nullptr;

private:
    // Shader and rendering resources
    ShaderManager mShaderManager;
    QOpenGLBuffer mVertexBuffer;
    QOpenGLBuffer mColorBuffer;
    QOpenGLBuffer mNormalBuffer;
    QOpenGLBuffer mIndexBuffer;
    QOpenGLBuffer mInstanceBuffer;
    QOpenGLVertexArrayObject mVAO;

    // Geometry management
    GeometryManager mGeometryManager;

    // Render command queue
    RenderCommandQueue mRenderCommandQueue;

    // Resource management
    ResourceManager mResourceManager;

    // Camera management
    CameraController mCameraController;

    // Host reference
    QPointer<Host> mpHost;

    // View state
    bool is2DView = false;
    bool mPanMode = false;
    float mPanXStart = 0;
    float mPanYStart = 0;
    float zmax = 9999999.0;
    float zmin = 9999999.0;

    // Map state
    int mRID = 0;
    int mAID = 0;
    int mMapCenterX = 0;
    int mMapCenterY = 0;
    int mMapCenterZ = 0;
    bool mShiftMode = false;
    int mFontHeight = 20;

    // Scales the size of rooms compared to the space between them - currently
    // hard coded to be a quarter (would be equivalent to a 2D room size setting
    // of "2.5"):
    float scale = 4;
    // Room size reduction factor on the z-axis (stacks with scale -> 1/32)
    float zFlattening = 8;
    int mShowTopLevels = 999999;
    int mShowBottomLevels = 999999;
    int mTargetRoomId = 0;

    // Frame timing for benchmarking
    QElapsedTimer mFrameTimer;

    // Smooth camera animation
    QTimer* mCameraAnimationTimer = nullptr;
    int mTargetAID = 0;
    float mTargetMapCenterX = 0.0f;
    float mTargetMapCenterY = 0.0f;
    float mTargetMapCenterZ = 0.0f;
    float mStartMapCenterX = 0.0f;
    float mStartMapCenterY = 0.0f;
    float mStartMapCenterZ = 0.0f;
    float mCurrentAnimationX = 0.0f; // Floating point current position during animation
    float mCurrentAnimationY = 0.0f;
    float mCurrentAnimationZ = 0.0f;
    qreal mAnimationProgress = 0.0;
    int mAnimationDuration = 100; // 100ms animation duration for smooth movement
    QEasingCurve mEasingCurve;
    bool mCameraSmoothAnimating = false; // Dedicated flag for smooth camera animation
    int mPreviousRID = 0; // Track previous room ID to detect changes
    int mPreviousAID = 0; // Track previous area ID to detect area changes

    // Private methods for modern OpenGL
    void updateMatrices();
    void renderRooms();
    void renderConnections();
    void renderCube(float x, float y, float z, float size, float r, float g, float b, float a);
    void renderLines(const QVector<float>& vertices, const QVector<float>& colors);
    void renderTriangles(const QVector<float>& vertices, const QVector<float>& colors);
    void renderUpDownIndicators(TRoom* pRoom, float x, float y, float z);
    void renderInOutIndicators(TRoom* pRoom, float x, float y, float z);
    void renderText(const QString& text, float x, float y);
    void setupBuffers();
    void cleanup();
    QColor getPlaneColor(int zLevel, bool belowOrAtLevel);
    QColor getEnvironmentColor(TRoom* pRoom);
    void startSmoothTransition(int targetAID, int targetX, int targetY, int targetZ);
};

#endif // MUDLET_MODERN_GLWIDGET_H
