#ifndef MUDLET_GLWIDGET_H
#define MUDLET_GLWIDGET_H

/***************************************************************************
 *   Copyright (C) 2010-2011 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2016, 2020-2021 by Stephen Lyons                        *
 *                                               - slysven@virginmedia.com *
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

#include "pre_guard.h"
#include <QElapsedTimer>
#include <QOpenGLWidget>
#include <QPointer>
#include "post_guard.h"

class Host;
class TMap;


class GLWidget : public QOpenGLWidget
{
    Q_OBJECT

public:
    Q_DISABLE_COPY(GLWidget)
    GLWidget(TMap*, Host*, QWidget* parent = nullptr);
    ~GLWidget() = default;

    void wheelEvent(QWheelEvent* e) override;
    void setViewCenter(int, int, int, int);

    QSize minimumSizeHint() const override;
    QSize sizeHint() const override;

public slots:
    void slot_shiftUp();
    void slot_shiftDown();
    void slot_shiftLeft();
    void slot_shiftRight();
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
    QPointer<Host> mpHost;
    bool is2DView = false;
    bool mPanMode = false;
    float mPanXStart = 0;
    float mPanYStart = 0;
    float zmax = 9999999.0;
    float zmin = 9999999.0;

    int mRID = 0;
    int mAID = 0;
    int mMapCenterX = 0;
    int mMapCenterY = 0;
    int mMapCenterZ = 0;
    bool mShiftMode = false;

    float xRot = 1.0;
    float yRot = 5.0;
    float zRot = 10.0;
    // Scales the size of rooms compared to the space between them - currently
    // hard coded to be a quarter (would be equivalent to a 2D room size setting
    // of "2.5"):
    float scale = 4;
    int mShowTopLevels = 999999;
    int mShowBottomLevels = 999999;

    float mScale = 1.0;
    int mTargetRoomId = 0;
    
    // Frame timing for benchmarking
    QElapsedTimer mFrameTimer;
};

#endif // MUDLET_GLWIDGET_H
